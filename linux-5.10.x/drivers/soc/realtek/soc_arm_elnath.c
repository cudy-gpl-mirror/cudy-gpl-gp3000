// SPDX-License-Identifier: GPL-2.0

#include <linux/version.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/arm-smccc.h>
#include <linux/dma-map-ops.h>
#include <soc/cortina/cortina-smc.h>
#include <soc/cortina/cortina-soc.h>
#include <soc/cortina/rtl9607f_registers.h>
#include "soc_arm.h"

#define SOC_READL(reg)			readl(global_base + (reg - GLOBAL_JTAG_ID))
#define SOC_WRITEL(val, reg)	writel(val, global_base + (reg - GLOBAL_JTAG_ID))
static void __iomem *global_base;

#define MHZ	(1000000)

enum CLK_TYPE {
	CPLL = 0,
	EPLL,
	FPLL,
	CPU,
	CORE,
	LSAXI,
	HSAXI,
	PE,
	PEAXI,
	CLK_TYPE_MAX,
};

static const u32 soc_default_clk[CLK_TYPE_MAX] = {
	1200000000,
	2000000000,
	1600000000,
	1000000000,
	 333000000,
	 200000000,
	 400000000,
	 800000000,
		 0,
};
#ifdef IS_FPGA
static void __ca_clock_probe(struct ca_soc_data *soc)
{
	uint32_t ref_clk = 25 * MHZ;

	soc->cpll = ref_clk;
	soc->epll = ref_clk;
	soc->fpll = ref_clk;
	soc->cpu_clk = ref_clk;
	soc->cci_clk = ref_clk;
	soc->lsaxi_clk = ref_clk;
	soc->hsaxi_clk = ref_clk;
	soc->core_clk = ref_clk;
	soc->crypto_clk = ref_clk;
	soc->eaxi_clk = ref_clk;
	soc->atb_clk = ref_clk;
	soc->pe_clk = ref_clk;
	soc->peaxi_clk = ref_clk;
}
#else
static void __ca_clock_probe(struct ca_soc_data *soc)
{
	uint32_t ref_clk = 25 * MHZ;
	GLOBAL_CPLL0_t cpll0;
	GLOBAL_EPLL0_t epll0;
	GLOBAL_FPLL0_t fpll0;
	GLOBAL_CPLLDIV_t cplldiv;
	GLOBAL_EPLLDIV_t eplldiv;
	GLOBAL_EPLLDIV2_t eplldiv2;
	GLOBAL_PEDIV_t pediv;
	GLOBAL_STRAP_t strap;

	/* GLOBAL_STRAP */
	strap.wrd = SOC_READL(GLOBAL_STRAP);
	if (strap.bf.xtal_sel) {
		ref_clk = 40 * MHZ;
		soc->sys_clk = ref_clk;
	}

	cplldiv.wrd = SOC_READL(GLOBAL_CPLLDIV);
	/* CPLL */
	cpll0.wrd = SOC_READL(GLOBAL_CPLL0);

	if (cplldiv.bf.cpll_mode_override) {
		/* CPLL */
		cpll0.wrd = SOC_READL(GLOBAL_CPLL0);
		if (strap.bf.xtal_sel) {
			soc->cpll = (ref_clk / (2)) * (cpll0.bf.SCPU_DIV_DIVN + 3);
		} else {
			if (cpll0.bf.DIV_PREDIV_BPS)
				soc->cpll = ref_clk * (cpll0.bf.SCPU_DIV_DIVN + 3);
			else
				soc->cpll = ref_clk / (cpll0.bf.DIV_PREDIV_SEL + 2) *
					(cpll0.bf.SCPU_DIV_DIVN + 3);
		}
	} else
		soc->cpll =  soc_default_clk[CPLL];

	/* EPLL */
	epll0.wrd = SOC_READL(GLOBAL_EPLL0);
	if (strap.bf.xtal_sel)
		soc->epll = 2000*MHZ;
	else {
		if (epll0.bf.DIV_PREDIV_BPS)
			soc->epll = ref_clk * (epll0.bf.SCPU_DIV_DIVN + 3);
		else
			soc->epll = ref_clk / (epll0.bf.DIV_PREDIV_SEL + 2) * (epll0.bf.SCPU_DIV_DIVN + 3);
	}
	/* FPLL */
	fpll0.wrd = SOC_READL(GLOBAL_FPLL0);
	if (strap.bf.xtal_sel)
		soc->fpll = 1600*MHZ;
	else {
		if (fpll0.bf.DIV_PREDIV_BPS)
			soc->fpll = ref_clk * (fpll0.bf.SCPU_DIV_DIVN + 3);
		else
			soc->fpll = ref_clk / (fpll0.bf.DIV_PREDIV_SEL + 2) * (fpll0.bf.SCPU_DIV_DIVN + 3);
	}
	/* CPLL related */
	cplldiv.wrd = SOC_READL(GLOBAL_CPLLDIV);

	if (cplldiv.bf.cpll_div_override) {
		/* use CPLL to get CPU clock */
		if (cplldiv.bf.cf_sel) {
			if (cplldiv.bf.f2c_divsel != 0)
				soc->cpu_clk = soc->fpll * 2 / cplldiv.bf.f2c_divsel;
			else
				soc->cpu_clk = soc->fpll;
		} else {
			if (cplldiv.bf.cortex_divsel != 0)
				soc->cpu_clk = soc->cpll * 2 / cplldiv.bf.cortex_divsel;
			else
				soc->cpu_clk = soc->cpll;
		}
	} else {
		/* use STRAP pin to get CPU clock */
		switch (strap.bf.speed) {
		case 0:
			soc->cpu_clk = 500 * MHZ;
			break;
		case 1:
			soc->cpu_clk = 600 * MHZ;
			break;
		case 2:
			soc->cpu_clk = 1000 * MHZ;
			break;
		case 3:
			soc->cpu_clk = 1200 * MHZ;
			break;
		}
	}

	/* EPLL related */
	eplldiv.wrd = SOC_READL(GLOBAL_EPLLDIV);
	eplldiv2.wrd = SOC_READL(GLOBAL_EPLLDIV2);

	/* 9607F no CCI */
	soc->cci_clk = 0;


	if (eplldiv.bf.lsaxi_divsel != 0)
		soc->lsaxi_clk = soc->epll * 2 / eplldiv.bf.lsaxi_divsel;
	else
		soc->lsaxi_clk = soc->epll; /* impossible */

	soc->hsaxi_clk = soc->lsaxi_clk << 1;

	if (eplldiv.bf.core_divsel != 0)
		soc->core_clk = soc->epll * 2 / eplldiv.bf.core_divsel;
	else
		soc->core_clk = soc->epll;  /* impossible */

	/* FPLL related */
	pediv.wrd =  SOC_READL(GLOBAL_PEDIV);
	if (pediv.bf.offload_sel) {
		if (pediv.bf.c2f_divsel != 0)
			soc->pe_clk = soc->cpll * 2 / pediv.bf.c2f_divsel;
		else
			soc->pe_clk = soc->cpll;
	} else {
		if (pediv.bf.f_divsel != 0)
			soc->pe_clk = soc->fpll * 2 / pediv.bf.f_divsel;
		else
			soc->pe_clk = soc->fpll;
	}

	if (pediv.bf.peaxi_fullspeedsel)
		soc->peaxi_clk = soc->pe_clk;
	else
		soc->peaxi_clk = soc->pe_clk / 2;
}
#endif

