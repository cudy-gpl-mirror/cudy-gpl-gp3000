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
#include <console.h>
#include <debug.h>
#include <partitions.h>
#include <platform.h>
#include <platform_def.h>
#include <string.h>
#include <mmio.h>
#include INC_CORTINA_SOC_REGS_HEADER
#include INC_PLAT_DEF_HEADER
#include INC_PLAT_PRIVATE_HEADER
#ifdef CORTINA_OTP_SUPPORT
#include <cortina_otp.h>
#include INC_CORTINA_PLATFORM_OTP_DATA_HEADER
#endif
#ifdef CORTINA_BOOT_EMMC
#include <dw_mmc.h>
#include <emmc.h>
#endif /* CORTINA_BOOT_EMMC */
#ifdef MIPS_R3K_TIMER
#include <r3k_timer.h> 
#endif /* MIPS_R3K_TIMER */

GLOBAL_STRAP_t saturn_global_strap;

/*******************************************************************************
 * Declarations of linker defined symbols which will help us find the layout
 * of trusted RAM
 ******************************************************************************/
extern unsigned long __COHERENT_RAM_START__;
extern unsigned long __COHERENT_RAM_END__;

#ifdef CORTINA_OTP_SUPPORT
CORTINA_OTP_DRV_t * cortina_pOtpDrv;
#endif /* CORTINA_OTP_SUPPORT */

/*
 * The next 2 constants identify the extents of the code & RO data region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __RO_START__ and __RO_END__ linker symbols refer to page-aligned addresses.
 */
#define BL2_RO_BASE (unsigned long)(&__RO_START__)
#define BL2_RO_LIMIT (unsigned long)(&__RO_END__)

#if USE_COHERENT_MEM
/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols refer to
 * page-aligned addresses.
 */
#define BL2_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL2_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)
#endif

/* Data structure which holds the extents of the trusted SRAM for BL2 */
static meminfo_t bl2_tzram_layout;


/*******************************************************************************
 * Structure which holds the arguments which need to be passed to BL3-1
 ******************************************************************************/
static bl2_to_bl31_params_mem_t bl31_params_mem;


meminfo_t *bl2_plat_sec_mem_layout(void)
{
        return &bl2_tzram_layout;
}

/*******************************************************************************
 * This function assigns a pointer to the memory that the platform has kept
 * aside to pass platform specific and trusted firmware related information
 * to BL31. This memory is allocated by allocating memory to
 * bl2_to_bl31_params_mem_t structure which is a superset of all the
 * structure whose information is passed to BL31
 * NOTE: This function should be called only once and should be done
 * before generating params to BL31
 ******************************************************************************/
bl31_params_t *bl2_plat_get_bl31_params(void)
{
        bl31_params_t *bl2_to_bl31_params;

        /*
         * Initialise the memory for all the arguments that needs to
         * be passed to BL3-1
         */
        memset(&bl31_params_mem, 0, sizeof(bl2_to_bl31_params_mem_t));

        /* Assign memory for TF related information */
        bl2_to_bl31_params = &bl31_params_mem.bl31_params;
        SET_PARAM_HEAD(bl2_to_bl31_params, PARAM_BL31, VERSION_1, 0);
#if 0
        /* Fill BL3-1 related information */
        bl2_to_bl31_params->bl31_image_info = &bl31_params_mem.bl31_image_info;
        SET_PARAM_HEAD(bl2_to_bl31_params->bl31_image_info, PARAM_IMAGE_BINARY,
                VERSION_1, 0);

        /* Fill BL3-2 related information if it exists */
#if BL32_BASE
        bl2_to_bl31_params->bl32_ep_info = &bl31_params_mem.bl32_ep_info;
        SET_PARAM_HEAD(bl2_to_bl31_params->bl32_ep_info, PARAM_EP,
                VERSION_1, 0);
        bl2_to_bl31_params->bl32_image_info = &bl31_params_mem.bl32_image_info;
        SET_PARAM_HEAD(bl2_to_bl31_params->bl32_image_info, PARAM_IMAGE_BINARY,
                VERSION_1, 0);
#endif
#endif /* 0 */

        /* Fill BL3-3 related information */
        bl2_to_bl31_params->bl33_ep_info = &bl31_params_mem.bl33_ep_info;
        SET_PARAM_HEAD(bl2_to_bl31_params->bl33_ep_info,
                PARAM_EP, VERSION_1, 0);
#if 0
        /* BL3-3 expects to receive the primary CPU MPID (through x0) */
        bl2_to_bl31_params->bl33_ep_info->args.arg0 = 0xffff & read_mpidr();
#endif /* 0 */

        bl2_to_bl31_params->bl33_image_info = &bl31_params_mem.bl33_image_info;
        SET_PARAM_HEAD(bl2_to_bl31_params->bl33_image_info, PARAM_IMAGE_BINARY,
                VERSION_1, 0);

        return bl2_to_bl31_params;
}

