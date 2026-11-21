#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

#include "timeout.h"
#include "igmp_mld_utility.h"
#include "igmp_mld_config.h"

#if defined(CONFIG_IGMPV3_SUPPORT)
char *snooping_config_file = "/proc/br_igmpsnoop";
char config_igmp_file[128] = "/var/igmp_config";
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
char *snooping_config_file = "/proc/br_mldsnoop";
char config_mld_file[128] = "/var/mld_config";
#endif

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

int config_proxy_para_from_file(struct mCastProxyConfig *mCastProxyPara)
{
	FILE *f=NULL;
	char *linebuf=NULL;
	char *strptr = NULL, *tokptr = NULL;
	int read, oldvalue, i=0;
	char delim[] = " =;";
	size_t len = 0;

#if defined(CONFIG_MLDV2_SUPPORT)
	if ((f = fopen(config_mld_file, "r")) == NULL) 
	{
		printf("[%s]open config file fail, use default proxy parameter!\n", __FUNCTION__);
		return SUCCESS;
	}

	while ((read = getline(&linebuf, &len, f)) != -1) {
		strptr = (char *)strtok(linebuf, delim);
		if(strptr)
		{
			for(i=0; i<CFG_LAST_PARAM;i++){
				if(!strcmp(strptr, cfg_param[i])){
					break;
				}
			}

			if(i==CFG_LAST_PARAM)
				break;

			tokptr = (char *)strtok(NULL, delim);
			if(tokptr == NULL){
				printf("[%s] null parameter value for %s\n", __FUNCTION__, cfg_param[i]);
				goto err;
			}

			switch(i){
			case CFG_IP_VER:
				if(atoi(tokptr) == IP_VERSION4)
					mCastProxyPara->ipVersion = IP_VERSION4;
				else
					mCastProxyPara->ipVersion = IP_VERSION6;
				break;
			case CFG_FORCE_VER:
				if(atoi(tokptr) == MLD_VER_1)
					mCastProxyPara->forceVersion = MLD_VER_1;
				else
					mCastProxyPara->forceVersion = MLD_VER_2;
				break;
			case CFG_FAST_LEAVE:
				mCastProxyPara->fastleaveEnable = atoi(tokptr);
				break;
			case CFG_LAN_CONPATIBLE:
				oldvalue = mCastProxyPara->compatibleEnbale;
				mCastProxyPara->compatibleEnbale= atoi(tokptr);
				if(oldvalue != mCastProxyPara->compatibleEnbale)
				{
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
					reset_compatible_status();
#endif				
				}
				break;
			case CFG_ROBUST:
				mCastProxyPara->timeConfig.robustness = atoi(tokptr);
				break;
			case CFG_MEMBER_QUERY_INTERVAL:
				mCastProxyPara->timeConfig.memberQueryInterval = atoi(tokptr);
				break;
			case CFG_MEMBER_QUERY_COUNT:
				mCastProxyPara->timeConfig.memberQueryCount = atoi(tokptr);
				break;
			case CFG_STARTUP_QUERY_INTERVAL:
				mCastProxyPara->timeConfig.setUpGeneralQueryInterval = atoi(tokptr);
				break;
			case CFG_STARTUP_QUERY_COUNT:
				mCastProxyPara->timeConfig.setUpGeneralQueryCount = atoi(tokptr);
				break;
			case CFG_LAST_MEMBER_QUERY_INTERVAL:
				mCastProxyPara->timeConfig.lastMemberInterval= atoi(tokptr);
				break;
			case CFG_LAST_MEMBER_QUERY_COUNT:
				mCastProxyPara->timeConfig.lastMemberCount = atoi(tokptr);
				break;
			case CFG_QUERY_INTERVAL:
				mCastProxyPara->timeConfig.periodGeneralQueryInterval = atoi(tokptr);
				break;
			case CFG_QUERY_COUNT:
				mCastProxyPara->timeConfig.periodGeneralQueryCount= atoi(tokptr);
				break;
			case CFG_QUERY_RESPONSE_INTERVAL:
				mCastProxyPara->timeConfig.QueryResponseInterval = atoi(tokptr);
				break;
			case CFG_LINK_CHANGE_QUERY_INTERVAL:
				mCastProxyPara->timeConfig.linkChangeQueryInterval = atoi(tokptr);
				break;
			case CFG_LINK_CHANGE_QUERY_COUNT:
				mCastProxyPara->timeConfig.linkChangeQueryCount = atoi(tokptr);
				break;
			case CFG_INTERVAL_SCALE:
				mCastProxyPara->timeConfig.intervalScale = atoi(tokptr);
				break;
			case CFG_HOST_PRESERT_INTERVAL:
				mCastProxyPara->timeConfig.oldVersionHostPresentTimeout = atoi(tokptr);
				break;
			default:
				fprintf(stderr, "[%s]unknow config(%s)\n", __FUNCTION__, cfg_param[i]);
				break;
			}
		}
	}

#if defined (SMALLER_TIMER_INTERVAL_SUPPORT)
	mCastProxyPara->timeConfig.memberQueryInterval = (mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval)*mCastProxyPara->timeConfig.intervalScale;
	mCastProxyPara->timeConfig.oldVersionHostPresentTimeout = (mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval)*mCastProxyPara->timeConfig.intervalScale;
#else
	mCastProxyPara->timeConfig.memberQueryInterval = mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval;
	mCastProxyPara->timeConfig.oldVersionHostPresentTimeout = mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval;
#endif
	fclose(f);
#endif	//#if defined(CONFIG_MLDV2_SUPPORT)
	return SUCCESS;
 err:
	fclose(f);
    return FAILED;
}

