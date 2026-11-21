#include <common.h>
#include <command.h>
#include <linux/delay.h>
#include "rtl8224b.h"

#define NI_8224B_DBG	1
#undef NI_8224B_DBG

const char *rt_serdesModeStr[] = {
    "RTK_MII_NONE",
    "RTK_MII_DISABLE",
    "RTK_MII_10GR",
    "RTK_MII_RXAUI",
    "RTK_MII_RXAUI_LITE",
    "RTK_MII_RXAUISGMII_AUTO",
    "RTK_MII_RXAUI1000BX_AUTO",
    "RTK_MII_RSGMII_PLUS",
    "RTK_MII_SGMII",
    "RTK_MII_QSGMII",
    "RTK_MII_1000BX_FIBER",
    "RTK_MII_100BX_FIBER",
    "RTK_MII_1000BX100BX_AUTO",
    "RTK_MII_10GR1000BX_AUTO",
    "RTK_MII_10GRSGMII_AUTO",
    "RTK_MII_XAUI",
    "RTK_MII_RMII",
    "RTK_MII_SMII",
    "RTK_MII_SSSMII",
    "RTK_MII_RSGMII",
    "RTK_MII_XSMII",
    "RTK_MII_XSGMII",
    "RTK_MII_QHSGMII",
    "RTK_MII_HISGMII",
    "RTK_MII_HISGMII_5G",
    "RTK_MII_DUAL_HISGMII",
    "RTK_MII_2500Base_X",
    "RTK_MII_RXAUI_PLUS",
    "RTK_MII_USXGMII_10GSXGMII",
    "RTK_MII_USXGMII_10GDXGMII",
    "RTK_MII_USXGMII_10GQXGMII",
    "RTK_MII_USXGMII_5GSXGMII",
    "RTK_MII_USXGMII_5GDXGMII",
    "RTK_MII_USXGMII_2_5GSXGMII",
    "RTK_MII_USXGMII_1G",
    "RTK_MII_USXGMII_100M",
    "RTK_MII_USXGMII_10M",
    "RTK_MII_5GBASEX",
    "RTK_MII_5GR",
    "RTK_MII_XFI_5G_ADAPT",
    "RTK_MII_XFI_5G_CPRI",
    "RTK_MII_XFI_2P5G_ADAPT",
    "RTK_MII_QUSGMII",
    "RTK_MII_OUSGMII",
    "RTK_MII_END"
};

#define GET_SERDES_MODE_STR(sdsMode) ((sdsMode >= RTK_MII_NONE && sdsMode <= RTK_MII_END) ? rt_serdesModeStr[sdsMode] : "Unknown mode")

typedef struct rtk_phy_hwpatch_s
{   
    u8    patch_op;
    u8    portmask;
    u16   pagemmd;
    u16   addr;
    u8    msb;
    u8    lsb;
    u16   data;
} rtk_phy_hwpatch_t;

rtk_phy_hwpatch_t *patch_fwpr_conf;
rtk_phy_hwpatch_t *patch_fwlm_conf;
rtk_phy_hwpatch_t *patch_afe_conf;
rtk_phy_hwpatch_t *patch_top_conf;
rtk_phy_hwpatch_t *patch_sds_conf;
ca_int32_t patch_fwpr_conf_size;
ca_int32_t patch_fwlm_conf_size;
ca_int32_t patch_afe_conf_size;
ca_int32_t patch_top_conf_size;
ca_int32_t patch_sds_conf_size;

rtk_phy_hwpatch_t rtl8224_patch_fwpr_conf[] = {
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8023},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1802},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb82e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb820,  7,  7, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa016},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa012},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa014},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8010},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8013},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8026},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x802a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x802e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x802e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x802e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x802e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc114},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0a21},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x416e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x09cc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0a9b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd501},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa103},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8203},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd500},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0188},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0aa1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd501},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8103},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa203},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd500},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0188},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xac03},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa138},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1ded},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xac03},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa138},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1cb8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa026},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa024},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa022},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa020},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa006},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1cb7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa004},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1dec},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa002},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x017f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0a20},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa008},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0f00},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa016},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0020},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa012},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa014},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8010},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x801f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8022},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8022},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8022},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8022},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8022},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8022},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa301},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0b55},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f9f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd131},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd047},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0b55},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5edf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f74},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x032a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8280},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0504},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa10e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa10c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa10a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa108},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa106},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa104},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa102},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0503},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa100},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0325},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa110},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0003},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb820,  7,  7, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x847d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf84},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x95af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x849a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf84},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa0af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x84ae},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf84},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xdbaf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x84fa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf84},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfaaf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x84fa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2901},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf16},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5802},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x84fa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf16},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2fee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8ffe},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8fff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffbf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x161c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf16},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00e2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xad30},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x21e2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8701},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe387},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x021b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x549f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x17ef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x54e1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8018},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0d14},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf85},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8402},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x55a4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0d11},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf85},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8702},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x55a4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef45},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe281},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc2af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4056},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe187},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00ad},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2813},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe180},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x180d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x16bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8584},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0255},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa40d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x11bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8587},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0255},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa402},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x3b96},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf3a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfc02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8501},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0285},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5904},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf8f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfafb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef79},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfbd7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd500},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00a2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0005},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf57},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x61ae},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x13a2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0105},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf57},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x64ae},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0ba2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0205},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf57},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x67ae},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x03bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x576a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0255},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc3ef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6402},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5b12},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x13e1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8ffb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1b13},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x9fd5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe083},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x7380},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1b02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x9f0a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef47},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe48f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfce5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8ffd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xae06},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2a01},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd300},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaebd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x97ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfefd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfc04},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf8f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfafb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe28f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfce3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8ffd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef65},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe08f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfee1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8fff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef74},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x025a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf7ad},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x500c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe68f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfee7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8fff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe283},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4ee6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x834f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfffe},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfdfc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x04cc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc010},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb818},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x164d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb81a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x162c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb81c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x15fd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb81e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4053},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb850},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x3af9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb852},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb878},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb884},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb832},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x001f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb896,  0,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb892, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc206},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  8, 0x00b1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb896,  0,  0, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb82e,  0,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8023},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
};

rtk_phy_hwpatch_t rtl8224_patch_fwlm_conf[] = {
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0032},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0032},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x001d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x002d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0009},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0048},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0048},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0008},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0004},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ba},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81bb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0020},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81bc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0004},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81bd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0020},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81be},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x001a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00e0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x003a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x001c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0060},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0011},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00cf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00b0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ca},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81cb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81cc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81cd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ad},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ce},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81cf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00be},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0018},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0008},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0070},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0037},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0048},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81da},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81db},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00f4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81dc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00eb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81dd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00a0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81de},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81df},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x002b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x009c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00b2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00eb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0002},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0092},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00fe},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00e4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ea},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81eb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x003c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ec},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0006},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ed},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00f2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x003a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x008a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00d0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0009},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00dc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0030},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81fa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8701},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0004},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8018},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0070},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0005},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x001c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81fb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x006c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8702},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0050},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8ffb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80dc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8378},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0022},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8217},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x001e},
};

rtk_phy_hwpatch_t rtl8224_patch_afe_conf[] = {
    {RTK_HWPATCH_OP_PHY, 0x02 ,0x00, 0xbf84,  2,  0, 0x0004},
    {RTK_HWPATCH_OP_PHY, 0x02 ,0x00, 0xbf8c, 10,  6, 0x0000},
};

rtk_phy_hwpatch_t rtl8224_patch_sds_conf[] = {
    {RTK_HWPATCH_OP_MMD, 0x01 ,0x1e, 0x7b20,  4,  0, 0x001f},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x06, 0x0012, 15,  0, 0x00a4},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x07, 0x0010,  7,  0, 0x0003},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x06, 0x0003, 15, 15, 0x0001},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x06, 0x001d,  9,  9, 0x0001},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x06, 0x001f, 13, 13, 0x0001},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x0010, 15,  0, 0x4480},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x0013, 15,  0, 0x0400},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x0018, 15,  0, 0x6d02},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x001b, 15,  0, 0x424e},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x001d, 15,  0, 0x0002},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x36, 0x001c, 15,  0, 0x1390},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0004, 15,  0, 0x0080},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0006, 15,  0, 0x0408},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0007, 15,  0, 0x020d},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0009, 15,  0, 0x0601},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x000b, 15,  0, 0x222c},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x000c, 15,  0, 0xa217},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x000d, 15,  0, 0xfe40},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0015, 15,  0, 0xf5f1},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0016, 15,  0, 0x0443},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x001d, 15,  0, 0xabb0},
    {RTK_HWPATCH_OP_MMD, 0x01 ,0x1e, 0x7b20, 14, 10, 0x0002},
    {RTK_HWPATCH_OP_MMD, 0x01 ,0x1e, 0x7b20,  4,  0, 0x000d},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0003},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0001},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0003},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0000},
};

rtk_phy_hwpatch_t rtl8224_patch_top_conf[] = {
};


