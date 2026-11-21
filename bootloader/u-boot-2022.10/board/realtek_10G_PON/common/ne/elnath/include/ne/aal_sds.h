/*
 * Copyright (C) 2021 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : Definition 
 *
 * Feature : The file have include the following module and sub-modules
 *           (1) Hardware abstract layer of Serdes
 */

#ifndef __AAL_SDS_H__
#define __AAL_SDS_H__

typedef enum {
    AAL_SDS_MODE_NONE = 0,
    AAL_SDS_MODE_SGMII = 1,         /* For SGMII/1G, HiSGMII/2.5G */
    AAL_SDS_MODE_FIBER = 2,         /* For 100BASE-X, 1000BASE-X, 2500BASE-X */
    AAL_SDS_MODE_XFI = 3,           /* For XFI/10G */
    AAL_SDS_MODE_USXGMII = 4,       /* For USXGMII/10G */
    AAL_SDS_MODE_RXAUI = 5,         /* For RXAUI/10G */
    AAL_SDS_MODE_RGMII = 6,         /* For RGMII/1G/100M */
    AAL_SDS_MODE_QXGMII = 7,        /* For QXGMII/10M/100M/1G/2.5G */
    AAL_SDS_MODE_END
} aal_sds_mode_t;

#define AAL_SDS_MODE_STR_SGMII/*CODEGEN_IGNORE_TAG*/      "SGMII"
#define AAL_SDS_MODE_STR_FIBER/*CODEGEN_IGNORE_TAG*/      "FIBER"
#define AAL_SDS_MODE_STR_XFI/*CODEGEN_IGNORE_TAG*/        "XFI"
#define AAL_SDS_MODE_STR_USXGMII/*CODEGEN_IGNORE_TAG*/    "USXGMII"
#define AAL_SDS_MODE_STR_RXAUI/*CODEGEN_IGNORE_TAG*/      "RXAUI"
#define AAL_SDS_MODE_STR_RGMII/*CODEGEN_IGNORE_TAG*/      "RGMII"
#define AAL_SDS_MODE_STR_QXGMII/*CODEGEN_IGNORE_TAG*/     "QXGMII"

typedef enum {
    AAL_SDS_SPEED_DEFAULT = 0,      /* No speed specified */
    AAL_SDS_SPEED_100M = 1,
    AAL_SDS_SPEED_1G = 2,
    AAL_SDS_SPEED_2G5 = 3,
    AAL_SDS_SPEED_5G = 4,
    AAL_SDS_SPEED_10G = 5,
    AAL_SDS_SPEED_10M = 6,
    AAL_SDS_SPEED_END
} aal_sds_speed_t;

typedef enum {
    AAL_SDS_NAME_SGMII_1G = 0,
    AAL_SDS_NAME_SGMII_2G5 = 1,
    AAL_SDS_NAME_FIBER_100M = 2,
    AAL_SDS_NAME_FIBER_1G = 3,
    AAL_SDS_NAME_XFI_10G = 4,
    AAL_SDS_NAME_USXGMII_10G = 5,
    AAL_SDS_NAME_RXAUI_10G = 6,
    AAL_SDS_NAME_RGMII_1G = 7,
    AAL_SDS_NAME_RGMII_100M = 8,
    AAL_SDS_NAME_QXGMII_2G5 = 9,
    AAL_SDS_NAME_END
} aal_sds_name_t;

typedef enum {
    AAL_SDS_CHAN_SPD_10M = 0,
    AAL_SDS_CHAN_SPD_100M,
    AAL_SDS_CHAN_SPD_1G,
    AAL_SDS_CHAN_SPD_500M,
    AAL_SDS_CHAN_SPD_10G,
    AAL_SDS_CHAN_SPD_2G5,
    AAL_SDS_CHAN_SPD_5G,
    MAX_AAL_SDS_CHAN_SPD_ABILITY
} aal_sds_chan_spd_ability_t;

typedef struct {
    ca_uint8_t signal_ok;
    ca_uint8_t link_ok;
    ca_uint8_t sync_ok;
    ca_uint8_t ber_notify;
    ca_uint8_t rx_idle;
    ca_uint8_t ck_rdy;
    ca_uint8_t blk_lock;
    ca_uint8_t link_ok_10G;
    ca_uint8_t link_ok_ch0;
    ca_uint8_t link_ok_ch1;
    ca_uint8_t link_ok_ch2;
    ca_uint8_t link_ok_ch3;
    ca_uint8_t link_ok_rxaui;
} aal_sds_status_t;

/* Structure for SDS indirect access */
typedef struct rtl8277c_sds_acc_s {
    ca_uint32_t reserved:21;
    ca_uint32_t page:6;
    ca_uint32_t regaddr:5;
} rtl8277c_sds_acc_t;

extern ca_status_t aal_sds_mode_set(
	CA_IN ca_device_id_t       device_id,
	CA_IN ca_uint32_t    	     port_index,
	CA_IN aal_sds_mode_t       mode,
	CA_IN aal_sds_speed_t      speed);

extern ca_status_t aal_sds_mode_get(
	CA_IN ca_device_id_t       device_id,
	CA_IN ca_uint32_t    	     port_index,
	CA_OUT aal_sds_mode_t       *mode,
	CA_OUT aal_sds_speed_t      *speed);

extern ca_status_t aal_sds_nway_set(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    ca_uint32_t          enable);

extern  ca_status_t aal_sds_nway_get(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    ca_uint32_t          *enable);

/** aal_sds_mode_get :
 * @brief: Get SDS interface status based on port index
 * @note: for each port, part or all of the status in aal_sds_status_t are valid
 * for CA_PORT_ID_NI6:
 *   ca_uint8_t ber_notify;
 *   ca_uint8_t rx_idle;
 *   ca_uint8_t ck_rdy;
 *   ca_uint8_t link_ok_10G;
 *   ca_uint8_t link_ok_ch0;
 *   ca_uint8_t link_ok_ch1;
 *   ca_uint8_t link_ok_ch2;
 *   ca_uint8_t link_ok_ch3;
 */
extern ca_status_t
aal_sds_status_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint32_t          port_index,
    CA_OUT aal_sds_status_t     *status);

extern ca_status_t aal_sds_ind_set(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    ca_uint32_t          page,
    ca_uint32_t          reg,
    ca_uint32_t          data);

extern ca_status_t aal_sds_ind_get(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    ca_uint32_t          page,
    ca_uint32_t          reg,
    ca_uint32_t          *data);

#endif /* __AAL_SDS_H__ */