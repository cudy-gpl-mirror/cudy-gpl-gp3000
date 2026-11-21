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

#include <arch.h>
#include <common_def.h>
#include <tbbr_img_def.h>
#include INC_PLAT_DEF_HEADER

#ifdef TEST_ON_G3_ASIC_SIM
#include "asic_sim_test.h"
#else
#include "hwsim_macro_disable.h"
#endif /* TEST_ON_G3_ASIC_SIM */

/*******************************************************************************
 * Platform binary types for linking
 ******************************************************************************/
#define PLATFORM_LINKER_FORMAT          "elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH            aarch64

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
#if DEBUG_XLAT_TABLE
# define PLATFORM_STACK_SIZE 0x800
#elif IMAGE_BL1
#if TRUSTED_BOARD_BOOT
# define PLATFORM_STACK_SIZE 0x1000
#else
# define PLATFORM_STACK_SIZE 0x800
#endif
#elif IMAGE_BL2
# if TRUSTED_BOARD_BOOT
#  define PLATFORM_STACK_SIZE 0x1000
# else
#  define PLATFORM_STACK_SIZE 0x800
# endif
#elif IMAGE_BL2U
# define PLATFORM_STACK_SIZE 0x200
#elif IMAGE_BL31
# define PLATFORM_STACK_SIZE 0x400
#elif IMAGE_BL32
# define PLATFORM_STACK_SIZE 0x440
#endif

#ifdef FIRMWARE_WELCOME_STR
#undef FIRMWARE_WELCOME_STR
#endif
#define FIRMWARE_WELCOME_STR		"Booting Trusted Firmware - Cortina Access Inc.\n"

#define LOADER_MEM_NAME			"loader_mem"

#define BOOT_EMMC_NAME			"l-loader.bin"

#define NORMAL_EMMC_NAME		"normal emmc"

/* Trusted Boot Firmware BL2 */
#define BL2_IMAGE_NAME			"bl2.bin"

/* EL3 Runtime Firmware BL3-1 */
#define BL31_IMAGE_NAME			"bl31.bin"

/* SCP - System Control Processor (none on G3)  Firmware BL3-0 */
#define BL30_IMAGE_NAME			"bl30.bin"

/* Secure Payload BL3-2 (Trusted OS) */
#define BL32_IMAGE_NAME			"bl32.bin"

/* Non-Trusted Firmware BL3-3 */
#define BL33_IMAGE_NAME			"bl33.bin" /* e.g. UEFI */

/* Firmware Image Package */
#define FIP_IMAGE_NAME			"fip.bin"

#define PLATFORM_CACHE_LINE_SIZE	64
#define PLATFORM_CLUSTER_COUNT		1
#define PLATFORM_CORE_COUNT_PER_CLUSTER	4


#define PLATFORM_CORE_COUNT             4
#define PLATFORM_NUM_AFFS		(PLATFORM_CLUSTER_COUNT + \
					 PLATFORM_CORE_COUNT)
#define PLATFORM_MAX_AFFLVL             MPIDR_AFFLVL1

#define MAX_IO_DEVICES			6
#define MAX_IO_HANDLES			6

/* eMMC User Data, NAND, SerialROM */
#define MAX_IO_BLOCK_DEVICES            3

/*******************************************************************************
 * Platform memory map related constants
 ******************************************************************************/

/* OTP defines */

/* OTP MODES */
#define OTP_MODE_EM 10
#define OTP_MODE_KP 11

#ifdef TEST_OTP
// FPGA, we have no way to write into the HW OTP, we need to turn on OTP_SIM and use DRAM as OTP_BASE
// cannot use DDR at 0xF0000000 but try the 1KB just before the OTP shadow 
#define OTP_BASE                        0xC003F800
#else
#define OTP_BASE			0xFFF80000	
#endif

#if (CORTINA_OTP_SUPPORT == OTP_MODE_KP)
#define OTP_BAD_BITMAP			256
#elif (CORTINA_OTP_SUPPORT == OTP_MODE_EM)
#define OTP_BAD_BITMAP			0
#else
#error OTP_MODE_not_set_in_platform.mk
#endif /* CORTINA_OTP_SUPPORT */
#define OTP_BIT_SIZE 			(8192 - OTP_BAD_BITMAP)
#define OTP_PAGE_ROUNDUP_SIZE		0x1000 /* we need to keep page alignment */
#if (CORTINA_OTP_SUPPORT == OTP_MODE_EM)
#define OTP_READ_TIMEOUT 2
#define OTP_WRITE_TIMEOUT 4
#elif (CORTINA_OTP_SUPPORT == OTP_MODE_KP)
#define OTP_READ_TIMEOUT 20000 /* up to 20 ms */
#define OTP_WRITE_TIMEOUT 20000 /* up to 20 ms */
#endif /* CORTINA_OTP_SUPPORT */

