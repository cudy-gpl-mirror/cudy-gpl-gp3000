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
 * aal_l2_classification.c: Hardware Abstraction Layer for classification driver to access hardware regsiters
 *
 */
#include <osal_soc.h>
#include <osal.h>
#include <aal_l2_classification.h>
#include <aal_common.h>
#include <net.h>
#include <realtek_ne.h>

#if 0
ca_uint __aal_l2_cf_lock = 0;
#define __CF_LOCK()	ca_spin_lock(__aal_l2_cf_lock)
#define __CF_UNLOCK() ca_spin_unlock(__aal_l2_cf_lock)
#else
#define __CF_LOCK()	
#define __CF_UNLOCK() 
#endif

ca_uint32_t debug_val = 0;

char* cs_act_show_str[5]={"Transparent","Delete","Stag_TPID","TPID_A","TPID_B"};
char* ctag_act_show_str[3]={"Transparent","Delete","Tagging"};
char* pri_act_show_str[4]={"Assign","Copy from outer","Copy from inner","Copy from DSCP"};
char* vid_act_show_str[4]={"Assign","Copy from outer","Copy from inner","Copy from LUT"};
char* cfi_act_show_str[2]={"Always 0","Keep from ingress"};
char* flowid_act_show_str[2]={"Nop","Assign"};
char* grpid_act_show_str[2]={"Nop","Assign"};
char* fwd_act_show_str[5]={"Nop","Drop","Trap to CPU","LDPID","MCGID"};
char* cos_act_show_str[2]={"Nop","Assign"};
char* pkt_type_show_str[5]={"Unicast","Broadcast","Multicast","Unknown Unicast","Unknown Multicast"};

typedef struct{
    L2FE_PONCLE_KEY_DATA0_t key_data0;
    L2FE_PONCLE_KEY_DATA1_t key_data1;
    L2FE_PONCLE_KEY_DATA2_t key_data2;
}__aal_cf_pattern_t;


typedef struct{
    __aal_cf_pattern_t data;
    __aal_cf_pattern_t mask;
}__aal_cf_key_data_t;

typedef struct{
    L2FE_PONCLE_FIB_DATA0_t fib_data0;
    L2FE_PONCLE_FIB_DATA1_t fib_data1;
    L2FE_PONCLE_FIB_DATA2_t fib_data2;
}__aal_cf_fib_data_t;


void __aal_l2_cf_key_show(CA_IN aal_l2_cf_patten_t *data,
                                      CA_IN aal_l2_cf_patten_t *mask)
{
    if(mask->direction != 0)
        PRINTF("     Direction: %s\n",data->direction == 0 ? "Upstream":"Downstream");
    if(mask->pkt_type != 0)
    {
        if(mask->pkt_type == 5 && data->pkt_type == 1)
            PRINTF("   Packet Type: Broadcast or Unknown Unicast\n");
        else
            PRINTF("   Packet Type: %s\n",pkt_type_show_str[data->pkt_type]);
    }
    if(mask->ev2pt_table != 0)
        PRINTF("    EV2PTVALUE: (%u,0x%x)\n",data->ev2pt_table,mask->ev2pt_table);
    if(mask->tos_tc != 0)
        PRINTF("        TOS/TC: (0x%02x,0x%x)\n",data->tos_tc,mask->tos_tc);
    if(mask->outer_pri != 0)
        PRINTF("Outer Priority: (%u,0x%x)\n",data->outer_pri,mask->outer_pri);
    if(mask->outer_dei_cfi != 0)
        PRINTF(" Outer DEI/CFI: (%u,0x%x)\n",data->outer_dei_cfi,mask->outer_dei_cfi);
    if(mask->outer_vid != 0)
        PRINTF("     Outer VID: (%u,0x%x)\n",data->outer_vid,mask->outer_vid);
    if(mask->ctag_pri != 0)
        PRINTF(" Ctag Priority: (%u,0x%x)\n",data->ctag_pri,mask->ctag_pri);
    if(mask->ctag_cfi != 0)
        PRINTF("      Ctag CFI: (%u,0x%x)\n",data->ctag_cfi,mask->ctag_cfi);
    if(mask->ctag_vid != 0)
        PRINTF("      Ctag VID: (%d,0x%x)\n",data->ctag_vid,mask->ctag_vid);
    if(mask->pol_id != 0)
        PRINTF("   GEM ID/LLID: (0x%x,0x%x)\n",data->pol_id,mask->pol_id);
    if(mask->igmp_mld != 0)
        PRINTF("      IGMP/MLD: (%u,0x%x)\n",data->igmp_mld,mask->igmp_mld);
    if(mask->ipv6 != 0)
        PRINTF("          IPv6: (%u,0x%x)\n",data->ipv6,mask->ipv6);
    if(mask->ipv4 != 0)
        PRINTF("          IPv4: (%u,0x%x)\n",data->ipv4,mask->ipv4);
    if(mask->s_tpid != 0)
        PRINTF("        S_TPID: (%u,0x%x)\n",data->s_tpid,mask->s_tpid);
    if(mask->ctag != 0)
        PRINTF("          Ctag: (%u,0x%x)\n",data->ctag,mask->ctag);
    if(mask->stag != 0)
        PRINTF("          Stag: (%u,0x%x)\n",data->stag,mask->stag);
    if(mask->lspid != 0)
        PRINTF("         LSPID: (0x%x,0x%x)\n",data->lspid,mask->lspid);
    if(mask->ldpid != 0)
        PRINTF("         LDPID: (0x%x,0x%x)\n",data->ldpid,mask->ldpid);
}

