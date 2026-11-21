#include <common.h>
#include <command.h>
#include <sound.h>
#include <asm/io.h>
#include <usb.h>
#include <net.h>

#include <stdio.h>
#include <config.h>
#include <dm.h>
#include <asm-generic/gpio.h>
#include <linux/mii.h>
#include <linux/list.h>
#include <i2c.h>
#include <miiphy.h>
#include <timestamp.h>

#include "ft2.h"
#include "rl6898_profile.c"


//OTP access
#include "../smc_call/cortina_smc.h"
#define SIP_SVC_CALL                    0x82000000
#define CA_SVC_OTP_ID           0xA
#define CA_SVC_OTP_BYTE_READ           (SIP_SVC_CALL|CA_SVC_OTP|7)
#define CA_SVC_OTP_BYTE_WRITE          (SIP_SVC_CALL|CA_SVC_OTP|8)
#define CA_SVC_OTP_BIT_READ            (SIP_SVC_CALL|CA_SVC_OTP|9)
#define CA_SVC_OTP_BIT_WRITE           (SIP_SVC_CALL|CA_SVC_OTP|10)
#define ca_otp_smc_call(smc_id, b_loc, b_len, src, x4) ca_smc_call(smc_id, b_loc, b_len, src, x4)
#define OTP_READ_BYTE(byte_loc, src) ca_otp_smc_call(CA_SVC_OTP_BYTE_READ, byte_loc, 1, src, 0)
#define OTP_WRITE_BYTE(byte_loc, data2w) ca_otp_smc_call(CA_SVC_OTP_BYTE_WRITE, byte_loc, 0, 0, data2w)
#define OTP_WRITE_BIT(byte_loc, byte_len, data2w) ca_otp_smc_call(CA_SVC_OTP_BIT_WRITE, byte_loc, byte_len, 0, data2w)

typedef struct cmd_tbl        cmd_tbl_t;
static inline void mdelay(unsigned long msec)
{
        udelay(1000 * msec);
}

unsigned long ft2_start;
unsigned long ft2_finish;

static ft2_record_t ft2_records[FT2_PARAM_MAX];

static char *ft2_module_name[] = {
    [FT2_PARAM_PRBS_PON] = "PRBS_PON",
    [FT2_PARAM_PRBS_PON_RESULT] = "PRBS_PON_RESULT",
    [FT2_PARAM_PON_BEN] = "PON_BEN",
    [FT2_PARAM_PCIE] = "PCIE",
    [FT2_PARAM_USB] = "USB",
    [FT2_PARAM_CPU] = "CPU",
    [FT2_PARAM_RISCV] = "RISCV",
    [FT2_PARAM_GPIO] = "GPIO",
    [FT2_PARAM_DBG] = "",
};

uint8_t ft2_dbg = 0;
#define FT2_DBG(fmt, args...) if (ft2_dbg & 0x1) printf(fmt, ##args)
int do_ft2_dbg(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    if (argc == 1) {
        printf("ft2_dbg = %d\n", ft2_dbg);
    } else {
        ft2_dbg = simple_strtoul(argv[1], NULL, 10);
						    }
    return 0;
}

ft2_ret_t ft2_gpio_mux_set(uint32_t gpio_num)
{
    uint32_t gpio_group, gpio_pin;
    GLOBAL_GPIO_MUX_0_t gpio_mux_0;
    uint32_t addr;

    gpio_group = gpio_num / 32;
    gpio_pin = gpio_num % 32;
    addr = GLOBAL_GPIO_MUX_0 + gpio_group*4;
    FT2_DBG("%s: gpio_mux_addr = 0x%08x, gpio_group = %d, gpio_pin = %d\n", __func__, addr, gpio_group, gpio_pin);
    gpio_mux_0.wrd = CA_REG_READ(addr);
    gpio_mux_0.wrd |= (0x1 << gpio_pin);
    writel(gpio_mux_0.wrd, addr);

    return FT2_OK;
}

static int handshake(uint32_t ptr, uint32_t mask, uint32_t done, int usec)
{
    uint32_t result;

    do {
        result = readl(ptr);
        result &= mask;
        if (result == done)
            return 0;
        usec--;
        udelay(1);
    } while (usec > 0);

    return 1;
}

void test_prepare(ft2_record_t *records, int record_num)
{
    static int gpio_init = 0;
    int i;

    printf("*** test prepare ...\n\n");

    memset(records, 0, record_num * sizeof(ft2_record_t));

    for (i = 0; i < record_num; i++)
        (records + i)->name = ft2_module_name[i];
}

#define FT_TIME U_BOOT_DATE " - " U_BOOT_TIME
char time_string[] = FT_TIME;
void system_info(void)
{
    printf("### SYSTEM INFO:\n");

    printf("{Uboot Mercury %s}\n", time_string);

    /* get UUID */

    printf("{SW Ver: %d.%d.%d}\n", (TEST_PROG_VER >> 16) & 0xf, (TEST_PROG_VER>>8) & 0xf, TEST_PROG_VER & 0xff);

    return;
}

int check_chip_model_block(void)
{
    u8 *rbuf = (u8 *)0x05000000;
    uint32_t byte_index = 0x7ef;
    uint32_t byte_index2 = 0x7f1;
    uint32_t byte_index3 = 0x7f3;

    uint8_t model0_id0, model0_id1, model1_id0, model1_id1, model2_id0, model2_id1;

    OTP_READ_BYTE(byte_index, (unsigned long)&rbuf[0]);
    model0_id0 = rbuf[0];
    OTP_READ_BYTE(byte_index+1, (unsigned long)&rbuf[0]);
    model0_id1 = rbuf[0];

    OTP_READ_BYTE(byte_index2, (unsigned long)&rbuf[0]);
    model0_id0 = rbuf[0];
    OTP_READ_BYTE(byte_index2+1, (unsigned long)&rbuf[0]);
    model0_id1 = rbuf[0];

    OTP_READ_BYTE(byte_index3, (unsigned long)&rbuf[0]);
    model0_id0 = rbuf[0];
    OTP_READ_BYTE(byte_index3+1, (unsigned long)&rbuf[0]);
    model0_id1 = rbuf[0];

    if (model2_id0 != 0x0 || model2_id1 != 0x0) {
        printf("check_chip_model_block: 3\n");
        return 3;
    } else if (model1_id0 != 0x0 || model1_id1 != 0x0) {
        printf("check_chip_model_block: 2\n");
        return 2;
    } else if (model0_id0 != 0x0 || model0_id1 != 0x0) {
        printf("check_chip_model_block: 1\n");
        return 1;
    } else {
        printf("OTP not found\n");
        return -1;
    }
}

unsigned int check_bonding_block(void)
{
    u8 *rbuf = (u8 *)0x05000000;
    uint32_t bonding1 = 0x440;
    uint32_t bonding2 = 0x444;
    uint32_t bonding3 = 0x448;
    uint8_t bonding_result1, bonding_result2, bonding_result3;

    OTP_READ_BYTE(bonding1, (unsigned long)&rbuf[0]);
    bonding_result1 = rbuf[0];

    OTP_READ_BYTE(bonding2, (unsigned long)&rbuf[0]);
    bonding_result2 = rbuf[0];

    OTP_READ_BYTE(bonding3, (unsigned long)&rbuf[0]);
    bonding_result3 = rbuf[0];

    if (bonding_result3 != 0) {
        printf("bonding block 3\n");
        return 3;
    } else if (bonding_result2 != 0) {
        printf("bonding block 2\n");
        return 2;
    } else if (bonding_result1 != 0) {
        printf("bonding block 1\n");
        return 1;
    } else {
        printf("OTP not found\n");
        return -1;
    }
}

void read_all_otp(void)
{
    uint32_t chip_model = 0x7ef;
    uint32_t bonding = 0x440;
    uint32_t result;

    int model_block = check_chip_model_block();
    if (model_block == 1) {
        chip_model = 0x7f0;
    } else if (model_block == 2) {
        chip_model = 0x7f2;
    } else if (model_block == 3) {
        chip_model = 0x7f4;
    } else {
        printf("read_otp wrong block, use default OTP reg\n");
        chip_model = 0x7f0;
    }

    int bonding_block = check_bonding_block();
    if (bonding_block == 1) {
        bonding = 0x443;
    } else if (bonding_block == 2) {
        bonding = 0x447;
    } else if (bonding_block == 3) {
        bonding = 0x44b;
    } else {
        bonding = 0x443;
    }

}

int do_ft2_read_all_otp(cmd_tbl_t *cmdtp, int flag, int argc, char
*const argv[])
{
    read_all_otp();

    return 0;
}

void test_result(ft2_record_t *records, int record_num, uint32_t test)
{
    uint32_t module = 0;
    uint32_t result = 0;
    uint32_t error = 0;
    int i, j;

    for (i = 0; i < record_num; i++) {
        if ((records + i)->entry_mask)
            module++;
        else
            continue;
        if ((records + i)->module_rlt) {
            result |= (1 << i);
            error++;
        }
    }

    //system_info();
    printf("### TEST RESULT:\n");

    if (module > 0)
        printf("CHIP:%s\n", result ? "FAIL" : "PASS");

    printf("%s-T:0x%08X/E:0x%08X\n", CHIP_NAME, test, result);
    printf("=== Total %d %s %ld.%03ld secs\n", module, module > 1 ? "tests take" : "test takes", ft2_finish/1000, ft2_finish%1000);

    for (i = 0; i < record_num; i++) {
        if ((records + i)->entry_mask) {
            printf("{%s_%s}\n",
                (records + i)->module_rlt ? "FAIL" : "PASS",
                (records + i)->name);
        }

#if 0
	for (j = 0; j < MODULE_MAX_ENTRY; j++) {
            if ((records + i)->entry_mask & (1 << j)) {
                printf("%s-T:0x%08X-E:0x%08X\n",
                    ((records + i)->entry_name[j] == NULL)?(records + i)->name:(records + i)->entry_name[j],
                    (records + i)->entry_test[j],
                    (records + i)->entry_rlt[j]);
            }
        }
#endif

        if ((records + i)->entry_mask) {
            printf("=== %s test takes %ld.%03d secs\n",
                (records + i)->name,
                (records + i)->finish/1000,
                (int)(records + i)->finish%1000);
        }
    }

    if (module > 0) {
        if (error)
            printf("%d Failure\n", error);
        else
            printf("No Failure\n");
    }
}

ft2_ret_t ft2_prbs_pon_test(ft2_record_t *record)
{
    ft2_ret_t rc = FT2_OK;

#if (PRBS_PON_MASK > 0)

    printf("FT2_PARAM_PRBS_PON test ...\n");

    record->entry_mask = PRBS_PON_MASK;
    record->start = get_timer(0);
    record->entry_test[0] |= 1;


    if (!record->module_rlt)
        record->module_rlt = rc;
    record->entry_rlt[0] = rc ? 1 : 0;
    record->finish = get_timer(record->start) * 1000 / CONFIG_SYS_HZ;

    printf("%s\n", rc ? "FAIL" : "PASS");
    FT2_DBG("test takes %ld.%03d secs\n", record->finish/1000, (int)(record->finish%1000));

    return rc;
#else
    printf("No PRBS PON test\n");
    return FT2_PRBS_PON;
#endif
}

ft2_ret_t ft2_prbs_pon_result(ft2_record_t *record)
{
    ft2_ret_t rc = FT2_OK;

#if (PRBS_PON_MASK > 0)

    printf("FT2_PARAM_PRBS_PON result ...\n");

    record->entry_mask = PRBS_PON_MASK;
    record->start = get_timer(0);
    record->entry_test[0] |= 1;

    printf("*************\n");

    if (!record->module_rlt)
        record->module_rlt = rc;
    record->entry_rlt[0] = rc ? 1 : 0;
    record->finish = get_timer(record->start) * 1000 / CONFIG_SYS_HZ;

    printf("%s\n", rc ? "FAIL" : "PASS");
    FT2_DBG("test takes %ld.%03d secs\n", record->finish/1000, (int)(record->finish%1000));

    return rc;
#else
    printf("No PRBS PON test\n");
    return FT2_PRBS_PON;
#endif
}

int do_ft2_prbs_pon_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    test_prepare(ft2_records, FT2_PARAM_MAX);
    ft2_prbs_pon_test(&ft2_records[FT2_PARAM_PRBS_PON]);
    ft2_prbs_pon_result(&ft2_records[FT2_PARAM_PRBS_PON_RESULT]);
    test_result(ft2_records, FT2_PARAM_MAX, (1 << FT2_PARAM_PRBS_PON));
    return 0;
}

ft2_ret_t ft2_pon_ben_test(ft2_record_t *record)
{
    ft2_ret_t rc = FT2_OK;
    uint8_t value, value1;

#if (PON_BEN_MASK > 0)

    printf("FT2_PARAM_PON_BEN result ...\n");

    record->entry_mask = PON_BEN_MASK;
    record->start = get_timer(0);
    record->entry_test[0] |= 1;

    gpio_request(GPIO_PON_BEN, "NULL");//PADS_BEN_OUT
    //default all high
    //value = readl(PER_GPIO4_CFG);
    //value |= 0x80;
    //writel(value, PER_GPIO4_CFG);

    /* set gpio mux */
    ft2_gpio_mux_set(GPIO_PON_BEN);
    GLOBAL_PON_CNTL_t g_pon_cntl;
    GLOBAL_EPON_CNTL_t g_epon_cntl;
    PSDS_MODE_t psds_mode;
    EPON_EPON_GLB_STATIC_CFG_t epon_glb_static_cfg;
    PSDS_DEBUG_CTRL_t psds_dbg;

    g_pon_cntl.wrd = readl(GLOBAL_PON_CNTL);
    g_pon_cntl.bf.epon_mode = 0x1;
    g_pon_cntl.bf.pon_serdes_rst_n = 0x1;
    g_pon_cntl.bf.ptp_rst_n = 0x1;
    g_pon_cntl.bf.puc_reset = 0x1;
    g_pon_cntl.bf.pdc_reset = 0x1;
    g_pon_cntl.bf.POW_PCIX  = 0x1;
    g_pon_cntl.bf.psds_wtg_reg_reset_n = 0x1;
    writel(g_pon_cntl.wrd, GLOBAL_PON_CNTL);
    FT2_DBG("GLOBAL_PON_CNTL: %x %x\n", GLOBAL_PON_CNTL, CA_REG_READ(GLOBAL_PON_CNTL));

    g_epon_cntl.wrd = readl(GLOBAL_EPON_CNTL);
    g_epon_cntl.bf.epon_onu_mode = 0x3;
    writel(g_epon_cntl.wrd, GLOBAL_EPON_CNTL);
    FT2_DBG("GLOBAL_EPON_CNTL: %x %x\n", GLOBAL_EPON_CNTL, CA_REG_READ(GLOBAL_EPON_CNTL));

    g_pon_cntl.wrd = readl(GLOBAL_PON_CNTL);
    g_pon_cntl.bf.psds_reg_rst_n = 0x0;
    writel(g_pon_cntl.wrd, GLOBAL_PON_CNTL);
    mdelay(20);
    FT2_DBG("GLOBAL_PON_CNTL: %x %x\n", GLOBAL_PON_CNTL, CA_REG_READ(GLOBAL_PON_CNTL));

    g_pon_cntl.wrd = readl(GLOBAL_PON_CNTL);
    g_pon_cntl.bf.psds_reg_rst_n = 0x1;
    writel(g_pon_cntl.wrd, GLOBAL_PON_CNTL);
    mdelay(20);
    FT2_DBG("GLOBAL_PON_CNTL: %x %x\n", GLOBAL_PON_CNTL, CA_REG_READ(GLOBAL_PON_CNTL));

    psds_mode.wrd = readl(PSDS_MODE);
    psds_mode.bf.sds_mode_s0 = 0xc;
    psds_mode.bf.usx_sub_mx_s0 = 0x3;
    writel(psds_mode.wrd, PSDS_MODE);

    //EPON 1.25G mode 20220310 by kent_hsu
    writel(0x40c, PSDS_MODE);
    FT2_DBG("PSDS_MODE: %x %x\n", PSDS_MODE, readl(PSDS_MODE));

    /* Turn on BEN output */
    writel(0x30, GLOBAL_PSDS_INIT_CNTL);
    udelay(10000);
    FT2_DBG("GLOBAL_PSDS_INIT_CNTL: %x %x\n", GLOBAL_PSDS_INIT_CNTL, readl(GLOBAL_PSDS_INIT_CNTL));

    //before access laser_tx_polarity need to enable GLOBAL_EPON_CNTL bit[3:0]
    g_epon_cntl.wrd = readl(GLOBAL_EPON_CNTL);
    g_epon_cntl.bf.epon_rst_n = 0x1;
    g_epon_cntl.bf.onumpcp_rst_n = 0x1;
    g_epon_cntl.bf.onugepn_rst_n = 0x1;
    g_epon_cntl.bf.xgepn_rst_n = 0x1;
    writel(g_epon_cntl.wrd, GLOBAL_EPON_CNTL);
    mdelay(20);
    FT2_DBG("GLOBAL_EPON_CNTL: %x %x\n", GLOBAL_EPON_CNTL, readl(GLOBAL_EPON_CNTL));

    FT2_DBG("PSDS_DEBUG_CTRL: %x %x\n", PSDS_DEBUG_CTRL, readl(PSDS_DEBUG_CTRL));
    //before access laser_tx_polarity need to enable GLOBAL_EPON_CNTL bit[3:0]
    psds_dbg.wrd = readl(PSDS_DEBUG_CTRL);
    psds_dbg.bf.sel = 0x8;
    writel(psds_dbg.wrd, PSDS_DEBUG_CTRL);
    mdelay(20);
    FT2_DBG("PSDS_DEBUG_CTRL: %x %x\n", PSDS_DEBUG_CTRL, readl(PSDS_DEBUG_CTRL));

    gpio_direction_input(GPIO_PON_BEN);
    value = gpio_get_value(GPIO_PON_BEN);
    //value = readl(PER_GPIO4_IN);
    FT2_DBG("%s: Turn on BEN output, value = %d\n", __func__, value);

    writel(0xffffffff, PSDS_PON_PRBS_BEN_H);
    udelay(10000);
    FT2_DBG("PSDS_PON_PRBS_BEN_H: %x %x\n", PSDS_PON_PRBS_BEN_H, readl(PSDS_PON_PRBS_BEN_H));

    gpio_direction_input(GPIO_PON_BEN);
    value = gpio_get_value(GPIO_PON_BEN);
    //value = readl(PER_GPIO4_IN);
    FT2_DBG("%s: Change BEN, value = %d\n", __func__, value);
    writel(0x0, PSDS_PON_PRBS_BEN_H);
    udelay(10000);
    FT2_DBG("PSDS_PON_PRBS_BEN_H: %x %x\n", PSDS_PON_PRBS_BEN_H, readl(PSDS_PON_PRBS_BEN_H));

    gpio_direction_input(GPIO_PON_BEN);
    value1 = gpio_get_value(GPIO_PON_BEN);
    //value1 = readl(PER_GPIO4_IN);
    if (value == value1) {
        FT2_DBG("%s: Change BEN again, value1 = %d\n", __func__, value1);
        rc = FT2_PON_BEN;
        goto End;
    } else {
        FT2_DBG("%s: value != value1, value1 = %d\n", __func__, value1);
    }

End:
    record->finish = get_timer(record->start) * 1000 / CONFIG_SYS_HZ;
    record->module_rlt = rc;
    record->entry_rlt[0] = rc ? 1 : 0;

    printf("%s\n", rc ? "FAIL" : "PASS");
    FT2_DBG("test takes %ld.%03d secs\n", record->finish/1000, (int)(record->finish%1000));

    return rc;
#else
    printf("No PON BEN test\n");
    return FT2_PON_BEN;
#endif
}

int do_ft2_pon_ben_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    test_prepare(ft2_records, FT2_PARAM_MAX);
    ft2_pon_ben_test(&ft2_records[FT2_PARAM_PON_BEN]);
    test_result(ft2_records, FT2_PARAM_MAX, (1 << FT2_PARAM_PON_BEN));
    return 0;
}

