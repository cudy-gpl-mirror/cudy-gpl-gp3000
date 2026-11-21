#include <common.h>
#include <linux/delay.h>
#include <command.h>
#include <malloc.h>
#include <net.h>
#include <miiphy.h>
//#include <environment.h>
#include <env.h>
#include <asm/types.h>
#include <asm/io.h>
#include <config.h>
#include "serdes_p5.h"


static void __p5_sds_mode_get(rtk_sds_mode_t *pMode, rtk_sds_speed_t *pSpeed)
{
#if defined(CONFIG_TARGET_RTK_MERCURY)

#endif

    return ;
}

static void __p5_sds_status_get(rtk_sds_status_t *status)
{
    ca_uint32_t status_reg;

    status_reg = CA_NE_SGMII_LP_REG_READ(0xf433a0f4);
    status->signal_ok = (status_reg & 0x100) ? 1 : 0;
    status->sync_ok = (status_reg & 0x001) ? 1 : 0;
    status->link_ok = (status_reg & 0x010) ? 1 : 0;

    return;
}

#ifdef CONFIG_RTK_NI_SUPPORT_HSGMII_P5
static void __p5_sds_1g_patch(void)
{
    return;
}

static void __p5_sds_2g5_patch(void)
{
    return;
}

u32 ca_ni_p5_hsgmii_speed_set(ca_uint16_t phySpeed)
{
#if defined(CONFIG_TARGET_RTK_MERCURY)
	GLOBAL_S7_SFI_CONTROL_C_t global_s7_sfi_control_c;
	rtk_sds_speed_t sdsSpeed;
	/*
		ability
		2.5G
		write 0xf4320108 0x1001
		1G
		write 0xf4320108 0x2
		100m
		write 0xf4320108 0x1
		10m
		write 0xf4320108 0x0
	*/

	/* Trans PHY speed to SDS speed */
	switch (phySpeed)
	{
		default:
		case 0:
			sdsSpeed = RTK_SDS_SPEED_10M;
			break;
		case 1:
			sdsSpeed = RTK_SDS_SPEED_100M;
			break;
		case 2:
		case 3:
			sdsSpeed = RTK_SDS_SPEED_1G;
			break;
		case 4:
			sdsSpeed = RTK_SDS_SPEED_10G;
			break;
		case 5:
			sdsSpeed = RTK_SDS_SPEED_2G5;
			break;
	}

	switch(sdsSpeed)
	{
		case RTK_SDS_SPEED_DEFAULT:
		case RTK_SDS_SPEED_1G:
			global_s7_sfi_control_c.wrd = SERDES_REG_READ(GLOBAL_S7_SFI_CONTROL_C);
			global_s7_sfi_control_c.bf.sfi_sds0_ablty = 0x2;
			SERDES_REG_WRITE(global_s7_sfi_control_c.wrd, GLOBAL_S7_SFI_CONTROL_C);
			break;
		case RTK_SDS_SPEED_100M:
			global_s7_sfi_control_c.wrd = SERDES_REG_READ(GLOBAL_S7_SFI_CONTROL_C);
			global_s7_sfi_control_c.bf.sfi_sds0_ablty = 0x1;
			SERDES_REG_WRITE(global_s7_sfi_control_c.wrd, GLOBAL_S7_SFI_CONTROL_C);
			break;
		case RTK_SDS_SPEED_10M:
			global_s7_sfi_control_c.wrd = SERDES_REG_READ(GLOBAL_S7_SFI_CONTROL_C);
			global_s7_sfi_control_c.bf.sfi_sds0_ablty = 0x1001;
			SERDES_REG_WRITE(global_s7_sfi_control_c.wrd, GLOBAL_S7_SFI_CONTROL_C);
			break;
		case RTK_SDS_SPEED_2G5:
			global_s7_sfi_control_c.wrd = SERDES_REG_READ(GLOBAL_S7_SFI_CONTROL_C);
			global_s7_sfi_control_c.bf.sfi_sds0_ablty = 0x0;
			SERDES_REG_WRITE(global_s7_sfi_control_c.wrd, GLOBAL_S7_SFI_CONTROL_C);
			break;
		default:
			return 1;
	}
#endif

	return 0;
}

