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

#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <cci.h>
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
#include <generic_delay_timer.h>

/*******************************************************************************
 * Declarations of linker defined symbols which will help us find the layout
 * of trusted RAM
 ******************************************************************************/
extern unsigned long __COHERENT_RAM_START__;
extern unsigned long __COHERENT_RAM_END__;

/* global */
GLOBAL_STRAP_t venus_global_strap;
GLOBAL_GLOBAL_CONFIG_t venus_global_cfg;
GLOBAL_ARM_DEBUG_t venus_global_arm_debug;
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

void cortina_force_icache_disable()
{
      /* clear the instruction cache enable I bit (bit 12, 0x4) */
        __asm__ volatile(
                "mov    x1, #0x1000     \n\t"
                "mrs    x0, sctlr_el3   \n\t"
                "bic    x0, x0, x1      \n\t"
                "msr    sctlr_el3, x0   \n\t"
                "isb                    \n\t"
                "dsb sy"
                :
                :
                : "memory"
                );
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

#if defined(TEST_OTP) || defined(TEST_ON_CORTINA_ASIC_SIM) && defined(TEST_ON_CORTINA_ASIC_SIM_HOTFIX)
char *msg1 = "fun called from hotfix code msg 1";
char *msg2 = "fun called from hotfix code msg 2";
char *msg3 = "fun called from hotfix code msg 3";

const unsigned int foo1[] = {
        0xf9400001,
        0xf9400400,
        0xd61f0020
};

unsigned long long int params[2];
void test_hotfix(void) {
    params[0] = (unsigned long long int) puts;
    params[1] = (unsigned long long int) msg1;
    BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_TEST1, &params[0]);
}
#endif

#ifdef TEST_OTP
#ifdef TEST_OTP_HASH_OF_ROTPK
// use Cortina ROTPK in TEST_OTP test
static const unsigned char arm_rotpk_test_hash[] = {
        0xf5,0x52,0xe5,0xbd,0xf3,0x68,0xa0,0x4d,
        0x9d,0xb7,0x78,0x59,0x9d,0xfe,0x84,0x02,
        0xef,0x30,0xfd,0xbb,0x32,0xa3,0xef,0xbe,
        0x21,0xc5,0xae,0xd3,0x30,0x7c,0x95,0x14
            };
#endif

            // len should be multiple of 4
#define MEMDUMP_WC_MAX_PERLINE 4
void memdump(char *start, int len) {
    int i;
    int numofword = (len + 3) >> 2;
    unsigned int * p = (unsigned int *) start;
    int wc_in_line = 0;
    for (i = 0; i < numofword; i++) {
        if (wc_in_line == 0) {
            printf("%016lx (%08x): ", (unsigned long int) (&p[i]), i * 4);
        }
            printf("%08x ", p[i]);
        wc_in_line++;
        if (wc_in_line >= MEMDUMP_WC_MAX_PERLINE) {
            printf("\n");
            wc_in_line = 0;
        }
    }
    if (wc_in_line != 0) {
        printf("\n");
    }
}

#define MEMDUMP_BYTE_MAX_PERLINE 16
void memdump_byte(char *start, int len) {
    int i;
    int numofword = len;
    char * p = start;
    int wc_in_line = 0;
    for (i = 0; i < numofword; i++) {
        if (wc_in_line == 0) {
            printf("%016lx (%08x): ", (unsigned long int) (&p[i]), i * 4);
        }
        printf("%02x ", p[i]);
        wc_in_line++;
        if (wc_in_line >= MEMDUMP_BYTE_MAX_PERLINE) {
            printf("\n");
            wc_in_line = 0;
        }
    }
    if (wc_in_line != 0) {
        printf("\n");
    }
}


