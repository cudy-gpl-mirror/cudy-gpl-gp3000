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

#include <asm.h>
#include <mipsregs.h>
#include <bl_common.h>
#include <platform_def.h>
#include <ca827x_atu.h>
#include INC_PLAT_DEF_HEADER
#include INC_CORTINA_SOC_REGS_HEADER
#include <asm_macros.S>

.globl  plat_get_otp_clock_mhz
.globl  plat_get_fabric_clock_mhz
.globl  plat_get_cpu_clock_mhz
.globl  plat_get_io_clock_mhz
.globl  plat_get_per_clock_mhz
.globl  plat_crash_console_init
.globl  plat_crash_console_putc
.globl  plat_report_exception
.globl  plat_reset_handler
.globl  platform_get_core_pos
.globl  platform_mem_init
.globl  plat_cache_reset
.globl  zeromem16
.globl  memcpy16

.set    noreorder

#define GLOBAL_STRAP_SPEEP_BIT_START             1  
#define GLOBAL_STRAP_SPEEP_BIT_MASK              6   /* 0b0110 */

.data
#if CORTINA_SATURN_ASIC
    _otp_clock_mhz:.hword  62,62,49,49 
    /*  fabric_clock TABLE is synonamous with the “proc_clk” according to the HW documentation */
    _fabric_clock_mhz:.hword  250,250,200,200 
    _cpu_clock_mhz:.hword  666,500,200,200 
    _io_clock_mhz :.hword  100,100,50,0
    _per_clock_mhz:.hword  125,125,125,125 
#else /* FPGA */
    /*  fabric_clock TABLE is synonamous with the “proc_clk” according to the HW documentation */
    _otp_clock_mhz:.hword  62,62,49,49 
    _fabric_clock_mhz:.hword  25,25,25,25 
    _cpu_clock_mhz:.hword  25,25,25,25 
    _io_clock_mhz :.hword  25,25,25,25
    _per_clock_mhz:.hword  25,25,25,25 
#endif 



/* -----------------------------------------------------------------------
 * void zeromem16(void *mem, unsigned int length);
 *
 * Initialise a memory region to 0.
 * The memory address must be 16-byte aligned.
 * -----------------------------------------------------------------------
 */
func zeromem16
    /* t0 = start +  len */ 
    add  $t0, $a0,$a1
    /* if mem > start + len, done */
    bgt  $a0, $t0, 2f
    nop
1:
    sw	 $zero, 0($a0)
    addi $a0, $a0, PTRSIZE
    blt  $a0, $t0, 1b
    nop
2:
    jr   $ra
    nop
endfunc zeromem16


/* --------------------------------------------------------------------------
 * void memcpy16(void *dest, const void *src, unsigned int length)
 *
 * Copy length bytes from memory area src to memory area dest.
 * The memory areas should not overlap.
 * Destination and source addresses must be 16-byte aligned.
 * --------------------------------------------------------------------------
 */
func memcpy16
    add  $t0, $a0, $a2
    
    /* length < 1,done */
    li   $t1, 1
    blt  $a2, $t1, 2f
    nop
1:
    lw   $t1, 0($a1)
    nop 
    sw   $t1, 0($a0)
    addi $a0, PTRSIZE
    addi $a1, PTRSIZE
    blt  $a0, $t0,1b
    nop
2:
    jr  $ra
    nop
endfunc memcpy16

func plat_get_otp_clock_mhz
    kseg1_atu_xlat $t0, GLOBAL_STRAP

    lw      $t1, 0($t0)                                   /* t1 = * GLOBAL_STRAP */
    nop

    /* t1 = speed */
    andi    $t1, $t1, GLOBAL_STRAP_SPEEP_BIT_MASK
    srl     $t1, $t1, GLOBAL_STRAP_SPEEP_BIT_START

    /* t1 = speed * 2, make t1 equal offset */
    sll     $t1, $t1, 1

    /* v0 = _cpu_clock_mhz[speed] */
    la      $t0, _otp_clock_mhz
    add     $t0, $t0,$t1
    lhu     $v0, 0($t0)
    nop

    /* return $v0 */
    jr      $ra
    nop
endfunc plat_get_otp_clock_mhz

func plat_get_fabric_clock_mhz
    kseg1_atu_xlat $t0, GLOBAL_STRAP

    lw      $t1, 0($t0)                                   /* t1 = * GLOBAL_STRAP */
    nop
    
    /* t1 = speed */ 
    andi    $t1, $t1, GLOBAL_STRAP_SPEEP_BIT_MASK  
    srl     $t1, $t1, GLOBAL_STRAP_SPEEP_BIT_START
    
    /* t1 = speed * 2, make t1 equal offset */ 
    sll     $t1, $t1, 1
    
    /* v0 = _cpu_clock_mhz[speed] */
    la      $t0, _fabric_clock_mhz
    add     $t0, $t0,$t1
    lhu     $v0, 0($t0)
    nop

    /* return $v0 */
    jr      $ra 
    nop 
endfunc plat_get_fabric_clock_mhz

