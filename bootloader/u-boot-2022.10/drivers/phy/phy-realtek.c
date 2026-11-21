// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Realtek
 *
 * Phy Driver for Realtek of SoCs
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <generic-phy.h>
#include <malloc.h>
#include <mapmem.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <dm/devres.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <dt-bindings/phy/phy.h>

//GLOBAL_PHY_CONTROL
#define USB2_LPM_PLL_ALIVE       BIT(4)
#define USB2_PORT_1_CLOSE        BIT(5)
#define USB2_REF_SEL_0           BIT(8)
#define USB2_REF_SEL_1           BIT(9)
#define USB2_REF_SEL_2           BIT(10)
#define USB2_REF_SEL_3           BIT(11)
#define SYS_PCIE2_AND_S2_SEL     BIT(18)
#define SYS_USB3_AND_S2_SEL      BIT(19)
#define S2_PCIE_USB3_SEL         BIT(20)
#define S2_PCIE_USB3_SGMII_SEL_0 BIT(21)
#define S2_PCIE_USB3_SGMII_SEL_1 BIT(22)
#define S3_PCIE_USB3_SEL         BIT(24)
#define S3_PCIE_USB3_SGMII_SEL_0 BIT(25)
#define S3_PCIE_USB3_SGMII_SEL_1 BIT(26)
#define S0_RXAUI_MODE            BIT(27)
#define S0_P_MDIO_ENABLE_REG	 BIT(28)
#define S1_P_MDIO_ENABLE_REG	 BIT(29)
#define S2_P_MDIO_ENABLE_REG	 BIT(30)
#define S3_P_MDIO_ENABLE_REG	 BIT(31)

//GLOBAL_PHY_MISC_CONTROL
#define S2_USB3_BG_EN           BIT(8)
#define S2_USB3_MBIAS_EN        BIT(9)
#define S3_USB3_BG_EN           BIT(10)
#define S3_USB3_MBIAS_EN        BIT(11)
#define S2_USB3_OOBS_PDB        BIT(30)
#define S3_USB3_OOBS_PDB        BIT(31)

//GLOBAL_PHY_STATUS
#define STATUS_USB2_CKUSABLE    GENMASK(1, 0)
#define STATUS_S2_CKUSABLE      BIT(3)
#define STATUS_S3_CKUSABLE      BIT(4)
#define STATUS_S0_CKUSABLE      BIT(5)
#define STATUS_S1_CKUSABLE      BIT(6)
#define STATUS_CKUSABLE_SGMII   BIT(7)

//GLOBAL_PHY_SGMII_MISC_CONTROL
#define PHY_SGMII_MISC_CONTROL  0xC
#define S3_SGMII_CKBUF_EN       BIT(0)
#define S3_SGMII_CLK_MODE_SEL_0 BIT(1)
#define S3_SGMII_CLK_MODE_SEL_1 BIT(2)
#define S3_SGMII_BG_EN          BIT(3)
#define S3_SGMII_MBIAS_EN       BIT(4)
#define S3_SGMII_RX50_LINK      BIT(5)
#define S3_SGMII_TX_DEEMP_SEL   BIT(7)
#define S3_SGMII_EQHOLD         BIT(8)

//GLOBAL_PHY_SGMII_PCS_CONTROL
#define PHY_SGMII_PCS_CONTROL   0x10

//GLOBAL_PHY_ISO_POWER_CONTROL
#define S0_PCIE2_ISOLATE_0      BIT(0)
#define S0_PCIE2_ISOLATE_1      BIT(1)
#define S1_PCIE2_ISOLATE_0      BIT(2)
#define S1_PCIE2_ISOLATE_1      BIT(3)
#define S2_CPHY_ISOLATE         BIT(4)
#define S3_CPHY_ISOLATE         BIT(5)
#define S0_POW_PCIE2_0          BIT(7)
#define S0_POW_PCIE2_1          BIT(8)
#define S1_POW_PCIE2_0          BIT(9)
#define S1_POW_PCIE2_1          BIT(10)
#define S2_POW_USB3             BIT(11)
#define S2_POW_PCIE2            BIT(12)
#define S3_POW_USB3             BIT(13)
#define S3_POW_PCIE2            BIT(14)
#define S0_ISO_ANA_B            BIT(16)
#define S1_ISO_ANA_B            BIT(17)
#define S2_ISO_ANA_B            BIT(18)
#define S3_ISO_ANA_B            BIT(19)
#define S0_LANE0_CA09PC_EN      BIT(20)
#define S0_LANE0_CA33PC_EN      BIT(21)
#define S0_LANE1_CA09PC_EN      BIT(22)
#define S0_LANE1_CA33PC_EN      BIT(23)
#define S1_LANE0_CA09PC_EN      BIT(24)
#define S1_LANE0_CA33PC_EN      BIT(25)
#define S1_LANE1_CA09PC_EN      BIT(26)
#define S1_LANE1_CA33PC_EN      BIT(27)
#define S2_CA09PC_EN            BIT(28)
#define S2_CA33PC_EN            BIT(29)
#define S3_CA09PC_EN            BIT(30)
#define S3_CA33PC_EN            BIT(31)

