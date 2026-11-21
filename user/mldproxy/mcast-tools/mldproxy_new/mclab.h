#ifndef _MCLAB_H_
#define _MCLAB_H_
/*
**  smcroute - static multicast routing control 
**  Copyright (C) 2001 Carsten Schill <carsten@cschill.de>
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
**  $Id: mclab.h,v 1.1.1.1 2007-08-06 10:04:43 root Exp $	
**
*/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <net/if.h>		/* ifreq struct         */
#include <arpa/inet.h>
#include <netinet/in.h>

#define USE_LINUX_IN_H
#ifdef USE_LINUX_IN_H
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/mroute.h>
#include <linux/mroute6.h>
#include <linux/ip.h>
#include <linux/icmpv6.h>
#include <linux/ipv6.h>
#include <linux/if_addr.h>
//#include <net/ipv6.h>
//#include <net/mld.h>
#else
#include <netinet/in6.h>
#endif

typedef u_int8_t   uint8;
typedef u_int16_t  uint16;
typedef u_int32_t  uint32;

#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
#define  MAXWAN 4
#elif defined(CONFIG_MLDPROXY_MULTIWAN)
#define  MAXWAN 8
#else
#define  MAXWAN 1
#endif

extern char mld_down_if_name[IFNAMSIZ];
extern char mld_down_if_idx;
#if defined(CONFIG_MLDPROXY_MULTIWAN) || defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
extern char mld_up_if_name[MAXWAN][IFNAMSIZ];
#if defined(CONFIG_MLDPROXY_MULTIWAN)
extern char mld_up_if_idx[MAXWAN];
#endif
#else
extern char mld_up_if_name[IFNAMSIZ];
extern char mld_up_if_idx;
#endif

/*
 *	ICMP codes for neighbour discovery messages
 */

#define NDISC_ROUTER_SOLICITATION	133
#define NDISC_ROUTER_ADVERTISEMENT	134
#define NDISC_NEIGHBOUR_SOLICITATION	135
#define NDISC_NEIGHBOUR_ADVERTISEMENT	136
#define NDISC_REDIRECT			137

#define IPV6_ADDR_UNICAST      	0x0001U	
#define IPV6_ADDR_MULTICAST    	0x0002U	

#define IPV6_ADDR_LOOPBACK	0x0010U
#define IPV6_ADDR_LINKLOCAL	0x0020U
#define IPV6_ADDR_SITELOCAL	0x0040U
#if 0
#define IN6_IS_ADDR_LOOPBACK(a) \
	(((__const uint32_t *) (a))[0] == 0				      \
	 && ((__const uint32_t *) (a))[1] == 0				      \
	 && ((__const uint32_t *) (a))[2] == 0				      \
	 && ((__const uint32_t *) (a))[3] == htonl (1))

#define IN6_IS_ADDR_MULTICAST(a) ((a[0] & htonl(0xff000000))== htonl(0xff000000))
//#define IN6_IS_ADDR_MULTICAST(a) (((__const uint8_t *) (a))[0] == 0xff)
#define IN6_IS_ADDR_LINKLOCAL(a) ((a[0] & htonl(0xffc00000)) == htonl(0xfe800000))
#endif
//#define IN6_IS_ADDR_LINKLOCAL(a) ((((__const uint32_t *) (a))[0] & htonl (0xffc00000)) == htonl (0xfe800000))
//#define IS_IPV6_LINKLOCAL_ADDRESS(ipv6addr)	((ipv6addr[0] & 0xffc00000)==(0xfe800000))


#define MIN( a, b ) ((a) < (b) ? (a) : (b))
#define MAX( a, b ) ((a) < (b) ? (b) : (a))
#define VCMC( Vc )  (sizeof( Vc ) / sizeof( (Vc)[ 0 ] ))
#define VCEP( Vc )  (&(Vc)[ VCMC( Vc ) ])
#define MAXVIFS		32	//defined in mroute.h before

#define MAX_MC_VIFS    MAXVIFS     // !!! check this const in the specific includes
// #define NO_VIF_IX  MAXVIFS       /* invalid VIF index (32) */