rtk_phy_hwpatch_t rtl8224_b_patch_fwpr_conf[] = {
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8023},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1801},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb82e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb820,  7,  7, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa016},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa012},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa014},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8010},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8016},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8019},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x802c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8030},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8034},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8034},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8034},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8808},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd711},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x404f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa808},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1dff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc114},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0a21},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x416e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x09cc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0a9b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd501},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa103},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8203},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd500},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0188},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0aa1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd501},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8103},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa203},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd500},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0188},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xac03},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa138},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1ded},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xac03},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa138},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1cb8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa026},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa024},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa022},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa020},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1cb7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa006},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1dec},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa004},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x017f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa002},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0a20},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1dfe},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa008},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1f00},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa016},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0020},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa012},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa014},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8010},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8027},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8036},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8036},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8036},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8036},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8036},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8036},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa710},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa00a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8190},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8204},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8280},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa404},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd130},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd047},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0b46},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5fb4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa110},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa204},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa280},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd130},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd047},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0b46},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5fb4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0505},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa301},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0b46},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f9f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd131},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd047},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0b46},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5edf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f74},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x032a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa10e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa10c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa10a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa108},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa106},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa104},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa102},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0325},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa100},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x04ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xa110},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0003},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb87c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x858b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb87e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf85},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa3af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8660},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf86},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x85af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8690},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf87},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x29af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x87c8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf88},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x07af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x884a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe18f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xff39},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x019e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x64e0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8397},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe182},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x001b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x019f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x07ee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x838c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x05af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x865d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd10f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0e02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f0a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf61},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf902},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f29},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe583},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8fbf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x620b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0265},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf9bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6211},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0265},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf9bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6214},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0265},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf0d1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x03bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x61f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x025f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0ae1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8201},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1702},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f0a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1a02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1a02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1d02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1d02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xee83},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8c02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf86},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5dd1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0fbf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x620e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x025f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0abf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x61f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x025f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x29e5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x838f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0b02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1102},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1402},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd103},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf61},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf902},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f0a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd140},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1702},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f0a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1a02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1a02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1d02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1d02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xee83},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8c02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf50},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5be1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8fff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa101},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0bee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x838c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x07ee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8fff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8682},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe183},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x94a1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0107},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xee83},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8c05},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf86},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x82ee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x838c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x03af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x50a4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xee8f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xff01},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xee83},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8c01},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf52},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x99ee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8029},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0eee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8ffe},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00ef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x021b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x03d1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x001b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x10ac},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2d0f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa200},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x06d7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x000e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf86},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xddd7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0011},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf86},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xdda0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x010f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa201},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x06d7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0012},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf86},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xddd7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0015},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf86},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xdda0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x020f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa202},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x06d7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0017},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf86},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xddd7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x001b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf86},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xddee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8ffe},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x01af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4451},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0244},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5fe0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8028},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xac20},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x09bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6259},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x025f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x29ac},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x281f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf61},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe702},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f29},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef21},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf61},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xea02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f29},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef31},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1b03},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa0ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x10a2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0008},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd170},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf44},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4eaf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x444b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd15f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf44},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4ea0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfe0d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa200},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x05d1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x59af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x444e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd14c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf44},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4eaf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4451},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5c07},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xff1c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x47ac},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2704},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0c41},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xae03},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd4ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffbf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x615a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x025f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0abf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x614e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0265},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf9bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x614e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0265},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf0bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x61ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x025f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x290d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x455c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x07ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1c47},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xac27},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x040c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x41ae},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x03d4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf61},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5a02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f0a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf61},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5102},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf61},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5102},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf62},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0202},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f29},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0d45},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5c07},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xff1c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x47ac},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2704},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0c41},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xae03},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd4ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffbf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x615a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x025f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0abf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6154},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0265},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf9bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6154},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0265},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf0bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6205},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x025f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x290d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x455c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x07ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1c47},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xac27},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x040c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x41ae},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x03d4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf61},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5a02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5f0a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf61},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5702},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf61},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5702},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x65f0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf44},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfca3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0008},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd646},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x9ed2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4567},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa301},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x13a2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0008},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd638},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x19d2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4567},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd63b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe6d2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x01af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4567},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa302},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x13a2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0008},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd62c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8fd2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4567},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd62f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x94d2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x01af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4567},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd625},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xcbd2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x01af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4567},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xac30},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x06bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x83c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf88},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x13bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x83c2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef13},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0c16},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1f00},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1a94},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd819},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd9ef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x74bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x83e2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef13},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0c16},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1f00},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1a94},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd819},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd9ef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x64ef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x03d3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0c02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6602},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0c74},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef30},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xac30},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x06d6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x91b1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf45},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc8d6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x886f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf45},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb85e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4ffb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb860},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5094},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb862},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5295},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb864},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4404},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb886},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x446c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb888},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4530},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb88a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x458a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb88c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb838},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x007f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb820,  7,  7, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x847d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf84},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x95af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x849a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf84},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xa0af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x84ae},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf84},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xdbaf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x84fa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf84},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfaaf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x84fa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2901},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf16},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5802},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x84fa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf16},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2fee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8ffe},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8fff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffbf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x161c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf16},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00e2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xad30},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x21e2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8701},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe387},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x021b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x549f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x17ef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x54e1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8018},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0d14},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf85},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8402},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5598},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0d11},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf85},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8702},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5598},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef45},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe281},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc2af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4056},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe187},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00ad},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2813},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe180},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x180d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x16bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8584},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0255},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x980d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x11bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8587},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0255},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x9802},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x3b96},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaf3a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfc02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8501},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0285},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5904},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf8f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfafb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef79},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfbd7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd500},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00a2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0005},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf57},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x55ae},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x13a2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0105},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf57},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x58ae},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0ba2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0205},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbf57},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5bae},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x03bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x575e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0255},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xb7ef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6402},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x5b00},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x13e1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8ffb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1b13},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x9fd5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe083},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x7380},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1b02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x9f0a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef47},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe48f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfce5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8ffd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xae06},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2a01},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd300},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xaebd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xffef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x97ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfefd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfc04},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf8f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfafb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe28f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfce3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8ffd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef65},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe08f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfee1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8fff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xef74},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x025a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe5ad},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x500c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe68f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfee7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x8fff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xe283},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4ee6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x834f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfffe},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfdfc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x04cc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc010},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x00c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb818},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x164d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb81a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x162c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb81c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x15fd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb81e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x4053},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb850},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x3af9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb852},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb878},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb884},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb832},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x001f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb896,  0,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb892, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc202},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xfa02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc204},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x4d02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc206},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xb103},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc208},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x3003},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc20a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xb504},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc20c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x3a04},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc20e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xe705},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc210},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xac06},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc212},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x9407},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc214},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x9808},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc216},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xd70a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc218},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5c0c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc21a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5e0e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc21c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x0110},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc21e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5ef6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc220},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xf6f5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc222},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xf3f3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc224},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xefeb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc226},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xe7e4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc228},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xe2df},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc22a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xdddb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc22c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xdad9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc270},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x1819},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc272},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x1c1e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc274},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2021},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc276},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2223},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc278},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2324},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc27a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2525},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc27c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2626},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc27e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x264a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc280},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x4a4a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc282},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x4d4d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc284},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5055},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc286},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5252},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc288},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5252},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc28a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5251},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc28c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5252},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc201},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890,  7,  0, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc22e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  8, 0x00d7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc26f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890,  7,  0, 0x0018},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc28e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  8, 0x0052},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb896,  0,  0, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8217},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x001e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb82e,  0,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8023},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
};

rtk_phy_hwpatch_t rtl8224_b_patch_fwlm_conf[] = {
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8017},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0007},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8066},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x0010},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8067},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x0040},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8068},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x000a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8069},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x0040},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x806e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x0002},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x806f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x00a0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8084},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x0010},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8085},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x0040},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8086},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x000a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8087},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x0040},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x808c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x0002},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x808d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x00a0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8029},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  8, 0x000e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80b5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0095},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80b6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00cb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80b7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00f6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80b8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0004},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80b9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00d2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80ba},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0006},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80bb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00f7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80bc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00bc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80bd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x004b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80be},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0019},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x002f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80c9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00a0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80ca},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00cb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80cb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00f6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80cc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0004},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80cd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x009e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80ce},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0006},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80cf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00f5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80d0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00c4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80d1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x004c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80d2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0019},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80d3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x002f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80d4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa87e, 12,  8, 0x0006},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80a1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0048},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80a2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x009c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80a3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00b6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80a4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0003},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80a5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00da},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80a6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80a7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ea},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80a8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00a6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80a9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00e3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80aa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x001f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80ab},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x003b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80ac},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x001e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0032},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0032},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x001d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x002d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0009},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0048},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0048},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0008},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0004},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ba},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81bb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0020},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81bc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0004},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81bd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0020},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81be},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x001a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00e0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x003a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x001c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0060},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0011},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00cf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81c9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00b0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ca},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81cb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81cc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81cd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ad},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ce},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81cf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00be},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0018},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0008},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0070},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0037},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81d9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0048},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81da},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81db},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00f4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81dc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00eb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81dd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00a0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81de},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81df},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x002b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x009c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00b2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00eb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00af},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0002},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0092},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00fe},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81e9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00e4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ea},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81eb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x003c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ec},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0006},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ed},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00f2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ee},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81ef},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x003a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x008a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00d0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0009},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00dc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00ff},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81f9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0030},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81fa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8700},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8701},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0004},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8018},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0070},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x00c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81a9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0005},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81b3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x001c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x81fb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x006c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8702},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0050},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa86a,  0,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8ffb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80dc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x000a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8378},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  8, 0x0022},
};

rtk_phy_hwpatch_t rtl8224_b_patch_afe_conf[] = {
    {RTK_HWPATCH_OP_PHY, 0x02 ,0x00, 0xbf84,  2,  0, 0x0004},
    {RTK_HWPATCH_OP_PHY, 0x02 ,0x00, 0xbf8c, 10,  6, 0x0000},
};

rtk_phy_hwpatch_t rtl8224_b_patch_sds_conf[] = {
    {RTK_HWPATCH_OP_MMD, 0x01 ,0x1e, 0x7b20,  4,  0, 0x001f},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x06, 0x0012, 15,  0, 0x00a4},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x07, 0x0010,  7,  0, 0x0003},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x06, 0x0003, 15, 15, 0x0001},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x06, 0x001d,  9,  9, 0x0001},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x06, 0x001f, 13, 13, 0x0001},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x0010, 15,  0, 0x4480},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x0013, 15,  0, 0x0400},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x0018, 15,  0, 0x6d02},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x001b, 15,  0, 0x424e},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x001d, 15,  0, 0x0002},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x36, 0x001c, 15,  0, 0x1390},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0004, 15,  0, 0x0080},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0006, 15,  0, 0x0408},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0007, 15,  0, 0x020d},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0009, 15,  0, 0x0601},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x000b, 15,  0, 0x222c},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x000c, 15,  0, 0xa217},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x000d, 15,  0, 0xfe40},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0015, 15,  0, 0xf5f1},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0016, 15,  0, 0x0443},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x001d, 15,  0, 0xabb0},
    {RTK_HWPATCH_OP_MMD, 0x01 ,0x1e, 0x7b20, 14, 10, 0x0002},
    {RTK_HWPATCH_OP_MMD, 0x01 ,0x1e, 0x7b20,  4,  0, 0x000d},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0003},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0001},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0003},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0000},
};

rtk_phy_hwpatch_t rtl8224_b_patch_top_conf[] = {
};


rtk_phy_hwpatch_t rtl8224_a_patch_fwpr_conf[] = {
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8023},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x1800},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0xb82e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb820, 15,  0, 0x0090},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb820, 15,  0, 0x0010},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb896, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb892, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc202},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xc101},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc204},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xff02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc206},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x6a02},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc208},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xd903},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc20a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5c03},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc20c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xf604},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc20e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x8805},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc210},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x4006},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc212},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x3d07},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc214},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2408},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc216},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2b09},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc218},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x840b},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc21a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x410c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc21c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x440e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc21e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x00f6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc220},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xf6f5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc222},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xf3f3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc224},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xefeb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc226},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xe7e4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc228},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xe2df},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc22a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xdddb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc22c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0xdad9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc270},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x1819},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc272},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x1c1e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc274},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2021},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc276},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2223},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc278},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2324},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc27a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2525},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc27c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x2626},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc27e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x264a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc280},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x4a4a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc282},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x4d4d},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc284},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5055},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc286},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5252},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc288},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5252},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc28a},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5251},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc28c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  0, 0x5252},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc201},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890,  7,  0, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc22e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  8, 0x00d7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc26f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890,  7,  0, 0x0018},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb88e, 15,  0, 0xc28e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb890, 15,  8, 0x0052},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb896, 15,  0, 0x0001},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb82e, 15,  0, 0x0000},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x8023},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0000},
};

rtk_phy_hwpatch_t rtl8224_a_patch_fwlm_conf[] = {
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80b5},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x98c3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80b6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc3f6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80b7},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf604},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80b8},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x04d2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80b9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xd206},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80ba},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x06fa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80bb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfabc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80bc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbc66},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80bd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6619},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80be},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x192f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80bf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2f0c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80c0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0c23},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80c9},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc0bb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80ca},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xbbf6},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80cb},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xf604},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80cc},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x049e},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80cd},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x9e06},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80ce},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x06fa},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80cf},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xfac4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80d0},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0xc463},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80d1},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x6319},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80d2},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x192f},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80d3},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x2f0c},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa436, 15,  0, 0x80d4},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xa438, 15,  0, 0x0c23},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87c, 15,  0, 0x8012},
    {RTK_HWPATCH_OP_PHY, 0xff ,0x00, 0xb87e, 15,  0, 0xee47},
};

rtk_phy_hwpatch_t rtl8224_a_patch_afe_conf[] = {
    {RTK_HWPATCH_OP_PHY, 0x02 ,0x00, 0xbf84,  2,  0, 0x0007},
    {RTK_HWPATCH_OP_PHY, 0x02 ,0x00, 0xbf8c, 10,  6, 0x0000},
};