void cortina_test_preload_otp(void) {
    // dump OTP
    char *otp_base = (char *)OTP_BASE;
    memset(&otp_base[0], 0, 1024);
    INFO("OTP start at 0x%x\n", OTP_BASE);
    INFO("OTP shadow at 0x%x\n", OTP_SHADOW_BASE);

    INFO("OTP OTP_BIT_SIZE %d\n", OTP_BIT_SIZE);
    INFO("OTP non_trusted_NV_min_ver bit loc %d blen %d\n", OTP_DATA_non_trusted_NV_min_ver_BSTART, OTP_DATA_non_trusted_NV_min_ver_BLEN);
    INFO("OTP trusted_NV_min_ver bit loc     %d blen %d\n", OTP_DATA_trusted_NV_min_ver_BSTART, OTP_DATA_trusted_NV_min_ver_BLEN);
    INFO("OTP Region # 3 End %d\n", OTP_DATA_trusted_NV_min_ver_BSTART + OTP_DATA_trusted_NV_min_ver_BLEN -1);

    INFO("BL1_RW_BASE 0x%x BL1_RW_LIMIT 0x%x BL1_RW_SIZE 0x%x BL1_RW_SIZE_MAX 0x%x\n", BL1_RW_BASE, BL1_RW_LIMIT, BL1_RW_SIZE, BL1_RW_SIZE_MAX);

    // zero OTP
    memset((char *)OTP_BASE, 0, OTP_BIT_SIZE/8);
    INFO("clean OTP\n");
    // preload OTP as we want
    INFO("preload OTP\n");

    // test OTP_DATA_programmed_BSTART
    int byte_offset = OTP_DATA_programmed_BSTART/8;
    int bit_pos = OTP_DATA_programmed_BSTART % 8;
    INFO("OTP programmed bit loc %d byte_offset %d  bit_pos %d\n", OTP_DATA_programmed_BSTART, byte_offset, bit_pos);
    // set the bit
    otp_base[byte_offset] |= (1<<bit_pos);

//#define TEST_OTP_MFG_CLI_ENABLE
#ifdef TEST_OTP_MFG_CLI_ENABLE
    // test OTP_DATA_mfg_cli_enable_BSTART
    byte_offset = OTP_DATA_mfg_cli_enable_BSTART/8;
    bit_pos = OTP_DATA_mfg_cli_enable_BSTART % 8;
    otp_base[byte_offset] |= (1<<bit_pos);
#endif // TEST_OTP_MFG_CLI_ENABLE

//#define TEST_OTP_DEBUG_OVERRIDE
#ifdef TEST_OTP_DEBUG_OVERRIDE
    // test OTP_DATA_debug_override_BSTART
    byte_offset = OTP_DATA_debug_override_BSTART/8;
    bit_pos = OTP_DATA_debug_override_BSTART % 8;
    otp_base[byte_offset] |= (1<<bit_pos);

    // test OTP_DATA_debug_cfg_tpmaxdatasize_BSTART
    byte_offset = OTP_DATA_debug_cfg_tpmaxdatasize_BSTART/8;
    bit_pos = OTP_DATA_debug_cfg_tpmaxdatasize_BSTART % 8;
    otp_base[byte_offset] |= (0x11<<bit_pos);

// enable secure watch dog timer
//#define TEST_OTP_SECURE_WATHDOG_ENABLE
#ifdef TEST_OTP_SECURE_WATHDOG_ENABLE
    byte_offset =   OTP_DATA_secure_watchdog_enable_BSTART / 8;
    bit_pos = OTP_DATA_secure_watchdog_enable_BSTART % 8;
    otp_base[byte_offset] |= (1<<bit_pos);
#endif // TEST_OTP_SECURE_WATHDOG_ENABLE
#endif // TEST_OTP_DEBUG_OVERRIDE

// enable secure watch dog timer
//#define TEST_OTP_ENBABLE_WDT
#ifdef TEST_OTP_ENBABLE_WDT
    byte_offset =   OTP_DATA_secure_watchdog_enable_BSTART / 8;
    bit_pos = OTP_DATA_secure_watchdog_enable_BSTART % 8;
    otp_base[byte_offset] |= (1<<bit_pos);
#endif // TEST_OTP_ENBABLE_WDT


// enable Multiboot OSB 2 bit Reduced Pattern
//#define TEST_OTP_MULTIBOOT_OSB_REDUCED_PATTERN
#ifdef TEST_OTP_MULTIBOOT_OSB_REDUCED_PATTERN
    byte_offset =   OTP_DATA_one_shot_boot_reduced_pattern_BSTART / 8;
    bit_pos = OTP_DATA_one_shot_boot_reduced_pattern_BSTART % 8;
    otp_base[byte_offset] |= (1<<bit_pos);
#endif // TEST_OTP_MULTIBOOT_OSB_REDUCED_PATTERN

    // enable the following when test ROTPK to preload OTP ROTPK with the test hash value
//#define TEST_OTP_HASH_OF_ROTPK
#ifdef TEST_OTP_HASH_OF_ROTPK
    byte_offset = OTP_DATA_hash_of_ROT_public_key_BSTART / 8;
    memcpy(&otp_base[byte_offset],  &arm_rotpk_test_hash[0], 32);
#endif // TEST_OTP_HASH_OF_ROTPK
//#define TEST_OTP_ALLOW_BL1_NV_CNTR_WRITES
#ifdef TEST_OTP_ALLOW_BL1_NV_CNTR_WRITES
    byte_offset = OTP_DATA_allow_bl1_nv_cntr_writes_BSTART / 8;
    bit_pos =  OTP_DATA_allow_bl1_nv_cntr_writes_BSTART % 8;
    otp_base[byte_offset] |= (0x1<<bit_pos);
#endif // TEST_OTP_ALLOW_BL1_NV_CNTR_WRITES

//#define TEST_OTP_NV_CNTR
#ifdef TEST_OTP_NV_CNTR
    // load non-secure nv ctr 3 and secure nv ctr 2
    byte_offset = OTP_DATA_non_trusted_NV_min_ver_BSTART / 8;
    otp_base[byte_offset] = 0x7; // 3, that is, 0b00000111
    byte_offset = OTP_DATA_trusted_NV_min_ver_BSTART / 8;
    otp_base[byte_offset] = 0x3; // 2, that is, 0b00000011
#endif // TEST_OTP_NV_CNTR

// preload forced_boot_order list
//#define TEST_OTP_FORCED_BOOT_ORDER
#ifdef TEST_OTP_FORCED_BOOT_ORDER
    byte_offset = OTP_DATA_forced_boot_order_BSTART/8;
    otp_base[byte_offset] = (CORTINA_BOOT_DEVICE_PARALLEL_NAND<<1|1) + ((CORTINA_BOOT_DEVICE_MMC<<1|0)<<4); //SPI NAND part0 , SERIAL_NOR part1
    otp_base[byte_offset+1] = (CORTINA_BOOT_DEVICE_SERIAL_NOR<<1|0) + ((CORTINA_BOOT_DEVICE_SERIAL_NAND<<1|1)<<4); // MMC part0, PARALLEL_NAND part0
#endif // TEST_OTP_FORCED_BOOT_ORDER

//#define TEST_OTP_SPI_NAND_PARAMETER
#ifdef TEST_OTP_SPI_NAND_PARAMETER
    int defaults_override = 1;
    int plane_select_bit = (0)+ (12<<1);
    int page_size = 2;//1;
    int blocks_per_unit = 11;//9;//11;
    int page_per_block = 6;
    int feature_conf_extra_bits = 0;
    int plane_select = (0)+ (6<<1);

    byte_offset = OTP_DATA_serial_nand_defaults_override_BSTART/8;
    otp_base[byte_offset+0] = defaults_override + (plane_select_bit<<1) + ((page_size&0x3)<<6);
    otp_base[byte_offset+1] = ((page_size>>2)&0x3) + ((blocks_per_unit)<<2) + ((page_per_block&0x3)<<6);
    otp_base[byte_offset+2] = ((page_per_block>>2)&0x3) + ((feature_conf_extra_bits)<<2) + ((plane_select&0x3)<<6);
    otp_base[byte_offset+3] = ((plane_select>>2)&0x3);
#endif // TEST_OTP_SPI_NAND_PARAMETER

//#define TEST_OTP_BL1_ICACHE_DISABLE
#ifdef TEST_OTP_BL1_ICACHE_DISABLE
    int remainder = OTP_DATA_force_bl1_icache_disable_BSTART%8;
    byte_offset = OTP_DATA_force_bl1_icache_disable_BSTART/8;
    otp_base[byte_offset] = (otp_base[byte_offset]&(~(1<<remainder)))|(1<<remainder);
#endif // TEST_OTP_SPI_NAND_PARAMETER

// preload auth_override[0] = 1 to test skipping BL2 auth
//#define TEST_OTP_AUTH_OVERRIDE
#ifdef TEST_OTP_AUTH_OVERRIDE
    byte_offset = OTP_DATA_auth_override_BSTART / 8;
    bit_pos = OTP_DATA_auth_override_BSTART % 8;
    otp_base[byte_offset] |= (0x1<<bit_pos);  // only set auth_override[0] as 1 to force skipping BL2 auth
#endif // TEST_OTP_AUTH_OVERRIDE

// test hotfix
#ifdef TEST_ON_CORTINA_ASIC_SIM_HOTFIX
    BL1_hotfix_entry_t *hotfix_tab = 0;
    // preload hotfix code foo1 into OTP

    memcpy(&otp_base[OTP_REGION2_BSTART/8 + 7 * 8], foo1, sizeof(foo1));
    // preload hotfix code foo1 into SRAM and also signagure
    *(unsigned int *)(0xC003C000 + 32) = 0x12349876; // signature
    memcpy((char *)(0xC003C004 + 32), foo1, sizeof(foo1));

    hotfix_tab = (BL1_hotfix_entry_t *) ((long long unsigned int) otp_base + OTP_DATA_bl1_hotfix_checkpoint_table_BSTART/8);

    hotfix_tab[1].bf.checkpoint_id = CHECKPOINT_ID_TEST1; //
    hotfix_tab[1].bf.in_sram = 0;
    hotfix_tab[1].bf.hotfix_offset = 7; // that is, at byte offset 0x0 from the base 0xC003C000

    hotfix_tab[0].bf.checkpoint_id = CHECKPOINT_ID_TEST2;
    hotfix_tab[0].bf.in_sram = 1;
    hotfix_tab[0].bf.hotfix_offset = 1; // that is, at byte offset OTP_REGION2_BSTART/8 in otp shadow space


    hotfix_tab[2].bf.checkpoint_id = CHECKPOINT_ID_TEST3;
    hotfix_tab[2].bf.in_sram = 0;
    hotfix_tab[2].bf.hotfix_offset = 7; // that is, at byte offset OTP_REGION2_BSTART/8 in otp shadow space
#endif


//#define TEST_OTP_DUMP_OTP_INFO
#ifdef TEST_OTP_DUMP_OTP_INFO
    // the following INFOs are to dump bit location and len of bits for each OTP fixed parameters

    // test OTP_DATA_hardware_unique_key_BSTART
    // BL1 does not use it. So, just display the len and position
    INFO("OTP hardware_unique_key bit location %d len %d\n",
    OTP_DATA_hardware_unique_key_BSTART, OTP_DATA_hardware_unique_key_BLEN);
    INFO("OTP endorsement_key bit location %d len %d\n",
    OTP_DATA_endorsement_key_BSTART, OTP_DATA_endorsement_key_BLEN);

    INFO("OTP secure_storage_key bit location %d len %d\n",
    OTP_DATA_secure_storage_key_BSTART, OTP_DATA_secure_storage_key_BLEN);

    INFO("OTP spare128_device_serial_no bit location %d len %d\n",
    OTP_DATA_spare128_device_serial_no_BSTART, OTP_DATA_spare128_device_serial_no_BLEN);

    INFO("OTP multiboot osb reduced pattern bit location %d len %d\n",
    OTP_DATA_one_shot_boot_reduced_pattern_BSTART, OTP_DATA_one_shot_boot_reduced_pattern_BLEN);


    INFO("OTP jtag_password bit location %d len %d\n",
    OTP_DATA_jtag_password_BSTART, OTP_DATA_jtag_password_BLEN);



    INFO("OTP pe_lockdown_0 bit location %d len %d\n",
    OTP_DATA_pe_lockdown_0_BSTART, OTP_DATA_pe_lockdown_0_BLEN);

    INFO("OTP pe_lockdown_1 bit location %d len %d\n",
    OTP_DATA_pe_lockdown_1_BSTART, OTP_DATA_pe_lockdown_1_BLEN);

    INFO("OTP mfg_cli_enable bit location %d len %d\n",
    OTP_DATA_mfg_cli_enable_BSTART, OTP_DATA_mfg_cli_enable_BLEN);

    INFO("OTP copy_ssk_to_sadb bit location %d len %d\n",
    OTP_DATA_copy_ssk_to_sadb_BSTART, OTP_DATA_copy_ssk_to_sadb_BLEN);

    INFO("OTP allow_bl1_nv_cntr_writes bit location %d len %d\n",
    OTP_DATA_allow_bl1_nv_cntr_writes_BSTART, OTP_DATA_allow_bl1_nv_cntr_writes_BLEN);

    INFO("OTP df0 bit location %d len %d\n",
    OTP_DATA_df0_BSTART, OTP_DATA_df0_BLEN);
    INFO("OTP df1 bit location %d len %d\n",
    OTP_DATA_df1_BSTART, OTP_DATA_df1_BLEN);
    INFO("OTP df2 bit location %d len %d\n",
    OTP_DATA_df2_BSTART, OTP_DATA_df2_BLEN);
    INFO("OTP df3 bit location %d len %d\n",
    OTP_DATA_df3_BSTART, OTP_DATA_df3_BLEN);
    INFO("OTP df4 bit location %d len %d\n",
    OTP_DATA_df4_BSTART, OTP_DATA_df4_BLEN);
    INFO("OTP df5 is the same as scrambled_jtag_password\n");

    INFO("OTP ssk_sadb_control_word bit location %d len %d\n",
    OTP_DATA_ssk_sadb_control_word_BSTART, OTP_DATA_ssk_sadb_control_word_BLEN);

    INFO("OTP forced_boot_order bit location %d len %d\n",
    OTP_DATA_forced_boot_order_BSTART, OTP_DATA_forced_boot_order_BLEN);

    INFO("OTP programmed bit location %d len %d\n",
    OTP_DATA_programmed_BSTART, OTP_DATA_programmed_BLEN);



    INFO("OTP auth_override bit location %d len %d\n",
    OTP_DATA_auth_override_BSTART, OTP_DATA_auth_override_BLEN);

    INFO("OTP forced_boot_order bit location %d len %d\n",
    OTP_DATA_forced_boot_order_BSTART, OTP_DATA_forced_boot_order_BLEN);

    INFO("OTP jtag_password_inhibit bit location %d len %d\n",
    OTP_DATA_jtag_password_inhibit_BSTART, OTP_DATA_jtag_password_inhibit_BLEN);

    INFO("OTP debug_override bit location %d len %d\n",
    OTP_DATA_debug_override_BSTART, OTP_DATA_debug_override_BLEN);

    INFO("OTP debug_debugen bit location %d len %d\n",
    OTP_DATA_debug_debugen_BSTART, OTP_DATA_debug_debugen_BLEN);
    INFO("OTP debug_cpu_niden bit location %d len %d\n",
    OTP_DATA_debug_cpu_niden_BSTART, OTP_DATA_debug_cpu_niden_BLEN);
    INFO("OTP debug_cpu_spiden bit location %d len %d\n",
    OTP_DATA_debug_cpu_spiden_BSTART, OTP_DATA_debug_cpu_spiden_BLEN);
    INFO("OTP debug_cpu_spinden bit location %d len %d\n",
    OTP_DATA_debug_cpu_spinden_BSTART, OTP_DATA_debug_cpu_spinden_BLEN);

    INFO("OTP debug_auth_cssys_dbgen bit location %d len %d\n",
    OTP_DATA_debug_auth_cssys_dbgen_BSTART, OTP_DATA_debug_auth_cssys_dbgen_BLEN);

    INFO("OTP debug_auth_cssys_niden bit location %d len %d\n",
    OTP_DATA_debug_auth_cssys_niden_BSTART, OTP_DATA_debug_auth_cssys_niden_BLEN);

    INFO("OTP debug_cs_auth_niden bit location %d len %d\n",
    OTP_DATA_debug_cs_auth_niden_BSTART, OTP_DATA_debug_cs_auth_niden_BLEN);

    INFO("OTP debug_cs_auth_spinden bit location %d len %d\n",
    OTP_DATA_debug_cs_auth_spinden_BSTART, OTP_DATA_debug_cs_auth_spinden_BLEN);

    INFO("OTP debug_cfg_tpctl bit location %d len %d\n",
    OTP_DATA_debug_cfg_tpctl_BSTART, OTP_DATA_debug_cfg_tpctl_BLEN);

    INFO("OTP debug_cfg_tpmaxdatasize bit location %d len %d\n",
    OTP_DATA_debug_cfg_tpmaxdatasize_BSTART, OTP_DATA_debug_cfg_tpmaxdatasize_BLEN);

    INFO("OTP auth_override bit location %d len %d\n",
    OTP_DATA_auth_override_BSTART, OTP_DATA_auth_override_BLEN);

    INFO("OTP jtag_password_inhibit bit location %d len %d\n",
    OTP_DATA_jtag_password_inhibit_BSTART, OTP_DATA_jtag_password_inhibit_BLEN);

    INFO("OTP secure_watchdog_enable bit location %d len %d\n",
    OTP_DATA_secure_watchdog_enable_BSTART, OTP_DATA_secure_watchdog_enable_BLEN);

    INFO("OTP hash_of_ROT_public_key bit location %d len %d\n",
    OTP_DATA_hash_of_ROT_public_key_BSTART, OTP_DATA_hash_of_ROT_public_key_BLEN);

    INFO("OTP scrambled_jtag_password bit location %d len %d\n",
    OTP_DATA_scrambled_jtag_password_BSTART, OTP_DATA_scrambled_jtag_password_BLEN);

    INFO("OTP non_trusted_NV_min_ver bit location %d len %d\n",
    OTP_DATA_non_trusted_NV_min_ver_BSTART, OTP_DATA_non_trusted_NV_min_ver_BLEN);

    INFO("OTP trusted_NV_min_ver bit location %d len %d\n",
    OTP_DATA_trusted_NV_min_ver_BSTART, OTP_DATA_trusted_NV_min_ver_BLEN);
#endif /* TEST_OTP_DUMP_OTP_INFO end of dump loc and len */

//#define TEST_OTP_JTAG_PASSWORD_SCRAMBLING
#ifdef TEST_OTP_JTAG_PASSWORD_SCRAMBLING
    // test jtag_password scrambling
    // load serial number with some value in the lowest byte
    byte_offset = OTP_DATA_spare128_device_serial_no_BSTART/8;
    otp_base[byte_offset] = 'K';
    static unsigned char jtag_password_test[] = "12345678";
    uint8_t scramble_seed;
    int byte_offset_jtag_password = OTP_DATA_jtag_password_BSTART/8;

    scramble_seed = otp_base[byte_offset];
    INFO("scramble_seed %d\n", scramble_seed);
    INFO("jtag_password before scrambled\n");
    memdump_byte((char *)&jtag_password_test[0], 8);
    scramble_gf(1, scramble_seed, jtag_password_test, (unsigned char *)&otp_base[byte_offset_jtag_password], CORTINA_SECURE_JTAG_PASSWORD_LEN);
    INFO("scrambed jtag_password in OTP\n");
    memdump_byte(&otp_base[byte_offset_jtag_password], 8);

    // turn on  OTP_DATA_scrambled_jtag_password_BSTART
    // test OTP_DATA_debug_override_BSTART
    byte_offset = OTP_DATA_scrambled_jtag_password_BSTART/8;
    bit_pos = OTP_DATA_scrambled_jtag_password_BSTART % 8;
    otp_base[byte_offset] |= (1<<bit_pos);

#endif // TEST_OTP_JTAG_PASSWORD_SCRAMBLING

// test scrambled and descrabled SSK
//#define TEST_OTP_SCRAMBLED_SSK
#ifdef TEST_OTP_SCRAMBLED_SSK
    static unsigned char ssk[] = "87651234abcd12341111222233334444";
    byte_offset = OTP_DATA_spare128_device_serial_no_BSTART/8;
    otp_base[byte_offset] = 'K';
    scramble_seed = otp_base[byte_offset];
    int byte_offset_ssk = OTP_DATA_secure_storage_key_BSTART/8;
    INFO("scramble_seed %d\n", scramble_seed);
    INFO("ssk before scrambed\n");
    memdump_byte((char*)&ssk[0], 32);
    scramble_gf(1, scramble_seed, ssk, (unsigned char *)&otp_base[byte_offset_ssk], 32);
    INFO("scrambed ssk in OTP\n");
    memdump_byte(&otp_base[byte_offset_ssk], 32);


    {
        typedef union {
            struct {
                uint32_t calg:3;       /* bits 2:0 */
                uint32_t key_sz:2;     /* bits 4:3 */
                uint32_t encrypt:1;    /* bits 5:5 */
                uint32_t cmode:4;      /* bits 9:6 */
                uint32_t secure:1;     /* bits 10 */
                uint32_t reserve:21;   /* bits 31:11 */
            } bf;
            struct {
                uint32_t sw1: 16;
                uint32_t sw2: 16;
            } swords;
            uint32_t wrd;
        } CORTINA_SADB_CTRL_t;
        CORTINA_SADB_CTRL_t sadb_ctrl;

        sadb_ctrl.bf.calg = 0x2; // AES
        sadb_ctrl.bf.key_sz = 0; // for 16 bytes, not 32 bytes
        sadb_ctrl.bf.encrypt = 1; // enabled
        sadb_ctrl.bf.cmode = 0; // cbc
        sadb_ctrl.bf.secure = 1 ; // secured ?
        sadb_ctrl.bf.reserve = 0; // reserved
        // write this 16 bits into OTP
        ((volatile unsigned short *)(OTP_BASE + OTP_DATA_ssk_sadb_control_word_BSTART/8))[0] = sadb_ctrl.swords.sw1;
    }

    byte_offset = OTP_DATA_copy_ssk_to_sadb_BSTART/8;
    bit_pos = OTP_DATA_copy_ssk_to_sadb_BSTART % 8;
    otp_base[byte_offset] |= (1<<bit_pos);

    byte_offset = OTP_DATA_df3_BSTART/8;
    bit_pos = OTP_DATA_df3_BSTART % 8;
    otp_base[byte_offset] |= (1<<bit_pos);
#endif // TEST_OTP_SCRAMBLED_SSK

    INFO("OTP DUMP ALL 1024:\n");
    memdump(&otp_base[0], 1024);
}
#endif // TEST_OTP