/* enable OTP SW simulator for testing logic, since FPGA board has no real OTP */
//#define OTP_SIM 1  // do not enable it here. Use OTP_SIM will be on if TEST_OTP is set in make env

#ifdef OTP_SIM
#define OTP_SIM_DATA_ARRAY_BASE		OTP_BASE
#endif /* OTP_SIM */


#define PLAT_G3_TRUSTED_ROM_BASE 	0xFFFC0000

#if defined(TEST_ON_G3_ASIC_SIM) || defined(TEST_OTP)
#define PLAT_G3_TRUSTED_ROM_SIZE	0x20000 /* 96 KB, now 128kB only if TEST_ON_CORTINA_ASIC_SIM is set */
#else
#define PLAT_G3_TRUSTED_ROM_SIZE	0x18000 /* 96 KB */
#endif

/* The first 4KB of Trusted SRAM are used as shared memory */
#define PLAT_G3_TRUSTED_SRAM_BASE	0xC0000000
#define PLAT_G3_TRUSTED_SRAM_SIZE	0x40000 /* 256 KB */

#define PLAT_G3_SHARED_RAM_BASE		PLAT_G3_TRUSTED_SRAM_BASE
#define PLAT_G3_SHARED_RAM_SIZE		0x00000000      /* 0 KB */

#define CORTINA_NS_IMAGE_OFFSET              (DDR_BASE + 0x35000000)

/* MMC */
#define CORTINA_DW_MMC_REG_BASE		0xF4400000
#define CORTINA_DW_MMC_CLK			50000000 /* 50Mhz */

/* MMC uses lower 128KB of SRAM when in BL1 */
#ifdef IMAGE_BL1
#define CORTINA_BL1_MMC_DESC_BASE            SRAM_BASE
#define CORTINA_BL1_MMC_DESC_SIZE            0x1000 /* 4KB */
#define CORTINA_BL1_MMC_DATA_BASE            (CORTINA_BL1_MMC_DESC_BASE + CORTINA_BL1_MMC_DESC_SIZE)
/* since Secure SRAM protection uses 16KB block boundries we might as well use 16KB total space */
#define CORTINA_BL1_MMC_DATA_SIZE            (0x00004000 - CORTINA_BL1_MMC_DESC_SIZE)
#else /* IMAGE_BL1 */
/* when in BL2 or higher use DRAM, 32-48MB range  */
#define CORTINA_MMC_DESC_BASE                (DDR_BASE + 0x02000000) /* above 32MB */
#define CORTINA_MMC_DESC_SIZE                0x1000 /* 4KB */
#define CORTINA_MMC_DATA_BASE                (CORTINA_MMC_DESC_BASE + CORTINA_MMC_DESC_SIZE)
#define CORTINA_MMC_DATA_SIZE                (0x01000000 - CORTINA_MMC_DESC_SIZE)	/* 16 MB */
#endif /* IMAGE_BL1 */

#define CORTINA_MMC_RAW_DATA_OFFSET          0
#define CORTINA_MMC_FIP_BASE                 (CORTINA_MMC_RAW_DATA_OFFSET + (0 << 20))
#define CORTINA_MMC_FIP_MAX_SIZE             (8 << 20)

/* The remaining Trusted SRAM is used to load the BL images */
/* reserve memory in front for any required SHARED IPC RAM */
/* reserve memory in back for OTP shadow */
#define PLAT_G3_BL_RAM_BASE                 (PLAT_G3_SHARED_RAM_BASE +  \
                                         PLAT_G3_SHARED_RAM_SIZE)
#define PLAT_G3_BL_RAM_SIZE                 (PLAT_G3_TRUSTED_SRAM_SIZE -   \
                                         PLAT_G3_SHARED_RAM_SIZE - \
					 OTP_PAGE_ROUNDUP_SIZE)


/* place OTP_SHADOW_BASE after BL_RAM, at the very end of the Secure SRAM. OTP size is 1 KBbytes */
#define OTP_SHADOW_BASE			(PLAT_G3_BL_RAM_BASE + PLAT_G3_TRUSTED_SRAM_SIZE - 0x400)

