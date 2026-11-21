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
#include "serdes_p4.h"


static void __p4_sds_mode_get(rtk_sds_mode_t *pMode, rtk_sds_speed_t *pSpeed)
{
#if defined(CONFIG_TARGET_RTK_MERCURY)
#elif defined(CONFIG_TARGET_RTK_ELNATH)
	GLOBAL_PHY_SGMII_PCS_CONTROL_s0_t phy_sgmii_pcs_s0;
    ca_uint32_t sds_spd;

    phy_sgmii_pcs_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
    sds_spd = ((phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty & (0x3 << 12)) >> 10) | (phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty & 0x3);
    switch(phy_sgmii_pcs_s0.bf.sgmii_sds_mode0)
    {
    case 0x1f:
    default:
        *pMode = RTK_SDS_MODE_NONE;
        *pSpeed = RTK_SDS_SPEED_DEFAULT;
        break;
    case 0x2:
    case 0x12:
        *pMode = RTK_SDS_MODE_SGMII;
        break;
    }

    if(RTK_SDS_MODE_NONE != *pMode)
    {
        switch(sds_spd)
        {
        case 0:
            *pSpeed = RTK_SDS_SPEED_10M;
            break;
        case 1:
            *pSpeed = RTK_SDS_SPEED_100M;
            break;
        case 2:
            *pSpeed = RTK_SDS_SPEED_1G;
            break;
        case 5:
            *pSpeed = RTK_SDS_SPEED_2G5;
            break;
        default:
            *pSpeed = RTK_SDS_SPEED_DEFAULT;
            break;
        }
    }

    return;
#else
    GLOBAL_DPHY_RESET_t glb_dphy_reset;
    GLOBAL_PHY_SGMII_PCS_CONTROL_t glb_sgmii_pcs_ctrl;

    glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
    if(glb_dphy_reset.bf.reset_sgmii)
    {
        *pMode = RTK_SDS_MODE_NONE;
        *pSpeed = RTK_SDS_SPEED_DEFAULT;
    }
    else
    {
        glb_sgmii_pcs_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL);
        *pMode = RTK_SDS_MODE_SGMII;

        switch(glb_sgmii_pcs_ctrl.bf.sgmii_cf_spd0x)
        {
        case 0:
            *pSpeed = RTK_SDS_SPEED_10M;
            break;
        case 1:
            *pSpeed = RTK_SDS_SPEED_100M;
            break;
        case 2:
            *pSpeed = RTK_SDS_SPEED_1G;
            break;
        case 5:
            *pSpeed = RTK_SDS_SPEED_2G5;
            break;
        default:
            *pSpeed = RTK_SDS_SPEED_DEFAULT;
            break;
        }
    }
#endif

    return ;
}

static void __p4_sds_status_get(rtk_sds_status_t *status)
{
    ca_uint32_t status_reg;

    status_reg = CA_NE_SGMII_LP_REG_READ(0xf433a0f4);
    status->signal_ok = (status_reg & 0x100) ? 1 : 0;
    status->sync_ok = (status_reg & 0x001) ? 1 : 0;
    status->link_ok = (status_reg & 0x010) ? 1 : 0;

    return;
}

