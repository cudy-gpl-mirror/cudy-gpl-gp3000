/*
 * Copyright (c) 2016, Cortina Access. All rights reserved.
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
#include <asm_macros.S>
#include <platform_def.h>

#define VENUS_GLOBAL_STRAP_REG GLOBAL_STRAP
#define VENUS_GLOBAL_STRAP_REG_UART_MESSAGE 11
	.globl	plat_cortina_per_uart_message_enabled

	/* ---------------------------------------------
	 * int plat_cortina_per_uart_message_enabled(void)
	 * Function to determine if UART messages should be
	 * enabled by reading global strapping register on VENUS
	 * Returns 1 to x2 if uart messages should be ENABLED
         * Returns 0 to x2 if uart messages should be DISABLED
	 * Clobber list : x2
	 * ---------------------------------------------
	 */
func plat_cortina_per_uart_message_enabled
        mov_imm x2, VENUS_GLOBAL_STRAP_REG
        ldr     w2, [x2, #0]
        tbnz    w2, #VENUS_GLOBAL_STRAP_REG_UART_MESSAGE, 1f
	mov	x2, #0 /* disable messages */
	ret
1:
	nop
	mov	x2, #1 /* enable messages */
	ret
endfunc plat_cortina_per_uart_message_enabled