void __aal_l2_cf_action_show(CA_IN aal_l2_cf_action_t *action)
{
    PRINTF("Stag Action: %s\n",cs_act_show_str[action->cs_act]);
    if(action->cs_act != AAL_L2_CF_ACTION_CS_ACT_TRANSPARENT && action->cs_act != AAL_L2_CF_ACTION_CS_ACT_DELETE_TAG)
    {
        PRINTF("Stag VID Action: %s\n",vid_act_show_str[action->cs_vid_act]);
        PRINTF("Stag VID: %u\n",action->cs_vid);
        PRINTF("Stag PRI Action: %s\n",pri_act_show_str[action->cs_pri_act]);
        PRINTF("Stag PRI: %u\n",action->cs_pri);
        PRINTF("Stag CFI Action: %s\n",cfi_act_show_str[action->cscfi_keep]);
    }
    PRINTF("Ctag Action: %s\n",ctag_act_show_str[action->cact]);
    if(action->cact != AAL_L2_CF_ACTION_C_ACT_TRANSPARENT && action->cact != AAL_L2_CF_ACTION_C_ACT_DELETE_CTAG)
    {
        PRINTF("Ctag VID Action: %s\n",vid_act_show_str[action->cvid_act]);
        PRINTF("Ctag VID: %u\n",action->cvid);
        PRINTF("Ctag PRI Action: %s\n",pri_act_show_str[action->cpri_act]);
        PRINTF("Ctag PRI: %u\n",action->cpri);
        PRINTF("Ctag CFI Action: %s\n",cfi_act_show_str[action->ccfi_keep]);
    }

    if(action->flowid_act != AAL_L2_CF_ACTION_FLOW_ID_ACT_NOP)
        PRINTF("Flow ID action  : %s %u\n",flowid_act_show_str[action->flowid_act], action->cf_flowid);
    else
        PRINTF("Flow ID action  : %s\n",flowid_act_show_str[action->flowid_act]);

    if(action->grpid_act != AAL_L2_CF_ACTION_GROUP_ID_ACT_NOP)
        PRINTF("Group ID action : %s %u\n",grpid_act_show_str[action->grpid_act], action->cf_grpid);
    else
        PRINTF("Group ID action : %s\n",grpid_act_show_str[action->grpid_act]);

    if(action->fwd_act != AAL_L2_CF_ACTION_FWD_ACT_NOP)
        PRINTF("Forward action  : %s 0x%x\n",fwd_act_show_str[action->fwd_act], action->cf_fwd_id);
    else
        PRINTF("Forward action  : %s\n",fwd_act_show_str[action->fwd_act]);

    if(action->cf_cos_act != AAL_L2_CF_ACTION_COS_ACT_NOP)
        PRINTF("COS action      : %s %u\n",cos_act_show_str[action->cf_cos_act], action->cf_cos);
    else
        PRINTF("COS action      : %s\n",cos_act_show_str[action->cf_cos_act]);
}

ca_status_t __aal_l2_cf_key_read(ca_uint32_t entry_id,
                                              __aal_cf_key_data_t* cf_key)
{
    __aal_cf_key_data_t tcam_cf_key;

    /*Read care bits*/
    READ_INDIRCT_REG(0, entry_id, L2FE_PONCLE_KEY_ACCESS);
    tcam_cf_key.mask.key_data0.wrd = CA_NE_REG_READ(L2FE_PONCLE_KEY_DATA0);
    tcam_cf_key.mask.key_data1.wrd = CA_NE_REG_READ(L2FE_PONCLE_KEY_DATA1);
    tcam_cf_key.mask.key_data2.wrd = CA_NE_REG_READ(L2FE_PONCLE_KEY_DATA2);
    /*Read data bits*/
    READ_INDIRCT_REG(0, entry_id + AAL_L2_CF_MAX_RULE_ID, L2FE_PONCLE_KEY_ACCESS);
    tcam_cf_key.data.key_data0.wrd = CA_NE_REG_READ(L2FE_PONCLE_KEY_DATA0);
    tcam_cf_key.data.key_data1.wrd = CA_NE_REG_READ(L2FE_PONCLE_KEY_DATA1);
    tcam_cf_key.data.key_data2.wrd = CA_NE_REG_READ(L2FE_PONCLE_KEY_DATA2);


    /*TCAM to raw data*/
    cf_key->data.key_data0.wrd = (~tcam_cf_key.mask.key_data0.wrd) & tcam_cf_key.data.key_data0.wrd;
    cf_key->mask.key_data0.wrd = tcam_cf_key.mask.key_data0.wrd ^ tcam_cf_key.data.key_data0.wrd;

    cf_key->data.key_data1.wrd = (~tcam_cf_key.mask.key_data1.wrd) & tcam_cf_key.data.key_data1.wrd;
    cf_key->mask.key_data1.wrd = tcam_cf_key.mask.key_data1.wrd ^ tcam_cf_key.data.key_data1.wrd;

    cf_key->data.key_data2.wrd = (~tcam_cf_key.mask.key_data2.wrd) & tcam_cf_key.data.key_data2.wrd;
    cf_key->mask.key_data2.wrd = tcam_cf_key.mask.key_data2.wrd ^ tcam_cf_key.data.key_data2.wrd;

    cf_key->data.key_data2.bf.valid = tcam_cf_key.data.key_data2.bf.valid;
    cf_key->mask.key_data2.bf.valid = tcam_cf_key.mask.key_data2.bf.valid;

    return CA_E_OK;
}

