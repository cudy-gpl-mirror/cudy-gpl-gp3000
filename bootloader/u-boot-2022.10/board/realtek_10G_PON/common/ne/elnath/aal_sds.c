/*
 * Copyright (C) 2021 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : Definition 
 *
 * Feature : The file have include the following module and sub-modules
 *           (1) Hardware abstract layer of Serdes
 */
#include <osal_soc.h>
#include <osal.h>
#include <aal_common.h>
#include <aal_sds.h>
//#include "aal_pon.h"
//#include "ca_serdes.h"
//#include "ca_event.h"
#include <eth_port.h>

struct sds_patch_s {
    unsigned int addr;
    unsigned int data;
};

enum {
    SYSCLK_25MHZ,
    SYSCLK_40MHZ,
    SYSCLK_END
};

/* API for system clock source frequency */
extern unsigned int rtk_soc_sysclk_get_khz(void);

#define SDS_PATCH_ENTRY_MAX     10
static struct sds_patch_s p4_hisgmii_patch[SYSCLK_END][SDS_PATCH_ENTRY_MAX] = {
    /* 20240606 */
    {
        /* 25MHz */
        {0xf433b404, 0xA853},
        {0xf433b408, 0x60C6},
        {0xf433b410, 0xC000},
        {0xf433b418, 0x001C},
        {0xf433b428, 0xE650},
        {0xf433b434, 0xF72A},
        {0xf433b44c, 0x9B66},
        {0xf433b45c, 0x61D6},
        {0, 0}
    },
    {
        /* 40MHz */
        {0xf433b404, 0xA853},
        {0xf433b408, 0x60C6},
        {0xf433b410, 0xC000},
        {0xf433b414, 0x6F7A},
        {0xf433b418, 0x001C},
        {0xf433b41c, 0x2EC0},
        {0xf433b428, 0xE650},
        {0xf433b434, 0xF72A},
        {0xf433b44c, 0x9B66},
        {0xf433b45c, 0x61D6},
        {0, 0}
    }
};

static struct sds_patch_s p4_sgmii_patch[SYSCLK_END][SDS_PATCH_ENTRY_MAX] = {
    /* 20240606 */
    {
        /* 25MHz */
        {0xf433b204, 0xA857},
        {0xf433b208, 0x60C6},
        {0xf433b210, 0xC000},
        {0xf433b218, 0x001C},
        {0xf433b228, 0xE650},
        {0xf433b234, 0xF72A},
        {0xf433b24c, 0x9B66},
        {0xf433b45c, 0x61D6},
        {0, 0}
    },
    {
        /* 40MHz */
        {0xf433b204, 0xA857},
        {0xf433b208, 0x60C6},
        {0xf433b210, 0xC000},
        {0xf433b214, 0x67AA},
        {0xf433b218, 0x001C},
        {0xf433b21c, 0x2E40},
        {0xf433b228, 0xE650},
        {0xf433b234, 0xF72A},
        {0xf433b24c, 0x9B66},
        {0xf433b45c, 0x61D6},
        {0, 0}
    }
};

static struct sds_patch_s p5_hisgmii_patch[SYSCLK_END][SDS_PATCH_ENTRY_MAX] = {
    /* 20240606 */
    {
        /* 25MHz */
        {0xf433d404, 0xA853},
        {0xf433d408, 0x60C6},
        {0xf433d410, 0xD000},
        {0xf433d418, 0x001C},
        {0xf433d434, 0xF72A},
        {0xf433d44c, 0x9B66},
        {0xf433d46c, 0x00d3},
        {0, 0}
    },
    {
        /* 40MHz */
        {0xf433d404, 0xA853},
        {0xf433d408, 0x60C6},
        {0xf433d410, 0xD000},
        {0xf433d414, 0xE7BA},
        {0xf433d418, 0x001C},
        {0xf433d41c, 0x2EE0},
        {0xf433d428, 0xE668},
        {0xf433d434, 0xF72A},
        {0xf433d44c, 0x9B66},
        {0xf433d46c, 0x00d3},
        {0, 0}
    }
};

static struct sds_patch_s p5_sgmii_patch[SYSCLK_END][SDS_PATCH_ENTRY_MAX] = {
    /* 20240606 */
    {
        /* 25MHz */
        {0xf433d204, 0xA857},
        {0xf433d208, 0x60C6},
        {0xf433d210, 0xD000},
        {0xf433d218, 0x001C},
        {0xf433d228, 0xE668},
        {0xf433d234, 0xF72A},
        {0xf433d24c, 0x9B66},
        {0xf433d46c, 0x00d3},
        {0, 0}
    },
    {
        /* 40MHz */
        {0xf433d204, 0xA857},
        {0xf433d208, 0x60C6},
        {0xf433d210, 0xD000},
        {0xf433d214, 0xE3D2},
        {0xf433d218, 0x001C},
        {0xf433d21c, 0x2E60},
        {0xf433d228, 0xE668},
        {0xf433d234, 0xF72A},
        {0xf433d24c, 0x9B66},
        {0xf433d46c, 0x00d3},
        {0, 0}
    }
};

static struct sds_patch_s p6_hisgmii_patch[SYSCLK_END][SDS_PATCH_ENTRY_MAX] = {
    /* 20240606 */
    {
        /* 25MHz */
        {0xf433f404, 0xA853},
        {0xf433f408, 0x60C6},
        {0xf433f410, 0xD000},
        {0xf433f418, 0x001C},
        {0xf433f434, 0xF72A},
        {0xf433f44c, 0x9B66},
        {0xf433f46c, 0x00d3},
        {0, 0}
    },
    {
        /* 40MHz */
        {0xf433f404, 0xA853},
        {0xf433f408, 0x60C6},
        {0xf433f410, 0xD000},
        {0xf433f414, 0xE7BA},
        {0xf433f418, 0x001C},
        {0xf433f41c, 0x2EE0},
        {0xf433f428, 0xE668},
        {0xf433f434, 0xF72A},
        {0xf433f44c, 0x9B66},
        {0xf433f46c, 0x00d3},
        {0, 0}
    }
};

