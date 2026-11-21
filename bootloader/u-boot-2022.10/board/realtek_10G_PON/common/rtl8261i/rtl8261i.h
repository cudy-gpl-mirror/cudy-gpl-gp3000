#ifndef __RL8261I_H__
#define __RL8261I_H__
#define REG32_FIELD_SET(_data, _val, _fOffset, _fMask)      ((_data & ~(_fMask)) | ((_val << (_fOffset)) & (_fMask)))
#define REG32_FIELD_GET(_data, _fOffset, _fMask)            ((_data & (_fMask)) >> (_fOffset))
/* convert the range of bits [_mBit, _lBit] to mask */
#define UINT32_BITS_MASK(_mBit, _lBit)                      ((0xFFFFFFFF >> (31 - _mBit)) ^ ((1 << _lBit) - 1))

#define PHY_MMD_PMAPMD              1
#define PHY_MMD_PCS                 3
#define PHY_MMD_AN                  7
#define PHY_MMD_VEND1               30   /* Vendor specific 1 */
#define PHY_MMD_VEND2               31   /* Vendor specific 2 */

#define BIT_0 (1<<0)
#define BIT_1 (1<<1)
#define BIT_2 (1<<2)
#define BIT_3 (1<<3)
#define BIT_4 (1<<4)
#define BIT_5 (1<<5)
#define BIT_6 (1<<6)
#define BIT_7 (1<<7)
#define BIT_8 (1<<8)
#define BIT_9 (1<<9)
#define BIT_10 (1<<10)
#define BIT_11 (1<<11)
#define BIT_12 (1<<12)
#define BIT_13 (1<<13)
#define BIT_14 (1<<14)
#define BIT_15 (1<<15)

#define PHY_826X_SERDES_MAX_RETRY                   10

/* value for RTK_PHY_CTRL_SERDES_MODE */
#define RTK_PHY_CTRL_SERDES_MODE_OFF                                  0x00FF  /* POWER OFF */
#define RTK_PHY_CTRL_SERDES_MODE_USXGMII                              0x0000  /* USXGMII */
#define RTK_PHY_CTRL_SERDES_MODE_10GR_5GX_2P5GX_SGMII                 0x1001  /* 10G-R/5G-X/2.5G-X/SGMII */
#define RTK_PHY_CTRL_SERDES_MODE_10GR_XFI5GADAPT_XFI2P5GADAPT_SGMII   0x1002  /* 10G-R/XFI-5G-ADAPT/XFI-2.5G-ADAPT/SGMII */
#define RTK_PHY_CTRL_SERDES_MODE_10GR_5GR_2P5GX_SGMII                 0x1101  /* 10G-R/5G-R/2.5G-X/SGMII */
#define RTK_PHY_CTRL_SERDES_MODE_10GR_XFI5GCPRI_2P5GX_SGMII           0x1102  /* 10G-R/XFI-5G-CPRI/2.5G-X/SGMII */
#define RTK_PHY_CTRL_SERDES_MODE_UNKNOWN                              0xFFFF
#define RTK_PHY_CTRL_SERDES_MODE_IS_AUTO(_mode)             ((_mode & 0x0100) == 0x0)

#define MACSEC_REG_SAM_IN_FLIGHT      (0x6104)
#define MACSEC_REG_COUNT_CTRL         (0xC810)
#define MACSEC_REG_IG_CC_CONTROL      (0xE840)
#define MACSEC_REG_COUNT_SECFAIL1     (0xF124)
#define MACSEC_REG_CTX_CTRL           (0xF408)
#define MACSEC_REG_CTX_UPD_CTRL       (0xF430)
#define MACSEC_REG_SAM_CP_TAG         (0x7900)
#define MACSEC_REG_SAM_NM_PARAMS      (0x7940)
#define MACSEC_REG_SAM_NM_FLOW_NCP        (0x7944)
#define MACSEC_REG_SAM_NM_FLOW_CP         (0x7948)

#define MACSEC_REG_MISC_CONTROL       (0x797C)

/* configuration mode type */
typedef enum rtk_enable_e
{
    DISABLED = 0,
    ENABLED,
    RTK_ENABLE_END
} rtk_enable_t;

typedef enum rtk_port_linkStatus_e
{
    PORT_LINKDOWN = 0,
    PORT_LINKUP,
    PORT_LINKSTATUS_END
} rtk_port_linkStatus_t;

