// SPDX-License-Identifier: GPL-2.0

#include <linux/version.h>
#include <linux/module.h>
#include <linux/io.h>
#include <soc/cortina/cortina-soc.h>
#include <soc/cortina/registers.h>

unsigned int rtk_soc_sysclk_get_khz(void)
{
	static unsigned int soc_clk;
	void __iomem *strap_base;
	GLOBAL_STRAP_t strap;

	if (soc_clk)
		return soc_clk;

	strap_base = ioremap(GLOBAL_STRAP, 4);

	if (!strap_base)
		return 0;

	strap.wrd = readl(strap_base);
	if (strap.bf.xtal_sel)
		soc_clk = 40000;
	else
		soc_clk = 25000;
	iounmap(strap_base);

	return soc_clk;
}


static void global_config_init(void)
{
	void __iomem *global_config_base;
	GLOBAL_GLOBAL_CONFIG_t global_config;

	global_config_base = ioremap(GLOBAL_GLOBAL_CONFIG, 4);

	if (!global_config_base)
		return;

	global_config.wrd = readl(global_config_base);
	global_config.bf.cfg_pcie_0_clken = 0;
	global_config.bf.cfg_pcie_1_clken = 0;
	global_config.bf.cfg_pcie_2_clken = 0;
	global_config.bf.ps_pcie0_en = 0;
	global_config.bf.ps_pcie2_en = 0;
	global_config.bf.ps_usb3_en = 0;
	writel(global_config.wrd, global_config_base);

	iounmap(global_config_base);
}

static void global_phy_power_init(void)
{
	void __iomem *global_phy_power_base;
	GLOBAL_PHY_ISO_POWER_CTRL_t global_phy_power;

	global_phy_power_base = ioremap(GLOBAL_PHY_ISO_POWER_CTRL, 4);

	if (!global_phy_power_base)
		return;

	global_phy_power.wrd = 0;
	global_phy_power.bf.s0_PCIE2_ISOLATE_0 = 1;
	global_phy_power.bf.s0_PCIE2_ISOLATE_1 = 1;
	global_phy_power.bf.s1_PCIE2_ISOLATE_0 = 1;
	global_phy_power.bf.s1_PCIE2_ISOLATE_1 = 1;
	global_phy_power.bf.s2_CPHY_ISOLATE = 1;
	global_phy_power.bf.s3_CPHY_ISOLATE = 1;
	writel(global_phy_power.wrd, global_phy_power_base);
	iounmap(global_phy_power_base);
}

static __init int soc_arch_init(void)
{
	global_config_init();
	global_phy_power_init();

	return 0;
}
arch_initcall(soc_arch_init);
