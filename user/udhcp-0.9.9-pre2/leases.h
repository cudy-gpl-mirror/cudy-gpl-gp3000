/* leases.h */
#ifndef _LEASES_H
#define _LEASES_H

#ifdef DHCPD_MULTI_THREAD_SUPPORT
#include <stdlib.h>
#include "list.h"
#include <unistd.h>
#include <fcntl.h>
#endif

#define DHCP_LEASE_TIMER_INTERVAL 5 /* 5s */

struct dhcpOfferedAddr;
typedef void (*pfLeaseExpiredCallBack)(struct dhcpOfferedAddr *pstLease);

enum ForcePortalDeviceType
{
	FP_InvalidDevice = 0,
	FP_Computer,
	FP_STB,
	FP_MOBILE,
	FP_HGW,
	FP_MVT,
	FP_Camera,
	FP_AP,
	FP_PAD,
	FP_Speaker,
#ifdef CONFIG_CU_FTTR_MASTER
	FP_FTTRsub
#endif
};

struct dhcpClientInfo
{
    unsigned long ulDevice;
    pfLeaseExpiredCallBack pfLeaseExpired;
    void *pvDHCPClientData;
};

struct dhcpOfferedAddr {
	u_int8_t chaddr[16];
	u_int32_t yiaddr;	/* network order */
	u_int32_t expires;	/* host order */
	u_int32_t lease_time;
	u_int32_t interfaceType;
	u_int8_t hostName[64];
#ifdef GET_VENDOR_CLASS_ID
	u_int8_t vendor_class_id[64];
#endif
	unsigned int active_time;
    struct dhcpClientInfo stClientInfo; /* Extra client info, such as option 60 */
};

//struct lease to lease file 
struct dhcpOfferedAddrLease {
	u_int8_t chaddr[16];
	u_int32_t yiaddr;       /* network order */
	u_int32_t expires;      /* host order */
	u_int32_t interfaceType;
	u_int8_t hostName[64];
	u_int32_t hostType;
#ifdef GET_VENDOR_CLASS_ID
	u_int8_t vendor_class_id[64];
#endif
	u_int32_t active_time;
#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
	int category;
	int isCtcVendor;
	char szVendor[36];
	char szModel[36];
	char szUserClass[36];	//user class id in opt 77
	char szClientID[36];	//client id in opt 61
	char szFQDN[64];
#if defined(CONFIG_CU)
	char szVendorClassID[DHCP_VENDORCLASSID_LEN];
#endif
#endif
};

#ifdef DHCPD_MULTI_THREAD_SUPPORT
#define ARP_REQUEST_INTERVAL 1
#define ARP_REQUEST_MAX_TIMES  10
#define ARP_REQUEST_MIN_TIMES  2

#define KEEPALIVE_TIME		1*60*60	// 1 hour

enum ARPPING_STATE {MODE_ARP_IDLE, MODE_ARP_REQ_SENT, MODE_ARP_REPLY_RCVD, MODE_ARP_REPLY_TIMEOUT};
struct ArpPingInfo {
	struct list_head list;
	u_int32_t yiaddr;	//client address
	u_int32_t ip;		//local address
	u_int8_t  lmac[6];
	u_int8_t  dmac[6];
	u_int8_t  chaddr[6];	//client mac address for check_ip
	char      ifname[32];
	int       arpCnt;
	int       checkTimes;
	int       status;       //refer to ARPPING_STATE
	long      lastReqTime;
};
extern struct list_head arppinglist;
extern int forceKeepAliveChk;

void * arpping_thread(void *data);
void * keepalive_thread(void *data);
#endif

extern unsigned char blank_chaddr[];

void clear_lease(u_int8_t *chaddr, u_int32_t yiaddr);
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, unsigned long lease);
int lease_expired(struct dhcpOfferedAddr *lease);
struct dhcpOfferedAddr *oldest_expired_lease(void);
struct dhcpOfferedAddr *find_lease_by_chaddr(u_int8_t *chaddr);
struct dhcpOfferedAddr *find_lease_by_yiaddr(u_int32_t yiaddr);
void clear_all_lease(void);
void clear_one_lease(struct dhcpOfferedAddr *pstLease);
void lease_timer(void);

int check_ip(u_int32_t addr, u_int8_t *chaddr, void *config);

void dump_leases(void);
long get_uptime(void);
#endif
