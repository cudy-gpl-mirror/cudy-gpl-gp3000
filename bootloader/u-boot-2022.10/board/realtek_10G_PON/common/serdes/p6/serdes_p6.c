#include <common.h>
#include <linux/delay.h>
#include <command.h>
#include <malloc.h>
#include <net.h>
#include <miiphy.h>
//#include <environment.h>
#include <env.h>
#include <asm/types.h>
#include <asm/io.h>
#include <config.h>
#include "serdes_p6.h"

#if defined(CONFIG_TARGET_RTK_TAURUS) || defined(CONFIG_TARGET_RTK_MERCURY)
rtk_sds_prof_t qxgmii_p6_prof[] = {
	{0xf550d050, 0xc000079f},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000079e},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000079d},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000079c},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000079b},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000079a},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000799},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000798},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000797},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000796},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000795},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000794},
	{0xf550d04c, 0x00000007},
	{0xf550d050, 0xc0000793},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000792},
	{0xf550d04c, 0x0000fe00},
	{0xf550d050, 0xc0000791},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000790},
	{0xf550d04c, 0x000001c7},
	{0xf550d050, 0xc000078f},
	{0xf550d04c, 0x0000490b},
	{0xf550d050, 0xc000078e},
	{0xf550d04c, 0x00000012},
	{0xf550d050, 0xc000078d},
	{0xf550d04c, 0x00008007},
	{0xf550d050, 0xc000078c},
	{0xf550d04c, 0x00000294},
	{0xf550d050, 0xc000078b},
	{0xf550d04c, 0x00000294},
	{0xf550d050, 0xc000078a},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000789},
	{0xf550d04c, 0x00004000},
	{0xf550d050, 0xc0000788},
	{0xf550d04c, 0x00001800},
	{0xf550d050, 0xc0000787},
	{0xf550d04c, 0x00000035},
	{0xf550d050, 0xc0000786},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000785},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000784},
	{0xf550d04c, 0x00004000},
	{0xf550d050, 0xc0000783},
	{0xf550d04c, 0x00000233},
	{0xf550d050, 0xc0000782},
	{0xf550d04c, 0x000003ff},
	{0xf550d050, 0xc0000781},
	{0xf550d04c, 0x000011e0},
	{0xf550d050, 0xc0000780},
	{0xf550d04c, 0x00000018},
	{0xf550d050, 0xc00005ff},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005fe},
	{0xf550d04c, 0x00008000},
	{0xf550d050, 0xc00005fd},
	{0xf550d04c, 0x0000001e},
	{0xf550d050, 0xc00005fc},
	{0xf550d04c, 0x00003fdf},
	{0xf550d050, 0xc00005fb},
	{0xf550d04c, 0x00000fe8},
	{0xf550d050, 0xc00005fa},
	{0xf550d04c, 0x000007ff},
	{0xf550d050, 0xc00005f9},
	{0xf550d04c, 0x00000500},
	{0xf550d050, 0xc00005f8},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005f7},
	{0xf550d04c, 0x00003e00},
	{0xf550d050, 0xc00005f6},
	{0xf550d04c, 0x0000ffff},
	{0xf550d050, 0xc00005f5},
	{0xf550d04c, 0x0000ffff},
	{0xf550d050, 0xc00005f4},
	{0xf550d04c, 0x0000f0ff},
	{0xf550d050, 0xc00005f3},
	{0xf550d04c, 0x00000020},
	{0xf550d050, 0xc00005f2},
	{0xf550d04c, 0x000086ff},
	{0xf550d050, 0xc00005f1},
	{0xf550d04c, 0x000003fc},
	{0xf550d050, 0xc00005f0},
	{0xf550d04c, 0x00000004},
	{0xf550d050, 0xc00005ef},
	{0xf550d04c, 0x0000180c},
	{0xf550d050, 0xc00005ee},
	{0xf550d04c, 0x00004000},
	{0xf550d050, 0xc00005ed},
	{0xf550d04c, 0x00008000},
	{0xf550d050, 0xc00005ec},
	{0xf550d04c, 0x00000080},
	{0xf550d050, 0xc00005eb},
	{0xf550d04c, 0x0000d800},
	{0xf550d050, 0xc00005ea},
	{0xf550d04c, 0x0000ff27},
	{0xf550d050, 0xc00005e9},
	{0xf550d04c, 0x00005840},
	{0xf550d050, 0xc00005e8},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005e7},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005e6},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005e5},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005e4},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005e3},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005e2},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005e1},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005e0},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005df},
	{0xf550d04c, 0x00004168},
	{0xf550d050, 0xc00005de},
	{0xf550d04c, 0x0000a128},
	{0xf550d050, 0xc00005dd},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00005dc},
	{0xf550d04c, 0x0000f0f0},
	{0xf550d050, 0xc00005db},
	{0xf550d04c, 0x00000100},
	{0xf550d050, 0xc00005da},
	{0xf550d04c, 0x00005ae8},
	{0xf550d050, 0xc00005d9},
	{0xf550d04c, 0x0000f0f0},
	{0xf550d050, 0xc00005d8},
	{0xf550d04c, 0x0000f0f2},
	{0xf550d050, 0xc00005d7},
	{0xf550d04c, 0x0000f0f0},
	{0xf550d050, 0xc00005d6},
	{0xf550d04c, 0x00001597},
	{0xf550d050, 0xc00005d5},
	{0xf550d04c, 0x0000fb69},
	{0xf550d050, 0xc00005d4},
	{0xf550d04c, 0x0000200a},
	{0xf550d050, 0xc00005d3},
	{0xf550d04c, 0x00001e22},
	{0xf550d050, 0xc00005d2},
	{0xf550d04c, 0x0000f060},
	{0xf550d050, 0xc00005d1},
	{0xf550d04c, 0x0000f0ae},
	{0xf550d050, 0xc00005d0},
	{0xf550d04c, 0x000089b3},
	{0xf550d050, 0xc00005cf},
	{0xf550d04c, 0x00004c58},
	{0xf550d050, 0xc00005ce},
	{0xf550d04c, 0x00000056},
	{0xf550d050, 0xc00005cd},
	{0xf550d04c, 0x00005f04},
	{0xf550d050, 0xc00005cc},
	{0xf550d04c, 0x00004a12},
	{0xf550d050, 0xc00005cb},
	{0xf550d04c, 0x00002424},
	{0xf550d050, 0xc00005ca},
	{0xf550d04c, 0x00004000},
	{0xf550d050, 0xc00005c9},
	{0xf550d04c, 0x00002504},
	{0xf550d050, 0xc00005c8},
	{0xf550d04c, 0x0000d125},
	{0xf550d050, 0xc00005c7},
	{0xf550d04c, 0x00003224},
	{0xf550d050, 0xc00005c6},
	{0xf550d04c, 0x00001dc8},
	{0xf550d050, 0xc00005c5},
	{0xf550d04c, 0x0000878c},
	{0xf550d050, 0xc00005c4},
	{0xf550d04c, 0x00001e40},
	{0xf550d050, 0xc00005c3},
	{0xf550d04c, 0x0000f0f9},
	{0xf550d050, 0xc00005c2},
	{0xf550d04c, 0x0000f0f0},
	{0xf550d050, 0xc00005c1},
	{0xf550d04c, 0x00003c30},
	{0xf550d050, 0xc00005c0},
	{0xf550d04c, 0x00009a45},
	{0xf550d050, 0xc00006df},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006de},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006dd},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006dc},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006db},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006da},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006d9},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006d8},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006d7},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006d6},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006d5},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006d4},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006d3},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006d2},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006d1},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006d0},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006cf},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006ce},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006cd},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006cc},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006cb},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006ca},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006c9},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006c8},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006c7},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006c6},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006c5},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006c4},
	{0xf550d04c, 0x0000ff00},
	{0xf550d050, 0xc00006c3},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc00006c2},
	{0xf550d04c, 0x0000ff01},
	{0xf550d050, 0xc00006c1},
	{0xf550d04c, 0x0000ff00},
	{0xf550d050, 0xc00006c0},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000043f},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000043e},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000043d},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000043c},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000043b},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000043a},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000439},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000438},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000437},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000436},
	{0xf550d04c, 0x00004000},
	{0xf550d050, 0xc0000435},
	{0xf550d04c, 0x00001000},
	{0xf550d050, 0xc0000434},
	{0xf550d04c, 0x00000401},
	{0xf550d050, 0xc0000433},
	{0xf550d04c, 0x0000e005},
	{0xf550d050, 0xc0000432},
	{0xf550d04c, 0x0000b87e},
	{0xf550d050, 0xc0000431},
	{0xf550d04c, 0x0000ffd0},
	{0xf550d050, 0xc0000430},
	{0xf550d04c, 0x00001800},
	{0xf550d050, 0xc000042f},
	{0xf550d04c, 0x0000f000},
	{0xf550d050, 0xc000042e},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000042d},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc000042c},
	{0xf550d04c, 0x0000ffff},
	{0xf550d050, 0xc000042b},
	{0xf550d04c, 0x00000180},
	{0xf550d050, 0xc000042a},
	{0xf550d04c, 0x00000304},
	{0xf550d050, 0xc0000429},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000428},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000427},
	{0xf550d04c, 0x0000f0f0},
	{0xf550d050, 0xc0000426},
	{0xf550d04c, 0x0000f0f0},
	{0xf550d050, 0xc0000425},
	{0xf550d04c, 0x0000f0f0},
	{0xf550d050, 0xc0000424},
	{0xf550d04c, 0x0000f0f0},
	{0xf550d050, 0xc0000423},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000422},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000421},
	{0xf550d04c, 0x00000000},
	{0xf550d050, 0xc0000420},
	{0xf550d04c, 0x00004802}
};

