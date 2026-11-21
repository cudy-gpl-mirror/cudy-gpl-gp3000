/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <bl1.h>
#include <debug.h>
#include <errno.h>
#include <platform.h>
#include <platform_def.h>
#ifdef CORTINA_G3_TESTCODE
#ifdef TEST_ON_G3_ASIC_SIM
#include <mmio.h>
#include INC_CORTINA_SOC_REGS_HEADER
#endif
#endif /* CORTINA_G3_TESTCODE */
#ifdef CORTINA_OTP_SUPPORT
#include <cortina_otp.h>
#include INC_CORTINA_PLATFORM_OTP_DATA_HEADER
#endif /* CORTINA_OTP_SUPPORT */

/*
 * The following platform functions are weakly defined. They
 * are default implementations that allow BL1 to compile in
 * absence of real definitions. The Platforms may override
 * with more complex definitions.
 */
#pragma weak bl1_plat_get_next_image_id
#pragma weak bl1_plat_set_ep_info
#pragma weak bl1_plat_get_image_desc
#pragma weak bl1_plat_fwu_done
#pragma weak bl1_plat_handle_pre_image_load
#pragma weak bl1_plat_handle_post_image_load


unsigned int bl1_plat_get_next_image_id(void)
{
	/* BL2 load will be done by default. */
	return BL2_IMAGE_ID;
}

void bl1_plat_set_ep_info(unsigned int image_id,
		entry_point_info_t *ep_info)
{

#ifdef CORTINA_HOTFIX_CHECKPOINT_SUPPORT
        BL1_HOTFIX_CHECKPOINT(CHECKPOINT_ID_BEFORE_BL2, (void *)ep_info);
#endif /* CORTINA_HOTFIX_CHECKPOINT_SUPPORT */
}

int bl1_plat_handle_pre_image_load(unsigned int image_id)
{
	return 0;
}

/*
 * Following is the default definition that always
 * returns BL2 image details.
 */
image_desc_t *bl1_plat_get_image_desc(unsigned int image_id)
{
	static image_desc_t bl2_img_desc = BL2_IMAGE_DESC;
	return &bl2_img_desc;
}

__dead2 void bl1_plat_fwu_done(void *client_cookie, void *reserved)
{
	while (1)
		wfi();
}

/*
 * The Platforms must override with real definition.
 */
#pragma weak bl1_plat_mem_check

int bl1_plat_mem_check(uintptr_t mem_base, unsigned int mem_size,
		unsigned int flags)
{
	assert(0);
	return -ENOMEM;
}

/*
 * Default implementation for bl1_plat_handle_post_image_load(). This function
 * populates the default arguments to BL2. The BL2 memory layout structure
 * is allocated and the calculated layout is populated in arg1 to BL2.
 */
int bl1_plat_handle_post_image_load(unsigned int image_id)
{
	meminfo_t *bl2_tzram_layout;
	meminfo_t *bl1_tzram_layout;
	image_desc_t *image_desc;
	entry_point_info_t *ep_info;

	if (image_id != BL2_IMAGE_ID)
		return 0;

	/* Get the image descriptor */
	image_desc = bl1_plat_get_image_desc(BL2_IMAGE_ID);
	assert(image_desc != NULL);

	/* Get the entry point info */
	ep_info = &image_desc->ep_info;

	/* Find out how much free trusted ram remains after BL1 load */
	bl1_tzram_layout = bl1_plat_sec_mem_layout();

	/*
	 * Create a new layout of memory for BL2 as seen by BL1 i.e.
	 * tell it the amount of total and free memory available.
	 * This layout is created at the first free address visible
	 * to BL2. BL2 will read the memory layout before using its
	 * memory for other purposes.
	 */
#if LOAD_IMAGE_V2
	bl2_tzram_layout = (meminfo_t *) bl1_tzram_layout->total_base;
#else
	bl2_tzram_layout = (meminfo_t *) bl1_tzram_layout->free_base;
#endif /* LOAD_IMAGE_V2 */

#if !ERROR_DEPRECATED
	bl1_init_bl2_mem_layout(bl1_tzram_layout, bl2_tzram_layout);
#else
	bl1_calc_bl2_mem_layout(bl1_tzram_layout, bl2_tzram_layout);
#endif

	ep_info->args.arg1 = (uintptr_t)bl2_tzram_layout;

	VERBOSE("BL1: BL2 memory layout address = %p\n",
		(void *) bl2_tzram_layout);
	return 0;
}

