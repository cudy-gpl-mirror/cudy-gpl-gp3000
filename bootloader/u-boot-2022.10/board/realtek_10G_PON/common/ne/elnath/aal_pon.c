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
#include <aal_pon.h>
#include <realtek_ne.h>

#ifdef CONFIG_RTK_NE_SUPPORT_HSGMII_P7

#define __SDS_CHK_TIMEOUT        1000 /* ms */
#define __SDS_CSR_NUM            512
#define __SDS_BYPASS_MSG_NUM     100
#define AAL_PPORT_QM             0x08   /* PSPID of L3_L2 Connection and PDPID of L2_L3 Connection in L2 Engine */
#define ONU_PON_MAC_MODE_IGNORED 0xFF	/* used to skip PON Initialization during SCFG loading, may be initialized later */
#define AAL_PDC_MAP_ENTRY_NUM    256
#define AAL_PON_PORT_ID            7

#define AAL_PON_REG_READ   CA_REG_READ
#define AAL_PON_REG_WRITE  CA_REG_WRITE
#define AAL_PERI_REG_WRITE CA_REG_WRITE

#define AAL_GLB_REG_FIELD_WRITE(reg, field, val)      do{\
                                            reg##_t reg_data;\
                                            reg_data.wrd = CA_REG_READ(reg);\
                                            reg_data.bf.field = (val);\
                                            CA_REG_WRITE(reg_data.wrd, reg); \
                                            }while(0)

#define AAL_PON_REG_FIELD_WRITE(reg, field, val)      do{\
                                            reg##_t reg_data;\
                                            reg_data.wrd = CA_REG_READ(reg);\
                                            reg_data.bf.field = (val);\
                                            CA_REG_WRITE(reg_data.wrd, reg); \
                                            }while(0)

#define AAL_PON_REG_FIELD_READ(reg, field, val)       do{\
                                            reg##_t reg_data;\
                                            reg_data.wrd = AAL_PON_REG_READ(reg); \
                                            (val) = reg_data.bf.field;\
                                            }while(0)

#define FOR_ALL_PDC_MAP_ENTRY(i) \
    for ((i) = 0; (i) < AAL_PDC_MAP_ENTRY_NUM; (i)++)

#define CA_RET_VALUE_CHECK(ret)         do{ \
                     if(CA_E_OK != ret) { \
                         printf("%s:[%d] return %d!!\n", __FUNCTION__, __LINE__, ret); \
                         return ret; \
                     } \
                 } while(0)

#define FIND_INDIRCT_ADDRESS(rbw, index)    (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|(ca_uint32_t)(index))
#define WRITE_INDIRCT_REG(dev,index,addr) DO_INDIRCT_OP(dev,1,index,addr)
#define READ_INDIRCT_REG(dev,index,addr)  DO_INDIRCT_OP(dev,0,index,addr)
#define AAL_PON_REG_INDIRECT_READ(index, reg)  DO_INDIRCT_OP(0, 0, index, reg)
#define AAL_PON_REG_INDIRECT_WRITE(index, reg) DO_INDIRCT_OP(0, 1, index, reg)

static ca_uint8_t  g_psds_sds_mode     = 0xFF;
static ca_uint8_t  g_psds_rx_los_polarity = 0;
static ca_uint8_t  g_psds_rx_los_polarity_set     = 0;
static ca_uint8_t  g_psds_tx_polarity_set     = 0;
static ca_uint8_t  g_psds_tx_polarity     = 0;
ca_uint32_t g_pon_chip_version = 'B'; /* 0x42 */
ca_uint32_t aal_pon_mac_mode = ONU_PON_MAC_MODE_IGNORED; /* should got from startup */

void aal_psds_burst_enable(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_boolean_t enable)
{
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PSDS_INIT_CNTL,ben_oen,enable);
}

static inline void __psds_ad_reset(void)
{
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PSDS_INIT_CNTL,POW_PCIX,0);
    mdelay(1);
}

static inline void __psds_csr_out_of_reset(void)
{
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL, psds_reg_rst_n, 0);
    mdelay(1);
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL, psds_reg_rst_n, 1);
    mdelay(1);
}

static void __pon_rx_polarity_init(ca_device_id_t device_id)
{
    /* set pon rx_los polarity */
    if(!g_psds_rx_los_polarity_set) /* if already set, skip reading from scfg */
    {
        g_psds_rx_los_polarity = 0;
    }

    AAL_GLB_REG_FIELD_WRITE(GLOBAL_OPT_MODULE_CNTL, los_invert, g_psds_rx_los_polarity);
    printf("ca-pon: init PON_RX_LOS_POLARITY_INVERT as %d.\r\n", g_psds_rx_los_polarity);
}

static void __pon_polarity_set(ca_device_id_t device_id)
{
    /* set pon RX_LOS polarity */
    __pon_rx_polarity_init(device_id);

    /* set pon laser burst tx polarity */
    if(g_psds_tx_polarity_set) /* already set, skip reading from scfg */
        return;

    g_psds_tx_polarity = 0;
}

static void __psds_eth_mode_reset(ca_uint32_t pon_mac_mode)
{
    CA_REG_WRITE(0x30f, GLOBAL_PON_CNTL);
}

static void __psds_eth_csr_reset(ca_uint32_t pon_mac_mode)
{
    CA_REG_WRITE(0x30000, GLOBAL_EPON_CNTL);
    mdelay(100);
    CA_REG_WRITE(0x30200, GLOBAL_EPON_CNTL);
    mdelay(100);
}

static inline void __psds_ad_out_of_reset(void)
{
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PSDS_INIT_CNTL,POW_PCIX,1);
    mdelay(1);
}

/*
 * bring the PON serdes interface Gearbox and related logic out of reset
 */
static inline void __psds_gbox_out_of_reset(void)
{
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL, pon_serdes_rst_n, 0);
    mdelay(1);
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL, pon_serdes_rst_n, 1);
    mdelay(1);
}