void reset_control_assert(void)
{
    GLOBAL_BLOCK_RESET1_t g_blk_reset1;
    GLOBAL_DPHY_RESET_t g_dphy_reset;

    //GLOBAL_BLOCK_RESET1 0xf4320074
    g_blk_reset1.wrd = readl(GLOBAL_BLOCK_RESET1);
    g_blk_reset1.bf.reset_pcie0 = 1;
    g_blk_reset1.bf.reset_pcie1 = 1;
    g_blk_reset1.bf.reset_pcie2 = 1;
    g_blk_reset1.bf.reset_pcie7 = 1;
    g_blk_reset1.bf.reset_pcie8 = 1;
    writel(g_blk_reset1.wrd, GLOBAL_BLOCK_RESET1);
    mdelay(10);

    //GLOBAL_DPHY_RESET 0xf432007c
    g_dphy_reset.wrd = readl(GLOBAL_DPHY_RESET);
    g_dphy_reset.bf.reset_dphy_s0_pcie = 1;
    g_dphy_reset.bf.reset_dphy_s1_pcie = 1;
    g_dphy_reset.bf.reset_dphy_s2_pcie = 1;
    g_dphy_reset.bf.reset_dphy_s7_pcie = 1;
    g_dphy_reset.bf.reset_dphy_s8_pcie = 1;
    writel(g_dphy_reset.wrd, GLOBAL_DPHY_RESET);
    mdelay(10);
}

void reset_control_deassert(void)
{
    GLOBAL_BLOCK_RESET1_t g_blk_reset1;
    GLOBAL_DPHY_RESET_t g_dphy_reset;

    //GLOBAL_BLOCK_RESET1 0xf4320074
    g_blk_reset1.wrd = readl(GLOBAL_BLOCK_RESET1);
    g_blk_reset1.bf.reset_pcie0 = 0;
    g_blk_reset1.bf.reset_pcie1 = 0;
    g_blk_reset1.bf.reset_pcie2 = 0;
    g_blk_reset1.bf.reset_pcie7 = 0;
    g_blk_reset1.bf.reset_pcie8 = 0;
    writel(g_blk_reset1.wrd, GLOBAL_BLOCK_RESET1);
    mdelay(10);

    //GLOBAL_DPHY_RESET 0xf432007c
    g_dphy_reset.wrd = readl(GLOBAL_DPHY_RESET);
    g_dphy_reset.bf.reset_dphy_s0_pcie = 0;
    g_dphy_reset.bf.reset_dphy_s1_pcie = 0;
    g_dphy_reset.bf.reset_dphy_s2_pcie = 0;
    g_dphy_reset.bf.reset_dphy_s7_pcie = 0;
    g_dphy_reset.bf.reset_dphy_s8_pcie = 0;
    writel(g_dphy_reset.wrd, GLOBAL_DPHY_RESET);
    mdelay(10);
}

void pcie_phy_power_on(void)
{
    GLOBAL_PHY_CONTROL_A_t g_phy_ctrl_a;
    GLOBAL_PHY_ISO_POWER_CTRL_t g_phy_pow_ctrl;

    //GLOBAL_PHY_CONTROL_A 0xf43200b8
    g_phy_ctrl_a.wrd = readl(GLOBAL_PHY_CONTROL_A);
    g_phy_ctrl_a.bf.s0_pipe_phymode = 0x0;
    g_phy_ctrl_a.bf.s1_pipe_phymode = 0x0;
    g_phy_ctrl_a.bf.s2_pcie_pipe_phymode = 0x0;
    g_phy_ctrl_a.bf.s2_pcie_usb3_sfi_sel = 0;
    g_phy_ctrl_a.bf.s2_pcie_usb3_sel = 0;
    g_phy_ctrl_a.bf.s7_pcie_usb3_sgmii_sel = 0x0;
    g_phy_ctrl_a.bf.s7_pcie_usb3_sel = 0;
    g_phy_ctrl_a.bf.s8_pcie_usb3_sgmii_sel = 0x0;
    g_phy_ctrl_a.bf.s8_pcie_usb3_sel = 0;
    writel(g_phy_ctrl_a.wrd, GLOBAL_PHY_CONTROL_A);

    //GLOBAL_PHY_ISO_POWER_CTRL 0xf43203e0
    g_phy_pow_ctrl.wrd = readl(GLOBAL_PHY_ISO_POWER_CTRL);
    g_phy_pow_ctrl.bf.s2_POW_PCIE = 1;
    g_phy_pow_ctrl.bf.s2_POW_USB3 = 0;
    g_phy_pow_ctrl.bf.s2_CPHY_ISOLATE = 0;

    g_phy_pow_ctrl.bf.s7_POW_PCIE = 1;
    g_phy_pow_ctrl.bf.s7_POW_USB3 = 0;
    g_phy_pow_ctrl.bf.s7_CPHY_ISOLATE = 0;
    g_phy_pow_ctrl.bf.s7_ISO_ANA_B_V09 = 1;

    g_phy_pow_ctrl.bf.s8_POW_PCIE = 1;
    g_phy_pow_ctrl.bf.s8_POW_USB3 = 0;
    g_phy_pow_ctrl.bf.s8_CPHY_ISOLATE = 0;
    g_phy_pow_ctrl.bf.s8_ISO_ANA_B_V09 = 1;
    writel(g_phy_pow_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);
    mdelay(10);

}

void pcie_phy_power_off(void)
{
    GLOBAL_PHY_CONTROL_A_t g_phy_ctrl_a;
    GLOBAL_PHY_ISO_POWER_CTRL_t g_phy_pow_ctrl;

    //GLOBAL_PHY_CONTROL_A 0xf43200b8
    g_phy_ctrl_a.wrd = readl(GLOBAL_PHY_CONTROL_A);
    g_phy_ctrl_a.bf.s0_pipe_phymode = 0x1;
    g_phy_ctrl_a.bf.s1_pipe_phymode = 0x1;
    g_phy_ctrl_a.bf.s2_pcie_pipe_phymode = 0x1;
    g_phy_ctrl_a.bf.s2_pcie_usb3_sfi_sel = 1;
    g_phy_ctrl_a.bf.s2_pcie_usb3_sel = 1;
    g_phy_ctrl_a.bf.s7_pcie_usb3_sgmii_sel = 0x1;
    g_phy_ctrl_a.bf.s7_pcie_usb3_sel = 1;
    g_phy_ctrl_a.bf.s8_pcie_usb3_sgmii_sel = 0x1;
    g_phy_ctrl_a.bf.s8_pcie_usb3_sel = 1;
    writel(g_phy_ctrl_a.wrd, GLOBAL_PHY_CONTROL_A);

    //GLOBAL_PHY_ISO_POWER_CTRL 0xf43203e0
    g_phy_pow_ctrl.wrd = readl(GLOBAL_PHY_ISO_POWER_CTRL);
    g_phy_pow_ctrl.bf.s2_POW_PCIE = 0;
    g_phy_pow_ctrl.bf.s2_POW_USB3 = 1;
    g_phy_pow_ctrl.bf.s2_CPHY_ISOLATE = 1;

    g_phy_pow_ctrl.bf.s7_POW_PCIE = 0;
    g_phy_pow_ctrl.bf.s7_POW_USB3 = 1;
    g_phy_pow_ctrl.bf.s7_CPHY_ISOLATE = 1;
    g_phy_pow_ctrl.bf.s7_ISO_ANA_B_V09 = 0;

    g_phy_pow_ctrl.bf.s8_POW_PCIE = 0;
    g_phy_pow_ctrl.bf.s8_POW_USB3 = 1;
    g_phy_pow_ctrl.bf.s8_CPHY_ISOLATE = 1;
    g_phy_pow_ctrl.bf.s8_ISO_ANA_B_V09 = 0;
    writel(g_phy_pow_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);
    mdelay(10);

}

void pcie0_phy_parameter(void)
{
    writel(0x000008f0, 0xf47070B8);
    writel(0x000040c1, 0xf4708180);
    writel(0x000040c1, 0xf4709180);
    writel(0x000040c1, 0xf470A180);
    writel(0x000040c1, 0xf470B180);
    writel(0x000040c1, 0xf4708580);
    writel(0x000040c1, 0xf4709580);
    writel(0x000040c1, 0xf470A580);
    writel(0x000040c1, 0xf470B580);
    writel(0x0000ffd0, 0xf47080C0);
    writel(0x0000ffd0, 0xf47090C0);
    writel(0x0000ffd0, 0xf470A0C0);
    writel(0x0000ffd0, 0xf470B0C0);
    writel(0x0000ffd0, 0xf47084C0);
    writel(0x0000ffd0, 0xf47094C0);
    writel(0x0000ffd0, 0xf470A4C0);
    writel(0x0000ffd0, 0xf470B4C0);
    writel(0x00000100, 0xf4708090);
    writel(0x00000100, 0xf4709090);
    writel(0x00000100, 0xf470A090);
    writel(0x00000100, 0xf470B090);
    writel(0x00000100, 0xf4708490);
    writel(0x00000100, 0xf4709490);
    writel(0x00000100, 0xf470A490);
    writel(0x00000100, 0xf470B490);
    writel(0x00000002, 0xf470011C);
    writel(0x00000002, 0xf470051C);
    writel(0x00000002, 0xf470091C);
    writel(0x00000002, 0xf4700D1C);
    writel(0x00000002, 0xf47001DC);
    writel(0x00000002, 0xf47005DC);
    writel(0x00000002, 0xf47009DC);
    writel(0x00000002, 0xf4700DDC);
    writel(0x00004001, 0xf4700124);
    writel(0x00004001, 0xf4700524);
    writel(0x00004001, 0xf4700924);
    writel(0x00004001, 0xf4700D24);
    writel(0x00004001, 0xf47001E4);
    writel(0x00004001, 0xf47005E4);
    writel(0x00004001, 0xf47009E4);
    writel(0x00004001, 0xf4700DE4);
    writel(0x00000038, 0xf470012C);
    writel(0x00000038, 0xf470052C);
    writel(0x00000038, 0xf470092C);
    writel(0x00000038, 0xf4700D2C);
    writel(0x00000038, 0xf47001EC);
    writel(0x00000038, 0xf47005EC);
    writel(0x00000038, 0xf47009EC);
    writel(0x00000038, 0xf4700DEC);
    writel(0x00003000, 0xf4708028);
    writel(0x00002000, 0xf4709028);
    writel(0x00002000, 0xf470A028);
    writel(0x00002000, 0xf470B028);
    writel(0x00003000, 0xf4708428);
    writel(0x00002000, 0xf4709428);
    writel(0x00002000, 0xf470A428);
    writel(0x00002000, 0xf470B428);
    writel(0x0000fecf, 0xf4708130);
    writel(0x0000fecf, 0xf4709130);
    writel(0x0000fecf, 0xf470A130);
    writel(0x0000fecf, 0xf470B130);
    writel(0x0000fecf, 0xf4708530);
    writel(0x0000fecf, 0xf4709530);
    writel(0x0000fecf, 0xf470A530);
    writel(0x0000fecf, 0xf470B530);
    writel(0x0000aff0, 0xf47080D0);
    writel(0x0000aff0, 0xf47090D0);
    writel(0x0000aff0, 0xf470A0D0);
    writel(0x00009840, 0xf470B0D0);
    writel(0x0000aff0, 0xf47084D0);
    writel(0x0000aff0, 0xf47094D0);
    writel(0x0000aff0, 0xf470A4D0);
    writel(0x00009840, 0xf470B4D0);
    writel(0x0000079e, 0xf4708150);
    writel(0x0000079e, 0xf4709150);
    writel(0x0000079e, 0xf470A150);
    writel(0x0000079e, 0xf470B150);
    writel(0x0000079e, 0xf4708550);
    writel(0x0000079e, 0xf4709550);
    writel(0x0000079e, 0xf470A550);
    writel(0x0000079e, 0xf470B550);
    writel(0x00008038, 0xf47081C8);
    writel(0x00008038, 0xf47091C8);
    writel(0x00008038, 0xf470A1C8);
    writel(0x00008038, 0xf470B1C8);
    writel(0x00008038, 0xf47085C8);
    writel(0x00008038, 0xf47095C8);
    writel(0x00008038, 0xf470A5C8);
    writel(0x00008038, 0xf470B5C8);
    writel(0x00003f20, 0xf47080A8);
    writel(0x00003f20, 0xf47090A8);
    writel(0x00003f20, 0xf470A0A8);
    writel(0x00003f20, 0xf470B0A8);
    writel(0x00003f20, 0xf47084A8);
    writel(0x00003f20, 0xf47094A8);
    writel(0x00003f20, 0xf470A4A8);
    writel(0x00003f20, 0xf470B4A8);
    writel(0x00006002, 0xf4708248);
    writel(0x00006002, 0xf4709248);
    writel(0x00002002, 0xf470A248);
    writel(0x00002002, 0xf470B248);
    writel(0x00006002, 0xf4708648);
    writel(0x00006002, 0xf4709648);
    writel(0x00002002, 0xf470A648);
    writel(0x00002002, 0xf470B648);
    writel(0x00008008, 0xf4708250);
    writel(0x00008008, 0xf4709250);
    writel(0x00008008, 0xf470A250);
    writel(0x00008008, 0xf470B250);
    writel(0x00008008, 0xf4708650);
    writel(0x00008008, 0xf4709650);
    writel(0x00008008, 0xf470A650);
    writel(0x00008008, 0xf470B650);
    writel(0x00000c49, 0xf4708268);
    writel(0x00000c49, 0xf4709268);
    writel(0x00000e49, 0xf470A268);
    writel(0x00000e49, 0xf470B268);
    writel(0x00000c49, 0xf4708668);
    writel(0x00000c49, 0xf4709668);
    writel(0x00000e49, 0xf470A668);
    writel(0x00000e49, 0xf470B668);
    writel(0x00007800, 0xf4708110);
    writel(0x00007800, 0xf4709110);
    writel(0x00007800, 0xf470A110);
    writel(0x00007800, 0xf470B110);
    writel(0x00007800, 0xf4708510);
    writel(0x00007800, 0xf4709510);
    writel(0x00007800, 0xf470A510);
    writel(0x00007800, 0xf470B510);
    writel(0x00007800, 0xf4708118);
    writel(0x00007800, 0xf4709118);
    writel(0x00007800, 0xf470A118);
    writel(0x00007800, 0xf470B118);
    writel(0x00007800, 0xf4708518);
    writel(0x00007800, 0xf4709518);
    writel(0x00007800, 0xf470A518);
    writel(0x00007800, 0xf470B518);
    writel(0x00000320, 0xf4708120);
    writel(0x00000320, 0xf4709120);
    writel(0x00000320, 0xf470A120);
    writel(0x00000320, 0xf470B120);
    writel(0x00000320, 0xf4708520);
    writel(0x00000320, 0xf4709520);
    writel(0x00000320, 0xf470A520);
    writel(0x00000320, 0xf470B520);
    writel(0x00000320, 0xf4708128);
    writel(0x00000320, 0xf4709128);
    writel(0x00000320, 0xf470A128);
    writel(0x00000320, 0xf470B128);
    writel(0x00000320, 0xf4708528);
    writel(0x00000320, 0xf4709528);
    writel(0x00000320, 0xf470A528);
    writel(0x00000320, 0xf470B528);
    writel(0x000080a6, 0xf4708148);
    writel(0x000080a6, 0xf4709148);
    writel(0x000080a6, 0xf470A148);
    writel(0x000080a6, 0xf470B148);
    writel(0x000080a6, 0xf4708548);
    writel(0x000080a6, 0xf4709548);
    writel(0x000080a6, 0xf470A548);
    writel(0x000080a6, 0xf470B548);
    writel(0x00000000, 0xf4708140);
    writel(0x00000000, 0xf4709140);
    writel(0x00000000, 0xf470A140);
    writel(0x00003980, 0xf470B140);
    writel(0x00000000, 0xf4708540);
    writel(0x00000000, 0xf4709540);
    writel(0x00000000, 0xf470A540);
    writel(0x00003980, 0xf470B540);
    writel(0x00000000, 0xf4708178);
    writel(0x00000000, 0xf4709178);
    writel(0x00000000, 0xf470A178);
    writel(0x00000000, 0xf470B178);
    writel(0x00000000, 0xf4708578);
    writel(0x00000000, 0xf4709578);
    writel(0x00000000, 0xf470A578);
    writel(0x00000000, 0xf470B578);
    writel(0x00000010, 0xf4708108);
    writel(0x00000010, 0xf4709108);
    writel(0x00000010, 0xf470A108);
    writel(0x00000010, 0xf470B108);
    writel(0x00000010, 0xf4708508);
    writel(0x00000010, 0xf4709508);
    writel(0x00000010, 0xf470A508);
    writel(0x00000010, 0xf470B508);
    writel(0x00000002, 0xf4708160);
    writel(0x00000002, 0xf4709160);
    writel(0x00000002, 0xf470A160);
    writel(0x00000002, 0xf470B160);
    writel(0x00000002, 0xf4708560);
    writel(0x00000002, 0xf4709560);
    writel(0x00000002, 0xf470A560);
    writel(0x00000002, 0xf470B560);
    writel(0x00000000, 0xf4708168);
    writel(0x00000000, 0xf4709168);
    writel(0x00000000, 0xf470A168);
    writel(0x00000000, 0xf470B168);
    writel(0x00000000, 0xf4708568);
    writel(0x00000000, 0xf4709568);
    writel(0x00000000, 0xf470A568);
    writel(0x00000000, 0xf470B568);
    writel(0x000000ff, 0xf47081E0);
    writel(0x000000ff, 0xf47091E0);
    writel(0x000000ff, 0xf470A1E0);
    writel(0x000000ff, 0xf470B1E0);
    writel(0x000000ff, 0xf47085E0);
    writel(0x000000ff, 0xf47095E0);
    writel(0x000000ff, 0xf470A5E0);
    writel(0x000000ff, 0xf470B5E0);
    writel(0x000000ff, 0xf47081F0);
    writel(0x000000ff, 0xf47091F0);
    writel(0x000000ff, 0xf470A1F0);
    writel(0x000000ff, 0xf470B1F0);
    writel(0x000000ff, 0xf47085F0);
    writel(0x000000ff, 0xf47095F0);
    writel(0x000000ff, 0xf470A5F0);
    writel(0x000000ff, 0xf470B5F0);
    writel(0x00002000, 0xf4708260);
    writel(0x00002000, 0xf4709260);
    writel(0x00002000, 0xf470A260);
    writel(0x00002000, 0xf470B260);
    writel(0x00002000, 0xf4708660);
    writel(0x00002000, 0xf4709660);
    writel(0x00002000, 0xf470A660);
    writel(0x00002000, 0xf470B660);
    writel(0x00000064, 0xf4708220);
    writel(0x00000064, 0xf4709220);
    writel(0x00000064, 0xf470A220);
    writel(0x00000064, 0xf470B220);
    writel(0x00000064, 0xf4708620);
    writel(0x00000064, 0xf4709620);
    writel(0x00000064, 0xf470A620);
    writel(0x00000064, 0xf470B620);
    writel(0x00002700, 0xf4708230);
    writel(0x00002700, 0xf4709230);
    writel(0x00003E70, 0xf470A230);
    writel(0x00003E70, 0xf470B230);
    writel(0x00002700, 0xf4708630);
    writel(0x00002700, 0xf4709630);
    writel(0x00003E70, 0xf470A630);
    writel(0x00003E70, 0xf470B630);
    writel(0x00002710, 0xf4708228);
    writel(0x00002710, 0xf4709228);
    writel(0x00003E80, 0xf470A228);
    writel(0x00003E80, 0xf470B228);
    writel(0x00002710, 0xf4708628);
    writel(0x00002710, 0xf4709628);
    writel(0x00003E80, 0xf470A628);
    writel(0x00003E80, 0xf470B628);
    writel(0x00000fff, 0xf470C000);
    writel(0x00000fff, 0xf470D000);
    writel(0x00000fff, 0xf470E000);
    writel(0x00000fff, 0xf470F000);
    writel(0x00006780, 0xf4708270);
    writel(0x00006780, 0xf4709270);
    writel(0x00006780, 0xf470A270);
    writel(0x00006780, 0xf470B270);
    writel(0x00006780, 0xf4708670);
    writel(0x00006780, 0xf4709670);
    writel(0x00006780, 0xf470A670);
    writel(0x00006780, 0xf470B670);
    writel(0x00006000, 0xf47081B8);
    writel(0x00006000, 0xf47091B8);
    writel(0x00006000, 0xf470A1B8);
    writel(0x00006000, 0xf470B1B8);
    writel(0x00006000, 0xf47085B8);
    writel(0x00006000, 0xf47095B8);
    writel(0x00006000, 0xf470A5B8);
    writel(0x00006000, 0xf470B5B8);
    writel(0x0000c970, 0xf4700058);
    writel(0x0000c970, 0xf4700458);
    writel(0x0000c970, 0xf4700858);
    writel(0x0000c970, 0xf4700C58);
    writel(0x0000c10d, 0xf4700044);
    writel(0x0000c10d, 0xf4700444);
    writel(0x0000c10d, 0xf4700844);
    writel(0x0000c10c, 0xf4700C44);
    writel(0x0000d2f0, 0xf4700068);
    writel(0x0000d2f0, 0xf4700468);
    writel(0x000052f0, 0xf4700868);
    writel(0x000052f0, 0xf4700C68);
    writel(0x00005c0f, 0xf47000C8);
    writel(0x00006c0f, 0xf47004C8);
    writel(0x00007c0f, 0xf47008C8);
    writel(0x00007c0f, 0xf4700CC8);
    writel(0x00005c0f, 0xf4700188);
    writel(0x00006c0f, 0xf4700588);
    writel(0x00007c0f, 0xf4700988);
    writel(0x00007c0f, 0xf4700D88);
    writel(0x00000240, 0xf47000DC);
    writel(0x00000240, 0xf47004DC);
    writel(0x00000240, 0xf47008DC);
    writel(0x00000240, 0xf4700CDC);
    writel(0x00000240, 0xf470019C);
    writel(0x00000240, 0xf470059C);
    writel(0x00000240, 0xf470099C);
    writel(0x00000240, 0xf4700D9C);
    writel(0x00000600, 0xf47000C0);
    writel(0x00000600, 0xf47004C0);
    writel(0x00000e00, 0xf47008C0);
    writel(0x00000e00, 0xf4700CC0);
    writel(0x00000600, 0xf4700180);
    writel(0x00000600, 0xf4700580);
    writel(0x00000e00, 0xf4700980);
    writel(0x00000e00, 0xf4700D80);
    writel(0x0000002b, 0xf470C118);
    writel(0x0000002b, 0xf470D118);
    writel(0x0000002b, 0xf470E118);
    writel(0x0000002b, 0xf470F118);
    writel(0x0000f104, 0xf4707218);
    writel(0x0000f145, 0xf4707258);
    writel(0x0000e1c7, 0xf4707260);
    writel(0x00009980, 0xf4707280);
    writel(0x0000d880, 0xf47072C0);
    writel(0x00008044, 0xf470016C);
    writel(0x00008044, 0xf470056C);
    writel(0x00008044, 0xf470096C);
    writel(0x0000804c, 0xf4700D6C);
    writel(0x00008044, 0xf470022C);
    writel(0x00008044, 0xf470062C);
    writel(0x00008044, 0xf4700A2C);
    writel(0x0000804c, 0xf4700E2C);
    writel(0x00008e7c, 0xf4700174);
    writel(0x00008e7c, 0xf4700574);
    writel(0x00008e7c, 0xf4700974);
    writel(0x00008e7c, 0xf4700D74);
    writel(0x00008e7c, 0xf4700234);
    writel(0x00008e7c, 0xf4700634);
    writel(0x00008e7c, 0xf4700A34);
    writel(0x00008e7c, 0xf4700E34);
    writel(0x00008000, 0xf470015C);
    writel(0x00008000, 0xf470055C);
    writel(0x00001000, 0xf470095C);
    writel(0x00001000, 0xf4700D5C);
    writel(0x00008000, 0xf470021C);
    writel(0x00008000, 0xf470061C);
    writel(0x00001000, 0xf4700A1C);
    writel(0x00001000, 0xf4700E1C);
    writel(0x000001e4, 0xf4700104);
    writel(0x000001e4, 0xf4700504);
    writel(0x000000a4, 0xf4700904);
    writel(0x000000a4, 0xf4700D04);
    writel(0x000001e4, 0xf47001C4);
    writel(0x000001e4, 0xf47005C4);
    writel(0x000000a4, 0xf47009C4);
    writel(0x000000a4, 0xf4700DC4);
    writel(0x00000744, 0xf4700138);
    writel(0x00000044, 0xf4700538);
    writel(0x00000043, 0xf4700938);
    writel(0x00000043, 0xf4700D38);
    writel(0x00000744, 0xf47001F8);
    writel(0x00000044, 0xf47005F8);
    writel(0x00000043, 0xf47009F8);
    writel(0x00000043, 0xf4700DF8);
    writel(0x00008864, 0xf4700170);
    writel(0x00008864, 0xf4700570);
    writel(0x00008864, 0xf4700970);
    writel(0x00008864, 0xf4700D70);
    writel(0x00008864, 0xf4700230);
    writel(0x00008864, 0xf4700630);
    writel(0x00008864, 0xf4700A30);
    writel(0x00008864, 0xf4700E30);
    writel(0x00005680, 0xf4700164);
    writel(0x00005680, 0xf4700564);
    writel(0x00005a82, 0xf4700964);
    writel(0x00005a82, 0xf4700D64);
    writel(0x00005680, 0xf4700224);
    writel(0x00005680, 0xf4700624);
    writel(0x00005a82, 0xf4700A24);
    writel(0x00005a82, 0xf4700E24);
    writel(0x0000000d, 0xf47080B0);
    writel(0x0000000d, 0xf47090B0);
    writel(0x0000000d, 0xf470A0B0);
    writel(0x0000000d, 0xf470B0B0);
    writel(0x0000000d, 0xf47084B0);
    writel(0x0000000d, 0xf47094B0);
    writel(0x0000000d, 0xf470A4B0);
    writel(0x0000000d, 0xf470B4B0);
    writel(0x000001ff, 0xf4708258);
    writel(0x000001ff, 0xf4709258);
    writel(0x000001ff, 0xf470A258);
    writel(0x000001ff, 0xf470B258);
    writel(0x000001ff, 0xf4708658);
    writel(0x000001ff, 0xf4709658);
    writel(0x000001ff, 0xf470A658);
    writel(0x000001ff, 0xf470B658);
    writel(0x00000003, 0xf4708210);
    writel(0x00000003, 0xf4709210);
    writel(0x00000003, 0xf470A210);
    writel(0x00000003, 0xf470B210);
    writel(0x00000003, 0xf4708610);
    writel(0x00000003, 0xf4709610);
    writel(0x00000003, 0xf470A610);
    writel(0x00000003, 0xf470B610);
    writel(0x00000007, 0xf4700108);
    writel(0x00000006, 0xf4700508);
    writel(0x00000004, 0xf4700908);
    writel(0x00000000, 0xf4700D08);
    writel(0x00000007, 0xf47001C8);
    writel(0x00000006, 0xf47005C8);
    writel(0x00000004, 0xf47009C8);
    writel(0x00000000, 0xf4700DC8);
    writel(0x000011e0, 0xf47080A0);
    writel(0x000011e0, 0xf47090A0);
    writel(0x000011e0, 0xf470A0A0);
    writel(0x000011e0, 0xf470B0A0);
    writel(0x000011e0, 0xf47084A0);
    writel(0x000011e0, 0xf47094A0);
    writel(0x000011e0, 0xf470A4A0);
    writel(0x000011e0, 0xf470B4A0);
    writel(0x00000200, 0xf4708030);
    writel(0x00000200, 0xf4709030);
    writel(0x00000200, 0xf470A030);
    writel(0x00000200, 0xf470B030);
    writel(0x00000200, 0xf4708430);
    writel(0x00000200, 0xf4709430);
    writel(0x00000200, 0xf470A430);
    writel(0x00000200, 0xf470B430);
    writel(0x000067c3, 0xf4700100);
    writel(0x000067c3, 0xf4700500);
    writel(0x000067c3, 0xf4700900);
    writel(0x000066c3, 0xf4700D00);
    writel(0x000067c3, 0xf47001C0);
    writel(0x000067c3, 0xf47005C0);
    writel(0x000067c3, 0xf47009C0);
    writel(0x000066c3, 0xf4700DC0);
    writel(0x00000e0c, 0xf4700144);
    writel(0x00000e0c, 0xf4700544);
    writel(0x0000060c, 0xf4700944);
    writel(0x0000040c, 0xf4700D44);
    writel(0x00000e0c, 0xf4700204);
    writel(0x00000e0c, 0xf4700604);
    writel(0x0000060c, 0xf4700A04);
    writel(0x0000040c, 0xf4700E04);
    writel(0x00000a63, 0xf4700148);
    writel(0x00000a63, 0xf4700548);
    writel(0x00000a63, 0xf4700948);
    writel(0x00000a63, 0xf4700D48);
    writel(0x00000a63, 0xf4700208);
    writel(0x00000a63, 0xf4700608);
    writel(0x00000a63, 0xf4700A08);
    writel(0x00000a63, 0xf4700E08);
    writel(0x0000101A, 0xf4700168);
    writel(0x0000101A, 0xf4700568);
    writel(0x0000101A, 0xf4700968);
    writel(0x0000101A, 0xf4700D68);
    writel(0x0000101A, 0xf4700228);
    writel(0x0000101A, 0xf4700628);
    writel(0x0000101A, 0xf4700A28);
    writel(0x0000101A, 0xf4700E28);
    writel(0x00004200, 0xf4700158);
    writel(0x00004200, 0xf4700558);
    writel(0x00004200, 0xf4700958);
    writel(0x00004200, 0xf4700D58);
    writel(0x00004200, 0xf4700218);
    writel(0x00004200, 0xf4700618);
    writel(0x00004200, 0xf4700A18);
    writel(0x00004200, 0xf4700E18);
    writel(0x0000ffff, 0xf47081D0);
    writel(0x0000ffff, 0xf47091D0);
    writel(0x0000ffff, 0xf470A1D0);
    writel(0x0000ffff, 0xf470B1D0);
    writel(0x0000ffff, 0xf47085D0);
    writel(0x0000ffff, 0xf47095D0);
    writel(0x0000ffff, 0xf470A5D0);
    writel(0x0000ffff, 0xf470B5D0);
    writel(0x00000108, 0xf4708098);
    writel(0x00000108, 0xf4709098);
    writel(0x00000108, 0xf470A098);
    writel(0x00000108, 0xf470B098);
    writel(0x00000108, 0xf4708498);
    writel(0x00000108, 0xf4709498);
    writel(0x00000108, 0xf470A498);
    writel(0x00000108, 0xf470B498);
    writel(0x0000426c, 0xf4708068);
    writel(0x0000426c, 0xf4709068);
    writel(0x0000426c, 0xf470A068);
    writel(0x0000c26c, 0xf470B068);
    writel(0x0000426c, 0xf4708468);
    writel(0x0000426c, 0xf4709468);
    writel(0x0000426c, 0xf470A468);
    writel(0x0000c26c, 0xf470B468);
    writel(0x00000000, 0xf4708008);
    writel(0x00000000, 0xf4709008);
    writel(0x00000000, 0xf470A008);
    writel(0x00000060, 0xf470B008);
    writel(0x00000000, 0xf4708408);
    writel(0x00000000, 0xf4709408);
    writel(0x00000000, 0xf470A408);
    writel(0x00000060, 0xf470B408);

    mdelay(20);
}