ca_status_t __aal_l2_cf_key_write(ca_uint32_t entry_id,
                                              __aal_cf_key_data_t* cf_key)
{
    __aal_cf_key_data_t tcam_cf_key;

    /* raw data to TCAM */
    tcam_cf_key.data.key_data0.wrd = (~cf_key->mask.key_data0.wrd) | (cf_key->data.key_data0.wrd);
    tcam_cf_key.mask.key_data0.wrd = (~cf_key->mask.key_data0.wrd) | (~cf_key->data.key_data0.wrd);

    tcam_cf_key.data.key_data1.wrd = (~cf_key->mask.key_data1.wrd) | (cf_key->data.key_data1.wrd);
    tcam_cf_key.mask.key_data1.wrd = (~cf_key->mask.key_data1.wrd) | (~cf_key->data.key_data1.wrd);

    tcam_cf_key.data.key_data2.wrd = (~cf_key->mask.key_data2.wrd) | (cf_key->data.key_data2.wrd);
    tcam_cf_key.mask.key_data2.wrd = (~cf_key->mask.key_data2.wrd) | (~cf_key->data.key_data2.wrd);

    tcam_cf_key.data.key_data2.bf.valid = cf_key->data.key_data2.bf.valid;
    tcam_cf_key.mask.key_data2.bf.valid = cf_key->mask.key_data2.bf.valid;

    /*Write care bits*/
    CA_NE_REG_WRITE(tcam_cf_key.mask.key_data0.wrd, L2FE_PONCLE_KEY_DATA0);
    CA_NE_REG_WRITE(tcam_cf_key.mask.key_data1.wrd, L2FE_PONCLE_KEY_DATA1);
    CA_NE_REG_WRITE(tcam_cf_key.mask.key_data2.wrd, L2FE_PONCLE_KEY_DATA2);
    WRITE_INDIRCT_REG(0, entry_id, L2FE_PONCLE_KEY_ACCESS);

    /*Write data bits*/
    CA_NE_REG_WRITE(tcam_cf_key.data.key_data0.wrd, L2FE_PONCLE_KEY_DATA0);
    CA_NE_REG_WRITE(tcam_cf_key.data.key_data1.wrd, L2FE_PONCLE_KEY_DATA1);
    CA_NE_REG_WRITE(tcam_cf_key.data.key_data2.wrd, L2FE_PONCLE_KEY_DATA2);
    WRITE_INDIRCT_REG(0, entry_id + AAL_L2_CF_MAX_RULE_ID, L2FE_PONCLE_KEY_ACCESS);

    return CA_E_OK;
}

ca_status_t __aal_l2_cf_action_read(ca_uint32_t entry_id,
                                              __aal_cf_fib_data_t* cf_fib)
{    
    READ_INDIRCT_REG(0, entry_id, L2FE_PONCLE_FIB_ACCESS);
    cf_fib->fib_data0.wrd = CA_NE_REG_READ(L2FE_PONCLE_FIB_DATA0);
    cf_fib->fib_data1.wrd = CA_NE_REG_READ(L2FE_PONCLE_FIB_DATA1);
    cf_fib->fib_data2.wrd = CA_NE_REG_READ(L2FE_PONCLE_FIB_DATA2);

    return CA_E_OK;
}

ca_status_t __aal_l2_cf_action_write(ca_uint32_t entry_id,
                                              __aal_cf_fib_data_t* cf_fib)
{
    CA_NE_REG_WRITE(cf_fib->fib_data0.wrd, L2FE_PONCLE_FIB_DATA0);
    CA_NE_REG_WRITE(cf_fib->fib_data1.wrd, L2FE_PONCLE_FIB_DATA1);
    CA_NE_REG_WRITE(cf_fib->fib_data2.wrd, L2FE_PONCLE_FIB_DATA2);
    WRITE_INDIRCT_REG(0, entry_id, L2FE_PONCLE_FIB_ACCESS);

    return CA_E_OK;
}

void __aal_l2_cf_delete_all(void)
{
    ca_uint32_t i;
    for(i=0;i<AAL_L2_CF_MAX_RULE_ID;i++)
        aal_l2_cf_rule_delete(0,i);
}