bool __p5_checkSdsReady(void)
{
	GLOBAL_S7_SFI_STATUS_B_t global_s7_sfi_status_b;
    int count = 1500;

    do {
        if(SERDES_REG_READ(0xf433a044) == 0x6)
            break;
        mdelay(10);
        count --;
    } while(count >= 0);

	if (SERDES_REG_READ(0xf433a044) != 0x6)
		return 0;

    do {
		global_s7_sfi_status_b.wrd = SERDES_REG_READ(GLOBAL_S7_SFI_STATUS_B);
        if(global_s7_sfi_status_b.bf.sfi_sds_reg_rdy)
            return 1;
        mdelay(10);
        count --;
    } while(count >= 0);

    return 0;
}

void ca_ni_p5_hsgmii_init(void)
{
#if defined(CONFIG_TARGET_RTK_MERCURY)
	GLOBAL_DPHY_RESET_t global_dphy_reset;
	GLOBAL_PHY_ISO_POWER_CTRL_t global_phy_iso_power_ctrl;
	GLOBAL_PHY_SGMII_MISC_CONTROL_t global_phy_sgmii_misc_control;
	GLOBAL_PHY_CONTROL_A_t global_phy_control_a;
	GLOBAL_S7_SFI_CONTROL_A_t global_s7_sfi_control_a;
	GLOBAL_S7_SFI_CONTROL_C_t global_s7_sfi_control_c;
	NI_HV_XGE_MISC_CONFIG_t ni_hv_xge_misc_config;
	/*
		// S7-HISGMII (port5)
		write 0xf432007c 0x5f
		write 0xf43203e0 0x1100
		write 0xf433a478 0xfffe
		write 0xf4320148 0x178bd84
		write 0xf43200b8 0x9e0200
		write 0xf4320100 0x2400001
		write 0xf4320108 0x1001
		write 0xf430bf68 0x900000ff

		//polling should be 0x6
		read 0xf433a044
		write 0xf430be00 0x8
		//polling should be 0x100000
		read 0xf4320114
	*/
	
	global_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
	global_dphy_reset.bf.reset_dphy_s7_sgmii = 0;
	global_dphy_reset.bf.reset_dphy_s8_sgmii = 0;
	SERDES_REG_WRITE(global_dphy_reset.wrd, GLOBAL_DPHY_RESET);

	global_phy_iso_power_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
	global_phy_iso_power_ctrl.bf.s7_ISO_ANA_B_V09 = 1;
	global_phy_iso_power_ctrl.bf.s8_ISO_ANA_B_V09 = 1;
	SERDES_REG_WRITE(global_phy_iso_power_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);
	
	SERDES_REG_WRITE(0xfffe, 0xf433a478);

	global_phy_sgmii_misc_control.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL);
	global_phy_sgmii_misc_control.bf.s7_MBIAS_EN_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s7_OOBS_PDB_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s7_RX50_LINK_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s7_TX_DEEMP_SEL_SGMII = 0;
	global_phy_sgmii_misc_control.bf.s7_EQHOLD_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s8_CCKBUF_EN_SGMII = 0;
	global_phy_sgmii_misc_control.bf.s8_BG_EN_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s8_MBIAS_EN_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s8_OOBS_PDB_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s8_RX50_LINK_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s8_TX_DEEMP_SEL_SGMII = 0;
	global_phy_sgmii_misc_control.bf.s8_EQHOLD_SGMII = 1;
	SERDES_REG_WRITE(global_phy_sgmii_misc_control.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL);

	global_phy_control_a.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL_A);
	global_phy_control_a.bf.s7_pcie_usb3_sgmii_sel = 2;
	global_phy_control_a.bf.s7_pcie_usb3_sel = 0;
	global_phy_control_a.bf.s8_pcie_usb3_sgmii_sel = 0;
	global_phy_control_a.bf.s8_pcie_usb3_sel = 0;
	SERDES_REG_WRITE(global_phy_control_a.wrd, GLOBAL_PHY_CONTROL_A);

	global_s7_sfi_control_a.wrd = SERDES_REG_READ(GLOBAL_S7_SFI_CONTROL_A);
	global_s7_sfi_control_a.bf.sfi_rst_sds_b = 1;
	global_s7_sfi_control_a.bf.sfi_sds_mode0 = 0x12;
	SERDES_REG_WRITE(global_s7_sfi_control_a.wrd, GLOBAL_S7_SFI_CONTROL_A);

	global_s7_sfi_control_c.wrd = SERDES_REG_READ(GLOBAL_S7_SFI_CONTROL_C);
	global_s7_sfi_control_c.bf.sfi_sds0_ablty = 1;
	global_s7_sfi_control_c.bf.sfi_sds0_ablty = 0x4;
	SERDES_REG_WRITE(global_s7_sfi_control_c.wrd, GLOBAL_S7_SFI_CONTROL_C);

	ni_hv_xge_misc_config.wrd = SERDES_REG_READ(NI_HV_XGE_MISC_CONFIG);
	ni_hv_xge_misc_config.bf.inf_sel = 1;
	ni_hv_xge_misc_config.bf.gige_mode = 1;
	SERDES_REG_WRITE(ni_hv_xge_misc_config.wrd, NI_HV_XGE_MISC_CONFIG);

	if(!__p5_checkSdsReady())
    {
        printf("rtk-sds: p5 no ready\r\n");
    }
    else
    {
        mdelay(400);
    }