static ca_status_t __pon_mode_to_sds_mode(
    ca_scfg_pon_mac_mode_t pon_mode,
    aal_psds_mode_t*       sds_mode)
{
    *sds_mode = AAL_PSDS_MODE_NUM;

    switch(pon_mode)
    {
    case ONU_PON_MAC_EPON_1G :
        *sds_mode = AAL_PSDS_MODE_EPON;
        break;
    case ONU_PON_MAC_EPON_D10G :
        *sds_mode = AAL_PSDS_MODE_XGEPON_ASYM;
        break;
    case ONU_PON_MAC_EPON_BI10G:
        *sds_mode = AAL_PSDS_MODE_XGEPON_SYM;
        break;
    case ONU_PON_MAC_GPON       :
        *sds_mode = AAL_PSDS_MODE_GPON;
        break;
    case ONU_PON_MAC_XGPON1    :
    case ONU_PON_MAC_NGPON2_D10G   :
        *sds_mode = AAL_PSDS_MODE_XGPON_ASYM_25M;
        break;
    case ONU_PON_MAC_XGSPON    :
    case ONU_PON_MAC_NGPON2    :
        *sds_mode = AAL_PSDS_MODE_XGPON_SYM_25M;
        break;
    case ONU_PON_MAC_ETHERNET_10G  :
        *sds_mode = AAL_PSDS_MODE_ETHERNET_10G;
        break;
    case ONU_PON_MAC_ETHERNET_1250M :
        *sds_mode = AAL_PSDS_MODE_ETHERNET_1250M;
        break;
    case ONU_PON_MAC_ETHERNET_3125M :
        *sds_mode = AAL_PSDS_MODE_ETHERNET_3125M;
        break;
    case ONU_PON_MAC_TURBO_EPON:
        *sds_mode = AAL_PSDS_MODE_TUBO_EPON;
        break;
    case ONU_PON_MAC_ETHERNET_SGMII_1G:
        *sds_mode = AAL_PSDS_MODE_SGMII_1G;
        break;
    case ONU_PON_MAC_ETHERNET_SGMII_2G5:
        *sds_mode = AAL_PSDS_MODE_SGMII_2G5;
        break;
    case ONU_PON_MAC_ETHERNET_SGMII_100M:
        *sds_mode = AAL_PSDS_MODE_SGMII_100M;
        break;
    case ONU_PON_MAC_ETHERNET_SGMII_10M:
        *sds_mode = AAL_PSDS_MODE_SGMII_10M;
        break;
    case ONU_PON_MAC_ETHERNET_FIBER_100M:
        *sds_mode = AAL_PSDS_MODE_FIBER_100M;
        break;
    case ONU_PON_MAC_ETHERNET_FIBER_1G:
        *sds_mode = AAL_PSDS_MODE_FIBER_1G;
        break;
    case ONU_PON_MAC_ETHERNET_FIBER_2G5:
        *sds_mode = AAL_PSDS_MODE_FIBER_2G5;
        break;
    case ONU_PON_MAC_ETHERNET_USXGMII_10G:
        *sds_mode = AAL_PSDS_MODE_USXGMII_10G;
        break;
    case ONU_PON_MAC_ETHERNET_USXGMII_5G:
        *sds_mode = AAL_PSDS_MODE_USXGMII_5G;
        break;
    case ONU_PON_MAC_ETHERNET_USXGMII_2G5:
        *sds_mode = AAL_PSDS_MODE_USXGMII_2G5;
        break;
    case ONU_PON_MAC_ETHERNET_USXGMII_1G:
        *sds_mode = AAL_PSDS_MODE_USXGMII_1G;
        break;
    case ONU_PON_MAC_ETHERNET_USXGMII_100M:
        *sds_mode = AAL_PSDS_MODE_USXGMII_100M;
        break;
    case ONU_PON_MAC_ETHERNET_USXGMII_10M:
        *sds_mode = AAL_PSDS_MODE_USXGMII_10M;
        break;
    default:
        return CA_E_PARAM;
    }
    return CA_E_OK;
}

void __psds_mode_init(aal_psds_mode_t  sds_mode)
{
    ca_uint8_t sd=1;
    ca_uint8_t sds=0;
    ca_uint8_t usx=0;
    ca_uint8_t gpon_2p5g=0;

    switch(sds_mode)
    {
    case AAL_PSDS_MODE_EPON:
        sds = 0xC;
        usx = 0x0;
        break;
    case AAL_PSDS_MODE_GPON:
        sds = 0x8;
        usx = 0x0;
        break;
    case AAL_PSDS_MODE_GPON_SYM:
        /* FIXME GPON_SYM */
        sds = 0x8;
        usx = 0x0;
        gpon_2p5g = 1;
        break;
    case AAL_PSDS_MODE_SGMII_1G:
    case AAL_PSDS_MODE_SGMII_500M:
    case AAL_PSDS_MODE_SGMII_100M:
    case AAL_PSDS_MODE_SGMII_10M:
        sds = 0x2;
        usx = 0x2;
        sd = 0;
        break;
    case AAL_PSDS_MODE_SGMII_2G5:
        sds = 0x12;
        usx = 0x2;
        sd = 0;
        break;
    case AAL_PSDS_MODE_FIBER_100M:
        sds = 0x5;
        usx = 0x2;
        sd = 0;
        break;
    case AAL_PSDS_MODE_FIBER_1G:
        sds = 0x4;
        usx = 0x2;
        sd = 0;
        break;
    case AAL_PSDS_MODE_FIBER_2G5:
        sds = 0x16;
        usx = 0x2;
        sd = 0;
        break;
    case AAL_PSDS_MODE_ETHERNET_1250M:
    case AAL_PSDS_MODE_ETHERNET_3125M:
    case AAL_PSDS_MODE_TUBO_EPON:
    default:
        printf("%s: unknown PSDS mode:%d\n",__FUNCTION__, sds_mode);
        return;
        break;
    }

    AAL_PON_REG_FIELD_WRITE(PSDS_MODE, sd_s0, sd);
    AAL_PON_REG_FIELD_WRITE(PSDS_MODE, sds_mode_s0, sds);
    AAL_PON_REG_FIELD_WRITE(PSDS_MODE, usx_sub_mx_s0, usx);
    AAL_PON_REG_FIELD_WRITE(PSDS_MODE, gpon_2p5g_mode, gpon_2p5g);
    udelay(10);

    // more PSDS setting for Ethernet modes
    switch(sds_mode)
    {
    case AAL_PSDS_MODE_SGMII_1G:
    case AAL_PSDS_MODE_FIBER_1G:
    case AAL_PSDS_MODE_USXGMII_1G:
        AAL_PON_REG_WRITE(0x00004cf6, PSDS_CH0_ABILITY);
        AAL_PON_REG_WRITE(0x00000000, PSDS_RESET);
        break;
    case AAL_PSDS_MODE_SGMII_2G5:
    case AAL_PSDS_MODE_FIBER_2G5:
        AAL_PON_REG_WRITE(0x00005cf5, PSDS_CH0_ABILITY);
        AAL_PON_REG_WRITE(0x00000000, PSDS_RESET);
        break;
    case AAL_PSDS_MODE_SGMII_500M:
        AAL_PON_REG_WRITE(0x00004cf7, PSDS_CH0_ABILITY);
        AAL_PON_REG_WRITE(0x00000000, PSDS_RESET);
        break;
    case AAL_PSDS_MODE_FIBER_100M:
    case AAL_PSDS_MODE_SGMII_100M:
        AAL_PON_REG_WRITE(0x00004cf5, PSDS_CH0_ABILITY);
        AAL_PON_REG_WRITE(0x00000000, PSDS_RESET);
        break;
    case AAL_PSDS_MODE_SGMII_10M:
        AAL_PON_REG_WRITE(0x00004cf4, PSDS_CH0_ABILITY);
        AAL_PON_REG_WRITE(0x00000000, PSDS_RESET);
        break;
    default:
        break;
    }
}