func plat_get_cpu_clock_mhz
    kseg1_atu_xlat $t0, GLOBAL_STRAP

    lw      $t1, 0($t0)                                   /* t1 = * GLOBAL_STRAP */
    nop
    
    /* t1 = speed */ 
    andi    $t1, $t1, GLOBAL_STRAP_SPEEP_BIT_MASK  
    srl     $t1, $t1, GLOBAL_STRAP_SPEEP_BIT_START
    
    /* t1 = speed * 2, make t1 equal offset */ 
    sll     $t1, $t1, 1
    
    /* v0 = _cpu_clock_mhz[speed] */
    la      $t0, _cpu_clock_mhz
    add     $t0, $t0,$t1
    lhu     $v0, 0($t0)
    nop

    /* return $v0 */
    jr      $ra 
    nop 
endfunc plat_get_cpu_clock_mhz


func plat_get_io_clock_mhz
    kseg1_atu_xlat $t0, GLOBAL_STRAP
    lw      $t1, 0($t0)                                   /* t1 = * GLOBAL_STRAP */
    nop
    
    /* t1 = speed */ 
    andi    $t1, $t1, GLOBAL_STRAP_SPEEP_BIT_MASK  
    srl     $t1, $t1, GLOBAL_STRAP_SPEEP_BIT_START
    
    /* t1 = speed * 2, make t1 equal offset */ 
    sll     $t1, $t1, 1
    
    /* v0 = _io_clock_mhz[speed] */
    la      $t0, _io_clock_mhz
    add     $t0, $t0,$t1
    lhu     $v0, 0($t0)
    nop
    
    /* return $v0 */
    jr      $ra 
    nop    
endfunc plat_get_io_clock_mhz

func plat_get_per_clock_mhz
    kseg1_atu_xlat $t0, GLOBAL_STRAP
    lw      $t1, 0($t0)                                   /* t1 = * GLOBAL_STRAP */
    nop

    /* t1 = speed */
    andi    $t1, $t1, GLOBAL_STRAP_SPEEP_BIT_MASK
    srl     $t1, $t1, GLOBAL_STRAP_SPEEP_BIT_START

    /* t1 = speed * 2, make t1 equal offset */
    sll     $t1, $t1, 1

    /* v0 = _per_clock_mhz[speed] */
    la      $t0, _per_clock_mhz
    add     $t0, $t0,$t1
    lhu     $v0, 0($t0)
    nop

    /* return $v0 */
    jr      $ra
    nop
endfunc plat_get_per_clock_mhz


/* ---------------------------------------------
 * int plat_crash_console_init(void)
 * Function to initialize the crash console
 * without a C Runtime to print crash report.
 * Clobber list : a0, a1, a2
 * ---------------------------------------------
 */
func plat_crash_console_init
    /* save the return address */ 
    move    $s3, $ra

    /* $a0 = CRASH_CONSOLE_BASE */
    kseg1_atu_xlat $a0, CRASH_CONSOLE_BASE


    /* $t1 = plat_get_io_clock_mhz 
     * $t2 = 1000000
     * $a1 = $t1 * $t2
     */
    jal     plat_get_per_clock_mhz
    nop
    addi    $t1, $v0, 0 
    lui     $t2, (0xffff & (1000000>>16)) 
    ori     $t2, $t2, (1000000& 0xffff)  
    multu   $t1, $t2
    mflo    $a1

    /* $a2 = CORTINA_PER_UART_BAUDRATE */
    lui     $a2, (0xffff & (CORTINA_PER_UART_BAUDRATE>>16)) 
    ori     $a2, $a2, (CORTINA_PER_UART_BAUDRATE& 0xffff)  
    
    /* call function console_core_init */
    jal     console_core_init
    nop

    jr      $s3
    nop
endfunc plat_crash_console_init


/* ---------------------------------------------
 * int plat_crash_console_putc(int c)
 * Function to print a character on the crash
 * console without a C Runtime.
 * Clobber list : a0, a1
 * ---------------------------------------------
 */
func plat_crash_console_putc
    /* $a1 = CRASH_CONSOLE_BASE */
    kseg1_atu_xlat $a1, CRASH_CONSOLE_BASE

    j       console_core_putc
    nop

endfunc plat_crash_console_putc

.macro  plat_print_reg _txt,_reg
    la    $s2, \_txt
    jal   asm_print_str
    nop 

    kseg1_atu_xlat $t0,\_reg

    lw    $s2, 0($t0)                             
    nop
    jal   asm_print_hex_32
    nop
.endm

/* ---------------------------------------------
* void plat_report_exception(unsigned int type)
* Function to report an unhandled exception
* with platform-specific means.
*
* arguments passed in:
* k0 : epc , k1 : cause, a0: which exception vector we came from
* ---------------------------------------------
*/
func plat_report_exception
    /* store away the arguement which tells us where we came from */
    move  $s6, $a0

    la    $s2, plat_err_str 
    jal   asm_print_str
    nop 

    la    $s2, exception_epc_str
    jal   asm_print_str
    nop

    move  $s2, $k0                /*print EPC */
    jal   asm_print_hex_32
    nop
	
    la    $s2, exception_cause_str
    jal   asm_print_str
    nop
	
    move  $s2, $k1                /*print CAUSE */
    jal   asm_print_hex_32
    nop

