#include <osal_soc.h>
#include <osal.h>
///--
#include <osal_type.h>
#include <aal_common.h>
#include <aal_l2_te.h>
#include <aal_l2_tm.h>
#include <aal_l3_te.h>
#include <aal_l3_te_cb.h>
#include <aal_l3_tm.h>
#include <aal_scfg.h>


aal_l3_te_wred_info_t g_aal_l3_te_wred_profile_info[CA_AAL_L3_WRED_MAX_PROFILE_NUMS];
ca_uint8_t g_aal_l3_te_wred_sel_info[CA_AAL_L3_WRED_MAX_PORT_ID + 1][8];

ca_uint32_t l3_te_wred_mark_idx = 0;
ca_uint32_t l3_te_wred_unmark_idx = 0;

#ifdef __L3_TE_GLB
#endif

ca_status_t aal_l3_te_glb_cfg_set
(
    ca_device_id_t             device_id,
    aal_l3_te_glb_cfg_msk_t msk,
    aal_l3_te_glb_cfg_t     *cfg
)
{
    TE_TE_GLB_CTRL_t ctrl;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;


    ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_CTRL);

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

    CA_DEV_NE_REG_WRITE2(device_id, TE_TE_GLB_CTRL, ctrl.wrd);

    return CA_E_OK;
}


ca_status_t aal_l3_te_glb_cfg_get
(
    ca_device_id_t         device_id,
    aal_l3_te_glb_cfg_t *cfg
)
{
    TE_TE_GLB_CTRL_t ctrl;


    if (cfg == NULL)
        return CA_E_PARAM;

    ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_CTRL);

    cfg->policer_ena   = ctrl.bf.pol_en;
    cfg->shaper_ena    = ctrl.bf.shp_en;
    cfg->tail_drop_ena = ctrl.bf.taildrop_en;
    cfg->wred_ena      = ctrl.bf.wred_en;
    cfg->mark_ecn_ena  = ctrl.bf.mark_ecn_en;
    cfg->pol_ecn_ena   = ctrl.bf.pol_ecn_en;
    cfg->wred_ecn_ena  = ctrl.bf.wred_ecn_en;
    cfg->l2_tail_drop_ena = ctrl.bf.l2taildrop_en;
    cfg->cm_selection = ctrl.bf.cm_selection;
    cfg->sel3flows = ctrl.bf.sel3flows;
    cfg->puc_tqm_voqbp_dis = ctrl.bf.puc_tqm_voqbp_dis;
    cfg->cntrcfg_clear_on_read = ctrl.bf.cntrcfg_clear_on_read;

    return CA_E_OK;
}

ca_status_t aal_l3_te_glb_match_set
(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint32_t                  profile,
    CA_IN aal_l3_te_glb_match_msk_t    msk,
    CA_IN aal_l3_te_glb_match_t        *cfg
)
{
    TE_TE_GLB_MATCH0_t val;
    ca_uint32_t addr = 0;

    if (!cfg)
        return CA_E_PARAM;

    if (profile > TE_TE_GLB_MATCH0_COUNT)
        return CA_E_PARAM;

    addr = TE_TE_GLB_MATCH0 + profile * TE_TE_GLB_MATCH0_STRIDE;
    val.wrd = CA_DEV_NE_REG_READ(device_id,addr);

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

    CA_DEV_NE_REG_WRITE2(device_id,addr,val.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_glb_match_get
(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  ca_uint32_t              profile,
    CA_OUT aal_l3_te_glb_match_t    *cfg
)
{
    TE_TE_GLB_MATCH0_t val;

    if (!cfg)
        return CA_E_PARAM;

    if (profile > TE_TE_GLB_MATCH0_COUNT)
        return CA_E_PARAM;

    val.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_MATCH0 + profile * TE_TE_GLB_MATCH0_STRIDE);

    cfg->ena = val.bf.enable_voq_match;
    cfg->ldpid_msk = val.bf.ldpid_voq_mask;
    cfg->cos_msk = val.bf.cos_voq_mask;
    cfg->ldpid = val.bf.ldpid_voq_match;
    cfg->cos = val.bf.cos_voq_match;

    return CA_E_OK;
}

ca_status_t aal_l3_te_glb_pie_wred_port_enable_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t     zone,
    CA_IN ca_uint32_t    en
)
{
    TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0_t value;

    if (zone > 0x3)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0 + zone * TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0_STRIDE);

    value.bf.en = en;

    CA_DEV_NE_REG_WRITE2(device_id,TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0 + zone * TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0_STRIDE, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_glb_pie_wred_port_enable_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t     zone,
    CA_OUT ca_uint32_t    *en
)
{
    TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0_t value;

    if (!en)
        return CA_E_PARAM;

    if (zone > 0x3)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0 + zone * TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0_STRIDE);

    *en = value.bf.en;

    return CA_E_OK;
}

ca_status_t aal_l3_te_flow_control_set
(
    CA_IN ca_device_id_t              device_id,
    CA_IN aal_l3_te_flow_ctrl_mask_t  mask,
    CA_IN aal_l3_te_flow_ctrl_t       *cfg
)
{
 
    TE_FC_GLB_CTRL_t glb_ctrl;
    TE_FC_CTRL_t  ctrl;
    if(cfg == NULL){
        return CA_E_PARAM;
    }
    glb_ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_FC_GLB_CTRL);
    if(mask.s.tx_wan_ena){
        glb_ctrl.bf.l3fewan_tx_en = cfg->tx_wan_ena;
        ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_FC_CTRL+ 0x40);
        ctrl.bf.tx_en = cfg->tx_wan_ena;        
        CA_DEV_NE_REG_WRITE2(device_id,TE_FC_CTRL + 0x40, ctrl.wrd);
    }
    if(mask.s.tx_lan_ena){
        glb_ctrl.bf.l3felan_tx_en = cfg->tx_lan_ena;
        ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_FC_CTRL);
        ctrl.bf.tx_en = cfg->tx_lan_ena;        
        CA_DEV_NE_REG_WRITE2(device_id,TE_FC_CTRL, ctrl.wrd);
    }
    CA_DEV_NE_REG_WRITE2(device_id,TE_FC_GLB_CTRL, glb_ctrl.wrd);
    
    return CA_E_OK;
}

ca_status_t aal_l3_te_flow_control_get
(
    CA_IN ca_device_id_t              device_id,
    CA_OUT aal_l3_te_flow_ctrl_t       *cfg
)
{
 
    TE_FC_GLB_CTRL_t ctrl;
    if(cfg == NULL){
        return CA_E_PARAM;
    }
    ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_FC_GLB_CTRL);
    cfg->tx_wan_ena   =   ctrl.bf.l3fewan_tx_en; 
    cfg->tx_lan_ena   =   ctrl.bf.l3felan_tx_en;
    return CA_E_OK;
}

ca_status_t aal_l3_te_sp_base_flow_control_get(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    
    ca_uint32_t                             port = 0;
    ca_uint32_t                             buf_cnt[TE_CB_SP_BASE_BP_LPID_CNT_EN_P0_LSB_COUNT];
    L3FE_STG0_CTRL_t                        l3fe_ctrl;
    L2TM_L2TM_BM_TM_TO_LANL3FE_QUEUE_MAP_t  lanl3fe_qmap;
    L2TM_L2TE_FC_GLB_CTRL_LANL3FE_t         lanl3fe_ctrl;
    L2TM_L2TE_FC_GLB_STS_t                  lanl3fe_rxpfc_sts;
    TE_CB_SRC_USED_BUF_CNT01_t              src01_used_buf_cnt;
    TE_CB_SRC_USED_BUF_CNT23_t              src23_used_buf_cnt;
    TE_CB_SRC_USED_BUF_CNT45_t              src45_used_buf_cnt;
    TE_CB_SRC_USED_BUF_CNT6_t               src6_used_buf_cnt;
    TE_CB_SP_BASE_BP_THRSH_P0_t             sp_bp_threshold;
    TE_CB_SP_BASE_BP_LPID_CNT_EN_P0_LSB_t   sp_bp_ldpid_cnt_en_lsb;
    TE_CB_SP_BASE_BP_LPID_CNT_EN_P0_MSB_t   sp_bp_ldpid_cnt_en_msb;
    ca_uint64_t                             sp_bp_ldpid_cnt_en;

    l3fe_ctrl.wrd = CA_NE_REG_READ(L3FE_STG0_CTRL);
    PRINTF("\r\n");
    PRINTF("keep lspid unchange:%-4u  \r\n", l3fe_ctrl.bf.keep_lspid_unchange);
    lanl3fe_qmap.wrd = CA_NE_REG_READ(L2TM_L2TM_BM_TM_TO_LANL3FE_QUEUE_MAP);
    PRINTF("tm queue mapping:0x%-4x  \r\n", lanl3fe_qmap.wrd);

    lanl3fe_ctrl.wrd = CA_NE_REG_READ(L2TM_L2TE_FC_GLB_CTRL_LANL3FE);
    lanl3fe_rxpfc_sts.wrd = CA_NE_REG_READ(L2TM_L2TE_FC_GLB_STS);

    src01_used_buf_cnt.wrd = CA_NE_REG_READ(TE_CB_SRC_USED_BUF_CNT01);
    buf_cnt[0] = src01_used_buf_cnt.bf.sid0;
    buf_cnt[1] = src01_used_buf_cnt.bf.sid1;
    src23_used_buf_cnt.wrd = CA_NE_REG_READ(TE_CB_SRC_USED_BUF_CNT23);
    buf_cnt[2] = src23_used_buf_cnt.bf.sid2;
    buf_cnt[3] = src23_used_buf_cnt.bf.sid3;
    src45_used_buf_cnt.wrd = CA_NE_REG_READ(TE_CB_SRC_USED_BUF_CNT45);
    buf_cnt[4] = src45_used_buf_cnt.bf.sid4;
    buf_cnt[5] = src45_used_buf_cnt.bf.sid5;
    src6_used_buf_cnt.wrd = CA_NE_REG_READ(TE_CB_SRC_USED_BUF_CNT6);
    buf_cnt[6] = src6_used_buf_cnt.bf.sid6;

    PRINTF("\r\n%-9s  %-16s  %-14s  %-14s  %-14s  %-22s  %-18s  %-17s\r\n",
        "src_port", "lanl3fe_rxpfc_en", "sp_base_hi_th", "sp_base_lo_th", "sp_base_th_en", "sp_base_lpid_en[0:31]", "lanl3fe_rxpfc_sts", "src_used_buf_cnt");
    PRINTF("-------------------------------------------------------------------------------------------------------------------------------------------\r\n");


    for (port = 0; port < TE_CB_SP_BASE_BP_LPID_CNT_EN_P0_LSB_COUNT; port++ ) 
    {
        sp_bp_threshold.wrd = CA_NE_REG_READ((TE_CB_SP_BASE_BP_THRSH_P0 + port * TE_CB_SP_BASE_BP_THRSH_P0_STRIDE));   
        sp_bp_ldpid_cnt_en_lsb.wrd = CA_NE_REG_READ((TE_CB_SP_BASE_BP_LPID_CNT_EN_P0_LSB + port * TE_CB_SP_BASE_BP_LPID_CNT_EN_P0_LSB_STRIDE));
        sp_bp_ldpid_cnt_en_msb.wrd = CA_NE_REG_READ((TE_CB_SP_BASE_BP_LPID_CNT_EN_P0_MSB + port * TE_CB_SP_BASE_BP_LPID_CNT_EN_P0_MSB_STRIDE));
        sp_bp_ldpid_cnt_en = ((ca_uint64_t)sp_bp_ldpid_cnt_en_msb.bf.lpid_en << 32) | sp_bp_ldpid_cnt_en_lsb.bf.lpid_en;

        PRINTF("%-9u  %-16u  %-14u  %-14u  %-14u  0x%-22llx  %-18u  0x%-17x  \r\n", 
            port,
            ((lanl3fe_ctrl.bf.lanl3fe_rxpfc_en >> port) & 0x1), 
            sp_bp_threshold.bf.hth, 
            sp_bp_threshold.bf.lth, 
            sp_bp_threshold.bf.en,
            sp_bp_ldpid_cnt_en, 
            ((lanl3fe_rxpfc_sts.bf.lanl3fe_rxpfc >> port) & 0x1), 
            buf_cnt[port]);

    }

    return CA_E_OK;
}

ca_status_t aal_l3_te_status_get
(
    CA_IN  ca_device_id_t        device_id,
    CA_OUT aal_l3_te_status_t *status
)
{
    TE_TE_GLB_STS_t value;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_STS);

    status->qmio_enq_ovrn   = value.bf.qmio_enq_ovrn;
    status->qmio_deq_ovrn   = value.bf.qmio_deq_ovrn;
    status->qmio_enp_ovrn   = value.bf.qmio_enp_ovrn;
    status->qmio_dep_ovrn   = value.bf.qmio_dep_ovrn;
    status->qmio_shp_ovrn   = value.bf.qmio_shp_ovrn;
    status->te_qm_nrdy      = value.bf.te_qm_nrdy;
    status->pol_init_done   = value.bf.pol_init_done;
    status->shp_init_done   = value.bf.shp_init_done;
    status->cb_init_done    = value.bf.cb_init_done;
    status->wred_init_done  = value.bf.wred_init_done;
    status->feio_sm         = value.bf.feio_sm;
    status->fe_te_req       = value.bf.fe_te_req;
    status->te_fe_ack       = value.bf.te_fe_ack;
    status->qmio_idle       = value.bf.qmio_idle;
    status->init_done       = value.bf.init_done;

    return CA_E_OK;
}


#ifdef __L3_SHAPER
#endif
static inline ca_status_t __l3_te_shaper_voq_tbc_access(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1
)
{
    TE_SHP_VOQ_TBC_MEM_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_VOQ_TBC_MEM_ACCESS);

    while (access.bf.access)
    {
        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_VOQ_TBC_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_VOQ_TBC_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_VOQ_TBC_MEM_DATA1, *data1);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_VOQ_TBC_MEM_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_VOQ_TBC_MEM_ACCESS);
        while (access.bf.access)
        {
            access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_VOQ_TBC_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_SHP_VOQ_TBC_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_SHP_VOQ_TBC_MEM_DATA1);
    }

    return CA_E_OK;

}


static inline ca_status_t __l3_te_shaper_port_tbc_access(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1
)
{
    TE_SHP_PORT_TBC_MEM_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_PORT_TBC_MEM_ACCESS);

    while (access.bf.access)
    {
        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_PORT_TBC_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_PORT_TBC_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_PORT_TBC_MEM_DATA1, *data1);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_PORT_TBC_MEM_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_PORT_TBC_MEM_ACCESS);
        while (access.bf.access)
        {
            access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_PORT_TBC_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_SHP_PORT_TBC_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_SHP_PORT_TBC_MEM_DATA1);
    }

    return CA_E_OK;
}

static inline ca_status_t __l3_te_shaper_group_tbc_access(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1
)
{
    TE_SHP_GROUP_TBC_MEM_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_GROUP_TBC_MEM_ACCESS);

    while (access.bf.access)
    {
        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_GROUP_TBC_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_GROUP_TBC_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_GROUP_TBC_MEM_DATA1, *data1);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_GROUP_TBC_MEM_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_GROUP_TBC_MEM_ACCESS);
        while (access.bf.access)
        {
            access.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_GROUP_TBC_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_SHP_GROUP_TBC_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_SHP_GROUP_TBC_MEM_DATA1);
    }

    return CA_E_OK;
}

ca_status_t aal_l3_te_shaper_ni_port_group_set(
    ca_device_id_t  device_id,
    ca_boolean_t enable)
{
    TE_SHP_CTRL_t value;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_CTRL);

    value.bf.ni_port_grp_en = enable ? 1 : 0;

    CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_shaper_ni_port_group_get(
    ca_device_id_t  device_id,
    ca_boolean_t *enable)
{
    TE_SHP_CTRL_t value;

    if (!enable)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_CTRL);

    *enable = (ca_boolean_t)value.bf.ni_port_grp_en;

    return CA_E_OK;
}

ca_status_t aal_l3_te_shaper_cpu_port_group_set
(
    ca_device_id_t  device_id,
    ca_boolean_t enable
)
{
    TE_SHP_CTRL_t value;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_CTRL);

    value.bf.cpu_port_grp_en = enable ? 1 : 0;

    CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_shaper_cpu_port_group_get
(
    ca_device_id_t  device_id,
    ca_boolean_t *enable
)
{
    TE_SHP_CTRL_t value;

    if (!enable)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_CTRL);

    *enable = (ca_boolean_t)value.bf.cpu_port_grp_en;

    return CA_E_OK;
}


ca_status_t aal_l3_te_shaper_voq_tbc_set(
    ca_device_id_t                 device_id,
    ca_uint32_t                 voq_id,
    aal_l3_te_shaper_tbc_cfg_msk_t msk,
    aal_l3_te_shaper_tbc_cfg_t     *cfg)
{
    TE_SHP_VOQ_TBC_MEM_DATA0_t value0;
    TE_SHP_VOQ_TBC_MEM_DATA1_t value1;
    ca_uint32_t index = voq_id;

    if (!cfg)
        return CA_E_PARAM;

    if (voq_id > AAL_L3TE_MAX_VOQ_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_shaper_voq_tbc_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;


    if (msk.s.rate_k)
    {
        if (cfg->rate_k > CA_AAL_L3_TE_SHP_MAX_RATE_K_VOQ)
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
    return __l3_te_shaper_voq_tbc_access(
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd));
}



ca_status_t aal_l3_te_shaper_voq_tbc_get(
    ca_device_id_t             device_id,
    ca_uint32_t             voq_id,
    aal_l3_te_shaper_tbc_cfg_t *cfg)
{
    TE_SHP_VOQ_TBC_MEM_DATA0_t value0;
    TE_SHP_VOQ_TBC_MEM_DATA1_t value1;

    ca_uint32_t index = voq_id;

    if (!cfg)
        return CA_E_PARAM;

    if (voq_id > AAL_L3TE_MAX_VOQ_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_shaper_voq_tbc_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    cfg->rate_k = value0.bf.rate_k;
    cfg->rate_m = value0.bf.rate_m;
    cfg->bs     = ((value1.bf.bs & 0x3f) << 6) | value0.bf.bs;
    cfg->state  = (aal_l3_te_shaper_admin_state_t)value1.bf.enb;
    cfg->mode   = (aal_l3_te_shaper_counting_mode_t)value1.bf.mode;
    cfg->tbc_count = value1.bf.tbc;

    return CA_E_OK;
}

/****************************************************************
** SHP_CTRL.cpu_port_grp_en is 0, entry 0 ~ 7 for 8 CPU ports
** SHP_CTRL.cpu_port_grp_en is 1, entry 0 for port group shaper
** SHP_CTRL.ni_port_grp_en  is 0, entry 8 ~ 15 for 8 NI ports
** SHP_CTRL.ni_port_grp_en  is 1, entry 0 for ni group shaper
****************************************************************/
ca_status_t aal_l3_te_shaper_port_tbc_set(
    ca_device_id_t                 device_id,
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
            device_id,
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
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd));
}



