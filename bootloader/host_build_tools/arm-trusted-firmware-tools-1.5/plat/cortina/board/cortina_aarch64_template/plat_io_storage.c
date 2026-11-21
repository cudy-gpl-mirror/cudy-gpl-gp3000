/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
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
#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>		/* For ARRAY_SIZE */
#include <debug.h>
#ifdef CORTINA_BOOT_EMMC
#include <dw_mmc.h>
#include <emmc.h>
#endif /* CORTINA_BOOT_EMMC */
#include <firmware_image_package.h>
#include <io_block.h>
#include <io_driver.h>
#include <io_fip.h>
#include <io_memmap.h>
#ifdef CORTINA_BOOT_PARALLEL_NAND
#include <cortina_parallel_nand.h>
#endif /* CORTINA_BOOT_PARALLEL_NAND */
#ifdef CORTINA_BOOT_SERIAL_NOR
#include <cortina_serial_nor.h>
#endif /* CORTINA_BOOT_SERIAL_NOR */
#include <io_storage.h>
#include <mmio.h>
#include <platform_def.h>
#include <semihosting.h>        /* For FOPEN_MODE_... */
#include <string.h>
#ifdef CORTINA_OTP_SUPPORT
#include <cortina_otp.h>
#include INC_CORTINA_PLATFORM_OTP_DATA_HEADER
extern CORTINA_OTP_DRV_t * cortina_pOtpDrv;
#endif
#include <plat_io_storage.h>
#include "partitions.h"
#include <delay_timer.h>

/* IO devices */
static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_handle;
static const io_dev_connector_t *memmap_dev_con;
static uintptr_t memmap_dev_handle;
#ifdef CORTINA_BOOT_PARALLEL_NAND
static const io_dev_connector_t *cortina_parallel_nand_dev_con;
static uintptr_t cortina_parallel_nand_dev_handle;
#endif /* CORTINA_BOOT_PARALLEL_NAND */
#ifdef CORTINA_BOOT_SERIAL_NOR
static const io_dev_connector_t *cortina_serial_nor_dev_con;
static uintptr_t cortina_serial_nor_dev_handle;
#endif /* CORTINA_BOOT_SERIAL_NOR */
#ifdef CORTINA_BOOT_EMMC
static const io_dev_connector_t *emmc_dev_con;
static uintptr_t emmc_dev_handle;
#endif /* CORTINA_BOOT_EMMC */

#ifdef CORTINA_MULTIBOOT_SUPPORT
extern GLOBAL_STRAP_t g3_global_strap;
uint8_t current_otp_boot_fip_index = 0;
#endif /* CORTINA_MULTIBOOT_SUPPORT */

#ifdef CORTINA_BOOT_EMMC
static int open_emmc(const uintptr_t spec);

static io_block_spec_t emmc_fip_spec = {
        .offset         = CORTINA_MMC_FIP_BASE,
        .length         = CORTINA_MMC_FIP_MAX_SIZE,
};

static const io_block_dev_spec_t emmc_dev_spec = {
        /* It's used as temp buffer in block driver. */
#if IMAGE_BL1 /* use Secure SRAM */
        .buffer         = {
                .offset = CORTINA_BL1_MMC_DATA_BASE,
                .length = CORTINA_BL1_MMC_DATA_SIZE,
        },
#else
        .buffer         = {
                .offset = CORTINA_MMC_DATA_BASE,
                .length = CORTINA_MMC_DATA_SIZE,
        },
#endif
        .ops            = {
                .read   = emmc_read_blocks,
                .write  = emmc_write_blocks,
        },
        .block_size     = EMMC_BLOCK_SIZE,
};
#endif /* CORTINA_BOOT_EMMC */

static  io_block_spec_t fip_block_spec;

static  io_block_spec_t memmap_fip_block_spec = {
	.offset = PLAT_G3_DRAM_FIP_BASE,
	.length = PLAT_G3_DRAM_FIP_MAX_SIZE
};

#ifdef CORTINA_BOOT_SERIAL_NOR
static  io_block_spec_t cortina_serial_nor_fip_block_spec = {
	.offset = 0,
	.length = PLAT_G3_FLASH_FIP_MAX_SIZE
};
#endif /* CORTINA_BOOT_SERIAL_NOR */

#ifdef CORTINA_BOOT_PARALLEL_NAND
static  io_block_spec_t cortina_parallel_nand_fip_block_spec = {
	.offset = 0,
	.length = PLAT_G3_FLASH_FIP_MAX_SIZE
};
#endif /* CORTINA_BOOT_PARALLEL_NAND */

static const io_uuid_spec_t bl2_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
};

static const io_uuid_spec_t scp_bl2_uuid_spec = {
	.uuid = UUID_SCP_FIRMWARE_SCP_BL2,
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl32_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

#if TRUSTED_BOARD_BOOT
static const io_uuid_spec_t tb_fw_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FW_CERT,
};

static const io_uuid_spec_t trusted_key_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_KEY_CERT,
};

static const io_uuid_spec_t scp_fw_key_cert_uuid_spec = {
	.uuid = UUID_SCP_FW_KEY_CERT,
};

static const io_uuid_spec_t soc_fw_key_cert_uuid_spec = {
	.uuid = UUID_SOC_FW_KEY_CERT,
};

static const io_uuid_spec_t tos_fw_key_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_OS_FW_KEY_CERT,
};

