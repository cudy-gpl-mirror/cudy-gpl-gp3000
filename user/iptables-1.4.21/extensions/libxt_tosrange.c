#include <stdio.h>
#include <xtables.h>
#include <linux/netfilter/xt_dscprange.h>

enum {
	O_TOS_RANGE = 0,
};

static void tos_range_help(void)
{
	printf(
"tos match options\n"
"[!] --tos-range value1[:value2]	Match ToS against value or range\n"
"                                of values (inclusive)\n");
}

static const struct xt_option_entry tos_range_opts[] = {
	{.name = "tos-range", .id = O_TOS_RANGE, .type = XTTYPE_UINT8RC,
	 .flags = XTOPT_MAND | XTOPT_INVERT},
	XTOPT_TABLEEND,
};

static void tos_range_parse(struct xt_option_call *cb)
{
	struct xt_tos_range_info *info = cb->data;

	xtables_option_parse(cb);
	
	info->tos_min = cb->val.u8_range[0];
	info->tos_max = cb->val.u8_range[0];
	if (cb->nvals >= 2) {
		info->tos_max = cb->val.u8_range[1];
	}
	if (cb->invert)
		info->invert = 1;
}

static void
tos_range_print(const void *ip, const struct xt_entry_match *match, int numeric)
{
	const struct xt_tos_range_info *dinfo = (const struct xt_tos_range_info *)match->data;
		
	printf(" ToS range %s0x%02x-0x%02x", dinfo->invert ? "!" : "", dinfo->tos_min, dinfo->tos_max);
}

static void tos_range_save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_tos_range_info *dinfo = (const struct xt_tos_range_info *)match->data;

	printf("%s --tos-range 0x%02x-0x%02x", dinfo->invert ? "! " : "", dinfo->tos_min, dinfo->tos_max);
}

static struct xtables_match tos_range_match = {
	.family		= NFPROTO_UNSPEC,
	.name 		= "tosrange",
	.version 	= XTABLES_VERSION,
	.size 		= XT_ALIGN(sizeof(struct xt_tos_range_info)),
	.userspacesize	= XT_ALIGN(sizeof(struct xt_tos_range_info)),
	.help		= tos_range_help,
	.print		= tos_range_print,
	.save		= tos_range_save,
	.x6_parse	= tos_range_parse,
	.x6_options	= tos_range_opts,
};

void _init(void)
{
	xtables_register_match(&tos_range_match);
}