// /etc/serdes/pon_serdes_activee_1d25g.txt
// RL6900_P7_1.25G_20230601
static const ca_uint32_t pon_serdes_data_1G25[]=
{
    0x00004802,
    0xc0000420,
    0x00000000,
    0xc0000421,
    0x00000000,
    0xc0000422,
    0x00000000,
    0xc0000423,
    0x0000f0f0,
    0xc0000424,
    0x0000f0f0,
    0xc0000425,
    0x0000f0f0,
    0xc0000426,
    0x0000f0f0,
    0xc0000427,
    0x00000000,
    0xc0000428,
    0x00008007,
    0xc0000429,
    0x00000304,
    0xc000042a,
    0x00000180,
    0xc000042b,
    0x0000ffff,
    0xc000042c,
    0x00000000,
    0xc000042d,
    0x00000000,
    0xc000042e,
    0x0000f000,
    0xc000042f,
    0x00001800,
    0xc0000430,
    0x0000ffd0,
    0xc0000431,
    0x0000b87e,
    0xc0000432,
    0x0000e005,
    0xc0000433,
    0x00000401,
    0xc0000434,
    0x00001000,
    0xc0000435,
    0x00004000,
    0xc0000436,
    0x00000000,
    0xc0000437,
    0x00000000,
    0xc0000438,
    0x00000000,
    0xc0000439,
    0x00000000,
    0xc000043a,
    0x00000000,
    0xc000043b,
    0x00000000,
    0xc000043c,
    0x00000000,
    0xc000043d,
    0x00000000,
    0xc000043e,
    0x00001000,
    0xc000043f,
    0x00000000,
    0xc00006c0,
    0x0000ff00,
    0xc00006c1,
    0x0000ff01,
    0xc00006c2,
    0x00000000,
    0xc00006c3,
    0x0000ff00,
    0xc00006c4,
    0x00005a25,
    0xc0000480,
    0x00005014,
    0xc0000481,
    0x0000f0f0,
    0xc0000482,
    0x0000f0f9,
    0xc0000483,
    0x00001e40,
    0xc0000484,
    0x0000878c,
    0xc0000485,
    0x000001c0,
    0xc0000486,
    0x00003e00,
    0xc0000487,
    0x0000d125,
    0xc0000488,
    0x00002584,
    0xc0000489,
    0x00004006,
    0xc000048a,
    0x00002424,
    0xc000048b,
    0x00004012,
    0xc000048c,
    0x00004f18,
    0xc000048d,
    0x00000042,
    0xc000048e,
    0x00005458,
    0xc000048f,
    0x000049b3,
    0xc0000490,
    0x0000f0ae,
    0xc0000491,
    0x0000f070,
    0xc0000492,
    0x00001e22,
    0xc0000493,
    0x0000200a,
    0xc0000494,
    0x00008a69,
    0xc0000495,
    0x00001597,
    0xc0000496,
    0x0000f0f0,
    0xc0000497,
    0x0000f0f2,
    0xc0000498,
    0x0000f0f0,
    0xc0000499,
    0x000042e8,
    0xc000049a,
    0x00000100,
    0xc000049b,
    0x0000f0f0,
    0xc000049c,
    0x00000000,
    0xc000049d,
    0x00008120,
    0xc000049e,
    0x00000008,
    0xc000049f,
    0x00003f0b,
    0xc00004a0,
    0x0000034d,
    0xc00004a1,
    0x00000000,
    0xc00004a2,
    0x0000fe40,
    0xc00004a3,
    0x00000000,
    0xc00004a4,
    0x00000095,
    0xc00004a5,
    0x00000000,
    0xc00004a6,
    0x00000000,
    0xc00004a7,
    0x00000000,
    0xc00004a8,
    0x00005848,
    0xc00004a9,
    0x0000ff27,
    0xc00004aa,
    0x0000d800,
    0xc00004ab,
    0x00000080,
    0xc00004ac,
    0x00008000,
    0xc00004ad,
    0x00004000,
    0xc00004ae,
    0x0000180c,
    0xc00004af,
    0x00000804,
    0xc00004b0,
    0x000003fc,
    0xc00004b1,
    0x000086ff,
    0xc00004b2,
    0x00000020,
    0xc00004b3,
    0x0000f0ff,
    0xc00004b4,
    0x0000ffff,
    0xc00004b5,
    0x0000ffff,
    0xc00004b6,
    0x00003e00,
    0xc00004b7,
    0x00000000,
    0xc00004b8,
    0x00000500,
    0xc00004b9,
    0x000007ff,
    0xc00004ba,
    0x00000fe8,
    0xc00004bb,
    0x00003fdf,
    0xc00004bc,
    0x0000001e,
    0xc00004bd,
    0x00008000,
    0xc00004be,
    0x00000000,
    0xc00004bf,
    0x00000018,
    0xc0000720,
    0x000011e0,
    0xc0000721,
    0x000003ff,
    0xc0000722,
    0x00004417,
    0xc0000723,
    0x00004000,
    0xc0000724,
    0x00008000,
    0xc0000725,
    0x00000000,
    0xc0000726,
    0x00000026,
    0xc0000727,
    0x00000f00,
    0xc0000728,
    0x00004000,
    0xc0000729,
    0x00000000,
    0xc000072a,
    0x00000258,
    0xc000072b,
    0x00000258,
    0xc000072c,
    0x0000c800,
    0xc000072d,
    0x00001070,
    0xc000072e,
    0x00000000,
    0xc000072f,
    0x00000000,
    0xc0000730,
    0x00000000,
    0xc0000731,
    0x0000005e,
    0xc0000732,
    0x00000000,
    0xc0000733,
    0x00000000,
    0xc0000734,
    0x0000f000,
    0xc0000735,
    0x00001033,
    0xc0000736,
    0x0000f0b0,
    0xc0000737,
    0x00007430,
    0xc0000738,
    0x00001c3f,
    0xc0000739,
    0x00000f0f,
    0xc000073a,
    0x00003116,
    0xc000073b,
    0x00006512,
    0xc000073c,
    0x00006117,
    0xc000073d,
    0x000086e7,
    0xc000073e,
    0x00000f0f,
    0xc000073f
};

// /etc/serdes/pon_serdes_activee_3d125g.txt
// RL6900_P7_3.125G_20230601
static const ca_uint32_t pon_serdes_data_3G125[]=
{
    0x00004802,
    0xc0000420,
    0x00000000,
    0xc0000421,
    0x00000000,
    0xc0000422,
    0x00000000,
    0xc0000423,
    0x0000f0f0,
    0xc0000424,
    0x0000f0f0,
    0xc0000425,
    0x0000f0f0,
    0xc0000426,
    0x0000f0f0,
    0xc0000427,
    0x00000000,
    0xc0000428,
    0x00008007,
    0xc0000429,
    0x00000304,
    0xc000042a,
    0x00000180,
    0xc000042b,
    0x0000ffff,
    0xc000042c,
    0x00000000,
    0xc000042d,
    0x00000000,
    0xc000042e,
    0x0000f000,
    0xc000042f,
    0x00001800,
    0xc0000430,
    0x0000ffd0,
    0xc0000431,
    0x0000b87e,
    0xc0000432,
    0x0000e005,
    0xc0000433,
    0x00000401,
    0xc0000434,
    0x00001000,
    0xc0000435,
    0x00004000,
    0xc0000436,
    0x00000000,
    0xc0000437,
    0x00000000,
    0xc0000438,
    0x00000000,
    0xc0000439,
    0x00000000,
    0xc000043a,
    0x00000000,
    0xc000043b,
    0x00000000,
    0xc000043c,
    0x00000000,
    0xc000043d,
    0x00000000,
    0xc000043e,
    0x00001000,
    0xc000043f,
    0x00000000,
    0xc00006c0,
    0x0000ff00,
    0xc00006c1,
    0x0000ff01,
    0xc00006c2,
    0x00000000,
    0xc00006c3,
    0x0000ff00,
    0xc00006c4,
    0x00005a25,
    0xc0000500,
    0x00006414,
    0xc0000501,
    0x0000f0f0,
    0xc0000502,
    0x0000f0f9,
    0xc0000503,
    0x00001e40,
    0xc0000504,
    0x0000878c,
    0xc0000505,
    0x000001c0,
    0xc0000506,
    0x00003e00,
    0xc0000507,
    0x0000d125,
    0xc0000508,
    0x00002584,
    0xc0000509,
    0x00004002,
    0xc000050a,
    0x00002424,
    0xc000050b,
    0x00004012,
    0xc000050c,
    0x00004f18,
    0xc000050d,
    0x00000042,
    0xc000050e,
    0x00005458,
    0xc000050f,
    0x000049b3,
    0xc0000510,
    0x0000f0ae,
    0xc0000511,
    0x0000f070,
    0xc0000512,
    0x00001e22,
    0xc0000513,
    0x0000200a,
    0xc0000514,
    0x00008969,
    0xc0000515,
    0x00001597,
    0xc0000516,
    0x0000f0f0,
    0xc0000517,
    0x0000f0f2,
    0xc0000518,
    0x0000f0f0,
    0xc0000519,
    0x000042e8,
    0xc000051a,
    0x00000100,
    0xc000051b,
    0x0000f0f0,
    0xc000051c,
    0x00000000,
    0xc000051d,
    0x00008120,
    0xc000051e,
    0x00000008,
    0xc000051f,
    0x00003f0b,
    0xc0000520,
    0x0000030c,
    0xc0000521,
    0x00000000,
    0xc0000522,
    0x0000fe40,
    0xc0000523,
    0x00000000,
    0xc0000524,
    0x00000096,
    0xc0000525,
    0x00000000,
    0xc0000526,
    0x00000000,
    0xc0000527,
    0x00000000,
    0xc0000528,
    0x00005848,
    0xc0000529,
    0x0000ff27,
    0xc000052a,
    0x0000d800,
    0xc000052b,
    0x00000080,
    0xc000052c,
    0x00008000,
    0xc000052d,
    0x00004000,
    0xc000052e,
    0x0000180c,
    0xc000052f,
    0x00000804,
    0xc0000530,
    0x000003fc,
    0xc0000531,
    0x000086ff,
    0xc0000532,
    0x00000020,
    0xc0000533,
    0x0000f0ff,
    0xc0000534,
    0x0000ffff,
    0xc0000535,
    0x0000ffff,
    0xc0000536,
    0x00003e00,
    0xc0000537,
    0x00000000,
    0xc0000538,
    0x00000500,
    0xc0000539,
    0x000007ff,
    0xc000053a,
    0x00000fe8,
    0xc000053b,
    0x00003fdf,
    0xc000053c,
    0x0000001e,
    0xc000053d,
    0x00008000,
    0xc000053e,
    0x00000000,
    0xc000053f,
    0x00000018,
    0xc0000740,
    0x000011e0,
    0xc0000741,
    0x000003ff,
    0xc0000742,
    0x00004417,
    0xc0000743,
    0x00004000,
    0xc0000744,
    0x00008000,
    0xc0000745,
    0x00000000,
    0xc0000746,
    0x00000030,
    0xc0000747,
    0x00000f00,
    0xc0000748,
    0x00004000,
    0xc0000749,
    0x00000000,
    0xc000074a,
    0x000002ee,
    0xc000074b,
    0x000002ee,
    0xc000074c,
    0x0000c800,
    0xc000074d,
    0x00001070,
    0xc000074e,
    0x00000000,
    0xc000074f,
    0x00000000,
    0xc0000750,
    0x00000000,
    0xc0000751,
    0x0000005e,
    0xc0000752,
    0x00000000,
    0xc0000753,
    0x00000000,
    0xc0000754,
    0x0000f000,
    0xc0000755,
    0x00001072,
    0xc0000756,
    0x0000f0b0,
    0xc0000757,
    0x00007430,
    0xc0000758,
    0x00001c3f,
    0xc0000759,
    0x00000f0f,
    0xc000075a,
    0x00003116,
    0xc000075b,
    0x00006517,
    0xc000075c,
    0x0000211d,
    0xc000075d,
    0x00008ca0,
    0xc000075e,
    0x00000f0f,
    0xc000075f
};

