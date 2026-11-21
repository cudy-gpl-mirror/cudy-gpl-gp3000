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
GLOBAL_STRAP_t g3_global_strap;
GLOBAL_GLOBAL_CONFIG_t g3_global_cfg;
GLOBAL_ARM_DEBUG_t g3_global_arm_debug;
#ifdef CORTINA_OTP_SUPPORT
CORTINA_OTP_DRV_t * cortina_pOtpDrv;
#endif /* CORTINA_OTP_SUPPORT */

#ifdef CORTINA_OTP_SUPPORT
OTP_FIXED_REGION1_32BIT_WORD1_MAP_t cortina_otp_fixed_region_word1;
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

meminfo_t *bl1_plat_sec_mem_layout(void)
{
	return &bl1_tzram_layout;
}


/*
 * BL1 OTP Hotfix Checkpoint
 * This function will compare the checkpoint id from the caller with the hotfix tab entries.
 * If find a match, depending on the sram flag in the hotfix table entry,
 * it will call a corrospoding hotfix code from OTP shadow or SRAM.
 * If in OTP shadow, the offset starts from the begining of region 2, that is, bit location at 1472, that is, 0xC003f0b8
 * If in SRAM, the offset starts from the begining of the last 16KB, that is, 0xC003C000
 * The offset is aligned with a multiple of 2**3 or 2**5 for hotfix code in OTP shadow or SRAM repecively.
 * Parameters:
 *    Checkpoint ID: range 1 to 126. should not be 0 or 127
 *    A pointer or a 64-bit data
 * Return:
 *    0 : if no hotfix code found, or found but not executed when the hotfix code signature in SRAM does not match.
 *    1 : hotfix code found and executed
 */   
volatile uint64_t hotfix_code_in_sram_base = (SRAM_BASE + SRAM_SIZE - 16 * 1024); // can be changed via jtag or other hotfix
unsigned int cortina_bl1_hotfix_checkpoint(uint32_t checkpoint_id, void *data)
{
        CORTINA_OTP_DRV_t * pOtpDrv;
        //uint64_t caller_addr; 
        //uint32_t checkpoint_addr, hotfix_addr;
        uint64_t hotfix_addr = 0;
        uint32_t i;
        typedef void func(void *);
        func * hotfix_func;
        BL1_hotfix_entry_t *hotfix_tab = 0;

        HWSIM_PROGRESS_MARK(0x51);
        TEST_INFO("cortina_otp_bl1_hotfix_checkpoint In checkpoint_id %d data %llx\n", checkpoint_id, (long long unsigned int) data);
        TEST_INFO("hotfix checkpoint num entries %d\n", BL1_HOTFIX_CHECKPOINT_NUM_ENTRIES);

        pOtpDrv = cortina_otp_get_drv_instance(0);
        if(pOtpDrv == NULL)
        {
                ERROR("ERROR: No Otp Driver Handle, can't search for hotfix id %d\n", (uint32_t) checkpoint_id);
                return 0;
        }
        TEST_INFO("Searching for hotfix id %d\n", (uint32_t) checkpoint_id);
        TEST_INFO("pOtpDrv->shadow_base_addr %llx\n", (long long unsigned int) pOtpDrv->shadow_base_addr);
        hotfix_tab = (BL1_hotfix_entry_t *) ((long long unsigned int) pOtpDrv->shadow_base_addr + OTP_DATA_bl1_hotfix_checkpoint_table_BSTART/8);
        TEST_INFO("hotfix_tab at %llx\n", (long long unsigned int)hotfix_tab);
        TEST_INFO("hotfix start in OTP from %llx\n", (long long unsigned int)pOtpDrv->shadow_base_addr + OTP_REGION2_BSTART/8);
        TEST_INFO("hotfix start in SRAM from %llx, change the base at 0x%llx\n",
                  (unsigned long long int)hotfix_code_in_sram_base, (unsigned long long int) &hotfix_code_in_sram_base);
   
        HWSIM_PROGRESS_MARK(0x52);
        for (i = 0; i < BL1_HOTFIX_CHECKPOINT_NUM_ENTRIES ; i++) {
                TEST_INFO("check hotfix_tab entry %d\n", i);
                if ((hotfix_tab[i].bf.checkpoint_id == 0) || (hotfix_tab[i].bf.checkpoint_id == 0x7f)) {
                        continue;
                }
                if (checkpoint_id == hotfix_tab[i].bf.checkpoint_id) {
                        NOTICE("Found matched checkpoint_id %d at hotfix_tab entry %d\n", checkpoint_id, i);
                        NOTICE("Matched: in_sram %d, offset 0x%x\n", hotfix_tab[i].bf.in_sram, hotfix_tab[i].bf.hotfix_offset);
                        if (hotfix_tab[i].bf.in_sram) {
                                hotfix_addr = hotfix_code_in_sram_base + (hotfix_tab[i].bf.hotfix_offset << 5); 
                                if (BL1_HOTFIX_SIGNATURE != ((unsigned int *)(hotfix_addr))[0]) {
                                        ERROR("Hotfix found but signature check failed for id %d\n", checkpoint_id);
                                        return 0;
                                }
                                hotfix_func = (func *) ((uint64_t) hotfix_addr + 4);
                        } else {
                                // offset is from the first free available OTP addr
                                hotfix_addr = (uint64_t) pOtpDrv->shadow_base_addr + OTP_REGION2_BSTART/8 + (hotfix_tab[i].bf.hotfix_offset << 3); 
                                hotfix_func = (func *) ((uint64_t) hotfix_addr);
                        }
                        NOTICE("Hotfix at %llx to be called\n", (long long unsigned int) hotfix_func);
                        hotfix_func(data);
                        HWSIM_PROGRESS_MARK(0x53);
                        return 1; // found and executed
                }
        }
        TEST_INFO("No hotfix matches found for %d\n", checkpoint_id); 
        HWSIM_PROGRESS_MARK(0x54);

        return 0;
}


