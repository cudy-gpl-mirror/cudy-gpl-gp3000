#include <stdio.h>
#if defined(__GLIBC__) && __GLIBC__ == 2
#include <net/ethernet.h>
#else
#include <linux/if_ether.h>
#endif
#include <xtables.h>
#include <linux/netfilter/xt_macrange.h>
#include <iptables/internal.h>

enum
{
	O_SRC_MAC_RANGE = 0,
};

static void mac_range_help(void)
{
	printf(
		"MAC v%s options:\n"
		"[!] --mac-source-range XX:XX:XX:XX:XX:XX-XX:XX:XX:XX:XX:XX\n"
		"				Match source MAC address range\n"
		"\n", IPTABLES_VERSION);
}

static const struct xt_option_entry mac_range_opts[] =
{
	{
		.name = "mac-source-range", .id = O_SRC_MAC_RANGE, .type = XTTYPE_STRING,
		.flags = XTOPT_MAND | XTOPT_INVERT,
	},
	XTOPT_TABLEEND,
};

static void
parse_mac_range(const char *arg, struct xt_mac_range *info)
{
	unsigned int i = 0;
	char *dash;

	dash = strchr(arg, '-');
	if (dash != NULL)
		*dash = '\0';
	else
		xtables_error(PARAMETER_PROBLEM,
			   "Invalid mac range!\n");

	if (strlen(arg) != ETH_ALEN*3-1)
		xtables_error(PARAMETER_PROBLEM, "Bad mac address \"%s\"", arg);

	for (i = 0; i < ETH_ALEN; i++) {
		long number;
		char *end;

		number = strtol(arg + i*3, &end, 16);

		if (end == arg + i*3 + 2
			&& number >= 0
			&& number <= 255)
		{
			info->macaddr_start[i] = number;
		}
		else
			xtables_error(PARAMETER_PROBLEM,
				   "Bad mac address `%s'", arg);
	}

	dash += 1;
	if (strlen(dash) != ETH_ALEN*3-1)
		xtables_error(PARAMETER_PROBLEM, "Bad mac address \"%s\"", dash);

	for (i = 0; i < ETH_ALEN; i++) {
		long number;
		char *end;

		number = strtol(dash + i*3, &end, 16);

		if (end == dash + i*3 + 2
			&& number >= 0
			&& number <= 255)
		{
			info->macaddr_end[i] = number;
		}
		else
			xtables_error(PARAMETER_PROBLEM,
				   "Bad mac address `%s'", dash);
	}
}

static void mac_range_parse(struct xt_option_call *cb)
{
	struct xt_mac_range_info *macinfo = cb->data;

	if(cb->entry->id == O_SRC_MAC_RANGE)  	//--mac-source-range
	{
		xtables_option_parse(cb);
		parse_mac_range(cb->arg, macinfo);
		if(cb->invert)
		{
			macinfo->flags |= MAC_SRC_RANGE_INV;
		}
		macinfo->flags |= MAC_SRC_RANGE;
	}
	else
	{
		xtables_error(PARAMETER_PROBLEM, "mac match: unknown option!");
	}
}

static void print_mac_range(struct xt_mac_range *info)
{
	unsigned int i;

	printf("%02X", info->macaddr_start[0]);
	for (i = 1; i < ETH_ALEN; i++)
		printf(":%02X", info->macaddr_start[i]);
	printf("-");

	printf("%02X", info->macaddr_end[0]);
	for (i = 1; i < ETH_ALEN; i++)
		printf(":%02X", info->macaddr_end[i]);
		
	printf(" ");
}

static void
mac_range_print(const void *ip, const struct xt_entry_match *match, int numeric)
{
	const struct xt_mac_range_info *info = (void *)match->data;

	if(info->flags & MAC_SRC_RANGE)
	{
		printf(" SRC MAC RANGE ");
		if(info->flags & MAC_SRC_RANGE_INV)
		{
			printf(" !");
		}
		print_mac_range(&(info->srcaddr));
	}
}

static void mac_range_save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_mac_range_info *info = (void *)match->data;

	if(info->flags & MAC_SRC_RANGE)
	{
		if(info->flags & MAC_SRC_RANGE_INV)
		{
			printf(" !");
		}
		printf(" --mac-source-range ");
		print_mac_range(&(info->srcaddr));
	}
}

static void final_check(unsigned int flags)
{
	int check;

	check = flags & (MAC_SRC_RANGE);

	if(!check)
		xtables_error(PARAMETER_PROBLEM, "You must specify '--mac-source-range'");
}

static struct xtables_match mac_range_match =
{
	.family		= NFPROTO_UNSPEC,
	.name		= "macrange",
	.version	= XTABLES_VERSION,
	.size		= XT_ALIGN(sizeof(struct xt_mac_range_info)),
	.userspacesize	= XT_ALIGN(sizeof(struct xt_mac_range_info)),
	.help		= mac_range_help,
	.x6_parse	= mac_range_parse,
	.final_check	= final_check,
	.print		= mac_range_print,
	.save		= mac_range_save,
	.x6_options	= mac_range_opts,
};

void _init(void)
{
	xtables_register_match(&mac_range_match);
}