#endif
}
#endif

#ifdef CONFIG_RTK_NI_SUPPORT_RXAUI_P5
#if defined(CONFIG_TARGET_RTK_TAURUS)
static signed char rxaui_workaround = -1;
static rtk_sds_patch_t p5_rxaui_patch_revA[] = {
    0xF4333064, 0xF15A,
    0xF433306C, 0x0010,
    0xF4333164, 0xF15A,
    0xF433316C, 0x0010,
    0xF4333014, 0x07B2,
    0xF4333014, 0x07BA,
    0xF4333004, 0xA846,
    0xF4333018, 0x000F,
    0xF4333028, 0xB650,
    0xF4333080, 0x9477,
    0xF4333044, 0x4C00,
    0xF4333114, 0x07B2,
    0xF4333114, 0x07BA,
    0xF4333104, 0xA846,
    0xF4333118, 0x000F,
    0xF4333128, 0xB650,
    0xF4333180, 0x9477,
    0xF4333184, 0x88AA,
    0xF4333188, 0x0063,
    0xF433318C, 0x0B66,
    0xF4334064, 0xF15A,
    0xF433406C, 0x0010,
    0xF4334164, 0xF15A,
    0xF433416C, 0x0010,
    0xF4334014, 0x07B2,
    0xF4334014, 0x07BA,
    0xF4334004, 0xA846,
    0xF4334018, 0x000F,
    0xF4334028, 0xB650,
    0xF4334080, 0x9477,
    0xF4334044, 0x4C00,
    0xF4334114, 0x07B2,
    0xF4334114, 0x07BA,
    0xF4334104, 0xA846,
    0xF4334118, 0x000F,
    0xF4334128, 0xB650,
    0xF4334180, 0x9477,
    0xF4334184, 0x88AA,
    0xF4334188, 0x0063,
    0xF433418C, 0x0B66
};

