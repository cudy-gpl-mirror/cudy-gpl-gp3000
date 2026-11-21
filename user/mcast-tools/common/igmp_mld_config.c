#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

#include "mclab.h"
#include "timeout.h"
#include "igmp_mld_utility.h"

char config_igmp_file[128] = "/var/igmp_config";
char *igmp_snooping_config_file = "/proc/igmp/ctrl/igmpSnoopingConfig";  //need replace with igmp hook proc file
char *igmp_compatiblity_file = "/proc/igmp/ctrl/igmpCompatiblity";
char *snooping_config_file = "/proc/igmp/ctrl/igmp6SnoopingConfig";  //need replace with igmp hook proc file
char config_mld_file[128] = "/var/mld_config";



char *cfg_param[] = {
"IpVersion",
"ForceVersion",
"FastLeave",
"LanCompatible",
"RobustnessValue",
"GroupMemberQueryInterval",
"GroupMemberQueryCount",
"StartupQueryInterval",
"StartupQueryCount",
"LastMemberQueryInterval",
"LastMemberQueryCount",
"GeneralQueryInterval",
"GeneralQueryCount",
"QueryResponseInterval",
"LinkChangeQueryInterval",
"LinkChangeQueryCount",
"IntervalScale",
"OldVersionHostPresentInterval",
NULL	//last parameter
};

#if defined(CONFIG_IGMPV3_SUPPORT)&&defined(CONFIG_COMPILE_IGMPV3)

