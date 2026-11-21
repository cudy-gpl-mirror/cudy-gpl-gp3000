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
#include <debug.h>
#include <runtime_svc.h>
#include <stdint.h>
#include <uuid.h>
#include <ca_svc.h>

/* Cortina-Access SiP Service Calls version numbers */
#define CA_SIP_SVC_VERSION_MAJOR	0x0
#define CA_SIP_SVC_VERSION_MINOR	0x1

DEFINE_SVC_UUID(ca77xx_sip_svc_uid,
		0xca7774ff, 0x7d07, 0x4c3a, 0x87, 0xcc,
		0x2d, 0x1f, 0xe8, 0x17, 0xe1, 0x99);

/*
 * Top-level CA Service SMC handler. This handler will in turn dispatch
 * calls to sub handler according to module ID.
 */
uint64_t ca_smc_handler(uint32_t smc_fid,
						uint64_t x1,
						uint64_t x2,
						uint64_t x3,
						uint64_t x4, void *cookie, void *handle, uint64_t flags)
{
	switch (smc_fid & CA_SVC_MODULE_MASK) {
		case SIP_SVC_CALL_COUNT:
			/* Return the number of Cortina-Access SiP Service Calls. */
			SMC_RET1(handle,
				 CA_COMMON_SIP_NUM_CALLS + CA_PLAT_SIP_NUM_CALLS);

		case SIP_SVC_UID:
			/* Return UID to the caller */
			SMC_UUID_RET(handle, ca77xx_sip_svc_uid);

		case SIP_SVC_VERSION:
			/* Return the version of current implementation */
			SMC_RET2(handle, CA_SIP_SVC_VERSION_MAJOR,
			CA_SIP_SVC_VERSION_MINOR);

		case CA_SVC_WDT:
			return ca_smc_handler_wdt(smc_fid, x1, x2, x3, x4, cookie,
									  handle, flags);

		case CA_SVC_NE:
		case CA_SVC_PER:
		case CA_SVC_FBM:
		case CA_SVC_DMA:
		case CA_SVC_LDMA:
		case CA_SVC_PCIE:
		case CA_SVC_CCI:
		case CA_SVC_CAPSRAM:
		case CA_SVC_SADB:
		case CA_SVC_OTP:
		case CA_SVC_PEATU:
		case CA_SVC_HSIOATU:
		case CA_SVC_EAXI:
		case CA_SVC_CRYP:
		case CA_SVC_RTC:

		default:
			WARN("Ambigious Service Call: 0x%x \n", smc_fid);
			SMC_RET1(handle, SMC_UNK);
	}
}

/* Register Standard Service Calls as runtime service */
DECLARE_RT_SVC(ca_svc,
			   OEN_SIP_START,
			   OEN_SIP_END, SMC_TYPE_FAST, NULL, ca_smc_handler);