rtk_phy_hwpatch_t rtl8224_a_patch_sds_conf[] = {
    {RTK_HWPATCH_OP_MMD, 0x01 ,0x1e, 0x7b20,  4,  0, 0x001f},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x06, 0x0012, 15,  0, 0x5078},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0004, 15,  0, 0x0080},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0016, 15,  0, 0x4743},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x36, 0x001c, 15,  0, 0x1390},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x000b, 15,  0, 0x232c},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x0018, 15,  0, 0x6c02},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x21, 0x0018, 15,  0, 0x6d02},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x001d, 15,  0, 0xabb0},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x2e, 0x0009, 10, 10, 0x0001},
    {RTK_HWPATCH_OP_MMD, 0x01 ,0x1e, 0x7b20, 14, 10, 0x0002},
    {RTK_HWPATCH_OP_MMD, 0x01 ,0x1e, 0x7b20,  4,  0, 0x000d},
    {RTK_HWPATCH_OP_MMD, 0x01 ,0x1e, 0x0ac8,  9,  9, 0x0001},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0003},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0001},
    {RTK_HWPATCH_OP_SDS, 0x01 ,0x20, 0x0000,  5,  4, 0x0003},
};

rtk_phy_hwpatch_t rtl8224_a_patch_top_conf[] = {
};

ca_uint32_t
_phy_8224_mask(u8 msb, u8 lsb)
{
    ca_uint16_t val = 0;
    ca_uint8_t i = 0;

    for (i = lsb; i <= msb; i++)
    {
        val |= (1 << i);
    }
    return val;
}

ca_int32_t
_phy_8224_mask_get(u8 msb, u8 lsb, u32 *mask)
{
    if ((msb > 31) || (lsb > 31) || (msb < lsb))
    {
        return CA_E_PARAM;
    }
    *mask = _phy_8224_mask(msb, lsb);

    return CA_E_OK;
}

ca_uint32_t
_phy_rtl8224_patch_mask(u8 msb, u8 lsb)
{
    ca_uint32_t val = 0;
    ca_uint8_t i = 0;

    for (i = lsb; i <= msb; i++)
    {
        val |= (1 << i);
    }
    return val;
}

ca_int32_t
_phy_rtl8224_patch_mask_get(u8 msb, u8 lsb, u32 *mask)
{
    if ((msb > 15) || (lsb > 15) || (msb < lsb))
    {
        return CA_E_PARAM;
    }
    *mask = _phy_rtl8224_patch_mask(msb, lsb);
    return CA_E_OK;
}

static ca_int32_t _phy_8224_intReg_get(u8 phy_addr, u32 reg_addr, u8 msb, u8 lsb, u32 *pData)
{
    ca_status_t ret;
    ca_uint32_t addr;
    ca_uint16_t l_data = 0;
    ca_uint16_t h_data = 0;
    ca_uint32_t reg_data = 0;
    ca_uint32_t mask = 0;

    if ((msb > 31) || (lsb > 31) || (msb < lsb))
    {
        return CA_E_PARAM;
    }

    /* Reg [15:0]*/
    addr = RTL8224_SWREG_ADDR(reg_addr);
	ret = ca_mdio_read(phy_addr, 0, PHY_MMD_VEND1, addr, &l_data);
    if(ret != CA_E_OK)
        return ret;

    /* Reg [31:16]*/
	ret = ca_mdio_read(phy_addr, 0, PHY_MMD_VEND1, (addr+1), &h_data);
    if(ret != CA_E_OK)
        return ret;

    ret = _phy_8224_mask_get(msb, lsb, &mask);
    if(ret != CA_E_OK)
        return ret;

    reg_data = ((h_data << 16) | (l_data));
    *pData = (((reg_data) & (mask)) >> lsb);

    return ret;
}

static ca_int32_t _phy_8224_intReg_set(u8 phy_addr, u32 reg_addr, u8 msb, u8 lsb, u32 data)
{
    ca_status_t ret;
    ca_uint32_t addr;
    ca_uint16_t l_data = 0;
    ca_uint16_t h_data = 0;
    ca_uint32_t reg_data = 0;
    ca_uint32_t mask = 0;

    if ((msb > 31) || (lsb > 31) || (msb < lsb))
    {
        return CA_E_PARAM;
    }

    /* Reg [15:0]*/
    addr = RTL8224_SWREG_ADDR(reg_addr);
	ret = ca_mdio_read(phy_addr, 0, PHY_MMD_VEND1, addr, &l_data);
    if(ret != CA_E_OK)
        return ret;

    /* Reg [31:16]*/
	ret = ca_mdio_read(phy_addr, 0, PHY_MMD_VEND1, (addr+1), &h_data);
    if(ret != CA_E_OK)
        return ret;

    ret = _phy_8224_mask_get(msb, lsb, &mask);
    if(ret != CA_E_OK)
        return ret;

    reg_data = ((h_data << 16) | (l_data));
    reg_data &= ~(mask);
    reg_data |= (((data) << lsb) & mask);

    l_data = (reg_data & 0xffff);
    h_data = ((reg_data >> 16)& 0xffff);

	ret = ca_mdio_write(phy_addr, 0, PHY_MMD_VEND1, addr, l_data);
    if(ret != CA_E_OK)
        return ret;

	ret = ca_mdio_write(phy_addr, 0, PHY_MMD_VEND1, (addr+1), h_data);
    if(ret != CA_E_OK)
        return ret;

    return CA_E_OK;
}

ca_int32_t
phy_8224_chipVer_get(u8 phy_addr, u32 *pVer)
{
    ca_status_t ret;
    ca_uint32_t regData;

    if ((ret = _phy_8224_intReg_set(phy_addr, 0xbb00000c, 19, 16, 0xa)) != CA_E_OK)
        return ret;

    if ((ret = _phy_8224_intReg_get(phy_addr, 0xbb00000c, 31, 28, &regData)) != CA_E_OK)
        return ret;

    *pVer = regData;

    if ((ret = _phy_8224_intReg_set(phy_addr, 0xbb00000c, 19, 16, 0x0)) != CA_E_OK)
        return ret;

    return CA_E_OK;
}

_phy_rtl8224_patch_assign(u8 phy_addr, u32 chipVer)
{
    if(chipVer == PHY_RTL8224_VER_A)
    {
        patch_fwpr_conf = rtl8224_a_patch_fwpr_conf;
        patch_fwlm_conf = rtl8224_a_patch_fwlm_conf;
        patch_afe_conf = rtl8224_a_patch_afe_conf;
        patch_top_conf = rtl8224_a_patch_top_conf;
        patch_sds_conf = rtl8224_a_patch_sds_conf;
        patch_fwpr_conf_size = sizeof(rtl8224_a_patch_fwpr_conf);
        patch_fwlm_conf_size= sizeof(rtl8224_a_patch_fwlm_conf);
        patch_afe_conf_size= sizeof(rtl8224_a_patch_afe_conf);
        patch_top_conf_size= sizeof(rtl8224_a_patch_top_conf);
        patch_sds_conf_size= sizeof(rtl8224_a_patch_sds_conf);
    }
    else if(chipVer == PHY_RTL8224_VER_B)
    {
        patch_fwpr_conf = rtl8224_b_patch_fwpr_conf;
        patch_fwlm_conf = rtl8224_b_patch_fwlm_conf;
        patch_afe_conf = rtl8224_b_patch_afe_conf;
        patch_top_conf = rtl8224_b_patch_top_conf;
        patch_sds_conf = rtl8224_b_patch_sds_conf;
        patch_fwpr_conf_size = sizeof(rtl8224_b_patch_fwpr_conf);
        patch_fwlm_conf_size= sizeof(rtl8224_b_patch_fwlm_conf);
        patch_afe_conf_size= sizeof(rtl8224_b_patch_afe_conf);
        patch_top_conf_size= sizeof(rtl8224_b_patch_top_conf);
        patch_sds_conf_size= sizeof(rtl8224_b_patch_sds_conf);
    }else{
        patch_fwpr_conf = rtl8224_patch_fwpr_conf;
        patch_fwlm_conf = rtl8224_patch_fwlm_conf;
        patch_afe_conf = rtl8224_patch_afe_conf;
        patch_top_conf = rtl8224_patch_top_conf;
        patch_sds_conf = rtl8224_patch_sds_conf;
        patch_fwpr_conf_size = sizeof(rtl8224_patch_fwpr_conf);
        patch_fwlm_conf_size= sizeof(rtl8224_patch_fwlm_conf);
        patch_afe_conf_size= sizeof(rtl8224_patch_afe_conf);
        patch_top_conf_size= sizeof(rtl8224_patch_top_conf);
        patch_sds_conf_size= sizeof(rtl8224_patch_sds_conf);
    }
    return CA_E_OK;
}

ca_int32_t
_phy_rtl8224_patch_top_get(u8 phy_addr, u32 topPage, u32 topReg, u32 *pData)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t rData = 0;

    if ((ret = ca_mdio_read(phy_addr, 0, PHY_MMD_VEND1, topReg, &rData)) != CA_E_OK)
        return ret;
    *pData = rData;

    return CA_E_OK;
}

ca_int32_t
_phy_rtl8224_patch_top_set(u8 phy_addr, u32 topPage, u32 topReg, u32 wData)
{
    ca_status_t ret = CA_E_OK;

    if ((ret = ca_mdio_write(phy_addr, 0, PHY_MMD_VEND1, topReg, wData)) != CA_E_OK)
        return ret;

    return CA_E_OK;
}

ca_int32_t
_phy_rtl8224_patch_wait(u8 phy_addr, u32 mmdAddr, u32 mmdReg, u16 data, u16 mask, u8 bcast_op)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t rData = 0;
	ca_uint32_t loop_wait = 1000;

	do {
		if ((ret = ca_mdio_read(phy_addr, 0, mmdAddr, mmdReg, &rData)) != CA_E_OK)
			return CA_E_ERROR;

		if ((rData & mask) == data)
            break;

		mdelay(1);
	} while(--loop_wait);
	if (loop_wait == 0) {
		printf("%s: CA_E_TIMEOUT!!\n", __func__);
		return CA_E_TIMEOUT;
	}

    return CA_E_OK;
}

ca_int32_t
_phy_rtl8224_patch_wait_not_equal(u8 phy_addr, u32 mmdAddr, u32 mmdReg, u32 data, u32 mask, u8 bcast_op)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t rData = 0;
	ca_uint32_t loop_wait = 1000;

	do {
		if ((ret = ca_mdio_read(phy_addr, 0, mmdAddr, mmdReg, &rData)) != CA_E_OK)
			return CA_E_ERROR;

		if ((rData & mask) != data)
            break;

		mdelay(1);
	} while(--loop_wait);
	if (loop_wait == 0) {
		printf("%s: CA_E_TIMEOUT!!\n", __func__);
		return CA_E_TIMEOUT;
	}

    return CA_E_OK;
}

#if 1
ca_int32_t
_phy_rtl8224_patch_sds_get(u8 phy_addr, u32 sdsPage, u32 sdsReg, u32 *pData)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t rData = 0;
    ca_uint32_t sdsAddr = (0x1 << 15) | (0x0 << 14) | (sdsReg << 7) | (sdsPage << 1);

    if ((ret = _phy_rtl8224_patch_top_set(phy_addr, 0, 0x3F8, sdsAddr)) != CA_E_OK)
        return ret;

    if ((ret = _phy_rtl8224_patch_top_get(phy_addr, 0, 0x3FC, &rData)) != CA_E_OK)
        return ret;
    *pData = rData;

    return _phy_rtl8224_patch_wait(phy_addr, PHY_MMD_VEND1, 0x3F8, 0, BIT_15, PHY_PATCH_OP_NORMAL);
}