struct IfDesc {
  char Name[ sizeof( ((struct ifreq *)NULL)->ifr_name ) ];
  struct in6_addr InAdr;       //need to change hx   /* == 0 for non IP interfaces */     
  struct in6_addr GlobalAdr;	//global address
  short Flags;
  int vif_idx;
  int sock;
  __u16 pif_idx;
};

#if 0//move to igmp_mld_utility.h
struct mld_hdr {
	struct icmp6hdr mld_icmp6_hdr;
	struct in6_addr	mld_addr;
};
#endif

//#define mld_type mld_icmp6_hdr.icmp6_type
//#define mld_code mld_icmp6_hdr.icmp6_code
//#define mld_maxdelay mld_icmp6_hdr.icmp6_maxdelay

/* ifvc.c
 */
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
#define MAX_IF		MAXWAN+1
#else
#define MAX_IF         MAXVIFS+8     // max. number of interfaces recognized 
#endif
#define NO_VIF -1
int buildIfVc( void );
void print_vif_ip(void);
void print_ips(struct ifconf *ifc);

struct IfDesc *getIfByName( const char *IfName );
#if 0 //not used yet
struct IfDesc *getIfByIx( unsigned Ix );
#endif

/* mroute-api.c
 */
struct MRouteDesc {
  struct in6_addr OriginAdr;
  // Kaohj
  struct in6_addr SubsAdr; // addr of the subscriber
  struct in6_addr McAdr;
  short InVif;
  uint8 TtlVc[ MAX_MC_VIFS ];
};

// MLD socket as interface for the mrouted API
// - receives the MLD messages

extern int MLD_Socket;

int enableMRouter( void );
void disableMRouter( void );
int addVIF( struct IfDesc *Dp );
int addMRoute( struct MRouteDesc * Dp );
int delMRoute( struct MRouteDesc * Dp );
#if !defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
int getVifIx( struct IfDesc *IfDp );
#endif
struct IfDesc *getIfbyVif(int vif);
#if defined(CONFIG_MLDPROXY_MULTIWAN)
int getOutVif(int in_vif, int out_index);
#else
int getOutVif(int in_vif);
#endif

/* ipc.c
 */
int initIpcServer( void );
struct CmdPkt *readIpcServer( uint8 Bu[], int BuSz );
int initIpcClient( void );
int  sendIpc( const void *Bu, int Sz );
int  readIpc( uint8 Bu[], int BuSz );
void cleanIpc( void );

/* cmdpkt.c
 */
struct CmdPkt {
  unsigned PktSz;
  uint16   Cmd;
  uint16   ParCn;  
  // 'ParCn' * '\0' terminated strings + '\0'
};

#define MX_CMDPKT_SZ 1024   // CmdPkt size including appended strings

void *buildCmdPkt( char Cmd, const char *ArgVc[], int ParCn );
const char *convCmdPkt2MRouteDesc( struct MRouteDesc *MrDp, const struct CmdPkt *PktPt );

/* lib.c
 */
char *fmtInAdr( char *St, struct in6_addr InAdr );
char *fmtSockAdr( char *St, const struct sockaddr_in *SockAdrPt );
int getInAdr( uint32 *InAdrPt, uint16 *PortPt, char *St );

/* mcgroup.c
 */
int joinMcGroup( int UdpSock, const char *IfName, struct in_addr McAdr );
int leaveMcGroup( int UdpSock, const char *IfName, struct in_addr McAdr );

/* syslog.c
 */
 #if 1
extern int  Log2Stderr;    // Log threshold for stderr, LOG_WARNING .... LOG_DEBUG 
extern int  LogLastServerity;     // last logged serverity
extern int  LogLastErrno;         // last logged errno value
extern char LogLastMsg[ 128 ];    // last logged message
#endif


void mld_log( int Serverity, int Errno, const char *FmtSt, ... );

/* udpsock.c
 */
int openUdpSocket( uint32 PeerInAdr, uint16 PeerPort );

#endif
