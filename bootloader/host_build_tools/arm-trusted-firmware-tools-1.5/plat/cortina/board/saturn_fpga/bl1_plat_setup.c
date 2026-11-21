/*
 * Copyright (c) 2015-2016, Cortina Access. All rights reserved.
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
#include "../../bl1/bl1_private.h"
#include <ca827x_atu.h>
#include INC_CORTINA_SOC_REGS_HEADER
#include INC_PLAT_DEF_HEADER
#include INC_PLAT_PRIVATE_HEADER
#include "plat_io_storage.h"
#include "partitions.h"
#ifdef CORTINA_PER_WATCHDOG
#include <cortina_per_watchdog.h>
#endif /* CORTINA_PER_WATCHDOG */
#ifdef CORTINA_SECURE_SRAM
#include <cortina_secure_sram.h>
#endif /* CORTINA_SECURE_SRAM */
#ifdef CORTINA_OTP_SUPPORT
#include <cortina_otp.h>
#include INC_CORTINA_PLATFORM_OTP_DATA_HEADER
#ifdef CORTINA_SECURE_JTAG_SUPPORT
#include <cortina_secure_jtag.h>
#endif /* CORTINA_SECURE_JTAG_SUPPORT */
#ifdef LIB_GF_SCRAMBLE
#include <lib_gf_scramble.h>
#endif /* LIB_GF_SCRAMBLE */
#endif /* CORTINA_OTP_SUPPORT */
#ifdef CORTINA_BOOT_EMMC
#include <dw_mmc.h>
#include <emmc.h>
#endif /* CORTINA_BOOT_EMMC */

#ifdef MIPS_R3K_TIMER
#include <r3k_timer.h> 
#endif /* MIPS_R3K_TIMER */

#ifdef CORTINA_BOOT_HGU
#include "bl1_plat_hgu.h"
#endif /* CORTINA_BOOT_HGU */

/*******************************************************************************
 * Declarations of linker defined symbols which will help us find the layout
 * of trusted RAM
 ******************************************************************************/
extern unsigned long __COHERENT_RAM_START__;
extern unsigned long __COHERENT_RAM_END__;

/* global */
GLOBAL_STRAP_t saturn_global_strap;
GLOBAL_GLOBAL_CONFIG_t saturn_global_cfg;
FLASH_FLASH_MASK_t saturn_flash_flash_mask;
GLOBAL_PIN_MUX_t   saturn_global_pin_mux;
#ifdef CORTINA_OTP_SUPPORT
CORTINA_OTP_DRV_t * cortina_pOtpDrv;
#endif /* CORTINA_OTP_SUPPORT */

#if USE_COHERENT_MEM
/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols refer to
 * page-aligned addresses.
 */
#define BL1_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL1_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)
#endif

/* Data structure which holds the extents of the trusted RAM for BL1 */
static meminfo_t bl1_tzram_layout;
extern void bl_cli(void);

#ifdef CORTINA_OTP_DUMP
    void plat_otp_item_dump();
#endif
meminfo_t *bl1_plat_sec_mem_layout(void)
{
    return &bl1_tzram_layout;
}

#ifdef CORTINA_HOTFIX_CHECKPOINT_SUPPORT
/*
 * BL1 OTP Hotfix Checkpoint
 * This function will compare the checkpoint id from the caller with the hotfix tab entries.
 * If find a match,it will call a corrospoding hotfix code from OTP shadow.
 * The offset starts from the begining of OTP shadow base, that is, bit location at pOtpDrv->shadow_base_addr 
 * The offset is aligned with a multiple of 2**3 for hotfix code in OTP shadow .
 * Parameters:
 *    Checkpoint ID: range 1 to 30. should not be 0 or 31
 *    A pointer or a 32-bit data
 * Return:
 *    0 : if no hotfix code found
 *    1 : hotfix code found and executed
 */   