static ca_status_t __psds_prof_load_from_file(
    aal_psds_mode_t sds_mode,
    ca_uint32_t     pon_chip_version)
{
    int i = 0, len;

    //rdes = load_pon_serdes(sds_mode, pon_chip_version);
    if ((sds_mode == AAL_PSDS_MODE_SGMII_1G) || (sds_mode == AAL_PSDS_MODE_SGMII_100M) || (sds_mode == AAL_PSDS_MODE_SGMII_10M))
    {
        printk("loading serders file: /etc/serdes/pon_serdes_activee_1d25g.txt , mode=%d\n", sds_mode);

        len = sizeof(pon_serdes_data_1G25)/sizeof(ca_uint32_t);
        for(i=0; i<len; i=i+2)
        {
            AAL_PON_REG_WRITE(pon_serdes_data_1G25[i],PSDS_REG_DATAIN);
            udelay(10);
            AAL_PON_REG_WRITE(pon_serdes_data_1G25[i+1],PSDS_REG_ACCESS);
            udelay(10);
        }
    }
    else if ((sds_mode == AAL_PSDS_MODE_SGMII_2G5) || (sds_mode == AAL_PSDS_MODE_FIBER_2G5))
    {
        printk("loading serders file: /etc/serdes/pon_serdes_activee_3d125g.txt , mode=%d\n", sds_mode);

        len = sizeof(pon_serdes_data_3G125)/sizeof(ca_uint32_t);
        for(i=0; i<len; i=i+2)
        {
            AAL_PON_REG_WRITE(pon_serdes_data_3G125[i],PSDS_REG_DATAIN);
            udelay(10);
            AAL_PON_REG_WRITE(pon_serdes_data_3G125[i+1],PSDS_REG_ACCESS);
            udelay(10);
        }
    }
    return CA_E_OK;
}

static void __psds_prof_load(
    aal_psds_mode_t  sds_mode)
{
    /* load from SerDes file, if failed, then load from local table */
    if (CA_E_OK != __psds_prof_load_from_file(sds_mode,
            g_pon_chip_version))
    {
        printf("ca-pon: load PON SerDes configuration from SerDes file failed\r\n");
    }
    else
    {
        printf("ca-pon: load PON SerDes configuration from SerDes file successfully\r\n");
    }
}

static ca_status_t __psds_burst_polarity_set(aal_psds_mode_t sds_mode, ca_uint32 polarity)
{
    ca_uint8_t reg_page=33; //ANA_COM_REG01
    ca_uint8_t reg_num=1; //ANA_COM_REG01
    ca_uint32_t reg_data=0;

    AAL_PON_REG_WRITE((0x80000000 |(reg_page << 5) |(reg_num)), PSDS_REG_ACCESS);
    reg_data = AAL_PON_REG_READ(PSDS_REG_DATAOUT);
    reg_data = (reg_data & 0xFFFFFCFF) | (polarity << 9);  //ANA_COM_REG01, bit#9
    AAL_PON_REG_WRITE(reg_data, PSDS_REG_DATAIN);
    AAL_PON_REG_WRITE((0xC0000000 |(reg_page << 5) |(reg_num)), PSDS_REG_ACCESS);

    return CA_E_OK;
}

static ca_status_t __psds_sync(aal_psds_mode_t sds_mode)
{
    ca_uint32_t timeout1 = 0, status = 0;
    PSDS_RGB8_t mask =
    {
        .bf.RX_LOS = 1,
        .bf.rsrvd1 = 0,
        .bf.CKRDY_RX = 1,
        .bf.CKRDY_TX = 1,
        .bf.CKRDY_MPCP = 1,
        .bf.RXIDLE = 0,
        .bf.BER_NOTIFY = 1,
        .bf.rsrvd2 = 0
    };
    PSDS_RGB8_t goal =
    {
        .bf.RX_LOS = 0,
        .bf.rsrvd1 = 0,
        .bf.CKRDY_RX = 1,
        .bf.CKRDY_TX = 1,
        .bf.CKRDY_MPCP = 1,
        .bf.RXIDLE = 0,
        .bf.BER_NOTIFY = 1,
        .bf.rsrvd2 = 0
    };

    switch(sds_mode)
    {
    case AAL_PSDS_MODE_ETHERNET_10G:
    case AAL_PSDS_MODE_ETHERNET_1250M:
    case AAL_PSDS_MODE_ETHERNET_3125M:
        mask.bf.CKRDY_MPCP = 0;
        goal.bf.CKRDY_MPCP = 0;
        break;
    default:
        ;
    }

    do
    {
        status = AAL_PON_REG_READ(PSDS_RGB8);
        if ((status & mask.wrd) == goal.wrd)
        {
            /* Recovering clock CLK_WR is active; */
            printf("INFO: PON SERDES Rx clock locked OK, status = %#x. timeout=%d\n",status,timeout1);
            return CA_E_OK;
        }
        mdelay(1);
    }
    while(timeout1++ < __SDS_CHK_TIMEOUT);

    printf("INFO: PON SERDES Rx clock status = %#x.\n",status);

    return CA_E_TIMEOUT;
}

