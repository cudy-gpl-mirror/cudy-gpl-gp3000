#include <osal_soc.h>
#include <osal.h>
#include <aal_l2_qm.h>
#include <aal_common.h>

ca_status_t aal_l2_qm_eq_cfg_set
(
    ca_uint8_t          eq_id,
    aal_l2_qm_eq_cfg_msk_t msk,
    aal_l2_qm_eq_cfg_t     *cfg
)
{
    L2TM_L2TM_QM_EQ_CFG_t ctrl;

    if (!cfg)
        return CA_E_PARAM;

    if (eq_id > 1)
        return CA_E_PARAM;

    if (!msk.u32)
        return CA_E_OK;

    ctrl.wrd = RTK_NE_REG_READ(L2TM_L2TM_QM_EQ_CFG);

    if (msk.s.eq_ena)
    {
        if (eq_id)
            ctrl.bf.eq1_en = cfg->eq_ena;
        else
            ctrl.bf.eq0_en = cfg->eq_ena;
    }

    if (msk.s.buff_num)
    {
        if (eq_id)
        {
            if (cfg->buff_num > 0xf)
                return CA_E_PARAM;

            ctrl.bf.eq1_bufnum = cfg->buff_num;
        }
        else
        {
            if (cfg->buff_num > 0x1f)
                return CA_E_PARAM;

            ctrl.bf.eq0_bufnum = cfg->buff_num;
        }
    }

    if (msk.s.prvt_buff_num)
    {
        if (eq_id)
        {
            if (cfg->prvt_buff_num > 0x3ff)
                return CA_E_PARAM;

            ctrl.bf.eq1_prvt = cfg->prvt_buff_num;
        }
        else
        {
            if (cfg->prvt_buff_num > 0x7ff)
                return CA_E_PARAM;

            ctrl.bf.eq0_prvt = cfg->prvt_buff_num;
        }
    }



    RTK_NE_REG_WRITE(L2TM_L2TM_QM_EQ_CFG, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l2_qm_port_prvt_profile_set
(
    ca_uint32_t profile_id,
    ca_uint32_t buff_nums
)
{
    L2TM_L2TM_QM_PORT_PRVT_PROFILE0_t ctrl;

    if (profile_id > CA_AAL_L2_QM_MAX_PORT_PRVT_PROFILE_ID) // up to 8 profiles in total
        return CA_E_PARAM;

    if (buff_nums > 0x7fff)
        return CA_E_PARAM;

    ctrl.wrd = RTK_NE_REG_READ(L2TM_L2TM_QM_PORT_PRVT_PROFILE0 + (profile_id / 2 * 4));

    ctrl.wrd &= ~(0x7fff << (16 * (profile_id % 2)));
    ctrl.wrd |= (buff_nums << (16 * (profile_id % 2)));

    RTK_NE_REG_WRITE(L2TM_L2TM_QM_PORT_PRVT_PROFILE0 + (profile_id / 2 * 4), ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l2_qm_buff_cfg_set
(
    aal_l2_qm_buff_cfg_msk_t msk,
    aal_l2_qm_buff_cfg_t     *cfg
)
{
    L2TM_L2TM_QM_GLOB_BUF_CFG_t value;

    if (!cfg)
        return CA_E_PARAM;

    value.wrd = RTK_NE_REG_READ(L2TM_L2TM_QM_GLOB_BUF_CFG);

    if (msk.s.no_drop_threshold)
    {
        if (cfg->no_drop_threshold > 0x7fff)
            return CA_E_PARAM;

        value.bf.nodrop  = cfg->no_drop_threshold;
    }

    if (msk.s.drop_ena)
    {
        value.bf.drop_en  = cfg->drop_ena ? 1 : 0;
    }

    if (msk.s.non_cong_threshold)
    {
        if (cfg->no_drop_threshold > 0x7fff)
            return CA_E_PARAM;

        value.bf.noncong  = cfg->non_cong_threshold;
    }

    if (msk.s.fe_bp_ena)
    {
        value.bf.fe_bp_en  = cfg->fe_bp_ena ? 1 : 0;
    }

    RTK_NE_REG_WRITE(L2TM_L2TM_QM_GLOB_BUF_CFG, value.wrd);


    return CA_E_OK;
}

ca_status_t aal_l2_qm_init()
{
    aal_l2_qm_eq_cfg_msk_t   qm_msk;
    aal_l2_qm_eq_cfg_t       qm_cfg;
    aal_l2_qm_buff_cfg_t     buff_cfg;
    aal_l2_qm_buff_cfg_msk_t buff_msk;
    ca_uint32_t non_congestion_threshold = CA_AAL_L2_QM_NON_CONGESTION_THRESHOLD;

    /* empty queue initilization */
    memset((void *)&qm_cfg, 0, sizeof(qm_cfg));
    qm_msk.u32 = 0;

    qm_msk.s.eq_ena      = 1;
    qm_msk.s.buff_num    = 1;
    qm_msk.s.prvt_buff_num = 1;


    qm_cfg.eq_ena        = FALSE;
    qm_cfg.buff_num      = 0;     /* eq0 : 0 means (32K-1) * 32bytes packet buffer, 1M */
    qm_cfg.prvt_buff_num = 0;


    (void)aal_l2_qm_eq_cfg_set(0, qm_msk, &qm_cfg); /* configure eq0 */

    qm_cfg.eq_ena        = TRUE;
    qm_cfg.buff_num      = 4;     /* eq1 : 0 means (16K-1) * 64 bytes packet buffer, 1M, better performance */
                                  /* for 5Gbps traffic, using 64 bytes buffer, 4K * 64 =256K */
    qm_cfg.prvt_buff_num = CA_AAL_L2_QM_PORT_PRVT_BUFF_NUM;    /* in unit of 16 buffers */

    (void)aal_l2_qm_eq_cfg_set(1, qm_msk, &qm_cfg); /* configure eq1 */

    PRINTF("L2_QM : EQ-0(0), EQ-1(4K) - system uses 4K * 64bytes buffer configuration.\n");

    /* port private buffer configuration, all port selects profile 0 by default */
    (void)aal_l2_qm_port_prvt_profile_set(0, CA_AAL_L2_QM_PORT_PRVT_BUFF_NUM);
    PRINTF("L2_QM : port private buffer number is %d\n", CA_AAL_L2_QM_PORT_PRVT_BUFF_NUM);


    /* read non-congestion buffer threshold from startup config file */
    #if 0
    if (aal_scfg_read(CFG_ID_L2TM_NON_CONGESTION_BUFF_THRESHOLD, 4, &non_congestion_threshold) != CA_E_OK)
    {
        PRINTF("L2_QM : failed to read CFG_ID_L2TM_NON_CONGESTION_BUFF_THRESHOLD, using 0x4000 as default value.\n");
        non_congestion_threshold = CA_AAL_L2_QM_NON_CONGESTION_THRESHOLD;  /* using default value */
    }
    #endif
    //INT             CFG_ID_L2TM_NON_CONGESTION_BUFF_THRESHOLD               = 0x800;
    non_congestion_threshold = 0x800;

    /* gobal buffer configuration */
    memset((void *)&buff_cfg, 0x0, sizeof(buff_cfg));
    buff_msk.u32 = 0;

    buff_msk.s.fe_bp_ena          = 1;
    buff_msk.s.drop_ena           = 1;
    buff_msk.s.no_drop_threshold  = 1;
    buff_msk.s.non_cong_threshold = 1;

    buff_cfg.fe_bp_ena          = CA_FALSE;  /* back-pressure to FE */
    buff_cfg.drop_ena           = CA_TRUE;
    buff_cfg.no_drop_threshold  = 0x21;
    buff_cfg.non_cong_threshold = non_congestion_threshold;    /* BM is non-congestion state if aviable buffer is more than 2K */

    (void)aal_l2_qm_buff_cfg_set(buff_msk, &buff_cfg);

    return CA_E_OK;
}