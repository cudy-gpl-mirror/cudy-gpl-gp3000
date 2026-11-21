/*
 * (C) Copyright 2024
 * SPDX-License-Identifier:	GPL-2.0+
 */
/*
 * Experiment Purpose:
 * TAURUS/ELNATH: HW not support Power domain for 4 portion.
 * So L3 partialPower down just not access partial RAM and not power cut.
 * Issue: 
 *        1. WDT reset hang on 1/4,2/4,3/4
 *        2. WDT reset doesn't reset L3Cache when FullDown.
 */
 
/*
 * Misc functions
 */
#include <common.h>
#include <command.h>
#include <cpu_func.h>
#include <asm/io.h>
#include <linux/delay.h>
#include "../../smc_call/cortina_smc.h"
#define L3_CMD_DEBUG 1
#if L3_CMD_DEBUG
#define L3_P_DEBUG(fmt, args...)	printf(fmt, ##args)
#else
#define L3_P_DEBUG(fmt, args...)
#endif


#define RTK_SVC_SREG_ACTLR_SET               (SIP_SVC_CALL|CA_SVC_MISC|4)
#define RTK_SVC_SREG_ACTLR_READ               (SIP_SVC_CALL|CA_SVC_MISC|5)

/* Bit-field positions for the CLUSTERPWRCTLR_EL1 register */
#define PORTION_1					4
#define PORTION_2					5
#define PORTION_3					6
#define PORTION_4					7
/* Bit-masks for the CLUSTERPWRCTLR_EL1 register */
#define PORTION_BITS		((1UL << PORTION_1) | \
				 (1UL << PORTION_2) | \
				 (1UL << PORTION_3) | \
				 (1UL << PORTION_4))
#define PORTION_MASK		(~(PORTION_BITS))
#define PORTION_MAX	4

#define GLOBAL_ARM_STATE	0xf4320118
#define CLUSTERPDENY		BIT(21)
#define CLUSTERPACCEPT		BIT(20)
#define CLUSTERPACTIVE_PORTION_SHIT	16
#define CLUSTERPACTIVE_PORTION_BITS(x)	BIT(x + CLUSTERPACTIVE_PORTION_SHIT)


#define GLOBAL_ARM_CONFIG_B	0xf4320140
#define CLUSTERPREQ	BIT(7)
#define CLUSTERPSTATE_MASK	0x7f
/************************/
/*
 * Read the system register 'sysreg' (using MRS), and then
 * explicitly clear it, by writing zero to it (using MSR).
 * Put the value read into 'result'.
 */
#define SYS_REG_READ_THEN_CLEAR(sysreg, result)		\
do {							\
	__asm__ __volatile__(				\
		"mrs	%0, " #sysreg		"\n\t"	\
		"msr	" #sysreg ", XZR"		\
		: "=r" (result) /* only one output */	\
		/* no inputs */				\
		/* no clobbers */			\
	);						\
} while (0)

/*
 * Read the system register 'sysreg' (using MRS).
 * Put the value read into 'result'.
 */
#define SYS_REG_READ(sysreg, result)			\
do {							\
	__asm__ __volatile__(				\
		"mrs	%0, " #sysreg			\
		: "=r" (result) /* only one output */	\
		/* no inputs */				\
		/* no clobbers */			\
	);						\
} while (0)

/*
 * Write 'value' to the system register 'sysreg' (using MSR).
 */
#define SYS_REG_WRITE(sysreg, value)			\
do {							\
	__asm__ __volatile__(				\
		"msr	" #sysreg ", %0"		\
		: /* no outputs */			\
		: "r" (value) /* only one input */	\
		/* no clobbers */			\
	);						\
} while (0)
/*****************************/
static u32 portion_active_val = 0xF;

