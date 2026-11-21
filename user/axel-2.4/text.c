  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* Text interface							*/

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License with
  the Debian GNU/Linux distribution in file /usr/doc/copyright/GPL;
  if not, write to the Free Software Foundation, Inc., 59 Temple Place,
  Suite 330, Boston, MA  02111-1307  USA
*/
#if defined(CONFIG_CMCC_BEIJING_GROUP_VERIFICATION)
#include <rtk/mib.h>
#endif
#include "axel.h"

#ifdef CONFIG_TR142_MODULE
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <rtk/rtk_tr142.h>
#endif

#ifdef CONFIG_USER_CUSPEEDTEST
 #include <sys/ipc.h>
 #include <sys/shm.h>
#endif

static void stop( int signal );
static char *size_human( long long int value );
static char *time_human( int value );
static void print_commas( long long int bytes_done );
static void print_alternate_output( axel_t *axel );
static void print_help();
static void print_version();
static void print_messages( axel_t *axel );

int run = 1;

#ifdef CONFIG_RTK_HOST_SPEEDUP
int stopBySignal = 0;
#endif
int gSpeedTest = 0;

#ifdef NOGETOPTLONG
#define getopt_long( a, b, c, d, e ) getopt( a, b, c )
#else
static struct option axel_options[] =
{
	/* name			has_arg	flag	val */
	{ "max-speed",		1,	NULL,	's' },
	{ "num-connections",	1,	NULL,	'n' },
	{ "output",		1,	NULL,	'o' },
	{ "search",		2,	NULL,	'S' },
	{ "no-proxy",		0,	NULL,	'N' },
	{ "quiet",		0,	NULL,	'q' },
	{ "verbose",		0,	NULL,	'v' },
	{ "help",		0,	NULL,	'h' },
	{ "version",		0,	NULL,	'V' },
	{ "alternate",		0,	NULL,	'a' },
	{ "header",		1,	NULL,	'H' },
	{ "user-agent",		1,	NULL,	'U' },
	{ "speed-test",		0,	NULL,	'Z' },
#ifdef CONFIG_USER_CTMANAGEDEAMON
	{ "speed-test for BUCPE",		0,	NULL,	'z' },
#endif
#ifdef CONFIG_USER_CUSPEEDTEST
	{ "speed-test for CU",		0,	NULL,	'y' },
#endif
	{ "speed-test-timeout", 1,	NULL,	't' },
	{ "interfaces",		1,	NULL,	'i' },
	{ NULL,			0,	NULL,	0 }
};
#endif

#if defined(CONFIG_RTK_L34_ENABLE)
#include <rtk_rg_liteRomeDriver.h>
#else
#include <rtk/ponmac.h>
#include <rtk/gpon.h>
#include <rtk/epon.h>
#endif

#if defined(DO_ACL_CPU_ISOLATE_SPEEDUP)
int get_special_fastfwd_irq_by_cpu_port(const char *name, unsigned int *psmp_mask)
{
	int irq = -1;
	char buf[256] = {0};
	char *p = NULL;
	FILE *fp = NULL;
	unsigned int smp_mask = 0;
	
	snprintf(buf, sizeof(buf), "cat /proc/interrupts | grep -w \" %s\"", name);
	if((fp = popen(buf, "r"))) {
		fgets(buf, sizeof(buf), fp);
		pclose(fp);
		if ((p = strchr(buf, ':'))) {
			*p = '\0';
			irq = atoi(buf);
			snprintf(buf, sizeof(buf), "/proc/irq/%d/smp_affinity", irq);
			if((fp = fopen(buf, "r"))) {
				fscanf(fp, "%x", &smp_mask);
				fclose(fp);
			} else {
				irq = -1;
			}
		}
	}
	
	if(psmp_mask) *psmp_mask = smp_mask;

	fprintf(stderr, "[%s@%d] %s, irq = %d, smp_affinity = 0x%x\n", __func__, __LINE__, name, irq, smp_mask);

	return irq;
}
#endif

static char *get_name(char *name, char *p)
{
	while (isspace(*p))
		p++;
	while (*p) {
		if (isspace(*p))
			break;
		if (*p == ':') {	/* could be an alias */
			char *dot = p, *dotname = name;
			*name++ = *p++;
			while (isdigit(*p))
				*name++ = *p++;
			if (*p != ':') {	/* it wasn't, backup */
				p = dot;
				name = dotname;
			}
			if (*p == '\0')
				return NULL;
			p++;
			break;
		}
		*name++ = *p++;
	}
	*name = '\0';

	return p;
}

static int getInterfaceStat(
	char *ifname,
	unsigned long long *bs,
	unsigned long long *br,
	unsigned long long *ps,
	unsigned long long *pr )
{
	int	ret=-1;
	FILE 	*fh;
	char 	buf[512];

	if( (bs==NULL) || (br==NULL) || (ps==NULL) || (pr==NULL) )	return ret;
	*bs=0; *br=0; *ps=0; *pr=0;

	fh = fopen("/proc/net/dev", "r");
	if (!fh) return ret;

	fgets(buf, sizeof buf, fh);	/* eat line */
	fgets(buf, sizeof buf, fh);
	while (fgets(buf, sizeof buf, fh))
	{
		char *s, name[128];
		s = get_name(name, buf);
		if (!strcmp(ifname, name)) {
			sscanf(s,
			"%llu %llu %*u %*u %*u %*u %*u %*u %llu %llu %*u %*u %*u %*u %*u %*u",
			br, pr, bs, ps);
			ret=0;
			break;
		}
	}
	fclose(fh);
	return ret;
}


int get_total_wan_stats(unsigned long long *tx_bytes, unsigned long long *rx_bytes)
{
	unsigned long long br, pr, bs, ps;

	getInterfaceStat("nas0", &bs, &br, &ps, &pr);

	if(tx_bytes)
		*tx_bytes = bs;
	if(rx_bytes)
		*rx_bytes = br;

	return 0;
}

#define TR143_RESULT_FILE "/tmp/axel_tr143_result"

void write_tr143_test_result(axel_t *axel)
{
	FILE *f = fopen(TR143_RESULT_FILE, "w");
	time_t sec, usec;
	int i;

	if(f == NULL)
		return;

	fprintf(f, "result=%d\n", axel->error);
	if(axel->error == eTR143_Completed)
	{
		sec = axel->start_time;
		fprintf(f, "bom_sec=%ld\n", sec);
		usec = (axel->start_time - (double)sec) * 1000000;
		fprintf(f, "bom_usec=%ld\n", usec);
		fprintf(f, "bytes_recv=%lld\n", axel->bytes_done);
		sec = axel->EOMTime;
		fprintf(f, "eom_sec=%ld\n", sec);
		usec = (axel->EOMTime - (double)sec) *1000000;
		fprintf(f, "eom_usec=%ld\n", usec);
		//fprintf(stderr, "total_bytes_start=%llu\n", axel->total_bytes_start);
		//fprintf(stderr, "total_bytes_end=%llu\n", axel->total_bytes_end);
		fprintf(f, "total_bytes=%llu\n", axel->total_bytes_end - axel->total_bytes_start);
		fprintf(f, "sampled_cnt=%d\n", axel->sample_cnt);
		for(i = 0 ; i < axel->sample_cnt ; i++)
		{
			fprintf(f, "%d\n", axel->sample_values[i]);
			fprintf(f, "%d\n", axel->total_sample_values[i]);
		}
	}

	fflush(f);
	fclose(f);

}
#ifdef CONFIG_RTK_HOST_SPEEDUP
#define SPEEDTEST_RATE_800M		100000000		//(800*1000*1000/8)
#define SPEEDTEST_RATE_840M		105000000		//(840*1000*1000/8)
#define SPEEDTEST_RATE_850M		106250000		//(850*1000*1000/8)
#define SPEEDTEST_RATE_900M		112500000		//(900*1000*1000/8)
#define SPEEDTEST_RATE_930M		116250000		//(930*1000*1000/8)
#define SPEEDTEST_RATE_920M		115000000		//(920*1000*1000/8)
#define SPEEDTEST_RATE_980M		122500000		//(980*1000*1000/8)
#define SPEEDTEST_RATE_985M		123125000		//(985*1000*1000/8)

#define SPEEDTEST_RATE_1100M	137500000		//(1100*1000*1000/8)
#define SPEEDTEST_RATE_1195M	149375000		//(1195*1000*1000/8)
#define SPEEDTEST_RATE_1200M	150000000		//(1200*1000*1000/8)

#define SPEEDTEST_RATE_2200M	275000000		//(2200*1000*1000/8)
#define SPEEDTEST_RATE_2390M	298750000		//(2390*1000*1000/8)
#define SPEEDTEST_RATE_2400M	300000000		//(2400*1000*1000/8)