ca_int32_t
_phy_rtl8224_patch_sds_set(u8 phy_addr, u32 sdsPage, u32 sdsReg, u32 wData, u8 bcast)
{
    ca_status_t ret = CA_E_OK;
#if 0
    ca_uint32_t sdsAddr = (0x1 << 15) | (0x1 << 14) | (sdsReg << 7) | (sdsPage << 1);
#endif

#if 1
	return ret;
#else
    if ((ret = _phy_rtl8224_patch_top_set(phy_addr, 0, 0x400, wData)) != CA_E_OK)
        return ret;
    if ((ret = _phy_rtl8224_patch_top_set(phy_addr, 0, 0x3F8, sdsAddr)) != CA_E_OK)
        return ret;

    return _phy_rtl8224_patch_wait(phy_addr, PHY_MMD_VEND1, 0x3F8, 0, BIT_15, bcast);
#endif
}
#else
ca_int32_t
_phy_rtl8224_patch_sds_get(u8 phy_addr, u32 sdsPage, u32 sdsReg, u32 *pData)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t rData = 0;
    ca_uint16_t sdsAddr = (0x1 << 15) | (0x0 << 14) | (sdsReg << 7) | (sdsPage << 1);

    if ((ret = ca_mdio_write(phy_addr, 0, 0, 0x3F8, sdsAddr)) != CA_E_OK)
        return ret;
    if ((ret = ca_mdio_read(phy_addr, 0, 0, 0x3FC, &rData)) != CA_E_OK)
        return ret;
    *pData = rData;

    return _phy_rtl8224_patch_wait(phy_addr, PHY_MMD_VEND1, 0x3F8, 0, BIT_15, PHY_PATCH_OP_NORMAL);
}

ca_int32_t
_phy_rtl8224_patch_sds_set(u8 phy_addr, u32 sdsPage, u32 sdsReg, u32 wData, u8 bcast)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t sdsAddr = (0x1 << 15) | (0x1 << 14) | (sdsReg << 7) | (sdsPage << 1);

    if ((ret = ca_mdio_write(phy_addr, 0, 0, 0x400, wData)) != CA_E_OK)
        return ret;
    if ((ret = ca_mdio_read(phy_addr, 0, 0, 0x3F8, sdsAddr)) != CA_E_OK)
        return ret;

    return _phy_rtl8224_patch_wait(phy_addr, PHY_MMD_VEND1, 0x3F8, 0, BIT_15, bcast);
}
#endif

ca_int32_t
phy_rtl8224_sdsReg_get(u8 phy_addr, u32 sdsPage, u32 sdsReg, u32 *pData)
{
    _phy_rtl8224_patch_sds_get(phy_addr, sdsPage, sdsReg, pData);

    return CA_E_OK;
}

ca_int32_t
phy_rtl8224_sdsReg_set(u8 phy_addr, u32 sdsPage, u32 sdsReg, u32 wData)
{
    return _phy_rtl8224_patch_sds_set(phy_addr, sdsPage, sdsReg, wData, 0);
}

ca_int32_t
phy_rtl8224_patch_process_op(u8 phy_addr, rtk_phy_hwpatch_t *op, u8 bcast)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t mask = 0;
    ca_uint16_t rData = 0;
    ca_uint16_t wData = 0;

    ret = _phy_rtl8224_patch_mask_get(op->msb, op->lsb, &mask);

    if (ret != CA_E_OK)
        return ret;

    switch (op->patch_op)
    {
        case RTK_HWPATCH_OP_PHY:
            if ((op->msb != 15) || (op->lsb != 0))
            {
                if ((ret = ca_mdio_read(phy_addr, 0, PHY_MMD_VEND2, op->addr, &rData)) != CA_E_OK)
                {
                    printf("\n[%s][phy_addr = %d] addr = 0x%08x ERROR!!!!!\n",__FUNCTION__, phy_addr, op->addr);
                    return ret;
                }
            }
            wData = REG32_FIELD_SET(rData, op->data, op->lsb, mask);			
            if ((ret = ca_mdio_write(phy_addr, 0, PHY_MMD_VEND2, op->addr, wData)) != CA_E_OK)
                return ret;

            break;

        case RTK_HWPATCH_OP_MMD:
            if ((op->msb != 15) || (op->lsb != 0))
            {
            	if ((ret = ca_mdio_read(phy_addr, 0, PHY_MMD_VEND1, op->addr, &rData)) != CA_E_OK)
                    return ret;
            }
            wData = REG32_FIELD_SET(rData, op->data, op->lsb, mask);
			if ((ret = ca_mdio_write(phy_addr, 0, PHY_MMD_VEND1, op->addr, wData)) != CA_E_OK)
                return ret;

            break;

        case RTK_HWPATCH_OP_TOP:
            if ((op->msb != 15) || (op->lsb != 0))
            {
                if ((ret = _phy_rtl8224_patch_top_get(phy_addr, PHY_MMD_VEND1, op->addr, &rData)) != CA_E_OK)
                    return ret;
            }
            wData = REG32_FIELD_SET(rData, op->data, op->lsb, mask);

            if ((ret = _phy_rtl8224_patch_top_set(phy_addr, PHY_MMD_VEND1, op->addr, wData)) != CA_E_OK)
                return ret;

            break;

        case RTK_HWPATCH_OP_SDS:
            if ((op->msb != 15) || (op->lsb != 0))
            {
                if ((ret = _phy_rtl8224_patch_sds_get(phy_addr, op->pagemmd, op->addr, &rData)) != CA_E_OK)
                    return ret;
            }
            wData = REG32_FIELD_SET(rData, op->data, op->lsb, mask);

            if ((ret = _phy_rtl8224_patch_sds_set(phy_addr, op->pagemmd, op->addr, wData, bcast)) != CA_E_OK)
                return ret;

            break;

        case RTK_HWPATCH_OP_PHYW:
        case RTK_HWPATCH_OP_ALGO:
        case RTK_HWPATCH_OP_DATARAM:
        case RTK_HWPATCH_OP_UNKNOWN:
        default:
            return CA_E_NOT_FOUND;
    }

    return CA_E_OK;
}

ca_int32_t
phy_rtl8224_patch_op(u8 phy_addr, u8 patch_op, u8 portmask, u16 pagemmd, u16 addr, u8 msb, u8 lsb, u16 data)
{
    rtk_phy_hwpatch_t op;

    op.patch_op = patch_op;
    op.portmask = portmask;
    op.pagemmd  = pagemmd;
    op.addr     = addr;
    op.msb      = msb;
    op.lsb      = lsb;
    op.data     = data;

    return phy_rtl8224_patch_process_op(phy_addr, &op, PHY_PATCH_OP_NORMAL);
}

ca_int32_t
_phy_rtl8224_patch_process(u8 phy_addr, rtk_phy_hwpatch_t *pPatch, int size, u32 *cnt, u8 bcast)
{
    ca_int32_t ret = CA_E_OK;
    ca_int32_t i = 0;
    ca_int32_t n = 0;
    rtk_phy_hwpatch_t *patch = pPatch;

    if (size <= 0)
    {
        *cnt = 0;
        return CA_E_OK;
    }
    n = size/sizeof(rtk_phy_hwpatch_t);

    for (i = 0; i < n; i++)
    {
        ret = phy_rtl8224_patch_process_op(phy_addr, &patch[i], bcast);
        if ((ret != CA_E_PARAM) && (ret != CA_E_OK))
        {
            printf("phy_addr %d %s failed! i=%u ret=0x%X\n", phy_addr, __FUNCTION__, i, ret);
            return ret;
        }
    }
    *cnt = i;

    return CA_E_OK;
}

ca_int32_t
_phy_rtl8224_patch_version_assign(u8 phy_addr, u32 chipVer)
{
    ca_status_t ret = CA_E_OK;
    //writephy_ocp $phynum 0xA438 $currentVersion
    if((chipVer == PHY_RTL8224_VER_A) || (chipVer == PHY_RTL8224_VER_B))
    {
        ret = phy_rtl8224_patch_op(phy_addr, RTK_HWPATCH_OP_PHY, 0xff, 0x00, 0xA438, 15, 0, RTL8224_AB_FW_VER);
    }
    else
    {
        ret = phy_rtl8224_patch_op(phy_addr, RTK_HWPATCH_OP_PHY, 0xff, 0x00, 0xA438, 15, 0, RTL8224_FW_VER);
    }
    return ret;
}

ca_int32_t
_phy_rtl8224_patch(u8 phy_addr, u8 bcast)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t cnt = 0;
    ca_uint32_t chipVer = 5;

    phy_8224_chipVer_get(phy_addr, &chipVer);

    _phy_rtl8224_patch_assign(phy_addr, chipVer);
    //Patch Request
    //PP_PHYReg w $PHYID 0xB820 bit.4 0x1 patch request
    if ((ret = phy_rtl8224_patch_op(phy_addr, RTK_HWPATCH_OP_PHY, 0xff, 0x00, 0xb820, 4, 4, 0x1)) != CA_E_OK)
        return ret;

    //PP_PHYReg_bit r $PHYID 0xB800 6 6 wait 1
    if ((ret = _phy_rtl8224_patch_wait(phy_addr, PHY_MMD_VEND2, 0xB800, BIT_6, BIT_6, bcast)) != CA_E_OK)
        return ret;

    if ((ret = _phy_rtl8224_patch_process(phy_addr, patch_fwpr_conf, patch_fwpr_conf_size, &cnt, bcast))!= CA_E_OK)
    {
        printf("phy_addr %d 8224 fwpr patch failed. ret:0x%X\n", phy_addr, ret);
        return ret;
    }
#ifdef NI_8224B_DBG
    printf("phy_addr %d 8224 fwpr patch done. ret:0x%X cnt:%d\n", phy_addr, ret, cnt);
#endif

    //PP_PHYReg w $PHYID 0xB820 0x0000
    if ((ret = phy_rtl8224_patch_op(phy_addr, RTK_HWPATCH_OP_PHY, 0xff, 0x00, 0xb820, 4, 4, 0x0)) != CA_E_OK)
        return ret;

    //PP_PHYReg_bit r $PHYID 0xB800 6 6  wait 0 (Release patch request & wait patch_rdy = 0)
    if ((ret = _phy_rtl8224_patch_wait(phy_addr, PHY_MMD_VEND2, 0xB800, 0, BIT_6, bcast)) != CA_E_OK)
        return ret;

    //Lock Main
    //writephybits_ocp $phynum 0xa4a0 10 10 0x1
    if ((ret = phy_rtl8224_patch_op(phy_addr, RTK_HWPATCH_OP_PHY, 0xff, 0x00, 0xA4A0, 10, 10, 0x1)) != CA_E_OK)
        return ret;
    //[PP_PHYReg_bit r $PHYID 0xa600 7 0]
    if ((ret = _phy_rtl8224_patch_wait(phy_addr, PHY_MMD_VEND2, 0xa600, 0x1, 0xFF, bcast)) != CA_E_OK)
        return ret;

    if ((ret = _phy_rtl8224_patch_process(phy_addr, patch_fwlm_conf, patch_fwlm_conf_size, &cnt, bcast))!= CA_E_OK)
    {
        printf("phy_addr %d 8224 fwlm patch failed. ret:0x%X\n", phy_addr, ret);
        return ret;
    }
#ifdef NI_8224B_DBG
    printf("phy_addr %d 8224 fwlm patch done. ret:0x%X cnt:%d\n", phy_addr, ret, cnt);