ca_status_t aal_l2_cf_key_set(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_IN aal_l2_cf_key_mask_t mask,
                                      CA_IN aal_l2_cf_key_t * cfg)
{
    ca_status_t ret = CA_E_OK;
    __aal_cf_key_data_t cf_key;
    ca_uint32_t vid,pri,dei;

    if ((entry_id >= AAL_L2_CF_MAX_RULE_ID) || (NULL == cfg)) {
       return CA_E_PARAM;
    }

    __CF_LOCK();

    __aal_l2_cf_key_read(entry_id,&cf_key);

    if(mask.care_bits.s.valid)
    {
        cf_key.mask.key_data2.bf.valid = cfg->care_bits.valid;
    }

    if(mask.care_bits.s.ev2pt_table)
    {
        cf_key.mask.key_data2.bf.ether_type = cfg->care_bits.ev2pt_table;
    }
    if(mask.care_bits.s.tos_tc)
    {
        cf_key.mask.key_data2.bf.tos = cfg->care_bits.tos_tc;
    }

    vid = cf_key.mask.key_data1.bf.outer_tag & 0xFFF;
    dei = (cf_key.mask.key_data1.bf.outer_tag >> 12) & 0x1;
    pri = (cf_key.mask.key_data1.bf.outer_tag >> 13) & 0x7;

    if(mask.care_bits.s.outer_vid)
    {
        vid = cfg->care_bits.outer_vid;
    }
    if(mask.care_bits.s.outer_pri)
    {
        pri = cfg->care_bits.outer_pri;
    }
    if(mask.care_bits.s.outer_dei_cfi)
    {
        dei = cfg->care_bits.outer_dei_cfi;
    }

    cf_key.mask.key_data1.bf.outer_tag = (pri << 13) | (dei << 12) | vid;

    vid = cf_key.mask.key_data1.bf.ctag & 0xFFF;
    dei = (cf_key.mask.key_data1.bf.ctag >> 12) & 0x1;
    pri = (cf_key.mask.key_data1.bf.ctag >> 13) & 0x7;

    if(mask.care_bits.s.ctag_vid)
    {
        vid = cfg->care_bits.ctag_vid;
    }
    if(mask.care_bits.s.ctag_pri)
    {
        pri = cfg->care_bits.ctag_pri;
    }
    if(mask.care_bits.s.ctag_cfi)
    {
        dei = cfg->care_bits.ctag_cfi;
    }

    cf_key.mask.key_data1.bf.ctag = (pri << 13) | (dei << 12) | vid;
    
    if(mask.care_bits.s.direction)
    {
        cf_key.mask.key_data0.bf.stream = cfg->care_bits.direction;
    }
    if(mask.care_bits.s.pol_id)
    {
        cf_key.mask.key_data0.bf.pol_id = cfg->care_bits.pol_id;
    }
    if(mask.care_bits.s.igmp_mld)
    {
        cf_key.mask.key_data0.bf.igmp_pkt = cfg->care_bits.igmp_mld;
    }
    if(mask.care_bits.s.ipv6)
    {
        cf_key.mask.key_data0.bf.ipv6_pkt = cfg->care_bits.ipv6;
    }
    if(mask.care_bits.s.ipv4)
    {
        cf_key.mask.key_data0.bf.ipv4_pkt = cfg->care_bits.ipv4;
    }
    if(mask.care_bits.s.pkt_type)
    {
        cf_key.mask.key_data0.bf.pkt_type = cfg->care_bits.pkt_type;
    }
    if(mask.care_bits.s.s_tpid)
    {
        cf_key.mask.key_data0.bf.stag_tpid_code = cfg->care_bits.s_tpid;
    }
    if(mask.care_bits.s.ctag)
    {
        cf_key.mask.key_data0.bf.ctag_pkt = cfg->care_bits.ctag;
    }
    if(mask.care_bits.s.stag)
    {
        cf_key.mask.key_data0.bf.stag_pkt = cfg->care_bits.stag;
    }
    if(mask.care_bits.s.lspid)
    {
        cf_key.mask.key_data0.bf.lspid = cfg->care_bits.lspid;
    }
    if(mask.care_bits.s.ldpid)
    {
        cf_key.mask.key_data0.bf.ldpid = cfg->care_bits.ldpid;
    }

    if(mask.data_bits.s.valid)
    {
        cf_key.data.key_data2.bf.valid = cfg->data_bits.valid;
    }

    if(mask.data_bits.s.ev2pt_table)
    {
        cf_key.data.key_data2.bf.ether_type = cfg->data_bits.ev2pt_table;
    }
    if(mask.data_bits.s.tos_tc)
    {
        cf_key.data.key_data2.bf.tos = cfg->data_bits.tos_tc;
    }

    vid = cf_key.data.key_data1.bf.outer_tag & 0xFFF;
    dei = (cf_key.data.key_data1.bf.outer_tag >> 12) & 0x1;
    pri = (cf_key.data.key_data1.bf.outer_tag >> 13) & 0x7;

    if(mask.data_bits.s.outer_vid)
    {
        vid = cfg->data_bits.outer_vid;
    }
    if(mask.data_bits.s.outer_pri)
    {
        pri = cfg->data_bits.outer_pri;
    }
    if(mask.data_bits.s.outer_dei_cfi)
    {
        dei = cfg->data_bits.outer_dei_cfi;
    }

    cf_key.data.key_data1.bf.outer_tag = (pri << 13) | (dei << 12) | vid;

    vid = cf_key.data.key_data1.bf.ctag & 0xFFF;
    dei = (cf_key.data.key_data1.bf.ctag >> 12) & 0x1;
    pri = (cf_key.data.key_data1.bf.ctag >> 13) & 0x7;

    if(mask.data_bits.s.ctag_vid)
    {
        vid = cfg->data_bits.ctag_vid;
    }
    if(mask.data_bits.s.ctag_pri)
    {
        pri = cfg->data_bits.ctag_pri;
    }
    if(mask.data_bits.s.ctag_cfi)
    {
        dei = cfg->data_bits.ctag_cfi;
    }

    cf_key.data.key_data1.bf.ctag = (pri << 13) | (dei << 12) | vid;
    
    if(mask.data_bits.s.direction)
    {
        cf_key.data.key_data0.bf.stream = cfg->data_bits.direction;
    }
    if(mask.data_bits.s.pol_id)
    {
        cf_key.data.key_data0.bf.pol_id = cfg->data_bits.pol_id;
    }
    if(mask.data_bits.s.igmp_mld)
    {
        cf_key.data.key_data0.bf.igmp_pkt = cfg->data_bits.igmp_mld;
    }
    if(mask.data_bits.s.ipv6)
    {
        cf_key.data.key_data0.bf.ipv6_pkt = cfg->data_bits.ipv6;
    }
    if(mask.data_bits.s.ipv4)
    {
        cf_key.data.key_data0.bf.ipv4_pkt = cfg->data_bits.ipv4;
    }
    if(mask.data_bits.s.pkt_type)
    {
        cf_key.data.key_data0.bf.pkt_type = cfg->data_bits.pkt_type;
    }
    if(mask.data_bits.s.s_tpid)
    {
        cf_key.data.key_data0.bf.stag_tpid_code = cfg->data_bits.s_tpid;
    }
    if(mask.data_bits.s.ctag)
    {
        cf_key.data.key_data0.bf.ctag_pkt = cfg->data_bits.ctag;
    }
    if(mask.data_bits.s.stag)
    {
        cf_key.data.key_data0.bf.stag_pkt = cfg->data_bits.stag;
    }
    if(mask.data_bits.s.lspid)
    {
        cf_key.data.key_data0.bf.lspid = cfg->data_bits.lspid;
    }
    if(mask.data_bits.s.ldpid)
    {
        cf_key.data.key_data0.bf.ldpid = cfg->data_bits.ldpid;
    }

    __aal_l2_cf_key_write(entry_id,&cf_key);

    __CF_UNLOCK();
    return ret;
}

