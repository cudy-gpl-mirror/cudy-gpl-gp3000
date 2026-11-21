  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* TCP control file							*/

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License with
  the Debian GNU/Linux distribution in file /usr/doc/copyright/GPL;
  if not, write to the Free Software Foundation, Inc., 59 Temple Place,
  Suite 330, Boston, MA  02111-1307  USA
*/

#include "axel.h"

/*
 *	Addr type
 *	
 *	type	-	unicast | multicast
 *	scope	-	local	| site	    | global
 *	v4	-	compat
 *	v4mapped
 *	any
 *	loopback
 */
#define IPV6_ADDR_ANY		0x0000U
#define IPV6_ADDR_UNICAST      	0x0001U	
#define IPV6_ADDR_MULTICAST    	0x0002U	
#define IPV6_ADDR_LOOPBACK	0x0010U
#define IPV6_ADDR_LINKLOCAL	0x0020U
#define IPV6_ADDR_SITELOCAL	0x0040U
#define IPV6_ADDR_COMPATv4	0x0080U
#define IPV6_ADDR_SCOPE_MASK	0x00f0U
#define IPV6_ADDR_MAPPED	0x1000U
/*
 *	Addr scopes
 */
#define IPV6_ADDR_MC_SCOPE(a)	\
	((a)->s6_addr[1] & 0x0f)	/* nonstandard */
#define __IPV6_ADDR_SCOPE_INVALID	-1
#define IPV6_ADDR_SCOPE_NODELOCAL	0x01
#define IPV6_ADDR_SCOPE_LINKLOCAL	0x02
#define IPV6_ADDR_SCOPE_SITELOCAL	0x05
#define IPV6_ADDR_SCOPE_ORGLOCAL	0x08
#define IPV6_ADDR_SCOPE_GLOBAL		0x0e

#define IPV6_ADDR_SCOPE_TYPE(scope)	((scope) << 16)
static inline unsigned int ipv6_addr_scope2type(unsigned int scope)
{
	switch (scope) {
	case IPV6_ADDR_SCOPE_NODELOCAL:
		return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_NODELOCAL) |
			IPV6_ADDR_LOOPBACK);
	case IPV6_ADDR_SCOPE_LINKLOCAL:
		return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL) |
			IPV6_ADDR_LINKLOCAL);
	case IPV6_ADDR_SCOPE_SITELOCAL:
		return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_SITELOCAL) |
			IPV6_ADDR_SITELOCAL);
	}
	return IPV6_ADDR_SCOPE_TYPE(scope);
}

int ipv6_addr_type(const struct in6_addr *addr)
{
	unsigned int st;

	st = addr->s6_addr32[0];

	/* Consider all addresses with the first three bits different of
	   000 and 111 as unicasts.
	 */
	if ((st & htonl(0xE0000000)) != htonl(0x00000000) &&
	    (st & htonl(0xE0000000)) != htonl(0xE0000000))
		return (IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));

	if ((st & htonl(0xFF000000)) == htonl(0xFF000000)) {
		/* multicast */
		/* addr-select 3.1 */
		return (IPV6_ADDR_MULTICAST |
			ipv6_addr_scope2type(IPV6_ADDR_MC_SCOPE(addr)));
	}

	if ((st & htonl(0xFFC00000)) == htonl(0xFE800000))
		return (IPV6_ADDR_LINKLOCAL | IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL));		/* addr-select 3.1 */
	if ((st & htonl(0xFFC00000)) == htonl(0xFEC00000))
		return (IPV6_ADDR_SITELOCAL | IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_SITELOCAL));		/* addr-select 3.1 */
	if ((st & htonl(0xFE000000)) == htonl(0xFC000000))
		return (IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));			/* RFC 4193 */

	if ((addr->s6_addr32[0] | addr->s6_addr32[1]) == 0) {
		if (addr->s6_addr32[2] == 0) {
			if (addr->s6_addr32[3] == 0)
				return IPV6_ADDR_ANY;

			if (addr->s6_addr32[3] == htonl(0x00000001))
				return (IPV6_ADDR_LOOPBACK | IPV6_ADDR_UNICAST |
					IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL));	/* addr-select 3.4 */

			return (IPV6_ADDR_COMPATv4 | IPV6_ADDR_UNICAST |
				IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));	/* addr-select 3.3 */
		}

		if (addr->s6_addr32[2] == htonl(0x0000ffff))
			return (IPV6_ADDR_MAPPED |
				IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));	/* addr-select 3.3 */
	}

	return (IPV6_ADDR_UNICAST |
		IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));	/* addr-select 3.4 */
}