#if 0
void init_boardid(void)
{
	unsigned int reg;

	/* Set chip id to sram */
	reg = read_midr_el1();
	mmio_write_32(MEMORY_AXI_CHIP_ADDR, reg);
	INFO("[BDID] [%x] midr: 0x%x\n", MEMORY_AXI_CHIP_ADDR, reg);

	/* Set board type to sram */
	mmio_write_32(MEMORY_AXI_BOARD_TYPE_ADDR, 0x0);
	INFO("[BDID] [%x] board type: 0\n", MEMORY_AXI_BOARD_TYPE_ADDR);

	/* Set board id to sram */
	mmio_write_32(MEMORY_AXI_BOARD_ID_ADDR, 0x2b);
	INFO("[BDID] [%x] board id: 0x2b\n", MEMORY_AXI_BOARD_ID_ADDR);

	mmio_write_32(ACPU_ARM64_FLAGA, 0x1234);
	mmio_write_32(ACPU_ARM64_FLAGB, 0x5678);
	return;
}
#endif

void bl2_init_ddr_ctrl()
{
	GLOBAL_BLOCK_RESET_t globalBlockResetReg;
	globalBlockResetReg.wrd = mmio_read_32(KSEG1_ATU_XLAT(GLOBAL_BLOCK_RESET));
	globalBlockResetReg.bf.reset_sdram = 0;
	mmio_write_32(KSEG1_ATU_XLAT(GLOBAL_BLOCK_RESET), globalBlockResetReg.wrd);

	DDR_CTRL_DDR_SCH_CTRL_t ddr_sch_ctrl;
	/* Just configure ddr_sch_ctrl for FGPA */
	ddr_sch_ctrl.wrd = 0x01f74010;
	mmio_write_32(KSEG1_ATU_XLAT(DDR_CTRL_DDR_SCH_CTRL), ddr_sch_ctrl.wrd);
}

/*******************************************************************************
 * BL1 has passed the extents of the trusted RAM that should be visible to BL2
 * in x0. This memory layout is sitting at the base of the free trusted RAM.
 * Copy it to a safe loaction before its reclaimed by later BL2 functionality.
 ******************************************************************************/
void bl2_early_platform_setup(meminfo_t *mem_layout)
{
    uint32_t otp_clock_mhz = plat_get_otp_clock_mhz();
    uint32_t cpu_clock_mhz = plat_get_cpu_clock_mhz();
    uint32_t per_clock_mhz  = plat_get_per_clock_mhz();

#ifdef MIPS_R3K_TIMER
    r3k_timer_init(cpu_clock_mhz);
#endif

	/* Initialize the console to provide early debug support */
	console_init(CONSOLE_BASE, per_clock_mhz * 1000000, CORTINA_PER_UART_BAUDRATE);

	/* Init DDR controller */
	bl2_init_ddr_ctrl();

	/* Setup the BL2 memory layout */
	bl2_tzram_layout = *mem_layout;
#if 0
	init_boardid();
#endif 

#ifdef CORTINA_OTP_SUPPORT
	cortina_pOtpDrv = cortina_otp_init(	(uintptr_t) OTP_BASE,
			(uint32_t) OTP_BIT_SIZE,
			(uintptr_t) OTP_SHADOW_BASE,
			(uint32_t) OTP_READ_TIMEOUT,
			(uint32_t) OTP_WRITE_TIMEOUT,
			(uint32_t) otp_clock_mhz*2 /* clock passed needs to be 2x OTP clock
						    * because called function divides by 2 internally */
		       );
	if(cortina_pOtpDrv == NULL)
		INFO("CORTINA OTP initialized FAILED\n");
#endif

	/* FIP mmip driver */
	plat_io_setup();
#ifdef CORTINA_MULTIBOOT_SUPPORT
        plat_set_pinstrap_fip_device();
#endif /* CORTINA_MULTIBOOT_SUPPORT */
}

void bl2_platform_setup(void)
{
}


/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment nothing is required.
 ******************************************************************************/
void bl2_plat_arch_setup(void)
{

}


/*******************************************************************************
 * Before calling this function BL33 is loaded in memory and its entrypoint
 * is set by load_image. This is a placeholder for the platform to change
 * the entrypoint of BL33.
 ******************************************************************************/
void bl2_plat_set_bl33_ep_info(image_info_t *image,
				       entry_point_info_t *bl33_ep_info)
{

}

/* Flush the TF params and the TF plat params */
void bl2_plat_flush_bl31_params(void)
{
#if 0  /* TBD */
        flush_dcache_range((unsigned long)&bl31_params_mem,
                        sizeof(bl2_to_bl31_params_mem_t));
#endif
}


/*******************************************************************************
 * Populate the extents of memory available for loading BL3-3
 ******************************************************************************/
void bl2_plat_get_bl33_meminfo(meminfo_t *bl33_meminfo)
{
	bl33_meminfo->total_base = DRAM_NS_BASE;
	bl33_meminfo->total_size = DRAM_NS_SIZE;
	bl33_meminfo->free_base = DRAM_NS_BASE;
	bl33_meminfo->free_size = DRAM_NS_SIZE;
}
