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
**  $Id: mroute-api.c,v 1.1.1.1 2007-08-06 10:04:43 root Exp $	
**
**  This module contains the interface routines to the Linux mrouted API
**
*/

#include "mclab.h"
#include "igmp_mld_debug.h"
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
//#include <linux/version.h>



// need an IGMP socket as interface for the mrouted API
// - receives the IGMP messages
int MRouterFD=0;

// my internal virtual interfaces descriptor vector  
#ifndef CONFIG_ISP_IGMPPROXY_MULTIWAN
static struct VifDesc {
  struct IfDesc *IfDp;
} VifDescVc[ MAXVIFS ];
#endif

int enableMRouter()
/*
** Initialises the mrouted API and locks it by this exclusively.
**     
** returns: - 0 if the functions succeeds     
**          - the errno value for non-fatal failure condition
*/
{
  int Va = 1;

  if (MRouterFD != 0) // already enabled
    return 0;
  
  if( (MRouterFD = socket( AF_INET, SOCK_RAW, IPPROTO_IGMP )) < 0 )
    igmpproxy_log( LOG_ERR, errno, "IGMP socket open" );
  
  if( setsockopt( MRouterFD, IPPROTO_IP, MRT_INIT, 
		  (void *)&Va, sizeof( Va ) ) ) 
    return errno;

  	if (setsockopt(MRouterFD, IPPROTO_IP, IP_PKTINFO, &Va,sizeof(Va)) < 0)
	   igmpproxy_log(LOG_ERR, errno, "setsockopt(IPV4_RECVPKTINFO)");

  //if(fcntl(MRouterFD, F_SETFL, O_NONBLOCK) == -1)
	//return errno;
	
  return 0;
}

void disableMRouter()
/*
** Diables the mrouted API and relases by this the lock.
**          
*/
{
  if( setsockopt( MRouterFD, IPPROTO_IP, MRT_DONE, NULL, 0 ) 
      || close( MRouterFD )
  ) {
    MRouterFD = 0;
    igmpproxy_log( LOG_ERR, errno, "MRT_DONE/close" );
  }
  
  MRouterFD = 0;
}
#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
int addVIF( struct IfDesc *IfDp )
/*
** Adds the interface '*IfDp' as virtual interface to the mrouted API
** 
*/
{
  	struct vifctl VifCtl;
  	struct VifDesc *VifDp;
  	struct IfDesc IfD_tmp;
	int ret;

  	//fixed vif index, bring in by IfDp

	if(strcmp(IfDp->Name, "")==0)
		return -1;

	if(IfDp->vif_idx<0 || IfDp->vif_idx>=MAXVIFS)
		return -1;
	
	memset(&VifCtl, 0, sizeof(VifCtl));

	VifCtl.vifc_vifi  = IfDp->vif_idx;
	VifCtl.vifc_flags = VIFF_USE_IFINDEX;
	VifCtl.vifc_threshold = 1;	  /* Packet TTL must be at least 1 to pass them */
	VifCtl.vifc_rate_limit = 0;   /* hopefully no limit */
	VifCtl.vifc_lcl_ifindex = IfDp->pif_idx;
	VifCtl.vifc_rmt_addr.s_addr = INADDR_ANY;


	igmpproxy_log( LOG_NOTICE, 0, "adding VIF, idx=%d Fl flags=0x%x IP=%s %s", 
	       VifCtl.vifc_vifi, VifCtl.vifc_flags,  inet_ntoa(VifCtl.vifc_lcl_addr), IfDp->Name );

	if( setsockopt( MRouterFD, IPPROTO_IP, MRT_ADD_VIF, 
			(char *)&VifCtl, sizeof( VifCtl ) ) )
	{
	  	igmpproxy_log( LOG_ERR, errno, "MRT_ADD_VIF" );
	}

	  
	return VifCtl.vifc_vifi;
}