#if defined(CONFIG_CU) && defined(CONFIG_GMAC1_USABLE) && defined(CONFIG_RTL9607C_SERIES)
static int axel_port = 0;
#define AXLE_START_PORT_NUMBER 32000 
int PortIsUsed(int port)
{
    int fd;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1)
		return 1;
	
	memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(bind(fd, (struct sockaddr *)(&addr), sizeof(struct sockaddr_in)) < 0)
    {
        close(fd);
        fprintf(stderr,"port %d has been used.\n", port);
        return 1;
    }
    close(fd);
    return 0;
}

int get_continuous_port_start()
{
	int i, found=0;
	int count=0, thread_num=0;

	thread_num = get_speedtest_thread_number();
	for(i=AXLE_START_PORT_NUMBER; i<65535; i++)
	{
		if(PortIsUsed(i) == 0)
			count++;
		else 
			count = 0;

		if(count == (thread_num+1))
		{
			found=1;
			break;
		}
	}

	if(found == 1)
		return (i-thread_num+1);

	return 0;
}
#endif

/* Get a TCP connection */
int tcp_connect( char *hostname, int port, char *local_if )
{
	struct addrinfo hints;
    struct addrinfo *res=NULL,*rp;
	struct ifaddrs *ifaddr=NULL, *ifa;
	int ret;
	int fd;
	char tcp_port_str[8];
	char serverhost[MAX_STRING];
	int recvbuflen=1024*1024;
	int startPort=0;

	if((hostname == NULL) || (strlen(hostname) >= sizeof(serverhost)))
		return -1;
	memset(serverhost,0,sizeof(serverhost));
	if((hostname[0]=='[') && (hostname[strlen(hostname)-1]==']'))//paser ipv6 addr
		strncpy(serverhost,hostname+1,strlen(hostname)-2);
	else
		snprintf(serverhost, sizeof(serverhost), "%s", hostname);

	sprintf(tcp_port_str, "%d", port);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_ADDRCONFIG;

    if(ret = getaddrinfo(serverhost, tcp_port_str, &hints, &res))
    {
		fprintf(stderr, "[axel]getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
    }
	if( local_if && *local_if )
	{
		if (getifaddrs(&ifaddr)) 
		{
			perror("[axel]getifaddrs");
			freeaddrinfo(res);
			return -1;		
		}
	}

#if defined(CONFIG_CU) && defined(CONFIG_GMAC1_USABLE) && defined(CONFIG_RTL9607C_SERIES)
	if(get_trap_hash_enable() == 1)
	{
		if(get_axel_start_port() == 0)
		{
			startPort = get_continuous_port_start();
			if(startPort)
			{
				set_axel_start_port(startPort);
				axel_port = startPort;
			}
			fprintf(stderr, "<%s, %d, axel_port=%d>\n", __FUNCTION__, __LINE__, axel_port);
		}
	}
#endif
	
	for(rp=res; rp!=NULL; rp=rp->ai_next)
	{
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(fd==-1)
			continue;
#if 0 //remove it because it make tcp window scale smaller
		if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&recvbuflen,sizeof(recvbuflen)) < 0)
			perror("[axel]setsockopt");
#endif

		if( local_if && *local_if )
		{
			for(ifa=ifaddr; ifa!=NULL; ifa=ifa->ifa_next)
			{
				if((ifa->ifa_addr!=NULL) && (ifa->ifa_addr->sa_family == rp->ai_family) && (strcmp(local_if,ifa->ifa_name)==0))
				{
					int addrlen;
					if(rp->ai_family==AF_INET6)
					{
						struct sockaddr_in6 *if_addr6 = (struct sockaddr_in6*)(ifa->ifa_addr);
						struct sockaddr_in6 *ip_addr6 = (struct sockaddr_in6*)(rp->ai_addr);
						addrlen = sizeof(struct sockaddr_in6);
						if(ipv6_addr_type(&(if_addr6->sin6_addr))!= ipv6_addr_type(&(ip_addr6->sin6_addr)))
							continue;
					}
					else
						addrlen = sizeof(struct sockaddr_in);

#if defined(CONFIG_CU) && defined(CONFIG_GMAC1_USABLE) && defined(CONFIG_RTL9607C_SERIES)
					if(get_trap_hash_enable() == 1)
						((struct sockaddr_in *)(ifa->ifa_addr))->sin_port = axel_port++;
#endif
					if( bind( fd, ifa->ifa_addr, addrlen ) == -1 )
						continue;
					if( connect( fd, rp->ai_addr, rp->ai_addrlen ) == 0 )
						goto CONNECT_SUCC;
					else
						perror("[axel]connect");
				}
			}
		}
		else
		{
			if( connect( fd, rp->ai_addr, rp->ai_addrlen ) == 0 )
				goto CONNECT_SUCC;
			else
				perror("[axel]connect");
			
		}
		close(fd);
	}
	freeaddrinfo(res);
	if(ifaddr)
		freeifaddrs(ifaddr);
	return -1;
