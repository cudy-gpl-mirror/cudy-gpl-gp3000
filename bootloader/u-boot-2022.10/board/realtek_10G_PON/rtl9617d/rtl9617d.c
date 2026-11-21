/*
 * (C) Copyright 2018 -
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <asm/global_data.h>
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <asm/io.h>
#include <linux/sizes.h>
#include <linux/compiler.h>
#include <linux/psci.h>
#include <fdt_support.h>
#include <irq_func.h>
#include <dwmmc.h>
#include <command.h>
#ifdef CONFIG_MTD_CORTINA_SERIAL_NOR
#include <spi_flash.h>
#endif
#include <linux/delay.h>
#include "../common/smc_call/cortina_smc.h"

#ifndef CONFIG_LAST_STAGE_INIT
#error CONFIG_LAST_STAGE_INIT should be enabled on RTL9617D platform
#endif

DECLARE_GLOBAL_DATA_PTR;
int ca77xx_eth_initialize(struct bd_info *bis);

#ifdef CONFIG_RTK_LED_STYLE_GPIO
	void rtk_gpio_leds_init(void);
#endif

static noinline int invoke_psci_fn_smc(u64 function_id, u64 arg0, u64 arg1,u64 arg2)
{
#ifdef CONFIG_ARM64
	asm volatile(
	  "mov x0, %0\n"
	  "mov x1, %1\n"
	  "mov x2, %2\n"
	  "mov x3, %3\n"
	  "smc	#0\n"
	  : "+r"(function_id)
	  : "r"(arg0), "r"(arg1), "r"(arg2));
#else
	asm volatile(
	  ".arch_extension sec\n"
	  "mov r0, %0\n"
	  "mov r1, %1\n"
	  "mov r2, %2\n"
	  "mov r3, %3\n"
	  "smc	#0\n"
	  : "+r"(function_id)
	  : "r"(arg0), "r"(arg1), "r"(arg2));

#endif

	return function_id;
}


#define XRAM_BASE 0xF4500000
#define XRAM_SIZE 0x2000
static void *xram_buf;
void board_preboot_os(void)
{
	if (xram_buf)
	{
		uint32_t val, port;
		unsigned long reg;

		#define NI_HV_PT_RXMAC_CFG_BASE		0xf430a5c8
		#define NI_HV_PT_RXMAC_CFG_STRIDE	0x90

		for (port = 0; port < 7; port++) {
			reg = NI_HV_PT_RXMAC_CFG_BASE + (port * NI_HV_PT_RXMAC_CFG_STRIDE);
			val = readl(reg) & ~0x1;
			writel(val, reg);
		}
		memcpy((void *)XRAM_BASE, xram_buf, XRAM_SIZE);
	}
}


int board_init(void)
{
	xram_buf = malloc(XRAM_SIZE);
	if (xram_buf)
	{
		memcpy(xram_buf, (void *)XRAM_BASE, XRAM_SIZE);
		printf("%s(%d):%p\n",__func__,__LINE__,xram_buf);
	}

#ifdef CONFIG_RTK_LED_STYLE_GPIO
	rtk_gpio_leds_init();
#endif

#ifdef CONFIG_MTD_CORTINA_SERIAL_NOR
    struct spi_flash *sflash __attribute__((unused));
    sflash = spi_flash_probe(0, 0, 0, 0);
#endif

#if !defined(CONFIG_RTK_V7_FPGA_PLATFORM) && !defined(CONFIG_RTK_V8_FPGA_PLATFORM)
extern void ca77xx_eth_change_default(void);
	ca77xx_eth_change_default();
#endif

	return 0;
}

int dram_init(void) {
	unsigned int ddr_size, i = 0;
	ddr_size = readl(0x111100c);

	/* DRAM size should be filled by DDR init code(BL2).\
	 * Hard code ddr_size here if no BL2!
	 */
    if (ddr_size == 128) {
        /* tail 256KB of 128MB are reserved for secure firmware(BL31) */
        gd->bd->bi_dram[i].start = PHYS_SDRAM_1;
        gd->bd->bi_dram[i].size = 128 * 0x100000 - 256 * 0x400;
        gd->ram_size = gd->bd->bi_dram[i].size;
        i++;
    }

	if (ddr_size > 240) { /* tail of 256M are reserved for secure firmware(BL31/BL32) */
		gd->bd->bi_dram[i].start = PHYS_SDRAM_1;
		gd->bd->bi_dram[i].size = 240 * 0x100000;
		gd->ram_size = 240 * 0x100000;
		i++;
	}

	/* segment #2 start from offset of 256M
     * Only 2GB physical mapping area is useable for u-boot booting
     */
	if (ddr_size > 256) {
		gd->bd->bi_dram[i].start = PHYS_SDRAM_1 + SZ_256M;
		gd->bd->bi_dram[i].size  = ((ddr_size <= 2048) ? (ddr_size - 256) : (2048 - 256)) * 0x100000;
		gd->ram_size += gd->bd->bi_dram[i].size;
		i++;
	}

	/* Segment #3:
	 * Only 2GB physical mapping area is useable for u-boot booting
	 */
	gd->bd->bi_dram[i].start = 0;
	gd->bd->bi_dram[i].size = 0;

	return 0;
}