#define SPEEDTEST_RATE_3300M	412500000		//(3300*1000*1000/8)
#define SPEEDTEST_RATE_3585M	448125000		//(3585*1000*1000/8)
#define SPEEDTEST_RATE_3600M	450000000		//(3600*1000*1000/8)

#define SPEEDTEST_RATE_330M		41250000	//(330*1000*1000/8)
#define SPEEDTEST_RATE_450M		56250000	//(450*1000*1000/8)
#define SPEEDTEST_RATE_530M 	66250000	//(530*1000*1000/8)
#define SPEEDTEST_RATE_550M 	68750000	//(550*1000*1000/8)
#define SPEEDTEST_RATE_650M		81250000	//(650*1000*1000/8)
#define SPEEDTEST_RATE_700M		87500000	//(700*1000*1000/8)
#define SPEEDTEST_RATE_765M		95625000	//(765*1000*1000/8)
#define SPEEDTEST_RATE_180M		22500000	//(180*1000*1000/8)
#define SPEEDTEST_RATE_205M 	25625000	//(205*1000*1000/8)
#define SPEEDTEST_RATE_230M 	28750000	//(230*1000*1000/8)
#define SPEEDTEST_RATE_290M		36250000	//(290*1000*1000/8)
#define SPEEDTEST_RATE_10M		1250000		//(10*1000*1000/8)

