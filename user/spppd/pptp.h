#ifndef _PPTP_H_
#define _PPTP_H_

#include "atm.h"
#include <linux/atmdev.h>
#include <linux/atmppp.h>
#include <sys/stat.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <linux/if_pppox.h>
#include "timeout.h"
#include "vpn.h"

#if 0
#define PPTP_DEBUG(...) 	do{printf( "PPtP:%s:%d> ", __FUNCTION__,__LINE__ );printf(__VA_ARGS__);printf("\n");}while(0)
#else
#define PPTP_DEBUG(...)
#endif

enum PPTP_STATE{
	PTP_ST_DEAD=0,
	PTP_ST_CTRL_INIT,
	PTP_ST_CTRL_ESTABLISH,
	PTP_ST_CALL_INIT,
	PTP_ST_CALL_FAIL,
	PTP_ST_CALL_ESTABLISH,

	PTP_ST_NUM /*last*/
};

enum PPTP_EVENT{
	PTP_EVENT_OPEN=0,
	PTP_EVENT_CLOSE,

	PTP_EVENT_NUM /*last*/
};
#ifdef CONFIG_IPV6_VPN
struct pptp_addr_v6 {
	__u16		call_id;
	struct in6_addr	sin6_addr;
};
struct sockaddr_pppox_v6 {
	__kernel_sa_family_t sa_family;
	unsigned int    sa_protocol;          /* protocol identifier */
	struct pptp_addr_v6   pptp;
} __attribute__((packed));
#endif

typedef struct pptp_s {
	char name[IFNAMSIZ];		/*dev name, pptp0... */
	char server[MAX_DOMAIN_LENGTH]; /* Server Address String */
	int		ipType;			/* 0: v4, 1: v6, 2: both */
	union{
		struct sockaddr_pppox local;
#ifdef CONFIG_IPV6_VPN
		struct sockaddr_pppox_v6 local6;
#endif
	};
	union{
		struct sockaddr_pppox sp;
#ifdef CONFIG_IPV6_VPN
		struct sockaddr_pppox_v6 sp6;
#endif
	};
	unsigned long previous_local_addr;	//just used for nat rule.
	int fd;				/* fd of PPTP socket */
	int ctrl_sock;

	int callctrlState;
	struct	callout ch; 				/* pptp timeout handler */
	int	timeout;
	int debug;
	int in_pptp_ctrl;				/* if in control connection establishing phase */
	struct callout ctrl_ch;
	int ctrl_timeout;

	void *conn;
	void *sppp;
	int up_retry;						// Mason Yu. Set up_retry flag
	struct	callout out_ctrl_ch; 		// Mason Yu.  pptp out_ctrl timeout handler
	int out_ctrl_timeout;				// Mason Yu. Set up_retry time
}PPTP_DRV_CTRL;

extern char *pptp_state[PTP_ST_NUM];
extern fd_set pptp_in_fds;
extern int pptp_max_fd;

struct sppp;
void process_pptp(void);
void pptp_down(PPTP_DRV_CTRL *p);
int pptp_open(PPTP_DRV_CTRL *p);
int pptp_close(struct sppp *sp);
int pptp_client_init(struct sppp *sppp);
int pptp_change_state(PPTP_DRV_CTRL *p, int state);
void pptp_add_fd(int fd);
void pptp_remove_fd(int fd);
//int pptp_init(struct sppp *sp);

#endif