ca_status_t aal_l3_te_shaper_port_tbc_get(
    ca_device_id_t             device_id,
    ca_port_id_t            port_id,
    aal_l3_te_shaper_tbc_cfg_t *cfg)
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
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    cfg->rate_k = value0.bf.rate_k;
    cfg->rate_m = value0.bf.rate_m;
    cfg->bs     = ((value1.bf.bs & 0x3f) << 6) | value0.bf.bs;
    cfg->state  = (aal_l3_te_shaper_admin_state_t)value1.bf.enb;
    cfg->mode   = (aal_l3_te_shaper_counting_mode_t)value1.bf.mode;
    cfg->tbc_count = value1.bf.tbc;

    return CA_E_OK;
}

ca_status_t aal_l3_te_shaper_group_tbc_set
(
    ca_device_id_t                    device_id,
    ca_uint32_t                    group_id,
    aal_l3_te_shaper_tbc_cfg_msk_t msk,
    aal_l3_te_shaper_tbc_cfg_t     *cfg
)
{
    TE_SHP_GROUP_TBC_MEM_DATA0_t value0;
    TE_SHP_GROUP_TBC_MEM_DATA1_t value1;
    ca_uint32_t index = group_id;

    if (!cfg)
        return CA_E_PARAM;

    if (group_id > AAL_L3TE_MAX_PORT_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_shaper_group_tbc_access(
            device_id,
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

    /* update l3 te group shaper table */
    return __l3_te_shaper_group_tbc_access(
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd));
}



ca_status_t aal_l3_te_shaper_group_tbc_get
(
    ca_device_id_t                device_id,
    ca_uint32_t                group_id,
    aal_l3_te_shaper_tbc_cfg_t *cfg
)
{
    TE_SHP_GROUP_TBC_MEM_DATA0_t value0;
    TE_SHP_GROUP_TBC_MEM_DATA1_t value1;

    ca_uint32_t index = group_id;

    if (!cfg)
        return CA_E_PARAM;

    if (group_id > AAL_L3TE_MAX_PORT_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_shaper_group_tbc_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    cfg->rate_k = value0.bf.rate_k;
    cfg->rate_m = value0.bf.rate_m;
    cfg->bs     = ((value1.bf.bs & 0x3f) << 6) | value0.bf.bs;
    cfg->state  = (aal_l3_te_shaper_admin_state_t)value1.bf.enb;
    cfg->mode   = (aal_l3_te_shaper_counting_mode_t)value1.bf.mode;
    cfg->tbc_count = value1.bf.tbc;

    return CA_E_OK;
}

ca_status_t aal_l3_te_shaper_ipq_set
(
    ca_device_id_t  device_id,
    ca_uint8_t      ipg_id,
    ca_uint16_t     ipg
)
{
    TE_SHP_IPG_PROFILE_t value;

    if (ipg_id > CA_AAL_MAX_IPG_ID)
        return CA_E_PARAM;

    if (ipg > CA_AAL_MAX_IPG)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_IPG_PROFILE);

    switch (ipg_id)
    {
    case 0 :
        value.bf.ipg0 = ipg;
        break;
    case 1 :
        value.bf.ipg1 = ipg;
        break;
    case 2 :
        value.bf.ipg2 = ipg;
        break;
    case 3 :
        value.bf.ipg3 = ipg;
        break;
    default:
        break;
    }

    CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_IPG_PROFILE, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_shaper_ipq_get
(
    ca_device_id_t  device_id,
    ca_uint8_t      ipg_id,
    ca_uint16_t     *ipg
)
{
    TE_SHP_IPG_PROFILE_t value;

    if (!ipg)
        return CA_E_PARAM;

    if (ipg_id > CA_AAL_MAX_IPG_ID)
        return CA_E_PARAM;

    if (*ipg > CA_AAL_MAX_IPG)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_IPG_PROFILE);

    switch (ipg_id)
    {
    case 0 :
        *ipg = value.bf.ipg0;
        break;
    case 1 :
        *ipg = value.bf.ipg1;
        break;
    case 2 :
        *ipg = value.bf.ipg2;
        break;
    case 3 :
        *ipg = value.bf.ipg3;
        break;
    default:
        break;
    }

    return CA_E_OK;
}


ca_status_t aal_l3_te_shaper_port_ipq_set
(
    ca_device_id_t  device_id,
    ca_port_id_t    port_id,
    ca_uint8_t      ipg_id
)
{
    ca_uint32_t val32 = ipg_id;
    TE_SHP_PORT_IPG_SELECT0_t value;

    if (port_id > AAL_L3TE_MAX_PORT_ID)
        return CA_E_PARAM;

    if (ipg_id > CA_AAL_MAX_IPG_ID)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_PORT_IPG_SELECT0);

    value.wrd &= ~(0x3 << (port_id * 2));
    value.wrd |= val32 << (port_id * 2);

    CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_PORT_IPG_SELECT0, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_shaper_port_ipq_get
(
    ca_device_id_t  device_id,
    ca_port_id_t    port_id,
    ca_uint8_t      *ipg_id
)
{
    TE_SHP_PORT_IPG_SELECT0_t value;

    if (port_id > AAL_L3TE_MAX_PORT_ID)
        return CA_E_PARAM;

    if (!ipg_id)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_PORT_IPG_SELECT0);

    *ipg_id = (value.wrd >> (port_id * 2)) & 0x3;

    return CA_E_OK;
}



#ifdef __L3_POL
#endif
static inline ca_status_t __l3_te_policer_pkt_type_profile_access
(
    ca_device_id_t             device_id,
    aal_tbl_op_flag_t       op_flag,
    ca_uint32_t             addr,
    volatile ca_uint32_t    *data0,
    volatile ca_uint32_t    *data1,
    volatile ca_uint32_t    *data2
)
{
    TE_POL_PKT_TYPE_PROFILE_MEM_ACCESS_t profile_access;
    ca_uint32_t i = 0;

    profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_ACCESS);

    while (profile_access.bf.access)
    {
        profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_DATA1, *data1);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_DATA2, *data2);
    }

    memset((void *)&profile_access, 0, sizeof(profile_access));

    profile_access.bf.address  = addr;
    profile_access.bf.rbw      = op_flag;
    profile_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_ACCESS, profile_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_ACCESS);
        while (profile_access.bf.access)
        {
            profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_DATA1);
        *data2 = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_PROFILE_MEM_DATA2);
    }

    return CA_E_OK;
}


static inline ca_status_t __l3_te_policer_pkt_type_counter_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1
)
{
    TE_POL_PKT_TYPE_COUNTER_MEM_ACCESS_t counter_access;
    ca_uint32_t i = 0;

    counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_COUNTER_MEM_ACCESS);

    while (counter_access.bf.access)
    {
        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_COUNTER_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PKT_TYPE_COUNTER_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PKT_TYPE_COUNTER_MEM_DATA1, *data1);
    }

    memset((void *)&counter_access, 0, sizeof(counter_access));

    counter_access.bf.address  = addr;
    counter_access.bf.rbw      = op_flag;
    counter_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PKT_TYPE_COUNTER_MEM_ACCESS, counter_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_COUNTER_MEM_ACCESS);
        while (counter_access.bf.access)
        {
            counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_COUNTER_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_COUNTER_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_COUNTER_MEM_DATA1);
    }

    return CA_E_OK;
}


static inline ca_status_t __l3_te_policer_port_profile_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1,
    volatile ca_uint32_t *data2
)
{
    TE_POL_PORT_PROFILE_MEM_ACCESS_t profile_access;
    ca_uint32_t i = 0;

    profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_PROFILE_MEM_ACCESS);

    while (profile_access.bf.access)
    {
        profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_PROFILE_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PORT_PROFILE_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PORT_PROFILE_MEM_DATA1, *data1);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PORT_PROFILE_MEM_DATA2, *data2);
    }

    memset((void *)&profile_access, 0, sizeof(profile_access));

    profile_access.bf.address  = addr;
    profile_access.bf.rbw      = op_flag;
    profile_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PORT_PROFILE_MEM_ACCESS, profile_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_PROFILE_MEM_ACCESS);
        while (profile_access.bf.access)
        {
            profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_PROFILE_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_PROFILE_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_PROFILE_MEM_DATA1);
        *data2 = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_PROFILE_MEM_DATA2);
    }

    return CA_E_OK;

}


static inline ca_status_t __l3_te_policer_port_counter_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1
)
{
    TE_POL_PORT_COUNTER_MEM_ACCESS_t counter_access;
    ca_uint32_t i = 0;

    counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_COUNTER_MEM_ACCESS);

    while (counter_access.bf.access)
    {
        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_COUNTER_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PORT_COUNTER_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PORT_COUNTER_MEM_DATA1, *data1);
    }

    memset((void *)&counter_access, 0, sizeof(counter_access));

    counter_access.bf.address  = addr;
    counter_access.bf.rbw      = op_flag;
    counter_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PORT_COUNTER_MEM_ACCESS, counter_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_COUNTER_MEM_ACCESS);
        while (counter_access.bf.access)
        {
            counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_COUNTER_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_COUNTER_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_COUNTER_MEM_DATA1);
    }

    return CA_E_OK;
}


static inline ca_status_t __l3_te_policer_flow_profile_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1,
    volatile ca_uint32_t *data2
)
{
    TE_POL_FLOW_PROFILE_MEM_ACCESS_t profile_access;
    ca_uint32_t i = 0;

    profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_PROFILE_MEM_ACCESS);

    while (profile_access.bf.access)
    {
        profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_PROFILE_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW_PROFILE_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW_PROFILE_MEM_DATA1, *data1);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW_PROFILE_MEM_DATA2, *data2);
    }

    memset((void *)&profile_access, 0, sizeof(profile_access));

    profile_access.bf.address  = addr;
    profile_access.bf.rbw      = op_flag;
    profile_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW_PROFILE_MEM_ACCESS, profile_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_PROFILE_MEM_ACCESS);
        while (profile_access.bf.access)
        {
            profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_PROFILE_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_PROFILE_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_PROFILE_MEM_DATA1);
        *data2 = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_PROFILE_MEM_DATA2);
    }

    return CA_E_OK;

}


static inline ca_status_t __l3_te_policer_flow_counter_access(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1
)
{
    TE_POL_FLOW_COUNTER_MEM_ACCESS_t counter_access;
    ca_uint32_t i = 0;

    counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_COUNTER_MEM_ACCESS);

    while (counter_access.bf.access)
    {
        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_COUNTER_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW_COUNTER_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW_COUNTER_MEM_DATA1, *data1);
    }

    memset((void *)&counter_access, 0, sizeof(counter_access));

    counter_access.bf.address  = addr;
    counter_access.bf.rbw      = op_flag;
    counter_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW_COUNTER_MEM_ACCESS, counter_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_COUNTER_MEM_ACCESS);
        while (counter_access.bf.access)
        {
            counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_COUNTER_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_COUNTER_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_COUNTER_MEM_DATA1);
    }

    return CA_E_OK;

}

static inline ca_status_t __l3_te_policer_flow3_profile_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1,
    volatile ca_uint32_t *data2
)
{
    TE_POL_FLOW3_PROFILE_MEM_ACCESS_t profile_access;
    ca_uint32_t i = 0;

    profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_PROFILE_MEM_ACCESS);

    while (profile_access.bf.access)
    {
        profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_PROFILE_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW3_PROFILE_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW3_PROFILE_MEM_DATA1, *data1);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW3_PROFILE_MEM_DATA2, *data2);
    }

    memset((void *)&profile_access, 0, sizeof(profile_access));

    profile_access.bf.address  = addr;
    profile_access.bf.rbw      = op_flag;
    profile_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW3_PROFILE_MEM_ACCESS, profile_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_PROFILE_MEM_ACCESS);
        while (profile_access.bf.access)
        {
            profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_PROFILE_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_PROFILE_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_PROFILE_MEM_DATA1);
        *data2 = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_PROFILE_MEM_DATA2);
    }

    return CA_E_OK;

}


static inline ca_status_t __l3_te_policer_flow3_counter_access(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1
)
{
    TE_POL_FLOW3_COUNTER_MEM_ACCESS_t counter_access;
    ca_uint32_t i = 0;

    counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_COUNTER_MEM_ACCESS);

    while (counter_access.bf.access)
    {
        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_COUNTER_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW3_COUNTER_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW3_COUNTER_MEM_DATA1, *data1);
    }

    memset((void *)&counter_access, 0, sizeof(counter_access));

    counter_access.bf.address  = addr;
    counter_access.bf.rbw      = op_flag;
    counter_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW3_COUNTER_MEM_ACCESS, counter_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_COUNTER_MEM_ACCESS);
        while (counter_access.bf.access)
        {
            counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_COUNTER_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_COUNTER_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_COUNTER_MEM_DATA1);
    }

    return CA_E_OK;

}

static inline ca_status_t __l3_te_policer_flow_grp_profile_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1,
    volatile ca_uint32_t *data2
)
{
    TE_POL_AGRFLOW_PROFILE_MEM_ACCESS_t profile_access;
    ca_uint32_t i = 0;

    profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_PROFILE_MEM_ACCESS);

    while (profile_access.bf.access)
    {
        profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_PROFILE_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_AGRFLOW_PROFILE_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_AGRFLOW_PROFILE_MEM_DATA1, *data1);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_AGRFLOW_PROFILE_MEM_DATA2, *data2);
    }

    memset((void *)&profile_access, 0, sizeof(profile_access));

    profile_access.bf.address  = addr;
    profile_access.bf.rbw      = op_flag;
    profile_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_AGRFLOW_PROFILE_MEM_ACCESS, profile_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_PROFILE_MEM_ACCESS);
        while (profile_access.bf.access)
        {
            profile_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_PROFILE_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_PROFILE_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_PROFILE_MEM_DATA1);
        *data2 = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_PROFILE_MEM_DATA2);
    }

    return CA_E_OK;

}



static inline ca_status_t __l3_te_policer_flow_grp_counter_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1
)
{
    TE_POL_AGRFLOW_COUNTER_MEM_ACCESS_t counter_access;
    ca_uint32_t i = 0;

    counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_COUNTER_MEM_ACCESS);

    while (counter_access.bf.access)
    {
        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_COUNTER_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_AGRFLOW_COUNTER_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_AGRFLOW_COUNTER_MEM_DATA1, *data1);
    }

    memset((void *)&counter_access, 0, sizeof(counter_access));

    counter_access.bf.address  = addr;
    counter_access.bf.rbw      = op_flag;
    counter_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_AGRFLOW_COUNTER_MEM_ACCESS, counter_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_COUNTER_MEM_ACCESS);
        while (counter_access.bf.access)
        {
            counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_COUNTER_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_COUNTER_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_POL_AGRFLOW_COUNTER_MEM_DATA1);
    }

    return CA_E_OK;

}

