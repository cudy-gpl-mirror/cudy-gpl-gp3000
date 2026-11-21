#include <common.h>
#include <linux/delay.h>
#include <phy.h>
#include "rtl8261i.h"

#define NI_8261I_DBG	1
#undef NI_8261I_DBG

#define MACSEC_REG_GET(phydev, dir, reg, pData) \
do\
{\
    if((ret = rtk_ne_rtl8261_macsec_reg_get(phydev, dir, reg, pData)) != 0)\
    {\
        return ret;\
    }\
} while (0)

#define MACSEC_REG_SET(phydev, dir, reg, data) \
do\
{\
    if((ret = rtk_ne_rtl8261_macsec_reg_set(phydev, dir, reg, data)) != 0)\
    {\
        return ret;\
    }\
} while (0)

u32 rtk_ne_rtl8261_broadcastID_set(struct phy_device *phydev, u32 broadcastID)
{
    u32 ret = 0;
    u32 phyData = 0;

    if (broadcastID > 0x1F)
        return 1;

	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x9003);
    phyData = REG32_FIELD_SET(phyData, broadcastID, 0, 0x1F);
	phy_write(phydev, PHY_MMD_VEND1, 0x9003, phyData);

    return 0;
}

u32 rtk_ne_rtl8261_broadcastEnable_set(struct phy_device *phydev, u32 enable)
{
    u32 ret = 0;
    u32 phyData = 0;

    if (enable == 0)
    {
        phy_write(phydev, PHY_MMD_VEND1, 0x9003, 0);
    }
    else
    {
    	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x9003);
        phyData |= (BIT_15);
		phy_write(phydev, PHY_MMD_VEND1, 0x9003, phyData);
    }

    return 0;
}

u32 rtk_ne_rtl8261_serdes_mode_init(struct phy_device *phydev)
{
    u32 ret = 0;
#if 0
    u32 phyData = 0;

	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x105);

    if(phyData & 0x1)
    {
        rtl826xb_info[unit]->sdsModeCfg[port] = RTK_PHY_CTRL_SERDES_MODE_UNKNOWN;
    }
    else
    {
        rtl826xb_info[unit]->sdsModeCfg[port] = RTK_PHY_CTRL_SERDES_MODE_USXGMII;
    }
#endif

    if ((ret = rtk_ne_rtl8261_sdsReg_write(phydev, 6, 3, 15, 0, 0x88C6) != 0))
        return ret;

    return 0;
}

u32 rtk_ne_rtl8261_interrupt_init(struct phy_device *phydev)
{
    u32 ret = 0;
    u32 phyData = 0;

    /* Disable all IMR*/
	phy_write(phydev, PHY_MMD_VEND1, 0xE1, 0);

    /* Disable all SDS IMR*/
	phy_write(phydev, PHY_MMD_VEND1, 0xE3, 0);

    /* Set SDS interrupt source to INT_PHY0 */
	phy_write(phydev, PHY_MMD_VEND1, 0xE4, 0x1);

	phy_write(phydev, PHY_MMD_VEND1, 0xE0, 0x2F);

    /* rlfd_en */
	phyData = phy_read(phydev, PHY_MMD_VEND2, 0xA442);
    phyData &= (~BIT_15);
    phyData |= (BIT_15);
	phy_write(phydev, PHY_MMD_VEND2, 0xA442, phyData);

    /* rlfd_fr_en */
	phyData = phy_read(phydev, PHY_MMD_VEND2, 0xA448);
	phyData |= (BIT_7);
	phy_write(phydev, PHY_MMD_VEND2, 0xA448, phyData);

    /* clear status */
	phyData = phy_read(phydev, PHY_MMD_VEND2, 0xA43A);
	phy_write(phydev, PHY_MMD_VEND1, 0xE2, 0x3F);

    return ret;
}

u32 rtk_ne_rtl8261_dbgCount_init(struct phy_device *phydev)
{
    u32 ret = 0;
    u32 phyData = 0;

	phy_write(phydev, PHY_MMD_VEND2, 0xC800, 0x5A02);

	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1EE);
    phyData |= BIT_1;
	phy_write(phydev, PHY_MMD_VEND1, 0x1EE, phyData);

	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x230);
    phyData |= BIT_1;
	phy_write(phydev, PHY_MMD_VEND1, 0x230, phyData);

	///-- RTK_PHY_CTRL_COUNTER_CLEAR
	phyData = phy_read(phydev, PHY_MMD_VEND2, 0xC802);
    phyData &= (~(0xFF));
    phyData |= (0x73);
	phy_write(phydev, PHY_MMD_VEND2, 0xC802, phyData);

	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1EF);
    phyData |= (BIT_13 | BIT_14 | BIT_15);
	phy_write(phydev, PHY_MMD_VEND1, 0x1EF, phyData);

	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1EF);
    phyData &= (~(BIT_13 | BIT_14 | BIT_15));
	phy_write(phydev, PHY_MMD_VEND1, 0x1EF, phyData);

	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x230);
    phyData |= (BIT_6 | BIT_7 | BIT_8);
	phy_write(phydev, PHY_MMD_VEND1, 0x230, phyData);

	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x230);
    phyData &= (~(BIT_6 | BIT_7 | BIT_8));
	phy_write(phydev, PHY_MMD_VEND1, 0x230, phyData);

    return ret;
}