static struct sds_patch_s p6_sgmii_patch[SYSCLK_END][SDS_PATCH_ENTRY_MAX] = {
    /* 20240606 */
    {
        /* 25MHz */
        {0xf433f204, 0xA857},
        {0xf433f208, 0x60C6},
        {0xf433f210, 0xD000},
        {0xf433f218, 0x001C},
        {0xf433f228, 0xE668},
        {0xf433f234, 0xF72A},
        {0xf433f24c, 0x9B66},
        {0xf433f46c, 0x00d3},
        {0, 0}
    },
    {
        /* 40MHz */
        {0xf433f204, 0xA857},
        {0xf433f208, 0x60C6},
        {0xf433f210, 0xD000},
        {0xf433f214, 0xE3D2},
        {0xf433f218, 0x001C},
        {0xf433f21c, 0x2E60},
        {0xf433f228, 0xE668},
        {0xf433f234, 0xF72A},
        {0xf433f24c, 0x9B66},
        {0xf433f46c, 0x00d3},
        {0, 0}
    }
};

#define AAL_CHIP_REVISION_A  'A'
#define AAL_CHIP_REVISION_B  'B'
static ca_uint32_t __get_chip_revision(void)
{
        #if 0
        struct ca_soc_data      data;

        ca_soc_data_get(&data);
        return data.chip_revision;
        #else
        return AAL_CHIP_REVISION_A;
        #endif
}

ca_status_t __sds_patch(ca_uint32_t port_index, aal_sds_mode_t mode, aal_sds_speed_t speed)
{
    #if 0
    unsigned int clk = rtk_soc_sysclk_get_khz();
    #else
    unsigned int clk = 25000;
    #endif
    unsigned int patchSelected = SYSCLK_END;
    struct sds_patch_s *patch_array;

    switch(clk)
    {
    case 25000:
        patchSelected = SYSCLK_25MHZ;
        break;
    case 40000:
        patchSelected = SYSCLK_40MHZ;
        break;
    default:
        return CA_E_ERROR;
    }

    if(mode != AAL_SDS_MODE_SGMII)
        return CA_E_PARAM;

    switch(speed)
    {
    case AAL_SDS_SPEED_DEFAULT:
    case AAL_SDS_SPEED_1G:
    case AAL_SDS_SPEED_100M:
    case AAL_SDS_SPEED_10M:
    case AAL_SDS_SPEED_2G5:
        break;
    default:
        return CA_E_PARAM;
    }

    switch(port_index)
    {
    case CA_PORT_ID_NI4:
        patch_array = (speed == AAL_SDS_SPEED_2G5) ? &p4_hisgmii_patch[patchSelected][0] : &p4_sgmii_patch[patchSelected][0];
        break;
    case CA_PORT_ID_NI5:
        patch_array = (speed == AAL_SDS_SPEED_2G5) ? &p5_hisgmii_patch[patchSelected][0] : &p5_sgmii_patch[patchSelected][0];
        break;
    case CA_PORT_ID_NI6:
        patch_array = (speed == AAL_SDS_SPEED_2G5) ? &p6_hisgmii_patch[patchSelected][0] : &p6_sgmii_patch[patchSelected][0];
        break;
    default:
        return CA_E_PARAM;
    }

    while(patch_array->addr != 0)
    {
        CA_NE_SDS_REG_WRITE(patch_array->data, patch_array->addr);
        patch_array ++;
    };

    return CA_E_OK;
}


ca_status_t __p4_serdes_ind_read(ca_uint16_t page, ca_uint16_t reg, ca_uint16_t *pData)
{
    unsigned int addr;

    if(!pData)
    {
        return CA_E_PARAM;
    }

    addr = (0xf433 << 16) | (0x5 << 13) | ((page & 0x3f) << 7) | ((reg & 0x1f) << 2);
    *pData = CA_NE_SDS_REG_READ(addr);

    return CA_E_OK;
}

int __p4_serdes_ind_write(ca_uint16_t page, ca_uint16_t reg, ca_uint16_t data)
{
    unsigned int addr;

    addr = (0xf433 << 16) | (0x5 << 13) | ((page & 0x3f) << 7) | ((reg & 0x1f) << 2);
    CA_NE_SDS_REG_WRITE(data & 0xffff, addr);

    return CA_E_OK;
}

void __p4_sds_status_get(
    aal_sds_status_t *status)
{
    ca_uint32_t status_reg;
    
    status_reg = CA_NE_SDS_REG_READ(0xf433a0f4);
    status->signal_ok = (status_reg & 0x100) ? 1 : 0;
    status->sync_ok = (status_reg & 0x001) ? 1 : 0;
    status->link_ok = (status_reg & 0x010) ? 1 : 0;

    return;
}