#ifdef CONFIG_RTK_NI_SUPPORT_HSGMII_P4
static void __p4_sds_1g_patch(void)
{
#if defined(CONFIG_TARGET_RTK_ELNATH)
    ca_uint32_t value;

    /* 0xF433B204 Bit[5:0]CDR_KP=[110000] */
    value = CA_NE_SGMII_LP_REG_READ(0xF433B204);
    value = (value & ~(0x3f)) | 0x30;
    CA_NE_SGMII_LP_REG_WRITE(value, 0xF433B204);

    /* 0xF433B20C Bit[5:1]OOBS_SEN_VAL=[11100] */
    value = CA_NE_SGMII_LP_REG_READ(0xF433B20C);
    value = (value & ~(0x1f<<1)) | (0x1c<<1);
    CA_NE_SGMII_LP_REG_WRITE(value, 0xF433B20C);

    /* 0xF433B218 Bit[4:0]CMU_CP_ISEL=[11111] */
    value = CA_NE_SGMII_LP_REG_READ(0xF433B218);
    value = (value & ~(0x1f)) | 0x1f;
    CA_NE_SGMII_LP_REG_WRITE(value, 0xF433B218);

    /* 0xF433B228 Bit[14:12]CMU_SR=[111] */
    value = CA_NE_SGMII_LP_REG_READ(0xF433B228);
    value = (value & ~(0x7<<12)) | (0x7<<12);
    CA_NE_SGMII_LP_REG_WRITE(value, 0xF433B228);

    CA_NE_SGMII_LP_REG_WRITE(0x000034AA, 0xF433B240);
    CA_NE_SGMII_LP_REG_WRITE(0x000088AA, 0xF433B244);
    CA_NE_SGMII_LP_REG_WRITE(0x00000053, 0xF433B248);
    CA_NE_SGMII_LP_REG_WRITE(0x00008008, 0xF433B27C);

    /* 0xF433B24C  Bit[15:12]TX_TERM=[0000] */
    value = CA_NE_SGMII_LP_REG_READ(0xF433B24C);
    value = (value & ~(0xf<<12)) | (0x0<<12);
    CA_NE_SGMII_LP_REG_WRITE(value, 0xF433B24C);
#endif

    return;
}

static void __p4_sds_2g5_patch(void)
{
#if defined(CONFIG_TARGET_RTK_ELNATH)
    ca_uint32_t value;

    /* 0xF433B404 Bit[5:0]CDR_KP=[010000] */
    value = CA_NE_SGMII_LP_REG_READ(0xF433B404);
    value = (value & ~(0x3f)) | 0x10;
    CA_NE_SGMII_LP_REG_WRITE(value, 0xF433B404);

    /* 0xF433B40C Bit[5:1]OOBS_SEN_VAL=[11100] */
    value = CA_NE_SGMII_LP_REG_READ(0xF433B40C);
    value = (value & ~(0x1f<<1)) | (0x1c<<1);
    CA_NE_SGMII_LP_REG_WRITE(value, 0xF433B40C);

    /* 0xF433B418 Bit[4:0]CMU_CP_ISEL=[11111] */
    value = CA_NE_SGMII_LP_REG_READ(0xF433B418);
    value = (value & ~(0x1f)) | 0x1f;
    CA_NE_SGMII_LP_REG_WRITE(value, 0xF433B418);

    /* 0xF433B428 Bit[14:12]CMU_SR=[111] */
    value = CA_NE_SGMII_LP_REG_READ(0xF433B428);
    value = (value & ~(0x7<<12)) | (0x7<<12);
    CA_NE_SGMII_LP_REG_WRITE(value, 0xF433B428);

    CA_NE_SGMII_LP_REG_WRITE(0x000034AA, 0xF433B440);
    CA_NE_SGMII_LP_REG_WRITE(0x000088AA, 0xF433B444);
    CA_NE_SGMII_LP_REG_WRITE(0x00000053, 0xF433B448);
    CA_NE_SGMII_LP_REG_WRITE(0x00008008, 0xF433B47C);

    /* 0xF433B44C Bit[15:12]TX_TERM=[0000] */
    value = CA_NE_SGMII_LP_REG_READ(0xF433B44C);
    value = (value & ~(0xf<<12)) | (0x0<<12);
    CA_NE_SGMII_LP_REG_WRITE(value, 0xF433B44C);
#endif

    return;
}