rtk_sds_prof_t usxgmii_p6_prof[] = {
	{0xf550d050, 0x00004802},
	{0xf550d04c, 0xc0000420},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000421},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000422},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000423},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc0000424},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc0000425},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc0000426},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc0000427},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000428},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000429},
	{0xf550d050, 0x00000304},
	{0xf550d04c, 0xc000042a},
	{0xf550d050, 0x00000180},
	{0xf550d04c, 0xc000042b},
	{0xf550d050, 0x0000ffff},
	{0xf550d04c, 0xc000042c},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000042d},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000042e},
	{0xf550d050, 0x0000f000},
	{0xf550d04c, 0xc000042f},
	{0xf550d050, 0x00001800},
	{0xf550d04c, 0xc0000430},
	{0xf550d050, 0x0000ffd0},
	{0xf550d04c, 0xc0000431},
	{0xf550d050, 0x0000b87e},
	{0xf550d04c, 0xc0000432},
	{0xf550d050, 0x0000e005},
	{0xf550d04c, 0xc0000433},
	{0xf550d050, 0x00000401},
	{0xf550d04c, 0xc0000434},
	{0xf550d050, 0x00001000},
	{0xf550d04c, 0xc0000435},
	{0xf550d050, 0x00004000},
	{0xf550d04c, 0xc0000436},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000437},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000438},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000439},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043a},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043b},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043c},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043d},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043e},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043f},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c0},
	{0xf550d050, 0x0000ff00},
	{0xf550d04c, 0xc00006c1},
	{0xf550d050, 0x0000ff01},
	{0xf550d04c, 0xc00006c2},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c3},
	{0xf550d050, 0x0000ff00},
	{0xf550d04c, 0xc00006c4},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c5},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c6},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c7},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c8},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c9},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006ca},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006cb},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006cc},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006cd},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006ce},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006cf},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d0},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d1},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d2},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d3},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d4},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d5},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d6},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d7},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d8},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d9},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006da},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006db},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006dc},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006dd},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006de},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006df},
	{0xf550d050, 0x00009a45},
	{0xf550d04c, 0xc00005c0},
	{0xf550d050, 0x00003c30},
	{0xf550d04c, 0xc00005c1},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc00005c2},
	{0xf550d050, 0x0000f0f9},
	{0xf550d04c, 0xc00005c3},
	{0xf550d050, 0x00001e40},
	{0xf550d04c, 0xc00005c4},
	{0xf550d050, 0x0000878c},
	{0xf550d04c, 0xc00005c5},
	{0xf550d050, 0x00001dc8},
	{0xf550d04c, 0xc00005c6},
	{0xf550d050, 0x00003224},
	{0xf550d04c, 0xc00005c7},
	{0xf550d050, 0x0000d125},
	{0xf550d04c, 0xc00005c8},
	{0xf550d050, 0x00002504},
	{0xf550d04c, 0xc00005c9},
	{0xf550d050, 0x00004000},
	{0xf550d04c, 0xc00005ca},
	{0xf550d050, 0x00002424},
	{0xf550d04c, 0xc00005cb},
	{0xf550d050, 0x00004a12},
	{0xf550d04c, 0xc00005cc},
	{0xf550d050, 0x00005f04},
	{0xf550d04c, 0xc00005cd},
	{0xf550d050, 0x00000056},
	{0xf550d04c, 0xc00005ce},
	{0xf550d050, 0x00004c58},
	{0xf550d04c, 0xc00005cf},
	{0xf550d050, 0x000089b3},
	{0xf550d04c, 0xc00005d0},
	{0xf550d050, 0x0000f0ae},
	{0xf550d04c, 0xc00005d1},
	{0xf550d050, 0x0000f060},
	{0xf550d04c, 0xc00005d2},
	{0xf550d050, 0x00001e22},
	{0xf550d04c, 0xc00005d3},
	{0xf550d050, 0x0000200a},
	{0xf550d04c, 0xc00005d4},
	{0xf550d050, 0x0000fb69},
	{0xf550d04c, 0xc00005d5},
	{0xf550d050, 0x00001597},
	{0xf550d04c, 0xc00005d6},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc00005d7},
	{0xf550d050, 0x0000f0f2},
	{0xf550d04c, 0xc00005d8},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc00005d9},
	{0xf550d050, 0x00005ae8},
	{0xf550d04c, 0xc00005da},
	{0xf550d050, 0x00000100},
	{0xf550d04c, 0xc00005db},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc00005dc},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005dd},
	{0xf550d050, 0x0000a128},
	{0xf550d04c, 0xc00005de},
	{0xf550d050, 0x00004168},
	{0xf550d04c, 0xc00005df},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e0},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e1},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e2},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e3},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e4},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e5},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e6},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e7},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e8},
	{0xf550d050, 0x00005840},
	{0xf550d04c, 0xc00005e9},
	{0xf550d050, 0x0000ff27},
	{0xf550d04c, 0xc00005ea},
	{0xf550d050, 0x0000d800},
	{0xf550d04c, 0xc00005eb},
	{0xf550d050, 0x00000080},
	{0xf550d04c, 0xc00005ec},
	{0xf550d050, 0x00008000},
	{0xf550d04c, 0xc00005ed},
	{0xf550d050, 0x00004000},
	{0xf550d04c, 0xc00005ee},
	{0xf550d050, 0x0000180c},
	{0xf550d04c, 0xc00005ef},
	{0xf550d050, 0x00000004},
	{0xf550d04c, 0xc00005f0},
	{0xf550d050, 0x000003fc},
	{0xf550d04c, 0xc00005f1},
	{0xf550d050, 0x000086ff},
	{0xf550d04c, 0xc00005f2},
	{0xf550d050, 0x00000020},
	{0xf550d04c, 0xc00005f3},
	{0xf550d050, 0x0000f0ff},
	{0xf550d04c, 0xc00005f4},
	{0xf550d050, 0x0000ffff},
	{0xf550d04c, 0xc00005f5},
	{0xf550d050, 0x0000ffff},
	{0xf550d04c, 0xc00005f6},
	{0xf550d050, 0x00003e00},
	{0xf550d04c, 0xc00005f7},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005f8},
	{0xf550d050, 0x00000500},
	{0xf550d04c, 0xc00005f9},
	{0xf550d050, 0x000007ff},
	{0xf550d04c, 0xc00005fa},
	{0xf550d050, 0x00000fe8},
	{0xf550d04c, 0xc00005fb},
	{0xf550d050, 0x00003fdf},
	{0xf550d04c, 0xc00005fc},
	{0xf550d050, 0x0000001e},
	{0xf550d04c, 0xc00005fd},
	{0xf550d050, 0x00008000},
	{0xf550d04c, 0xc00005fe},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005ff},
	{0xf550d050, 0x00000018},
	{0xf550d04c, 0xc0000780},
	{0xf550d050, 0x000011e0},
	{0xf550d04c, 0xc0000781},
	{0xf550d050, 0x000003ff},
	{0xf550d04c, 0xc0000782},
	{0xf550d050, 0x00000233},
	{0xf550d04c, 0xc0000783},
	{0xf550d050, 0x00004000},
	{0xf550d04c, 0xc0000784},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000785},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000786},
	{0xf550d050, 0x00000035},
	{0xf550d04c, 0xc0000787},
	{0xf550d050, 0x00001800},
	{0xf550d04c, 0xc0000788},
	{0xf550d050, 0x00004000},
	{0xf550d04c, 0xc0000789},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000078a},
	{0xf550d050, 0x00000294},
	{0xf550d04c, 0xc000078b},
	{0xf550d050, 0x00000294},
	{0xf550d04c, 0xc000078c},
	{0xf550d050, 0x00008007},
	{0xf550d04c, 0xc000078d},
	{0xf550d050, 0x00000012},
	{0xf550d04c, 0xc000078e},
	{0xf550d050, 0x0000490b},
	{0xf550d04c, 0xc000078f},
	{0xf550d050, 0x000001c7},
	{0xf550d04c, 0xc0000790},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000791},
	{0xf550d050, 0x0000fe00},
	{0xf550d04c, 0xc0000792},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000793},
	{0xf550d050, 0x00000007},
	{0xf550d04c, 0xc0000794},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000795},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000796},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000797},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000798},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000799},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079a},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079b},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079c},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079d},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079e},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079f}
};