ca_status_t aal_psds_status_get(
    CA_IN ca_device_id_t         device_id,
    CA_IN aal_psds_status_t*     sts)
{
    PSDS_RGB8_t  reg_b8;
    PSDS_PRBS_ERR_CNT_H_t prbs_err_h;
    PSDS_PRBS_ERR_CNT_L_t prbs_err_l;
    PSDS_PRBS_STS_t prbs_sync_state;

    reg_b8.wrd = AAL_PON_REG_READ(PSDS_RGB8);
    prbs_err_h.wrd = AAL_PON_REG_READ(PSDS_PRBS_ERR_CNT_H);
    prbs_err_l.wrd = AAL_PON_REG_READ(PSDS_PRBS_ERR_CNT_L);
    prbs_sync_state.wrd = AAL_PON_REG_READ(PSDS_PRBS_STS);

    sts->rx_los         = reg_b8.bf.RX_LOS;
    sts->ckrdy_rx       = reg_b8.bf.CKRDY_RX;
    sts->ckrdy_tx       = reg_b8.bf.CKRDY_TX;
    sts->ckrdy_mpcp     = reg_b8.bf.CKRDY_MPCP;
    sts->rxidle         = reg_b8.bf.RXIDLE;
    sts->ber_notify     = reg_b8.bf.BER_NOTIFY;
    sts->prbs_err_cnt   = (( prbs_err_h.bf.prbsErrCnt_h << 16 ) | prbs_err_l.bf.prbsErrCnt_l);
    sts->prbs_rx_out_of_sync = prbs_sync_state.bf.syncState;

    return CA_E_OK;
}

ca_status_t aal_psds_eth_reset(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t pon_mac_mode)
{
    aal_psds_mode_t sds_mode = AAL_PSDS_MODE_ETHERNET_10G;
    aal_psds_status_t status = {0};
    ca_status_t ret =  CA_E_OK;

    __psds_ad_reset();
    __pon_polarity_set(device_id);

    __psds_eth_mode_reset(pon_mac_mode);
    __psds_csr_out_of_reset();
    ret = __pon_mode_to_sds_mode(pon_mac_mode, &sds_mode);
    if ((ret != CA_E_OK) || (sds_mode >= AAL_PSDS_MODE_NUM))
    {
        printf("ca-psds: wrong SerDes mode : %d\n",sds_mode);
        return CA_E_NOT_SUPPORT;
    }
    g_psds_sds_mode = sds_mode;
    __psds_mode_init(sds_mode);
    __psds_prof_load(sds_mode);
    __psds_burst_polarity_set(sds_mode, g_psds_tx_polarity);

    __psds_ad_out_of_reset();
    ret = __psds_sync(sds_mode);
    if (ret != CA_E_OK)
        printf("%s: Warning, failed to do psds_sync\n", __func__);

    aal_psds_status_get(device_id, &status);
    if (status.ckrdy_rx != 1 || status.ckrdy_tx != 1)
        printf("%s: serdes clock is not ready. ckrdy_rx = %d, "
               "ckrdy_tx = %d\n", __func__, status.ckrdy_rx,
               status.ckrdy_tx);

    __psds_eth_csr_reset(pon_mac_mode);

    return CA_E_OK;
}

ca_status_t aal_pon_mac_enable_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t        port_id,
    CA_IN ca_boolean_t         tx_en,
    CA_IN ca_boolean_t         rx_en)
{
    if(1) //CA_PON_MODE_IS_ETHERNET(aal_pon_mac_mode))
    {
        switch(aal_pon_mac_mode)
        {
        case ONU_PON_MAC_ETHERNET_SGMII_10M:	//SGMII_10M
        case ONU_PON_MAC_ETHERNET_SGMII_100M:	//SGMII_100M
        case ONU_PON_MAC_ETHERNET_SGMII_500M:	//SGMII_500M
        case ONU_PON_MAC_ETHERNET_SGMII_1G:		//SGMII_1G
        case ONU_PON_MAC_ETHERNET_SGMII_2G5:	//HiSGMII
        case ONU_PON_MAC_ETHERNET_FIBER_100M:
        case ONU_PON_MAC_ETHERNET_FIBER_1G:
        case ONU_PON_MAC_ETHERNET_FIBER_2G5:
            AAL_PON_REG_FIELD_WRITE(GLOBAL_EPON_CNTL,act_eth_rst_n,0);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(GLOBAL_EPON_CNTL,act_eth_rst_n,1);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(ACT_ETH_ACT_ETH_GLB_CTL,active_eth_mode,1);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(ACT_ETH_ACT_ETH_GLB_CTL,clk_sel,0);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(ACT_ETH_ACT_ETH_GLB_CTL,clk_sel,2);
            mdelay(1);

            AAL_PON_REG_WRITE(0x10F1, ACT_ETH_ACT_ETH_GLB_RESET_CTRL); /* reset */
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(ACT_ETH_ACT_ETH_GLB_RESET_CTRL,reset,0);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(ACT_ETH_ACT_ETH_GLB_RESET_CTRL,intf_tx_rst_p7,0);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(ACT_ETH_ACT_ETH_GLB_RESET_CTRL,intf_rx_rst_p7,0);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(ACT_ETH_ACT_ETH_GLB_RESET_CTRL,mac_tx_rst_p7,0);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(ACT_ETH_ACT_ETH_GLB_RESET_CTRL,mac_rx_rst_p7,0);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(ACT_ETH_ACT_ETH_GLB_RESET_CTRL,ptp_rst_p7,0);
            mdelay(1);

            AAL_PON_REG_FIELD_WRITE(ACT_ETH_PT_PORT_STATIC_CFG,int_cfg,0);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(ACT_ETH_PT_PORT_GLB_CFG,duplex,0);
            mdelay(1);

            AAL_PON_REG_FIELD_WRITE(PUC_BTCCFG, ethmode, 1);
            mdelay(1);
            AAL_PON_REG_FIELD_WRITE(PUC_PUCCFG, crccntl, 1);
            mdelay(1);
            break;
        default:
            return CA_E_PARAM;
            break;
        }
    }
    return CA_E_OK;
}