#endif

    if ((ret = _phy_rtl8224_patch_process(phy_addr, patch_afe_conf, patch_afe_conf_size, &cnt, bcast)) != CA_E_OK)
    {
        printf("phy_addr %d 8224 afe patch failed. ret:0x%X\n", phy_addr, ret);
        return ret;
    }
#ifdef NI_8224B_DBG
    printf("phy_addr %d 8224 afe patch done. ret:0x%X cnt:%d\n", phy_addr, ret, cnt);
#endif

    //Patch PHY
    if(chipVer == PHY_RTL8224_VER_A){
        //writephy_ocp $phynum 0xa5d0 0x0
        if ((ret = phy_rtl8224_patch_op(phy_addr, RTK_HWPATCH_OP_PHY, 0xff, 0x00, 0xa5d0, 15, 0, 0x0)) != CA_E_OK)
            return ret;

        //writephybits_ocp $phynum 0xa430 2 0 0x0
        if ((ret = phy_rtl8224_patch_op(phy_addr, RTK_HWPATCH_OP_PHY, 0xff, 0x00, 0xa430, 2, 0, 0x0)) != CA_E_OK)
            return ret;
    }

    //writephy_ocp $phynum 0xa47E 7 6 0x1
    if ((ret = phy_rtl8224_patch_op(phy_addr, RTK_HWPATCH_OP_PHY, 0xff, 0x00, 0xa47E, 7, 6, 0x1)) != CA_E_OK)
        return ret;

    //Release Lock Main
    //writephybits_ocp $phynum 0xa4a0 10 10 0x0
    if ((ret = phy_rtl8224_patch_op(phy_addr, RTK_HWPATCH_OP_PHY, 0xff, 0x00, 0xA4A0, 10, 10, 0x0)) != CA_E_OK)
        return ret;

    //[PP_PHYReg_bit r $PHYID 0xa600 7 0]
    if ((ret = _phy_rtl8224_patch_wait_not_equal(phy_addr, PHY_MMD_VEND2, 0xa600, 0x1, 0xFF, bcast)) != CA_E_OK)
        return ret;

    if ((ret = _phy_rtl8224_patch_process(phy_addr, patch_top_conf, patch_top_conf_size, &cnt, bcast))!= CA_E_OK)
    {
        printf("phy_addr %d 8224 top patch failed. ret:0x%X\n", phy_addr, ret);
        return ret;
    }
#ifdef NI_8224B_DBG
    printf("phy_addr %d 8224 top patch done. ret:0x%X cnt:%d\n", phy_addr, ret, cnt);
#endif

    if ((ret = _phy_rtl8224_patch_process(phy_addr, patch_sds_conf, patch_sds_conf_size, &cnt, bcast))!= CA_E_OK)
    {
        printf("phy_addr %d 8224 sds patch failed. ret:0x%X\n", phy_addr, ret);
        return ret;
    }
#ifdef NI_8224B_DBG
    printf("phy_addr %d 8224 sds patch done. ret:0x%X cnt:%d\n", phy_addr, ret, cnt);
#endif

    //writephy_ocp $phynum 0xA436 0x801E
    if ((ret = phy_rtl8224_patch_op(phy_addr, RTK_HWPATCH_OP_PHY, 0xff, 0x00, 0xA436, 15, 0, 0x801E)) != CA_E_OK)
        return ret;

    ret = _phy_rtl8224_patch_version_assign(phy_addr, chipVer);

    return ret;
}

static int _phy_8224_sdsRegField_set(u8 phy_addr, u32 sdsPage, u32 sdsReg, u8 msb, u8 lsb, u32 data)
{
    ca_int32_t ret;
    ca_uint32_t reg_data = 0;
    ca_uint32_t mask = 0;

    if ((msb > 15) || (lsb > 15) || (msb < lsb))
    {
        return CA_E_PARAM;
    }

    phy_rtl8224_sdsReg_get(phy_addr, sdsPage, sdsReg, &reg_data);

    ret = _phy_8224_mask_get(msb, lsb, &mask);
    if(ret != CA_E_OK)
        return ret;

    reg_data &= ~(mask);
    reg_data |= (((data) << lsb) & mask);

    phy_rtl8224_sdsReg_set(phy_addr, sdsPage, sdsReg, reg_data);

    return CA_E_OK;
}

ca_int32_t
phy_8224_sdsOpCode_set(u8 phy_addr, u32 opCode)
{
    return _phy_8224_sdsRegField_set(phy_addr, PHY_8224_NWAY_OPCODE_PAGE, PHY_8224_NWAY_OPCODE_REG, PHY_8224_NWAY_OPCODE_HIGH_BIT, PHY_8224_NWAY_OPCODE_LOW_BIT, opCode);
}

ca_int32_t
phy_8224_sdsAmPeriod_set(u8 phy_addr, u32 amPeriod)
{
    return _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PERIOD_PAGE, PHY_8224_AM_PERIOD_REG, PHY_8224_AM_PERIOD_HIGH_BIT, PHY_8224_AM_PERIOD_LOW_BIT, amPeriod);
}

ca_int32_t
phy_8224_sdsAllAm_set(u8 phy_addr, u32 value)
{
    /* AM0 */
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG0_REG, PHY_8224_AM0_M0_HIGH_BIT, PHY_8224_AM0_M0_LOW_BIT, value);
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG0_REG, PHY_8224_AM0_M1_HIGH_BIT, PHY_8224_AM0_M1_LOW_BIT, value);
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG1_REG, PHY_8224_AM0_M2_HIGH_BIT, PHY_8224_AM0_M2_LOW_BIT, value);
    /* AM1 */
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG1_REG, PHY_8224_AM1_M0_HIGH_BIT, PHY_8224_AM1_M0_LOW_BIT, value);
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG2_REG, PHY_8224_AM1_M1_HIGH_BIT, PHY_8224_AM1_M1_LOW_BIT, value);
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG2_REG, PHY_8224_AM1_M2_HIGH_BIT, PHY_8224_AM1_M2_LOW_BIT, value);
    /* AM2 */
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG3_REG, PHY_8224_AM2_M0_HIGH_BIT, PHY_8224_AM2_M0_LOW_BIT, value);
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG3_REG, PHY_8224_AM2_M1_HIGH_BIT, PHY_8224_AM2_M1_LOW_BIT, value);
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG4_REG, PHY_8224_AM2_M2_HIGH_BIT, PHY_8224_AM2_M2_LOW_BIT, value);
    /* AM3 */
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG4_REG, PHY_8224_AM3_M0_HIGH_BIT, PHY_8224_AM3_M0_LOW_BIT, value);
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG5_REG, PHY_8224_AM3_M1_HIGH_BIT, PHY_8224_AM3_M1_LOW_BIT, value);
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PAGE, PHY_8224_AM_CFG5_REG, PHY_8224_AM3_M2_HIGH_BIT, PHY_8224_AM3_M2_LOW_BIT, value);

    return CA_E_OK;
}

ca_int32_t
phy_8224_sdsExtra_set(u8 phy_addr)
{
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_AM_PERIOD_PAGE, PHY_8224_TGR_PRO_0_REG17, PHY_8224_USXG_V2P1P2_RXOFF_HIGH_BIT, PHY_8224_USXG_V2P1P2_RXOFF_LOW_BIT, 0x1);
    _phy_8224_sdsRegField_set(phy_addr, PHY_8224_NWAY_OPCODE_PAGE, PHY_8224_TGR_PRO_1_REG19, PHY_8224_USXG_RXDV_OFF_HIGH_BIT, PHY_8224_USXG_RXDV_OFF_LOW_BIT, 0x1);

    return CA_E_OK;
}

/* Function Name:
 *      phy_8224_ctrl_set
 * Description:
 *      Set PHY settings for specific feature
 * Input:
 *      unit      - unit id
 *      port      - port id
 *      ctrl_type - setting type
 *      value     - setting value
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_NOT_SUPPORTED
 * Note:
 *      None
 */