static const io_uuid_spec_t nt_fw_key_cert_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FW_KEY_CERT,
};

static const io_uuid_spec_t scp_fw_cert_uuid_spec = {
	.uuid = UUID_SCP_FW_CONTENT_CERT,
};

static const io_uuid_spec_t soc_fw_cert_uuid_spec = {
	.uuid = UUID_SOC_FW_CONTENT_CERT,
};

static const io_uuid_spec_t tos_fw_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_OS_FW_CONTENT_CERT,
};

static const io_uuid_spec_t nt_fw_cert_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FW_CONTENT_CERT,
};
#endif /* TRUSTED_BOARD_BOOT */


static int open_fip(const uintptr_t spec);
static int open_memmap(const uintptr_t spec);

/* By default, Cortina platforms load images from the FIP */
plat_io_policy_t policies[] = {
	[FIP_IMAGE_ID] = {
		&memmap_dev_handle,
		(uintptr_t)&memmap_fip_block_spec,
		open_memmap
	},
	[BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl2_uuid_spec,
		open_fip
	},
	[SCP_BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&scp_bl2_uuid_spec,
		open_fip
	},
	[BL31_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		open_fip
	},
	[BL32_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_uuid_spec,
		open_fip
	},
	[BL33_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl33_uuid_spec,
		open_fip
	},
#if TRUSTED_BOARD_BOOT
	[TRUSTED_BOOT_FW_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&tb_fw_cert_uuid_spec,
		open_fip
	},
	[TRUSTED_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&trusted_key_cert_uuid_spec,
		open_fip
	},
	[SCP_FW_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&scp_fw_key_cert_uuid_spec,
		open_fip
	},
	[SOC_FW_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&soc_fw_key_cert_uuid_spec,
		open_fip
	},
	[TRUSTED_OS_FW_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&tos_fw_key_cert_uuid_spec,
		open_fip
	},
	[NON_TRUSTED_FW_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&nt_fw_key_cert_uuid_spec,
		open_fip
	},
	[SCP_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&scp_fw_cert_uuid_spec,
		open_fip
	},
	[SOC_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&soc_fw_cert_uuid_spec,
		open_fip
	},
	[TRUSTED_OS_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&tos_fw_cert_uuid_spec,
		open_fip
	},
	[NON_TRUSTED_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&nt_fw_cert_uuid_spec,
		open_fip
	},
#endif /* TRUSTED_BOARD_BOOT */
};

/*
 * Decode Platform Specific logic to determine the logical parameters to
 * pass to the Cortina Serial NOR driver. 
 * This function also resets any platform specific logic and and does
 * setup of platform specific decode to support the Serial NOR driver.
 *
 * Return 0 if OK, -1 if ERROR
 */
#ifdef CORTINA_BOOT_SERIAL_NOR
static int plat_io_serial_nor_init()
{
#ifndef CORTINA_BOOT_SERIAL_NOR_LEGACY_API	
	uint32_t norFamily;
	uint32_t totalSize;
	uint32_t flags;
	FLASH_TYPE_t flashTypeReg;
	GLOBAL_STRAP_t bootInfo;
	GLOBAL_BLOCK_RESET_t globalBlockResetReg;
#endif /* CORTINA_BOOT_SERIAL_NOR_LEGACY_API */

#ifdef CORTINA_BOOT_SERIAL_NOR_LEGACY_API	
	cortina_serial_nor_ctrl_init(FLASH_ID);
#else /* CORTINA_BOOT_SERIAL_NOR_LEGACY_API */

	/* reset the HW block */

	globalBlockResetReg.wrd = mmio_read_32(GLOBAL_BLOCK_RESET);

	/* assert reset */
	globalBlockResetReg.bf.reset_flash = 1;
	mmio_write_32(GLOBAL_BLOCK_RESET, globalBlockResetReg.wrd);

	udelay(10);

	/* turn reset off */
	globalBlockResetReg.bf.reset_flash = 0;
	mmio_write_32(GLOBAL_BLOCK_RESET, globalBlockResetReg.wrd);

	/* for now we assume we only support primary boot device
         * however, using bootInfo varable allows us to get secondary
         * bootInfo info from a different location in the future
         * like OTP 
         */
	bootInfo.wrd = g3_global_strap.wrd;

	/* determine NOR family */
	if(bootInfo.bf.flash_width == 1)
	  norFamily = SERIAL_NOR_FAMILY_ID_ATMEL;
	else
	  norFamily = SERIAL_NOR_FAMILY_ID_OTHER;

	/* determine flags */
	if(norFamily == SERIAL_NOR_FAMILY_ID_ATMEL)
	 {
	   if(bootInfo.bf.flash_pin == 1)
	   flags = SERIAL_NOR_FLAGS_ATMEL_2X_PAGE_SIZE;
	 }

	/* determine totalSize*/
	if(norFamily == SERIAL_NOR_FAMILY_ID_ATMEL)
	  {
             switch(bootInfo.bf.flash_size)
		{
		 case 0x0:
			totalSize = 1 * 1024 * 1024;
			break;
		 case 0x1:
			totalSize = 4 * 1024 * 1024;
			break;
		 case 0x2:
			totalSize = 8 * 1024 * 1024;
			break;
		 case 0x3:
			totalSize = 16 * 1024 * 1024;
			break;
		}
		
	  }
	else
	  {
            /* non ATMEL family */
		/* only significate bit is bit 1 */
             if((bootInfo.bf.flash_size & 0x2) == 0)
               totalSize = 16 * 1024 * 1024;
	     else
               totalSize = 32 * 1024 * 1024;
	  }

	/* set decode to general flash controller logic */
	flashTypeReg.bf.flashSize = bootInfo.bf.flash_size;
	flashTypeReg.bf.flashWidth = bootInfo.bf.flash_width;
	flashTypeReg.bf.flashType = bootInfo.bf.flash_type;
	flashTypeReg.bf.flashPin = bootInfo.bf.flash_pin;
	mmio_write_32(FLASH_TYPE, flashTypeReg.wrd);
	
	/* start the driver */
	cortina_serial_nor_ctrl_init(FLASH_ID, norFamily, totalSize, flags);
#endif /* CORTINA_BOOT_SERIAL_NOR_LEGACY_API */

	return 0;
}
#endif /* CORTINA_BOOT_SERIAL_NOR */