static void __p4_sds_power_set(
    ca_boolean_t enable)
{
    GLOBAL_PHY_ISO_POWER_CTRL_t glb_phy_iso_pwr_ctrl;
    GLOBAL_BLOCK_RESET_EXT_t glb_blk_reset_ext;
    GLOBAL_GLOBAL_CONFIG_t glb_cfg;

    if(enable)
    {
        glb_phy_iso_pwr_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_phy_iso_pwr_ctrl.bf.s0_LANE0_CA09PC_EN = 1;
        glb_phy_iso_pwr_ctrl.bf.s0_LANE0_CA33PC_EN = 1;
        glb_phy_iso_pwr_ctrl.bf.s0_LANE1_CA09PC_EN = 1;
        glb_phy_iso_pwr_ctrl.bf.s0_LANE1_CA33PC_EN = 1;
        glb_phy_iso_pwr_ctrl.bf.s0_ISO_ANA_B = 1;
        glb_phy_iso_pwr_ctrl.bf.s0_POW_PCIE2_1 = 1;
        glb_phy_iso_pwr_ctrl.bf.s0_POW_PCIE2_0 = 1;
        glb_phy_iso_pwr_ctrl.bf.s0_PCIE2_ISOLATE_1 = 0;
        glb_phy_iso_pwr_ctrl.bf.s0_PCIE2_ISOLATE_0 = 0;
        AAL_GLB_REG_WRITE(glb_phy_iso_pwr_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_blk_reset_ext.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_blk_reset_ext.bf.wrap_sds_s0_en = 1;
        AAL_GLB_REG_WRITE(glb_blk_reset_ext.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_cfg.wrd = AAL_GLB_REG_READ(GLOBAL_GLOBAL_CONFIG);
        glb_cfg.bf.ps_pcie0_en = 1;
        glb_cfg.bf.cfg_pcie_0_clken = 1;
        AAL_GLB_REG_WRITE(glb_cfg.wrd, GLOBAL_GLOBAL_CONFIG);    }
    else
    {
        glb_phy_iso_pwr_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_phy_iso_pwr_ctrl.bf.s0_LANE0_CA09PC_EN = 0;
        glb_phy_iso_pwr_ctrl.bf.s0_LANE0_CA33PC_EN = 0;
        glb_phy_iso_pwr_ctrl.bf.s0_LANE1_CA09PC_EN = 0;
        glb_phy_iso_pwr_ctrl.bf.s0_LANE1_CA33PC_EN = 0;
        glb_phy_iso_pwr_ctrl.bf.s0_ISO_ANA_B = 0;
        glb_phy_iso_pwr_ctrl.bf.s0_POW_PCIE2_1 = 0;
        glb_phy_iso_pwr_ctrl.bf.s0_POW_PCIE2_0 = 0;
        glb_phy_iso_pwr_ctrl.bf.s0_PCIE2_ISOLATE_1 = 1;
        glb_phy_iso_pwr_ctrl.bf.s0_PCIE2_ISOLATE_0 = 1;
        AAL_GLB_REG_WRITE(glb_phy_iso_pwr_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_blk_reset_ext.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_blk_reset_ext.bf.wrap_sds_s0_en = 0;
        AAL_GLB_REG_WRITE(glb_blk_reset_ext.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_cfg.wrd = AAL_GLB_REG_READ(GLOBAL_GLOBAL_CONFIG);
        glb_cfg.bf.ps_pcie0_en = 0;
        glb_cfg.bf.cfg_pcie_0_clken = 0;
        AAL_GLB_REG_WRITE(glb_cfg.wrd, GLOBAL_GLOBAL_CONFIG);
    }
}

static ca_status_t __p4_sds_mode_set(
    aal_sds_mode_t mode,
    aal_sds_speed_t speed)
{
    ca_status_t ret;
    ca_uint16_t data;
    ca_uint32_t rev;
    ca_uint32_t sds_mode;
    GLOBAL_DPHY_RESET_t glb_dphy_reset;
    GLOBAL_PHY_CONTROL_t glb_phy_ctrl;
    GLOBAL_PHY_SGMII_MISC_CONTROL_s0_t phy_sgmii_misc_s0; 
    GLOBAL_PHY_SGMII_PCS_CONTROL_s0_t phy_sgmii_pcs_s0;

    rev =__get_chip_revision();

    switch(mode)
    {
    case AAL_SDS_MODE_NONE:
        __p4_sds_power_set(0);

        /* Global reset */
        glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
        glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
        glb_dphy_reset.bf.s0_reset_sgmii = 1;
        AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

        glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
        glb_phy_ctrl.bf.s0_pcie_sgmii_sel = 0x0;
        AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

        phy_sgmii_pcs_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
        phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x1f;
        AAL_GLB_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
        break;
    case AAL_SDS_MODE_SGMII:
        __p4_sds_power_set(1);
        switch(speed)
        {
        case AAL_SDS_SPEED_DEFAULT:
        case AAL_SDS_SPEED_1G:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
            glb_dphy_reset.bf.s0_reset_sgmii = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 0;
            glb_dphy_reset.bf.s0_reset_sgmii = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s0_pcie_sgmii_sel = 0x1;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
            phy_sgmii_misc_s0.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s0.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
            phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty = 0x14002;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
            break;
        case AAL_SDS_SPEED_100M:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
            glb_dphy_reset.bf.s0_reset_sgmii = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 0;
            glb_dphy_reset.bf.s0_reset_sgmii = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s0_pcie_sgmii_sel = 0x1;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
            phy_sgmii_misc_s0.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s0.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
            phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty = 0x14001;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
            break;
        case AAL_SDS_SPEED_10M:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
            glb_dphy_reset.bf.s0_reset_sgmii = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 0;
            glb_dphy_reset.bf.s0_reset_sgmii = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s0_pcie_sgmii_sel = 0x1;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
            phy_sgmii_misc_s0.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s0.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
            phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty = 0x14000;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
            break;
        case AAL_SDS_SPEED_2G5:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 1;
            glb_dphy_reset.bf.s0_reset_sgmii = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s0_pcie = 0;
            glb_dphy_reset.bf.s0_reset_sgmii = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s0_pcie_sgmii_sel = 0x2;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
            phy_sgmii_misc_s0.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s0.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s0);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
            phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x12;
            phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty = 0x01001;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
            break;
        default:
            return CA_E_PARAM;
        }
        break;
    default:
        return CA_E_PARAM;
    }

    ret = __sds_patch(CA_PORT_ID_NI4, mode, speed);
    if(ret != CA_E_OK) {
        PRINTF("SDS patch failed (%d)\n", ret);
        return CA_E_ERROR;
    }

    /* Toggle mode to reset SDS */
    phy_sgmii_pcs_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
    sds_mode = phy_sgmii_pcs_s0.bf.sgmii_sds_mode0;
    phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = 0x1f;
    AAL_GLB_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
    MDELAY(10);
    phy_sgmii_pcs_s0.bf.sgmii_sds_mode0 = sds_mode;
    AAL_GLB_REG_WRITE(phy_sgmii_pcs_s0.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s0);

    return CA_E_OK;
}

static void __p4_sds_mode_get(
    aal_sds_mode_t *pMode,
    aal_sds_speed_t *pSpeed)
{
    GLOBAL_PHY_SGMII_PCS_CONTROL_s0_t phy_sgmii_pcs_s0;
    ca_uint32_t sds_spd;

    phy_sgmii_pcs_s0.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s0);
    sds_spd = ((phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty & (0x3 << 12)) >> 10) | (phy_sgmii_pcs_s0.bf.sgmii_sds0_ablty & 0x3);
    switch(phy_sgmii_pcs_s0.bf.sgmii_sds_mode0)
    {
    case 0x1f:
    default:
        *pMode = AAL_SDS_MODE_NONE;
        *pSpeed = AAL_SDS_SPEED_DEFAULT;
        break;
    case 0x2:
    case 0x12:
        *pMode = AAL_SDS_MODE_SGMII;
        break;
    }

    if(AAL_SDS_MODE_NONE != *pMode)
    {
        switch(sds_spd)
        {
        case 0:
            *pSpeed = AAL_SDS_SPEED_10M;
            break;
        case 1:
            *pSpeed = AAL_SDS_SPEED_100M;
            break;
        case 2:
            *pSpeed = AAL_SDS_SPEED_1G;
            break;
        case 5:
            *pSpeed = AAL_SDS_SPEED_2G5;
            break;
        default:
            *pSpeed = AAL_SDS_SPEED_DEFAULT;
            break;
        }
    }

    return;
}

static ca_status_t __p4_sds_nway_set(
    ca_uint32_t enable)
{
    ca_uint16_t data;

    __p4_serdes_ind_read(0, 2, &data);
    data &= ~(1<<8);
    data |= (enable ? (1<<8) : 0);
    __p4_serdes_ind_write(0, 2, data);
    
    return CA_E_OK;
}

static ca_status_t __p4_sds_nway_get(
    ca_uint32_t *pEnable)
{
    ca_uint16_t data;

    __p4_serdes_ind_read(0, 2, &data);
    *pEnable = ((data & (1<<8)) == (1<<8)) ? 1 : 0;

    return CA_E_OK;
}

ca_status_t __p5_serdes_ind_read(ca_uint16_t page, ca_uint16_t reg, ca_uint16_t *pData)
{
    unsigned int addr;

    if(!pData)
    {
        return CA_E_PARAM;
    }

    addr = (0xf433 << 16) | (0x6 << 13) | ((page & 0x3f) << 7) | ((reg & 0x1f) << 2);
    *pData = CA_NE_SDS_REG_READ(addr);

    return CA_E_OK;
}

int __p5_serdes_ind_write(ca_uint16_t page, ca_uint16_t reg, ca_uint16_t data)
{
    unsigned int addr;

    addr = (0xf433 << 16) | (0x6 << 13) | ((page & 0x3f) << 7) | ((reg & 0x1f) << 2);
    CA_NE_SDS_REG_WRITE(data & 0xffff, addr);

    return CA_E_OK;
}

void __p5_sds_status_get(
    aal_sds_status_t *status)
{
    ca_uint32_t status_reg;
    
    status_reg = CA_NE_SDS_REG_READ(0xf433c0f4);
    status->signal_ok = (status_reg & 0x100) ? 1 : 0;
    status->sync_ok = (status_reg & 0x001) ? 1 : 0;
    status->link_ok = (status_reg & 0x010) ? 1 : 0;

    return;
}

static void __p5_sds_power_set(
    ca_boolean_t enable)
{
    GLOBAL_PHY_ISO_POWER_CTRL_t glb_phy_iso_pwr_ctrl;
    GLOBAL_BLOCK_RESET_EXT_t glb_blk_reset_ext;
    GLOBAL_GLOBAL_CONFIG_t glb_cfg;

    if(enable)
    {
        glb_phy_iso_pwr_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_phy_iso_pwr_ctrl.bf.s2_CA33PC_EN = 1;
        glb_phy_iso_pwr_ctrl.bf.s2_CA09PC_EN = 1;
        glb_phy_iso_pwr_ctrl.bf.s2_ISO_ANA_B = 1;
        glb_phy_iso_pwr_ctrl.bf.s2_POW_PCIE2 = 1;
        glb_phy_iso_pwr_ctrl.bf.s2_CPHY_ISOLATE = 0;
        AAL_GLB_REG_WRITE(glb_phy_iso_pwr_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_blk_reset_ext.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_blk_reset_ext.bf.wrap_sds_s2_en = 1;
        AAL_GLB_REG_WRITE(glb_blk_reset_ext.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_cfg.wrd = AAL_GLB_REG_READ(GLOBAL_GLOBAL_CONFIG);
        glb_cfg.bf.ps_pcie2_en = 1;
        glb_cfg.bf.cfg_pcie_2_clken = 1;
        AAL_GLB_REG_WRITE(glb_cfg.wrd, GLOBAL_GLOBAL_CONFIG);    }
    else
    {
        glb_phy_iso_pwr_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_phy_iso_pwr_ctrl.bf.s2_CA33PC_EN = 0;
        glb_phy_iso_pwr_ctrl.bf.s2_CA09PC_EN = 0;
        glb_phy_iso_pwr_ctrl.bf.s2_ISO_ANA_B = 0;
        glb_phy_iso_pwr_ctrl.bf.s2_POW_PCIE2 = 0;
        glb_phy_iso_pwr_ctrl.bf.s2_CPHY_ISOLATE = 1;
        AAL_GLB_REG_WRITE(glb_phy_iso_pwr_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_blk_reset_ext.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_blk_reset_ext.bf.wrap_sds_s2_en = 0;
        AAL_GLB_REG_WRITE(glb_blk_reset_ext.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_cfg.wrd = AAL_GLB_REG_READ(GLOBAL_GLOBAL_CONFIG);
        glb_cfg.bf.ps_pcie2_en = 0;
        glb_cfg.bf.cfg_pcie_2_clken = 0;
        AAL_GLB_REG_WRITE(glb_cfg.wrd, GLOBAL_GLOBAL_CONFIG);
    }
}

static ca_status_t __p5_sds_mode_set(
    aal_sds_mode_t mode,
    aal_sds_speed_t speed)
{
    ca_status_t ret;
    ca_uint16_t data;
    ca_uint32_t rev;
    ca_uint32_t sds_mode;
    GLOBAL_DPHY_RESET_t glb_dphy_reset;
    GLOBAL_PHY_CONTROL_t glb_phy_ctrl;
    GLOBAL_PHY_SGMII_MISC_CONTROL_s2_t phy_sgmii_misc_s2; 
    GLOBAL_PHY_SGMII_PCS_CONTROL_s2_t phy_sgmii_pcs_s2;

    rev =__get_chip_revision();

    switch(mode)
    {
    case AAL_SDS_MODE_NONE:
        __p5_sds_power_set(0);

        glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
        glb_dphy_reset.bf.reset_dphy_s2_pcie = 1;
        glb_dphy_reset.bf.s2_reset_sgmii = 1;
        AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

        glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
        glb_phy_ctrl.bf.s2_pcie_usb3_sgmii_sel = 0;
        AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

        phy_sgmii_pcs_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
        phy_sgmii_pcs_s2.bf.sgmii_sds_mode0 = 0x1f;
        AAL_GLB_REG_WRITE(phy_sgmii_pcs_s2.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
        break;
    case AAL_SDS_MODE_SGMII:
        __p5_sds_power_set(1);
        switch(speed)
        {
        case AAL_SDS_SPEED_DEFAULT:
        case AAL_SDS_SPEED_1G:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s2_pcie = 1;
            glb_dphy_reset.bf.s2_reset_sgmii = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s2_pcie = 0;
            glb_dphy_reset.bf.s2_reset_sgmii = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s2_pcie_usb3_sgmii_sel = 0x1;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s2);
            phy_sgmii_misc_s2.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s2.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s2);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
            phy_sgmii_pcs_s2.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s2.bf.sgmii_sds0_ablty = 0x14002;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s2.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
            break;
        case AAL_SDS_SPEED_100M:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s2_pcie = 1;
            glb_dphy_reset.bf.s2_reset_sgmii = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s2_pcie = 0;
            glb_dphy_reset.bf.s2_reset_sgmii = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s2_pcie_usb3_sgmii_sel = 0x1;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s2);
            phy_sgmii_misc_s2.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s2.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s2);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
            phy_sgmii_pcs_s2.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s2.bf.sgmii_sds0_ablty = 0x14001;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s2.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
            break;
        case AAL_SDS_SPEED_10M:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s2_pcie = 1;
            glb_dphy_reset.bf.s2_reset_sgmii = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s2_pcie = 0;
            glb_dphy_reset.bf.s2_reset_sgmii = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s2_pcie_usb3_sgmii_sel = 0x1;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s2);
            phy_sgmii_misc_s2.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s2.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s2);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
            phy_sgmii_pcs_s2.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s2.bf.sgmii_sds0_ablty = 0x14000;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s2.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
            break;
        case AAL_SDS_SPEED_2G5:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s2_pcie = 1;
            glb_dphy_reset.bf.s2_reset_sgmii = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.reset_dphy_s2_pcie = 0;
            glb_dphy_reset.bf.s2_reset_sgmii = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s2_pcie_usb3_sgmii_sel = 0x2;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s2);
            phy_sgmii_misc_s2.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s2.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s2);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
            phy_sgmii_pcs_s2.bf.sgmii_sds_mode0 = 0x12;
            phy_sgmii_pcs_s2.bf.sgmii_sds0_ablty = 0x01001;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s2.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
            break;
        default:
            return CA_E_PARAM;
        }
        break;
    default:
        return CA_E_PARAM;
    }

    ret = __sds_patch(CA_PORT_ID_NI5, mode, speed);
    if(ret != CA_E_OK) {
        PRINTF("SDS patch failed (%d)\n", ret);
        return CA_E_ERROR;
    }

    /* Toggle mode to reset SDS */
    phy_sgmii_pcs_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
    sds_mode = phy_sgmii_pcs_s2.bf.sgmii_sds_mode0;
    phy_sgmii_pcs_s2.bf.sgmii_sds_mode0 = 0x1f;
    AAL_GLB_REG_WRITE(phy_sgmii_pcs_s2.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
    MDELAY(10);
    phy_sgmii_pcs_s2.bf.sgmii_sds_mode0 = sds_mode;
    AAL_GLB_REG_WRITE(phy_sgmii_pcs_s2.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s2);

    return CA_E_OK;
}