#ifdef CORTINA_G3_TESTCODE
#if defined(TEST_ON_G3_ASIC_SIM) && defined(FORCE_MMIO_FUN)
void mmio_write_8(uintptr_t addr, uint8_t value)
{
        HWSIM_PROGRESS_MARK(0x1108);
        HWSIM_PROGRESS_VAL(addr);
        HWSIM_PROGRESS_VAL(value);
	*(volatile uint8_t*)addr = value;
}

uint8_t mmio_read_8(uintptr_t addr)
{
        volatile uint8_t value;
        HWSIM_PROGRESS_MARK(0x1208);
        HWSIM_PROGRESS_VAL(addr);
        value = *(volatile uint8_t*)addr;
        HWSIM_PROGRESS_VAL(value);
        return value;
}

void mmio_write_16(uintptr_t addr, uint16_t value)
{
        HWSIM_PROGRESS_MARK(0x1116);
        HWSIM_PROGRESS_VAL(addr);
        HWSIM_PROGRESS_VAL(value);
	*(volatile uint16_t*)addr = value;
}

uint16_t mmio_read_16(uintptr_t addr)
{
        volatile uint16_t value;
        HWSIM_PROGRESS_MARK(0x1216);
        HWSIM_PROGRESS_VAL(addr);
        value = *(volatile uint16_t*)addr;
        HWSIM_PROGRESS_VAL(value);
        return value;
}

void mmio_write_32(uintptr_t addr, uint32_t value)
{
        HWSIM_PROGRESS_MARK(0x1132);
        HWSIM_PROGRESS_VAL(addr);
        HWSIM_PROGRESS_VAL(value);
	*(volatile uint32_t* volatile)addr = (volatile uint32_t) value;
}
uint32_t mmio_read_32(uintptr_t addr)
{
        volatile uint32_t value;
        HWSIM_PROGRESS_MARK(0x1232);
        HWSIM_PROGRESS_VAL(addr);
	value = *(volatile uint32_t*)addr;
        HWSIM_PROGRESS_VAL(value);
        return value;
}

void mmio_write_64(uintptr_t addr, uint64_t value)
{
        HWSIM_PROGRESS_MARK(0x1164);
        HWSIM_PROGRESS_VAL(addr);
        HWSIM_PROGRESS_VAL64(value);
	*(volatile uint64_t*)addr = value;
}

uint64_t mmio_read_64(uintptr_t addr)
{
        volatile uint64_t value;
        HWSIM_PROGRESS_MARK(0x1264);
        HWSIM_PROGRESS_VAL(addr);
	value = *(volatile uint64_t*)addr;
        HWSIM_PROGRESS_VAL64(value);
        return value;
}

void mmio_clrbits_32(uintptr_t addr, uint32_t clear)
{
        HWSIM_PROGRESS_MARK(0x1332);
        HWSIM_PROGRESS_VAL(addr);
        HWSIM_PROGRESS_VAL(clear);
	mmio_write_32(addr, mmio_read_32(addr) & ~clear);
}

void mmio_setbits_32(uintptr_t addr, uint32_t set)
{
        HWSIM_PROGRESS_MARK(0x1432);
        HWSIM_PROGRESS_VAL(addr);
        HWSIM_PROGRESS_VAL(set);
	mmio_write_32(addr, mmio_read_32(addr) | set);
}

void mmio_clrsetbits_32(uintptr_t addr,
                        uint32_t clear,
                        uint32_t set)
{
        HWSIM_PROGRESS_MARK(0x1532);
        HWSIM_PROGRESS_VAL(addr);
        HWSIM_PROGRESS_VAL(clear);
        HWSIM_PROGRESS_VAL(set);
	mmio_write_32(addr, (mmio_read_32(addr) & ~clear) | set);
}
#endif

#if defined(TEST_ON_G3_ASIC_SIM) && !defined(TEST_ON_G3_ASIC_SIM_DISABLE_PROGRESS_MARK)
volatile unsigned long long int data64; 
volatile unsigned int data; 
void HWSIM_PROGRESS_VAL64(unsigned long long int val) {
        volatile unsigned int * volatile p; p = (volatile unsigned int * volatile)&data64; 
        data64 = val; 
        data = *(volatile unsigned int *)GLOBAL_SCRATCH;
        if (data == p[0]) {p[0]++;}; 
        *(volatile unsigned int *)GLOBAL_SCRATCH = p[0];
        data = *(volatile unsigned int *)GLOBAL_SCRATCH;
        if (data == p[1]) {p[1]++;}; 
        *(volatile unsigned int *)GLOBAL_SCRATCH = p[1];
        return;
}
#endif
#endif /* CORTINA_G3_TESTCODE */
