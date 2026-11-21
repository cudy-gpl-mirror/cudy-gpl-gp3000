#ifndef _FT2_H
#define _FT2_H

#define NONE_PORT               0xfe
#define INVALID_PORT            0xff

typedef enum {
    FT2_PARAM_GPHY_K = 0,
    FT2_PARAM_PRBS_PON,
    FT2_PARAM_PRBS_PON_RESULT,
    FT2_PARAM_PON_BEN,
    FT2_PARAM_PCIE,
    FT2_PARAM_USB,
    FT2_PARAM_CPU,
    FT2_PARAM_RISCV,
    FT2_PARAM_GPIO,
    FT2_PARAM_DBG,
    FT2_PARAM_MAX
} ft2_param_type_t;

#define MODULE_MAX_ENTRY    8
typedef struct ft2_record_s {
    char *name;
    void *ptr_data;
    uint32_t module_rlt;
    uint32_t entry_mask;
    char *entry_name[MODULE_MAX_ENTRY];
    uint32_t entry_test[MODULE_MAX_ENTRY];
    uint32_t entry_rlt[MODULE_MAX_ENTRY];

    unsigned long start;
    unsigned long finish; /* msecs */

    uint32_t data;
} ft2_record_t;

typedef enum {
    FT2_ERROR       = -1,
    FT2_TIMEOUT     = -2,
    FT2_PRBS_PON    = -3,
    FT2_PON_BEN     = -4,
    FT2_PCIE        = -5,
    FT2_USB         = -6,
    FT2_CPU         = -7,
    FT2_RISCV       = -8,
    FT2_GPIO        = -9,
    FT2_OK          = 0
} ft2_ret_t;

typedef struct serdes_cfg_s {
    uint32_t addr;
    uint32_t value;
} serdes_cfg_t;

typedef enum {
    GLD_FIX_DEV = 0x51,
    GLD_A_PROGRAM_DEV = 0x54,
    GLD_B_PROGRAM_DEV = 0x56,
    GLD_C_PROGRAM_DEV = 0x58,
} golden_addr_t;

typedef enum {
    /* EPON RX 10.3125G/TX 10.3125G; XFI0 10.3125G; XFI1 10.3125G */
    CFG_SERDES_MOD_1 = 1,
    /* GPON RX 9.98325G/TX 9.98325G; XFI0 3.125G; XFI1 3.125G */
    CFG_SERDES_MOD_2 = 2,
    /* PON/XFI serdes 3.125G mode */
    CFG_SERDES_MOD_3 = 3,
    CFG_PON_MODE = 5,
    CFG_XFI_MODE = 6,
    CFG_PRBS_START = 0x10,
    CFG_PRBS_STOP = 0x11,
    CFG_LINK_START = 0x20
} golden_cfg_t;

typedef enum {
    STATUS_READY = 0x80,
    STATUS_FAIL = 0x81,
    STATUS_PASS = 0x82,
} golden_status_t;

typedef enum {
    PHY_CHANNEL_A = 0,
    PHY_CHANNEL_B = 1,
    PHY_CHANNEL_C = 2,
    PHY_CHANNEL_D = 3,
    PHY_CHANNEL_MAX
} phy_channel_t;

typedef enum {
    CA_PORT_ID_NI0 = 0,     /* 1 Gbps NI Port (Integrated GPHY) */
    CA_PORT_ID_NI1 = 1,     /* 1 Gbps NI Port (Integrated GPHY) */
    CA_PORT_ID_NI2 = 2,     /* 1 Gbps NI Port (Integrated GPHY) */
    CA_PORT_ID_NI3 = 3,     /* 1 Gbps NI Port (Integrated GPHY) */
    CA_PORT_ID_NI4 = 4,     /* 1 Gbps NI Port (RGMII)*/
    CA_PORT_ID_NI5 = 5,     /* 10 Gbps XFI NI Port (RXAUI/2.5GSGMII/RGMII) */
    CA_PORT_ID_NI6 = 6,     /* 10 Gbps XFI NI Port (LAN) */
    CA_PORT_ID_NI7 = 7,     /* 10 Gbps XFI NI Port (WAN) */
} ca_port_index_t;

