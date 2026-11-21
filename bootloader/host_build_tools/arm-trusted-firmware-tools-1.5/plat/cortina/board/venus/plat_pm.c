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

#include <assert.h>
#include <arch_helpers.h>
#include <arm_gic.h>
#include <debug.h>
#include <cci400.h>
#include <errno.h>
#include <plat_gicv3.h>
#include <mmio.h>
#include <platform.h>
#include <platform_def.h>
#include <psci.h>
#include INC_CORTINA_SOC_REGS_HEADER
#include INC_PLAT_DEF_HEADER
#include INC_PLAT_PRIVATE_HEADER
#ifdef CORTINA_PER_WATCHDOG
#include <cortina_per_watchdog.h>
#endif /* CORTINA_PER_WATCHDOG */

extern void udelay(int us);

#define VENUS_SOC_SUSPEND_STATE	0x4

static int32_t venus_do_plat_actions(uint32_t afflvl, uint32_t state)
{
	assert(afflvl <= MPIDR_AFFLVL1);

	if (state != PSCI_STATE_OFF)
		return -EAGAIN;

	return 0;
}

int32_t venus_affinst_on(uint64_t mpidr,
			 uint64_t sec_entrypoint,
			 uint32_t afflvl,
			 uint32_t state)
{
	int cpu;
	GLOBAL_ARM_CONFIG_B_t arm_pwr;
	GLOBAL_ARM_RESET_t	arm_reset;

	cpu = mpidr & MPIDR_CPU_MASK;

	VERBOSE("#%s, mpidr:%lx, afflvl:%x, state:%x\n", __func__, mpidr, afflvl, state);

	/* directly return for power on */
	if (state == PSCI_STATE_ON)
		return PSCI_E_SUCCESS;

	switch (afflvl) {
	case MPIDR_AFFLVL0:
		/* Release CPU */
		arm_reset.wrd = mmio_read_32(GLOBAL_ARM_RESET);
		arm_reset.bf.CPUPORESET &= ~(1 << cpu);
		arm_reset.bf.CORERESET &= ~(1 << cpu);
		udelay(1);
		mmio_write_32(GLOBAL_ARM_RESET, arm_reset.wrd);

		__asm__ volatile ("isb":::"memory");

		udelay(1);
		/* Restore default Reset Vector or system will fail to boot if system reset by  watchdog. */
		mmio_write_32(GLOBAL_ARM_CONFIG_D0, 0xFFFF0000 >> 2);
		break;

	case MPIDR_AFFLVL1:
		/* Reset Vector Base Addr(RVBAR) must be 4 byte alignment.
		 * HW shift 2 bits left as RVBAR.
		 * So we have to shift 2 bit right here.
		 */
		mmio_write_32(GLOBAL_ARM_CONFIG_D0, sec_entrypoint >> 2);

		/* Assert reset signal */
		arm_reset.wrd = mmio_read_32(GLOBAL_ARM_RESET);
		arm_reset.bf.CPUPORESET |= 1 << cpu;
		arm_reset.bf.CORERESET |= 1 << cpu;
		mmio_write_32(GLOBAL_ARM_RESET, arm_reset.wrd);

		__asm__ volatile ("isb":::"memory");

		/* Power on CPU:
		 * Enable power domain sequentially to avoid rush current.
		 */
		arm_pwr.wrd = mmio_read_32(GLOBAL_ARM_CONFIG_B);
		arm_pwr.bf.ca55_core_pwrctrl_in &= ~(1 << cpu);
		mmio_write_32(GLOBAL_ARM_CONFIG_B, arm_pwr.wrd);
		arm_pwr.bf.ca55_core_pwrctrl_2nd_in &= ~(1 << cpu);
		mmio_write_32(GLOBAL_ARM_CONFIG_B, arm_pwr.wrd);

		__asm__ volatile ("isb":::"memory");
		/* TODO */
		break;
	}

	return PSCI_E_SUCCESS;
}


static void venus_affinst_off(uint32_t afflvl, uint32_t state)
{
#if 0
	unsigned int mpidr = read_mpidr_el1();
	int cpu, cluster;

	cluster = (mpidr & MPIDR_CLUSTER_MASK) >> MPIDR_AFF1_SHIFT;
	cpu = mpidr & MPIDR_CPU_MASK;
#endif

	if (venus_do_plat_actions(afflvl, state) == -EAGAIN)
		return;

	switch (afflvl) {
	case MPIDR_AFFLVL1:
#if 0
		cci_disable_cluster_coherency(mpidr);
#endif
		/* TODO */
		break;

	case MPIDR_AFFLVL0:
		plat_arm_gic_cpuif_disable();
		/* TODO */
		break;
	}

	return;
}