unsigned int cortina_bl1_hotfix_checkpoint(uint32_t checkpoint_id, void *data)
{
    CORTINA_OTP_DRV_t * pOtpDrv;
    uint32_t hotfix_addr = 0;
    uint32_t i;
    typedef void func(void *);
    func * hotfix_func;
    uint8_t id; 
    uint8_t offset;

    TEST_INFO("cortina_otp_bl1_hotfix_checkpoint In checkpoint_id %d data %p\n", checkpoint_id, data);
    TEST_INFO("hotfix checkpoint num entries %d\n", BL1_HOTFIX_CHECKPOINT_NUM_ENTRIES);

    pOtpDrv = cortina_otp_get_drv_instance(0);
    if(pOtpDrv == NULL)
    {
        ERROR("ERROR: No Otp Driver Handle, can't search for hotfix id %d\n",  checkpoint_id);
        return 0;
    }

    TEST_INFO("Searching for hotfix id %d\n", checkpoint_id);
    TEST_INFO("pOtpDrv->shadow_base_addr 0x%x\n", pOtpDrv->shadow_base_addr);
    
    for (i = 0; i < BL1_HOTFIX_CHECKPOINT_NUM_ENTRIES ; i++) {
        TEST_INFO("check hotfix_tab entry %d\n", i);
        id = 0; 
        offset = 0;
        cortina_otp_shadow_get_field(pOtpDrv,
                OTP_DATA_df6_BSTART + i * (BL1_HOTFIX_CHECKPOINT_ENTRY_BLEN) ,
                BL1_HOTFIX_CHECKPOINT_ID_BLEN,
                (uintptr_t) &id);

        cortina_otp_shadow_get_field(pOtpDrv,
                OTP_DATA_df6_BSTART + i * (BL1_HOTFIX_CHECKPOINT_ENTRY_BLEN) + BL1_HOTFIX_CHECKPOINT_ID_BLEN,
                BL1_HOTFIX_CHECKPOINT_OFFSET_BLEN,
                (uintptr_t) & offset);
        TEST_INFO(" id = %d offset = 0x%x \n",id,offset);
        if ((id == CHECKPOINT_ID_UNDEF) 
                || (id == CHECKPOINT_ID_INVALID)) {
            continue;
        }

        if (checkpoint_id == id) {
            TEST_INFO("Found matched checkpoint_id %d at hotfix_tab entry %d, offset 0x%x \n", checkpoint_id, i,offset);
            // offset is from the first free available OTP addr
            hotfix_addr = (uintptr_t) pOtpDrv->shadow_base_addr + (offset << 3); 
            hotfix_func = (func *) ((uintptr_t) hotfix_addr);
            TEST_INFO("Hotfix at %p to be called\n",  hotfix_func);
            hotfix_func(data);
            return 1; // found and executed
        }
    }
    TEST_INFO("No hotfix matches found for %d\n", checkpoint_id); 

    return 0;
}
#endif /* CORTINA_HOTFIX_CHECKPOINT_SUPPORT */

/*******************************************************************************
 * Perform any BL1 specific platform actions.
 ******************************************************************************/
void plat_saturn_bl1_early_platform_setup(void)
{
#ifdef CORTINA_OTP_SUPPORT
    CORTINA_OTP_DRV_t * pOtpDrv = NULL;
#endif /* CORTINA_OTP_SUPPORT */

    HWSIM_PROGRESS_MARK(0xffffffff);
    saturn_global_cfg.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_GLOBAL_CONFIG));
#ifdef CORTINA_PER_WATCHDOG
    CORTINA_PER_WATCHDOG_DRV_t * pWatchDogDrv;
    uint8_t secure_watchdog_enable = 0;
#endif /* CORTINA_PER_WATCHDOG */

    const size_t bl1_size = BL1_RAM_LIMIT - BL1_RAM_BASE;
    unsigned int otp_clock_mhz = plat_get_otp_clock_mhz();
    unsigned int fabric_clock_mhz = plat_get_fabric_clock_mhz();
    unsigned int cpu_clock_mhz = plat_get_cpu_clock_mhz();
    unsigned int io_clock_mhz  = plat_get_io_clock_mhz();
    unsigned int per_clock_mhz  = plat_get_per_clock_mhz();
    
#ifdef MIPS_R3K_TIMER
    r3k_timer_init(cpu_clock_mhz);
#endif
#ifdef CORTINA_BOOT_HGU
    if(plat_saturn_in_hgu_mode()){
        /* Initialize the console to provide early debug support */
        plat_saturn_uart1_enable();
        console_init(CORTINA_PER_UART1_BASE, per_clock_mhz * 1000000, CORTINA_PER_UART_BAUDRATE);
        NOTICE("BL1: Chip in HGU mode\n");
    }else