/*******************************************************************************
 * BL1 specific defines.
 * BL1 RW data is relocated from ROM to RAM at runtime so we need 2 sets of
 * addresses.
 ******************************************************************************/
#define BL1_RO_BASE                     PLAT_G3_TRUSTED_ROM_BASE
#define BL1_RO_LIMIT                    (PLAT_G3_TRUSTED_ROM_BASE      \
                                         + PLAT_G3_TRUSTED_ROM_SIZE)
/*
 * Put BL1 RW at the top of the Trusted SRAM. BL1_RW_BASE is calculated using
 * the current BL1 RW debug size plus a little space for growth.
 */
#if TRUSTED_BOARD_BOOT
#define BL1_RW_SIZE_MAX			0xa000
#else
#define BL1_RW_SIZE_MAX			0x7000
#endif

#define BL1_RW_BASE                     (PLAT_G3_BL_RAM_BASE +              \
                                                PLAT_G3_BL_RAM_SIZE -       \
                                                BL1_RW_SIZE_MAX)

#define BL1_RW_LIMIT                    (PLAT_G3_BL_RAM_BASE + PLAT_G3_BL_RAM_SIZE)
#define BL1_RW_SIZE                     (BL1_RW_LIMIT - BL1_RW_BASE)

#ifdef CORTINA_MULTIBOOT_SUPPORT
#if (CORTINA_OTP_SUPPORT == OTP_MODE_KP)
/* Since we dont copy KP bad block info into the shadow, 
 * we can stash away the choosen BootTuple in its place  
 */
#define MULTIBOOT_BL2_BOOT_TUPLE_ADDR	(OTP_SHADOW_BASE + (OTP_BIT_SIZE/8))
#endif /* (CORTINA_OTP_SUPPORT == OTP_MODE_KP) */
#endif /* CORTINA_MULTIBOOT_SUPPORT */


/*******************************************************************************
 * BL2 specific defines.
 ******************************************************************************/

#if PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID == PLAT_G3_TRUSTED_SRAM_ID
/*
 * Put BL2 just below BL3-1. BL2_BASE is calculated using the current BL2 debug
 * size plus a little space for growth.
 */
#if TRUSTED_BOARD_BOOT
#define BL2_BASE                        (BL31_BASE - 0x1D000)
#else /* TRUSTED_BOARD_BOOT */
#define BL2_BASE                        (BL31_BASE - 0xC000)
#endif /* TRUSTED_BOARD_BOOT */
#define BL2_LIMIT                       BL31_BASE
#elif PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID == PLAT_G3_TRUSTED_DRAM_ID
#if TRUSTED_BOARD_BOOT
#define BL2_BASE                       (BL1_RW_BASE -              \
                                                0x1D000)
#else /* TRUSTED_BOARD_BOOT */
#define BL2_BASE                       (BL1_RW_BASE -              \
                                                0xC000)
#endif /* TRUSTED_BOARD_BOOT */
#define BL2_LIMIT                      (BL1_RW_BASE)
#else /* PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID */
# error "Unsupported PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID value"
#endif /* PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID */
/*******************************************************************************
 * BL3-1 specific defines.
 ******************************************************************************/
/*
 * Put BL3-1 at the top of the Trusted SRAM. BL31_BASE is calculated using the
 * current BL3-1 debug size plus a little space for growth.
 */
#define BL31_SIZE_MAX	0x1D000

#if TRUSTED_BOARD_BOOT
#define BL1_DATASEG_SIZE_MAX                 0x9000
#else
#define BL1_DATASEG_SIZE_MAX                 0x6000
#endif

#if PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID == PLAT_G3_TRUSTED_SRAM_ID
#define BL31_BASE                       (PLAT_G3_BL_RAM_BASE +              \
                                                PLAT_G3_BL_RAM_SIZE -       \
                                                BL31_SIZE_MAX)
#define BL31_PROGBITS_LIMIT             BL1_RW_BASE
#define BL31_LIMIT                      (PLAT_G3_BL_RAM_BASE + PLAT_G3_BL_RAM_SIZE)
#elif PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID == PLAT_G3_TRUSTED_DRAM_ID
	/* place BL3-1 EL3 at the top of secure DRAM */