u32 ca_ni_p4_hsgmii_speed_set(ca_uint16_t phySpeed)
{
#if defined(CONFIG_TARGET_RTK_MERCURY)
	GLOBAL_S8_SFI_CONTROL_H_t global_s8_sfi_control_h;
	rtk_sds_speed_t sdsSpeed;

	/*
		ability
		1G
		write 0xf4320134 0x200
		100m
		write 0xf4320134 0x100
		10m
		write 0xf4320134 0x0
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
			global_s8_sfi_control_h.wrd = SERDES_REG_READ(GLOBAL_S8_SFI_CONTROL_H);
			global_s8_sfi_control_h.bf.sfi_sds01_sds0_ablty = 0x2;
			SERDES_REG_WRITE(global_s8_sfi_control_h.wrd, GLOBAL_S8_SFI_CONTROL_H);
			break;
		case RTK_SDS_SPEED_100M:
			global_s8_sfi_control_h.wrd = SERDES_REG_READ(GLOBAL_S8_SFI_CONTROL_H);
			global_s8_sfi_control_h.bf.sfi_sds01_sds0_ablty = 0x1;
			SERDES_REG_WRITE(global_s8_sfi_control_h.wrd, GLOBAL_S8_SFI_CONTROL_H);
			break;
		case RTK_SDS_SPEED_10M:
			global_s8_sfi_control_h.wrd = SERDES_REG_READ(GLOBAL_S8_SFI_CONTROL_H);
			global_s8_sfi_control_h.bf.sfi_sds01_sds0_ablty = 0x0;
			SERDES_REG_WRITE(global_s8_sfi_control_h.wrd, GLOBAL_S8_SFI_CONTROL_H);
			break;
		default:
			return 1;
	}
	
#elif defined(CONFIG_TARGET_RTK_ELNATH)
	GLOBAL_PHY_CONTROL_t glb_phy_ctrl;
    GLOBAL_DPHY_RESET_t glb_dphy_reset;
	GLOBAL_PHY_SGMII_MISC_CONTROL_s0_t phy_sgmii_misc_s0;
	GLOBAL_PHY_SGMII_PCS_CONTROL_s0_t phy_sgmii_pcs_s0;
	rtk_sds_speed_t sdsSpeed;

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
			/* Global reset */
            glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
            glb_dphy_reset.bf.s0_reset_sgmii = 1;
            SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            udelay(100);
            /* Release reset */
            glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 0;
            glb_dphy_reset.bf.s0_reset_sgmii = 0;
            SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s0_pcie_sgmii_sel = 0x1;
            SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
            phy_sgmii_misc_s0.bf.RX50_LINK_SGMII = 0x1;
            SERDES_REG_WRITE(phy_sgmii_misc_s0.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
            phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty = 0x14002;
            SERDES_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
			break;
		case RTK_SDS_SPEED_100M:
			/* Global reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
			glb_dphy_reset.bf.s0_reset_sgmii = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(100);
			/* Release reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s0_pcie = 0;
			glb_dphy_reset.bf.s0_reset_sgmii = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
	
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s0_pcie_sgmii_sel = 0x1;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
		
			phy_sgmii_misc_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
			phy_sgmii_misc_s0.bf.RX50_LINK_SGMII = 0x1;
			SERDES_REG_WRITE(phy_sgmii_misc_s0.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
			
			/* Speed & mode configuration */
			phy_sgmii_pcs_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
			phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x2;
			phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty = 0x14001;
			SERDES_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
			break;
		case RTK_SDS_SPEED_10M:
			/* Global reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
			glb_dphy_reset.bf.s0_reset_sgmii = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(100);
			/* Release reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s0_pcie = 0;
			glb_dphy_reset.bf.s0_reset_sgmii = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
		
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s0_pcie_sgmii_sel = 0x1;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
		
			phy_sgmii_misc_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
			phy_sgmii_misc_s0.bf.RX50_LINK_SGMII = 0x1;
			SERDES_REG_WRITE(phy_sgmii_misc_s0.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
			
			/* Speed & mode configuration */
			phy_sgmii_pcs_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
			phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x2;
			phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty = 0x14000;
			SERDES_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
			break;
		case RTK_SDS_SPEED_2G5:
			/* Global reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
			glb_dphy_reset.bf.s0_reset_sgmii = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(100);
			/* Release reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s0_pcie = 0;
			glb_dphy_reset.bf.s0_reset_sgmii = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
		
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s0_pcie_sgmii_sel = 0x2;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
		
			phy_sgmii_misc_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
			phy_sgmii_misc_s0.bf.RX50_LINK_SGMII = 0x1;
			SERDES_REG_WRITE(phy_sgmii_misc_s0.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
			
			/* Speed & mode configuration */
			phy_sgmii_pcs_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
			phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x12;
			phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty = 0x01001;
			SERDES_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
			break;
		default:
			return 1;
	}
#else
	GLOBAL_PHY_CONTROL_t glb_phy_ctrl;
    GLOBAL_DPHY_RESET_t glb_dphy_reset;
    GLOBAL_PHY_SGMII_PCS_CONTROL_t glb_sgmii_pcs_ctrl;
	rtk_sds_speed_t sdsSpeed;

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
			/* Disable SDS */
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

			/* Reset SDS */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
			glb_dphy_reset.bf.reset_sgmii = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(10);
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
			glb_dphy_reset.bf.reset_sgmii = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(10);

			__p4_sds_1g_patch();

			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 1;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

			glb_sgmii_pcs_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL);
			glb_sgmii_pcs_ctrl.bf.sgmii_sds_mode0 = 0x2;
			glb_sgmii_pcs_ctrl.bf.sgmii_cf_spd0x = 0x2;
			glb_sgmii_pcs_ctrl.bf.sgmii_sds0_ablty = 0x14002;
			SERDES_REG_WRITE(glb_sgmii_pcs_ctrl.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL);
			break;
		case RTK_SDS_SPEED_500M:
			/* Disable SDS */
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

			/* Reset SDS */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
			glb_dphy_reset.bf.reset_sgmii = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(10);
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
			glb_dphy_reset.bf.reset_sgmii = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(10);

			__p4_sds_1g_patch();

			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 1;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

			glb_sgmii_pcs_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL);
			glb_sgmii_pcs_ctrl.bf.sgmii_sds_mode0 = 0x2;
			glb_sgmii_pcs_ctrl.bf.sgmii_cf_spd0x = 0x2;
			glb_sgmii_pcs_ctrl.bf.sgmii_sds0_ablty = 0x14003;
			SERDES_REG_WRITE(glb_sgmii_pcs_ctrl.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL);
			break;
		case RTK_SDS_SPEED_100M:
			/* Disable SDS */
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
	
			/* Reset SDS */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
			glb_dphy_reset.bf.reset_sgmii = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(10);
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
			glb_dphy_reset.bf.reset_sgmii = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(10);
	
			__p4_sds_1g_patch();
	
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 1;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
	
			glb_sgmii_pcs_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL);
			glb_sgmii_pcs_ctrl.bf.sgmii_sds_mode0 = 0x2;
			glb_sgmii_pcs_ctrl.bf.sgmii_cf_spd0x = 0x1;
			glb_sgmii_pcs_ctrl.bf.sgmii_sds0_ablty = 0x14001;
			SERDES_REG_WRITE(glb_sgmii_pcs_ctrl.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL);
			break;
		case RTK_SDS_SPEED_10M:
			/* Disable SDS */
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
	
			/* Reset SDS */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
			glb_dphy_reset.bf.reset_sgmii = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(10);
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
			glb_dphy_reset.bf.reset_sgmii = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(10);
	
			__p4_sds_1g_patch();
	
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 1;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
	
			glb_sgmii_pcs_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL);
			glb_sgmii_pcs_ctrl.bf.sgmii_sds_mode0 = 0x2;
			glb_sgmii_pcs_ctrl.bf.sgmii_cf_spd0x = 0x0;
			glb_sgmii_pcs_ctrl.bf.sgmii_sds0_ablty = 0x14000;
			SERDES_REG_WRITE(glb_sgmii_pcs_ctrl.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL);
			break;
		case RTK_SDS_SPEED_2G5:
			/* Disable SDS */
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
	
			/* Reset SDS */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
			glb_dphy_reset.bf.reset_sgmii = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(10);
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
			glb_dphy_reset.bf.reset_sgmii = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(10);
	
			__p4_sds_2g5_patch();
	
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 2;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
	
			glb_sgmii_pcs_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL);
			glb_sgmii_pcs_ctrl.bf.sgmii_sds_mode0 = 0x12;
			glb_sgmii_pcs_ctrl.bf.sgmii_sds0_ablty = 0x01001;
			glb_sgmii_pcs_ctrl.bf.sgmii_cf_spd0x = 0x5;
			glb_sgmii_pcs_ctrl.bf.sgmii_spdup_ini = 1;
			SERDES_REG_WRITE(glb_sgmii_pcs_ctrl.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL);
			break;
		default:
			return 1;
	}