/*******************************************************************************
 * Perform any BL1 specific platform actions.
 ******************************************************************************/
void plat_venus_bl1_early_platform_setup(void)
{
#ifdef CORTINA_OTP_SUPPORT
        CORTINA_OTP_DRV_t * pOtpDrv = NULL;
	uint8_t force_bl1_icache_disable = 0;
#endif /* CORTINA_OTP_SUPPORT */

        HWSIM_PROGRESS_MARK(0xffffffff);
        venus_global_strap.wrd = mmio_read_32((GLOBAL_STRAP));
	venus_global_cfg.wrd = mmio_read_32(GLOBAL_GLOBAL_CONFIG);
#ifdef CORTINA_PER_WATCHDOG
        CORTINA_PER_WATCHDOG_DRV_t * pWatchDogDrv;
	uint8_t secure_watchdog_enable = 0;
#endif /* CORTINA_PER_WATCHDOG */
#ifndef CORTINA_VENUS_ASIC
	DDR_CTRL_DDR_SCH_CTRL_t ddr_sch_ctrl;
#endif
	const size_t bl1_size = BL1_RAM_LIMIT - BL1_RAM_BASE;

	/* support dual core variants */
	if(venus_global_strap.bf.NO_QUAD == 1)
	 {
	  venus_global_cfg.bf.quad_off = 1;
	  mmio_write_32(GLOBAL_GLOBAL_CONFIG, venus_global_cfg.wrd);
	 }

	/* VENUS System Timer Init */
	mmio_write_32(VENUS_SYS_TIMER_BASE, 0x1);

	generic_delay_timer_init();

#ifdef CORTINA_OTP_SUPPORT
#ifdef TEST_OTP
        /* Init DDR controller */
        ddr_sch_ctrl.wrd = 0x01f74010;
        mmio_write_32(DDR_CTRL_DDR_SCH_CTRL, ddr_sch_ctrl.wrd);
        console_init(CONSOLE_BASE, CORTINA_PER_UART_CLK_IN_HZ, CORTINA_PER_UART_BAUDRATE);
        cortina_test_preload_otp();
#endif // TEST_OTP
        HWSIM_PROGRESS_MARK(27);
        pOtpDrv = cortina_otp_init(  (uintptr_t) OTP_BASE,
                        (uint32_t) OTP_BIT_SIZE,
                        (uintptr_t) OTP_SHADOW_BASE,
                        (uint32_t) OTP_READ_TIMEOUT,
                        (uint32_t) OTP_WRITE_TIMEOUT,
                        (uint32_t) (VENUS_LSAXI_CLK/1000000)
                       );
        HWSIM_PROGRESS_MARK(1);
	if (pOtpDrv == NULL) {
            HWSIM_PROGRESS_MARK(2);
	    console_init(CONSOLE_BASE, CORTINA_PER_UART_CLK_IN_HZ, CORTINA_PER_UART_BAUDRATE);
	    INFO("VENUS OTP Driver Init FAILED\n");
	    panic();
	}
	cortina_pOtpDrv = pOtpDrv;
#endif /* CORTINA_OTP_SUPPORT */

#ifdef TEST_ON_CORTINA_ASIC_SIM
#if defined(TEST_ON_CORTINA_ASIC_SIM_OTP_SHADOW_COPY) || defined(TEST_ON_CORTINA_ASIC_SIM_OTP_WRITE)
        do_test_on_asic_sim(pOtpDrv);
        HWSIM_PROGRESS_MARK(3);
#endif
#endif

#ifdef TEST_OTP
    cortina_pOtpDrv->shadow_base_addr = OTP_BASE;
#endif
	/* Initialize the console to provide early debug support */
	console_init(CONSOLE_BASE, CORTINA_PER_UART_CLK_IN_HZ, CORTINA_PER_UART_BAUDRATE);

        cortina_otp_shadow_field_is_programmed(pOtpDrv,
                                        (uint32_t) OTP_DATA_force_bl1_icache_disable_BSTART,
                                        (uint32_t) OTP_DATA_force_bl1_icache_disable_BLEN,
                                        &force_bl1_icache_disable);

	if(force_bl1_icache_disable != 0)
	  cortina_force_icache_disable();

        HWSIM_PROGRESS_MARK(4);
	BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_UART_INIT, pOtpDrv);