static void __p5_sds_mode_get(
    aal_sds_mode_t *pMode,
    aal_sds_speed_t *pSpeed)
{
    GLOBAL_PHY_SGMII_PCS_CONTROL_s2_t phy_sgmii_pcs_s2;
    ca_uint32_t sds_spd;

    phy_sgmii_pcs_s2.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s2);
    sds_spd = ((phy_sgmii_pcs_s2.bf.sgmii_sds0_ablty & (0x3 << 12)) >> 10) | (phy_sgmii_pcs_s2.bf.sgmii_sds0_ablty & 0x3);
    switch(phy_sgmii_pcs_s2.bf.sgmii_sds_mode0)
    {
    case 0x1f:
    default:
        *pMode = AAL_SDS_MODE_NONE;
        *pSpeed = AAL_SDS_SPEED_DEFAULT;
        break;
    case 0x2:
    case 0x12:
        *pMode = AAL_SDS_MODE_SGMII;
        break;
    }

    if(AAL_SDS_MODE_NONE != *pMode)
    {
        switch(sds_spd)
        {
        case 0:
            *pSpeed = AAL_SDS_SPEED_10M;
            break;
        case 1:
            *pSpeed = AAL_SDS_SPEED_100M;
            break;
        case 2:
            *pSpeed = AAL_SDS_SPEED_1G;
            break;
        case 5:
            *pSpeed = AAL_SDS_SPEED_2G5;
            break;
        default:
            *pSpeed = AAL_SDS_SPEED_DEFAULT;
            break;
        }
    }

    return;
}

