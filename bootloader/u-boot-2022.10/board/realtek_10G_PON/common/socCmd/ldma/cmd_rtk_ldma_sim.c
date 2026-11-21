/*
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Misc functions
 */
#include <common.h>
#include <command.h>
#include <cpu_func.h>
#include <asm/io.h>
#include <linux/delay.h>

static int do_sim_ldma(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	writel( 0x0083c080,  0x90301054    );
    writel( 0x7AFF, 0x903011a0  );
	writel(0x80000088,  0x90301000);

	writel (0x10000000,0x90301210) ;
	writel (0x0,0x90301210) ;

	memset((void *)0x00100000, 0x91, 0x1000);

	writel (0x80000000, 0x90301210);

	writel (0x00200000, 0x90301208 );
	writel (0x06060fff, 0x90301204 );
	writel (0x00100000, 0x9030120c );

	while(readl(0x90301220) & 0xF000000){
		udelay(10);
	}

	if(memcmp((void *) 0x00100000,(void *) 0x00200000 , 0x1000))
			printf("Compare 0x00100000 , 0x00200000, size(0x1000) failed  ");


	return 0;
}

U_BOOT_CMD(
	ldma_sim_test ,    5,    0,     do_sim_ldma,
	"ldma: ldma engine",
	"\n\t\t <case>\n"
	"\t\t <qid|irq_mux|dsize> <para>\n"
	"    - ldma test code."
);

