/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2010 by Cortina Systems Incorporated.                 */
/***********************************************************************/
/*
 *
 * aal_pon.h
 *
 */
#ifndef __AAL_PON_H
#define __AAL_PON_H

#include <asm/types.h>
#include <asm/io.h>
#include <config.h>

#define __TX_SYNC_FIFO_THOLD_MAX    0x7FF
#define __TX_ASYNC_FIFO_THOLD_MAX   0xF
#define __TX_MAX_PKT_SIZE               2047
#define __PON_MAX_FRAME_SIZE_DEFAULT  	12288
#define __PON_MAX_TX_FRAME_SIZE_DEFAULT  (__TX_MAX_PKT_SIZE-1)

#define __PON_IPG_12B                   0
#define __PON_IPG_11B                   1
#define __PON_IPG_10B                   2
#define __PON_IPG_13B                   3

#define SDS_P7_INV_HSO		(1<<12)
#define SDS_P7_INV_HSI		(1<<13)
#define SDS_INV_HSO			(1<<8)
#define SDS_INV_HSI			(1<<9)

#if defined(CONFIG_API_VENDOR) || defined(CONFIG_API_VENDOR_MODULE)
#define GLOBAL_INTR_EN  0x00000009  //GLOBAL_INTR_PON_MAC_BIT | GLOBAL_INTR_OPT_BIT;
#else
#define GLOBAL_INTR_EN  0x00000001  //GLOBAL_INTR_PON_MAC_BIT 
#endif

typedef enum
{
    ONU_PON_MAC_EPON_1G               = 0,   /* 1G Down, 1G Up, EPON  */
    ONU_PON_MAC_EPON_D10G             = 1,   /* 10G Down, 1G Up, EPON  */
    ONU_PON_MAC_EPON_BI10G            = 2,   /* 10G Down, 10G Up, 10G-EPON */
    ONU_PON_MAC_GPON                  = 3,   /* 2.5G Down, 1G Up, GPON */
    ONU_PON_MAC_XGPON1                = 4,   /* 10G Down, 2.5G Up, XG-PON1 */
    ONU_PON_MAC_XGSPON                = 5,   /* 10G Down, 10G Up, XGS-PON */
    ONU_PON_MAC_NGPON2                = 6,   /* 10G Down, 10G Up, NG-PON2 with multiple wavelength support*/
    ONU_PON_MAC_ETHERNET_10G          = 7,   /* 10G Ethernet */
    ONU_PON_MAC_TURBO_EPON            = 8,   /* 2G Down, 1G UP, EPON */
    ONU_PON_MAC_ETHERNET_1250M        = 9,   /* 1G Ethernet */
    ONU_PON_MAC_ETHERNET_3125M        = 10,  /* 2.5G Ethernet */
    ONU_PON_MAC_ETHERNET_SGMII_1G     = 11,  /* SGMII/1G */
    ONU_PON_MAC_ETHERNET_SGMII_2G5    = 12,  /* HiSGMII/2.5G */
    ONU_PON_MAC_ETHERNET_FIBER_100M   = 13,  /* 100BASE-X */
    ONU_PON_MAC_ETHERNET_FIBER_1G     = 14,  /* 1000BASE-X, */
    ONU_PON_MAC_ETHERNET_USXGMII_10G  = 15,  /* USXGMII/10G */
    ONU_PON_MAC_NGPON2_D10G           = 16,  /* 10G Down, 2.5GG Up, NG-PON2 with multiple wavelength support*/
    ONU_PON_MAC_ETHERNET_USXGMII_5G   = 17,  /* USXGMII/5G */
    ONU_PON_MAC_ETHERNET_USXGMII_2G5  = 18,  /* USXGMII/2G5 */
    ONU_PON_MAC_ETHERNET_USXGMII_1G   = 19,  /* USXGMII/1G */
    ONU_PON_MAC_ETHERNET_USXGMII_100M = 20,  /* USXGMII/100M */
    ONU_PON_MAC_ETHERNET_USXGMII_10M  = 21,  /* USXGMII/10M */
    ONU_PON_MAC_ETHERNET_SGMII_100M   = 22,  /* SGMII/100M */
    ONU_PON_MAC_ETHERNET_SGMII_10M    = 23,  /* SGMII/10M */
    ONU_PON_MAC_ETHERNET_FIBER_2G5    = 24,  /* 2500BASE-X, */
    ONU_PON_MAC_ETHERNET_USXGMII_500M = 25,  /* USXGMII/500M */
    ONU_PON_MAC_GPON_BI2G5            = 26,  /* 2.5G Down, 2.5G Up, GPON */
    ONU_PON_MAC_ETHERNET_SGMII_500M   = 27,  /* SGMII/500M */
    ONU_PON_MAC_MAX
} ca_scfg_pon_mac_mode_t;

