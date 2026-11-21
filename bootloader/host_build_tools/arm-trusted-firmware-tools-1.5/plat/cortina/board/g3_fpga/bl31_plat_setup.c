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

#include <arch.h>
#include <arch_helpers.h>
#include <arm_gic.h>
#include <assert.h>
#include <bl31.h>
#include <bl_common.h>
#include <cci.h>
#include <console.h>
#include <mmio.h>
#include <platform.h>
#include <stddef.h>
#include <gic_v2.h>
#include INC_PLAT_PRIVATE_HEADER
#include INC_PLAT_DEF_HEADER
#include INC_CORTINA_SOC_REGS_HEADER
#ifdef CORTINA_SECURE_SRAM
#include <cortina_secure_sram.h>
#endif /* CORTINA_SECURE_SRAM */
#ifdef CORTINA_PER_WATCHDOG
#include <cortina_per_watchdog.h>
#endif /* CORTINA_PER_WATCHDOG */
#include <generic_delay_timer.h>


/*******************************************************************************
 * Declarations of linker defined symbols which will help us find the layout
 * of trusted RAM
 ******************************************************************************/
extern unsigned long __RO_START__;
extern unsigned long __RO_END__;

extern unsigned long __COHERENT_RAM_START__;
extern unsigned long __COHERENT_RAM_END__;

/*
 * The next 3 constants identify the extents of the code, RO data region and the
 * limit of the BL3-1 image.  These addresses are used by the MMU setup code and
 * therefore they must be page-aligned.  It is the responsibility of the linker
 * script to ensure that __RO_START__, __RO_END__ & __BL31_END__ linker symbols
 * refer to page-aligned addresses.
 */
#define BL31_RO_BASE (unsigned long)(&__RO_START__)
#define BL31_RO_LIMIT (unsigned long)(&__RO_END__)
#define BL31_END (unsigned long)(&__BL31_END__)

#ifndef BL32_BASE
/* Value used to initialize Non-Secure IRQ priorities four at a time */
#define GICD_IPRIORITYR_DEF_VAL \
	(GIC_HIGHEST_NS_PRIORITY | \
	(GIC_HIGHEST_NS_PRIORITY << 8) | \
	(GIC_HIGHEST_NS_PRIORITY << 16) | \
	(GIC_HIGHEST_NS_PRIORITY << 24))
#endif /* BL32_BASE */

#if USE_COHERENT_MEM
/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols
 * refer to page-aligned addresses.
 */
#define BL31_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL31_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)
#endif

/******************************************************************************
 * Placeholder variables for copying the arguments that have been passed to
 * BL3-1 from BL2.
 ******************************************************************************/
#ifdef BL32_BASE
static entry_point_info_t bl32_ep_info;
#endif /* BL32_BASE */
static entry_point_info_t bl33_ep_info;

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for
 * the security state specified. BL3-3 corresponds to the non-secure image type
 * while BL3-2 corresponds to the secure image type. A NULL pointer is returned
 * if the image does not exist.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

#ifdef BL32_BASE
	next_image_info = (type == NON_SECURE) ? &bl33_ep_info : &bl32_ep_info;
#else /* if we dont support option BL32 optional, then it can only be bl33 */
	next_image_info = &bl33_ep_info;
#endif /* BL32_BASE */


	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

/*******************************************************************************
 * Perform any BL3-1 specific platform actions. Here is an opportunity to copy
 * parameters passed by the calling EL (S-EL1 in BL2 & S-EL3 in BL1) before they
 * are lost (potentially). This needs to be done before the MMU is initialized
 * so that the memory layout can be used while creating page tables. Also, BL2
 * has flushed this information to memory, so we are guaranteed to pick up good
 * data
 ******************************************************************************/