ca_status_t aal_l2_cf_key_get(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_OUT aal_l2_cf_key_t * cfg)
{
    ca_status_t ret = CA_E_OK;
    __aal_cf_key_data_t cf_key;
    if ((entry_id >= AAL_L2_CF_MAX_RULE_ID) || (NULL == cfg)) {
       return CA_E_PARAM;
    }

    __CF_LOCK();

    __aal_l2_cf_key_read(entry_id,&cf_key);
    cfg->care_bits.valid = cf_key.mask.key_data2.bf.valid;
    cfg->care_bits.ev2pt_table = cf_key.mask.key_data2.bf.ether_type;
    cfg->care_bits.tos_tc = cf_key.mask.key_data2.bf.tos;
    cfg->care_bits.outer_vid = cf_key.mask.key_data1.bf.outer_tag & 0xFFF;
    cfg->care_bits.outer_dei_cfi = (cf_key.mask.key_data1.bf.outer_tag >> 12) & 0x1;
    cfg->care_bits.outer_pri = (cf_key.mask.key_data1.bf.outer_tag >> 13) & 0x7;
    cfg->care_bits.ctag_vid = cf_key.mask.key_data1.bf.ctag & 0xFFF;
    cfg->care_bits.ctag_cfi = (cf_key.mask.key_data1.bf.ctag >> 12) & 0x1;
    cfg->care_bits.ctag_pri = (cf_key.mask.key_data1.bf.ctag >> 13) & 0x7;
    cfg->care_bits.direction = cf_key.mask.key_data0.bf.stream;
    cfg->care_bits.pol_id = cf_key.mask.key_data0.bf.pol_id;
    cfg->care_bits.igmp_mld = cf_key.mask.key_data0.bf.igmp_pkt;
    cfg->care_bits.ipv6 = cf_key.mask.key_data0.bf.ipv6_pkt;
    cfg->care_bits.ipv4 = cf_key.mask.key_data0.bf.ipv4_pkt;
    cfg->care_bits.pkt_type = cf_key.mask.key_data0.bf.pkt_type;
    cfg->care_bits.s_tpid = cf_key.mask.key_data0.bf.stag_tpid_code;
    cfg->care_bits.ctag = cf_key.mask.key_data0.bf.ctag_pkt;
    cfg->care_bits.stag = cf_key.mask.key_data0.bf.stag_pkt;
    cfg->care_bits.lspid = cf_key.mask.key_data0.bf.lspid;
    cfg->care_bits.ldpid = cf_key.mask.key_data0.bf.ldpid;

    cfg->data_bits.valid = cf_key.data.key_data2.bf.valid;
    cfg->data_bits.ev2pt_table = cf_key.data.key_data2.bf.ether_type;
    cfg->data_bits.tos_tc = cf_key.data.key_data2.bf.tos;
    cfg->data_bits.outer_vid = cf_key.data.key_data1.bf.outer_tag & 0xFFF;
    cfg->data_bits.outer_dei_cfi = (cf_key.data.key_data1.bf.outer_tag >> 12) & 0x1;
    cfg->data_bits.outer_pri = (cf_key.data.key_data1.bf.outer_tag >> 13) & 0x7;
    cfg->data_bits.ctag_vid = cf_key.data.key_data1.bf.ctag & 0xFFF;
    cfg->data_bits.ctag_cfi = (cf_key.data.key_data1.bf.ctag >> 12) & 0x1;
    cfg->data_bits.ctag_pri = (cf_key.data.key_data1.bf.ctag >> 13) & 0x7;
    cfg->data_bits.direction = cf_key.data.key_data0.bf.stream;
    cfg->data_bits.pol_id = cf_key.data.key_data0.bf.pol_id;
    cfg->data_bits.igmp_mld = cf_key.data.key_data0.bf.igmp_pkt;
    cfg->data_bits.ipv6 = cf_key.data.key_data0.bf.ipv6_pkt;
    cfg->data_bits.ipv4 = cf_key.data.key_data0.bf.ipv4_pkt;
    cfg->data_bits.pkt_type = cf_key.data.key_data0.bf.pkt_type;
    cfg->data_bits.s_tpid = cf_key.data.key_data0.bf.stag_tpid_code;
    cfg->data_bits.ctag = cf_key.data.key_data0.bf.ctag_pkt;
    cfg->data_bits.stag = cf_key.data.key_data0.bf.stag_pkt;
    cfg->data_bits.lspid = cf_key.data.key_data0.bf.lspid;
    cfg->data_bits.ldpid = cf_key.data.key_data0.bf.ldpid;

    __CF_UNLOCK();
    return ret;
}