rtk_sds_prof_t xfi_p6_prof[] = {
	{0xf550d050, 0x00004802},
	{0xf550d04c, 0xc0000420},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000421},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000422},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000423},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc0000424},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc0000425},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc0000426},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc0000427},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000428},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000429},
	{0xf550d050, 0x00000304},
	{0xf550d04c, 0xc000042a},
	{0xf550d050, 0x00000180},
	{0xf550d04c, 0xc000042b},
	{0xf550d050, 0x0000ffff},
	{0xf550d04c, 0xc000042c},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000042d},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000042e},
	{0xf550d050, 0x0000f000},
	{0xf550d04c, 0xc000042f},
	{0xf550d050, 0x00001800},
	{0xf550d04c, 0xc0000430},
	{0xf550d050, 0x0000ffd0},
	{0xf550d04c, 0xc0000431},
	{0xf550d050, 0x0000b87e},
	{0xf550d04c, 0xc0000432},
	{0xf550d050, 0x0000e005},
	{0xf550d04c, 0xc0000433},
	{0xf550d050, 0x00000401},
	{0xf550d04c, 0xc0000434},
	{0xf550d050, 0x00001000},
	{0xf550d04c, 0xc0000435},
	{0xf550d050, 0x00004000},
	{0xf550d04c, 0xc0000436},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000437},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000438},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000439},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043a},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043b},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043c},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043d},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043e},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000043f},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c0},
	{0xf550d050, 0x0000ff00},
	{0xf550d04c, 0xc00006c1},
	{0xf550d050, 0x0000ff01},
	{0xf550d04c, 0xc00006c2},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c3},
	{0xf550d050, 0x0000ff00},
	{0xf550d04c, 0xc00006c4},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c5},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c6},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c7},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c8},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006c9},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006ca},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006cb},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006cc},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006cd},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006ce},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006cf},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d0},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d1},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d2},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d3},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d4},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d5},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d6},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d7},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d8},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006d9},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006da},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006db},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006dc},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006dd},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006de},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00006df},
	{0xf550d050, 0x00009a45},
	{0xf550d04c, 0xc00005c0},
	{0xf550d050, 0x00003c30},
	{0xf550d04c, 0xc00005c1},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc00005c2},
	{0xf550d050, 0x0000f0f9},
	{0xf550d04c, 0xc00005c3},
	{0xf550d050, 0x00001e40},
	{0xf550d04c, 0xc00005c4},
	{0xf550d050, 0x0000878c},
	{0xf550d04c, 0xc00005c5},
	{0xf550d050, 0x00001dc8},
	{0xf550d04c, 0xc00005c6},
	{0xf550d050, 0x00003224},
	{0xf550d04c, 0xc00005c7},
	{0xf550d050, 0x0000d125},
	{0xf550d04c, 0xc00005c8},
	{0xf550d050, 0x00002504},
	{0xf550d04c, 0xc00005c9},
	{0xf550d050, 0x00004000},
	{0xf550d04c, 0xc00005ca},
	{0xf550d050, 0x00002424},
	{0xf550d04c, 0xc00005cb},
	{0xf550d050, 0x00004a12},
	{0xf550d04c, 0xc00005cc},
	{0xf550d050, 0x00005f04},
	{0xf550d04c, 0xc00005cd},
	{0xf550d050, 0x00000056},
	{0xf550d04c, 0xc00005ce},
	{0xf550d050, 0x00004c58},
	{0xf550d04c, 0xc00005cf},
	{0xf550d050, 0x000089b3},
	{0xf550d04c, 0xc00005d0},
	{0xf550d050, 0x0000f0ae},
	{0xf550d04c, 0xc00005d1},
	{0xf550d050, 0x0000f060},
	{0xf550d04c, 0xc00005d2},
	{0xf550d050, 0x00001e22},
	{0xf550d04c, 0xc00005d3},
	{0xf550d050, 0x0000200a},
	{0xf550d04c, 0xc00005d4},
	{0xf550d050, 0x0000fb69},
	{0xf550d04c, 0xc00005d5},
	{0xf550d050, 0x00001597},
	{0xf550d04c, 0xc00005d6},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc00005d7},
	{0xf550d050, 0x0000f0f2},
	{0xf550d04c, 0xc00005d8},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc00005d9},
	{0xf550d050, 0x00005ae8},
	{0xf550d04c, 0xc00005da},
	{0xf550d050, 0x00000100},
	{0xf550d04c, 0xc00005db},
	{0xf550d050, 0x0000f0f0},
	{0xf550d04c, 0xc00005dc},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005dd},
	{0xf550d050, 0x0000a128},
	{0xf550d04c, 0xc00005de},
	{0xf550d050, 0x00004168},
	{0xf550d04c, 0xc00005df},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e0},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e1},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e2},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e3},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e4},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e5},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e6},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e7},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005e8},
	{0xf550d050, 0x00005840},
	{0xf550d04c, 0xc00005e9},
	{0xf550d050, 0x0000ff27},
	{0xf550d04c, 0xc00005ea},
	{0xf550d050, 0x0000d800},
	{0xf550d04c, 0xc00005eb},
	{0xf550d050, 0x00000080},
	{0xf550d04c, 0xc00005ec},
	{0xf550d050, 0x00008000},
	{0xf550d04c, 0xc00005ed},
	{0xf550d050, 0x00004000},
	{0xf550d04c, 0xc00005ee},
	{0xf550d050, 0x0000180c},
	{0xf550d04c, 0xc00005ef},
	{0xf550d050, 0x00000004},
	{0xf550d04c, 0xc00005f0},
	{0xf550d050, 0x000003fc},
	{0xf550d04c, 0xc00005f1},
	{0xf550d050, 0x000086ff},
	{0xf550d04c, 0xc00005f2},
	{0xf550d050, 0x00000020},
	{0xf550d04c, 0xc00005f3},
	{0xf550d050, 0x0000f0ff},
	{0xf550d04c, 0xc00005f4},
	{0xf550d050, 0x0000ffff},
	{0xf550d04c, 0xc00005f5},
	{0xf550d050, 0x0000ffff},
	{0xf550d04c, 0xc00005f6},
	{0xf550d050, 0x00003e00},
	{0xf550d04c, 0xc00005f7},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005f8},
	{0xf550d050, 0x00000500},
	{0xf550d04c, 0xc00005f9},
	{0xf550d050, 0x000007ff},
	{0xf550d04c, 0xc00005fa},
	{0xf550d050, 0x00000fe8},
	{0xf550d04c, 0xc00005fb},
	{0xf550d050, 0x00003fdf},
	{0xf550d04c, 0xc00005fc},
	{0xf550d050, 0x0000001e},
	{0xf550d04c, 0xc00005fd},
	{0xf550d050, 0x00008000},
	{0xf550d04c, 0xc00005fe},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc00005ff},
	{0xf550d050, 0x00000018},
	{0xf550d04c, 0xc0000780},
	{0xf550d050, 0x000011e0},
	{0xf550d04c, 0xc0000781},
	{0xf550d050, 0x000003ff},
	{0xf550d04c, 0xc0000782},
	{0xf550d050, 0x00000233},
	{0xf550d04c, 0xc0000783},
	{0xf550d050, 0x00004000},
	{0xf550d04c, 0xc0000784},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000785},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000786},
	{0xf550d050, 0x00000035},
	{0xf550d04c, 0xc0000787},
	{0xf550d050, 0x00001800},
	{0xf550d04c, 0xc0000788},
	{0xf550d050, 0x00004000},
	{0xf550d04c, 0xc0000789},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000078a},
	{0xf550d050, 0x00000294},
	{0xf550d04c, 0xc000078b},
	{0xf550d050, 0x00000294},
	{0xf550d04c, 0xc000078c},
	{0xf550d050, 0x00008007},
	{0xf550d04c, 0xc000078d},
	{0xf550d050, 0x00000012},
	{0xf550d04c, 0xc000078e},
	{0xf550d050, 0x0000490b},
	{0xf550d04c, 0xc000078f},
	{0xf550d050, 0x000001c7},
	{0xf550d04c, 0xc0000790},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000791},
	{0xf550d050, 0x0000fe00},
	{0xf550d04c, 0xc0000792},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000793},
	{0xf550d050, 0x00000007},
	{0xf550d04c, 0xc0000794},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000795},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000796},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000797},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000798},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc0000799},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079a},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079b},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079c},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079d},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079e},
	{0xf550d050, 0x00000000},
	{0xf550d04c, 0xc000079f}
};

bool __p6_checkSdsClkReady(void)
{
	QX_SDS_STATE_t qx_sds_state;
    int count = 1500;

    do {
		qx_sds_state.wrd = SERDES_REG_READ(QX_SDS_STATE);
        if(qx_sds_state.bf.CKRDY)
            return 1;
        mdelay(10);
        count --;
    } while(count >= 0);

    return 0;
}

static void __p6_mac_10g_config(void)
{
    NI_HV_XGE_MISC_CONFIG_t xge_misc_cfg;
    NI_HV_XGE_MAC_CFG_COM_t xge_mac_cfg;
    NI_HV_XGE_MAC_CFG_TX_t xge_tx_cfg;
    NI_HV_XGE_MAC_CFG_TX_2_t xge_tx2_cfg;
    NI_HV_XGE_MAC_CFG_RX_t xge_rx_cfg;
    L2TM_L2TM_ES_CTRL_t tm_es_ctrl;
    QM_QM_ES_CTRL_t qm_es_ctrl;
    NI_HV_GLB_NITX_MISC_CFG_t nitx_misc_cfg;

#ifdef CONFIG_RTK_NE
    /* Stop traffic */
    tm_es_ctrl.wrd = SERDES_REG_READ(L2TM_L2TM_ES_CTRL);
    tm_es_ctrl.bf.tx_port6_en = 0;
    SERDES_REG_WRITE(tm_es_ctrl.wrd, L2TM_L2TM_ES_CTRL);
    qm_es_ctrl.wrd = SERDES_REG_READ(QM_QM_ES_CTRL);
    qm_es_ctrl.bf.ni_en &= ~(1<<6);
    SERDES_REG_WRITE(qm_es_ctrl.wrd, QM_QM_ES_CTRL);
#endif

    /* Drain out packets */
    nitx_misc_cfg.wrd = SERDES_REG_READ(NI_HV_GLB_NITX_MISC_CFG);
    nitx_misc_cfg.bf.xge6_tx_drain = 1;
    SERDES_REG_WRITE(nitx_misc_cfg.wrd, NI_HV_GLB_NITX_MISC_CFG);

    /* interface select to init state */
    xge_misc_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MISC_CONFIG);
    xge_misc_cfg.bf.gige_mode = 0;
    xge_misc_cfg.bf.inf_sel = 0x0;
    SERDES_REG_WRITE(xge_misc_cfg.wrd, NI_HV_XGE_MISC_CONFIG);
    xge_misc_cfg.bf.gige_mode = 0;
    xge_misc_cfg.bf.inf_sel = 0x3;
    SERDES_REG_WRITE(xge_misc_cfg.wrd, NI_HV_XGE_MISC_CONFIG);

    mdelay(10);

    /* Tx IPG */
    xge_tx_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MAC_CFG_TX);
    xge_tx_cfg.bf.IFG_mode_tx = 0x1;
    SERDES_REG_WRITE(xge_tx_cfg.wrd, NI_HV_XGE_MAC_CFG_TX);
    /* Read thr */
    xge_tx2_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MAC_CFG_TX_2);
    xge_tx2_cfg.bf.fifo1_read_thr = 0xc;
    xge_tx2_cfg.bf.fifo1_thr_afull = 0xe;
    SERDES_REG_WRITE(xge_tx2_cfg.wrd, NI_HV_XGE_MAC_CFG_TX_2);
    #if 0
    /* Rx */
    xge_rx_cfg.wrd = CA_NI_REG_READ(NI_HV_XGE_MAC_CFG_RX);
    xge_rx_cfg.wrd = 0xbc1;
    CA_NI_REG_WRITE(xge_rx_cfg.wrd, NI_HV_XGE_MAC_CFG_RX);
    #endif
    /* Set MAC Tx/Rx resets */
    xge_mac_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MAC_CFG_COM);
    xge_mac_cfg.bf.tx_rst = 0x1;
    xge_mac_cfg.bf.rx_rst = 0x1;
    xge_mac_cfg.bf.cntr_rst = 0x0;
    xge_mac_cfg.bf.stats_err_order = 0x0;
    SERDES_REG_WRITE(xge_mac_cfg.wrd, NI_HV_XGE_MAC_CFG_COM);
    mdelay(10);
    /* Release MAC resets */
    xge_mac_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MAC_CFG_COM);
    xge_mac_cfg.bf.tx_rst = 0x0;
    xge_mac_cfg.bf.rx_rst = 0x0;
    xge_mac_cfg.bf.cntr_rst = 0x0;
    xge_mac_cfg.bf.stats_err_order = 0x0;
    SERDES_REG_WRITE(xge_mac_cfg.wrd, NI_HV_XGE_MAC_CFG_COM);

    /* interface select to 10G */
    xge_misc_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MISC_CONFIG);
    xge_misc_cfg.bf.gige_mode = 0;
    xge_misc_cfg.bf.inf_sel = 0x0;
    SERDES_REG_WRITE(xge_misc_cfg.wrd, NI_HV_XGE_MISC_CONFIG);
    xge_misc_cfg.bf.gige_mode = 0;
    xge_misc_cfg.bf.inf_sel = 0x2;
    SERDES_REG_WRITE(xge_misc_cfg.wrd, NI_HV_XGE_MISC_CONFIG);

    /* Stop drain out */
    nitx_misc_cfg.wrd = SERDES_REG_READ(NI_HV_GLB_NITX_MISC_CFG);
    nitx_misc_cfg.bf.xge6_tx_drain = 0;
    SERDES_REG_WRITE(nitx_misc_cfg.wrd, NI_HV_GLB_NITX_MISC_CFG);

