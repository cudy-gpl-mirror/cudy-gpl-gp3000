#include <miiphy.h>
#include <osal.h>
#include <asm/io.h>
#include <osal_type.h>
#include <rtk_mdio.h>
#include <realtek_ne.h>


#if (defined(CONFIG_TARGET_RTK_TAURUS) || defined(CONFIG_TARGET_RTK_ELNATH)) && !defined(CONFIG_RTK_V7_FPGA_PLATFORM)
void rtk_phy_global_config(void)
{
	WRAP_GPHY_CTRL12_t wrap_gphy_ctrl12;
	WRAP_GPHY_CTRL13_t wrap_gphy_ctrl13;

	/* Set WRAP_GPHY_CTRL12 cfg_ocp_timeout to 0xff */
	wrap_gphy_ctrl12.wrd = CA_REG_READ(WRAP_GPHY_CTRL12);
	wrap_gphy_ctrl12.bf.cfg_ocp_timeout = 0xff;
	CA_REG_WRITE(wrap_gphy_ctrl12.wrd, WRAP_GPHY_CTRL12);

	/* Set WRAP_GPHY_CTRL13 patch_phy_done to 1 */
	wrap_gphy_ctrl13.wrd = CA_REG_READ(WRAP_GPHY_CTRL13);
	wrap_gphy_ctrl13.bf.patch_phy_done = 1;
	CA_REG_WRITE(wrap_gphy_ctrl13.wrd, WRAP_GPHY_CTRL13);
}

void rtk_phy_config(struct phy_device *phydev)
{
	u32 val;

	/* REG31 write 0x0007, set to extension page */
	phy_write(phydev, MDIO_DEVAD_NONE, 31, 0x0007);

	/* REG30 write 0x0A43, set to extension page 2627 */
	phy_write(phydev, MDIO_DEVAD_NONE, 30, 0x0A43);

	/*
	 * REG24 write bit[7] =0
	 * fix half duplex unidirectional not wire speed.
	 * we should read REG24 and clear bit[7], and write back
	 */
	val = phy_read(phydev, MDIO_DEVAD_NONE, 24);
	val &= ~(1<<7);
	phy_write(phydev, MDIO_DEVAD_NONE, 24, val);

	return;
}
#else
void rtk_phy_global_config(void)
{
	// TODO: Add PHY associative SoC setting here if neccessary
	return;
}

void rtk_phy_config(struct phy_device *phydev)
{
	// TODO: Add internal PHY patch here if neccessary
	return;
}
#endif

void ca77xx_eth_change_default(void)
{
	u8 phy_addr;
	u16 val;

	rtk_phy_global_config();

	for (phy_addr = 4; phy_addr > 0; phy_addr--) {
		/* REG31 write 0x0007, set to extension page */
		ca_mdio_write(phy_addr, 1, 31, 0, 0x0007);

		/* REG30 write 0x0A43, set to extension page 2627 */
		ca_mdio_write(phy_addr, 1, 30, 0, 0x0A43);

		/*
		 * REG24 write bit[7] =0
		 * fix half duplex unidirectional not wire speed.
		 * we should read REG24 and clear bit[7], and write back
		 */
		ca_mdio_read(phy_addr, 1, 24, 0, &val);
		val &= ~(1<<7);
		ca_mdio_write(phy_addr, 1, 24, 0, val);
	}

#ifdef CONFIG_RTK_NE_SUPPORT_HSGMII_P7
#define MMD_VEND1               30   /* Vendor specific 1 */
#define MMD_VEND2               31   /* Vendor specific 2 */
	{
	u32 data;

	#ifdef CONFIG_RTK_NI_SUPPORT_RTL8221B
	/* port 7 8221B reset pin: GPIO3_17 */
	// external phy, reset pin operation
	data = CA_REG_READ(GLOBAL_GPIO_MUX_3);
	data |= 0x20000;
	CA_REG_WRITE(data, GLOBAL_GPIO_MUX_3);

	data = CA_REG_READ(PER_GPIO3_CFG);
	data &= ~0x20000;
	CA_REG_WRITE(data, PER_GPIO3_CFG);

	data = CA_REG_READ(PER_GPIO3_OUT);
	data &= ~0x20000;
	CA_REG_WRITE(data, PER_GPIO3_OUT);
	mdelay(50);

	data |= 0x20000;
	CA_REG_WRITE(data, PER_GPIO3_OUT);
	mdelay(100);
	#endif
	
	/* set MDIO pre-scale value to the same as kernel setting */
	data = CA_REG_READ(PER_MDIO_CFG);
	data = (data & ~0xFFFF0000) | (0xF << 16);
	CA_REG_WRITE(data, PER_MDIO_CFG);

	data = CA_REG_READ(GLOBAL_PIN_MUX_2);
	data |= 0x100000; // enable mdio
	CA_REG_WRITE(data, GLOBAL_PIN_MUX_2);

	// disable watchdog
	CA_REG_WRITE(0, PER_WDT_CTRL);

	#ifdef CONFIG_RTK_NI_SUPPORT_RTL8221B
	data = PORT7_8221B_PHY_ID;
	ca_mdio_write(data, 0, MMD_VEND1, 0x697a, 1); // serdes mode
	ca_mdio_write(data, 0, MMD_VEND2, 0xA400, 0x1340); // re-nway
	#endif
	}
#endif
}

