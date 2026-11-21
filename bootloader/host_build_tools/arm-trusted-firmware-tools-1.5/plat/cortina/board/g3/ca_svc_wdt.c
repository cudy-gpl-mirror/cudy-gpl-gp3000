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
#include <cortina_per_watchdog.h>

/*
 * Top-level CA Service SMC handler. This handler will in turn dispatch
 * calls to sub handler according module ID.
 */
uint64_t ca_smc_handler_wdt(uint32_t smc_fid,
							uint64_t x1,
							uint64_t x2,
							uint64_t x3,
							uint64_t x4, void *cookie, void *handle,
							uint64_t flags)
{
	CORTINA_PER_WATCHDOG_DRV_t *pWatchDogDrv;

	switch (smc_fid & CA_SVC_FID_MASK) {
		case CA_SVC_WDT_DISABLE:
			pWatchDogDrv = cortina_per_watchdog_get_drv_instance(0);
			SMC_RET1(handle, cortina_per_watchdog_disable_sys_reset(pWatchDogDrv));

		case CA_SVC_WDT_ENABLE:
			pWatchDogDrv = cortina_per_watchdog_get_drv_instance(0);
			SMC_RET1(handle, cortina_per_watchdog_enable_sys_reset(pWatchDogDrv));

		case CA_SVC_WDT_SET_TIME:
		case CA_SVC_WDT_EN_RST:
		case CA_SVC_WDT_RELOAD:
			/** TBD */
		default:
			WARN("Unimplemented Service Call: 0x%x \n", smc_fid);
			SMC_RET1(handle, SMC_UNK);
	}
}