void pcie1_phy_parameter(void)
{
    writel(0x000008f0, 0xf47170B8);
    writel(0x000040c1, 0xf4718180);
    writel(0x000040c1, 0xf4719180);
    writel(0x000040c1, 0xf471A180);
    writel(0x000040c1, 0xf471B180);
    writel(0x000040c1, 0xf4718580);
    writel(0x000040c1, 0xf4719580);
    writel(0x000040c1, 0xf471A580);
    writel(0x000040c1, 0xf471B580);
    writel(0x0000ffd0, 0xf47180C0);
    writel(0x0000ffd0, 0xf47190C0);
    writel(0x0000ffd0, 0xf471A0C0);
    writel(0x0000ffd0, 0xf471B0C0);
    writel(0x0000ffd0, 0xf47184C0);
    writel(0x0000ffd0, 0xf47194C0);
    writel(0x0000ffd0, 0xf471A4C0);
    writel(0x0000ffd0, 0xf471B4C0);
    writel(0x00000100, 0xf4718090);
    writel(0x00000100, 0xf4719090);
    writel(0x00000100, 0xf471A090);
    writel(0x00000100, 0xf471B090);
    writel(0x00000100, 0xf4718490);
    writel(0x00000100, 0xf4719490);
    writel(0x00000100, 0xf471A490);
    writel(0x00000100, 0xf471B490);
    writel(0x00000002, 0xf471011C);
    writel(0x00000002, 0xf471051C);
    writel(0x00000002, 0xf471091C);
    writel(0x00000002, 0xf4710D1C);
    writel(0x00000002, 0xf47101DC);
    writel(0x00000002, 0xf47105DC);
    writel(0x00000002, 0xf47109DC);
    writel(0x00000002, 0xf4710DDC);
    writel(0x00004001, 0xf4710124);
    writel(0x00004001, 0xf4710524);
    writel(0x00004001, 0xf4710924);
    writel(0x00004001, 0xf4710D24);
    writel(0x00004001, 0xf47101E4);
    writel(0x00004001, 0xf47105E4);
    writel(0x00004001, 0xf47109E4);
    writel(0x00004001, 0xf4710DE4);
    writel(0x00000038, 0xf471012C);
    writel(0x00000038, 0xf471052C);
    writel(0x00000038, 0xf471092C);
    writel(0x00000038, 0xf4710D2C);
    writel(0x00000038, 0xf47101EC);
    writel(0x00000038, 0xf47105EC);
    writel(0x00000038, 0xf47109EC);
    writel(0x00000038, 0xf4710DEC);
    writel(0x00003000, 0xf4718028);
    writel(0x00002000, 0xf4719028);
    writel(0x00002000, 0xf471A028);
    writel(0x00002000, 0xf471B028);
    writel(0x00003000, 0xf4718428);
    writel(0x00002000, 0xf4719428);
    writel(0x00002000, 0xf471A428);
    writel(0x00002000, 0xf471B428);
    writel(0x0000fecf, 0xf4718130);
    writel(0x0000fecf, 0xf4719130);
    writel(0x0000fecf, 0xf471A130);
    writel(0x0000fecf, 0xf471B130);
    writel(0x0000fecf, 0xf4718530);
    writel(0x0000fecf, 0xf4719530);
    writel(0x0000fecf, 0xf471A530);
    writel(0x0000fecf, 0xf471B530);
    writel(0x0000aff0, 0xf47180D0);
    writel(0x0000aff0, 0xf47190D0);
    writel(0x0000aff0, 0xf471A0D0);
    writel(0x00009840, 0xf471B0D0);
    writel(0x0000aff0, 0xf47184D0);
    writel(0x0000aff0, 0xf47194D0);
    writel(0x0000aff0, 0xf471A4D0);
    writel(0x00009840, 0xf471B4D0);
    writel(0x0000079e, 0xf4718150);
    writel(0x0000079e, 0xf4719150);
    writel(0x0000079e, 0xf471A150);
    writel(0x0000079e, 0xf471B150);
    writel(0x0000079e, 0xf4718550);
    writel(0x0000079e, 0xf4719550);
    writel(0x0000079e, 0xf471A550);
    writel(0x0000079e, 0xf471B550);
    writel(0x00008038, 0xf47181C8);
    writel(0x00008038, 0xf47191C8);
    writel(0x00008038, 0xf471A1C8);
    writel(0x00008038, 0xf471B1C8);
    writel(0x00008038, 0xf47185C8);
    writel(0x00008038, 0xf47195C8);
    writel(0x00008038, 0xf471A5C8);
    writel(0x00008038, 0xf471B5C8);
    writel(0x00003f20, 0xf47180A8);
    writel(0x00003f20, 0xf47190A8);
    writel(0x00003f20, 0xf471A0A8);
    writel(0x00003f20, 0xf471B0A8);
    writel(0x00003f20, 0xf47184A8);
    writel(0x00003f20, 0xf47194A8);
    writel(0x00003f20, 0xf471A4A8);
    writel(0x00003f20, 0xf471B4A8);
    writel(0x00006002, 0xf4718248);
    writel(0x00006002, 0xf4719248);
    writel(0x00002002, 0xf471A248);
    writel(0x00002002, 0xf471B248);
    writel(0x00006002, 0xf4718648);
    writel(0x00006002, 0xf4719648);
    writel(0x00002002, 0xf471A648);
    writel(0x00002002, 0xf471B648);
    writel(0x00008008, 0xf4718250);
    writel(0x00008008, 0xf4719250);
    writel(0x00008008, 0xf471A250);
    writel(0x00008008, 0xf471B250);
    writel(0x00008008, 0xf4718650);
    writel(0x00008008, 0xf4719650);
    writel(0x00008008, 0xf471A650);
    writel(0x00008008, 0xf471B650);
    writel(0x00000c49, 0xf4718268);
    writel(0x00000c49, 0xf4719268);
    writel(0x00000e49, 0xf471A268);
    writel(0x00000e49, 0xf471B268);
    writel(0x00000c49, 0xf4718668);
    writel(0x00000c49, 0xf4719668);
    writel(0x00000e49, 0xf471A668);
    writel(0x00000e49, 0xf471B668);
    writel(0x00007800, 0xf4718110);
    writel(0x00007800, 0xf4719110);
    writel(0x00007800, 0xf471A110);
    writel(0x00007800, 0xf471B110);
    writel(0x00007800, 0xf4718510);
    writel(0x00007800, 0xf4719510);
    writel(0x00007800, 0xf471A510);
    writel(0x00007800, 0xf471B510);
    writel(0x00007800, 0xf4718118);
    writel(0x00007800, 0xf4719118);
    writel(0x00007800, 0xf471A118);
    writel(0x00007800, 0xf471B118);
    writel(0x00007800, 0xf4718518);
    writel(0x00007800, 0xf4719518);
    writel(0x00007800, 0xf471A518);
    writel(0x00007800, 0xf471B518);
    writel(0x00000320, 0xf4718120);
    writel(0x00000320, 0xf4719120);
    writel(0x00000320, 0xf471A120);
    writel(0x00000320, 0xf471B120);
    writel(0x00000320, 0xf4718520);
    writel(0x00000320, 0xf4719520);
    writel(0x00000320, 0xf471A520);
    writel(0x00000320, 0xf471B520);
    writel(0x00000320, 0xf4718128);
    writel(0x00000320, 0xf4719128);
    writel(0x00000320, 0xf471A128);
    writel(0x00000320, 0xf471B128);
    writel(0x00000320, 0xf4718528);
    writel(0x00000320, 0xf4719528);
    writel(0x00000320, 0xf471A528);
    writel(0x00000320, 0xf471B528);
    writel(0x000080a6, 0xf4718148);
    writel(0x000080a6, 0xf4719148);
    writel(0x000080a6, 0xf471A148);
    writel(0x000080a6, 0xf471B148);
    writel(0x000080a6, 0xf4718548);
    writel(0x000080a6, 0xf4719548);
    writel(0x000080a6, 0xf471A548);
    writel(0x000080a6, 0xf471B548);
    writel(0x00000000, 0xf4718140);
    writel(0x00000000, 0xf4719140);
    writel(0x00000000, 0xf471A140);
    writel(0x00003980, 0xf471B140);
    writel(0x00000000, 0xf4718540);
    writel(0x00000000, 0xf4719540);
    writel(0x00000000, 0xf471A540);
    writel(0x00003980, 0xf471B540);
    writel(0x00000000, 0xf4718178);
    writel(0x00000000, 0xf4719178);
    writel(0x00000000, 0xf471A178);
    writel(0x00000000, 0xf471B178);
    writel(0x00000000, 0xf4718578);
    writel(0x00000000, 0xf4719578);
    writel(0x00000000, 0xf471A578);
    writel(0x00000000, 0xf471B578);
    writel(0x00000010, 0xf4718108);
    writel(0x00000010, 0xf4719108);
    writel(0x00000010, 0xf471A108);
    writel(0x00000010, 0xf471B108);
    writel(0x00000010, 0xf4718508);
    writel(0x00000010, 0xf4719508);
    writel(0x00000010, 0xf471A508);
    writel(0x00000010, 0xf471B508);
    writel(0x00000002, 0xf4718160);
    writel(0x00000002, 0xf4719160);
    writel(0x00000002, 0xf471A160);
    writel(0x00000002, 0xf471B160);
    writel(0x00000002, 0xf4718560);
    writel(0x00000002, 0xf4719560);
    writel(0x00000002, 0xf471A560);
    writel(0x00000002, 0xf471B560);
    writel(0x00000000, 0xf4718168);
    writel(0x00000000, 0xf4719168);
    writel(0x00000000, 0xf471A168);
    writel(0x00000000, 0xf471B168);
    writel(0x00000000, 0xf4718568);
    writel(0x00000000, 0xf4719568);
    writel(0x00000000, 0xf471A568);
    writel(0x00000000, 0xf471B568);
    writel(0x000000ff, 0xf47181E0);
    writel(0x000000ff, 0xf47191E0);
    writel(0x000000ff, 0xf471A1E0);
    writel(0x000000ff, 0xf471B1E0);
    writel(0x000000ff, 0xf47185E0);
    writel(0x000000ff, 0xf47195E0);
    writel(0x000000ff, 0xf471A5E0);
    writel(0x000000ff, 0xf471B5E0);
    writel(0x000000ff, 0xf47181F0);
    writel(0x000000ff, 0xf47191F0);
    writel(0x000000ff, 0xf471A1F0);
    writel(0x000000ff, 0xf471B1F0);
    writel(0x000000ff, 0xf47185F0);
    writel(0x000000ff, 0xf47195F0);
    writel(0x000000ff, 0xf471A5F0);
    writel(0x000000ff, 0xf471B5F0);
    writel(0x00002000, 0xf4718260);
    writel(0x00002000, 0xf4719260);
    writel(0x00002000, 0xf471A260);
    writel(0x00002000, 0xf471B260);
    writel(0x00002000, 0xf4718660);
    writel(0x00002000, 0xf4719660);
    writel(0x00002000, 0xf471A660);
    writel(0x00002000, 0xf471B660);
    writel(0x00000064, 0xf4718220);
    writel(0x00000064, 0xf4719220);
    writel(0x00000064, 0xf471A220);
    writel(0x00000064, 0xf471B220);
    writel(0x00000064, 0xf4718620);
    writel(0x00000064, 0xf4719620);
    writel(0x00000064, 0xf471A620);
    writel(0x00000064, 0xf471B620);
    writel(0x00002700, 0xf4718230);
    writel(0x00002700, 0xf4719230);
    writel(0x00003E70, 0xf471A230);
    writel(0x00003E70, 0xf471B230);
    writel(0x00002700, 0xf4718630);
    writel(0x00002700, 0xf4719630);
    writel(0x00003E70, 0xf471A630);
    writel(0x00003E70, 0xf471B630);
    writel(0x00002710, 0xf4718228);
    writel(0x00002710, 0xf4719228);
    writel(0x00003E80, 0xf471A228);
    writel(0x00003E80, 0xf471B228);
    writel(0x00002710, 0xf4718628);
    writel(0x00002710, 0xf4719628);
    writel(0x00003E80, 0xf471A628);
    writel(0x00003E80, 0xf471B628);
    writel(0x00000fff, 0xf471C000);
    writel(0x00000fff, 0xf471D000);
    writel(0x00000fff, 0xf471E000);
    writel(0x00000fff, 0xf471F000);
    writel(0x00006780, 0xf4718270);
    writel(0x00006780, 0xf4719270);
    writel(0x00006780, 0xf471A270);
    writel(0x00006780, 0xf471B270);
    writel(0x00006780, 0xf4718670);
    writel(0x00006780, 0xf4719670);
    writel(0x00006780, 0xf471A670);
    writel(0x00006780, 0xf471B670);
    writel(0x00006000, 0xf47181B8);
    writel(0x00006000, 0xf47191B8);
    writel(0x00006000, 0xf471A1B8);
    writel(0x00006000, 0xf471B1B8);
    writel(0x00006000, 0xf47185B8);
    writel(0x00006000, 0xf47195B8);
    writel(0x00006000, 0xf471A5B8);
    writel(0x00006000, 0xf471B5B8);
    writel(0x0000c970, 0xf4710058);
    writel(0x0000c970, 0xf4710458);
    writel(0x0000c970, 0xf4710858);
    writel(0x0000c970, 0xf4710C58);
    writel(0x0000c10d, 0xf4710044);
    writel(0x0000c10d, 0xf4710444);
    writel(0x0000c10d, 0xf4710844);
    writel(0x0000c10c, 0xf4710C44);
    writel(0x0000d2f0, 0xf4710068);
    writel(0x0000d2f0, 0xf4710468);
    writel(0x000052f0, 0xf4710868);
    writel(0x000052f0, 0xf4710C68);
    writel(0x00005c0f, 0xf47100C8);
    writel(0x00006c0f, 0xf47104C8);
    writel(0x00007c0f, 0xf47108C8);
    writel(0x00007c0f, 0xf4710CC8);
    writel(0x00005c0f, 0xf4710188);
    writel(0x00006c0f, 0xf4710588);
    writel(0x00007c0f, 0xf4710988);
    writel(0x00007c0f, 0xf4710D88);
    writel(0x00000240, 0xf47100DC);
    writel(0x00000240, 0xf47104DC);
    writel(0x00000240, 0xf47108DC);
    writel(0x00000240, 0xf4710CDC);
    writel(0x00000240, 0xf471019C);
    writel(0x00000240, 0xf471059C);
    writel(0x00000240, 0xf471099C);
    writel(0x00000240, 0xf4710D9C);
    writel(0x00000600, 0xf47100C0);
    writel(0x00000600, 0xf47104C0);
    writel(0x00000e00, 0xf47108C0);
    writel(0x00000e00, 0xf4710CC0);
    writel(0x00000600, 0xf4710180);
    writel(0x00000600, 0xf4710580);
    writel(0x00000e00, 0xf4710980);
    writel(0x00000e00, 0xf4710D80);
    writel(0x0000002b, 0xf471C118);
    writel(0x0000002b, 0xf471D118);
    writel(0x0000002b, 0xf471E118);
    writel(0x0000002b, 0xf471F118);
    writel(0x0000f104, 0xf4717218);
    writel(0x0000f145, 0xf4717258);
    writel(0x0000e1c7, 0xf4717260);
    writel(0x00009980, 0xf4717280);
    writel(0x0000d880, 0xf47172C0);
    writel(0x00008044, 0xf471016C);
    writel(0x00008044, 0xf471056C);
    writel(0x00008044, 0xf471096C);
    writel(0x0000804c, 0xf4710D6C);
    writel(0x00008044, 0xf471022C);
    writel(0x00008044, 0xf471062C);
    writel(0x00008044, 0xf4710A2C);
    writel(0x0000804c, 0xf4710E2C);
    writel(0x00008e7c, 0xf4710174);
    writel(0x00008e7c, 0xf4710574);
    writel(0x00008e7c, 0xf4710974);
    writel(0x00008e7c, 0xf4710D74);
    writel(0x00008e7c, 0xf4710234);
    writel(0x00008e7c, 0xf4710634);
    writel(0x00008e7c, 0xf4710A34);
    writel(0x00008e7c, 0xf4710E34);
    writel(0x00008000, 0xf471015C);
    writel(0x00008000, 0xf471055C);
    writel(0x00001000, 0xf471095C);
    writel(0x00001000, 0xf4710D5C);
    writel(0x00008000, 0xf471021C);
    writel(0x00008000, 0xf471061C);
    writel(0x00001000, 0xf4710A1C);
    writel(0x00001000, 0xf4710E1C);
    writel(0x000001e4, 0xf4710104);
    writel(0x000001e4, 0xf4710504);
    writel(0x000000a4, 0xf4710904);
    writel(0x000000a4, 0xf4710D04);
    writel(0x000001e4, 0xf47101C4);
    writel(0x000001e4, 0xf47105C4);
    writel(0x000000a4, 0xf47109C4);
    writel(0x000000a4, 0xf4710DC4);
    writel(0x00000744, 0xf4710138);
    writel(0x00000044, 0xf4710538);
    writel(0x00000043, 0xf4710938);
    writel(0x00000043, 0xf4710D38);
    writel(0x00000744, 0xf47101F8);
    writel(0x00000044, 0xf47105F8);
    writel(0x00000043, 0xf47109F8);
    writel(0x00000043, 0xf4710DF8);
    writel(0x00008864, 0xf4710170);
    writel(0x00008864, 0xf4710570);
    writel(0x00008864, 0xf4710970);
    writel(0x00008864, 0xf4710D70);
    writel(0x00008864, 0xf4710230);
    writel(0x00008864, 0xf4710630);
    writel(0x00008864, 0xf4710A30);
    writel(0x00008864, 0xf4710E30);
    writel(0x00005680, 0xf4710164);
    writel(0x00005680, 0xf4710564);
    writel(0x00005a82, 0xf4710964);
    writel(0x00005a82, 0xf4710D64);
    writel(0x00005680, 0xf4710224);
    writel(0x00005680, 0xf4710624);
    writel(0x00005a82, 0xf4710A24);
    writel(0x00005a82, 0xf4710E24);
    writel(0x0000000d, 0xf47180B0);
    writel(0x0000000d, 0xf47190B0);
    writel(0x0000000d, 0xf471A0B0);
    writel(0x0000000d, 0xf471B0B0);
    writel(0x0000000d, 0xf47184B0);
    writel(0x0000000d, 0xf47194B0);
    writel(0x0000000d, 0xf471A4B0);
    writel(0x0000000d, 0xf471B4B0);
    writel(0x000001ff, 0xf4718258);
    writel(0x000001ff, 0xf4719258);
    writel(0x000001ff, 0xf471A258);
    writel(0x000001ff, 0xf471B258);
    writel(0x000001ff, 0xf4718658);
    writel(0x000001ff, 0xf4719658);
    writel(0x000001ff, 0xf471A658);
    writel(0x000001ff, 0xf471B658);
    writel(0x00000003, 0xf4718210);
    writel(0x00000003, 0xf4719210);
    writel(0x00000003, 0xf471A210);
    writel(0x00000003, 0xf471B210);
    writel(0x00000003, 0xf4718610);
    writel(0x00000003, 0xf4719610);
    writel(0x00000003, 0xf471A610);
    writel(0x00000003, 0xf471B610);
    writel(0x00000007, 0xf4710108);
    writel(0x00000006, 0xf4710508);
    writel(0x00000004, 0xf4710908);
    writel(0x00000000, 0xf4710D08);
    writel(0x00000007, 0xf47101C8);
    writel(0x00000006, 0xf47105C8);
    writel(0x00000004, 0xf47109C8);
    writel(0x00000000, 0xf4710DC8);
    writel(0x000011e0, 0xf47180A0);
    writel(0x000011e0, 0xf47190A0);
    writel(0x000011e0, 0xf471A0A0);
    writel(0x000011e0, 0xf471B0A0);
    writel(0x000011e0, 0xf47184A0);
    writel(0x000011e0, 0xf47194A0);
    writel(0x000011e0, 0xf471A4A0);
    writel(0x000011e0, 0xf471B4A0);
    writel(0x00000200, 0xf4718030);
    writel(0x00000200, 0xf4719030);
    writel(0x00000200, 0xf471A030);
    writel(0x00000200, 0xf471B030);
    writel(0x00000200, 0xf4718430);
    writel(0x00000200, 0xf4719430);
    writel(0x00000200, 0xf471A430);
    writel(0x00000200, 0xf471B430);
    writel(0x000067c3, 0xf4710100);
    writel(0x000067c3, 0xf4710500);
    writel(0x000067c3, 0xf4710900);
    writel(0x000066c3, 0xf4710D00);
    writel(0x000067c3, 0xf47101C0);
    writel(0x000067c3, 0xf47105C0);
    writel(0x000067c3, 0xf47109C0);
    writel(0x000066c3, 0xf4710DC0);
    writel(0x00000e0c, 0xf4710144);
    writel(0x00000e0c, 0xf4710544);
    writel(0x0000060c, 0xf4710944);
    writel(0x0000040c, 0xf4710D44);
    writel(0x00000e0c, 0xf4710204);
    writel(0x00000e0c, 0xf4710604);
    writel(0x0000060c, 0xf4710A04);
    writel(0x0000040c, 0xf4710E04);
    writel(0x00000a63, 0xf4710148);
    writel(0x00000a63, 0xf4710548);
    writel(0x00000a63, 0xf4710948);
    writel(0x00000a63, 0xf4710D48);
    writel(0x00000a63, 0xf4710208);
    writel(0x00000a63, 0xf4710608);
    writel(0x00000a63, 0xf4710A08);
    writel(0x00000a63, 0xf4710E08);
    writel(0x0000101A, 0xf4710168);
    writel(0x0000101A, 0xf4710568);
    writel(0x0000101A, 0xf4710968);
    writel(0x0000101A, 0xf4710D68);
    writel(0x0000101A, 0xf4710228);
    writel(0x0000101A, 0xf4710628);
    writel(0x0000101A, 0xf4710A28);
    writel(0x0000101A, 0xf4710E28);
    writel(0x00004200, 0xf4710158);
    writel(0x00004200, 0xf4710558);
    writel(0x00004200, 0xf4710958);
    writel(0x00004200, 0xf4710D58);
    writel(0x00004200, 0xf4710218);
    writel(0x00004200, 0xf4710618);
    writel(0x00004200, 0xf4710A18);
    writel(0x00004200, 0xf4710E18);
    writel(0x0000ffff, 0xf47181D0);
    writel(0x0000ffff, 0xf47191D0);
    writel(0x0000ffff, 0xf471A1D0);
    writel(0x0000ffff, 0xf471B1D0);
    writel(0x0000ffff, 0xf47185D0);
    writel(0x0000ffff, 0xf47195D0);
    writel(0x0000ffff, 0xf471A5D0);
    writel(0x0000ffff, 0xf471B5D0);
    writel(0x00000108, 0xf4718098);
    writel(0x00000108, 0xf4719098);
    writel(0x00000108, 0xf471A098);
    writel(0x00000108, 0xf471B098);
    writel(0x00000108, 0xf4718498);
    writel(0x00000108, 0xf4719498);
    writel(0x00000108, 0xf471A498);
    writel(0x00000108, 0xf471B498);
    writel(0x0000426c, 0xf4718068);
    writel(0x0000426c, 0xf4719068);
    writel(0x0000426c, 0xf471A068);
    writel(0x0000c26c, 0xf471B068);
    writel(0x0000426c, 0xf4718468);
    writel(0x0000426c, 0xf4719468);
    writel(0x0000426c, 0xf471A468);
    writel(0x0000c26c, 0xf471B468);
    writel(0x00000000, 0xf4718008);
    writel(0x00000000, 0xf4719008);
    writel(0x00000000, 0xf471A008);
    writel(0x00000060, 0xf471B008);
    writel(0x00000000, 0xf4718408);
    writel(0x00000000, 0xf4719408);
    writel(0x00000000, 0xf471A408);
    writel(0x00000060, 0xf471B408);

    mdelay(20);
}

