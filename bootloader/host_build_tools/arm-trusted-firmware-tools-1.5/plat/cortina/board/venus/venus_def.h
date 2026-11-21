/*
 * Copyright (c) 2015-2018, Cortina Access. All rights reserved.
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

#ifndef __VENUS_DEF_H__
#define __VENUS_DEF_H__
#include INC_CORTINA_SOC_REGS_HEADER
#include <ca_addr_trans.h>

/* new VENUS DRAM Trusted Access Controller support, turn off for now, until code review */
#define VENUS_USE_DRAM_TAC 1

#define HIGH_SPEED_IO_BASE	0x400000000
#define DEVICE_BASE			0xF0000000
#define DEVICE_SIZE			0x0FFC0000

#define BOOTROM_BASE			0xFFFC0000

#define BOOTROM_SIZE			0x00020000

/* The size of DDR RAM is 256MB */
#define DRAM_BASE			0x00000000
#define DRAM_SIZE			0x10000000
#define DDR_BASE 			DRAM_BASE

#define SRAM_BASE			0x80000000
#define SRAM_SIZE			0x00040000

#define FLASH_BASE			0xE0000000
#define FLASH_SIZE			0x10000000

#define PLAT_VENUS_DRAM_FIP_BASE           0x08000000
#define PLAT_VENUS_DRAM_FIP_MAX_SIZE       0x00800000 /* 8 MB should be enough */

#define PLAT_VENUS_FLASH_FIP_BASE          FLASH_BASE
#define PLAT_VENUS_FLASH_FIP_MAX_SIZE      FLASH_SIZE

#define PLAT_VENUS_TRUSTED_SRAM_ID	1
#define PLAT_VENUS_TRUSTED_DRAM_ID	2

/*
 * DRAM at 0x0000_0000 is divided in two regions:
 *   - Secure DRAM (default is the top 16MB)
 *   - Non-Secure DRAM (remaining DRAM starting at DRAM_BASE)
 */
#define DRAM_SEC_SIZE			0x01000000 /* 16 MB */
#define DRAM_SEC_BASE			(DRAM_BASE + DRAM_SIZE - DRAM_SEC_SIZE)

#define DRAM_NS_BASE			DRAM_BASE
#define DRAM_NS_SIZE			(DRAM_SIZE - DRAM_SEC_SIZE)

/* We load the BL3-3 NS image, like UEFI or U-Boot at 16MB below Secure Memory */
/*******************************************************************************
 * Load address of BL3-3 in the VENUS port - UEFI or U-Boot
 ******************************************************************************/
#define DRAM_BL33_IMAGE_SIZE_MAX	0x01000000 /* 16MB */

//#define NS_IMAGE_OFFSET                 (DRAM_SIZE - DRAM_SEC_SIZE - DRAM_BL33_IMAGE_SIZE_MAX)
#define NS_IMAGE_OFFSET                 0x04000000



/*******************************************************************************
 * GIC-600 & interrupt handling related constants
 ******************************************************************************/
#define GICD_BASE			0x4F8000000
#define GICR_BASE 			0x4F8040000

#define IRQ_SEC_PHY_TIMER		29
#define IRQ_SEC_SGI_0			8
#define IRQ_SEC_SGI_1			9
#define IRQ_SEC_SGI_2			10
#define IRQ_SEC_SGI_3			11
#define IRQ_SEC_SGI_4			12
#define IRQ_SEC_SGI_5			13
#define IRQ_SEC_SGI_6			14
#define IRQ_SEC_SGI_7			15
#define IRQ_SEC_SGI_8			16

/*******************************************************************************
 * Cortina VENUS related clock constants
 ******************************************************************************/
#ifdef CORTINA_VENUS_ASIC
#define APB_PCLK			125000000
#else
#define APB_PCLK			25000000
#endif
#define VENUS_ARMV8_COUNTER_CLK		25000000
#define VENUS_SYS_TIMER_BASE   		0xf4321000
#define VENUS_SYS_TIMER_TICK   		(VENUS_SYS_TIMER_BASE + 8)

/*******************************************************************************
 * Cortina PER GPIO related constants
 ******************************************************************************/
#define CORTINA_PER_GPIO0_CFG           PER_GPIO0_CFG

/*******************************************************************************
 * Cortina PER UART related constants
 ******************************************************************************/
#define CORTINA_PER_UART0_BASE		PER_UART0_CFG
#define CORTINA_PER_UART1_BASE		PER_UART1_CFG
#define CORTINA_PER_UART_BAUDRATE	115200
#define CORTINA_PER_UART_CLK_IN_HZ	APB_PCLK

/*******************************************************************************
 * CCI-550 related constants
 ******************************************************************************/
#define PLAT_VENUS_CCI_BASE		0xF6000000
#define PLAT_VENUS_CCI_CLUSTER0_SL_IFACE_IX 2
#define PLAT_VENUS_CCI_CLUSTER1_SL_IFACE_IX 1

#endif /* __VENUS_DEF_H__ */