#define PHY_S0_PCIE0		0
#define PHY_S0_PCIE1		1
#define PHY_S1_PCIE0		2
#define PHY_S1_PCIE1		3
#define PHY_S2_PCIE0		4
#define PHY_S2_USB			5
#define PHY_S3_USB			6
#define PHY_S3_SGMII		7
#define PHY_S3_HSGMII		8
#define PHY_S0_RXAUI		9
#define PHY_NUMS			10

#define USB3CFG_PHY			(0xF432B000)
#define USB2CFG_PHY			(0xF432B020)
#define USB3EXT_REG			(0xF432B04C)
#define SERDES_S2_PHY		(0xF4337000)
#define SERDES_S3_PHY		(0xF4338000)

#define USB2CFG_PHY_PORT0_CONFIG_OFFSET		0x00
#define USB2CFG_PHY_PORT1_CONFIG_OFFSET		0x08
#define USB2CFG_PHY_PORT_VSTATUS_IN_MASK(x)		((0xff & (x)) << 24)
#define USB2CFG_PHY_PORT_VCNTRL_MASK(x)		((0xf & (x)) << 20)
#define USB2CFG_PHY_PORT_VLOADM					BIT(19)
#define USB2CFG_PHY_PORT_BY_PASS_ON				BIT(18)
#define USB2CFG_PHY_PORT_LF_PD_R_EN				BIT(9)
#define USB2CFG_PHY_PORT_CLKTSTEN				BIT(8)
#define USB2CFG_PHY_PORT_DPPULLDOWN				BIT(6)
#define USB2CFG_PHY_PORT_DMPULLDOWN				BIT(5)
#define USB2CFG_PHY_PORT_TXBITSTUFF_ENABLE		BIT(4)
#define USB2CFG_PHY_PORT_TXBITSTUFF_ENABLE_H	BIT(3)
#define USB2CFG_PHY_PORT_TX_ENABLE_N			BIT(2)
#define USB2CFG_PHY_PORT_TX_DAT					BIT(1)
#define USB2CFG_PHY_PORT_TX_SE0					BIT(0)

#define USB2CFG_PHY_PORT0_STATUS_OFFSET		0x04
#define USB2CFG_PHY_PORT1_STATUS_OFFSET		0x0c
#define USB2CFG_PHY_PORT_VSTATUS_OUT_MASK(x)	((0xff & (x)) << 24)
#define USB2CFG_PHY_PORT_DEBUG_MASK(x)			(0xff & (x))

#define USB2CFG_PHY_PORT0_FORCE_MODE_OFFSET 0x10
#define USB2CFG_PHY_PORT0_FSUS			BIT(7)
#define USB2CFG_PHY_PORT0_RX_TERM_SEL			BIT(9)
#define USB2CFG_PHY_PORT1_FORCE_MODE_OFFSET 0x14
#define USB2CFG_PHY_PORT1_RX_TERM_SEL			BIT(25)

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

#define USB3CFG_STATUS_OFFSET				0x04
#define USB3CFG_HOST_SYSTEM_ERR					BIT(31)
#define USB3CFG_LEGACY_SMI_INTERRUPT			BIT(16)
#define USB3CFG_HOST_CURRENT_BELT_MASK(x)		(0xfff & (x))