static ca_status_t __p5_sds_nway_set(
    ca_uint32_t enable)
{
    ca_uint16_t data;

    __p5_serdes_ind_read(0, 2, &data);
    data &= ~(1<<8);
    data |= (enable ? (1<<8) : 0);
    __p5_serdes_ind_write(0, 2, data);

    return CA_E_OK;
}

static ca_status_t __p5_sds_nway_get(
    ca_uint32_t *pEnable)
{
    ca_uint16_t data;

    __p5_serdes_ind_read(0, 2, &data);
    *pEnable = ((data & (1<<8)) == (1<<8)) ? 1 : 0;

    return CA_E_OK;
}

ca_status_t __p6_serdes_ind_read(ca_uint16_t page, ca_uint16_t reg, ca_uint16_t *pData)
{
    unsigned int addr;

    if(!pData)
    {
        return CA_E_PARAM;
    }

    addr = (0xf433 << 16) | (0x7 << 13) | ((page & 0x3f) << 7) | ((reg & 0x1f) << 2);
    *pData = CA_NE_SDS_REG_READ(addr);

    return CA_E_OK;
}

int __p6_serdes_ind_write(ca_uint16_t page, ca_uint16_t reg, ca_uint16_t data)
{
    unsigned int addr;

    addr = (0xf433 << 16) | (0x7 << 13) | ((page & 0x3f) << 7) | ((reg & 0x1f) << 2);
    CA_NE_SDS_REG_WRITE(data & 0xffff, addr);

    return CA_E_OK;
}