ca_status_t aal_l2_cf_action_set(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_IN aal_l2_cf_action_mask_t mask,
                                      CA_IN aal_l2_cf_action_t * cfg)
{
    ca_status_t ret = CA_E_OK;
    __aal_cf_fib_data_t fib_data;
    if ((entry_id >= AAL_L2_CF_MAX_RULE_ID) || (NULL == cfg)) {
       return CA_E_PARAM;
    }
    __CF_LOCK();
    __aal_l2_cf_action_read(entry_id,&fib_data);
    if(mask.s.cs_act)
    {
        fib_data.fib_data0.bf.csact = cfg->cs_act;
    }
    if(mask.s.cs_vid)
    {
        fib_data.fib_data0.bf.cs_vid = cfg->cs_vid;
    }
    if(mask.s.cs_pri)
    {
        fib_data.fib_data0.bf.cs_pri = cfg->cs_pri;
    }
    if(mask.s.cs_pri_act)
    {
        fib_data.fib_data0.bf.cspri_act = cfg->cs_pri_act;
    }
    if(mask.s.cs_vid_act)
    {
        fib_data.fib_data0.bf.csvid_act = cfg->cs_vid_act;
    }
    if(mask.s.cscfi_keep)
    {
        fib_data.fib_data0.bf.cscfi_keep = cfg->cscfi_keep;
    }
    if(mask.s.flowid_act)
    {
        fib_data.fib_data0.bf.flowid_act = cfg->flowid_act;
    }
    if(mask.s.grpid_act)
    {
        fib_data.fib_data0.bf.grpid_act = cfg->grpid_act;
    }
    if(mask.s.cf_flowid)
    {
        fib_data.fib_data0.bf.cf_flowid = cfg->cf_flowid;
        fib_data.fib_data1.bf.cf_flowid = (fib_data.fib_data1.bf.cf_flowid & 0x1c) + ((cfg->cf_flowid >> 7) & 0x3);
    }
    if(mask.s.cf_grpid)
    {
        fib_data.fib_data1.bf.cf_flowid = (fib_data.fib_data1.bf.cf_flowid & 0x3) + (cfg->cf_grpid << 2);
    }

    if(mask.s.fwd_act)
    {
        fib_data.fib_data1.bf.forward_act = cfg->fwd_act;
    }
    if(mask.s.cf_fwd_id)
    {
        fib_data.fib_data1.bf.cf_fwd_id = cfg->cf_fwd_id;
    }
    if(mask.s.cact)
    {
        fib_data.fib_data1.bf.cact = cfg->cact;
    }
    if(mask.s.cvid)
    {
        fib_data.fib_data1.bf.cvid = cfg->cvid;
    }
    if(mask.s.cpri)
    {
        fib_data.fib_data1.bf.cpri = cfg->cpri;
        fib_data.fib_data2.bf.cpri = (cfg->cpri >> 2);

    }
    if(mask.s.cvid_act)
    {
        fib_data.fib_data2.bf.cvid_act = cfg->cvid_act;
    }
    if(mask.s.cpri_act)
    {
        fib_data.fib_data2.bf.cpri_act = cfg->cpri_act;
    }
    if(mask.s.ccfi_keep)
    {
        fib_data.fib_data2.bf.ccfi_keep = cfg->ccfi_keep;
    }
    if(mask.s.cf_cos_act)
    {
        fib_data.fib_data2.bf.cf_cos_act = cfg->cf_cos_act;
    }
    if(mask.s.cf_cos)
    {
        fib_data.fib_data2.bf.cf_cos = cfg->cf_cos;
    }

    __aal_l2_cf_action_write(entry_id,&fib_data);
    __CF_UNLOCK();
    return ret;
}