#define USB3CFG_PORT_CONFIG_OFFSET			0x08
#define USB3CFG_U3P1_HOST_DISABLE				BIT(26)
#define USB3CFG_U3P1_HUB_PORT_OVERCURRENT		BIT(25)
#define USB3CFG_U3P1_HUB_PORT_PERM_ATTACH		BIT(24)
#define USB3CFG_U3P0_HOST_DISABLE				BIT(18)
#define USB3CFG_U3P0_HUB_PORT_OVERCURRENT		BIT(17)
#define USB3CFG_U3P0_HUB_PORT_PERM_ATTACH		BIT(16)
#define USB3CFG_U2P1_HOST_DISABLE				BIT(10)
#define USB3CFG_U2P1_HUB_PORT_OVERCURRENT		BIT(9)
#define USB3CFG_U2P1_HUB_PORT_PERM_ATTACH		BIT(8)
#define USB3CFG_U2P0_HOST_DISABLE				BIT(2)
#define USB3CFG_U2P0_HUB_PORT_OVERCURRENT		BIT(1)
#define USB3CFG_U2P0_HUB_PORT_PERM_ATTACH		BIT(0)

#define USB3CFG_PORT_STATUS_OFFSET			0x0c
#define USB3CFG_U3P1_PIPE3_PHY_MODE_MASK(x)		((0x3 & (x)) << 28)
#define USB3CFG_U3P1_HUB_VBUS_CTRL				BIT(24)
#define USB3CFG_U3P0_PIPE3_PHY_MODE_MASK(x)		((0x3 & (x)) << 20)
#define USB3CFG_U3P0_HUB_VBUS_CTRL				BIT(16)
#define USB3CFG_U2P1_UTMI_FSLS_LOW_POWER		BIT(9)
#define USB3CFG_U2P1_HUB_VBUS_CTRL				BIT(8)
#define USB3CFG_U2P0_UTMI_FSLS_LOW_POWER		BIT(1)
#define USB3CFG_U2P0_HUB_VBUS_CTRL				BIT(0)

#define USB3CFG_PHY_PORT0_CONFIG0_OFFSET	0x10
#define USB3CFG_PHY_PORT1_CONFIG0_OFFSET	0x1c
#define USB3CFG_PHY_PORT_CKBUF_EN					BIT(31)
#define USB3CFG_PHY_PORT_MAC_PHY_PLL_EN_REG			BIT(30)
#define USB3CFG_PHY_PORT_MAC_PHY_RECV_DET_EN_REG	BIT(29)
#define USB3CFG_PHY_PORT_MAC_PHY_RECV_DET_REQ_REG	BIT(28)
#define USB3CFG_PHY_PORT_CLK_MODE_SEL_MASK(x)		(0x3 & (x))

#define USB3CFG_PHY_PORT0_STATUS0_OFFSET	0x14
#define USB3CFG_PHY_PORT1_STATUS0_OFFSET	0x20
#define USB3CFG_PHY_PORT_PHY_MAC_RECV_DET_END		BIT(21)
#define USB3CFG_PHY_PORT_PHY_MAC_RECV_DET_ACK		BIT(20)
#define USB3CFG_PHY_PORT_CLK_RDY					BIT(19)
#define USB3CFG_PHY_PORT_COUNT_NUM_VAL(x)			(0x7ffff & (x))

#define USB3CFG_PHY_PORT0_STATUS1_OFFSET	0x18
#define USB3CFG_PHY_PORT1_STATUS1_OFFSET	0x24
#define USB3CFG_PHY_PORT_DISPARITY_ERR_MASK(x)		((0xff & (x)) << 24)
#define USB3CFG_PHY_PORT_ELASTIC_BUF_UDF_MASK(x)	((0xff & (x)) << 16)
#define USB3CFG_PHY_PORT_ELASTIC_BUF_OVF_MASK(x)	((0xff & (x)) << 8)
#define USB3CFG_PHY_PORT_DECODE_ERR_MASK(x)			(0xff & (x))

#define USB3CFG_EXT_CTRL_OFFSET				0x00
#define USB3CFG_RX50_TERM_SEL				BIT(28)

struct usb_phy_data {
	const char *u3_ver;
	const char u3_phy_size;
	const u8 *u3_addr;
	const u16 *u3_data;
	const char *u2_ver;
	const char u2_phy_size;
	const u8 *u2_addr;
	const u8 *u2_data;
};

static const char taurus_u3_ver[] = "revB v1.0 (2022.02.24)";
static const u8 taurus_u3_phy_addr[] = {0x01, 0x04, 0x05, 0x08, 0x09,
							0x0a, 0x0b, 0x0d, 0x0e,
							0x20, 0x21, 0x22, 0x23, 0x27,
							0x2b, 0x33,
							0x09, 0x09, 0x09};
