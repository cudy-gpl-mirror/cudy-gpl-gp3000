/* Shared library add-on to iptables to add connmark2 matching support.
 *
 * (C) 2002,2004 MARA Systems AB <http://www.marasystems.com>
 * by Henrik Nordstrom <hno@marasystems.com>
 *
 * Version 1.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <xtables.h>
#include <linux/netfilter/xt_connmark2.h>

struct xt_connmark2_info {
	unsigned long long mark2, mask2;
	uint8_t invert;
};

enum {
	O_MARK2 = 0,
};

static void connmark2_mt_help(void)
{
	printf(
"connmark2 match options:\n"
"[!] --mark2 value[/mask]    Match ctmark value with optional mask\n");
}

static const struct xt_option_entry connmark2_mt_opts[] = {
	{.name = "mark2", .id = O_MARK2, .type = XTTYPE_MARK2MASK64,
	 .flags = XTOPT_MAND | XTOPT_INVERT},
	XTOPT_TABLEEND,
};

static void connmark2_mt_parse(struct xt_option_call *cb)
{
	struct xt_connmark2_mtinfo1 *info = cb->data;

	xtables_option_parse(cb);
	if (cb->invert)
		info->invert = true;
	info->mark2 = cb->val.mark2;
	info->mask2 = cb->val.mask2;
}

static void connmark2_parse(struct xt_option_call *cb)
{
	struct xt_connmark2_info *markinfo = cb->data;

	xtables_option_parse(cb);
	markinfo->mark2 = cb->val.mark2;
	markinfo->mask2 = cb->val.mask2;
	if (cb->invert)
		markinfo->invert = 1;
}

static void print_mark2(unsigned long long mark2, unsigned long long mask2)
{
	if (mask2 != 0xffffffffffffffffLU)
		printf(" 0x%llx/0x%llx", mark2, mask2);
	else
		printf(" 0x%llx", mark2);
}

static void
connmark2_print(const void *ip, const struct xt_entry_match *match, int numeric)
{
	const struct xt_connmark2_info *info = (const void *)match->data;

	printf(" CONNMARK2 match ");
	if (info->invert)
		printf("!");
	print_mark2(info->mark2, info->mask2);
}

static void
connmark2_mt_print(const void *ip, const struct xt_entry_match *match, int numeric)
{
	const struct xt_connmark2_mtinfo1 *info = (const void *)match->data;

	printf(" connmark2 match ");
	if (info->invert)
		printf("!");
	print_mark2(info->mark2, info->mask2);
}

static void connmark2_save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_connmark2_info *info = (const void *)match->data;

	if (info->invert)
		printf(" !");

	printf(" --mark2");
	print_mark2(info->mark2, info->mask2);
}

static void
connmark2_mt_save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_connmark2_mtinfo1 *info = (const void *)match->data;

	if (info->invert)
		printf(" !");

	printf(" --mark2");
	print_mark2(info->mark2, info->mask2);
}

static struct xtables_match connmark2_mt_reg[] = {
	{
		.family        = NFPROTO_UNSPEC,
		.name          = "connmark2",
		.revision      = 0,
		.version       = XTABLES_VERSION,
		.size          = XT_ALIGN(sizeof(struct xt_connmark2_info)),
		.userspacesize = XT_ALIGN(sizeof(struct xt_connmark2_info)),
		.help          = connmark2_mt_help,
		.print         = connmark2_print,
		.save          = connmark2_save,
		.x6_parse      = connmark2_parse,
		.x6_options    = connmark2_mt_opts,
	},
	{
		.version       = XTABLES_VERSION,
		.name          = "connmark2",
		.revision      = 1,
		.family        = NFPROTO_UNSPEC,
		.size          = XT_ALIGN(sizeof(struct xt_connmark2_mtinfo1)),
		.userspacesize = XT_ALIGN(sizeof(struct xt_connmark2_mtinfo1)),
		.help          = connmark2_mt_help,
		.print         = connmark2_mt_print,
		.save          = connmark2_mt_save,
		.x6_parse      = connmark2_mt_parse,
		.x6_options    = connmark2_mt_opts,
	},
};

void _init(void)
{
	xtables_register_matches(connmark2_mt_reg, ARRAY_SIZE(connmark2_mt_reg));
}