int delVIf(int vif_idx)
{
	struct vifctl VifCtl;
	memset(&VifCtl, 0, sizeof(struct vifctl));
	
	if(vif_idx <0 || vif_idx>=MAXVIFS)
		return -1;

	VifCtl.vifc_vifi  = vif_idx;
	if( setsockopt( MRouterFD, IPPROTO_IP, MRT_DEL_VIF, 
			(char *)&VifCtl, sizeof( VifCtl ) ) )
	{
	  	igmpproxy_log( LOG_ERR, errno, "MRT_DEL_VIF" );
	}
	return 0;
}

#else
int addVIF( struct IfDesc *IfDp )
/*
** Adds the interface '*IfDp' as virtual interface to the mrouted API
** 
*/
{
  struct vifctl VifCtl;
  struct VifDesc *VifDp;

	/*check if IfDp has beed added*/
	 for( VifDp = VifDescVc; VifDp < VCEP( VifDescVc ); VifDp++ ) {
	 	//printf("%s: name=%s\n",__FUNCTION__,VifDp->IfDp->Name);
		if( VifDp->IfDp  && strcmp(VifDp->IfDp->Name,IfDp->Name)==0)
			return VifDp - VifDescVc; 
	}
  /* search free VifDesc
   */
  for( VifDp = VifDescVc; VifDp < VCEP( VifDescVc ); VifDp++ ) {
    if( ! VifDp->IfDp )
      break;
  }
    
  /* no more space
   */
  if( VifDp >= VCEP( VifDescVc ) )
  {
		igmpproxy_log( LOG_ERR, ENOMEM, "addVIF, out of VIF space" );
		return -1;
  }

  VifDp->IfDp = IfDp;

  VifCtl.vifc_vifi  = VifDp - VifDescVc; 
  VifCtl.vifc_flags = 0;        /* no tunnel, no source routing, register ? */
  VifCtl.vifc_threshold = 1;    /* Packet TTL must be at least 1 to pass them */
  VifCtl.vifc_rate_limit = 0;   /* hopefully no limit */
  VifCtl.vifc_lcl_addr.s_addr = VifDp->IfDp->InAdr.s_addr;
  VifCtl.vifc_rmt_addr.s_addr = INADDR_ANY;

  igmpproxy_log( LOG_NOTICE, 0, "adding VIF, idx=%d Fl flags=0x%x IP=%x %s", 
       VifCtl.vifc_vifi, VifCtl.vifc_flags,  VifCtl.vifc_lcl_addr, VifDp->IfDp->Name );

  printf( "adding VIF, idx=%d Fl flags=0x%x IP=%x %s", 
       VifCtl.vifc_vifi, VifCtl.vifc_flags,  VifCtl.vifc_lcl_addr.s_addr, VifDp->IfDp->Name );

  if( setsockopt( MRouterFD, IPPROTO_IP, MRT_ADD_VIF, 
		  (char *)&VifCtl, sizeof( VifCtl ) ) )
  {
    igmpproxy_log( LOG_WARNING, errno, "MRT_ADD_VIF" );
	return -1;
	
  }
  
  IfDp->vif_idx = VifCtl.vifc_vifi;
  
  return VifCtl.vifc_vifi;
}
#endif

