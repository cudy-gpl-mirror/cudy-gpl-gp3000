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

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#include <common_def.h>
#include <tbbr_img_def.h>
#include <mipsregs.h>
#include INC_PLAT_DEF_HEADER

#ifdef CORTINA_HOTFIX_CHECKPOINT_SUPPORT
#include <bl1_checkpoint_hotfix.h>
#else /* CORTINA_HOTFIX_CHECKPOINT_SUPPORT */
#define BL1_HOTFIX_CHECKPOINT(id, data)
#endif  /* CORTINA_HOTFIX_CHECKPOINT_SUPPORT */

#ifdef TEST_ON_SATURN_ASIC_SIM
#include "asic_sim_test.h"
#else
#include "hwsim_macro_disable.h"
#endif /* TEST_ON_SATURN_ASIC_SIM */
/*******************************************************************************
 * Generic platform constants
 ******************************************************************************/
/* Firmware Image Package */
#define FIP_IMAGE_ID                    0

/* Trusted Boot Firmware BL2 */
#define BL2_IMAGE_ID                    1

/* SCP Firmware BL3-0 */
#define BL30_IMAGE_ID                   2

/* EL3 Runtime Firmware BL31 */
#define BL31_IMAGE_ID                   3

/* Secure Payload BL32 (Trusted OS) */
#define BL32_IMAGE_ID                   4

/* Non-Trusted Firmware BL33 */
#define BL33_IMAGE_ID                   5

/* Certificates */
#define TRUSTED_KEY_CERT_ID             7

/* Size of cacheable stacks */
#if IMAGE_BL1
#if TRUSTED_BOARD_BOOT
# define PLATFORM_STACK_SIZE            0x1000
#else
# define PLATFORM_STACK_SIZE            0x800
#endif
#elif IMAGE_BL2
# if TRUSTED_BOARD_BOOT
#  define PLATFORM_STACK_SIZE           0x1000
# else
#  define PLATFORM_STACK_SIZE           0x800
# endif
#elif IMAGE_BL2U
# define PLATFORM_STACK_SIZE            0x200
#elif IMAGE_BL31
#error BL31_IMAGE_UNSUPPORTED
#elif IMAGE_BL32
#error BL32_IMAGE_UNSUPPORTED
#endif

#ifdef FIRMWARE_WELCOME_STR
#undef FIRMWARE_WELCOME_STR
#endif
#define FIRMWARE_WELCOME_STR            "Secure Boot Firmware - Cortina Access Inc.\n"

#define LOADER_MEM_NAME                 "loader_mem"

#define BOOT_EMMC_NAME                  "l-loader.bin"

#define NORMAL_EMMC_NAME                "normal emmc"

/* Trusted Boot Firmware BL2 */
#define BL2_IMAGE_NAME                  "bl2.bin"

/* First Loader Firmware BL3-3 */
#define BL33_IMAGE_NAME                 "bl33.bin" /* e.g. U-Boot or UEFI */

/* Firmware Image Package */
#define FIP_IMAGE_NAME                  "fip.bin"

#define PLATFORM_CACHE_LINE_SIZE        32

#define PLATFORM_CORE_COUNT             1

#define MAX_IO_DEVICES                  6
#define MAX_IO_HANDLES                  6

/* eMMC User Data, NAND, SerialROM */
#define MAX_IO_BLOCK_DEVICES            3

/*******************************************************************************
 * Platform memory map related constants
 ******************************************************************************/

/* OTP defines */

/* OTP MODES */
#define OTP_MODE_EM                     10
#define OTP_MODE_KP                     11

#define OTP_BASE                        KSEG1ADDR(0x1A000000)