#if defined(TEST_OTP) || defined(TEST_ON_CORTINA_ASIC_SIM) && defined(TEST_ON_CORTINA_ASIC_SIM_HOTFIX)
        HWSIM_PROGRESS_MARK(0x47);
        test_hotfix();
        HWSIM_PROGRESS_MARK(0x48);
        params[0] = (unsigned long long int) puts;
        params[1] = (unsigned long long int) msg2;
        BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_TEST2, &params[0]);

        params[1] = (unsigned long long int) msg3;
        if (1 == BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_TEST3, &params[0])) {
                HWSIM_PROGRESS_MARK(0x49);
                INFO("TEST3 hotfix executed\n");
        } else {
                HWSIM_PROGRESS_MARK(0x50);
                INFO("TEST3 hotfix not executed\n");
        }
#endif


#ifdef CORTINA_PER_WATCHDOG
        cortina_otp_shadow_field_is_programmed(pOtpDrv,
                                        (uint32_t) OTP_DATA_secure_watchdog_enable_BSTART,
                                        (uint32_t) OTP_DATA_secure_watchdog_enable_BLEN,
                                        &secure_watchdog_enable);
        HWSIM_PROGRESS_MARK(5);
        /* initialized Watchdog Driver for 256 sec. period */
        /* Note: 2nd argument must be in Khz, not Hz */
        pWatchDogDrv = cortina_per_watchdog_init((uintptr_t) (PER_WDT_CTRL),
                           (APB_PCLK/1000),
                           256,
                           CORTINA_PER_WATCHDOG_UNIT_SEC
                          );
        HWSIM_PROGRESS_MARK(6);
	BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_WATHDOG, pOtpDrv);
        if(pWatchDogDrv == NULL)
          WARN("Watchdog driver init failed!!\n");
        else if(secure_watchdog_enable != 0)
	  {
            TEST_INFO("Enable WDT reset in GLOBAL_CONFIG\n");
	    /* Enable WDT reset in GLOBAL_CONFIG */
	    venus_global_cfg.bf.wd_reset_ext_reset = 1;
	    venus_global_cfg.bf.wd_reset_remap = 1;
	    venus_global_cfg.bf.wd_reset_all_blocks= 1;
	    venus_global_cfg.bf.wd_reset_subsys_enable = 1;
	    mmio_write_32(GLOBAL_GLOBAL_CONFIG, venus_global_cfg.wrd);

            /* enable system reset if watchdog expires */
            cortina_per_watchdog_enable_sys_reset(pWatchDogDrv);
	  }
        HWSIM_PROGRESS_MARK(7);