void pcie2_phy_parameter(void)
{
    writel(0x0000C1FC, 0xf4724480);
    writel(0x0000C1FC, 0xf4724540);
    writel(0x0000C1FC, 0xf4724600);
    writel(0x0000E87F, 0xf4724488);
    writel(0x0000E87F, 0xf4724548);
    writel(0x0000E87F, 0xf4724608);
    writel(0x0000E060, 0xf47244A8);
    writel(0x0000E060, 0xf4724568);
    writel(0x0000E060, 0xf4724628);
    writel(0x000000C6, 0xf4727CA0);
    writel(0x000000C6, 0xf4727CE0);
    writel(0x0000009E, 0xf4727D20);
    writel(0x000000C6, 0xf4727C88);
    writel(0x000000C6, 0xf4727CC8);
    writel(0x0000009E, 0xf4727D08);
    writel(0x00000010, 0xf4727CA8);
    writel(0x00000010, 0xf4727CE8);
    writel(0x00000018, 0xf4727D28);
    writel(0x00000280, 0xf4727CB0);
    writel(0x00000280, 0xf4727CF0);
    writel(0x00000300, 0xf4727D30);
    writel(0x00000280, 0xf4727CB8);
    writel(0x00000280, 0xf4727CF8);
    writel(0x00000300, 0xf4727D38);
    writel(0x00000280, 0xf4726090);
    writel(0x00000280, 0xf4726098);
    writel(0x00000001, 0xf4727100);
    writel(0x00003500, 0xf4720024);
    writel(0x00003500, 0xf4720164);
    writel(0x00003500, 0xf47202A4);
    writel(0x00000188, 0xf4720050);
    writel(0x00000150, 0xf4720190);
    writel(0x00000150, 0xf47202D0);
    writel(0x00001821, 0xf4720058);
    writel(0x00001821, 0xf4720198);
    writel(0x00001821, 0xf47202D8);
    writel(0x00000010, 0xf472005C);
    writel(0x00000014, 0xf472019C);
    writel(0x00000260, 0xf4720070);
    writel(0x00000260, 0xf47201B0);
    writel(0x00000260, 0xf47202F0);
    writel(0x00000C45, 0xf4720104);
    writel(0x00000C45, 0xf4720244);
    writel(0x00000645, 0xf4720384);
    writel(0x0000011B, 0xf4720108);
    writel(0x0000011B, 0xf4720248);
    writel(0x0000011B, 0xf4720388);
    writel(0x00006FE3, 0xf4720114);
    writel(0x00006FE3, 0xf4720254);
    writel(0x00006A03, 0xf4720394);
    writel(0x0000F0F0, 0xf472039C);
    writel(0x00000031, 0xf4720130);
    writel(0x00000031, 0xf4720270);
    writel(0x00000031, 0xf47203B0);
    writel(0x00005000, 0xf47269E8);
    writel(0x00000002, 0xf4724000);
    writel(0x00000000, 0xf4724018);
    writel(0x00002034, 0xf4724020);

    mdelay(20);
}

void pcie7_phy_parameter(void)
{
    /*GEN1*/
    writel(0x00002853, 0xf4330004);
    writel(0x00003019, 0xf4330020);
    writel(0x0000af0d, 0xf433002c);
    writel(0x0000e726, 0xf4330034);
    writel(0x00005000, 0xf4330038);
    writel(0x000044ca, 0xf4330080);
    writel(0x000067aa, 0xf4330084);
    writel(0x00004b46, 0xf433008c);
    writel(0x000004cb, 0xf433009c);
    writel(0x00003d45, 0xf43300a8);
    writel(0x0000b0d1, 0xf43300ac);
    writel(0x0000a053, 0xf43300bc);
    writel(0x00007f10, 0xf4330158);
    /*GEN2*/
    writel(0x00003599, 0xf4330220);
    writel(0x0000af0d, 0xf433022c);
    writel(0x0000e726, 0xf4330234);
    writel(0x00005000, 0xf4330238);
    writel(0x000044ea, 0xf4330280);
    writel(0x0000a7aa, 0xf4330284);
    writel(0x00001b46, 0xf433028c);
    writel(0x000004cb, 0xf433029c);
    writel(0x00003d45, 0xf43302a8);
    writel(0x0000b091, 0xf43302ac);
    writel(0x00007f10, 0xf4330358);

    mdelay(10);
}

void pcie8_phy_parameter(void)
{
    /*GEN1*/
    writel(0x00002853, 0xf4331004);
    writel(0x00003019, 0xf4331020);
    writel(0x0000af0d, 0xf433102c);
    writel(0x0000e726, 0xf4331034);
    writel(0x00005000, 0xf4331038);
    writel(0x000044ca, 0xf4331080);
    writel(0x000067aa, 0xf4331084);
    writel(0x00004b46, 0xf433108c);
    writel(0x000004cb, 0xf433109c);
    writel(0x00003d45, 0xf43310a8);
    writel(0x0000b0d1, 0xf43310ac);
    writel(0x0000a053, 0xf43310bc);
    writel(0x00007f10, 0xf4331158);
    /*GEN2*/
    writel(0x00003599, 0xf4331220);
    writel(0x0000af0d, 0xf433122c);
    writel(0x0000e726, 0xf4331234);
    writel(0x00005000, 0xf4331238);
    writel(0x000044ea, 0xf4331280);
    writel(0x0000a7aa, 0xf4331284);
    writel(0x00001b46, 0xf433128c);
    writel(0x000004cb, 0xf433129c);
    writel(0x00003d45, 0xf43312a8);
    writel(0x0000b091, 0xf43312ac);
    writel(0x00007f10, 0xf4331358);

    mdelay(10);
}

