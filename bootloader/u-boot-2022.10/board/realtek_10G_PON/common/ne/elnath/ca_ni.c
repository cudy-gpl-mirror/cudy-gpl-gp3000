#include <osal_soc.h>
#include <osal.h>
#include <aal_port.h>
#include <aal_ni_l2.h>
#include <aal_l2_tm.h>
#include <aal_l3_te.h>
#include <aal_l3_te_cb.h>
#include <aal_l2_classification.h>
#include <aal_ni.h>
#include <aal_l3qm.h>
#include <ca_aal_proc.h>
#include <ca_ni_tx.h>
///--
#include <osal_type.h>
#include <aal_common.h>

static int aal_driver_init()
{
    int ret = 0,i;
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t igr_cfg;
    ca_device_id_t device_id = 0;

    /* NI */
    if ((ret = aal_ni_init(device_id)) != CA_E_OK) {
        PRINTF("%s:%d: Fail to initialize NI. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    if ((ret = aal_l2_tm_init(device_id)) != CA_E_OK) {
        PRINTF("%s:%d: Fail to initialize L2 TM. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    /* L2 Classification */ //clear all cf 
    if ((ret = aal_l2_cf_init(device_id)) != CA_E_OK) {
        PRINTF("%s:%d: Fail to initialize L2 CF. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    /* port */
    if ((ret = aal_port_init(device_id, NULL)) != CA_E_OK) {
        PRINTF("%s:%d: Fail to initialize PORT. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    //enable unknown vlan fwd
    L2FE_PLC_DPID_FWD_CTRL_t l2fe_pdf_ctrl_t = {0};

    l2fe_pdf_ctrl_t.wrd = RTK_NE_REG_READ(L2FE_PLC_DPID_FWD_CTRL);
    l2fe_pdf_ctrl_t.bf.vid_fff_fwd = 1; 
    l2fe_pdf_ctrl_t.bf.unkvlan_fwd = 1;
    RTK_NE_REG_WRITE(L2FE_PLC_DPID_FWD_CTRL, l2fe_pdf_ctrl_t.wrd);

    // CONFIG_RTK_NE_SUPPORT_HSGMII_P7
    // for port7 active ethernet mode
    RTK_NE_REG_WRITE(L2FE_ARB_PDPID_MAP_TBL_DATA, 0x7);
    RTK_NE_REG_WRITE(L2FE_ARB_PDPID_MAP_TBL_ACCESS, 0xC0000007);

    return CA_E_OK;
}

static int ca_ni_init_l3qm(void)
{
	int i;
	uint8 _ca_ni_cpu64_use_fbm = 0;

	/* make sure L3QM module is ready */
	aal_l3qm_check_init_done();

	/* disable RX of L3QM, because default it's enabled */
	aal_l3qm_enable_rx(0);

	aal_l3qm_init_empty_buffer(_ca_ni_cpu64_use_fbm);

	/* disable all of EPP interrupts */
	for (i = 0; i < 8; i++) {
		aal_l3qm_enable_cpu_epp_interrupt(i, 0);
	}

	aal_l3qm_init_cpu_epp();

	/* enable all of VoQs */
	aal_l3qm_init_voq();

	/* enable TX of L3QM */
	aal_l3qm_enable_tx(1);

	/* enable RX of L3QM */
	aal_l3qm_enable_rx(1);

	aal_l3qm_init_jumbo_buf();
	aal_l3qm_init_upper_ldpid_map();
	aal_l3qm_axi_reo_rd_init(0);
	aal_l3qm_axi_reo_wr_init(0);

	return 0;
}

static void ca_ni_global_reset(void)
{
    GLOBAL_BLOCK_RESET_t g_blk_rst = {0};

    //read
    g_blk_rst.wrd = RTK_NE_REG_READ(GLOBAL_BLOCK_RESET);

    //ni_reset
    g_blk_rst.bf.reset_ni = 1;
    MDELAY(1);
    g_blk_rst.bf.reset_ni = 0;

    //l2fe_reset
    g_blk_rst.bf.reset_l2fe = 1;
    MDELAY(1);
    g_blk_rst.bf.reset_l2fe = 0;

    //l2tm_reset
    g_blk_rst.bf.reset_l2tm = 1;
    MDELAY(1);
    g_blk_rst.bf.reset_l2tm = 0;

    //l3fe_reset
    g_blk_rst.bf.reset_l3fe = 1;
    MDELAY(1);
    g_blk_rst.bf.reset_l3fe = 0;

    //tqm_reset
    g_blk_rst.bf.reset_tqm = 1;
    MDELAY(1);
    g_blk_rst.bf.reset_tqm = 0;

    //write
    RTK_NE_REG_WRITE(GLOBAL_BLOCK_RESET, g_blk_rst.wrd);

    #if 0 //need ?
    /* Reset HSGMII */
    aal_scfg_read(CFG_ID_PHY_INTERFACE, CFG_ID_PHY_INTERFACE_LEN, &(phy_intfs[0]));
    if (phy_intfs[AAL_NI_PORT_GE4] != AAL_NI_IF_MODE_GE_RGMII) {
        PRINTF("%s: Port 4 connect to HSGMII\n", __func__);
    }
    else {
        PRINTF("%s: Port 4 connect to RGMII\n", __func__);
    }
    if (phy_intfs[AAL_NI_PORT_GE4] != AAL_NI_IF_MODE_GE_RGMII) {
        sgmii_phy = devm_phy_get(&pdev->dev, "sgmii-phy");
        if (IS_ERR(sgmii_phy)) {
            dev_warn(&pdev->dev, "OF get sgmii-phy failed!!");
        }
        else {
            PRINTF("%s: sgmii_phy=%p\n", __func__, sgmii_phy);
            phy_power_on(sgmii_phy);
            mdelay(1);
        }

        phy_sdsif_reset = of_reset_control_get(np, "phy_sdsif_reset");
        if (IS_ERR(phy_sdsif_reset)) {
            dev_warn(&pdev->dev, "OF get phy_sdsif_reset failed!!");
        }
        else {
            reset_control_assert(phy_sdsif_reset);
            mdelay(1);
            reset_control_deassert(phy_sdsif_reset);
        }
        reset_control_put(phy_sdsif_reset);

        phy_sgmii_reset = of_reset_control_get(np, "phy_sgmii_reset");
        if (IS_ERR(phy_sgmii_reset)) {
            dev_warn(&pdev->dev, "OF get phy_sgmii_reset failed!!");
        }
        else {
            reset_control_assert(phy_sgmii_reset);
            mdelay(1);
            reset_control_deassert(phy_sgmii_reset);
        }
        reset_control_put(phy_sgmii_reset);

        PRINTF("%s: Do external RESET for RTL8367SB HSGMII mode....\n", __func__);
        ext_reset = of_reset_control_get(np, "ext_reset");
        if (IS_ERR(ext_reset)) {
            dev_warn(&pdev->dev, "OF get ext_reset failed!!");
        }
        else {
            reset_control_assert(ext_reset);
            mdelay(200);
            reset_control_deassert(ext_reset);
        }
        reset_control_put(ext_reset);
    }
    ptp_timer_reset = of_reset_control_get(np, "ptp_timer_reset");
    if (IS_ERR(ptp_timer_reset)) {
        dev_warn(&pdev->dev, "OF get ptp_timer_reset failed!!");
    }
    else {
        reset_control_assert(ptp_timer_reset);
        mdelay(1);
        reset_control_deassert(ptp_timer_reset);
    }
    reset_control_put(ptp_timer_reset);

    ptp_reset = of_reset_control_get(np, "ptp_reset");
    if (IS_ERR(ptp_reset)) {
        dev_warn(&pdev->dev, "OF get ptp_reset failed!!");
    }
    else {
        reset_control_deassert(ptp_reset);
        mdelay(1);
        reset_control_assert(ptp_reset);
    }
    reset_control_put(ptp_reset);
    mdelay(100);
    #endif
}

//sram
uint32 ca_ne_sram_phy_base = 0;
uint32 ca_ne_sram_base = 0;

//ddr cache
uint32 ca_ne_ddr_coherent_phy_base = 0;
uint32 ca_ne_ddr_coherent_base = 0;

//ddr no cache
uint32 ca_ne_ddr_noncache_phy_base = 0;
uint32 ca_ne_ddr_noncache_base = 0;

static void ca_ni_l3qm_enable_tx_cpu(void) 
{
    uint32 cpu_idx = 0;
    
    for (cpu_idx = 3; cpu_idx <= 7; cpu_idx++) 
    {
        aal_l3qm_enable_tx_cpu(cpu_idx, 1);
    }
}

static void ca_ni_sram_ddr_addr_setup(void)
{
    //sram
    ca_ne_sram_phy_base = CA_NI_SRAM_BASE_ADDR;
    ca_ne_sram_base = PHYS_TO_UNCACHE_VIRT(ca_ne_sram_phy_base);
    //ca_ne_sram_size = 0x3c000;

    //ddr cache
    ca_ne_ddr_coherent_phy_base = 0xbc48000;
    ca_ne_ddr_coherent_base = PHYS_TO_UNCACHE_VIRT(ca_ne_ddr_coherent_phy_base);
    //ca_ne_ddr_coherent_reserved_size = 0x10000;

    //ddr no cache
    ca_ne_ddr_noncache_phy_base = 0x09000000;
    ca_ne_ddr_noncache_base = PHYS_TO_UNCACHE_VIRT(ca_ne_ddr_noncache_phy_base);
    //ca_ne_ddr_noncache_reserved_size = 0x1a50000;
}

static void ca_ni_init_reserved_mem(void)
{
    ca_ni_sram_ddr_addr_setup();
    aal_l3qm_init_startup_config();
    /* need it? 
     * it seems for l3 hash, qm deepq, arm epp fifo */
    //aal_l3qm_init_reserved_mem(ca_ne_sram_base, /*ca_ne_sram_size*/CA_NI_SRAM_SIZE, (void *)ca_ne_sram_phy_base, (void *)ca_ne_ddr_coherent_phy_base, ca_ne_ddr_noncache_base, (void *)ca_ne_ddr_noncache_phy_base);
}

uint32 ca_aal_debug = 0;
ca_uint8_t ca_ni_cpu64_use_fbm = 0;
ca_uint8_t ca_ni_alloc_scatter = 0;
int ca_ni_napi_budget = 16;

void rtk_ne_init(void)
{
    PRINTF("goto ca_init %s\n", CA_INIT_DATE);
    //ca_aal_debug |= AAL_DBG_L3QM;
    ca_ni_init_reserved_mem();
    ca_ni_global_reset();
	aal_driver_init();
	aal_ni_init_ni(0);//port_id not use //init dma lso
	ca_ni_init_l3qm();
    ca_ni_l3qm_enable_tx_cpu();
    ca_ni_init_tx_dma_lso();
	//ca_ni_init_tx_dma_lso();//no need?
	//aal_l3qm_ne_int_init();
    //init l2cls rule
    extern void aal_l2_cf_10g_sfu_rule_init(void);
    aal_l2_cf_10g_sfu_rule_init();
}