CONNECT_SUCC:
#if 0
	{
		char ip_str[64];
		if( local_if && *local_if )
		{
			if(rp->ai_family==AF_INET6)
				inet_ntop(AF_INET6,&(((struct sockaddr_in6 *)(ifa->ifa_addr))->sin6_addr),ip_str,sizeof(ip_str));
			else
				inet_ntop(AF_INET,&(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr),ip_str,sizeof(ip_str));
			fprintf(stderr, "[axel]bind local addr %s!\n",ip_str);
		}
		if(rp->ai_family==AF_INET6)
			inet_ntop(AF_INET6,&(((struct sockaddr_in6 *)(rp->ai_addr))->sin6_addr),ip_str,sizeof(ip_str));
		else
			inet_ntop(AF_INET,&(((struct sockaddr_in *)(rp->ai_addr))->sin_addr),ip_str,sizeof(ip_str));
		fprintf(stderr, "[axel]connect %s(%s):%d success!\n",hostname,ip_str,port);
	}
#endif
#ifdef CONFIG_RTK_HOST_SPEEDUP
	{
		char ip[64] = {0};
		FILE *fp = NULL;
		FILE *fp1 = NULL;
		if(fp1 = fopen("/proc/HostSpeedUP", "r"))
		{
			int speedup=0;
			if((fscanf(fp1, "HostSpeedUP=%d", &speedup)==1)&&(speedup==1))
			{
				char lip[64];
				if(rp->ai_family == AF_INET)
				{
					sprintf(lip,"0.0.0.0");
					inet_ntop(AF_INET,&(((struct sockaddr_in *)(rp->ai_addr))->sin_addr),ip,sizeof(ip));
				}
				else
				{
					sprintf(lip,"::");
					inet_ntop(AF_INET6,&(((struct sockaddr_in6 *)(rp->ai_addr))->sin6_addr),ip,sizeof(ip));
				}
				//fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m host=%s, port=%d \n", __FILE__, __FUNCTION__, __LINE__, ip, port);
	
				fp = fopen("/proc/HostSpeedUP-Info", "w");
				if (fp) {
					fprintf(fp, "add rip %s rport %d lip %s lport %d\n", ip, 0, lip, 0);
					fclose(fp);
				}
	
				setSpeedupSackAttr(fd, ip, lip);
#if 0
				char cmd[32] = {0};
				sprintf(cmd, "echo 0 > /proc/fc/ctrl/hwnat");
				system(cmd);
#endif
			}
			fclose(fp1);
		}
	}
#endif

	freeaddrinfo(res);
	if(ifaddr)
		freeifaddrs(ifaddr);
	return fd;

#if 0
	struct hostent *host = NULL;
	struct sockaddr_in addr;
	struct sockaddr_in local;
	int fd;

#ifdef DEBUG
	socklen_t i = sizeof( local );
	
	fprintf( stderr, "tcp_connect( %s, %i ) = ", hostname, port );