#endif /* CORTINA_BOOT_HGU */
    {
        /* Initialize the console to provide early debug support */
        console_init(CONSOLE_BASE, per_clock_mhz * 1000000, CORTINA_PER_UART_BAUDRATE);
    }
    HWSIM_PROGRESS_MARK(4);

    /* read global strap reg, and assign to global variable */
    saturn_global_strap.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_STRAP));      

    NOTICE("BL1: GLOBAL_STRAP 0x%x\n", saturn_global_strap.wrd);
    NOTICE("BL1: Fabric Clock Speed %d MHz\n", fabric_clock_mhz);
    NOTICE("BL1: OTP Clock Speed %d MHz\n", otp_clock_mhz);
    NOTICE("BL1: CPU Clock Speed %d MHz\n", cpu_clock_mhz);
    NOTICE("BL1: I/O Clock %d MHz\n", io_clock_mhz);
    NOTICE("BL1: PER Clock %d MHz\n", per_clock_mhz);

    /* disable flash interrupt propogation */
    saturn_flash_flash_mask.wrd = mmio_read_32(KSEG1_ATU_XLAT(FLASH_FLASH_MASK));
    saturn_flash_flash_mask.bf.flashRegIrqMask = 1;
    mmio_write_32(KSEG1_ATU_XLAT(FLASH_FLASH_MASK), saturn_flash_flash_mask.wrd);

	/* enable SPI interface for serial NOR and NAND */
    saturn_global_pin_mux.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_PIN_MUX));
    saturn_global_pin_mux.bf.iomux_spi_cs1_en= 1;
    mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_PIN_MUX), saturn_global_pin_mux.wrd);

#ifdef CORTINA_OTP_SUPPORT

    HWSIM_PROGRESS_MARK(27);
    pOtpDrv = cortina_otp_init(  (uintptr_t) OTP_BASE,
            (uint32_t) OTP_BIT_SIZE,
            (uintptr_t) OTP_SHADOW_BASE,
            (uint32_t) OTP_READ_TIMEOUT,
            (uint32_t) OTP_WRITE_TIMEOUT,
            (uint32_t) (otp_clock_mhz*2) /* clock passed to functions needs to be 2 times 
					  * OTP clock because called func. divides by 2 internally */
            );
    HWSIM_PROGRESS_MARK(1);
    if (pOtpDrv == NULL) {
        HWSIM_PROGRESS_MARK(2);
        INFO("Cortina OTP Driver Init FAILED\n");
        panic();
    }
    cortina_pOtpDrv = pOtpDrv;
#endif /* CORTINA_OTP_SUPPORT */

#ifdef CORTINA_BOOT_HGU
    /* init the console again when in backup mode,
     * because the backup mode depend on OTP */
    if(plat_saturn_in_backup_boot()){
        /* Initialize the console to provide early debug support */
        plat_saturn_uart1_enable();
        console_init(CORTINA_PER_UART1_BASE, per_clock_mhz * 1000000, CORTINA_PER_UART_BAUDRATE);
        NOTICE("BL1: Chip in Backup mode\n");
    }
#endif /* CORTINA_BOOT_HGU */

    BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_UART_INIT, pOtpDrv);

#ifdef CORTINA_PER_WATCHDOG
    cortina_otp_shadow_get_field(pOtpDrv,
            OTP_DATA_secure_watchdog_enable_BSTART,
            OTP_DATA_secure_watchdog_enable_BLEN,
            (uintptr_t) &secure_watchdog_enable);
    HWSIM_PROGRESS_MARK(5);
    /* initialized Watchdog Driver for 256 sec. period */
    /* Note: 2nd argument must be in Khz, not Hz */
    pWatchDogDrv = cortina_per_watchdog_init((uintptr_t) (KSEG1_ATU_XLAT(PER_WDT_CTRL)),
            (per_clock_mhz * 1000),
            256,
            CORTINA_PER_WATCHDOG_UNIT_SEC
            );

    HWSIM_PROGRESS_MARK(6);
    BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_WATHDOG, pOtpDrv);
    if(pWatchDogDrv == NULL){
        WARN("Watchdog driver init failed!!\n");
    }
    else if(secure_watchdog_enable != 0 
            && !plat_saturn_in_hgu_mode()
            && !plat_saturn_in_backup_boot())
    {
        TEST_INFO("Enable WDT reset in GLOBAL_CONFIG\n");
        /* Enable WDT reset in GLOBAL_CONFIG */
        saturn_global_cfg.bf.wd_reset_ext_reset = 1;
        saturn_global_cfg.bf.wd_reset_remap = 1;
        saturn_global_cfg.bf.wd_reset_all_blocks= 1;
        saturn_global_cfg.bf.wd_reset_subsys_enable = 1;
        mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_GLOBAL_CONFIG), saturn_global_cfg.wrd);

        /* enable system reset if watchdog expires */
        cortina_per_watchdog_enable_sys_reset(pWatchDogDrv);
    }
    HWSIM_PROGRESS_MARK(7);
