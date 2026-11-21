/*
 * Copyright (c) 2017, Cortina Access. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <assert.h>
#include <bl_common.h>
#include <console.h>
#include <ctype.h>
#include <debug.h>
#include <errno.h>
#include <mmio.h>
#include <partitions.h>
#include <platform.h>
#include <platform_def.h>
#include <string.h>
#include <ca827x_atu.h>
#include INC_CORTINA_SOC_REGS_HEADER
#include INC_PLAT_DEF_HEADER
#include INC_PLAT_PRIVATE_HEADER
#ifdef CORTINA_OTP_SUPPORT
#include <cortina_otp.h>
#include INC_CORTINA_PLATFORM_OTP_DATA_HEADER
#endif
#ifdef MIPS_R3K_TIMER
#include <r3k_timer.h> 
#endif /* MIPS_R3K_TIMER */

#include "bl1_plat_hgu.h"
#ifdef CORTINA_BOOT_HGU
uint8_t plat_saturn_in_hgu_mode()
{
    GLOBAL_STRAP_t global_strap;

    global_strap.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_STRAP));      

    if(global_strap.bf.chip_type == SATURN_CPU_TYPE_HGU){
        return 1;
    }

    return 0;
}

uint8_t plat_saturn_in_backup_boot()
{
    GLOBAL_STRAP_t global_strap;
    uint8_t programmed = 0;
#ifdef CORTINA_OTP_SUPPORT
    CORTINA_OTP_DRV_t * pOtpDrv = NULL;

    pOtpDrv = cortina_otp_get_drv_instance(0);
    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_programmed_BSTART,
            OTP_DATA_programmed_BLEN,
            (uintptr_t) &programmed);
#endif /* CORTINA_OTP_SUPPORT */

    global_strap.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_STRAP));      

    if((global_strap.bf.chip_type == SATURN_CPU_TYPE_BACKUP 
                ||(global_strap.bf.jtag_osc_2 == 1
                   && global_strap.bf.jtag_osc_3 == 1))
            && programmed == 0){
        return 1;
    }

    return 0;
}

void plat_saturn_uart1_enable()
{
    GLOBAL_PIN_MUX_t pin_mux;
    GLOBAL_GPIO_MUX_0_t gpio_mux; 

    pin_mux.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_PIN_MUX));      

    pin_mux.bf.iomux_uart_1_enable = 1;

    mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_PIN_MUX),pin_mux.wrd);

    gpio_mux.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_GPIO_MUX_0)); 
    gpio_mux.bf.gpio0en |= 0x3;
    mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_GPIO_MUX_0),gpio_mux.wrd);
}

/* Reset EAXI ATU HW 
 *
 * Reset EAXI ATU register to stock power on defaults
 *
 * Return: 
 * 0 - OK
 * 1 - ERROR
 *
 */

int plat_saturn_eaxi_atu_xlat_reset()
{
	/* write defaults to all registers */
	mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_CTL), 
			EAXI_WRAPPER_EAXI_ATU_CTL_dft);
	mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA0), 
			EAXI_WRAPPER_EAXI_ATU_DATA0_dft);
	mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA1), 
			EAXI_WRAPPER_EAXI_ATU_DATA1_dft);
	mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA2), 
			EAXI_WRAPPER_EAXI_ATU_DATA2_dft);
	mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA3), 
			EAXI_WRAPPER_EAXI_ATU_DATA3_dft);
	mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA4), 
			EAXI_WRAPPER_EAXI_ATU_DATA4_dft);
	mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA5), 
			EAXI_WRAPPER_EAXI_ATU_DATA5_dft);
	return 0;
}


/* Setup and Enable an EAXI ATU Tranlation Entry 
 *
 * Parameters:
 *  entryIdx - Which XLAT Entry to Set/Enable, Range of 0-7
 *  inputAddr - Input Address to Compare
 *  inputMask - Input Address Mask, used as a logical AND mask
 *  outputAddr - Output Tranlated Addr value
 *
 * Return: 
 * 0 - OK
 * 1 - ERROR
 *
 */