static int open_fip(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if (result == 0) {
		result = io_open(fip_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using FIP\n");
			io_close(local_image_handle);
		}
	}
	return result;
}


static int open_memmap(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	result = io_dev_init(memmap_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(memmap_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using Memmap\n");
			io_close(local_image_handle);
		}
	}
	return result;
}

#ifdef CORTINA_BOOT_SERIAL_NOR
static int open_cortina_serial_nor_map(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	result = io_dev_init(cortina_serial_nor_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(cortina_serial_nor_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using Serial NOR Flash\n");
			io_close(local_image_handle);
		}
	}
	return result;
}
#endif

#ifdef CORTINA_BOOT_PARALLEL_NAND
static int open_parallel_nand_flash_map(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;
	result = io_dev_init(cortina_parallel_nand_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(cortina_parallel_nand_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using Parallel NAND Flash\n");
			io_close(local_image_handle);
		}
	}
	return result;
}
#endif

#ifdef CORTINA_BOOT_EMMC

unsigned int dw_mmc_init_setup_done = 0;
void dw_mmc_init_setup(uint8_t boot_device_info) {
	GLOBAL_SD_DLL_CTRL_t g3_global_sd_dll_ctrl;

	GLOBAL_PIN_MUX_t g3_global_pin_mux;
	dw_mmc_params_t params;

	if (dw_mmc_init_setup_done)
		return;
	dw_mmc_init_setup_done = 1;

	params.reg_base = CORTINA_DW_MMC_REG_BASE;
#ifdef IMAGE_BL1
	params.desc_base = CORTINA_BL1_MMC_DESC_BASE;
	params.desc_size = CORTINA_BL1_MMC_DESC_SIZE;
#else /* IMAGE_BL1 */
	params.desc_base = CORTINA_MMC_DESC_BASE;
	params.desc_size = CORTINA_MMC_DESC_SIZE;
#endif /* IMAGE_BL1 */
	params.clk_rate = CORTINA_DW_MMC_CLK;
	params.io_clk = 12500000;		// Default IO clock is 12.5 MHz
	params.bus_width = EMMC_BUS_WIDTH_1;

	// ACARR - added for auto-detection and device selection
	// We believe here that if the strapping for SD is set (how we got here)
	// Then we use flash_type for the boot type
	/* bit0: atuo-detect
	  * bit1: 4bit width
	  * bit2: SD or MMC
	  * bit3: SD/eMMC boot
	  */
	if (boot_device_info & 0x8)	{
		// Check the Default boot type between eMMC(1) and SD(0)
		if (boot_device_info & 4)
			params.device_type=CORTINA_SDEMMMC_DEVICE_TYPE_EMMC;
		else
			params.device_type=CORTINA_SDEMMMC_DEVICE_TYPE_SD;

		// Check the Default width between 1-bit (0) and 4-bit (1)
		if (boot_device_info & 2)
			params.bus_width=EMMC_BUS_WIDTH_4;
		else
			params.bus_width=EMMC_BUS_WIDTH_1;

		// Check fixed or auto-detection scheme
		if (boot_device_info & 1)
			params.auto_det=CORTINA_SDEMMC_AUTO_DET_ON;
		else
			params.auto_det=CORTINA_SDEMMC_AUTO_DET_OFF;
	} else {
		// ACARR - I don't believe we should be here... but in case pick a default 
		// of auto-detect 1-bit - start SD
		params.bus_width = EMMC_BUS_WIDTH_1;
		params.device_type=CORTINA_SDEMMMC_DEVICE_TYPE_SD;
		params.auto_det=CORTINA_SDEMMC_AUTO_DET_ON;
	}

	g3_global_sd_dll_ctrl.wrd = mmio_read_32(GLOBAL_SD_DLL_CTRL);

	/* turn on DLL0 and DLL1 bypass per HW documentation */
	g3_global_sd_dll_ctrl.bf.sd_dll_shiftClk0_bypass = 1;
	g3_global_sd_dll_ctrl.bf.sd_dll_shiftClk1_bypass = 1;

	/* clear non-functional bits per HW documentation */
	g3_global_sd_dll_ctrl.bf.sd_dll_reset = 0;
	g3_global_sd_dll_ctrl.bf.sd_dll_shiftClk0_phaseSel = 0;
	g3_global_sd_dll_ctrl.bf.sd_dll_shiftClk1_phaseSel = 0;
	g3_global_sd_dll_ctrl.bf.sd_phase_reset_override = 0;

        HWSIM_PROGRESS_MARK(44);
	/* tuneable shifts, do not adjust for now. provide sample code below */
#if 0
	g3_global_sd_dll_ctrl.bf.sd_drv_phase = ???;
	g3_global_sd_dll_ctrl.bf.sd_sample_phase = ???;
#endif /* 0 */

	/* G3 can support 3 different input clocks, 50, 100, or 200Mhz */
	// ACARR - to be clear, this is the input clock rate to the Synopsys IP and not the actual wire clock necessarily
	// This is because the Synopsys IP will divide this clock based on the desired frequency output and the input reference clock
	// This is done in the 
	switch(params.clk_rate)
	{
		case 50000000	:
			g3_global_sd_dll_ctrl.bf.sd_clk_sel = 0;
			break;
		case 100000000 :
			g3_global_sd_dll_ctrl.bf.sd_clk_sel = 1;
			break;
		case 200000000 :
			g3_global_sd_dll_ctrl.bf.sd_clk_sel = 2;
			break;
		default :
			WARN("SD/eMMC: unknown SD clock requested, reverting to 50Mhz\n");
			params.clk_rate = 50000000;
			g3_global_sd_dll_ctrl.bf.sd_clk_sel = 0;
			break;
	 }
	mmio_write_32(GLOBAL_SD_DLL_CTRL, g3_global_sd_dll_ctrl.wrd);
	HWSIM_PROGRESS_MARK(45);

	// This is already okay for the initialization specific patch set (ACARR)
	/* configure IO PIN mux to pass through
	 * SD/MMC reset and voltage select signals
	 */
	g3_global_pin_mux.wrd = mmio_read_32(GLOBAL_PIN_MUX);
	g3_global_pin_mux.bf.iomux_sd_volt_reg = 1;
	g3_global_pin_mux.bf.iomux_sd_mmc_resetn = 1;
	mmio_write_32(GLOBAL_PIN_MUX, g3_global_pin_mux.wrd);

#ifdef CORTINA_OTP_SUPPORT
#ifdef  IMAGE_BL1
	/* Call dw_mmc_init if no hotfix here */
	BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_MMC_INIT_BEFORE, cortina_pOtpDrv);
	if(BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_MMC_INIT, cortina_pOtpDrv)==0)