struct m_id {
	int id;
	const char *name;
};

static const struct m_id m_id_table[] = {
	{ 0x040, "RTL9607FQ" },
	{ 0x110, "RTL9607FK" },
	{ 0x210, "RTL9607F" },
	{ 0x220, "RTL9607FS" },
	{ 0x310, "RTL9607FD" },
};

static const char *id_to_name(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(m_id_table); i++) {
		if (id == (m_id_table[i].id))
			return m_id_table[i].name;
	}

	return "<Unknown>";
}

static void rtk_global_config_init(void)
{
	GLOBAL_GLOBAL_CONFIG_t global_config;

	/* GLOBAL_GLOBAL_CONFIG */
	global_config.wrd = SOC_READL(GLOBAL_GLOBAL_CONFIG);
	global_config.bf.cfg_pcie_0_clken = 0;
	global_config.bf.cfg_pcie_1_clken = 0;
	global_config.bf.cfg_pcie_2_clken = 0;
	global_config.bf.ps_pcie0_en = 0;
	global_config.bf.ps_pcie2_en = 0;
	global_config.bf.ps_usb3_en = 0;
	SOC_WRITEL(global_config.wrd, GLOBAL_GLOBAL_CONFIG);
}

static void rtk_global_phy_power_init(void)
{
	GLOBAL_PHY_ISO_POWER_CTRL_t global_phy_power;

	global_phy_power.wrd = 0;
	global_phy_power.bf.s0_PCIE2_ISOLATE_0 = 1;
	global_phy_power.bf.s0_PCIE2_ISOLATE_1 = 1;
	global_phy_power.bf.s1_PCIE2_ISOLATE_0 = 1;
	global_phy_power.bf.s1_PCIE2_ISOLATE_1 = 1;
	global_phy_power.bf.s2_CPHY_ISOLATE = 1;
	global_phy_power.bf.s3_CPHY_ISOLATE = 1;
	SOC_WRITEL(global_phy_power.wrd, GLOBAL_PHY_ISO_POWER_CTRL);
}