typedef enum
{
    AAL_PSDS_MODE_XGEPON_SYM     = 0,
    AAL_PSDS_MODE_XGPON_SYM_25M  = 1,
    AAL_PSDS_MODE_XGPON_ASYM_25M = 2,
    AAL_PSDS_MODE_GPON           = 3,
    AAL_PSDS_MODE_XGEPON_ASYM    = 4,
    AAL_PSDS_MODE_ETHERNET_10G   = 5,	//XFI 10.3125G
    AAL_PSDS_MODE_EPON           = 6,
    AAL_PSDS_MODE_TUBO_EPON      = 7,
    AAL_PSDS_MODE_ETHERNET_1250M = 8,	//XFI 1.25G
    AAL_PSDS_MODE_ETHERNET_3125M = 9,	//XFI 3.125G
    AAL_PSDS_MODE_SGMII_1G       = 10,
    AAL_PSDS_MODE_SGMII_2G5      = 11,
    AAL_PSDS_MODE_FIBER_100M     = 12,
    AAL_PSDS_MODE_FIBER_1G       = 13,
    AAL_PSDS_MODE_USXGMII_10G    = 14,
    AAL_PSDS_MODE_USXGMII_5G     = 15,
    AAL_PSDS_MODE_USXGMII_2G5    = 16,
    AAL_PSDS_MODE_USXGMII_1G     = 17,
    AAL_PSDS_MODE_USXGMII_100M   = 18,
    AAL_PSDS_MODE_USXGMII_10M    = 19,
    AAL_PSDS_MODE_SGMII_100M     = 20,
    AAL_PSDS_MODE_SGMII_10M      = 21,
    AAL_PSDS_MODE_FIBER_2G5      = 22,
    AAL_PSDS_MODE_GPON_SYM       = 23,
    AAL_PSDS_MODE_SGMII_500M     = 24,
    AAL_PSDS_MODE_NUM
} aal_psds_mode_t;

typedef struct   /* PON SERDES status */
{
    ca_uint8_t rx_los        ;
    ca_uint8_t ckrdy_rx      ;
    ca_uint8_t ckrdy_tx      ;
    ca_uint8_t ckrdy_mpcp    ;
    ca_uint8_t rxidle        ;
    ca_uint8_t ber_notify    ;
    ca_uint32_t prbs_err_cnt;
    ca_uint8_t prbs_rx_out_of_sync;
} aal_psds_status_t;

typedef union
{
    struct
    {
        ca_uint32_t nocrc_insert           :  1 ;
        ca_uint32_t pdc_map_mem_en         :  1 ;
        ca_uint32_t epon_insert_16byte_hdr :  1 ;
        ca_uint32_t omci_hp_cos            :  1 ;
        ca_uint32_t omci_hp_ldpid          :  1 ;
        ca_uint32_t omci_hp_en             :  1 ;
        ca_uint32_t rsrvd1                 : 26;
    } bf ;
    ca_uint32_t     wrd ;
} aal_pdc_ctrl_mask_t;