#endif

	return 0;
}

bool __p4_checkSdsReady(void)
{
#if defined(CONFIG_TARGET_RTK_MERCURY)
	GLOBAL_S7_SFI_STATUS_B_t global_s7_sfi_status_b;
    int count = 1500;

    do {
        if(SERDES_REG_READ(0xf433c044) == 0x6)
            return 1;
        mdelay(10);
        count --;
    } while(count >= 0);
#endif

	return 0;
}

void ca_ni_p4_hsgmii_init(void)
{
#if defined(CONFIG_TARGET_RTK_MERCURY)
	GLOBAL_DPHY_RESET_t global_dphy_rest;
	GLOBAL_PHY_ISO_POWER_CTRL_t global_phy_iso_power_ctrl;
	GLOBAL_PHY_SGMII_MISC_CONTROL_t global_phy_sgmii_misc_control;
	GLOBAL_PHY_CONTROL_A_t global_phy_control_a;
	GLOBAL_S8_SFI_CONTROL_A_t global_s8_sfi_control_a;
	GLOBAL_S8_SFI_CONTROL_H_t global_s8_sfi_control_h;

	/*
		// S8-SGMII (port4)
		write 0xf432007c 0x7f
		write 0xf433a478 0xfffe
		write 0xf43203e0 0x1000
		write 0xf4320148 0x1789584
		write 0xf43200b8 0x9e0800
		write 0xf4320118 0x40005
		write 0xf4320134 0x200
		//polling should be 0x6
		read 0xf433c044 0x1
	*/
	global_dphy_rest.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
	global_dphy_rest.bf.reset_dphy_s8_sgmii = 0;
	SERDES_REG_WRITE(global_dphy_rest.wrd, GLOBAL_DPHY_RESET);

	SERDES_REG_WRITE(0xfffe, 0xf433a478);

	global_phy_iso_power_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
	global_phy_iso_power_ctrl.bf.s8_ISO_ANA_B_V09 = 1;
	SERDES_REG_WRITE(global_phy_iso_power_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

	global_phy_sgmii_misc_control.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL);
	global_phy_sgmii_misc_control.bf.s8_CCKBUF_EN_SGMII = 0;
	global_phy_sgmii_misc_control.bf.s8_BG_EN_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s8_MBIAS_EN_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s8_OOBS_PDB_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s8_RX50_LINK_SGMII = 1;
	global_phy_sgmii_misc_control.bf.s8_TX_DEEMP_SEL_SGMII = 0;
	global_phy_sgmii_misc_control.bf.s8_EQHOLD_SGMII = 1;
	SERDES_REG_WRITE(global_phy_sgmii_misc_control.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL);

	global_phy_control_a.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL_A);
	global_phy_control_a.bf.s8_pcie_usb3_sgmii_sel = 1;
	SERDES_REG_WRITE(global_phy_control_a.wrd, GLOBAL_PHY_CONTROL_A);

	global_s8_sfi_control_a.wrd = SERDES_REG_READ(GLOBAL_S8_SFI_CONTROL_A);
	global_s8_sfi_control_a.bf.sfi_rst_sds_b = 1;
	global_s8_sfi_control_a.bf.sfi_sds_mode0 = 0x2;
	SERDES_REG_WRITE(global_s8_sfi_control_a.wrd, GLOBAL_S8_SFI_CONTROL_A);

	global_s8_sfi_control_h.wrd = SERDES_REG_READ(GLOBAL_S8_SFI_CONTROL_H);
	global_s8_sfi_control_h.bf.sfi_sds01_phy_lpi_rx = 0x1;
	SERDES_REG_WRITE(global_s8_sfi_control_h.wrd, GLOBAL_S8_SFI_CONTROL_H);

	if(!__p4_checkSdsReady())
    {
        printf("rtk-sds: p4 no ready\r\n");
    }
    else
    {
        mdelay(400);
    }