#endif /* CORTINA_PER_WATCHDOG */

    /* Allow BL1 to see the whole Trusted RAM */
    bl1_tzram_layout.total_base = PLAT_SATURN_BL_RAM_BASE;
    bl1_tzram_layout.total_size = PLAT_SATURN_BL_RAM_SIZE;

    /* Calculate how much RAM BL1 is using and how much remains free */
    bl1_tzram_layout.free_base = PLAT_SATURN_BL_RAM_BASE;
    bl1_tzram_layout.free_size = PLAT_SATURN_BL_RAM_SIZE;
    HWSIM_PROGRESS_MARK(8);
    reserve_mem(&bl1_tzram_layout.free_base,
            &bl1_tzram_layout.free_size,
            BL1_RAM_BASE,
            bl1_size);
    HWSIM_PROGRESS_MARK(9);

    INFO("BL1: 0x%x - 0x%x [size = %d]\n", BL1_RAM_BASE, BL1_RAM_LIMIT,
         bl1_size);
    HWSIM_PROGRESS_MARK(28);

#ifdef CORTINA_BOOT_HGU
    if(plat_saturn_in_hgu_mode()){
        plat_saturn_eaxi_init();
    }
#endif /* CORTINA_BOOT_HGU */
}

void bl1_early_platform_setup(void)
{
    plat_saturn_bl1_early_platform_setup();
    HWSIM_PROGRESS_MARK(10);
}

/*******************************************************************************
 * Perform the very early platform specific architecture setup here. At the
 * moment this only does basic initialization. Later architectural setup
 * (bl1_arch_setup()) does not do anything platform specific.
 ******************************************************************************/
void bl1_plat_arch_setup(void)
{
#if CORTINA_MFG_CLI_SUPPORT
#ifdef CORTINA_OTP_SUPPORT
    uint8_t mfg_cli_enable = 0;
    uint8_t otp_programmed = 0;
    CORTINA_OTP_DRV_t * pOtpDrv;
    saturn_global_strap.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_STRAP));
    HWSIM_PROGRESS_MARK(33);
    HWSIM_PROGRESS_VAL(saturn_global_strap.wrd);

#ifdef CORTINA_BOOT_HGU
    if(plat_saturn_in_backup_boot()){
        bl_cli();
    }else
#endif /* CORTINA_BOOT_HGU */
    {
        pOtpDrv = cortina_otp_get_drv_instance(0);
        cortina_otp_shadow_get_field(
                pOtpDrv,
                OTP_DATA_mfg_cli_enable_BSTART,
                OTP_DATA_mfg_cli_enable_BLEN,
                (uintptr_t)&mfg_cli_enable);

        cortina_otp_shadow_get_field(
                pOtpDrv,
                OTP_DATA_programmed_BSTART,
                OTP_DATA_programmed_BLEN,
                (uintptr_t)&otp_programmed);

        /* Waffer test mode
         * Case#1: Blank OTP + jtag_osc
         * Case#2: OTP programmed with mfg_cli_enable
         */
        //saturn_global_strap.bf.jtag_osc_2 = 1; // temp force
        //saturn_global_strap.bf.jtag_osc_3 = 1; // temp force
        if(((otp_programmed == 0) && \
                    (saturn_global_strap.bf.jtag_osc_2==1) && \
                    (saturn_global_strap.bf.jtag_osc_3==1)) || \
                ((otp_programmed == 1) && \
                 (mfg_cli_enable == 1))) {
            HWSIM_PROGRESS_MARK(0x46);
            bl_cli();
        }
    }
#endif /* CORTINA_OTP_SUPPORT */
#endif /* CORTINA_MFG_CLI_SUPPORT */

#ifdef CORTINA_BOOT_HGU
    if(plat_saturn_in_hgu_mode()){
        plat_saturn_hgu_boot_main();
    }
#endif
}

/* TODO SD or EMMC Card init here */

static inline char hex2str(unsigned int data)
{
    data &= 0xf;
    if ((data >= 0) && (data <= 9))
        return (char)(data + 0x30);
    return (char)(data - 10 + 0x41);
}

