/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 *
 */

#include <asm-offsets.h>
#include <config.h>
#include <linux/linkage.h>
#include <asm/macro.h>
ENTRY(armv8_el2_to_aarch32)
         mov     x3, x2
         mov     x2, x1
         mov     x1, x4
         ldr     x0, =0x82001201
         smc     #0
         ret
ENDPROC(armv8_el2_to_aarch32)

