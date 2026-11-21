#include <osal_soc.h>
#include <osal.h>
#include <aal_l2_tm.h>
#include <aal_l2_qm.h>
#include <aal_common.h>

ca_status_t aal_l2_tm_read_clr_set
(
    ca_boolean_t read_clr
)
{
    L2TM_L2TM_GLB_CTRL_t value;

    value.wrd = RTK_NE_REG_READ(L2TM_L2TM_GLB_CTRL);

    value.bf.cntrcfg_clear_on_read = read_clr;

    RTK_NE_REG_WRITE(L2TM_L2TM_GLB_CTRL, value.wrd);

    return CA_E_OK;
}

ca_status_t aal_l2_tm_es_ena_set
(
    ca_boolean_t ena
)
{
    L2TM_L2TM_ES_CTRL_t ctrl;

    ctrl.wrd = RTK_NE_REG_READ(L2TM_L2TM_ES_CTRL);

    ctrl.bf.tx_en = ena;

    RTK_NE_REG_WRITE(L2TM_L2TM_ES_CTRL, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l2_tm_es_port_ena_set
(
    ca_port_id_t port_id,
    ca_boolean_t ena
)
{
    L2TM_L2TM_ES_CTRL_t ctrl;

    ctrl.wrd = RTK_NE_REG_READ(L2TM_L2TM_ES_CTRL);

    if (ena)
        ctrl.wrd |= (1 << port_id);
    else
        ctrl.wrd &= ~(1 << port_id);

    RTK_NE_REG_WRITE(L2TM_L2TM_ES_CTRL, ctrl.wrd);

    return CA_E_OK;
}

ca_status_t aal_l2_tm_es_voq_ena_set
(
    ca_port_id_t port_id,
    ca_uint32_t  voq_id,
    ca_boolean_t ena
)
{
    L2TM_L2TM_ES_SCH_CFG0_t ctrl;

    if (voq_id > 7)
        return CA_E_PARAM;

    /*
    Instances:  15 instances
    Names: %d=[0,1,2,3,4,5,6,7,8,9,10,11,12,13,15]
    Addresses: ( base=[0x2204] ) + ( 12 * ( %d=[0..14] ) )
    */
    if (port_id == CA_AAL_ES_MAX_PORT_ID)
        port_id -= 1;

    ctrl.wrd = RTK_NE_REG_READ(L2TM_L2TM_ES_SCH_CFG0 + 12 * port_id);

    if (ena)
        ctrl.wrd |= (1 << voq_id);
    else
        ctrl.wrd &= ~(1 << voq_id);

    RTK_NE_REG_WRITE(L2TM_L2TM_ES_SCH_CFG0 + 12 * port_id, ctrl.wrd);

    return CA_E_OK;
}

/*
** L2 TM module initialization function
*/
ca_status_t aal_l2_tm_init()
{
    ca_int8 sz_info[256], sz_tmp[16];
    ca_uint32_t i = 0, voq_id = 0;
	L2TM_L2TM_ES_SCH_CFG0_t ctrl;
	ca_uint32_t addr;

    {
        (void)aal_l2_qm_init();

        /* TM counter will be cleared on reading */
        (void)aal_l2_tm_read_clr_set(CA_TRUE);

        /* ES initialization */
        sprintf(sz_info, "%s", "L2_TM : enable ES for port ");

        /* enable global ES control */
        (void)aal_l2_tm_es_ena_set(TRUE);

        /* enable port level ES control */
        for (i=CA_AAL_ES_PORT_NI0; i<=CA_AAL_ES_MAX_PORT_ID; i++)
        {
            if (i != 14)
            {
                sprintf(sz_tmp, (i == CA_AAL_ES_MAX_PORT_ID) ? "%02d\n" : "%02d, ", i);
                strcat(sz_info, sz_tmp);
            }
            (void)aal_l2_tm_es_port_ena_set(i, TRUE);
        }

        PRINTF("%s", sz_info);

        #if 0
        for (i=CA_AAL_ES_PORT_NI0; i<=CA_AAL_ES_PORT_NI6; i++)
            (void)aal_l2_tm_es_port_ena_set(device_id, i, TRUE);

        (void)aal_l2_tm_es_port_ena_set(device_id, CA_AAL_ES_PORT_L3QM, TRUE);
        (void)aal_l2_tm_es_port_ena_set(device_id, CA_AAL_ES_PORT_L3LAN, TRUE);
        (void)aal_l2_tm_es_port_ena_set(device_id, CA_AAL_ES_PORT_L3WAN, TRUE);
        (void)aal_l2_tm_es_port_ena_set(device_id, CA_AAL_ES_PORT_MC, TRUE);
        #endif
        /* enable voq level ES control */
        for (i=0; i<=CA_AAL_MAX_PORT_ID; i++)
            for (voq_id=0; voq_id<=7; voq_id++)
                (void)aal_l2_tm_es_voq_ena_set(i, voq_id, TRUE);
	
	
		/* increase L2TM tick to slow down tx speed */
		addr = L2TM_L2TM_ES_SCH_CFG0 + 12 * CA_AAL_ES_PORT_L3QM;
		ctrl.wrd = RTK_NE_REG_READ(addr);
		ctrl.bf.speed = 6;
		RTK_NE_REG_WRITE(addr, ctrl.wrd);

        addr = L2TM_L2TM_ES_SCH_CFG0 + 12 * CA_AAL_ES_PORT_L3LAN;
        ctrl.wrd = RTK_NE_REG_READ(addr);
        ctrl.bf.speed = 6;
        RTK_NE_REG_WRITE(addr, ctrl.wrd);

        addr = L2TM_L2TM_ES_SCH_CFG0 + 12 * CA_AAL_ES_PORT_L3WAN;
        ctrl.wrd = RTK_NE_REG_READ(addr);
        ctrl.bf.speed = 6;
        RTK_NE_REG_WRITE(addr, ctrl.wrd);

	    /* init L2 TE */
        (void)aal_l2_te_init();
        /* init l3 tm and te */
        //(void)aal_l3_tm_init(); //empty
        (void)aal_l3_te_init();
    }

     return CA_E_OK;
}