void __p6_sds_status_get(
    aal_sds_status_t *status)
{
    ca_uint32_t status_reg;

    memset(&status_reg, 0, sizeof(status_reg));
    status_reg = CA_NE_SDS_REG_READ(0xf433e0f4);
    status->signal_ok = (status_reg & 0x100) ? 1 : 0;
    status->sync_ok = (status_reg & 0x001) ? 1 : 0;
    status->link_ok = (status_reg & 0x010) ? 1 : 0;

    return;
}

static void __p6_sds_power_set(
    ca_boolean_t enable)
{
    GLOBAL_PHY_ISO_POWER_CTRL_t glb_phy_iso_pwr_ctrl;
    GLOBAL_BLOCK_RESET_EXT_t glb_blk_reset_ext;
    GLOBAL_GLOBAL_CONFIG_t glb_cfg;

    if(enable)
    {
        glb_phy_iso_pwr_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_phy_iso_pwr_ctrl.bf.s3_CA33PC_EN = 1;
        glb_phy_iso_pwr_ctrl.bf.s3_CA09PC_EN = 1;
        glb_phy_iso_pwr_ctrl.bf.s3_ISO_ANA_B = 1;
        glb_phy_iso_pwr_ctrl.bf.s3_POW_USB3 = 1;
        glb_phy_iso_pwr_ctrl.bf.s3_CPHY_ISOLATE = 0;
        AAL_GLB_REG_WRITE(glb_phy_iso_pwr_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_blk_reset_ext.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_blk_reset_ext.bf.wrap_sds_s3_en = 1;
        AAL_GLB_REG_WRITE(glb_blk_reset_ext.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_cfg.wrd = AAL_GLB_REG_READ(GLOBAL_GLOBAL_CONFIG);
        glb_cfg.bf.ps_usb3_en = 1;
        AAL_GLB_REG_WRITE(glb_cfg.wrd, GLOBAL_GLOBAL_CONFIG);    }
    else
    {
        glb_phy_iso_pwr_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_phy_iso_pwr_ctrl.bf.s3_CA33PC_EN = 0;
        glb_phy_iso_pwr_ctrl.bf.s3_CA09PC_EN = 0;
        glb_phy_iso_pwr_ctrl.bf.s3_ISO_ANA_B = 0;
        glb_phy_iso_pwr_ctrl.bf.s3_POW_USB3 = 0;
        glb_phy_iso_pwr_ctrl.bf.s3_CPHY_ISOLATE = 1;
        AAL_GLB_REG_WRITE(glb_phy_iso_pwr_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_blk_reset_ext.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_ISO_POWER_CTRL);
        glb_blk_reset_ext.bf.wrap_sds_s3_en = 0;
        AAL_GLB_REG_WRITE(glb_blk_reset_ext.wrd, GLOBAL_PHY_ISO_POWER_CTRL);

        glb_cfg.wrd = AAL_GLB_REG_READ(GLOBAL_GLOBAL_CONFIG);
        glb_cfg.bf.ps_usb3_en = 0;
        AAL_GLB_REG_WRITE(glb_cfg.wrd, GLOBAL_GLOBAL_CONFIG);
    }
}

static ca_status_t __p6_sds_mode_set(
    aal_sds_mode_t mode,
    aal_sds_speed_t speed)
{
    ca_status_t ret;
    ca_uint16_t data;
    ca_uint32_t rev;
    ca_uint32_t sds_mode;
    GLOBAL_DPHY_RESET_t glb_dphy_reset;
    GLOBAL_PHY_CONTROL_t glb_phy_ctrl;
    GLOBAL_PHY_SGMII_MISC_CONTROL_s3_t phy_sgmii_misc_s3; 
    GLOBAL_PHY_SGMII_PCS_CONTROL_s3_t phy_sgmii_pcs_s3;

    rev =__get_chip_revision();

    switch(mode)
    {
    case AAL_SDS_MODE_NONE:
        __p6_sds_power_set(0);

        glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
        glb_dphy_reset.bf.s3_reset_sgmii = 1;
        glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
        glb_dphy_reset.bf.reset_dphy_s3_pcie = 1;
        AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

        glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
        glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0;
        AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

        phy_sgmii_pcs_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
        phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = 0x1f;
        AAL_GLB_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
        break;
    case AAL_SDS_MODE_SGMII:
        __p6_sds_power_set(1);
        switch(speed)
        {
        case AAL_SDS_SPEED_DEFAULT:
        case AAL_SDS_SPEED_1G:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.s3_reset_sgmii = 1;
            glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
            glb_dphy_reset.bf.reset_dphy_s3_pcie = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.s3_reset_sgmii = 0;
            glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
            glb_dphy_reset.bf.reset_dphy_s3_pcie = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0x1;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
            phy_sgmii_misc_s3.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s3.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
            phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s3.bf.sgmii_sds0_ablty = 0x14002;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
            break;
        case AAL_SDS_SPEED_100M:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.s3_reset_sgmii = 1;
            glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
            glb_dphy_reset.bf.reset_dphy_s3_pcie = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.s3_reset_sgmii = 0;
            glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
            glb_dphy_reset.bf.reset_dphy_s3_pcie = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0x1;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
            phy_sgmii_misc_s3.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s3.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
            phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s3.bf.sgmii_sds0_ablty = 0x14001;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
            break;
        case AAL_SDS_SPEED_10M:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.s3_reset_sgmii = 1;
            glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
            glb_dphy_reset.bf.reset_dphy_s3_pcie = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.s3_reset_sgmii = 0;
            glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
            glb_dphy_reset.bf.reset_dphy_s3_pcie = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0x1;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
            phy_sgmii_misc_s3.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s3.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
            phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s3.bf.sgmii_sds0_ablty = 0x14000;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
            break;
        case AAL_SDS_SPEED_2G5:
            /* Global reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.s3_reset_sgmii = 1;
            glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
            glb_dphy_reset.bf.reset_dphy_s3_pcie = 1;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            MDELAY(100);
            /* Release reset */
            glb_dphy_reset.wrd = AAL_GLB_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.s3_reset_sgmii = 0;
            glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
            glb_dphy_reset.bf.reset_dphy_s3_pcie = 0;
            AAL_GLB_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0x2;
            AAL_GLB_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
            phy_sgmii_misc_s3.bf.RX50_LINK_SGMII = 0x1;
            AAL_GLB_REG_WRITE(phy_sgmii_misc_s3.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
            phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = 0x12;
            phy_sgmii_pcs_s3.bf.sgmii_sds0_ablty = 0x01001;
            AAL_GLB_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
            break;
        default:
            return CA_E_PARAM;
        }
        break;
    default:
        return CA_E_PARAM;
    }

    ret = __sds_patch(CA_PORT_ID_NI6, mode, speed);
    if(ret != CA_E_OK) {
        PRINTF("SDS patch failed (%d)\n", ret);
        return CA_E_ERROR;
    }

    /* Toggle mode to reset SDS */
    phy_sgmii_pcs_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
    sds_mode = phy_sgmii_pcs_s3.bf.sgmii_sds_mode0;
    phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = 0x1f;
    AAL_GLB_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
    MDELAY(10);
    phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = sds_mode;
    AAL_GLB_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);

    return CA_E_OK;
}

static void __p6_sds_mode_get(
    aal_sds_mode_t *pMode,
    aal_sds_speed_t *pSpeed)
{
    GLOBAL_PHY_SGMII_PCS_CONTROL_s3_t phy_sgmii_pcs_s3;
    ca_uint32_t sds_spd;

    phy_sgmii_pcs_s3.wrd = AAL_GLB_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
    sds_spd = ((phy_sgmii_pcs_s3.bf.sgmii_sds0_ablty & (0x3 << 12)) >> 10) | (phy_sgmii_pcs_s3.bf.sgmii_sds0_ablty & 0x3);
    switch(phy_sgmii_pcs_s3.bf.sgmii_sds_mode0)
    {
    case 0x1f:
    default:
        *pMode = AAL_SDS_MODE_NONE;
        *pSpeed = AAL_SDS_SPEED_DEFAULT;
        break;
    case 0x2:
    case 0x12:
        *pMode = AAL_SDS_MODE_SGMII;
        break;
    }

    if(AAL_SDS_MODE_NONE != *pMode)
    {
        switch(sds_spd)
        {
        case 0:
            *pSpeed = AAL_SDS_SPEED_10M;
            break;
        case 1:
            *pSpeed = AAL_SDS_SPEED_100M;
            break;
        case 2:
            *pSpeed = AAL_SDS_SPEED_1G;
            break;
        case 5:
            *pSpeed = AAL_SDS_SPEED_2G5;
            break;
        default:
            *pSpeed = AAL_SDS_SPEED_DEFAULT;
            break;
        }
    }

    return;
}

static ca_status_t __p6_sds_nway_set(
    ca_uint32_t enable)
{
    ca_uint16_t data;

    __p6_serdes_ind_read(0, 2, &data);
    data &= ~(1<<8);
    data |= (enable ? (1<<8) : 0);
    __p6_serdes_ind_write(0, 2, data);

    return CA_E_OK;
}

static ca_status_t __p6_sds_nway_get(
    ca_uint32_t *pEnable)
{
    ca_uint16_t data;

    __p6_serdes_ind_read(0, 2, &data);
    *pEnable = ((data & (1<<8)) == (1<<8)) ? 1 : 0;

    return CA_E_OK;
}

/** aal_sds_mode_set :
 * @brief: Set SDS interface mode/speed based on port index
 * @note: 
 */
ca_status_t
aal_sds_mode_set(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    aal_sds_mode_t       mode,
    aal_sds_speed_t      speed)
{
    ca_status_t ret;

    switch(port_index)
    {
    case CA_PORT_ID_NI4:
        if((ret = __p4_sds_mode_set(mode, speed)) != CA_E_OK)
        {
            return ret;
        }
        break;
    case CA_PORT_ID_NI5:
        if((ret = __p5_sds_mode_set(mode, speed)) != CA_E_OK)
        {
            return ret;
        }
        break;
    case CA_PORT_ID_NI6:
        if((ret = __p6_sds_mode_set(mode, speed)) != CA_E_OK)
        {
            return ret;
        }
        break;
    default:
        return CA_E_PARAM;
    }

    return CA_E_OK;
}

/** aal_sds_mode_get :
 * @brief: Get SDS interface mode/speed based on port index
 * @note: 
 */
ca_status_t
aal_sds_mode_get(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    aal_sds_mode_t       *mode,
    aal_sds_speed_t      *speed)
{
    ca_status_t ret;

    switch(port_index)
    {
    case CA_PORT_ID_NI4:
        __p4_sds_mode_get(mode, speed);
        break;
    case CA_PORT_ID_NI5:
        __p5_sds_mode_get(mode, speed);
        break;
    case CA_PORT_ID_NI6:
        __p6_sds_mode_get(mode, speed);
        break;
    default:
        return CA_E_PARAM;
    }

    return CA_E_OK;
}

/** aal_sds_nway_set :
 * @brief: Set SDS interface n-way ability based on port index
 * @note: 
 */
ca_status_t
aal_sds_nway_set(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    ca_uint32_t          enable)
{
    ca_status_t ret;

    switch(port_index)
    {
    case CA_PORT_ID_NI4:
        if((ret = __p4_sds_nway_set(enable)) != CA_E_OK)
        {
            return ret;
        }
        break;
    case CA_PORT_ID_NI5:
        if((ret = __p5_sds_nway_set(enable)) != CA_E_OK)
        {
            return ret;
        }
        break;
    case CA_PORT_ID_NI6:
        if((ret = __p6_sds_nway_set(enable)) != CA_E_OK)
        {
            return ret;
        }
        break;
    default:
        return CA_E_PARAM;
    }

    return CA_E_OK;
}

/** aal_sds_nway_get :
 * @brief: Get SDS interface n-way ability based on port index
 * @note: 
 */
ca_status_t
aal_sds_nway_get(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    ca_uint32_t          *enable)
{
    ca_status_t ret;

    *enable = 0;

    switch(port_index)
    {
    case CA_PORT_ID_NI4:
        if((ret = __p4_sds_nway_get(enable)) != CA_E_OK)
        {
            return ret;
        }
        break;
    case CA_PORT_ID_NI5:
        if((ret = __p5_sds_nway_get(enable)) != CA_E_OK)
        {
            return ret;
        }
        break;
    case CA_PORT_ID_NI6:
        if((ret = __p6_sds_nway_get(enable)) != CA_E_OK)
        {
            return ret;
        }
        break;
    default:
        return CA_E_PARAM;
    }

    return CA_E_OK;
}

/** aal_sds_status_get :
 * @brief: Get SDS interface status based on port index
 * @note: for each port, part or all of the status in aal_sds_status_t are valid
 * for CA_PORT_ID_NI6:
 *   ca_uint8_t ber_notify;
 *   ca_uint8_t rx_idle;
 *   ca_uint8_t ck_rdy;
 *   ca_uint8_t link_ok_10G;
 *   ca_uint8_t link_ok_ch0;
 *   ca_uint8_t link_ok_ch1;
 *   ca_uint8_t link_ok_ch2;
 *   ca_uint8_t link_ok_ch3;
 */
ca_status_t
aal_sds_status_get(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    aal_sds_status_t     *status)
{

    memset(status, 0, sizeof(aal_sds_status_t));

    switch(port_index)
    {
    case CA_PORT_ID_NI4:
        __p4_sds_status_get(status);
        break;
    case CA_PORT_ID_NI5:
        __p5_sds_status_get(status);
        break;
    case CA_PORT_ID_NI6:
        __p6_sds_status_get(status);
        break;
    default:
        return CA_E_PARAM;
    }
    
    return CA_E_OK;
}

/** aal_sds_ind_set :
 * @brief: Set SDS indirect access based on port index
 * @note: 
 */
ca_status_t
aal_sds_ind_set(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    ca_uint32_t          page,
    ca_uint32_t          reg,
    ca_uint32_t          data)
{
    switch(port_index)
    {
    case CA_PORT_ID_NI4:
        return __p4_serdes_ind_write(page, reg, data);
    case CA_PORT_ID_NI5:
        return __p5_serdes_ind_write(page, reg, data);
    case CA_PORT_ID_NI6:
        return __p6_serdes_ind_write(page, reg, data);
    default:
        return CA_E_PARAM;
    }

    return CA_E_OK;
}

/** aal_sds_ind_get :
 * @brief: Get SDS indirect access based on port index
 * @note: 
 */
ca_status_t
aal_sds_ind_get(
    ca_device_id_t       device_id,
    ca_uint32_t          port_index,
    ca_uint32_t          page,
    ca_uint32_t          reg,
    ca_uint32_t          *data)
{
    switch(port_index)
    {
    case CA_PORT_ID_NI4:
        return __p4_serdes_ind_read(page, reg, data);
    case CA_PORT_ID_NI5:
        return __p5_serdes_ind_read(page, reg, data);
    case CA_PORT_ID_NI6:
        return __p6_serdes_ind_read(page, reg, data);
    default:
        return CA_E_PARAM;
    }

    return CA_E_OK;
}