#if defined(TEST_OTP) || defined(TEST_ON_CORTINA_ASIC_SIM) && defined(TEST_ON_CORTINA_ASIC_SIM_HOTFIX)
char *msg1 = "fun called from hotfix code msg 1"; 
char *msg2 = "fun called from hotfix code msg 2"; 
char *msg3 = "fun called from hotfix code msg 3"; 
 
//void foo(char *m, void *ptr) { 
//  int (*func)(char *) = (int(*)(char*))ptr;
//  func(m);
//}
// foo1 const is the same as foo1() 
const unsigned int foo1[] = {
        0xf9400001,
        0xf9400400,
        0xd61f0020
};

#if 0
// keep this code here to understand how the above binary code foo1 is equivalent to
void foo1(void *data) {
        unsigned long long int *p = (unsigned long long int *)data;
        char *msg = (char *)p[1];
        int (*func)(char *) = (int(*)(char*)) p[0];
        func(msg);
}
#endif

unsigned long long int params[2];
void test_hotfix(void) { 
        params[0] = (unsigned long long int) puts;
        params[1] = (unsigned long long int) msg1;
        BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_TEST1, &params[0]);
}

#endif

#ifdef TEST_OTP

// use Cortina ROTPK in TEST_OTP test
static const unsigned char arm_rotpk_test_hash[] = {
        0xf5,0x52,0xe5,0xbd,0xf3,0x68,0xa0,0x4d,
        0x9d,0xb7,0x78,0x59,0x9d,0xfe,0x84,0x02,
        0xef,0x30,0xfd,0xbb,0x32,0xa3,0xef,0xbe,
        0x21,0xc5,0xae,0xd3,0x30,0x7c,0x95,0x14
};

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

        INFO("OTP OTP_BAD_BITMAP_SIZE %d\n", OTP_BAD_BITMAP_SIZE);

        INFO("OTP OTP_BIT_SIZE %d\n", OTP_BIT_SIZE);
        INFO("OTP FIXED_REGION_KEYS  bit loc %d\n", OTP_DATA_FIXED_REGION_KEYS);
        INFO("OTP FIXED_REGION_WORD1 bit loc %d\n", OTP_DATA_FIXED_REGION_WORD1);
        INFO("OTP FIXED_REGION_WORD2 bit loc %d\n", OTP_DATA_FIXED_REGION_WORD2);
        INFO("OTP FIXED_REGION_WORD3 bit loc %d\n", OTP_DATA_FIXED_REGION_WORD3);
        INFO("OTP hotfix_checkpoint_table bit loc %d blen %d\n", OTP_DATA_bl1_hotfix_checkpoint_table_BSTART, OTP_DATA_bl1_hotfix_checkpoint_table_BLEN);
        INFO("OTP Region # 1 End at %d\n", OTP_DATA_bl1_hotfix_checkpoint_table_BSTART + OTP_DATA_bl1_hotfix_checkpoint_table_BLEN - 1); 
        INFO("OTP Region # 2 range from %d to %d\n", 
             OTP_DATA_bl1_hotfix_checkpoint_table_BSTART + OTP_DATA_bl1_hotfix_checkpoint_table_BLEN,
             OTP_DATA_non_trusted_NV_min_ver_BSTART - 1);
        INFO("OTP non_trusted_NV_min_ver bit loc %d blen %d\n", OTP_DATA_non_trusted_NV_min_ver_BSTART, OTP_DATA_non_trusted_NV_min_ver_BLEN);
        INFO("OTP trusted_NV_min_ver bit loc     %d blen %d\n", OTP_DATA_trusted_NV_min_ver_BSTART, OTP_DATA_trusted_NV_min_ver_BLEN);
        INFO("OTP Region # 3 End %d\n", OTP_DATA_trusted_NV_min_ver_BSTART + OTP_DATA_trusted_NV_min_ver_BLEN -1);

        INFO("BL1_RW_BASE 0x%x BL1_RW_LIMIT 0x%x BL1_RW_SIZE 0x%x BL1_RW_SIZE_MAX 0x%x\n", BL1_RW_BASE, BL1_RW_LIMIT, BL1_RW_SIZE, BL1_RW_SIZE_MAX);
        INFO("PLAT_G3_BL_RAM_BASE 0x%x PLAT_G3_BL_RAM_SIZE 0x%x\n", PLAT_G3_BL_RAM_BASE, PLAT_G3_BL_RAM_SIZE);

        INFO("OTP OTP_DATA_FIXED_REGION_WORD1 dump\n");
        memdump_byte(&otp_base[OTP_DATA_FIXED_REGION_WORD1/8], (OTP_DATA_FIXED_REGION_WORD2 - OTP_DATA_FIXED_REGION_WORD1)/8);
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
#define TEST_OTP_SECURE_WATHDOG_ENABLE
#ifdef TEST_OTP_SECURE_WATHDOG_ENABLE
        byte_offset =   OTP_DATA_secure_watchdog_enable_BSTART / 8;
        bit_pos = OTP_DATA_secure_watchdog_enable_BSTART % 8;
        otp_base[byte_offset] |= (1<<bit_pos);