ca_status_t aal_l2_cf_action_get(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_OUT aal_l2_cf_action_t * cfg)
{
    ca_status_t ret = CA_E_OK;
    __aal_cf_fib_data_t fib_data;
    if ((entry_id >= AAL_L2_CF_MAX_RULE_ID) || (NULL == cfg)) {
       return CA_E_PARAM;
    }
    __CF_LOCK();
    __aal_l2_cf_action_read(entry_id,&fib_data);
    cfg->cs_act = fib_data.fib_data0.bf.csact;
    cfg->cs_vid = fib_data.fib_data0.bf.cs_vid;
    cfg->cs_pri = fib_data.fib_data0.bf.cs_pri;
    cfg->cs_pri_act = fib_data.fib_data0.bf.cspri_act;
    cfg->cs_vid_act = fib_data.fib_data0.bf.csvid_act;
    cfg->cscfi_keep = fib_data.fib_data0.bf.cscfi_keep;
    cfg->flowid_act = fib_data.fib_data0.bf.flowid_act;
    cfg->grpid_act = fib_data.fib_data0.bf.grpid_act;
    cfg->cf_flowid = fib_data.fib_data0.bf.cf_flowid + ((fib_data.fib_data1.bf.cf_flowid & 0x3) << 7);
    cfg->cf_grpid = (fib_data.fib_data1.bf.cf_flowid >> 2) & 0x7;
    cfg->fwd_act = fib_data.fib_data1.bf.forward_act;
    cfg->cf_fwd_id = fib_data.fib_data1.bf.cf_fwd_id;
    cfg->cact = fib_data.fib_data1.bf.cact;
    cfg->cvid = fib_data.fib_data1.bf.cvid;
    cfg->cpri = fib_data.fib_data1.bf.cpri + (fib_data.fib_data2.bf.cpri << 2);
    cfg->cvid_act = fib_data.fib_data2.bf.cvid_act;
    cfg->cpri_act = fib_data.fib_data2.bf.cpri_act;
    cfg->ccfi_keep = fib_data.fib_data2.bf.ccfi_keep;
    cfg->cf_cos_act = fib_data.fib_data2.bf.cf_cos_act;
    cfg->cf_cos = fib_data.fib_data2.bf.cf_cos;
    __CF_UNLOCK();
    return ret;
}

ca_status_t aal_l2_cf_rule_add(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_IN aal_l2_cf_rule_t * cfg)
{
    ca_status_t ret = CA_E_OK;
    aal_l2_cf_rule_mask_t mask;
    if ((entry_id >= AAL_L2_CF_MAX_RULE_ID) || (NULL == cfg)) {
       return CA_E_PARAM;
    }

    memset(&mask,0xff,sizeof(aal_l2_cf_rule_mask_t));
    ret = aal_l2_cf_key_set(device_id,entry_id,mask.key,&cfg->key);
    if(ret != CA_E_OK)
        return ret;
    ret = aal_l2_cf_action_set(device_id,entry_id,mask.action,&cfg->action);
    return ret;
}

ca_status_t aal_l2_cf_rule_delete(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id)
{
    ca_status_t ret = CA_E_OK;
    aal_l2_cf_rule_t rule;
    aal_l2_cf_rule_mask_t mask;
    if ((entry_id >= AAL_L2_CF_MAX_RULE_ID)) {
       return CA_E_PARAM;
    }

    memset(&rule,0,sizeof(aal_l2_cf_rule_t));
    memset(&mask,0xff,sizeof(aal_l2_cf_rule_mask_t));
    ret = aal_l2_cf_key_set(device_id,entry_id,mask.key,&rule.key);
    if(ret != CA_E_OK)
        return ret;
    ret = aal_l2_cf_action_set(device_id,entry_id,mask.action,&rule.action);

    return ret;
}

ca_status_t aal_l2_cf_rule_get(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id,
                                      CA_OUT aal_l2_cf_rule_t * cfg)
{
    ca_status_t ret = CA_E_OK;
    if ((entry_id >= AAL_L2_CF_MAX_RULE_ID) || (NULL == cfg)) {
       return CA_E_PARAM;
    }
    ret = aal_l2_cf_key_get(device_id,entry_id,&cfg->key);
    if(ret != CA_E_OK)
        return ret;
    ret = aal_l2_cf_action_get(device_id,entry_id,&cfg->action);
    return ret;
}

