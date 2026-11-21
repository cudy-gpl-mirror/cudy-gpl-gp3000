/* Copyright (C) 2000-2002 Joakim Axelsson <gozem@linux.nu>
 *                         Patrick Schaaf <bof@bof.de>
 *                         Martin Josefsson <gandalf@wlug.westbo.se>
 * Copyright (C) 2003-2010 Jozsef Kadlecsik <kadlec@blackhole.kfki.hu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.  
 */

/* Shared library add-on to iptables to add IP set matching. */
#include <stdbool.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <errno.h>
#include "../include/ebtables_u.h"

#include <linux/netfilter/xt_set.h>
#include "ebt_set.h"

/* Revision 0 */

static void set_help(void)
{
	printf("set match options:\n"
	       " --match-set [!] name flags [--return-nomatch]\n"
	       "		 'name' is the set name from to match,\n" 
	       "		 'flags' are the comma separated list of\n"
	       "		 'src' and 'dst' specifications.\n");
}

static struct option opts[] = {
	{ "match-set",      required_argument, 0, '1' },
	{ "return-nomatch", no_argument,       0, '2' },
	{0}
};

static void set_init(struct ebt_entry_match *m)
{
	struct xt_set_info_match_v1 *info = (struct xt_set_info_match_v1*)m->data;

	memset(info, 0, sizeof(struct xt_set_info_match_v1));
}

static int set_parse(int c, char **argv, int argc, const struct ebt_u_entry *entry,
   unsigned int *flags, struct ebt_entry_match **match)
{
	struct xt_set_info_match_v1 *myinfo = 
		(struct xt_set_info_match_v1 *) (*match)->data;
	struct xt_set_info *info = &myinfo->match_set;

	switch (c) {
	case '2':
		info->flags |= IPSET_RETURN_NOMATCH;
		break;
	case '1':	/* --match-set <set> <flag>[,<flag> */
		if (info->dim)
			ebt_print_error( "--match-set can be specified only once");
		
		if (ebt_check_inverse2(optarg))
			info->flags |= IPSET_INV_MATCH;

		if (strlen(optarg) > IPSET_MAXNAMELEN - 1)
			ebt_print_error("setname `%s' too long, max %d characters.",
				      optarg, IPSET_MAXNAMELEN - 1);

		get_set_byname(optarg, info);
		parse_dirs(argv[optind], info);
		optind++;
		break;
	default:
		return 0;
	}

	return 1;
}

static void print_match(const char *prefix, const struct xt_set_info *info)
{
	int i;
	char setname[IPSET_MAXNAMELEN];

	get_set_byid(setname, info->index);
	printf("%s %s %s",
			prefix,
	       (info->flags & IPSET_INV_MATCH) ? " !" : "",
	       setname); 
	for (i = 1; i <= info->dim; i++) {		
		printf("%s%s",
		       i == 1 ? " " : ",",
		       info->flags & (1 << i) ? "src" : "dst");
	}
	printf(" ");
}

/* Prints out the matchinfo. */
static void set_print(const struct ebt_u_entry *entry, const struct ebt_entry_match *match)
{
	struct xt_set_info_match_v1 *info = (struct xt_set_info_match_v1*)match->data;

	print_match("--match-set", &info->match_set);
	if (info->match_set.flags & IPSET_RETURN_NOMATCH)
		printf(" --return-nomatch");
}

static int set_compare(const struct ebt_entry_match *m1, const struct ebt_entry_match *m2)
{
	struct xt_set_info_match_v1 *info1 = (struct xt_set_info_match_v1 *)m1->data;
	struct xt_set_info_match_v1 *info2 = (struct xt_set_info_match_v1 *)m2->data;
		
	return (memcmp(info1, info2, sizeof(struct xt_set_info_match_v1)))?0:1;
}

static void set_check(const struct ebt_u_entry *entry,
   const struct ebt_entry_match *match, const char *name,
   unsigned int hookmask, unsigned int time)
{
	struct xt_set_info_match_v1 *info = (struct xt_set_info_match_v1*)match->data;
	return;
} 

static struct ebt_u_match set_match =
{
   .name          = "set",
   .size          = sizeof(struct xt_set_info_match_v1),
   .help          = set_help,
   .init          = set_init,
   .parse         = set_parse,
   .final_check   = set_check,
   .print         = set_print,
   .compare       = set_compare,
   .extra_ops     = opts,
};

void _init(void)
{
	ebt_register_match(&set_match);
}
