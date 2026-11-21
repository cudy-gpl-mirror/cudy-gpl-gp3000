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

#define SATURN_GLOBAL_STRAP_REG                 GLOBAL_STRAP
#define SATURN_GLOBAL_STRAP_REG_UART_MESSAGE    15
    .globl    plat_cortina_per_uart_message_enabled

    /* ---------------------------------------------
     * int plat_cortina_per_uart_message_enabled(void)
     * Function to determine if UART messages should be
     * enabled by reading global strapping register on SATURN
     * Returns 1 to v0 if uart messages should be ENABLED
         * Returns 0 to v0 if uart messages should be DISABLED
     * Clobber list : 
     * ---------------------------------------------
     */
func plat_cortina_per_uart_message_enabled
       /* t1 = * SATURN_GLOBAL_STRAP_REG  */
       kseg1_atu_xlat $t0,SATURN_GLOBAL_STRAP_REG

       lw      $t1,0($t0)
       nop

       /* t2 = 1 << SATURN_GLOBAL_STRAP_REG_UART_MESSAGE */
       addi $t0,$zero,1
       addi $t3,$zero,SATURN_GLOBAL_STRAP_REG_UART_MESSAGE
       sllv $t2,$t0,$t3
       
       and  $t1,$t1,$t2  /* Check the UART MESSAGE bit is set or not */ 
       beq  $t1,$t2,enable_msg    
       nop
       move $v0,$zero          /* return 0, disable messages */
       jr   $ra
       nop

enable_msg:
       li $v0,1        /* return 1, enable messages */
       jr   $ra
       nop
endfunc plat_cortina_per_uart_message_enabled
