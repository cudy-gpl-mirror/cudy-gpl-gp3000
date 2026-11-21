/* dhcpc.h */
#ifndef _DHCPC_H
#define _DHCPC_H

#include "libbb_udhcp.h"

// Kaohj --- support option 125 Vendor-Identifying Vendor-Specific
//#define	DHCP_OPTION_125
#ifdef _PRMT_X_CT_COM_DHCP_
#define RECEIVE_DHCP_OPTION_125
#endif

#define INIT_SELECTING	0
#define REQUESTING	1
#define BOUND		2
#define RENEWING	3
#define REBINDING	4
#define INIT_REBOOT	5
#define RENEW_REQUESTED 6
#define RELEASED	7

#ifdef CONFIG_RTK_DEV_AP
#define FAST_SEND_DISCOVER
#endif

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#define DHCPC_DISCOVERY_TIMEOUT_PHASE_1 3
#else
#define DHCPC_DISCOVERY_TIMEOUT_PHASE_1 10
#endif
#if defined(CONFIG_E8B)
#define DHCPC_DISCOVERY_TIMEOUT_PHASE_2 60
#else
#ifdef FAST_SEND_DISCOVER
#define DHCPC_DISCOVERY_TIMEOUT_PHASE_2 5
#else
#define DHCPC_DISCOVERY_TIMEOUT_PHASE_2 30
#endif 
#endif

#define IPOE_DIAG_RESULT_DHCPC_FILE  "/tmp/ipoe_diag_dhcp"
enum ipoe_diag_result_t {
	IPOE_DIAG_RESULT_OK,
	IPOE_DIAG_RESULT_SendDHCPMsgError,
	IPOE_DIAG_RESULT_ServerNotFound,
	IPOE_DIAG_RESULT_ServerDeny,
	IPOE_DIAG_RESULT_GetIPAddressTimeout,
	IPOE_DIAG_RESULT_Other,
};

#ifdef DHCP_OPTION_125
// Kaohj --- VI Vendor-Specific detail
struct vendor_info_t {
	unsigned int ent_num;
	unsigned char data_len;
	char option_data;	/* list of option-data */
};
#endif

struct client_config_t {
	char foreground;		/* Do not fork */
	char quit_after_lease;		/* Quit after obtaining lease */
	char abort_if_no_lease;		/* Abort if no lease */
	char background_if_no_lease;	/* Fork to background if no lease */
	char microsoft_auto_ip_enable; /* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
	char *interface;		/* The name of the interface to use */
	char *pidfile;			/* Optionally store the process ID */
	char *script;			/* User script to run at dhcp events */
	unsigned char *clientid;	/* Optional client id to use */
	unsigned char *hostname;	/* Optional hostname to use */
#ifdef DHCP_OPTION_125
	unsigned char *vi_vendorSpec;	/* Optional VI Vendor-Specific to use */
#endif
	int ifindex;			/* Index number of the interface to use */
	unsigned char arp[6];		/* Our arp address */
	unsigned char Hardware_NAT;
	unsigned char is_ipoe_diag;
	int delay;
};

extern struct client_config_t client_config;
/*ql:20080926 START: initial MIB_DHCP_CLIENT_OPTION_TBL*/
//#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
#if	defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
extern unsigned int wan_ifIndex;
#endif
/*ql:20080926 END*/

//wpeng 20120411 added for parsestatic router option
#ifdef CONFIG_USER_DHCP_OPT_33_249
extern struct client_config_t client_config;
#define STATIC_ROUTE_FILE "/tmp/udhcpc_static_route."
#endif
extern char province[32];
#ifdef DHCPC_ELAPSED_TIME_SUPPORT
extern int dhcpc_elapsed_time;
#endif
//end of add
#endif
