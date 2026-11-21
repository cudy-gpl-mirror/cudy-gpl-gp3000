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
#include <asm/armv8/mmu.h>
#include <linux/sizes.h>

DECLARE_GLOBAL_DATA_PTR;

#define CA_PERIPH_BASE                  0xE0000000UL
#define CA_PERIPH_SIZE                  0x20000000UL

#define CA_DDR_ALIAS_BASE               0x200000000

#define CA_SRAM_APB1_BASE			0x80000000UL
#define CA_SRAM_APB1_SIZE			0x20000000UL
#define CA_GIC_BASE			        0xF8000000
#define CA_GIC_SIZE			        0x00040000

int ca77xx_eth_initialize(struct bd_info *bis);

static struct mm_region mercury_mem_map[] = {
/*
	{
	.virt = DDR_BASE,
	.phys = DDR_BASE,
	.size = PHYS_SDRAM_1_SIZE,
	.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
		 PTE_BLOCK_OUTER_SHARE
	},
*/
	{
	.virt = PHYS_NS_DRAM_0,
	.phys = PHYS_NS_DRAM_0,
	.size = PHYS_NS_DRAM_SIZE0,
	.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
		 PTE_BLOCK_OUTER_SHARE
	},
	{
	.virt = PHYS_NS_DRAM_1,
	.phys = PHYS_NS_DRAM_1,
	.size = PHYS_NS_DRAM_SIZE1,
	.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
		 PTE_BLOCK_OUTER_SHARE
	},
	{
	.virt = CA_PERIPH_BASE,
	.phys = CA_PERIPH_BASE,
	.size = CA_PERIPH_SIZE,
	.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
		 PTE_BLOCK_NON_SHARE
	},
	{
	.virt = CA_SRAM_APB1_BASE,
	.phys = CA_SRAM_APB1_BASE,
	.size = CA_SRAM_APB1_SIZE,
	.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
		PTE_BLOCK_NON_SHARE | PTE_BLOCK_PXN | PTE_BLOCK_UXN
	},
	{
	.virt = CA_GIC_BASE,
	.phys = CA_GIC_BASE,
	.size = CA_GIC_SIZE,
	.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
		PTE_BLOCK_NON_SHARE | PTE_BLOCK_PXN | PTE_BLOCK_UXN
	},
	{
	/* List terminator */
	0,
	}
};

struct mm_region *mem_map = mercury_mem_map;

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

int board_early_init_r(void)
{
	dcache_disable();
	return 0;
}

int board_init(void)
{
	/* Enable timer */
	writel(1, CONFIG_SYS_TIMER_BASE);


#ifdef CONFIG_CORTINA_LED
	int i;
#endif

#ifdef CONFIG_CORTINA_LED
	ca77xx_leds_init(GLOBAL_LED_CONTROL, led_blink_rate1, led_blink_rate2);

	for (i = 0; i < CORTINA_LED_NUM; i++)
		ca77xx_led_config(i, led_cfg[i][0], led_cfg[i][1], led_cfg[i][2],
		                  led_cfg[i][3], led_cfg[i][4], led_cfg[i][5]);

#	ifdef CONFIG_RTK_LED_STYLE_1
	__led_set(1 << RTK_POWER_LED, STATUS_LED_ON);
#	endif
#endif // CONFIG_CORTINA_LED
#ifdef CONFIG_RTK_LED_STYLE_GPIO
void rtk_gpio_leds_init(void);
	rtk_gpio_leds_init();
#endif

#ifdef CONFIG_MTD_CORTINA_SERIAL_NOR
    struct spi_flash *sflash __attribute__((unused));
    sflash = spi_flash_probe(0, 0, 0, 0);
#endif

#if !defined(CONFIG_RTK_V7_FPGA_PLATFORM) && !defined(CONFIG_RTK_V8_FPGA_PLATFORM) && !defined(CONFIG_RTK_ZEBU_SIM)
extern void ca77xx_eth_change_default(void);
	ca77xx_eth_change_default();
#endif

	return 0;
}