void pcie0_rc_atu_setup(void)
{
    //rc
    /*1. cortina_pcie_host_setup*/
    writel(0xa0000000, 0xf4325110);//OK
    writel(0xa0000fff, 0xf4325114);//OK
    writel(0xa0300000, 0xf4325118);//OK
    writel(0x00000000, 0xf4325018);//new

    /*2. dw_pcie_setup_rc*/
    //dw_pcie_dbi_ro_wr_en
    //drivers/pci/controller/dwc/pcie-designware.h
    //PCIE_MISC_CONTROL_1_OFF
    writel(0x000bff49, 0xa00008bc);//new
    //writel(0x000bff41, 0xf43308bc);

    // dw_pcie_setup
    //drivers/pci/controller/dwc/pcie-designware.c
    //PCIE_PORT_LINK_CONTROL
    writel(0x00030120, 0xa0000710);//OK
    //PCIE_LINK_WIDTH_SPEED_CONTROL
    writel(0x000302ff, 0xa000080c);//update

    //Setup RC BARs
    writel(0x00000004, 0xa0000010);//OK
    writel(0x00000000, 0xa0000014);//OK

    //Setup interrupt pins
    writel(0x000001ff, 0xa000003c);//OK
    ///* Setup bus numbers */
    writel(0x00ff0100, 0xa0000018);//update
    //Setup command register
    writel(0x00100107, 0xa0000004);//OK

    //__dw_pcie_prog_outbound_atu
    writel(0xa0400000, 0xa0000008);
    writel(0x00000000, 0xa000000c);
    writel(0xafffffff, 0xa0000010);
    writel(0x00000000, 0xa0000020);
    writel(0xa0400000, 0xa0000014);
    writel(0x00000000, 0xa0000018);
    writel(0x00000000, 0xa0000000);
    writel(0x80000000, 0xa0000004);

    //PCI_BASE_ADDRESS_0
    writel(0x00000000, 0xa0000010);
    //PCI_CLASS_DEVICE
    writel(0x00000604, 0xa000000a);
    //PCIE_LINK_WIDTH_SPEED_CONTROL
    writel(0x000302ff, 0xa000080c);

    //dw_pcie_dbi_ro_wr_dis
    writel(0x000bff48, 0xa00008bc);
}

void pcie1_rc_atu_setup(void)
{
    //rc
    /*1. cortina_pcie_host_setup*/
    writel(0xb0000000, 0xf4325910);//OK
    writel(0xb0000fff, 0xf4325914);//OK
    writel(0xb0300000, 0xf4325918);//OK
    writel(0x00000000, 0xf4325818);//new

    /*2. dw_pcie_setup_rc*/
    //dw_pcie_dbi_ro_wr_en
    //drivers/pci/controller/dwc/pcie-designware.h
    //PCIE_MISC_CONTROL_1_OFF
    writel(0x000bff49, 0xb00008bc);//new
    //writel(0x000bff41, 0xf43308bc);

    // dw_pcie_setup
    //drivers/pci/controller/dwc/pcie-designware.c
    //PCIE_PORT_LINK_CONTROL
    writel(0x00030120, 0xb0000710);//OK
    //PCIE_LINK_WIDTH_SPEED_CONTROL
    writel(0x000302ff, 0xb000080c);//update

    //Setup RC BARs
    writel(0x00000004, 0xb0000010);//OK
    writel(0x00000000, 0xb0000014);//OK

    //Setup interrupt pins
    writel(0x000001ff, 0xb000003c);//OK
    ///* Setup bus numbers */
    writel(0x00ff0100, 0xb0000018);//update
    //Setup command register
    writel(0x00100107, 0xb0000004);//OK

    //__dw_pcie_prog_outbound_atu
    writel(0xb0400000, 0xb0000008);
    writel(0x00000000, 0xb000000c);
    writel(0xbfffffff, 0xb0000010);
    writel(0x00000000, 0xb0000020);
    writel(0xb0400000, 0xb0000014);
    writel(0x00000000, 0xb0000018);
    writel(0x00000000, 0xb0000000);
    writel(0x80000000, 0xb0000004);

    //PCI_BASE_ADDRESS_0
    writel(0x00000000, 0xb0000010);
    //PCI_CLASS_DEVICE
    writel(0x00000604, 0xb000000a);
    //PCIE_LINK_WIDTH_SPEED_CONTROL
    writel(0x000302ff, 0xb000080c);

    //dw_pcie_dbi_ro_wr_dis
    writel(0x000bff48, 0xb00008bc);

}

void pcie2_rc_atu_setup(void)
{
    //rc
    /*1. cortina_pcie_host_setup*/
    writel(0xc0000000, 0xf4326110);//OK
    writel(0xc0000fff, 0xf4326114);//OK
    writel(0xc0300000, 0xf4326118);//OK
    writel(0x00000000, 0xf4326018);//new

    /*2. dw_pcie_setup_rc*/
    //dw_pcie_dbi_ro_wr_en
    //drivers/pci/controller/dwc/pcie-designware.h
    //PCIE_MISC_CONTROL_1_OFF
    writel(0x000bff41, 0xc00008bc);//new
    //writel(0x000bff41, 0xf43308bc);

    // dw_pcie_setup
    //drivers/pci/controller/dwc/pcie-designware.c
    //PCIE_PORT_LINK_CONTROL
    writel(0x00010120, 0xc0000710);//OK
    //PCIE_LINK_WIDTH_SPEED_CONTROL
    writel(0x000201ff, 0xc000080c);//update

    //Setup RC BARs
    writel(0x00000004, 0xc0000010);//OK
    writel(0x00000000, 0xc0000014);//OK

    //Setup interrupt pins
    writel(0x000001ff, 0xc000003c);//OK
    ///* Setup bus numbers */
    writel(0x00ff0100, 0xc0000018);//update
    //Setup command register
    writel(0x00100107, 0xc0000004);//OK

    //__dw_pcie_prog_outbound_atu
    writel(0xc0400000, 0xc0000008);
    writel(0x00000000, 0xc000000c);
    writel(0xcfffffff, 0xc0000010);
    writel(0x00000000, 0xc0000020);
    writel(0xc0400000, 0xc0000014);
    writel(0x00000000, 0xc0000018);
    writel(0x00000000, 0xc0000000);
    writel(0x80000000, 0xc0000004);

    //PCI_BASE_ADDRESS_0
    writel(0x00000000, 0xc0000010);
    //PCI_CLASS_DEVICE
    writel(0x00000604, 0xc000000a);
    //PCIE_LINK_WIDTH_SPEED_CONTROL
    writel(0x000201ff, 0xc000080c);

    //dw_pcie_dbi_ro_wr_dis
    writel(0x000bff40, 0xc00008bc);

}

void pcie7_rc_atu_setup(void)
{
    //rc
    /*1. cortina_pcie_host_setup*/
    writel(0xd0000000, 0xf4326910);//OK
    writel(0xd0000fff, 0xf4326914);//OK
    writel(0xd0300000, 0xf4326918);//OK
    writel(0x00000000, 0xf4326818);//new

    /*2. dw_pcie_setup_rc*/
    //dw_pcie_dbi_ro_wr_en
    //drivers/pci/controller/dwc/pcie-designware.h
    //PCIE_MISC_CONTROL_1_OFF
    writel(0x000bff41, 0xd00008bc);//new
    //writel(0x000bff41, 0xf43308bc);

    // dw_pcie_setup
    //drivers/pci/controller/dwc/pcie-designware.c
    //PCIE_PORT_LINK_CONTROL
    writel(0x00010120, 0xd0000710);//OK
    //PCIE_LINK_WIDTH_SPEED_CONTROL
    writel(0x000201ff, 0xd000080c);//update

    //Setup RC BARs
    writel(0x00000004, 0xd0000010);//OK
    writel(0x00000000, 0xd0000014);//OK

    //Setup interrupt pins
    writel(0x000001ff, 0xd000003c);//OK
    ///* Setup bus numbers */
    writel(0x00ff0100, 0xd0000018);//update
    //Setup command register
    writel(0x00100107, 0xd0000004);//OK

    //__dw_pcie_prog_outbound_atu
    writel(0xd0400000, 0xd0000008);
    writel(0x00000000, 0xd000000c);
    writel(0xd81fffff, 0xd0000010);
    writel(0x00000000, 0xd0000020);
    writel(0xd0400000, 0xd0000014);
    writel(0x00000000, 0xd0000018);
    writel(0x00000000, 0xd0000000);
    writel(0x80000000, 0xd0000004);

    //PCI_BASE_ADDRESS_0
    writel(0x00000000, 0xd0000010);
    //PCI_CLASS_DEVICE
    writel(0x00000604, 0xd000000a);
    //PCIE_LINK_WIDTH_SPEED_CONTROL
    writel(0x000201ff, 0xd000080c);

    //dw_pcie_dbi_ro_wr_dis
    writel(0x000bff40, 0xd00008bc);

}

void pcie8_rc_atu_setup(void)
{
    //rc
    /*1. cortina_pcie_host_setup*/
    writel(0xd8000000, 0xf4327110);
    writel(0xd8000fff, 0xf4327114);
    writel(0xd8300000, 0xf4327118);
    writel(0x00000000, 0xf4327018);

    /*2. dw_pcie_setup_rc*/
    //dw_pcie_dbi_ro_wr_en
    //drivers/pci/controller/dwc/pcie-designware.h
    //PCIE_MISC_CONTROL_1_OFF
    writel(0x000bff41, 0xd80008bc);//new

    // dw_pcie_setup
    //drivers/pci/controller/dwc/pcie-designware.c
    //PCIE_PORT_LINK_CONTROL
    writel(0x00010120, 0xd8000710);//OK
    //PCIE_LINK_WIDTH_SPEED_CONTROL
    writel(0x000201ff, 0xd800080c);//update

    //Setup RC BARs
    writel(0x00000004, 0xd8000010);//OK
    writel(0x00000000, 0xd8000014);//OK

    //Setup interrupt pins
    writel(0x000001ff, 0xd800003c);//OK
    ///* Setup bus numbers */
    writel(0x00ff0100, 0xd8000018);//update
    //Setup command register
    writel(0x00100107, 0xd8000004);//OK

    //__dw_pcie_prog_outbound_atu
    writel(0xd8400000, 0xd8000008);
    writel(0x00000000, 0xd800000c);
    writel(0xe01fffff, 0xd8000010);
    writel(0x00000000, 0xd8000020);
    writel(0xd8400000, 0xd8000014);
    writel(0x00000000, 0xd8000018);
    writel(0x00000000, 0xd8000000);
    writel(0x80000000, 0xd8000004);

    //PCI_BASE_ADDRESS_0
    writel(0x00000000, 0xd8000010);
    //PCI_CLASS_DEVICE
    writel(0x00000604, 0xd800000a);
    //PCIE_LINK_WIDTH_SPEED_CONTROL
    writel(0x000201ff, 0xd800080c);

    //dw_pcie_dbi_ro_wr_dis
    writel(0x000bff40, 0xd80008bc);
}

void manaual_LEQ(unsigned int pcie_base)
{
    uint32_t val;

    val = 0x3000;
    writel(val, pcie_base + (0x200a * 4));
    udelay(100);
    writel(val, pcie_base + (0x240a * 4));
    udelay(100);
    writel(val, pcie_base + (0x280a * 4));
    udelay(100);
    writel(val, pcie_base + (0x210a * 4));
    udelay(100);
    writel(val, pcie_base + (0x250a * 4));
    udelay(100);
    writel(val, pcie_base + (0x290a * 4));
    udelay(100);
}

static inline u32 genmask(u8 hi, u8 lo)
{
    u32 mask;
    if (hi > 31 || lo > 31 || hi < lo)
        return 0;

    mask = ((1U << (hi - lo + 1)) - 1) << lo;

    return mask;
}//val &= ~(1 << 9);

static void s0_s1_phy_cali_start(unsigned int pcie_base)
{
    writel(0x8185, pcie_base + (0x1c48 * 4));
    writel(0xffff, pcie_base + (0x1c4a * 4));
    writel(0xffff, pcie_base + (0x1c4c * 4));
    writel(0x8084, pcie_base + (0x1c5c * 4));
    writel(0xbfd0, pcie_base + (0x2030 * 4));
    writel(0xbfd0, pcie_base + (0x2130 * 4));
    writel(0xbfd0, pcie_base + (0x2430 * 4));
    writel(0xbfd0, pcie_base + (0x2530 * 4));
    writel(0xbfd0, pcie_base + (0x2830 * 4));
    writel(0xbfd0, pcie_base + (0x2930 * 4));
    writel(0xbfd0, pcie_base + (0x2c30 * 4));
    writel(0xbfd0, pcie_base + (0x2d30 * 4));
}

static void s0_s1_phy_cali_gen1(unsigned int pcie_base)
{
    uint32_t tmp, val;

    writel(0x8084, pcie_base + (0x1c5c * 4));
    writel(0x5059, pcie_base + (0x201e * 4));
    writel(0x5059, pcie_base + (0x211e * 4));
    writel(0x8185, pcie_base + (0x1c48 * 4));
    writel(0x818f, pcie_base + (0x1c48 * 4));
    writel(0x0050, pcie_base + (0x205c * 4));
    writel(0x001a, pcie_base + (0x1c64 * 4));
    while(val != 0x1){
        val = readl(pcie_base + (0x1c68 * 4));
        val = (val & (1 << 9)) >> 9;
    }
    writel(0x7059, pcie_base + (0x201e * 4));
    writel(0x7059, pcie_base + (0x211e * 4));
    writel(0x0c1e, pcie_base + (0x202e * 4));
    while(val != 0x1){
	val = readl(pcie_base + (0x1804 * 4));
	val = (val & (1 << 2)) >> 2;
    }
    while(val != 0x1){
	val = readl(pcie_base + (0x1904 * 4));
	val = (val & (1 << 2)) >> 2;
    }
    writel(0x0c11, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x1f;
    val = readl(pcie_base + (0x201c * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x201c * 4));
    val = readl(pcie_base + (0x201c * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x201c * 4));
    writel(0x0c12, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x1f;
    val = readl(pcie_base + (0x201e * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x201e * 4));
    val = readl(pcie_base + (0x201e * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x201e * 4));
    writel(0x0c13, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0xff;
    val = readl(pcie_base + (0x202c * 4)) & ~(0xff00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x202c * 4));
    val = readl(pcie_base + (0x202c * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x202c * 4));
    writel(0x0c16, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x20a4 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x20a4 * 4));
    val = readl(pcie_base + (0x2022 * 4)) | (0x1 << 0);
    writel(val, pcie_base + (0x2022 * 4));
    writel(0x0c17, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x20a4 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x20a4 * 4));
    val = readl(pcie_base + (0x2022 * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x2022 * 4));
    writel(0x0c18, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x20a6 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x20a6 * 4));
    val = readl(pcie_base + (0x2022 * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2022 * 4));
    writel(0x0c19, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x20a6 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x20a6 * 4));
    val = readl(pcie_base + (0x2022 * 4)) | (0x1 << 3);
    writel(val, pcie_base + (0x2022 * 4));
    writel(0x0c1a, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x20a8 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x20a8 * 4));
    val = readl(pcie_base + (0x2022 * 4)) | (0x1 << 4);
    writel(val, pcie_base + (0x2022 * 4));
    writel(0x0c1b, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x20a8 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x20a8 * 4));
    val = readl(pcie_base + (0x2022 * 4)) | (0x1 << 5);
    writel(val, pcie_base + (0x2022 * 4));
    writel(0x0c1c, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x20aa * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x20aa * 4));
    val = readl(pcie_base + (0x2022 * 4)) | (0x1 << 6);
    writel(val, pcie_base + (0x2022 * 4));
    writel(0x0c1d, pcie_base + (0x202e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x20aa * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x20aa * 4));
    val = readl(pcie_base + (0x2022 * 4)) | (0x1 << 7);
    writel(val, pcie_base + (0x2022 * 4));
    writel(0x0c11, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x1f;
    val = readl(pcie_base + (0x211c * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x211c * 4));
    val = readl(pcie_base + (0x211c * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x211c * 4));
    writel(0x0c12, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x1f;
    val = readl(pcie_base + (0x211e * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x211e * 4));
    val = readl(pcie_base + (0x211e * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x211e * 4));
    writel(0x0c13, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0xff;
    val = readl(pcie_base + (0x212c * 4)) & ~(0xff00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x212c * 4));
    val = readl(pcie_base + (0x212c * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x212c * 4));
    writel(0x0c16, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x21a4 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x21a4 * 4));
    val = readl(pcie_base + (0x2122 * 4)) | (0x1 << 0);
    writel(val, pcie_base + (0x2122 * 4));
    writel(0x0c17, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x21a4 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x21a4 * 4));
    val = readl(pcie_base + (0x2122 * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x2122 * 4));
    writel(0x0c18, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x21a6 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x21a6 * 4));
    val = readl(pcie_base + (0x2122 * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2122 * 4));
    writel(0x0c19, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x21a6 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x21a6 * 4));
    val = readl(pcie_base + (0x2122 * 4)) | (0x1 << 3);
    writel(val, pcie_base + (0x2122 * 4));
    writel(0x0c1a, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x21a8 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x21a8 * 4));
    val = readl(pcie_base + (0x2122 * 4)) | (0x1 << 4);
    writel(val, pcie_base + (0x2122 * 4));
    writel(0x0c1b, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x21a8 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x21a8 * 4));
    val = readl(pcie_base + (0x2122 * 4)) | (0x1 << 5);
    writel(val, pcie_base + (0x2122 * 4));
    writel(0x0c1c, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x21aa * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x21aa * 4));
    val = readl(pcie_base + (0x2122 * 4)) | (0x1 << 6);
    writel(val, pcie_base + (0x2122 * 4));
    writel(0x0c1d, pcie_base + (0x212e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x21aa * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x21aa * 4));
    val = readl(pcie_base + (0x2122 * 4)) | (0x1 << 7);
    writel(val, pcie_base + (0x2122 * 4));

    printf("s0_s1_phy_cali_gen1 done\n");
}

static void s0_s1_phy_cali_gen2(unsigned int pcie_base)
{
    uint32_t tmp, val;

    writel(0x8085, pcie_base + (0x1c5c * 4));
    writel(0x5059, pcie_base + (0x241e * 4));
    writel(0x5059, pcie_base + (0x251e * 4));
    writel(0x8185, pcie_base + (0x1c48 * 4));
    writel(0x818f, pcie_base + (0x1c48 * 4));
    writel(0x7059, pcie_base + (0x241e * 4));
    writel(0x7059, pcie_base + (0x251e * 4));
    writel(0x0c1e, pcie_base + (0x242e * 4));
    writel(0x0c1e, pcie_base + (0x252e * 4));
    while(val != 0x1){
        val = readl(pcie_base + (0x1804 * 4));
        val = (val & (1 << 2)) >> 2;
    }
    while(val != 0x1){
        val = readl(pcie_base + (0x1904 * 4));
        val = (val & (1 << 2)) >> 2;
    }
    writel(0x0c11, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x1f;
    val = readl(pcie_base + (0x241c * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x241c * 4));
    val = readl(pcie_base + (0x241c * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x241c * 4));
    writel(0x0c12, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x1f;
    val = readl(pcie_base + (0x241e * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x241e * 4));
    val = readl(pcie_base + (0x241e * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x241e * 4));
    writel(0x0c13, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0xff;
    val = readl(pcie_base + (0x242c * 4)) & ~(0xff00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x242c * 4));
    val = readl(pcie_base + (0x242c * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x241c * 4));
    writel(0x0c16, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x24a4 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x24a4 * 4));
    val = readl(pcie_base + (0x2422 * 4)) | (0x1 << 0);
    writel(val, pcie_base + (0x2422 * 4));
    writel(0x0c17, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x24a4 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x24a4 * 4));
    val = readl(pcie_base + (0x2422 * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x2422 * 4));
    writel(0x0c18, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x24a6 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x24a6 * 4));
    val = readl(pcie_base + (0x2422 * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2422 * 4));
    writel(0x0c19, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x24a6 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x24a6 * 4));
    val = readl(pcie_base + (0x2422 * 4)) | (0x1 << 3);
    writel(val, pcie_base + (0x2422 * 4));
    writel(0x0c1a, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x24a8 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x24a8 * 4));
    val = readl(pcie_base + (0x2422 * 4)) | (0x1 << 4);
    writel(val, pcie_base + (0x2422 * 4));
    writel(0x0c1b, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x24a8 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x24a8 * 4));
    val = readl(pcie_base + (0x2422 * 4)) | (0x1 << 5);
    writel(val, pcie_base + (0x2422 * 4));
    writel(0x0c1c, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x24aa * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x24aa * 4));
    val = readl(pcie_base + (0x2422 * 4)) | (0x1 << 6);
    writel(val, pcie_base + (0x2422 * 4));
    writel(0x0c1d, pcie_base + (0x242e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x24aa * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x24aa * 4));
    val = readl(pcie_base + (0x2422 * 4)) | (0x1 << 7);
    writel(val, pcie_base + (0x2422 * 4));
    writel(0x0c11, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x1f;
    val = readl(pcie_base + (0x251c * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x251c * 4));
    val = readl(pcie_base + (0x251c * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x251c * 4));
    writel(0x0c12, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x1f;
    val = readl(pcie_base + (0x251e * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x251e * 4));
    val = readl(pcie_base + (0x251e * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x251e * 4));
    writel(0x0c13, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0xff;
    val = readl(pcie_base + (0x252c * 4)) & ~(0xff00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x252c * 4));
    val = readl(pcie_base + (0x252c * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x252c * 4));
    writel(0x0c16, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x25a4 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x25a4 * 4));
    val = readl(pcie_base + (0x2522 * 4)) | (0x1 << 0);
    writel(val, pcie_base + (0x2522 * 4));
    writel(0x0c17, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x25a4 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x25a4 * 4));
    val = readl(pcie_base + (0x2522 * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x2522 * 4));
    writel(0x0c18, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x25a6 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x25a6 * 4));
    val = readl(pcie_base + (0x2522 * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2522 * 4));
    writel(0x0c19, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x25a6 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x25a6 * 4));
    val = readl(pcie_base + (0x2522 * 4)) | (0x1 << 3);
    writel(val, pcie_base + (0x2522 * 4));
    writel(0x0c1a, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x25a8 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x25a8 * 4));
    val = readl(pcie_base + (0x2522 * 4)) | (0x1 << 4);
    writel(val, pcie_base + (0x2522 * 4));
    writel(0x0c1b, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x25a8 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x25a8 * 4));
    val = readl(pcie_base + (0x2522 * 4)) | (0x1 << 5);
    writel(val, pcie_base + (0x2522 * 4));
    writel(0x0c1c, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x25aa * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x25aa * 4));
    val = readl(pcie_base + (0x2522 * 4)) | (0x1 << 6);
    writel(val, pcie_base + (0x2522 * 4));
    writel(0x0c1d, pcie_base + (0x252e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x25aa * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x25aa * 4));
    val = readl(pcie_base + (0x2522 * 4)) | (0x1 << 7);
    writel(val, pcie_base + (0x2522 * 4));

    printf("s0_s1_phy_cali_gen2 done\n");
}