#endif
	
	/* Why this loop? Because the call might return an empty record.
	   At least it very rarely does, on my system...		*/
	for( fd = 0; fd < 5; fd ++ )
	{
		if( ( host = gethostbyname( hostname ) ) == NULL )
		{		
#ifdef CONFIG_USER_CTMANAGEDEAMON
			fprintf(stderr, "gethostbyname() fail, h_errno=%d\n",h_errno);
			if (h_errno == HOST_NOT_FOUND)
				write_BUCPE_test_error(eBUCPE_DNSError_NoSuchDomain);
			else if (h_errno == NO_ADDRESS)
				write_BUCPE_test_error(eBUCPE_DNSError_NoSuchDomain);
			else
				write_BUCPE_test_error(eBUCPE_DNSError_Timeout);
			continue;
#endif
			return( -1 );
		}
		if( *host->h_name ) break;
	}
	if( !host || !host->h_name || !*host->h_name )
		return( -1 );
	
	if( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
		return( -1 );
	
	if( local_if && *local_if )
	{
		local.sin_family = AF_INET;
		local.sin_port = 0;
		local.sin_addr.s_addr = inet_addr( local_if );
		if( bind( fd, (struct sockaddr *) &local, sizeof( struct sockaddr_in ) ) == -1 )
		{
			close( fd );
			return( -1 );
		}
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons( port );
	addr.sin_addr = *( (struct in_addr *) host->h_addr );

	if( connect( fd, (struct sockaddr *) &addr, sizeof( struct sockaddr_in ) ) == -1 )
	{
		
#ifdef CONFIG_USER_CTMANAGEDEAMON
			fprintf(stderr, "connect() fail, errno=%d\n",errno);
			if(errno ==  ETIMEDOUT)
				write_BUCPE_test_error(eBUCPE_NetError_Timeout);
			else if(errno == EHOSTUNREACH)
				write_BUCPE_test_error(eBUCPE_NetError_DestAddrUnreachable);
			else if(errno == ENETUNREACH)
				write_BUCPE_test_error(eBUCPE_NetError_DestAddrUnreachable);
			else if(errno == ECONNREFUSED)
				write_BUCPE_test_error(eBUCPE_NetError_DestPortUnreachable);
#endif
		close( fd );
		return( -1 );
	}
	
#ifdef CONFIG_RTK_HOST_SPEEDUP
	char ip[64] = {0};
	FILE *fp = NULL;
	FILE *fp1 = NULL;
	if(fp1 = fopen("/proc/HostSpeedUP", "r"))
	{
		int speedup=0;
		if((fscanf(fp1, "HostSpeedUP=%d", &speedup)==1)&&(speedup==1))
		{
	strcpy(ip, inet_ntoa(addr.sin_addr));
	//fprintf(stderr, "\033[1;31m[%s:%s@%d]\033[0m host=%s, port=%d \n", __FILE__, __FUNCTION__, __LINE__, ip, port);

	fp = fopen("/proc/HostSpeedUP-Info", "w");
	if (fp) {
		fprintf(fp, "add rip %s rport %d lip %s lport %d\n", ip, 0, "0.0.0.0", 0);
		fclose(fp);
	}

	setSpeedupSackAttr(fd, ip);
#if 0
	char cmd[32] = {0};
	sprintf(cmd, "echo 2 > /proc/rg/hwnat");
	system(cmd);
#endif
		}
		fclose(fp1);
	}
#endif

#ifdef DEBUG
	getsockname( fd, &local, &i );
	fprintf( stderr, "%i\n", ntohs( local.sin_port ) );
#endif
	
	return( fd );
#endif
}

int get_if_ip( char *iface, char *ip )
{
	struct ifreq ifr;
	int fd = socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );

	if (fd < 0)
		return 0;
	
	memset( &ifr, 0, sizeof( struct ifreq ) );
	strcpy( ifr.ifr_name, iface );
	ifr.ifr_addr.sa_family = AF_INET;
	if( ioctl( fd, SIOCGIFADDR, &ifr ) == 0 )
	{
		struct sockaddr_in *x = (struct sockaddr_in *) &ifr.ifr_addr;
		strcpy( ip, inet_ntoa( x->sin_addr ) );
		close(fd);
		return( 1 );
	}
	else
	{
		close(fd);
		return( 0 );
	}
}