#endif /*IMAGE_BL1*/
#endif /*CORTINA_OTP_SUPPORT*/
		dw_mmc_init(&params);
	HWSIM_PROGRESS_MARK(46);
	
#ifdef CORTINA_OTP_SUPPORT
#ifdef  IMAGE_BL1
	BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_MMC_AFTER, cortina_pOtpDrv);
#endif /*IMAGE_BL1*/
#endif /*CORTINA_OTP_SUPPORT*/

}

static int open_emmc(const uintptr_t spec)
{
	int result;
	uintptr_t local_handle;

	result = io_dev_init(emmc_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(emmc_dev_handle, spec, &local_handle);
		if (result == 0) {
			io_close(local_handle);
		}
	}
	return result;
}
#endif /* CORTINA_BOOT_EMMC */

static const char * partition_type(uint8_t boot_partition)
{
	return (boot_partition == CORTINA_BOOT_PARTITION_ACTIVE) ? "Active":"Standby";
}

int update_fip_spec(uint8_t boot_device, uint8_t boot_partition, plat_io_policy_t * policy)
{
	ptentry_t *ptn;
	io_block_spec_t * spec = (io_block_spec_t *) policy->image_spec;

	ptn = find_fip_partition(boot_partition);
	if (!ptn) {
		WARN("failed to find %s partition\n", partition_type(boot_partition));
		return -ENOENT;
	}
	VERBOSE("%s: name:%s, start:%x, length:%x\n",
		__func__, ptn->name, ptn->start, ptn->length);

	spec->offset = (uintptr_t) ptn->start;
	spec->length = (uintptr_t) ptn->length;

	return 0;
}


/* This function may be called to obtain the Parallel NAND or SD/eMMC configuration info.
 * If the device is configured as the primary boot device  then info is obtained
 * from the HW pin strapping register.
 * If the devices is configured as a secondary boot device, then info is obtained 
 * from an OTP field.
 * The format of the boot_device_info obtained is defined as follows:
 *
 * Bit 3 - If set, SD/eMMC info is in bits position 2:0. If clear, NAND info is in bit position 2:0
 * Bit 2:0 - SD/eMMC or Parallel NAND flash type info.
*/

uint8_t plat_get_boot_device_config_info(uint8_t boot_device, uint8_t * boot_device_info)
{

	int rc;

	/* boot_device_info is only applicable for eMMC/SD or NAND */
 	if((boot_device != CORTINA_BOOT_DEVICE_MMC) && (boot_device != CORTINA_BOOT_DEVICE_PARALLEL_NAND)) 
		return OTP_ERRNO_ARG_SANITY_CHECK;

	/* read the info from HW pin strap if the requested device is primary boot device */
	if( ((boot_device == CORTINA_BOOT_DEVICE_MMC) && (g3_global_strap.bf.sd == 1)) || 
            ((boot_device == CORTINA_BOOT_DEVICE_PARALLEL_NAND) && (g3_global_strap.bf.sd == 0)))
	{
		/* place into correct format as defined above */
		*boot_device_info = (g3_global_strap.bf.sd <<3) | g3_global_strap.bf.flash_type;
		return OTP_ERRNO_OK;
	}
 	/* otherwise, read the info from OTP if the requested device is a secondary boot device */
	else
	{
		/* this function will return OK, if read from OTP is successful */
		rc = cortina_otp_shadow_get_field(cortina_pOtpDrv, 
					(uint32_t) OTP_DATA_secondary_boot_device_info_BSTART, 
					(uint32_t) OTP_DATA_secondary_boot_device_info_BLEN,
					(uintptr_t) boot_device_info);	
		/* sanity check */
		if( ((boot_device == CORTINA_BOOT_DEVICE_PARALLEL_NAND) && ((*boot_device_info & 0xc) != 0x4)) ||
			((boot_device == CORTINA_BOOT_DEVICE_MMC) &&  ((*boot_device_info & 0x8) == 0))
           )
			rc = OTP_ERRNO_ARG_SANITY_CHECK;

			return rc;
	 }
}

