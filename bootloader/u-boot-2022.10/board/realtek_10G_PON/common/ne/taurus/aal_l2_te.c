#include <osal_soc.h>
#include <osal.h>
#include <aal_common.h>
#include <aal_l2_te.h>
#include <aal_l2_tm.h>
#include <aal_l2_qm.h>


aal_l2_te_wred_info_t g_aal_l2_te_wred_profile_info[CA_AAL_WRED_MAX_PROFILE_NUMS];
ca_uint8_t g_aal_l2_te_wred_sel_info[CA_AAL_MAX_PORT_ID + 1][8];

static ca_uint32_t g_mark_dp[8][CA_AAL_WRED_MAX_DROP_PROBABILTY] =
{
    {63, 63, 63, 63, 63, 57, 52, 46, 40, 34, 29, 23, 17, 11, 6, 0},
    {63, 63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5, 0, 0},
    {63, 63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5, 0, 0},
    {63, 63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5, 0, 0},
    {63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5,  0, 0, 0},
    {63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5,  0, 0, 0},
    {63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5,  0,  0, 0, 0},
    {63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5,  0,  0, 0, 0}
};

static ca_uint32_t g_unmark_dp[8][CA_AAL_WRED_MAX_DROP_PROBABILTY] =
{
    {63, 63, 63, 63, 63, 63, 57, 52, 46, 40, 34, 29, 23, 17, 11, 6},
    {63, 63, 63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5, 0},
    {63, 63, 63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5, 0},
    {63, 63, 63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5, 0},
    {63, 63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5,  0, 0},
    {63, 63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11,  5,  0, 0},
    {63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11, 5,   0,  0, 0},
    {63, 63, 58, 53, 47, 42, 37, 32, 26, 21, 16, 11, 5,   0,  0, 0}

};