ca_status_t aal_pdc_map_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_port_id,
    CA_IN aal_pdc_pdc_map_mem_data_mask_t mask,
    CA_IN aal_pdc_pdc_map_mem_data_t *cfg)
{
    ca_status_t ret = CA_E_OK;

    PDC_PDC_MAP_MEM_DATA0_t  reg_data0;
    PDC_PDC_MAP_MEM_DATA1_t  reg_data1;

    if (!mask.wrd)
    {
        printf("nothing to set\r\n");
        return CA_E_OK;
    }

    reg_data0.wrd = 0;
    reg_data1.wrd = 0;

    //__PDC_LOCK();

    AAL_PON_REG_INDIRECT_READ(gem_port_id, PDC_PDC_MAP_MEM_ACCESS);

    reg_data0.wrd = AAL_PON_REG_READ (PDC_PDC_MAP_MEM_DATA0);
    reg_data1.wrd = AAL_PON_REG_READ (PDC_PDC_MAP_MEM_DATA1);

    if (mask.bf.cos)
        reg_data0.bf.cos = cfg->cos;
    if (mask.bf.ldpid)
        reg_data0.bf.ldpid = cfg->ldpid;
    if (mask.bf.lspid)
        reg_data0.bf.lspid = cfg->lspid;
    if (mask.bf.fe_bypass)
        reg_data0.bf.fe_bypass = cfg->fe_bypass;
    if (mask.bf.hdr_type)
        reg_data0.bf.hdr_type = cfg->hdr_type;
    if (mask.bf.mcgid)
        reg_data0.bf.mcgid = cfg->mcgid;
    if (mask.bf.drop_code)
        reg_data0.bf.drop_code = cfg->drop_code;
    if (mask.bf.rx_pkt_type)
        reg_data0.bf.rx_pkt_type = cfg->rx_pkt_type;
    if (mask.bf.no_drop)
        reg_data0.bf.no_drop = cfg->no_drop;

    if (mask.bf.mirror)
        reg_data1.bf.mirror = cfg->mirror;
    if (mask.bf.mark)
        reg_data1.bf.mark = cfg->mark;
    if (mask.bf.pol_en)
        reg_data1.bf.pol_en = cfg->pol_en;
    if (mask.bf.pol_id)
        reg_data1.bf.pol_id = cfg->pol_id;
    if (mask.bf.pol_grp_id)
        reg_data1.bf.pol_grp_id = cfg->pol_grp_id;
    if (mask.bf.deepq)
        reg_data1.bf.deepq = cfg->deepq;
    if (mask.bf.cpu_flag)
        reg_data1.bf.cpu_flag = cfg->cpu_flag;

    AAL_PON_REG_WRITE (reg_data0.wrd, PDC_PDC_MAP_MEM_DATA0);
    AAL_PON_REG_WRITE (reg_data1.wrd, PDC_PDC_MAP_MEM_DATA1);
    AAL_PON_REG_INDIRECT_WRITE(gem_port_id, PDC_PDC_MAP_MEM_ACCESS);

    //__PDC_UNLOCK();

    return ret;
}

ca_status_t aal_pdc_ctrl_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN aal_pdc_ctrl_mask_t  mask,
    CA_IN aal_pdc_ctrl_t       *data
)
{
    PDC_CTRL_t reg_data, data_old;

    reg_data.wrd  = AAL_PON_REG_READ(PDC_CTRL);
    data_old.wrd = reg_data.wrd;

    if (mask.bf.nocrc_insert)
        reg_data.bf.nocrc_insert = data->nocrc_insert;

    if (mask.bf.pdc_map_mem_en)
        reg_data.bf.pdc_map_mem_en = data->pdc_map_mem_en;

    if (mask.bf.epon_insert_16byte_hdr)
        reg_data.bf.epon_insert_16byte_hdr = data->epon_insert_16byte_hdr;

    if (mask.bf.omci_hp_cos)
        reg_data.bf.omci_hp_cos = data->omci_hp_cos;

    if (mask.bf.omci_hp_ldpid)
        reg_data.bf.omci_hp_ldpid = data->omci_hp_ldpid;

    if (mask.bf.omci_hp_en)
        reg_data.bf.omci_hp_en = data->omci_hp_en;

    if (reg_data.wrd != data_old.wrd)
    {
        //printf("write pdc reg ret:0x%x\r\n", reg_data.wrd);
        //__PDC_LOCK();
        AAL_PON_REG_WRITE(reg_data.wrd, PDC_CTRL);
        //__PDC_UNLOCK();
    }
    else
    {
        //printf("write data 0x%08x is the same as old\r\n", reg_data.wrd);
    }

    return CA_E_OK;
}

static ca_status_t __pdc_eth_init(ca_device_id_t device_id)
{
    aal_pdc_pdc_map_mem_data_mask_t pdc_mask;
    aal_pdc_pdc_map_mem_data_t      pdc_cfg;
    aal_pdc_ctrl_mask_t             pdc_ctrl_mask;
    aal_pdc_ctrl_t                  pdc_ctrl;
    ca_uint32_t                     i = 0;
    ca_status_t                     ret = CA_E_OK;

    pdc_mask.wrd = ~0;
    memset(&pdc_cfg, 0, sizeof(pdc_cfg));
    memset(&pdc_ctrl, 0, sizeof(aal_pdc_ctrl_t));

    pdc_cfg.lspid = AAL_PON_PORT_ID;

    /* for all downstream mapping: source port, dest port */
    FOR_ALL_PDC_MAP_ENTRY(i)
    {
        ret |= aal_pdc_map_mem_set(device_id, i, pdc_mask, &pdc_cfg);
    }
    CA_RET_VALUE_CHECK(ret);

    /* enable downstream mapping */
    pdc_ctrl_mask.wrd = ~0;
    pdc_ctrl.nocrc_insert = 1;

    ret = aal_pdc_ctrl_set(device_id, pdc_ctrl_mask, &pdc_ctrl);

    return ret;
}

ca_status_t aal_pdc_init(
    ca_device_id_t  device_id,
    ca_uint32_t     pon_mac_mode
)
{
    static ca_uint32_t  pdc_init_done = 0;
    ca_status_t         ret = CA_E_OK;

    if (1)   //CA_PON_MODE_IS_ETHERNET(pon_mac_mode)) {
    {
        ret = __pdc_eth_init(device_id);
    }

    if (CA_E_OK != ret)
    {
        printf("%s(): init PDC fail, return %#x\r\n", __func__,ret);
        return ret;
    }

    if (!pdc_init_done)
        printf("ca-pon: init PDC OK\n");

    pdc_init_done = 1;

    return CA_E_OK;
}

ca_status_t __pon_eth_init(
    ca_device_id_t device_id,
    ca_scfg_pon_mac_mode_t mode)
{
    ca_status_t ret = CA_E_OK;
    printf("%s(): init start\r\n", __func__);
    aal_psds_burst_enable(device_id, 0);
    aal_psds_eth_reset(device_id, mode);

    /* PDC Init */
    ret = aal_pdc_init(device_id, mode);
    if (CA_E_OK != ret)
    {
        printf("aal_pdc_init for eth mode failed(%d)\n", ret);
        return ret;
    }

    AAL_PON_REG_WRITE( 0x00000E00, ACT_ETH_ACT_ETH_GLB_HDR_A);/*Set LSPID [14:9]to 7 */

    AAL_PON_REG_WRITE( 0xDCEA7338, PUC_PVTBL_DATA0);
    AAL_PON_REG_WRITE( 0xCFA7B3c9, PUC_PVTBL_DATA1);
    AAL_PON_REG_WRITE( 0x109F, PUC_PVTBL_DATA2);
    AAL_PON_REG_WRITE( 0x0, PUC_PVTBL_DATA3);
    AAL_PON_REG_WRITE( 0x0, PUC_PVTBL_DATA4);
    AAL_PON_REG_WRITE( 0xc0000000, PUC_PVTBL_ACCESS);

    aal_pon_mac_enable_set(device_id, 0, TRUE, TRUE);
    AAL_PON_REG_WRITE( 0x00000E00, ACT_ETH_ACT_ETH_GLB_HDR_A);/*Set LSPID [14:9]to 7 */

    AAL_GLB_REG_WRITE (0, GLOBAL_OPT_INTENABLE);
    aal_psds_burst_enable(device_id, 1);
    mdelay(1);
    AAL_PON_REG_FIELD_WRITE(PSDS_GBOX_CTRL, lao_en, 1);

    CA_NI_REG_WRITE (0xe2AA0000, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG1);
    CA_NI_REG_WRITE (0xe2AA0000, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG1);

    AAL_PON_REG_WRITE( 0x41000114, PUC_BTCCFG);
    AAL_PON_REG_WRITE( 0x41000914, PUC_BTCCFG);

    aal_port_arb_ldpid_pdpid_map_set(device_id, 0, 0, 7, AAL_PPORT_QM);
    aal_port_arb_ldpid_pdpid_map_set(device_id, 0, 1, 7, AAL_PPORT_QM);
    aal_port_arb_ldpid_pdpid_map_set(device_id, 1, 0, 7, AAL_PPORT_QM);
    aal_port_arb_ldpid_pdpid_map_set(device_id, 1, 1, 7, AAL_PPORT_QM);

    AAL_PERI_REG_WRITE(0x105,PER_DYING_GASP);
    AAL_PERI_REG_WRITE(1,PER_DG_IE_0);
    printf("set Dying Gasp Registers\r\n");

    /* check link status per second */
    //g_eth_link_monitor_timer = ca_circle_timer_add(1000, __eth_link_monitor_handler, NULL);
    printf ( "%s: init active-Ethernet successfully\n", __FUNCTION__ );

    return ret;
}