typedef struct
{
    ca_uint32_t nocrc_insert           ;
    ca_uint32_t pdc_map_mem_en         ;
    ca_uint32_t epon_insert_16byte_hdr ;
    ca_uint32_t omci_hp_cos;
    ca_uint32_t omci_hp_ldpid;
    ca_uint32_t omci_hp_en;
} aal_pdc_ctrl_t;

typedef union
{
    struct
    {
        ca_uint32_t cos                  :  1 ;
        ca_uint32_t ldpid                :  1 ;
        ca_uint32_t lspid                :  1 ;
        ca_uint32_t fe_bypass            :  1 ;
        ca_uint32_t hdr_type             :  1 ;
        ca_uint32_t mcgid                :  1 ;
        ca_uint32_t drop_code            :  1 ;
        ca_uint32_t rx_pkt_type          :  1 ;
        ca_uint32_t no_drop              :  1 ;

        ca_uint32_t mirror               :  1 ;
        ca_uint32_t mark                 :  1 ;
        ca_uint32_t pol_en               :  1 ;
        ca_uint32_t pol_id               :  1 ;
        ca_uint32_t pol_grp_id           :  1 ;
        ca_uint32_t deepq                :  1 ;
        ca_uint32_t cpu_flag             :  1 ;
        ca_uint32_t rsrvd1               : 16 ;
    } bf ;
    ca_uint32_t     wrd ;
} aal_pdc_pdc_map_mem_data_mask_t;

typedef struct
{
    ca_uint8_t   cos;
    ca_uint16_t  ldpid;
    ca_uint16_t  lspid;
    ca_boolean_t fe_bypass;
    ca_uint8_t   hdr_type;
    ca_uint8_t   mcgid;
    ca_boolean_t drop_code;
    ca_uint8_t   rx_pkt_type;
    ca_boolean_t no_drop;

    ca_uint32_t mirror             ;
    ca_uint32_t mark               ;
    ca_uint32_t pol_en             ;
    ca_uint32_t pol_id             ;
    ca_uint32_t pol_grp_id         ;
    ca_uint32_t deepq              ;
    ca_uint32_t cpu_flag           ;
} aal_pdc_pdc_map_mem_data_t;

typedef union
{
    struct
    {
        ca_uint32_t   onu_type               : 1;
        ca_uint32_t   rx_xg_ena              : 1;
        ca_uint32_t   tx_xg_ena              : 1;
        ca_uint32_t   rx_xg_psize_max        : 1;
        ca_uint32_t   rx_xg_flowctrl_en      : 1;
        ca_uint32_t   tx_xg_ipg              : 1;
        ca_uint32_t   tx_xg_crc32_regen_en   : 1;
        ca_uint32_t   tx_xg_thold_sfifo_rd   : 1;
        ca_uint32_t   tx_xg_thold_sfifo_wr   : 1;
        ca_uint32_t   tx_xg_thold_asfifo_rd  : 1;
        ca_uint32_t   tx_xg_thold_asfifo_wr  : 1;

        ca_uint32_t   max_mtu_tag_en         : 1;
        ca_uint32_t   rx_1g_ena              : 1;
        ca_uint32_t   tx_1g_ena              : 1;
        ca_uint32_t   rx_1g_psize_max        : 1;
        ca_uint32_t   tx_1g_psize_max_ena    : 1;
        ca_uint32_t   tx_1g_psize_max        : 1;
        ca_uint32_t   tx_1g_ipg              : 1;
        ca_uint32_t   tx_1g_8b_preamble_ena  : 1;
        ca_uint32_t   tx_1g_crc8_order       : 1;
        ca_uint32_t   tx_1g_crc32_regen_en   : 1;
        ca_uint32_t   tx_1g_drain_ena        : 1;
        ca_uint32_t   rx2tx_lookback         : 1;
        ca_uint32_t   tx2rx_lookback         : 1;
    } s;
    ca_uint32_t u32;
} aal_epon_mac_ctrl_msk_t;