static const u16 taurus_u3_phy_data[] = {0xaccc, 0x7000, 0x6313, 0x30d1, 0x424c,
							0xc610, 0xa905, 0xef2e, 0x1000,
							0x705e, 0xf8e8, 0x0003, 0x0b66, 0x0241,
							0x0038, 0x0001,
							0x424c, 0x404c, 0x424c};
static const char taurus_u2_ver[] = "revB v1.1 (2022.04.20)";
static const u8 taurus_u2_phy_addr[] = {0xE0, 0xE2, 0xE4, 0xE7, 0xF4, 0xE7, 0xF4, 0xE0, 0xE0, 0xE0};
static const u8 taurus_u2_phy_data[] = {0x1F, 0xF9, 0x6B, 0x71, 0xDB, 0x43, 0xBB, 0x26, 0x22, 0x26};

struct usb_phy_data taurus_usb_data = {
	.u3_ver = taurus_u3_ver,
	.u3_phy_size = sizeof(taurus_u3_phy_addr),
	.u3_addr = taurus_u3_phy_addr,
	.u3_data = taurus_u3_phy_data,
	.u2_ver = taurus_u2_ver,
	.u2_phy_size = sizeof(taurus_u2_phy_addr),
	.u2_addr = taurus_u2_phy_addr,
	.u2_data = taurus_u2_phy_data,
};

static const char elnath_u3_ver[] = "revA v1.0 (2023.04.21)";
static const u8 elnath_u3_phy_addr[] = {0x06, 0x0A, 0x23, 0x0d, 0x20, 0x21, 0x0e};
static const u16 elnath_u3_phy_data[] = {0x0009, 0xb208, 0x9b66, 0xef2a, 0x70ff, 0x88bb, 0x1000};
static const char elnath_u2_ver[] = "revA v1.0 (2023.05.23)";
static const u8 elnath_u2_phy_addr[] = {0xF4, 0xE5, 0xF4, 0xE1, 0xE0, 0xE2, 0xE4, 0xF4, 0xE7, 0xF4, 0xE0, 0xE0};
static const u8 elnath_u2_phy_data[] = {0xBB, 0x5E, 0x9B, 0x99, 0x9B, 0x7B, 0x6D, 0xDB, 0x46, 0xBB, 0x21, 0x25};

struct usb_phy_data elnath_usb_data = {
	.u3_ver = elnath_u3_ver,
	.u3_phy_size = sizeof(elnath_u3_phy_addr),
	.u3_addr = elnath_u3_phy_addr,
	.u3_data = elnath_u3_phy_data,
	.u2_ver = elnath_u2_ver,
	.u2_phy_size = sizeof(elnath_u2_phy_addr),
	.u2_addr = elnath_u2_phy_addr,
	.u2_data = elnath_u2_phy_data,
};

struct rtk_phy_core {
	struct udevice *dev;
	void __iomem *reg_ctrl;
	void __iomem *reg_pwr;
	void __iomem *serdes_regbase;
	struct gpio_desc *vbus;
};

static void rtk_usb3_rx_term_setup(void)
{
	u32 reg_val = 0;

	reg_val = readl(USB3EXT_REG + USB3CFG_EXT_CTRL_OFFSET);
	reg_val &= ~USB3CFG_RX50_TERM_SEL;
	writel(reg_val, USB3EXT_REG + USB3CFG_EXT_CTRL_OFFSET);
}

static void rtk_usb2_rx_term_setup(void)
{
	u32 reg_val = 0;

	reg_val = readl(USB2CFG_PHY + USB2CFG_PHY_PORT0_FORCE_MODE_OFFSET);
	reg_val &= ~USB2CFG_PHY_PORT0_RX_TERM_SEL;
	reg_val |= USB2CFG_PHY_PORT0_FSUS;
	writel(reg_val, USB2CFG_PHY + USB2CFG_PHY_PORT0_FORCE_MODE_OFFSET);

	reg_val = readl(USB2CFG_PHY + USB2CFG_PHY_PORT1_FORCE_MODE_OFFSET);
	reg_val &= ~USB2CFG_PHY_PORT1_RX_TERM_SEL;
	writel(reg_val, USB2CFG_PHY + USB2CFG_PHY_PORT1_FORCE_MODE_OFFSET);
}