ca_int32_t
phy_8224_ctrl_set(u8 phy_addr, rtk_phy_ctrl_t ctrl_type, u32 value)
{
    ca_int32_t ret = CA_E_OK;
    ca_uint32_t phyData, msb_bit, lsb_bit;
    ca_uint32_t reg_addr;
    ca_uint8_t base_port = 0;
    ca_uint8_t port_offset = 0;

    base_port = 5;
	if (phy_addr < 5 || phy_addr >= 9)
	{
		printf("%s: illegal phy_addr=%u\n", phy_addr);
		return 0;
	}
	port_offset = phy_addr - base_port;

    switch (ctrl_type)
    {
        case RTK_PHY_CTRL_MDI_POLARITY_SWAP:
            lsb_bit = (port_offset*RTL8224_PHY_TX_POLARITY_SWAP_OFFSET);
            msb_bit = lsb_bit + (RTL8224_PHY_TX_POLARITY_SWAP_OFFSET - 1);
            if ((ret = _phy_8224_intReg_get(phy_addr, RTL8224_PHY_TX_POLARITY_SWAP, msb_bit, lsb_bit, &phyData)) != CA_E_OK)
                return ret;

            phyData &= (~(BIT_0 | BIT_1 | BIT_2 | BIT_3));
            phyData |= (value & RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_A)? (BIT_0) : (0);
            phyData |= (value & RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_B)? (BIT_1) : (0);
            phyData |= (value & RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_C)? (BIT_2) : (0);
            phyData |= (value & RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_D)? (BIT_3) : (0);
            if ((ret = _phy_8224_intReg_set(phy_addr, RTL8224_PHY_TX_POLARITY_SWAP, msb_bit, lsb_bit, phyData)) != CA_E_OK)
                return ret;
            break;
        case RTK_PHY_CTRL_MDI_INVERSE:
            if ((ret = _phy_8224_intReg_get(phy_addr, RTL8224_PHY_MDI_REVERSE, port_offset, port_offset, &phyData)) != CA_E_OK)
                return ret;
            /* PortOffset 0&1 --> 0: Reverse, 1: Normal */
            /* PortOffset 2&3 --> 0: Normal,  1: Reverse */
            if(value != 0) /*Need to reverse*/
            {
                switch(port_offset)
                {
                    case 0:
                    case 1:
                        phyData = 0;
                        break;
                    case 2:
                    case 3:
                        phyData = 1;
                        break;
                    default:
                        return CA_E_ERROR;
                }
            }else{
                switch(port_offset)
                {
                    case 0:
                    case 1:
                phyData = 1;
                        break;
                    case 2:
                    case 3:
                phyData = 0;
                        break;
                    default:
                        return CA_E_ERROR;
                }
            }
            if ((ret = _phy_8224_intReg_set(phy_addr, RTL8224_PHY_MDI_REVERSE, port_offset, port_offset, phyData)) != CA_E_OK)
                return ret;
            break;
#if 0
        case RTK_PHY_CTRL_LED_1_MODE:
        case RTK_PHY_CTRL_LED_2_MODE:
        case RTK_PHY_CTRL_LED_3_MODE:
        case RTK_PHY_CTRL_LED_4_MODE:
            ret = _phy_8224_led_mode_set(phy_addr, (ctrl_type - RTK_PHY_CTRL_LED_1_MODE), value);
            break;
        case RTK_PHY_CTRL_LED_CFG_BLINK_RATE:
            ret = _phy_8224_led_config_blink_rate_set(phy_addr, value);
            break;
        case RTK_PHY_CTRL_LED_1_CFG_ACTIVE_LOW:
        case RTK_PHY_CTRL_LED_2_CFG_ACTIVE_LOW:
        case RTK_PHY_CTRL_LED_3_CFG_ACTIVE_LOW:
        case RTK_PHY_CTRL_LED_4_CFG_ACTIVE_LOW:
            ret = _phy_8224_led_config_active_low_set(phy_addr, (ctrl_type - RTK_PHY_CTRL_LED_1_CFG_ACTIVE_LOW), value);
            break;
        case RTK_PHY_CTRL_LED_1_CFG_FORCE:
        case RTK_PHY_CTRL_LED_2_CFG_FORCE:
        case RTK_PHY_CTRL_LED_3_CFG_FORCE:
        case RTK_PHY_CTRL_LED_4_CFG_FORCE:
            ret = _phy_8224_led_config_force_set(phy_addr, (ctrl_type - RTK_PHY_CTRL_LED_1_CFG_FORCE), value);
            /* active software led setting*/ 
            if ((ret = _phy_8224_intReg_set(phy_addr, RTL8224_SW_LED_LOAD, RTL8224_SW_LED_LOAD_MSB, RTL8224_SW_LED_LOAD_LSB, 1)) != RT_ERR_OK)
                return ret;
            break;
        case RTK_PHY_CTRL_LOOPBACK_INTERNAL_PMA:
            if ((ret = phy_common_general_reg_mmd_get(phy_addr, PHY_MMD_PMAPMD, 0, &phyData)) != RT_ERR_OK)
                return ret;
            phyData &= (~BIT_0);
            phyData |= (value == 0)? (0):(BIT_0);
            if ((ret = phy_common_general_reg_mmd_set(phy_addr, PHY_MMD_PMAPMD, 0, phyData)) != RT_ERR_OK)
                return ret;
            break;

        case RTK_PHY_CTRL_PTP_REFTIME_TOD_DELAY:
            if (value > 0xFFFF)
                return RT_ERR_OUT_OF_RANGE;
            if ((ret = _phy_8224_intReg_set(phy_addr, RTL8224_PTP_TIME_TOD_DELAY_R, RTL8224_TOD_DELAY_MSB, RTL8224_TOD_DELAY_LSB, value)) != RT_ERR_OK)
                return ret;
            break;

        case RTK_PHY_CTRL_PTP_DURATION_THRESHOLD:
            if (value > 0x3FF)
                return RT_ERR_OUT_OF_RANGE;
            if ((ret = _phy_8224_intReg_set(phy_addr, RTL8224_PTP_TIME_OP_DURATION_R, RTL8224_TIME_OP_DURATION_MSB, RTL8224_TIME_OP_DURATION_LSB, value)) != RT_ERR_OK)
                return ret;
            break;

        case RTK_PHY_CTRL_PTP_PORT_ROLE:
            switch (value)
            {
                case RTK_PHY_CTRL_PTP_PORT_ROLE_NONE:
                    phyData = 0x0;
                    break;
                case RTK_PHY_CTRL_PTP_PORT_ROLE_BC_OC:
                    phyData = 0x1;
                    break;
                case RTK_PHY_CTRL_PTP_PORT_ROLE_E2E_TC:
                    phyData = 0x2;
                    break;
                case RTK_PHY_CTRL_PTP_PORT_ROLE_P2P_TC:
                    phyData = 0x3;
                    break;
                default:
                    return RT_ERR_INPUT;
            }
            reg_addr = (RTL8224_P0_PORT_CTRL_R + (RTL8224_P0_REG_PORT_OFFSET*port_offset));
            if ((ret = _phy_8224_intReg_set(phy_addr, reg_addr, RTL8224_P0_PTP_ROLE_MSB, RTL8224_P0_PTP_ROLE_LSB, phyData)) != RT_ERR_OK)
                return ret;
            break;

        case RTK_PHY_CTRL_PTP_TX_IMBAL:
            if (value > 0xFFF)
                return RT_ERR_OUT_OF_RANGE;

            reg_addr = (RTL8224_P0_TX_IMBAL_R + (RTL8224_P0_REG_PORT_OFFSET*port_offset));
            if ((ret = _phy_8224_intReg_set(phy_addr, reg_addr, RTL8224_P0_TX_IMBAL_MSB, RTL8224_P0_TX_IMBAL_LSB, value)) != RT_ERR_OK)
                return ret;
            break;

        case RTK_PHY_CTRL_PTP_RX_IMBAL:
            if (value > 0xFFF)
                return RT_ERR_OUT_OF_RANGE;

            reg_addr = (RTL8224_P0_RX_IMBAL_R + (RTL8224_P0_REG_PORT_OFFSET*port_offset));
            if ((ret = _phy_8224_intReg_set(phy_addr, reg_addr, RTL8224_P0_RX_IMBAL_MSB, RTL8224_P0_RX_IMBAL_LSB, value)) != RT_ERR_OK)
                return ret;
            break;

        case RTK_PHY_CTRL_PTP_CLOCK_SRC:
            if ((ret = _phy_8224_intRegBit_set(phy_addr, RTL8224_PTP_CLK_CTRL_R, RTL8224_CFG_CLK_SRC_OFFSET, ((value == RTK_PHY_CTRL_PTP_CLOCK_SRC_INT) ? 0 : 1))) != RT_ERR_OK)
                return ret;
            break;
        case RTK_PHY_CTRL_TEMP_THRESHOLD_HIGH_HIGHER:
        case RTK_PHY_CTRL_TEMP_THRESHOLD_LOW_LOWER:
            ret = _phy_8224_temperature_threshold_set(phy_addr, ctrl_type, value);
            break;
#endif
        case RTK_PHY_CTRL_SERDES_TX_POLARITY:
            if ((ret = phy_rtl8224_sdsReg_get(base_port, 0x6, 0x2, &phyData)) != CA_E_OK)
                return ret;
            phyData &= (~BIT_14);
            phyData |= (value == 0) ? (0) : (BIT_14);
            if ((ret = phy_rtl8224_sdsReg_set(phy_addr, 0x6, 0x2, phyData)) != CA_E_OK)
                return ret;
            break;
        case RTK_PHY_CTRL_SERDES_RX_POLARITY:
            if ((ret = phy_rtl8224_sdsReg_get(base_port, 0x6, 0x2, &phyData)) != CA_E_OK)
                return ret;
            phyData &= (~BIT_13);
            phyData |= (value == 0) ? (0) : (BIT_13);
            if ((ret = phy_rtl8224_sdsReg_set(phy_addr, 0x6, 0x2, phyData)) != CA_E_OK)
                return ret;
            break;
#if 0
        case RTK_PHY_CTRL_FAST_RETRAIN_NFR:
            switch (value)
            {
                case RTK_PHY_CTRL_FAST_RETRAIN_NFR_ENABLE:
                    if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_AN, 32, &phyData)) != RT_ERR_OK)
                        return ret;
                    phyData &= (~BIT_5);
                    if ((ret = phy_common_general_reg_mmd_set(unit, port, PHY_MMD_AN, 32, phyData)) != RT_ERR_OK)
                        return ret;
                    if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_VEND2, 0xA6D8, &phyData)) != RT_ERR_OK)
                        return ret;
                    phyData &= (~BIT_0);
                    if ((ret = phy_common_general_reg_mmd_set(unit, port, PHY_MMD_VEND2, 0xA6D8, phyData)) != RT_ERR_OK)
                        return ret;

                    if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_VEND2, 0xA654, &phyData)) != RT_ERR_OK)
                        return ret;
                    phyData |= (BIT_11);
                    if ((ret = phy_common_general_reg_mmd_set(unit, port, PHY_MMD_VEND2, 0xA654, phyData)) != RT_ERR_OK)
                        return ret;

                    break;

                case RTK_PHY_CTRL_FAST_RETRAIN_NFR_DISABLE:
                    if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_AN, 32, &phyData)) != RT_ERR_OK)
                        return ret;
                    phyData |= (BIT_5);
                    if ((ret = phy_common_general_reg_mmd_set(unit, port, PHY_MMD_AN, 32, phyData)) != RT_ERR_OK)
                        return ret;
                    if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_VEND2, 0xA6D8, &phyData)) != RT_ERR_OK)
                        return ret;
                    phyData |= (BIT_0);
                    if ((ret = phy_common_general_reg_mmd_set(unit, port, PHY_MMD_VEND2, 0xA6D8, phyData)) != RT_ERR_OK)
                        return ret;

                    if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_VEND2, 0xA654, &phyData)) != RT_ERR_OK)
                        return ret;
                    phyData &= (~BIT_11);
                    if ((ret = phy_common_general_reg_mmd_set(unit, port, PHY_MMD_VEND2, 0xA654, phyData)) != RT_ERR_OK)
                        return ret;

                    break;

                default:
                    return RT_ERR_INPUT;
            }
            ret = _phy_8224_an_restart(unit, port);
            break;
            case RTK_PHY_CTRL_FAST_RETRAIN:
                switch (value)
                {
                    case RTK_PHY_CTRL_FAST_RETRAIN_ENABLE:
                        if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_AN, 32, &phyData)) != RT_ERR_OK)
                            return ret;
                        phyData |= (BIT_5);
                        if ((ret = phy_common_general_reg_mmd_set(unit, port, PHY_MMD_AN, 32, phyData)) != RT_ERR_OK)
                            return ret;
                        if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_VEND2, 0xA6D8, &phyData)) != RT_ERR_OK)
                            return ret;
                        phyData |= (BIT_0);
                        if ((ret = phy_common_general_reg_mmd_set(unit, port, PHY_MMD_VEND2, 0xA6D8, phyData)) != RT_ERR_OK)
                            return ret;
                        break;

                    case RTK_PHY_CTRL_FAST_RETRAIN_DISABLE:
                        if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_AN, 32, &phyData)) != RT_ERR_OK)
                            return ret;
                        phyData &= (~BIT_5);
                        if ((ret = phy_common_general_reg_mmd_set(unit, port, PHY_MMD_AN, 32, phyData)) != RT_ERR_OK)
                            return ret;
                        if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_VEND2, 0xA6D8, &phyData)) != RT_ERR_OK)
                            return ret;
                        phyData &= (~BIT_0);
                        if ((ret = phy_common_general_reg_mmd_set(unit, port, PHY_MMD_VEND2, 0xA6D8, phyData)) != RT_ERR_OK)
                            return ret;
                        break;

                    default:
                        return RT_ERR_INPUT;
                }
                ret = _phy_8224_an_restart(unit, port);
                break;
            case RTK_PHY_CTRL_SERDES_USXGMII_AN:
                if ((ret = _phy_8224_sdsRegFieldBit_set(unit, port, PHY_8224_NWAY_AN_PAGE, PHY_8224_NWAY_AN_REG, PHY_8224_QHSG_AN_CH3_EN_BIT, value)) != RT_ERR_OK)
                    return ret;
                if ((ret = _phy_8224_sdsRegFieldBit_set(unit, port, PHY_8224_NWAY_AN_PAGE, PHY_8224_NWAY_AN_REG, PHY_8224_QHSG_AN_CH2_EN_BIT, value)) != RT_ERR_OK)
                    return ret;
                if ((ret = _phy_8224_sdsRegFieldBit_set(unit, port, PHY_8224_NWAY_AN_PAGE, PHY_8224_NWAY_AN_REG, PHY_8224_QHSG_AN_CH1_EN_BIT, value)) != RT_ERR_OK)
                    return ret;
                if ((ret = _phy_8224_sdsRegFieldBit_set(unit, port, PHY_8224_NWAY_AN_PAGE, PHY_8224_NWAY_AN_REG, PHY_8224_QHSG_AN_CH0_EN_BIT, value)) != RT_ERR_OK)
                    return ret;
                break;
            case RTK_PHY_CTRL_SERDES_USXGMII_AM_PERIOD:
                if ((ret = phy_8224_sdsAmPeriod_set(unit, port, value)) != RT_ERR_OK)
                    return ret;
                break;
