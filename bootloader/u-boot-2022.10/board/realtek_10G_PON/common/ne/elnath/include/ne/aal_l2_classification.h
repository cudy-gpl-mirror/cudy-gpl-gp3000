/*
 * Copyright (C) 2020 Realtek Semiconductor Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * aal_l2_classification.h: Hardware Abstraction Layer for classification driver to access hardware regsiters
 *
 */


#ifndef __AAL_L2_CLASSIFICATION_H__
#define __AAL_L2_CLASSIFICATION_H__
///--
#include <osal_type.h>

#define AAL_L2_CF_MAX_RULE_ID 512


typedef enum {
    AAL_L2_CF_PATTEN_DIRECTION_UPSTREAM = 0,
    AAL_L2_CF_PATTEN_DIRECTION_DOWNSTREAM = 1,
    AAL_L2_CF_PATTEN_DIRECTION_MAX = AAL_L2_CF_PATTEN_DIRECTION_DOWNSTREAM
} aal_l2_cf_patten_direction_t;

typedef enum {
    AAL_L2_CF_PATTEN_PKT_TYPE_UC = 0,
    AAL_L2_CF_PATTEN_PKT_TYPE_BC = 1,
    AAL_L2_CF_PATTEN_PKT_TYPE_MC = 2,
    AAL_L2_CF_PATTEN_PKT_TYPE_UUC = 3,
    AAL_L2_CF_PATTEN_PKT_TYPE_UMC = 4,
    AAL_L2_CF_PATTEN_PKT_TYPE_MAX = AAL_L2_CF_PATTEN_PKT_TYPE_UMC
} aal_l2_cf_patten_pkt_type_t;


typedef enum {
    AAL_L2_CF_ACTION_CS_ACT_TRANSPARENT = 0,
    AAL_L2_CF_ACTION_CS_ACT_DELETE_TAG = 1,
    AAL_L2_CF_ACTION_CS_ACT_ADD_TAG_STAG_TPID = 2,
    AAL_L2_CF_ACTION_CS_ACT_ADD_TAG_TPID_A = 3,
    AAL_L2_CF_ACTION_CS_ACT_ADD_TAG_TPID_B = 4,
    AAL_L2_CF_ACTION_CS_ACT_MAX = AAL_L2_CF_ACTION_CS_ACT_ADD_TAG_TPID_B
} aal_l2_cf_action_cs_act_t;

typedef enum {
    AAL_L2_CF_ACTION_PRI_ACT_ASSIGN = 0,
    AAL_L2_CF_ACTION_PRI_ACT_FROM_1ST_TAG = 1,
    AAL_L2_CF_ACTION_PRI_ACT_FROM_2ND_TAG = 2,
    AAL_L2_CF_ACTION_PRI_ACT_FROM_DSCP = 3,
    AAL_L2_CF_ACTION_PRI_ACT_MAX = AAL_L2_CF_ACTION_PRI_ACT_FROM_DSCP
} aal_l2_cf_action_pri_act_t;

typedef enum {
    AAL_L2_CF_ACTION_VID_ACT_ASSIGN = 0,
    AAL_L2_CF_ACTION_VID_ACT_FROM_1ST_TAG = 1,
    AAL_L2_CF_ACTION_VID_ACT_FROM_2ND_TAG = 2,
    AAL_L2_CF_ACTION_VID_ACT_FROM_LUT = 3,
    AAL_L2_CF_ACTION_VID_ACT_MAX = AAL_L2_CF_ACTION_VID_ACT_FROM_LUT
} aal_l2_cf_action_vid_act_t;

typedef enum {
    AAL_L2_CF_ACTION_FWD_ACT_NOP = 0,
    AAL_L2_CF_ACTION_FWD_ACT_DROP = 1,
    AAL_L2_CF_ACTION_FWD_ACT_TRAP_TO_CPU = 2,
    AAL_L2_CF_ACTION_FWD_ACT_LDPID = 3,
    AAL_L2_CF_ACTION_FWD_ACT_MCGID = 4,
    AAL_L2_CF_ACTION_FWD_ACT_MAX = AAL_L2_CF_ACTION_FWD_ACT_MCGID
} aal_l2_cf_action_fwd_act_t;