#endif /* CORTINA_PER_WATCHDOG */

	/* Init DDR controller */
#ifndef CORTINA_VENUS_ASIC
	ddr_sch_ctrl.wrd = 0x01f74010;
	mmio_write_32(DDR_CTRL_DDR_SCH_CTRL, ddr_sch_ctrl.wrd);
#endif
	/* Allow BL1 to see the whole Trusted RAM */
	bl1_tzram_layout.total_base = PLAT_VENUS_BL_RAM_BASE;
	bl1_tzram_layout.total_size = PLAT_VENUS_BL_RAM_SIZE;

	/* Calculate how much RAM BL1 is using and how much remains free */
	bl1_tzram_layout.free_base = PLAT_VENUS_BL_RAM_BASE;
	bl1_tzram_layout.free_size = PLAT_VENUS_BL_RAM_SIZE;
        HWSIM_PROGRESS_MARK(8);
	reserve_mem(&bl1_tzram_layout.free_base,
		    &bl1_tzram_layout.free_size,
		    BL1_RAM_BASE,
		    bl1_size);
        HWSIM_PROGRESS_MARK(9);

	INFO("BL1: 0x%lx - 0x%lx [size = %lu]\n", BL1_RAM_BASE, BL1_RAM_LIMIT,
	     bl1_size);
        HWSIM_PROGRESS_MARK(28);

//#define TIMER_TEST
#ifdef TIMER_TEST
    mmio_write_32(PER_TMR_LD1, 0x000fffff);
    mmio_write_32(PER_TMR_LOADE, (1<<1));
    mmio_write_32(PER_TMR_IE1_0, 1);
    mmio_write_32(PER_TMR_CTRL1, ((1<<7)));
    while(!mmio_read_32(PER_TMR_INT1_0)){
        NOTICE("PER_TMR_CNT1=0x%x, PER_TMR_STAT1=%d\n",mmio_read_32(PER_TMR_CNT1), mmio_read_32(PER_TMR_STAT1));
    }
    NOTICE("End: PER_TMR_CNT1=0x%x, PER_TMR_STAT1=%d\n",mmio_read_32(PER_TMR_CNT1), mmio_read_32(PER_TMR_STAT1));


    mmio_write_32(PER_TMR_LD2, 0x000fffff);
    mmio_write_32(PER_TMR_LOADE, (1<<2));
    mmio_write_32(PER_TMR_IE2_0, 1);
    mmio_write_32(PER_TMR_CTRL2, ((1<<7)|(1<<6)));
    while(!mmio_read_32(PER_TMR_INT2_0)){
        NOTICE("PER_TMR_CNT2=0x%x, PER_TMR_STAT2=%d\n",mmio_read_32(PER_TMR_CNT2), mmio_read_32(PER_TMR_STAT2));
    }
    NOTICE("End: PER_TMR_CNT2=0x%x, PER_TMR_STAT2=%d\n",mmio_read_32(PER_TMR_CNT2), mmio_read_32(PER_TMR_STAT2));


    mmio_write_32(PER_TMR_LD3, 0x000fffff);
    mmio_write_32(PER_TMR_LOADE, (1<<3));
    mmio_write_32(PER_TMR_IE3_1, 1);
    mmio_write_32(PER_TMR_CTRL3, ((1<<7)));
    while(!mmio_read_32(PER_TMR_INT3_1)){
        NOTICE("PER_TMR_CNT3=0x%x, PER_TMR_STAT3=%d\n",mmio_read_32(PER_TMR_CNT3), mmio_read_32(PER_TMR_STAT3));
    }
    NOTICE("End: PER_TMR_CNT3=0x%x, PER_TMR_STAT3=%d\n",mmio_read_32(PER_TMR_CNT3), mmio_read_32(PER_TMR_STAT3));


    mmio_write_32(PER_TMR_LD4, 0x000fffff);
    mmio_write_32(PER_TMR_LOADE, (1<<4));
    mmio_write_32(PER_TMR_IE4_1, 1);
    mmio_write_32(PER_TMR_CTRL4, ((1<<7)|(1<<6)));
    while(!mmio_read_32(PER_TMR_INT4_1)){
        NOTICE("PER_TMR_CNT4=0x%x, PER_TMR_STAT4=%d\n",mmio_read_32(PER_TMR_CNT4), mmio_read_32(PER_TMR_STAT4));
    }
    NOTICE("End: PER_TMR_CNT4=0x%x, PER_TMR_STAT4=%d\n",mmio_read_32(PER_TMR_CNT4), mmio_read_32(PER_TMR_STAT4));
#if 0
    int i;
    for(i=0; i<50; i++){
        NOTICE("PER_TMR_INT4_1=%d, PER_TMR_CNT4=0x%x, PER_TMR_STAT4=%d\n",mmio_read_32(PER_TMR_INT4_1), mmio_read_32(PER_TMR_CNT4), mmio_read_32(PER_TMR_STAT4));
    }
    mmio_write_32(PER_TMR_INT4_1, 1);
    NOTICE("W1CLR\n\n");
    for(i=0; i<50; i++){
        NOTICE("PER_TMR_INT4_1=%d, PER_TMR_CNT4=0x%x, PER_TMR_STAT4=%d\n",mmio_read_32(PER_TMR_INT4_1), mmio_read_32(PER_TMR_CNT4), mmio_read_32(PER_TMR_STAT4));
    }