u32 rtk_ne_rtl8261_is_single_port(struct phy_device *phydev)
{
    u32 ret = 0;
    u32 phyData;

	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x103);

    if ((phyData & 0xF) == 0x1)
    {
        return 1;
    }

    return 0;
}

u32 rtk_ne_rtl8261_synce_init(struct phy_device *phydev)
{
    u32 ret = 0;
    u32 phyData = 0;
    u32 is_singlePort = rtk_ne_rtl8261_is_single_port(phydev);

#define HWP_PHY_BASE_MACID(phydev)	0

    if (is_singlePort)
    {
		phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1C8);
        phyData &= (~(BIT_3));
        phyData |= (BIT_8);
		phy_write(phydev, PHY_MMD_VEND1, 0x1C8, phyData);

		phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1C9);
        phyData &= (~(BIT_4 | BIT_12));
        phyData |= (BIT_7 | BIT_15);
		phy_write(phydev, PHY_MMD_VEND1, 0x1C9, phyData);

		phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1CA);
        phyData &= (~(BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_8 | BIT_9 | BIT_10 | BIT_11 | BIT_12 | BIT_15));
        phyData |= (BIT_5 | BIT_13);
		phy_write(phydev, PHY_MMD_VEND1, 0x1CA, phyData);
    }
    else
    {
        if (HWP_PHY_BASE_MACID(phydev))
        {
        	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1C9);
	        phyData &= (~(BIT_4 | BIT_12));
            phyData |= (BIT_7 | BIT_15);
			phy_write(phydev, PHY_MMD_VEND1, 0x1C9, phyData);

			phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1CA);
	        phyData &= (~(BIT_4 | BIT_12));
            phyData |= (BIT_5 | BIT_13);
			phy_write(phydev, PHY_MMD_VEND1, 0x1CA, phyData);
        }

		phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1C8);
        phyData &= (~(BIT_3 | BIT_0));
        phyData |= (BIT_8);
		phy_write(phydev, PHY_MMD_VEND1, 0x1C8, phyData);

		phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1CA);
        phyData &= (~(BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_8 | BIT_9 | BIT_10 | BIT_11 | BIT_15));
		phy_write(phydev, PHY_MMD_VEND1, 0x1CA, phyData);
    }
    return ret;
}

u32 phy_macsec_init(struct phy_device *phydev)
{
    u32 ret = 0;
    u32 data = 0;

#define BIT_8_IN32	0x00000100U
#define MASK_8_BITS	(BIT_8_IN32 - 1)

    //HW version check
    MACSEC_REG_GET(phydev, RTK_MACSEC_DIR_EGRESS, 0xFFFC, &data);
    if ((data & MASK_8_BITS) != 160)
    {
        printf("PHY addr %u bad MACSec HW ver 0x%08X", phydev->addr, data);
        return 1;
    }

    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_EGRESS,  MACSEC_REG_COUNT_CTRL,     0x00000001);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_EGRESS,  MACSEC_REG_COUNT_CTRL,     0x0000000c);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_EGRESS,  MACSEC_REG_COUNT_SECFAIL1, 0x80fe0000);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_EGRESS,  MACSEC_REG_MISC_CONTROL,   0x02000046);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_EGRESS,  MACSEC_REG_CTX_CTRL,       0xe5880618);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_EGRESS,  MACSEC_REG_CTX_UPD_CTRL,   0x00000003);

    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_COUNT_CTRL,     0x00000001);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_COUNT_CTRL,     0x0000000c);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_COUNT_SECFAIL1, 0x80fe0000);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_MISC_CONTROL,   0x01001046);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_CTX_CTRL,       0xe5880614);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_CTX_UPD_CTRL,   0x00000003);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_SAM_CP_TAG,     0xe0fac688);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_IG_CC_CONTROL,  0x0000C000);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_SAM_NM_PARAMS,  0xe588003f);

    /* default Egress non-match pkt action:
       bypass: NCP - KaY tag; CP - Untagged
       drop: others
       */
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_EGRESS,  MACSEC_REG_SAM_NM_FLOW_NCP,    0x00010101);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_EGRESS,  MACSEC_REG_SAM_NM_FLOW_CP,     0x01010100);
    /* default Ingress non-match pkt action:
       bypass: NCP - KaY tag,Tagged; CP - Untagged
       drop: others
       */
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_SAM_NM_FLOW_NCP,    0x08090809);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_INGRESS, MACSEC_REG_SAM_NM_FLOW_CP,     0x09090908);

    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_EGRESS,  0xF810,  0x000003ff);
    MACSEC_REG_SET(phydev, RTK_MACSEC_DIR_EGRESS,  0xF808,  0x00000300);

    return ret;
}