#ifdef CONFIG_RTK_NE
    /* Resume traffic */
    tm_es_ctrl.wrd = SERDES_REG_READ(L2TM_L2TM_ES_CTRL);
    tm_es_ctrl.bf.tx_port6_en = 1;
    SERDES_REG_WRITE(tm_es_ctrl.wrd, L2TM_L2TM_ES_CTRL);
    qm_es_ctrl.wrd = SERDES_REG_READ(QM_QM_ES_CTRL);
    qm_es_ctrl.bf.ni_en |= (1<<6);
    SERDES_REG_WRITE(qm_es_ctrl.wrd, QM_QM_ES_CTRL);
#endif

    return;
}

static void __p6_mac_1g_config(void)
{
    NI_HV_XGE_MISC_CONFIG_t xge_misc_cfg;
    NI_HV_XGE_MAC_CFG_COM_t xge_mac_cfg;
    NI_HV_XGE_MAC_CFG_TX_t xge_tx_cfg;
    NI_HV_XGE_MAC_CFG_TX_2_t xge_tx2_cfg;
    NI_HV_XGE_MAC_CFG_RX_t xge_rx_cfg;
    L2TM_L2TM_ES_CTRL_t tm_es_ctrl;
    QM_QM_ES_CTRL_t qm_es_ctrl;
    NI_HV_GLB_NITX_MISC_CFG_t nitx_misc_cfg;

#ifdef CONFIG_RTK_NE
	/* Stop traffic */
	tm_es_ctrl.wrd = SERDES_REG_READ(L2TM_L2TM_ES_CTRL);
	tm_es_ctrl.bf.tx_port6_en = 0;
	SERDES_REG_WRITE(tm_es_ctrl.wrd, L2TM_L2TM_ES_CTRL);
	qm_es_ctrl.wrd = SERDES_REG_READ(QM_QM_ES_CTRL);
	qm_es_ctrl.bf.ni_en &= ~(1<<6);
	SERDES_REG_WRITE(qm_es_ctrl.wrd, QM_QM_ES_CTRL);
#endif

	/* Drain out packets */
    nitx_misc_cfg.wrd = SERDES_REG_READ(NI_HV_GLB_NITX_MISC_CFG);
    nitx_misc_cfg.bf.xge6_tx_drain = 1;
    SERDES_REG_WRITE(nitx_misc_cfg.wrd, NI_HV_GLB_NITX_MISC_CFG);

	/* interface select to init state */
    xge_misc_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MISC_CONFIG);
    xge_misc_cfg.bf.gige_mode = 1;
    xge_misc_cfg.bf.inf_sel = 0x0;
    SERDES_REG_WRITE(xge_misc_cfg.wrd, NI_HV_XGE_MISC_CONFIG);
    xge_misc_cfg.bf.gige_mode = 1;
    xge_misc_cfg.bf.inf_sel = 0x3;
    SERDES_REG_WRITE(xge_misc_cfg.wrd, NI_HV_XGE_MISC_CONFIG);

    mdelay(10);

    /* Tx IPG */
	xge_tx_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MAC_CFG_TX);
    xge_tx_cfg.bf.IFG_mode_tx = 0x1;
    SERDES_REG_WRITE(xge_tx_cfg.wrd, NI_HV_XGE_MAC_CFG_TX);
    /* Read thr */
    xge_tx2_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MAC_CFG_TX_2);
    xge_tx2_cfg.bf.fifo1_read_thr = 0xa;
    xge_tx2_cfg.bf.fifo1_thr_afull = 0xa;
    SERDES_REG_WRITE(xge_tx2_cfg.wrd, NI_HV_XGE_MAC_CFG_TX_2);
    #if 0
    /* Rx */
    xge_rx_cfg.wrd = CA_NI_REG_READ(NI_HV_XGE_MAC_CFG_RX);
    xge_rx_cfg.wrd = 0xbc1;
    CA_NI_REG_WRITE(xge_rx_cfg.wrd, NI_HV_XGE_MAC_CFG_RX);
    #endif

	xge_mac_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MAC_CFG_COM);
    xge_mac_cfg.bf.tx_rst = 0x1;
    xge_mac_cfg.bf.rx_rst = 0x1;
    xge_mac_cfg.bf.cntr_rst = 0x0;
    xge_mac_cfg.bf.stats_err_order = 0x0;
    SERDES_REG_WRITE(xge_mac_cfg.wrd, NI_HV_XGE_MAC_CFG_COM);
    mdelay(10);
    /* Release MAC resets */
    xge_mac_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MAC_CFG_COM);
    xge_mac_cfg.bf.tx_rst = 0x0;
    xge_mac_cfg.bf.rx_rst = 0x0;
    xge_mac_cfg.bf.cntr_rst = 0x0;
    xge_mac_cfg.bf.stats_err_order = 0x0;
    SERDES_REG_WRITE(xge_mac_cfg.wrd, NI_HV_XGE_MAC_CFG_COM);

    /* interface select to 1G */
	xge_misc_cfg.wrd = SERDES_REG_READ(NI_HV_XGE_MISC_CONFIG);
    xge_misc_cfg.bf.gige_mode = 1;
    xge_misc_cfg.bf.inf_sel = 0x0;
    SERDES_REG_WRITE(xge_misc_cfg.wrd, NI_HV_XGE_MISC_CONFIG);
    xge_misc_cfg.bf.gige_mode = 1;
    xge_misc_cfg.bf.inf_sel = 0x1;
    SERDES_REG_WRITE(xge_misc_cfg.wrd, NI_HV_XGE_MISC_CONFIG);

    /* Stop drain out */
	nitx_misc_cfg.wrd = SERDES_REG_READ(NI_HV_GLB_NITX_MISC_CFG);
    nitx_misc_cfg.bf.xge6_tx_drain = 0;
    SERDES_REG_WRITE(nitx_misc_cfg.wrd, NI_HV_GLB_NITX_MISC_CFG);

#ifdef CONFIG_RTK_NE
	/* Resume traffic */
	tm_es_ctrl.wrd = SERDES_REG_READ(L2TM_L2TM_ES_CTRL);
	tm_es_ctrl.bf.tx_port6_en = 1;
	SERDES_REG_WRITE(tm_es_ctrl.wrd, L2TM_L2TM_ES_CTRL);
	qm_es_ctrl.wrd = SERDES_REG_READ(QM_QM_ES_CTRL);
	qm_es_ctrl.bf.ni_en |= (1<<6);
	SERDES_REG_WRITE(qm_es_ctrl.wrd, QM_QM_ES_CTRL);
#endif

    return ;
}

ca_status_t __p6_serdes_ind_read(int page, int regaddr, ca_uint16_t *pData)
{
    QX_REG_ACCESS_t sds_access;
    QX_REG_DATAOUT_t sds_dataout;

    if(!pData)
    {
        return CA_E_PARAM;
    }

    sds_access.wrd = SERDES_REG_READ(QX_REG_ACCESS);
    sds_access.bf.sds_a = ((page << 5) | (regaddr));
    sds_access.bf.sds_we = 0;
    sds_access.bf.sds_access = 1;
    SERDES_REG_WRITE(sds_access.wrd, QX_REG_ACCESS);

    /* Wait at least 100ns */
    mdelay(1);
    
    sds_dataout.wrd = SERDES_REG_READ(QX_REG_DATAOUT);
    *pData = sds_dataout.bf.sds_d_out;

    return CA_E_OK;
}

ca_int32_t __p6_serdes_ind_write(int page, int regaddr, ca_uint16_t data)
{
    QX_REG_ACCESS_t sds_access;
    QX_REG_DATAIN_t sds_datain;

    sds_datain.wrd = SERDES_REG_READ(QX_REG_DATAIN);
    sds_datain.bf.sds_d_in = data;
    SERDES_REG_WRITE(sds_datain.wrd, QX_REG_DATAIN);

    sds_access.wrd = SERDES_REG_READ(QX_REG_ACCESS);
    sds_access.bf.sds_a = ((page << 5) | (regaddr));
    sds_access.bf.sds_we = 1;
    sds_access.bf.sds_access = 1;
    SERDES_REG_WRITE(sds_access.wrd, QX_REG_ACCESS);

    /* Wait at least 100ns */
    mdelay(1);

    return CA_E_OK;
}

