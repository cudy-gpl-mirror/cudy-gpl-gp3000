/*
 *	ebt_time - ebtables part for xt_time
 *	Copyright © CC Computer Consultants GmbH, 2007
 *
 *	ebt_time.c is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 or 3 of the License.
 *
 *	Based on libxt_time.c.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <linux/types.h>
#include "../include/ebtables_u.h"

#ifndef INT_MAX
#define INT_MAX		((int)(~0U>>1))
#endif
#ifndef ULONG_MAX
#define ULONG_MAX	(~0UL)
#endif
#ifndef LONG_MAX
#define LONG_MAX	((long)(~0UL >> 1))
#endif

struct xt_time_info {
	__u32 date_start;
	__u32 date_stop;
	__u32 daytime_start;
	__u32 daytime_stop;
	__u32 monthdays_match;
	__u8 weekdays_match;
	__u8 flags;
};

enum {
	/* Match against local time (instead of UTC) */
	XT_TIME_LOCAL_TZ = 1 << 0,
	XT_TIME_CONTIGUOUS = 1 << 1,

	/* Shortcuts */
	XT_TIME_ALL_MONTHDAYS = 0xFFFFFFFE,
	XT_TIME_ALL_WEEKDAYS  = 0xFE,
	XT_TIME_MIN_DAYTIME   = 0,
	XT_TIME_MAX_DAYTIME   = 24 * 60 * 60 - 1,
};

static const char *const week_days[] = {
	NULL, "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun",
};

static struct option opts[] = {
	{ "datestart",  required_argument, 0, '1' },
	{ "datestop",   required_argument, 0, '2' },
	{ "timestart",  required_argument, 0, '3' },
	{ "timestop",   required_argument, 0, '4' },
	{ "contiguous", no_argument,       0, '5' },
	{ "weekdays",   required_argument, 0, '6' },
	{ "monthdays",  required_argument, 0, '7' },
	{ "localtz",    no_argument,       0, '8' },
	{ "utc",        no_argument,       0, '9' },
	{ "kerneltz",   no_argument,       0, '0' },
	{0}
};

static void time_help(void)
{
	printf(
"time match options:\n"
"    --datestart time       Start and stop time, to be given in ISO 8601\n"
"    --datestop time        (YYYY[-MM[-DD[Thh[:mm[:ss]]]]])\n"
"    --timestart time       Start and stop daytime (hh:mm[:ss])\n"
"    --timestop time        (between 00:00:00 and 23:59:59)\n"
"    --monthdays [!] value  List of days on which to match, separated by comma\n"
"                           (Possible days: 1 to 31; defaults to all)\n"
"    --weekdays [!] value   List of weekdays on which to match, sep. by comma\n"
"                           (Possible days: Mon,Tue,Wed,Thu,Fri,Sat,Sun or 1 to 7\n"
"                           Defaults to all weekdays.)\n"
"    --kerneltz             Work with the kernel timezone instead of UTC\n");
}

static void time_init(struct ebt_entry_match *m)
{
	struct xt_time_info *info = (struct xt_time_info *)m->data;

	memset(info, 0, sizeof(struct xt_time_info));
	/* By default, we match on every day, every daytime */
	info->monthdays_match = XT_TIME_ALL_MONTHDAYS;
	info->weekdays_match  = XT_TIME_ALL_WEEKDAYS;
	info->daytime_start   = XT_TIME_MIN_DAYTIME;
	info->daytime_stop    = XT_TIME_MAX_DAYTIME;

	/* ...and have no date-begin or date-end boundary */
	info->date_start = 0;
	info->date_stop  = INT_MAX;
}

static time_t time_parse_date(const char *s, uint8_t end)
{
	unsigned int month = 1, day = 1, hour = 0, minute = 0, second = 0;
	unsigned int year  = end ? 2038 : 1970;
	const char *os = s;
	struct tm tm;
	time_t ret;
	char *e;

	year = strtoul(s, &e, 10);
	if ((*e != '-' && *e != '\0') || year < 1970 || year > 2038)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	month = strtoul(s, &e, 10);
	if ((*e != '-' && *e != '\0') || month > 12)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	day = strtoul(s, &e, 10);
	if ((*e != 'T' && *e != '\0') || day > 31)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	hour = strtoul(s, &e, 10);
	if ((*e != ':' && *e != '\0') || hour > 23)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	minute = strtoul(s, &e, 10);
	if ((*e != ':' && *e != '\0') || minute > 59)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	second = strtoul(s, &e, 10);
	if (*e != '\0' || second > 59)
		goto out;

 eval:
	tm.tm_year = year - 1900;
	tm.tm_mon  = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min  = minute;
	tm.tm_sec  = second;
	tm.tm_isdst = 0;
	/*
	 * Offsetting, if any, is done by xt_time.ko,
	 * so we have to disable it here in userspace.
	 */
	setenv("TZ", "UTC", 1);
	tzset();
	ret = mktime(&tm);
	if (ret >= 0)
		return ret;
	perror("mktime");
	ebt_print_error("mktime returned an error");

 out:
	ebt_print_error("Invalid date \"%s\" specified. Should "
	           "be YYYY[-MM[-DD[Thh[:mm[:ss]]]]]", os);
	return -1;
}