int plat_saturn_eaxi_atu_xlat_config(unsigned int entryIdx, uint16_t inputAddr, uint16_t inputMask, uint16_t outputAddr)
{
	/* setup EAXI ATU translations */
	EAXI_WRAPPER_EAXI_ATU_CTL_t eaxi_atu_ctl;
	EAXI_WRAPPER_EAXI_ATU_DATA0_t eaxi_atu_data0;
	EAXI_WRAPPER_EAXI_ATU_DATA1_t eaxi_atu_data1;
	EAXI_WRAPPER_EAXI_ATU_DATA2_t eaxi_atu_data2;
	EAXI_WRAPPER_EAXI_ATU_DATA3_t eaxi_atu_data3;
	EAXI_WRAPPER_EAXI_ATU_DATA4_t eaxi_atu_data4;
	EAXI_WRAPPER_EAXI_ATU_DATA5_t eaxi_atu_data5;

 	/* sanity tests */
	if(entryIdx >= 8)
	 return 1;

	eaxi_atu_ctl.wrd = mmio_read_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_CTL));

        eaxi_atu_data0.wrd = 0;
        eaxi_atu_data1.wrd = 0;
        eaxi_atu_data2.wrd = 0;
        eaxi_atu_data3.wrd = 0;
        eaxi_atu_data4.wrd = 0;
        eaxi_atu_data5.wrd = 0;

        eaxi_atu_ctl.bf.idx = entryIdx;
        eaxi_atu_ctl.bf.enable |= (1<<entryIdx);

	/* write CTL set for READ */
        eaxi_atu_ctl.bf.w = 0;
        eaxi_atu_ctl.bf.r = 1;
	mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_CTL),eaxi_atu_ctl.wrd);

        /* address input compare */
        eaxi_atu_data0.bf.addr_input_comp = inputAddr;
        mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA0),eaxi_atu_data0.wrd);

        /* address input mask */
        eaxi_atu_data1.bf.addr_input_mask = inputMask;
        mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA1),eaxi_atu_data1.wrd);

        /* address output xlat */
        eaxi_atu_data2.bf.addr_output_value = outputAddr;
        mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA2),eaxi_atu_data2.wrd);

        eaxi_atu_data3.bf.arprot = 0x1;
        eaxi_atu_data3.bf.awprot = 0x1;
        eaxi_atu_data3.bf.arqos = 0x0;
        eaxi_atu_data3.bf.awqos = 0x0;
        eaxi_atu_data3.bf.arcache = 0x0;
        eaxi_atu_data3.bf.awcache = 0x0;
        eaxi_atu_data3.bf.user_en = 0x0;
        eaxi_atu_data3.bf.prot_en = 0x1;
        eaxi_atu_data3.bf.cache_en = 0x0;
        eaxi_atu_data3.bf.qos_en = 0x0;
        mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA3),eaxi_atu_data3.wrd);

        eaxi_atu_data4.bf.aruser = 0x0;
        mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA4),eaxi_atu_data4.wrd);

        eaxi_atu_data5.bf.awuser = 0x0;
        mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_DATA5),eaxi_atu_data5.wrd);

	/* write CTL set for WRITE */
        eaxi_atu_ctl.bf.w = 1;
        eaxi_atu_ctl.bf.r = 0;
	mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_ATU_CTL),eaxi_atu_ctl.wrd);

	return 0;
}

void plat_saturn_eaxi_init()
{
	GLOBAL_BLOCK_RESET_t block_rst;
    	EAXI_WRAPPER_EAXI_EAXI_CTL0_t eaxi_ctrl0;
    	uint8_t reserved_a = 0;
#ifdef CORTINA_OTP_SUPPORT
    	CORTINA_OTP_DRV_t * pOtpDrv = NULL;

    	pOtpDrv = cortina_otp_get_drv_instance(0);
    	cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_reserved_a_BSTART,
            OTP_DATA_reserved_a_BLEN,
            (uintptr_t) &reserved_a);
#endif /* CORTINA_OTP_SUPPORT */

	BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_HGU_EAXI_MASTER_INIT_BEFORE, pOtpDrv);
	block_rst.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_BLOCK_RESET));

	/* assert RESET to EAXI */
	block_rst.bf.reset_eaxi = 1;
    	mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_BLOCK_RESET),block_rst.wrd);

	/* drive reset for 100 us */
    	udelay(100);

	/* deassert RESET to EAXI, i.e. take it out of reset */
	block_rst.bf.reset_eaxi = 0;
	mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_BLOCK_RESET),block_rst.wrd);

	/* wait 100 us for reset to complete */
    	udelay(100);

	if(reserved_a == 0)
	{
	 plat_saturn_eaxi_atu_xlat_reset();
	 plat_saturn_eaxi_atu_xlat_config(0, 0xf300, 0xfff0, 0xd000);
	 plat_saturn_eaxi_atu_xlat_config(1, 0xf310, 0xfff0, 0x4410);
	 plat_saturn_eaxi_atu_xlat_config(2, 0xf320, 0xfff0, 0x5220);
	 plat_saturn_eaxi_atu_xlat_config(3, 0xf380, 0xfff0, 0x4000);

	 /* wait 100 us for EAXI XLATs to complete */
    	 udelay(100);
	}

	/* enable EAXI controller */
    	eaxi_ctrl0.wrd = mmio_read_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_EAXI_CTL0));
    	eaxi_ctrl0.bf.enable = 1; 
    	mmio_write_32(KSEG1_ATU_XLAT(EAXI_WRAPPER_EAXI_EAXI_CTL0),eaxi_ctrl0.wrd);

	/* wait 100 us for EAXI enable to complete */
    	udelay(100);
	BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_HGU_EAXI_MASTER_AFTER, pOtpDrv);
}

