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
#include <soc/cortina/rtl8277c_registers.h>
#include "soc_arm.h"

#define SOC_READL(reg)			readl(global_base + (reg - GLOBAL_JTAG_ID))
#define SOC_WRITEL(val, reg)	writel(val, global_base + (reg - GLOBAL_JTAG_ID))
static void __iomem *global_base;

#define MHZ	(1000000)

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

	/* CPLL */
	cpll0.wrd = SOC_READL(GLOBAL_CPLL0);
	if (cpll0.bf.DIV_PREDIV_BPS)
		soc->cpll = ref_clk * (cpll0.bf.SCPU_DIV_DIVN + 3);
	else
		soc->cpll = ref_clk / (cpll0.bf.DIV_PREDIV_SEL + 2) * (cpll0.bf.SCPU_DIV_DIVN + 3);

	/* EPLL */
	epll0.wrd = SOC_READL(GLOBAL_EPLL0);
	if (epll0.bf.DIV_PREDIV_BPS)
		soc->epll = ref_clk * (epll0.bf.SCPU_DIV_DIVN + 3);
	else
		soc->epll = ref_clk / (epll0.bf.DIV_PREDIV_SEL + 2) * (epll0.bf.SCPU_DIV_DIVN + 3);

	/* FPLL */
	fpll0.wrd = SOC_READL(GLOBAL_FPLL0);

	if (fpll0.bf.DIV_PREDIV_BPS)
		soc->fpll = ref_clk * (fpll0.bf.SCPU_DIV_DIVN + 3);
	else
		soc->fpll = ref_clk / (fpll0.bf.DIV_PREDIV_SEL + 2) * (fpll0.bf.SCPU_DIV_DIVN + 3);

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
			soc->cpu_clk = 400 * MHZ;
			break;
		case 1:
			soc->cpu_clk = 625 * MHZ;
			break;
		case 2:
			soc->cpu_clk = 700 * MHZ;
			break;
		case 3:
			soc->cpu_clk = 800 * MHZ;
			break;
		case 4:
			soc->cpu_clk = 900 * MHZ;
			break;
		case 5:
			soc->cpu_clk = 1000 * MHZ;
			break;
		case 6:
			soc->cpu_clk = 1100 * MHZ;
			break;
		case 7:
			soc->cpu_clk = 1200 * MHZ;
			break;
		}
	}

	/* EPLL related */
	eplldiv.wrd = SOC_READL(GLOBAL_EPLLDIV);
	eplldiv2.wrd = SOC_READL(GLOBAL_EPLLDIV2);

	soc->cci_clk = soc->epll/((eplldiv2.wrd >> 8)/2);


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
	pediv.wrd = SOC_READL(GLOBAL_PEDIV);
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

struct m_id {
	int id;
	const char *name;
};

static const struct m_id m_id_table[] = {
	{ 0x000, "CA8277C" },
	{ 0x030, "RTL9615C" },
	{ 0x020, "RTL9617C" },
	{ 0x010, "RTL9617C-VW" },
	{ 0x040, "RTL9607DQ" },
	{ 0x080, "RTL8198XB" },
	{ 0x100, "RTL9619" },
	{ 0x110, "RTL9619C" },
	{ 0x210, "RTL9607DM" },
	{ 0x220, "RTL9607DS" },
	{ 0x240, "RTL9617CM" },
	{ 0x280, "RTL9607DM" }
};

static const char *id_to_name(int id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(m_id_table); i++) {
		if (id == m_id_table[i].id)
			return m_id_table[i].name;
	}
	return "<Unknown>";
}

static void register_soc(struct ca_soc_data *soc)
{
	struct soc_device_attribute *attr;
	uint16_t id;

	attr = kzalloc(sizeof(*attr), GFP_KERNEL);
	if (!attr)
		return;

	id = soc->chipmode & 0xfff0;
	attr->family = "Taurus";
	attr->soc_id =  kasprintf(GFP_KERNEL, "0x%x", id);
	attr->machine = id_to_name(id);

	soc_device_register(attr);
}

/* return 1 if probed OK */
__init int probe_soc_taurus(struct ca_soc_data *soc)
{
	u32 v;
	phys_addr_t phyaddr;
	int ret = 0;

	global_base = ioremap(GLOBAL_JTAG_ID, 0x1000);
	if (!global_base)
		return 0;

	v = SOC_READL(GLOBAL_JTAG_ID);
	if (RTK_SOC_CHIP_ID_GET(v) != CHIP_CA8277C)
		goto NOT_FOUND;

	soc->chip_id = RTK_SOC_CHIP_ID_GET(v);
	soc->vendor_id = RTK_SOC_VENDOR_ID_GET(v);
	soc->reg_global_software = GLOBAL_SOFTWARE;

	phyaddr = virt_to_phys(&soc->chipmode);
	arch_sync_dma_for_cpu(phyaddr, sizeof(soc->chipmode), DMA_BIDIRECTIONAL);
	ca_smc_call(CA_SVC_OTP_CHIP_MODE_READ, 0, 0, phyaddr, 0);

	__ca_clock_probe(soc);

	register_soc(soc);

	ret = 1;
NOT_FOUND:
	iounmap(global_base);
	return ret;
}