#ifdef CONFIG_RTK_DP_DIS_SOC
static void dis_rtk_global_config(void)
{
	GLOBAL_GLOBAL_CONFIG_t global_config;

	global_config.wrd = 0x740001D0 ; //disable most IP clocks, except core_pd, l3fe_pd, ps_pe_en. Keep networing work.
	SOC_WRITEL(global_config.wrd, GLOBAL_GLOBAL_CONFIG);
}

static void dis_rtk_global_phy_power(void)
{
	GLOBAL_PHY_ISO_POWER_CTRL_t global_phy_power;

	global_phy_power.wrd = 0;
	SOC_WRITEL(global_phy_power.wrd, GLOBAL_PHY_ISO_POWER_CTRL);
}

static int dyg_soc_fn(struct notifier_block *nb, unsigned long event, void *ptr)
{
	dis_rtk_global_config();
	dis_rtk_global_phy_power();

	return NOTIFY_DONE;
}

static struct notifier_block dyg_soc_block = {
	dyg_soc_fn,
	NULL,
};

extern int register_dyinggasp_notifier(struct notifier_block *nb);

static int __init dyg_soc_init(void)
{
	register_dyinggasp_notifier(&dyg_soc_block);

	return 0;
}
#endif //CONFIG_RTK_DP_DIS_SOC

static void register_soc(struct ca_soc_data *soc)
{
	struct soc_device_attribute *attr;
	uint16_t id;

	attr = kzalloc(sizeof(*attr), GFP_KERNEL);
	if (!attr)
		return;

	id = soc->chipmode & 0xfff0;
	attr->family = "Elnath";
	attr->soc_id =  kasprintf(GFP_KERNEL, "0x%x", id);
	attr->machine = id_to_name(id);

	soc_device_register(attr);
}

/* return 1 if probed OK */
__init int probe_soc_elnath(struct ca_soc_data *soc)
{
	u32 v;
#ifndef IS_FPGA
	phys_addr_t phyaddr;
#endif
	int ret = 0;

	global_base = ioremap(GLOBAL_JTAG_ID, 0x1000);
	if (!global_base)
		return 0;

	v = SOC_READL(GLOBAL_JTAG_ID);
	if (RTK_SOC_CHIP_ID_GET(v) != CHIP_RTL9607F)
		goto NOT_FOUND;

	soc->chip_id = RTK_SOC_CHIP_ID_GET(v);
	soc->vendor_id = RTK_SOC_VENDOR_ID_GET(v);
	soc->reg_global_software = GLOBAL_SOFTWARE;
#ifndef IS_FPGA
	phyaddr = virt_to_phys(&soc->chipmode);
	arch_sync_dma_for_cpu(phyaddr, sizeof(soc->chipmode), DMA_BIDIRECTIONAL);
	ca_smc_call(CA_SVC_OTP_CHIP_MODE_READ, 0, 0, phyaddr, 0);
#endif
	__ca_clock_probe(soc);
	rtk_global_config_init();
	rtk_global_phy_power_init();
#ifdef CONFIG_RTK_DP_DIS_SOC
	dyg_soc_init();
#endif

	register_soc(soc);

	ret = 1;
NOT_FOUND:
	iounmap(global_base);
	return ret;
}