#define BL31_BASE                       (DRAM_SEC_BASE +  \
						DRAM_SEC_SIZE - \
                                                BL31_SIZE_MAX)
#define BL31_PROGBITS_LIMIT             (DRAM_SEC_BASE + DRAM_SEC_SIZE -   \
						BL1_DATASEG_SIZE_MAX)
#define BL31_LIMIT                      (DRAM_SEC_BASE + DRAM_SEC_SIZE)
#else
# error "Unsupported PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID value"
#endif
/*******************************************************************************
 * BL3-2 specific defines.
 ******************************************************************************/
/*
 * On Cortina standard platforms, the TSP can execute from Trusted SRAM,
 * Trusted DRAM (if available) or the DRAM region secured by the TrustZone
 * controller.
 */
#if 0 /* do not use BL3-2 for now */
#if PLAT_G3_TSP_RAM_LOCATION_ID == PLAT_G3_TRUSTED_SRAM_ID
# define TSP_SEC_MEM_BASE               PLAT_G3_BL_RAM_BASE
# define TSP_SEC_MEM_SIZE               PLAT_G3_BL_RAM_SIZE
# define TSP_PROGBITS_LIMIT             BL2_BASE
# define BL32_BASE                      PLAT_G3_BL_RAM_BASE
# define BL32_LIMIT                     BL31_BASE
#elif PLAT_G3_TSP_RAM_LOCATION_ID == PLAT_G3_TRUSTED_DRAM_ID
#if PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID == PLAT_G3_TRUSTED_SRAM_ID
  /* since we dont share space with BL3_1_EL3 we consume entire protected DRAM */
# define TSP_SEC_MEM_BASE               DRAM_SEC_BASE
# define TSP_SEC_MEM_SIZE               DRAM_SEC_SIZE
# define BL32_BASE                      TSP_SEC_MEM_BASE
# define BL32_LIMIT                     (TSP_SEC_MEM_BASE + TSP_SEC_MEM_SIZE)
#elif PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID == PLAT_G3_TRUSTED_DRAM_ID
  /* we are sharing protected DRAM and have to leave the end of protected DRAM for BL3_1_EL3 */
# define TSP_SEC_MEM_BASE               DRAM_SEC_BASE
# define TSP_SEC_MEM_SIZE               DRAM_SEC_SIZE - BL31_SIZE_MAX
# define BL32_BASE                      TSP_SEC_MEM_BASE
# define BL32_LIMIT                     (TSP_SEC_MEM_BASE + TSP_SEC_MEM_SIZE)
#endif /* PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID == PLAT_G3_TRUSTED_DRAM_ID */
#else /* PLAT_G3_TSP_RAM_LOCATION_ID == PLAT_G3_TRUSTED_SRAM_ID */
# error "Unsupported PLAT_G3_TSP_RAM_LOCATION_ID value"
#endif /* PLAT_G3_TSP_RAM_LOCATION_ID == PLAT_G3_TRUSTED_SRAM_ID */
#endif /* do not use BL3-2 for now */

/*
 * ID of the secure physical generic timer interrupt used by the TSP.
 */
#define TSP_IRQ_SEC_PHY_TIMER           PLAT_G3_IRQ_SEC_PHY_TIMER


/* Clocks */
#define G3_LSAXI_CLK			166000000   /* 166 Mhz */

/*******************************************************************************
 * Platform specific page table and MMU setup constants
 ******************************************************************************/
#define ADDR_SPACE_SIZE			(1ull << 32)

#if IMAGE_BL1 || IMAGE_BL32
# define MAX_XLAT_TABLES		5
#endif

#if IMAGE_BL2
# define MAX_XLAT_TABLES		5
#endif

#if IMAGE_BL31
# define MAX_XLAT_TABLES		5
#endif

#define MAX_MMAP_REGIONS		16

/*******************************************************************************
 * Declarations and constants to access the mailboxes safely. Each mailbox is
 * aligned on the biggest cache line size in the platform. This is known only
 * to the platform as it might have a combination of integrated and external
 * caches. Such alignment ensures that two maiboxes do not sit on the same cache
 * line at any cache level. They could belong to different cpus/clusters &
 * get written while being protected by different locks causing corruption of
 * a valid mailbox address.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT   6
#define CACHE_WRITEBACK_GRANULE (1 << CACHE_WRITEBACK_SHIFT)


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
#define __warn_deprecated       __attribute__ ((deprecated))

#endif /* __PLATFORM_DEF_H__ */