/*
** Add and update the multicast routed '*Dp' to the kernel routes
**
** returns: - 0 if the function succeeds
**          - the errno value for non-fatal failure condition
*/
int addMRoute(struct MRouteDesc *Dp)
{
  	struct mfcctl CtlReq;
  
  	CtlReq.mfcc_origin    = Dp->OriginAdr;
  	CtlReq.mfcc_mcastgrp  = Dp->McAdr;
  	CtlReq.mfcc_parent    = Dp->InVif;


	/* copy the TTL vector*/
  	if(sizeof( CtlReq.mfcc_ttls ) != sizeof( Dp->TtlVc ) 
      || VCMC( CtlReq.mfcc_ttls ) != VCMC( Dp->TtlVc ))
    	igmpproxy_log( LOG_ERR, 0, "data types doesn't match in " __FILE__ ", source adaption needed !" );

 	 memcpy( CtlReq.mfcc_ttls, Dp->TtlVc, sizeof( CtlReq.mfcc_ttls ) );

  	{
    	char FmtBuO[ 32 ], FmtBuM[ 32 ];

    	igmpproxy_log( LOG_DEBUG, 0, "adding MFC: %s -> %s, InpVIf: %d", 
	    	fmtInAdr( FmtBuO, CtlReq.mfcc_origin ), 
	    	fmtInAdr( FmtBuM, CtlReq.mfcc_mcastgrp ),
	    	CtlReq.mfcc_parent == ALL_VIFS ? -1 : CtlReq.mfcc_parent
	    );
  	}

  	if(setsockopt( MRouterFD, IPPROTO_IP, MRT_ADD_MFC,
		  (void *)&CtlReq, sizeof( CtlReq )))
  	{
    	igmpproxy_log( LOG_WARNING, errno, "MRT_ADD_MFC" );
		return errno;
  	}
  
   	DBG_IGMP_MLD_PRK(IGMP_MFC, "add MFC successfully!\n");
  	return 1;
}

/*
** Removes the multicast routed '*Dp' from the kernel routes
**
** returns: - 0 if the function succeeds
**          - the errno value for non-fatal failure condition
*/

int delMRoute(struct MRouteDesc *Dp)
{
  	struct mfcctl CtlReq;
  
  	CtlReq.mfcc_origin    = Dp->OriginAdr;
  	CtlReq.mfcc_mcastgrp  = Dp->McAdr;
  	CtlReq.mfcc_parent    = Dp->InVif;

  /* clear the TTL vector
   */
  	memset( CtlReq.mfcc_ttls, 0, sizeof( CtlReq.mfcc_ttls ) );
  	{
    	char FmtBuO[ 32 ], FmtBuM[ 32 ];

    	igmpproxy_log( LOG_NOTICE, 0, "removing MFC: %s -> %s, InpVIf: %d", 
	    	fmtInAdr( FmtBuO, CtlReq.mfcc_origin ), 
	    	fmtInAdr( FmtBuM, CtlReq.mfcc_mcastgrp ),
	    	CtlReq.mfcc_parent == ALL_VIFS ? -1 : CtlReq.mfcc_parent);
  	}

  	if(setsockopt( MRouterFD, IPPROTO_IP, MRT_DEL_MFC,(void *)&CtlReq, sizeof( CtlReq )))
  	{
  		if(errno != ENOENT)
  		{
    		igmpproxy_log( LOG_WARNING, errno, "MRT_DEL_MFC");
			return errno;
  		}
  	}

	DBG_IGMP_MLD_PRK(IGMP_MFC, "remove MFC successfully!\n");

	return 1;
}

#ifndef CONFIG_ISP_IGMPPROXY_MULTIWAN
int getVifIx( struct IfDesc *IfDp )
/*
** Returns for the virtual interface index for '*IfDp'
**
** returns: - the vitrual interface index if the interface is registered
**          - -1 if no virtual interface exists for the interface 
**          
*/
{
  struct VifDesc *Dp;

  for( Dp = VifDescVc; Dp < VCEP( VifDescVc ); Dp++ ) 
    if( Dp->IfDp == IfDp )
      return Dp - VifDescVc;

  return -1;
}

unsigned long getAddrByVifIx(int ix)
{
	if(ix >= MAXVIFS)
		return 0;
	return VifDescVc[ix].IfDp->InAdr.s_addr;
}
#endif

#define NS_IN6ADDRSZ        16        /*%< IPv6 T_AAAA */
#define NS_INT16SZ        2        /*%< #/bytes of data in a uint16_t */
#define __set_errno(val) (errno = (val))
#ifdef SPRINTF_CHAR
#define SPRINTF(x) strlen(sprintf/**/x)
#else
#define SPRINTF(x) ((size_t)sprintf x)
#endif