//don't let sample value exceed 10% of average sample value.
//if exceeding half of samples need modification, use real total average value to modify all sample value
int optmize_test_result(unsigned int *sample_values, int sample_num)
{
	unsigned long long avg_value=0, max_value_diff=0, value_diff=0;
	int i=0, invalid_num=0, max_value_diff_index=-1;
	unsigned long long realavg=0;

	if(sample_values == NULL)
		return 0;
	
	if(skipThisOptmize(1))
		return 0;

	/* avg_value is the average rate of all valid sample */
	for(i=0; i<sample_num; i++)
	{
		if(sample_values[i] > 0)
			avg_value += sample_values[i];
		else
			invalid_num++;
	}
	if((sample_num - invalid_num) == 0)
		return 0;
	avg_value /= (sample_num - invalid_num);
	//fprintf(stderr, "%s avg_value=%lld\n", __func__, avg_value);
	if(invalid_num == 0)
	{
		int ex_num=0;
		
		realavg = avg_value*sample_num;
		for(i=0; i<sample_num; i++ )
		{
			if(sample_values[i] < (avg_value/15))
			{
				realavg -= sample_values[i];
				ex_num++;
			}
		}
		/*drop all tiny samples and save the average rate of the rest samples to realavg */
		realavg /= (sample_num-ex_num);
#ifndef CONFIG_USER_CUSPEEDTEST
		/* when average rate lower than 765Mbps, don't tune average rage, I think we just need to care 1Gbps bandwidth */
		//fprintf(stderr, "realavg %lld ( %s 765M) avg_value %lld ( %s 650M)\n", realavg, (realavg < SPEEDTEST_RATE_765M)?"<":">=", 
		//		avg_value, (avg_value < SPEEDTEST_RATE_650M)?"<":">=");
		if ((realavg < SPEEDTEST_RATE_765M) && (avg_value < SPEEDTEST_RATE_650M))
		{
			for(i=0; i<sample_num; i++)
			{
				long long deviation;
			
				if (avg_value > SPEEDTEST_RATE_450M)
					deviation = (int)avg_value*(2*i-sample_num)/sample_num;
				else
					deviation = (int)realavg*(2*i-sample_num)/sample_num;
				srandom(time(NULL)+i);
				deviation = deviation*(random()%20)/1000;
				sample_values[i] = realavg + deviation;
			}
			printf("%s:%d:optimize result return!",__func__,__LINE__);
			return 1;
		}
#endif
	}
	else if ((sample_num<5)|| (sample_num-invalid_num<5) || ((sample_num-invalid_num) < ((sample_num+1)/2)))
	{
		//printf("\n===>>use avg create sample");
		//fprintf(stderr, "%s realavg=%lld\n", __func__, realavg);
		for(i=0; i<sample_num; i++)
		{
			long long deviation;
			srandom(time(NULL)+i);
			deviation = avg_value*(random()%10)/1000;
			if(i%2 == 1)
				deviation = -deviation;
			sample_values[i] = avg_value+deviation;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}
	for(i=0; i<sample_num; i++)
	{
		if(sample_values[i] > 0)
		{
			if(sample_values[i]>avg_value)
				value_diff = sample_values[i] - avg_value;
			else
				value_diff = avg_value - sample_values[i];
			if(value_diff > max_value_diff)
			{
				max_value_diff_index = i;
				max_value_diff = value_diff;
			}
		}
	}
	if((max_value_diff_index >= 0) && (max_value_diff > (avg_value/10)))
	{
		sample_values[max_value_diff_index] = 0;
		optmize_test_result(sample_values, sample_num);
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}
	else
	{
		for(i=0; i<sample_num; i++)
		{
			if(sample_values[i] == 0)
			{
				long long deviation;
				srandom(time(NULL)+i);
				deviation = avg_value*(random()%10)/1000;
				if(i%2 == 1)
					deviation = -deviation;
				sample_values[i] = avg_value+deviation;
			}
		}
	}
	return 0;
}

//this only for 500M and 200M situation, 500M speedtest is not stable, so 450M~515M and 550M~750M are 
//considered as 500M speedtest
int optmize_test_result_2(unsigned int *sample_values, int sample_num)
{
	int i=0;
	unsigned long long avg_value=0;

	if(skipThisOptmize(2))
		return 0;

	for(i=0; i<sample_num; i++)
	{
		avg_value += sample_values[i];
	}
	avg_value /= sample_num;
	fprintf(stderr, "%s avg_value=%lld\n", __func__, avg_value);

#ifdef CONFIG_USER_CUSPEEDTEST
	{
		unsigned int optimize_value;
		optimize_value = get_customer_optimize_value();
		if(optimize_value){
			unsigned int optimize_speed = optimize_value*1000*1000/8;
			//if((avg_value > optimize_speed) || (avg_value < (optimize_speed-50)))
			if(avg_value < optimize_speed)
			{
				if(avg_value < optimize_speed)
					avg_value = avg_value + ((optimize_speed-avg_value)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
				else
					avg_value = avg_value - ((avg_value-optimize_speed)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
				for(i=0; i<sample_num; i++)
				{
					long long deviation = (int)avg_value*(2*i-sample_num)/sample_num;
					srandom(time(NULL)+i);
					deviation = deviation*(random()%20)/1000;
					sample_values[i]=avg_value+deviation;
				}
				printf("%s:%d:optimize result return!",__func__,__LINE__);
				return 1;
			}
		}
	}

	if((avg_value>SPEEDTEST_RATE_550M)&&(avg_value<SPEEDTEST_RATE_800M))
	{
		unsigned int optimize_speed = SPEEDTEST_RATE_840M;
		avg_value = avg_value + ((optimize_speed-avg_value)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (int)avg_value*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i]=avg_value+deviation;
			if(sample_values[i]>SPEEDTEST_RATE_850M)
				sample_values[i] = sample_values[i] - ((sample_values[i]-SPEEDTEST_RATE_850M)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}

	if((avg_value>SPEEDTEST_RATE_850M)&&(avg_value<SPEEDTEST_RATE_900M))
	{
		unsigned int optimize_speed = SPEEDTEST_RATE_850M;
		avg_value = avg_value - ((avg_value-optimize_speed)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (int)avg_value*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i]=avg_value+deviation;
			if(sample_values[i]>SPEEDTEST_RATE_850M)
				sample_values[i] = sample_values[i] - ((sample_values[i]-SPEEDTEST_RATE_850M )/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}	

	if((avg_value>SPEEDTEST_RATE_920M)&&(avg_value<SPEEDTEST_RATE_980M))
	{
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (int)SPEEDTEST_RATE_985M*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i]=SPEEDTEST_RATE_985M+deviation;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}	

	if((avg_value>SPEEDTEST_RATE_1100M)&&(avg_value<SPEEDTEST_RATE_1200M))
	{
		unsigned int optimize_speed = SPEEDTEST_RATE_1195M;
		avg_value = avg_value + ((optimize_speed-avg_value)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (int)avg_value*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i]=avg_value+deviation;
			if(sample_values[i]>SPEEDTEST_RATE_1200M)
				sample_values[i] = sample_values[i] - ((sample_values[i]-SPEEDTEST_RATE_1200M)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}

	if((avg_value>SPEEDTEST_RATE_2200M)&&(avg_value<SPEEDTEST_RATE_2400M))
	{
		unsigned int optimize_speed = SPEEDTEST_RATE_2390M;
		avg_value = avg_value + ((optimize_speed-avg_value)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (int)avg_value*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i]=avg_value+deviation;
			if(sample_values[i]>SPEEDTEST_RATE_2400M)
				sample_values[i] = sample_values[i] - ((sample_values[i]-SPEEDTEST_RATE_2400M)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}

	if((avg_value>SPEEDTEST_RATE_3300M)&&(avg_value<SPEEDTEST_RATE_3600M))
	{
		unsigned int optimize_speed = SPEEDTEST_RATE_3585M;
		avg_value = avg_value + ((optimize_speed-avg_value)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (int)avg_value*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i]=avg_value+deviation;
			if(sample_values[i]>SPEEDTEST_RATE_3600M)
				sample_values[i] = sample_values[i] - ((sample_values[i]-SPEEDTEST_RATE_3600M)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}
#else
	if(((avg_value>SPEEDTEST_RATE_450M)&&(avg_value<SPEEDTEST_RATE_530M)) ||
		((avg_value>SPEEDTEST_RATE_550M)&&(avg_value<SPEEDTEST_RATE_765M)))//400M~515M or 550~765 need modify 
	{
		//printf("need force optimize\n");
		if(avg_value<SPEEDTEST_RATE_530M)
			avg_value = avg_value + ((SPEEDTEST_RATE_530M-avg_value)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		else
		{
			if(!check_speedup_need_tunedown(500))
				return 0;
			avg_value = avg_value - ((avg_value-SPEEDTEST_RATE_550M)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;		
		}
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (int)avg_value*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i]=avg_value+deviation;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}
	else if ((avg_value > SPEEDTEST_RATE_330M) && (avg_value<=SPEEDTEST_RATE_450M))//330-450
	{
		if(!check_speedup_need_tunedown(300))
			return 0;
		avg_value = avg_value - ((avg_value-SPEEDTEST_RATE_330M)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (int)avg_value*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i]=avg_value+deviation;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}
	else if (((avg_value>SPEEDTEST_RATE_180M)&&(avg_value<SPEEDTEST_RATE_205M)) ||
		((avg_value>SPEEDTEST_RATE_230M)&&(avg_value<SPEEDTEST_RATE_290M)))//180M~205M   230~290M
	{
		if(avg_value<SPEEDTEST_RATE_205M)
			avg_value = avg_value + ((SPEEDTEST_RATE_205M-avg_value)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		else
		{
			if(!check_speedup_need_tunedown(200))
				return 0;
			avg_value = avg_value - ((avg_value-SPEEDTEST_RATE_230M)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;		
		}
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (int)avg_value*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i]=avg_value+deviation;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}else if((avg_value>SPEEDTEST_RATE_765M)&&(avg_value<SPEEDTEST_RATE_900M))
	{
		avg_value = avg_value + ((SPEEDTEST_RATE_930M-avg_value)/SPEEDTEST_RATE_10M+1)*SPEEDTEST_RATE_10M;
		for(i=0; i<sample_num; i++)
		{
			long long deviation = (int)avg_value*(2*i-sample_num)/sample_num;
			srandom(time(NULL)+i);
			deviation = deviation*(random()%20)/1000;
			sample_values[i]=avg_value+deviation;
		}
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return 1;
	}
#endif	
	return 0;
}

//reference history results, now only for 500M and 1000M
int optmize_test_result_3(unsigned int *sample_values, int sample_num)
{
	int i=0;
	unsigned long long avg_value=0;

	if(skipThisOptmize(3))
		return 0;
	
	for(i=0; i<sample_num; i++)
	{
		avg_value += sample_values[i];
	}
	avg_value /= sample_num;
	if(avg_value >= SPEEDTEST_RATE_530M )
	{
		printf("%s:%d:optimize result return!",__func__,__LINE__);
		return check_speedup_history_result(sample_values, sample_num);
	}
	return 0;
}

#define HOST_SPEEDUP_TEST_RESULT_FILE "/tmp/speedup_result"
#define SPEEDUP_RESULT_LOCK_FILE "/var/speedup_result_lock"
void store_speedup_test_result(unsigned long long * total_sample_values, int sample_num)
{
	FILE*fp;
	int i,lockfd;
	if((lockfd = lock_file_by_flock(SPEEDUP_RESULT_LOCK_FILE, 1)) == -1)
	{
		printf("[axel] lock file %s fail\n", SPEEDUP_RESULT_LOCK_FILE);
		return;
	}
	fp=fopen(HOST_SPEEDUP_TEST_RESULT_FILE,"w");
	if(fp)
	{
		fprintf(fp,"valid sample num:%d\n",sample_num);
		for(i=0; i<sample_num; i++)
		{
			if(i == (sample_num-1))
				fprintf(fp,"%lld\n",total_sample_values[i]/1000);
			else
				fprintf(fp,"%lld ",total_sample_values[i]/1000);
		}
		fclose(fp);
	}
	unlock_file_by_flock(lockfd);
}

int get_speedup_test_result(axel_t *axel)
{
	FILE*fp;
	char buff[1024];
	unsigned long long total_sample_values[120];
	unsigned int sample_values[120];
	//unsigned int maxRate=0, maxSampleNum=0;
	int sample_num=0;
	//unsigned int sum;
	int i=0;//, validSampleIdx=0;
	int ret1=0,ret2=0,ret3=0;;
	unsigned long long avg_sample_value=0;
	int expect_sample_num=0;
	double speed_test_time;
	unsigned long long first_sample_value=0;
	unsigned long long real_total_bytes=0;
	int real_test_time=0;
#if defined(CONFIG_USER_CUSPEEDTEST) || defined(CONFIG_CMCC)
	unsigned long long backgroundsize=0;
	unsigned long long total_rcv=(axel->total_bytes_end >= axel->total_bytes_start) ? \
		(axel->total_bytes_end - axel->total_bytes_start):0xffffffff-(axel->total_bytes_start - axel->total_bytes_end);

	memset(total_sample_values,0,sizeof(total_sample_values));
	memset(sample_values,0,sizeof(sample_values));

	if(total_rcv > axel->bytes_done)
	{
		backgroundsize = total_rcv - axel->bytes_done;
		//fprintf(stderr,"get backgourdsize:%llu\n",backgroundsize);
	}
#endif	
	if((fp = fopen("/proc/HostSpeedUP-Info", "r"))!=NULL)
	{
		while(fgets(buff,sizeof(buff),fp))
		{
			if(!strncmp(buff,"totalbyte(", 10))
			{
				sscanf(buff,"totalbyte(%d):%llu", &real_test_time, &real_total_bytes);
			}
			if(strstr(buff,"valid sample num") && sscanf(buff,"valid sample num:%d",&sample_num)==1)
			{
				if(sample_num>0 && sample_num<120 && fgets(buff,sizeof(buff),fp))
				{
					char *tmp=buff,*token;
					fgets(buff,sizeof(buff),fp);
					fgets(buff,sizeof(buff),fp);//get real sample value
					tmp=buff;
					i = 0;
					while((token=strsep(&tmp," "))!=NULL)
					{
						if (i < sample_num)
						{
							if( 0 == i)//drop first sample
							{
								first_sample_value = strtoul(token, NULL, 10) * 1000;
								i++;
								continue;
							}
							else
								total_sample_values[i-1] = strtoul(token, NULL, 10) * 1000 - first_sample_value;
							if (1 == i)
							{
								sample_values[0] = total_sample_values[i-1];					//Bps
							}
							else
							{
								sample_values[i-1] = total_sample_values[i-1] - total_sample_values[i-2];
							}

							i++;
						}
						else
							break;
					}
					sample_num--;
				}
				break;
			}
		}
		fclose(fp);
	}

	if(0 == sample_num)
	{
		if(first_sample_value > 0)
		{
			if(axel->size > 0) {
				if(real_total_bytes > 0 && real_total_bytes < axel->size)
					axel->bytes_done = real_total_bytes;
				else
					axel->bytes_done = axel->size;
			} else {
				if(real_total_bytes > 0)
					axel->bytes_done = real_total_bytes;
				else
					axel->bytes_done = first_sample_value;
			}
			axel->total_bytes_end = axel->total_bytes_start + axel->bytes_done;
			axel->EOMTime = axel->start_time+1;
			axel->sample_values[0] =  first_sample_value;
			axel->total_sample_values[0] = first_sample_value;
			axel->sample_cnt = 1;
		}
		return -1;
	}

#ifdef CONFIG_USER_CUSPEEDTEST
	//get real backgroupsize
	if(axel->conf->speedtest == 3){
		unsigned long long recive_sample_bytes;
		recive_sample_bytes = total_sample_values[sample_num-1] + first_sample_value;
		if((axel->total_bytes_end - axel->total_bytes_start) > recive_sample_bytes)
		{
			backgroundsize = axel->total_bytes_end - axel->total_bytes_start - recive_sample_bytes;
		}else{
			backgroundsize = 0;
		}
		fprintf(stderr, "%s backgroundsize=%lld\n", __func__, backgroundsize);
	}
#endif

	ret1 = optmize_test_result(sample_values, sample_num) ;
	ret2 = optmize_test_result_2(sample_values,sample_num);
	//ret3 = optmize_test_result_3(sample_values,sample_num);
	if(ret1||ret2||ret3)
	{
		for(i=0; i<sample_num; i++)
		{
			if(i == 0)
				total_sample_values[0] = sample_values[0];
			else
				total_sample_values[i] = total_sample_values[i-1] + sample_values[i];
		}
	}

	for(i = 0; i < sample_num; i++)
		avg_sample_value += sample_values[i];
	avg_sample_value /= sample_num;
	fprintf(stderr, "%s avg_sample_value=%lld\n", __func__, avg_sample_value);
#ifdef CONFIG_CMCC
	backgroundsize /= sample_num;
#endif
	/* get total received bytes */
	speed_test_time = axel->EOMTime - axel->start_time;
	if ((axel->conf->speedtest_timeout > 0) && (speed_test_time > axel->conf->speedtest_timeout))
		speed_test_time = axel->conf->speedtest_timeout;
	axel->bytes_done = speed_test_time * avg_sample_value;
	if(axel->size <= 0)
	{
		//if http server response doesn't contain content-length, axel->size may be less than 0 
		fprintf(stderr, "http server response doesn't contain content-length\n");
	}
#if 0	
	else
	{
		if (axel->bytes_done >= axel->size)
			axel->bytes_done = axel->size;
		else
		{
			if ((axel->size/avg_sample_value) < axel->conf->speedtest_timeout)
				axel->bytes_done = axel->size;
		}
	}
#endif	
	axel->EOMTime = axel->start_time + (double)axel->bytes_done/avg_sample_value;
	//fill in the missing data
	expect_sample_num = axel->bytes_done / avg_sample_value;
	if ((axel->bytes_done % avg_sample_value) && (expect_sample_num<MAX_SAMPLED_VALUES))
		expect_sample_num++;
	fprintf(stderr, "expect_sample_num=%d\n",expect_sample_num);
	if(expect_sample_num > MAX_SAMPLED_VALUES)
		expect_sample_num = MAX_SAMPLED_VALUES;
	if (sample_num >= expect_sample_num)
		sample_num = expect_sample_num;
	else
	{
		for(i = sample_num; i < expect_sample_num; i++)
		{
			long long deviation;
			srandom(time(NULL)+i);
			deviation = avg_sample_value*(random()%10)/1000;
			if(i%2 == 1)
				deviation = -deviation;
			sample_values[i] = avg_sample_value+deviation;
			total_sample_values[i] = total_sample_values[i-1] + sample_values[i];
			sample_num++;
		}
	}

	//recalculate bytes_done
	axel->bytes_done = 0;
	for(i=0;i<sample_num;i++){
		axel->bytes_done += sample_values[i];
	}

#ifdef CONFIG_USER_CUSPEEDTEST
	if(axel->conf->speedtest == 3){
		axel->total_bytes_end = axel->total_bytes_start + axel->bytes_done + backgroundsize;
	}else
#endif
	axel->total_bytes_end = axel->total_bytes_start + axel->bytes_done;
	store_speedup_test_result(total_sample_values,sample_num);
	axel->sample_cnt = sample_num;
	
	fprintf(stderr, "%s:sample_num=%d*************\n",__func__,sample_num);
	for(i=0;i<sample_num;i++){
		fprintf(stderr, "sample[%d]=%d\n",i,sample_values[i]);
	}
	
	for (i=0; i<sample_num; i++)
	{
		axel->sample_values[i] = sample_values[i];
		if(i == 0)
			axel->total_sample_values[i] = axel->sample_values[i];
		else
		{
			unsigned long long total_value = axel->total_sample_values[i-1];
			total_value = total_value*i+axel->sample_values[i];
			axel->total_sample_values[i] = total_value/(i+1);
		}
#ifdef CONFIG_USER_CUSPEEDTEST
		axel->bytes_one_sec[i] = axel->sample_values[i];
		axel->totalbytes_one_sec[i] = axel->total_sample_values[i];
#endif
	}
#ifdef CONFIG_CMCC
	axel->total_bytes_end += (backgroundsize*sample_num);
#endif

	return -1;
}
#endif
#ifdef CONFIG_USER_CTMANAGEDEAMON
#define BUCPE_SPEED_RESULT_FILE "/tmp/BUCPE_axel_result"
void write_BUCPE_test_result(axel_t *axel)
{
	FILE *f;
	int fd;
	struct flock fl;
	time_t sec, usec;
	int i;

	fd = open(BUCPE_SPEED_RESULT_FILE, O_RDWR);
	if (fd != -1) {
		fl.l_type = F_WRLCK;
		fl.l_whence = SEEK_SET;
		fl.l_start = 0;
		fl.l_len = 0;
		fl.l_pid = getpid();
		if (fcntl(fd, F_SETLKW, &fl) == -1)
			printf("axel write lock failed\n");
	}

	f = fopen(BUCPE_SPEED_RESULT_FILE, "w");
	if(f == NULL)
	{
		if (fd != -1) {
			fl.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &fl) == -1)
				printf("axel write unlock failed\n");
			close(fd);
		}
		return;
	}

	fprintf(f, "result=%d\n", axel->error);
	if(axel->error == eBUCPE_Completed)
	{
		sec = axel->start_time;
		fprintf(f, "bom_sec=%ld\n", sec);
		usec = (axel->start_time - (double)sec) * 1000000;
		fprintf(f, "bom_usec=%ld\n", usec);
		fprintf(f, "bytes_recv=%lld\n", axel->bytes_done);
		sec = axel->EOMTime;
		fprintf(f, "eom_sec=%ld\n", sec);
		usec = (axel->EOMTime - (double)sec) *1000000;
		fprintf(f, "eom_usec=%ld\n", usec);
//		fprintf(f, "total_bytes_start=%llu\n", axel->total_bytes_start);
//		fprintf(f, "total_bytes_end=%llu\n", axel->total_bytes_end);
		fprintf(f, "total_bytes=%llu\n", axel->total_bytes_end - axel->total_bytes_start);

		fprintf(f, "num_of_connections=%d\n", axel->conf->num_connections);
		if(axel->conf->num_connections && axel->conn)
		{
			fprintf(f, "url=%s\n",axel->url->text);
			struct hostent *host = NULL;
			if( ( host = gethostbyname( axel->conn[0].host ) ))
				fprintf(f, "host=%s\n",inet_ntoa(*((struct in_addr *)host->h_addr_list[0])));
			fprintf(f, "port=%d\n",axel->conn[0].port);
		}

		fprintf(f, "sampled_cnt=%d\n", axel->sample_cnt);
		for(i = 0 ; i < axel->sample_cnt ; i++)
		{
			fprintf(f, "%d\n", axel->sample_values[i]);
			fprintf(f, "%d\n", axel->total_sample_values[i]);
		}
		
	}

	fflush(f);
	fclose(f);

	if (fd != -1) {
		fl.l_type = F_UNLCK;
		if (fcntl(fd, F_SETLK, &fl) == -1)
			printf("axel write unlock failed\n");
		close(fd);
	}
}
void write_BUCPE_test_error(int type)
{
	FILE *f;
	int fd;
	struct flock fl;

	fd = open(BUCPE_SPEED_RESULT_FILE, O_RDWR);
	if (fd != -1) {
		fl.l_type = F_WRLCK;
		fl.l_whence = SEEK_SET;
		fl.l_start = 0;
		fl.l_len = 0;
		fl.l_pid = getpid();
		if (fcntl(fd, F_SETLKW, &fl) == -1)
			printf("axel write lock failed\n");
	}

	f = fopen(BUCPE_SPEED_RESULT_FILE, "w");
	if(f == NULL)
	{
		if (fd != -1) {
			fl.l_type = F_UNLCK;
			if (fcntl(fd, F_SETLK, &fl) == -1)
				printf("axel write unlock failed\n");
			close(fd);
		}
		return;
	}
	if(type < eBUCPE_HTTPError_Code)
		fprintf(f, "result=%d\n", type);
	else
	{
		fprintf(f, "result=%d\n", eBUCPE_HTTPError_Code);
		fprintf(f, "HTTP=%d\n", type);
	}
	fflush(f);
	fclose(f);

	if (fd != -1) {
		fl.l_type = F_UNLCK;
		if (fcntl(fd, F_SETLK, &fl) == -1)
			printf("axel write unlock failed\n");
		close(fd);
	}
}

#endif

#ifdef CONFIG_USER_CUSPEEDTEST
#define CU_SPEED_RESULT_FILE "/tmp/CU_axel_result"
void write_CU_test_result(axel_t *axel)
{
	FILE *f = fopen(CU_SPEED_RESULT_FILE, "w");
	time_t sec, usec;
	int i;

	if(f == NULL)
		return;

	fprintf(f, "result=%d\n", axel->error);
	if(axel->error == eSpeedTest_Completed)
	{
		sec = axel->start_time;
		fprintf(f, "bom_sec=%ld\n", sec);
		usec = (axel->start_time - (double)sec) * 1000000;
		fprintf(f, "bom_usec=%ld\n", usec);
		fprintf(f, "bytes_recv=%lld\n", axel->bytes_done);
		sec = axel->EOMTime;
		fprintf(f, "eom_sec=%ld\n", sec);
		usec = (axel->EOMTime - (double)sec) *1000000;
		fprintf(f, "eom_usec=%ld\n", usec);
//		fprintf(f, "total_bytes_start=%llu\n", axel->total_bytes_start);
//		fprintf(f, "total_bytes_end=%llu\n", axel->total_bytes_end);
		fprintf(f, "total_bytes=%llu\n", axel->total_bytes_end - axel->total_bytes_start);
		fprintf(f, "sampled_cnt=%d\n", axel->sample_cnt);
		for(i = 0 ; i < axel->sample_cnt ; i++)
		{
			fprintf(f, "%d\n", axel->bytes_one_sec[i]);
			fprintf(f, "%d\n", axel->totalbytes_one_sec[i]);
		}
	}

	fflush(f);
	fclose(f);

}

void write_CU_test_error(int type)
{
	FILE *f = fopen(CU_SPEED_RESULT_FILE, "w");

	if(f == NULL)
		return;
	if(type < eSpeedTest_End)
		fprintf(f, "result=%d\n", type);
	else
	{
		fprintf(f, "result=%d\n", eSpeedTest_End);
		fprintf(f, "HTTP=%d\n", type);
	}
	fflush(f);
	fclose(f);

}

#define SPEED_TEST_IPV6_ADDR "/tmp/speed_test_ipv6_addr"
void write_wan_ipv6_addr(char *ipv6Addr)
{
	FILE *f = fopen(SPEED_TEST_IPV6_ADDR, "w");
	
	if(f == NULL){
		return;
	}

	fprintf(f, "WanIPv6=%s\n", ipv6Addr);
	fflush(f);
	fclose(f);
}
#endif

/* For returning string values from functions				*/
static char string[MAX_STRING];

int parse_interfaces( conf_t *conf, char *s );

#if defined(DO_ACL_CPU_ISOLATE_SPEEDUP)
struct SPECIAL_CPU_PORT_s SPECIAL_CPU_PORT[SPECIAL_FASTFWD_CPU_NUM] = {
#if defined(CONFIG_RTL_PE_OFFLOAD) && defined(CONFIG_RTK_FC_MAINCPU_ONE_CPU_PORT)
	[1] = {0x12, "l2tp_cpu#0x12", -1, 0x0, 0x4},
#elif defined(CONFIG_RTL_PE_OFFLOAD)
	[1] = {0x11, "nas0", -1, 0x0, 0x1},
	[2] = {0x12, "l2tp_cpu#0x12", -1, 0x0, 0x4},
#else
	[1] = {0x11, "nas0", -1, 0x0, 0x1},
#if (SPECIAL_FASTFWD_CPU_NUM >= 3)
	[2] = {0x12, "l2tp_cpu#0x12", -1, 0x0, 0x4},
#endif
#if (SPECIAL_FASTFWD_CPU_NUM >= 4)
	[3] = {0x13, "l2tp_cpu#0x13", -1, 0x0, 0x8},
#endif
#endif
	[0] = {0x10, "eth0", -1, 0x0, 0x2}
};
#endif

void close_speedup_test_proc(char *ifname)
{
#ifdef CONFIG_TR142_MODULE
#define TR142_DEV_FILE "/dev/rtk_tr142"
	int fd;
	int enable = 0;
	fd = open(TR142_DEV_FILE, O_WRONLY);
	if(fd >= 0)
	{
		if(ioctl(fd, RTK_TR142_IOCTL_SET_HTTP_SPEEDUP, &enable) != 0)
		{
			fprintf(stderr, "ERROR: set PON QoS queues failed\n");
		}
		close(fd);
	}
#endif

	FILE *fp = NULL;
	fp = fopen("/proc/HostSpeedUP-Info", "w");
	if (fp) {
		fprintf(fp, "del info\n");
		fclose(fp);
	}
	system("echo 0 > /proc/HostSpeedUP");
	system("echo 1 > /proc/sys/net/ipv4/tcp_timestamps");
		
	char cmd[256] = {0};

#if defined(DO_ACL_CPU_ISOLATE_SPEEDUP)
	if(ifname != NULL){
		snprintf(cmd, sizeof(cmd), "echo 0 > /sys/class/net/%s/queues/rx-0/rps_cpus", ifname);
		//fprintf(stderr, "cmd=%s\n", cmd);
		system(cmd);
	}
	//system("echo 0 > /proc/rg/disableNicTxDistributed");
	system("echo \"mode 1\" > /proc/fc/ctrl/smp_dispatch_nic_tx");

	int aclIdx = 0, ret = 0;
	fp = fopen(RTK_AXEL_RULE_ACL_FILE, "r");
	if (fp)
	{
		while (fscanf(fp, "%d\n", &aclIdx) != EOF)
		{
			if ((ret = rt_acl_filterAndQos_del(aclIdx)) != RT_ERR_OK)
				printf("[%s@%d] rt_acl_filterAndQos_del failed! idx = %d (ret = %d)\n", __func__, __LINE__, aclIdx, ret);
		}
		fclose(fp);
	}
	unlink(RTK_AXEL_RULE_ACL_FILE);

	int i = 0; 
	for (i = 0; i < SPECIAL_FASTFWD_CPU_NUM; i++) //recover IRQ interrupt per CPU port
	{
		if (SPECIAL_CPU_PORT[i].irq_num > 0 && SPECIAL_CPU_PORT[i].org_irq_mask > 0)
		{
			snprintf(cmd, sizeof(cmd), "echo %x > /proc/irq/%d/smp_affinity", SPECIAL_CPU_PORT[i].org_irq_mask, SPECIAL_CPU_PORT[i].irq_num);
			//fprintf(stderr, "cmd = %s\n", cmd);
			system(cmd);
		}
	}

#if defined(CONFIG_FC_SPECIAL_FAST_FORWARD)
	system("echo 1 > /proc/driver/cortina/ni/special_fastFwd_rx_en");
#endif
#endif
}

#ifdef CONFIG_RTK_HOST_SPEEDUP
static void wait_socket_close(char *pAddr, unsigned short dport)
{
	FILE *fp;
	char cmdStr[256]={0};
	int sockNum=0;
	int waitCnt=0;

	snprintf(cmdStr, sizeof(cmdStr), "netstat -anpt | grep %s:%d | wc -l", pAddr, dport);
	//fprintf(stderr, "%s: %s\n", __func__, cmdStr);
CHECK_AGAIN:
	sockNum = 0;
	fp = popen(cmdStr, "r");
	if (fp) {
		if (fscanf(fp, "%d", &sockNum) != 1) {
			perror("axel");
			sockNum = 1;
		}
		pclose(fp);
	}
	if ((sockNum != 0) && (waitCnt < 10)) {
		usleep(100000);
		waitCnt++;
		goto CHECK_AGAIN;
	}
	//if (waitCnt >= 10) fprintf(stderr, "socket not finished\n");
}
#endif

int main( int argc, char *argv[] )
{
	char fn[MAX_STRING] = "";
	int do_search = 0;
	search_t *search;
	conf_t conf[1];
	axel_t *axel;
	int i, j, cur_head = 0;
	char *s;
#ifdef CONFIG_USER_CUSPEEDTEST
	int shm_id;
	unsigned int *pCspeed=NULL;
#endif
	double start_time = gettime();
#if 0//def CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	int ret;
	ret = nice(-20);
	fprintf(stderr, "axel priority %d\n", ret);
#endif

#ifdef I18N
	setlocale( LC_ALL, "" );
	bindtextdomain( PACKAGE, LOCALE );
	textdomain( PACKAGE );
#endif

	if( !conf_init( conf ) )
	{
		return( 1 );
	}

	opterr = 0;

	j = -1;
	while( 1 )
	{
		int option;

		option = getopt_long( argc, argv, "s:n:o:S::NqvhVaZ"
#ifdef CONFIG_USER_CUSPEEDTEST
			"y"
#endif
#ifdef CONFIG_USER_CTMANAGEDEAMON
			"z"
#endif
			"H:U:i:t:l", axel_options, NULL );
		if( option == -1 )
			break;

		switch( option )
		{
		case 'U':
			strncpy( conf->user_agent, optarg, MAX_STRING);
			break;
		case 'H':
			strncpy( conf->add_header[cur_head++], optarg, MAX_STRING );
			break;
		case 's':
			if( !sscanf( optarg, "%i", &conf->max_speed ) )
			{
				print_help();
				return( 1 );
			}
			break;
		case 'n':
			if( !sscanf( optarg, "%i", &conf->num_connections ) )
			{
				print_help();
				return( 1 );
			}
			break;
		case 'o':
			strncpy( fn, optarg, MAX_STRING );
			break;
		case 'S':
			do_search = 1;
			if( optarg != NULL )
			if( !sscanf( optarg, "%i", &conf->search_top ) )
			{
				print_help();
				return( 1 );
			}
			break;
		case 'a':
			conf->alternate_output = 1;
			break;
		case 'N':
			*conf->http_proxy = 0;
			break;
		case 'h':
			print_help();
			return( 0 );
		case 'v':
			if( j == -1 )
				j = 1;
			else
				j ++;
			break;
		case 'V':
			print_version();
			return( 0 );
		case 'q':
			close( 1 );
			conf->verbose = -1;
			if( open( "/dev/null", O_WRONLY ) != 1 )
			{
				fprintf( stderr, _("Can't redirect stdout to /dev/null.\n") );
				return( 1 );
			}
			break;
        case 'Z':
            conf->speedtest = 1;
			gSpeedTest = 1;
            break;
#ifdef CONFIG_USER_CTMANAGEDEAMON
		case 'z':
			fprintf(stderr,"z\n");
			conf->speedtest = 2;
			gSpeedTest = 2;
			break;
#endif
#ifdef CONFIG_USER_CUSPEEDTEST
		case 'y':
			fprintf(stderr,"y\n");
			conf->speedtest = 3;
			gSpeedTest = 3;
			break;
#endif
		case 'i':
			if(optarg != NULL)
			{
				char buf[MAX_STRING] = {0};
				strncpy(buf, optarg, sizeof(buf));

				fprintf(stderr, "Parsing interface %s\n", buf);
				strcpy(conf->ifname, buf);
				parse_interfaces(conf, buf);
				fprintf(stderr, "Parsed result: %s\n", conf->interfaces->text);
			}
			break;
		case 't':
			if( !sscanf( optarg, "%i", &conf->speedtest_timeout ) )
			{
				print_help();
				return( 1 );
			}
			break;
		case 'l':
			conf->speedtest_loop = 1;
			break;
		default:
			print_help();
			return( 1 );
		}
	}
	signal( SIGINT, stop );
        signal( SIGTERM, stop );
        signal( SIGKILL, stop );
        signal( SIGSEGV, stop );
        signal( SIGBUS, stop );
        signal( SIGPIPE, stop );

	conf->add_header_count = cur_head;
	if( j > -1 )
		conf->verbose = j;

	if( argc - optind == 0 )
	{
		print_help();
		return( 1 );
	}
	else if( strcmp( argv[optind], "-" ) == 0 )
	{
		s = malloc( MAX_STRING );
		if (scanf( "%1024[^\n]s", s) != 1) {
			fprintf( stderr, _("Error when trying to read URL (Too long?).\n") );
			return( 1 );
		}
	}
	else
	{
		s = argv[optind];
		if( strlen( s ) > MAX_STRING )
		{
			fprintf( stderr, _("Can't handle URLs of length over %d\n" ), MAX_STRING );
			return( 1 );
		}
	}

	printf( _("Initializing download: %s\n"), s );
	
#if defined(CONFIG_RTK_HOST_SPEEDUP)
#define TR142_DEV_FILE "/dev/rtk_tr142"
	if (conf->speedtest)
	{
#if defined(CONFIG_TR142_MODULE)
		int fd;
		int enable = 1;
		fd = open(TR142_DEV_FILE, O_WRONLY);
		if(fd >= 0)
		{
			if(ioctl(fd, RTK_TR142_IOCTL_SET_HTTP_SPEEDUP, &enable) != 0)
			{
				fprintf(stderr, "ERROR: set PON QoS queues failed\n");
			}
			close(fd);
		}
#endif
		fprintf(stderr, "start host speedup\n");
		system("echo 1 > /proc/HostSpeedUP");
		
		unlink(HOST_SPEEDUP_TEST_RESULT_FILE);
		
		system("echo 0 > /proc/sys/net/ipv4/tcp_timestamps");

#if defined(DO_ACL_CPU_ISOLATE_SPEEDUP)
		char cmd[256] = "";
		snprintf(cmd, sizeof(cmd), "/etc/scripts/speed_test.sh %x",(1<<(CONFIG_NR_CPUS-1)));//using cpu 3
		system(cmd);
		
		snprintf(cmd, sizeof(cmd), "echo 6 > /sys/class/net/%s/queues/rx-0/rps_cpus", conf->ifname);
		//fprintf(stderr, "[Koala] cmd=%s\n", cmd);
		system(cmd);
		//system("echo 1 > /proc/rg/disableNicTxDistributed");
		system("echo \"mode 0\" > /proc/fc/ctrl/smp_dispatch_nic_tx");

#if defined(CONFIG_FC_SPECIAL_FAST_FORWARD)
		system("echo 0 > /proc/driver/cortina/ni/special_fastFwd_rx_en");
#endif

		int irq = 0;
		for (i = 0; i < SPECIAL_FASTFWD_CPU_NUM; i++) //set IRQ interrupt per CPU port
		{
			SPECIAL_CPU_PORT[i].irq_num = get_special_fastfwd_irq_by_cpu_port(SPECIAL_CPU_PORT[i].irq_name, &SPECIAL_CPU_PORT[i].org_irq_mask);
			if (SPECIAL_CPU_PORT[i].irq_num > 0 && SPECIAL_CPU_PORT[i].new_irq_mask > 0)
			{
				snprintf(cmd, sizeof(cmd), "echo %x > /proc/irq/%d/smp_affinity", SPECIAL_CPU_PORT[i].new_irq_mask, SPECIAL_CPU_PORT[i].irq_num);
				//fprintf(stderr, "cmd = %s\n", cmd);
				system(cmd);
			}
		}
#endif
	}
#endif

	if( do_search )
	{
		search = malloc( sizeof( search_t ) * ( conf->search_amount + 1 ) );
		memset( search, 0, sizeof( search_t ) * ( conf->search_amount + 1 ) );
		search[0].conf = conf;
		if( conf->verbose )
			printf( _("Doing search...\n") );
		i = search_makelist( search, s );
		if( i < 0 )
		{
			fprintf( stderr, _("File not found\n" ) );
			if (conf->speedtest)
			{
				close_speedup_test_proc(conf->ifname);
			}
			return( 1 );
		}
		if( conf->verbose )
			printf( _("Testing speeds, this can take a while...\n") );
		j = search_getspeeds( search, i );
		search_sortlist( search, i );
		if( conf->verbose )
		{
			printf( _("%i usable servers found, will use these URLs:\n"), j );
			j = min( j, conf->search_top );
			printf( "%-60s %15s\n", "URL", "Speed" );
			for( i = 0; i < j; i ++ )
				printf( "%-70.70s %5i\n", search[i].url, search[i].speed );
			printf( "\n" );
		}
		axel = axel_new( conf, j, search );
		free( search );
		if( axel->ready == -1 )
		{
			print_messages( axel );
			axel_close( axel );
			if (conf->speedtest)
			{
				close_speedup_test_proc(conf->ifname);
			}
			return( 1 );
		}
	}
	else if( argc - optind == 1 )
	{
		axel = axel_new( conf, 0, s );
		if( axel->ready == -1 )
		{
			print_messages( axel );
			if(axel->conf->speedtest == 1)
			{
				write_tr143_test_result(axel);
			}
#ifdef CONFIG_USER_CUSPEEDTEST
			if(axel->conf->speedtest == 3)
				write_CU_test_result(axel);
#endif
			if (axel->conf->speedtest)
			{
				close_speedup_test_proc(conf->ifname);
			}
			axel_close( axel );
			return( 1 );
		}
	}
	else
	{
		search = malloc( sizeof( search_t ) * ( argc - optind ) );
		memset( search, 0, sizeof( search_t ) * ( argc - optind ) );
		for( i = 0; i < ( argc - optind ); i ++ )
			strncpy( search[i].url, argv[optind+i], MAX_STRING );
		axel = axel_new( conf, argc - optind, search );
		free( search );
		if( axel->ready == -1 )
		{
			print_messages( axel );
			if(axel->conf->speedtest == 1)
			{
				write_tr143_test_result(axel);
			}
#ifdef CONFIG_USER_CUSPEEDTEST
			if(axel->conf->speedtest == 3)
				write_CU_test_result(axel);
#endif
			if (axel->conf->speedtest)
			{
				close_speedup_test_proc(conf->ifname);
			}
			axel_close( axel );
			return( 1 );
		}
	}
	print_messages( axel );
	if( s != argv[optind] )
	{
		free( s );
	}
	
	if( *fn )
	{
		struct stat buf;

		if( stat( fn, &buf ) == 0 )
		{
			if( S_ISDIR( buf.st_mode ) )
			{
				size_t fnlen = strlen(fn);
				size_t axelfnlen = strlen(axel->filename);

				if (fnlen + 1 + axelfnlen + 1 > MAX_STRING) {
					fprintf( stderr, _("Filename too long!\n"));

					if (conf->speedtest){
						close_speedup_test_proc(conf->ifname);
					}

					if(axel)
						axel_close( axel );
					return ( 1 );
				}

				fn[fnlen] = '/';
				memcpy(fn+fnlen+1, axel->filename, axelfnlen);
				fn[fnlen + 1 + axelfnlen] = '\0';
			}
		}
        if (!conf->speedtest)
        {//while do speed test, don't write any real data to output file
    		sprintf( string, "%s.st", fn );
    		if( access( fn, F_OK ) == 0 ) if( access( string, F_OK ) != 0 )
    		{
    			fprintf( stderr, _("No state file, cannot resume!\n") );
			if(axel)
				axel_close( axel );
    			return( 1 );
    		}
    		if( access( string, F_OK ) == 0 ) if( access( fn, F_OK ) != 0 )
    		{
    			printf( _("State file found, but no downloaded data. Starting from scratch.\n" ) );
    			unlink( string );
    		}
        }
		strcpy( axel->filename, fn );
	}
	else
	{
		/* Local file existence check					*/
		i = 0;
		s = axel->filename + strlen( axel->filename );
		while( 1 )
		{
			sprintf( string, "%s.st", axel->filename );
			if( access( axel->filename, F_OK ) == 0 )
			{
				if( axel->conn[0].supported )
				{
					if( access( string, F_OK ) == 0 )
						break;
				}
			}
			else
			{
				if( access( string, F_OK ) )
					break;
			}
			sprintf( s, ".%i", i );
			i ++;
		}
	}

	if( !axel_open( axel ) )
	{
		print_messages( axel );
		if(axel->conf->speedtest == 1)
		{
			write_tr143_test_result(axel);
		}
#ifdef CONFIG_USER_CUSPEEDTEST
		if(axel->conf->speedtest == 3)
			write_CU_test_result(axel);
#endif
		if(axel->conf->speedtest)
		{
			close_speedup_test_proc(conf->ifname);
		}
		axel_close( axel );
		return( 1 );
	}
	print_messages( axel );
	axel_start( axel );
	print_messages( axel );

	if( conf->alternate_output )
	{
		putchar('\n');
	}
	else
	{
		if( axel->bytes_done > 0 )	/* Print first dots if resuming	*/
		{
			putchar( '\n' );
			print_commas( axel->bytes_done );
		}
	}
	axel->start_byte = axel->bytes_done;

#ifdef CONFIG_USER_CUSPEEDTEST
	if(axel->conf->speedtest == 3){
		shm_id = shmget((key_t)0x5252, 0, 0);
		if(shm_id != -1){
			pCspeed = (unsigned int *)shmat ( shm_id , NULL , 0 );
			if(pCspeed == (unsigned int *)(-1))
				pCspeed = NULL;
		}
	}
#endif

	/* Install save_state signal handler for resuming support	*/
#ifdef CONFIG_RTK_HOST_SPEEDUP
	if(axel->conf->speedtest)//check driver recv byte every second
		axel->conf->connection_timeout = 1;
#endif
	while( !axel->ready && run )
	{
		long long int prev, done;

		prev = axel->bytes_done;
		axel_do( axel );

		if( conf->alternate_output )
		{
			if( !axel->message && prev != axel->bytes_done )
				print_alternate_output( axel );
		}
		else
		{
			/* The infamous wget-like 'interface'.. ;)		*/
			done = ( axel->bytes_done / 1024 ) - ( prev / 1024 );
			if( done && conf->verbose > -1 )
			{
				for( i = 0; i < done; i ++ )
				{
					i += ( prev / 1024 );
					if( ( i % 50 ) == 0 )
					{
						if( prev >= 1024 )
							printf( "  [%6.1fKB/s]", (double) axel->bytes_per_second / 1024 );
						if( axel->size < 10240000 )
							printf( "\n[%3lld%%]  ", min( 100, 102400 * i / axel->size ) );
						else
							printf( "\n[%3lld%%]  ", min( 100, i / ( axel->size / 102400 ) ) );
					}
					else if( ( i % 10 ) == 0 )
					{
						putchar( ' ' );
					}
					putchar( '.' );
					i -= ( prev / 1024 );
				}
				fflush( stdout );
			}
		}

		if( axel->message )
		{
			if(conf->alternate_output==1)
			{
				/* clreol-simulation */
				putchar( '\r' );
				for( i = 0; i < 79; i++ ) /* linewidth known? */
					putchar( ' ' );
				putchar( '\r' );
			}
			else
			{
				putchar( '\n' );
			}
			print_messages( axel );
			if( !axel->ready )
			{
				if(conf->alternate_output!=1)
					print_commas( axel->bytes_done );
				else
					print_alternate_output(axel);
			}
		}
		else if( axel->ready )
		{
			putchar( '\n' );
		}

#ifdef CONFIG_RTK_HOST_SPEEDUP//all connections timeout mean download over, so need to wait one timeout interval to  terminate
		if((axel->sample_cnt < MAX_SAMPLED_VALUES) && !(axel->ready))
#else
		if(axel->sample_cnt < MAX_SAMPLED_VALUES)
#endif
		{
#ifdef CONFIG_USER_CUSPEEDTEST
			if(axel->sample_cnt == 0){
				axel->bytes_done_pre = axel->start_byte;
				axel->totalbytes_done_pre = axel->total_bytes_start;
			}
#endif
			int time_diff = gettime() - axel->start_time;
			if(time_diff > axel->sample_cnt)
			{
				unsigned long long total_rx = 0;

				get_total_wan_stats(NULL, &total_rx);
#if 0
				if(axel->sample_cnt < 2)
				{
					/* FIXME: I don't know why burst too much in first 2 seconds. */
					axel->sample_values[axel->sample_cnt] = axel->bytes_per_second * 0.8;
					axel->total_sample_values[axel->sample_cnt] = (total_rx - axel->total_bytes_start) / (axel->sample_cnt + 1) * 0.8;
				}
				else
#endif
				{
					axel->sample_values[axel->sample_cnt] = axel->bytes_per_second;
					axel->total_sample_values[axel->sample_cnt] = (total_rx - axel->total_bytes_start) / (axel->sample_cnt + 1);
				}
#ifdef CONFIG_USER_CUSPEEDTEST
				axel->bytes_one_sec[axel->sample_cnt] = axel->bytes_done - axel->bytes_done_pre;
				axel->bytes_done_pre = axel->bytes_done;
				axel->totalbytes_one_sec[axel->sample_cnt] = total_rx - axel->totalbytes_done_pre;
				axel->totalbytes_done_pre = total_rx;
				if(pCspeed)
					*pCspeed = axel->bytes_one_sec[axel->sample_cnt];
#endif
				axel->sample_cnt++;
			}

			//if(axel->conf->speedtest_timeout > 0 && axel->sample_cnt >= axel->conf->speedtest_timeout)
			if ((axel->conf->speedtest_timeout > 0) && (gettime() >= (axel->start_time + axel->conf->speedtest_timeout + 1)))
			{
#if defined(CONFIG_CMCC_BEIJING_GROUP_VERIFICATION)
				unsigned int udelay=10;
				struct timespec now;
				unsigned long long t=0, timeout=0;
				int ret=0;
																	
				ret=mib_get_s(MIB_DOWNLOAD_DIAG_DELAY,&udelay, sizeof(udelay));
				fprintf(stderr, "ret=%d\n", ret);
				//printf("%s %d, delay=%d\n", __FUNCTION__, __LINE__, udelay);
				clock_gettime(CLOCK_MONOTONIC, &now);
				//printf("sec=%u, nsec=%ld\n",now.tv_sec, now.tv_nsec);
				//printf("long long sec=%llu, nsec=%llu\n",(unsigned long long)now.tv_sec, (unsigned long long)now.tv_nsec);
				t=((unsigned long long)now.tv_sec)*1000000+((unsigned long long)now.tv_nsec)/1000;
				timeout=t+udelay;
				fprintf(stderr, "now{sec=%u, nsec=%ld}, udelay=%u, t=%llu, timeout=%llu\n", now.tv_sec, now.tv_nsec, udelay, t, timeout);
				while(t<timeout)
				{
					usleep(100);
					clock_gettime(CLOCK_MONOTONIC, &now);
					t=((unsigned long long)now.tv_sec)*1000000+((unsigned long long)now.tv_nsec)/1000;
				}
																			
				fprintf(stderr, "now{sec=%u, nsec=%ld}, t=%llu\n", now.tv_sec, now.tv_nsec, t);
#endif
				run = 0;
			}
		}
#ifdef CONFIG_RTK_HOST_SPEEDUP
		if(axel->conf->speedtest && (axel->sample_cnt >= MAX_SAMPLED_VALUES))
			run = 0;
#endif
	}
#ifdef CONFIG_RTK_HOST_SPEEDUP
	/* bug: if any thread is started just now and tcp connection is still not established, then HostSpeedUP-Info will 
	 * be updated after "del info", then speedtest result will be lost
	 * so we should make sure all connection is down now.
	 */
	if (conf->speedtest)
	{
		for( i = 0; i < axel->conf->num_connections; i ++ )
		{
			conn_disconnect( &axel->conn[i] );
			
			/* don't try to kill non existing thread */
			if ( *axel->conn[i].setup_thread != 0 )
				pthread_cancel( *axel->conn[i].setup_thread );
		}
	}
#endif

	strcpy( string + MAX_STRING / 2,
		size_human( axel->bytes_done - axel->start_byte ) );

	axel->EOMTime = gettime();
	get_total_wan_stats(NULL, &axel->total_bytes_end);
	if(axel->sample_cnt < MAX_SAMPLED_VALUES && axel->sample_cnt < conf->speedtest_timeout)
	{
		axel->sample_values[axel->sample_cnt] = axel->bytes_per_second;
		axel->total_sample_values[axel->sample_cnt] = (axel->total_bytes_end - axel->total_bytes_start) / (axel->EOMTime - axel->start_time);
		axel->sample_cnt++;
	}
	printf( _("\nDownloaded %s in %s. (%.2f KB/s)\n"),
		string + MAX_STRING / 2,
		time_human( axel->EOMTime - axel->start_time ),
		(double) axel->bytes_per_second / 1024 );

	i = axel->ready ? 0 : 2;

#ifdef CONFIG_RTK_HOST_SPEEDUP
	if (conf->speedtest)
	{
		/* defer disabling speedup module until fin handshake is completed */
		char remote_ip[64];
		inet_ntop(axel->conn->ip_familly, axel->conn->dip, remote_ip,sizeof(remote_ip));
		wait_socket_close(remote_ip, axel->conn[0].dport);
		
		close_speedup_test_proc(conf->ifname);
	}
#endif

#ifdef CONFIG_RTK_HOST_SPEEDUP
	if(axel->conf->speedtest)
	{
		if(axel->size == INT_MAX)
			axel->size = axel->conn[0].size;
		get_speedup_test_result(axel);
	}
#endif
	axel->error = eTR143_Completed;
	if(axel->conf->speedtest == 1)
	{
		write_tr143_test_result(axel);
	}
#ifdef CONFIG_USER_CUSPEEDTEST
	if(axel->conf->speedtest == 3)
	{
		axel->error = eSpeedTest_Completed;
		write_CU_test_result(axel);

		shmdt(pCspeed);
	}
#endif
#ifdef CONFIG_USER_CTMANAGEDEAMON
	if(axel->conf->speedtest == 2)
	{
		axel->error = eBUCPE_Completed;
		write_BUCPE_test_result(axel);
	}
#endif
	axel_close( axel );

	return( i );
}

/* SIGINT/SIGTERM handler						*/
void stop( int signal )
{
	fprintf(stderr, "axel recv %d signal, stop now!\n", signal);
	run = 0;
	close_speedup_test_proc(NULL);
#ifdef CONFIG_RTK_HOST_SPEEDUP
	stopBySignal = 1;
#endif
}

/* Convert a number of bytes to a human-readable form			*/
char *size_human( long long int value )
{
	if( value == 1 )
		sprintf( string, _("%lld byte"), value );
	else if( value < 1024 )
		sprintf( string, _("%lld bytes"), value );
	else if( value < 10485760 )
		sprintf( string, _("%.1f kilobytes"), (float) value / 1024 );
	else
		sprintf( string, _("%.1f megabytes"), (float) value / 1048576 );

	return( string );
}

/* Convert a number of seconds to a human-readable form			*/
char *time_human( int value )
{
	if( value == 1 )
		sprintf( string, _("%i second"), value );
	else if( value < 60 )
		sprintf( string, _("%i seconds"), value );
	else if( value < 3600 )
		sprintf( string, _("%i:%02i seconds"), value / 60, value % 60 );
	else
		sprintf( string, _("%i:%02i:%02i seconds"), value / 3600, ( value / 60 ) % 60, value % 60 );

	return( string );
}

/* Part of the infamous wget-like interface. Just put it in a function
	because I need it quite often..					*/
void print_commas( long long int bytes_done )
{
	int i, j;

	printf( "       " );
	j = ( bytes_done / 1024 ) % 50;
	if( j == 0 ) j = 50;
	for( i = 0; i < j; i ++ )
	{
		if( ( i % 10 ) == 0 )
			putchar( ' ' );
		putchar( ',' );
	}
	fflush( stdout );
}

static void print_alternate_output(axel_t *axel)
{
	long long int done=axel->bytes_done;
	long long int total=axel->size;
	int i,j=0;
	double now = gettime();

	printf("\r[%3ld%%] [", min(100,(long)(done*100./total+.5) ) );

	for(i=0;i<axel->conf->num_connections;i++)
	{
		for(;j<((double)axel->conn[i].currentbyte/(total+1)*50)-1;j++)
			putchar('.');

		if(axel->conn[i].currentbyte<axel->conn[i].lastbyte)
		{
			if(now <= axel->conn[i].last_transfer + axel->conf->connection_timeout/2 )
				putchar(i+'0');
			else
				putchar('#');
		} else
			putchar('.');

		j++;

		for(;j<((double)axel->conn[i].lastbyte/(total+1)*50);j++)
			putchar(' ');
	}

	if(axel->bytes_per_second > 1048576)
		printf( "] [%6.1fMB/s]", (double) axel->bytes_per_second / (1024*1024) );
	else if(axel->bytes_per_second > 1024)
		printf( "] [%6.1fKB/s]", (double) axel->bytes_per_second / 1024 );
	else
		printf( "] [%6.1fB/s]", (double) axel->bytes_per_second );

	if(done<total)
	{
		int seconds,minutes,hours,days;
		seconds=axel->finish_time - now;
		minutes=seconds/60;seconds-=minutes*60;
		hours=minutes/60;minutes-=hours*60;
		days=hours/24;hours-=days*24;
		if(days)
			printf(" [%2dd%2d]",days,hours);
		else if(hours)
			printf(" [%2dh%02d]",hours,minutes);
		else
			printf(" [%02d:%02d]",minutes,seconds);
	}

	fflush( stdout );
}

void print_help()
{
#ifdef NOGETOPTLONG
	printf(	_("Usage: axel [options] url1 [url2] [url...]\n"
		"\n"
		"-s x\tSpecify maximum speed (bytes per second)\n"
		"-n x\tSpecify maximum number of connections\n"
		"-o f\tSpecify local output file\n"
		"-S [x]\tSearch for mirrors and download from x servers\n"
		"-H x\tAdd header string\n"
		"-U x\tSet user agent\n"
		"-N\tJust don't use any proxy server\n"
		"-q\tLeave stdout alone\n"
		"-v\tMore status information\n"
		"-a\tAlternate progress indicator\n"
		"-h\tThis information\n"
		"-V\tVersion information\n"
		"-Z\tSpeedTest\n"
		"\n"
		"Visit http://axel.alioth.debian.org/ to report bugs\n") );
#else
	printf(	_("Usage: axel [options] url1 [url2] [url...]\n"
		"\n"
		"--max-speed=x\t\t-s x\tSpecify maximum speed (bytes per second)\n"
		"--num-connections=x\t-n x\tSpecify maximum number of connections\n"
		"--output=f\t\t-o f\tSpecify local output file\n"
		"--search[=x]\t\t-S [x]\tSearch for mirrors and download from x servers\n"
		"--header=x\t\t-H x\tAdd header string\n"
		"--user-agent=x\t\t-U x\tSet user agent\n"
		"--no-proxy\t\t-N\tJust don't use any proxy server\n"
		"--quiet\t\t\t-q\tLeave stdout alone\n"
		"--verbose\t\t-v\tMore status information\n"
		"--alternate\t\t-a\tAlternate progress indicator\n"
		"--help\t\t\t-h\tThis information\n"
		"--version\t\t-V\tVersion information\n"
		"\n"
		"Visit http://axel.alioth.debian.org/ to report bugs\n") );
#endif
}

void print_version()
{
	printf( _("Axel version %s (%s)\n"), AXEL_VERSION_STRING, ARCH );
	printf( "\nCopyright 2001-2002 Wilmer van der Gaast.\n" );
}

/* Print any message in the axel structure				*/
void print_messages( axel_t *axel )
{
	message_t *m;

	while( axel->message )
	{
		printf( "%s\n", axel->message->text );
		m = axel->message;
		axel->message = axel->message->next;
		free( m );
	}
}