static void rtk_usb2_phy_port_cfg(u8 vstat_in_mask, u8 vctrl_mask, int port_id)
{
	u32 offset = 0;
	u32 reg_val = 0;
	u8 vctrl_mask1, vctrl_mask2;

	vctrl_mask1 = vctrl_mask & 0x0F;
	vctrl_mask2 = (vctrl_mask >> 4) & 0x0F;
	reg_val |= (USB2CFG_PHY_PORT_VSTATUS_IN_MASK(vstat_in_mask)
				| USB2CFG_PHY_PORT_VCNTRL_MASK(vctrl_mask1)
				| USB2CFG_PHY_PORT_DPPULLDOWN
				| USB2CFG_PHY_PORT_DMPULLDOWN
				| USB2CFG_PHY_PORT_VLOADM
				| USB2CFG_PHY_PORT_TX_ENABLE_N);

	if (port_id == 0) {
		offset = USB2CFG_PHY_PORT0_CONFIG_OFFSET;
	} else if (port_id == 1) {
		offset = USB2CFG_PHY_PORT1_CONFIG_OFFSET;
	} else {
		puts("The usb2phy port id is invalid.\n");
		return;
	}

	writel(reg_val, USB2CFG_PHY + offset);

	reg_val &= ~USB2CFG_PHY_PORT_VLOADM;
	writel(reg_val, USB2CFG_PHY + offset);

	reg_val |= USB2CFG_PHY_PORT_VLOADM;
	writel(reg_val, USB2CFG_PHY + offset);

	/* Clear VCONTROL field before refilling */
	reg_val &= ~USB2CFG_PHY_PORT_VCNTRL_MASK(0xF);
	reg_val |= USB2CFG_PHY_PORT_VCNTRL_MASK(vctrl_mask2);
	writel(reg_val, USB2CFG_PHY + offset);

	reg_val &= ~USB2CFG_PHY_PORT_VLOADM;
	writel(reg_val, USB2CFG_PHY + offset);

	reg_val |= USB2CFG_PHY_PORT_VLOADM;
	writel(reg_val, USB2CFG_PHY + offset);
}

static inline void rtk_usb3_phy_write(struct rtk_phy_core *core, u8 addr, u16 data)
{
	u32 offset = addr << 2;

	writel(data, core->serdes_regbase + offset);
}

static int rtk_phy_power_config(struct phy *p, bool enable)
{
	struct rtk_phy_core *core = dev_get_priv(p->dev);
	u32 pwr_en_set_m, pwr_en_clr_m;
	u32 pwr_v;

	switch (p->id) {
	case PHY_S0_PCIE0:
		pwr_en_set_m  = S0_POW_PCIE2_0 | S0_ISO_ANA_B | S0_LANE0_CA09PC_EN | S0_LANE0_CA33PC_EN;
		pwr_en_clr_m  = S0_PCIE2_ISOLATE_0;
		break;
	case PHY_S0_PCIE1:
		pwr_en_set_m  = S0_POW_PCIE2_1 | S0_LANE1_CA09PC_EN | S0_LANE1_CA33PC_EN;
		pwr_en_clr_m  = S0_PCIE2_ISOLATE_1;
		break;
	case PHY_S0_RXAUI:
		//lane 0
		pwr_en_set_m  = S0_POW_PCIE2_0 | S0_ISO_ANA_B | S0_LANE0_CA09PC_EN | S0_LANE0_CA33PC_EN;
		pwr_en_clr_m  = S0_PCIE2_ISOLATE_0;
		//lane 1
		pwr_en_set_m  |= S0_POW_PCIE2_1 | S0_LANE1_CA09PC_EN | S0_LANE1_CA33PC_EN;
		pwr_en_clr_m  |= S0_PCIE2_ISOLATE_1;
		break;
	case PHY_S1_PCIE0:
		pwr_en_set_m  = S1_POW_PCIE2_0 | S1_ISO_ANA_B | S1_LANE0_CA09PC_EN | S1_LANE0_CA33PC_EN;
		pwr_en_clr_m  = S1_PCIE2_ISOLATE_0;
		break;
	case PHY_S1_PCIE1:
		pwr_en_set_m  = S1_POW_PCIE2_1 | S1_LANE1_CA09PC_EN | S1_LANE1_CA33PC_EN;
		pwr_en_clr_m  = S1_PCIE2_ISOLATE_1;
		break;
	case PHY_S2_PCIE0:
		pwr_en_set_m  = S2_POW_PCIE2 | S2_ISO_ANA_B | S2_CA09PC_EN | S2_CA33PC_EN;
		pwr_en_clr_m  = S2_CPHY_ISOLATE | S2_POW_USB3;
		break;
	case PHY_S2_USB:
		pwr_en_set_m  = S2_POW_USB3 | S2_ISO_ANA_B | S2_CA09PC_EN | S2_CA33PC_EN;
		pwr_en_clr_m  = S2_CPHY_ISOLATE | S2_POW_PCIE2;
		break;
	case PHY_S3_USB:
		pwr_en_set_m  = S3_POW_USB3 | S3_ISO_ANA_B | S3_CA09PC_EN | S3_CA33PC_EN;
		pwr_en_clr_m  = S3_CPHY_ISOLATE | S3_POW_PCIE2;
		break;
	case PHY_S3_SGMII:
		pwr_en_set_m  = S3_ISO_ANA_B | S3_CA09PC_EN | S3_CA33PC_EN;
		pwr_en_clr_m  = S3_CPHY_ISOLATE | S3_POW_PCIE2 | S3_POW_USB3;
		break;
	case PHY_S3_HSGMII:
		pwr_en_set_m  = S3_ISO_ANA_B | S3_CA09PC_EN | S3_CA33PC_EN;
		pwr_en_clr_m  = S3_CPHY_ISOLATE | S3_POW_PCIE2 | S3_POW_USB3;
		break;
	default:
		debug("PHY %ld invalid\n", p->id);
		return -EINVAL;
	}

	pwr_v = readl(core->reg_pwr);

	if (enable) {
		pwr_v |= pwr_en_set_m;
		pwr_v &= ~pwr_en_clr_m;
	} else {
		pwr_v &= ~pwr_en_set_m;
	}

	writel(pwr_v, core->reg_pwr);
	mdelay(800);

	return 0;
}