int dram_init(void)
{
	unsigned int ddr_size, i = 0;

	ddr_size = readl(0x111100c);

	/* DRAM size should be filled by DDR init code(BL2).\
	 * Hard code ddr_size here if no BL2!
	 */

	if (ddr_size > 240) {
		/* tail of 256M are reserved for secure firmware(BL31/BL32) */
		gd->bd->bi_dram[i].start = PHYS_SDRAM_1;
		gd->bd->bi_dram[i].size = 240 * 0x100000;
		gd->ram_size = gd->bd->bi_dram[i].size;
		i++;
	}

	/* segment #2 start from offset of 256M */
	if (ddr_size > 256) {
		if (ddr_size <= 2048)
			gd->bd->bi_dram[i].size = (ddr_size - 256) * 0x100000;
		else
			gd->bd->bi_dram[i].size = (2048 - 256) * 0x100000;

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

int dram_init_banksize(void)
{
    unsigned int i = 0;
    unsigned int bit_width;
    unsigned int dram_type;
    unsigned int ddr_size;
    unsigned int ddr_freq;
    bit_width = readl(0x1111004)*8;
    dram_type = readl(0x1111008);
    ddr_size = readl(GLOBAL_DRAM_SIZE_RCOD)?readl(GLOBAL_DRAM_SIZE_RCOD):512;
    ddr_freq  = readl(0x1111010);

    printf("\b\b\b\b\b\b\b%s%4d: %d-bit mode, ", (dram_type == 0) ? "LPDDR4-" : "LPDDR5-", ddr_freq * 2, bit_width);

    gd->bd->bi_dram[i].start = PHYS_SDRAM_1;
    gd->bd->bi_dram[i].size = ((ddr_size >= 2048) ? 2048 : ddr_size) * 0x100000;
    gd->ram_size = gd->bd->bi_dram[i].size;
    i++;

    /* Segment #1:
     * Tail of base address 0 are for PCI and IO port.
     * So we must use DDR alias base to add DDR bank.
     */
    if (ddr_size > 2048) {
        gd->bd->bi_dram[i].start = CA_DDR_ALIAS_BASE + SZ_2G;
        gd->bd->bi_dram[i].size = (unsigned long long)(ddr_size - 2048) * 0x100000;
        gd->ram_size += gd->bd->bi_dram[i].size;
        i++;
    }

    for ( ; i < CONFIG_NR_DRAM_BANKS; ++i) {
        gd->bd->bi_dram[i].start = 0;
        gd->bd->bi_dram[i].size = 0;
    }

    return 0;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
extern uint64_t rtk_cortex_freq_get(void);

int show_board_info(void)
{
    printf("CortexA73: %d MHz\n",(uint32_t)rtk_cortex_freq_get());
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

	ca77xx_eth_initialize(bis);

	return rc;
}

/*
 * Board specific ethernet initialization routine.
 */
#ifdef CONFIG_LAST_STAGE_INIT
int last_stage_init(void) {
    /* BL2 in FIP0 will enable WDOG timeout to 23 seconds and FORCE_BOOT_STANDBY_BL3_FLAG
     * When boots up to here successfully, clear FORCE_BOOT_STANDBY_BL3_FLAG and disable WDOG rest
     * This is developed for [run upf] command
     */
	writel(0, GLOBAL_SCRATCH);
#if defined(CONFIG_RTK_V8_FPGA_PLATFORM) || defined(CONFIG_RTK_ZEBU_SIM)
	if((readl(PER_WDT_DIV)==1000) && (readl(PER_WDT_LD)==40)) {
#else
    if((readl(PER_WDT_DIV)==1000) && (readl(PER_WDT_LD)==23)) {
#endif
        writel(0, PER_WDT_CTRL);
    }
	return 0;
}
#endif