#if (CORTINA_OTP_SUPPORT == OTP_MODE_KP)
#define OTP_BAD_BITMAP                  256
#elif (CORTINA_OTP_SUPPORT == OTP_MODE_EM)
#define OTP_BAD_BITMAP                  0
#else
#error OTP_MODE_not_set_in_platform.mk
#endif /* CORTINA_OTP_SUPPORT */
#define OTP_BIT_SIZE                    (8192 - OTP_BAD_BITMAP)
#define OTP_BYTE_SIZE                   (OTP_BIT_SIZE/8)
#define OTP_PAGE_ROUNDUP_SIZE           0x1000 /* we need to keep page alignment */
#if (CORTINA_OTP_SUPPORT == OTP_MODE_EM)
#define OTP_READ_TIMEOUT                2
#define OTP_WRITE_TIMEOUT               4
#elif (CORTINA_OTP_SUPPORT == OTP_MODE_KP)
#define OTP_READ_TIMEOUT                20000 /* up to 20 ms */
#define OTP_WRITE_TIMEOUT               20000 /* up to 20 ms */
#endif /* CORTINA_OTP_SUPPORT */

/* enable OTP SW simulator for testing logic, since FPGA board has no real OTP */
//#define OTP_SIM 1  // do not enable it here. Use OTP_SIM will be on if TEST_OTP is set in make env

#ifdef OTP_SIM
#define OTP_SIM_DATA_ARRAY_BASE         OTP_BASE
#endif /* OTP_SIM */


#define PLAT_SATURN_TRUSTED_ROM_BASE    BOOTROM_BASE

#if defined(TEST_ON_SATURN_ASIC_SIM) 
#define PLAT_SATURN_TRUSTED_ROM_SIZE    0x20000 /* 96 KB, now 128kB only if TEST_ON_CORTINA_ASIC_SIM is set */
#else
#define PLAT_SATURN_TRUSTED_ROM_SIZE    0x18000 /* 96 KB */
#endif

/* The first 4KB of Trusted SRAM are used as shared memory */
#define PLAT_SATURN_TRUSTED_SRAM_BASE   SRAM_BASE
#define PLAT_SATURN_TRUSTED_SRAM_SIZE   SRAM_SIZE /* 512 KB */

#define PLAT_SATURN_SHARED_RAM_BASE     PLAT_SATURN_TRUSTED_SRAM_BASE
#define PLAT_SATURN_SHARED_RAM_SIZE     0x00000000      /* 0 KB */


/* MMC */
#define CORTINA_DW_MMC_REG_BASE         KSEG1ADDR(0x12100000)
#define CORTINA_DW_MMC_CLK              50000000 /* 50Mhz */

/* MMC uses lower 128KB of SRAM when in BL1 */
#ifdef IMAGE_BL1
#define CORTINA_BL1_MMC_DESC_BASE       SRAM_BASE
#define CORTINA_BL1_MMC_DESC_SIZE       0x1000 /* 4KB */
#define CORTINA_BL1_MMC_DATA_BASE       (CORTINA_BL1_MMC_DESC_BASE + CORTINA_BL1_MMC_DESC_SIZE)
/* since Secure SRAM protection uses 16KB block boundries we might as well use 16KB total space */
#define CORTINA_BL1_MMC_DATA_SIZE       (0x00004000 - CORTINA_BL1_MMC_DESC_SIZE)
#else /* IMAGE_BL1 */
/* when in BL2 or higher use DRAM, 32-48MB range  */
#define CORTINA_MMC_DESC_BASE           (DDR_BASE + 0x02000000) /* above 32MB */
#define CORTINA_MMC_DESC_SIZE           0x1000 /* 4KB */
#define CORTINA_MMC_DATA_BASE           (CORTINA_MMC_DESC_BASE + CORTINA_MMC_DESC_SIZE)
#define CORTINA_MMC_DATA_SIZE           (0x01000000 - CORTINA_MMC_DESC_SIZE)    /* 16 MB */
#endif /* IMAGE_BL1 */

#define CORTINA_MMC_RAW_DATA_OFFSET     0
#define CORTINA_MMC_FIP_BASE            (CORTINA_MMC_RAW_DATA_OFFSET + (0 << 20))   
#define CORTINA_MMC_FIP_MAX_SIZE        (8 << 20)

/* The remaining Trusted SRAM is used to load the BL images */
/* reserve memory in front for any required SHARED IPC RAM */
/* reserve memory in back for OTP shadow */
#define PLAT_SATURN_BL_RAM_BASE         (PLAT_SATURN_SHARED_RAM_BASE +  \
                                            PLAT_SATURN_SHARED_RAM_SIZE)