void bl31_early_platform_setup(bl31_params_t *from_bl2,
			       void *plat_params_from_bl2)
{
	generic_delay_timer_init();

	/* Initialize the console to provide early debug support */
	console_init(CONSOLE_BASE, CORTINA_PER_UART_CLK_IN_HZ, CORTINA_PER_UART_BAUDRATE);

        /*
         * Initialize CCI for this cluster during cold boot.
         * No need for locks as no other CPU is active.
         */
        plat_cci_init();

	/*
	 * Copy BL3-2 (if including this optional component) and BL3-3 entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
#ifdef BL32_BASE
	bl32_ep_info = *from_bl2->bl32_ep_info;
#endif /* BL32_BASE */
	bl33_ep_info = *from_bl2->bl33_ep_info;

        /*
         * Enable CCI coherency for the primary CPU's cluster.
         * Earlier bootloader stages might already do this (e.g. Trusted
         * Firmware's BL1 does it) but we can't assume so. There is no harm in
         * executing this code twice anyway.
         * Platform specific PSCI code will enable coherency for other
         * clusters.
         */
        cci_enable_snoop_dvm_reqs(MPIDR_AFFLVL1_VAL(read_mpidr()));
		mmio_write_32(PLAT_G3_CCI_BASE + SECURE_ACCESS_REG , 1);
}

static void init_rtc(void)
{
#if 0
	uint32_t data;
#endif

	/* TODO */
}

#ifndef BL32_BASE
void g3_gic_pcpu_distif_setup(unsigned int gicd_base)
{
	unsigned int index, sec_ppi_sgi_mask = 0;

	assert(gicd_base);

	/* Setup PPI priorities doing four at a time */
	for (index = 0; index < 32; index += 4) {
		gicd_write_ipriorityr(gicd_base, index,
				GICD_IPRIORITYR_DEF_VAL);
	}

	/*
	 * Invert the bitmask to create a mask for non-secure PPIs and
	 * SGIs. Program the GICD_IGROUPR0 with this bit mask. This write will
	 * update the GICR_IGROUPR0 as well in case we are running on a GICv3
	 * system. This is critical if GICD_CTLR.ARE_NS=1.
	 */
	gicd_write_igroupr(gicd_base, 0, ~sec_ppi_sgi_mask);
}

void g3_gic_distif_setup(unsigned int gicd_base)
{
	unsigned int index, num_ints;

	/*
	 * Mark out non-secure interrupts. Calculate number of
	 * IGROUPR registers to consider. Will be equal to the
	 * number of IT_LINES
	 */
	num_ints = gicd_read_typer(gicd_base) & IT_LINES_NO_MASK;
	num_ints = (num_ints + 1) << 5;
	for (index = MIN_SPI_ID; index < num_ints; index += 32)
		gicd_write_igroupr(gicd_base, index, ~0);

	/* Setup SPI priorities doing four at a time */
	for (index = MIN_SPI_ID; index < num_ints; index += 4) {
		gicd_write_ipriorityr(gicd_base, index,
				GICD_IPRIORITYR_DEF_VAL);
	}

	/*
	 * Configure the SGI and PPI. This is done in a separated function
	 * because each CPU is responsible for initializing its own private
	 * interrupts.
	 */
	g3_gic_pcpu_distif_setup(gicd_base);

	/* enable distributor */
	gicd_write_ctlr(gicd_base, ENABLE_GRP0 | ENABLE_GRP1);
}

/*******************************************************************************
 * Enable secure interrupts and set the priority mask register to allow all
 * interrupts to trickle in.
 ******************************************************************************/
void g3_gic_cpuif_setup(unsigned int gicc_base)
{
	unsigned int val;

	val = ENABLE_GRP0 | ENABLE_GRP1 | FIQ_EN | FIQ_BYP_DIS_GRP0;
	val |= IRQ_BYP_DIS_GRP0 | FIQ_BYP_DIS_GRP1 | IRQ_BYP_DIS_GRP1;

	gicc_write_ctlr(gicc_base, val);
	gicc_write_pmr(gicc_base, GIC_PRI_MASK);
}

#endif /* BL32_BASE */

/*******************************************************************************
 * Initialize the GIC.
 ******************************************************************************/