u32 rtk_ne_rtl8261_macsec_init(struct phy_device *phydev)
{
    u32 ret = 0;

    if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x2e0, 1, 0, 0b11)) != 0)
        return ret;

    if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x2d8, 15, 0, 0x5212)) != 0)
        return ret;

    if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x2da, 15, 0, 0x0101)) != 0)
        return ret;
    if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x2dc, 15, 0, 0x0101)) != 0)
        return ret;

    //MACSEC_RXSYS_CFG4
    if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x3c6, 7, 0, 0xa)) != 0)
        return ret;
    //MACSEC_TXLINE_CFG4
    if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x37b, 7, 0, 0x6)) != 0)
        return ret;
    //loopback fifo_setting
    if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x2f7, 15, 0, 0x486c)) != 0)
        return ret;
    //RA_setting
    if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x3f1, 15, 0, 0x72)) != 0)
        return ret;
    if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x3f0, 15, 0, 0x0b0b)) != 0)
        return ret;

    //RA ifg
    if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x3ee, 15, 13, 0x2)) != 0)
        return ret;

    if ((ret = phy_macsec_init(phydev)) != 0)
        return ret;

    return ret;
}

u32 rtk_ne_rtl8261_init(struct phy_device *phydev)
{
    u32 ret = 0;

    if ((ret = rtk_ne_rtl8261_serdes_mode_init(phydev)) != 0)
        return ret;

    if ((ret = rtk_ne_rtl8261_interrupt_init(phydev)) != 0)
        return ret;

    if ((ret = rtk_ne_rtl8261_dbgCount_init(phydev)) != 0)
        return ret;

    if ((ret = rtk_ne_rtl8261_synce_init(phydev)) != 0)
        return ret;

    if ((ret = rtk_ne_rtl8261_macsec_init(phydev)) != 0)
        return ret;

    return 0;
}

u32 rtk_ne_rtl8261_autoNegoEnable_set(struct phy_device *phydev, rtk_enable_t enable)
{
    return rtk_ne_rtl8261_c45_autoNegoEnable_set(phydev, enable);
}

u32 rtk_ne_rtl8261_autoNegoAbility_set(struct phy_device *phydev)
{
	u32 phyData = 0;

	phy_write(phydev, PHY_MMD_AN, 16, 0x1581);
	phy_write(phydev, PHY_MMD_AN, 32, 0x7181);
	phy_write(phydev, PHY_MMD_VEND2, 0xA412, 0xe00);

	phyData = phy_read(phydev, PHY_MMD_AN, 0);
	if (phyData & BIT_12) /*AN is enabled*/
    {
    	phyData |= BIT_9; /*AN restart*/
		phy_write(phydev, PHY_MMD_AN, 0, phyData);
	}
}

u32 rtk_ne_rtl8261_dal_phy_init(struct phy_device *phydev)
{
	u32 phyData = 0;

	// phy_826xb_autoNegoAbility_set()
	phy_write(phydev, PHY_MMD_AN, 16, 0x1581);
	phy_write(phydev, PHY_MMD_AN, 32, 0x7181);
	phy_write(phydev, PHY_MMD_VEND2, 0xA412, 0xf00);

	phyData = phy_read(phydev, PHY_MMD_AN, 0);
	if (phyData & BIT_12) /*AN is enabled*/
    {
    	phyData |= BIT_9; /*AN restart*/
		phy_write(phydev, PHY_MMD_AN, 0, phyData);
	}

	// phy_826xb_eeeEnable_set()
	phy_write(phydev, PHY_MMD_AN, 60, 0);
	phy_write(phydev, PHY_MMD_AN, 62, 0);
	phy_write(phydev, PHY_MMD_VEND2, 0xA428, 0);

	phyData = phy_read(phydev, PHY_MMD_AN, 0);
	if (phyData & BIT_12) /*AN is enabled*/
    {
    	phyData |= BIT_9; /*AN restart*/
		phy_write(phydev, PHY_MMD_AN, 0, phyData);
	}

	// dal_phy_portAutoNegoEnablePortmask_set()
	/// phy_826xb_autoNegoEnable_set()
	rtk_ne_rtl8261_autoNegoEnable_set(phydev, ENABLED);
	/*Delay 100ms*/
	mdelay(100);

	// phy_826xb_autoNegoAbility_set()
	rtk_ne_rtl8261_autoNegoAbility_set(phydev);

	return 0;
}

void rtl8261_config(struct phy_device *phydev)
{
	// __eth_phy_rtkmsphy_init()
	// rtk_init()

	/* Construct PHY */
	/// phy_construct_config_init()
	//// _phy_construct_config_init_broadcast()
	///// phy_826xb_ctrl_set()
	rtk_ne_rtl8261_broadcastID_set(phydev, phydev->addr);
	///// phy_826xb_broadcastEnable_set()
	rtk_ne_rtl8261_broadcastEnable_set(phydev, 1);
	///// phy_patch()
	// TODO: Add later if need
	///// phy_826xb_broadcastEnable_set()
	rtk_ne_rtl8261_broadcastEnable_set(phydev, 0);
	//// rtl826XB_config()

	/* Init PHY */
	/// phy_init()
	//// phy_826xb_init()
	rtk_ne_rtl8261_init(phydev);

	/* Init PHY module */
	/// pMgmt_node[0]->pMapper->phy_init
	//// dal_phy_init()
	///--rtk_ne_rtl8261_dal_phy_init() will restart AN, do not do that
	///--rtk_ne_rtl8261_dal_phy_init(phydev);

	return;
}