#define PLAT_SATURN_BL_RAM_SIZE         (PLAT_SATURN_TRUSTED_SRAM_SIZE -   \
                                            PLAT_SATURN_SHARED_RAM_SIZE - \
                                            OTP_PAGE_ROUNDUP_SIZE)


/* place OTP_SHADOW_BASE after BL_RAM, at the very end of the Secure SRAM. OTP size is 1 KBbytes */
#define OTP_SHADOW_BASE                 KSEG1ADDR(PLAT_SATURN_BL_RAM_BASE + \
                                            PLAT_SATURN_TRUSTED_SRAM_SIZE - 0x400)

/*******************************************************************************
 * BL1 specific defines.
 * BL1 RW data is relocated from ROM to RAM at runtime so we need 2 sets of
 * addresses.
 ******************************************************************************/
#define BL1_RO_BASE                     PLAT_SATURN_TRUSTED_ROM_BASE
#define BL1_RO_LIMIT                    (PLAT_SATURN_TRUSTED_ROM_BASE      \
                                            + PLAT_SATURN_TRUSTED_ROM_SIZE)
/*
 * Put BL1 RW at the top of the Trusted SRAM. BL1_RW_BASE is calculated using
 * the current BL1 RW debug size plus a little space for growth.
 */
#if TRUSTED_BOARD_BOOT
#define BL1_RW_SIZE_MAX                 0xa000
#else
#define BL1_RW_SIZE_MAX                 0x7000
#endif

#define BL1_RW_BASE                     (PLAT_SATURN_BL_RAM_BASE +              \
                                            PLAT_SATURN_BL_RAM_SIZE -       \
                                            BL1_RW_SIZE_MAX)

#define BL1_RW_LIMIT                    (PLAT_SATURN_BL_RAM_BASE + PLAT_SATURN_BL_RAM_SIZE)
#define BL1_RW_SIZE                     (BL1_RW_LIMIT - BL1_RW_BASE)

#ifdef CORTINA_MULTIBOOT_SUPPORT
#if (CORTINA_OTP_SUPPORT == OTP_MODE_KP)
/* Since we dont copy KP bad block info into the shadow, 
 * we can stash away the choosen BootTuple in its place  
 */
#define MULTIBOOT_BL2_BOOT_TUPLE_ADDR    (OTP_SHADOW_BASE + (OTP_BIT_SIZE/8))
#endif /* (CORTINA_OTP_SUPPORT == OTP_MODE_KP) */
#endif /* CORTINA_MULTIBOOT_SUPPORT */


/*******************************************************************************
 * BL2 specific defines.
 ******************************************************************************/

#if TRUSTED_BOARD_BOOT
#define BL2_BASE                        (BL1_RW_BASE -              \
                                            0x20000)
#else /* TRUSTED_BOARD_BOOT */
#define BL2_BASE                        (BL1_RW_BASE -              \
                                            0xC000)
#endif /* TRUSTED_BOARD_BOOT */
#define BL2_LIMIT                       (BL1_RW_BASE)

/*
 * Some of the platform porting definitions use the 'ull' suffix in order to
 * avoid subtle integer overflow errors due to implicit integer type promotion
 * when working with 32-bit values.
 *
 * The TSP linker script includes some of these definitions to define the BL32
 * memory map, but the GNU LD does not support the 'ull' suffix, causing the
 * build process to fail. To solve this problem, the auxiliary macro MAKE_ULL(x)
 * will add the 'ull' suffix only when the macro __LINKER__  is not defined
 * (__LINKER__ is defined in the command line to preprocess the linker script).
 * Constants in the linker script will not have the 'ull' suffix, but this is
 * not a problem since the linker evaluates all constant expressions to 64 bit
 * (assuming the target architecture is 64 bit).
 */
#ifndef __LINKER__
  #define MAKE_ULL(x)                   x##ull
#else
  #define MAKE_ULL(x)                   x
#endif

/*
 * Macros to wrap declarations of deprecated APIs within Trusted Firmware.
 * The callers of these APIs will continue to compile with a warning as long
 * as the build flag ERROR_DEPRECATED is zero.
 */
#define __warn_deprecated               __attribute__ ((deprecated))

#endif /* __PLATFORM_DEF_H__ */