// Version 20240904
static rtk_sds_patch_t p5_rxaui_patch_revB[] = {
	0xF4333064, 0xF15A,
	0xF433306C, 0x0010,
	0xF4333164, 0xF15A,
	0xF433316C, 0x0010,
	0xF4333014, 0x07B2,
	0xF4333014, 0x07BA,
	0xF4333004, 0xA846,
	0xF4333018, 0x001F,
	0xF4333028, 0xF650,
	0xF4333080, 0x9477,
	0xF4333044, 0x4C00,
	0xF433309C, 0x61D6,
	0xF4333114, 0x07B2,
	0xF4333114, 0x07BA,
	0xF4333104, 0xA846,
	0xF4333118, 0x001F,
	0xF4333128, 0xF650,
	0xF4333180, 0x9477,
	0xF4333184, 0x88AA,
	0xF4333188, 0x0063,
	0xF433318C, 0x0B66,
	0xF4333144, 0x4C00,
	0xF433319C, 0x61D6,
	0xF4334064, 0xF15A,
	0xF433406C, 0x0010,
	0xF4334164, 0xF15A,
	0xF433416C, 0x0010,
	0xF4334014, 0x07B2,
	0xF4334014, 0x07BA,
	0xF4334004, 0xA846,
	0xF4334018, 0x001F,
	0xF4334028, 0xF650,
	0xF4334080, 0x9477,
	0xF4334044, 0x4C00,
	0xF433409C, 0x61D6,
	0xF4334114, 0x07B2,
	0xF4334114, 0x07BA,
	0xF4334104, 0xA846,
	0xF4334118, 0x001F,
	0xF4334128, 0xF650,
	0xF4334180, 0x9477,
	0xF4334184, 0x88AA,
	0xF4334188, 0x0063,
	0xF433418C, 0x0B66,
	0xF4334144, 0x4C00,
	0xF433419C, 0x61D6,

	//Off Reference clock
	0xF43330AC, 0x0013,
	0xF43340AC, 0x0013,
	0xF43350AC, 0x0013,
	0xF43360AC, 0x0013,
	0xF43370AC, 0x0038,
	0xF43380AC, 0x0038,
	0xF43331AC, 0x0013,
	0xF43341AC, 0x0013,
	0xF43351AC, 0x0013,
	0xF43361AC, 0x0013,
	0xF43371AC, 0x0000,
	0xF43381AC, 0x0000,

	/*PCIE oobs*/
	0xF4333024,0x500C,
	0xF4333124,0x500C,
	0xF4334024,0x500C,
	0xF4334124,0x500C,
	0xF4333024,0x520C,
	0xF4333124,0x520C,
	0xF4334024,0x520C,
	0xF4334124,0x520C
};

static void __p5_rxaui_ni_init(void)
{
	NI_HV_GLB_STATIC_CFG_t glb_static_cfg;
	NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_CTL_t xaui_tx_cfg;
	NI_HV_XAUI_XAUI_MAC_TXMAC_TX_CONFIG_t xaui_mactx_cfg;
	NI_HV_XAUI_XAUI_MAC_RXMAC_RX_CONFIG_t xaui_macrx_cfg;

	glb_static_cfg.wrd = SERDES_REG_READ(NI_HV_GLB_STATIC_CFG);
	glb_static_cfg.bf.venus_p5_tx_crc32_add = 1;
	SERDES_REG_WRITE(glb_static_cfg.wrd, NI_HV_GLB_STATIC_CFG);

	xaui_tx_cfg.wrd = SERDES_REG_READ(NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_CTL);
	xaui_tx_cfg.bf.rx_pausereq_sel = 1;
	SERDES_REG_WRITE(xaui_tx_cfg.wrd, NI_HV_XAUI_XAUI_MAC_CTRL_XAUI_TX_CTL);

	xaui_mactx_cfg.wrd = SERDES_REG_READ(NI_HV_XAUI_XAUI_MAC_TXMAC_TX_CONFIG);
	xaui_mactx_cfg.bf.mac_en = 1;
	SERDES_REG_WRITE(xaui_mactx_cfg.wrd, NI_HV_XAUI_XAUI_MAC_TXMAC_TX_CONFIG);

	xaui_macrx_cfg.wrd = SERDES_REG_READ(NI_HV_XAUI_XAUI_MAC_RXMAC_RX_CONFIG);
	xaui_macrx_cfg.bf.mac_en = 1;
	SERDES_REG_WRITE(xaui_macrx_cfg.wrd, NI_HV_XAUI_XAUI_MAC_RXMAC_RX_CONFIG);
}
#endif

