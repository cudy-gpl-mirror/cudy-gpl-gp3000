#include <stdio.h>
#include <xtables.h>
#include <linux/netfilter/xt_dscprange.h>

enum {
	O_DSCP_RANGE = 0,
};

static void dscp_range_help(void)
{
	printf(
"dscp match options\n"
"[!] --dscp-range value1[:value2]		Match DSCP against value or range\n"
"                                of values (inclusive)\n");
}

static const struct xt_option_entry dscp_range_opts[] = {
	{.name = "dscp-range", .id = O_DSCP_RANGE, .type = XTTYPE_UINT8RC,
	 .flags = XTOPT_MAND | XTOPT_INVERT},
	XTOPT_TABLEEND,
};

static void dscp_range_parse(struct xt_option_call *cb)
{
	struct xt_dscp_range_info *info = cb->data;

	xtables_option_parse(cb);

	if (cb->val.u8_range[0] > XT_DSCP_MAX)
		xtables_error(PARAMETER_PROBLEM,
			   "DSCP `%d` out of range\n", cb->val.u8_range[0]);
	
	info->dscp_min = cb->val.u8_range[0];
	info->dscp_max = cb->val.u8_range[0];
	if (cb->nvals >= 2) {
		if (cb->val.u8_range[1] > XT_DSCP_MAX)
			xtables_error(PARAMETER_PROBLEM,
			   "DSCP `%d` out of range\n", cb->val.u8_range[1]);
		info->dscp_max = cb->val.u8_range[1];
	}
	if (cb->invert)
		info->invert = 1;
}

static void
dscp_range_print(const void *ip, const struct xt_entry_match *match, int numeric)
{
	const struct xt_dscp_range_info *dinfo = (const struct xt_dscp_range_info *)match->data;
		
	printf(" DSCP range %s0x%02x-0x%02x", dinfo->invert ? "!" : "", dinfo->dscp_min, dinfo->dscp_max);
}

static void dscp_range_save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_dscp_range_info *dinfo = (const struct xt_dscp_range_info *)match->data;

	printf("%s --dscp-range 0x%02x-0x%02x", dinfo->invert ? "! " : "", dinfo->dscp_min, dinfo->dscp_max);
}

static struct xtables_match dscp_range_match = {
	.family		= NFPROTO_UNSPEC,
	.name 		= "dscprange",
	.version 	= XTABLES_VERSION,
	.size 		= XT_ALIGN(sizeof(struct xt_dscp_range_info)),
	.userspacesize	= XT_ALIGN(sizeof(struct xt_dscp_range_info)),
	.help		= dscp_range_help,
	.print		= dscp_range_print,
	.save		= dscp_range_save,
	.x6_parse	= dscp_range_parse,
	.x6_options	= dscp_range_opts,
};

void _init(void)
{
	xtables_register_match(&dscp_range_match);
}
