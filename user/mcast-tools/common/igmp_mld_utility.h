#ifndef _IGMP_MLD_UTILITY_H_
#define _IGMP_MLD_UTILITY_H_

#include "timeout.h"


/**********MACRO define start**********/
#define HOST_TRACKING_SUPPORT 1
#if defined(CONFIG_IGMPV3_SUPPORT)&& !defined(SMALLER_TIMER_INTERVAL_SUPPORT)
#define SMALLER_TIMER_INTERVAL_SUPPORT
#endif

#define USE_STATIC_ENTRY_BUFFER 1
#define MAX_MFCT_ENTRY 128
#define MAX_MBR_ENTRY 512
#define MAX_IFREC_ENTRY 8  
#define MAX_SRC_ENTRY 64

#define PERIODICAL_TIMER_TPYE 		0x1
#define LIMIT_RETRY_TIMER_TYPE 		0x2
#define IGMP_TIMER_V1					0x10
#define IGMP_TIMER_V2					0x20
#define IGMP_TIMER_V3					0x40
#define IGMP_TIMER						(IGMP_TIMER_V1|IGMP_TIMER_V2|IGMP_TIMER_V3)
#define MLD_TIMER_V1					0x100
#define MLD_TIMER_V2					0x200
#define MLD_TIMER						(MLD_TIMER_V1|MLD_TIMER_V2)
#define IGMP_VER_TO_TIMER_VER(version) (8<<version)
#define MLD_VER_TO_TIMER_VER(version) (0x80<<version)


#define IP_VERSION4	4
#define IP_VERSION6 6

#define IGMP_VER_1      1
#define IGMP_VER_2		2
#define IGMP_VER_3		3
#define MLD_VER_1		1
#define MLD_VER_2		2

#define MODE_IS_INCLUDE			1
#define MODE_IS_EXCLUDE			2
#define CHANGE_TO_INCLUDE		3
#define CHANGE_TO_EXCLUDE		4
#define ALLOW_NEW_SOURCES		5
#define BLOCK_OLD_SOURCES		6

#define MCAST_EXCLUDE	0
#define MCAST_INCLUDE	1

#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
#define COMPATIBILITY_IGMPV3_MLDV2	0
#define COMPATIBILITY_IGMPV2_MLDV1	1
#define COMPATIBILITY_IGMPV1		2
#endif
#define ADDR_SIZE(ipVersion) ((ipVersion==IP_VERSION4)?sizeof(unsigned int):(4*sizeof(unsigned int)))
#define ADDR_SIZE_INT(ipVersion) ((ipVersion==IP_VERSION4)?1:4)
#define IGMPV3_MLDV2_QUERY_SIZE(ipVersion) ((ipVersion==IP_VERSION4)?12:28)

#define IGMPV3_MLDV2_MASK(value, nb) ((nb)>=32 ? (value) : ((1<<(nb))-1) & (value))
#define IGMPV3_MLDV2_EXP(thresh, nbmant, nbexp, value) \
	((value) < (thresh) ? (value) : \
        ((IGMPV3_MLDV2_MASK(value, nbmant) | (1<<(nbmant))) << \
         (IGMPV3_MLDV2_MASK((value) >> (nbmant), nbexp) + (nbexp))))
         
#define IGMPV3_MLDV2_MRC(ipVersion, value) \
	((ipVersion==IP_VERSION4)?\
	 (IGMPV3_MLDV2_EXP(0x8000, 12, 3, value)):\
	 (IGMPV3_MLDV2_EXP(0x80, 4, 3, value)))

#define IGMPV3_MLDV2_QQIC(ipVersion, value) (IGMPV3_MLDV2_EXP(0x80, 4, 3, value))

#define MEMBER_IN_KERNEL 0x01

#if defined(CONFIG_IGMPV3_SUPPORT)
#define ALL_SYSTEMS		htonl(0xE0000001)	// General Query - 224.0.0.1
#define ALL_ROUTERS		htonl(0xE0000002)	// Leave - 224.0.0.2	
#define ALL_ROUTERS_V3	htonl(0xE0000016)	// Leave - 224.0.0.22
#define ALL_PRINTER		htonl(0xEFFFFFFA)	// notify all printer - 239.255.255.250
#define CLASS_D_MASK	0xE0000000		// the mask that defines IP Class D
#define IPMULTI_MASK	0x007FFFFF		// to get the low-order 23 bits
#define IN_MULTICAST_RESV1(addr)	((((unsigned int)(addr)) & 0xFFFFFF00) == 0xe0000000)	// 224.0.0.x
#endif

#if defined(CONFIG_MLDV2_SUPPORT)
#define IS_IPV6_MULTICAST(ipv6addr) 			((ipv6addr[0] & 0xFF000000)==0xFF000000)
#define IS_IPV6_RESVER_0_ADDRESS(ipv6addr)		((ipv6addr[0] & 0x00F00000)==0x00000000)
#endif

#ifndef NULL
#define NULL 	0
#endif

#ifndef TRUE
#define TRUE 	1
#endif

#ifndef FALSE
#define FALSE 	0
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILED
#define FAILED -1
#endif

/**********MACRO define end**********/

#if defined(CONFIG_MLDV2_SUPPORT)
#ifndef IPV6_CACHE_REPORT
/* Special Case:
 * Due to first byte of struct mrt6msg must be zero, and there is no zero for icmpv6 type
 *      - https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol_for_IPv6
 *      - please check kernel/include/uapi/linux/mroute6.h
 * So, We assume type 0 is IPv6 cache report
 */