ca_status_t _aal_l3_te_policer_flow_counter_clear(
    ca_device_id_t              device_id,
    ca_flow_id_t                flow_id
)
{
    TE_POL_FLOW_COUNTER_MEM_ACCESS_t cntr_acc;
    ca_uint32_t i = 0;

    cntr_acc.wrd = CA_DEV_NE_REG_READ(device_id, TE_POL_FLOW_COUNTER_MEM_ACCESS);

    while(cntr_acc.bf.access)
    {
        cntr_acc.wrd = CA_DEV_NE_REG_READ(device_id, TE_POL_FLOW_COUNTER_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    /* the operation to write table */
    CA_DEV_NE_REG_WRITE2(device_id, TE_POL_FLOW_COUNTER_MEM_DATA0, 0);
    CA_DEV_NE_REG_WRITE2(device_id, TE_POL_FLOW_COUNTER_MEM_DATA1, 0);

    memset((void *)&cntr_acc, 0, sizeof(cntr_acc));

    cntr_acc.bf.address  = flow_id;
    cntr_acc.bf.rbw      = AAL_TBL_OP_FLAG_WR;
    cntr_acc.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id, TE_POL_FLOW_COUNTER_MEM_ACCESS, cntr_acc.wrd);

    while (cntr_acc.bf.access)
    {
        cntr_acc.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_COUNTER_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }
    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_pkt_type_cfg_set
(
    ca_device_id_t               device_id,
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

    ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_CTRL0 + (ca_uint32_t)pkt_type * 4);

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

    CA_DEV_NE_REG_WRITE2(device_id, TE_POL_PKT_TYPE_CTRL0 + (ca_uint32_t)pkt_type * 4, ctrl.wrd);

    return CA_E_OK;
}


ca_status_t aal_l3_te_policer_pkt_type_cfg_get
(
    ca_device_id_t                   device_id,
    aal_l3_te_policer_pkt_type_t pkt_type,
    aal_l3_te_policer_cfg_t      *cfg
)
{
    TE_POL_PKT_TYPE_CTRL0_t ctrl;

    if (pkt_type >= CA_AAL_L3_POLICER_PKT_TYPE_END)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    //addr = TE_POL_PKT_TYPE_CTRL0 + pkt_type * 4;

    ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PKT_TYPE_CTRL0 + pkt_type * 4);

    cfg->type            = ctrl.bf.type;
    cfg->color_blind     = ctrl.bf.color_blind;
    cfg->pol_update_mode = ctrl.bf.pkt_mode;
    cfg->pkt_len_sel     = ctrl.bf.egr_len_sel;
    cfg->ipg             = ctrl.bf.ipg;
    cfg->bypass_yellow   = ctrl.bf.bypass_yellow;
    cfg->bypass_red      = ctrl.bf.bypass_red;
    cfg->result_update_mode = ctrl.bf.update_mode;
    cfg->nest_level      = ctrl.bf.nest_level;

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_pkt_type_profile_set
(
    ca_device_id_t                      device_id,
    aal_l3_te_policer_pkt_type_t    pkt_type,
    aal_l3_te_policer_profile_msk_t msk,
    aal_l3_te_policer_profile_t     *cfg
)
{
    TE_POL_PKT_TYPE_PROFILE_MEM_DATA0_t value0;
    TE_POL_PKT_TYPE_PROFILE_MEM_DATA1_t value1;
    TE_POL_PKT_TYPE_PROFILE_MEM_DATA2_t value2;
    ca_uint32_t index = (ca_uint32_t)pkt_type;

    if (!cfg)
        return CA_E_PARAM;

    if (pkt_type >= CA_AAL_L3_POLICER_PKT_TYPE_END)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_pkt_type_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd))
            != CA_E_OK)
        return CA_E_ERROR;


    if (msk.s.cir_k)
    {
        if (cfg->cir_k > CA_AAL_L3_TE_POL_MAX_RATE_K)
            return CA_E_PARAM;

        value0.bf.committed_rate_k = cfg->cir_k;
    }

    if (msk.s.cir_m)
    {
        if (cfg->cir_m > CA_AAL_L3_TE_POL_MAX_RATE_M)
            return CA_E_PARAM;

        value0.bf.committed_rate_m = cfg->cir_m;
    }

    if (msk.s.cbs)
    {
        if (cfg->cbs > CA_AAL_L3_TE_POL_MAX_CBS || cfg->cbs == 0)
            return CA_E_PARAM;

        value0.bf.committed_bs = cfg->cbs & 0x3f;
        value1.bf.committed_bs = (cfg->cbs >> 6) & 0x3f;
    }

    if (msk.s.pir_k)
    {
        if (cfg->pir_k > CA_AAL_L3_TE_POL_MAX_RATE_K)
            return CA_E_PARAM;

        value1.bf.peak_rate_k = cfg->pir_k;
    }

    if (msk.s.pir_m)
    {
        if (cfg->pir_m > CA_AAL_L3_TE_POL_MAX_RATE_M)
            return CA_E_PARAM;

        value1.bf.peak_rate_m = cfg->pir_m;
    }

    if (msk.s.pbs)
    {
        if (cfg->pbs > CA_AAL_L3_TE_POL_MAX_PBS || cfg->pbs == 0)
            return CA_E_PARAM;

        value2.bf.peak_bs = cfg->pbs;
    }


    /* update packet type based policing table */
    return __l3_te_policer_pkt_type_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd));
}


ca_status_t aal_l3_te_policer_pkt_type_profile_get
(
    ca_device_id_t                  device_id,
    aal_l3_te_policer_pkt_type_t pkt_type,
    aal_l3_te_policer_profile_t  *cfg
)
{
    TE_POL_PKT_TYPE_PROFILE_MEM_DATA0_t value0;
    TE_POL_PKT_TYPE_PROFILE_MEM_DATA1_t value1;
    TE_POL_PKT_TYPE_PROFILE_MEM_DATA2_t value2;
    ca_uint32_t index = pkt_type;

    if (!cfg)
        return CA_E_PARAM;

    if (pkt_type >= CA_AAL_L3_POLICER_PKT_TYPE_END)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_pkt_type_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    cfg->cir_k = value0.bf.committed_rate_k;
    cfg->cir_m = value0.bf.committed_rate_m;
    cfg->cbs   = ((value1.bf.committed_bs & 0x3f) << 6) | value0.bf.committed_bs;
    cfg->pir_k = value1.bf.peak_rate_k;
    cfg->pir_m = value1.bf.peak_rate_m;
    cfg->pbs   = value2.bf.peak_bs;

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_pkt_type_stats_get
(
    ca_device_id_t                   device_id,
    aal_l3_te_policer_pkt_type_t pkt_type,
    aal_l3_te_policer_stats_t    *stats
)
{
    TE_POL_PKT_TYPE_COUNTER_MEM_DATA0_t value0;
    TE_POL_PKT_TYPE_COUNTER_MEM_DATA1_t value1;
    ca_uint32_t index = (ca_uint32_t)pkt_type;

    if (!stats)
        return CA_E_PARAM;

    if (pkt_type >= CA_AAL_L3_POLICER_PKT_TYPE_END)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_pkt_type_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    stats->committed_tb_nums = value0.bf.committed_tbc;
    stats->peak_tb_nums      = (value0.bf.peak_tbc) | ((value1.bf.peak_tbc & 0x3ff) << 11);

    return CA_E_OK;
}


ca_status_t aal_l3_te_policer_port_cfg_set
(
    ca_device_id_t                  device_id,
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

    ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_CTRL0 + port_id * 4);

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

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_PORT_CTRL0 + port_id * 4, ctrl.wrd);

    return CA_E_OK;
}


ca_status_t aal_l3_te_policer_port_cfg_get
(
    ca_device_id_t                device_id,
    ca_flow_id_t               port_id,
    aal_l3_te_policer_cfg_t   *cfg
)
{
    TE_POL_PORT_CTRL0_t ctrl;

    if (port_id > CA_AAL_L3_MAX_PORT_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_PORT_CTRL0 + port_id * 4);

    cfg->type            = ctrl.bf.type;
    cfg->color_blind     = ctrl.bf.color_blind;
    cfg->pol_update_mode = ctrl.bf.pkt_mode;
    cfg->pkt_len_sel     = ctrl.bf.egr_len_sel;
    cfg->ipg             = ctrl.bf.ipg;
    cfg->bypass_yellow   = ctrl.bf.bypass_yellow;
    cfg->bypass_red      = ctrl.bf.bypass_red;
    cfg->result_update_mode = ctrl.bf.update_mode;
    cfg->nest_level      = ctrl.bf.nest_level;

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_port_profile_set
(
    ca_device_id_t                      device_id,
    ca_flow_id_t                     port_id,
    aal_l3_te_policer_profile_msk_t msk,
    aal_l3_te_policer_profile_t     *cfg
)
{
    TE_POL_PORT_PROFILE_MEM_DATA0_t value0;
    TE_POL_PORT_PROFILE_MEM_DATA1_t value1;
    TE_POL_PORT_PROFILE_MEM_DATA2_t value2;
    ca_uint32_t index = port_id;

    if (!cfg)
        return CA_E_PARAM;

    if (port_id > CA_AAL_L3_MAX_PORT_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_port_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd))
            != CA_E_OK)
        return CA_E_ERROR;


    if (msk.s.cir_k)
    {
        if (cfg->cir_k > CA_AAL_L3_TE_POL_MAX_RATE_K)
            return CA_E_PARAM;

        value0.bf.committed_rate_k = cfg->cir_k;
    }

    if (msk.s.cir_m)
    {
        if (cfg->cir_m > CA_AAL_L3_TE_POL_MAX_RATE_M)
            return CA_E_PARAM;

        value0.bf.committed_rate_m = cfg->cir_m;
    }

    if (msk.s.cbs)
    {
        if (cfg->cbs > CA_AAL_L3_TE_POL_MAX_CBS || cfg->cbs == 0)
            return CA_E_PARAM;

        value0.bf.committed_bs = cfg->cbs & 0x3f;
        value1.bf.committed_bs = (cfg->cbs >> 6) & 0x3f;
    }

    if (msk.s.pir_k)
    {
        if (cfg->pir_k > CA_AAL_L3_TE_POL_MAX_RATE_K)
            return CA_E_PARAM;

        value1.bf.peak_rate_k = cfg->pir_k;
    }

    if (msk.s.pir_m)
    {
        if (cfg->pir_m > CA_AAL_L3_TE_POL_MAX_RATE_M)
            return CA_E_PARAM;

        value1.bf.peak_rate_m = cfg->pir_m;
    }

    if (msk.s.pbs)
    {
        if (cfg->pbs > CA_AAL_L3_TE_POL_MAX_PBS || cfg->pbs == 0)
            return CA_E_PARAM;

        value2.bf.peak_bs = cfg->pbs;
    }

    /* update packet type based policing table */
    return __l3_te_policer_port_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd));
}


ca_status_t aal_l3_te_policer_port_profile_get
(
    ca_device_id_t                  device_id,
    ca_flow_id_t                 port_id,
    aal_l3_te_policer_profile_t *cfg
)
{
    TE_POL_PORT_PROFILE_MEM_DATA0_t value0;
    TE_POL_PORT_PROFILE_MEM_DATA1_t value1;
    TE_POL_PORT_PROFILE_MEM_DATA2_t value2;
    ca_uint32_t index = port_id;

    if (!cfg)
        return CA_E_PARAM;

    if (port_id > CA_AAL_L3_MAX_PORT_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_port_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    cfg->cir_k = value0.bf.committed_rate_k;
    cfg->cir_m = value0.bf.committed_rate_m;
    cfg->cbs   = ((value1.bf.committed_bs & 0x3f) << 6) | value0.bf.committed_bs;
    cfg->pir_k = value1.bf.peak_rate_k;
    cfg->pir_m = value1.bf.peak_rate_m;
    cfg->pbs   = value2.bf.peak_bs;

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_port_stats_get
(
    ca_device_id_t                device_id,
    ca_flow_id_t               port_id,
    aal_l3_te_policer_stats_t *stats
)
{
    TE_POL_PORT_COUNTER_MEM_DATA0_t value0;
    TE_POL_PORT_COUNTER_MEM_DATA1_t value1;
    ca_uint32_t index = port_id;

    if (!stats)
        return CA_E_PARAM;

    if (port_id > CA_AAL_L3_MAX_PORT_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_port_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    stats->committed_tb_nums = value0.bf.committed_tbc;
    stats->peak_tb_nums      = (value0.bf.peak_tbc) | ((value1.bf.peak_tbc & 0x3ff) << 11);

    return CA_E_OK;
}


ca_status_t aal_l3_te_policer_flow_cfg_set
(
    ca_device_id_t              device_id,
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

    ctrl.wrd = CA_DEV_NE_REG_READ(device_id, TE_POL_FLOW_GRP_CTRL0 + (flow_id / 32) * 4);

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

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW_GRP_CTRL0 + (flow_id / 32) * 4, ctrl.wrd);

    return CA_E_OK;
}


ca_status_t aal_l3_te_policer_flow_cfg_get
(
    ca_device_id_t              device_id,
    ca_flow_id_t             flow_id,
    aal_l3_te_policer_cfg_t *cfg)
{
    TE_POL_FLOW_GRP_CTRL0_t ctrl;

    if (flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    //addr = TE_POL_FLOW_GRP_CTRL0 + (flow_id / 32) * 4;

    ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW_GRP_CTRL0 + (flow_id / 32) * 4);

    cfg->type            = ctrl.bf.type;
    cfg->color_blind     = ctrl.bf.color_blind;
    cfg->pol_update_mode = ctrl.bf.pkt_mode;
    cfg->pkt_len_sel     = ctrl.bf.egr_len_sel;
    cfg->ipg             = ctrl.bf.ipg;
    cfg->bypass_yellow   = ctrl.bf.bypass_yellow;
    cfg->bypass_red      = ctrl.bf.bypass_red;
    cfg->result_update_mode = ctrl.bf.update_mode;
    cfg->nest_level      = ctrl.bf.nest_level;

    return CA_E_OK;
}


ca_status_t aal_l3_te_policer_flow_profile_set(
    ca_device_id_t                   device_id,
    ca_flow_id_t                  flow_id,
    aal_l3_te_policer_profile_msk_t msk,
    aal_l3_te_policer_profile_t     *cfg)
{
    TE_POL_FLOW_PROFILE_MEM_DATA0_t value0;
    TE_POL_FLOW_PROFILE_MEM_DATA1_t value1;
    TE_POL_FLOW_PROFILE_MEM_DATA2_t value2;
    ca_uint32_t index = flow_id;

    if (!cfg)
        return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_flow_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd))
            != CA_E_OK)
        return CA_E_ERROR;


    if (msk.s.cir_k)
    {
        if (cfg->cir_k > CA_AAL_L3_TE_POL_MAX_RATE_K)
            return CA_E_PARAM;

        value0.bf.committed_rate_k = cfg->cir_k;
    }

    if (msk.s.cir_m)
    {
        if (cfg->cir_m > CA_AAL_L3_TE_POL_MAX_RATE_M)
            return CA_E_PARAM;

        value0.bf.committed_rate_m = cfg->cir_m;
    }

    if (msk.s.cbs)
    {
        if (cfg->cbs > CA_AAL_L3_TE_POL_MAX_CBS || cfg->cbs == 0)
            return CA_E_PARAM;

        value0.bf.committed_bs = cfg->cbs & 0x3f;
        value1.bf.committed_bs = (cfg->cbs >> 6) & 0x3f;
    }

    if (msk.s.pir_k)
    {
        if (cfg->pir_k > CA_AAL_L3_TE_POL_MAX_RATE_K)
            return CA_E_PARAM;

        value1.bf.peak_rate_k = cfg->pir_k;
    }

    if (msk.s.pir_m)
    {
        if (cfg->pir_m > CA_AAL_L3_TE_POL_MAX_RATE_M)
            return CA_E_PARAM;

        value1.bf.peak_rate_m = cfg->pir_m;
    }

    if (msk.s.pbs)
    {
        if (cfg->pbs > CA_AAL_L3_TE_POL_MAX_PBS || cfg->pbs == 0)
            return CA_E_PARAM;

        value2.bf.peak_bs = cfg->pbs;
    }

    /* update packet type based policing table */
    return __l3_te_policer_flow_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd));
}



ca_status_t aal_l3_te_policer_flow_profile_get
(
    ca_device_id_t                  device_id,
    ca_flow_id_t                 flow_id,
    aal_l3_te_policer_profile_t *cfg)
{
    TE_POL_FLOW_PROFILE_MEM_DATA0_t value0;
    TE_POL_FLOW_PROFILE_MEM_DATA1_t value1;
    TE_POL_FLOW_PROFILE_MEM_DATA2_t value2;
    ca_uint32_t index = flow_id;

    if (!cfg)
        return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_flow_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    cfg->cir_k = value0.bf.committed_rate_k;
    cfg->cir_m = value0.bf.committed_rate_m;
    cfg->cbs   = ((value1.bf.committed_bs & 0x3f) << 6) | value0.bf.committed_bs;
    cfg->pir_k = value1.bf.peak_rate_k;
    cfg->pir_m = value1.bf.peak_rate_m;
    cfg->pbs   = value2.bf.peak_bs;

    return CA_E_OK;
}



ca_status_t aal_l3_te_policer_flow_stats_get
(
    ca_device_id_t                device_id,
    ca_flow_id_t               flow_id,
    aal_l3_te_policer_stats_t *stats)
{
    TE_POL_FLOW_COUNTER_MEM_DATA0_t value0;
    TE_POL_FLOW_COUNTER_MEM_DATA1_t value1;
    ca_uint32_t index = flow_id;

    if (!stats)
        return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_flow_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    stats->committed_tb_nums = value0.bf.committed_tbc;
    stats->peak_tb_nums      = (value0.bf.peak_tbc) | ((value1.bf.peak_tbc & 0x3ff) << 11);

    return CA_E_OK;
}
ca_status_t aal_l3_te_policer_flow3_cfg_set
(
    ca_device_id_t              device_id,
    ca_flow_id_t                flow_id,
    aal_l3_te_policer_cfg_msk_t msk,
    aal_l3_te_policer_cfg_t     *cfg
)
{
    TE_POL_FLOW3_CTRL0_t ctrl;

    if (flow_id > CA_AAL_L3_MAX_FLOW3_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    //addr = TE_POL_FLOW3_CTRL0 + (flow_id / 32) * 4;    /* 32 flows share one configuration */

    ctrl.wrd = CA_DEV_NE_REG_READ(device_id, TE_POL_FLOW3_CTRL0 + (flow_id / 32) * 4);

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

    CA_DEV_NE_REG_WRITE2(device_id,TE_POL_FLOW3_CTRL0 + (flow_id / 32) * 4, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_flow3_cfg_get
(
    ca_device_id_t              device_id,
    ca_flow_id_t             flow_id,
    aal_l3_te_policer_cfg_t *cfg)
{
    TE_POL_FLOW3_CTRL0_t ctrl;

    if (flow_id > CA_AAL_L3_MAX_FLOW3_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    //addr = TE_POL_FLOW3_CTRL0 + (flow_id / 32) * 4;

    ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_FLOW3_CTRL0 + (flow_id / 32) * 4);

    cfg->type            = ctrl.bf.type;
    cfg->color_blind     = ctrl.bf.color_blind;
    cfg->pol_update_mode = ctrl.bf.pkt_mode;
    cfg->pkt_len_sel     = ctrl.bf.egr_len_sel;
    cfg->ipg             = ctrl.bf.ipg;
    cfg->bypass_yellow   = ctrl.bf.bypass_yellow;
    cfg->bypass_red      = ctrl.bf.bypass_red;
    cfg->result_update_mode = ctrl.bf.update_mode;
    cfg->nest_level      = ctrl.bf.nest_level;

    return CA_E_OK;
}


ca_status_t aal_l3_te_policer_flow3_profile_set(
    ca_device_id_t                   device_id,
    ca_flow_id_t                  flow_id,
    aal_l3_te_policer_profile_msk_t msk,
    aal_l3_te_policer_profile_t     *cfg)
{
    TE_POL_FLOW3_PROFILE_MEM_DATA0_t value0;
    TE_POL_FLOW3_PROFILE_MEM_DATA1_t value1;
    TE_POL_FLOW3_PROFILE_MEM_DATA2_t value2;
    ca_uint32_t index = flow_id;

    if (!cfg)
        return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW3_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_flow3_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd))
            != CA_E_OK)
        return CA_E_ERROR;


    if (msk.s.cir_k)
    {
        if (cfg->cir_k > CA_AAL_L3_TE_POL_MAX_RATE_K)
            return CA_E_PARAM;

        value0.bf.committed_rate_k = cfg->cir_k;
    }

    if (msk.s.cir_m)
    {
        if (cfg->cir_m > CA_AAL_L3_TE_POL_MAX_RATE_M)
            return CA_E_PARAM;

        value0.bf.committed_rate_m = cfg->cir_m;
    }

    if (msk.s.cbs)
    {
        if (cfg->cbs > CA_AAL_L3_TE_POL_MAX_CBS || cfg->cbs == 0)
            return CA_E_PARAM;

        value0.bf.committed_bs = cfg->cbs & 0x3f;
        value1.bf.committed_bs = (cfg->cbs >> 6) & 0x3f;
    }

    if (msk.s.pir_k)
    {
        if (cfg->pir_k > CA_AAL_L3_TE_POL_MAX_RATE_K)
            return CA_E_PARAM;

        value1.bf.peak_rate_k = cfg->pir_k;
    }

    if (msk.s.pir_m)
    {
        if (cfg->pir_m > CA_AAL_L3_TE_POL_MAX_RATE_M)
            return CA_E_PARAM;

        value1.bf.peak_rate_m = cfg->pir_m;
    }

    if (msk.s.pbs)
    {
        if (cfg->pbs > CA_AAL_L3_TE_POL_MAX_PBS || cfg->pbs == 0)
            return CA_E_PARAM;

        value2.bf.peak_bs = cfg->pbs;
    }

    /* update packet type based policing table */
    return __l3_te_policer_flow3_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd));
}