ca_status_t aal_psds_eth_link_status_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_boolean_t  *link_up
)
{
    switch(g_psds_sds_mode)
    {
    case AAL_PSDS_MODE_SGMII_10M:
    case AAL_PSDS_MODE_SGMII_100M:
    case AAL_PSDS_MODE_SGMII_500M:
    case AAL_PSDS_MODE_SGMII_1G:
    case AAL_PSDS_MODE_SGMII_2G5:
    case AAL_PSDS_MODE_FIBER_100M:
    case AAL_PSDS_MODE_FIBER_1G:
    case AAL_PSDS_MODE_FIBER_2G5:
        AAL_PON_REG_FIELD_READ(PSDS_IF_STATUS, sds01_sds_link_ok, *link_up);
        break;
    case AAL_PSDS_MODE_ETHERNET_1250M:
    case AAL_PSDS_MODE_ETHERNET_3125M:
    default:
        printf("%s: non-supported Ethernet mode\n", __FUNCTION__) ;
        return CA_E_NOT_SUPPORT;
        break;
    }

    printf("%s: link status of active-Ethernet port is %u, g_psds_sds_mode= %d\n", __FUNCTION__, *link_up, g_psds_sds_mode);

    return CA_E_OK;
}

ca_status_t aal_epon_mac_cfg_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_mac_ctrl_msk_t        mask,
    CA_IN aal_epon_mac_ctrl_t           *cfg
)
{

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(cfg, CA_E_PARAM);

    /* param check */
    if(mask.s.tx_xg_ipg)
    {
        if((cfg->tx_xg_ipg!=8) && (cfg->tx_xg_ipg!=12))
            return CA_E_PARAM;
    }

    if(mask.s.tx_xg_thold_sfifo_rd)
    {
        if(cfg->tx_xg_thold_sfifo_rd > __TX_SYNC_FIFO_THOLD_MAX)
            return CA_E_PARAM;
    }
    if(mask.s.tx_xg_thold_sfifo_wr)
    {
        if(cfg->tx_xg_thold_sfifo_wr > __TX_SYNC_FIFO_THOLD_MAX)
            return CA_E_PARAM;
    }
    if(mask.s.tx_xg_thold_asfifo_rd)
    {
        if(cfg->tx_xg_thold_asfifo_rd > __TX_ASYNC_FIFO_THOLD_MAX)
            return CA_E_PARAM;
    }
    if(mask.s.tx_xg_thold_asfifo_wr)
    {
        if(cfg->tx_xg_thold_asfifo_wr > __TX_ASYNC_FIFO_THOLD_MAX)
            return CA_E_PARAM;
    }

    if(mask.s.tx_1g_ipg)
    {
        if((cfg->tx_1g_ipg < 10) || (cfg->tx_1g_ipg > 13))
            return CA_E_PARAM;
    }

    if(mask.s.tx_1g_psize_max)
    {
        if (cfg->tx_1g_psize_max > __TX_MAX_PKT_SIZE)
            return CA_E_PARAM;
    }

    if(mask.s.tx_1g_crc8_order)
    {
        if(cfg->tx_1g_crc8_order > AAL_PON_CRC8_MODE_REV_CRC8_DATA)
            return CA_E_PARAM;
    }

    if(mask.s.onu_type)
    {
        AAL_GLB_REG_FIELD_WRITE(GLOBAL_EPON_CNTL,epon_onu_mode, cfg->onu_type);
    }

    if(mask.s.max_mtu_tag_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_RXCTRL, max_mtu_tag_en, cfg->max_mtu_tag_en);
    }

    if(mask.s.rx_1g_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_RXCTRL, RX_EN, (cfg->rx_1g_ena!=0));
    }

    if(mask.s.tx_1g_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, TX_EN, (cfg->tx_1g_ena!=0));
    }

    if(mask.s.rx_1g_psize_max)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_MAXPKTSIZE, rx_max_pkt_size, cfg->rx_1g_psize_max);
    }

    if(mask.s.tx_1g_psize_max_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, max_pkt_size_limit, (cfg->tx_1g_psize_max_ena!=0));
    }

    if(mask.s.tx_1g_psize_max)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_MAXPKTSIZE, tx_max_pkt_size, cfg->tx_1g_psize_max);
    }

    if(mask.s.tx_1g_ipg) // TODO: ipg
    {
        switch(cfg->tx_1g_ipg)
        {
        case 12:
            AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, ipg_sel, __PON_IPG_12B);
            break;
        case 11:
            AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, ipg_sel, __PON_IPG_11B);
            break;
        case 10:
            AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, ipg_sel, __PON_IPG_10B);
            break;
        case 13:
            AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, ipg_sel, __PON_IPG_13B);
            break;
        default:
            break;
        }
        //REG_FIELD_WRITE(BM_PORT_SHAPERI_CFG, w_rate, cfg->tx_ipg+8);
    }

    if(mask.s.tx_1g_8b_preamble_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, reserved_1, cfg->tx_1g_8b_preamble_ena);
    }

    if(mask.s.tx_1g_crc8_order)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, crc8_calculation_order_select, (ca_uint32_t)(cfg->tx_1g_crc8_order));
    }

    if(mask.s.tx_1g_crc32_regen_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, disable_crc32_re_generation, (cfg->tx_1g_crc32_regen_en==0));
    }

    if(mask.s.tx_1g_drain_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, drain, cfg->tx_1g_drain_ena);
    }

    return CA_E_OK;
}

ca_status_t aal_pon_mac_max_frame_size_set(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t        port_id,
    CA_IN ca_uint32_t        tx_size,
    CA_IN ca_uint32_t        rx_size,
    CA_IN ca_boolean_t       mtu_tag_en)
{
    if (1)   //(CA_PON_MODE_IS_EPON_FAMILY(aal_pon_mac_mode) || CA_PON_MODE_IS_ETHERNET(aal_pon_mac_mode)) {
    {
        aal_epon_mac_ctrl_msk_t epon_msk;
        aal_epon_mac_ctrl_t epon_cfg;

        epon_msk.u32 = 0;
        epon_msk.s.rx_xg_psize_max = 1;
        epon_msk.s.tx_1g_psize_max_ena = 1;
        epon_msk.s.rx_1g_psize_max = 1;
        epon_msk.s.tx_1g_psize_max = 1;
        epon_msk.s.max_mtu_tag_en = 1;

        memset(&epon_cfg, 0, sizeof(aal_epon_mac_ctrl_t));
        epon_cfg.rx_xg_psize_max = rx_size;
        //epon_cfg.tx_xg_psize_max = tx_size;
        epon_cfg.tx_1g_psize_max_ena = 1;
        epon_cfg.rx_1g_psize_max = rx_size;
        epon_cfg.tx_1g_psize_max = tx_size;
        epon_cfg.max_mtu_tag_en = mtu_tag_en;

        return aal_epon_mac_cfg_set(device_id, port_id, epon_msk, &epon_cfg);
    }

    return CA_E_OK;
}