u32 rtk_ne_rtl8261_serdes_mode_usxgmii_set(struct phy_device *phydev)
{
    u32 data = 0;

	data = phy_read(phydev, PHY_MMD_VEND1, 0x105);
	data = REG32_FIELD_SET(data, 0x0, 0, 0x1);
	phy_write(phydev, PHY_MMD_VEND1, 0x105, data);

	data = phy_read(phydev, PHY_MMD_VEND1, 0xC2);
	data = REG32_FIELD_SET(data, 0xD, 0, 0x3FF);
	phy_write(phydev, PHY_MMD_VEND1, 0xC2, data);

	data = phy_read(phydev, PHY_MMD_VEND1, 0x3F1);
	data = REG32_FIELD_SET(data, 0x72, 0, 0xFFFF);
	phy_write(phydev, PHY_MMD_VEND1, 0x3F1, data);

	data = phy_read(phydev, PHY_MMD_VEND1, 0x2A2);
	data = REG32_FIELD_SET(data, 0x0, 7, 0x80);
	phy_write(phydev, PHY_MMD_VEND1, 0x2A2, data);

    return 0;
}

u32 rtk_ne_rtl8261_serdes_mode_sgmii_set(struct phy_device *phydev)
{
    u32 data = 0;

	data = phy_read(phydev, PHY_MMD_VEND1, 0x105);
	data = REG32_FIELD_SET(data, 0x1, 0, 0x1);
	phy_write(phydev, PHY_MMD_VEND1, 0x105, data);

	data = phy_read(phydev, PHY_MMD_VEND1, 0xC3);
	data = REG32_FIELD_SET(data, 0x2, 0, 0xF);
	phy_write(phydev, PHY_MMD_VEND1, 0xC3, data);

	data = phy_read(phydev, PHY_MMD_VEND1, 0xC2);
	data = REG32_FIELD_SET(data, 0x0, 5, 0x3E0);
	phy_write(phydev, PHY_MMD_VEND1, 0xC2, data);

	data = phy_read(phydev, PHY_MMD_VEND1, 0x2A2);
	data = REG32_FIELD_SET(data, 0x0, 7, 0x80);
	phy_write(phydev, PHY_MMD_VEND1, 0x2A2, data);

    return 0;
}

u32 rtk_ne_rtl8261_serdes_reg_write(struct phy_device *phydev, u32 page, u32 reg, u32 data)
{
    u32 regData = 0, phyData = 0;

    regData |= (page & 0x3f);
    regData |= ((reg & 0x1f) << 6);
    regData |= (BIT_11 | BIT_15);
    phyData = data;

	phy_write(phydev, PHY_MMD_VEND1, 321, phyData);
	phy_write(phydev, PHY_MMD_VEND1, 323, phyData);

    return 0;
}

u32 rtk_ne_rtl8261_serdes_reg_read(struct phy_device *phydev, u32 page, u32 reg, u32 *pData)
{
    u32 i;
    u32 regData = 0;
    u32 phyData = 0;

    regData |= (page & 0x3f);
    regData |= ((reg & 0x1f) << 6);
    regData |= BIT_15;

	phy_write(phydev, PHY_MMD_VEND1, 323, regData);

    for (i = 0; i < PHY_826X_SERDES_MAX_RETRY; i++)
    {
    	regData = phy_read(phydev, PHY_MMD_VEND1, 323);
        if ((regData & BIT_15) == 0)
        {
            break;
        }
        mdelay(10);
    }
    if (i == PHY_826X_SERDES_MAX_RETRY)
    {
        return 1;
    }

	phyData = phy_read(phydev, PHY_MMD_VEND1, 322);

    *pData = phyData;
    return 0;
}

u32 rtk_ne_rtl8261_sdsReg_write(struct phy_device *phydev, u32 page, u32 reg, u8 msb, u8 lsb, u32 data)
{
    u32 ret = 0;
    u32 mask = 0, rData = 0, wData = 0;

    mask = UINT32_BITS_MASK(msb,lsb);

    if ((msb != 15) || (lsb != 0))
    {
        if ((ret = rtk_ne_rtl8261_serdes_reg_read(phydev, page, reg, &rData)) != 0)
            return ret;
    }
    wData = REG32_FIELD_SET(rData, data, lsb, mask);
    if ((ret = rtk_ne_rtl8261_serdes_reg_write(phydev, page, reg, wData)) != 0)
        return ret;
    return 0;
}

u32 rtk_ne_rtl8261_sdsReg_read(struct phy_device *phydev, u32 page, u32 reg, u8 msb, u8 lsb, u32 *pData)
{
    u32 ret = 0;
    u32 mask = 0, rData = 0;

    mask = UINT32_BITS_MASK(msb,lsb);

    if ((ret = rtk_ne_rtl8261_serdes_reg_read(phydev, page, reg, &rData)) != 0)
        return ret;

    *pData = REG32_FIELD_GET(rData, lsb, mask);
    return 0;
}

