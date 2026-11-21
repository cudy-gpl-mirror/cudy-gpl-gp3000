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

#ifndef __SATURN_FPGA_DEF_H__
#define __SATURN_FPGA_DEF_H__
#include <ca827x_atu.h>
#include <ca_addr_trans.h>
#include INC_CORTINA_SOC_REGS_HEADER

/* MIPS reset vector address */ 
#define BOOTROM_BASE        KSEG0ADDR(0x1FC00000)
#if defined(TEST_ON_SATURN_ASIC_SIM) 
#define BOOTROM_SIZE        0x00020000  /* 128KB */
#else
#define BOOTROM_SIZE        0x00018000  /* 96KB */
#endif

/* The size of DDR RAM is 256MB */
#define DRAM_BASE           KSEG0ADDR(0x00000000)
#define DRAM_SIZE           0x10000000  /* 256MB */
#define DDR_BASE            DRAM_BASE

#define SRAM_BASE           KSEG0ADDR(0x18000000)
#define SRAM_SIZE           0x80000     /* 512Kb */

#define FLASH_BASE          KSEG0ADDR(0x10000000)
#define FLASH_SIZE          0x02000000  /* 32MB */

#define PLAT_SATURN_DRAM_FIP_BASE           KSEG0ADDR(0x08000000)
#define PLAT_SATURN_DRAM_FIP_MAX_SIZE       0x00800000 /* 8 MB should be enough */

#define PLAT_SATURN_FLASH_FIP_BASE          FLASH_BASE
#define PLAT_SATURN_FLASH_FIP_MAX_SIZE      FLASH_SIZE


/*
 * DRAM at 0x0000_0000 - Non-Secure DRAM 
 */
#define DRAM_NS_BASE        DRAM_BASE
#define DRAM_NS_SIZE        DRAM_SIZE

/* We load the BL3-3 NS image, like UEFI or U-Boot at 16MB below Secure Memory */
/*******************************************************************************
 * Load address of BL3-3 in the SATURN port - UEFI or U-Boot
 ******************************************************************************/
#define DRAM_BL33_IMAGE_SIZE_MAX            0x01000000 /* 16MB */

#define NS_IMAGE_OFFSET                     KSEG0ADDR(0x04000000)


/*******************************************************************************
 * Cortina PER GPIO related constants
 ******************************************************************************/
#define CORTINA_PER_GPIO0_CFG               KSEG1_ATU_XLAT(PER_GPIO0_CFG)

/*******************************************************************************
 * Cortina PER UART related constants
 ******************************************************************************/
#define CORTINA_PER_UART0_PRE_ATU_BASE      PER_UART0_CFG
#define CORTINA_PER_UART1_PRE_ATU_BASE      PER_UART1_CFG
#define CORTINA_PER_UART0_BASE              KSEG1_ATU_XLAT(CORTINA_PER_UART0_PRE_ATU_BASE)
#define CORTINA_PER_UART1_BASE              KSEG1_ATU_XLAT(CORTINA_PER_UART1_PRE_ATU_BASE)

#define CORTINA_PER_UART_BAUDRATE           115200

#endif /* __SATURN_FPGA_DEF_H__ */