typedef enum rtk_port_speed_e
{
    PORT_SPEED_10M = 0,
    PORT_SPEED_100M,
    PORT_SPEED_1000M,
    PORT_SPEED_500M,
    PORT_SPEED_2G,   /* Applicable to 8380 */
    PORT_SPEED_2_5G,
    PORT_SPEED_5G,
    PORT_SPEED_10G,
    PORT_SPEED_2_5G_LITE,
    PORT_SPEED_5G_LITE,
    PORT_SPEED_10G_LITE,
    PORT_SPEED_END
} rtk_port_speed_t;

typedef enum rtk_port_duplex_e
{
    PORT_HALF_DUPLEX = 0,
    PORT_FULL_DUPLEX,
    PORT_DUPLEX_END
} rtk_port_duplex_t;

typedef enum rt_serdesMode_e
{
    RTK_MII_NONE = 0,
    RTK_MII_DISABLE,
    RTK_MII_10GR,
    RTK_MII_RXAUI,
    RTK_MII_RXAUI_LITE,
    RTK_MII_RXAUISGMII_AUTO,
    RTK_MII_RXAUI1000BX_AUTO,
    RTK_MII_RSGMII_PLUS,
    RTK_MII_SGMII,
    RTK_MII_QSGMII,
    RTK_MII_1000BX_FIBER,
    RTK_MII_100BX_FIBER,
    RTK_MII_1000BX100BX_AUTO,
    RTK_MII_10GR1000BX_AUTO,
    RTK_MII_10GRSGMII_AUTO,
    RTK_MII_XAUI,
    RTK_MII_RMII,
    RTK_MII_SMII,
    RTK_MII_SSSMII,
    RTK_MII_RSGMII,
    RTK_MII_XSMII,
    RTK_MII_RS8MII = RTK_MII_XSMII,     /* RTK_MII_RS8MII is obsoleted */
    RTK_MII_XSGMII,
    RTK_MII_QHSGMII,
    RTK_MII_HISGMII,
    RTK_MII_HISGMII_5G,
    RTK_MII_DUAL_HISGMII,
    RTK_MII_2500Base_X,
    RTK_MII_RXAUI_PLUS,
    RTK_MII_USXGMII_10GSXGMII,
    RTK_MII_USXGMII_10GDXGMII,
    RTK_MII_USXGMII_10GQXGMII,
    RTK_MII_USXGMII_5GSXGMII,
    RTK_MII_USXGMII_5GDXGMII,
    RTK_MII_USXGMII_2_5GSXGMII,
    RTK_MII_USXGMII_1G,
    RTK_MII_USXGMII_100M,
    RTK_MII_USXGMII_10M,
    RTK_MII_5GBASEX,
    RTK_MII_5GR,
    RTK_MII_XFI_5G_ADAPT,
    RTK_MII_XFI_5G_CPRI,
    RTK_MII_XFI_2P5G_ADAPT,
    RTK_MII_QUSGMII,
    RTK_MII_OUSGMII,
    RTK_MII_END,
}rt_serdesMode_t;
#define rtk_serdesMode_t            rt_serdesMode_t

