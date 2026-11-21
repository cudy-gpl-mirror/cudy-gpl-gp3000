#ifndef NOT_SCFG_H_A2D1ECC7_2D11_4F3A_B82E_0AFF2AC6ABC9_
#define NOT_SCFG_H_A2D1ECC7_2D11_4F3A_B82E_0AFF2AC6ABC9_

typedef enum {
    SCFG_GPIO_INVALID   = 0,
    SCFG_GPIO_INPUT     = 1,
    SCFG_GPIO_OUTPUT    = 2,
    SCFG_GPIO_TRIGGER   = 3,
} ca_scfg_GpioPinType_t;

typedef enum {
    ONU_PON_MAC_EPON_1G   = 0,   /* 1G Down, 1G Up, EPON  */
    ONU_PON_MAC_EPON_D10G = 1,   /* 10G Down, 1G Up, EPON  */
    ONU_PON_MAC_EPON_BI10G= 2,   /* 10G Down, 10G Up, 10G-EPON */
    ONU_PON_MAC_GPON      = 3,   /* 2.5G Down, 1G Up, GPON */
    ONU_PON_MAC_XGPON1    = 4,   /* 10G Down, 2.5G Up, XG-PON1 */
    ONU_PON_MAC_XGSPON    = 5,   /* 10G Down, 10G Up, XGS-PON */
    ONU_PON_MAC_NGPON2    = 6,   /* 10G Down, 10G Up, NG-PON2 with multiple wavelength support*/
    ONU_PON_MAC_ETHERNET_10G    = 7,   /* 10G Ethernet */
    ONU_PON_MAC_TURBO_EPON      = 8,   /* 2G Down, 1G UP, EPON */
    ONU_PON_MAC_ETHERNET_1250M  = 9,   /* 1G Ethernet */
    ONU_PON_MAC_ETHERNET_3125M  = 10,  /* 2.5G Ethernet */
    ONU_PON_MAC_ETHERNET_SGMII_1G  = 11,  /* SGMII/1G */
    ONU_PON_MAC_ETHERNET_SGMII_2G5  = 12,  /* HiSGMII/2.5G */
    ONU_PON_MAC_ETHERNET_FIBER_100M  = 13,  /* 100BASE-X */
    ONU_PON_MAC_ETHERNET_FIBER_1G  = 14,  /* 1000BASE-X, */
    ONU_PON_MAC_ETHERNET_USXGMII_10G  = 15,  /* USXGMII/10G */
    ONU_PON_MAC_NGPON2_D10G    = 16,   /* 10G Down, 2.5GG Up, NG-PON2 with multiple wavelength support*/
    ONU_PON_MAC_ETHERNET_USXGMII_5G   = 17,  /* USXGMII/5G */
    ONU_PON_MAC_ETHERNET_USXGMII_2G5  = 18,  /* USXGMII/2G5 */
    ONU_PON_MAC_ETHERNET_USXGMII_1G   = 19,  /* USXGMII/1G */
    ONU_PON_MAC_ETHERNET_USXGMII_100M = 20,  /* USXGMII/100M */
    ONU_PON_MAC_ETHERNET_USXGMII_10M = 21,  /* USXGMII/10M */
    ONU_PON_MAC_ETHERNET_SGMII_100M  = 22,  /* SGMII/100M */
    ONU_PON_MAC_ETHERNET_SGMII_10M  = 23,  /* SGMII/10M */
    ONU_PON_MAC_ETHERNET_FIBER_2G5  = 24,  /* 2500BASE-X, */
    ONU_PON_MAC_MAX
} ca_scfg_pon_mac_mode_t;

#ifdef CONFIG_LUNA_G3_SERIES
#define ONU_PON_MAC_MODE_IGNORED 0xFF	/* used to skip PON Initialization during SCFG loading, may be initialized later */
#endif

#define CA_PON_MODE_IS_EPON_FAMILY(mode) (((mode) >= ONU_PON_MAC_EPON_1G && (mode) <= ONU_PON_MAC_EPON_BI10G) || ((mode) == ONU_PON_MAC_TURBO_EPON))
#define CA_PON_MODE_IS_GPON_FAMILY(mode) (((mode) >= ONU_PON_MAC_GPON && (mode) <= ONU_PON_MAC_NGPON2) ||((mode) == ONU_PON_MAC_NGPON2_D10G))
#define CA_PON_MODE_IS_ETHERNET(mode) ((mode) == ONU_PON_MAC_ETHERNET_10G  || ((mode) >= ONU_PON_MAC_ETHERNET_1250M && (mode) <= ONU_PON_MAC_ETHERNET_USXGMII_10G) || ((mode) >= ONU_PON_MAC_ETHERNET_USXGMII_5G && (mode) <= ONU_PON_MAC_ETHERNET_FIBER_2G5))

typedef enum {
    CUSTOMER_BIT_0   = 0x1,   /* DPOE  */
    CUSTOMER_BIT_1   = 0x2,   /* K  */
    CUSTOMER_BIT_2   = 0x4,   /* G C */
    CUSTOMER_BIT_12   = 0x1000,/* Minimal SFP + */
    CUSTOMER_BIT_13   = 0x2000,/* SFU */
    CUSTOMER_BIT_14   = 0x4000,/* HGU */
    CUSTOMER_BIT_16   = 0x10000,   /* U special OAM*/
    CUSTOMER_BIT_17   = 0x20000,   /*  VOIP   */
    CUSTOMER_BIT_18   = 0x40000,   /*  Iperf Offload  */
    CUSTOMER_BIT_ALL   = 0xFFFFFFFF
} ca_scfg_customer_bit_t;

typedef enum {
    BOARD_INFO_BIT_0   = 0x1,   /*SFU/SFP/HGU Engineer Board, Distinguished by Macro */
    BOARD_INFO_BIT_1   = 0x2,   /*C : SFU */
    BOARD_INFO_BIT_2   = 0x4,   /*C : SFP+ */
    BOARD_INFO_BIT_3   = 0x8,   /*U : K  */
    BOARD_INFO_BIT_4   = 0x10,  /*U : DPOE */
    BOARD_INFO_BIT_5   = 0x20,  /*H : K SFP  */
    BOARD_INFO_BIT_6   = 0x40,  /*P : SFP  */
} ca_scfg_board_info_bit_t;

typedef enum
{
    CA_OLT_TYPE_CALIX           = 0,
    CA_OLT_TYPE_ADTRAN          = 1,
    CA_OLT_TYPE_ZTE             = 2,
    CA_OLT_TYPE_SUMITOMO        = 3,
    CA_OLT_TYPE_CORTINA         = 4,
    CA_OLT_TYPE_UNKNOWN         = 5
}ca_scfg_olt_type_t;


#endif