static void s0_s1_phy_cali_gen3(unsigned int pcie_base)
{
    uint32_t tmp, val;

    writel(0x8086, pcie_base + (0x1c5c * 4));
    writel(0x5059, pcie_base + (0x281e * 4));
    writel(0x5059, pcie_base + (0x291e * 4));
    writel(0x8185, pcie_base + (0x1c48 * 4));
    writel(0x818f, pcie_base + (0x1c48 * 4));
    writel(0x7059, pcie_base + (0x281e * 4));
    writel(0x7059, pcie_base + (0x291e * 4));
    writel(0x0c1e, pcie_base + (0x282e * 4));
    writel(0x0c1e, pcie_base + (0x292e * 4));
    while(val != 0x1){
        val = readl(pcie_base + (0x1804 * 4));
        val = (val & (1 << 2)) >> 2;
    }
    while(val != 0x1){
        val = readl(pcie_base + (0x1904 * 4));
        val = (val & (1 << 2)) >> 2;
    }
    writel(0x0c11, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x1f;
    val = readl(pcie_base + (0x281c * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x281c * 4));
    val = readl(pcie_base + (0x281c * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x281c * 4));
    writel(0x0c12, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x1f;
    val = readl(pcie_base + (0x281e * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x281e * 4));
    val = readl(pcie_base + (0x281e * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x281e * 4));
    writel(0x0c13, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0xff;
    val = readl(pcie_base + (0x282c * 4)) & ~(0xff00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x282c * 4));
    val = readl(pcie_base + (0x282c * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x282c * 4));
    writel(0x0c16, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x28a4 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x28a4 * 4));
    val = readl(pcie_base + (0x2822 * 4)) | (0x1 << 0);
    writel(val, pcie_base + (0x2822 * 4));
    writel(0x0c17, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x28a4 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x28a4 * 4));
    val = readl(pcie_base + (0x2822 * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x2822 * 4));
    writel(0x0c18, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x28a6 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x28a6 * 4));
    val = readl(pcie_base + (0x2822 * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2822 * 4));
    writel(0x0c19, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x28a6 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x28a6 * 4));
    val = readl(pcie_base + (0x2822 * 4)) | (0x1 << 3);
    writel(val, pcie_base + (0x2822 * 4));
    writel(0x0c1a, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x28a8 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x28a8 * 4));
    val = readl(pcie_base + (0x2822 * 4)) | (0x1 << 4);
    writel(val, pcie_base + (0x2822 * 4));
    writel(0x0c1b, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x28a8 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x28a8 * 4));
    val = readl(pcie_base + (0x2822 * 4)) | (0x1 << 5);
    writel(val, pcie_base + (0x2822 * 4));
    writel(0x0c1c, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x28aa * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x28aa * 4));
    val = readl(pcie_base + (0x2822 * 4)) | (0x1 << 6);
    writel(val, pcie_base + (0x2822 * 4));
    writel(0x0c1d, pcie_base + (0x282e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x28aa * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x28aa * 4));
    val = readl(pcie_base + (0x2822 * 4)) | (0x1 << 7);
    writel(val, pcie_base + (0x2822 * 4));
    writel(0x0c11, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x1f;
    val = readl(pcie_base + (0x291c * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x291c * 4));
    val = readl(pcie_base + (0x291c * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x291c * 4));
    writel(0x0c12, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x1f;
    val = readl(pcie_base + (0x291e * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x291e * 4));
    val = readl(pcie_base + (0x291e * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x291e * 4));
    writel(0x0c13, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0xff;
    val = readl(pcie_base + (0x292c * 4)) & ~(0xff00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x292c * 4));
    val = readl(pcie_base + (0x292c * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x292c * 4));
    writel(0x0c16, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x29a4 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x29a4 * 4));
    val = readl(pcie_base + (0x2922 * 4)) | (0x1 << 0);
    writel(val, pcie_base + (0x2922 * 4));
    writel(0x0c17, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x29a4 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x29a4 * 4));
    val = readl(pcie_base + (0x2922 * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x2922 * 4));
    writel(0x0c18, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x29a6 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x29a6 * 4));
    val = readl(pcie_base + (0x2922 * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2922 * 4));
    writel(0x0c19, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x29a6 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x29a6 * 4));
    val = readl(pcie_base + (0x2922 * 4)) | (0x1 << 3);
    writel(val, pcie_base + (0x2922 * 4));
    writel(0x0c1a, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x29a8 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x29a8 * 4));
    val = readl(pcie_base + (0x2922 * 4)) | (0x1 << 4);
    writel(val, pcie_base + (0x2922 * 4));
    writel(0x0c1b, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x29a8 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x29a8 * 4));
    val = readl(pcie_base + (0x2922 * 4)) | (0x1 << 5);
    writel(val, pcie_base + (0x2922 * 4));
    writel(0x0c1c, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x29aa * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x29aa * 4));
    val = readl(pcie_base + (0x2922 * 4)) | (0x1 << 6);
    writel(val, pcie_base + (0x2922 * 4));
    writel(0x0c1d, pcie_base + (0x292e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x29aa * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x29aa * 4));
    val = readl(pcie_base + (0x2922 * 4)) | (0x1 << 7);
    writel(val, pcie_base + (0x2922 * 4));

    printf("s0_s1_phy_cali_gen3 done\n");
}

static void s0_s1_phy_cali_gen4(unsigned int pcie_base)
{
    uint32_t tmp, val;

    writel(0x8087, pcie_base + (0x1c5c * 4));
    writel(0x5059, pcie_base + (0x2c1e * 4));
    writel(0x5059, pcie_base + (0x2d1e * 4));
    writel(0x8185, pcie_base + (0x1c48 * 4));
    writel(0x818f, pcie_base + (0x1c48 * 4));
    writel(0x7059, pcie_base + (0x2c1e * 4));
    writel(0x7059, pcie_base + (0x2d1e * 4));
    writel(0x0c1e, pcie_base + (0x2c2e * 4));
    writel(0x0c1e, pcie_base + (0x2d2e * 4));
    while(val != 0x1){
        val = readl(pcie_base + (0x1804 * 4));
        val = (val & (1 << 2)) >> 2;
    }
    while(val != 0x1){
        val = readl(pcie_base + (0x1904 * 4));
        val = (val & (1 << 2)) >> 2;
    }
    writel(0x0c11, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x1f;
    val = readl(pcie_base + (0x2c1c * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2c1c * 4));
    val = readl(pcie_base + (0x2c1c * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2c1c * 4));
    writel(0x0c12, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x1f;
    val = readl(pcie_base + (0x2c1e * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2c1e * 4));
    val = readl(pcie_base + (0x2c1e * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2c1e * 4));
    writel(0x0c13, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0xff;
    val = readl(pcie_base + (0x2c2c * 4)) & ~(0xff00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2c2c * 4));
    val = readl(pcie_base + (0x2c2c * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x2c2c * 4));
    writel(0x0c16, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2ca4 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x2ca4 * 4));
    val = readl(pcie_base + (0x2c22 * 4)) | (0x1 << 0);
    writel(val, pcie_base + (0x2c22 * 4));
    writel(0x0c17, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2ca4 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2ca4 * 4));
    val = readl(pcie_base + (0x2c22 * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x2c22 * 4));
    writel(0x0c18, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2ca6 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x2ca6 * 4));
    val = readl(pcie_base + (0x2c22 * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2c22 * 4));
    writel(0x0c19, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2ca6 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2ca6 * 4));
    val = readl(pcie_base + (0x2c22 * 4)) | (0x1 << 3);
    writel(val, pcie_base + (0x2c22 * 4));
    writel(0x0c1a, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2ca8 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x2ca8 * 4));
    val = readl(pcie_base + (0x2c22 * 4)) | (0x1 << 4);
    writel(val, pcie_base + (0x2c22 * 4));
    writel(0x0c1b, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2ca8 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2ca8 * 4));
    val = readl(pcie_base + (0x2c22 * 4)) | (0x1 << 5);
    writel(val, pcie_base + (0x2c22 * 4));
    writel(0x0c1c, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2caa * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x2caa * 4));
    val = readl(pcie_base + (0x2c22 * 4)) | (0x1 << 6);
    writel(val, pcie_base + (0x2c22 * 4));
    writel(0x0c1d, pcie_base + (0x2c2e * 4));

    tmp = readl(pcie_base + (0x1804 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2caa * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2caa * 4));
    val = readl(pcie_base + (0x2c22 * 4)) | (0x1 << 7);
    writel(val, pcie_base + (0x2c22 * 4));
    writel(0x0c11, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x1f;
    val = readl(pcie_base + (0x2d1c * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2d1c * 4));
    val = readl(pcie_base + (0x2d1c * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2d1c * 4));
    writel(0x0c12, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x1f;
    val = readl(pcie_base + (0x2d1e * 4)) & ~(0x1f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2d1e * 4));
    val = readl(pcie_base + (0x2d1e * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2d1e * 4));
    writel(0x0c13, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0xff;
    val = readl(pcie_base + (0x2d2c * 4)) & ~(0xff00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2d2c * 4));
    val = readl(pcie_base + (0x2d2c * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x2d2c * 4));
    writel(0x0c16, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2da4 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x2da4 * 4));
    val = readl(pcie_base + (0x2d22 * 4)) | (0x1 << 0);
    writel(val, pcie_base + (0x2d22 * 4));
    writel(0x0c17, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2da4 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2da4 * 4));
    val = readl(pcie_base + (0x2d22 * 4)) | (0x1 << 1);
    writel(val, pcie_base + (0x2d22 * 4));
    writel(0x0c18, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2da6 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x2da6 * 4));
    val = readl(pcie_base + (0x2d22 * 4)) | (0x1 << 2);
    writel(val, pcie_base + (0x2d22 * 4));
    writel(0x0c19, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2da6 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2da6 * 4));
    val = readl(pcie_base + (0x2d22 * 4)) | (0x1 << 3);
    writel(val, pcie_base + (0x2d22 * 4));
    writel(0x0c1a, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2da8 * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x2da8 * 4));
    val = readl(pcie_base + (0x2d22 * 4)) | (0x1 << 4);
    writel(val, pcie_base + (0x2d22 * 4));
    writel(0x0c1b, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2da8 * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2da8 * 4));
    val = readl(pcie_base + (0x2d22 * 4)) | (0x1 << 5);
    writel(val, pcie_base + (0x2d22 * 4));
    writel(0x0c1c, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2daa * 4)) & ~(0x7f);
    val |= tmp << 0;
    writel(val, pcie_base + (0x2daa * 4));
    val = readl(pcie_base + (0x2d22 * 4)) | (0x1 << 6);
    writel(val, pcie_base + (0x2d22 * 4));
    writel(0x0c1d, pcie_base + (0x2d2e * 4));

    tmp = readl(pcie_base + (0x1904 * 4)) & 0x7f;
    val = readl(pcie_base + (0x2daa * 4)) & ~(0x7f00);
    val |= tmp << 8;
    writel(val, pcie_base + (0x2daa * 4));
    val = readl(pcie_base + (0x2d22 * 4)) | (0x1 << 7);
    writel(val, pcie_base + (0x2d22 * 4));

    printf("s0_s1_phy_cali_gen4 done\n");
}

static void s0_s1_phy_cali_end(unsigned int pcie_base)
{
    writel(0x8180, pcie_base + (0x1c48 * 4));
    writel(0x0000, pcie_base + (0x1c4a * 4));
    writel(0x0000, pcie_base + (0x1c4c * 4));
    writel(0x8080, pcie_base + (0x1c5c * 4));

    printf("s0_s1_phy_cali_end\n");
}

static void serdes_phy_calibration(void)
{
    s0_s1_phy_cali_start(0xf4700000);
    s0_s1_phy_cali_gen1(0xf4700000);
    s0_s1_phy_cali_gen2(0xf4700000);
    s0_s1_phy_cali_gen3(0xf4700000);
    s0_s1_phy_cali_gen4(0xf4700000);
    s0_s1_phy_cali_end(0xf4700000);

    s0_s1_phy_cali_start(0xf4710000);
    s0_s1_phy_cali_gen1(0xf4710000);
    s0_s1_phy_cali_gen2(0xf4710000);
    s0_s1_phy_cali_gen3(0xf4710000);
    s0_s1_phy_cali_gen4(0xf4710000);
    s0_s1_phy_cali_end(0xf4710000);
}

#define GEN3_RELATED_OFF_REG            0x890
#define GEN3_EQ_FS_LF_REG               0x894
#define GEN3_EQ_PSET_COEF_MAP_REG       0x898
#define GEN3_EQ_PSET_INDEX_OFF_REG      0x89c
#define GEN3_EQ_CONTROL_OFF             0x8a8
#define SPCIE_CAP_OFF_GEN3              0x180

static unsigned int pset_s0_s1[11][3] =
{
    {0x0, 0x1f, 0xb},
    {0x0, 0x23, 0x7},
    {0x0, 0x22, 0x8},
    {0x0, 0x25, 0x5},
    {0x0, 0x2a, 0x0},
    {0x4, 0x26, 0x0},
    {0x5, 0x25, 0x0},
    {0x4, 0x1e, 0x8},
    {0x5, 0x20, 0x5},
    {0x7, 0x23, 0x0},
    {0x0, 0x1c, 0xe}
};
static unsigned int pset_s2[11][3] =
{
    {0x0, 0x1e, 0xa},
    {0x0, 0x21, 0x7},
    {0x0, 0x20, 0x8},
    {0x0, 0x23, 0x5},
    {0x0, 0x28, 0x0},
    {0x4, 0x24, 0x0},
    {0x5, 0x23, 0x0},
    {0x4, 0x1c, 0x8},
    {0x5, 0x1e, 0x5},
    {0x7, 0x21, 0x0},
    {0x0, 0x1b, 0xd}
};

static void mac_preset(unsigned int pcie_base)
{
    uint32_t tmp, val;
    int pcie_gen;

    if (pcie_base == 0xf4700000) {
        pcie_gen = 4;
    } else if (pcie_base == 0xf4710000) {
        pcie_gen = 4;
    } else if (pcie_base == 0xf4720000) {
        pcie_gen = 3;
    }

    //Set enable
    for(int i=0;i<=(pcie_gen>>2);i++) {
        val = readl(pcie_base + GEN3_EQ_CONTROL_OFF);
        val &= ~(0x7ff00);//~GENMASK(18, 8)
        writel(val, pcie_base + GEN3_EQ_CONTROL_OFF);
        //set GEN3
        val = readl(pcie_base + GEN3_RELATED_OFF_REG);
        val &= ~(0x3000000);//~GENMASK(25, 24)
        val |= i << 24;
        writel(val, pcie_base + GEN3_RELATED_OFF_REG);

        //Write FS: 0x894[11:6], LF: 0x894[5:0]
        tmp = (0x2a << 6) + 0xe;//(pcie->pset_fs << 6)+pcie->pset_lf
        if (pcie_base == 0xf4720000)
            tmp = (0x28 << 6) + 0xc;//s2
        val = readl(pcie_base + GEN3_EQ_FS_LF_REG);
        val &= ~(0xfff);//~GENMASK(11, 0)
        val |= tmp;
        writel(val, pcie_base + GEN3_EQ_FS_LF_REG);

        for (int j=0;j<=10;j++) {
            val = readl(pcie_base + GEN3_EQ_PSET_INDEX_OFF_REG);
            val &= ~(0xf);//~GENMASK(3, 0)
            val = j;
            writel(val, pcie_base + GEN3_EQ_PSET_INDEX_OFF_REG);
            tmp = 0;
            val = pset_s0_s1[j][0];//Pre
            if (pcie_base == 0xf4720000)
                val = pset_s2[j][0];
            tmp |= val << 0;
            val = pset_s0_s1[j][1];//Main
            if (pcie_base == 0xf4720000)
                val = pset_s2[j][1];
            tmp |= val << 6;
            val = pset_s0_s1[j][2];//Post
            if (pcie_base == 0xf4720000)
                val = pset_s2[j][2];
            tmp |= val << 12;
            writel(tmp, pcie_base + GEN3_EQ_PSET_COEF_MAP_REG);
        }

        val = readl(pcie_base + SPCIE_CAP_OFF_GEN3);
        val &= ~(0xf);//~GENMASK(3, 0)
        tmp = 0x1;//DSP RX_PRESET0
        val |= tmp << 0;
        val &= ~(0x70);//~GENMASK(6, 4)
        tmp = 0x1;//DSP_TX_HINT0
        val |= tmp << 4;
        val &= ~(0xf0000);//~GENMASK(19, 16)
        tmp = 0x1;//DSP_TX_PRESET1
        val |= tmp << 16;
        val &= ~(0x700000);//~GENMASK(22, 20)
        tmp = 0x1;//DSP_TX_HINT1
        val |= tmp << 20;
        writel(val, pcie_base + SPCIE_CAP_OFF_GEN3);
        //set enable
        tmp = readl(pcie_base + GEN3_EQ_CONTROL_OFF);
        tmp &= ~(0x7ff00);//~GENMASK(18, 8)
        tmp |= 0x7ff00;
        writel(tmp, pcie_base + GEN3_EQ_CONTROL_OFF);
    }

    printf("mac_preset done\n");
}