ca_status_t aal_l2_te_glb_cfg_set
(
    aal_l2_te_glb_cfg_msk_t msk,
    aal_l2_te_glb_cfg_t     *cfg
)
{
    L2TM_L2TE_GLB_CTRL_t ctrl;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;


    ctrl.wrd = RTK_NE_REG_READ(L2TM_L2TE_GLB_CTRL);

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

    RTK_NE_REG_WRITE(L2TM_L2TE_GLB_CTRL, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l2_te_policer_port_cfg_set
(
    ca_port_id_t                 port_id,
    aal_l2_te_policer_cfg_msk_t msk,
    aal_l2_te_policer_cfg_t     *cfg
)
{
    L2TM_L2TE_POL_PORT_CTRL0_t ctrl;

    if (port_id > CA_AAL_MAX_PORT_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    ctrl.wrd = RTK_NE_REG_READ(L2TM_L2TE_POL_PORT_CTRL0 + port_id * 4);

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

    RTK_NE_REG_WRITE(L2TM_L2TE_POL_PORT_CTRL0 + port_id * 4, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l2_te_policer_pkt_type_cfg_set
(
    aal_l2_te_policer_pkt_type_t pkt_type,
    aal_l2_te_policer_cfg_msk_t  msk,
    aal_l2_te_policer_cfg_t      *cfg)
{
    L2TM_L2TE_POL_PKT_TYPE_CTRL0_t ctrl;

    if (pkt_type >= CA_AAL_POLICER_PKT_TYPE_END)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    ctrl.wrd = RTK_NE_REG_READ(L2TM_L2TE_POL_PKT_TYPE_CTRL0 + (ca_uint32_t)pkt_type * 4);

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

    RTK_NE_REG_WRITE(L2TM_L2TE_POL_PKT_TYPE_CTRL0 + (ca_uint32_t)pkt_type * 4, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l2_te_policer_flow_cfg_set
(
    ca_flow_id_t                flow_id,
    aal_l2_te_policer_cfg_msk_t msk,
    aal_l2_te_policer_cfg_t     *cfg
)
{
#if 0 /*TBD for RTL8277C AAL*/
    L2TM_L2TE_POL_FLOW_GRP_CTRL0_t ctrl;

    if (flow_id > CA_AAL_MAX_FLOW_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

   // addr = L2TM_L2TE_POL_FLOW_GRP_CTRL0 + (flow_id / 32) * 4;    /* 32 flows share one configuration */

    ctrl.wrd = CA_DEV_NE_REG_READ(device_id, L2TM_L2TE_POL_FLOW_GRP_CTRL0 + (flow_id / 32) * 4);

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

    CA_DEV_NE_REG_WRITE2(device_id, L2TM_L2TE_POL_FLOW_GRP_CTRL0 + (flow_id / 32) * 4, ctrl.wrd);
#endif
    return CA_E_OK;
}

ca_status_t aal_l2_te_policer_agr_flow_cfg_set
(
    ca_uint32_t                  agr_flow_id,
    aal_l2_te_policer_cfg_msk_t msk,
    aal_l2_te_policer_cfg_t     *cfg
)
{
    L2TM_L2TE_POL_AGRFLOW_CTRL0_t ctrl;

    if (agr_flow_id > CA_AAL_MAX_AGR_FLOW_ID)
        return CA_E_PARAM;

    if (cfg == NULL)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    //addr = L2TM_L2TE_POL_AGRFLOW_CTRL0 + agr_flow_id * 4;

    ctrl.wrd = RTK_NE_REG_READ(L2TM_L2TE_POL_AGRFLOW_CTRL0 + agr_flow_id * 4);

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

    RTK_NE_REG_WRITE(L2TM_L2TE_POL_AGRFLOW_CTRL0 + agr_flow_id * 4, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l2_te_pm_enable_set
(
    ca_boolean_t enable
)
{
    L2TM_L2TE_PM_CTRL_t value;

    value.wrd = CA_NE_REG_READ(L2TM_L2TE_PM_CTRL);

    value.bf.mem_disable = enable ? 0 : 1;

    RTK_NE_REG_WRITE(L2TM_L2TE_PM_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l2_te_pm_read_clr_set
(
    ca_boolean_t read_clr
)
{
    L2TM_L2TE_PM_CTRL_t value;

    value.wrd = RTK_NE_REG_READ(L2TM_L2TE_PM_CTRL);

    value.bf.mem_auto_clr_on_read = read_clr;

    RTK_NE_REG_WRITE(L2TM_L2TE_PM_CTRL, value.wrd);

    return CA_E_OK;
}

static inline ca_status_t __l2_te_wred_free_global_mem_size_access
(
    aal_tbl_op_flag_t    op_flag,
    ca_uint32_t          addr,
    volatile ca_uint32_t *data0
)
{
    L2TM_L2TE_WRED_AFGS_MEM_ACCESS_t access;
    ca_uint32_t i = 0;

    access.wrd = RTK_NE_REG_READ(L2TM_L2TE_WRED_AFGS_MEM_ACCESS);

    while (access.bf.access && (i < AAL_TBL_OP_TIMEOUT))
    {
        access.wrd = RTK_NE_REG_READ(L2TM_L2TE_WRED_AFGS_MEM_ACCESS);
        if (i++ >= AAL_TBL_OP_TIMEOUT)
        {
            return CA_E_TIMEOUT;
        }
    }

    if (op_flag == AAL_TBL_OP_FLAG_WR)
    {
        /* the operation to write table */

        RTK_NE_REG_WRITE(L2TM_L2TE_WRED_AFGS_MEM_DATA, *data0);
    }

    memset((void *)&access, 0, sizeof(access));

    access.bf.address  = addr;
    access.bf.rbw      = op_flag;
    access.bf.access   = 1;

    RTK_NE_REG_WRITE(L2TM_L2TE_WRED_AFGS_MEM_ACCESS, access.wrd);

    /* waiting untill read-operation is completed */
    if (op_flag == AAL_TBL_OP_FLAG_RD)
    {
        i = 0;

        access.wrd = RTK_NE_REG_READ(L2TM_L2TE_WRED_AFGS_MEM_ACCESS);
        while (access.bf.access)
        {
            access.wrd = RTK_NE_REG_READ(L2TM_L2TE_WRED_AFGS_MEM_ACCESS);
            if (i++ >= AAL_TBL_OP_TIMEOUT)
            {
                return CA_E_TIMEOUT;
            }
        }

        *data0 = RTK_NE_REG_READ(L2TM_L2TE_WRED_AFGS_MEM_DATA);
    }

    return CA_E_OK;

}

ca_status_t aal_l2_te_wred_global_free_mem_size_set
(
    ca_uint32_t  pool_id,
    ca_uint32_t  size
)
{
    L2TM_L2TE_WRED_AFGS_MEM_DATA_t value0;
    ca_uint32_t index = pool_id;

    if (pool_id > 0)
        return CA_E_PARAM;

    if (size > CA_AAL_L2_BM_MAX_GLOBAL_BUFF_SIZE)
        return CA_E_PARAM;

    memset((void *)&(value0), 0, sizeof(value0));

    value0.wrd = size;

    /* update table */
    return __l2_te_wred_free_global_mem_size_access(
            AAL_TBL_OP_FLAG_WR,
            index,
            (volatile ca_uint32_t *)&(value0.wrd));
}

ca_boolean aal_l2_te_wred_profile_info_allocate
(
    ca_uint32_t *index
)
{
    ca_uint32_t i = 0;

    if (!index)
        return FALSE;

    for (i=0; i<CA_AAL_WRED_MAX_PROFILE_NUMS; i++)
    {
        if (g_aal_l2_te_wred_profile_info[i].ref_counter == 0)
        {
            *index = i;

            return TRUE;
        }
    }

    return FALSE;
}

void aal_l2_te_wred_profile_info_ref_inc(ca_uint32_t index)
{
    g_aal_l2_te_wred_profile_info[index].ref_counter++;
}

void aal_l2_te_wred_sel_info_set
(
    ca_port_id_t port,
    ca_uint32_t  queue,
    ca_uint32    index
)
{
    if (port <= CA_AAL_MAX_PORT_ID && queue <= 7)
        g_aal_l2_te_wred_sel_info[port][queue] = index;
}

/*
** L2 TE module initialization function
*/
ca_status_t aal_l2_te_init
(
)
{
    aal_l2_te_wred_cfg_t     wred_cfg;
    aal_l2_te_wred_cfg_msk_t wred_msk;
    aal_l2_te_glb_cfg_t      te_cfg;
    aal_l2_te_glb_cfg_msk_t  te_cfg_msk;
    aal_l2_te_policer_cfg_msk_t policer_msk;
    aal_l2_te_policer_cfg_t     policer_cfg;
    ca_uint32_t i = 0;
    ca_uint32_t j = 0;

    /* enable wred, policer, shaper, taildrop etc.*/
    memset((void *)&te_cfg, 0, sizeof(te_cfg));
    te_cfg_msk.u32 = 0;

    te_cfg_msk.s.policer_ena   = 1;
    te_cfg_msk.s.shaper_ena    = 1;
    te_cfg_msk.s.tail_drop_ena = 1;
    te_cfg_msk.s.wred_ena      = 1;
    te_cfg_msk.s.mark_ecn_ena  = 1;
    te_cfg_msk.s.pol_ecn_ena   = 1;
    te_cfg_msk.s.wred_ecn_ena  = 1;

    te_cfg.policer_ena   = CA_TRUE;
    te_cfg.shaper_ena    = CA_TRUE;
    te_cfg.tail_drop_ena = CA_TRUE;
    te_cfg.wred_ena      = CA_TRUE;
    te_cfg.mark_ecn_ena  = CA_FALSE;
    te_cfg.pol_ecn_ena   = CA_FALSE;
    te_cfg.wred_ecn_ena  = CA_FALSE;

    (void)aal_l2_te_glb_cfg_set(te_cfg_msk, &te_cfg);
    

    /* policer settings */
    memset((void *)&policer_cfg, 0, sizeof(policer_cfg));
    policer_msk.u32 = 0;

    policer_msk.s.type            = 1;
    policer_msk.s.pol_update_mode = 1;
    policer_cfg.type              = CA_AAL_POLICER_TYPE_NONE;     /* two rates three colors */
    policer_cfg.pol_update_mode   = CA_AAL_POLICER_UPDATE_MODE_BYTE; /* bytes mode            */

    /* port based policer settings */
    for (i=0; i<=CA_AAL_MAX_PORT_ID; i++)
        (void)aal_l2_te_policer_port_cfg_set(i, policer_msk, &policer_cfg);

    /* packet type based policer settings */
    for (i=CA_AAL_POLICER_PKT_TYPE_UC; i<CA_AAL_POLICER_PKT_TYPE_END; i++)
        (void)aal_l2_te_policer_pkt_type_cfg_set(i, policer_msk, &policer_cfg);

    /* flow based policer settings, each 32 flows share one configuration */
    for (i=0; i<=(CA_AAL_MAX_FLOW_ID / 32); i++)
        (void)aal_l2_te_policer_flow_cfg_set(i * 32, policer_msk, &policer_cfg);

    for (i=0; i<=8; i++)
        (void)aal_l2_te_policer_agr_flow_cfg_set(i, policer_msk, &policer_cfg);

    /* statistics counter settings */
    (void)aal_l2_te_pm_enable_set(TRUE);    /* enable performance monitor  */
    (void)aal_l2_te_pm_read_clr_set(TRUE);  /* clear counter after reading */

    /*
    ** mem block 0
    ** 000 ~ 255 : flow
    ** 256 ~ 263 : agr flow
    ** 384 ~ 399 : packet type
    ** 400 ~ 415 : aggregation flow
    */

    /* WRED initialization */
    memset((void *)&wred_cfg, 0, sizeof(wred_cfg));
    wred_msk.u32 = 0;

    /* reset WRED software table */
    memset((void *)&g_aal_l2_te_wred_profile_info[0], 0x0, sizeof(g_aal_l2_te_wred_profile_info));
    memset((void *)&g_aal_l2_te_wred_sel_info[0][0], 0xff, sizeof(g_aal_l2_te_wred_sel_info));

    /* WRED initialization */
    (void)aal_l2_te_wred_global_free_mem_size_set(0, 0x1000); /* CA_AAL_L2_BM_MAX_GLOBAL_BUFF_SIZE, 1K buffer here */

    #if 0
    aal_l2_te_wred_profile_msk_t wred_profile_msk;
    aal_l2_te_wred_profile_t wred_profile;

    for (i=0; i<8; i++)
    {
        wred_profile_msk.u32 = 0;
        memset((void *)&wred_profile, 0, sizeof(wred_profile));

        wred_profile_msk.s.mark_dp    = 1;
        wred_profile_msk.s.unmark_dp  = 1;
        wred_profile_msk.s.mark_idx   = 1;
        wred_profile_msk.s.unmark_idx = 1;

        memcpy((void *)&wred_profile.mark_dp, &g_mark_dp[i][0], sizeof(wred_profile.mark_dp));
        memset((void *)&wred_profile.unmark_dp, 0x00, sizeof(wred_profile.unmark_dp));
        wred_profile.mark_idx   = 8;    /* 16 DP * 2^8 = 4096 buffers, drop range is 0 ~ 4095  */

        (void)aal_l2_te_wred_profile_set(device_id, i, wred_profile_msk, &wred_profile);
    }

    for (i=8; i<16; i++)
    {
        wred_profile_msk.u32 = 0;
        memset((void *)&wred_profile, 0, sizeof(wred_profile));

        wred_profile_msk.s.mark_dp    = 1;
        wred_profile_msk.s.unmark_dp  = 1;
        wred_profile_msk.s.mark_idx   = 1;
        wred_profile_msk.s.unmark_idx = 1;

        memcpy((void *)&wred_profile.unmark_dp, &g_unmark_dp[i][0], sizeof(wred_profile.unmark_dp));
        memset((void *)&wred_profile.mark_dp, 0x00, sizeof(wred_profile.mark_dp));

        wred_profile.unmark_idx   = 8;

        (void)aal_l2_te_wred_profile_set(device_id, i, wred_profile_msk, &wred_profile);
    }
    #endif

    PRINTF("L2_TE : initialize WRED profile & selection table ... ");
    do
    {
        /* setup WRED profile and selection table */
        ca_uint32_t index = 0;

        if (aal_l2_te_wred_profile_info_allocate(&index))
        {
            for (i=0; i<=CA_AAL_MAX_PORT_ID; i++)
            {
                for (j=0; j<8; j++)
                {
                    aal_l2_te_wred_profile_info_ref_inc(index); /* reference counter ++ */

                    /* update software selection table */
                    aal_l2_te_wred_sel_info_set(i, j, index);
                }
            }

            PRINTF("done.\n");
        }
        else
        {
            PRINTF("failed.\n");
        }
    }while(0);
    do{
        L2TM_L2TE_FC_GLB_CTRL_t fc_glb_ctrl;        
        fc_glb_ctrl.wrd = RTK_NE_REG_READ(L2TM_L2TE_FC_GLB_CTRL);
        fc_glb_ctrl.bf.l3felan_rxpfc_en = 0xFF;
        RTK_NE_REG_WRITE(L2TM_L2TE_FC_GLB_CTRL, fc_glb_ctrl.wrd);
    }while(0);

    return CA_E_OK;
}