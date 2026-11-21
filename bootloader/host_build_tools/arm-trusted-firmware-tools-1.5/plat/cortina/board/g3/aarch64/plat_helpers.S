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

#include <arch.h>
#include <asm_macros.S>
#include <bl_common.h>
#include <cortex_a53.h>
#include <cpu_macros.S>
#include <platform_def.h>

	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_report_exception
	.globl	plat_reset_handler
	.globl	platform_get_core_pos
	.globl	platform_mem_init

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Function to initialize the crash console
	 * without a C Runtime to print crash report.
	 * Clobber list : x0, x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_init
	mov_imm	x0, CRASH_CONSOLE_BASE
	mov_imm	x1, CORTINA_PER_UART_CLK_IN_HZ
	mov_imm	x2, CORTINA_PER_UART_BAUDRATE
	b	console_core_init
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(int c)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
	mov_imm	x1, CRASH_CONSOLE_BASE
	b	console_core_putc
endfunc plat_crash_console_putc

	/* ---------------------------------------------
	 * void plat_report_exception(unsigned int type)
	 * Function to report an unhandled exception
	 * with platform-specific means.
	 * ---------------------------------------------
	 */
func plat_report_exception
	mov	x8, x30

	adr	x4, plat_err_str
	bl	asm_print_str

	adr	x4, esr_el3_str
	bl	asm_print_str

	mrs	x4, esr_el3
	bl	asm_print_hex

	adr	x4, elr_el3_str
	bl	asm_print_str

	mrs	x4, elr_el3
	bl	asm_print_hex

	adr     x4, g3_specific_err_str
	bl	asm_print_str

	adr     x4, global_strap_str
        bl      asm_print_str

	mov_imm x4, GLOBAL_STRAP
	ldr  w4, [x4]
	bl      asm_print_hex_32

	adr     x4, global_pe_dss_state_str
	bl      asm_print_str

	mov_imm x4, GLOBAL_PE_DSS_STATE
	ldr  w4, [x4]
	bl      asm_print_hex_32

	adr     x4, global_system_interrupt_str
	bl      asm_print_str

	mov_imm x4, GLOBAL_SYSTEM_INTERRUPT
	ldr  w4, [x4]
	bl      asm_print_hex_32

	adr     x4, global_jtag_tap_control_str
	bl      asm_print_str

	mov_imm x4, GLOBAL_JTAG_TAP_CONTROL
	ldr  w4, [x4]
	bl      asm_print_hex_32

	adr     x4, global_software_str
	bl      asm_print_str

	mov_imm x4, GLOBAL_SOFTWARE
	ldr  w4, [x4]
	bl      asm_print_hex_32

	adr     x4, global_software2_str
	bl      asm_print_str

	mov_imm x4, GLOBAL_SOFTWARE2
	ldr  w4, [x4]
	bl      asm_print_hex_32

	adr     x4, global_arm_debug_str
	bl      asm_print_str

	mov_imm x4, GLOBAL_ARM_DEBUG
	ldr  w4, [x4]
	bl      asm_print_hex_32

	adr     x4, flash_status_str
	bl      asm_print_str

	mov_imm x4, FLASH_STATUS
	ldr  w4, [x4]
	bl      asm_print_hex_32

	adr     x4, flash_flash_interrupt_str
	bl      asm_print_str

	mov_imm x4, FLASH_FLASH_INTERRUPT
	ldr  w4, [x4]
	bl      asm_print_hex_32

	adr     x4, otp_status_str
	bl      asm_print_str

	mov_imm x4, OTP_STATUS
	ldr  w4, [x4]
	bl      asm_print_hex_32

	mov	x30, x8
stall:
	bl stall
endfunc plat_report_exception

	/* -----------------------------------------------------
	 * void plat_reset_handler(void);
	 *
	 * Implement workaround for defect id 831273 by enabling
	 * an event stream every 65536 cycles and set the L2 RAM
	 * latencies for Cortex-A57.
	 * -----------------------------------------------------
	 */
func plat_reset_handler
	/* In juno, it sets the latency of L2 Data and Tag. How about g3-fgpa? */
	/* Do anything just after reset. At here, do we need? */
	ret
endfunc plat_reset_handler

	/*
	 * Return 0 to 7
	 */
func platform_get_core_pos
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #6
	ret
endfunc platform_get_core_pos

	/* -----------------------------------------------------
	 * void platform_mem_init(void);
	 *
	 * We don't need to carry out any memory initialization
	 * on g3-fgpa. The Secure RAM is accessible straight away.
         * On G3 ASIC, it is recommended to clean up all SRAM by
	 * setting CAPSRAM_INIT 0xf4322004 and wait 16K cycles
         * The following asm code is translated from the C code compiled with -O0
        	void capsram_init(void) {
	           register int i	;
	           *(volatile unsigned int *)0xf4322004 = 1 ;
	           for (i = 0 		; i < 4 * 1024 ; i++) ;
	}
	* -----------------------------------------------------
	 */
func platform_mem_init

capsram_init:
	.LFB183:
	mov     x0, 8196
	movk    x0, 0xf432, lsl 16
	mov     w1, 1
	str     w1, [x0]
	mov     w19, 0
	b       .L6
.L7:
	add     w19, w19, 1
.L6:
	cmp     w19, 4095
	ble     .L7
	ret
endfunc platform_mem_init

.section .rodata.rev_err_str, "aS"
plat_err_str:
	.asciz "\nPlatform exception reporting:"
esr_el3_str:
	.asciz "\nESR_EL3: "
elr_el3_str:
	.asciz "\nELR_EL3: "
g3_specific_err_str:
	.asciz "\n\nCortina SoC specific registers:"
global_strap_str:
        .asciz "\nGLOBAL_STRAP: "
global_pe_dss_state_str:
        .asciz "\nGLOBAL_PE_DSS_STATE: "
global_system_interrupt_str:
        .asciz "\nGLOBAL_SYSTEM_INTERRUPT: "
global_jtag_tap_control_str:
        .asciz "\nGLOBAL_JTAG_TAP_CONTROL: "
global_software_str:
        .asciz "\nGLOBAL_SOFTWARE: "
global_software2_str:
        .asciz "\nGLOBAL_SOFTWARE2: "
global_arm_debug_str:
        .asciz "\nGLOBAL_ARM_DEBUG: "
flash_status_str:
        .asciz "\nFLASH_STATUS: "
flash_flash_interrupt_str:
        .asciz "\nFLASH_FLASH_INTERRUPT: "
otp_status_str:
        .asciz "\nOTP_STATUS: "