static unsigned int txm_data_val[147] = {
    0x0000, 0x3C0, 0x00,
    0x0821, 0x3C1, 0x01,
    0x1042, 0x3C2, 0x02,
    0x1863, 0x3C3, 0x03,
    0x2084, 0x3C4, 0x04,
    0x28A6, 0x3C5, 0x05,
    0x30C7, 0x3C6, 0x06,
    0x40E8, 0x3C7, 0x07,
    0x4909, 0x3C8, 0x08,
    0x5129, 0x3C9, 0x09,
    0x5949, 0x3CA, 0x0A,
    0x6169, 0x3CB, 0x0B,
    0x6989, 0x3CC, 0x0C,
    0x71A9, 0x3CD, 0x0D,
    0x71C9, 0x3CE, 0x0E,
    0x79E9, 0x3CF, 0x0F,
    0x7A09, 0x3D0, 0x10,
    0x7A49, 0x3D1, 0x11,
    0x7A69, 0x3D2, 0x12,
    0x7A89, 0x3D3, 0x13,
    0x7AA9, 0x3D4, 0x14,
    0x7AE9, 0x3D5, 0x15,
    0x7B09, 0x3D6, 0x16,
    0x7B29, 0x3D7, 0x17,
    0x7B69, 0x3D8, 0x18,
    0x7B89, 0x3D9, 0x19,
    0x7BC9, 0x3DA, 0x1A,
    0x7C09, 0x3DB, 0x1B,
    0x7C49, 0x3DC, 0x1C,
    0x7C89, 0x3DD, 0x1D,
    0x7CC9, 0x3DE, 0x1E,
    0x7D09, 0x3DF, 0x1F,
    0x7D69, 0x3E0, 0x20,
    0x7DC9, 0x3E1, 0x21,
    0x7E29, 0x3E2, 0x22,
    0x7EA9, 0x3E3, 0x23,
    0x7F69, 0x3E4, 0x24,
    0x7FE9, 0x3E5, 0x25,
    0x7FE9, 0x3E6, 0x26,
    0x7FE9, 0x3E7, 0x27,
    0x7FE9, 0x3E8, 0x28,
    0x7FE9, 0x3E9, 0x29,
    0x7FE9, 0x3EA, 0x2A,
    0x7FE9, 0x3EB, 0x2B,
    0x7FE9, 0x3EC, 0x2C,
    0x7FE9, 0x3ED, 0x2D,
    0x7FE9, 0x3EE, 0x2E,
    0x7FE9, 0x3EF, 0x2F,
    0x7FE9, 0x3F0, 0x30
};

static void pcie_tx_matrix_patch(unsigned int pcie_base)
{
    int txm_data_size = 147;
    int ephy_addr[3] = {0x1c62, 0x1c60, 0x1c60};
    uint32_t val, result, addr;

    for(int i=0;i<txm_data_size;i++) {
        if(i%3 != 0) {
            addr = pcie_base + (ephy_addr[i%3] * 4);
            result = readl(addr);
            result &= ~(0x3ff);
            val = txm_data_val[i] & 0x3ff;
            result |= val;
            writel(result, addr);
        } else {
            addr = pcie_base + (ephy_addr[i%3] * 4);
            result = readl(addr);
            result &= ~(0xffff);
            val = txm_data_val[i] & 0xffff;
            result |= val;
            writel(result, addr);
        }
        mdelay(2);
    }
}

ft2_ret_t ft2_pcie_test(ft2_record_t *record)
{
    ft2_ret_t rc = FT2_OK;
    uint32_t val;
    uint32_t status0, status1, status2, status7, status8;
    uint32_t speed0, speed1, speed2, speed7, speed8;
    uint8_t value, value1;

#if (PCIE_MASK > 0)

    printf("FT2_PARAM_PCIE result ...\n");

    record->entry_mask = PCIE_MASK;
    record->start = get_timer(0);
    record->entry_test[0] |= 1;

    ft2_gpio_mux_set(GPIO_PCIE0);
    ft2_gpio_mux_set(GPIO_PCIE1);
    ft2_gpio_mux_set(GPIO_PCIE2);
    ft2_gpio_mux_set(GPIO_PCIE7);
    ft2_gpio_mux_set(GPIO_PCIE8);
    gpio_request(GPIO_PCIE0, "NULL");//QA board s0 4
    gpio_request(GPIO_PCIE1, "NULL");//QA board s1 5
    gpio_request(GPIO_PCIE2, "NULL");//QA board s2 6
    gpio_request(GPIO_PCIE7, "NULL");//QA board s7 79
    gpio_request(GPIO_PCIE8, "NULL");//QA board s8 8
    gpio_direction_output(GPIO_PCIE0, 0);
    gpio_direction_output(GPIO_PCIE1, 0);
    gpio_direction_output(GPIO_PCIE2, 0);
    gpio_direction_output(GPIO_PCIE7, 0);
    gpio_direction_output(GPIO_PCIE8, 0);
    mdelay(150);

    reset_control_assert();

    pcie0_phy_parameter();
    pcie1_phy_parameter();
    pcie2_phy_parameter();
    pcie7_phy_parameter();
    pcie8_phy_parameter();

    pcie_phy_power_on();

    reset_control_deassert();

    //EQ training
    /*pcie0*/
    writel(0x770701EC, 0xf47008f8);
    writel(0x000f7770, 0xf47008fc);
    /*pcie1*/
    writel(0x770701EC, 0xf47108f8);
    writel(0x000f7770, 0xf47108fc);
    /*pcie2*/
    writel(0x770701EC, 0xf47208f8);
    writel(0x000f7770, 0xf47208fc);
    /*pcie7*/
    writel(0x770701EC, 0xf43308f8);
    writel(0x000f7770, 0xf43308fc);
    /*pcie8*/
    writel(0x770701EC, 0xf43318f8);
    writel(0x000f7770, 0xf43318fc);

    //manaual_LEQ(0xf4700000);
    //manaual_LEQ(0xf4710000);
    //manaual_LEQ(0xf4720000);

    serdes_phy_calibration();

    /* Gen3 & Gen4 do this
    ref: https://cn2sd5.rtkbf.com/gerrit/c/rlxlinux/kernel/linux-5.10.x/+/122614 */
    mac_preset(0xf4700000);
    mac_preset(0xf4710000);
    mac_preset(0xf4720000);

    pcie_tx_matrix_patch(0xf4700000);
    pcie_tx_matrix_patch(0xf4710000);

    val = readl(GLOBAL_PHY_STATUS);//0xf43200c4
    printf("GLOBAL_PHY_STATUS: 0x%x\n", val);

    pcie0_rc_atu_setup();
    pcie1_rc_atu_setup();
    pcie2_rc_atu_setup();
    pcie7_rc_atu_setup();
    pcie8_rc_atu_setup();

    /* assert LTSSM enable */
    writel(0x00000080, PCIE0_X2_PCIE4X2_GLBL_DWC_cfg);//pcie0 0xf4325018
    writel(0x00000080, PCIE1_X1_PCIE4X1_GLBL_DWC_cfg);//pcie1 0xf4325818
    writel(0x00000080, PCIE2_X1_PCIE3_GLBL_DWC_cfg);//pcie2 0xf4326018
    writel(0x00000080, PCIE7_X1_PCIE2_GLBL_DWC_cfg);//pcie7 0xf4326818
    writel(0x00000080, PCIE8_X1_PCIE2_GLBL_DWC_cfg);//pcie8 0xf4327018
    mdelay(10);

    gpio_direction_output(GPIO_PCIE0, 1);
    gpio_direction_output(GPIO_PCIE1, 1);
    gpio_direction_output(GPIO_PCIE2, 1);
    gpio_direction_output(GPIO_PCIE7, 1);
    gpio_direction_output(GPIO_PCIE8, 1);
    mdelay(150);

    int cnt = 0;
    do {
        mdelay(1);
        val = 0x20;
        status0 = readl(PCIE0_X2_PCIE4X2_GLBL_DWC_info1);//0xf4325010
        if (val == (status0 & 0x20))//check link
            break;
    } while (++cnt < 10);

    status0 = readl(PCIE0_X2_PCIE4X2_GLBL_DWC_info1);
    speed0 = readl(0xa0000080);
    FT2_DBG("status0: 0x%x, speed0: 0x%x\n", status0, speed0);

    if ((status0 & 0x20) != 0x20) {
        printf("\n[pcie0 not link up]\n");
        printf("status0: 0x%x, bit[5] should be 1\n", status0);
        rc = -1;
    } else {
        printf("\n[pcie0 link up]\n");
        if ((speed0 >> 16 & 0xf) == 0x1) {
            printf("Current Link Speed 2.5GT/s\n");
            rc = -1;
        } else if ((speed0 >> 16 & 0xf) == 0x2) {
            printf("Current Link Speed 5.0GT/s\n");
            rc = -1;
        } else if ((speed0 >> 16 & 0xf) == 0x3) {
            printf("Current Link Speed 8.0GT/s\n");
            rc = -1;
        } else if ((speed0 >> 16 & 0xf) == 0x4) {
            printf("Current Link Speed 16.0GT/s Gen4\n");
        } else {
            printf("Link speed error\n");
            rc = -1;
        }

        if ((speed0 >> 16 & 0xf0) == 0x10) {
            printf("Current Link Width x1\n");
            rc = -1;
        } else if ((speed0 >> 16 & 0xf0) == 0x20) {
            printf("Current Link Width x2 two lane\n");
        } else if ((speed0 >> 16 & 0xf0) == 0x40) {
            printf("Current Link Width x4\n");
            rc = -1;
        } else {
            printf("Link Width Error\n");
            rc = -1;
        }
    }

    cnt = 0;
    do {
        mdelay(1);
        val = 0x20;
        status1 = readl(PCIE1_X1_PCIE4X1_GLBL_DWC_info1);//0xf4325810
        if (val == (status1 & 0x20))//check link
            break;
    } while (++cnt < 10);

    status1 = readl(PCIE1_X1_PCIE4X1_GLBL_DWC_info1);
    speed1 = readl(0xb0000080);
    FT2_DBG("status1: 0x%x, speed1: 0x%x\n", status1, speed1);

    if ((status1 & 0x20) != 0x20) {
        printf("\n[pcie1 not link up]\n");
        printf("status1: 0x%x, bit[5] should be 1\n", status1);
        rc = -1;
    } else {
        printf("\n[pcie1 link up]\n");
        if ((speed1 >> 16 & 0xf) == 0x1) {
            printf("Current Link Speed 2.5GT/s\n");
            rc = -1;
        } else if ((speed1 >> 16 & 0xf) == 0x2) {
            printf("Current Link Speed 5.0GT/s\n");
            rc = -1;
        } else if ((speed1 >> 16 & 0xf) == 0x3) {
            printf("Current Link Speed 8.0GT/s\n");
            rc = -1;
        } else if ((speed1 >> 16 & 0xf) == 0x4) {
            printf("Current Link Speed 16.0GT/s Gen4\n");
        } else {
            printf("Link speed error\n");
            rc = -1;
        }

        if ((speed1 >> 16 & 0xf0) == 0x10) {
            printf("Current Link Width x1\n");
            rc = -1;
        } else if ((speed1 >> 16 & 0xf0) == 0x20) {
            printf("Current Link Width x2 two lane\n");
        } else if ((speed1 >> 16 & 0xf0) == 0x40) {
            printf("Current Link Width x4\n");
            rc = -1;
        } else {
            printf("Link Width Error\n");
            rc = -1;
        }
    }

    cnt = 0;
    do {
        mdelay(1);
        val = 0x20;
        status2 = readl(PCIE2_X1_PCIE3_GLBL_DWC_info1);//0xf4326010
        if (val == (status2 & 0x20))//check link
            break;
    } while (++cnt < 10);

    status2 = readl(PCIE2_X1_PCIE3_GLBL_DWC_info1);
    speed2 = readl(0xc0000080);
    FT2_DBG("status2: 0x%x, speed2: 0x%x\n", status2, speed2);

    if ((status2 & 0x20) != 0x20) {
        printf("\n[pcie2 not link up]\n");
        printf("status2: 0x%x, bit[5] should be 1\n", status2);
        rc = -1;
    } else {
        printf("\n[pcie2 link up]\n");
        if ((speed2 >> 16 & 0xf) == 0x1) {
            printf("Current Link Speed 2.5GT/s\n");
            rc = -1;
        } else if ((speed2 >> 16 & 0xf) == 0x2) {
            printf("Current Link Speed 5.0GT/s\n");
            rc = -1;
        } else if ((speed2 >> 16 & 0xf) == 0x3) {
            printf("Current Link Speed 8.0GT/s Gen3\n");
        } else if ((speed2 >> 16 & 0xf) == 0x4) {
            printf("Current Link Speed 16.0GT/s\n");
            rc = -1;
        } else {
            printf("Link speed error\n");
            rc = -1;
        }

        if ((speed2 >> 16 & 0xf0) == 0x10) {
            printf("Current Link Width x1 one lane\n");
        } else if ((speed2 >> 16 & 0xf0) == 0x20) {
            printf("Current Link Width x2\n");
            rc = -1;
        } else if ((speed2 >> 16 & 0xf0) == 0x40) {
            printf("Current Link Width x4\n");
            rc = -1;
        } else {
            printf("Link Width Error\n");
            rc = -1;
        }
    }

    cnt = 0;
    do {
        mdelay(1);
        val = 0x20;
        status7 = readl(PCIE7_X1_PCIE2_GLBL_DWC_info1);//0xf4326810
        if (val == (status7 & 0x20))//check link
            break;
    } while (++cnt < 10);

    status7 = readl(PCIE7_X1_PCIE2_GLBL_DWC_info1);
    speed7 = readl(0xd0000080);
    FT2_DBG("status7: 0x%x, speed7: 0x%x\n", status7, speed7);

    if ((status7 & 0x20) != 0x20) {
        printf("\n[pcie7 not link up]\n");
        printf("status7: 0x%x, bit[5] should be 1\n", status7);
        rc = -1;
    } else {
        printf("\n[pcie7 link up]\n");
        if ((speed7 >> 16 & 0xf) == 0x1) {
            printf("Current Link Speed 2.5GT/s\n");
            rc = -1;
        } else if ((speed7 >> 16 & 0xf) == 0x2) {
            printf("Current Link Speed 5.0GT/s Gen2\n");
        } else if ((speed7 >> 16 & 0xf) == 0x3) {
            printf("Current Link Speed 8.0GT/s\n");
            rc = -1;
        } else if ((speed7 >> 16 & 0xf) == 0x4) {
            printf("Current Link Speed 16.0GT/s\n");
            rc = -1;
        } else {
            printf("Link speed error\n");
            rc = -1;
        }

        if ((speed7 >> 16 & 0xf0) == 0x10) {
            printf("Current Link Width x1 one lane\n");
        } else if ((speed7 >> 16 & 0xf0) == 0x20) {
            printf("Current Link Width x2\n");
            rc = -1;
        } else if ((speed7 >> 16 & 0xf0) == 0x40) {
            printf("Current Link Width x4\n");
            rc = -1;
        } else {
            printf("Link Width Error\n");
            rc = -1;
        }
    }

    cnt = 0;
    do {
        mdelay(1);
        val = 0x20;
        status8 = readl(PCIE8_X1_PCIE2_GLBL_DWC_info1);//0xf4327010
        if (val == (status8 & 0x20))//check link
            break;
    } while (++cnt < 10);

    status8 = readl(PCIE8_X1_PCIE2_GLBL_DWC_info1);
    speed8 = readl(0xd8000080);
    FT2_DBG("status8: 0x%x, speed8: 0x%x\n", status8, speed8);

    if ((status8 & 0x20) != 0x20) {
        printf("\n[pcie8 not link up]\n");
        printf("status8: 0x%x, bit[5] should be 1\n", status8);
        rc = -1;
    } else {
        printf("\n[pcie8 link up]\n");
        if ((speed8 >> 16 & 0xf) == 0x1) {
            printf("Current Link Speed 2.5GT/s\n");
            rc = -1;
        } else if ((speed8 >> 16 & 0xf) == 0x2) {
            printf("Current Link Speed 5.0GT/s Gen2\n");
        } else if ((speed8 >> 16 & 0xf) == 0x3) {
            printf("Current Link Speed 8.0GT/s\n");
            rc = -1;
        } else if ((speed8 >> 16 & 0xf) == 0x4) {
            printf("Current Link Speed 16.0GT/s\n");
            rc = -1;
        } else {
            printf("Link speed error\n");
            rc = -1;
        }

        if ((speed8 >> 16 & 0xf0) == 0x10) {
            printf("Current Link Width x1 one lane\n");
        } else if ((speed8 >> 16 & 0xf0) == 0x20) {
            printf("Current Link Width x2\n");
            rc = -1;
        } else if ((speed8 >> 16 & 0xf0) == 0x40) {
            printf("Current Link Width x4\n");
            rc = -1;
        } else {
            printf("Link Width Error\n");
            rc = -1;
        }
    }

    record->finish = get_timer(record->start) * 1000 / CONFIG_SYS_HZ;
    record->module_rlt = rc;
    record->entry_rlt[0] = rc ? 1 : 0;

    printf("%s\n", rc ? "FAIL" : "PASS");
    FT2_DBG("test takes %ld.%03d secs\n", record->finish/1000, (int)(record->finish%1000));

    return rc;
#else
    printf("No PCIE test\n");
    return FT2_PCIE;
#endif
}

int do_ft2_pcie_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    test_prepare(ft2_records, FT2_PARAM_MAX);
    ft2_pcie_test(&ft2_records[FT2_PARAM_PCIE]);
    test_result(ft2_records, FT2_PARAM_MAX, (1 << FT2_PARAM_PCIE));
    return 0;
}

void u3_phy_data(void)
{
    writel(0x0001, 0xf4737100);
    writel(0x0002, 0xf47346B0);
    writel(0x0002, 0xf4734770);
    writel(0x3139, 0xf4737030);
    writel(0x4501, 0xf4730564);
    writel(0x4501, 0xf47306A4);
    writel(0x2002, 0xf473058C);
    writel(0x2303, 0xf47306CC);
    writel(0x0160, 0xf4730590);
    writel(0x0160, 0xf47306D0);
    writel(0x0110, 0xf4730594);
    writel(0x0110, 0xf47306D4);
    writel(0x1821, 0xf4730598);
    writel(0x1821, 0xf47306D8);
    writel(0x0262, 0xf47305B0);
    writel(0x0262, 0xf47306F0);
    writel(0x6842, 0xf4730654);
    writel(0x6842, 0xf4730794);
    writel(0x003a, 0xf4730670);
    writel(0x003a, 0xf47307B0);
    writel(0xc1fc, 0xf47346C0);
    writel(0xc1fc, 0xf4734780);
    writel(0xe87f, 0xf47346C8);
    writel(0xe87f, 0xf4734788);
    writel(0x00c6, 0xf4737D48);
    writel(0x00c6, 0xf4737D88);
    writel(0x00c6, 0xf4737D60);
    writel(0x00c6, 0xf4737DA0);
    writel(0x0080, 0xf4737D68);
    writel(0x0080, 0xf4737DA8);
    writel(0x0280, 0xf4737D70);
    writel(0x0280, 0xf4737DB0);
    writel(0x0280, 0xf4737D78);
    writel(0x0280, 0xf4737DB8);
    writel(0x0020, 0xf4737D68);
    writel(0x0020, 0xf4737DA8);
    writel(0x0260, 0xf4730658);
    writel(0x0360, 0xf4730658);
    writel(0x0260, 0xf4730798);
    writel(0x0360, 0xf4730798);
    writel(0x0949, 0xf4736320);
    writel(0x0949, 0xf4736360);
    writel(0x031a, 0xf4736330);
    writel(0x031a, 0xf4736370);
    writel(0x00c5, 0xf47364C0);
    writel(0x00c5, 0xf4736500);
    writel(0x00c5, 0xf47364D8);
    writel(0x00c5, 0xf4736518);
    writel(0x0001, 0xf47364D0);
    writel(0x999a, 0xf47364C8);
    writel(0x0001, 0xf4736510);
    writel(0x999a, 0xf4736508);
    writel(0x0008, 0xf47364E8);
    writel(0xcccd, 0xf47364E0);
    writel(0x0008, 0xf4736528);
    writel(0xcccd, 0xf4736520);
    writel(0x0001, 0xf4736300);
    writel(0x0003, 0xf4736300);
    writel(0x0001, 0xf4736300);
    writel(0x0001, 0xf4736340);
    writel(0x0003, 0xf4736340);
    writel(0x0001, 0xf4736340);
    writel(0x9180, 0xf4734650);
    writel(0x9100, 0xf4734710);
    writel(0x1402, 0xf473058C);
    writel(0x1703, 0xf47306CC);
    writel(0x00c0, 0xf47306E0);
    writel(0x0020, 0xf47306DC);

    writel(0x04c3, 0xf030d064);
    writel(0x84c0, 0xf030d068);
    writel(0x84c3, 0xf030d060);
    writel(0x04c3, 0xf020d064);
    writel(0x84c0, 0xf020d068);
    writel(0x84c3, 0xf020d060);
#if 1
    writel(0x5084, 0xf4730554);
    writel(0x1084, 0xf4730554);
    writel(0x5084, 0xf4730554);
    writel(0x1084, 0xf4730554);
    writel(0x5084, 0xf4730694);
    writel(0x1084, 0xf4730694);
    writel(0x5084, 0xf4730694);
    writel(0x1084, 0xf4730694);
#endif
    mdelay(20);
}

