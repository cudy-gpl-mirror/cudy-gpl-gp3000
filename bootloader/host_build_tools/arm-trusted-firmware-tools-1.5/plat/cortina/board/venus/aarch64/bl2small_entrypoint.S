/*
 * Copyright (c) 2015-2019, Cortina Access and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include INC_PLAT_DEF_HEADER


	.globl	bl2small_entrypoint

#define BL2_ENTRY_POSTCODE	0xCA8277B
func bl2small_entrypoint
	/*---------------------------------------------
	 * Store the extents of the tzram available to
	 * BL2SMALL and other platform specific information
	 * for future use. x0 is currently not used.
	 * ---------------------------------------------
	 */
        mov_imm x0, GLOBAL_SOFTWARE
        mov_imm w1, BL2_ENTRY_POSTCODE
        str     w1, [x0]
deadloop:
	b deadloop

endfunc bl2small_entrypoint