ca_status_t aal_l2_cf_entry_get(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t entry_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t min, max, i;
    aal_l2_cf_rule_t rule;

    if(entry_id > AAL_L2_CF_MAX_RULE_ID)
    {
        PRINTF("show l2_cf_entry 0 <0~%d>: show entry index <0~%d>\n",AAL_L2_CF_MAX_RULE_ID-1,AAL_L2_CF_MAX_RULE_ID-1);
        PRINTF("show l2_cf_entry 0 %d: show all vaild entry\n",AAL_L2_CF_MAX_RULE_ID);
        return ret;
    }
    else if(entry_id < AAL_L2_CF_MAX_RULE_ID)
    {
        min = entry_id;
        max = entry_id;
    }
    else
    {
        min = 0;
        max = AAL_L2_CF_MAX_RULE_ID-1;
    }

    for(i=min;i<=max;i++)
    {
        memset(&rule, 0,sizeof(aal_l2_cf_rule_t));
        aal_l2_cf_rule_get(device_id,i,&rule);
        if(rule.key.data_bits.valid != TRUE && min != max)
            continue;
        PRINTF("**********************************\n");
        PRINTF("Index: %u\n",i);
        PRINTF("Vaild: %u\n",rule.key.data_bits.valid);
        PRINTF("Filter:\n");
        __aal_l2_cf_key_show(&rule.key.data_bits,&rule.key.care_bits);
        PRINTF("Action:\n");
        __aal_l2_cf_action_show(&rule.action);
    }
    return ret;
}

 ca_status_t aal_l2_cf_debug_get(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t *debug)
{
    ca_status_t ret = CA_E_OK;

    *debug=debug_val;
    
    return ret;
}

ca_status_t aal_l2_cf_debug_set(CA_IN ca_device_id_t device_id,
                                      CA_IN ca_uint32_t *debug)
{
    ca_status_t ret = CA_E_OK;

    switch(*debug)
    {
        case 0 : debug_val = (debug_val + 1) % 2;
                 break;
        case 1 : __aal_l2_cf_delete_all();
                 break;
    }
    
    return ret;
}

ca_status_t aal_l2_cf_hit_entry_get(CA_IN ca_device_id_t device_id)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    L2FE_GLB_L2FE_MONITOR_RETURN_t l2fe_monitor_return;
    ca_uint8_t i;
    ca_uint32_t index;

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel = 0x82;

    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);

    l2fe_monitor_return.wrd = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    if(l2fe_monitor_return.wrd >> 12 & 0x1)
    {
        index = (l2fe_monitor_return.wrd >> 13) & 0x1ff;
        PRINTF("CF hit entry: %d\n",index);
    }
    else
    {
        PRINTF("CF hit entry: none\n");
    }

    return CA_E_OK; 
}

void aal_l2_cf_10g_sfu_rule_init(void)
{
    struct eth_device *dev = eth_get_dev_by_name(CONFIG_IDENT_STRING);
    struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
    uint32 totalLanPort = rtk_ne_max_lan_port_num_get();
    uint32 i;

    for (i = 0; i < totalLanPort; i++) {

        if (priv->lan_port_table[i].dport == -1)
            continue;
        
        //ilpb config
        CA_NE_REG_WRITE(0x00000001, L2FE_PP_ILPB_DATA4);
        CA_NE_REG_WRITE(0x00108003, L2FE_PP_ILPB_DATA3);
        CA_NE_REG_WRITE(0x1800a363, L2FE_PP_ILPB_DATA2);
        CA_NE_REG_WRITE(0xc1800deb, L2FE_PP_ILPB_DATA1);
        CA_NE_REG_WRITE(0xc1000b80, L2FE_PP_ILPB_DATA0);
        //0xC0000006 for port 6
        //0xC0000001 for port 1
        ///--
        WRITE_INDIRCT_REG(0, (0xC0000000|priv->lan_port_table[i].dport), L2FE_PP_ILPB_ACCESS);
         
        //icls key
        CA_NE_REG_WRITE(0x00000a40, L2FE_CLE_IGR_KEY_DATA12);
        CA_NE_REG_WRITE(0x0a000000, L2FE_CLE_IGR_KEY_DATA11);
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_KEY_DATA10);
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_KEY_DATA9);
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_KEY_DATA8);
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_KEY_DATA7);
        CA_NE_REG_WRITE(0x0001fbc0, L2FE_CLE_IGR_KEY_DATA6);
        CA_NE_REG_WRITE(0x02800000, L2FE_CLE_IGR_KEY_DATA5);
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_KEY_DATA4);
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_KEY_DATA3);
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_KEY_DATA2);
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_KEY_DATA1);
        CA_NE_REG_WRITE(0x00007ef0, L2FE_CLE_IGR_KEY_DATA0);
        WRITE_INDIRCT_REG(0, 0xC0000000, L2FE_CLE_IGR_KEY_ACCESS);
         
        //icls fib
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_FIB_DATA4);
        //0x00000042 for cpu 0-0
        //0x0000004e for cpu 3-0
        CA_NE_REG_WRITE(0x0000004e, L2FE_CLE_IGR_FIB_DATA3);
        CA_NE_REG_WRITE(0x0003c000, L2FE_CLE_IGR_FIB_DATA2);
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_FIB_DATA1);
        CA_NE_REG_WRITE(0x00000000, L2FE_CLE_IGR_FIB_DATA0);
        WRITE_INDIRCT_REG(0, 0xC0000000, L2FE_CLE_IGR_FIB_ACCESS);
    }
}


ca_status_t aal_l2_cf_init(CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    #if 0
    if(CA_OSAL_OK != ca_spin_lock_init(&__aal_l2_cf_lock,SPINLOCK_BH)){
        PRINTF("%s: Fail to init l2 CF spinlock. \n", __func__);
    }
    #endif
    //delete all l2 classification
    __aal_l2_cf_delete_all();

    //init l2 classification //it will be overwrite
    //aal_l2_cf_10g_sfu_rule_init();

    return ret;
}

