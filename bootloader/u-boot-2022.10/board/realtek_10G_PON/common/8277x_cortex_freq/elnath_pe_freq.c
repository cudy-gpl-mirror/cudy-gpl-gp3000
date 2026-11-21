/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <linux/delay.h>
#define REF_40MHZ		40000000
#define REF_25MHZ		25000000
static int pe_clk_daemon(u32 target){

	/* keep FPLL and PEDIV untainted */
	GLOBAL_FPLL0_t fpll0;
	GLOBAL_PEDIV_t pediv;
	GLOBAL_FPLL1_t fpll1;

	GLOBAL_STRAP_t strap;
	u32 divn = 0;
	int xlat = REF_25MHZ;

	strap.wrd = readl(GLOBAL_STRAP);
	if( strap.bf.xtal_sel == 1 ) {
		xlat = REF_40MHZ;
	}

	printf("xlat = %u, target=%u\n", xlat, target);

	//1. GLOBAL_CPLLMUX = 0xff000000 (cf_i1guard=15, cf_i0guard=15)
	writel(0xff000000, GLOBAL_CPLLMUX);


	//switch pe clk to c2f_clk from CPLL(1.2G/2=600M)
	pediv.wrd = readl(GLOBAL_PEDIV);
	pediv.bf.offload_sel = 1;
	writel(pediv.wrd, GLOBAL_PEDIV);
	//2. wait 2ms
	udelay(2000);

	//3.power down FPLL:  FPLL1_OEB=0x3 -> FPLL0_SCPU_RSTB=0 -> FPLL0_SCPU_POW=0 (power down FPLL)
	fpll1.wrd = readl(GLOBAL_FPLL1);
	fpll1.bf.OEB=0x3;
	writel(fpll1.wrd, GLOBAL_FPLL1);

	fpll0.wrd = readl(GLOBAL_FPLL0);
	fpll0.bf.SCPU_RSTB=0;
	writel(fpll0.wrd, GLOBAL_FPLL0);

	fpll0.bf.SCPU_POW=0;
	writel(fpll0.wrd, GLOBAL_FPLL0);




	//4. FPLL0_FPLL_mode_override = 0x1 (1=override the fpll settings with the FPLL0 pll parameters)
	fpll0.wrd = readl(GLOBAL_FPLL0);
	fpll0.bf.fpll_mode_override=0x1;
	writel(fpll0.wrd, GLOBAL_FPLL0);

	//5. 	Set FPLL freq register(NOTE)
	/*
	(FPLL0_DIV_PREDIV_BPS / FPLL0_SCPU_DIV_DIVN / FPLL0_DIV_PREDIV_SEL)
	*/

	if(xlat == REF_25MHZ){
		divn = (2*target/REF_25MHZ) -3;
		fpll0.bf.DIV_PREDIV_BPS = 1;
		fpll0.bf.DIV_PREDIV_SEL = 0;
		fpll0.bf.SCPU_DIV_DIVN = divn;
	}else {
		divn = (2*target/REF_40MHZ/2) -3;
		fpll0.bf.DIV_PREDIV_BPS = 0;
		fpll0.bf.DIV_PREDIV_SEL = 0;
		fpll0.bf.SCPU_DIV_DIVN = divn;
	}
	printf("DIV_PREDIV_BPS 0x%x, DIV_PREDIV_SEL 0x%x, SCPU_DIV_DIVN 0x%x\n", fpll0.bf.DIV_PREDIV_BPS , fpll0.bf.DIV_PREDIV_SEL , fpll0.bf.SCPU_DIV_DIVN );
	writel(fpll0.wrd, GLOBAL_FPLL0);

	//7.power up FPLL :  FPLL0_SCPU_POW=1 -> FPLL0_SCPU_RSTB=1 (power up FPLL)
	fpll0.wrd = readl(GLOBAL_FPLL0);
	fpll0.bf.SCPU_POW=1;
	writel(fpll0.wrd, GLOBAL_FPLL0);

	fpll0.bf.SCPU_RSTB=1;
	writel(fpll0.wrd, GLOBAL_FPLL0);

	printf("GLOBAL_FPLL0 = 0x%08x\n", readl(GLOBAL_FPLL0));

	//8. wait 500us then FPLL1_OEB=0x0 (PLL stable and enable clk output)
	udelay(500);
	fpll1.bf.OEB=0x0;
	writel(fpll1.wrd, GLOBAL_FPLL1);
	//9. wait 10us
	udelay(10);

	//10. switch pe clk to f_clk_noscan from FPLL
	pediv.bf.offload_sel = 0;
	writel(pediv.wrd, GLOBAL_PEDIV);
	printf("GLOBAL_FPLL0(0x%08x) = 0x%08x\n", GLOBAL_FPLL0, readl(GLOBAL_FPLL0));
	printf("GLOBAL_FPLL1(0x%08x) = 0x%08x\n", GLOBAL_FPLL1, readl(GLOBAL_FPLL1));
	printf("GLOBAL_PEDIV(0x%08x) = 0x%08x\n", GLOBAL_PEDIV, readl(GLOBAL_PEDIV));


	return 0;
}



static int do_pe_clk(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32 t;
	if(argc<2)
		return cmd_usage(cmdtp);

	t = simple_strtoul(argv[1], NULL, 10);
	if ( t >= 400000000 ) {
		pe_clk_daemon(t);
	} else
		return cmd_usage(cmdtp);

	return 0;
}

U_BOOT_CMD(pe_clk, CONFIG_SYS_MAXARGS, 0, do_pe_clk,
		   "change PE clock",
		   "Target_clk\n"
		   "ex, pe_clk 600000000\n"
		   "Note: min clk 400000000\n"
		   );