static int l3cache_partial_powerdown(u32 portion_mode){
	int i;
	u32 reg_v;
	u32 global_arm_config_b_v;
	u32 check_bit;
	u32 timeout = 10000;
	u8 cmd;
	int ret = 0;
	L3_P_DEBUG("%s %d: 0x%x, 0x%x\n", __func__, __LINE__, portion_active_val, portion_mode);
	if (portion_active_val > portion_mode) {
		/* Disable l3 portion*/
		for (i=3;i>=0;i--) {
			check_bit = (1<<i);
			if ((check_bit & portion_active_val) != (check_bit & portion_mode)){
				if(check_bit & portion_mode) {
					/* nothing */
				} else {
					mdelay(10);
					reg_v = readl(GLOBAL_ARM_STATE);
					if((reg_v & CLUSTERPACTIVE_PORTION_BITS(i)) == 0) {
						global_arm_config_b_v = readl(GLOBAL_ARM_CONFIG_B);
						L3_P_DEBUG("%s %d: 0x%x\n", __func__, __LINE__, global_arm_config_b_v);
						cmd = (i<<4) | 0x8;
						global_arm_config_b_v &= ~CLUSTERPSTATE_MASK;
						global_arm_config_b_v |= cmd;
						L3_P_DEBUG("%s %d: 0x%x\n", __func__, __LINE__, global_arm_config_b_v);
						writel(global_arm_config_b_v, GLOBAL_ARM_CONFIG_B);
						global_arm_config_b_v |= CLUSTERPREQ;
						L3_P_DEBUG("%s %d: 0x%x\n", __func__, __LINE__, global_arm_config_b_v);
						writel(global_arm_config_b_v, GLOBAL_ARM_CONFIG_B);
						timeout = 3000;
						while(timeout){
							/* CLUSTERPDENY		BIT(21)
							   CLUSTERPACCEPT		BIT(20)
							 */
								reg_v = readl(GLOBAL_ARM_STATE);
								if(reg_v & CLUSTERPDENY){
									ret = -1;
									goto err_exit;
								}else if (reg_v & CLUSTERPACCEPT){
									global_arm_config_b_v &= ~CLUSTERPREQ;
						L3_P_DEBUG("%s %d: 0x%x\n", __func__, __LINE__, global_arm_config_b_v);
									writel(global_arm_config_b_v, GLOBAL_ARM_CONFIG_B);
									portion_active_val = portion_active_val & ~(1<<i);
						L3_P_DEBUG("%s %d: 0x%x, 0x%x\n", __func__, __LINE__, global_arm_config_b_v, portion_active_val);
									break;
								}
								timeout--;
						}
					}
				}
					
			}
		}
	} else {
		for (i=0;i<4;i++) {
			check_bit = (1<<i);
				L3_P_DEBUG("%s %d: 0x%x\n", __func__, __LINE__, i);
			if ((check_bit & portion_active_val) != (check_bit & portion_mode)){
								L3_P_DEBUG("%s %d: 0x%x 0x%x 0x%x\n", __func__, __LINE__, check_bit,portion_active_val,portion_mode );
				if(check_bit & portion_mode) {
					mdelay(10);
					reg_v = readl(GLOBAL_ARM_STATE);
					if (reg_v & CLUSTERPACTIVE_PORTION_BITS(i))  {
						global_arm_config_b_v = readl(GLOBAL_ARM_CONFIG_B);
						L3_P_DEBUG("%s %d: 0x%x\n", __func__, __LINE__, global_arm_config_b_v);
						cmd = ((i+1)<<4) | 0x8;
						global_arm_config_b_v &= ~CLUSTERPSTATE_MASK;
						global_arm_config_b_v |= cmd;
						L3_P_DEBUG("%s %d: 0x%x\n", __func__, __LINE__, global_arm_config_b_v);
						writel(global_arm_config_b_v, GLOBAL_ARM_CONFIG_B);
						global_arm_config_b_v |= CLUSTERPREQ;
						L3_P_DEBUG("%s %d: 0x%x\n", __func__, __LINE__, global_arm_config_b_v);
						writel(global_arm_config_b_v, GLOBAL_ARM_CONFIG_B);
						timeout = 3000;
						while(timeout){
							/* CLUSTERPDENY		BIT(21)
							   CLUSTERPACCEPT		BIT(20)
							 */
								reg_v = readl(GLOBAL_ARM_STATE);
								if(reg_v & CLUSTERPDENY){
									ret = -1;
									goto err_exit;
								}else if (reg_v & CLUSTERPACCEPT){
									global_arm_config_b_v &= ~CLUSTERPREQ;
						L3_P_DEBUG("%s %d: 0x%x\n", __func__, __LINE__, global_arm_config_b_v);
									writel(global_arm_config_b_v, GLOBAL_ARM_CONFIG_B);
									portion_active_val = portion_active_val | (1<<i);
						L3_P_DEBUG("%s %d: 0x%x, 0x%x\n", __func__, __LINE__, global_arm_config_b_v, portion_active_val);
									break;
								}
								timeout--;
						}
					}
				} else {
						/* nothing */
				}
			}
		}
	}
err_exit:
	printf("l3cache_partial_powerdown: Expected status(0x%x) , Finnal status(0x%x), \n", portion_mode, portion_active_val);
	return ret;
}
static int do_l3cache(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long portions;
	unsigned long portion_active;
	unsigned long portion_control;
	unsigned int val;
	/*
	 * Set the number of portions in the DSU to portions
	 *
	 * portions	Set of bit-fields to Enable
	 * ---------	---------------------------
	 *	4	PORTION_1|PORTION_2|PORTION_3|PORTION_4
	 *	3	PORTION_1|PORTION_2|PORTION_3
	 *	2	PORTION_1|PORTION_2
	 *	1	PORTION_1
	 *	0	<none>
	 */

	if (argc <2)
		return CMD_RET_USAGE;


	portions = simple_strtoul(argv[1], NULL, 10);
	if (portions > PORTION_MAX)
		return -EINVAL;

	L3_P_DEBUG("%s %d: %lu\n", __func__, __LINE__, portions);

	val = ca_smc_call(RTK_SVC_SREG_ACTLR_READ, 2, 0, 0, 0);
	if ((val & (1<<7)) == 0 ) {
		val |= (1<<7);
		ca_smc_call(RTK_SVC_SREG_ACTLR_SET, 0xc, val, 0, 0);
		val = ca_smc_call(RTK_SVC_SREG_ACTLR_READ, 2, 0, 0, 0);
	}
	L3_P_DEBUG("%s %d: %u\n", __func__, __LINE__, val);

	portion_active = ((1UL << portions) - 1) << PORTION_1;
	
	SYS_REG_READ(S3_0_c15_c3_5, portion_control);
	
	portion_control &= PORTION_MASK;
	portion_control |= portion_active;
	
	SYS_REG_WRITE(S3_0_c15_c3_5, portion_control);
	L3_P_DEBUG("%s %d: portion_control %lu\n", __func__, __LINE__, portion_control);
	L3_P_DEBUG("%s %d: l3cache_partial_powerdown 0x%lx\n", __func__, __LINE__, (1UL << portions) - 1);
	l3cache_partial_powerdown((1UL << portions) - 1);

	return 0;
}

U_BOOT_CMD(
	l3cache ,    2,    0,     do_l3cache,
	"L3 cache partial powerdown",
	"\n\t\t l3cache 0 : Full down\n"
	"\n\t\t l3cache 1 : 1/4 on\n"
	"\n\t\t l3cache 2 : 2/4 on\n"
	"\n\t\t l3cache 3 : 3/4 on\n"
	"\n\t\t l3cache 4 : Full on\n"
	"\t\t \n"
	"."
);

