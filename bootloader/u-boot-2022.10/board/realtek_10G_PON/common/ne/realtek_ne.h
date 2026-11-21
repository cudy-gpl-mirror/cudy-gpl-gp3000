#ifndef __REALTEK_NE_H__
#define __REALTEK_NE_H__
#include <net.h>
#include <osal_type.h>

#define REALTEK_NE_DBG 1
#undef REALTEK_NE_DBG

#define PHY_ID_INTERNAL 0x001ccac4
#define PHY_ID_RTL8201 0x001cc810
#define PHY_ID_RTL8211 0x001cc910
#define PHY_ID_RTL8214 0x001cc940
#define PHY_ID_RTL8211_G3_ASIC 0x001cc980
#define PHY_ID_RTL8211_SATURN_ASIC 0x001cc900
#define PHY_ID_QCA8337 0x004dd035
#define PHY_ID_RTL8224 0x001ccad0
#define PHY_ID_RTL8261 0x001ccaf0
#define PHY_ID_RTL8221B 0x001CC840

#define PHY_ID_MASK 0xFFFFFFF0

#define GE_MAC_INTF_GMII 0x0
#define GE_MAC_INTF_MII 0x1
#define GE_MAC_INTF_RGMII_1000 0x2
#define GE_MAC_INTF_RGMII_100 0x3
#define GE_MAC_INTF_QSGMII_1000 0x4
#define GE_MAC_INTF_RMII 0x5

#if defined(CONFIG_TARGET_SATURN_FPGA) || defined(CONFIG_TARGET_SATURN_ASIC)
#define NE_READ_POLL_COUNT 1000000
#else
#define NE_READ_POLL_COUNT 1000
#endif

typedef enum {
	NE_PORT_0 = 0,
	NE_PORT_1,
	NE_PORT_2,
	NE_PORT_3,
	NE_PORT_4,
	NE_PORT_5,
	NE_PORT_6,
	NE_PORT_7,
	NE_PORT_8
} RTK_NE_PORT_ID_T;

#define MAX_LAN_PORT_NUM	10
struct lan_port_table_entry {
	int dport;
	int phyAddr;
	uint8 serdesMode;
	struct phy_device *phydev;
	uint8 is_phyinited;
};

#define MAX_LUT_ENTRY_NUM	32
struct lut_table_entry {
	unsigned int valid;
	unsigned char mac[ARP_HLEN];
	unsigned int sport;
	unsigned long updateTime;
};

struct sw_cnt_entry {
	uint64 rx_uc_pkts;
	uint64 rx_mc_pkts;
	uint64 rx_bc_pkts;
	uint64 tx_uc_pkts;
	uint64 tx_mc_pkts;
	uint64 tx_bc_pkts;
	uint64 tx_lookup_miss_pkts;
};

struct realtek_ne_priv {
	struct mii_dev *bus_int;
	struct lan_port_table_entry lan_port_table[MAX_LAN_PORT_NUM];
	struct lut_table_entry lut_table[MAX_LUT_ENTRY_NUM];
	uint8 is_intphy_global_config_done;
	struct sw_cnt_entry sw_cnt;
#ifdef CONFIG_RTK_NI_SUPPORT_RTL8221B
	uint8 is_8221b_detect;
	uint8 is_8221b_global_config_done;
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_RTL8224B
	uint8 is_8224b_detect;
	uint8 is_8224b_global_config_done;
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_RTL8261I
	uint8 is_8261i_detect;
	uint8 is_8261i_global_config_done;
#endif
#if defined(CONFIG_RTK_NI_SUPPORT_HSGMII_P4) || defined(CONFIG_RTK_NI_SUPPORT_HSGMII_P6) || defined(CONFIG_RTK_NE_SUPPORT_HSGMII_P7)
	uint8 is_hsgmii_need;
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_QXGMII_P6
	uint8 is_qxgmii_need;
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_USXGMII_P6
	uint8 is_usxgmii_need;
#endif
};