#endif // TEST_OTP_SECURE_WATHDOG_ENABLE
#endif // TEST_OTP_DEBUG_OVERRIDE

       // enable Multiboot OSB 2 bit Reduced Pattern
#define TEST_OTP_MULTIBOOT_OSB_REDUCED_PATTERN
#ifdef TEST_OTP_MULTIBOOT_OSB_REDUCED_PATTERN
        byte_offset =   OTP_DATA_one_shot_boot_reduced_pattern_BSTART / 8;
        bit_pos = OTP_DATA_one_shot_boot_reduced_pattern_BSTART % 8;
        otp_base[byte_offset] |= (1<<bit_pos);
#endif // TEST_OTP_MULTIBOOT_OSB_REDUCED_PATTERN
        
        // enable the following when test ROTPK to preload OTP ROTPK with the test hash value
#define TEST_OTP_HASH_OF_ROTPK
#ifdef TEST_OTP_HASH_OF_ROTPK
        byte_offset = OTP_DATA_hash_of_ROT_public_key_BSTART / 8;
        memcpy(&otp_base[byte_offset],  &arm_rotpk_test_hash[0], 32); 
#endif // TEST_OTP_HASH_OF_ROTPK

#define TEST_OTP_ALLOW_BL1_NV_CNTR_WRITES
#ifdef TEST_OTP_ALLOW_BL1_NV_CNTR_WRITES
        byte_offset = OTP_DATA_allow_bl1_nv_cntr_writes_BSTART / 8;
        bit_pos =  OTP_DATA_allow_bl1_nv_cntr_writes_BSTART % 8;
        otp_base[byte_offset] |= (0x1<<bit_pos);
#endif // TEST_OTP_ALLOW_BL1_NV_CNTR_WRITES

#define TEST_OTP_NV_CNTR
#ifdef TEST_OTP_NV_CNTR
        // load non-secure nv ctr 3 and secure nv ctr 2
        byte_offset = OTP_DATA_non_trusted_NV_min_ver_BSTART / 8;
        otp_base[byte_offset] = 0x7; // 3, that is, 0b00000111
        byte_offset = OTP_DATA_trusted_NV_min_ver_BSTART / 8;
        otp_base[byte_offset] = 0x3; // 2, that is, 0b00000011