unsigned short int
htons (unsigned short int x)
{
#if BYTE_ORDER == BIG_ENDIAN
  return x;
#elif BYTE_ORDER == LITTLE_ENDIAN
  return __bswap_16 (x);
#else
# error "What kind of system is this?"
#endif
}

unsigned short int
ntohs (unsigned short int x)
{
#if BYTE_ORDER == BIG_ENDIAN
  return x;
#elif BYTE_ORDER == LITTLE_ENDIAN
  return __bswap_16 (x);
#else
# error "What kind of system is this?"
#endif
}

unsigned int
htonl (unsigned int x)
{
#if BYTE_ORDER == BIG_ENDIAN
  return x;
#elif BYTE_ORDER == LITTLE_ENDIAN
  return __bswap_32 (x);
#else
# error "What kind of system is this?"
#endif
}

unsigned int
ntohl (unsigned int x)
{
#if BYTE_ORDER == BIG_ENDIAN
  return x;
#elif BYTE_ORDER == LITTLE_ENDIAN
  return __bswap_32 (x);
#else
# error "What kind of system is this?"
#endif
}

static char buffer[18];
char *
inet_ntoa (struct in_addr in)
{
  unsigned char *bytes = (unsigned char *) &in;
  snprintf (buffer, sizeof (buffer), "%d.%d.%d.%d",
              bytes[0], bytes[1], bytes[2], bytes[3]);

  return buffer;
}

/*
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 */
int
inet_aton(const char *cp, struct in_addr *addr)
{
        static const unsigned int max[4] = { 0xffffffff, 0xffffff, 0xffff, 0xff };
        unsigned int val;
        char c;
        union iaddr {
          unsigned char bytes[4];
          unsigned int word;
        } res;
        unsigned char *pp = res.bytes;
        int digit;

        int saved_errno = errno;
        __set_errno (0);

        res.word = 0;

        c = *cp;
        for (;;) {
                /*
                 * Collect number up to ``.''.
                 * Values are specified as for C:
                 * 0x=hex, 0=octal, isdigit=decimal.
                 */
                if (!isdigit(c))
                        goto ret_0;
                {
                        char *endp;
                        unsigned long ul = strtoul (cp, (char **) &endp, 0);
                        if (ul == ULONG_MAX && errno == ERANGE)
                                goto ret_0;
                        if (ul > 0xfffffffful)
                                goto ret_0;
                        val = ul;
                        digit = cp != endp;
                        cp = endp;
                }
                c = *cp;
                if (c == '.') {
                        /*
                         * Internet format:
                         *        a.b.c.d
                         *        a.b.c        (with c treated as 16 bits)
                         *        a.b        (with b treated as 24 bits)
                         */
                        if (pp > res.bytes + 2 || val > 0xff)
                                goto ret_0;
                        *pp++ = val;
                        c = *++cp;
                } else
                        break;
        }
        /*
         * Check for trailing characters.
         */
        if (c != '\0' && (!isascii(c) || !isspace(c)))
                goto ret_0;
        /*
         * Did we get a valid digit?
         */
        if (!digit)
                goto ret_0;

        /* Check whether the last part is in its limits depending on
           the number of parts in total.  */
        if (val > max[pp - res.bytes])
          goto ret_0;

        if (addr != NULL)
                addr->s_addr = res.word | htonl (val);

        __set_errno (saved_errno);
        return (1);

ret_0:
        __set_errno (saved_errno);
        return (0);
}

/*
 * Ascii internet address interpretation routine.
 * The value returned is in network order.
 */
unsigned int
inet_addr(const char *cp) {
        struct in_addr val;

        if (inet_aton(cp, &val))
                return (val.s_addr);
        return (INADDR_NONE);
}