typedef enum {
	NE_SERDES_MODE_NONE = 0,
	NE_SERDES_MODE_SGMII,
	NE_SERDES_MODE_FIBER,
	NE_SERDES_MODE_XFI,
	NE_SERDES_MODE_USXGMII,
	NE_SERDES_MODE_RXAUI,
	NE_SERDES_MODE_RGMII,
	NE_SERDES_MODE_QXGMII,
	NE_SERDES_MODE_HSGMII,
	NE_SERDES_MODE_END
} RTK_NE_SERDES_MODE_T;

#define RTK_NE_SERDES_MODE_STR(x) (\
(x)==NE_SERDES_MODE_NONE ? "NONE":\
(x)==NE_SERDES_MODE_SGMII ? "SGMII":\
(x)==NE_SERDES_MODE_FIBER ? "FIBER":\
(x)==NE_SERDES_MODE_XFI ? "XFI":\
(x)==NE_SERDES_MODE_USXGMII ? "USXGMII":\
(x)==NE_SERDES_MODE_RXAUI?"RXAUI":\
(x)==NE_SERDES_MODE_RGMII?"RGMII":\
(x)==NE_SERDES_MODE_QXGMII?"QXGMII":\
(x)==NE_SERDES_MODE_HSGMII?"HSGMII":\
"unknown")

uint32 rtk_ne_max_lan_port_num_get(void);

#ifdef REALTEK_NE_DBG
typedef enum
{
    __NE_PORT_MIB_RX_UC_PKT_CNT = 0,
    __NE_PORT_MIB_RX_MC_FRAME_CNT,
    __NE_PORT_MIB_RX_BC_FRAME_CNT,
    __NE_PORT_MIB_RX_OAM_FRAME_CNT,
    __NE_PORT_MIB_RX_JUMBO_FRAME_CNT,
    __NE_PORT_MIB_RX_PAUSE_FRAME_CNT,
    __NE_PORT_MIB_RX_PFC_FRAME_CNT,
    __NE_PORT_MIB_RX_UNK_OPCODE_FRAME_CNT,
    __NE_PORT_MIB_RX_CRC_ERROR_FRAME_CNT,
    __NE_PORT_MIB_RX_UNDERSIZE_FRAME_CNT,
    __NE_PORT_MIB_RX_RUNT_FRAME_CNT,
    __NE_PORT_MIB_RX_OVSIZE_FRAME_CNT,
    __NE_PORT_MIB_RX_JABBER_FRAME_CNT,
    __NE_PORT_MIB_RX_INVALID_FRAME_CNT,
    __NE_PORT_MIB_RX_INVALID_FLOW_FRAME_CNT,
    __NE_PORT_MIB_RX_FRAME_64_CNT,
    __NE_PORT_MIB_RX_FRAME_65TO127_CNT,
    __NE_PORT_MIB_RX_FRAME_128TO255_CNT,
    __NE_PORT_MIB_RX_FRAME_256TO511_CNT,
    __NE_PORT_MIB_RX_FRAME_512TO1023_CNT,
    __NE_PORT_MIB_RX_FRAME_1024TO1518_CNT,
    __NE_PORT_MIB_RX_FRAME_1519TO2100_CNT,
    __NE_PORT_MIB_RX_FRAME_2101TO9200_CNT,
    __NE_PORT_MIB_RX_FRAME_9201TOMAX_CNT,
    __NE_PORT_MIB_RX_BYTE_CNT,
    __NE_PORT_MIB_RX_ALL_BYTE_CNT,
    __NE_PORT_MIB_TX_UC_PKT_CNT,
    __NE_PORT_MIB_TX_MC_FRAME_CNT,
    __NE_PORT_MIB_TX_BC_FRAME_CNT,
    __NE_PORT_MIB_TX_OAM_FRAME_CNT,
    __NE_PORT_MIB_TX_JUMBO_FRAME_CNT,
    __NE_PORT_MIB_TX_PAUSE_FRAME_CNT,
    __NE_PORT_MIB_TX_PFC_FRAME_CNT,
    __NE_PORT_MIB_TX_CRC_ERROR_FRAME_CNT,
    __NE_PORT_MIB_TX_OVSIZE_FRAME_CNT,
    __NE_PORT_MIB_TX_SINGLE_COL_FRAME_CNT,
    __NE_PORT_MIB_TX_MULTI_COL_FRAME_CNT,
    __NE_PORT_MIB_TX_LATE_COL_FRAME_CNT,
    __NE_PORT_MIB_TX_EXESS_COL_FRAME_CNT,
    __NE_PORT_MIB_TX_FRAME_64_CNT,
    __NE_PORT_MIB_TX_FRAME_65TO127_CNT,
    __NE_PORT_MIB_TX_FRAME_128TO255_CNT,
    __NE_PORT_MIB_TX_FRAME_256TO511_CNT,
    __NE_PORT_MIB_TX_FRAME_512TO1023_CNT,
    __NE_PORT_MIB_TX_FRAME_1024TO1518_CNT,
    __NE_PORT_MIB_TX_FRAME_1519TO2100_CNT,
    __NE_PORT_MIB_TX_FRAME_2101TO9200_CNT,
    __NE_PORT_MIB_TX_FRAME_9201TOMAX_CNT,
    __NE_PORT_MIB_TX_BYTE_CNT,
    __NE_PORT_MIB_TX_ALL_BYTE_CNT,
    __NE_PORT_MIB_DROP_RX_CRC_ERR_FRAME_CNT,
    __NE_PORT_MIB_DROP_RX_ALIGN_ERR_FRAME_CNT,
    __NE_PORT_MIB_RX_ERR_FRAME_CNT,
    __NE_PORT_MIB_TX_ERR_FRAME_CNT,
    __NE_PORT_MIB_RX_ALIGN_ERR_FRAME_CNT,
    __NE_PORT_MIB_RX_SHORT_FRAME_CNT,
    __NE_PORT_MIB_RX_GIANT_FRAME_CNT,
    __NE_PORT_MIB_NUM
} __ne_port_mib_id_t;