ca_status_t aal_l3_te_policer_flow3_profile_get
(
    ca_device_id_t                  device_id,
    ca_flow_id_t                 flow_id,
    aal_l3_te_policer_profile_t *cfg)
{
    TE_POL_FLOW3_PROFILE_MEM_DATA0_t value0;
    TE_POL_FLOW3_PROFILE_MEM_DATA1_t value1;
    TE_POL_FLOW3_PROFILE_MEM_DATA2_t value2;
    ca_uint32_t index = flow_id;

    if (!cfg)
        return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW3_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_flow3_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    cfg->cir_k = value0.bf.committed_rate_k;
    cfg->cir_m = value0.bf.committed_rate_m;
    cfg->cbs   = ((value1.bf.committed_bs & 0x3f) << 6) | value0.bf.committed_bs;
    cfg->pir_k = value1.bf.peak_rate_k;
    cfg->pir_m = value1.bf.peak_rate_m;
    cfg->pbs   = value2.bf.peak_bs;

    return CA_E_OK;
}



ca_status_t aal_l3_te_policer_flow3_stats_get
(
    ca_device_id_t                device_id,
    ca_flow_id_t               flow_id,
    aal_l3_te_policer_stats_t *stats)
{
    TE_POL_FLOW3_COUNTER_MEM_DATA0_t value0;
    TE_POL_FLOW3_COUNTER_MEM_DATA1_t value1;
    ca_uint32_t index = flow_id;

    if (!stats)
        return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW3_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_flow3_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    stats->committed_tb_nums = value0.bf.committed_tbc;
    stats->peak_tb_nums      = (value0.bf.peak_tbc) | ((value1.bf.peak_tbc & 0x3ff) << 11);

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_agr_flow_cfg_set
(
    ca_device_id_t              device_id,
    ca_uint32_t                 agr_flow_id,
    aal_l3_te_policer_cfg_msk_t msk,
    aal_l3_te_policer_cfg_t     *cfg
)
{
    TE_POL_AGRFLOW_CTRL0_t ctrl;
    TE_TE_GLB_CTRL_t glb_ctrl;

    glb_ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_CTRL);

    /* 0 = 512 flow and 8 aggregate flow
       1 = 512 flow, 64 flow2 and 64 flow3 */
    if (glb_ctrl.bf.sel3flows == 0 && agr_flow_id > CA_AAL_L3_MAX_AGR_FLOW_ID)
        return CA_E_PARAM;
    else if (glb_ctrl.bf.sel3flows == 1 && agr_flow_id > CA_AAL_L3_MAX_FLOW2_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    if (glb_ctrl.bf.sel3flows == 0)
        ctrl.wrd = CA_DEV_NE_REG_READ(device_id, TE_POL_AGRFLOW_CTRL0 + agr_flow_id * 4);
    else
        ctrl.wrd = CA_DEV_NE_REG_READ(device_id, TE_POL_AGRFLOW_CTRL0 + (agr_flow_id / 32) * 4);

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

    if (glb_ctrl.bf.sel3flows == 0)
        CA_DEV_NE_REG_WRITE2(device_id, TE_POL_AGRFLOW_CTRL0 + agr_flow_id * 4, ctrl.wrd);
    else
        CA_DEV_NE_REG_WRITE2(device_id, TE_POL_AGRFLOW_CTRL0 + (agr_flow_id / 32) * 4, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_agr_flow_cfg_get
(
    ca_device_id_t          device_id,
    ca_uint32_t             agr_flow_id,
    aal_l3_te_policer_cfg_t *cfg
)
{
    TE_POL_AGRFLOW_CTRL0_t ctrl;
    TE_TE_GLB_CTRL_t glb_ctrl;

    glb_ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_CTRL);

    /* 0 = 512 flow and 8 aggregate flow
       1 = 512 flow, 64 flow2 and 64 flow3 */
    if (glb_ctrl.bf.sel3flows == 0 && agr_flow_id > CA_AAL_L3_MAX_AGR_FLOW_ID)
        return CA_E_PARAM;
    else if (glb_ctrl.bf.sel3flows == 1 && agr_flow_id > CA_AAL_L3_MAX_FLOW2_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (glb_ctrl.bf.sel3flows == 0)
        ctrl.wrd = CA_DEV_NE_REG_READ(device_id, TE_POL_AGRFLOW_CTRL0 + agr_flow_id * 4);
    else
        ctrl.wrd = CA_DEV_NE_REG_READ(device_id, TE_POL_AGRFLOW_CTRL0 + (agr_flow_id / 32) * 4);

    cfg->type            = ctrl.bf.type;
    cfg->color_blind     = ctrl.bf.color_blind;
    cfg->pol_update_mode = ctrl.bf.pkt_mode;
    cfg->pkt_len_sel     = ctrl.bf.egr_len_sel;
    cfg->ipg             = ctrl.bf.ipg;
    cfg->bypass_yellow   = ctrl.bf.bypass_yellow;
    cfg->bypass_red      = ctrl.bf.bypass_red;
    cfg->result_update_mode = ctrl.bf.update_mode;
    cfg->nest_level      = ctrl.bf.nest_level;

    return CA_E_OK;
}

ca_status_t aal_l3_te_policer_agr_flow_profile_set
(
    ca_device_id_t                       device_id,
    ca_uint32_t                       agr_flow_id,
    aal_l3_te_policer_profile_msk_t  msk,
    aal_l3_te_policer_profile_t      *cfg)
{
    TE_POL_AGRFLOW_PROFILE_MEM_DATA0_t value0;
    TE_POL_AGRFLOW_PROFILE_MEM_DATA1_t value1;
    TE_POL_AGRFLOW_PROFILE_MEM_DATA2_t value2;
    TE_TE_GLB_CTRL_t glb_ctrl;
    ca_uint32_t index = agr_flow_id;

    glb_ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_CTRL);

    if (!cfg)
        return CA_E_PARAM;

    /* 0 = 512 flow and 8 aggregate flow
       1 = 512 flow, 64 flow2 and 64 flow3 */
    if (glb_ctrl.bf.sel3flows == 0 && agr_flow_id > CA_AAL_L3_MAX_AGR_FLOW_ID)
        return CA_E_PARAM;
    else if (glb_ctrl.bf.sel3flows == 1 && agr_flow_id > CA_AAL_L3_MAX_FLOW2_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_flow_grp_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd))
            != CA_E_OK)
        return CA_E_ERROR;


    if (msk.s.cir_k)
    {
        if (cfg->cir_k > CA_AAL_L3_TE_POL_MAX_RATE_K)
            return CA_E_PARAM;

        value0.bf.committed_rate_k = cfg->cir_k;
    }

    if (msk.s.cir_m)
    {
        if (cfg->cir_m > CA_AAL_L3_TE_POL_MAX_RATE_M)
            return CA_E_PARAM;

        value0.bf.committed_rate_m = cfg->cir_m;
    }

    if (msk.s.cbs)
    {
        if (cfg->cbs > CA_AAL_L3_TE_POL_MAX_CBS || cfg->cbs == 0)
            return CA_E_PARAM;

        value0.bf.committed_bs = cfg->cbs & 0x3f;
        value1.bf.committed_bs = (cfg->cbs >> 6) & 0x3f;
    }

    if (msk.s.pir_k)
    {
        if (cfg->pir_k > CA_AAL_L3_TE_POL_MAX_RATE_K)
            return CA_E_PARAM;

        value1.bf.peak_rate_k = cfg->pir_k;
    }

    if (msk.s.pir_m)
    {
        if (cfg->pir_m > CA_AAL_L3_TE_POL_MAX_RATE_M)
            return CA_E_PARAM;

        value1.bf.peak_rate_m = cfg->pir_m;
    }

    if (msk.s.pbs)
    {
        if (cfg->pbs > CA_AAL_L3_TE_POL_MAX_PBS || cfg->pbs == 0)
            return CA_E_PARAM;

        value2.bf.peak_bs = cfg->pbs;
    }

    /* update packet type based policing table */
    return __l3_te_policer_flow_grp_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd));
}



ca_status_t aal_l3_te_policer_agr_flow_profile_get
(
    ca_device_id_t                       device_id,
    ca_uint32_t                       agr_flow_id,
    aal_l3_te_policer_profile_t      *cfg)
{
    TE_POL_AGRFLOW_PROFILE_MEM_DATA0_t value0;
    TE_POL_AGRFLOW_PROFILE_MEM_DATA1_t value1;
    TE_POL_AGRFLOW_PROFILE_MEM_DATA2_t value2;
    TE_TE_GLB_CTRL_t glb_ctrl;
    ca_uint32_t index = agr_flow_id;

    glb_ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_CTRL);

    if (!cfg)
        return CA_E_PARAM;

    /* 0 = 512 flow and 8 aggregate flow
       1 = 512 flow, 64 flow2 and 64 flow3 */
    if (glb_ctrl.bf.sel3flows == 0 && agr_flow_id > CA_AAL_L3_MAX_AGR_FLOW_ID)
        return CA_E_PARAM;
    else if (glb_ctrl.bf.sel3flows == 1 && agr_flow_id > CA_AAL_L3_MAX_FLOW2_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_flow_grp_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    cfg->cir_k = value0.bf.committed_rate_k;
    cfg->cir_m = value0.bf.committed_rate_m;
    cfg->cbs   = ((value1.bf.committed_bs & 0x3f) << 6) | value0.bf.committed_bs;
    cfg->pir_k = value1.bf.peak_rate_k;
    cfg->pir_m = value1.bf.peak_rate_m;
    cfg->pbs   = value2.bf.peak_bs;

    return CA_E_OK;
}



ca_status_t aal_l3_te_policer_agr_flow_stats_get
(
    ca_device_id_t                       device_id,
    ca_uint32_t                       agr_flow_id,
    aal_l3_te_policer_stats_t        *stats)
{
    TE_POL_AGRFLOW_COUNTER_MEM_DATA0_t value0;
    TE_POL_AGRFLOW_COUNTER_MEM_DATA1_t value1;
    TE_TE_GLB_CTRL_t glb_ctrl;
    ca_uint32_t index = agr_flow_id;

    glb_ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_CTRL);

    if (!stats)
        return CA_E_PARAM;

    /* 0 = 512 flow and 8 aggregate flow
       1 = 512 flow, 64 flow2 and 64 flow3 */
    if (glb_ctrl.bf.sel3flows == 0 && agr_flow_id > CA_AAL_L3_MAX_AGR_FLOW_ID)
        return CA_E_PARAM;
    else if (glb_ctrl.bf.sel3flows == 1 && agr_flow_id > CA_AAL_L3_MAX_FLOW2_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_policer_flow_grp_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    stats->committed_tb_nums = value0.bf.committed_tbc;
    stats->peak_tb_nums      = (value0.bf.peak_tbc) | ((value1.bf.peak_tbc & 0x3ff) << 11);

    return CA_E_OK;
}


#ifdef __L3_WRED
#endif
static inline ca_status_t __l3_te_wred_free_global_mem_size_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0
)
{
    TE_WRED_AFGS_MEM_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = CA_DEV_NE_REG_READ(device_id, TE_WRED_AFGS_MEM_ACCESS);

    while (access.bf.access)
    {
        access.wrd = CA_DEV_NE_REG_READ(device_id, TE_WRED_AFGS_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id, TE_WRED_AFGS_MEM_DATA, *data0);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id, TE_WRED_AFGS_MEM_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = CA_DEV_NE_REG_READ(device_id, TE_WRED_AFGS_MEM_ACCESS);
        while (access.bf.access)
        {
            access.wrd = CA_DEV_NE_REG_READ(device_id, TE_WRED_AFGS_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id, TE_WRED_AFGS_MEM_DATA);
    }

    return CA_E_OK;

}

static inline ca_status_t __l3_te_wred_profile_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1,
    volatile ca_uint32_t *data2,
    volatile ca_uint32_t *data3,
    volatile ca_uint32_t *data4,
    volatile ca_uint32_t *data5,
    volatile ca_uint32_t *data6,
    volatile ca_uint32_t *data7
)
{
    TE_WRED_PROFILE_MEM_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_ACCESS);

    while (access.bf.access)
    {
        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_MEM_DATA1, *data1);
        CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_MEM_DATA2, *data2);
        CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_MEM_DATA3, *data3);
        CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_MEM_DATA4, *data4);
        CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_MEM_DATA5, *data5);
        CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_MEM_DATA6, *data6);
        CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_MEM_DATA7, *data7);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_MEM_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_ACCESS);
        while (access.bf.access)
        {
            access.wrd = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_DATA1);
        *data2 = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_DATA2);
        *data3 = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_DATA3);
        *data4 = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_DATA4);
        *data5 = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_DATA5);
        *data6 = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_DATA6);
        *data7 = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_MEM_DATA7);
    }

    return CA_E_OK;
}


static inline ca_status_t __l3_te_wred_profile_sel_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1
)
{
    TE_WRED_PROFILE_SELECT_MEM_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_SELECT_MEM_ACCESS);

    while (access.bf.access)
    {
        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_SELECT_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_SELECT_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_SELECT_MEM_DATA1, *data1);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_WRED_PROFILE_SELECT_MEM_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_SELECT_MEM_ACCESS);
        while (access.bf.access)
        {
            access.wrd = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_SELECT_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_SELECT_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_WRED_PROFILE_SELECT_MEM_DATA1);
    }

    return CA_E_OK;
}


ca_status_t aal_l3_te_wred_profile_set
(
    ca_device_id_t                  device_id,
    ca_uint32_t                  id,
    aal_l3_te_wred_profile_msk_t msk,
    aal_l3_te_wred_profile_t     *profile
)
{
    TE_WRED_PROFILE_MEM_DATA0_t value0;
    TE_WRED_PROFILE_MEM_DATA1_t value1;
    TE_WRED_PROFILE_MEM_DATA2_t value2;
    TE_WRED_PROFILE_MEM_DATA3_t value3;
    TE_WRED_PROFILE_MEM_DATA4_t value4;
    TE_WRED_PROFILE_MEM_DATA5_t value5;
    TE_WRED_PROFILE_MEM_DATA6_t value6;
    TE_WRED_PROFILE_MEM_DATA7_t value7;

    ca_uint32_t index = id;

    if (id > CA_AAL_L3_WRED_MAX_PROFILE_ID)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    if (!profile)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));
    memset((void *)&(value3), 0, sizeof(value3));
    memset((void *)&(value4), 0, sizeof(value4));
    memset((void *)&(value5), 0, sizeof(value5));
    memset((void *)&(value6), 0, sizeof(value6));
    memset((void *)&(value7), 0, sizeof(value7));


    /* get current configuration at first and then update specified field */
    if (__l3_te_wred_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd),
            (volatile ca_uint32_t *)&(value3.wrd),
            (volatile ca_uint32_t *)&(value4.wrd),
            (volatile ca_uint32_t *)&(value5.wrd),
            (volatile ca_uint32_t *)&(value6.wrd),
            (volatile ca_uint32_t *)&(value7.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    if (msk.s.unmark_dp)
    {
        value0.wrd = 0;
        value1.wrd = 0;
        value2.wrd = 0;

        value0.wrd |= (profile->unmark_dp[0]);
        value0.wrd |= (profile->unmark_dp[1] << 6);
        value0.wrd |= (profile->unmark_dp[2] << 12);
        value0.wrd |= (profile->unmark_dp[3] << 18);
        value0.wrd |= (profile->unmark_dp[4] << 24);
        value0.wrd |= ((profile->unmark_dp[5] & 0x3) << 30);
        value1.wrd |= ((profile->unmark_dp[5] >> 2) & 0xf);
        value1.wrd |= (profile->unmark_dp[6] << 4);
        value1.wrd |= (profile->unmark_dp[7] << 10);
        value1.wrd |= (profile->unmark_dp[8] << 16);
        value1.wrd |= (profile->unmark_dp[9] << 22);
        value1.wrd |= ((profile->unmark_dp[10] & 0xf) << 28);
        value2.wrd |= ((profile->unmark_dp[10] >> 4) & 0x3);
        value2.wrd |= (profile->unmark_dp[11] << 2);
        value2.wrd |= (profile->unmark_dp[12] << 8);
        value2.wrd |= (profile->unmark_dp[13] << 14);
        value2.wrd |= (profile->unmark_dp[14] << 20);
        value2.wrd |= (profile->unmark_dp[15] << 26);
    }

    if (msk.s.mark_dp)
    {
        value3.wrd = 0;
        value4.wrd = 0;
        value5.wrd = 0;

        value3.wrd |= (profile->mark_dp[0]);
        value3.wrd |= (profile->mark_dp[1] << 6);
        value3.wrd |= (profile->mark_dp[2] << 12);
        value3.wrd |= (profile->mark_dp[3] << 18);
        value3.wrd |= (profile->mark_dp[4] << 24);
        value3.wrd |= ((profile->mark_dp[5] & 0x3) << 30);
        value4.wrd |= ((profile->mark_dp[5] >> 2) & 0xf);
        value4.wrd |= (profile->mark_dp[6] << 4);
        value4.wrd |= (profile->mark_dp[7] << 10);
        value4.wrd |= (profile->mark_dp[8] << 16);
        value4.wrd |= (profile->mark_dp[9] << 22);
        value4.wrd |= ((profile->mark_dp[10] & 0xf) << 28);
        value5.wrd |= ((profile->mark_dp[10] >> 4) & 0x3);
        value5.wrd |= (profile->mark_dp[11] << 2);
        value5.wrd |= (profile->mark_dp[12] << 8);
        value5.wrd |= (profile->mark_dp[13] << 14);
        value5.wrd |= (profile->mark_dp[14] << 20);
        value5.wrd |= (profile->mark_dp[15] << 26);
    }

    if (msk.s.unmark_idx)
    {
        value6.bf.unmark_idx = profile->unmark_idx;
    }

    if (msk.s.mark_idx)
    {
        value6.bf.mark_idx = profile->mark_idx;
    }

    /* update WRED profile selection table */
    return __l3_te_wred_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd),
            (volatile ca_uint32_t *)&(value3.wrd),
            (volatile ca_uint32_t *)&(value4.wrd),
            (volatile ca_uint32_t *)&(value5.wrd),
            (volatile ca_uint32_t *)&(value6.wrd),
            (volatile ca_uint32_t *)&(value7.wrd));
}


