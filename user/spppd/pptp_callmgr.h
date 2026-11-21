/* pptp_callmgr.h ... Call manager for PPTP connections.
 *                    Handles TCP port 1723 protocol.
 *                    C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: pptp_callmgr.h,v 1.1 2011/11/18 12:20:05 ql Exp $
 */

#ifndef __PPTP_CALLMGR_H__
#define __PPTP_CALLMGR_H__

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "pptp_ctrl.h"

int get_call_ID(PPTP_CONN *conn, u_int16_t *callid, u_int16_t *peer_callid);
int conninfo_init(PPTP_CONN *conn);
void call_callback(PPTP_CONN *conn, PPTP_CALL *call, enum call_state state);
void conninfo_release(PPTP_CONN *conn);
int open_inetsock(struct in_addr inetaddr);
void close_inetsock(int fd, struct in_addr inetaddr);
#ifdef CONFIG_IPV6_VPN
int open_inet6sock(struct in6_addr inet6addr);
#endif // end of CONFIG_IPV6_VPN
#endif