static unsigned int time_parse_minutes(const char *s)
{
	unsigned int hour, minute, second = 0;
	char *e;

	hour = strtoul(s, &e, 10);
	if (*e != ':' || hour > 23)
		goto out;

	s = e + 1;
	minute = strtoul(s, &e, 10);
	if ((*e != ':' && *e != '\0') || minute > 59)
		goto out;
	if (*e == '\0')
		goto eval;

	s = e + 1;
	second = strtoul(s, &e, 10);
	if (*e != '\0' || second > 59)
		goto out;

 eval:
	return 60 * 60 * hour + 60 * minute + second;

 out:
	ebt_print_error("invalid time \"%s\" specified, "
	           "should be hh:mm[:ss] format and within the boundaries", s);
	return -1;
}

static const char *my_strseg(char *buf, unsigned int buflen,
    const char **arg, char delim)
{
	const char *sep;

	if (*arg == NULL || **arg == '\0')
		return NULL;
	sep = strchr(*arg, delim);
	if (sep == NULL) {
		snprintf(buf, buflen, "%s", *arg);
		*arg = NULL;
		return buf;
	}
	snprintf(buf, buflen, "%.*s", (unsigned int)(sep - *arg), *arg);
	*arg = sep + 1;
	return buf;
}

static uint32_t time_parse_monthdays(const char *arg)
{
	char day[3], *err = NULL;
	uint32_t ret = 0;
	unsigned int i;

	while (my_strseg(day, sizeof(day), &arg, ',') != NULL) {
		i = strtoul(day, &err, 0);
		if ((*err != ',' && *err != '\0') || i > 31)
			ebt_print_error("%s is not a valid day for --monthdays", day);
		ret |= 1 << i;
	}

	return ret;
}

static unsigned int time_parse_weekdays(const char *arg)
{
	char day[4], *err = NULL;
	unsigned int i, ret = 0;
	uint8_t valid;

	while (my_strseg(day, sizeof(day), &arg, ',') != NULL) {
		i = strtoul(day, &err, 0);
		if (*err == '\0') {
			if (i == 0)
				ebt_print_error("No, the week does NOT begin with Sunday.");

			ret |= 1 << i;
			continue;
		}

		valid = 0;
		for (i = 1; i < ARRAY_SIZE(week_days); ++i)
			if (strncmp(day, week_days[i], 2) == 0) {
				ret |= 1 << i;
				valid = 1;
			}

		if (!valid)
			ebt_print_error("%s is not a valid day specifier", day);
	}

	return ret;
}

static int time_parse(int c, char **argv, int argc, const struct ebt_u_entry *entry,
   unsigned int *flags, struct ebt_entry_match **match)
{
	struct xt_time_info *info = (struct xt_time_info *)(*match)->data;
	unsigned char inverse = 0;

	switch (c) {
	case '1':
		info->date_start = time_parse_date(optarg, 0);
		break;
	case '2':
		info->date_stop = time_parse_date(optarg, 1);
		break;
	case '3':
		info->daytime_start = time_parse_minutes(optarg);
		break;
	case '4':
		info->daytime_stop = time_parse_minutes(optarg);
		break;
	case '5':
		info->flags |= XT_TIME_CONTIGUOUS;
		break;
	case '8':
	case '0':
		info->flags |= XT_TIME_LOCAL_TZ;
		break;
	case '7':
		if (ebt_check_inverse2(optarg))
			inverse = 1;
		info->monthdays_match = time_parse_monthdays(optarg);
		if(inverse)
			info->monthdays_match ^= XT_TIME_ALL_MONTHDAYS;
		break;
	case '6':
		if (ebt_check_inverse2(optarg))
			inverse = 1;
		info->weekdays_match = time_parse_weekdays(optarg);
		if (inverse)
			info->weekdays_match ^= XT_TIME_ALL_WEEKDAYS;
		break;
	default:
		return 0;
	}
	return 1;
}