#endif
            case RTK_PHY_CTRL_COUNTER_CLEAR:
                if ((ret = _phy_8224_intReg_get(phy_addr, RTL8224_PHY_MIB_GLOBAL_CONFIG, 3, 0, &phyData)) != CA_E_OK)
                    return ret;
                phyData |= (0x1 << port_offset);
                if ((ret = _phy_8224_intReg_set(phy_addr, RTL8224_PHY_MIB_GLOBAL_CONFIG, 3, 0, phyData)) != CA_E_OK)
                    return ret;
                break;
            default:
                return CA_E_ERROR;
    }

    return ret;
}

/* Function Name:
*      phy_8224_ctrl_get
* Description:
*      Get PHY settings for specific feature
* Input:
*      unit      - unit id
*      port      - port id
*      ctrl_type - setting type
* Output:
*      pValue    - pointer to setting value
* Return:
*      RT_ERR_OK
*      RT_ERR_FAILED
*      RT_ERR_NOT_SUPPORTED
* Note:
*      None
*/
ca_int32_t
phy_8224_ctrl_get(u8 phy_addr, rtk_phy_ctrl_t ctrl_type, u32 *pValue)
{
    ca_int32_t ret = CA_E_OK;
    ca_uint32_t phyData = 0, phyData1 = 0;
    ca_uint32_t tmp, msb_bit, lsb_bit;
    ca_uint32_t reg_addr;
    ca_uint8_t base_port = 0;
    ca_uint8_t port_offset = 0;

    base_port = 5;
	if (phy_addr < 5 || phy_addr >= 9)
	{
		printf("%s: illegal phy_addr=%u\n", phy_addr);
		return 0;
	}
	port_offset = phy_addr - base_port;

    switch (ctrl_type)
    {
        case RTK_PHY_CTRL_MDI_POLARITY_SWAP:
            lsb_bit = (port_offset*RTL8224_PHY_TX_POLARITY_SWAP_OFFSET);
            msb_bit = lsb_bit + (RTL8224_PHY_TX_POLARITY_SWAP_OFFSET - 1);
            if ((ret = _phy_8224_intReg_get(phy_addr, RTL8224_PHY_TX_POLARITY_SWAP, msb_bit, lsb_bit, &phyData)) != CA_E_ERROR)
                return ret;

            tmp = 0;
            tmp |= (phyData & BIT_0) ? (RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_A) : (0);
            tmp |= (phyData & BIT_1) ? (RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_B) : (0);
            tmp |= (phyData & BIT_2) ? (RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_C) : (0);
            tmp |= (phyData & BIT_3) ? (RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_D) : (0);
            *pValue = tmp;
            break;
        case RTK_PHY_CTRL_MDI_INVERSE:
            if ((ret = _phy_8224_intReg_get(phy_addr, RTL8224_PHY_MDI_REVERSE, port_offset, port_offset, &phyData)) != CA_E_ERROR)
                return ret;
            /* PortOffset 0&1 --> 0: Reverse, 1: Normal */
            /* PortOffset 2&3 --> 0: Normal,  1: Reverse */
            if(phyData == 0)
            {
                switch(port_offset)
                {
                    case 0:
                    case 1:
                        phyData = 1;
                        break;
                    case 2:
                    case 3:
                        phyData = 0;
                        break;
                    default:
                        return CA_E_ERROR;
                }
            }else{
                switch(port_offset)
                {
                    case 0:
                    case 1:
                        phyData = 0;
                        break;
                    case 2:
                    case 3:
                        phyData = 1;
                        break;
                    default:
                        return CA_E_ERROR;
                }
            }
            *pValue = phyData;
            break;
#if 0
        case RTK_PHY_CTRL_LED_1_MODE:
        case RTK_PHY_CTRL_LED_2_MODE:
        case RTK_PHY_CTRL_LED_3_MODE:
        case RTK_PHY_CTRL_LED_4_MODE:
            ret = _phy_8224_led_mode_get(phy_addr, (ctrl_type - RTK_PHY_CTRL_LED_1_MODE), &phyData);
            *pValue = phyData;
            break;
        case RTK_PHY_CTRL_LED_CFG_BLINK_RATE:
            ret = _phy_8224_led_config_blink_rate_get(phy_addr, &phyData);
            *pValue = phyData;
            break;
        case RTK_PHY_CTRL_LED_1_CFG_ACTIVE_LOW:
        case RTK_PHY_CTRL_LED_2_CFG_ACTIVE_LOW:
        case RTK_PHY_CTRL_LED_3_CFG_ACTIVE_LOW:
        case RTK_PHY_CTRL_LED_4_CFG_ACTIVE_LOW:
            ret = _phy_8224_led_config_active_low_get(phy_addr, (ctrl_type - RTK_PHY_CTRL_LED_1_CFG_ACTIVE_LOW), &phyData);
            *pValue = phyData;
            break;
        case RTK_PHY_CTRL_LED_1_CFG_FORCE:
        case RTK_PHY_CTRL_LED_2_CFG_FORCE:
        case RTK_PHY_CTRL_LED_3_CFG_FORCE:
        case RTK_PHY_CTRL_LED_4_CFG_FORCE:
            ret = _phy_8224_led_config_force_get(unit, port, (ctrl_type - RTK_PHY_CTRL_LED_1_CFG_FORCE), &phyData);
            *pValue = phyData;
            break;
        case RTK_PHY_CTRL_LOOPBACK_INTERNAL_PMA:
            if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_PMAPMD, 0x0, &phyData)) != RT_ERR_OK)
                return ret;
            *pValue = (phyData & BIT_0) ? (1) : (0);
            break;
        case RTK_PHY_CTRL_PTP_REFTIME_TOD_DELAY:
            if ((ret = _phy_8224_intReg_get(unit, port, RTL8224_PTP_TIME_TOD_DELAY_R, RTL8224_TOD_DELAY_MSB, RTL8224_TOD_DELAY_LSB, &phyData)) != RT_ERR_OK)
                return ret;
            *pValue = phyData;
            break;

        case RTK_PHY_CTRL_PTP_DURATION_THRESHOLD:
            if ((ret = _phy_8224_intReg_get(unit, port, RTL8224_PTP_TIME_OP_DURATION_R, RTL8224_TIME_OP_DURATION_MSB, RTL8224_TIME_OP_DURATION_LSB, &phyData)) != RT_ERR_OK)
                return ret;
            *pValue = phyData;
            break;

        case RTK_PHY_CTRL_PTP_PORT_ROLE:
            reg_addr = (RTL8224_P0_PORT_CTRL_R + (RTL8224_P0_REG_PORT_OFFSET*port_offset));
            if ((ret = _phy_8224_intReg_get(unit, port, reg_addr, RTL8224_P0_PTP_ROLE_MSB, RTL8224_P0_PTP_ROLE_LSB, &phyData)) != RT_ERR_OK)
                return ret;
            switch (phyData)
            {
                case 0x0:
                    *pValue = RTK_PHY_CTRL_PTP_PORT_ROLE_NONE;
                    break;
                case 0x1:
                    *pValue = RTK_PHY_CTRL_PTP_PORT_ROLE_BC_OC;
                    break;
                case 0x2:
                    *pValue = RTK_PHY_CTRL_PTP_PORT_ROLE_E2E_TC;
                    break;
                case 0x3:
                    *pValue = RTK_PHY_CTRL_PTP_PORT_ROLE_P2P_TC;
                    break;
                default:
                    return RT_ERR_FAILED;
            }
            break;

        case RTK_PHY_CTRL_PTP_TX_IMBAL:
            reg_addr = (RTL8224_P0_TX_IMBAL_R + (RTL8224_P0_REG_PORT_OFFSET*port_offset));
            if ((ret = _phy_8224_intReg_get(unit, port, reg_addr, RTL8224_P0_TX_IMBAL_MSB, RTL8224_P0_TX_IMBAL_LSB, &phyData)) != RT_ERR_OK)
                return ret;
            *pValue = phyData;
            break;
        case RTK_PHY_CTRL_PTP_RX_IMBAL:
            reg_addr = (RTL8224_P0_RX_IMBAL_R + (RTL8224_P0_REG_PORT_OFFSET*port_offset));
            if ((ret = _phy_8224_intReg_get(unit, port, reg_addr, RTL8224_P0_RX_IMBAL_MSB, RTL8224_P0_RX_IMBAL_LSB, &phyData)) != RT_ERR_OK)
                return ret;
            *pValue = phyData;
            break;

        case RTK_PHY_CTRL_PTP_CLOCK_SRC:
            if ((ret = _phy_8224_intRegBit_get(unit, port, RTL8224_PTP_CLK_CTRL_R, RTL8224_CFG_CLK_SRC_OFFSET, &phyData)) != RT_ERR_OK)
                return ret;
            *pValue = (phyData == 0) ? RTK_PHY_CTRL_PTP_CLOCK_SRC_INT : RTK_PHY_CTRL_PTP_CLOCK_SRC_EXT;
            break;
        case RTK_PHY_CTRL_TEMP_THRESHOLD_HIGH_HIGHER:
        case RTK_PHY_CTRL_TEMP_THRESHOLD_LOW_LOWER:
            ret = _phy_8224_temperature_threshold_get(unit, port, ctrl_type, &phyData);
            *pValue = phyData;
            break;
#endif
        case RTK_PHY_CTRL_SERDES_TX_POLARITY:
            if ((ret = phy_rtl8224_sdsReg_get(phy_addr, 0x6, 0x2, &phyData)) != CA_E_ERROR)
                return ret;
            *pValue = (phyData & BIT_14) ? (1) : (0);
            break;
        case RTK_PHY_CTRL_SERDES_RX_POLARITY:
            if ((ret = phy_rtl8224_sdsReg_get(phy_addr, 0x6, 0x2, &phyData)) != CA_E_ERROR)
                return ret;
            *pValue = (phyData & BIT_13) ? (1) : (0);
            break;