void bl31_platform_setup(void)
{
#ifdef CORTINA_SECURE_SRAM
CORTINA_SECURE_SRAM_DRV_t * pSecureSRAMDrv;
#endif /* CORTINA_SECURE_SRAM */
	/* Initialize the gic cpu and distributor interfaces */
	plat_gic_init();
	arm_gic_setup();

#ifndef BL32_BASE
	g3_gic_cpuif_setup(GICC_BASE);
	g3_gic_distif_setup(GICD_BASE);
#endif /* BL32_BASE */

	init_rtc();
#ifdef CORTINA_SECURE_SRAM
        /* initialize Secure SRAM driver */
        pSecureSRAMDrv = cortina_secure_sram_init((uintptr_t) CAPSRAM_TZCONTROL,
                                             (uintptr_t) SRAM_BASE,
                                             CORTINA_SECURE_SRAM_NO_CLEAR);
        /* protect the entire SRAM */
        if(pSecureSRAMDrv != NULL)
         cortina_secure_sram_protect_range( pSecureSRAMDrv,
                                        SRAM_BASE,
                                        SRAM_SIZE,
                                        CORTINA_SECURE_SRAM_NO_CLEAR);

        if(pSecureSRAMDrv != NULL)
#if PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID == PLAT_G3_TRUSTED_SRAM_ID
        /* unprotect and clear everything except BL3 and OTP Shadow */
         cortina_secure_sram_unprotect_range( pSecureSRAMDrv,
                                        SRAM_BASE,
                                        CORTINA_SECURE_SRAM_ROUND_DOWN_16KB((BL31_BASE - SRAM_BASE)),
					CORTINA_SECURE_SRAM_ALL_CLEAR);
#elif PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID == PLAT_G3_TRUSTED_DRAM_ID
        /* unprotect and clear everything except the final 16KB block
	 * containing the OTP Shadow at the end */
         cortina_secure_sram_unprotect_range( pSecureSRAMDrv,
                                        SRAM_BASE,
                                        (SRAM_SIZE - CORTINA_SECURE_SRAM_ROUND_UP_16KB((OTP_BIT_SIZE/8))),
					CORTINA_SECURE_SRAM_ALL_CLEAR);
#endif /* PLAT_G3_BL3_1_EL3_RAM_LOCATION_ID */
#endif /* CORTINA_SECURE_SRAM */
#ifdef CORTINA_PER_WATCHDOG
	CORTINA_PER_WATCHDOG_DRV_t * pWatchDogDrv;
	GLOBAL_GLOBAL_CONFIG_t g3_global_cfg;

	 pWatchDogDrv = cortina_per_watchdog_init((uintptr_t) (PER_WDT_CTRL),
					(APB_PCLK/1000),
					60,
					CORTINA_PER_WATCHDOG_UNIT_SEC
					);
        if(pWatchDogDrv == NULL)
		printf("Watchdog driver init failed!!\n");
        else
	  {
		g3_global_cfg.wrd = mmio_read_32(GLOBAL_GLOBAL_CONFIG);
		/* Enable WDT reset in GLOBAL_CONFIG */
		g3_global_cfg.bf.wd_reset_ext_reset = 1;
		g3_global_cfg.bf.wd_reset_remap = 1;
		g3_global_cfg.bf.wd_reset_all_blocks= 1;
		g3_global_cfg.bf.wd_reset_subsys_enable = 1;
		mmio_write_32(GLOBAL_GLOBAL_CONFIG, g3_global_cfg.wrd);

		/* enable system reset if watchdog expires */
		cortina_per_watchdog_enable_sys_reset(pWatchDogDrv);
	}
#endif /* CORTINA_PER_WATCHDOG */
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void bl31_plat_arch_setup()
{
        configure_mmu_el3(BL31_RO_BASE,
                              (BL31_END - BL31_RO_BASE),
                              BL31_RO_BASE,
                              BL31_RO_LIMIT
#if USE_COHERENT_MEM
                              , BL31_COHERENT_RAM_BASE,
                              BL31_COHERENT_RAM_LIMIT
#endif
                              );
}