u32 rtk_ne_rtl8261_phyReg_write(struct phy_device *phydev, u32 page, u32 reg, u8 msb, u8 lsb, u32 data)
{
    u32 ret = 0;
    u32 mask = 0, rData = 0, wData = 0;

    mask = UINT32_BITS_MASK(msb,lsb);

    if ((msb != 15) || (lsb != 0))
    {
    	rData = phy_read(phydev, page, reg);
    }
    wData = REG32_FIELD_SET(rData, data, lsb, mask);
	phy_write(phydev, page, reg, wData);
    return 0;
}

u32 rtk_ne_rtl8261_phyReg_read(struct phy_device *phydev, u32 page, u32 reg, u8 msb, u8 lsb, u32 *pData)
{
    u32 ret = 0;
    u32 mask = 0, rData = 0;

    mask = UINT32_BITS_MASK(msb,lsb);

	rData = phy_read(phydev, page, reg);

    *pData = REG32_FIELD_GET(rData, lsb, mask);
    return 0;
}

u32 rtk_ne_rtl8261_macsec_reg_get(struct phy_device *phydev, rtk_macsec_dir_t dir, u32 reg, u32 *pData)
{
    u32 ret = 0;
    u32 data_h = 0;
    u32 data_l = 0;

    switch(dir)
    {
        case RTK_MACSEC_DIR_EGRESS:
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x02FB, 15, 0, reg)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x02FC, 15, 0, 0x10)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_read(phydev, 30, 0x02F8, 15, 0, &data_h)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_read(phydev, 30, 0x02F9, 15, 0, &data_l)) != 0)
                return ret;
            break;
        case RTK_MACSEC_DIR_INGRESS:
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x0300, 15, 0, reg)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x0301, 15, 0, 0x10)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_read(phydev, 30, 0x02FD, 15, 0, &data_h)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_read(phydev, 30, 0x02FE, 15, 0, &data_l)) != 0)
                return ret;
            break;
        default:
            return 1;
    }

    *pData = (data_h << 16) + data_l;
    return ret;
}

u32 rtk_ne_rtl8261_macsec_reg_set(struct phy_device *phydev, rtk_macsec_dir_t dir, u32 reg, u32 data)
{
    u32 ret = 0;
    u32 data_l = data & 0xFFFF;
    u32 data_h = (data >> 16) & 0xFFFF;

    switch(dir)
    {
        case RTK_MACSEC_DIR_EGRESS:
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x02F8 , 15, 0, data_h)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x02F9 , 15, 0, data_l)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x02FB , 15, 0, reg)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x02FC, 15, 0, 0x1)) != 0)
                return ret;
            break;

        case RTK_MACSEC_DIR_INGRESS:
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x02FD, 15, 0, data_h)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x02FE, 15, 0, data_l)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x0300, 15, 0, reg)) != 0)
                return ret;
            if ((ret = rtk_ne_rtl8261_phyReg_write(phydev, 30, 0x0301, 15, 0, 0x1)) != 0)
                return ret;
            break;

        default:
            return 1;
    }

    return ret;
}

u32 rtk_ne_rtl8261_realTime_linkStatus_get(struct phy_device *phydev, rtk_port_linkStatus_t *pStatus)
{
    u32 ret = 0;
    u32 phyData = 0;

	phyData = phy_read(phydev, PHY_MMD_VEND2, 0xA434);

    *pStatus = (phyData & BIT_2) ? PORT_LINKUP : PORT_LINKDOWN;
    return ret;
}

u32 rtk_ne_rtl8261_linkStatus_get(struct phy_device *phydev, rtk_port_linkStatus_t *pStatus)
{
    u32 ret;
    u32 phyData = 0;

	phyData = phy_read(phydev, PHY_MMD_PMAPMD, 0x1);
    *pStatus = (phyData & BIT_2) ? PORT_LINKUP : PORT_LINKDOWN;
    return ret;
}

u32 rtk_ne_rtl8261_c45_speedDuplexStatusResReg_get(struct phy_device *phydev, rtk_port_speed_t *pSpeed, rtk_port_duplex_t *pDuplex)
{
    u32 ret = 1;
    u32 phyData, spd;

    *pSpeed = PORT_SPEED_10M;
    *pDuplex = PORT_HALF_DUPLEX;
	phyData = phy_read(phydev, PHY_MMD_VEND2, 0xA434);

    /* [10:9,5:4] */
    spd = ((phyData & (0x3 << 9)) >> (9 - 2)) | ((phyData & (0x3 << 4)) >> 4);
    switch (spd)
    {
        case 0x0:
          *pSpeed = PORT_SPEED_10M;
          break;
        case 0x1:
          *pSpeed = PORT_SPEED_100M;
          break;
        case 0x2:
          *pSpeed = PORT_SPEED_1000M;
          break;
        case 0x3:
          *pSpeed = PORT_SPEED_500M;
          break;
        case 0x4:
          *pSpeed = PORT_SPEED_10G;
          break;
        case 0x5:
          *pSpeed = PORT_SPEED_2_5G;
          break;
        case 0x6:
          *pSpeed = PORT_SPEED_5G;
          break;
        case 0x7:
          *pSpeed = PORT_SPEED_2_5G_LITE;
          break;
        case 0x8:
          *pSpeed = PORT_SPEED_5G_LITE;
          break;
        case 0x9:
          *pSpeed = PORT_SPEED_10G_LITE;
          break;
        default:
          *pSpeed = PORT_SPEED_10M;
          break;
    }

    *pDuplex = (phyData & 0x8)? PORT_FULL_DUPLEX : PORT_HALF_DUPLEX;
    return 0;
}

