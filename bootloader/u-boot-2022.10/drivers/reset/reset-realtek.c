// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Realtek
 *
 * Reset Driver for Realtek of SoCs
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <log.h>
#include <malloc.h>
#include <reset-uclass.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#define USB3CFG_PHY			(0xF432B000)

/* USB3(USB Super Speed) register offsets */
#define USB3CFG_CNTRL_OFFSET				0x00
#define USB3CFG_PHY_VAUX_RESET					BIT(31)
#define USB3CFG_BUS_CLKEN_GSLAVE				BIT(30)
#define USB3CFG_BUS_CLKEN_GMASTER				BIT(29)
#define USB3CFG_BIGENDIAN_GSLAVE				BIT(28)
#define USB3CFG_HOST_PORT_POWER_CTRL_PRESENT	BIT(27)
#define USB3CFG_HOST_MSI_ENABLE					BIT(26)
#define USB3CFG_HOST_LEGACY_SMI_PCI_CMD_REG_WR	BIT(25)
#define USB3CFG_HOST_LEGACY_SMI_BAR_WR			BIT(24)
#define USB3CFG_FLADJ_30MHZ_REG_MASK(x)			((0x3f & (x)) << 16)
#define USB3CFG_PHY_VAUX_RESET_U2_PORT1			BIT(9)
#define USB3CFG_PHY_VAUX_RESET_U2_PORT0			BIT(8)
#define USB3CFG_XHCI_BUS_MASTER_ENABLE			BIT(4)
#define USB3CFG_BUS_FILTER_BYPASS_MASK(x)		(0xf & (x))

struct realtek_reset_priv {
	void __iomem *regs;
};

static int realtek_reset_assert(struct reset_ctl *rst)
{
	struct realtek_reset_priv *priv = dev_get_priv(rst->dev);

	if ((strcmp(rst->dev->name, "block_rst") == 0 && rst->id == 13) ||
	    (strcmp(rst->dev->name, "block_ext_rst") == 0 && rst->id == 5))
		return 0;

	setbits_32(priv->regs, BIT(rst->id));
	mdelay(20);

	return 0;
}

static int realtek_reset_deassert(struct reset_ctl *rst)
{
	struct realtek_reset_priv *priv = dev_get_priv(rst->dev);
	u32 reg_val = 0;

	clrbits_32(priv->regs, BIT(rst->id));

	if (strcmp(rst->dev->name, "block_ext_rst") == 0 && rst->id == 5) {
		reg_val = readl(USB3CFG_PHY + USB3CFG_CNTRL_OFFSET);
		reg_val |= (USB3CFG_PHY_VAUX_RESET | USB3CFG_PHY_VAUX_RESET_U2_PORT0 | USB3CFG_PHY_VAUX_RESET_U2_PORT1);
		writel(reg_val, USB3CFG_PHY + USB3CFG_CNTRL_OFFSET);
	}

	mdelay(20);

	return 0;
}

struct reset_ops realtek_reset_ops = {
	.rst_assert = realtek_reset_assert,
	.rst_deassert = realtek_reset_deassert,
};

static const struct udevice_id realtek_reset_ids[] = {
	{ .compatible = "realtek,reset" },
	{ /* sentinel */ }
};

static int realtek_reset_probe(struct udevice *dev)
{
	struct realtek_reset_priv *priv = dev_get_priv(dev);

	priv->regs = dev_remap_addr(dev);

	if (!priv->regs)
		return -EINVAL;

	return 0;
}

U_BOOT_DRIVER(realtek_reset) = {
	.name = "realtek_reset",
	.id = UCLASS_RESET,
	.of_match = realtek_reset_ids,
	.probe = realtek_reset_probe,
	.ops = &realtek_reset_ops,
	.priv_auto	= sizeof(struct realtek_reset_priv),
};