void config_igmpproxy_para_from_file(struct mCastProxyConfig *mCastProxyPara)
{
	FILE *f;
	char linebuf[512];
	char *strptr = NULL, *tokptr = NULL;
	int para1, para2;

	if ((f = fopen(config_igmp_file, "r")) == NULL) 
	{
		printf("open config file fail, use default proxy parameter!\n");
		return;
	}

	//fscanf(f, "%d,%d", &mCastProxyPara->fastleaveEnable, &mCastProxyPara->compatibleEnbale);
	fgets(linebuf, sizeof(linebuf), f);
	strptr = linebuf;
	tokptr = strsep(&strptr, " ");
	while(tokptr)
	{
		if(!strcmp(tokptr, "fastleave"))
		{	
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			mCastProxyPara->fastleaveEnable = atoi(tokptr);	
			//printf("fastleaveEnable:%d\n", mCastProxyPara->fastleaveEnable);
		}
		else if(!strcmp(tokptr, "compatible"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			para1 = mCastProxyPara->compatibleEnbale;
			mCastProxyPara->compatibleEnbale= atoi(tokptr);
			printf("mCastProxyPara->compatibleEnbale = %d\n",mCastProxyPara->compatibleEnbale);
			if(para1 != mCastProxyPara->compatibleEnbale)
			{
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
				reset_compatible_status();
#endif				
			}
		
			//printf("compatibleEnbale:%d\n", mCastProxyPara->compatibleEnbale);
		}
		else if(!strcmp(tokptr, "robust"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			
			mCastProxyPara->timeConfig.robustness = atoi(tokptr);
			mCastProxyPara->timeConfig.lastMemberCount = atoi(tokptr);
			mCastProxyPara->timeConfig.setUpGeneralQueryCount = atoi(tokptr);
			mCastProxyPara->timeConfig.linkChangeQueryCount = atoi(tokptr);
			
			//printf("compatibleEnbale:%d\n", mCastProxyPara->timeConfig.robustness);
		}
		else if(!strcmp(tokptr, "query"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			
			mCastProxyPara->timeConfig.periodGeneralQueryInterval = atoi(tokptr);
			//printf("query interval:%d\n", mCastProxyPara->timeConfig.periodGeneralQueryInterval);
		}
		else if(!strcmp(tokptr, "response"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			
			mCastProxyPara->timeConfig.QueryResponseInterval = atoi(tokptr);
			
		}
		else if(!strcmp(tokptr, "startup"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			
			mCastProxyPara->timeConfig.setUpGeneralQueryInterval = atoi(tokptr);
		}
		else if(!strcmp(tokptr, "lastmember"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			
			mCastProxyPara->timeConfig.lastMemberInterval= atoi(tokptr);
		}

		tokptr = strsep(&strptr, " ");
	}
#if defined (SMALLER_TIMER_INTERVAL_SUPPORT)
	mCastProxyPara->timeConfig.memberQueryInterval = (mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval)*mCastProxyPara->timeConfig.intervalScale;
	mCastProxyPara->timeConfig.oldVersionHostPresentTimeout = (mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval)*mCastProxyPara->timeConfig.intervalScale;
#else
	mCastProxyPara->timeConfig.memberQueryInterval = mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval;
	mCastProxyPara->timeConfig.oldVersionHostPresentTimeout = mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval;
#endif
	fclose(f);
}
int write_igmpproxy_para_to_file(struct mCastProxyConfig *mCastProxyPara)
{
	FILE *f;
	char linebuf[1024]={0};
	char used_buff[32]={0};
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)
	snprintf(used_buff, sizeof(used_buff), "%s", "smaller timer interval used");
#endif
	
	
	if ((f = fopen(config_igmp_file, "w")) == NULL) 
	{
		printf("open config file fail, use default proxy parameter!\n");
		return 0;
	}
	
	snprintf(linebuf, sizeof(linebuf), "%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s\n",
		"Fast Leave:", mCastProxyPara->fastleaveEnable,
		"Lan Compatible:", mCastProxyPara->compatibleEnbale,
		"Old Version Host Present Interval:", mCastProxyPara->timeConfig.oldVersionHostPresentTimeout,
		"robustness value:", mCastProxyPara->timeConfig.robustness,
		"Query Interval:", mCastProxyPara->timeConfig.periodGeneralQueryInterval,
		"Query Response Interval:", mCastProxyPara->timeConfig.QueryResponseInterval,
		"Group Member Interval:", mCastProxyPara->timeConfig.memberQueryInterval,
		"Startup Query Interval:", mCastProxyPara->timeConfig.setUpGeneralQueryInterval,
		"Startup Query Count:", mCastProxyPara->timeConfig.setUpGeneralQueryCount,
		"Last Member Query Interval:", mCastProxyPara->timeConfig.lastMemberInterval,
		"Last Member Query Count:", mCastProxyPara->timeConfig.lastMemberCount,
		"Interval Scale:", mCastProxyPara->timeConfig.intervalScale,
		used_buff
		);

	fputs(linebuf, f);
	
	fclose(f);

	return 1;
}
void sync_igmpsnooping_para(struct mCastProxyConfig *mCastProxyPara)
{
	char config_cmd[128]={0};
	unsigned int lastMemberInterval;
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)
	 lastMemberInterval = ((mCastProxyPara->timeConfig.lastMemberInterval-1)/mCastProxyPara->timeConfig.intervalScale)+1;
#else
	 lastMemberInterval = mCastProxyPara->timeConfig.lastMemberInterval;
#endif
	//sync time config
	snprintf(config_cmd, sizeof(config_cmd), "echo time %s %d %s %d %s %d %s %d > %s", 
		"robust", mCastProxyPara->timeConfig.robustness,
		"query", mCastProxyPara->timeConfig.periodGeneralQueryInterval,
		"response", mCastProxyPara->timeConfig.QueryResponseInterval,
		"lastmember", lastMemberInterval,
		igmp_snooping_config_file);
	//system(config_cmd);
	va_cmd("/bin/sh" , 2, 1, "-c",config_cmd);

	//sync fastleave
	snprintf(config_cmd, sizeof(config_cmd), "echo fastleave %d %d > %s", 
 		mCastProxyPara->fastleaveEnable, 0, igmp_snooping_config_file);
	//system(config_cmd);
	va_cmd("/bin/sh" , 2, 1, "-c",config_cmd);

	//sync lan compatibility
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
	snprintf(config_cmd, sizeof(config_cmd), "echo compatible %d > %s", 
 		mCastProxyPara->compatibleEnbale, igmp_snooping_config_file);
 	//system(config_cmd);
	va_cmd("/bin/sh" , 2, 1, "-c",config_cmd);
#endif
}

#endif


#if defined(CONFIG_MLDV2_SUPPORT)&& defined(CONFIG_COMPILE_MLDV2)
void config_proxy_para_from_file(struct mCastProxyConfig *mCastProxyPara)
{
	FILE *f;
	char linebuf[512];
	char *strptr = NULL, *tokptr = NULL;
	int para1, para2;

	if ((f = fopen(config_mld_file, "r")) == NULL) 
	{
		printf("open config file fail, use default proxy parameter!\n");
		return;
	}

	//fscanf(f, "%d,%d", &mCastProxyPara->fastleaveEnable, &mCastProxyPara->compatibleEnbale);
	fgets(linebuf, sizeof(linebuf), f);
	strptr = linebuf;
	tokptr = strsep(&strptr, " ");
	while(tokptr)
	{
		if(!strcmp(tokptr, "fastleave"))
		{	
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			mCastProxyPara->fastleaveEnable = atoi(tokptr);	
			//printf("fastleaveEnable:%d\n", mCastProxyPara->fastleaveEnable);
		}
		else if(!strcmp(tokptr, "compatible"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			para1 = mCastProxyPara->compatibleEnbale;
			mCastProxyPara->compatibleEnbale= atoi(tokptr);
			if(para1 != mCastProxyPara->compatibleEnbale)
			{
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
				reset_compatible_status();
#endif				
			}
		
			//printf("compatibleEnbale:%d\n", mCastProxyPara->compatibleEnbale);
		}
		else if(!strcmp(tokptr, "robust"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			
			mCastProxyPara->timeConfig.robustness = atoi(tokptr);
			mCastProxyPara->timeConfig.lastMemberCount = atoi(tokptr);
			mCastProxyPara->timeConfig.setUpGeneralQueryCount = atoi(tokptr);
			mCastProxyPara->timeConfig.linkChangeQueryCount = atoi(tokptr);
			
			//printf("compatibleEnbale:%d\n", mCastProxyPara->timeConfig.robustness);
		}
		else if(!strcmp(tokptr, "query"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			
			mCastProxyPara->timeConfig.periodGeneralQueryInterval = atoi(tokptr);
			//printf("query interval:%d\n", mCastProxyPara->timeConfig.periodGeneralQueryInterval);
		}
		else if(!strcmp(tokptr, "response"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			
			mCastProxyPara->timeConfig.QueryResponseInterval = atoi(tokptr);
			
		}
		else if(!strcmp(tokptr, "startup"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			
			mCastProxyPara->timeConfig.setUpGeneralQueryInterval = atoi(tokptr);
		}
		else if(!strcmp(tokptr, "lastmember"))
		{
			tokptr = strsep(&strptr, " ");
			if(tokptr == NULL)
				return;
			
			mCastProxyPara->timeConfig.lastMemberInterval= atoi(tokptr);
		}

		tokptr = strsep(&strptr, " ");
	}
#if defined (SMALLER_TIMER_INTERVAL_SUPPORT)
	mCastProxyPara->timeConfig.memberQueryInterval = (mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval)*mCastProxyPara->timeConfig.intervalScale;
	mCastProxyPara->timeConfig.oldVersionHostPresentTimeout = (mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval)*mCastProxyPara->timeConfig.intervalScale;
#else
	mCastProxyPara->timeConfig.memberQueryInterval = mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval;
	mCastProxyPara->timeConfig.oldVersionHostPresentTimeout = mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval;
#endif
	fclose(f);
}
int write_proxy_para_to_file(struct mCastProxyConfig *mCastProxyPara)
{
	FILE *f;
	char linebuf[1024]={0};
	char used_buff[32]={0};
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)
	snprintf(used_buff, sizeof(used_buff), "%s", "smaller timer interval used");
#endif
	
	
	if ((f = fopen(config_mld_file, "w")) == NULL) 
	{
		printf("open config file fail, use default proxy parameter!\n");
		return 0;
	}
	
	snprintf(linebuf, sizeof(linebuf), "%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s %d\n%s\n",
		"Fast Leave:", mCastProxyPara->fastleaveEnable,
		"Lan Compatible:", mCastProxyPara->compatibleEnbale,
		"Old Version Host Present Interval:", mCastProxyPara->timeConfig.oldVersionHostPresentTimeout,
		"robustness value:", mCastProxyPara->timeConfig.robustness,
		"Query Interval:", mCastProxyPara->timeConfig.periodGeneralQueryInterval,
		"Query Response Interval:", mCastProxyPara->timeConfig.QueryResponseInterval,
		"Group Member Interval:", mCastProxyPara->timeConfig.memberQueryInterval,
		"Startup Query Interval:", mCastProxyPara->timeConfig.setUpGeneralQueryInterval,
		"Startup Query Count:", mCastProxyPara->timeConfig.setUpGeneralQueryCount,
		"Last Member Query Interval:", mCastProxyPara->timeConfig.lastMemberInterval,
		"Last Member Query Count:", mCastProxyPara->timeConfig.lastMemberCount,
		"Interval Scale:", mCastProxyPara->timeConfig.intervalScale,
		used_buff
		);

	fputs(linebuf, f);
	
	fclose(f);

	return 1;
}
void sync_snooping_para(struct mCastProxyConfig *mCastProxyPara)
{
	char config_cmd[128]={0};
	unsigned int lastMemberInterval;
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)
	 lastMemberInterval = ((mCastProxyPara->timeConfig.lastMemberInterval-1)/mCastProxyPara->timeConfig.intervalScale)+1;
#else
	 lastMemberInterval = mCastProxyPara->timeConfig.lastMemberInterval;
#endif
	//sync time config
	snprintf(config_cmd, sizeof(config_cmd), "echo time %s %d %s %d %s %d %s %d > %s", 
		"robust", mCastProxyPara->timeConfig.robustness,
		"query", mCastProxyPara->timeConfig.periodGeneralQueryInterval,
		"response", mCastProxyPara->timeConfig.QueryResponseInterval,
		"lastmember", lastMemberInterval,
		snooping_config_file);

		//system(config_cmd);
		va_cmd("/bin/sh" , 2, 1, "-c",config_cmd);

	//sync fastleave
	snprintf(config_cmd, sizeof(config_cmd), "echo fastleave %d %d > %s", 
 		mCastProxyPara->fastleaveEnable, 0, snooping_config_file);
	//system(config_cmd);
	va_cmd("/bin/sh" , 2, 1, "-c",config_cmd);


}

#endif