ca_status_t aal_l3_te_wred_profile_get
(
    ca_device_id_t              device_id,
    ca_uint32_t              id,
    aal_l3_te_wred_profile_t *profile
)
{
    TE_WRED_PROFILE_MEM_DATA0_t value0;
    TE_WRED_PROFILE_MEM_DATA1_t value1;
    TE_WRED_PROFILE_MEM_DATA2_t value2;
    TE_WRED_PROFILE_MEM_DATA3_t value3;
    TE_WRED_PROFILE_MEM_DATA4_t value4;
    TE_WRED_PROFILE_MEM_DATA5_t value5;
    TE_WRED_PROFILE_MEM_DATA6_t value6;
    TE_WRED_PROFILE_MEM_DATA7_t value7;

    ca_uint32_t index = id;

    if (id > CA_AAL_L3_WRED_MAX_PROFILE_ID)
        return CA_E_PARAM;

    if (!profile)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));
    memset((void *)&(value3), 0, sizeof(value3));
    memset((void *)&(value4), 0, sizeof(value4));
    memset((void *)&(value5), 0, sizeof(value5));
    memset((void *)&(value6), 0, sizeof(value6));
    memset((void *)&(value7), 0, sizeof(value7));


    /* get current configuration at first and then update specificed field */
    if (__l3_te_wred_profile_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd),
            (volatile ca_uint32_t *)&(value3.wrd),
            (volatile ca_uint32_t *)&(value4.wrd),
            (volatile ca_uint32_t *)&(value5.wrd),
            (volatile ca_uint32_t *)&(value6.wrd),
            (volatile ca_uint32_t *)&(value7.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    profile->unmark_dp[0] = value0.wrd & 0x3f;
    profile->unmark_dp[1] = (value0.wrd >> 6) & 0x3f;
    profile->unmark_dp[2] = (value0.wrd >> 12) & 0x3f;
    profile->unmark_dp[3] = (value0.wrd >> 18) & 0x3f;
    profile->unmark_dp[4] = (value0.wrd >> 24) & 0x3f;
    profile->unmark_dp[5] = ((value0.wrd >> 30) & 0x3) | ((value1.wrd & 0xf) << 2);
    profile->unmark_dp[6] = (value1.wrd >> 4) & 0x3f;
    profile->unmark_dp[7] = (value1.wrd >> 10) & 0x3f;
    profile->unmark_dp[8] = (value1.wrd >> 16) & 0x3f;
    profile->unmark_dp[9] = (value1.wrd >> 22) & 0x3f;
    profile->unmark_dp[10] = ((value1.wrd >> 28) & 0xf) | ((value2.wrd & 0x3) << 4);
    profile->unmark_dp[11] = (value2.wrd >> 2) & 0x3f;
    profile->unmark_dp[12] = (value2.wrd >> 8) & 0x3f;
    profile->unmark_dp[13] = (value2.wrd >> 14) & 0x3f;
    profile->unmark_dp[14] = (value2.wrd >> 20) & 0x3f;
    profile->unmark_dp[15] = (value2.wrd >> 26) & 0x3f;

    profile->mark_dp[0] = value3.wrd & 0x3f;
    profile->mark_dp[1] = (value3.wrd >> 6) & 0x3f;
    profile->mark_dp[2] = (value3.wrd >> 12) & 0x3f;
    profile->mark_dp[3] = (value3.wrd >> 18) & 0x3f;
    profile->mark_dp[4] = (value3.wrd >> 24) & 0x3f;
    profile->mark_dp[5] = ((value3.wrd >> 30) & 0x3) | ((value4.wrd & 0xf) << 2);
    profile->mark_dp[6] = (value4.wrd >> 4) & 0x3f;
    profile->mark_dp[7] = (value4.wrd >> 10) & 0x3f;
    profile->mark_dp[8] = (value4.wrd >> 16) & 0x3f;
    profile->mark_dp[9] = (value4.wrd >> 22) & 0x3f;
    profile->mark_dp[10] = ((value4.wrd >> 28) & 0xf) | ((value5.wrd & 0x3) << 4);
    profile->mark_dp[11] = (value5.wrd >> 2) & 0x3f;
    profile->mark_dp[12] = (value5.wrd >> 8) & 0x3f;
    profile->mark_dp[13] = (value5.wrd >> 14) & 0x3f;
    profile->mark_dp[14] = (value5.wrd >> 20) & 0x3f;
    profile->mark_dp[15] = (value5.wrd >> 26) & 0x3f;

    profile->unmark_idx  = value6.bf.unmark_idx;
    profile->mark_idx    = value6.bf.mark_idx;

    return CA_E_OK;
}

ca_status_t aal_l3_te_wred_profile_sel_set
(
    ca_device_id_t                   device_id,
    ca_uint32_t                   id,
    aal_l3_te_wred_profile_sel_msk_t msk,
    aal_l3_te_wred_profile_sel_t     *sel
)
{
    TE_WRED_PROFILE_SELECT_MEM_DATA0_t value0;
    TE_WRED_PROFILE_SELECT_MEM_DATA1_t value1;
    ca_uint32_t index = id;

    if (id > CA_AAL_L3_WRED_MAX_PROFILE_SEL_ID)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    if (!sel)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_wred_profile_sel_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    if (msk.s.profile_id_0)
    {
        value0.bf.sel0 = sel->profile_id[0];
    }

    if (msk.s.profile_id_1)
    {
        value0.bf.sel1 = sel->profile_id[1];
    }

    if (msk.s.profile_id_2)
    {
        value0.bf.sel2 = sel->profile_id[2];
    }

    if (msk.s.profile_id_3)
    {
        value0.bf.sel3 = sel->profile_id[3];
    }

    if (msk.s.profile_id_4)
    {
        value0.bf.sel4 = sel->profile_id[4];
    }

    if (msk.s.profile_id_5)
    {
        value0.bf.sel5 = sel->profile_id[5];
    }

    if (msk.s.profile_id_6)
    {
        /* cos6 low 2 bits */
        value0.bf.sel6 = sel->profile_id[6] & 0x3;

        /* cos6 high 3 bits */
        value1.bf.sel6 = (sel->profile_id[6] >> 2) & 0x7;
    }

    if (msk.s.profile_id_7)
    {
        value1.bf.sel7 = sel->profile_id[7];
    }


    /* update WRED profile selection table */
    return __l3_te_wred_profile_sel_access(
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd));
}


ca_status_t aal_l3_te_wred_profile_sel_get
(
    ca_device_id_t                  device_id,
    ca_uint32_t                  id,
    aal_l3_te_wred_profile_sel_t *sel
)
{
    TE_WRED_PROFILE_SELECT_MEM_DATA0_t value0;
    TE_WRED_PROFILE_SELECT_MEM_DATA1_t value1;
    ca_uint32_t index = id;

    if (id > CA_AAL_L3_WRED_MAX_PROFILE_SEL_ID)
        return CA_E_PARAM;

    if (!sel)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));


    /* get current configuration at first and then update specificed field */
    if (__l3_te_wred_profile_sel_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    sel->profile_id[0] = value0.bf.sel0;
    sel->profile_id[1] = value0.bf.sel1;
    sel->profile_id[2] = value0.bf.sel2;
    sel->profile_id[3] = value0.bf.sel3;
    sel->profile_id[4] = value0.bf.sel4;
    sel->profile_id[5] = value0.bf.sel5;
    sel->profile_id[6] = (value1.bf.sel6 << 2) | value0.bf.sel6;
    sel->profile_id[7] = value1.bf.sel7;


    return CA_E_OK;
}

ca_boolean aal_l3_te_wred_info_lookup
(
    ca_uint32_t *marked_dp,
    ca_uint32_t *unmarked_dp,
    ca_uint32_t *index
)
{
    ca_uint32_t i = 0;

    if (!marked_dp)
        return FALSE;

    if (!unmarked_dp)
        return FALSE;

    if (!index)
        return FALSE;

    for (i=0; i<CA_AAL_L3_WRED_MAX_PROFILE_NUMS; i++)
    {
        if (g_aal_l3_te_wred_profile_info[i].ref_counter == 0)
            continue;

        if (
            memcmp(g_aal_l3_te_wred_profile_info[i].marked_dp, marked_dp, CA_AAL_L3_WRED_MAX_DROP_PROBABILTY * sizeof(ca_uint32_t)) == 0 &&
            memcmp(g_aal_l3_te_wred_profile_info[i].unmarked_dp, unmarked_dp, CA_AAL_L3_WRED_MAX_DROP_PROBABILTY * sizeof(ca_uint32_t)) == 0
            )
        {
            *index = i;     /* hit, this profile entry can be reused */
            return TRUE;
        }

    }

    return FALSE;

}

ca_boolean aal_l3_te_wred_info_empty(void)
{
    ca_uint32_t zero_dp[CA_AAL_L3_WRED_MAX_DROP_PROBABILTY];
    ca_uint32_t i = 0;

    memset((void *)&zero_dp[0], 0, sizeof(zero_dp));

    for (i=0; i<CA_AAL_L3_WRED_MAX_PROFILE_NUMS; i++)
    {
        if (memcmp(g_aal_l3_te_wred_profile_info[i].marked_dp, zero_dp, sizeof(zero_dp)) ||
            memcmp(g_aal_l3_te_wred_profile_info[i].unmarked_dp, zero_dp, sizeof(zero_dp)))
        {
            return FALSE;
        }
    }

    return TRUE;
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

ca_status_t aal_l3_te_wred_profile_info_set
(
    ca_uint32_t index,
    ca_uint32_t *marked_dp,
    ca_uint32_t *unmarked_dp
)
{
    if (index > CA_AAL_L3_WRED_MAX_PROFILE_ID)
        return CA_E_PARAM;

    if (!marked_dp)
        return CA_E_PARAM;

    if (!unmarked_dp)
        return CA_E_PARAM;

    memcpy(g_aal_l3_te_wred_profile_info[index].marked_dp, marked_dp, sizeof(g_aal_l3_te_wred_profile_info[index].marked_dp));
    memcpy(g_aal_l3_te_wred_profile_info[index].unmarked_dp, unmarked_dp, sizeof(g_aal_l3_te_wred_profile_info[index].unmarked_dp));

    return CA_E_OK;
}

ca_status_t aal_l3_te_wred_profile_info_get
(
    ca_uint32_t index,
    ca_uint32_t *marked_dp,
    ca_uint32_t *unmarked_dp
)
{
    if (index > CA_AAL_L3_WRED_MAX_PROFILE_ID)
        return CA_E_PARAM;

    if (!marked_dp)
        return CA_E_PARAM;

    if (!unmarked_dp)
        return CA_E_PARAM;

    memcpy(marked_dp, g_aal_l3_te_wred_profile_info[index].marked_dp, sizeof(g_aal_l3_te_wred_profile_info[index].marked_dp));
    memcpy(unmarked_dp, g_aal_l3_te_wred_profile_info[index].unmarked_dp, sizeof(g_aal_l3_te_wred_profile_info[index].unmarked_dp));

    return CA_E_OK;
}


ca_uint32_t aal_l3_te_wred_profile_info_ref_counter_get(ca_uint32_t index)
{
    return g_aal_l3_te_wred_profile_info[index].ref_counter;
}

void aal_l3_te_wred_profile_info_ref_inc(ca_uint32_t index)
{
    g_aal_l3_te_wred_profile_info[index].ref_counter++;
}

void aal_l3_te_wred_profile_info_ref_dec(ca_uint32_t index)
{
    if (g_aal_l3_te_wred_profile_info[index].ref_counter)
        g_aal_l3_te_wred_profile_info[index].ref_counter--;
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

ca_uint8_t aal_l3_te_wred_sel_info_get
(
    ca_port_id_t port,
    ca_uint32_t  queue
)
{
    if (port <= CA_AAL_L3_WRED_MAX_PORT_ID && queue <= 7)
        return g_aal_l3_te_wred_sel_info[port][queue];
    else
        return 0xff;
}

ca_status_t aal_l3_te_wred_global_free_mem_size_set
(
    ca_device_id_t  device_id,
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
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd));
}

ca_status_t aal_l3_te_wred_global_free_mem_size_get
(
    ca_device_id_t  device_id,
    ca_uint32_t  pool_id,
    ca_uint32_t  *depth,
    ca_uint32_t  *depth_eq1
)
{
    TE_WRED_AFGS_MEM_DATA_t value0;
    ca_uint32_t index = pool_id;

    if (!depth || !depth_eq1)
        return CA_E_PARAM;

    if (pool_id >= CA_AAL_L3_WRED_MAX_GLOBAL_POOL_NUMS)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));

    /* read table */
    if (__l3_te_wred_free_global_mem_size_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd))
            != CA_E_OK)
            return CA_E_ERROR;

    *depth = value0.bf.depth;
    *depth_eq1 = value0.bf.depth_eq1;

    return CA_E_OK;
}

ca_status_t aal_l3_te_wred_debug_get(ca_device_id_t device_id)
{
    ca_uint32_t i = 0, j = 0;
    ca_int8_t str_marked[256] = {0};
    ca_int8_t str_unmarked[256] = {0};

    PRINTF("============= S/W WRED profile table ============= \n");

    for (i=0; i<CA_AAL_L3_WRED_MAX_PROFILE_NUMS; i++)
    {
        PRINTF("index %02d : ref_counter(%d)\n", i, g_aal_l3_te_wred_profile_info[i].ref_counter);
        sprintf(str_marked, "  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
            g_aal_l3_te_wred_profile_info[i].marked_dp[0], g_aal_l3_te_wred_profile_info[i].marked_dp[1],
            g_aal_l3_te_wred_profile_info[i].marked_dp[2], g_aal_l3_te_wred_profile_info[i].marked_dp[3],
            g_aal_l3_te_wred_profile_info[i].marked_dp[4], g_aal_l3_te_wred_profile_info[i].marked_dp[5],
            g_aal_l3_te_wred_profile_info[i].marked_dp[6], g_aal_l3_te_wred_profile_info[i].marked_dp[7],
            g_aal_l3_te_wred_profile_info[i].marked_dp[8], g_aal_l3_te_wred_profile_info[i].marked_dp[9],
            g_aal_l3_te_wred_profile_info[i].marked_dp[10], g_aal_l3_te_wred_profile_info[i].marked_dp[11],
            g_aal_l3_te_wred_profile_info[i].marked_dp[12], g_aal_l3_te_wred_profile_info[i].marked_dp[13],
            g_aal_l3_te_wred_profile_info[i].marked_dp[14], g_aal_l3_te_wred_profile_info[i].marked_dp[15]);
        sprintf(str_unmarked, "  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
            g_aal_l3_te_wred_profile_info[i].unmarked_dp[0], g_aal_l3_te_wred_profile_info[i].unmarked_dp[1],
            g_aal_l3_te_wred_profile_info[i].unmarked_dp[2], g_aal_l3_te_wred_profile_info[i].unmarked_dp[3],
            g_aal_l3_te_wred_profile_info[i].unmarked_dp[4], g_aal_l3_te_wred_profile_info[i].unmarked_dp[5],
            g_aal_l3_te_wred_profile_info[i].unmarked_dp[6], g_aal_l3_te_wred_profile_info[i].unmarked_dp[7],
            g_aal_l3_te_wred_profile_info[i].unmarked_dp[8], g_aal_l3_te_wred_profile_info[i].unmarked_dp[9],
            g_aal_l3_te_wred_profile_info[i].unmarked_dp[10], g_aal_l3_te_wred_profile_info[i].unmarked_dp[11],
            g_aal_l3_te_wred_profile_info[i].unmarked_dp[12], g_aal_l3_te_wred_profile_info[i].unmarked_dp[13],
            g_aal_l3_te_wred_profile_info[i].unmarked_dp[14], g_aal_l3_te_wred_profile_info[i].unmarked_dp[15]);

        PRINTF("%s", str_marked);
        PRINTF("%s", str_unmarked);
    }

    PRINTF("\n");

    PRINTF("===== S/W WRED Selection table ===== \n");
    PRINTF("          Q0 Q1 Q2 Q3 Q4 Q5 Q6 Q7\n");
    PRINTF("          -- -- -- -- -- -- -- --\n");

    for (i=0; i<=CA_AAL_L3_WRED_MAX_PORT_ID; i++)
    {
        PRINTF("port %02d : ", i);
        for (j=0; j<8; j++)
        {
            PRINTF("%02d ", g_aal_l3_te_wred_sel_info[i][j]);
        }
        PRINTF("\n");
    }

    return CA_E_OK;
}


