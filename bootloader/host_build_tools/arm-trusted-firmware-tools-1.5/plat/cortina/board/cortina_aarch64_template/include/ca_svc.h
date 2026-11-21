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

#ifndef __CA_SVC_H__
#define __CA_SVC_H__

/* The macros below are used to identify Cortina SMC calls from the SMC function ID */

/* Number of CA77XX SiP Calls implemented */
#define CA_COMMON_SIP_NUM_CALLS	4
#define CA_PLAT_SIP_NUM_CALLS		512


/* SMC function IDs for SiP Service queries */
#define SIP_SVC_CALL_COUNT		0x8200ff00
#define SIP_SVC_UID			0x8200ff01
/*					0x8200ff02 is reserved */
#define SIP_SVC_VERSION			0x8200ff03

/* Cortina-Access SiP Service Calls version numbers */
#define CA_SIP_SVC_VERSION_MAJOR	0x0
#define CA_SIP_SVC_VERSION_MINOR	0x1


/* Valid FID  range for SiP is 0x82000000-0x8200FEFF
  * We use bit8~11 as sub-module ID.
  * All module can use up to 256 function ID(bit0~7).
  */
#define CA_SVC_NE_ID		1
#define CA_SVC_PER_ID		(CA_SVC_NE_ID + 1)
#define CA_SVC_FBM_ID		(CA_SVC_PER_ID + 1)
#define CA_SVC_DMA_ID		(CA_SVC_FBM_ID + 1)
#define CA_SVC_LDMA_ID		(CA_SVC_DMA_ID + 1)
#define CA_SVC_PCIE_ID		(CA_SVC_LDMA_ID + 1)
#define CA_SVC_CCI_ID		(CA_SVC_PCIE_ID + 1)
#define CA_SVC_CAPSRAM_ID	(CA_SVC_CCI_ID + 1)
#define CA_SVC_SADB_ID		(CA_SVC_CAPSRAM_ID + 1)
#define CA_SVC_OTP_ID		(CA_SVC_SADB_ID + 1)
#define CA_SVC_PEATU_ID		(CA_SVC_OTP_ID + 1)
#define CA_SVC_HSIOATU_ID	(CA_SVC_PEATU_ID + 1)
#define CA_SVC_EAXI_ID		(CA_SVC_HSIOATU_ID + 1)
#define CA_SVC_CRYP_ID		(CA_SVC_EAXI_ID + 1)
#define CA_SVC_RTC_ID		(CA_SVC_CRYP_ID + 1)
#define CA_SVC_WDT_ID		(CA_SVC_RTC_ID + 1)

#define CA_SVC_NE			(CA_SVC_NE_ID << 8)
#define CA_SVC_PER			(CA_SVC_PER_ID << 8)
#define CA_SVC_FBM			(CA_SVC_FBM_ID << 8)
#define CA_SVC_DMA			(CA_SVC_DMA_ID << 8)
#define CA_SVC_LDMA			(CA_SVC_LDMA_ID << 8)
#define CA_SVC_PCIE			(CA_SVC_PCIE_ID << 8)
#define CA_SVC_CCI			(CA_SVC_CCI_ID << 8)
#define CA_SVC_CAPSRAM		(CA_SVC_CAPSRAM_ID << 8)
#define CA_SVC_SADB			(CA_SVC_SADB_ID << 8)
#define CA_SVC_OTP			(CA_SVC_OTP_ID << 8)
#define CA_SVC_PEATU		(CA_SVC_PEATU_ID << 8)
#define CA_SVC_HSIOATU		(CA_SVC_HSIOATU_ID << 8)
#define CA_SVC_EAXI			(CA_SVC_EAXI_ID << 8)
#define CA_SVC_CRYP			(CA_SVC_CRYP_ID << 8)
#define CA_SVC_RTC			(CA_SVC_RTC_ID << 8)
#define CA_SVC_WDT			(CA_SVC_WDT_ID << 8)

#define CA_SVC_MODULE_MASK		0x0000FF00

#define CA_SVC_FID_MASK			0x000000FF

/* FID for CA_SVC_WDT */
#define CA_SVC_WDT_ENABLE		0x00
#define CA_SVC_WDT_DISABLE		0x01
#define CA_SVC_WDT_SET_TIME		0x02
#define CA_SVC_WDT_EN_RST		0x03
#define CA_SVC_WDT_RELOAD		0x04
#define CA_SVC_WDT_READ_CNT		0x05
uint64_t ca_smc_handler_wdt(uint32_t smc_fid,
							uint64_t x1,
							uint64_t x2,
							uint64_t x3,
							uint64_t x4, void *cookie, void *handle,
							uint64_t flags);

/*  RTC */

/* CRYPTO */

/* SADB */

/* EAXI */

/* HSIOATU */

/* OTP */

/* NE */

/* FBM */

/* CAPSRAM */

uint64_t ca_smc_handler(uint32_t smc_fid,
						uint64_t x1,
						uint64_t x2,
						uint64_t x3,
						uint64_t x4,
						void *cookie, void *handle, uint64_t flags);

#endif							/* __STD_SVC_H__ */
