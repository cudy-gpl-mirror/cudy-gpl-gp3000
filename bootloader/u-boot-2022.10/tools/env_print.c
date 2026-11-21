// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2011 Free Electrons
 * David Wagner <david.wagner@free-electrons.com>
 *
 * Inspired from envcrc.c:
 * (C) Copyright 2001
 * Paolo Scaffardi, AIRVENT SAM s.p.a - RIMINI(ITALY), arsenio@tin.it
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <u-boot/crc.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "compiler.h"
#include <u-boot/crc.h>
#include <version.h>

#include <linux/kconfig.h>
#include <config.h>

#define CRC_SIZE sizeof(uint32_t)

static void usage(const char *exec_name)
{
	fprintf(stderr, "%s [-e]\n"
	       "\n"
	       "\n",
	       exec_name);
}

long int xstrtol(const char *s)
{
	long int tmp;

	errno = 0;
	tmp = strtol(s, NULL, 0);
	if (!errno)
		return tmp;

	if (errno == ERANGE)
		fprintf(stderr, "Bad integer format: %s\n",  s);
	else
		fprintf(stderr, "Error while parsing %s: %s\n", s,
				strerror(errno));

	exit(EXIT_FAILURE);
}

#define CHUNK_SIZE 4096

int main(int argc, char **argv)
{
	int option;
	int ret = EXIT_SUCCESS;

	const char *prg;

	prg = basename(argv[0]);

	/* Turn off getopt()'s internal error message */
	opterr = 0;

	/* Parse the cmdline */
	while ((option = getopt(argc, argv, ":abe")) != -1) {
		switch (option) {
		case 'a':
#ifdef CONFIG_ENV_OFFSET
			printf("0x%08x", CONFIG_ENV_OFFSET);
#endif
			return EXIT_SUCCESS;
		case 'b':
#ifdef CONFIG_ENV_OFFSET
			printf("0x%08x", CONFIG_ENV_OFFSET_REDUND);
#endif
			return EXIT_SUCCESS;
		case 'e':
#ifdef CONFIG_ENV_OFFSET
			printf("CONFIG_ENV_OFFSET 0x%08x\n", CONFIG_ENV_OFFSET);
#endif
#ifdef CONFIG_ENV_OFFSET_REDUND
			printf("CONFIG_ENV_OFFSET_REDUND 0x%08x\n", CONFIG_ENV_OFFSET_REDUND);
#endif
#ifdef CONFIG_NAND_HIDE_BAD
			printf("NAND_BBL_OFFSET 0x%08x\n", NAND_BBL_OFFSET);
			printf("NAND_BBL_OFFSET_REDUND 0x%08x\n", NAND_BBL_OFFSET_REDUND);
#endif
			return EXIT_SUCCESS;
		default:
			fprintf(stderr, "Wrong option -%c\n", optopt);
			usage(prg);
			return EXIT_FAILURE;
		}
	}


	return ret;
}