#ifdef __L3_PM
#endif
static inline ca_status_t __l3_te_pm_counter_access
(
    ca_device_id_t          device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0,
    volatile ca_uint32_t *data1,
    volatile ca_uint32_t *data2,
    volatile ca_uint32_t *data3,
    volatile ca_uint32_t *data4,
    volatile ca_uint32_t *data5,
    volatile ca_uint32_t *data6,
    volatile ca_uint32_t *data7,
    volatile ca_uint32_t *data8
)
{
    TE_PM_CNT_MEM_ACCESS_t counter_access;
    ca_uint32_t i = 0;

    counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_ACCESS);

    while (counter_access.bf.access)
    {
        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CNT_MEM_DATA0, *data0);
        CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CNT_MEM_DATA1, *data1);
        CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CNT_MEM_DATA2, *data2);
        CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CNT_MEM_DATA3, *data3);
        CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CNT_MEM_DATA4, *data4);
        CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CNT_MEM_DATA5, *data5);
        CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CNT_MEM_DATA6, *data6);
        CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CNT_MEM_DATA7, *data7);
        CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CNT_MEM_DATA8, *data8);
    }

    memset((void *)&counter_access, 0, sizeof(counter_access));

    counter_access.bf.address  = addr;
    counter_access.bf.rbw      = op_flag;
    counter_access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CNT_MEM_ACCESS, counter_access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_ACCESS);
        while (counter_access.bf.access)
        {
            counter_access.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_DATA0);
        *data1 = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_DATA1);
        *data2 = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_DATA2);
        *data3 = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_DATA3);
        *data4 = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_DATA4);
        *data5 = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_DATA5);
        *data6 = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_DATA6);
        *data7 = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_DATA7);
        *data8 = CA_DEV_NE_REG_READ(device_id,TE_PM_CNT_MEM_DATA8);
    }

    return CA_E_OK;
}


ca_status_t aal_l3_te_pm_policer_flow_get
(
    ca_device_id_t            device_id,
    ca_flow_id_t           flow_id,
    aal_l3_te_pm_policer_t *pm
)
{
    TE_PM_CNT_MEM_DATA0_t value0;
    TE_PM_CNT_MEM_DATA1_t value1;
    TE_PM_CNT_MEM_DATA2_t value2;
    TE_PM_CNT_MEM_DATA3_t value3;
    TE_PM_CNT_MEM_DATA4_t value4;
    TE_PM_CNT_MEM_DATA5_t value5;
    TE_PM_CNT_MEM_DATA6_t value6;
    TE_PM_CNT_MEM_DATA7_t value7;
    TE_PM_CNT_MEM_DATA8_t value8;
    ca_uint64_t tmp_val1 = 0;
    ca_uint64_t tmp_val2 = 0;
    ca_uint32_t index = flow_id + CA_AAL_L3_TE_POL_PM_FLOW_OFFSET;

    if (!pm)
        return CA_E_PARAM;

    if (flow_id > CA_AAL_L3_MAX_FLOW_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));
    memset((void *)&(value3), 0, sizeof(value3));
    memset((void *)&(value4), 0, sizeof(value4));
    memset((void *)&(value5), 0, sizeof(value5));
    memset((void *)&(value6), 0, sizeof(value6));
    memset((void *)&(value7), 0, sizeof(value7));
    memset((void *)&(value8), 0, sizeof(value8));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_pm_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd),
            (volatile ca_uint32_t *)&(value3.wrd),
            (volatile ca_uint32_t *)&(value4.wrd),
            (volatile ca_uint32_t *)&(value5.wrd),
            (volatile ca_uint32_t *)&(value6.wrd),
            (volatile ca_uint32_t *)&(value7.wrd),
            (volatile ca_uint32_t *)&(value8.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    pm->total_pkt  = value0.wrd;
    pm->red_pkt    = value1.wrd;
    pm->yellow_pkt = value2.wrd;
    pm->green_pkt  = value3.wrd;

    tmp_val1 = value5.wrd;
    pm->total_bytes      = value4.wrd | (tmp_val1 & 0xf) << 32;  /* total 36 bits */

    tmp_val1 = value5.wrd;
    tmp_val2 = value6.wrd;

    pm->red_bytes = (tmp_val1 >> 4) | ((tmp_val2 & 0xff) << 28);

    tmp_val1 = value6.wrd;
    tmp_val2 = value7.wrd;

    pm->yellow_bytes = (tmp_val1 >> 8) | ((tmp_val2 & 0xfff) << 24);

    tmp_val1 = value7.wrd;
    tmp_val2 = value8.wrd;
    pm->green_bytes = (tmp_val1 >> 12) | ((tmp_val2 & 0xffff) << 20);

    return CA_E_OK;

}


ca_status_t aal_l3_te_pm_policer_agr_flow_get
(
    ca_device_id_t            device_id,
    ca_uint32_t            agr_flow_id,
    aal_l3_te_pm_policer_t *pm
)
{
    TE_PM_CNT_MEM_DATA0_t value0;
    TE_PM_CNT_MEM_DATA1_t value1;
    TE_PM_CNT_MEM_DATA2_t value2;
    TE_PM_CNT_MEM_DATA3_t value3;
    TE_PM_CNT_MEM_DATA4_t value4;
    TE_PM_CNT_MEM_DATA5_t value5;
    TE_PM_CNT_MEM_DATA6_t value6;
    TE_PM_CNT_MEM_DATA7_t value7;
    TE_PM_CNT_MEM_DATA8_t value8;
    ca_uint64_t tmp_val1 = 0;
    ca_uint64_t tmp_val2 = 0;
    ca_uint32_t index = agr_flow_id + CA_AAL_L3_TE_POL_PM_AGR_FLOW_OFFSET;

    TE_TE_GLB_CTRL_t glb_ctrl;

    glb_ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_TE_GLB_CTRL);

    /* 0 = 512 flow and 8 aggregate flow
       1 = 512 flow, 64 flow2 and 64 flow3 */
    if (glb_ctrl.bf.sel3flows == 0 && agr_flow_id > CA_AAL_L3_MAX_AGR_FLOW_ID)
        return CA_E_PARAM;
    else if (glb_ctrl.bf.sel3flows == 1 && agr_flow_id > CA_AAL_L3_MAX_FLOW2_ID)
        return CA_E_PARAM;

    if (!pm)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));
    memset((void *)&(value3), 0, sizeof(value3));
    memset((void *)&(value4), 0, sizeof(value4));
    memset((void *)&(value5), 0, sizeof(value5));
    memset((void *)&(value6), 0, sizeof(value6));
    memset((void *)&(value7), 0, sizeof(value7));
    memset((void *)&(value8), 0, sizeof(value8));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_pm_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd),
            (volatile ca_uint32_t *)&(value3.wrd),
            (volatile ca_uint32_t *)&(value4.wrd),
            (volatile ca_uint32_t *)&(value5.wrd),
            (volatile ca_uint32_t *)&(value6.wrd),
            (volatile ca_uint32_t *)&(value7.wrd),
            (volatile ca_uint32_t *)&(value8.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    pm->total_pkt  = value0.wrd;
    pm->red_pkt    = value1.wrd;
    pm->yellow_pkt = value2.wrd;
    pm->green_pkt  = value3.wrd;

    tmp_val1 = value5.wrd;
    pm->total_bytes      = value4.wrd | (tmp_val1 & 0xf) << 32;  /* total 36 bits */

    tmp_val1 = value5.wrd;
    tmp_val2 = value6.wrd;

    pm->red_bytes = (tmp_val1 >> 4) | ((tmp_val2 & 0xff) << 28);

    tmp_val1 = value6.wrd;
    tmp_val2 = value7.wrd;

    pm->yellow_bytes = (tmp_val1 >> 8) | ((tmp_val2 & 0xfff) << 24);

    tmp_val1 = value7.wrd;
    tmp_val2 = value8.wrd;
    pm->green_bytes = (tmp_val1 >> 12) | ((tmp_val2 & 0xffff) << 20);

    return CA_E_OK;

}


ca_status_t aal_l3_te_pm_policer_flow3_get
(
    ca_device_id_t            device_id,
    ca_uint32_t            flow3_id,
    aal_l3_te_pm_policer_t *pm
)
{
    TE_PM_CNT_MEM_DATA0_t value0;
    TE_PM_CNT_MEM_DATA1_t value1;
    TE_PM_CNT_MEM_DATA2_t value2;
    TE_PM_CNT_MEM_DATA3_t value3;
    TE_PM_CNT_MEM_DATA4_t value4;
    TE_PM_CNT_MEM_DATA5_t value5;
    TE_PM_CNT_MEM_DATA6_t value6;
    TE_PM_CNT_MEM_DATA7_t value7;
    TE_PM_CNT_MEM_DATA8_t value8;
    ca_uint64_t tmp_val1 = 0;
    ca_uint64_t tmp_val2 = 0;
    ca_uint32_t index = flow3_id + CA_AAL_L3_TE_POL_PM_FLOW3_OFFSET;

    if (!pm)
        return CA_E_PARAM;

    if (flow3_id > CA_AAL_L3_MAX_FLOW3_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));
    memset((void *)&(value3), 0, sizeof(value3));
    memset((void *)&(value4), 0, sizeof(value4));
    memset((void *)&(value5), 0, sizeof(value5));
    memset((void *)&(value6), 0, sizeof(value6));
    memset((void *)&(value7), 0, sizeof(value7));
    memset((void *)&(value8), 0, sizeof(value8));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_pm_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd),
            (volatile ca_uint32_t *)&(value3.wrd),
            (volatile ca_uint32_t *)&(value4.wrd),
            (volatile ca_uint32_t *)&(value5.wrd),
            (volatile ca_uint32_t *)&(value6.wrd),
            (volatile ca_uint32_t *)&(value7.wrd),
            (volatile ca_uint32_t *)&(value8.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    pm->total_pkt  = value0.wrd;
    pm->red_pkt    = value1.wrd;
    pm->yellow_pkt = value2.wrd;
    pm->green_pkt  = value3.wrd;

    tmp_val1 = value5.wrd;
    pm->total_bytes      = value4.wrd | (tmp_val1 & 0xf) << 32;  /* total 36 bits */

    tmp_val1 = value5.wrd;
    tmp_val2 = value6.wrd;

    pm->red_bytes = (tmp_val1 >> 4) | ((tmp_val2 & 0xff) << 28);

    tmp_val1 = value6.wrd;
    tmp_val2 = value7.wrd;

    pm->yellow_bytes = (tmp_val1 >> 8) | ((tmp_val2 & 0xfff) << 24);

    tmp_val1 = value7.wrd;
    tmp_val2 = value8.wrd;
    pm->green_bytes = (tmp_val1 >> 12) | ((tmp_val2 & 0xffff) << 20);

    return CA_E_OK;

}
ca_status_t aal_l3_te_pm_policer_port_get
(
    ca_device_id_t            device_id,
    ca_port_id_t           port_id,
    aal_l3_te_pm_policer_t *pm
)
{
    TE_PM_CNT_MEM_DATA0_t value0;
    TE_PM_CNT_MEM_DATA1_t value1;
    TE_PM_CNT_MEM_DATA2_t value2;
    TE_PM_CNT_MEM_DATA3_t value3;
    TE_PM_CNT_MEM_DATA4_t value4;
    TE_PM_CNT_MEM_DATA5_t value5;
    TE_PM_CNT_MEM_DATA6_t value6;
    TE_PM_CNT_MEM_DATA7_t value7;
    TE_PM_CNT_MEM_DATA8_t value8;
    ca_uint64_t tmp_val1 = 0;
    ca_uint64_t tmp_val2 = 0;
    ca_uint32_t index = port_id + CA_AAL_L3_TE_POL_PM_SRC_PORT_OFFSET;

    if (!pm)
        return CA_E_PARAM;

#if 1//defined(CONFIG_LUNA_G3_SERIES)
    if (port_id > (CA_AAL_L3_MAX_PORT_ID+1))
        return CA_E_PARAM;
#else
    if (port_id > CA_AAL_L3_MAX_PORT_ID)
        return CA_E_PARAM;
#endif

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));
    memset((void *)&(value3), 0, sizeof(value3));
    memset((void *)&(value4), 0, sizeof(value4));
    memset((void *)&(value5), 0, sizeof(value5));
    memset((void *)&(value6), 0, sizeof(value6));
    memset((void *)&(value7), 0, sizeof(value7));
    memset((void *)&(value8), 0, sizeof(value8));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_pm_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd),
            (volatile ca_uint32_t *)&(value3.wrd),
            (volatile ca_uint32_t *)&(value4.wrd),
            (volatile ca_uint32_t *)&(value5.wrd),
            (volatile ca_uint32_t *)&(value6.wrd),
            (volatile ca_uint32_t *)&(value7.wrd),
            (volatile ca_uint32_t *)&(value8.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    pm->total_pkt  = value0.wrd;
    pm->red_pkt    = value1.wrd;
    pm->yellow_pkt = value2.wrd;
    pm->green_pkt  = value3.wrd;

    tmp_val1 = value5.wrd;
    pm->total_bytes      = value4.wrd | (tmp_val1 & 0xf) << 32;  /* total 36 bits */

    tmp_val1 = value5.wrd;
    tmp_val2 = value6.wrd;

    pm->red_bytes = (tmp_val1 >> 4) | ((tmp_val2 & 0xff) << 28);

    tmp_val1 = value6.wrd;
    tmp_val2 = value7.wrd;

    pm->yellow_bytes = (tmp_val1 >> 8) | ((tmp_val2 & 0xfff) << 24);

    tmp_val1 = value7.wrd;
    tmp_val2 = value8.wrd;
    pm->green_bytes = (tmp_val1 >> 12) | ((tmp_val2 & 0xffff) << 20);

    return CA_E_OK;

}

ca_status_t aal_l3_te_pm_policer_pkt_type_get
(
    ca_device_id_t                  device_id,
    aal_l3_te_policer_pkt_type_t pkt_type,
    aal_l3_te_pm_policer_t       *pm
)
{
    TE_PM_CNT_MEM_DATA0_t value0;
    TE_PM_CNT_MEM_DATA1_t value1;
    TE_PM_CNT_MEM_DATA2_t value2;
    TE_PM_CNT_MEM_DATA3_t value3;
    TE_PM_CNT_MEM_DATA4_t value4;
    TE_PM_CNT_MEM_DATA5_t value5;
    TE_PM_CNT_MEM_DATA6_t value6;
    TE_PM_CNT_MEM_DATA7_t value7;
    TE_PM_CNT_MEM_DATA8_t value8;
    ca_uint64_t tmp_val1 = 0;
    ca_uint64_t tmp_val2 = 0;
    ca_uint32_t index = (ca_uint32_t)pkt_type + CA_AAL_L3_TE_POL_PM_PKT_TYPE_OFFSET;

    if (!pm)
        return CA_E_PARAM;

    if (pkt_type >= CA_AAL_L3_POLICER_PKT_TYPE_END)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));
    memset((void *)&(value3), 0, sizeof(value3));
    memset((void *)&(value4), 0, sizeof(value4));
    memset((void *)&(value5), 0, sizeof(value5));
    memset((void *)&(value6), 0, sizeof(value6));
    memset((void *)&(value7), 0, sizeof(value7));
    memset((void *)&(value8), 0, sizeof(value8));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_pm_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd),
            (volatile ca_uint32_t *)&(value3.wrd),
            (volatile ca_uint32_t *)&(value4.wrd),
            (volatile ca_uint32_t *)&(value5.wrd),
            (volatile ca_uint32_t *)&(value6.wrd),
            (volatile ca_uint32_t *)&(value7.wrd),
            (volatile ca_uint32_t *)&(value8.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    pm->total_pkt  = value0.wrd;
    pm->red_pkt    = value1.wrd;
    pm->yellow_pkt = value2.wrd;
    pm->green_pkt  = value3.wrd;

    tmp_val1 = value5.wrd;
    pm->total_bytes      = value4.wrd | (tmp_val1 & 0xf) << 32;  /* total 36 bits */

    tmp_val1 = value5.wrd;
    tmp_val2 = value6.wrd;

    pm->red_bytes = (tmp_val1 >> 4) | ((tmp_val2 & 0xff) << 28);

    tmp_val1 = value6.wrd;
    tmp_val2 = value7.wrd;

    pm->yellow_bytes = (tmp_val1 >> 8) | ((tmp_val2 & 0xfff) << 24);

    tmp_val1 = value7.wrd;
    tmp_val2 = value8.wrd;
    pm->green_bytes = (tmp_val1 >> 12) | ((tmp_val2 & 0xffff) << 20);

    return CA_E_OK;

}