u32 ca_ni_p5_hsgmii_rxaui_set(ca_uint16_t phySpeed)
{
#if defined(CONFIG_TARGET_RTK_TAURUS)
	ca_uint16 i, patchSize;
	unsigned int rev;
	rtk_sds_patch_t *sdsPatch;
	NI_HV_PT_PORT_STATIC_CFG_t port_static_cfg;
    NI_HV_GLB_STATIC_CFG_t glb_static_cfg;
    GLOBAL_GPIO_MUX_3_t gpio_mux_3;
    GLOBAL_IO_DRIVE_CONTROL_t io_drv_ctrl;
    NI_HV_GLB_INTF_RST_CONFIG_t intf_rst_cfg;
    GLOBAL_DPHY_RESET_t glb_dphy_reset;
    GLOBAL_PHY_ISO_POWER_CTRL_t phy_pwr_ctrl;
    GLOBAL_PHY_CONTROL_t glb_phy_ctrl;
    GLOBAL_RXAUI_SERDES_CONTROL_t rxaui_sds_ctrl;
	ca_uint32 tmp;
	rtk_sds_speed_t sdsSpeed;
	/*
		ability
		2.5G
		write 0xf4320108 0x1001
		1G
		write 0xf4320108 0x2
		100m
		write 0xf4320108 0x1
		10m
		write 0xf4320108 0x0
	*/

	/* Trans PHY speed to SDS speed */
	switch (phySpeed)
	{
		default:
		case 0:
			sdsSpeed = RTK_SDS_SPEED_10M;
			break;
		case 1:
			sdsSpeed = RTK_SDS_SPEED_100M;
			break;
		case 2:
		case 3:
			sdsSpeed = RTK_SDS_SPEED_1G;
			break;
		case 4:
			sdsSpeed = RTK_SDS_SPEED_10G;
			break;
		case 5:
			sdsSpeed = RTK_SDS_SPEED_2G5;
			break;
	}

	rev = __rtk_sds_get_chip_revision();
#if 0
	if (rev == RTK_SDS_CHIP_REVISION_A)
		printf("SoC revision A\n");
	else if (rev == RTK_SDS_CHIP_REVISION_B)
		printf("SoC revision B\n");
	else if (rev == RTK_SDS_CHIP_REVISION_C)
		printf("SoC revision C\n");
#endif

	if (rxaui_workaround<0) {
		rxaui_workaround = (rev<=RTK_SDS_CHIP_REVISION_B);
		printf("RXAUI: wa=%d\n",rxaui_workaround);
	}

	switch(sdsSpeed)
	{
		case RTK_SDS_SPEED_DEFAULT:
		case RTK_SDS_SPEED_10G:
			intf_rst_cfg.wrd = 	SERDES_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);
            intf_rst_cfg.bf.port_rst_p5 = 1;
            intf_rst_cfg.bf.pcs_rst_p5 = 1;
            intf_rst_cfg.bf.mac_rst_p5 = 1;
            SERDES_REG_WRITE(intf_rst_cfg.wrd, NI_HV_GLB_INTF_RST_CONFIG);

            glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
            SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            phy_pwr_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
            phy_pwr_ctrl.bf.s0_POW_PCIE2_1 = 0;
            phy_pwr_ctrl.bf.s0_POW_PCIE2_0 = 0;
            SERDES_REG_WRITE(phy_pwr_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

            glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s0_rxaui_mode = 0;
            SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            /* Analog SDS patch */
            /* TBD: add load from file */
            switch(rev)
            {
            case RTK_SDS_CHIP_REVISION_A:
                sdsPatch = p5_rxaui_patch_revA;
                patchSize = sizeof(p5_rxaui_patch_revA)/sizeof(rtk_sds_patch_t);
                break;
            case RTK_SDS_CHIP_REVISION_B:
            default:
                sdsPatch = p5_rxaui_patch_revB;
                patchSize = sizeof(p5_rxaui_patch_revB)/sizeof(rtk_sds_patch_t);
                break;
            }

            for(i = 0 ; i < patchSize; i ++)
            {
                SERDES_REG_WRITE(sdsPatch->data, sdsPatch->reg);
                sdsPatch ++;
            }

            glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s0_rxaui_mode = 1;
            SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_pwr_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
            phy_pwr_ctrl.bf.s0_POW_PCIE2_1 = 1;
            phy_pwr_ctrl.bf.s0_POW_PCIE2_0 = 1;
            SERDES_REG_WRITE(phy_pwr_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

            mdelay(1);

            glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 0;
            SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            mdelay(1);

            intf_rst_cfg.wrd = 	SERDES_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);
            intf_rst_cfg.bf.port_rst_p5 = 0;
            intf_rst_cfg.bf.pcs_rst_p5 = 0;
            intf_rst_cfg.bf.mac_rst_p5 = 0;
            SERDES_REG_WRITE(intf_rst_cfg.wrd, NI_HV_GLB_INTF_RST_CONFIG);

            mdelay(1);

            rxaui_sds_ctrl.wrd = SERDES_REG_READ(GLOBAL_RXAUI_SERDES_CONTROL);
            rxaui_sds_ctrl.bf.aphy_txdata_en = 3;
            SERDES_REG_WRITE(rxaui_sds_ctrl.wrd, GLOBAL_RXAUI_SERDES_CONTROL);

			/* for checking sds clock */
			if (rxaui_workaround) {
				tmp = SERDES_REG_READ(0xf43331bc);
				tmp |= (1<<10);
				SERDES_REG_WRITE(tmp, 0xf43331bc);

				tmp = SERDES_REG_READ(0xf4333140);
				tmp = tmp & ~(0x3F8);
				tmp |= (0x19 << 4);
				SERDES_REG_WRITE(tmp, 0xf4333140);

				tmp = SERDES_REG_READ(0xf433316c);
				tmp = tmp & ~(0x1F00);
				tmp |= (0x17 << 8);
				SERDES_REG_WRITE(tmp, 0xf433316c);

				tmp = SERDES_REG_READ(0xf4333134);
				tmp = tmp | (1<<6);
				SERDES_REG_WRITE(tmp, 0xf4333134);
			}

			tmp = SERDES_REG_READ(NI_HV_XAUI_XGXS_TOP_XGXS_RX_SCRATCH);
			SERDES_REG_WRITE(tmp|1, NI_HV_XAUI_XGXS_TOP_XGXS_RX_SCRATCH);

            __p5_rxaui_ni_init();
            break;
		default:
            return 1;
	}

	return 0;
#endif
}

