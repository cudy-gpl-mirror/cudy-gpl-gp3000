#include <stdio.h>
#include <xtables.h>
#include <linux/netfilter/xt_outphysdev.h>

enum {
	O_PHYSDEV = 0,
};

static void outphysdev_help(void)
{
	printf(
"output physdev match options:\n"
"--physdev			will leave on a bridge device\n");
}

static const struct xt_option_entry outphysdev_opts[] =
{
	{
		.name = "physdev", .id = O_PHYSDEV, .type = XTTYPE_STRING,
		.flags = XTOPT_MAND | XTOPT_INVERT,
	},
	XTOPT_TABLEEND,
};

static void outphysdev_parse(struct xt_option_call *cb)
{
	struct xt_outphysdev_info *info = cb->data;

	xtables_option_parse(cb);
	if(cb->entry->id == O_PHYSDEV) {
		xtables_parse_interface(cb->arg, info->out_physdev,
			(unsigned char *)info->mask);
		
		info->bitmask |= XT_OUTPHYSDEV_OP;
	}
}

static void outphysdev_check(struct xt_fcheck_call *cb)
{
	if (cb->xflags == 0)
		xtables_error(PARAMETER_PROBLEM, "OUTPHYSDEV: no outphysdev option specified");
}

static void
outphysdev_print(const void *ip, const struct xt_entry_match *match, int numeric)
{
	const struct xt_outphysdev_info *info = (const void *)match->data;

	printf(" OUTPHYSDEV match");
	if (info->bitmask & XT_OUTPHYSDEV_OP)
		printf("--physdev %s", info->out_physdev);
}

static void outphysdev_save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_outphysdev_info *info = (const void *)match->data;

	if (info->bitmask & XT_OUTPHYSDEV_OP)
		printf("--physdev %s", info->out_physdev);
}

static struct xtables_match outphysdev_match = {
	.family		= NFPROTO_UNSPEC,
	.name		= "outphysdev",
	.version	= XTABLES_VERSION,
	.size		= XT_ALIGN(sizeof(struct xt_outphysdev_info)),
	.userspacesize	= XT_ALIGN(sizeof(struct xt_outphysdev_info)),
	.help		= outphysdev_help,
	.print		= outphysdev_print,
	.save		= outphysdev_save,
	.x6_parse	= outphysdev_parse,
	.x6_fcheck	= outphysdev_check,
	.x6_options	= outphysdev_opts,
};

void _init(void)
{
	xtables_register_match(&outphysdev_match);
}