static ca_status_t __p6_prof_load_from_file(rtk_sds_name_t sds_name)
{
	ca_int32_t i, profSize;;
	rtk_sds_prof_t *prof_p;

	printf("loading serders file: %s\n", RTK_SDS_SDS_NAME_STR(sds_name));

	switch (sds_name)
	{
		case RTK_SDS_NAME_QXGMII_2G5:
			prof_p = &qxgmii_p6_prof[0];
			profSize = sizeof(qxgmii_p6_prof);
			break;
		case RTK_SDS_NAME_USXGMII_10G:
			prof_p = &usxgmii_p6_prof[0];
			profSize = sizeof(usxgmii_p6_prof);
			break;
		case RTK_SDS_NAME_XFI_10G:
			prof_p = &xfi_p6_prof[0];
			profSize = sizeof(xfi_p6_prof);
			break;
		default:
			printf("rtk-sds: sds_name=%d not supported !\r\n", sds_name);
			return CA_E_ERROR;
	}

	for (i=0 ; i<profSize/sizeof(rtk_sds_prof_t) ; i++)
	{
		///--printf("### 0x%08x 0x%08x\n", prof_p[i].reg, prof_p[i].value);
		SERDES_REG_WRITE(prof_p[i].value, prof_p[i].reg);
	}

	return CA_E_OK;
}

static void __p6_sds_mode_get(rtk_sds_mode_t *pMode, rtk_sds_speed_t *pSpeed)
{
    QX_MODE_t qx_mode;
    QX_CH0_ABILITY_t qx_ability0;
    ca_uint32_t sds_spd;

    qx_mode.wrd = SERDES_REG_READ(QX_MODE);
    /* For most of the SDS mode, only channel 0 is referenced,
     * Only 10G-QXGMII mode the speed configuration for all 4 channels might be different
     */
    qx_ability0.wrd = SERDES_REG_READ(QX_CH0_ABILITY);
    sds_spd = ((qx_ability0.bf.sds01_sds0_ablty & (0x3 << 12)) >> 10) | (qx_ability0.bf.sds01_sds0_ablty & 0x3);
    switch(qx_mode.bf.sds_mode_s0)
    {
    case 0x1f:
    default:
        *pMode = RTK_SDS_MODE_NONE;
        *pSpeed = RTK_SDS_SPEED_DEFAULT;
        break;
    case 0x2:
    case 0x12:
        *pMode = RTK_SDS_MODE_SGMII;
        break;
    case 0x4:
    case 0x5:
        *pMode = RTK_SDS_MODE_FIBER;
        break;
    case 0x1a:
        *pMode = RTK_SDS_MODE_XFI;
        break;
    case 0xd:
        if(0 == qx_mode.bf.usx_sub_mx_s0)
        {
            *pMode = RTK_SDS_MODE_USXGMII;
        }
        else
        {
            *pMode = RTK_SDS_MODE_QXGMII;
        }        
        break;
    }

    if(RTK_SDS_MODE_NONE != *pMode)
    {
        switch(sds_spd)
        {
        case 0:
            *pSpeed = RTK_SDS_SPEED_10M;
            break;
        case 1:
            *pSpeed = RTK_SDS_SPEED_100M;
            break;
        case 2:
            *pSpeed = RTK_SDS_SPEED_1G;
            break;
        case 4:
            *pSpeed = RTK_SDS_SPEED_10G;
            break;
        case 5:
            *pSpeed = RTK_SDS_SPEED_2G5;
            break;
        case 6:
            *pSpeed = RTK_SDS_SPEED_5G;
            break;
        default:
            *pSpeed = RTK_SDS_SPEED_DEFAULT;
            break;
        }
    }

    return;
}


/*
4'b0000:  10M
4'b0001:  100M
4'b0010:  1G
4'b0011:  500M (USXG MODE)
4'b0100:  10G  (USXG MODE)
4'b0101:  2.5G
4'b0110:  5G   (USXG MODE)
*/
#ifdef CONFIG_RTK_NI_SUPPORT_QXGMII_P6
ca_int32_t
ca_ni_p6_qxgmii_port_macForceAbility_set(u32 port, u32 speed)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t speed01,speed23, mac_reg;
    QX_CH0_ABILITY_t qx_ability0;
    QX_CH0_ABILITY_t qx_ability1;
    QX_CH0_ABILITY_t qx_ability2;
    QX_CH0_ABILITY_t qx_ability3;
    NI_HV_PT_PORT_GLB_CFG_t port_glb_cfg;

    speed01 = speed & 0x3;
    speed23 = speed >> 2;
    mac_reg = NI_HV_PT_PORT_GLB_CFG + port * APB0_NI_HV_PT_STRIDE;
    switch(port)
    {
    case 0:
	case 1:
	case 2:
	case 3:
        /* Stop MAC Tx/Rx */
#if !defined(CONFIG_TARGET_RTK_MERCURY)
        port_glb_cfg.wrd = SERDES_REG_READ(mac_reg);
        port_glb_cfg.bf.power_dwn_tx = 1;
        port_glb_cfg.bf.power_dwn_rx = 1;
		SERDES_REG_WRITE(port_glb_cfg.wrd, mac_reg);
        mdelay(100);
#endif
        break;
    default:
        return CA_E_PORT_ID;
    }

    switch(port)
    {
	    case 0:
#if defined(CONFIG_TARGET_RTK_MERCURY)
			if (speed == 0)
				qx_ability0.wrd = 0x4cf4;
			else if (speed == 1)
				qx_ability0.wrd = 0x4cf5;
			else if (speed == 2)
				qx_ability0.wrd = 0x4cf6;
			else if (speed == 5)
				qx_ability0.wrd = 0x5cf5;
			else if (speed == 6)
				qx_ability0.wrd = 0x5cf6;
			else if (speed == 4)
				qx_ability0.wrd = 0x5cf4;
			SERDES_REG_WRITE(qx_ability0.wrd, QX_CH0_ABILITY);
#else
	        qx_ability0.wrd = SERDES_REG_READ(QX_CH0_ABILITY);
	        qx_ability0.bf.sds01_sds0_ablty &= ~((0x3 << 12) | (0x3 << 0));
	        qx_ability0.bf.sds01_sds0_ablty |= speed01 | (speed23 << 12);
	        SERDES_REG_WRITE(qx_ability0.wrd, QX_CH0_ABILITY);
#endif
	        break;
	    case 1:
#if defined(CONFIG_TARGET_RTK_MERCURY)
			if (speed == 0)
				qx_ability1.wrd = 0x4cf4;
			else if (speed == 1)
				qx_ability1.wrd = 0x4cf5;
			else if (speed == 2)
				qx_ability1.wrd = 0x4cf6;
			else if (speed == 5)
				qx_ability1.wrd = 0x5cf5;
			else if (speed == 6)
				qx_ability1.wrd = 0x5cf6;
			else if (speed == 4)
				qx_ability1.wrd = 0x5cf4;
			SERDES_REG_WRITE(qx_ability1.wrd, QX_CH1_ABILITY);
#else
	        qx_ability1.wrd = SERDES_REG_READ(QX_CH1_ABILITY);
	        qx_ability1.bf.sds01_sds0_ablty &= ~((0x3 << 12) | (0x3 << 0));
	        qx_ability1.bf.sds01_sds0_ablty |= speed01 | (speed23 << 12);
	        SERDES_REG_WRITE(qx_ability1.wrd, QX_CH1_ABILITY);
#endif
	        break;
	    case 2:
#if defined(CONFIG_TARGET_RTK_MERCURY)
			if (speed == 0)
				qx_ability2.wrd = 0x4cf4;
			else if (speed == 1)
				qx_ability2.wrd = 0x4cf5;
			else if (speed == 2)
				qx_ability2.wrd = 0x4cf6;
			else if (speed == 5)
				qx_ability2.wrd = 0x5cf5;
			else if (speed == 6)
				qx_ability2.wrd = 0x5cf6;
			else if (speed == 4)
				qx_ability2.wrd = 0x5cf4;
			SERDES_REG_WRITE(qx_ability2.wrd, QX_CH2_ABILITY);
#else
	        qx_ability2.wrd = SERDES_REG_READ(QX_CH2_ABILITY);
	        qx_ability2.bf.sds01_sds0_ablty &= ~((0x3 << 12) | (0x3 << 0));
	        qx_ability2.bf.sds01_sds0_ablty |= speed01 | (speed23 << 12);
	        SERDES_REG_WRITE(qx_ability2.wrd, QX_CH2_ABILITY);
#endif
	        break;
	    case 3:
#if defined(CONFIG_TARGET_RTK_MERCURY)
			if (speed == 0)
				qx_ability3.wrd = 0x4cf4;
			else if (speed == 1)
				qx_ability3.wrd = 0x4cf5;
			else if (speed == 2)
				qx_ability3.wrd = 0x4cf6;
			else if (speed == 5)
				qx_ability3.wrd = 0x5cf5;
			else if (speed == 6)
				qx_ability3.wrd = 0x5cf6;
			else if (speed == 4)
				qx_ability3.wrd = 0x5cf4;
			SERDES_REG_WRITE(qx_ability3.wrd, QX_CH3_ABILITY);
#else
	        qx_ability3.wrd = SERDES_REG_READ(QX_CH3_ABILITY);
	        qx_ability3.bf.sds01_sds0_ablty &= ~((0x3 << 12) | (0x3 << 0));
	        qx_ability3.bf.sds01_sds0_ablty |= speed01 | (speed23 << 12);
	        SERDES_REG_WRITE(qx_ability3.wrd, QX_CH3_ABILITY);
#endif
	        break;
    }
    mdelay(100);

