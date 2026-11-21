#ifndef __SERDES_COMMON_H__
#define __SERDES_COMMON_H__

#if defined(CONFIG_TARGET_SATURN_FPGA) || defined(CONFIG_TARGET_SATURN_ASIC)
#define SERDES_REG_READ(off)		CA_REG_READ(KSEG1_ATU_XLAT(off))
#define SERDES_REG_WRITE(data, off)         CA_REG_WRITE(data, KSEG1_ATU_XLAT(off))
#else
#define SERDES_REG_READ(off)                CA_REG_READ(off)
#define SERDES_REG_WRITE(data, off)         CA_REG_WRITE(data, off)
#endif

#define CA_NE_SGMII_LP_REG_READ(addr)		SERDES_REG_READ(addr)
#define CA_NE_SGMII_LP_REG_WRITE(data, addr)		SERDES_REG_WRITE(data, addr)

#define RTK_SDS_CHIP_REVISION_A	'A'
#define RTK_SDS_CHIP_REVISION_B	'B'
#define RTK_SDS_CHIP_REVISION_C 'C'
unsigned int __rtk_sds_get_chip_revision(void);

typedef enum {
    RTK_SDS_NAME_SGMII_1G = 0,
    RTK_SDS_NAME_SGMII_2G5 = 1,
    RTK_SDS_NAME_FIBER_100M = 2,
    RTK_SDS_NAME_FIBER_1G = 3,
    RTK_SDS_NAME_XFI_10G = 4,
    RTK_SDS_NAME_USXGMII_10G = 5,
    RTK_SDS_NAME_RXAUI_10G = 6,
    RTK_SDS_NAME_RGMII_1G = 7,
    RTK_SDS_NAME_RGMII_100M = 8,
    RTK_SDS_NAME_QXGMII_2G5 = 9,
    RTK_SDS_NAME_END
} rtk_sds_name_t;

#define RTK_SDS_SDS_NAME_STR(x) (\
(x)==RTK_SDS_NAME_SGMII_1G ? "SGMII_1G":\
(x)==RTK_SDS_NAME_SGMII_2G5 ? "SGMII_2G5":\
(x)==RTK_SDS_NAME_FIBER_100M ? "FIBER_100M":\
(x)==RTK_SDS_NAME_FIBER_1G ? "FIBER_1G":\
(x)==RTK_SDS_NAME_XFI_10G ? "XFI_10G":\
(x)==RTK_SDS_NAME_USXGMII_10G?"USXGMII_10G":\
(x)==RTK_SDS_NAME_RXAUI_10G?"RXAUI_10G":\
(x)==RTK_SDS_NAME_RGMII_1G?"RGMII_1G":\
(x)==RTK_SDS_NAME_RGMII_100M?"RGMII_100M":\
(x)==RTK_SDS_NAME_QXGMII_2G5?"QXGMII_2G5":\
"unknown")

typedef enum {
    RTK_SDS_MODE_NONE = 0,
    RTK_SDS_MODE_SGMII = 1,         /* For SGMII/1G, HiSGMII/2.5G */
    RTK_SDS_MODE_FIBER = 2,         /* For 100BASE-X, 1000BASE-X, 2500BASE-X */
    RTK_SDS_MODE_XFI = 3,           /* For XFI/10G */
    RTK_SDS_MODE_USXGMII = 4,       /* For USXGMII/10G */
    RTK_SDS_MODE_RXAUI = 5,         /* For RXAUI/10G */
    RTK_SDS_MODE_RGMII = 6,         /* For RGMII/1G/100M */
    RTK_SDS_MODE_QXGMII = 7,        /* For QXGMII/10M/100M/1G/2.5G */
    RTK_SDS_MODE_END
} rtk_sds_mode_t;

typedef enum {
	RTK_SDS_SPEED_DEFAULT = 0,      /* No speed specified */
	RTK_SDS_SPEED_100M = 1,
	RTK_SDS_SPEED_1G = 2,
	RTK_SDS_SPEED_2G5 = 3,
	RTK_SDS_SPEED_5G = 4,
	RTK_SDS_SPEED_10G = 5,
	RTK_SDS_SPEED_10M = 6,
	RTK_SDS_SPEED_500M = 7,
	RTK_SDS_SPEED_END
} rtk_sds_speed_t;

typedef struct rtk_sds_prof_s
{   
    u32 reg;
    u32 value;
} rtk_sds_prof_t;

typedef struct rtk_sds_patch_s {
    u32 reg;
    u32 data;
} rtk_sds_patch_t;

typedef struct {
    u8 signal_ok;
    u8 link_ok;
    u8 sync_ok;
    u8 ber_notify;
    u8 rx_idle;
    u8 ck_rdy;
    u8 blk_lock;
    u8 link_ok_10G;
    u8 link_ok_ch0;
    u8 link_ok_ch1;
    u8 link_ok_ch2;
    u8 link_ok_ch3;
    u8 link_ok_rxaui;
} rtk_sds_status_t;

#endif /* __SERDES_COMMON_H__ */