#endif // TEST_OTP_NV_CNTR
        
// preload forced_boot_order list 
#define TEST_OTP_FORCED_BOOT_ORDER
#ifdef TEST_OTP_FORCED_BOOT_ORDER
        byte_offset = OTP_DATA_forced_boot_order_BSTART/8;
        otp_base[byte_offset] = (CORTINA_BOOT_DEVICE_PARALLEL_NAND<<1|0) + ((CORTINA_BOOT_DEVICE_SERIAL_NOR<<1|1)<<4); // PARALLEL_NAND part0, SERIAL_NOR part1
        otp_base[byte_offset+1] = (CORTINA_BOOT_DEVICE_MMC<<1|0) + ((CORTINA_BOOT_DEVICE_DRAM_DEV_MEMMAP<<1|1)<<4); // MMC part0, MEMMAP part0
#endif // TEST_OTP_FORCED_BOOT_ORDER

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


#define TEST_OTP_DUMP_OTP_INFO
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

#define TEST_OTP_JTAG_PASSWORD_SCRAMBLING
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

        // enable secure watch dog timer
#define TEST_OTP_ENBABLE_WDT
#ifdef TEST_OTP_ENBABLE_WDT
        byte_offset =   OTP_DATA_secure_watchdog_enable_BSTART / 8;
        bit_pos = OTP_DATA_secure_watchdog_enable_BSTART % 8;
        otp_base[byte_offset] |= (1<<bit_pos);
#endif // TEST_OTP_ENBABLE_WDT


// test scrambled and descrabled SSK
#define TEST_OTP_SCRAMBLED_SSK
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

        INFO("OTP OTP_DATA_FIXED_REGION_WORD1 dump\n");
        memdump_byte(&otp_base[OTP_DATA_FIXED_REGION_WORD1/8], (OTP_DATA_FIXED_REGION_WORD2 - OTP_DATA_FIXED_REGION_WORD1)/8);

        INFO("OTP DUMP ALL 1024:\n");
        memdump(&otp_base[0], 1024);
        
}
#endif // TEST_OTP

#ifdef TEST_ON_CORTINA_ASIC_SIM
#ifdef TEST_ON_CORTINA_ASIC_SIM_OTP_WRITE
uint8_t bitstream_buf[16];
uint8_t otp_readback_buf[16];
void do_test_on_asic_sim_otp_write(CORTINA_OTP_DRV_t *pOtpDrv) {
        // otp write test here - do not preload any OTP patterns in the asic sim
        // clear some bits - disable OTP PROT_EN
        HWSIM_PROGRESS_MARK(0x2000); // a mark for the test start
        if (cortina_otp_set_protection_range(pOtpDrv, 0, 0) == OTP_ERRNO_OK) {
                HWSIM_PROGRESS_MARK(0x2001); // OTP prot en OK
        } else {
                HWSIM_PROGRESS_MARK(0x2002); // OTP prot en Failed, test aborted
                for(;;);
        }

        bitstream_buf[0]=0x55;
        bitstream_buf[1]=0xAA;
        cortina_otp_program_bits(pOtpDrv, 0, 15, bitstream_buf); // expect 00002a55 in OTP

        HWSIM_PROGRESS_MARK(0x2003);
        unsigned int *p_shadow = (unsigned int *)OTP_SHADOW_BASE;
        HWSIM_PROGRESS_VAL(p_shadow[0]); // expect 00002a55
        
        // read back the 4 bytes from OTP 
        /* power up for read access */
        extern int cortina_otp_read_byte(CORTINA_OTP_DRV_t * pOtpDrv, uint32_t addr, uint8_t * pBuf);
        extern int cortina_otp_read_active(CORTINA_OTP_DRV_t * pOtpDrv);
        cortina_otp_read_active(pOtpDrv);
        HWSIM_PROGRESS_MARK(0x2004);
        int rc = 0;
        for(int i=0; i < 4; i++)
        {
                HWSIM_PROGRESS_MARK(0x2005);
                rc = cortina_otp_read_byte(pOtpDrv, i, &otp_readback_buf[i]);
                HWSIM_PROGRESS_MARK(0x2006);
                HWSIM_PROGRESS_VAL(i);
                HWSIM_PROGRESS_VAL(otp_readback_buf[i]);
                HWSIM_PROGRESS_VAL(rc);
        }
        if ((otp_readback_buf[0] == 0x55) &&
            (otp_readback_buf[1] == 0x2a) && /* not 0xaa because we do not write bit 15 in otp */
            (otp_readback_buf[2] == 0) &&
            (otp_readback_buf[3] == 0)) {
                HWSIM_PROGRESS_FLAG_SET(31); // SUCCESS
        } else {
                HWSIM_PROGRESS_FLAG_SET(30); // FAILED
        }
        HWSIM_PROGRESS_MARK(0x2007); // OTP write test finish
        for(;;);
}        
#endif // TEST_ON_CORTINA_ASIC_SIM_OTP_WRITE

