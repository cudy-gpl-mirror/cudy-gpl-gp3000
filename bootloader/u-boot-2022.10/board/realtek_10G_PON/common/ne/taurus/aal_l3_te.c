#include <osal_soc.h>
#include <osal.h>
#include <aal_common.h>
#include <aal_l2_te.h>
#include <aal_l2_tm.h>
#include <aal_l3_te.h>
#include <aal_l3_te_cb.h>
#include <aal_l3_tm.h>


aal_l3_te_wred_info_t g_aal_l3_te_wred_profile_info[CA_AAL_L3_WRED_MAX_PROFILE_NUMS];
ca_uint8_t g_aal_l3_te_wred_sel_info[CA_AAL_L3_WRED_MAX_PORT_ID + 1][8];

ca_uint32_t l3_te_wred_mark_idx = 0;
ca_uint32_t l3_te_wred_unmark_idx = 0;

ca_status_t aal_l3_te_shaper_ni_port_group_set(
    ca_boolean_t enable)
{
    TE_SHP_CTRL_t value;

    value.wrd = RTK_NE_REG_READ(TE_SHP_CTRL);

    value.bf.ni_port_grp_en = enable ? 1 : 0;

    RTK_NE_REG_WRITE(TE_SHP_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_shaper_cpu_port_group_set
(
    ca_boolean_t enable
)
{
    TE_SHP_CTRL_t value;

    value.wrd = RTK_NE_REG_READ(TE_SHP_CTRL);

    value.bf.cpu_port_grp_en = enable ? 1 : 0;

    RTK_NE_REG_WRITE(TE_SHP_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_pm_enable_set
(
    ca_boolean_t enable
)
{
    TE_PM_CTRL_t value;

    value.wrd = RTK_NE_REG_READ(TE_PM_CTRL);

    value.bf.mem_disable = enable ? 0 : 1;

    RTK_NE_REG_WRITE(TE_PM_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_pm_read_clr_set
(
    ca_boolean_t read_clr
)
{
    TE_PM_CTRL_t value;

    value.wrd = RTK_NE_REG_READ(TE_PM_CTRL);

    value.bf.mem_auto_clr_on_read = read_clr;

    RTK_NE_REG_WRITE(TE_PM_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_pm_cnt_mem_set
(
    ca_uint32_t block_id,
    ca_uint32_t sel_id,
    ca_uint32_t sel_event
)
{
    TE_PM_CTRL_t value;

    if (block_id > 1)
        return CA_E_PARAM;

    if (sel_id > 1)
        return CA_E_PARAM;

    if (sel_event > 1)
        return CA_E_PARAM;

    value.wrd = RTK_NE_REG_READ(TE_PM_CTRL);

    if (block_id == 0)
    {
        value.bf.cnt0_mem_id_sel    = sel_id;
        value.bf.cnt0_mem_event_sel = sel_event;
    }
    else
    {
        value.bf.cnt1_mem_id_sel    = sel_id;
        value.bf.cnt1_mem_event_sel = sel_event;
    }

    RTK_NE_REG_WRITE(TE_PM_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_glb_cfg_set
(
    aal_l3_te_glb_cfg_msk_t msk,
    aal_l3_te_glb_cfg_t     *cfg
)
{
    TE_TE_GLB_CTRL_t ctrl;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;


    ctrl.wrd = RTK_NE_REG_READ(TE_TE_GLB_CTRL);

    if (msk.s.policer_ena)
    {
        ctrl.bf.pol_en = cfg->policer_ena;
    }

    if (msk.s.shaper_ena)
    {
        ctrl.bf.shp_en = cfg->shaper_ena;
    }

    if (msk.s.tail_drop_ena)
    {
        ctrl.bf.taildrop_en = cfg->tail_drop_ena;
    }

    if (msk.s.wred_ena)
    {
        ctrl.bf.wred_en = cfg->wred_ena;
    }

    if (msk.s.mark_ecn_ena)
    {
        ctrl.bf.mark_ecn_en = cfg->mark_ecn_ena;
    }

    if (msk.s.pol_ecn_ena)
    {
        ctrl.bf.pol_ecn_en = cfg->pol_ecn_ena;
    }

    if (msk.s.wred_ecn_ena)
    {
        ctrl.bf.wred_ecn_en = cfg->wred_ecn_ena;
    }

    if (msk.s.l2_tail_drop_ena)
    {
        ctrl.bf.l2taildrop_en = cfg->l2_tail_drop_ena;
    }

    if (msk.s.cm_selection)
    {
        ctrl.bf.cm_selection = cfg->cm_selection;
    }

    if (msk.s.sel3flows)
    {
        ctrl.bf.sel3flows = cfg->sel3flows;
    }

    if (msk.s.puc_tqm_voqbp_dis)
    {
        ctrl.bf.puc_tqm_voqbp_dis = cfg->puc_tqm_voqbp_dis;
    }

    if (msk.s.cntrcfg_clear_on_read)
    {
        ctrl.bf.cntrcfg_clear_on_read = cfg->cntrcfg_clear_on_read;
    }

    RTK_NE_REG_WRITE(TE_TE_GLB_CTRL, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_flow_cfg_set
(
    ca_flow_id_t                flow_id,
    aal_l3_te_policer_cfg_msk_t msk,
    aal_l3_te_policer_cfg_t     *cfg
)
{
    TE_POL_FLOW_GRP_CTRL0_t ctrl;

    if (flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    //addr = TE_POL_FLOW_GRP_CTRL0 + (flow_id / 32) * 4;    /* 32 flows share one configuration */

    ctrl.wrd = RTK_NE_REG_READ(TE_POL_FLOW_GRP_CTRL0 + (flow_id / 32) * 4);

    if (msk.s.type)
    {
        ctrl.bf.type = cfg->type;
    }

    if (msk.s.color_blind)
    {
        ctrl.bf.color_blind = cfg->color_blind;
    }

    if (msk.s.pol_update_mode)
    {
        ctrl.bf.pkt_mode = cfg->pol_update_mode;
    }

    if (msk.s.pkt_len_sel)
    {
        ctrl.bf.egr_len_sel = cfg->pkt_len_sel;
    }

    if (msk.s.ipg)
    {
        ctrl.bf.ipg = cfg->ipg;
    }

    if (msk.s.bypass_yellow)
    {
        ctrl.bf.bypass_yellow = cfg->bypass_yellow;
    }

    if (msk.s.bypass_red)
    {
        ctrl.bf.bypass_red = cfg->bypass_red;
    }

    if (msk.s.result_update_mode)
    {
        ctrl.bf.update_mode = cfg->result_update_mode;
    }

    if (msk.s.nest_level)
    {
        ctrl.bf.nest_level = cfg->nest_level;
    }

    RTK_NE_REG_WRITE(TE_POL_FLOW_GRP_CTRL0 + (flow_id / 32) * 4, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_port_cfg_set
(
    ca_flow_id_t                 port_id,
    aal_l3_te_policer_cfg_msk_t msk,
    aal_l3_te_policer_cfg_t     *cfg
)
{
    TE_POL_PORT_CTRL0_t ctrl;

    if (port_id > CA_AAL_L3_MAX_PORT_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    //addr = TE_POL_PORT_CTRL0 + port_id * 4;    /* 8 instances in total */

    ctrl.wrd = RTK_NE_REG_READ(TE_POL_PORT_CTRL0 + port_id * 4);

    if (msk.s.type)
    {
        ctrl.bf.type = cfg->type;
    }

    if (msk.s.color_blind)
    {
        ctrl.bf.color_blind = cfg->color_blind;
    }

    if (msk.s.pol_update_mode)
    {
        ctrl.bf.pkt_mode = cfg->pol_update_mode;
    }

    if (msk.s.pkt_len_sel)
    {
        ctrl.bf.egr_len_sel = cfg->pkt_len_sel;
    }

    if (msk.s.ipg)
    {
        ctrl.bf.ipg = cfg->ipg;
    }

    if (msk.s.bypass_yellow)
    {
        ctrl.bf.bypass_yellow = cfg->bypass_yellow;
    }

    if (msk.s.bypass_red)
    {
        ctrl.bf.bypass_red = cfg->bypass_red;
    }

    if (msk.s.result_update_mode)
    {
        ctrl.bf.update_mode = cfg->result_update_mode;
    }

    if (msk.s.nest_level)
    {
        ctrl.bf.nest_level = cfg->nest_level;
    }

    RTK_NE_REG_WRITE(TE_POL_PORT_CTRL0 + port_id * 4, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_pkt_type_cfg_set
(
    aal_l3_te_policer_pkt_type_t pkt_type,
    aal_l3_te_policer_cfg_msk_t  msk,
    aal_l3_te_policer_cfg_t      *cfg
)
{
    TE_POL_PKT_TYPE_CTRL0_t ctrl;

    if (pkt_type >= CA_AAL_L3_POLICER_PKT_TYPE_END)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    //addr = TE_POL_PKT_TYPE_CTRL0 + (ca_uint32_t)pkt_type * 4;    /* 8 instances in total */

    ctrl.wrd = RTK_NE_REG_READ(TE_POL_PKT_TYPE_CTRL0 + (ca_uint32_t)pkt_type * 4);

    if (msk.s.type)
    {
        ctrl.bf.type = cfg->type;
    }

    if (msk.s.color_blind)
    {
        ctrl.bf.color_blind = cfg->color_blind;
    }

    if (msk.s.pol_update_mode)
    {
        ctrl.bf.pkt_mode = cfg->pol_update_mode;
    }

    if (msk.s.pkt_len_sel)
    {
        ctrl.bf.egr_len_sel = cfg->pkt_len_sel;
    }

    if (msk.s.ipg)
    {
        ctrl.bf.ipg = cfg->ipg;
    }

    if (msk.s.bypass_yellow)
    {
        ctrl.bf.bypass_yellow = cfg->bypass_yellow;
    }

    if (msk.s.bypass_red)
    {
        ctrl.bf.bypass_red = cfg->bypass_red;
    }

    if (msk.s.result_update_mode)
    {
        ctrl.bf.update_mode = cfg->result_update_mode;
    }

    if (msk.s.nest_level)
    {
        ctrl.bf.nest_level = cfg->nest_level;
    }

    RTK_NE_REG_WRITE(TE_POL_PKT_TYPE_CTRL0 + (ca_uint32_t)pkt_type * 4, ctrl.wrd);

    return CA_E_OK;
}

static inline ca_status_t __l3_te_shaper_port_tbc_access(
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1
)
{
    TE_SHP_PORT_TBC_MEM_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = RTK_NE_REG_READ(TE_SHP_PORT_TBC_MEM_ACCESS);

    while (access.bf.access && (i < AAL_TBL_OP_TIMEOUT))
    {
        access.wrd = RTK_NE_REG_READ(TE_SHP_PORT_TBC_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        RTK_NE_REG_WRITE(TE_SHP_PORT_TBC_MEM_DATA0, *data0);
        RTK_NE_REG_WRITE(TE_SHP_PORT_TBC_MEM_DATA1, *data1);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    RTK_NE_REG_WRITE(TE_SHP_PORT_TBC_MEM_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = RTK_NE_REG_READ(TE_SHP_PORT_TBC_MEM_ACCESS);
        while (access.bf.access)
        {
            access.wrd = RTK_NE_REG_READ(TE_SHP_PORT_TBC_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = RTK_NE_REG_READ(TE_SHP_PORT_TBC_MEM_DATA0);
        *data1 = RTK_NE_REG_READ(TE_SHP_PORT_TBC_MEM_DATA1);
    }

    return CA_E_OK;
}

/****************************************************************
** SHP_CTRL.cpu_port_grp_en is 0, entry 0 ~ 7 for 8 CPU ports
** SHP_CTRL.cpu_port_grp_en is 1, entry 0 for port group shaper
** SHP_CTRL.ni_port_grp_en  is 0, entry 8 ~ 15 for 8 NI ports
** SHP_CTRL.ni_port_grp_en  is 1, entry 0 for ni group shaper
****************************************************************/
ca_status_t aal_l3_te_shaper_port_tbc_set(
    ca_port_id_t                port_id,
    aal_l3_te_shaper_tbc_cfg_msk_t msk,
    aal_l3_te_shaper_tbc_cfg_t     *cfg)
{
    TE_SHP_PORT_TBC_MEM_DATA0_t value0;
    TE_SHP_PORT_TBC_MEM_DATA1_t value1;
    ca_uint32_t index = port_id;

    if (!cfg)
        return CA_E_PARAM;

    if (port_id > AAL_L3TE_MAX_PORT_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_shaper_port_tbc_access(
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;


    if (msk.s.rate_k)
    {
        if (cfg->rate_k > CA_AAL_L3_TE_SHP_MAX_RATE_K)
            return CA_E_PARAM;

        value0.bf.rate_k = cfg->rate_k;
    }

    if (msk.s.rate_m)
    {
        if (cfg->rate_m > CA_AAL_L3_TE_SHP_MAX_RATE_M)
            return CA_E_PARAM;

        value0.bf.rate_m = cfg->rate_m;
    }

    if (msk.s.bs)
    {
        if (cfg->bs > CA_AAL_L3_TE_SHP_MAX_BS || cfg->bs == 0)
            return CA_E_PARAM;

    if (cfg->mode == CA_AAL_SHAPER_MODE_BPS)
        {
            if (cfg->bs < ((cfg->rate_m + 511) >> 9))
                return CA_E_PARAM;
        }

        if (cfg->mode == CA_AAL_SHAPER_MODE_PPS)
        {
            if (cfg->bs < ((cfg->rate_m + 1) >> 1))
                return CA_E_PARAM;
        }
        
        value0.bf.bs = cfg->bs & 0x3f;
        value1.bf.bs = (cfg->bs >> 6) & 0x3f;
    }

    if (msk.s.state)
    {
        if (cfg->state >= CA_AAL_L3_SHAPER_STATE_END)
            return CA_E_PARAM;

        value1.bf.enb = cfg->state;
    }

    if (msk.s.mode)
    {
        if (cfg->mode >= CA_AAL_L3_SHAPER_MODE_END)
            return CA_E_PARAM;

        value1.bf.mode = cfg->mode;
    }

    if (msk.s.tbc_count)
    {
        value1.bf.tbc = cfg->tbc_count;
    }

    /* update packet type based policing table */
    return __l3_te_shaper_port_tbc_access(
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd));
}

ca_boolean aal_l3_te_wred_profile_info_allocate
(
    ca_uint32_t *index
)
{
    ca_uint32_t i = 0;

    if (!index)
        return FALSE;

    for (i=0; i<CA_AAL_L3_WRED_MAX_PROFILE_NUMS; i++)
    {
        if (g_aal_l3_te_wred_profile_info[i].ref_counter == 0)
        {
            *index = i;

            return TRUE;
        }
    }

    return FALSE;
}

void aal_l3_te_wred_profile_info_ref_inc(ca_uint32_t index)
{
    g_aal_l3_te_wred_profile_info[index].ref_counter++;
}

void aal_l3_te_wred_sel_info_set
(
    ca_port_id_t port,
    ca_uint32_t  queue,
    ca_uint32    index
)
{
    if (port <= CA_AAL_L3_WRED_MAX_PORT_ID && queue <= 7)
        g_aal_l3_te_wred_sel_info[port][queue] = index;
}

static inline ca_status_t __l3_te_wred_free_global_mem_size_access
(
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0
)
{
    TE_WRED_AFGS_MEM_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = RTK_NE_REG_READ(TE_WRED_AFGS_MEM_ACCESS);

    while (access.bf.access && (i < AAL_TBL_OP_TIMEOUT))
    {
        access.wrd = RTK_NE_REG_READ(TE_WRED_AFGS_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        RTK_NE_REG_WRITE(TE_WRED_AFGS_MEM_DATA, *data0);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    RTK_NE_REG_WRITE(TE_WRED_AFGS_MEM_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = RTK_NE_REG_READ(TE_WRED_AFGS_MEM_ACCESS);
        while (access.bf.access)
        {
            access.wrd = RTK_NE_REG_READ(TE_WRED_AFGS_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = RTK_NE_REG_READ(TE_WRED_AFGS_MEM_DATA);
    }

    return CA_E_OK;

}

static inline ca_status_t __l3_te_pie_lpid_qos_voq_access
(
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data
)
{
    TE_PIE_LPID_QOS_VOQ_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = RTK_NE_REG_READ(TE_PIE_LPID_QOS_VOQ_ACCESS);

    while (access.bf.access && (i < AAL_TBL_OP_TIMEOUT))
    {
        access.wrd = RTK_NE_REG_READ(TE_PIE_LPID_QOS_VOQ_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        RTK_NE_REG_WRITE(TE_PIE_LPID_QOS_VOQ_DATA, *data);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    RTK_NE_REG_WRITE(TE_PIE_LPID_QOS_VOQ_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = RTK_NE_REG_READ(TE_PIE_LPID_QOS_VOQ_ACCESS);
        while (access.bf.access)
        {
            access.wrd = RTK_NE_REG_READ(TE_PIE_LPID_QOS_VOQ_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data = RTK_NE_REG_READ(TE_PIE_LPID_QOS_VOQ_DATA);
    }

    return CA_E_OK;
}

ca_status_t aal_l3_te_pie_lpid_qos_voq_set
(
    ca_port_id_t                     lpid,
    ca_uint32_t                      cos,
    aal_l3_te_pie_lpid_qos_voq_msk_t msk,
    aal_l3_te_pie_lpid_qos_voq_t     *cfg
)
{
    TE_PIE_LPID_QOS_VOQ_DATA_t value0;
    ca_uint32_t index = (lpid << 3 | cos);

    if (!cfg)
        return CA_E_PARAM;

    if (cos > 7)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_pie_lpid_qos_voq_access(
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd))
            != CA_E_OK)
        return CA_E_ERROR;


    if (msk.s.ena)
    {
        value0.bf.value = cfg->ena << 4;
    }

    if (msk.s.voq)
    {
        if (cfg->voq > CA_AAL_L3_PIE_MAX_VOQ)
            return CA_E_PARAM;

        value0.bf.value = (value0.bf.value & (~0xf)) | cfg->voq;
    }

    /* update packet type based policing table */
    return __l3_te_pie_lpid_qos_voq_access(
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd));
}

ca_status_t aal_l3_te_glb_match_set
(
    CA_IN ca_uint32_t                  pie_voq,
    CA_IN aal_l3_te_glb_match_msk_t    msk,
    CA_IN aal_l3_te_glb_match_t        *cfg
)
{
    TE_TE_GLB_MATCH0_t val;
    ca_uint32_t addr = 0;

    if (!cfg)
        return CA_E_PARAM;

    if (pie_voq > TE_TE_GLB_MATCH0_COUNT)
        return CA_E_PARAM;

    addr = TE_TE_GLB_MATCH0 + pie_voq * TE_TE_GLB_MATCH0_STRIDE;
    val.wrd = RTK_NE_REG_READ(addr);

    if (msk.s.ena)
    {
        val.bf.enable_voq_match = cfg->ena;
    }

    if (msk.s.ldpid_msk)
    {
        val.bf.ldpid_voq_mask = cfg->ldpid_msk;
    }

    if (msk.s.ldpid)
    {
        val.bf.ldpid_voq_match = cfg->ldpid;
    }

    if (msk.s.cos_msk)
    {
        val.bf.cos_voq_mask = cfg->cos_msk;
    }

    if (msk.s.cos)
    {
        val.bf.cos_voq_match = cfg->cos;
    }

    RTK_NE_REG_WRITE(addr, val.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_glb_pie_wred_port_enable_set
(
    CA_IN ca_uint8_t     zone,
    CA_IN ca_uint32_t    en
)
{
    TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0_t value;

    if (zone > 0x3)
        return CA_E_PARAM;

    value.wrd = RTK_NE_REG_READ(TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0 + zone * TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0_STRIDE);

    value.bf.en = en;

    RTK_NE_REG_WRITE(TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0 + zone * TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0_STRIDE, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_pie_reset
(
)
{
    ca_status_t ret;
    ca_uint32_t i = 0, j = 0, index = 0;
    ca_uint32_t value=0;
    aal_l3_te_pie_lpid_qos_voq_msk_t msk;
    aal_l3_te_pie_lpid_qos_voq_t cfg;
    aal_l3_te_glb_match_msk_t glb_msk;
    aal_l3_te_glb_match_t glb_cfg;

    memset((void *)&msk, 0, sizeof(msk));
    memset((void *)&cfg, 0, sizeof(cfg));
    memset((void *)&glb_msk, 0, sizeof(glb_msk));
    memset((void *)&glb_cfg, 0, sizeof(glb_cfg));

    msk.s.ena=1;
    msk.s.voq=1;

    glb_msk.s.ena=1;
    glb_msk.s.ldpid_msk=1;
    glb_msk.s.ldpid=1;
    glb_msk.s.cos_msk=1;
    glb_msk.s.cos=1;

    for (i=0; i<=AAL_L3TE_PORT_NUM; i++){
        for (j=0; j<8; j++){
            ret=aal_l3_te_pie_lpid_qos_voq_set(i, j, msk, &cfg);
            if(ret!=CA_E_OK)PRINTF("Error!ldpid=%d cos=%d ret=%d.\n",i,j,ret);
        }
    }

    for(index=0;index<CA_AAL_L3_PIE_MAX_VOQ;index++){
        ret=aal_l3_te_glb_match_set(index, glb_msk, &glb_cfg);
        if(ret!=CA_E_OK)PRINTF("Error! index=%d, ret=%d.\n",ret);
    }

    for(index=0;index<TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0_COUNT;index++){
        ret=aal_l3_te_glb_pie_wred_port_enable_set(index, value);
            if(ret!=CA_E_OK)PRINTF("Error! index=%d, ret=%d.\n",ret);
    }

    //reset all pie registers
    do{
        TE_PIE_buf_t buf;
        TE_PIE_dp0_t dp0;
        TE_PIE_dp1_t dp1;
        TE_PIE_dp2_0_t dp2;
        TE_PIE_dp3_0_t dp3;
        TE_PIE_dp4_0_t dp4;
        TE_PIE_dp5_0_t dp5;
        TE_PIE_dp6_0_t dp6;
        TE_PIE_dp8_0_t dp8;
        TE_PIE_en0_t en0;
        TE_PIE_en1_t en1;
        TE_PIE_en2_t en2;
        TE_PIE_en3_t en3;
        TE_PIE_en4_t en4;
        TE_PIE_mean_t mean;
        TE_PIE_pie_t pie;
        TE_PIE_te_t te;             //1
        TE_PIE_timer0_t timer0;     //1
        TE_PIE_timer1_t timer1;     //1
        TE_PIE_timer2_t timer2;
        TE_PIE_dbg0_t dbg0;         //0~18
        TE_PIE_dbg_sel_t dsel;

        buf.bf.one_third=TE_PIE_buf_dft;
        dp0.bf.A=TE_PIE_dp0_dft&0x3f;
        dp0.bf.B=TE_PIE_dp0_dft>>6;
        dp1.bf.ceiling=TE_PIE_dp1_dft;
        dp2.bf.burst_reset_timeout=TE_PIE_dp2_0_dft;
        dp3.bf.latency_high=TE_PIE_dp3_0_dft;
        dp4.bf.latency_low=TE_PIE_dp4_0_dft;
        dp5.bf.latency_target=TE_PIE_dp5_0_dft;
        dp6.bf.msr=TE_PIE_dp6_0_dft;
        dp8.bf.peak_rate=TE_PIE_dp8_0_dft;
        en0.bf.high=TE_PIE_en0_dft;
        en1.bf.low=TE_PIE_en1_dft;
        en2.bf.random_disable=TE_PIE_en2_dft;
        en3.bf.threshold=TE_PIE_en3_dft;
        en4.bf.burst=TE_PIE_en4_dft;
        mean.bf.size=TE_PIE_mean_dft;
        pie.bf.q_full=TE_PIE_pie_dft;
        te.bf.schif_tbc_mode=TE_PIE_te_dft;
        timer0.bf.scalar=TE_PIE_timer0_dft;
        timer1.bf.interval=TE_PIE_timer1_dft;
        timer2.bf.burst_decrement=TE_PIE_timer2_dft;
        dbg0.wrd=0x0;
        dsel.wrd=0x0;

        RTK_NE_REG_WRITE(TE_PIE_te, te.wrd);
        RTK_NE_REG_WRITE(TE_PIE_timer0, timer0.wrd);
        RTK_NE_REG_WRITE(TE_PIE_timer1, timer1.wrd);
        RTK_NE_REG_WRITE(TE_PIE_en2, en2.wrd);
        RTK_NE_REG_WRITE(TE_PIE_en3, en3.wrd);

        for(i=0;i<3;i++){
            RTK_NE_REG_WRITE(TE_PIE_buf+i*4, buf.wrd);
            RTK_NE_REG_WRITE(TE_PIE_dp0+i*4, dp0.wrd);
            RTK_NE_REG_WRITE(TE_PIE_dp1+i*4, dp1.wrd);
            RTK_NE_REG_WRITE(TE_PIE_en0+i*4, en0.wrd);
            RTK_NE_REG_WRITE(TE_PIE_en1+i*4, en1.wrd);
            RTK_NE_REG_WRITE(TE_PIE_en4+i*4, en4.wrd);
            RTK_NE_REG_WRITE(TE_PIE_mean+i*4, mean.wrd);
            RTK_NE_REG_WRITE(TE_PIE_pie+i*4, pie.wrd);
            RTK_NE_REG_WRITE(TE_PIE_timer2+i*4, timer2.wrd);
        }

        for(i=0;i<=CA_AAL_L3_PIE_MAX_VOQ;i++){
            for(j=0;j<3;j++){
                RTK_NE_REG_WRITE(TE_PIE_dp2_0+j*4+i*TE_PIE_dp2_0_STRIDE, dp2.wrd);
                RTK_NE_REG_WRITE(TE_PIE_dp3_0+j*4+i*TE_PIE_dp3_0_STRIDE, dp3.wrd);
                RTK_NE_REG_WRITE(TE_PIE_dp4_0+j*4+i*TE_PIE_dp4_0_STRIDE, dp4.wrd);
                RTK_NE_REG_WRITE(TE_PIE_dp5_0+j*4+i*TE_PIE_dp5_0_STRIDE, dp5.wrd);
            }
            RTK_NE_REG_WRITE(TE_PIE_dp6_0+i*TE_PIE_dp6_0_STRIDE, dp6.wrd);
            RTK_NE_REG_WRITE(TE_PIE_dp8_0+i*TE_PIE_dp8_0_STRIDE, dp8.wrd);
        }

        for(j=0;j<3;j++){
            dsel.bf.sel=j;
            RTK_NE_REG_WRITE(TE_PIE_dbg_sel, dsel.wrd);
            for(i=0;i<19;i++){
                RTK_NE_REG_WRITE(TE_PIE_dbg0+i*4, dbg0.wrd);
            }
        }
        RTK_NE_REG_WRITE(TE_PIE_dbg_sel, TE_PIE_dbg_sel_dft);
    }while(0);

    return CA_E_OK;
}

ca_status_t aal_l3_te_wred_global_free_mem_size_set
(
    ca_uint32_t  pool_id,
    ca_uint32_t  depth,
    ca_uint32_t  depth_eq1
)
{
    TE_WRED_AFGS_MEM_DATA_t value0;
    ca_uint32_t index = pool_id;

    if (pool_id >= CA_AAL_L3_WRED_MAX_GLOBAL_POOL_NUMS)
        return CA_E_PARAM;

    if (depth > CA_AAL_L3_WRED_MAX_GLOBAL_BUFF_SIZE ||
        depth_eq1 > CA_AAL_L3_WRED_MAX_GLOBAL_BUFF_SIZE)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));

    value0.bf.depth = depth;
    value0.bf.depth_eq1 = depth_eq1;

    /* update table */
    return __l3_te_wred_free_global_mem_size_access(
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd));
}

/*
** L3 TE module initialization function
*/
ca_status_t aal_l3_te_init
(
)
{
    aal_l3_te_glb_cfg_t         te_cfg;
    aal_l3_te_glb_cfg_msk_t     te_msk;
    aal_l3_te_policer_cfg_msk_t l3_policer_msk;
    aal_l3_te_policer_cfg_t     l3_policer_cfg;
    ca_uint32_t i = 0;

    /*
    ** disable l3 shaper port group function, each 8 cpu ports  and 8 NI ports
    ** shall have individual port shaper
    */
    //aal_scfg_read(CFG_ID_L3TE_WRED_MARK_IDX, 4, &l3_te_wred_mark_idx);
    //aal_scfg_read(CFG_ID_L3TE_WRED_UNMARK_IDX, 4, &l3_te_wred_unmark_idx);
    //INT             CFG_ID_L3TE_WRED_MARK_IDX                               = 8;
    l3_te_wred_mark_idx = 8;
    //INT             CFG_ID_L3TE_WRED_UNMARK_IDX                             = 8;
    l3_te_wred_unmark_idx = 8;
    (void)aal_l3_te_shaper_ni_port_group_set(FALSE);
    (void)aal_l3_te_shaper_cpu_port_group_set(FALSE);

    /* statistics counter settings */
    (void)aal_l3_te_pm_enable_set(TRUE);
    (void)aal_l3_te_pm_read_clr_set(TRUE);
    aal_l3_te_pm_cnt_mem_set(0, 0, 0);
    aal_l3_te_pm_cnt_mem_set(1, 1, 1);

    /* enable wred, policer, shaper, taildrop etc.*/
    memset((void *)&te_cfg, 0, sizeof(te_cfg));
    te_msk.u32 = 0;

    te_msk.s.policer_ena   = 1;
    te_msk.s.shaper_ena    = 1;
    te_msk.s.tail_drop_ena = 1;
    te_msk.s.wred_ena      = 1;
    te_msk.s.mark_ecn_ena  = 1;
    te_msk.s.pol_ecn_ena   = 1;
    te_msk.s.wred_ecn_ena  = 1;
    te_msk.s.l2_tail_drop_ena = 1;
	te_msk.s.cm_selection  = 1;

    te_cfg.policer_ena   = CA_TRUE;
    te_cfg.shaper_ena    = CA_TRUE;
    te_cfg.tail_drop_ena = CA_TRUE;
    te_cfg.wred_ena      = CA_FALSE;
    te_cfg.mark_ecn_ena  = CA_FALSE;
    te_cfg.pol_ecn_ena   = CA_FALSE;
    te_cfg.wred_ecn_ena  = CA_FALSE;
    te_cfg.l2_tail_drop_ena = CA_TRUE;
	te_cfg.cm_selection  = CA_FALSE;

    //te_cfg need to zero to prevent pkt drop by l2fe
    (void)aal_l3_te_glb_cfg_set(te_msk, &te_cfg);

    /* policer settings */
    memset((void *)&l3_policer_cfg, 0, sizeof(l3_policer_cfg));
    memset((void *)&l3_policer_msk, 0, sizeof(l3_policer_msk));

    l3_policer_msk.s.ipg = 1;
    l3_policer_cfg.ipg   = 24;  /* IPG 20 to 24, bug 50074 */
    for (i=0; i<=CA_AAL_L3_MAX_FLOW_ID; i++)
    {
        if (aal_l3_te_policer_flow_cfg_set(i, l3_policer_msk, &l3_policer_cfg) != CA_E_OK)
            CA_TM_LOG("aal_l3_te_policer_flow_cfg_set() : failed to set policer configurations for l3 flow %d, L%d\n", i, __LINE__);
    }

    for (i=0; i<=CA_AAL_L3_MAX_PORT_ID; i++)
    {
        if (aal_l3_te_policer_port_cfg_set(i, l3_policer_msk, &l3_policer_cfg) != CA_E_OK)
            CA_TM_LOG("aal_l3_te_policer_port_cfg_set() : failed to set policer configurations for l3 port %d, L%d\n", i, __LINE__);
    }

    for (i=0; i<CA_AAL_L3_POLICER_PKT_TYPE_END; i++)
    {
        if (aal_l3_te_policer_pkt_type_cfg_set(i, l3_policer_msk, &l3_policer_cfg) != CA_E_OK)
            CA_TM_LOG("aal_l3_te_policer_pkt_type_cfg_set() : failed to set policer configurations for l3 packet type %d, L%d\n", i, __LINE__);
    }

	/*default enable TE shaper to control QM burst*/
	do {
		aal_l3_te_shaper_tbc_cfg_msk_t shp_msk;
		aal_l3_te_shaper_tbc_cfg_t shp;
		shp_msk.u32 = 0;
		memset(&shp, 0, sizeof(shp));
		
		for(i = 8; i<AAL_L3TE_PORT_NUM; i++) {
			shp_msk.s.bs = 1;
			shp_msk.s.state = 1;
			shp.state = CA_AAL_L3_SHAPER_ADMIN_STATE_SHAPER;
			switch(i)
			{
#if !defined(CONFIG_RTL8224_SUPPORT)
				case 8 ... 11: 
					shp.bs = 3;
					break;
				case 12:
#else
				// 4 * 2.5G
				case 8 ... 12: 
#endif
					shp.bs = 6;
					break;
				default:
					shp.bs = 20;
					break;
			}
			aal_l3_te_shaper_port_tbc_set(i, shp_msk, &shp);
		}
		shp_msk.u32 = 0;
		for(i = 8; i<AAL_L3TE_PORT_NUM; i++) {
			shp_msk.s.rate_m = 1;
			switch(i) 
			{
#if !defined(CONFIG_RTL8224_SUPPORT)
				case 8 ... 11: 
					shp.rate_m = 1536;
					break;
				case 12:
#else
				// 4 * 2.5G
				case 8 ... 12: 
#endif
					shp.rate_m = 3072;
					break;
				default:
					shp.rate_m = 10240;
					break;
			}
			aal_l3_te_shaper_port_tbc_set(i, shp_msk, &shp);
		}
	} while(0);	

    /* reset WRED software table */
    memset((void *)&g_aal_l3_te_wred_profile_info[0], 0x0, sizeof(g_aal_l3_te_wred_profile_info));
    memset((void *)&g_aal_l3_te_wred_sel_info[0][0], 0xff, sizeof(g_aal_l3_te_wred_sel_info));


    PRINTF("L3_TE : initialize WRED profile & selection table ... ");
    do
    {
        /* setup WRED profile and selection table */
        ca_uint32_t i = 0, j = 0, index = 0;

        if (aal_l3_te_wred_profile_info_allocate(&index))
        {
            for (i=0; i<=CA_AAL_L3_WRED_MAX_PORT_ID; i++)
            {
                for (j=0; j<8; j++)
                {
                    aal_l3_te_wred_profile_info_ref_inc(index); /* reference counter ++ */

                    /* update software selection table */
                    aal_l3_te_wred_sel_info_set(i, j, index);
                }
            }

            PRINTF("done.\n");
        }
        else
        {
            PRINTF("failed.\n");
        }

		//reset global free memory size register
		for (i=0; i<=CA_AAL_L3_WRED_MAX_GLOBAL_POOL_NUMS; i++)
			aal_l3_te_wred_global_free_mem_size_set(i, 0, 0);

		//enable CB scan
		do{
			aal_l3_te_cb_ctrl_mask_t mask;
			aal_l3_te_cb_ctrl_t cfg;

			memset((void *)&mask, 0, sizeof(mask));
			memset((void *)&cfg, 0, sizeof(cfg));

			mask.s.scan_ena=1;
			cfg.scan_ena=1;
			aal_l3_te_cb_ctrl_set(0, mask, &cfg);
		}while(0);
    }while(0);

	PRINTF("L3_TE : initialize PIE ... ");
    aal_l3_te_pie_reset();
	PRINTF("done.\n");

	do{
	    TE_FC_CTRL_t te_fc_ctrl;

	    te_fc_ctrl.wrd = CA_DEV_NE_REG_READ(0, TE_FC_CTRL);
	//    te_fc_ctrl.bf.tx_en = 1;
	    te_fc_ctrl.bf.rx_en = 1;
	    RTK_NE_REG_WRITE(TE_FC_CTRL, te_fc_ctrl.wrd);
	    RTK_NE_REG_WRITE(TE_FC_CTRL + sizeof(APB0_TE_FC_t)/4, te_fc_ctrl.wrd);
	}while(0);

    return CA_E_OK;
}