#if !defined(CONFIG_TARGET_RTK_MERCURY)
    /* Resum MAC Tx/Rx */
	port_glb_cfg.wrd = SERDES_REG_READ(mac_reg);
    port_glb_cfg.bf.power_dwn_tx = 0;
    port_glb_cfg.bf.power_dwn_rx = 0;
	SERDES_REG_WRITE(port_glb_cfg.wrd, mac_reg);
#endif

    return CA_E_OK;
}

void ca_ni_p6_qxgmii_init(void)
{
#if defined(CONFIG_TARGET_RTK_MERCURY)
	NI_HV_GLB_SDS_CFG_t ni_hv_glb_sds_cfg;
	GLOBAL_BLOCK_RESET_t glb_blk_reset;
	QX_MODE_t qx_mode;
	QX_CH0_ABILITY_t qx_ch0_ability;
	QX_CH1_ABILITY_t qx_ch1_ability;
	QX_MISC_CNTL_1_t qx_misc_cntl_1;
	QX_RESET_t qx_reset;

	// PHY reset GPIO: Mercury GPIO0[2]:
#if 0
	SERDES_REG_WRITE(0x4, GLOBAL_GPIO_MUX_0);
	SERDES_REG_WRITE(0xfffffffb, PER_GPIO0_CFG);
	SERDES_REG_WRITE(0x4, PER_GPIO0_OUT);
#endif

	// write 0xf430b00c 0x10
	ni_hv_glb_sds_cfg.wrd = SERDES_REG_READ(NI_HV_GLB_SDS_CFG);
	ni_hv_glb_sds_cfg.bf.p6_sds_sel = 1;
	SERDES_REG_WRITE(ni_hv_glb_sds_cfg.wrd, NI_HV_GLB_SDS_CFG);
	// write 0xf4320070 0x6500005d (def 0x7d00005d)
	glb_blk_reset.wrd = SERDES_REG_READ(GLOBAL_BLOCK_RESET);
	glb_blk_reset.bf.reset_qxgmii = 0;
	glb_blk_reset.bf.reset_qxgmii_sds_reg = 0;
	SERDES_REG_WRITE(glb_blk_reset.wrd, GLOBAL_BLOCK_RESET);
	// write 0xf550c004 0x104d (def 0x0000105f)
	qx_mode.wrd = SERDES_REG_READ(QX_MODE);
	qx_mode.bf.sds_mode_s0 = 0xd;
	qx_mode.bf.use_wtg = 1;
	SERDES_REG_WRITE(qx_mode.wrd, QX_MODE);
	// write 0xf550c008 0x5cf4 
	qx_ch0_ability.wrd = SERDES_REG_READ(QX_CH0_ABILITY);
	qx_ch0_ability.bf.sds01_sds0_ablty = 0x5cf4;
	SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH0_ABILITY);
	// write 0xf550c00c 0x5cf4 
	qx_ch1_ability.wrd = SERDES_REG_READ(QX_CH1_ABILITY);
	qx_ch1_ability.bf.sds01_sds1_ablty = 0x5cf4;
	SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH1_ABILITY);
	// write 0xf550c018 0x2002 (def 0x00002000)
	qx_misc_cntl_1.wrd = SERDES_REG_READ(QX_MISC_CNTL_1);
	qx_misc_cntl_1.bf.cfg_fb_on = 1;
	SERDES_REG_WRITE(qx_misc_cntl_1.wrd, QX_MISC_CNTL_1);
	// write 0xf550c000 0x0 (def 0x00000001)
	qx_reset.wrd = SERDES_REG_READ(QX_RESET);
	qx_reset.bf.dp_reset = 0;
	SERDES_REG_WRITE(qx_reset.wrd, QX_RESET);
	// polling 0x1a000 read 0xf550c058
	if(!__p6_checkSdsClkReady())
    {
        printf("rtk-sds: p6 no clk ready\r\n");
    }
    else
    {
        mdelay(400);
    }
#else
	GLOBAL_BLOCK_RESET_t glb_blk_reset;
	QX_RESET_t qx_reset;
	QX_MODE_t qx_mode;
	QX_CH0_ABILITY_t qx_ch0_ability;
	QX_CH1_ABILITY_t qx_ch1_ability;
	QX_CH2_ABILITY_t qx_ch2_ability;
	QX_CH3_ABILITY_t qx_ch3_ability;
	NI_HV_GLB_STATIC_CFG_t ni_hv_glb_static_cfg;
	QX_MISC_CNTL_1_t qx_misc_cntl_1;	
	QX_SDS_STATE_t qx_sds_state;
	ca_uint16_t data;

	// PHY reset GPIO: 9615C GPIO4[0]:
#if 0
	SERDES_REG_WRITE(0x1, GLOBAL_GPIO_MUX_4);
	SERDES_REG_WRITE(0xfffffffe, PER_GPIO4_CFG);
	SERDES_REG_WRITE(0x1, PER_GPIO4_OUT);
#endif

	/* Global reset */
	glb_blk_reset.wrd = SERDES_REG_READ(GLOBAL_BLOCK_RESET);
	glb_blk_reset.bf.reset_qxgmii = 1;
	SERDES_REG_WRITE(glb_blk_reset.wrd, GLOBAL_BLOCK_RESET);
	mdelay(50);
	/* Release reset */
	glb_blk_reset.wrd = SERDES_REG_READ(GLOBAL_BLOCK_RESET);
	glb_blk_reset.bf.reset_qxgmii = 0;
	SERDES_REG_WRITE(glb_blk_reset.wrd, GLOBAL_BLOCK_RESET);

	/* Digital SDS related configuration */
    /* Datapath reset */
	qx_reset.wrd = SERDES_REG_READ(QX_RESET);
	qx_reset.bf.dp_reset = 1;
	SERDES_REG_WRITE(qx_reset.wrd, QX_RESET);
	/* SDS mode off */
	qx_mode.wrd = SERDES_REG_READ(QX_MODE);
	qx_mode.bf.sds_mode_s0 = 0x1f;
	SERDES_REG_WRITE(qx_mode.wrd, QX_MODE);
	mdelay(50);

	/* SDS mode */
	qx_mode.wrd = SERDES_REG_READ(QX_MODE);
	qx_mode.bf.sds_mode_s0 = 0xd;
	qx_mode.bf.usx_sub_mx_s0 = 0x2;
	SERDES_REG_WRITE(qx_mode.wrd, QX_MODE);
	/* SDS ability */
	qx_ch0_ability.wrd = SERDES_REG_READ(QX_CH0_ABILITY);
	qx_ch0_ability.bf.sds01_sds0_ablty = 0x5cf5;
	SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH0_ABILITY);
	qx_ch1_ability.wrd = SERDES_REG_READ(QX_CH1_ABILITY);
	qx_ch1_ability.bf.sds01_sds1_ablty = 0x5cf5;
	SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH1_ABILITY);
	qx_ch2_ability.wrd = SERDES_REG_READ(QX_CH2_ABILITY);
	qx_ch2_ability.bf.sds01_sds2_ablty = 0x5cf5;
	SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH2_ABILITY);
	qx_ch3_ability.wrd = SERDES_REG_READ(QX_CH3_ABILITY);
	qx_ch3_ability.bf.sds01_sds3_ablty = 0x5cf5;
	SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH3_ABILITY);

	/* Select MAC source */
#if !defined(CONFIG_TARGET_RTK_MERCURY)
	ni_hv_glb_static_cfg.wrd = SERDES_REG_READ(NI_HV_GLB_STATIC_CFG);
	ni_hv_glb_static_cfg.bf.port0_qxgmii_sel = 1;
    ni_hv_glb_static_cfg.bf.port1_qxgmii_sel = 1;
    ni_hv_glb_static_cfg.bf.port2_qxgmii_sel = 1;
    ni_hv_glb_static_cfg.bf.port3_qxgmii_sel = 1;
	SERDES_REG_WRITE(ni_hv_glb_static_cfg.wrd, NI_HV_GLB_STATIC_CFG);
#endif

	qx_misc_cntl_1.wrd = SERDES_REG_READ(QX_MISC_CNTL_1);
	qx_misc_cntl_1.bf.cfg_fb_on = 1;
	SERDES_REG_WRITE(qx_misc_cntl_1.wrd, QX_MISC_CNTL_1);

	/* Anlog SDS configurations */
    if(__p6_serdes_ind_read(0x20, 5, &data) != CA_E_OK)
    {
    	printf("rtk-sds: __p6_serdes_ind_read FAIL\r\n");
        return;
    }
    data = 0x9f;
    if(__p6_serdes_ind_write(0x20, 5, data) != CA_E_OK)
    {
    	printf("rtk-sds: __p6_serdes_ind_write FAIL\r\n");
        return;
    }

	/* Load serdes patch from file if any */
    /* TBD: chip revsion */
	__p6_prof_load_from_file(RTK_SDS_NAME_QXGMII_2G5);

	/* Release datapath reset */
	qx_reset.wrd = SERDES_REG_READ(QX_RESET);
	qx_reset.bf.dp_reset = 0;
	SERDES_REG_WRITE(qx_reset.wrd, QX_RESET);

	/* SDS mode */
	qx_mode.wrd = SERDES_REG_READ(QX_MODE);
	qx_mode.bf.use_wtg = 0x1;
	SERDES_REG_WRITE(qx_mode.wrd, QX_MODE);

	if(!__p6_checkSdsClkReady())
    {
        printf("rtk-sds: p6 no clk ready\r\n");
    }
    else
    {
        mdelay(400);
    }

	/* MODIFY_ME - Remove P6 MAC configure */
    /* NI related configuration */
	__p6_mac_10g_config();
#endif
}
#endif