#ifdef TEST_ON_CORTINA_ASIC_SIM_OTP_SHADOW_COPY
void do_test_on_asic_sim_otp_shadow_copy(void) {
	// check the OTP pattern 
	// pattern is 0x5a5annnn for 64 or OTP_BIT_SIZE/8/4 bytes check
        // If verifying passed, bit 31 in GLOBAL_REGISTER is 1, otherwise bit 30 is 1
	unsigned int * p = (unsigned int *) OTP_SHADOW_BASE;
	unsigned int expected;
	int i;
        HWSIM_PROGRESS_MARK(0x3000); 
	for (i = 0; i < 16 /* that is 64 bytes */; i++) {
                HWSIM_PROGRESS_VAL(i);
                expected = 0x5a5a0000 + (i+1);
                if (expected != p[i]) {
                        HWSIM_PROGRESS_MARK(0x3001);
                        HWSIM_PROGRESS_VAL(expected);
                        HWSIM_PROGRESS_VAL(p[i]);
                        HWSIM_PROGRESS_FLAG_SET(30);
                        for(;;);
                }
        };
        HWSIM_PROGRESS_FLAG_SET(31);
        for(;;);
}
#endif // TEST_ON_CORTINA_ASIC_SIM_OTP_SHADOW_COPY

void do_test_on_asic_sim(CORTINA_OTP_DRV_t * pOtpDrv) {
#ifdef TEST_ON_CORTINA_ASIC_SIM_OTP_WRITE
        // otp write test is destructive, do not combine it with other tests
#ifdef TEST_ON_CORTINA_ASIC_SIM_OTP_SHADOW_COPY
#error Wrong use of combining TEST_ON_CORTINA_ASIC_SIM_OTP_WRITE and TEST_ON_CORTINA_ASIC_SIM_OTP_SHADOW_COPY
#endif // TEST_ON_CORTINA_ASIC_SIM_OTP_SHADOW_COPY
        do_test_on_asic_sim_otp_write(pOtpDrv);        // 
#endif // TEST_ON_CORTINA_ASIC_SIM_OTP_WRITE
#ifdef TEST_ON_CORTINA_ASIC_SIM_OTP_SHADOW_COPY
        do_test_on_asic_sim_otp_shadow_copy();  // with a preloaded pattern
#endif // TEST_ON_CORTINA_ASIC_SIM_OTP_SHADOW_COPY
}

#endif // TEST_ON_CORTINA_ASIC_SIM

/*******************************************************************************
 * Perform any BL1 specific platform actions.
 ******************************************************************************/
void plat_g3_bl1_early_platform_setup(void)
{
#ifdef CORTINA_OTP_SUPPORT
	CORTINA_OTP_DRV_t * pOtpDrv = NULL;
#endif /* CORTINA_OTP_SUPPORT */

        HWSIM_PROGRESS_MARK(0xffffffff);
	g3_global_cfg.wrd = mmio_read_32(GLOBAL_GLOBAL_CONFIG);
#ifdef CORTINA_PER_WATCHDOG
        CORTINA_PER_WATCHDOG_DRV_t * pWatchDogDrv;
	uint8_t secure_watchdog_enable = 0;
#endif /* CORTINA_PER_WATCHDOG */
#ifndef CORTINA_G3_ASIC
	DDR_CTRL_DDR_SCH_CTRL_t ddr_sch_ctrl;
#endif
	const size_t bl1_size = BL1_RAM_LIMIT - BL1_RAM_BASE;

	/* G3 System Timer Init */
	mmio_write_32(G3_SYS_TIMER_BASE, 0x1);

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
                        (uint32_t) (G3_LSAXI_CLK/1000000)
                       );
        HWSIM_PROGRESS_MARK(1);
	if (pOtpDrv == NULL) {
            HWSIM_PROGRESS_MARK(2);
	    console_init(CONSOLE_BASE, CORTINA_PER_UART_CLK_IN_HZ, CORTINA_PER_UART_BAUDRATE);
	    INFO("G3 OTP Driver Init FAILED\n");
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
        
	/* Initialize the console to provide early debug support */
	console_init(CONSOLE_BASE, CORTINA_PER_UART_CLK_IN_HZ, CORTINA_PER_UART_BAUDRATE);
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
	    g3_global_cfg.bf.wd_reset_ext_reset = 1;
	    g3_global_cfg.bf.wd_reset_remap = 1;
	    g3_global_cfg.bf.wd_reset_all_blocks= 1;
	    g3_global_cfg.bf.wd_reset_subsys_enable = 1;
	    mmio_write_32(GLOBAL_GLOBAL_CONFIG, g3_global_cfg.wrd);

            /* enable system reset if watchdog expires */
            cortina_per_watchdog_enable_sys_reset(pWatchDogDrv);
	  }
        HWSIM_PROGRESS_MARK(7);