static int rtk_phy_init(struct phy *p)
{
	struct rtk_phy_core *core = dev_get_priv(p->dev);
	int index, port_id;
	struct usb_phy_data *data;
	u32 reg_val = 0;
	u32 ctrl_en_set_m, ctrl_en_clr_m;
	u32 ctrl_v;

	data = (struct usb_phy_data *)dev_get_driver_data(p->dev);

	if (!data)
		return -EINVAL;

	switch (p->id) {
	case PHY_S0_PCIE0:
		ctrl_en_set_m = 0;
		ctrl_en_clr_m = S0_RXAUI_MODE;
		break;
	case PHY_S0_PCIE1:
		//assume lane 1 will not be alone, don't care about S0_RXAUI_MODE and S0_ISO_ANA_B
		ctrl_en_set_m = 0;
		ctrl_en_clr_m = 0;
		break;
	case PHY_S0_RXAUI:
		ctrl_en_set_m = S0_RXAUI_MODE;
		ctrl_en_clr_m = 0;
		break;
	case PHY_S1_PCIE0:
		ctrl_en_set_m = 0;
		ctrl_en_clr_m = 0;
		break;
	case PHY_S1_PCIE1:
		//assume lane 1 will not be alone, don't care about S1_ISO_ANA_B
		ctrl_en_set_m = 0;
		ctrl_en_clr_m = 0;
		break;
	case PHY_S2_PCIE0:
		ctrl_en_set_m = SYS_PCIE2_AND_S2_SEL;
		ctrl_en_clr_m = S2_PCIE_USB3_SEL | S2_PCIE_USB3_SGMII_SEL_0 | S2_PCIE_USB3_SGMII_SEL_1;
		break;
	case PHY_S2_USB:
		ctrl_en_set_m = S2_PCIE_USB3_SEL | SYS_USB3_AND_S2_SEL;
		ctrl_en_clr_m = S2_PCIE_USB3_SGMII_SEL_0 | S2_PCIE_USB3_SGMII_SEL_1;
		break;
	case PHY_S3_USB:
		ctrl_en_set_m = S3_PCIE_USB3_SEL;
		ctrl_en_clr_m = S3_PCIE_USB3_SGMII_SEL_0 | S3_PCIE_USB3_SGMII_SEL_1 | SYS_USB3_AND_S2_SEL;
		break;
	case PHY_S3_SGMII:
		ctrl_en_set_m = S3_PCIE_USB3_SGMII_SEL_0;
		ctrl_en_clr_m = S3_PCIE_USB3_SGMII_SEL_1;
		break;
	case PHY_S3_HSGMII:
		ctrl_en_set_m = S3_PCIE_USB3_SGMII_SEL_1;
		ctrl_en_clr_m = S3_PCIE_USB3_SGMII_SEL_0;
		break;
	default:
		debug("PHY %ld invalid\n", p->id);
		return -EINVAL;
	}

	ctrl_v = readl(core->reg_ctrl);
	ctrl_v |= ctrl_en_set_m;
	ctrl_v &= ~ctrl_en_clr_m;

	writel(ctrl_v, core->reg_ctrl);
	mdelay(20);

	switch (p->id) {
	case PHY_S2_USB:
	case PHY_S3_USB:
		//Pull-down resistor connected to DP
		//Pull-down resistor connected to DM
		reg_val = readl(USB2CFG_PHY + USB2CFG_PHY_PORT0_CONFIG_OFFSET);
		reg_val |= 0x00080064;
		writel(reg_val, USB2CFG_PHY + USB2CFG_PHY_PORT0_CONFIG_OFFSET);

		//Pull-down resistor connected to DP
		//Pull-down resistor connected to DM
		reg_val = readl(USB2CFG_PHY + USB2CFG_PHY_PORT1_CONFIG_OFFSET);
		reg_val |= 0x00080064;
		writel(reg_val, USB2CFG_PHY + USB2CFG_PHY_PORT1_CONFIG_OFFSET);
		mdelay(20);

		if (!core->serdes_regbase) {
			if (p->id == PHY_S2_USB)
				core->serdes_regbase = map_physmem(SERDES_S2_PHY, 0x80, MAP_NOCACHE);
			else if (p->id == PHY_S3_USB)
				core->serdes_regbase = map_physmem(SERDES_S3_PHY, 0x80, MAP_NOCACHE);
		}

		for (index = 0; index < data->u3_phy_size; index++)
			rtk_usb3_phy_write(core, *(data->u3_addr + index), *(data->u3_data + index));

		rtk_usb3_rx_term_setup();
		mdelay(20);
		for (port_id = 0; port_id < 2; port_id++) {
			if (1 << port_id & 3) {
				for (index = 0; index < data->u2_phy_size; index++)
					rtk_usb2_phy_port_cfg(*(data->u2_data + index), *(data->u2_addr + index), port_id);
			}
		}
		rtk_usb2_rx_term_setup();
		mdelay(20);
		break;
	default:
		debug("incompatible PHY type\n");
		return -EINVAL;
	}

	return 0;
}