#ifdef CONFIG_RTK_NI_SUPPORT_USXGMII_P6
u32 ca_ni_p6_usxgmii_speed_set(ca_uint16_t phySpeed)
{
	GLOBAL_BLOCK_RESET_t glb_blk_reset;
	QX_RESET_t qx_reset;
	QX_MODE_t qx_mode;
	QX_CH0_ABILITY_t qx_ch0_ability;
	NI_HV_GLB_STATIC_CFG_t ni_hv_glb_static_cfg;
	QX_MISC_CNTL_1_t qx_misc_cntl_1;	
	QX_SDS_STATE_t qx_sds_state;
	ca_uint16_t data;
	rtk_sds_speed_t sdsSpeed;

	/* Trans PHY speed to SDS speed */
	switch (phySpeed)
	{
		default:
		case 0:
			sdsSpeed = RTK_SDS_SPEED_10M;
			break;
		case 1:
			sdsSpeed = RTK_SDS_SPEED_100M;
			break;
		case 2:
		case 3:
			sdsSpeed = RTK_SDS_SPEED_1G;
			break;
		case 4:
			sdsSpeed = RTK_SDS_SPEED_10G;
			break;
		case 5:
			sdsSpeed = RTK_SDS_SPEED_2G5;
			break;
	}

	/* Global reset */
	glb_blk_reset.wrd = SERDES_REG_READ(GLOBAL_BLOCK_RESET);
	glb_blk_reset.bf.reset_qxgmii = 1;
	SERDES_REG_WRITE(glb_blk_reset.wrd, GLOBAL_BLOCK_RESET);
	mdelay(50);
	/* Release reset */
	glb_blk_reset.wrd = SERDES_REG_READ(GLOBAL_BLOCK_RESET);
	glb_blk_reset.bf.reset_qxgmii = 0;
	SERDES_REG_WRITE(glb_blk_reset.wrd, GLOBAL_BLOCK_RESET);

	/* Digital SDS related configuration */
    /* Datapath reset */
	qx_reset.wrd = SERDES_REG_READ(QX_RESET);
	qx_reset.bf.dp_reset = 1;
	SERDES_REG_WRITE(qx_reset.wrd, QX_RESET);
	/* SDS mode off */
	qx_mode.wrd = SERDES_REG_READ(QX_MODE);
	qx_mode.bf.sds_mode_s0 = 0x1f;
	SERDES_REG_WRITE(qx_mode.wrd, QX_MODE);
	mdelay(50);

	/* SDS mode */
	qx_mode.wrd = SERDES_REG_READ(QX_MODE);
	qx_mode.bf.sds_mode_s0 = 0xd;
	qx_mode.bf.usx_sub_mx_s0 = 0;
	SERDES_REG_WRITE(qx_mode.wrd, QX_MODE);

	switch(sdsSpeed)
    {
    	case RTK_SDS_SPEED_DEFAULT:
		case RTK_SDS_SPEED_10G:
			/* SDS ability */
			qx_ch0_ability.wrd = SERDES_REG_READ(QX_CH0_ABILITY);
			qx_ch0_ability.bf.sds01_sds0_ablty = 0x5cf4;
			SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH0_ABILITY);
			break;
		case RTK_SDS_SPEED_100M:
			/* SDS ability */
			qx_ch0_ability.wrd = SERDES_REG_READ(QX_CH0_ABILITY);
			qx_ch0_ability.bf.sds01_sds0_ablty = 0x4cf5;
			SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH0_ABILITY);
			break;
		case RTK_SDS_SPEED_5G:
			/* SDS ability */
			qx_ch0_ability.wrd = SERDES_REG_READ(QX_CH0_ABILITY);
			qx_ch0_ability.bf.sds01_sds0_ablty = 0x5cf6;
			SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH0_ABILITY);
			break;
		case RTK_SDS_SPEED_2G5:
			/* SDS ability */
			qx_ch0_ability.wrd = SERDES_REG_READ(QX_CH0_ABILITY);
			qx_ch0_ability.bf.sds01_sds0_ablty = 0x5cf5;
			SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH0_ABILITY);
			break;
		case RTK_SDS_SPEED_1G:
			/* SDS ability */
			qx_ch0_ability.wrd = SERDES_REG_READ(QX_CH0_ABILITY);
			qx_ch0_ability.bf.sds01_sds0_ablty = 0x4cf6;
			SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH0_ABILITY);
			break;
	}

	qx_misc_cntl_1.wrd = SERDES_REG_READ(QX_MISC_CNTL_1);
	qx_misc_cntl_1.bf.cfg_fb_on = 1;
	SERDES_REG_WRITE(qx_misc_cntl_1.wrd, QX_MISC_CNTL_1);

	/* Anlog SDS configurations */
	if(__p6_serdes_ind_read(0x20, 5, &data) != CA_E_OK)
    {
    	printf("rtk-sds: __p6_serdes_ind_read FAIL\r\n");
        return;
    }
    data = 0x9f;
    if(__p6_serdes_ind_write(0x20, 5, data) != CA_E_OK)
    {
    	printf("rtk-sds: __p6_serdes_ind_write FAIL\r\n");
        return;
    }

	/* Load serdes patch from file if any */
    /* TBD: chip revsion */
	__p6_prof_load_from_file(RTK_SDS_NAME_USXGMII_10G);

	/* Release datapath reset */
	qx_reset.wrd = SERDES_REG_READ(QX_RESET);
	qx_reset.bf.dp_reset = 0;
	SERDES_REG_WRITE(qx_reset.wrd, QX_RESET);

	if(!__p6_checkSdsClkReady())
    {
        printf("rtk-sds: p6 no clk ready\r\n");
    }
    else
    {
        mdelay(400);
    }

	/* MODIFY_ME - Remove P6 MAC configure */
    /* NI related configuration */
	switch(sdsSpeed)
    {
    	case RTK_SDS_SPEED_DEFAULT:
		case RTK_SDS_SPEED_10G:
		case RTK_SDS_SPEED_5G:
			__p6_mac_10g_config();
			break;
		case RTK_SDS_SPEED_2G5:
		case RTK_SDS_SPEED_1G:
		case RTK_SDS_SPEED_100M:
			__p6_mac_1g_config();
			break;
	}

#if 0 ///--port6 analog loopback
	data = 0x2c24;
    if(__p6_serdes_ind_write(46, 11, data) != CA_E_OK)
    {
    	printf("rtk-sds: __p6_serdes_ind_write FAIL\r\n");
        return;
    }
	data = 0x4c78;
    if(__p6_serdes_ind_write(46, 15, data) != CA_E_OK)
    {
    	printf("rtk-sds: __p6_serdes_ind_write FAIL\r\n");
        return;
    }
#endif

	return 0;
}

void ca_ni_p6_usxgmii_init(void)
{
	// PHY reset GPIO: 9615C GPIO4[0]:
#if 0
	SERDES_REG_WRITE(0x1, GLOBAL_GPIO_MUX_4);
	SERDES_REG_WRITE(0xfffffffe, PER_GPIO4_CFG);
	SERDES_REG_WRITE(0x1, PER_GPIO4_OUT);
#endif

	ca_ni_p6_usxgmii_speed_set(4);
}
#endif

#ifdef CONFIG_RTK_NI_SUPPORT_XFI_P6
u32 ca_ni_p6_xfi_speed_set(rtk_sds_speed_t sdsSpeed)
{
	GLOBAL_BLOCK_RESET_t glb_blk_reset;
	QX_RESET_t qx_reset;
	QX_MODE_t qx_mode;
	QX_CH0_ABILITY_t qx_ch0_ability;
	NI_HV_GLB_STATIC_CFG_t ni_hv_glb_static_cfg;
	QX_MISC_CNTL_1_t qx_misc_cntl_1;
	QX_SDS_STATE_t qx_sds_state;
	ca_uint16_t data;

	switch(sdsSpeed)
	{
		case RTK_SDS_SPEED_DEFAULT:
			/* XFI only 10G */
		case RTK_SDS_SPEED_10G:
			/* Global reset */
			glb_blk_reset.wrd = SERDES_REG_READ(GLOBAL_BLOCK_RESET);
			glb_blk_reset.bf.reset_qxgmii = 1;
			SERDES_REG_WRITE(glb_blk_reset.wrd, GLOBAL_BLOCK_RESET);
			mdelay(1);
			/* Release reset */
			glb_blk_reset.wrd = SERDES_REG_READ(GLOBAL_BLOCK_RESET);
			glb_blk_reset.bf.reset_qxgmii = 0;
			SERDES_REG_WRITE(glb_blk_reset.wrd, GLOBAL_BLOCK_RESET);

			/* Digital SDS related configuration */
			/* Datapath reset */
			qx_reset.wrd = SERDES_REG_READ(QX_RESET);
			qx_reset.bf.dp_reset = 1;
			SERDES_REG_WRITE(qx_reset.wrd, QX_RESET);
			/* SDS mode off */
			qx_mode.wrd = SERDES_REG_READ(QX_MODE);
			qx_mode.bf.sds_mode_s0 = 0x1f;
			SERDES_REG_WRITE(qx_mode.wrd, QX_MODE);
			mdelay(1);

			/* SDS mode */
			qx_mode.wrd = SERDES_REG_READ(QX_MODE);
			qx_mode.bf.sds_mode_s0 = 0x1a;
			SERDES_REG_WRITE(qx_mode.wrd, QX_MODE);
			/* SDS ability */
			qx_ch0_ability.wrd = SERDES_REG_READ(QX_CH0_ABILITY);
			qx_ch0_ability.bf.sds01_sds0_ablty = 0x5cf4;
			SERDES_REG_WRITE(qx_ch0_ability.wrd, QX_CH0_ABILITY);

			qx_misc_cntl_1.wrd = SERDES_REG_READ(QX_MISC_CNTL_1);
			qx_misc_cntl_1.bf.cfg_fb_on = 1;
			SERDES_REG_WRITE(qx_misc_cntl_1.wrd, QX_MISC_CNTL_1);


			/* Anlog SDS configurations */
			if(__p6_serdes_ind_read(0x20, 5, &data) != CA_E_OK)
		    {
		        printf("rtk-sds: __p6_serdes_ind_read FAIL\r\n");
		        return;
		    }
		    data = 0x9f;
		    if(__p6_serdes_ind_write(0x20, 5, data) != CA_E_OK)
		    {
		        printf("rtk-sds: __p6_serdes_ind_write FAIL\r\n");
		        return;
		    }

			/* Load serdes patch from file if any */
			/* TBD: chip revsion */
			__p6_prof_load_from_file(RTK_SDS_NAME_XFI_10G);

			/* Release datapath reset */
			qx_reset.wrd = SERDES_REG_READ(QX_RESET);
			qx_reset.bf.dp_reset = 0;
			SERDES_REG_WRITE(qx_reset.wrd, QX_RESET);

			if(!__p6_checkSdsClkReady())
			{
				printf("ca-sds: p6 no clk ready\r\n");
			}
			else
			{
				mdelay(400);
			}

			/* NI related configuration */
			__p6_mac_10g_config();
			break;
		default:
			return 1;
	}

	return 0;
}

