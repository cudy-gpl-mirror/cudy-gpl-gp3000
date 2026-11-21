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

#include <net/if.h>		/* ifreq struct         */

#define USE_LINUX_IN_H
#ifdef USE_LINUX_IN_H
#include <linux/in.h>
#include <linux/mroute.h>
#include <linux/ip.h>
#include <linux/igmp.h>
#else
#include <netinet/in.h>
#endif

#define CONFIG_COMPILE_IGMPV3

#define CONFIG_IGMPPROXY_MULTIWAN

#if defined(CONFIG_IGMPPROXY_MULTIWAN)
#define  MAXWAN 8
#endif



typedef u_int8_t   uint8;
typedef u_int16_t  uint16;
typedef u_int32_t  uint32;

extern char igmp_down_if_name[IFNAMSIZ];
extern char igmp_down_if_idx;

#if defined(CONFIG_IGMPPROXY_MULTIWAN) || defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
extern char igmp_up_if_name[MAXWAN][IFNAMSIZ];
#else
extern char igmp_up_if_name[IFNAMSIZ];
#endif

#if defined(CONFIG_IGMPPROXY_MULTIWAN) || defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
extern char igmp_up_if_idx[MAXWAN] ;
#else
extern char igmp_up_if_idx;
#endif

#if defined(CONFIG_IGMPPROXY_MULTIWAN) || defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
extern int igmp_up_if_num;
#endif


#define MIN( a, b ) ((a) < (b) ? (a) : (b))
#define MAX( a, b ) ((a) < (b) ? (b) : (a))
#define VCMC( Vc )  (sizeof( Vc ) / sizeof( (Vc)[ 0 ] ))
#define VCEP( Vc )  (&(Vc)[ VCMC( Vc ) ])

#define MAX_MC_VIFS    MAXVIFS     // !!! check this const in the specific includes
// #define NO_VIF_IX  MAXVIFS       /* invalid VIF index (32) */

struct IfDesc {
	char Name[ sizeof( ((struct ifreq *)NULL)->ifr_name ) ];
	struct in_addr InAdr;          /* == 0 for non IP interfaces */            
	short Flags;
	int vif_idx;
#if 1//defined (CONFIG_ISP_IGMPPROXY_MULTIWAN)
	int pif_idx;
#endif
	int sock;
};

/* ifvc.c
 */
#if defined (CONFIG_ISP_IGMPPROXY_MULTIWAN)
#define MAX_IF			MAXWAN+1
#else
#define MAX_IF         MAXVIFS+8     // max. number of interfaces recognized 
#endif
#define NO_VIF  -1

void buildIfVc( void );
struct IfDesc *getIfByName( const char *IfName );
struct IfDesc *getIfByIx( unsigned Ix );
int get_vif_by_pif(int pif_index);
struct IfDesc *getIfbyVif(int vif_index);
#if defined(CONFIG_IGMPPROXY_MULTIWAN)
int getOutVif(int in_vif, int out_index);
#else
int getOutVif(int in_vif);
#endif

/* mroute-api.c
 */
struct MRouteDesc {
  struct in_addr OriginAdr;
  // Kaohj
  struct in_addr SubsAdr; // addr of the subscriber
  struct in_addr McAdr;
  short InVif;
  uint8 TtlVc[ MAX_MC_VIFS ];
};

// IGMP socket as interface for the mrouted API
// - receives the IGMP messages
extern int MRouterFD;

int enableMRouter( void );
void disableMRouter( void );
int addVIF( struct IfDesc *Dp );
#if defined (CONFIG_ISP_IGMPPROXY_MULTIWAN)
int delVIf(int vif_idx);
unsigned long getAddrByVifIx(int ix);
#endif
int addMRoute( struct MRouteDesc * Dp );
int delMRoute( struct MRouteDesc * Dp );
int getVifIx( struct IfDesc *IfDp );

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
char *fmtInAdr( char *St, struct in_addr InAdr );
char *fmtSockAdr( char *St, const struct sockaddr_in *SockAdrPt );
int getInAdr( uint32 *InAdrPt, uint16 *PortPt, char *St );

/* mcgroup.c
 */
int joinMcGroup( int UdpSock, const char *IfName, struct in_addr McAdr );
int leaveMcGroup( int UdpSock, const char *IfName, struct in_addr McAdr );

/* syslog.c
 */
extern int  Log2Stderr;    // Log threshold for stderr, LOG_WARNING .... LOG_DEBUG 
extern int  LogLastServerity;     // last logged serverity
extern int  LogLastErrno;         // last logged errno value
extern char LogLastMsg[ 128 ];    // last logged message



void igmpproxy_log( int Serverity, int Errno, const char *FmtSt, ... );

/* udpsock.c
 */
int openUdpSocket( uint32 PeerInAdr, uint16 PeerPort );

/* due to use linux header instead of netinet header, so need porting function */
unsigned short int htons (unsigned short int x);
unsigned short int ntohs (unsigned short int x);
unsigned int htonl (unsigned int x);
unsigned int ntohl (unsigned int x);
char *inet_ntoa (struct in_addr in);
int inet_aton(const char *cp, struct in_addr *addr);
unsigned int inet_addr(const char *cp);
const char *inet_ntop (int af, const void *src, char *dst, socklen_t size);

#ifdef CONFIG_RTL_NEW_IGMP_REPORT_BEHAVIOR
#define IP_ADD_IN_MEMBERSHIP  51
#endif


#endif
