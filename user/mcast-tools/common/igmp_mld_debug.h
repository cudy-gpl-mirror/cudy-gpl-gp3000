#ifndef _IGMP_MLD_DEBUG_H_
#define _IGMP_MLD_DEBUG_H_

#include "igmp_mld_utility.h"

//#define DBG_IGMP_MLD
#define IGMP_RECEIVE  	0x00000001
#define IGMP_MEMBERSHIP 0x00000002
#define IGMP_MFC		0x00000004
#define IGMP_PROTO		0x00000008

#define MLD_RECEIVE		0x00010000
#define MLD_MEMBERSHIP 0x0002000
#define MLD_MFC		0x00040000
#define MLD_PROTO	0x00080000



#define IGMP_MLD_RECEIVE (IGMP_RECEIVE|MLD_RECEIVE)
#define IGMP_MLD_MEMBERSHIP 	(IGMP_MEMBERSHIP|MLD_MEMBERSHIP)
#define IGMP_MLD_MFC			(IGMP_MFC|MLD_MFC)
#define IGMP_MLD_PROTO 	(IGMP_PROTO|MLD_PROTO)

#define RTL_IGMP_MLD_DEBUG_LEVEL (IGMP_MLD_RECEIVE|IGMP_MLD_MEMBERSHIP|IGMP_MLD_MFC|IGMP_MLD_PROTO)


#ifdef DBG_IGMP_MLD
#define DBG_IGMP_MLD_PRK(level , comment, arg...) DBG_PRK(level, comment, ##arg)
#else
#define DBG_IGMP_MLD_PRK(format, args...)
#endif

#ifndef DBG_PRK
#define DBG_PRK(bitmask, comment, arg...) \
do {\
	    if(RTL_IGMP_MLD_DEBUG_LEVEL&bitmask)\
	    {\
	    	printf("[%s:%d] ", __FUNCTION__, __LINE__);\
	        printf(comment, ##arg);\
	    }\
} while(0)
#endif


extern char *addr_sprintf(unsigned int *addr, int ipVersion);
extern int show_mfct(struct mcft_entry *groupList);
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
extern int show_compatibility_status(struct mCastProxyConfig * mCastProxyPara);
#endif
#endif