/* const char *
 * inet_ntop4(src, dst, size)
 *        format an IPv4 address
 * return:
 *        `dst' (as a const)
 * notes:
 *        (1) uses no statics
 *        (2) takes a u_char* not an in_addr as input
 * author:
 *        Paul Vixie, 1996.
 */
static const char *
inet_ntop4 (const u_char *src, char *dst, socklen_t size)
{
        static const char fmt[] = "%u.%u.%u.%u";
        char tmp[sizeof "255.255.255.255"];

        if (SPRINTF((tmp, fmt, src[0], src[1], src[2], src[3])) >= size) {
                __set_errno (ENOSPC);
                return (NULL);
        }
        return strncpy(dst, tmp,size);
}

/* const char *
 * inet_ntop6(src, dst, size)
 *        convert IPv6 binary address into presentation (printable) format
 * author:
 *        Paul Vixie, 1996.
 */
static const char *
inet_ntop6 (const u_char *src, char *dst, socklen_t size)
{
        /*
         * Note that int32_t and int16_t need only be "at least" large enough
         * to contain a value of the specified size.  On some systems, like
         * Crays, there is no such thing as an integer variable with 16 bits.
         * Keep this in mind if you think this function should have been coded
         * to use pointer overlays.  All the world's not a VAX.
         */
        char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
        struct { int base, len; } best, cur;
        u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
        int i;

        /*
         * Preprocess:
         *        Copy the input (bytewise) array into a wordwise array.
         *        Find the longest run of 0x00's in src[] for :: shorthanding.
         */
        memset(words, '\0', sizeof words);
        for (i = 0; i < NS_IN6ADDRSZ; i += 2)
                words[i / 2] = (src[i] << 8) | src[i + 1];
        best.base = -1;
        cur.base = -1;
        best.len = 0;
        cur.len = 0;
        for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
                if (words[i] == 0) {
                        if (cur.base == -1)
                                cur.base = i, cur.len = 1;
                        else
                                cur.len++;
                } else {
                        if (cur.base != -1) {
                                if (best.base == -1 || cur.len > best.len)
                                        best = cur;
                                cur.base = -1;
                        }
                }
        }
        if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                        best = cur;
        }
        if (best.base != -1 && best.len < 2)
                best.base = -1;

        /*
         * Format the result.
         */
        tp = tmp;
        for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
                /* Are we inside the best run of 0x00's? */
                if (best.base != -1 && i >= best.base &&
                    i < (best.base + best.len)) {
                        if (i == best.base)
                                *tp++ = ':';
                        continue;
                }
                /* Are we following an initial run of 0x00s or any real hex? */
                if (i != 0)
                        *tp++ = ':';
                /* Is this address an encapsulated IPv4? */
                if (i == 6 && best.base == 0 &&
                    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
                        if (!inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
                                return (NULL);
                        tp += strlen(tp);
                        break;
                }
                tp += SPRINTF((tp, "%x", words[i]));
        }
        /* Was it a trailing run of 0x00's? */
        if (best.base != -1 && (best.base + best.len) ==
            (NS_IN6ADDRSZ / NS_INT16SZ))
                *tp++ = ':';
        *tp++ = '\0';

        /*
         * Check for overflow, copy, and we're done.
         */
        if ((socklen_t)(tp - tmp) > size) {
                __set_errno (ENOSPC);
                return (NULL);
        }
        return strncpy(dst, tmp,size);
}

/* char *
 * inet_ntop(af, src, dst, size)
 *        convert a network format address to presentation format.
 * return:
 *        pointer to presentation format address (`dst'), or NULL (see errno).
 * author:
 *        Paul Vixie, 1996.
 */
const char *
inet_ntop (int af, const void *src, char *dst, socklen_t size)
{
        switch (af) {
        case AF_INET:
                return (inet_ntop4(src, dst, size));
        case AF_INET6:
                return (inet_ntop6(src, dst, size));
        default:
                __set_errno (EAFNOSUPPORT);
                return (NULL);
        }
        /* NOTREACHED */
}


