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

#if 0
extern  void rtk_pmu_setup_start(void);
extern void rtk_pmu_stop_show(void);
#else
static void rtk_pmu_setup_start(void) {};
static void rtk_pmu_stop_show(void) {};
#endif

#define START_MEM 0x10000000
int do_mem_burst_read(void){
	u32 addr = START_MEM;
	u32 size = 0x10000000;
	u32 range = addr + size;
printf("4: %s: 0x%x ~ 0x%x\n",__func__,  addr, range);
rtk_pmu_setup_start();
again:
	while(addr < (range)){

        asm volatile(
              "ldp  x8,  x9, [%0]  \n\t"
              "ldp  x10, x11, [%0, #16]  \n\t"
              "ldp  x12, x13, [%0, #32]  \n\t"
              "ldp  x13, x14, [%0, #48]  \n\t"
                :
                :"r" (addr)
                :
        );
		addr = addr + 64;
		//printf("%s %d: 0x%x ~ 0x%x\n",__func__,__LINE__,  addr, range);
	}
	
	addr = START_MEM;
	goto again;
	
rtk_pmu_stop_show();
	return 0;
}


int do_mem_burst_cp(void){
	u32 addr = START_MEM;
	u32 size = 0x04000000;
	u32 oaddr = addr + size;

	u32 range = addr + size;
printf("%s: 0x%x ~ 0x%x\n",__func__,  addr, range);
rtk_pmu_setup_start();
again:
	while(addr < range){
        asm volatile(
              "ldp  x8,  x9,  [%x[in1]]  \n\t"
			  "stp  x8,  x9,  [%x[in2]]   \n\t"
			  "ldp  x10,  x11,  [%x[in1], #16]  \n\t"
			  "stp  x10,  x11,  [%x[in2], #16]  \n\t"
			  "ldp  x12,  x13,  [%x[in1], #16]  \n\t"
			  "stp  x12,  x13,  [%x[in2], #16]  \n\t"
			  "ldp  x14,  x15,  [%x[in1], #16]  \n\t"
			  "stp  x14,  x15,  [%x[in2], #16]  \n\t"
                :
                : [in1] "r" (addr), [in2] "r" (oaddr)
                : "memory"
        );
		addr = addr + 64;
		oaddr = oaddr + 64;
	}
	addr = START_MEM;
	oaddr = addr + size;
	goto again;
rtk_pmu_stop_show();
printf("%s: end: 0x%x ~ 0x%x\n",__func__,  addr, oaddr);
	return range;
}


int do_mem_burst_st(void){
	u32 addr = START_MEM;
	u32 size = 0x04000000;
	u64 data1 =  0xfabcdef912345678;
	u64 data2 =  0x0280817822671299;
	u32 range = addr + size;
printf("%s: 0x%x ~ 0x%x\n",__func__,  addr, range);
rtk_pmu_setup_start();
again:
	while(addr < range){
        asm volatile(
                "stp  %x[in1], %x[in2], [%2]  \n\t"
                "stp  %x[in1], %x[in2],  [%2, #16]  \n\t"
                "stp  %x[in1], %x[in2],  [%2, #32]  \n\t"
                "stp  %x[in1], %x[in2],  [%2, #48]  \n\t"
		        "stp  %x[in1], %x[in2],  [%2, #48]  \n\t"
				"stp  %x[in1], %x[in2],  [%2, #64]  \n\t"
				"stp  %x[in1], %x[in2],  [%2, #80]  \n\t"
				"stp  %x[in1], %x[in2],  [%2, #96]  \n\t"
				"stp  %x[in1], %x[in2],  [%2, #112]  \n\t"
                :
                : [in1] "r" (data1), [in2] "r" (data2), "r" (addr)
                :
        );
		addr = addr + 128;
	}
	addr = START_MEM;
	goto again;
rtk_pmu_stop_show();
printf("%s: end: 0x%x \n",__func__,  addr);
	return range;
}




static int do_burst_init(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32 t=0;

	if (argc > 2)
		return CMD_RET_USAGE;
	if (argc == 2){
		t = simple_strtoul(argv[1], NULL, 10);
	}
	switch (t){
		case 0:
			do_mem_burst_read();
			break;
		case 1:
			do_mem_burst_cp();
			break;
		case 2:
			do_mem_burst_st();
			break;
		default:
			do_mem_burst_read();
			break;
	}
	return 0;
}

U_BOOT_CMD(
	burst ,    2,    1,     do_burst_init,
	"",
	"N\n"
	" "
);