void __pon_mac_intr_enable(void)
{
    ca_uint32_t top_ie = 0;
    top_ie = AAL_GLB_REG_READ(GLOBAL_PON_INTENABLE_0);
    top_ie |= GLOBAL_INTR_EN;
    AAL_GLB_REG_WRITE(top_ie, GLOBAL_PON_INTENABLE_0);
}

ca_status_t aal_pon_opt_rogue_thresh_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  at0_th,
    CA_IN ca_uint32_t  at1_th,
    CA_IN ca_uint32_t  sdlong_th)
{
    ca_status_t ret = CA_E_OK;

    if((at0_th > 0xffff) || (at1_th > 0xffff) || (sdlong_th > 0xffff))
        return CA_E_PARAM;

    AAL_GLB_REG_FIELD_WRITE(GLOBAL_OPT_MODULE_LAO_CNTL,lao_accu_zero,at0_th);
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_OPT_MODULE_LAO_CNTL,lao_accu_one,at1_th);
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_OPT_MODULE_LAO_SD_CNTL,lao_sd_long,sdlong_th);

    return ret;
}

ca_status_t aal_pon_init(ca_device_id_t device_id, ca_uint32_t mode)
{
    ca_status_t ret = CA_E_OK;

    if (1)   //CA_PON_MODE_IS_ETHERNET(mode)) {
    {
        ret = __pon_eth_init(device_id, mode);
        if (CA_E_OK != ret)
        {
            printf("__pon_eth_init failed(%d)\n", ret);
            return ret;
        }
        /*Keep TS*/
        aal_ni_xge_ts_en_set(device_id,1);
    }

    ret = aal_pon_mac_max_frame_size_set(device_id, 0, __PON_MAX_TX_FRAME_SIZE_DEFAULT, __PON_MAX_FRAME_SIZE_DEFAULT, 1);
    if (CA_E_OK != ret)
    {
        printf("aal_pon_mac_max_frame_size_set fail, return %d.\n", ret);
        return ret;
    }

    /* sometimes uboot will happen: ""Synchronous Abort" handler, esr 0x96000061",
         when do __pon_mac_intr_enable() */
    //__pon_mac_intr_enable();

    /* ret = aal_pon_intr_init(device_id);
    if (CA_E_OK != ret) {
        printf("aal_pon_intr_init fail, return %d.\r\n", ret);
        return ret;
    } */

    /* set rogue detect threshold */
    ret = aal_pon_opt_rogue_thresh_set(device_id, 0xffff, 0xffff, 0x1327);
    if (CA_E_OK != ret)
    {
        printf("aal_pon_opt_rogue_thresh_set fail, return %d.\n", ret);
        return ret;
    }

    return CA_E_OK;
}

ca_status_t aal_psds_ind_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint32_t          page,
    CA_IN ca_uint32_t          regaddr,
    CA_IN ca_uint32_t          data)
{
    PSDS_REG_ACCESS_t psds_access;
    PSDS_REG_DATAIN_t psds_datain;

    psds_datain.bf.sds_d_in = data;
    AAL_PON_REG_WRITE(psds_datain.wrd, PSDS_REG_DATAIN);

    psds_access.bf.sds_a = ((page << 5) | (regaddr));
    psds_access.bf.sds_we = 1;
    psds_access.bf.sds_access = 1;
    AAL_PON_REG_WRITE(psds_access.wrd, PSDS_REG_ACCESS);

    /* Wait at least 100ns */
    udelay(1);
    return CA_E_OK;
}

ca_status_t aal_psds_ind_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint32_t          page,
    CA_IN ca_uint32_t          regaddr,
    CA_OUT ca_uint32_t*      pData)
{
    PSDS_REG_ACCESS_t psds_access;
    PSDS_REG_DATAOUT_t psds_dataout;

    psds_access.bf.sds_a = ((page << 5) | (regaddr));
    psds_access.bf.sds_we = 0;
    psds_access.bf.sds_access = 1;
    AAL_PON_REG_WRITE(psds_access.wrd, PSDS_REG_ACCESS);

    /* Wait at least 100ns */
    udelay(1);

    psds_dataout.wrd = AAL_PON_REG_READ(PSDS_REG_DATAOUT);
    *pData = psds_dataout.bf.sds_d_out;

    return CA_E_OK;
}

//ca_status_t ca_pon_init(ca_device_id_t device_id)
ca_status_t ca_pon_init(ca_device_id_t device_id, ca_uint32_t pon_mode_in)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t pon_mode = 0;

    pon_mode = pon_mode_in;
    aal_pon_mac_mode = pon_mode;

    ret = aal_pon_init(device_id, pon_mode);
    if(CA_E_OK != ret)
    {
        printf("%s:%d aal_pon_init() returns error, ret = %d\n",__func__, __LINE__, ret);
        return ret;
    }
#if 0
    ret = __pon_wq_init(pon_mode);
    if(CA_E_OK != ret)
    {
        printf("%s:%d __pon_wq_init() returns error, ret = %d\n",__func__, __LINE__, ret);
        return ret;
    }

    ret = __pon_irq_setup(pon_mode);
    if (ret)
    {
        printf("__pon_irq_setup fail(%d)\r\n", ret);
        return ret;
    }
#endif

    printf("ca-pon: init OK.\n");

    return CA_E_OK;
}

void rtk_sds_polarity_setting(uint32 hw_config)
{
    ca_uint32_t data;

    if (hw_config & SDS_P7_INV_HSO)
    {
        if (aal_psds_ind_get(0,0,0,&data) == CA_E_OK)
            aal_psds_ind_set(0,0,0,(data|SDS_INV_HSO));
    }
    if (hw_config & SDS_P7_INV_HSI)
    {
        if (aal_psds_ind_get(0,0,0,&data) == CA_E_OK)
            aal_psds_ind_set(0,0,0,(data|SDS_INV_HSI));
    }
#if 0
    if (hw_config & SDS_P4_INV_HSO)
    {
        if (aal_sds_ind_get(0,CA_PORT_ID_NI4,0,0,&data) == CA_E_OK)
            aal_sds_ind_set(0,CA_PORT_ID_NI4,0,0,(data|SDS_INV_HSO));
    }
    if (hw_config & SDS_P4_INV_HSI)
    {
        if (aal_sds_ind_get(0,CA_PORT_ID_NI4,0,0,&data) == CA_E_OK)
            aal_sds_ind_set(0,CA_PORT_ID_NI4,0,0,(data|SDS_INV_HSI));
    }
    if (hw_config & SDS_P5_INV_HSO)
    {
        if (aal_sds_ind_get(0,CA_PORT_ID_NI5,0,0,&data) == CA_E_OK)
            aal_sds_ind_set(0,CA_PORT_ID_NI5,0,0,(data|SDS_INV_HSO));
    }
    if (hw_config & SDS_P5_INV_HSI)
    {
        if (aal_sds_ind_get(0,CA_PORT_ID_NI5,0,0,&data) == CA_E_OK)
            aal_sds_ind_set(0,CA_PORT_ID_NI5,0,0,(data|SDS_INV_HSI));
    }
    if (hw_config & SDS_P6_INV_HSO)
    {
        if (aal_sds_ind_get(0,CA_PORT_ID_NI6,0,0,&data) == CA_E_OK)
            aal_sds_ind_set(0,CA_PORT_ID_NI6,0,0,(data|SDS_INV_HSO));
    }
    if (hw_config & SDS_P6_INV_HSI)
    {
        if (aal_sds_ind_get(0,CA_PORT_ID_NI6,0,0,&data) == CA_E_OK)
            aal_sds_ind_set(0,CA_PORT_ID_NI6,0,0,(data|SDS_INV_HSI));
    }
#endif
}
#endif