#endif
#endif

//#define INTERRUPT_TEST
#ifdef INTERRUPT_TEST
//    mmio_write_32(PER_PERIPHERAL_INTENABLE_0, (0xF<<2));
    NOTICE("PER_PERIPHERAL_INTERRUPT_0=0x%x\n",mmio_read_32(PER_PERIPHERAL_INTERRUPT_0));
    NOTICE("PER_PERIPHERAL_INTERRUPT_1=0x%x\n",mmio_read_32(PER_PERIPHERAL_INTERRUPT_1));


    mmio_write_32(PER_TMR_LD1, 0x000fffff);
    mmio_write_32(PER_TMR_LOADE, (1<<1));
    mmio_write_32(PER_TMR_IE1_0, 1);
    mmio_write_32(PER_TMR_CTRL1, ((1<<7)));
    while(!mmio_read_32(PER_TMR_INT1_0));
    NOTICE("PER_TMR_CNT1=0x%x, PER_TMR_STAT1=%d\n",mmio_read_32(PER_TMR_CNT1), mmio_read_32(PER_TMR_STAT1));
    NOTICE("PER_PERIPHERAL_INTERRUPT_0=0x%x\n",mmio_read_32(PER_PERIPHERAL_INTERRUPT_0));
    NOTICE("PER_PERIPHERAL_INTERRUPT_1=0x%x\n",mmio_read_32(PER_PERIPHERAL_INTERRUPT_1));


    mmio_write_32(PER_TMR_LD2, 0x000fffff);
    mmio_write_32(PER_TMR_LOADE, (1<<2));
    mmio_write_32(PER_TMR_IE2_0, 1);
    mmio_write_32(PER_TMR_CTRL2, ((1<<7)|(1<<6)));
    while(!mmio_read_32(PER_TMR_INT2_0));
    NOTICE("PER_TMR_CNT2=0x%x, PER_TMR_STAT2=%d\n",mmio_read_32(PER_TMR_CNT2), mmio_read_32(PER_TMR_STAT2));
    NOTICE("PER_PERIPHERAL_INTERRUPT_0=0x%x\n",mmio_read_32(PER_PERIPHERAL_INTERRUPT_0));
    NOTICE("PER_PERIPHERAL_INTERRUPT_1=0x%x\n",mmio_read_32(PER_PERIPHERAL_INTERRUPT_1));


    mmio_write_32(PER_TMR_LD3, 0x000fffff);
    mmio_write_32(PER_TMR_LOADE, (1<<3));
    mmio_write_32(PER_TMR_IE3_1, 1);
    mmio_write_32(PER_TMR_CTRL3, ((1<<7)));
    while(!mmio_read_32(PER_TMR_INT3_1));
    NOTICE("PER_TMR_CNT3=0x%x, PER_TMR_STAT3=%d\n",mmio_read_32(PER_TMR_CNT3), mmio_read_32(PER_TMR_STAT3));
    NOTICE("PER_PERIPHERAL_INTERRUPT_0=0x%x\n",mmio_read_32(PER_PERIPHERAL_INTERRUPT_0));
    NOTICE("PER_PERIPHERAL_INTERRUPT_1=0x%x\n",mmio_read_32(PER_PERIPHERAL_INTERRUPT_1));


    mmio_write_32(PER_TMR_LD4, 0x000fffff);
    mmio_write_32(PER_TMR_LOADE, (1<<4));
    mmio_write_32(PER_TMR_IE4_1, 1);
    mmio_write_32(PER_TMR_CTRL4, ((1<<7)|(1<<6)));
    while(!mmio_read_32(PER_TMR_INT4_1));
    NOTICE("PER_TMR_CNT4=0x%x, PER_TMR_STAT4=%d\n",mmio_read_32(PER_TMR_CNT4), mmio_read_32(PER_TMR_STAT4));
    NOTICE("PER_PERIPHERAL_INTERRUPT_0=0x%x\n",mmio_read_32(PER_PERIPHERAL_INTERRUPT_0));
    NOTICE("PER_PERIPHERAL_INTERRUPT_1=0x%x\n",mmio_read_32(PER_PERIPHERAL_INTERRUPT_1));
#endif

}

void bl1_early_platform_setup(void)
{
        plat_venus_bl1_early_platform_setup();
        HWSIM_PROGRESS_MARK(10);

        /*
         * Initialize CCI for this cluster during cold boot.
         * No need for locks as no other CPU is active.
         */

	/* use new ARM cci driver */
        plat_cci_init();
        HWSIM_PROGRESS_MARK(11);
        /*
         * Enable CCI coherency for the primary CPU's cluster.
         */
        cci_enable_snoop_dvm_reqs(MPIDR_AFFLVL1_VAL(read_mpidr()));
        HWSIM_PROGRESS_MARK(12);

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
    HWSIM_PROGRESS_MARK(33);
    HWSIM_PROGRESS_VAL(venus_global_strap.wrd);


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
        //venus_global_strap.bf.jtag_osc_2 = 1; // temp force
        //venus_global_strap.bf.jtag_osc_3 = 1; // temp force
        if(((otp_programmed == 0) && \
                    (venus_global_strap.bf.jtag_osc_2==1) && \
                    (venus_global_strap.bf.jtag_osc_3==1)) || \
                ((otp_programmed == 1) && \
                 (mfg_cli_enable == 1))) {
            HWSIM_PROGRESS_MARK(0x46);
            bl_cli();
        }
    }
#endif /* CORTINA_OTP_SUPPORT */
#endif /* CORTINA_MFG_CLI_SUPPORT */


        HWSIM_PROGRESS_MARK(31);
        configure_mmu_el3(bl1_tzram_layout.total_base,
                              bl1_tzram_layout.total_size,
                              BL1_RO_BASE,
                              BL1_RO_LIMIT
#if USE_COHERENT_MEM
                              , BL1_COHERENT_RAM_BASE,
                              BL1_COHERENT_RAM_LIMIT
#endif
                             );
        HWSIM_PROGRESS_MARK(32);
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
static uint64_t rand(unsigned int data)
{
	int64_t quotient, remainder, t;

	quotient = data / 127773;
	remainder = data % 127773;
	t = 16807 * remainder - 2836 * quotient;
	if (t <= 0)
		t += RANDOM_MAX;
	return (t % ((uint64_t)RANDOM_MAX + 1));
}
#endif


/*******************************************************************************
 * Function which will perform any remaining platform-specific setup that can
 * occur after the MMU and data cache have been enabled.
 ******************************************************************************/