ca_status_t aal_l3_te_pm_egress_voq_get
(
    ca_device_id_t           device_id,
    ca_uint32_t           voq_id,
    aal_l3_te_pm_egress_t *pm
)
{
    TE_PM_CNT_MEM_DATA0_t value0;
    TE_PM_CNT_MEM_DATA1_t value1;
    TE_PM_CNT_MEM_DATA2_t value2;
    TE_PM_CNT_MEM_DATA3_t value3;
    TE_PM_CNT_MEM_DATA4_t value4;
    TE_PM_CNT_MEM_DATA5_t value5;
    TE_PM_CNT_MEM_DATA6_t value6;
    TE_PM_CNT_MEM_DATA7_t value7;
    TE_PM_CNT_MEM_DATA8_t value8;
    ca_uint64_t tmp_val1 = 0;
    ca_uint64_t tmp_val2 = 0;
    ca_uint32_t index = voq_id + CA_AAL_L3_TE_PM_MEM_BLOCK_SIZE;

    if (!pm)
        return CA_E_PARAM;

    if (voq_id > AAL_L3TE_MAX_VOQ_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));
    memset((void *)&(value3), 0, sizeof(value3));
    memset((void *)&(value4), 0, sizeof(value4));
    memset((void *)&(value5), 0, sizeof(value5));
    memset((void *)&(value6), 0, sizeof(value6));
    memset((void *)&(value7), 0, sizeof(value7));
    memset((void *)&(value8), 0, sizeof(value8));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_pm_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd),
            (volatile ca_uint32_t *)&(value3.wrd),
            (volatile ca_uint32_t *)&(value4.wrd),
            (volatile ca_uint32_t *)&(value5.wrd),
            (volatile ca_uint32_t *)&(value6.wrd),
            (volatile ca_uint32_t *)&(value7.wrd),
            (volatile ca_uint32_t *)&(value8.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    pm->total_pkt       = value0.wrd;
    pm->td_drop_pkt     = value1.wrd;
    pm->wred_mark_pkt   = value2.wrd;
    pm->wred_unmark_pkt = value3.wrd;

    tmp_val1 = value5.wrd;
    pm->total_bytes      = value4.wrd | (tmp_val1 & 0xf) << 32;  /* total 36 bits */

    tmp_val1 = value5.wrd;
    tmp_val2 = value6.wrd;

    pm->td_drop_bytes = (tmp_val1 >> 4) | ((tmp_val2 & 0xff) << 28);

    tmp_val1 = value6.wrd;
    tmp_val2 = value7.wrd;

    pm->wred_mark_bytes = (tmp_val1 >> 8) | ((tmp_val2 & 0xfff) << 24);

    tmp_val1 = value7.wrd;
    tmp_val2 = value8.wrd;
    pm->wred_unmark_bytes = (tmp_val1 >> 12) | ((tmp_val2 & 0xffff) << 20);


    return CA_E_OK;

}

ca_status_t aal_l3_te_pm_egress_port_get
(
    ca_device_id_t        device_id,
    ca_uint32_t           port_id,
    aal_l3_te_pm_egress_t *pm
)
{
    TE_PM_CNT_MEM_DATA0_t value0;
    TE_PM_CNT_MEM_DATA1_t value1;
    TE_PM_CNT_MEM_DATA2_t value2;
    TE_PM_CNT_MEM_DATA3_t value3;
    TE_PM_CNT_MEM_DATA4_t value4;
    TE_PM_CNT_MEM_DATA5_t value5;
    TE_PM_CNT_MEM_DATA6_t value6;
    TE_PM_CNT_MEM_DATA7_t value7;
    TE_PM_CNT_MEM_DATA8_t value8;
    ca_uint64_t tmp_val1 = 0;
    ca_uint64_t tmp_val2 = 0;
    ca_uint32_t index = port_id + CA_AAL_L3_TE_POL_PM_DEST_PORT_OFFSET;

    if (!pm)
        return CA_E_PARAM;

    if (port_id > AAL_L3TE_MAX_PORT_ID)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));
    memset((void *)&(value1), 0, sizeof(value1));
    memset((void *)&(value2), 0, sizeof(value2));
    memset((void *)&(value3), 0, sizeof(value3));
    memset((void *)&(value4), 0, sizeof(value4));
    memset((void *)&(value5), 0, sizeof(value5));
    memset((void *)&(value6), 0, sizeof(value6));
    memset((void *)&(value7), 0, sizeof(value7));
    memset((void *)&(value8), 0, sizeof(value8));

    /* get current configuration at first and then update specificed field */
    if (__l3_te_pm_counter_access(
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd),
            (volatile ca_uint32_t *)&(value1.wrd),
            (volatile ca_uint32_t *)&(value2.wrd),
            (volatile ca_uint32_t *)&(value3.wrd),
            (volatile ca_uint32_t *)&(value4.wrd),
            (volatile ca_uint32_t *)&(value5.wrd),
            (volatile ca_uint32_t *)&(value6.wrd),
            (volatile ca_uint32_t *)&(value7.wrd),
            (volatile ca_uint32_t *)&(value8.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    pm->total_pkt       = value0.wrd;
    pm->td_drop_pkt     = value1.wrd;
    pm->wred_mark_pkt   = value2.wrd;
    pm->wred_unmark_pkt = value3.wrd;

    tmp_val1 = value5.wrd;
    pm->total_bytes      = value4.wrd | (tmp_val1 & 0xf) << 32;  /* total 36 bits */

    tmp_val1 = value5.wrd;
    tmp_val2 = value6.wrd;

    pm->td_drop_bytes = (tmp_val1 >> 4) | ((tmp_val2 & 0xff) << 28);

    tmp_val1 = value6.wrd;
    tmp_val2 = value7.wrd;

    pm->wred_mark_bytes = (tmp_val1 >> 8) | ((tmp_val2 & 0xfff) << 24);

    tmp_val1 = value7.wrd;
    tmp_val2 = value8.wrd;
    pm->wred_unmark_bytes = (tmp_val1 >> 12) | ((tmp_val2 & 0xffff) << 20);


    return CA_E_OK;

}

ca_status_t aal_l3_te_pm_enable_set
(
    ca_device_id_t device_id,
    ca_boolean_t enable
)
{
    TE_PM_CTRL_t value;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_CTRL);

    value.bf.mem_disable = enable ? 0 : 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_pm_enable_get
(
    ca_device_id_t device_id,
    ca_boolean_t *enable
)
{
    TE_PM_CTRL_t value;

    if (!enable)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_CTRL);

    *enable = value.bf.mem_disable ? FALSE : TRUE;

    return CA_E_OK;
}


ca_status_t aal_l3_te_pm_read_clr_set
(
    ca_device_id_t  device_id,
    ca_boolean_t read_clr
)
{
    TE_PM_CTRL_t value;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_CTRL);

    value.bf.mem_auto_clr_on_read = read_clr;

    CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l3_te_pm_read_clr_get
(
    ca_device_id_t  device_id,
    ca_boolean_t *read_clr
)
{
    TE_PM_CTRL_t value;

    if (!read_clr)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_CTRL);

    *read_clr = value.bf.mem_auto_clr_on_read ? TRUE :FALSE ;

    return CA_E_OK;
}

ca_status_t aal_l3_te_pm_cnt_mem_set
(
    ca_device_id_t device_id,
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

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_CTRL);

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

    CA_DEV_NE_REG_WRITE2(device_id,TE_PM_CTRL, value.wrd);

    return CA_E_OK;
}


ca_status_t aal_l3_te_pm_cnt_mem_get
(
    ca_device_id_t device_id,
    ca_uint32_t block_id,
    ca_uint32_t *sel_id,
    ca_uint32_t *sel_event
)
{
    TE_PM_CTRL_t value;

    if (block_id > 1)
        return CA_E_PARAM;

    if (!sel_id)
        return CA_E_PARAM;

    if (!sel_event)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_CTRL);

    if (block_id == 0)
    {
        *sel_id    = value.bf.cnt0_mem_id_sel;
        *sel_event = value.bf.cnt0_mem_event_sel;
    }
    else
    {
        *sel_id    = value.bf.cnt1_mem_id_sel;
        *sel_event = value.bf.cnt1_mem_event_sel;
    }

    return CA_E_OK;
}


ca_status_t aal_l3_te_pm_status_get
(
    ca_device_id_t           device_id,
    aal_l3_te_pm_status_t *status
)
{
    TE_PM_STS_t value;

    if (!status)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PM_STS);

    status->pkt_cnt0_init_done  = value.bf.pcnt0_init_done;
    status->pkt_cnt1_init_done  = value.bf.pcnt1_init_done;
    status->byte_cnt0_init_done = value.bf.bcnt0_init_done;
    status->byte_cnt1_init_done = value.bf.bcnt1_init_done;

    return CA_E_OK;
}

ca_status_t aal_l3_te_pm_stats_get
(
    ca_device_id_t device_id,
    aal_l3_te_pm_stats_t *stats
)
{
    if (!stats)
        return CA_E_ERROR;

    stats->total_cnt            = CA_DEV_NE_REG_READ(device_id,TE_PM_PCNT);
    stats->pre_mark_cnt         = CA_DEV_NE_REG_READ(device_id,TE_PM_PRE_MARK_PCNT);
    stats->policer_drop_cnt     = CA_DEV_NE_REG_READ(device_id,TE_PM_POL_DROP_PCNT);
    stats->policer_mark_cnt     = CA_DEV_NE_REG_READ(device_id,TE_PM_MARK_PCNT);
    stats->tail_drop_cnt        = CA_DEV_NE_REG_READ(device_id,TE_PM_TAIL_DROP_PCNT);
    stats->wred_yellow_drop_cnt = CA_DEV_NE_REG_READ(device_id,TE_PM_WRED_Y_DROP_PCNT);
    stats->wred_green_drop_cnt  = CA_DEV_NE_REG_READ(device_id,TE_PM_WRED_G_DROP_PCNT);
    stats->mark_cnt             = CA_DEV_NE_REG_READ(device_id,TE_PM_MARK_PCNT);
    stats->bypass_cnt           = CA_DEV_NE_REG_READ(device_id,TE_PM_BYPASS_PCNT);
    stats->bypass_flow_cnt      = CA_DEV_NE_REG_READ(device_id,TE_PM_BYPASS_FLOW_PCNT);
    stats->ce_cnt               = CA_DEV_NE_REG_READ(device_id,TE_PM_CE_PCNT);

    return CA_E_OK;
}

#ifdef __L3_PIE
#endif
static inline ca_status_t __l3_te_pie_lpid_qos_voq_access
(
    ca_device_id_t       device_id,
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data
)
{
    TE_PIE_LPID_QOS_VOQ_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_LPID_QOS_VOQ_ACCESS);

    while (access.bf.access)
    {
        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_LPID_QOS_VOQ_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_LPID_QOS_VOQ_DATA, *data);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_LPID_QOS_VOQ_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_LPID_QOS_VOQ_ACCESS);
        while (access.bf.access)
        {
            access.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_LPID_QOS_VOQ_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data = CA_DEV_NE_REG_READ(device_id,TE_PIE_LPID_QOS_VOQ_DATA);
    }

    return CA_E_OK;
}

ca_status_t aal_l3_te_pie_lpid_qos_voq_set
(
    ca_device_id_t                   device_id,
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
            device_id,
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
            device_id,
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd));
}

ca_status_t aal_l3_te_pie_lpid_qos_voq_get
(
    ca_device_id_t               device_id,
    ca_port_id_t                 lpid,
    ca_uint32_t                  cos,
    aal_l3_te_pie_lpid_qos_voq_t *cfg
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
            device_id,
            AAL_TBL_OP_FLAG_RD,
            index,
            (volatile ca_uint32_t *)&(value0.wrd))
            != CA_E_OK)
        return CA_E_ERROR;

    cfg->ena = (value0.bf.value >> 4) & 0x1;
    cfg->voq = value0.bf.value & 0xf;

    return CA_E_OK;

}

/**********************************************************************
** set the buffer queue depth marked for the PIE 1/3 depth;
** the configuration is precalculated number of bytes
** that are in the queue that specifies when we reach 1/3 queue depth
**********************************************************************/
ca_status_t aal_l3_te_pie_buff_threshold_set
(
    ca_device_id_t device_id,
    ca_uint8_t     flowType,
    ca_uint32_t    size
)
{
    TE_PIE_buf_t value;

    if (size > 0x1fffff)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_buf + flowType * 4);

    value.bf.one_third = size;

    CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_buf + flowType * 4, value.wrd);

    return CA_E_OK;

}

ca_status_t aal_l3_te_pie_buff_threshold_get
(
    ca_device_id_t device_id,
    ca_uint8_t     flowType,
    ca_uint32_t    *size
)
{
    TE_PIE_buf_t value;

    if (!size)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_buf + flowType * 4);

    *size = value.bf.one_third;

    return CA_E_OK;

}

ca_status_t aal_l3_te_pie_dp_scaling_set
(
    ca_device_id_t                 device_id,
    ca_uint8_t                     flowType,
    aal_l3_te_pie_dp_scaling_msk_t msk,
    aal_l3_te_pie_dp_scaling_t     *value
)
{
    TE_PIE_dp0_t val;

    if (!value)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    val.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_dp0 + flowType * 4);

    if (msk.s.value_a)
    {
        if (value->value_a > 0x3f)
            return CA_E_PARAM;

        val.bf.A = value->value_a;
    }

    if (msk.s.value_b)
    {
        if (value->value_b > 0x3ff)
            return CA_E_PARAM;

        val.bf.B = value->value_b;
    }

    CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_dp0 + flowType * 4, val.wrd);

    return CA_E_OK;

}

ca_status_t aal_l3_te_pie_dp_scaling_get
(
    ca_device_id_t             device_id,
    ca_uint8_t                 flowType,
    aal_l3_te_pie_dp_scaling_t *value
)
{
    TE_PIE_dp0_t val;

    if (!value)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    val.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_dp0 + flowType * 4);

    value->value_a = val.bf.A;
    value->value_b = val.bf.B;

    return CA_E_OK;

}

ca_status_t aal_l3_te_pie_dp_ceiling_set
(
    ca_device_id_t device_id,
    ca_uint8_t     flowType,
    ca_uint32_t    ceiling
)
{
    TE_PIE_dp1_t value;

    if (ceiling > 0xfffffff)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_dp1 + flowType * 4);

    value.bf.ceiling = ceiling;

    CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_dp1 + flowType * 4, value.wrd);

    return CA_E_OK;

}

ca_status_t aal_l3_te_pie_dp_ceiling_get
(
    ca_device_id_t device_id,
    ca_uint8_t     flowType,
    ca_uint32_t    *ceiling
)
{
    TE_PIE_dp1_t value;

    if (!ceiling)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_dp1 + flowType * 4);

    *ceiling = value.bf.ceiling;

    return CA_E_OK;

}

ca_status_t aal_l3_te_pie_dp_cfg_set
(
    ca_device_id_t         device_id,
    ca_uint8_t             flowType,
    ca_uint32_t            pie_voq_id,
    aal_l3_te_pie_dp_msk_t msk,
    aal_l3_te_pie_dp_cfg_t *cfg
)
{
    TE_PIE_dp2_0_t val20;
    TE_PIE_dp3_0_t val30;
    TE_PIE_dp4_0_t val40;
    TE_PIE_dp5_0_t val50;
    TE_PIE_dp6_0_t val60;
    TE_PIE_dp8_0_t val80;
    ca_uint32_t addr = 0;

    if (!cfg)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    if (pie_voq_id > CA_AAL_L3_PIE_MAX_VOQ)
        return CA_E_PARAM;

    if (msk.s.burst_reset_timeout)
    {
        if (cfg->burst_reset_timeout > 0x1fffff)
            return CA_E_PARAM;

        addr = TE_PIE_dp2_0 + flowType * 4 + pie_voq_id * TE_PIE_dp2_0_STRIDE;

        val20.wrd = CA_DEV_NE_REG_READ(device_id,addr);

        val20.bf.burst_reset_timeout = cfg->burst_reset_timeout;

        CA_DEV_NE_REG_WRITE2(device_id,addr, val20.wrd);
    }

    if (msk.s.latency_threshold_hi)
    {
        if (cfg->latency_threshold_hi > 0x1fffff)
            return CA_E_PARAM;

        addr = TE_PIE_dp3_0 + flowType * 4 + pie_voq_id * TE_PIE_dp3_0_STRIDE;

        val30.wrd = CA_DEV_NE_REG_READ(device_id,addr);

        val30.bf.latency_high = cfg->latency_threshold_hi;

        CA_DEV_NE_REG_WRITE2(device_id,addr, val30.wrd);
    }

    if (msk.s.latency_threshold_lo)
    {
        if (cfg->latency_threshold_lo > 0x1fffff)
            return CA_E_PARAM;

        addr = TE_PIE_dp4_0 + flowType * 4 + pie_voq_id * TE_PIE_dp4_0_STRIDE;

        val40.wrd = CA_DEV_NE_REG_READ(device_id,addr);

        val40.bf.latency_low = cfg->latency_threshold_lo;

        CA_DEV_NE_REG_WRITE2(device_id,addr, val40.wrd);
    }

    if (msk.s.latency_target)
    {
        if (cfg->latency_target > 0x1fffff)
            return CA_E_PARAM;

        addr = TE_PIE_dp5_0 + flowType * 4 + pie_voq_id * TE_PIE_dp5_0_STRIDE;

        val50.wrd = CA_DEV_NE_REG_READ(device_id,addr);

        val50.bf.latency_target = cfg->latency_target;

        CA_DEV_NE_REG_WRITE2(device_id,addr, val50.wrd);
    }

    if (msk.s.max_service_rate)
    {
        addr = TE_PIE_dp6_0 + pie_voq_id * TE_PIE_dp6_0_STRIDE;

        val60.wrd = CA_DEV_NE_REG_READ(device_id,addr);

        val60.bf.msr = cfg->max_service_rate;

        CA_DEV_NE_REG_WRITE2(device_id,addr, val60.wrd);
    }

    if (msk.s.peak_rate)
    {
        addr = TE_PIE_dp8_0 + pie_voq_id * TE_PIE_dp8_0_STRIDE;

        val80.wrd = CA_DEV_NE_REG_READ(device_id,addr);

        val80.bf.peak_rate = cfg->peak_rate;

        CA_DEV_NE_REG_WRITE2(device_id,addr, val80.wrd);
    }

    return CA_E_OK;

}