u32 rtk_ne_rtl8261_c45_speedStatusResReg_get(struct phy_device *phydev, rtk_port_speed_t *pSpeed)
{
    u32 ret = 1;
    rtk_port_duplex_t duplex;

    *pSpeed = PORT_SPEED_10M;
    ret = rtk_ne_rtl8261_c45_speedDuplexStatusResReg_get(phydev, pSpeed, &duplex);

    return ret;
}

u32 rtk_ne_rtl8261_c45_autoNegoEnable_set(struct phy_device *phydev, rtk_enable_t enable)
{
    u32 ret = 0;
    u32 phyData = 0;

	phyData = phy_read(phydev, PHY_MMD_AN, 0);
    phyData &= (~(BIT_12 | BIT_9));
    phyData |= (enable == ENABLED) ? (BIT_12 | BIT_9) : (0);

	phy_write(phydev, PHY_MMD_AN, 0, phyData);
    return ret;
}

u32 rtk_ne_rtl8261_macIntfSerdesMode_get(struct phy_device *phydev, rt_serdesMode_t *pMode)
{
    u32 ret = 0;
    u32 sdsmode = 0;
    u32 cprimode = 0;
    u32 phyData = 0;
    rtk_port_linkStatus_t link = PORT_LINKDOWN;
    rtk_port_speed_t speed = PORT_SPEED_10G;

	sdsmode = RTK_PHY_CTRL_SERDES_MODE_USXGMII;

    if ((ret = rtk_ne_rtl8261_realTime_linkStatus_get(phydev, &link)) != 0)
        return ret;

    if (PORT_LINKUP == link)
    {
        if ((ret = rtk_ne_rtl8261_c45_speedStatusResReg_get(phydev, &speed)) != 0)
        {
            return ret;
        }
    }

    ///--printf("PHY addr %u macIntfSerdesMode m:0x%04X, l:%u, s:%u\n", phydev->addr, sdsmode, link, speed);

    if ((ret = rtk_ne_rtl8261_serdes_reg_read(phydev, 31, 22, &phyData)) != 0)
                return ret;

    sdsmode = (phyData & 0x1F);

    switch (sdsmode)
    {
        case 0b00010:
            *pMode = RTK_MII_SGMII;
            break;

        case 0b00100:
            *pMode = RTK_MII_1000BX_FIBER;
            break;

        case 0b01101:
        {
            switch (speed)
            {
                case PORT_SPEED_1000M:
                    *pMode = RTK_MII_USXGMII_1G;
                    break;
                case PORT_SPEED_100M:
                    *pMode = RTK_MII_USXGMII_100M;
                    break;
                case PORT_SPEED_2_5G:
                case PORT_SPEED_5G_LITE:
                    *pMode = RTK_MII_USXGMII_2_5GSXGMII;
                    break;
                case PORT_SPEED_5G:
                case PORT_SPEED_10G_LITE:
                    *pMode = RTK_MII_USXGMII_5GSXGMII;
                    break;
                case PORT_SPEED_10G:
                default:
                    *pMode = RTK_MII_USXGMII_10GSXGMII;
                    break;
            }
        }
        break;

        case 0b10101:
            *pMode = RTK_MII_5GBASEX;
            break;

        case 0b10110:
            *pMode = RTK_MII_2500Base_X;
            break;

        case 0b11010:
            if ((ret = rtk_ne_rtl8261_serdes_reg_read(phydev, 31, 19, &phyData)) != 0)
                return ret;
            cprimode = (phyData & 0x1F);

            if (cprimode == 0x2)
            {
                *pMode = RTK_MII_5GR;
                break;
            }
            else if (cprimode == 0x1)
            {
                *pMode = RTK_MII_XFI_5G_CPRI;
                break;
            }

			phyData = phy_read(phydev, PHY_MMD_VEND1, 0x68);
            if (phyData & BIT_15)
            {
                switch (speed)
                {
                    case PORT_SPEED_2_5G:
                    case PORT_SPEED_5G_LITE:
                        *pMode = RTK_MII_XFI_2P5G_ADAPT;
                        break;
                    case PORT_SPEED_5G:
                    case PORT_SPEED_10G_LITE:
                        *pMode = RTK_MII_XFI_5G_ADAPT;
                        break;
                    default:
                        *pMode = RTK_MII_10GR;
                        break;
                }
            }
            else
            {
                *pMode = RTK_MII_10GR;
            }

            break;

        default:
            *pMode = RTK_MII_DISABLE;
    }

    return 0;
}

