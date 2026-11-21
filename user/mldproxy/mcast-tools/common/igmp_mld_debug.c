#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#if defined(CONFIG_IGMPV3_SUPPORT)
#include <linux/in.h>
#endif

#if defined(CONFIG_MLDV2_SUPPORT)
#include "mld.h"
#endif

#include "timeout.h"
#include "igmp_mld_utility.h"
#include "igmp_mld_debug.h"

#if defined(CONFIG_MLDV2_SUPPORT)
static char digits[] = "0123456789abcdef";
static char ip6buf[8][48];
static int ip6round = 0;
#endif

#if defined(CONFIG_IGMPV3_SUPPORT)
static char ip4buf[8][16];
static int ip4round = 0;
#endif

/**********grobal parameters define end**********/
#if defined(CONFIG_IGMPV3_SUPPORT)
char *ip4_sprintf(const struct in_addr *addr)
{
	char *cp;
	if(addr==NULL)
		return NULL;
	
	unsigned int addr_tmp = ntohl(addr->s_addr);
	ip4round = (ip4round+1)&7;
	cp = ip4buf[ip4round];

	sprintf(cp,"%d.%d.%d.%d",(addr_tmp&0xFF000000)>>24, (addr_tmp&0xFF0000)>>16, (addr_tmp&0xFF00)>>8, (addr_tmp&0xFF));
	return ip4buf[ip4round];
}
#endif

#if defined(CONFIG_MLDV2_SUPPORT)
char * ip6_sprintf(const struct in6_addr *addr)
{
	int i;
	char *cp;
	const u_short *a = (const u_short *)addr;
	const u_char *d;
	int dcolon = 0;

	if(addr==NULL)
		return NULL;
	
	ip6round = (ip6round + 1) & 7;
	cp = ip6buf[ip6round];

	for (i = 0; i < 8; i++) {
		if (dcolon == 1) {
			if (*a == 0) {
				if (i == 7)
					*cp++ = ':';
				a++;
				continue;
			} else
				dcolon = 2;
		}
		if (*a == 0) {
			if (dcolon == 0 && *(a + 1) == 0) {
				if (i == 0)
					*cp++ = ':';
				*cp++ = ':';
				dcolon = 1;
			} else {
				*cp++ = '0';
				*cp++ = ':';
			}
			a++;
			continue;
		}
		d = (const u_char *)a;
		*cp++ = digits[*d >> 4];
		*cp++ = digits[*d++ & 0xf];
		*cp++ = digits[*d >> 4];
		*cp++ = digits[*d & 0xf];
		*cp++ = ':';
		a++;
	}
	*--cp = 0;
	return(ip6buf[ip6round]);
}
#endif

char *addr_sprintf(unsigned int *addr, int ipVersion)
{
#if defined(CONFIG_IGMPV3_SUPPORT)
	if(ipVersion==IP_VERSION4)
	{
		return ip4_sprintf(addr);
	}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	if(ipVersion==IP_VERSION6)
	{
		struct in6_addr group;
		group.s6_addr32[0] = addr[0];
		group.s6_addr32[1] = addr[1];
		group.s6_addr32[2] = addr[2];
		group.s6_addr32[3] = addr[3];
		return ip6_sprintf(&group);
	}
#endif

	return NULL;
}

int show_mfct(struct mcft_entry *groupList)
{
	struct mcft_entry *mcp = groupList;
	struct ifrec_entry *ifp = NULL;
	struct src_entry *srcp = NULL;
	struct mbr_entry *mbrp = NULL;
	int i =0, j=0;
	printf("Multicast Data Base\n");
	while(mcp)
	{
		printf("[%d] Group:%s\n", i, addr_sprintf(mcp->grp_addr, mcp->ip_version));
		ifp = mcp->iflist;
		while(ifp)
		{
			printf("\t Interface:%d Filter Mode:%s, Timer:%d, Count:%d, Listener Num:%d\n", 
				ifp->ifindex, (ifp->filter_mode==MCAST_INCLUDE)?"IN":"EX", ifp->timer.lefttime, ifp->timer.retry_left, ifp->user_count);
			mbrp = ifp->grp_mbr;
			while(mbrp)
			{
				printf("\t\t --> %s %d %d\n", addr_sprintf(mbrp->user_addr, mcp->ip_version), mbrp->timer.lefttime, mbrp->timer.retry_left);
				mbrp = mbrp->next;
			}

			printf("\t Src List:\n");
			srcp = ifp->srclist;
			j = 0;
			while(srcp)
			{
				printf("\t [%d] Src:%s, Timer:%d, Count:%d, IN Listener Num:%d, EX Listerner Num:%d\n", 
					j, addr_sprintf(srcp->srcaddr, ifp->ip_version), srcp->timer.lefttime, srcp->timer.retry_left, srcp->user_count[MCAST_INCLUDE], srcp->user_count[MCAST_EXCLUDE]);
				mbrp = srcp->grp_mbr;
				while(mbrp)
				{
					printf("\t\t --> %s %s %d %d\n", addr_sprintf(mbrp->user_addr, mcp->ip_version), (mbrp->filter_mode==MCAST_INCLUDE)?"IN":"EX", mbrp->timer.lefttime, mbrp->timer.retry_left);
					mbrp = mbrp->next;
				}
				srcp = srcp->next;
				j++;
			}
			
			ifp = ifp->next;
		}
		mcp = mcp->next;
		i++;
	}
}
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
extern struct proxy_timer	igmpV2MldV1Timer[MAX_IFREC_ENTRY];
extern struct proxy_timer   igmpV1Timer[MAX_IFREC_ENTRY];
extern int compatibilityStatus[MAX_IFREC_ENTRY];

int show_compatibility_status(struct mCastProxyConfig *mCastProxyPara)
{
	int i=0;

	printf("Compatibility Status\n");
	printf("\tCompatibility Function Valid: %d\n", mCastProxyPara->compatibleEnbale);
	printf("\tvif\tstatus\ttimer\n");
	for(i=0; i<MAX_IFREC_ENTRY; i++)
	{
		if(compatibilityStatus[i]!=COMPATIBILITY_IGMPV3_MLDV2)
			printf("\t%d\t%s\t%d\n", i, 
			(compatibilityStatus[i]==COMPATIBILITY_IGMPV2_MLDV1)?(mCastProxyPara->ipVersion==IP_VERSION4?"IGMPV2":"MLDV1"):"IGMPV1",
			(compatibilityStatus[i]==COMPATIBILITY_IGMPV2_MLDV1)?(igmpV2MldV1Timer[i].lefttime):(igmpV1Timer[i].lefttime));
	}
}
#endif