#if 0
        case RTK_PHY_CTRL_FAST_RETRAIN_NFR:
            if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_VEND2, 0xA654, &phyData)) != RT_ERR_OK)
                return ret;
            if (phyData & BIT_11)
            {
                *pValue = RTK_PHY_CTRL_FAST_RETRAIN_NFR_ENABLE;
            }
            else
            {
                *pValue = RTK_PHY_CTRL_FAST_RETRAIN_NFR_DISABLE;
            }
            break;
        case RTK_PHY_CTRL_FAST_RETRAIN:
            if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_AN, 32, &phyData)) != RT_ERR_OK)
                return ret;
            phyData &= (BIT_5);
            if ((ret = phy_common_general_reg_mmd_get(unit, port, PHY_MMD_VEND2, 0xA6D8, &phyData1)) != RT_ERR_OK)
                return ret;
            phyData1 &= (BIT_0);
            if ((phyData != 0) || (phyData1 != 0))
            {
                *pValue = RTK_PHY_CTRL_FAST_RETRAIN_ENABLE;
            }
            else
            {
                *pValue = RTK_PHY_CTRL_FAST_RETRAIN_DISABLE;
            }
            break;
        case RTK_PHY_CTRL_SERDES_USXGMII_AN:
            phyData1 = 0;
            if ((ret = _phy_8224_sdsRegFieldBit_get(unit, port, PHY_8224_NWAY_AN_PAGE, PHY_8224_NWAY_AN_REG, PHY_8224_QHSG_AN_CH3_EN_BIT, &phyData)) != RT_ERR_OK)
                return ret;
            phyData1 |= phyData;
            if ((ret = _phy_8224_sdsRegFieldBit_get(unit, port, PHY_8224_NWAY_AN_PAGE, PHY_8224_NWAY_AN_REG, PHY_8224_QHSG_AN_CH2_EN_BIT, &phyData)) != RT_ERR_OK)
                return ret;
            phyData1 |= phyData;
            if ((ret = _phy_8224_sdsRegFieldBit_get(unit, port, PHY_8224_NWAY_AN_PAGE, PHY_8224_NWAY_AN_REG, PHY_8224_QHSG_AN_CH1_EN_BIT, &phyData)) != RT_ERR_OK)
                return ret;
            phyData1 |= phyData;
            if ((ret = _phy_8224_sdsRegFieldBit_get(unit, port, PHY_8224_NWAY_AN_PAGE, PHY_8224_NWAY_AN_REG, PHY_8224_QHSG_AN_CH0_EN_BIT, &phyData)) != RT_ERR_OK)
                return ret;
            phyData1 |= phyData;
            if(phyData1 == 0)
                *pValue = DISABLED;
            else
                *pValue = ENABLED;
            break;
        case RTK_PHY_CTRL_SERDES_USXGMII_AM_PERIOD:
            if ((ret = phy_8224_sdsAmPeriod_get(unit, port, pValue)) != RT_ERR_OK)
                return ret;
            break;
#endif
        default:
            return CA_E_ERROR;
    }

    return ret;
}

/* Function Name:
 *      phy_8224_macIntfSerdesMode_get
 * Description:
 *      Get PHY's MAC side SerDes interface mode
 * Input:
 *      unit   - unit id
 *      port   - port id
 * Output:
 *      pMode  - serdes mode
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
ca_int32_t
phy_8224_macIntfSerdesMode_get(u8 phy_addr, rt_serdesMode_t *pMode)
{
    ca_int32_t ret = CA_E_OK;
    ca_uint32_t sdsmode = 0;
    ca_uint32_t usx_subMode = 0;
    ca_uint32_t phyData = 0;

	if ((ret = ca_mdio_read(phy_addr, 0, PHY_MMD_VEND1, 0x7b20, &phyData)) != CA_E_OK)
    	return ret;

    sdsmode = (phyData & 0x1F);
    switch (sdsmode)
    {
        case 0b00010:
            *pMode = RTK_MII_SGMII;
            break;

        case 0b00100:
            *pMode = RTK_MII_1000BX_FIBER;
            break;

        case 0b00101:
            *pMode = RTK_MII_100BX_FIBER;
            break;

        case 0b00111:
            *pMode = RTK_MII_1000BX100BX_AUTO;
            break;

        case 0b01101:
        {
            usx_subMode = ((phyData & 0x7c00) >> 10);

            switch (usx_subMode)
            {
                case 0b00000:
                    *pMode = RTK_MII_USXGMII_10GSXGMII;
                    break;
                case 0b00010:
                    *pMode = RTK_MII_USXGMII_10GQXGMII;
                    break;
                default:
                    *pMode = RTK_MII_USXGMII_10GQXGMII;
                    break;
            }
        }
        break;

        case 0b10010:
            *pMode = RTK_MII_HISGMII;
            break;

        case 0b10110:
            *pMode = RTK_MII_2500Base_X;
            break;

        case 0b11010:
            *pMode = RTK_MII_10GR;
            break;

        case 0b11011:
            *pMode = RTK_MII_10GRSGMII_AUTO;
            break;

        case 0b11100:
            *pMode = RTK_MII_10GR1000BX_AUTO;
            break;
        default:
            *pMode = RTK_MII_DISABLE;
    }

    return CA_E_OK;
}

/* Function Name:
 *      phy_8224_macIntfSerdesLinkStatus_get
 * Description:
 *      Get PHY's MAC side SerDes interface link status
 * Input:
 *      unit    - unit ID
 *      port    - port id
 * Output:
 *      pStatus - link status of the SerDes
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 * Note:
 *      None
 */
ca_int32_t
phy_8224_macIntfSerdesLinkStatus_get(u8 phy_addr, rtk_phy_macIntfSdsLinkStatus_t *pStatus)
{
    ca_int32_t ret = CA_E_OK;
    ca_uint32_t phyData = 0;

    ret = phy_rtl8224_sdsReg_get(phy_addr, 0x5, 0x0, &phyData);

    pStatus->sds_num=1;
    pStatus->link_status[0] = ((phyData & 0x1000) == 0x1000) ? (PORT_LINKUP) : (PORT_LINKDOWN);

    return ret;
}

void phy_8224_dbg_dump(void)
{
	rtk_phy_macIntfSdsLinkStatus_t sdsStatus;
	rt_serdesMode_t sdsMode;
	ca_int32_t ret = CA_E_OK;

#define RTL8224_MAIN_PHY_ADDR	5

	printf("RTL8224B info:\n");
	ret = phy_8224_macIntfSerdesMode_get(RTL8224_MAIN_PHY_ADDR, &sdsMode);
	if (ret == CA_E_OK)
	{
		printf("	Serdes Mode: %s\n", GET_SERDES_MODE_STR(sdsMode));
	}
	ret = phy_8224_macIntfSerdesLinkStatus_get(RTL8224_MAIN_PHY_ADDR, &sdsStatus);
	if (ret == CA_E_OK)
	{
		printf("	Serdes Num: %d Serdes Link Status: %s\n", sdsStatus.sds_num, (sdsStatus.link_status[0] == PORT_LINKUP) ? "UP":"DOWN");
	}
}

ca_int32_t
phy_rtl8224_patch(u8 phy_addr)
{
    ca_status_t ret = CA_E_OK;
	ca_uint16_t phyData = 0;

    _phy_rtl8224_patch(phy_addr, PHY_PATCH_OP_NORMAL);

    /*Disable 100M/1G EEE advertise ability */
    phyData = 0;
	if ((ret = ca_mdio_write(phy_addr, 0, PHY_MMD_VEND2, 0xa5d0, phyData)) != CA_E_OK)
		return ret;

    /*Disable 1G lite ability */
	if ((ret = ca_mdio_read(phy_addr, 0, PHY_MMD_VEND2, 0xa428, &phyData)) != CA_E_OK)
		return ret;

	if ((ret = ca_mdio_write(phy_addr, 0, PHY_MMD_VEND2, 0xa428, phyData)) != CA_E_OK)
		return ret;

	if ((ret = phy_8224_sdsOpCode_set(phy_addr, 0xaa)) != CA_E_OK)
            return ret;

    if ((ret = phy_8224_sdsAmPeriod_set(phy_addr, 0x5078)) != CA_E_OK)
        return ret;

    if ((ret = phy_8224_sdsAllAm_set(phy_addr, 0x0)) != CA_E_OK)
        return ret;

    if ((ret = phy_8224_sdsExtra_set(phy_addr)) != CA_E_OK)
        return ret;

    return ret;
}

void rtl8224_config(u32 qxgmii_port_base_phy_addr)
{
	u8 phy_addr;

	for (phy_addr = 0; phy_addr < 4; phy_addr++)
	{
		phy_rtl8224_patch(qxgmii_port_base_phy_addr+phy_addr);
	}
}

void ca_ni_rtl8224_phy_init(u32 qxgmii_port_base_phy_addr)
{
	u8 phy_addr;
	u16 data;

	for (phy_addr = 0; phy_addr < 4; phy_addr++) {
		ca_mdio_write(qxgmii_port_base_phy_addr+phy_addr, 0, 30, 0xa90, 0xc);
		ca_mdio_write(qxgmii_port_base_phy_addr+phy_addr, 0, 31, 0xa610, 0x2058);
	}

#ifdef NI_8224B_DBG
	ca_mdio_read(qxgmii_port_base_phy_addr, 0, 31, 0xa610, &data);
	printf("%s: phy_addr=%d, read register 31 waddr 0xa610, value=0x%x\n", __func__, qxgmii_port_base_phy_addr, data);
	ca_mdio_read(qxgmii_port_base_phy_addr, 0, 30, 0xa90, &data);
	printf("%s: phy_addr=%d, read register 30 waddr 0xa90, value=0x%x\n", __func__, qxgmii_port_base_phy_addr, data);
#endif
}

#if 0
static int do_ctrl_8224(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	ca_int32_t ret;
	rtk_phy_ctrl_t ctrl_type;
	ca_uint16_t phy_addr;
	ca_uint32_t value;

	if (argc <= 3) // get
	{
		phy_addr = simple_strtoul(argv[1], NULL, 10);

		if (!strcmp(argv[2], "RTK_PHY_CTRL_MDI_POLARITY_SWAP"))
			ctrl_type = RTK_PHY_CTRL_MDI_POLARITY_SWAP;
		else if (!strcmp(argv[2], "RTK_PHY_CTRL_MDI_INVERSE"))
			ctrl_type = RTK_PHY_CTRL_MDI_INVERSE;
		else if (!strcmp(argv[2], "RTK_PHY_CTRL_SERDES_TX_POLARITY"))
			ctrl_type = RTK_PHY_CTRL_SERDES_TX_POLARITY;
		else if (!strcmp(argv[2], "RTK_PHY_CTRL_SERDES_RX_POLARITY"))
			ctrl_type = RTK_PHY_CTRL_SERDES_RX_POLARITY;
		else {
			printf("illegal ctrl_type\n");
			return 0;
		}
		
		ret = phy_8224_ctrl_get(phy_addr, ctrl_type, value);
		printf("phy_8224_ctrl_get, ret=%d, ctrl_type=%s, value=0x%x\n", ret, argv[2], value);
	}
	else if (argc <= 4) // get
	{
		phy_addr = simple_strtoul(argv[1], NULL, 10);

		if (!strcmp(argv[2], "RTK_PHY_CTRL_MDI_POLARITY_SWAP"))
			ctrl_type = RTK_PHY_CTRL_MDI_POLARITY_SWAP;
		else if (!strcmp(argv[2], "RTK_PHY_CTRL_MDI_INVERSE"))
			ctrl_type = RTK_PHY_CTRL_MDI_INVERSE;
		else if (!strcmp(argv[2], "RTK_PHY_CTRL_SERDES_TX_POLARITY"))
			ctrl_type = RTK_PHY_CTRL_SERDES_TX_POLARITY;
		else if (!strcmp(argv[2], "RTK_PHY_CTRL_SERDES_RX_POLARITY"))
			ctrl_type = RTK_PHY_CTRL_SERDES_RX_POLARITY;
		else {
			printf("illegal ctrl_type\n");
			return 0;
		}

		value = simple_strtoull(argv[3], NULL, 10);
		
		ret = phy_8224_ctrl_set(phy_addr, ctrl_type, value);
		printf("phy_8224_ctrl_set, ret=%d, ctrl_type=%s, value=0x%x\n", ret, argv[2], value);
	}
	return 0;
}

U_BOOT_CMD(
	ctrl_8224, 4, 1, do_ctrl_8224,
	"ctrl_8224 \n",
	"[PHY addr] [RTK_PHY_CTRL_MDI_POLARITY_SWAP/RTK_PHY_CTRL_MDI_INVERSE/RTK_PHY_CTRL_SERDES_TX_POLARITY/RTK_PHY_CTRL_SERDES_RX_POLARITY] ([value])\n"
	"PHY addr : 0-31\n"
);
#endif