int write_proxy_para_to_file(struct mCastProxyConfig *mCastProxyPara)
{
	FILE *f=NULL;

	if ((f = fopen(config_mld_file, "w")) == NULL) 
	{
		printf("open config file fail, use default proxy parameter!\n");
		return FAILED;
	}

	fprintf(f, "%s=%d;\n", cfg_param[CFG_IP_VER], mCastProxyPara->ipVersion);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_FORCE_VER], mCastProxyPara->forceVersion);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_FAST_LEAVE], mCastProxyPara->fastleaveEnable);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_LAN_CONPATIBLE], mCastProxyPara->compatibleEnbale);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_ROBUST], mCastProxyPara->timeConfig.robustness);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_MEMBER_QUERY_INTERVAL], mCastProxyPara->timeConfig.memberQueryInterval);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_MEMBER_QUERY_COUNT], mCastProxyPara->timeConfig.memberQueryCount);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_STARTUP_QUERY_INTERVAL], mCastProxyPara->timeConfig.setUpGeneralQueryInterval);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_STARTUP_QUERY_COUNT], mCastProxyPara->timeConfig.setUpGeneralQueryCount);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_LAST_MEMBER_QUERY_INTERVAL], mCastProxyPara->timeConfig.lastMemberInterval);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_LAST_MEMBER_QUERY_COUNT], mCastProxyPara->timeConfig.lastMemberCount);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_QUERY_INTERVAL], mCastProxyPara->timeConfig.periodGeneralQueryInterval);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_QUERY_COUNT], mCastProxyPara->timeConfig.periodGeneralQueryCount);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_QUERY_RESPONSE_INTERVAL], mCastProxyPara->timeConfig.QueryResponseInterval);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_LINK_CHANGE_QUERY_INTERVAL], mCastProxyPara->timeConfig.linkChangeQueryInterval);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_LINK_CHANGE_QUERY_COUNT], mCastProxyPara->timeConfig.linkChangeQueryCount);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_INTERVAL_SCALE], mCastProxyPara->timeConfig.intervalScale);
	fprintf(f, "%s=%d;\n", cfg_param[CFG_HOST_PRESERT_INTERVAL], mCastProxyPara->timeConfig.oldVersionHostPresentTimeout);
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)
	fprintf(f, "%s;\n", "smaller timer interval used");
#endif
	fclose(f);

	return SUCCESS;
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
	system(config_cmd);

	//sync fastleave
	snprintf(config_cmd, sizeof(config_cmd), "echo fastleave %d %d > %s", 
 		mCastProxyPara->fastleaveEnable, 0, snooping_config_file);
	system(config_cmd);

	//sync lan compatibility
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
	snprintf(config_cmd, sizeof(config_cmd), "echo compatible %d > %s", 
 		mCastProxyPara->compatibleEnbale, snooping_config_file);
 	system(config_cmd);
#endif
}