// return NULL if failed check
plat_io_policy_t * get_multiboot_policy(unsigned int image_id, uint8_t boot_device, uint8_t boot_partition)
{
	plat_io_policy_t *policy = 0;
#ifdef  IMAGE_BL1
	uint8_t * pBL2BootTuple = (uint8_t *) MULTIBOOT_BL2_BOOT_TUPLE_ADDR;
	uint8_t bootTuple;
#endif /* IMAGE_BL1 */
	
	if(image_id == FIP_IMAGE_ID)
	{
#ifdef  IMAGE_BL1
		/* Record each boot tuple requested by the BL1 image. This location will be continually overwriten 
		 after each multiboot failure such that the final value read by BL2 will contain 
		 the actual boot device and partition source of the loaded BL2 image.
		 */

		bootTuple = ((boot_device & 0x7) << 1) | (boot_partition & 0x1);
		*pBL2BootTuple = bootTuple;
#endif /* IMAGE_BL1 */

#ifndef CORTINA_G3_ASIC
		if(boot_device == CORTINA_BOOT_DEVICE_DRAM_DEV_MEMMAP)
		{
			policy = &policies[FIP_IMAGE_ID];
			policy->dev_handle= &memmap_dev_handle;
			fip_block_spec=memmap_fip_block_spec;
			policy->image_spec = (uintptr_t)&fip_block_spec;
			policy->check = &open_memmap;
			return policy;
		}
#endif
#ifdef CORTINA_BOOT_SERIAL_NOR
		if(boot_device == CORTINA_BOOT_DEVICE_SERIAL_NOR)
		{
			policy = &policies[FIP_IMAGE_ID];
			plat_io_serial_nor_init();
			policy->dev_handle= &cortina_serial_nor_dev_handle;
			fip_block_spec = cortina_serial_nor_fip_block_spec;
			policy->image_spec = (uintptr_t)&fip_block_spec;
			policy->check = &open_cortina_serial_nor_map;
			int result = policy->check(policy->image_spec);
			if (result != 0) {
				INFO("SERIAL NOR is not good\n");
				policy = 0;
			} else {
				get_partition(boot_device, boot_partition, policy);
			}
			INFO("get_partition returned\n");
			return policy;
		}
#endif /* CORTINA_BOOT_SERIAL_NOR */
#ifdef CORTINA_BOOT_PARALLEL_NAND
		if(boot_device == CORTINA_BOOT_DEVICE_PARALLEL_NAND)
		{
			policy = &policies[FIP_IMAGE_ID];
			NOTICE("boot_device PARALLEL NAND policy %llx\n", (long long unsigned int)policy);
			cortina_parallel_nand_ctrl_init(FLASH_ID);
			policy->dev_handle= &cortina_parallel_nand_dev_handle;
			fip_block_spec = cortina_parallel_nand_fip_block_spec;
			policy->image_spec = (uintptr_t)&fip_block_spec;
			policy->check = &open_parallel_nand_flash_map;
			HWSIM_PROGRESS_MARK(0x55);
			int result = policy->check(policy->image_spec);
			HWSIM_PROGRESS_MARK(0x56);
			if (result != 0) {
				HWSIM_PROGRESS_MARK(0x56);                  
				ERROR("PARALLEL_NAND is not good\n");
				policy = 0;
			} else {
				HWSIM_PROGRESS_MARK(0x57);
				get_partition(boot_device, boot_partition, policy);
				HWSIM_PROGRESS_MARK(0x58);
			}
			HWSIM_PROGRESS_MARK(0x59);
			INFO("get_partition returned\n");
			return policy;
		}
#endif /* CORTINA_BOOT_PARALLEL_NAND */
#ifdef CORTINA_BOOT_EMMC
		if(boot_device == CORTINA_BOOT_DEVICE_MMC)
		{
			uint8_t boot_device_info;
			policy = &policies[FIP_IMAGE_ID];
			policy->dev_handle= &emmc_dev_handle;
			fip_block_spec= emmc_fip_spec;
			policy->image_spec = (uintptr_t)&fip_block_spec;
			policy->check = &open_emmc;

			// we may not initialize dw_mmc yet if strap.sd pin is not set but we want to boot from SD as multiboot specified.
			plat_get_boot_device_config_info(boot_device, &boot_device_info);
			dw_mmc_init_setup(boot_device_info);

			get_partition(boot_device, boot_partition, policy);
			return policy;
		}
#endif /* CORTINA_BOOT_EMMC */
		/* default */
		policy = &policies[image_id];
		return policy;
	}
	else
	{
		policy = &policies[image_id];
		return policy;
	}
}

