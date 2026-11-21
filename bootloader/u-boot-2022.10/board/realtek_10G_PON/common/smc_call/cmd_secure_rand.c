/*
 * Copyright (C) 2020 Realtek
 */
#include <common.h>
#include <command.h>
#include <asm/io.h>
#include "cortina_smc.h"
void rtk_board_prng_read64(uint64_t *data);
static int do_get_secure_rnd64(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u64 val = 0;;
	rtk_board_prng_read64(&val);
	printf("%llu\n", val);
	return 0;
}


U_BOOT_CMD(
    secure_rnd64,  4,  1,  do_get_secure_rnd64,
	"Getting SoC Random 64bit number\n",
	"\n"
);