static void venus_affinst_suspend(uint64_t sec_entrypoint,
				  uint32_t afflvl,
				  uint32_t state)
{
#if 0
	unsigned int mpidr = read_mpidr_el1();
	int cpu, cluster;

	cluster = (mpidr & MPIDR_CLUSTER_MASK) >> MPIDR_AFF1_SHIFT;
	cpu = mpidr & MPIDR_CPU_MASK;
#endif

	if (venus_do_plat_actions(afflvl, state) == -EAGAIN)
		return;

	switch (afflvl) {
	case MPIDR_AFFLVL1:
#if 0
		cci_disable_cluster_coherency(mpidr);
#endif

		/* TODO */
		break;

	case MPIDR_AFFLVL0:

		/* Program the jump address for the target cpu */
		/* TODO */

		plat_arm_gic_cpuif_disable();

		break;
	}

	return;
}

void venus_affinst_on_finish(uint32_t afflvl, uint32_t state)
{
#if 0
	unsigned long mpidr;
	int cpu, cluster;
#endif

	if (venus_do_plat_actions(afflvl, state) == -EAGAIN)
		return;

#if 0
	/* Get the mpidr for this cpu */
	mpidr = read_mpidr_el1();
	cluster = (mpidr & MPIDR_CLUSTER_MASK) >> MPIDR_AFF1_SHIFT;
	cpu = mpidr & MPIDR_CPU_MASK;
#endif

#if 0
	/* Perform the common cluster specific operations */
	if (afflvl != MPIDR_AFFLVL0)
		cci_enable_cluster_coherency(mpidr);
#endif 

	/* Enable the gic cpu interface */
	plat_arm_gic_pcpu_init();
	/* Program the gic per-cpu distributor or re-distributor interface */
	plat_arm_gic_cpuif_enable();

	return;
}

static void venus_affinst_suspend_finish(uint32_t afflvl,
					 uint32_t state)
{
	venus_affinst_on_finish(afflvl, state);
	return;
}

static void __dead2 venus_system_off(void)
{
#if 0
	mdelay(1000);
#endif
	/* Send the system reset request since it fails to power off */
	/* TODO */
	wfi();
	panic();
}

static void __dead2 venus_system_reset(void)
{
#ifdef CORTINA_PER_WATCHDOG
	CORTINA_PER_WATCHDOG_DRV_t * pWatchDogDrv;
	GLOBAL_GLOBAL_CONFIG_t venus_global_cfg;
	pWatchDogDrv = cortina_per_watchdog_re_init((uintptr_t) (PER_WDT_CTRL),
					(APB_PCLK/1000),
					1,
					CORTINA_PER_WATCHDOG_UNIT_MSEC
					);
	if(pWatchDogDrv == NULL)
		printf("Watchdog driver init failed!!\n");
	else
	{
		/* Reset RVBAR */
		mmio_write_32(GLOBAL_ARM_CONFIG_D0, 0xFFFF0000 >> 2);

		venus_global_cfg.wrd = mmio_read_32(GLOBAL_GLOBAL_CONFIG);
		/* Enable WDT reset in GLOBAL_CONFIG */
		venus_global_cfg.bf.wd_reset_ext_reset = 1;
		venus_global_cfg.bf.wd_reset_remap = 1;
		venus_global_cfg.bf.wd_reset_all_blocks= 1;
		venus_global_cfg.bf.wd_reset_subsys_enable = 1;
		mmio_write_32(GLOBAL_GLOBAL_CONFIG, venus_global_cfg.wrd);

		/* enable system reset if watchdog expires */
		cortina_per_watchdog_enable_sys_reset(pWatchDogDrv);
		__asm__ volatile ("isb sy":::"memory");
		__asm__ volatile ("wfi":::"memory");
	}
#endif /* CORTINA_PER_WATCHDOG */

	wfi();
	panic();
}

unsigned int venus_get_sys_suspend_power_state(void)
{
	unsigned int power_state;

	power_state = psci_make_powerstate(VENUS_SOC_SUSPEND_STATE,
			PSTATE_TYPE_POWERDOWN, MPIDR_AFFLVL1);

	return power_state;
}

static const plat_pm_ops_t venus_plat_pm_ops = {
	.affinst_on		     = venus_affinst_on,
	.affinst_on_finish	     = venus_affinst_on_finish,
	.affinst_off		     = venus_affinst_off,
	.affinst_standby	     = NULL,
	.affinst_suspend	     = venus_affinst_suspend,
	.affinst_suspend_finish	     = venus_affinst_suspend_finish,
	.system_off		     = venus_system_off,
	.system_reset		     = venus_system_reset,
	.get_sys_suspend_power_state = venus_get_sys_suspend_power_state,
};

int platform_setup_pm(const plat_pm_ops_t **plat_ops)
{
	*plat_ops = &venus_plat_pm_ops;
	return 0;
}