static int rtk_phy_exit(struct phy *p)
{
	return 0;
}

static int rtk_phy_power_on(struct phy *p)
{
	return rtk_phy_power_config(p, true);
}

static int rtk_phy_power_off(struct phy *p)
{
	return rtk_phy_power_config(p, false);
}

static const struct phy_ops rtk_phy_ops = {
	.init		= rtk_phy_init,
	.exit		= rtk_phy_exit,
	.power_on	= rtk_phy_power_on,
	.power_off	= rtk_phy_power_off,

};

static int rtk_phy_probe(struct udevice *dev)
{
	struct rtk_phy_core *core = dev_get_priv(dev);
	struct usb_phy_data *data;
	int rc;

	core->dev = dev;

	core->reg_ctrl = dev_remap_addr_name(dev, "control");
	if (!core->reg_ctrl)
		return -EINVAL;

	core->reg_pwr = dev_remap_addr_name(dev, "power");
	if (!core->reg_ctrl)
		return -EINVAL;

	core->vbus = devm_gpiod_get_optional(dev, "usb-vbus",  GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

	if (core->vbus) {
		rc = dm_gpio_set_value(core->vbus, 1);
		if (rc)
			return rc;

		mdelay(100);
	}

	data = (struct usb_phy_data *)dev_get_driver_data(dev);
	if (!data)
		return -EINVAL;

	return 0;
}

static const struct udevice_id rtk_phy_id_table[] = {
	{ .compatible = "rtk,taurus-phy", .data = (ulong)&taurus_usb_data },
	{ .compatible = "rtk,elnath-phy", .data = (ulong)&elnath_usb_data },
	{ }
};

U_BOOT_DRIVER(rtk_phy) = {
	.name		= "rtk-phy",
	.id		= UCLASS_PHY,
	.of_match	= rtk_phy_id_table,
	.ops		= &rtk_phy_ops,
	.probe		= rtk_phy_probe,
	.priv_auto	= sizeof(struct rtk_phy_core),
};