#endif /* CORTINA_PER_WATCHDOG */

	/* Init DDR controller */
#ifndef CORTINA_G3_ASIC
	ddr_sch_ctrl.wrd = 0x01f74010;
	mmio_write_32(DDR_CTRL_DDR_SCH_CTRL, ddr_sch_ctrl.wrd);
#endif
	/* Allow BL1 to see the whole Trusted RAM */
	bl1_tzram_layout.total_base = PLAT_G3_BL_RAM_BASE;
	bl1_tzram_layout.total_size = PLAT_G3_BL_RAM_SIZE;

	/* Calculate how much RAM BL1 is using and how much remains free */
	bl1_tzram_layout.free_base = PLAT_G3_BL_RAM_BASE;
	bl1_tzram_layout.free_size = PLAT_G3_BL_RAM_SIZE;
        HWSIM_PROGRESS_MARK(8);
	reserve_mem(&bl1_tzram_layout.free_base,
		    &bl1_tzram_layout.free_size,
		    BL1_RAM_BASE,
		    bl1_size);
        HWSIM_PROGRESS_MARK(9);

	INFO("BL1: 0x%lx - 0x%lx [size = %lu]\n", BL1_RAM_BASE, BL1_RAM_LIMIT,
	     bl1_size);
        HWSIM_PROGRESS_MARK(28);
}