int dram_init_banksize(void) {
#if defined(CONFIG_REALTEK_10G_PON) && defined(CONFIG_NR_DRAM_BANKS)
    unsigned int bit_width;
    unsigned int dram_type;
    unsigned int ddr_size;
    unsigned int ddr_freq;
    bit_width = readl(0x1111004)*8;
    dram_type = readl(0x1111008);
    ddr_size  = readl(0x111100c);
    ddr_freq  = readl(0x1111010);

    printf("\b\b\b\b\b\b\b%s%4d: %d-bit mode, ", (dram_type == 0) ? "DDR3-" : "DDR4-", ddr_freq * 2, bit_width);

    gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
    gd->bd->bi_dram[0].size = ((ddr_size > 2048) ? 2048 : ddr_size) * 0x100000;

    if ((ddr_size > 2048) && (ddr_size <= 4096)) {
        gd->bd->bi_dram[1].start = DDR_ALIAS_BASE + SZ_2G;
        gd->bd->bi_dram[1].size = (ddr_size - 2048) * 0x100000;
    }

    gd->bd->bi_dram[2].start = 0;
    gd->bd->bi_dram[2].size = 0;
#else
	dram_init();
#endif
	return 0;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
extern uint64_t rtk_cortex_freq_get(void);

int show_board_info(void)
{
    char model_name[16];

    if(readl(0x111111c) == 0x111111c) {
        ca_smc_call(CA_SVC_GET_MODEL_NAME, 0, 0, (unsigned long)model_name, 0);
        printf("Board: %s\n",model_name);
    }

    printf("CortexA55: %d MHz\n",(uint32_t)rtk_cortex_freq_get());
    return 0;
}
#endif

/*
 * Board specific reset that is system reset.
 */
static void ddr_reset_mode_config(uint32_t ddr_warm_reset)
{
    /* Restore to DDR cold reset mode */
    GLOBAL_DDR_RESET_MODE_t reg_v;
    reg_v.wrd = readl(GLOBAL_DDR_RESET_MODE);
    reg_v.bf.ddr_warm_rst_mode = (ddr_warm_reset)?1:0;
    writel(reg_v.wrd, GLOBAL_DDR_RESET_MODE);

    if (ddr_warm_reset)
        puts ("Reboot with DRAM controller warm reset ...\n");
}

static void system_reset(void)
{
    invoke_psci_fn_smc(PSCI_0_2_FN_SYSTEM_RESET, 0, 0, 0);
}

void reset_cpu(void)
{
    /* Configurate DDR reset type */
    ddr_reset_mode_config(do_warm_reset);

    /* Doing system reset*/
	system_reset();

	return;
}

/*
 * Board specific ethernet initialization routine.
 */
int board_eth_init(struct bd_info *bis) {
	int rc = 0;

#ifdef CONFIG_RTK_NE
	rtk_eth_initialize(bis);
#else
#ifndef CONFIG_RTK_V8_FPGA_PLATFORM
	ca77xx_eth_initialize(bis);
#endif
#endif

	return rc;
}

#ifdef CONFIG_LAST_STAGE_INIT
extern void  board_env_fix(void);
int last_stage_init(void) {
    /* BL2 in FIP0 will enable WDOG timeout and indicate enable status in REG(GLOBAL_SCRATCH)
     * When boots up to here successfully, clear REG(GLOBAL_SCRATCH) and disable WDOG rest
     * This is developed for [run upf] command
     */
	if((readl(GLOBAL_SCRATCH)&0xF)==0xE) {
		writel(0, PER_WDT_CTRL);
	}
	writel(0, GLOBAL_SCRATCH);

    /* Clear BL31 exists flag */
	writel(0, 0x111111c);

	board_env_fix();

	return 0;
}
#endif