void bl1_platform_setup(void)
{
	unsigned short cpu_speed_tab [] = { 400, 625, 700, 800, 900, 1000, 1100, 1200};

        HWSIM_PROGRESS_MARK(34);
	/* read global strap reg, and assign to global variable */
	venus_global_strap.wrd = mmio_read_32(GLOBAL_STRAP);

	NOTICE("BL1: CPU Speed %d MHz\n", cpu_speed_tab[venus_global_strap.bf.speed]);
        NOTICE("GLOBAL_STRAP 0x%x\n", mmio_read_32(GLOBAL_STRAP));
        HWSIM_PROGRESS_MARK(35);


#ifdef CORTINA_SECURE_SRAM
CORTINA_SECURE_SRAM_DRV_t * pSecureSRAMDrv;
#endif /* CORTINA_SECURE_SRAM */

#ifdef CORTINA_OTP_SUPPORT
    CORTINA_OTP_DRV_t * pOtpDrv = NULL;
#ifdef CORTINA_SECURE_JTAG_SUPPORT
    CORTINA_SECURE_JTAG_DRV_t * pSecureJTAGDrv;


    uint8_t jtag_password[CORTINA_SECURE_JTAG_PASSWORD_LEN];
    uint8_t jtag_password_inhibit = 0;
#endif /* CORTINA_SECURE_JTAG_SUPPORT */
    uint8_t programmed = 0;
    uint8_t copy_ssk_to_sadb = 0;
    uint8_t debug_override = 0;
    uint8_t debug_debugen = 0;
    uint8_t debug_cpu_niden = 0;
    uint8_t debug_cpu_spiden = 0;
    uint8_t debug_cpu_spinden = 0;
    uint8_t debug_auth_cssys_dbgen = 0;
    uint8_t debug_auth_cssys_niden = 0;
    uint8_t debug_cs_auth_niden = 0;
    uint8_t debug_cs_auth_spinden = 0;
    uint8_t debug_cfg_tpctl = 0;
    uint8_t debug_cfg_tpmaxdatasize = 0;

#ifdef LIB_GF_SCRAMBLE
    uint8_t scramble_seed;
    uint8_t scrambled_jtag_password = 0;
#endif /* LIB_GF_SCRAMBLE */

#endif /* CORTINA_OTP_SUPPORT */

	/* TODO VENUS GPIO Init */
	/* TODO VENUS PLL Init */
	/* TODO VENUS SD EMMC Init */
#ifdef CORTINA_SECURE_SRAM
	/* initialize Secure SRAM driver */
        HWSIM_PROGRESS_MARK(36);
	pSecureSRAMDrv = cortina_secure_sram_init((uintptr_t) CAPSRAM_TZCONTROL,
					     (uintptr_t) SRAM_BASE,
					     CORTINA_SECURE_SRAM_NO_CLEAR);

        HWSIM_PROGRESS_MARK(37);
	/* protect the entire SRAM */
	if(pSecureSRAMDrv != NULL)
	 cortina_secure_sram_protect_range( pSecureSRAMDrv,
					SRAM_BASE,
					SRAM_SIZE,
					CORTINA_SECURE_SRAM_NO_CLEAR);

        HWSIM_PROGRESS_MARK(38);
#endif /* CORTINA_SECURE_SRAM */

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

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_copy_ssk_to_sadb_BSTART,
            OTP_DATA_copy_ssk_to_sadb_BLEN,
            (uintptr_t) &copy_ssk_to_sadb);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_override_BSTART,
            OTP_DATA_debug_override_BLEN,
            (uintptr_t) &debug_override);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_debugen_BSTART,
            OTP_DATA_debug_debugen_BLEN,
            (uintptr_t) &debug_debugen);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_cpu_niden_BSTART,
            OTP_DATA_debug_cpu_niden_BLEN,
            (uintptr_t) &debug_cpu_niden);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_cpu_spiden_BSTART,
            OTP_DATA_debug_cpu_spiden_BLEN,
            (uintptr_t) &debug_cpu_spiden);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_cpu_spinden_BSTART,
            OTP_DATA_debug_cpu_spinden_BLEN,
            (uintptr_t) &debug_cpu_spinden);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_auth_cssys_dbgen_BSTART,
            OTP_DATA_debug_auth_cssys_dbgen_BLEN,
            (uintptr_t) &debug_auth_cssys_dbgen);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_auth_cssys_niden_BSTART,
            OTP_DATA_debug_auth_cssys_niden_BLEN,
            (uintptr_t) &debug_auth_cssys_niden);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_cs_auth_niden_BSTART,
            OTP_DATA_debug_cs_auth_niden_BLEN,
            (uintptr_t) &debug_cs_auth_niden);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_cs_auth_spinden_BSTART,
            OTP_DATA_debug_cs_auth_spinden_BLEN,
            (uintptr_t) &debug_cs_auth_spinden);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_cfg_tpctl_BSTART,
            OTP_DATA_debug_cfg_tpctl_BLEN,
            (uintptr_t) &debug_cfg_tpctl);

    cortina_otp_shadow_get_field(
            pOtpDrv,
            OTP_DATA_debug_cfg_tpmaxdatasize_BSTART,
            OTP_DATA_debug_cfg_tpmaxdatasize_BLEN,
            (uintptr_t) &debug_cfg_tpmaxdatasize);

#endif  /* CORTINA_OTP_SUPPORT */