void ca_ni_p6_xfi_init(void)
{
	ca_ni_p6_xfi_speed_set(RTK_SDS_SPEED_DEFAULT);
}
#endif
#endif

#ifdef CONFIG_TARGET_RTK_ELNATH
#ifdef CONFIG_RTK_NI_SUPPORT_HSGMII_P6
u32 ca_ni_p6_hsgmii_speed_set(rtk_sds_speed_t sdsSpeed)
{
	GLOBAL_DPHY_RESET_t glb_dphy_reset;
    GLOBAL_PHY_CONTROL_t glb_phy_ctrl;
    GLOBAL_PHY_SGMII_MISC_CONTROL_s3_t phy_sgmii_misc_s3; 
    GLOBAL_PHY_SGMII_PCS_CONTROL_s3_t phy_sgmii_pcs_s3;
	ca_uint16_t data;

	switch(sdsSpeed)
	{
		case RTK_SDS_SPEED_DEFAULT:
			/* XFI only 10G */
		case RTK_SDS_SPEED_10G:
			/* Global reset */
            glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.s3_reset_sgmii = 1;
            glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
            glb_dphy_reset.bf.reset_dphy_s3_pcie = 1;
            SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
            udelay(100);
            /* Release reset */
            glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
            glb_dphy_reset.bf.s3_reset_sgmii = 0;
            glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
            glb_dphy_reset.bf.reset_dphy_s3_pcie = 0;
            SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);

            glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
            glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0x1;
            SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);

            phy_sgmii_misc_s3.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
            phy_sgmii_misc_s3.bf.RX50_LINK_SGMII = 0x1;
            SERDES_REG_WRITE(phy_sgmii_misc_s3.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
            
            /* Speed & mode configuration */
            phy_sgmii_pcs_s3.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
            phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = 0x2;
            phy_sgmii_pcs_s3.bf.sgmii_sds0_ablty = 0x14002;
            SERDES_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
			break;
		case RTK_SDS_SPEED_100M:
			/* Global reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.s3_reset_sgmii = 1;
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
			glb_dphy_reset.bf.reset_dphy_s3_pcie = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(100);
			/* Release reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.s3_reset_sgmii = 0;
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
			glb_dphy_reset.bf.reset_dphy_s3_pcie = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
		
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0x1;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
		
			phy_sgmii_misc_s3.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
			phy_sgmii_misc_s3.bf.RX50_LINK_SGMII = 0x1;
			SERDES_REG_WRITE(phy_sgmii_misc_s3.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
			
			/* Speed & mode configuration */
			phy_sgmii_pcs_s3.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
			phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = 0x2;
			phy_sgmii_pcs_s3.bf.sgmii_sds0_ablty = 0x14001;
			SERDES_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
			break;
		case RTK_SDS_SPEED_10M:
			/* Global reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.s3_reset_sgmii = 1;
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
			glb_dphy_reset.bf.reset_dphy_s3_pcie = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(100);
			/* Release reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.s3_reset_sgmii = 0;
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
			glb_dphy_reset.bf.reset_dphy_s3_pcie = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
		
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0x1;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
		
			phy_sgmii_misc_s3.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
			phy_sgmii_misc_s3.bf.RX50_LINK_SGMII = 0x1;
			SERDES_REG_WRITE(phy_sgmii_misc_s3.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
			
			/* Speed & mode configuration */
			phy_sgmii_pcs_s3.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
			phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = 0x2;
			phy_sgmii_pcs_s3.bf.sgmii_sds0_ablty = 0x14000;
			SERDES_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
			break;
		case RTK_SDS_SPEED_2G5:
			/* Global reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.s3_reset_sgmii = 1;
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 1;
			glb_dphy_reset.bf.reset_dphy_s3_pcie = 1;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
			udelay(100);
			/* Release reset */
			glb_dphy_reset.wrd = SERDES_REG_READ(GLOBAL_DPHY_RESET);
			glb_dphy_reset.bf.s3_reset_sgmii = 0;
			glb_dphy_reset.bf.reset_dphy_s3_sgmii = 0;
			glb_dphy_reset.bf.reset_dphy_s3_pcie = 0;
			SERDES_REG_WRITE(glb_dphy_reset.wrd, GLOBAL_DPHY_RESET);
		
			glb_phy_ctrl.wrd = SERDES_REG_READ(GLOBAL_PHY_CONTROL);
			glb_phy_ctrl.bf.s3_pcie_usb3_sgmii_sel = 0x2;
			SERDES_REG_WRITE(glb_phy_ctrl.wrd, GLOBAL_PHY_CONTROL);
		
			phy_sgmii_misc_s3.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
			phy_sgmii_misc_s3.bf.RX50_LINK_SGMII = 0x1;
			SERDES_REG_WRITE(phy_sgmii_misc_s3.wrd, GLOBAL_PHY_SGMII_MISC_CONTROL_s3);
			
			/* Speed & mode configuration */
			phy_sgmii_pcs_s3.wrd = SERDES_REG_READ(GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
			phy_sgmii_pcs_s3.bf.sgmii_sds_mode0 = 0x12;
			phy_sgmii_pcs_s3.bf.sgmii_sds0_ablty = 0x01001;
			SERDES_REG_WRITE(phy_sgmii_pcs_s3.wrd, GLOBAL_PHY_SGMII_PCS_CONTROL_s3);
			break;
		default:
			return 1;
	}

	return 0;
}

void ca_ni_p6_hsgmii_init(void)
{
	ca_ni_p6_hsgmii_speed_set(RTK_SDS_SPEED_DEFAULT);
}
#endif
#endif

void ca_ni_serdes_p6_dbg_dump(void)
{
#ifdef CONFIG_TARGET_RTK_TAURUS
	ca_uint16_t data;
	rtk_sds_mode_t sdsMode;
	rtk_sds_speed_t sdsSpeed;
    QX_SDS_STATE_t sds_state;
    QX_IF_STATUS_t if_status;

	printf("Port 6 Serdes:\n");
	__p6_sds_mode_get(&sdsMode, &sdsSpeed);
	printf("	mode = 0x%08x\n", sdsMode);
	printf("	speed = 0x%08x\n", sdsSpeed);
	/* SDS status for speed < 5G */
    __p6_serdes_ind_read(0x1, 30, &data);
	printf("	signal_ok = 0x%08x\n", (data & 0x100) ? 1 : 0);
	printf("	link_ok = 0x%08x\n", (data & 0x010) ? 1 : 0);
	printf("	sync_ok = 0x%08x\n", (data & 0x001) ? 1 : 0);
    /* Analog SDS status */
    sds_state.wrd = SERDES_REG_READ(QX_SDS_STATE);
	printf("	ber_notify = 0x%08x\n", sds_state.bf.BER_NOTIFY);
	printf("	rx_idle = 0x%08x\n", sds_state.bf.RXIDLE);
	printf("	ck_rdy = 0x%08x\n", sds_state.bf.CKRDY);
	/* SDS status for 10G(sync for 10G) */
    __p6_serdes_ind_read(0x5, 1, &data);
	printf("	blk_lock = 0x%08x\n", (data & (1<<15)) ? 1 : 0);
	/* Digital SDS status */
    if_status.wrd = SERDES_REG_READ(QX_IF_STATUS);
	printf("	link_ok_10G = 0x%08x\n", if_status.bf.link_ok_tgxr0);
	printf("	link_ok_ch0 = 0x%08x\n", if_status.bf.sds01_sds_link_ok_0);
	printf("	link_ok_ch1 = 0x%08x\n", if_status.bf.sds01_sds_link_ok_1);
	printf("	link_ok_ch2 = 0x%08x\n", if_status.bf.sds01_sds_link_ok_2);
	printf("	link_ok_ch3 = 0x%08x\n", if_status.bf.sds01_sds_link_ok_3);
	printf("	link_ok_rxaui = 0x%08x\n", 0);
#endif
}

static int do_sds_ind_reg(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int ret, i;
	u32 sds_page, reg;
  	u16 val;

	if (argc < 2) {
#if CONFIG_SYS_LONGHELP
		printf("Usage:\nsds_reg_value%s\n", cmdtp->help);
#endif
		return -1;
	}

	sds_page = simple_strtoul(argv[1], NULL, 10);
	reg = simple_strtoul(argv[2], NULL, 10);

	if (argc == 3) {
		/* read cmd */
		__p6_serdes_ind_read(sds_page, reg, &val);
		printf("SDS page %d Reg 0x%04X = 0x%04X\n", sds_page, reg, val);
	} else {
		/* argc > 3*/
		/* write cmd */
		val = simple_strtoul(argv[3], NULL, 10);
		ret = __p6_serdes_ind_write(sds_page, reg, val);
		if (!ret) {
			printf("SDS page %d Reg 0x%04X = 0x%04X\n", sds_page, reg, val);
		} else {
			printf("Can't write PHY addr %d Reg 0x%04X as 0x%04X, ret = %d\n", sds_page, reg, val, ret);
		}
	}
}

U_BOOT_CMD(
		sds_ind_reg,   4,      1,      do_sds_ind_reg,
		"read/write sds ind reg\n",
		"[SDS page] [SDS ext reg] ([value])\n"
	  );