u32 rtk_ne_rtl8261_macIntfSerdesLinkStatus_get(struct phy_device *phydev, rtk_port_linkStatus_t *pStatus)
{
    u32 ret = 0;
    u32 phyData = 0;
    rt_serdesMode_t mode = 0;

    if ((ret = rtk_ne_rtl8261_macIntfSerdesMode_get(phydev, &mode)) != 0)
        return ret;

    switch (mode)
    {
        case RTK_MII_5GBASEX:
        case RTK_MII_2500Base_X:
        case RTK_MII_SGMII:
            if ((ret = rtk_ne_rtl8261_serdes_reg_read(phydev, 0x1, 29, &phyData)) != 0)
                return ret;

            *pStatus = ((phyData & 0x111) == 0x111) ? (PORT_LINKUP) : (PORT_LINKDOWN);
            break;

        case RTK_MII_USXGMII_10GSXGMII:
        case RTK_MII_USXGMII_5GSXGMII:
        case RTK_MII_USXGMII_2_5GSXGMII:
        case RTK_MII_USXGMII_1G:
        case RTK_MII_USXGMII_100M:
        case RTK_MII_10GR:
        case RTK_MII_XFI_5G_ADAPT:
        case RTK_MII_XFI_5G_CPRI:
        case RTK_MII_XFI_2P5G_ADAPT:
        case RTK_MII_5GR:
        default:
            if ((ret = rtk_ne_rtl8261_serdes_reg_read(phydev, 0x5, 0, &phyData)) != 0)
                return ret;
            *pStatus = (phyData & BIT_12) ? (PORT_LINKUP) : (PORT_LINKDOWN);
            break;
    }

    return ret;
}

u32 rtk_ne_rtl8261_dbgCounter_get(struct phy_device *phydev, rtk_port_phy_dbg_cnt_t type, u64 *pCnt)
{
    u32 ret = 0;
    u32 phyData = 0;
    u64 cnt = 0;
    rtk_port_speed_t spd = 0;
    u8 tg = 0;

    switch (type)
    {
        case PHY_DBG_CNT_RX:
			phyData = phy_read(phydev, PHY_MMD_VEND2, 0xC81A);
            cnt |= (((u64)(phyData & 0xFF)) << 32);
			phyData = phy_read(phydev, PHY_MMD_VEND2, 0xC812);
            cnt |= (((u64)(phyData & 0xFFFF)) << 16);
			phyData = phy_read(phydev, PHY_MMD_VEND2, 0xC810);
            cnt |= ((u64)(phyData & 0xFFFF));
            break;
        case PHY_DBG_CNT_RX_ERR:
            if ((ret = rtk_ne_rtl8261_c45_speedStatusResReg_get(phydev, &spd)) != 0)
            {
                spd = PORT_SPEED_10G;
            }
            switch (spd)
            {
                case PORT_SPEED_10M:
                case PORT_SPEED_100M:
                case PORT_SPEED_1000M:
                case PORT_SPEED_500M:
                    tg = 0;
                    break;
                default:
                    tg = 1;
                    break;
            }
            if (tg == 1)
            {
            	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x230);
                phyData &= (~BIT_1);
				phy_write(phydev, PHY_MMD_VEND1, 0x230, phyData);

				phyData = phy_read(phydev, PHY_MMD_VEND1, 0x24A);
                cnt |= (((u64)(phyData & 0xFFFF)) << 32);
				phyData = phy_read(phydev, PHY_MMD_VEND1, 0x249);
                cnt |= (((u64)(phyData & 0xFFFF)) << 16);
				phyData = phy_read(phydev, PHY_MMD_VEND1, 0x248);
                cnt |= ((u64)(phyData & 0xFFFF));

				phyData = phy_read(phydev, PHY_MMD_VEND1, 0x230);
                phyData |= (BIT_1);
				phy_write(phydev, PHY_MMD_VEND1, 0x230, phyData);
            }
            else
            {
            	phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1EE);
                phyData &= (~BIT_1);
				phy_write(phydev, PHY_MMD_VEND1, 0x1EE, phyData);

				phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1F7);
                cnt |= ((u64)phyData);


				phyData = phy_read(phydev, PHY_MMD_VEND1, 0x1EE);
                phyData |= (BIT_1);
				phy_write(phydev, PHY_MMD_VEND1, 0x1EE, phyData);
            }
            break;
        case PHY_DBG_CNT_RX_CRCERR:
			phyData = phy_read(phydev, PHY_MMD_VEND2, 0xC814);
            cnt = ((u64)(phyData & 0xFFFF));
            break;

        case PHY_DBG_CNT_LDPC_ERR:
			phyData = phy_read(phydev, PHY_MMD_VEND2, 0xCE42);

            cnt = ((u64)(phyData & 0xFF));
            break;
        default:
            return 1;
    }
    *pCnt = cnt;
    return ret;
}