void bl1_early_platform_setup(void)
{
        plat_g3_bl1_early_platform_setup();
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
        HWSIM_PROGRESS_MARK(29);
#ifdef CORTINA_OTP_SUPPORT
	OTP_FIXED_REGION1_32BIT_WORD2_MAP_t cortina_otp_fixed_region_word2;
	g3_global_strap.wrd = mmio_read_32(GLOBAL_STRAP);
        HWSIM_PROGRESS_MARK(33);
        HWSIM_PROGRESS_VAL(g3_global_strap.wrd);
	cortina_otp_fixed_region_word1.wrd =
		mmio_read_32(OTP_SHADOW_BASE + OTP_DATA_FIXED_REGION_WORD1/8);
	cortina_otp_fixed_region_word2.wrd =
		mmio_read_32(OTP_SHADOW_BASE + OTP_DATA_FIXED_REGION_WORD2/8);
        HWSIM_PROGRESS_MARK(30);
        HWSIM_PROGRESS_VAL(cortina_otp_fixed_region_word1.wrd);
        HWSIM_PROGRESS_VAL(cortina_otp_fixed_region_word2.wrd);

	/* Wafter test mode
	  * Case#1: Blank OTP + jtag_osc
	  * Case#2: OTP programmed with mfg_cli_enable
	  */
#ifdef TEST_OTP
        //g3_global_strap.bf.jtag_osc_2 = 1; // temp force
        //g3_global_strap.bf.jtag_osc_3 = 1; // temp force
#endif
	if(((cortina_otp_fixed_region_word1.bf.programmed == 0) && \
			(g3_global_strap.bf.jtag_osc_2==1) && \
			(g3_global_strap.bf.jtag_osc_3==1)) || \
			((cortina_otp_fixed_region_word1.bf.programmed == 1) && \
                         cortina_otp_fixed_region_word2.bf.mfg_cli_enable ==1)) {
                HWSIM_PROGRESS_MARK(0x46);
		bl_cli();
        }
#endif
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
	OTP_FIXED_REGION1_32BIT_WORD2_MAP_t cortina_otp_fixed_region_word2;
	cortina_otp_fixed_region_word2.wrd =
	  mmio_read_32(OTP_SHADOW_BASE + OTP_DATA_FIXED_REGION_WORD2/8);
        HWSIM_PROGRESS_MARK(34);
	/* read global strap reg, and assign to global variable */
	g3_global_strap.wrd = mmio_read_32(GLOBAL_STRAP);  	

	NOTICE("BL1: CPU Speed %d MHz\n", cpu_speed_tab[g3_global_strap.bf.speed]);
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
#ifdef LIB_GF_SCRAMBLE
	uint8_t scramble_seed;
	uint8_t scrambled_jtag_password = 0;
#endif /* LIB_GF_SCRAMBLE */
#endif /* CORTINA_SECURE_JTAG_SUPPORT */
#endif /* CORTINA_OTP_SUPPORT */

	/* TODO G3 GPIO Init */
	/* TODO G3 PLL Init */
	/* TODO G3 SD EMMC Init */
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
	if (pOtpDrv == NULL)
	 INFO("G3 OTP get_drv_instace 0 FAILED\n");

	/* assign global pointer to OTP Shadow Base */
	cortina_otp_fixed_region_word1.wrd = 
	 mmio_read_32(OTP_SHADOW_BASE + OTP_DATA_FIXED_REGION_WORD1/8);  	
        HWSIM_PROGRESS_MARK(39);

#ifdef CORTINA_SECURE_JTAG_SUPPORT

	TEST_INFO("GLOBAL_STRAP debug_mode %d\n", g3_global_strap.bf.debug_mode);
	TEST_INFO("OTP fixed bitfield programmed %d\n", cortina_otp_fixed_region_word1.bf.programmed);
	TEST_INFO("OTP fixed bitfield jtag_password_inhibit %d\n", cortina_otp_fixed_region_word1.bf.jtag_password_inhibit);

	/* check if we should descramble ssk and copy to crypto engine */
	if (1 == cortina_otp_fixed_region_word2.bf.copy_ssk_to_sadb) {
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
	    (g3_global_strap.bf.debug_mode == 0x1) ||
	    (cortina_otp_fixed_region_word1.bf.programmed_n == 0x1) ||
	    (cortina_otp_fixed_region_word1.bf.jtag_password_inhibit_n == 0x0) 
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
	    (g3_global_strap.bf.debug_mode == 0x1) ||
	    (cortina_otp_fixed_region_word1.bf.programmed == 0x0) ||
	    (cortina_otp_fixed_region_word1.bf.jtag_password_inhibit == 0x1) 
#else /* CORTINA_OTP_SUPPORT */
#error OTP_MODE_is_not_set_in_platform.mk
#endif /* CORTINA_OTP_SUPPORT */ 
           )
	  {
	   /* clear the password to all ZEROs and set it */
	   for(int index = 0; index < CORTINA_SECURE_JTAG_PASSWORD_LEN; index++)
	     {
		jtag_password[index] = 0;
	     }

	  }
	else
	  {
	    /* read password from shadow copy of OTP 
	     * which is already byte aligned in shadow memory 
             */
#ifdef LIB_GF_SCRAMBLE
	   /* check to see if JTAG password is scrambled */
       	   cortina_otp_shadow_field_is_programmed(pOtpDrv, 
                                        (uint32_t) OTP_DATA_scrambled_jtag_password_BSTART, 
                                        (uint32_t) OTP_DATA_scrambled_jtag_password_BLEN, 
                                        &scrambled_jtag_password);
	   if(scrambled_jtag_password != 0)
	    {
             TEST_INFO("read scrambled jtag_password\n");
	     /* read scramble seed, based on lower byte of serial number */
	     scramble_seed = *(uint8_t *) (OTP_SHADOW_BASE + OTP_DATA_spare128_device_serial_no_BSTART/8);

	     if (scramble_seed == 0x0)
	      scramble_seed = 100;
	     /* descramble password, using seed of 100 */
	     scramble_gf(1,
	  	       scramble_seed,
                       (unsigned char *)(OTP_SHADOW_BASE + (OTP_DATA_jtag_password_BSTART/8)),
                       (unsigned char *) jtag_password,
		       CORTINA_SECURE_JTAG_PASSWORD_LEN);
#ifdef TEST_OTP
             memdump_byte((char *)jtag_password, 8); 
#endif
	    }
	   else
	    {
	      /* not scrambled */
	      memcpy(jtag_password, (const uint8_t *) (OTP_SHADOW_BASE + 
	        (OTP_DATA_jtag_password_BSTART/8)), CORTINA_SECURE_JTAG_PASSWORD_LEN);
	    }
#else /* LIB_GF_SCRAMBLE */
	   /* not scrambled */
	   memcpy(jtag_password, (const uint8_t *) (OTP_SHADOW_BASE + 
	    (OTP_DATA_jtag_password_BSTART/8)), CORTINA_SECURE_JTAG_PASSWORD_LEN);
#endif /* LIB_GF_SCRAMBLE */
          }
        HWSIM_PROGRESS_MARK(41);

	/* write the password */
        pSecureJTAGDrv = cortina_secure_jtag_init((uintptr_t) GLOBAL_DEVICE_ASSIGN_ID1);
	if(pSecureJTAGDrv != NULL)
         {
	  cortina_secure_jtag_set_password(pSecureJTAGDrv, jtag_password, CORTINA_SECURE_JTAG_PASSWORD_LEN);
          HWSIM_PROGRESS_MARK(42);
	 }


#endif /* CORTINA_SECURE_JTAG_SUPPORT */

       /* G3 ARM DEBUG OVERRIDE */
#if (CORTINA_OTP_SUPPORT == OTP_MODE_EM )
	if(cortina_otp_fixed_region_word1.bf.debug_override_n == 0x0) 
#elif (CORTINA_OTP_SUPPORT == OTP_MODE_KP )
	if(cortina_otp_fixed_region_word1.bf.debug_override == 0x1) 
#endif /* CORTINA_OTP_SUPPORT OTP_MODE */
         {
          g3_global_arm_debug.wrd = mmio_read_32(GLOBAL_ARM_DEBUG);
          NOTICE("GLOBAL_ARM_DEBUG reg 0x%x\n", g3_global_arm_debug.wrd);
          g3_global_arm_debug.bf.cpu_dbgen = cortina_otp_fixed_region_word1.bf.debug_debugen;
          g3_global_arm_debug.bf.cpu_niden = cortina_otp_fixed_region_word1.bf.debug_cpu_niden;
          g3_global_arm_debug.bf.cpu_spiden = cortina_otp_fixed_region_word1.bf.debug_cpu_spiden;
          g3_global_arm_debug.bf.cpu_spinden = cortina_otp_fixed_region_word1.bf.debug_cpu_spinden;
          g3_global_arm_debug.bf.auth_cssys_dbgen = cortina_otp_fixed_region_word1.bf.debug_auth_cssys_dbgen;
          g3_global_arm_debug.bf.auth_cssys_niden = cortina_otp_fixed_region_word1.bf.debug_auth_cssys_niden;
          g3_global_arm_debug.bf.cs_auth_niden = cortina_otp_fixed_region_word1.bf.debug_cs_auth_niden;
          g3_global_arm_debug.bf.cs_auth_spinden = cortina_otp_fixed_region_word1.bf.debug_cs_auth_spinden;
          g3_global_arm_debug.bf.cfg_tpctl = cortina_otp_fixed_region_word1.bf.debug_cfg_tpctl;
          g3_global_arm_debug.bf.cfg_tpmaxdatasize = cortina_otp_fixed_region_word1.bf.debug_cfg_tpmaxdatasize;
          mmio_write_32(GLOBAL_ARM_DEBUG, g3_global_arm_debug.wrd);
	  /* sync */
	  dsb();
	  isb();
          TEST_INFO("GLOBAL_ARM_DEBUG reg updated 0x%x\n", g3_global_arm_debug.wrd);
         }
#endif /* CORTINA_OTP_SUPPORT */
        HWSIM_PROGRESS_MARK(43);

#ifdef CORTINA_BOOT_EMMC
	/* since eMMC can not do secure access to its descriptors and buffers
         * we need to unprotect these in secure SRAM when in BL1 
         */
	if(pSecureSRAMDrv != NULL)
	 cortina_secure_sram_unprotect_range( pSecureSRAMDrv,
					CORTINA_BL1_MMC_DESC_BASE,
					CORTINA_SECURE_SRAM_ROUND_UP_16KB(CORTINA_BL1_MMC_DESC_SIZE + CORTINA_BL1_MMC_DATA_SIZE),
					CORTINA_SECURE_SRAM_ALL_CLEAR);
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

