/*
 * Realtek Semiconductor Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright 2014  Tony Wu (tonywu@realtek.com)
 */

#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>

#include <linux/clkdev.h>
#include <linux/clk-provider.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <asm/smp_twd.h>

#define WDT_CTRL_OFFSET			0x0
#define WDT_PS_OFFSET			0x4
#define WDT_LD_OFFSET			0xC
#define WDT_LOADE_OFFSET		0x10

static void __init plat_map_io(void)
{

}

static void __init plat_init_machine(void)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

static void plat_arch_reset(char mode, const char *cmd)
{
}

void (*arch_reset)(char, const char *) = plat_arch_reset;

static void plat_restart(enum reboot_mode mode, const char *cmd)
{
	printk("[%s %d]Empty function\n", __func__, __LINE__);
}

static const char * const plat_dt_match[] __initconst = {
	"cortina,venus",
	"cortina,ca8277B",
	"realtek,taurus",
	"realtek,9607f",
	NULL
};

DT_MACHINE_START(RLXARM_DT, "Realtek ARM (Flattened Device Tree)")
	.dt_compat = plat_dt_match,
	.init_machine = plat_init_machine,
	.map_io = plat_map_io,
	.restart = plat_restart,
MACHINE_END