void plat_io_setup(void)
{
	int io_result;

#ifdef CORTINA_BOOT_EMMC
	io_result = register_io_dev_block(&emmc_dev_con);
	assert(io_result == 0);
#endif /* CORTINA_BOOT_EMMC */

	io_result = register_io_dev_fip(&fip_dev_con);
	assert(io_result == 0);

	io_result = register_io_dev_memmap(&memmap_dev_con);
	assert(io_result == 0);

#ifdef CORTINA_BOOT_PARALLEL_NAND
	io_result = register_io_dev_cortina_parallel_nand(&cortina_parallel_nand_dev_con);
	assert(io_result == 0);
#endif /* CORTINA_BOOT_PARALLEL_NAND */

#ifdef CORTINA_BOOT_SERIAL_NOR
	io_result = register_io_dev_cortina_serial_nor(&cortina_serial_nor_dev_con);
	assert(io_result == 0);
	if(cortina_serial_nor_dev_con == NULL)
		printf("serial_nor_dev_con ==NULL !\n");
#endif /* CORTINA_BOOT_SERIAL_NOR */

#ifdef CORTINA_BOOT_EMMC
	io_result = io_dev_open(emmc_dev_con, (uintptr_t)&emmc_dev_spec,
					&emmc_dev_handle);
	assert(result == 0);
#endif /* CORTINA_BOOT_EMMC */

	/* Open connections to devices and cache the handles */
	io_result = io_dev_open(fip_dev_con, (uintptr_t)NULL,
				&fip_dev_handle);
	assert(io_result == 0);

	io_result = io_dev_open(memmap_dev_con, (uintptr_t)NULL,
				&memmap_dev_handle);
	assert(io_result == 0);

#ifdef CORTINA_BOOT_PARALLEL_NAND
	io_result = io_dev_open(cortina_parallel_nand_dev_con, (uintptr_t)NULL,
				&cortina_parallel_nand_dev_handle);
	assert(io_result == 0);
#endif /* CORTINA_BOOT_PARALLEL_NAND */

#ifdef CORTINA_BOOT_SERIAL_NOR
	io_result = io_dev_open(cortina_serial_nor_dev_con, (uintptr_t)NULL,
				&cortina_serial_nor_dev_handle);
	assert(io_result == 0);
#endif /* CORTINA_BOOT_SERIAL_NOR */

	/* Ignore improbable errors in release builds */
	(void)io_result;
}


#ifdef CORTINA_MULTIBOOT_SUPPORT
int plat_set_next_otp_fip_device()
{
	int result;
	plat_io_policy_t *policy;
	uint8_t boot_device;
	uint8_t boot_partition;
	uint8_t * pShadow = (uint8_t *) (OTP_SHADOW_BASE + OTP_DATA_forced_boot_order_BSTART/8);

	/* 4 bit tuple, 3 bits boot device followed by 1 bit partition no */
	/* boot device 7 means skip, 0 means also skip */
	uint8_t bootTuple;


	while(current_otp_boot_fip_index < OTP_DATA_forced_boot_order_BLEN/4)
	{
		bootTuple = pShadow[current_otp_boot_fip_index/2];
		INFO("bootTuples: %x\n", bootTuple);
		if (current_otp_boot_fip_index % 2) {
			bootTuple = bootTuple >> 4;
		} else {
			bootTuple &= 0xF;
		}
		if(bootTuple!=0){
			NOTICE("current_otp_boot_fip_index %d\n", current_otp_boot_fip_index);
			NOTICE("bootTuple: %x\n", bootTuple);
		}
		/* increment now in case we issue a return below but 
		 * need to return to this function again to continue where we left off 
		 */
		current_otp_boot_fip_index++;

#ifdef  IMAGE_BL1
		BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_MULTIBOOT_NEXT_DEVICE, (void *) &bootTuple);
#endif /* IMAGE_BL1 */


		boot_device = bootTuple>>1;
		boot_device &= 0x7;
		boot_partition = bootTuple;
		boot_partition &= 0x1;

		/* if we find an unprogrammed Tuple, skip it and find the next one */
		if(boot_device == CORTINA_BOOT_DEVICE_NONE ){
			continue;
		}
	
		/* if we find an skip Tuple, skip it and find the next one */
		if(boot_device == CORTINA_BOOT_DEVICE_SKIP){
			NOTICE("OTP Boot Index: Found SKIP tuple at index %d\n",
				current_otp_boot_fip_index-1);
			continue;
		}

		/* get and set the next boot policy */
		NOTICE("boot_device %d boot_partition (%s)\n", boot_device, partition_type(boot_partition));
		policy = get_multiboot_policy(FIP_IMAGE_ID, boot_device, boot_partition);
		if (policy == 0) {
			WARN("OTP Boot Index: Could not find FIB at tuple index %d\n", current_otp_boot_fip_index-1);
			continue;
		}
		/* determine if we can read a FIP on this device */
		result = policy->check(policy->image_spec);
		if (result == 0){
			NOTICE("OTP Boot Index: Found FIP data at tuple index %d\n",
				current_otp_boot_fip_index-1);
			return result;
		} 
		else 
		{
			/* we failed, skip to the next boot device */
			WARN("OTP Boot Index: Could not find FIP data at tuple index %d\n",
				current_otp_boot_fip_index-1);
			continue;
		}
	}
	WARN("OTP Boot Index: No more devices to try %d\n", current_otp_boot_fip_index-1);
	return -ENOENT;
}
#endif /* CORTINA_MULTIBOOT_SUPPORT */