typedef enum {
    AAL_L2_CF_ACTION_C_ACT_TRANSPARENT = 0,
    AAL_L2_CF_ACTION_C_ACT_DELETE_CTAG = 1,
    AAL_L2_CF_ACTION_C_ACT_ADD_CTAG = 2,
    AAL_L2_CF_ACTION_C_ACT_MAX = AAL_L2_CF_ACTION_C_ACT_ADD_CTAG
} aal_l2_cf_action_c_act_t;

typedef enum {
    AAL_L2_CF_ACTION_FLOW_ID_ACT_NOP = 0,
    AAL_L2_CF_ACTION_FLOW_ID_ACT_ASSIGN = 1,
    AAL_L2_CF_ACTION_FLOW_ID_ACT_MAX = AAL_L2_CF_ACTION_FLOW_ID_ACT_ASSIGN
} aal_l2_cf_action_flow_id_act_t;

typedef enum {
    AAL_L2_CF_ACTION_GROUP_ID_ACT_NOP = 0,
    AAL_L2_CF_ACTION_GROUP_ID_ACT_ASSIGN = 1,
    AAL_L2_CF_ACTION_GROUP_ID_ACT_MAX = AAL_L2_CF_ACTION_GROUP_ID_ACT_ASSIGN
} aal_l2_cf_action_group_id_act_t;


typedef enum {
    AAL_L2_CF_ACTION_COS_ACT_NOP = 0,
    AAL_L2_CF_ACTION_COS_ACT_ASSIGN = 1,
    AAL_L2_CF_ACTION_COS_ACT_MAX = AAL_L2_CF_ACTION_COS_ACT_ASSIGN
} aal_l2_cf_action_cos_act_t;

typedef union{
    struct{
        ca_uint32_t    valid : 1;
        ca_uint32_t    ev2pt_table : 1;
        ca_uint32_t    tos_tc : 1;
        ca_uint32_t    outer_vid : 1;
        ca_uint32_t    outer_pri : 1;
        ca_uint32_t    outer_dei_cfi : 1;
        ca_uint32_t    ctag_vid : 1;
        ca_uint32_t    ctag_pri : 1;
        ca_uint32_t    ctag_cfi : 1;
        ca_uint32_t    direction : 1;
        ca_uint32_t    pol_id : 1;
        ca_uint32_t    igmp_mld : 1;
        ca_uint32_t    ipv6 : 1;
        ca_uint32_t    ipv4 : 1;
        ca_uint32_t    pkt_type : 1;
        ca_uint32_t    s_tpid : 1;
        ca_uint32_t    ctag : 1;
        ca_uint32_t    stag : 1;
        ca_uint32_t    lspid : 1;
        ca_uint32_t    ldpid : 1;
    }s;
    ca_uint32_t u32;
}aal_l2_cf_patten_mask_t;


typedef struct{
    ca_uint32_t                   valid;
    ca_uint32_t                   ev2pt_table;
    ca_uint32_t                   tos_tc;
    ca_uint32_t                   outer_vid;
    ca_uint32_t                   outer_pri;
    ca_boolean_t                  outer_dei_cfi;
    ca_uint32_t                   ctag_vid;
    ca_uint32_t                   ctag_pri;
    ca_boolean_t                  ctag_cfi;
    aal_l2_cf_patten_direction_t  direction;
    ca_uint32_t                   pol_id;
    ca_boolean_t                  igmp_mld;
    ca_boolean_t                  ipv6;
    ca_boolean_t                  ipv4;
    aal_l2_cf_patten_pkt_type_t   pkt_type;
    ca_uint32_t                   s_tpid;
    ca_boolean_t                  ctag;
    ca_boolean_t                  stag;
    ca_uint32_t                   lspid;
    ca_uint32_t                   ldpid;
}aal_l2_cf_patten_t;