ca_status_t aal_l3_te_pie_dp_cfg_get
(
    ca_device_id_t         device_id,
    ca_uint8_t             flowType,
    ca_uint32_t            pie_voq_id,
    aal_l3_te_pie_dp_cfg_t *cfg
)
{
    TE_PIE_dp2_0_t val20;
    TE_PIE_dp3_0_t val30;
    TE_PIE_dp4_0_t val40;
    TE_PIE_dp5_0_t val50;
    TE_PIE_dp6_0_t val60;
    TE_PIE_dp8_0_t val80;

    if (!cfg)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    if (pie_voq_id > CA_AAL_L3_PIE_MAX_VOQ)
        return CA_E_PARAM;

    val20.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_dp2_0 + flowType * 4 + pie_voq_id * TE_PIE_dp2_0_STRIDE);
    val30.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_dp3_0 + flowType * 4 + pie_voq_id * TE_PIE_dp3_0_STRIDE);
    val40.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_dp4_0 + flowType * 4 + pie_voq_id * TE_PIE_dp4_0_STRIDE);
    val50.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_dp5_0 + flowType * 4 + pie_voq_id * TE_PIE_dp5_0_STRIDE);
    val60.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_dp6_0 + pie_voq_id * TE_PIE_dp6_0_STRIDE);
    val80.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_dp8_0 + pie_voq_id * TE_PIE_dp8_0_STRIDE);

    cfg->burst_reset_timeout  = val20.bf.burst_reset_timeout;
    cfg->latency_threshold_hi = val30.bf.latency_high;
    cfg->latency_threshold_lo = val40.bf.latency_low;
    cfg->latency_target       = val50.bf.latency_target;
    cfg->max_service_rate     = val60.bf.msr;
    cfg->peak_rate            = val80.bf.peak_rate;

    return CA_E_OK;

}


ca_status_t aal_l3_te_pie_timer_set
(
    ca_device_id_t            device_id,
    ca_uint8_t                flowType,
    aal_l3_te_pie_timer_msk_t msk,
    aal_l3_te_pie_timer_cfg_t *cfg
)
{
    TE_PIE_timer0_t val0;
    TE_PIE_timer1_t val1;
    TE_PIE_timer2_t val2;

    if (!cfg)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    if (msk.s.scalar)
    {
        if (cfg->scalar > 0x1ff)
            return CA_E_PARAM;

        val0.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_timer0);

        val0.bf.scalar = cfg->scalar;

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_timer0, val0.wrd);
    }

    if (msk.s.interval)
    {
        if (cfg->interval > 0x1fffff)
            return CA_E_PARAM;

        val1.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_timer1);

        val1.bf.interval = cfg->interval;

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_timer1, val1.wrd);
    }

    if (msk.s.burst_dec)
    {
        if (cfg->burst_dec > 0x1fffff)
            return CA_E_PARAM;

        val2.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_timer2 + flowType * 4);

        val2.bf.burst_decrement = cfg->burst_dec;

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_timer2 + flowType * 4, val2.wrd);
    }

    return CA_E_OK;
}

ca_status_t aal_l3_te_pie_timer_get
(
    ca_device_id_t            device_id,
    ca_uint8_t                flowType,
    aal_l3_te_pie_timer_cfg_t *cfg
)
{
    TE_PIE_timer0_t val0;
    TE_PIE_timer1_t val1;
    TE_PIE_timer2_t val2;

    if (!cfg)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    val0.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_timer0);
    val1.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_timer1);
    val2.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_timer2 + flowType * 4);

    cfg->scalar    = val0.bf.scalar;
    cfg->interval  = val1.bf.interval;
    cfg->burst_dec = val2.bf.burst_decrement;

    return CA_E_OK;
}

ca_status_t aal_l3_te_pie_dp_qfull_set
(
    ca_device_id_t device_id,
    ca_uint8_t     flowType,
    ca_uint32_t    threshold
)
{
    TE_PIE_pie_t value;

    if (threshold > 0x1fffff)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_pie + flowType * 4);

    value.bf.q_full = threshold;

    CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_pie + flowType * 4, value.wrd);

    return CA_E_OK;

}

ca_status_t aal_l3_te_pie_dp_qfull_get
(
    ca_device_id_t device_id,
    ca_uint8_t     flowType,
    ca_uint32_t    *threshold
)
{
    TE_PIE_pie_t value;

    if (!threshold)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_pie + flowType * 4);

    *threshold = value.bf.q_full;

    return CA_E_OK;
}

ca_status_t aal_l3_te_pie_dp_glb_cfg_set
(
    ca_device_id_t             device_id,
    ca_uint8_t                 flowType,
    aal_l3_te_pie_dp_glb_msk_t msk,
    aal_l3_te_pie_dp_glb_cfg_t *cfg
)
{
    TE_PIE_en0_t val0;
    TE_PIE_en1_t val1;
    TE_PIE_en2_t val2;
    TE_PIE_en3_t val3;
    TE_PIE_en4_t val4;
    TE_PIE_te_t  te;

    if (!cfg)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    if (msk.s.high)
    {
        val0.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en0 + flowType * 4);

        val0.bf.high = cfg->high;

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_en0 + flowType * 4, val0.wrd);
    }

    if (msk.s.low)
    {
        val1.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en1 + flowType * 4);

        val1.bf.low = cfg->low;

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_en1 + flowType * 4, val1.wrd);
    }

    if (msk.s.random_disable)
    {
        val2.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en2);

        val2.bf.random_disable = cfg->random_disable;

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_en2, val2.wrd);
    }

    if (msk.s.drop_prob_scale)
    {
        val2.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en2);

        val2.bf.drop_prob_scale = cfg->drop_prob_scale;

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_en2, val2.wrd);
    }

    if (msk.s.threshold)
    {
        val3.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en3);

        val3.bf.threshold = cfg->threshold;

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_en3, val3.wrd);
    }

    if (msk.s.burst)
    {
        val4.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en4 + flowType * 4);

        val4.bf.burst = cfg->burst;

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_en4 + flowType * 4, val4.wrd);
    }

    if (msk.s.tbc_mode)
    {
        te.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_te);

        te.bf.schif_tbc_mode = cfg->tbc_mode;

        CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_te, te.wrd);
    }

    return CA_E_OK;

}


ca_status_t aal_l3_te_pie_dp_glb_cfg_get
(
    ca_device_id_t             device_id,
    ca_uint8_t                 flowType,
    aal_l3_te_pie_dp_glb_cfg_t *cfg
)
{
    TE_PIE_en0_t val0;
    TE_PIE_en1_t val1;
    TE_PIE_en2_t val2;
    TE_PIE_en3_t val3;
    TE_PIE_en4_t val4;
    TE_PIE_te_t  te;

    if (!cfg)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    val0.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en0 + flowType * 4);
    val1.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en1 + flowType * 4);
    val2.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en2);
    val3.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en3);
    val4.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_en4 + flowType * 4);
    te.wrd   = CA_DEV_NE_REG_READ(device_id,TE_PIE_te);

    cfg->high  = val0.bf.high;
    cfg->low   = val1.bf.low;
    cfg->random_disable = val2.bf.random_disable;
    cfg->drop_prob_scale= val2.bf.drop_prob_scale;
    cfg->threshold      = val3.bf.threshold;
    cfg->burst          = val4.bf.burst;
    cfg->tbc_mode       = te.bf.schif_tbc_mode;
    return CA_E_OK;

}

/* size is in unit of 2^(x). for example, 128 bytes packet, its value should be 7 */
ca_status_t aal_l3_te_pie_mean_pkt_set
(
    ca_device_id_t device_id,
    ca_uint8_t     flowType,
    ca_uint32_t    size
)
{
    TE_PIE_mean_t value;

    if (size > 0xf)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_mean + flowType * 4);

    value.bf.size = size;

    CA_DEV_NE_REG_WRITE2(device_id,TE_PIE_mean + flowType * 4, value.wrd);

    return CA_E_OK;

}

ca_status_t aal_l3_te_pie_mean_pkt_get
(
    ca_device_id_t device_id,
    ca_uint8_t     flowType,
    ca_uint32_t    *size
)
{
    TE_PIE_mean_t value;

    if (!size)
        return CA_E_PARAM;

    if (flowType > 0x2)
        return CA_E_PARAM;

    value.wrd = CA_DEV_NE_REG_READ(device_id,TE_PIE_mean + flowType * 4);

    *size = value.bf.size;

    return CA_E_OK;

}

ca_status_t aal_l3_te_pie_reset
(
    ca_device_id_t device_id
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
            ret=aal_l3_te_pie_lpid_qos_voq_set(device_id, i, j, msk, &cfg);
            if(ret!=CA_E_OK)PRINTF("Error!ldpid=%d cos=%d ret=%d.\n",i,j,ret);
        }
    }

    for(index=0;index<CA_AAL_L3_PIE_MAX_VOQ;index++){
        ret=aal_l3_te_glb_match_set(device_id, index, glb_msk, &glb_cfg);
        if(ret!=CA_E_OK)PRINTF("Error! index=%d, ret=%d.\n",index,ret);
    }

    for(index=0;index<TE_TE_GLB_TE_PIE_WRED_PORT_ENABLE0_COUNT;index++){
        ret=aal_l3_te_glb_pie_wred_port_enable_set(device_id, index, value);
            if(ret!=CA_E_OK)PRINTF("Error! index=%d, ret=%d.\n",index,ret);
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

        CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_te, te.wrd);
        CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_timer0, timer0.wrd);
        CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_timer1, timer1.wrd);
        CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_en2, en2.wrd);
        CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_en3, en3.wrd);

        for(i=0;i<3;i++){
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_buf+i*4, buf.wrd);
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dp0+i*4, dp0.wrd);
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dp1+i*4, dp1.wrd);
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_en0+i*4, en0.wrd);
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_en1+i*4, en1.wrd);
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_en4+i*4, en4.wrd);
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_mean+i*4, mean.wrd);
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_pie+i*4, pie.wrd);
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_timer2+i*4, timer2.wrd);
        }

        for(i=0;i<=CA_AAL_L3_PIE_MAX_VOQ;i++){
            for(j=0;j<3;j++){
                CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dp2_0+j*4+i*TE_PIE_dp2_0_STRIDE, dp2.wrd);
                CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dp3_0+j*4+i*TE_PIE_dp3_0_STRIDE, dp3.wrd);
                CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dp4_0+j*4+i*TE_PIE_dp4_0_STRIDE, dp4.wrd);
                CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dp5_0+j*4+i*TE_PIE_dp5_0_STRIDE, dp5.wrd);
            }
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dp6_0+i*TE_PIE_dp6_0_STRIDE, dp6.wrd);
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dp8_0+i*TE_PIE_dp8_0_STRIDE, dp8.wrd);
        }

        for(j=0;j<3;j++){
            dsel.bf.sel=j;
            CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dbg_sel, dsel.wrd);
            for(i=0;i<19;i++){
                CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dbg0+i*4, dbg0.wrd);
            }
        }
        CA_DEV_NE_REG_WRITE2(device_id, TE_PIE_dbg_sel, TE_PIE_dbg_sel_dft);
    }while(0);

    return CA_E_OK;
}

/*
** L3 TE module initialization function
*/
ca_status_t aal_l3_te_init
(
    ca_device_id_t device_id
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
    #if 0
    aal_scfg_read(CFG_ID_L3TE_WRED_MARK_IDX, 4, &l3_te_wred_mark_idx);
    aal_scfg_read(CFG_ID_L3TE_WRED_UNMARK_IDX, 4, &l3_te_wred_unmark_idx);
    #endif
    l3_te_wred_mark_idx = CFG_ID_L3TE_WRED_MARK_IDX;
    l3_te_wred_unmark_idx = CFG_ID_L3TE_WRED_UNMARK_IDX;
    (void)aal_l3_te_shaper_ni_port_group_set(device_id, FALSE);
    (void)aal_l3_te_shaper_cpu_port_group_set(device_id, FALSE);

    /* statistics counter settings */
    (void)aal_l3_te_pm_enable_set(device_id, TRUE);
    (void)aal_l3_te_pm_read_clr_set(device_id, TRUE);
    aal_l3_te_pm_cnt_mem_set(device_id, 0, 0, 0);
    aal_l3_te_pm_cnt_mem_set(device_id, 1, 1, 1);

    /* make sure clear SRAM data */
    {
        int i =0;
        aal_l3_te_pm_egress_t l3pm_stats;
        for (i = 0; i <= AAL_L3TE_MAX_VOQ_ID; i++) {
            aal_l3_te_pm_egress_voq_get(device_id, i, &l3pm_stats);
        }
    }

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

    (void)aal_l3_te_glb_cfg_set(device_id, te_msk, &te_cfg);

    /* policer settings */
    memset((void *)&l3_policer_cfg, 0, sizeof(l3_policer_cfg));
    memset((void *)&l3_policer_msk, 0, sizeof(l3_policer_msk));

    l3_policer_msk.s.ipg = 1;
    l3_policer_cfg.ipg   = 24;  /* IPG 20 to 24, bug 50074 */
    for (i=0; i<=CA_AAL_L3_MAX_FLOW_ID; i++)
    {
        if (aal_l3_te_policer_flow_cfg_set(device_id, i, l3_policer_msk, &l3_policer_cfg) != CA_E_OK)
            CA_TM_LOG("aal_l3_te_policer_flow_cfg_set() : failed to set policer configurations for l3 flow %d, L%d\n", i, __LINE__);
    }

    for (i=0; i<=CA_AAL_L3_MAX_PORT_ID; i++)
    {
        if (aal_l3_te_policer_port_cfg_set(device_id, i, l3_policer_msk, &l3_policer_cfg) != CA_E_OK)
            CA_TM_LOG("aal_l3_te_policer_port_cfg_set() : failed to set policer configurations for l3 port %d, L%d\n", i, __LINE__);
    }

    for (i=0; i<CA_AAL_L3_POLICER_PKT_TYPE_END; i++)
    {
        if (aal_l3_te_policer_pkt_type_cfg_set(device_id, i, l3_policer_msk, &l3_policer_cfg) != CA_E_OK)
            CA_TM_LOG("aal_l3_te_policer_pkt_type_cfg_set() : failed to set policer configurations for l3 packet type %d, L%d\n", i, __LINE__);
    }

    /* shaper settings */
    #if defined(CONFIG_CORTINA_BOARD_FPGA) || defined(CONFIG_REALTEK_BOARD_FPGA) || defined(CONFIG_REALTEK_BOARD_FPGA_V8)
    /*
    ** For ASIC: ethernet clock: 125MHz, core clock: 200MHz;
    ** For FPGA: ethernet clock: 25MHz,  core clock: 25MHz
    ** So, shaper and policer token bucket update time needs to be adjusted for G3 FPGA.
    */
    do
    {
        /* shaper update */
        TE_SHP_TB_CTRL_t ctrl;

        ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_SHP_TB_CTRL);

        ctrl.bf.upd_cnt_lmt = 3;

        CA_DEV_NE_REG_WRITE2(device_id,TE_SHP_TB_CTRL, ctrl.wrd);

    }while(0);


    do
    {   /* policer update */
        TE_POL_TB_CTRL_t ctrl;

        ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_POL_TB_CTRL);

        ctrl.bf.upd_cnt_lmt = 3;

        CA_DEV_NE_REG_WRITE2(device_id,TE_POL_TB_CTRL, ctrl.wrd);
    } while(0);

    #endif

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
                case 8 ... 11:
                    shp.bs = 3;
                    break;
                default:
                    shp.bs = 6;
                    break;
            }
            (void)aal_l3_te_shaper_port_tbc_set(0, i, shp_msk, &shp);
        }
        shp_msk.u32 = 0;
        for(i = 8; i<AAL_L3TE_PORT_NUM; i++) {
            shp_msk.s.rate_m = 1;
            switch(i)
            {
                case 8 ... 11:
                    shp.rate_m = 1536;
                    break;
                default:
                    shp.rate_m = 3072;
                    break;
            }
            (void)aal_l3_te_shaper_port_tbc_set(0, i, shp_msk, &shp);
        }
    } while(0);
    
    /* reset WRED software table */
    memset((void *)&g_aal_l3_te_wred_profile_info[0], 0x0, sizeof(g_aal_l3_te_wred_profile_info));
    memset((void *)&g_aal_l3_te_wred_sel_info[0][0], 0xff, sizeof(g_aal_l3_te_wred_sel_info));


    PRINTF( "L3_TE : initialize WRED profile & selection table ... ");
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

            PRINTF( "done.\n");
        }
        else
        {
            PRINTF("failed.\n");
        }

        //reset global free memory size register
        for (i=0; i<=CA_AAL_L3_WRED_MAX_GLOBAL_POOL_NUMS; i++)
            aal_l3_te_wred_global_free_mem_size_set(device_id, i, 0, 0);

        //enable CB scan
        do{
            aal_l3_te_cb_ctrl_mask_t mask;
            aal_l3_te_cb_ctrl_t cfg;

            memset((void *)&mask, 0, sizeof(mask));
            memset((void *)&cfg, 0, sizeof(cfg));

            mask.s.scan_ena=1;
            cfg.scan_ena=1;
            aal_l3_te_cb_ctrl_set(device_id, mask, &cfg);
        }while(0);
    }while(0);

    PRINTF( "L3_TE : initialize PIE ... ");
    aal_l3_te_pie_reset(device_id);
    PRINTF( "done.\n");

    do{
        TE_FC_CTRL_t te_fc_ctrl;

        te_fc_ctrl.wrd = CA_DEV_NE_REG_READ(device_id,TE_FC_CTRL);
    //    te_fc_ctrl.bf.tx_en = 1;
        te_fc_ctrl.bf.rx_en = 1;
        CA_DEV_NE_REG_WRITE2(device_id,TE_FC_CTRL, te_fc_ctrl.wrd);
        CA_DEV_NE_REG_WRITE2(device_id,TE_FC_CTRL + APB0_TE_FC_STRIDE, te_fc_ctrl.wrd);
    }while(0);

    return CA_E_OK;
}