void plat_saturn_eaxi_ack()
{
    GLOBAL_PIN_MUX_t pin_mux;
    GLOBAL_TAROKO_INTENABLE_t taroko_intenable;

    /* enable global pin mux interrupt output */
    pin_mux.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_PIN_MUX));      
    pin_mux.bf.iomux_intout_enable = 1;
    mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_PIN_MUX),pin_mux.wrd);

    /* enable taroko SW ext int0 */
    taroko_intenable.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_TAROKO_INTENABLE));
    taroko_intenable.bf.sw_ext_int0e = 1;
    mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_TAROKO_INTENABLE),taroko_intenable.wrd);

    /* wait to settle */
    udelay(5);

    /* send IPC message EAXI READY to G3 */
    saturn_ipc_msg_t msg;

    msg.wrd = 0; 
    msg.bf.msg_type = HGU_IPC_MSG_TYPE_EAXI_READY;
    msg.bf.msg_data = 0;
    msg.bf.irq_allow = 1; 
    msg.bf.irq_request = 1; 

    /* don't ack */
    msg.bf.irq_ack_allow = 1;
    msg.bf.irq_ack_clear = 0;
    mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_SOFTWARE4),msg.wrd );
}



void plat_saturn_boot_error(uint16_t code)
{
    saturn_ipc_msg_t msg;

    msg.wrd = 0; 
    msg.bf.msg_type = HGU_IPC_MSG_TYPE_BAD_MESSAGE;
    msg.bf.msg_data = code;
    msg.bf.irq_allow = 1; 
    msg.bf.irq_request = 1; 

    /* don't ack */
    msg.bf.irq_ack_allow = 1;
    msg.bf.irq_ack_clear = 0;
    mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_SOFTWARE4),msg.wrd );
}



uint8_t plat_saturn_setpc_jump()
{
    saturn_ipc_msg_t msg;
	typedef void  sram_func_t(void);
    sram_func_t * sram_func;

    msg.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_SOFTWARE3));      

    /* sanity test here - msg type should HGU_IPC_MSG_TYPE_SET_PC_AND_JUMP */
    if(msg.bf.msg_type != HGU_IPC_MSG_TYPE_SET_PC_AND_JUMP){
        WARN("BL1: Bad IPC message type %d \n",msg.bf.msg_type);
        plat_saturn_boot_error(HGU_IPC_BAD_MESSAGE_ID_INVALID_MESSAGE_TYPE);
        return 1;
    }

    /* sanity test here - Address should in KSEG0 and KSEG1 */
    if(msg.bf.msg_data >= (KSEG2 >> 8) 
            || msg.bf.msg_data < (KSEG0 >> 8)){
        WARN("BL1: Invalid jump address 0x%x \n",msg.bf.msg_data << 8);
        plat_saturn_boot_error(HGU_IPC_BAD_MESSAGE_ID_INVALID_PC_JUMP_ADDR);
        return 1;
    }
    
    NOTICE("BL1: Jump to address 0x%x \n",msg.bf.msg_data << 8);

    /* Post KSEG0 or KSEG1 address << 8 */
    sram_func =(sram_func_t*) ((msg.bf.msg_data << 8));

    /* clean and invalidate data cache to commit any dirty data
       to SRAM, and throw away any cached non-dirty data 
       we in our data cache to force reading directly from
       SRAM all data that G3 has written */

    /* DANGER!!! G3 should not write any data to the SRAM that we
       we may have written to with to our data cache, else this operation
       will overwrite it and we will likely crash. Our cached SRAM region
       is at the very end of SRAM. Therefore, it is advised that G3 should
       write its image to the very beginning of SRAM to avoid a crash. */

    flush_dcache_all();

    /* invalidate instruction cache before jumping to force reading
       of instructions from SRAM that G3 has written */
    inv_icache_all();

    sram_func();

    /* we should never reach this point unless we return from loaded code */
    return 0;

}

void plat_saturn_hgu_boot_main()
{
    GLOBAL_SW_INTERRUPT_0_t sw_int; 

    plat_saturn_eaxi_ack();
    
    while(1){
        sw_int.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_SW_INTERRUPT_0));      
        
        if(sw_int.bf.sw_int == 0){
            udelay(1);
            continue;
        }
    
        if(plat_saturn_setpc_jump() != 0){
            udelay(1);
            continue;
        }

        break;
    }

}

#endif /* CORTINA_BOOT_HGU */