void rtk_ne_rtl8261_dbg_dump(struct phy_device *phydev)
{
	rtk_port_linkStatus_t phyLinkStatus, sdsLinkSatus;
	rtk_port_speed_t phySpeed;
	rtk_port_duplex_t phyDuplex;
	rtk_serdesMode_t sdsMode;
	u64 cnt = 0;

	static const char *speed_info[] = {
		"10Mbps",
		"100Mbps",
		"1Gbps",
		"500Mbpd",
		"2Gbps",   /* Applicable to 8380 */
		"2.5Gbps",
		"5Gbps",
		"10Gbps",
		"2.5Gbps Lite",
		"5Gbps Lite",
		"10Gbos Lite",
		NULL
	};
	static const char *duplex_info[] = {
		"HALF",
		"FULL",
		NULL
	};

	printf("RTL8261 info:\n");
	rtk_ne_rtl8261_linkStatus_get(phydev, &phyLinkStatus);
	if (phyLinkStatus == PORT_LINKUP) {
		if (rtk_ne_rtl8261_c45_speedDuplexStatusResReg_get(phydev, &phySpeed, &phyDuplex) == 0) {
			printf("	Speed: %s\n", speed_info[phySpeed]);
			printf("	Duplex: %s\n", duplex_info[phyDuplex]);
		}
	} else {
		printf("	Speed: N/A\n");
		printf("	Duplex: N/A\n");
	}
	if (rtk_ne_rtl8261_macIntfSerdesMode_get(phydev, &sdsMode) == 0) {
		printf("	Serdes mode: %s\n", RTK_NE_RT_MII_SERDES_MODE_STR(sdsMode));
	}
	if (rtk_ne_rtl8261_macIntfSerdesLinkStatus_get(phydev, &sdsLinkSatus) == 0) {					
		printf("	Serdes Link: %s\n", (sdsLinkSatus==PORT_LINKUP)?"UP":"DOWN");
	}
	if (rtk_ne_rtl8261_dbgCounter_get(phydev, PHY_DBG_CNT_RX, &cnt) == 0) {
		printf("	RX: 0x%016llx\n", cnt);
	}
	if (rtk_ne_rtl8261_dbgCounter_get(phydev, PHY_DBG_CNT_RX_ERR, &cnt) == 0) {
		printf("	RX_ERR: 0x%016llx\n", cnt);
	}
	if (rtk_ne_rtl8261_dbgCounter_get(phydev, PHY_DBG_CNT_RX_CRCERR, &cnt) == 0) {
		printf("	RX_CRCERR: 0x%016llx\n", cnt);
	}
	if (rtk_ne_rtl8261_dbgCounter_get(phydev, PHY_DBG_CNT_LDPC_ERR, &cnt) == 0) {
		printf("	LDPC_ERR: 0x%016llx\n", cnt);
	}
}

/* Implement rtk_port_phyCtrl_set(RTK_PHY_CTRL_SERDES_RX_RESET) for phy_rtl826xb */
u32 rtk_ne_rtl8261_phy_sds_rx_reset(struct phy_device *phydev)
{
	printf("%s(%d): phy_addr = %d\n", __FUNCTION__, __LINE__, phydev->addr);

	/* Hard Code Setting for 8261N */
	printf(" - Reset sds rx for RTL8261N\n");
	phy_write(phydev, PHY_MMD_VEND1, 0x141, 0x10);
	phy_write(phydev, PHY_MMD_VEND1, 0x143, 0x8820);
	phy_write(phydev, PHY_MMD_VEND1, 0x141, 0x30);
	phy_write(phydev, PHY_MMD_VEND1, 0x143, 0x8820);

	return 0;
}

void rtk_ne_rtl8261_phy_init(struct phy_device *phydev, int defSdsMode)
{
	u32 data;

	printf("Setting RTL8261(I/N)/RTL8251L\n");
	if (defSdsMode == 0)
	{
		printf(" - Set serdes mode as RTK_PHY_CTRL_SERDES_MODE_USXGMII\n");
		rtk_ne_rtl8261_serdes_mode_usxgmii_set(phydev);
	}
	else if (defSdsMode == 1)
	{
		printf(" - Set serdes mode as RTK_PHY_CTRL_SERDES_MODE_SGMII\n");
		rtk_ne_rtl8261_serdes_mode_sgmii_set(phydev);
	}

	/* Patch for port 6 on luna G3 Series */
	printf(" - Set USXGMII CFW as 0xaa\n");
	data = phy_read(phydev, PHY_MMD_VEND1, 0x141);
	data = 0x80aa;
	phy_write(phydev, PHY_MMD_VEND1, 0x141, data);

	data = phy_read(phydev, PHY_MMD_VEND1, 0x143);
	data = 0x8C07;
	phy_write(phydev, PHY_MMD_VEND1, 0x143, data);

	printf(" - Set TX Polarity as 1 for RTL8261N\n");
	data = phy_read(phydev, PHY_MMD_VEND1, 0xC1);
    data &= (~BIT_7);
    data |= (BIT_7);
	phy_write(phydev, PHY_MMD_VEND1, 0xC1, data);

	return;
}