static void time_print_date(time_t date, const char *command)
{
	struct tm *t;

	/* If it is the default value, do not print it. */
	if (date == 0 || date == LONG_MAX)
		return;

	t = gmtime(&date);
	if (command != NULL)
		/*
		 * Need a contiguous string (no whitespaces), hence using
		 * the ISO 8601 "T" variant.
		 */
		printf(" %s %04u-%02u-%02uT%02u:%02u:%02u",
		       command, t->tm_year + 1900, t->tm_mon + 1,
		       t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	else
		printf(" %04u-%02u-%02u %02u:%02u:%02u",
		       t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		       t->tm_hour, t->tm_min, t->tm_sec);
}

static void time_print_monthdays(uint32_t mask, uint8_t human_readable)
{
	unsigned int i, nbdays = 0;

	printf(" ");
	for (i = 1; i <= 31; ++i)
		if (mask & (1 << i)) {
			if (nbdays++ > 0)
				printf(",");
			printf("%u", i);
			if (human_readable)
				switch (i % 10) {
					case 1:
						printf("st");
						break;
					case 2:
						printf("nd");
						break;
					case 3:
						printf("rd");
						break;
					default:
						printf("th");
						break;
				}
		}
}

static void time_print_weekdays(unsigned int mask)
{
	unsigned int i, nbdays = 0;

	printf(" ");
	for (i = 1; i <= 7; ++i)
		if (mask & (1 << i)) {
			if (nbdays > 0)
				printf(",%s", week_days[i]);
			else
				printf("%s", week_days[i]);
			++nbdays;
		}
}

static inline void divide_time(unsigned int fulltime, unsigned int *hours,
    unsigned int *minutes, unsigned int *seconds)
{
	*seconds  = fulltime % 60;
	fulltime /= 60;
	*minutes  = fulltime % 60;
	*hours    = fulltime / 60;
}

static void time_print(const struct ebt_u_entry *entry, const struct ebt_entry_match *match)
{
	const struct xt_time_info *info = (const struct xt_time_info *)match->data;
	unsigned int h, m, s;

	printf(" TIME");

	if (info->daytime_start != XT_TIME_MIN_DAYTIME ||
	    info->daytime_stop != XT_TIME_MAX_DAYTIME) {
		divide_time(info->daytime_start, &h, &m, &s);
		printf(" from %02u:%02u:%02u", h, m, s);
		divide_time(info->daytime_stop, &h, &m, &s);
		printf(" to %02u:%02u:%02u", h, m, s);
	}
	if (info->weekdays_match != XT_TIME_ALL_WEEKDAYS) {
		printf(" on");
		time_print_weekdays(info->weekdays_match);
	}
	if (info->monthdays_match != XT_TIME_ALL_MONTHDAYS) {
		printf(" on");
		time_print_monthdays(info->monthdays_match, 1);
	}
	if (info->date_start != 0) {
		printf(" starting from");
		time_print_date(info->date_start, NULL);
	}
	if (info->date_stop != INT_MAX) {
		printf(" until date");
		time_print_date(info->date_stop, NULL);
	}
	if (!(info->flags & XT_TIME_LOCAL_TZ))
		printf(" UTC");
	if (info->flags & XT_TIME_CONTIGUOUS)
		printf(" contiguous");
	printf(" ");
}

static int time_compare(const struct ebt_entry_match *m1, const struct ebt_entry_match *m2)
{
	struct xt_time_info *timeinfo1 = (struct xt_time_info *)m1->data;
	struct xt_time_info *timeinfo2 = (struct xt_time_info *)m2->data;
		
	return (memcmp(timeinfo1, timeinfo2, sizeof(struct xt_time_info)))?0:1;
}

static void time_check(const struct ebt_u_entry *entry,
   const struct ebt_entry_match *match, const char *name,
   unsigned int hookmask, unsigned int time)
{
    struct xt_time_info *timeinfo = (struct xt_time_info *)match->data;

	if (timeinfo->date_stop < timeinfo->date_start || 
		timeinfo->daytime_stop < timeinfo->daytime_start)
		ebt_print_error("stop time can't be smaller than start time");
}

static struct ebt_u_match time_match =
{
   .name          = "time",
   .size          = sizeof(struct xt_time_info),
   .help          = time_help,
   .init          = time_init,
   .parse         = time_parse,
   .final_check   = time_check,
   .print         = time_print,
   .compare       = time_compare,
   .extra_ops     = opts,
};

void _init(void)
{
	ebt_register_match(&time_match);
}