ft2_ret_t ft2_usb_test(ft2_record_t *record)
{
    ft2_ret_t rc = FT2_OK;
    uint32_t val;
    uint32_t status0, status1, status2, status7, status8;
    uint32_t speed0, speed1, speed2, speed7, speed8;
    uint8_t value, value1;

#if (USB_MASK > 0)

    printf("FT2_PARAM_USB result ...\n");

    record->entry_mask = USB_MASK;
    record->start = get_timer(0);
    record->entry_test[0] |= 1;

    GLOBAL_PHY_CONTROL_A_t g_phy_ctrl_a;//0xf43200b8
    g_phy_ctrl_a.wrd = readl(GLOBAL_PHY_CONTROL_A);
    g_phy_ctrl_a.bf.s2_pcie_usb3_sel = 0;
    g_phy_ctrl_a.bf.s8_pcie_usb3_sel = 0;
    g_phy_ctrl_a.bf.s2_p_mdio_enable_reg = 0;
    g_phy_ctrl_a.bf.s6_p_mdio_enable_reg = 0;
    g_phy_ctrl_a.bf.s8_p_mdio_enable_reg = 0;
    writel(g_phy_ctrl_a.wrd, GLOBAL_PHY_CONTROL_A);
    mdelay(20);

    GLOBAL_PHY_MISC_CONTROL_t g_phy_misc;//0xf43200c0
    g_phy_misc.wrd = readl(GLOBAL_PHY_MISC_CONTROL);
    g_phy_misc.bf.s2_USB3_BG_EN = 1;
    g_phy_misc.bf.s8_USB3_BG_EN = 1;
    writel(g_phy_misc.wrd, GLOBAL_PHY_MISC_CONTROL);
    mdelay(20);

    GLOBAL_PHY_ISO_POWER_CTRL_t g_phy_pow_ctrl;//0xf43203e0
    g_phy_pow_ctrl.wrd = readl(GLOBAL_PHY_ISO_POWER_CTRL);
    g_phy_pow_ctrl.bf.s6_POW_USB31 = 0;
    g_phy_pow_ctrl.bf.s2_POW_USB3 = 0;
    g_phy_pow_ctrl.bf.s8_POW_USB3 = 0;
    g_phy_pow_ctrl.bf.s8_ISO_ANA_B_V09 = 0;
    writel(g_phy_pow_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);
    mdelay(20);

    GLOBAL_BLOCK_RESET1_t g_blk_reset1;//0xf4320074
    g_blk_reset1.wrd = readl(GLOBAL_BLOCK_RESET1);
    g_blk_reset1.bf.reset_usb3_s6 = 1;
    g_blk_reset1.bf.reset_usb3_s2 = 1;
    g_blk_reset1.bf.reset_usb3_s8 = 1;
    writel(g_blk_reset1.wrd, GLOBAL_BLOCK_RESET1);
    mdelay(20);

    /* usb2 phy reset */
    /* USB2 DPPULLDOWN/DMPULLDOWN */
    writel(0x00080064, USBCFG_U2PHY_PORT0_CONFIG);//0xf432b130
    writel(0x00080064, USBCFG_U2PHY_PORT1_CONFIG);//0xf432b138
    mdelay(20);

    g_phy_ctrl_a.wrd = readl(GLOBAL_PHY_CONTROL_A);
    g_phy_ctrl_a.bf.s2_pcie_usb3_sel = 1;
    g_phy_ctrl_a.bf.s8_pcie_usb3_sel = 1;
    g_phy_ctrl_a.bf.s1_p_mdio_enable_reg = 0;//
    g_phy_ctrl_a.bf.s2_p_mdio_enable_reg = 1;//
    g_phy_ctrl_a.bf.s6_p_mdio_enable_reg = 1;//
    g_phy_ctrl_a.bf.s8_p_mdio_enable_reg = 1;//
    writel(g_phy_ctrl_a.wrd, GLOBAL_PHY_CONTROL_A);
    mdelay(20);

    g_phy_misc.wrd = readl(GLOBAL_PHY_MISC_CONTROL);
    g_phy_misc.bf.s2_USB3_BG_EN = 1;
    g_phy_misc.bf.s8_USB3_BG_EN = 1;
    writel(g_phy_misc.wrd, GLOBAL_PHY_MISC_CONTROL);
    mdelay(20);

    g_phy_pow_ctrl.wrd = readl(GLOBAL_PHY_ISO_POWER_CTRL);
    g_phy_pow_ctrl.bf.s2_POW_USB3 = 1;//
    g_phy_pow_ctrl.bf.s6_POW_USB31 = 1;
    g_phy_pow_ctrl.bf.s8_POW_USB3 = 1;//
    g_phy_pow_ctrl.bf.s8_ISO_ANA_B_V09 = 1;//
    writel(g_phy_pow_ctrl.wrd, GLOBAL_PHY_ISO_POWER_CTRL);
    mdelay(20);

    g_blk_reset1.wrd = readl(GLOBAL_BLOCK_RESET1);
    g_blk_reset1.bf.reset_usb3_s2 = 0;
    g_blk_reset1.bf.reset_usb3_s6 = 0;
    g_blk_reset1.bf.reset_usb3_s8 = 0;
    writel(g_blk_reset1.wrd, GLOBAL_BLOCK_RESET1);
    mdelay(20);

    writel(0x111111, USBCFG_USB_TOP_CONTROL0);//0xf432b000
    mdelay(20);

    writel(0x0a408012, 0xf040c12c);
    mdelay(2);
    writel(0x0a87f000, 0xf040c630);
    mdelay(2);
    writel(0x0a414802, 0xf020c12c);
    mdelay(2);
    writel(0x0a87f020, 0xf020c630);
    mdelay(2);

    printf("u3_phy_data before\n");
    //u3 phy data
    u3_phy_data();
    printf("u3_phy_data after\n");

    //usb3 set rx termination
    writel(0x0, USBCFG_U31_P0_EXT_CONTROL);//0xf432b020
    writel(0x0, USBCFG_U31_P1_EXT_CONTROL);//0xf432b050
    writel(0x0, USBCFG_U3_P2_EXT_CONTROL);//0xf432b090
    mdelay(20);

    //u2 phy data

    //usb2 set rx termination
    writel(0x2, USBCFG_U2_P0_EXT_CONTROL);//0xf432b01c
    writel(0x2, USBCFG_U2_P1_EXT_CONTROL);//0xf432b04c
    mdelay(20);

#if 0

    writel(0x00121313, 0xf4320004);
    mdelay(2);
#endif

    usb_init();//u2 need to run usb_init() then check portsc




    record->finish = get_timer(record->start) * 1000 / CONFIG_SYS_HZ;
    record->module_rlt = rc;
    record->entry_rlt[0] = rc ? 1 : 0;

    printf("%s\n", rc ? "FAIL" : "PASS");
    FT2_DBG("test takes %ld.%03d secs\n", record->finish/1000, (int)(record->finish%1000));

    return rc;
#else
    printf("No USB test\n");
    return FT2_USB;
#endif
}

int do_ft2_usb_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    test_prepare(ft2_records, FT2_PARAM_MAX);
    ft2_usb_test(&ft2_records[FT2_PARAM_USB]);
    test_result(ft2_records, FT2_PARAM_MAX, (1 << FT2_PARAM_USB));
    return 0;
}

extern void dcache_enable(void);
extern void dcache_disable(void);

static int wakeup_cpu(uint64_t sec_entrypoint, uint32_t cpu)
{
    GLOBAL_ARM_RESET_t arm_reset;

    writel(sec_entrypoint >> 2, GLOBAL_ARM_CONFIG_D0);

    /* Assert reset signal */
    arm_reset.wrd = readl(GLOBAL_ARM_RESET);
    arm_reset.bf.CPUPORESET |= 1 << cpu;
    arm_reset.bf.CORERESET |= 1 << cpu;
    writel(arm_reset.wrd, GLOBAL_ARM_RESET);

    __asm__ volatile ("isb":::"memory");

    /* Release CPU */
    arm_reset.wrd = readl(GLOBAL_ARM_RESET);
    arm_reset.bf.CPUPORESET &=~(1 << cpu);
    arm_reset.bf.CORERESET &= ~(1 << cpu);
    writel(arm_reset.wrd, GLOBAL_ARM_RESET);

    udelay(10);
    /* Default Reset vector */
    writel(0xFFFF0000 >> 2, GLOBAL_ARM_CONFIG_D0);

    return 0;
}

static int do_cpu_on(void)
{
    unsigned int cpu_idx;

    dcache_enable();

    udelay(1000);
    for(cpu_idx=1 ; cpu_idx < 4;cpu_idx++) {
        wakeup_cpu(0x4000000, cpu_idx);
        mdelay(5);
    }

    dcache_disable();

    return 0;
}

#define CPU_CORE_NUM 4
#define CPU_CORE_VERIFY_ADDR 0x80000000
/* need manually modify file 
   arch/arm/cpu/armv8/cache_v8.c
   arch/arm/cpu/armv8/start.S */
ft2_ret_t ft2_cpu_test(ft2_record_t *record)
{
    ft2_ret_t rc = FT2_OK;
    char cmd[64];
    int cmd_ret;
    uint32_t cpu, addr, value;

#if (CPU_CORE_MASK > 0)

    printf("FT2_PARAM_CPU test ...\n");

    record->entry_mask = CPU_CORE_MASK;
    record->start = get_timer(0);
    record->entry_test[0] |= 1;

    /* wake up cpu */
    do_cpu_on();

    /* check cpu 1,2,3 results */
    for (cpu = 1; cpu < CPU_CORE_NUM; cpu++) {
        if (!((CPU_CORE_MASK >> cpu) & 0x1))
            continue;
        record->entry_test[cpu] = 1;
        addr = CPU_CORE_VERIFY_ADDR + 4*cpu;
        if (handshake(addr, 0xffff, 0x900d, 8000000)) {
            printf("cpu %d fail\n", cpu);
            rc |= FT2_CPU;
            record->entry_rlt[cpu] = 1;
        }
        value = readl(addr);
        printf("%s: cpu#%d, addr = 0x%04x, value = 0x%04x\n", __func__, cpu, addr, value);
    }

    record->finish = get_timer(record->start) * 1000 / CONFIG_SYS_HZ;
    record->module_rlt = rc;

    printf("%s\n", rc ? "FAIL" : "PASS");
    FT2_DBG("test takes %ld.%03d secs\n", record->finish/1000, (int)(record->finish%1000));

    return rc ? FT2_CPU : FT2_OK;
#else
    printf("No CPU test\n");
    return FT2_CPU;
#endif
}

int do_ft2_cpu_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    test_prepare(ft2_records, FT2_PARAM_MAX);
    ft2_cpu_test(&ft2_records[FT2_PARAM_CPU]);
    test_result(ft2_records, FT2_PARAM_MAX, (1 << FT2_PARAM_CPU));
    return 0;
}

ft2_ret_t ft2_riscv_test(ft2_record_t *record)
{
    ft2_ret_t rc = FT2_OK;
    uint32_t value0, value1, value2, value3;

#if (RISCV_MASK > 0)

    printf("FT2_PARAM_RISCV result ...\n");

    record->entry_mask = RISCV_MASK;
    record->start = get_timer(0);
    record->entry_test[0] |= 1;

    printf("*************\n");

    writel(0xfd1027f3, 0x80000000);
    mdelay(10);
    writel(0xf79383d1, 0x80000004);
    mdelay(10);
    writel(0x06b73ff7, 0x80000008);
    mdelay(10);
    writel(0x9713bfc0, 0x8000000c);
    mdelay(10);
    writel(0x86930027, 0x80000010);
    mdelay(10);
    writel(0x97360406, 0x80000014);
    mdelay(10);
    writel(0x5afe06b7, 0x80000018);
    mdelay(10);
    writel(0xc31c8fd5, 0x8000001c);
    mdelay(10);
    writel(0xa001a001, 0x80000020);
    mdelay(10);

    writel(0x7d00005d, 0xf4320070);
    mdelay(10);
    writel(0x000000ff, 0xf4320074);
    mdelay(10);

    value0 = readl(0x80000040);
    value1 = readl(0x80000044);
    value2 = readl(0x80000048);
    value3 = readl(0x80000050);

    if ((value0 == 0x5afe0000) && (value1 == 0x5afe0001) && (value2 == 0x5afe0002) && (value3 == 0x5afe0004)) {
        printf("riscv pass\n");
    } else {
        printf("0x80000040: 0x%x\n", value0);
        printf("0x80000044: 0x%x\n", value1);
        printf("0x80000048: 0x%x\n", value2);
        printf("0x80000050: 0x%x\n", value3);
        record->entry_rlt[0] |= 0x1;
        rc = FT2_RISCV;
    }

    record->finish = get_timer(record->start) * 1000 / CONFIG_SYS_HZ;

    printf("%s\n", rc ? "FAIL" : "PASS");
    FT2_DBG("test takes %ld.%03d secs\n", record->finish/1000, (int)(record->finish%1000));

    return rc;
#else
    printf("No RISCV test\n");
    return FT2_RISCV;
#endif
}

int do_ft2_riscv_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    test_prepare(ft2_records, FT2_PARAM_MAX);
    ft2_riscv_test(&ft2_records[FT2_PARAM_RISCV]);
    test_result(ft2_records, FT2_PARAM_MAX, (1 << FT2_PARAM_RISCV));
    return 0;
}

int do_ft2_otp_access(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    int ret=0;
    u8 *rbuf = (u8 *)0x05000000;
    u32 byte_loc;

    ca_uint32_t op = 0, value = 0;
    if (argc <= 1) {
        return 0;
    } else {
        /* get addr */
        byte_loc = simple_strtoul(argv[1], NULL, 16);
        /* get value */
        if (NULL != argv[2]) {
            value = simple_strtoul(argv[2], NULL, 16);
            op = 1;
        }
    }
    if (op) {
        printf("Write OTP\n");
        OTP_WRITE_BYTE(byte_loc, value);
    } else {
        printf("Read OTP\n");
        ret = OTP_READ_BYTE(byte_loc, (unsigned long)&rbuf[0]);
        printf("OTP %x : 0x%02x\n", byte_loc, rbuf[0]);
    }

    return 0;
}

int do_ft2_mdio_access(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t op = 0;

    ca_uint8_t device_id;
    ca_uint8_t st_code;
    ca_uint8_t phy_port_addr;
    ca_uint8_t reg_dev_addr;
    ca_uint16_t addr;
    ca_uint16_t *data;

    uint16_t out_data;

    //gpiomux gpio4-2, gpio4-3
    uint32_t gpio_val;
    gpio_val = readl(GLOBAL_GPIO_MUX_4);
    gpio_val &= ~((0x1 << 2) | (0x1 << 3));
    writel(gpio_val, GLOBAL_GPIO_MUX_4);
/*
    //mdio pinmux set
    //GLOBAL_PIN_MUX_2 -> iomux_en_mdio
    uint32_t mdio_en = readl(GLOBAL_PIN_MUX_2);
    mdio_en |= 0x100000;
    writel(mdio_en, GLOBAL_PIN_MUX_2);
    printf("GLOBAL_PIN_MUX_2: 0x%x\n", readl(GLOBAL_PIN_MUX_2));
*/

    if (argc <= 1) {
        return 0;
    } else {
        device_id = simple_strtoul(argv[1], NULL, 16);
        st_code = simple_strtoul(argv[2], NULL, 10);
        phy_port_addr = simple_strtoul(argv[3], NULL, 16);
        reg_dev_addr = simple_strtoul(argv[4], NULL, 10);
        addr = simple_strtoul(argv[5], NULL, 16);
        if (NULL != argv[6]) {
            data = simple_strtoul(argv[6], NULL, 16);
            op = 1;
        }
    }

    //aal_mdio_read_indirect(0, 7, 31, 0xa400, &out_data);
    if (op) {
        printf("mdio write\n");
        ret = aal_mdio_write(device_id, st_code, phy_port_addr, reg_dev_addr, addr, data);
        if (ret)
            printf("write mdio fail\n");
    } else {
        ret = aal_mdio_read(device_id, st_code, phy_port_addr, reg_dev_addr, addr, &out_data);
        if (ret)
            printf("read mdio fail\n");
            printf("mdio read data: 0x%x\n", out_data);
    }

    return 0;
}

#define CHAR_ENTER      0x0d
static int console_input(void)
{
    int v = 0;
    char c;
    while (1) {
        c = getchar();
        if (c == 0)
            continue;
        if ((c >= '0') && (c <= '9')) {
            putc(c);
            v *= 10;
            v += c - '0';
        } else if (c == CHAR_ENTER) {
            break;
        } else {
            continue;
        }
    }
    return v;
}

int thermal(void)
{
    uint32_t result;

    return result;
}

int do_ft2_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    uint32_t test = DEFAULT_TESTS;
    int thermal_before, thermal_after;
    ft2_start = get_timer(0);

    test_prepare(ft2_records, FT2_PARAM_MAX);

    thermal_before = thermal();
    printf("thermal_before: %d\n", thermal_before/1024);

    system_info();

    if (test & (1 << FT2_PARAM_PRBS_PON)) {
        ft2_prbs_pon_test(&ft2_records[FT2_PARAM_PRBS_PON]);
    }

    if (test & (1 << FT2_PARAM_PRBS_PON_RESULT)) {
        ft2_prbs_pon_result(&ft2_records[FT2_PARAM_PRBS_PON_RESULT]);
    }

    if (test & (1 << FT2_PARAM_GPIO)) {
    }

    thermal_after = thermal();
    printf("thermal_after: %d\n", thermal_after/1024);
    printf("thermal_diff: %d\n", (thermal_after-thermal_before)/1024);

    ft2_finish = get_timer(ft2_start) * 1000 / CONFIG_SYS_HZ;

    test_result(ft2_records, FT2_PARAM_MAX, test);

    return 0;
}

static cmd_tbl_t cmd_ft2_sub[] = {
    U_BOOT_CMD_MKENT(test, 2, 1, do_ft2_test, "", ""),
    U_BOOT_CMD_MKENT(prbs_pon, 2, 1, do_ft2_prbs_pon_test, "", ""),
    U_BOOT_CMD_MKENT(pon_ben, 2, 1, do_ft2_pon_ben_test, "", ""),
    U_BOOT_CMD_MKENT(pcie, 2, 1, do_ft2_pcie_test, "", ""),
    U_BOOT_CMD_MKENT(usb, 2, 1, do_ft2_usb_test, "", ""),
    U_BOOT_CMD_MKENT(cpu, 2, 1, do_ft2_cpu_test, "", ""),
    U_BOOT_CMD_MKENT(riscv, 2, 1, do_ft2_riscv_test, "", ""),
    U_BOOT_CMD_MKENT(allotp, 2, 1, do_ft2_read_all_otp, "", ""),
    U_BOOT_CMD_MKENT(otp_rw, 2, 1, do_ft2_otp_access, "", ""),
    U_BOOT_CMD_MKENT(mdio, 5, 1, do_ft2_mdio_access, "", ""),
    U_BOOT_CMD_MKENT(dbg, 2, 1, do_ft2_dbg, "", "")
};

static char ft2_help_text[] =
    "prbs_pon - Run prbs pon port test\n"
    "ft pon_ben - Run pon ben test\n"
    "ft riscv - Run risc-v test\n"
    "ft gpio - gpio test\n"
    "ft allotp - Read chip model, bonding option, HVSID OTP\n"
    "ft dbg <value> - Dump more debug messages\n";

int do_ft2(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    cmd_tbl_t *c;
    if (argc < 2)
        return CMD_RET_USAGE;

    /* Strip off leading 'ft2' command argument */
    argc--;
    argv++;

    c = find_cmd_tbl(argv[0], &cmd_ft2_sub[0], ARRAY_SIZE(cmd_ft2_sub));
    if (c)
        return c->cmd(cmdtp, flag, argc, argv);
    else
        return CMD_RET_USAGE;
}

U_BOOT_CMD(
    ft, 20, 1, do_ft2,
    "FT2 utility commands",
    ft2_help_text
);
