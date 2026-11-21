// SPDX-License-Identifier: GPL-2.0

#include <linux/version.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/arm-smccc.h>
#include <linux/dma-map-ops.h>
#include <soc/cortina/cortina-smc.h>
#include <soc/cortina/cortina-soc.h>
#include "soc_arm.h" /* defined FPGA */

#ifdef IS_FPGA
#include <soc/cortina/mercury_fpga_registers.h>
#else
#error "No mercury_registers yet"
#endif


#define SOC_READL(reg)			readl(global_base + (reg - GLOBAL_JTAG_ID))
#define SOC_WRITEL(val, reg)	writel(val, global_base + (reg - GLOBAL_JTAG_ID))
static void __iomem *global_base;

#define MHZ	(1000000)

static void __ca_clock_probe(struct ca_soc_data *soc)
{
#if defined(IS_FPGA)
#define MCECURY_HAPS_V8_PARTITION_DIV	100
#define MCECURY_REF_CLK		(MHZ / MCECURY_HAPS_V8_PARTITION_DIV)
	uint32_t ref_clk = 2000 * MCECURY_REF_CLK;

	soc->cpll = ref_clk;
	soc->epll = ref_clk;
	soc->fpll = ref_clk;
	soc->cpu_clk = ref_clk;
	soc->cci_clk = 600*MCECURY_REF_CLK;
	soc->lsaxi_clk = 250*MCECURY_REF_CLK;
	soc->hsaxi_clk = 600*MCECURY_REF_CLK;
	soc->core_clk = 750*MCECURY_REF_CLK;
	soc->crypto_clk = 600*MCECURY_REF_CLK;
	soc->eaxi_clk = 600*MCECURY_REF_CLK;
	soc->atb_clk = 600*MCECURY_REF_CLK;
	soc->pe_clk = 1200*MCECURY_REF_CLK;
	soc->peaxi_clk = 600*MCECURY_REF_CLK;
#endif
}


/* return 1 if probed OK */
__init int probe_soc_mercury(struct ca_soc_data *soc)
{
	u32 v;
	//phys_addr_t phyaddr;
	int ret = 0;

	global_base = ioremap(GLOBAL_JTAG_ID, 0x1000);
	if (!global_base)
		return 0;

	v = SOC_READL(GLOBAL_JTAG_ID);
#if 0 /* for fpga, alwyas success */
	if (RTK_SOC_CHIP_ID_GET(v) != CHIP_CA8277C)
		goto NOT_FOUND;
#endif

	soc->chip_id = RTK_SOC_CHIP_ID_GET(v);
	soc->vendor_id = RTK_SOC_VENDOR_ID_GET(v);

	//phyaddr = virt_to_phys(&soc->chipmode);
	//arch_sync_dma_for_cpu(phyaddr, sizeof(soc->chipmode), DMA_BIDIRECTIONAL);
	//ca_smc_call(CA_SVC_OTP_CHIP_MODE_READ, 0, 0, phyaddr, 0);

	__ca_clock_probe(soc);

	ret = 1;
//NOT_FOUND:
	iounmap(global_base);
	return ret;
}