typedef struct
{
    ca_uint8_t                  onu_type               ; /* 1G-10G or 10G-10G                                                */
    ca_boolean_t                rx_xg_ena              ; /* (default=false) Enable/Disable XG RX MAC port                    */
    ca_boolean_t                tx_xg_ena              ; /* (default=false) Enable/Disable XG TX MAC port                    */

    ca_uint32_t                 rx_xg_psize_max        ; /* (default=2039) The max length of the packet for RX               */
    ca_boolean_t                rx_xg_flowctrl_en      ; /* (default=1) Enable receiving pause frame and stop transimition   */
    ca_uint8_t                  tx_xg_ipg              ; /* (default=12) Should be 8 or 12 only                              */
    ca_boolean_t                tx_xg_crc32_regen_en   ; /* enable crc32 regeneration                                        */
    ca_uint16_t                 tx_xg_thold_sfifo_rd   ; /* (default=15) 0~512, read threshold in the sync fifo of tx ctrl   */
    ca_uint16_t                 tx_xg_thold_sfifo_wr   ; /* (default=15) 0~512, write threshold in the sync fifo of tx ctrl  */
    ca_uint8_t                  tx_xg_thold_asfifo_rd  ; /* (default=3) 0~15, read threshold in the async fifo of tx part    */
    ca_uint8_t                  tx_xg_thold_asfifo_wr  ; /* (default=3) 0~15, write threshold in the async fifo of tx part   */

    ca_boolean_t                max_mtu_tag_en         ; /* for epon rx. NOTE:  when
                                                            1. CSR bit max_mtu_tag_en is set, and
                                                            2. ingress frame has tag and tag pattern matches one of  0x8100, 0x9100 or CSR
                                                            mtu_tag_pattern, and
                                                            3. ingress frame size is no more than 14'h3FFB

                                                            then the max packet size will increase by 4.*/
    ca_boolean_t                rx_1g_ena              ; /* (default=false) Enable/Disable 1G RX MAC port                    */
    ca_boolean_t                tx_1g_ena              ; /* enable MAC transimit                                             */

    ca_uint32_t                 rx_1g_psize_max        ; /* (default=2039) The max length of the packet for RX               */
    ca_boolean_t                tx_1g_psize_max_ena    ; /* Left data will be cut off but still counted into packet size   */
    ca_uint32_t                 tx_1g_psize_max        ; /* configure tx mtu, 0~2047B                                        */
    ca_uint8_t                  tx_1g_ipg              ; /* IPG setting: 10~13, unit is byte                                 */
    ca_boolean_t                tx_1g_8b_preamble_ena  ; /* 1-transimit data with fixing 8B preamble, 0-normal               */
    ca_uint8_t                  tx_1g_crc8_order       ; /* reverse crc8 and data or not                                     */
    ca_boolean_t                tx_1g_crc32_regen_en   ; /* enable crc32 regeneration                                        */
    ca_boolean_t                tx_1g_drain_ena        ; /* enable drain out data from BM b dropping                         */
    ca_boolean_t                rx2tx_lookback         ;
    ca_boolean_t                tx2rx_lookback         ;
} aal_epon_mac_ctrl_t;

typedef enum
{
    AAL_PON_CRC8_MODE_NORMAL                   = 0, /* not reverse crc8, not reverse data order   */
    AAL_PON_CRC8_MODE_REV_DATA                 = 1, /* not reverse crc8, reverse data order       */
    AAL_PON_CRC8_MODE_REV_CRC8                 = 2, /* reverse crc8, not reverse data order       */
    AAL_PON_CRC8_MODE_REV_CRC8_DATA            = 3, /* reverse crc8, reverse data order           */
} aal_epon_mac_crc8_order_t;

ca_status_t ca_pon_init(ca_device_id_t device_id, ca_uint32_t pon_mode_in);
void rtk_sds_polarity_setting(uint32 hw_config);

#endif /* __CORTINA_NI_PON_H */