void ca_ni_p5_rxaui_init(void)
{
	ca_ni_p5_hsgmii_rxaui_set(4);
}
#endif

void ca_ni_serdes_p5_dbg_dump(void)
{
	rtk_sds_mode_t sdsMode;
	rtk_sds_speed_t sdsSpeed;
    rtk_sds_status_t status;

	printf("Port 5 Serdes:\n");
	__p5_sds_mode_get(&sdsMode, &sdsSpeed);
	printf("	mode = 0x%08x\n", sdsMode);
	printf("	speed = 0x%08x\n", sdsSpeed);
	memset(&status, 0, sizeof(status));
	__p5_sds_status_get(&status);
	printf("	signal_ok = 0x%08x\n", status.signal_ok);
	printf("	link_ok = 0x%08x\n", status.link_ok);
	printf("	sync_ok = 0x%08x\n", status.sync_ok);
	printf("	ber_notify = 0x%08x\n", status.ber_notify);
	printf("	rx_idle = 0x%08x\n", status.rx_idle);
	printf("	ck_rdy = 0x%08x\n", status.ck_rdy);
	printf("	blk_lock = 0x%08x\n", status.blk_lock);
	printf("	link_ok_10G = 0x%08x\n", status.link_ok_10G);
	printf("	link_ok_ch0 = 0x%08x\n", status.link_ok_ch0);
	printf("	link_ok_ch1 = 0x%08x\n", status.link_ok_ch1);
	printf("	link_ok_ch2 = 0x%08x\n", status.link_ok_ch2);
	printf("	link_ok_ch3 = 0x%08x\n", status.link_ok_ch3);
	printf("	link_ok_rxaui = 0x%08x\n", status.link_ok_rxaui);
}