#if 0
static uint32_t rand(unsigned int data)
{
    int32_t quotient, remainder, t;

    quotient = data / 127773;
    remainder = data % 127773;
    t = 16807 * remainder - 2836 * quotient;
    if (t <= 0)
        t += RANDOM_MAX;
    return (t % ((uint32_t)RANDOM_MAX + 1));
}
#endif


/*******************************************************************************
 * Function which will perform any remaining platform-specific setup that can
 * occur after the MMU and data cache have been enabled.
 ******************************************************************************/
void bl1_platform_setup(void)
{

#ifdef CORTINA_OTP_SUPPORT
    CORTINA_OTP_DRV_t * pOtpDrv = NULL;
#ifdef CORTINA_SECURE_JTAG_SUPPORT
    CORTINA_SECURE_JTAG_DRV_t * pSecureJTAGDrv;
    uint8_t jtag_password[CORTINA_SECURE_JTAG_PASSWORD_LEN];
    uint8_t jtag_password_inhibit = 0;
    uint8_t programmed = 0;
#ifdef LIB_GF_SCRAMBLE
    uint8_t scramble_seed;
    uint8_t scrambled_jtag_password = 0;
#endif /* LIB_GF_SCRAMBLE */
#endif /* CORTINA_SECURE_JTAG_SUPPORT */
#endif /* CORTINA_OTP_SUPPORT */

    HWSIM_PROGRESS_MARK(35);

#ifdef CORTINA_OTP_SUPPORT
    pOtpDrv = cortina_otp_get_drv_instance(0);
    if (pOtpDrv == NULL){
        INFO("Cortina OTP get_drv_instace 0 FAILED\n");
    }
#ifdef CORTINA_OTP_DUMP
    plat_otp_item_dump();
#endif
    cortina_otp_shadow_get_field(pOtpDrv, 
            OTP_DATA_jtag_password_inhibit_BSTART, 
            OTP_DATA_jtag_password_inhibit_BLEN, 
            (uintptr_t) &jtag_password_inhibit);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_programmed_BSTART,
            OTP_DATA_programmed_BLEN,
            (uintptr_t) &programmed);
#endif 

#ifdef CORTINA_SECURE_JTAG_SUPPORT

    TEST_INFO("GLOBAL_STRAP debug_mode %d\n", saturn_global_strap.bf.debug_mode);
    TEST_INFO("OTP fixed bitfield programmed %d\n", programmed);
    TEST_INFO("OTP fixed bitfield jtag_password_inhibit %d\n", jtag_password_inhibit);


    HWSIM_PROGRESS_MARK(40);
    if(
#if (CORTINA_OTP_SUPPORT == OTP_MODE_EM )
    /* if we:
     *  1. discover that the CPU running in debug_mode (i.e. non-secure) via pin strapping
     *    OR
     *  2. discover that the OTP programmed_n (neg. assert) bit is NOT cleared
         *    OR
     *  3. discover that the OTP bf.jtag_password_inhibit_n (neg. assert) is cleared in OTP 
     * then
     *  clear the secure JTAG password to all ZEROs
         */
        (saturn_global_strap.bf.debug_mode == 0x1) ||
        (programmed == 0x1) ||
        (jtag_password_inhibit == 0x0) 
#elif (CORTINA_OTP_SUPPORT == OTP_MODE_KP )
    /* if we:
     *  1. discover that the CPU running in debug_mode (i.e. non-secure) via pin strapping
     *    OR
     *  2. discover that the OTP programmed (pos. assert) bit is cleared
         *    OR
     *  3. discover that the OTP bf.jtag_password_inhibit (pos. assert) is set in OTP 
     * then
     *  clear the secure JTAG password to all ZEROs
         */
        (saturn_global_strap.bf.debug_mode == 0x1) ||
        (programmed == 0x0) ||
        (jtag_password_inhibit == 0x1) 
#else /* CORTINA_OTP_SUPPORT */
#error OTP_MODE_is_not_set_in_platform.mk
#endif /* CORTINA_OTP_SUPPORT */ 
        )
    {
        /* clear the password to all ZEROs and set it */
        memset(jtag_password,0x00,CORTINA_SECURE_JTAG_PASSWORD_LEN);
    }else{
        /* read password from shadow copy of OTP 
         * which is already byte aligned in shadow memory 
             */
#ifdef LIB_GF_SCRAMBLE
       /* check to see if JTAG password is scrambled */
        cortina_otp_shadow_get_field(pOtpDrv, 
                OTP_DATA_scrambled_jtag_password_BSTART, 
                OTP_DATA_scrambled_jtag_password_BLEN, 
                (uintptr_t) &scrambled_jtag_password);

        if(scrambled_jtag_password != 0)
        {
            TEST_INFO("read scrambled jtag_password\n");
            /* read scramble seed, based on lower byte of serial number */
            scramble_seed = *(uint8_t *) (OTP_SHADOW_BASE + OTP_BIT_TO_BYTE(OTP_DATA_spare128_device_serial_no_BSTART));

            if (scramble_seed == 0x0){
                scramble_seed = 100;
            }
            /* descramble password, using seed of 100 */
            scramble_gf(1,
                    scramble_seed,
                    (unsigned char *)(OTP_SHADOW_BASE + OTP_BIT_TO_BYTE(OTP_DATA_jtag_password_BSTART)),
                    (unsigned char *) jtag_password,
                    CORTINA_SECURE_JTAG_PASSWORD_LEN);
        }else{
          /* not scrambled */
            memcpy(jtag_password, (const uint8_t *) (OTP_SHADOW_BASE + 
                        OTP_BIT_TO_BYTE(OTP_DATA_jtag_password_BSTART)), CORTINA_SECURE_JTAG_PASSWORD_LEN);
        }
#else /* LIB_GF_SCRAMBLE */
        /* not scrambled */
        memcpy(jtag_password, (const uint8_t *) (OTP_SHADOW_BASE + 
                    OTP_BIT_TO_BYTE(OTP_DATA_jtag_password_BSTART)), CORTINA_SECURE_JTAG_PASSWORD_LEN);
#endif /* LIB_GF_SCRAMBLE */

    }

    HWSIM_PROGRESS_MARK(41);

    /* write the password */
    pSecureJTAGDrv = cortina_secure_jtag_init((uintptr_t) KSEG1_ATU_XLAT(GLOBAL_DEVICE_ASSIGN_ID1));
	if(pSecureJTAGDrv != NULL){
        cortina_secure_jtag_set_password(pSecureJTAGDrv, jtag_password, CORTINA_SECURE_JTAG_PASSWORD_LEN);
        HWSIM_PROGRESS_MARK(42);
    }