typedef union{
    struct{
        ca_uint32_t    cs_act : 1;
        ca_uint32_t    cs_vid : 1;
        ca_uint32_t    cs_pri : 1;
        ca_uint32_t    cs_pri_act : 1;
        ca_uint32_t    cs_vid_act : 1;
        ca_uint32_t    cscfi_keep : 1;
        ca_uint32_t    grpid_act : 1;
        ca_uint32_t    flowid_act : 1;
        ca_uint32_t    cf_grpid : 1;
        ca_uint32_t    cf_flowid : 1;
        ca_uint32_t    fwd_act : 1;
        ca_uint32_t    cf_fwd_id : 1;
        ca_uint32_t    cact : 1;
        ca_uint32_t    cvid : 1;
        ca_uint32_t    cpri : 1;
        ca_uint32_t    cvid_act : 1;
        ca_uint32_t    cpri_act : 1;
        ca_uint32_t    ccfi_keep : 1;
        ca_uint32_t    cf_cos_act : 1;
        ca_uint32_t    cf_cos : 1;
    }s;
    ca_uint32_t u32;
}aal_l2_cf_action_mask_t;

typedef struct{
    aal_l2_cf_action_cs_act_t       cs_act;
    ca_uint32_t                     cs_vid;
    ca_uint32_t                     cs_pri;
    aal_l2_cf_action_pri_act_t      cs_pri_act;
    aal_l2_cf_action_vid_act_t      cs_vid_act;
    ca_boolean_t                    cscfi_keep;
    aal_l2_cf_action_group_id_act_t grpid_act;
    aal_l2_cf_action_flow_id_act_t  flowid_act;
    ca_uint32_t                     cf_grpid;
    ca_uint32_t                     cf_flowid;
    aal_l2_cf_action_fwd_act_t      fwd_act;
    ca_uint32_t                     cf_fwd_id;
    aal_l2_cf_action_c_act_t        cact;
    ca_uint32_t                     cvid;
    ca_uint32_t                     cpri;
    aal_l2_cf_action_vid_act_t      cvid_act;
    aal_l2_cf_action_pri_act_t      cpri_act;
    ca_boolean_t                    ccfi_keep;
    aal_l2_cf_action_cos_act_t      cf_cos_act;
    ca_uint32_t                     cf_cos;
}aal_l2_cf_action_t;

typedef struct{
    aal_l2_cf_patten_mask_t data_bits;
    aal_l2_cf_patten_mask_t care_bits;
}aal_l2_cf_key_mask_t;



typedef struct{
    aal_l2_cf_patten_t data_bits;
    aal_l2_cf_patten_t care_bits;
}aal_l2_cf_key_t;


typedef struct{
    aal_l2_cf_key_mask_t key;
    aal_l2_cf_action_mask_t action;
}aal_l2_cf_rule_mask_t;




typedef struct{
    aal_l2_cf_key_t    key;
    aal_l2_cf_action_t action;
}aal_l2_cf_rule_t;

ca_status_t aal_l2_cf_key_set(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_IN aal_l2_cf_key_mask_t mask,
                                      CA_IN aal_l2_cf_key_t * cfg);

ca_status_t aal_l2_cf_key_get(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_OUT aal_l2_cf_key_t * cfg);


ca_status_t aal_l2_cf_action_set(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_IN aal_l2_cf_action_mask_t mask,
                                      CA_IN aal_l2_cf_action_t * cfg);

ca_status_t aal_l2_cf_action_get(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_OUT aal_l2_cf_action_t * cfg);
 
ca_status_t aal_l2_cf_rule_add(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_IN aal_l2_cf_rule_t * cfg);

ca_status_t aal_l2_cf_rule_delete(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id);

ca_status_t aal_l2_cf_rule_get(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_OUT aal_l2_cf_rule_t * cfg);

ca_status_t aal_l2_cf_entry_get(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id);

ca_status_t aal_l2_cf_hit_entry_get(CA_IN ca_device_id_t device_id);

ca_status_t aal_l2_cf_debug_get(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t *debug);

ca_status_t aal_l2_cf_debug_set(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t *debug);

ca_status_t aal_l2_cf_init(CA_IN ca_device_id_t device_id);


#endif