/* Ethernet Rx MIB item enumeration */
typedef enum {
    __NI_MIB_RX_UC_PKT_CNT              = 0x0 ,
    __NI_MIB_RX_MC_FRM_CNT              = 0x1 ,
    __NI_MIB_RX_BC_FRM_CNT              = 0x2 ,
    __NI_MIB_RX_OAM_FRM_CNT             = 0x3 ,
    __NI_MIB_RX_JUMBO_FRM_CNT           = 0x4 ,
    __NI_MIB_RX_PAUSE_FRM_CNT           = 0x5 ,
    __NI_MIB_RX_PFC_FRM_CNT             = 0x6 ,
    __NI_MIB_RX_UNK_OPCODE_FRM_CNT      = 0x7 ,
    __NI_MIB_RX_CRC_ERR_FRM_CNT         = 0x8 ,
    __NI_MIB_RX_UNDER_SIZE_FRM_CNT      = 0x9 ,
    __NI_MIB_RX_RUNT_FRM_CNT            = 0xA ,
    __NI_MIB_RX_OVSIZE_FRM_CNT          = 0xB ,
    __NI_MIB_RX_JABBER_FRM_CNT          = 0xC ,
    __NI_MIB_RX_INVALID_FRM_CNT         = 0xD ,
    __NI_MIB_RX_INVALID_FLOW_FRM_CNT    = 0xE ,
    __NI_MIB_RX_STATS_FRM_64_OCT        = 0xF ,
    __NI_MIB_RX_STATS_FRM_65_TO_127_OCT    = 0x10 ,
    __NI_MIB_RX_STATS_FRM_128_TO_255_OCT   = 0x11 ,
    __NI_MIB_RX_STATS_FRM_256_TO_511_OCT   = 0x12,
    __NI_MIB_RX_STATS_FRM_512_TO_1023_OCT  = 0x13,
    __NI_MIB_RX_STATS_FRM_1024_TO_1518_OCT = 0x14,
    __NI_MIB_RX_STATS_FRM_1519_TO_2100_OCT = 0x15,
    __NI_MIB_RX_STATS_FRM_2101_TO_9200_OCT = 0x16,
    __NI_MIB_RX_STATS_FRM_9201_TO_MAX_OCT  = 0x17,
    __NI_MIB_RX_BYTE_COUNT_LO              = 0x18,
    __NI_MIB_RX_BYTE_COUNT_HI              = 0x19,
    __NI_MIB_RX_NUM
} __ni_rx_mib_id_t;


/* Ethernet Tx MIB item enumeration */
typedef enum {
    __NI_MIB_TX_UC_PKT_CNT                 = 0x0 ,
    __NI_MIB_TX_MC_FRM_CNT                 = 0x1 ,
    __NI_MIB_TX_BC_FRM_CNT                 = 0x2 ,
    __NI_MIB_TX_OAM_FRM_CNT                = 0x3 ,
    __NI_MIB_TX_JUMBO_FRM_CNT              = 0x4 ,
    __NI_MIB_TX_PAUSE_FRM_CNT              = 0x5 ,
    __NI_MIB_TX_PFC_FRM_CNT                = 0x6 ,
    __NI_MIB_TX_CRCERR_FRM_CNT             = 0x7 ,
    __NI_MIB_TX_OVSIZE_FRM_CNT             = 0x8 ,
    __NI_MIB_TX_SINGLE_COL_FRM_CNT         = 0x9 ,
    __NI_MIB_TX_MULTI_COL_FRM_CNT          = 0xA ,
    __NI_MIB_TX_LATE_COL_FRM_CNT           = 0xB ,
    __NI_MIB_TX_RSVD_12, /* 0xC is reserved */
    __NI_MIB_TX_EXESS_COL_FRM_CNT          = 0xD ,
    __NI_MIB_TX_STATS_FRM_64_OCT           = 0xE ,
    __NI_MIB_TX_STATS_FRM_65_TO_127_OCT    = 0xF ,
    __NI_MIB_TX_STATS_FRM_128_TO_255_OCT   = 0x10 ,
    __NI_MIB_TX_STATS_FRM_256_TO_511_OCT   = 0x11,
    __NI_MIB_TX_STATS_FRM_512_TO_1023_OCT  = 0x12,
    __NI_MIB_TX_STATS_FRM_1024_TO_1518_OCT = 0x13,
    __NI_MIB_TX_STATS_FRM_1519_TO_2100_OCT = 0x14,
    __NI_MIB_TX_STATS_FRM_2101_TO_9200_OCT = 0x15,
    __NI_MIB_TX_STATS_FRM_9201_TO_MAX_OCT  = 0x16,
    __NI_MIB_TX_RSVD_23 = 0x17,/* 0x17 is reserved */
    __NI_MIB_TX_BYTE_CNT_LO                = 0x18,
    __NI_MIB_TX_BYTE_CNT_HI                = 0x19,
    __NI_MIB_TX_NUM
} __ni_tx_mib_id_t;


#endif