#endif /* CORTINA_SECURE_JTAG_SUPPORT */


    HWSIM_PROGRESS_MARK(47);
    plat_io_setup();

#ifdef CORTINA_MULTIBOOT_SUPPORT
    HWSIM_PROGRESS_MARK(48);
    plat_set_pinstrap_fip_device();
#endif /* CORTINA_MULTIBOOT_SUPPORT */
    HWSIM_PROGRESS_MARK(49);

    NOTICE("Platform initialized\n");
}


/*******************************************************************************
 * Before calling this function BL2 is loaded in memory and its entrypoint
 * is set by load_image. This is a placeholder for the platform to change
 * the entrypoint of BL2.
 ******************************************************************************/
void bl1_plat_set_bl2_ep_info(image_info_t *bl2_image,
                  entry_point_info_t *bl2_ep)
{
    /* No change necessary for Saturn */
}

/* This function is called just before we leave BL1 */

void bl1_plat_prepare_exit(entry_point_info_t *ep_info)
{
#ifdef CORTINA_PER_WATCHDOG
#ifdef CORTINA_OTP_SUPPORT
    CORTINA_OTP_DRV_t * pOtpDrv = NULL;
#endif /* CORTINA_OTP_SUPPORT */
    CORTINA_PER_WATCHDOG_DRV_t* pWatchdogDrv;
    uint8_t secure_watchdog_enable = 0;
    
#ifdef CORTINA_OTP_SUPPORT
    pOtpDrv = cortina_otp_get_drv_instance(0);
    cortina_otp_shadow_get_field(pOtpDrv,
            OTP_DATA_secure_watchdog_enable_BSTART,
            OTP_DATA_secure_watchdog_enable_BLEN,
            (uintptr_t) &secure_watchdog_enable);

    if(secure_watchdog_enable 
            && !plat_saturn_in_hgu_mode()
            && !plat_saturn_in_backup_boot()){
#endif /* CORTINA_OTP_SUPPORT */
        /* Disable watchdog before leaving BL1 */
        pWatchdogDrv = cortina_per_watchdog_get_drv_instance(0);

        if(pWatchdogDrv != NULL){
            cortina_per_watchdog_disable_sys_reset(pWatchdogDrv);
        }
#ifdef CORTINA_OTP_SUPPORT
    }
#endif /* CORTINA_OTP_SUPPORT */
#endif /* CORTINA_PER_WATCHDOG */
}