#define IPV6_CACHE_REPORT       0
#endif
#endif  /* CONFIG_MLDV2_SUPPORT */


/**********struct define start**********/
struct mCastTimerParameter{
	unsigned int robustness;
	unsigned int memberQueryInterval;    //Multicast Address Listening Interval,MALI
	unsigned int memberQueryCount;
	unsigned int lastMemberInterval;	 //Last Member Query Interval,  LLQI*interval Scale, to calculate max response code in special query
	unsigned int lastMemberCount;		 //Last Member Query Count
	unsigned int setUpGeneralQueryInterval;	 //Startup query Interval
	unsigned int setUpGeneralQueryCount;	 //Startup query Count
	unsigned int periodGeneralQueryInterval; //Query Interval, QQI
	unsigned int periodGeneralQueryCount;
	unsigned int QueryResponseInterval;		 //QRI*interval Scale, to calculate max response code in general query
	unsigned int linkChangeQueryInterval;	 
	unsigned int linkChangeQueryCount;
	unsigned int intervalScale;
	unsigned int oldVersionHostPresentTimeout;  //also used for MLDV2 compatible;
};
struct mCastProxyConfig{
	unsigned char ipVersion;
	struct mCastTimerParameter timeConfig;
	unsigned char forceVersion;
	unsigned char fastleaveEnable;
	unsigned char compatibleEnbale;	
};

struct proxy_timer {
	int		type;			// timer type
	unsigned int		timerInterval;
	unsigned int		retry_left;		// retry counts left
	struct callout	ch;
	unsigned int	lefttime;
};

extern struct mCastProxyConfig mldProxyPara;

struct query_timer {
	int		type;			// timer type
	unsigned int		timerInterval;
	unsigned int		retry_left;		// retry counts left
	struct callout	ch;
	unsigned int	lefttime;

	unsigned int    maxDelay;
	unsigned int 	robustValue;
};

struct mbr_entry {
	struct mbr_entry	*next;
	unsigned int		user_addr[4];
	struct proxy_timer	timer;
	int filter_mode;
};

struct src_entry {
	struct src_entry	*next;
	unsigned int		srcaddr[4];
	unsigned int	user_count[2];	// group member count
	struct mbr_entry *grp_mbr;	//user entry
	struct proxy_timer	timer;
	//int ifindex;
	//int filter_mode;
};


struct ifrec_entry{
	
	struct ifrec_entry *next;
	int ip_version;
	unsigned int 	grp_addr[4];
	unsigned int	user_count;	// group member count
	struct mbr_entry *grp_mbr;	//user entry
	int ifindex;
	struct proxy_timer timer;
	struct src_entry *srclist;
	int filter_mode;
	int	proxy_ver;
	int proxy_vif;
	int	mrt_state;				//0:disable, 1: enable all sources
	int flag;  					//indicate if this group is added to the interface.
};

struct mcft_entry {
	struct mcft_entry 	*next;
	int ip_version;
	unsigned int 	grp_addr[4];
	unsigned char	if_count;//group interface count(br0,eth1...)
	struct ifrec_entry		*iflist; 	//group interface entry
};

#ifdef USE_STATIC_ENTRY_BUFFER
struct mcft_entry_en {
	int valid;
	struct mcft_entry entry_mcft_;
};

struct mbr_entry_en {
	int valid;
	struct mbr_entry entry_mbr_;
};
struct ifrec_entry_en {
	int valid;
	struct ifrec_entry entry_ifrec_;
};
struct src_entry_en{
	int valid;
	struct src_entry entry_src_;
};
#endif
/**********struct define end**********/

/**********function prototype start**********/
extern int va_cmd(const char *cmd, int num, int dowait, ...) ;
void init_general_query(struct mCastProxyConfig *mCastProxyPara);
void set_link_change_query(struct mCastProxyConfig *mCastProxyPara);
void send_setup_general_query(struct mCastProxyConfig *mCastProxyPara);

#if defined(CONFIG_IGMPV3_SUPPORT)&&defined(CONFIG_COMPILE_IGMPV3)
int igmp_send_query(int version, unsigned int dst, unsigned int grp, unsigned char mrt, unsigned char qqic, unsigned char qrv, unsigned short srcnum, unsigned int *srcfilter);
int igmp_add_group(unsigned int group, int vif);
int igmp_del_group(unsigned int group, int vif);
int igmp_set_srcfilter(unsigned int group, int filterMode, int srcNum, unsigned int *igmpSrcList, int vif);
int igmp_add_mr(unsigned int group, unsigned int src, int enable, int in_vif, int out_vif);
int igmp_del_mr(unsigned int group, unsigned int src, int in_vif);
#endif
#if defined(CONFIG_MLDV2_SUPPORT)&& defined(CONFIG_COMPILE_MLDV2)
int mld_send_query(int version, struct in6_addr dst, struct in6_addr grp, unsigned short mrt, unsigned char qqic, unsigned char qrv, unsigned short srcnum, struct in6_addr *srcfilter);
int mld_add_group( struct in6_addr group, int vif);
int mld_del_group( struct in6_addr group,int vif);
int mld_set_srcfilter(struct in6_addr group, int filterMode, int srcNum, struct in6_addr *mldSrcList, int vif);
int mld_add_mr(struct in6_addr group, struct in6_addr src, int enable ,int in_vif, int out_vif);
int mld_del_mr( struct in6_addr group, struct in6_addr src,int invif);
#endif
int handle_group_timer(struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList);
int handle_src_timer(struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList);
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
extern int reset_compatible_status(void);
#endif

/**********function prototype end**********/

#endif