#elif defined(CONFIG_TARGET_RTK_ELNATH)
	GLOBAL_PHY_CONTROL_t glb_phy_ctrl;
    GLOBAL_DPHY_RESET_t glb_dphy_reset;
	GLOBAL_PHY_SGMII_MISC_CONTROL_s0_t phy_sgmii_misc_s0;
	GLOBAL_PHY_SGMII_PCS_CONTROL_s0_t phy_sgmii_pcs_s0;
	rtk_sds_speed_t sdsSpeed;

	/* Global reset */
    glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
    glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
    glb_dphy_reset.bf.s0_reset_sgmii = 1;
    SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
    udelay(100);
    /* Release reset */
    glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
    glb_dphy_reset.bf.reset_dphy_s0_pcie = 0;
    glb_dphy_reset.bf.s0_reset_sgmii = 0;
    SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

    glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
    glb_phy_ctrl.bf.s0_pcie_sgmii_sel = 0x1;
    SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

    phy_sgmii_misc_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
    phy_sgmii_misc_s0.bf.RX50_LINK_SGMII = 0x1;
    SERDES_REG_WRITE(phy_sgmii_misc_s0.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
    
    /* Speed & mode configuration */
    phy_sgmii_pcs_s0.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
    phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x2;
    phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty = 0x14002;
    SERDES_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
#else
	GLOBAL_PHY_CONTROL_t glb_phy_ctrl;
	GLOBAL_DPHY_RESET_t glb_dphy_reset;
    GLOBAL_PHY_SGMII_PCS_CONTROL_t glb_sgmii_pcs_ctrl;

	/* Disable SDS */
	glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
	glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0;
	SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

	/* Reset SDS */
    glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
    glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
    glb_dphy_reset.bf.reset_sgmii = 1;
    SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
    udelay(10);
    glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
    glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
    glb_dphy_reset.bf.reset_sgmii = 0;
    SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
    udelay(10);

	__p4_sds_1g_patch();

	glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
    glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 1;
    SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

    glb_sgmii_pcs_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL);
    glb_sgmii_pcs_ctrl.bf.sgmii_sds_mode0 = 0x2;
    glb_sgmii_pcs_ctrl.bf.sgmii_cf_spd0x = 0x2;
    glb_sgmii_pcs_ctrl.bf.sgmii_sds0_ablty = 0x14002;
    SERDES_REG_WRITE(glb_sgmii_pcs_ctrl.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL);
#endif
}
#endif

void ca_ni_serdes_p4_dbg_dump(void)
{
	rtk_sds_mode_t sdsMode;
	rtk_sds_speed_t sdsSpeed;
    rtk_sds_status_t status;

	printf("Port 4 Serdes:\n");
	__p4_sds_mode_get(&sdsMode, &sdsSpeed);
	printf("	mode = 0x%08x\n", sdsMode);
	printf("	speed = 0x%08x\n", sdsSpeed);
	memset(&status, 0, sizeof(status));
	__p4_sds_status_get(&status);
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