#ifdef CORTINA_SECURE_JTAG_SUPPORT
    TEST_INFO("GLOBAL_STRAP debug_mode %d\n", venus_global_strap.bf.debug_mode);
    TEST_INFO("OTP fixed bitfield programmed %d\n", programmed);
    TEST_INFO("OTP fixed bitfield jtag_password_inhibit %d\n", jtag_password_inhibit);

	/* check if we should descramble ssk and copy to crypto engine */
	if (copy_ssk_to_sadb) {
		uint8_t df3 = 0;
		cortina_otp_shadow_field_is_programmed(pOtpDrv,
										  (uint32_t) OTP_DATA_df3_BSTART,
										  (uint32_t) OTP_DATA_df3_BLEN, &df3);
		uint8_t *ssk =
			(uint8_t *) (OTP_SHADOW_BASE +
						 OTP_DATA_spare128_device_serial_no_BSTART / 8);
		uint8_t ssk_descrambled[32];
		if (df3 != 0) {
			ssk = ssk_descrambled;
                        TEST_INFO("ssk descrambling\n");
                        TEST_INFO("read scrambled ssk\n");
                        /* read scramble seed, based on lower byte of serial number */
			scramble_seed =
				*(uint8_t *) (OTP_SHADOW_BASE +
							  OTP_DATA_spare128_device_serial_no_BSTART / 8);
			if (scramble_seed == 0x0)
				scramble_seed = 100;
			/* descramble password, using seed of 100 */
			scramble_gf(1,
						scramble_seed,
						(unsigned char *)(OTP_SHADOW_BASE +
						(OTP_DATA_secure_storage_key_BSTART /8)),
						(unsigned char *)ssk, 32);
                        TEST_INFO("de-scrambed ssk in OTP\n");
#ifdef TEST_OTP
                        memdump_byte((char *)ssk, 32);
#endif
		}
		// write descramble ssk into crypto
		/*
		   Programming sequence for AES-128-CBC mode
		   0xF630_0000   - 32-bits key {byte0,byte1,byte2,byte3}
		   0xF630_0004   - 32-bits data (control 12-bits) in the order of { cipher mode,encrypt enable,key size, cipher algorithm}
		   Cipher –algorithm – 3’b010 –for (AES)
		   Key size                -- 2’b00 –for (16 bytes)
		   Encrypt enable      -- 1’b1
		   Cipher mode         -- 4’b0001 (CBC)
		   Data written is     -- 32’h0000_0062
		   0xF630_0008   - 32-bits key {byte4,byte5,byte6,byte7}
		   0xF630_000C   - 32-bits control word (same as written in address 0xF630_0004)
		   0xF630_0010   - 32-bits key {byte8,byte9,byte10,byte11}
		   0xF630_0014   - 32-bits control word (same as written in address 0xF630_0004)
		   0xF630_0018   - 32-bits key {byte12,byte13,byte14,byte15}
		   0xF630_001C   - 32-bits control word (same as written in address 0xF630_0004)
		   0xF630_0020   - 32-bits key {byte16,byte17,byte18,byte19}
		   0xF630_0024   - 32-bits control word (same as written in address 0xF630_0004)
		   0xF630_0028   - 32-bits key {byte20,byte21,byte22,byte23}
		   0xF630_002C   - 32-bits control word (same as written in address 0xF630_0004)
		   0xF630_0030   - 32-bits key {byte24,byte25,byte26,byte27}
		   0xF630_0034   - 32-bits control word (same as written in address 0xF630_0004)
		   0xF630_0038   - 32-bits key {byte28,byte29,byte30,byte31}
		   0xF630_003C   - 32-bits control word (same as written in address 0xF630_0004)
		 */
		{
			typedef union {
				struct {
					uint32_t calg:3;	/* bits 2:0 */
					uint32_t key_sz:2;	/* bits 4:3 */
					uint32_t encrypt:1;	/* bits 5:5 */
					uint32_t cmode:4;	/* bits 9:6 */
					uint32_t secure:1;	/* bits 10 */
					uint32_t reserve:21;	/* bits 31:11 */
				} bf;
				struct {
					uint32_t sw1:16;
					uint32_t sw2:16;
				} swords;
				uint32_t wrd;
			} CORTINA_SADB_CTRL_t;
			CORTINA_SADB_CTRL_t sadb_ctrl;
			uint32_t *ssk_word = (uint32_t *) ssk;
			uint32_t  key_reord, sadb_base, vspacc_instance;

#if 0
			sadb_ctrl.bf.calg = 0x2;	// AES
			sadb_ctrl.bf.key_sz = 0;	// for 16 bytes, not 32 bytes
			sadb_ctrl.bf.encrypt = 1;	// enabled
			sadb_ctrl.bf.cmode = 0;	// cbc
			sadb_ctrl.bf.secure = 1;	// secured ?
			sadb_ctrl.bf.reserve = 0;	// reserved
#endif
			// read control paramters from ssk_sadb_control_word
			unsigned short sw1_in_ssk_sadb_control_word =
				*(unsigned short *)(OTP_SHADOW_BASE +
									OTP_DATA_ssk_sadb_control_word_BSTART / 8);
			sadb_ctrl.swords.sw1 = sw1_in_ssk_sadb_control_word;

			sadb_ctrl.swords.sw2 = 0;
#define swab32(x) ((uint32_t)(				\
	(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |		\
	(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |		\
	(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |		\
	(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))

			vspacc_instance = 0;
			for(vspacc_instance=0; vspacc_instance < 3; vspacc_instance++){
				sadb_base = 0xF6300000 + (vspacc_instance * 0x400);

				key_reord = swab32(ssk_word[0]);
				mmio_write_32(sadb_base, key_reord);
				mmio_write_32(sadb_base + 4, sadb_ctrl.wrd);

				key_reord = swab32(ssk_word[1]);
				mmio_write_32(sadb_base + 8, key_reord);
				mmio_write_32(sadb_base + 12, sadb_ctrl.wrd);

				key_reord = swab32(ssk_word[2]);
				mmio_write_32(sadb_base + 16, key_reord);
				mmio_write_32(sadb_base + 20, sadb_ctrl.wrd);

				key_reord = swab32(ssk_word[3]);
				mmio_write_32(sadb_base + 24, key_reord);
				mmio_write_32(sadb_base + 28, sadb_ctrl.wrd);

				key_reord = swab32(ssk_word[4]);
				mmio_write_32(sadb_base + 32, key_reord);
				mmio_write_32(sadb_base + 36, sadb_ctrl.wrd);

				key_reord = swab32(ssk_word[5]);
				mmio_write_32(sadb_base + 40, key_reord);
				mmio_write_32(sadb_base + 44, sadb_ctrl.wrd);

				key_reord = swab32(ssk_word[6]);
				mmio_write_32(sadb_base + 48, key_reord);
				mmio_write_32(sadb_base + 52, sadb_ctrl.wrd);

				key_reord = swab32(ssk_word[7]);
				mmio_write_32(sadb_base + 56, key_reord);
				mmio_write_32(sadb_base + 60, sadb_ctrl.wrd);
			}
		}
	}


	/* check if we should set password to all ZEROs */
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
        (venus_global_strap.bf.debug_mode == 0x1) ||
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
        (venus_global_strap.bf.debug_mode == 0x1) ||
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
    pSecureJTAGDrv = cortina_secure_jtag_init((uintptr_t) (GLOBAL_DEVICE_ASSIGN_ID1));
	if(pSecureJTAGDrv != NULL){
        cortina_secure_jtag_set_password(pSecureJTAGDrv, jtag_password, CORTINA_SECURE_JTAG_PASSWORD_LEN);
        HWSIM_PROGRESS_MARK(42);
    }


#endif /* CORTINA_SECURE_JTAG_SUPPORT */

       /* VENUS ARM DEBUG OVERRIDE */
#if (CORTINA_OTP_SUPPORT == OTP_MODE_EM )
	if(cortina_otp_fixed_region_word1.bf.debug_override_n == 0x0)
#elif (CORTINA_OTP_SUPPORT == OTP_MODE_KP )
	if(debug_override == 0x1)
#endif /* CORTINA_OTP_SUPPORT OTP_MODE */
         {
          venus_global_arm_debug.wrd = mmio_read_32(GLOBAL_ARM_DEBUG);
          NOTICE("GLOBAL_ARM_DEBUG reg 0x%x\n", venus_global_arm_debug.wrd);
          venus_global_arm_debug.bf.cpu_dbgen = debug_debugen;
          venus_global_arm_debug.bf.cpu_niden = debug_cpu_niden;
          venus_global_arm_debug.bf.cpu_spiden = debug_cpu_spiden;
          venus_global_arm_debug.bf.cpu_spinden = debug_cpu_spinden;
          venus_global_arm_debug.bf.auth_cssys_dbgen = debug_auth_cssys_dbgen;
          venus_global_arm_debug.bf.auth_cssys_niden = debug_auth_cssys_niden;
          venus_global_arm_debug.bf.cs_auth_niden = debug_cs_auth_niden;
          venus_global_arm_debug.bf.cs_auth_spinden = debug_cs_auth_spinden;
          venus_global_arm_debug.bf.cfg_tpctl = debug_cfg_tpctl;
          venus_global_arm_debug.bf.cfg_tpmaxdatasize = debug_cfg_tpmaxdatasize;

          mmio_write_32(GLOBAL_ARM_DEBUG, venus_global_arm_debug.wrd);
	  /* sync */
	  dsb();
	  isb();
          TEST_INFO("GLOBAL_ARM_DEBUG reg updated 0x%x\n", venus_global_arm_debug.wrd);
         }
        HWSIM_PROGRESS_MARK(43);

#ifdef CORTINA_BOOT_EMMC
#ifdef CORTINA_SECURE_SRAM
	/* since eMMC can not do secure access to its descriptors and buffers
         * we need to unprotect these in secure SRAM when in BL1
         */
	if(pSecureSRAMDrv != NULL)
	 cortina_secure_sram_unprotect_range( pSecureSRAMDrv,
					CORTINA_BL1_MMC_DESC_BASE,
					CORTINA_SECURE_SRAM_ROUND_UP_16KB(CORTINA_BL1_MMC_DESC_SIZE + CORTINA_BL1_MMC_DATA_SIZE),
					CORTINA_SECURE_SRAM_ALL_CLEAR);
#endif /* CORTINA_SECURE_SRAM */
#endif /* CORTINA_BOOT_EMMC */

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
 * the entrypoint of BL2 and set SPSR and security state.
 * On Juno we are only setting the security state, entrypoint
 ******************************************************************************/
void bl1_plat_set_bl2_ep_info(image_info_t *bl2_image,
			      entry_point_info_t *bl2_ep)
{
	SET_SECURITY_STATE(bl2_ep->h.attr, SECURE);
	bl2_ep->spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
}

/* This function is called just before we leave BL1 */

void bl1_plat_prepare_exit(entry_point_info_t *ep_info)
{
/* Keep WDT alive, so that system reboot in case BL2/BL31/BL33 fail to boot */
#if 0
	CORTINA_PERWATCHDOG_DRV_t * pWatchdogDrv;
        /* Disable watchdog before leaving BL1 */
	pWatchdogDrv = cortina_per_watchdog_get_drv_instance(0);

	if(pWatchdogDrv != NULL)
         cortina_per_watchdog_disable_sys_reset(pWatchdogDrv);
#endif /* CORTINA_PER_WATCHDOG */

}

