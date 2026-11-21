#ifndef _IGMP_MLD_CONFIG_H_
#define _IGMP_MLD_CONFIG_H_

void config_proxy_para_from_file(struct mCastProxyConfig *mCastProxyPara);
int write_proxy_para_to_file(struct mCastProxyConfig *mCastProxyPara);
void sync_snooping_para(struct mCastProxyConfig *mCastProxyPara);
#endif