static const char *ne_port_mib_strings[__NE_PORT_MIB_NUM] = {
    "rxUcPktCnt",
    "rxMcFrameCnt",
    "rxBcFrameCnt",
    "rxOamFrameCnt",
    "rxJumboFrameCnt",
    "rxPauseFrameCnt",
    "rxPfcFrameCnt",
    "rxUnkOpcodeFrameCnt",
    "rxCrcErrorFrameCnt",
    "rxUndersizeFrameCnt",
    "rxRuntFrameCnt",
    "rxOvsizeFrameCnt",
    "rxJabberFrameCnt",
    "rxInvalidFrameCnt",
    "rxInvalidFlowFrameCnt",
    "rxFrame64Cnt",
    "rxFrame65to127Cnt",
    "rxFrame128to255Cnt",
    "rxFrame256to511Cnt",
    "rxFrame512to1023Cnt",
    "rxFrame1024to1518Cnt",
    "rxFrame1519to2100Cnt",
    "rxFrame2101to9200Cnt",
    "rxFrame9201tomaxCnt",
    "rxByteCnt",
    "rxAllByteCnt",
    "txUcPktCnt",
    "txMcFrameCnt",
    "txBcFrameCnt",
    "txOamFrameCnt",
    "txJumboFrameCnt",
    "txPauseFrameCnt",
    "txPfcFrameCnt",
    "txCrcErrorFrameCnt",
    "txOvsizeFrameCnt",
    "txSingleColFrameCnt",
    "txMultiColFrameCnt",
    "txLateColFrameCnt",
    "txExessColFrameCnt",
    "txFrame64Cnt",
    "txFrame65to127Cnt",
    "txFrame128to255Cnt",
    "txFrame256to511Cnt",
    "txFrame512to1023Cnt",
    "txFrame1024to1518Cnt",
    "txFrame1519to2100Cnt",
    "txFrame2101to9200Cnt",
    "txFrame9201tomaxCnt",
    "txByteCnt",
    "txAllByteCnt",
    "dropRxCrcErrFrameCnt",
    "dropRxAlignErrFrameCnt",
    "rxErrFrameCnt",
    "txErrFrameCnt",
    "rxAlignErrFrameCnt",
    "rxShortFrameCnt",
    "rxGiantFrameCnt"
};
#endif

#if defined(CONFIG_RTK_NE_SUPPORT_HSGMII_P7) && defined(CONFIG_RTK_NI_SUPPORT_RTL8221B)
#define PORT7_8221B_PHY_ID		6
#endif

#endif /* __REALTEK_NE_H__ */