print_saturn_soc_regs:
    la    $s2, saturn_specific_err_str
    jal   asm_print_str
    nop 

    plat_print_reg _txt=global_strap_str _reg=GLOBAL_STRAP

    plat_print_reg _txt=global_system_interrupt_str _reg=GLOBAL_SYSTEM_INTERRUPT_0

    plat_print_reg _txt=global_jtag_tap_control_str _reg=GLOBAL_JTAG_TAP_CONTROL

    plat_print_reg _txt=global_software_str _reg=GLOBAL_SOFTWARE

    plat_print_reg _txt=global_software2_str _reg=GLOBAL_SOFTWARE2

    plat_print_reg _txt=flash_status_str _reg=FLASH_STATUS

    plat_print_reg _txt=flash_flash_interrupt_str _reg=FLASH_FLASH_INTERRUPT

    /* go to stall to halt the cpu, when we return from exception context */
    la 	$t0, stall
    rfe    
    jr $t0
    nop
stall:
    j stall
    nop
endfunc plat_report_exception

/* -----------------------------------------------------
 * void plat_reset_handler(void);
 *
 * Reserved for Saturn workaround in reset sequence
 * -----------------------------------------------------
 */
func plat_reset_handler
    /* Reserved for unique Saturn reset vector sequence */
    jr	$ra
    nop
endfunc plat_reset_handler

/*
* Return CPU Number
*/
func platform_get_core_pos
	mfc0	$v0, CP0_PRID
#ifdef CONFIG_CPU_RLX 
#if ((CONFIG_CPU_RLX == RX5281) || (CONFIG_CPU_RLX == RX4281) || (CONFIG_CPU_RLX == RX5271) || (CONFIG_CPU_RLX == RX4271))
    /* CPU Number 16-25 bit */
	srl     $v0, 16  
	andi	$v0, 0x3ff  
#elif ((CONFIG_CPU_RLX == RX5381) || (CONFIG_CPU_RLX == RX4381))
    /* CPU Number 19-25 bit */
	srl     $v0, 19  
	andi	$v0, 0x7f  
#else /* (CONFIG_CPU_RLX == Unknown RLX CPU type */
#error platform_get_core_pos_unknown_for_this_RLX_cpu_type
#endif  /* CONFIG_CPU_RLX == type */
#else /* ! CONFIG_CPU_RLX */
#error platform_get_core_pos_unknown_for_this_MIPS_cpu_type
#endif /* CONFIG_CPU_RLX */
    jr	    $ra
    nop
endfunc  platform_get_core_pos

/* -----------------------------------------------------
 * void platform_mem_init(void);
 *
 * We don't need to carry out any memory initialization
 * on saturn-fgpa. The Secure RAM is accessible straight away.
     * On Saturn ASIC, it is recommended to clean up all SRAM by
 * setting CAPSRAM_INIT and wait 16K cycles
     * The following asm code is translated from the C code compiled with -O0
        void capsram_init(void) {
           register int i    ;
           *(volatile unsigned int *)CAPSRAM_INIT = 1 ;
           for (i = 0         ; i < 32 * 1024 ; i++) ;
     }
* -----------------------------------------------------
*/
func platform_mem_init
    
    /* t1 = CAPSRAM_INIT */
    kseg1_atu_xlat $t1, CAPSRAM_INIT
    
    /* *t1 = 1 */
    li      $t2, 1
    sw      $t2, 0($t1)
    
    /* t1 = 32 * 1024  */
    li      $t1, 32768
forloop:
    addiu	$t1, $t1, -1
    bnez	$t1, forloop
    nop

    jr		$ra
    nop
endfunc platform_mem_init

func plat_cache_reset
    jr		$ra
    nop
endfunc plat_cache_reset

.section .rodata.rev_err_str, "aS"
    plat_err_str:
        .asciz "\nPlatform exception reporting: "
    saturn_specific_err_str:
        .asciz "\n\nCortina SoC specific registers: "
    global_strap_str:
        .asciz "\nGLOBAL_STRAP: "
    global_system_interrupt_str:
        .asciz "\nGLOBAL_SYSTEM_INTERRUPT: "
    global_jtag_tap_control_str:
        .asciz "\nGLOBAL_JTAG_TAP_CONTROL: "
    global_software_str:
        .asciz "\nGLOBAL_SOFTWARE: "
    global_software2_str:
        .asciz "\nGLOBAL_SOFTWARE2: "
    flash_status_str:
        .asciz "\nFLASH_STATUS: "
    flash_flash_interrupt_str:
        .asciz "\nFLASH_FLASH_INTERRUPT: "
    otp_status_str:
        .asciz "\nOTP_STATUS: "
    exception_epc_str:
        .asciz "\nEPC: "
    exception_cause_str:
        .asciz "\nCAUSE: "

