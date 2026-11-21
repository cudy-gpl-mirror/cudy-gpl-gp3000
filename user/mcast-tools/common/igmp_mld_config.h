#ifndef _IGMP_MLD_CONFIG_H_
#define _IGMP_MLD_CONFIG_H_


extern char  config_igmp_file[128];
extern char config_mld_file[128];


enum {
  CFG_IP_VER=0,
  CFG_FORCE_VER,
  CFG_FAST_LEAVE,
  CFG_LAN_CONPATIBLE,
  CFG_ROBUST,
  CFG_MEMBER_QUERY_INTERVAL,
  CFG_MEMBER_QUERY_COUNT,
  CFG_STARTUP_QUERY_INTERVAL,
  CFG_STARTUP_QUERY_COUNT,
  CFG_LAST_MEMBER_QUERY_INTERVAL,
  CFG_LAST_MEMBER_QUERY_COUNT,
  CFG_QUERY_INTERVAL,
  CFG_QUERY_COUNT,
  CFG_QUERY_RESPONSE_INTERVAL,
  CFG_LINK_CHANGE_QUERY_INTERVAL,
  CFG_LINK_CHANGE_QUERY_COUNT,
  CFG_INTERVAL_SCALE,
  CFG_HOST_PRESERT_INTERVAL,

  CFG_LAST_PARAM	//last parameter
};

extern char *cfg_param[];


#if defined(CONFIG_MLDV2_SUPPORT) && defined(CONFIG_COMPILE_MLDV2)
void config_proxy_para_from_file(struct mCastProxyConfig *mCastProxyPara);
int write_proxy_para_to_file(struct mCastProxyConfig *mCastProxyPara);
void sync_snooping_para(struct mCastProxyConfig *mCastProxyPara);
#endif 

#if defined(CONFIG_IGMPV3_SUPPORT)&&defined(CONFIG_COMPILE_IGMPV3)

void config_igmpproxy_para_from_file(struct mCastProxyConfig *mCastProxyPara);
int write_igmpproxy_para_to_file(struct mCastProxyConfig *mCastProxyPara);
void sync_igmpsnooping_para(struct mCastProxyConfig *mCastProxyPara);
#endif

#endif