int plat_io_get_alt_image_source(
	unsigned int image_id __attribute__((unused)),
	uintptr_t *dev_handle __attribute__((unused)),
	uintptr_t *image_spec __attribute__((unused)))
{
#ifdef CORTINA_MULTIBOOT_SUPPORT
	int rc;
	rc = plat_set_next_otp_fip_device();
	if(rc == 0) 
	 {
	  /* try it again */
	  return plat_get_image_source(image_id, dev_handle, image_spec);
	 }
	else 
         {
	  /* give up for now. Reset to the boot strap device */
  	  plat_set_pinstrap_fip_device();
	  return -ENOENT;
	 }
#else /* CORTINA_MULTIBOOT_SUPPORT */
	  return -ENOENT;
#endif /* CORTINA_MULTIBOOT_SUPPORT */
}

#ifdef CORTINA_MULTIBOOT_SUPPORT
void plat_set_pinstrap_fip_device(void)
{
	uint8_t boot_device;
	uint8_t boot_partition;
#ifdef CORTINA_MULTIBOOT_ONE_SHOT_SUPPORT
	uint32_t	g3_global_software2;
	uint32_t	osb_pattern_blen;
	uint32_t	osb_pattern_bmask;
#ifdef IMAGE_BL1
	uint32_t	osb_pattern_bl1;
#endif /* IMAGE_BL1 */
	uint32_t	osb_pattern_bl2;
#ifdef IMAGE_BL2
	uint32_t	osb_pattern_done;
#endif /* IMAGE_BL2 */
	uint32_t	osb_boot_tuple;
	uint8_t 	osb_reduced_pattern = 0;
	int		rc;

	g3_global_software2 = mmio_read_32(GLOBAL_SOFTWARE2);
#endif /* CORTINA_MULTIBOOT_ONE_SHOT_SUPPORT */
	/* reset otp index back to first alternate device */
	current_otp_boot_fip_index = 0;

	/* get pin strapping to determine default boot device */
	g3_global_strap.wrd = mmio_read_32(GLOBAL_STRAP);

	/* boot from SD ? */
#ifdef TEST_OTP
	g3_global_strap.bf.sd = 0; // force sd 0
	TEST_INFO("g3_global_strap.wrd 0x%x\n", g3_global_strap.wrd);
	TEST_INFO("g3_global_strap.bf.flash_type %d\n", g3_global_strap.bf.flash_type);
#endif
#ifdef CORTINA_BOOT_EMMC
	if(g3_global_strap.bf.sd == 1)
	{
		HWSIM_PROGRESS_MARK(57) ; // 0x39
		NOTICE("boot from SD\n");
		boot_device = CORTINA_BOOT_DEVICE_MMC;
		boot_partition = CORTINA_BOOT_PARTITION_ACTIVE;

		goto boot_device_demux;
	}
	/* boot from serial flash */
	else
#endif
	if(g3_global_strap.bf.flash_type == 0)
	{
		HWSIM_PROGRESS_MARK(0x40);
		NOTICE("boot from serial flash\n");
		boot_device = CORTINA_BOOT_DEVICE_SERIAL_NOR;
		boot_partition = CORTINA_BOOT_PARTITION_ACTIVE;
		goto boot_device_demux;
	}
	/* use reserved type 0x3 for DRAM Dev. MEMMAP */
#ifndef CORTINA_G3_ASIC
	else if(g3_global_strap.bf.flash_type == 0x3)
	{
		HWSIM_PROGRESS_MARK(0x41);
		NOTICE("boot from MEMMAP\n");
		boot_device = CORTINA_BOOT_DEVICE_DRAM_DEV_MEMMAP;
		boot_partition = CORTINA_BOOT_PARTITION_ACTIVE;
		goto boot_device_demux;
	}
#endif	
	else if(g3_global_strap.bf.flash_type & 0x4)
	{
		HWSIM_PROGRESS_MARK(0x42);
		NOTICE("boot from PARALLEL NAND\n");
		boot_device = CORTINA_BOOT_DEVICE_PARALLEL_NAND;
		boot_partition = CORTINA_BOOT_PARTITION_ACTIVE;
		goto boot_device_demux;
	}
	else
	{
		HWSIM_PROGRESS_MARK(0x43);
		WARN("boot from unknown, default to PARALLEL NAND\n");
		boot_device = CORTINA_BOOT_DEVICE_PARALLEL_NAND;
		boot_partition = CORTINA_BOOT_PARTITION_ACTIVE;
	}
boot_device_demux:
#ifdef CORTINA_MULTIBOOT_ONE_SHOT_SUPPORT
	/*  One Shot Boot (OSB) can support two different kinds of patterns in the GLOBAL_SOFTWARE2 register
		to detect an OSB request requested by Customer SW upgrade.
		#1. By default, a UNIQUE 3 character ASCII string pattern found in the 
	               lower 24 bits of the register.
		   or
		#2. A simple 2 bit request, found in the lower 2 bits of the register. 

		An OTP bit may be perminently set to indicate which kind pattern will be used for OSB requests.
		If no OTP bit is set, then the OSB support default to looking for a unique 24 pattern 
		in the GLOBAL_SOFTWARE2 register upon boot to detect on OSB request. This allows the to
		temporarly request a a single boot attempt from a specific boot device and partition.
		In additition, the customer may pass 4 bits worth of arbitrary customer specific data.
		If 4 bits is not enough, then the customer by set a bit in the OTP to reduce the OSB 
		detection pattern from 24 bits to only 2 bits. This effectively allows bits normally 
		used for pattern detect to be used to pass up to 26 bits of arbitrary customer data
		instead of only 4.  However, in order to support a simpler 2 bit OSB request pattern, 
		2 bits must ALWAYS be reserved away from the GLOBAL_SOFTWARE2 reg from general customer use. 

              Definition of 24 bit or 2 bit pattern in in lower bits of the register. Respectively,
              unknown or 00b - No One Shot Boot Request. Ignore. Boot normally.
              "osb" or 10b - BL1 One Shot Boot Request.
              "Osb" or 01b - BL2 One Shot Boot Request. Set by BL1 when passing OSB Req to BL2.
              "OSB" or 11b - One Shot Boot Request fully processed by BL2.

              When One Shot Boot Request is processed the next 4 bits indicate the Boot Device and partition.
		The rest of the upper bits may be used by the customer to pass arbitrary data.
	*/

	rc = cortina_otp_shadow_field_is_programmed(cortina_pOtpDrv, 
			OTP_DATA_one_shot_boot_reduced_pattern_BSTART, 
			OTP_DATA_one_shot_boot_reduced_pattern_BLEN, &osb_reduced_pattern);
	if(rc != OTP_ERRNO_OK)
	{
		ERROR("multiboot: osb: could not read one_shot_boot_reduced_pattern bit\n");
		/* default to not set */
		osb_reduced_pattern = 0;
	}

	if(osb_reduced_pattern)
	{
		/* use reduced 2 bit pattern, which reserves 2 bits globally */
#ifdef IMAGE_BL1
		osb_pattern_bl1 = OSB_2_BIT_BL1_SIG; 
#endif /* IMAGE_BL1 */
		osb_pattern_bl2 = OSB_2_BIT_BL2_SIG;
#ifdef IMAGE_BL2
		osb_pattern_done = OSB_2_BIT_DONE_SIG;
#endif /* IMAGE_BL2 */
		osb_pattern_blen = OSB_2_BIT_SIG_BLEN;
		osb_pattern_bmask = OSB_2_BIT_SIG_MASK;
	 }
	else
	{
#ifdef IMAGE_BL1
		osb_pattern_bl1 = OSB_24_BIT_BL1_SIG; 
#endif /* IMAGE_BL1 */
		osb_pattern_bl2 = OSB_24_BIT_BL2_SIG;
#ifdef IMAGE_BL2
		osb_pattern_done = OSB_24_BIT_DONE_SIG;
#endif /* IMAGE_BL2 */
		osb_pattern_blen = OSB_24_BIT_SIG_BLEN;
		osb_pattern_bmask = OSB_24_BIT_SIG_MASK;
	 }

#ifdef IMAGE_BL1
	if((g3_global_software2 & osb_pattern_bmask) == osb_pattern_bl1) 
	{
		osb_boot_tuple = g3_global_software2 >> osb_pattern_blen;
		boot_device = (osb_boot_tuple >> 1) & 0x7;
		boot_partition = osb_boot_tuple & 1;
			
		/* request BL2 OSB request */
		g3_global_software2 &= ~osb_pattern_bmask;	/* Clear BL1 req */
		g3_global_software2 |= osb_pattern_bl2;		/* Set BL2 req */
		mmio_write_32(GLOBAL_SOFTWARE2, g3_global_software2);
		NOTICE("One Shot boot: Device:%d, partition %x\n", boot_device, boot_partition);
	}
#endif /* IMAGE_BL1 */
#ifdef IMAGE_BL2
	if( (g3_global_software2 & osb_pattern_bmask) == osb_pattern_bl2) 
	{
		osb_boot_tuple = g3_global_software2 >> osb_pattern_blen;
		boot_device = (osb_boot_tuple >> 1) & 0x7;
		boot_partition = osb_boot_tuple & 1;
 
		/* Mark OSB request done */
		g3_global_software2 &= ~osb_pattern_bmask;      /* Clear BL2 req */
		g3_global_software2 |= osb_pattern_done;        /* Mark as Done */
		mmio_write_32(GLOBAL_SOFTWARE2, g3_global_software2);
	}
#endif /* IMAGE_BL2 */
 	
#endif /* CORTINA_MULTIBOOT_ONE_SHOT_SUPPORT */
	get_multiboot_policy(FIP_IMAGE_ID, boot_device, boot_partition);
}
#endif /* CORTINA_MULTIBOOT_SUPPORT */

/* Return an IO device handle and specification which can be used to access
 * an image. Use this to enforce platform load policy */
int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	int result;
	plat_io_policy_t *policy;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	result = policy->check(policy->image_spec);
	if (result == 0) {
		*image_spec = policy->image_spec;
		*dev_handle = *(policy->dev_handle);
	} else {
                 HWSIM_PROGRESS_MARK(0x44);
		VERBOSE("Trying alternative IO\n");
		result = plat_io_get_alt_image_source(image_id, dev_handle,
							image_spec);
	}

	return result;
}

/*
 * See if a Firmware Image Package is available,
 * by checking if TOC is valid or not.
 */
int plat_io_is_toc_valid(void)
{
	int result;

	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);

	return (result == 0);
}

