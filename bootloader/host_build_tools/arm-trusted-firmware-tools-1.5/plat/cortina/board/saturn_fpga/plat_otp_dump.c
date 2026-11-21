#ifdef CORTINA_OTP_SUPPORT
#include <debug.h>
#include <assert.h>
#include <string.h>
#include <cortina_otp.h>
#include INC_CORTINA_PLATFORM_OTP_DATA_HEADER

#ifdef CORTINA_OTP_DUMP
static otp_item_desc_t otp_descs[] = {
    OTP_DATA_DESC
};

#define OTP_DATA_MAX_LEN   512

void plat_otp_item_dump()
{
	CORTINA_OTP_DRV_t * pOtpDrv;
    uint32_t  i = 0;
    uint32_t  j = 0;
	uint8_t   buf[OTP_DATA_MAX_LEN/8];

	pOtpDrv = cortina_otp_get_drv_instance(0);
	assert(pOtpDrv != NULL);
    
    /* find the max len of the OTP field */
    for(i = 0; i < sizeof(otp_descs)/sizeof(otp_descs[0]); i++){
        if(otp_descs[i].len > OTP_DATA_MAX_LEN){
            NOTICE("OTP dump failed,the field %s'slen is greater than %d \n",
                    otp_descs[i].name,
                    OTP_DATA_MAX_LEN
                    );
            return;
        }
    }

    for(i = 0; i < sizeof(otp_descs)/sizeof(otp_descs[0]); i++){
        memset(buf,0x00,sizeof(buf));

        cortina_otp_shadow_get_field(pOtpDrv, 
                (uint32_t)  otp_descs[i].pos, 
                (uint32_t)  otp_descs[i].len,
                (uintptr_t) buf);

        if(otp_descs[i].len <= 8){
            NOTICE("OTP %s : %x\n",
                    otp_descs[i].name,
                    buf[0]);
        }else{
            NOTICE("OTP %s :",
                    otp_descs[i].name);
            for(j = 0; j < otp_descs[i].len/8; j++){
                if(j % 8 == 0){
                    tf_printf("\n    ");
                }
                tf_printf("%x ",buf[j]);
            }

            tf_printf("\n");
        }
    }

}

#endif /* CORTINA_OTP_DUMP */
#endif /* CORTINA_OTP_SUPPORT */

