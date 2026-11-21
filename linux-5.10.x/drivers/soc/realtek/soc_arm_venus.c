// SPDX-License-Identifier: GPL-2.0

#include <linux/version.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/arm-smccc.h>
#include <linux/dma-map-ops.h>
#include <soc/cortina/cortina-smc.h>
#include <soc/cortina/cortina-soc.h>
#include <soc/cortina/ca8277b_registers.h>


#define SOC_READL(reg)			readl(global_base + (reg - GLOBAL_JTAG_ID))
#define SOC_WRITEL(val, reg)	writel(val, global_base + (reg - GLOBAL_JTAG_ID))
static void __iomem *global_base;

#define MHZ	(1000000)
#define CRYPTO_DIVSEL_MASK		GENMASK(6, 0)

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
	epll0.wrd =  SOC_READL(GLOBAL_EPLL0);
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

	if ((eplldiv.wrd & CRYPTO_DIVSEL_MASK) != 0)
		soc->crypto_clk = soc->epll * 2 / (eplldiv.wrd & CRYPTO_DIVSEL_MASK);
	else
		soc->crypto_clk = soc->epll; /* impossible */

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

/* return 1 if probed OK */
__init int probe_soc_venus(struct ca_soc_data *soc)
{
	u32 v;
	int ret = 0;

	global_base = ioremap(GLOBAL_JTAG_ID, 0x1000);
	if (!global_base)
		return 0;

	v = SOC_READL(GLOBAL_JTAG_ID);
	if ((CA_SOC_CHIP_ID(v) != CHIP_CA8277B) &&
		(CA_SOC_CHIP_ID(v) != CHIP_CA8289))
		goto NOT_FOUND;

	soc->chip_id = CA_SOC_CHIP_ID(v);
	soc->vendor_id = CA_SOC_VENDOR_ID(v);
	soc->reg_global_software = GLOBAL_SOFTWARE;

	__ca_clock_probe(soc);

	ret = 1;
NOT_FOUND:
	iounmap(global_base);
	return ret;
}