#define RTK_NE_RT_MII_SERDES_MODE_STR(x) (\
(x)==RTK_MII_NONE ? "NONE":\
(x)==RTK_MII_DISABLE ? "DISABLE":\
(x)==RTK_MII_10GR ? "10GR":\
(x)==RTK_MII_RXAUI ? "RXAUI":\
(x)==RTK_MII_RXAUI_LITE ? "RXAUI_LITE":\
(x)==RTK_MII_RXAUISGMII_AUTO?"RXAUISGMII_AUTO":\
(x)==RTK_MII_RXAUI1000BX_AUTO?"RXAUI1000BX_AUTO":\
(x)==RTK_MII_SGMII?"SGMII":\
(x)==RTK_MII_QSGMII?"QSGMII":\
(x)==RTK_MII_1000BX_FIBER?"1000BX_FIBER":\
(x)==RTK_MII_100BX_FIBER?"100BX_FIBER":\
(x)==RTK_MII_1000BX100BX_AUTO?"1000BX100BX_AUTO":\
(x)==RTK_MII_10GR1000BX_AUTO?"10GR1000BX_AUTO":\
(x)==RTK_MII_10GRSGMII_AUTO?"10GRSGMII_AUTO":\
(x)==RTK_MII_XAUI?"XAUI":\
(x)==RTK_MII_RMII?"RMII":\
(x)==RTK_MII_SMII?"SMII":\
(x)==RTK_MII_SSSMII?"SSSMII":\
(x)==RTK_MII_RSGMII?"RSGMII":\
(x)==RTK_MII_XSMII?"XSMII":\
(x)==RTK_MII_RS8MII?"RS8MII":\
(x)==RTK_MII_QHSGMII?"QHSGMII":\
(x)==RTK_MII_HISGMII?"HISGMII":\
(x)==RTK_MII_HISGMII_5G?"HISGMII_5G":\
(x)==RTK_MII_DUAL_HISGMII?"DUAL_HISGMII":\
(x)==RTK_MII_2500Base_X?"2500Base_X":\
(x)==RTK_MII_RXAUI_PLUS?"RXAUI_PLUS":\
(x)==RTK_MII_USXGMII_10GSXGMII?"USXGMII_10GSXGMII":\
(x)==RTK_MII_USXGMII_10GDXGMII?"USXGMII_10GDXGMII":\
(x)==RTK_MII_USXGMII_10GQXGMII?"USXGMII_10GQXGMII":\
(x)==RTK_MII_USXGMII_5GSXGMII?"USXGMII_5GSXGMII":\
(x)==RTK_MII_USXGMII_5GDXGMII?"USXGMII_5GDXGMII":\
(x)==RTK_MII_USXGMII_2_5GSXGMII?"USXGMII_2_5GSXGMII":\
(x)==RTK_MII_USXGMII_1G?"USXGMII_1G":\
(x)==RTK_MII_USXGMII_100M?"USXGMII_100M":\
(x)==RTK_MII_USXGMII_10M?"USXGMII_10M":\
(x)==RTK_MII_5GBASEX?"5GBASEX":\
(x)==RTK_MII_5GR?"5GR":\
(x)==RTK_MII_XFI_5G_ADAPT?"XFI_5G_ADAPT":\
(x)==RTK_MII_XFI_5G_CPRI?"XFI_5G_CPRI":\
(x)==RTK_MII_XFI_2P5G_ADAPT?"XFI_2P5G_ADAPT":\
(x)==RTK_MII_QUSGMII?"QUSGMII":\
(x)==RTK_MII_OUSGMII?"OUSGMII":\
"unknown")

typedef enum rtk_macsec_reg_e
{
    RTK_MACSEC_DIR_EGRESS = 0,
    RTK_MACSEC_DIR_INGRESS,
    RTK_MACSEC_DIR_END,
} rtk_macsec_dir_t;

typedef enum rtk_port_phy_dbg_cnt_e
{
    PHY_DBG_CNT_RX,
    PHY_DBG_CNT_RX_ERR,
    PHY_DBG_CNT_RX_CRCERR,
    PHY_DBG_CNT_LDPC_ERR,
    PHY_DBG_CNT_END
} rtk_port_phy_dbg_cnt_t;

u32 rtk_ne_rtl8261_sdsReg_write(struct phy_device *phydev, u32 page, u32 reg, u8 msb, u8 lsb, u32 data);
u32 rtk_ne_rtl8261_sdsReg_read(struct phy_device *phydev, u32 page, u32 reg, u8 msb, u8 lsb, u32 *pData);
u32 rtk_ne_rtl8261_phyReg_write(struct phy_device *phydev, u32 page, u32 reg, u8 msb, u8 lsb, u32 data);
u32 rtk_ne_rtl8261_phyReg_read(struct phy_device *phydev, u32 page, u32 reg, u8 msb, u8 lsb, u32 *pData);
u32 rtk_ne_rtl8261_macsec_reg_get(struct phy_device *phydev, rtk_macsec_dir_t dir, u32 reg, u32 *pData);
u32 rtk_ne_rtl8261_macsec_reg_set(struct phy_device *phydev, rtk_macsec_dir_t dir, u32 reg, u32 data);
u32 rtk_ne_rtl8261_c45_speedDuplexStatusResReg_get(struct phy_device *phydev, rtk_port_speed_t *pSpeed, rtk_port_duplex_t *pDuplex);
u32 rtk_ne_rtl8261_c45_speedStatusResReg_get(struct phy_device *phydev, rtk_port_speed_t *pSpeed);
u32 rtk_ne_rtl8261_c45_autoNegoEnable_set(struct phy_device *phydev, rtk_enable_t enable);

#endif /* __RL8261I_H__ */
