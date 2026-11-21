/* dnsmasq is Copyright (c) 2000-2021 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991, or
   (at your option) version 3 dated 29 June, 2007.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
     
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dnsmasq.h"

#ifdef CONFIG_CMCC
extern unsigned char is_dpi_test;
#endif
static struct frec *lookup_frec(unsigned short id, int fd, void *hash);
static struct frec *lookup_frec_by_query(void *hash, unsigned int flags);

static unsigned short get_id(void);
static void free_frec(struct frec *f);
static void query_full(time_t now);
#if defined(DNSQUERY_PORT_BINDING)
int rtk_lannetinfo_get_ifname_by_macaddr(const char *brname, unsigned char *macaddr, char *ifname);
unsigned char rtk_get_logic_port_by_ifname(unsigned char *ifName);
int rtk_check_mac_exist_in_bridge(unsigned char *mac);
int get_phyport_index_by_IP(union mysockaddr *udpaddr);
#endif

/* Send a UDP packet with its source address set as "source" 
   unless nowild is true, when we just send it with the kernel default */
int send_from(int fd, int nowild, char *packet, size_t len, 
	      union mysockaddr *to, union all_addr *source,
	      unsigned int iface)
{
  struct msghdr msg;
  struct iovec iov[1]; 
  union {
    struct cmsghdr align; /* this ensures alignment */
#if defined(HAVE_LINUX_NETWORK)
    char control[CMSG_SPACE(sizeof(struct in_pktinfo))];
#elif defined(IP_SENDSRCADDR)
    char control[CMSG_SPACE(sizeof(struct in_addr))];
#endif
    char control6[CMSG_SPACE(sizeof(struct in6_pktinfo))];
  } control_u;
  
  iov[0].iov_base = packet;
  iov[0].iov_len = len;

  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;
  msg.msg_name = to;
  msg.msg_namelen = sa_len(to);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  
  if (!nowild)
    {
      struct cmsghdr *cmptr;
      msg.msg_control = &control_u;
      msg.msg_controllen = sizeof(control_u);
      cmptr = CMSG_FIRSTHDR(&msg);

      if (to->sa.sa_family == AF_INET)
	{
#if defined(HAVE_LINUX_NETWORK)
	  struct in_pktinfo p;
	  p.ipi_ifindex = 0;
	  p.ipi_spec_dst = source->addr4;
	  msg.msg_controllen = CMSG_SPACE(sizeof(struct in_pktinfo));
	  memcpy(CMSG_DATA(cmptr), &p, sizeof(p));
	  cmptr->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
	  cmptr->cmsg_level = IPPROTO_IP;
	  cmptr->cmsg_type = IP_PKTINFO;
#elif defined(IP_SENDSRCADDR)
	  msg.msg_controllen = CMSG_SPACE(sizeof(struct in_addr));
	  memcpy(CMSG_DATA(cmptr), &(source->addr4), sizeof(source->addr4));
	  cmptr->cmsg_len = CMSG_LEN(sizeof(struct in_addr));
	  cmptr->cmsg_level = IPPROTO_IP;
	  cmptr->cmsg_type = IP_SENDSRCADDR;
#endif
	}
      else
	{
	  struct in6_pktinfo p;
	  p.ipi6_ifindex = iface; /* Need iface for IPv6 to handle link-local addrs */
	  p.ipi6_addr = source->addr6;
	  msg.msg_controllen = CMSG_SPACE(sizeof(struct in6_pktinfo));
	  memcpy(CMSG_DATA(cmptr), &p, sizeof(p));
	  cmptr->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
	  cmptr->cmsg_type = daemon->v6pktinfo;
	  cmptr->cmsg_level = IPPROTO_IPV6;
	}
    }
  
  while (retry_send(sendmsg(fd, &msg, 0)));

  if (errno != 0)
    {
#ifdef HAVE_LINUX_NETWORK
      /* If interface is still in DAD, EINVAL results - ignore that. */
      if (errno != EINVAL)
	my_syslog(LOG_ERR, _("failed to send packet: %s"), strerror(errno));
#endif
      return 0;
    }
  
  return 1;
}

#if defined(DNSQUERY_PORT_BINDING)
int port_match(int port_index, char *bindport)
{
	int serv_port=-1;
	char *delim = ",",*p;
	char bport[128]={0};

	if(!bindport || port_index == -1)
		return 0;

	memcpy(bport,bindport,strlen(bindport));

	p = strtok(bport,delim);
	serv_port = strtol(p, (char**)NULL, 0);

	if(serv_port == port_index)
		return 1;

	while(p = strtok(NULL,delim)){
		serv_port = strtol(p, (char**)NULL, 0);
		my_syslog(LOG_DEBUG, _("[%s:%d] port_index=%d, server port=%d"), __func__,__LINE__,port_index,serv_port);
		if(serv_port == port_index)
			return 1;
	}

	return 0;
}
#endif

#if defined(DNSQUERY_PORT_BINDING)
static unsigned int search_servers(time_t now, union all_addr **addrpp, unsigned int qtype, 
					char *qdomain, int *type, char **domain, int *norebind, char **bindport)
#elif defined(DNSQUERY_SKBMARK_BINDING)
static unsigned int search_servers(time_t now, union all_addr **addrpp, unsigned int qtype, 
					char *qdomain, int *type, char **domain, int *norebind, unsigned int skb_mark)
#else
static unsigned int search_servers(time_t now, union all_addr **addrpp, unsigned int qtype,
				   char *qdomain, int *type, char **domain, int *norebind)
#endif
{
  /* If the query ends in the domain in one of our servers, set
     domain to point to that name. We find the largest match to allow both
     domain.org and sub.domain.org to exist. */
  
  unsigned int namelen = strlen(qdomain);
  unsigned int matchlen = 0;
  struct server *serv;
  unsigned int flags = 0;
  static union all_addr zero;
  
  for (serv = daemon->servers; serv; serv=serv->next)
    if (qtype == F_DNSSECOK && !(serv->flags & SERV_DO_DNSSEC))
      continue;
    /* domain matches take priority over NODOTS matches */
#if defined(DNSQUERY_PORT_BINDING)
    else if ((serv->flags & SERV_FOR_NODOTS) && *type != SERV_HAS_DOMAIN && *type != SERV_HAS_BIND_PORT && !strchr(qdomain, '.') && namelen != 0)
#elif defined(DNSQUERY_SKBMARK_BINDING)
    else if ((serv->flags & SERV_FOR_NODOTS) && *type != SERV_HAS_DOMAIN && *type != SERV_HAS_BIND_SKBMARK && !strchr(qdomain, '.') && namelen != 0)
#else
    else if ((serv->flags & SERV_FOR_NODOTS) && *type != SERV_HAS_DOMAIN && !strchr(qdomain, '.') && namelen != 0)
#endif
      {
	unsigned int sflag = serv->addr.sa.sa_family == AF_INET ? F_IPV4 : F_IPV6; 
	*type = SERV_FOR_NODOTS;
	if ((serv->flags & SERV_NO_REBIND) && norebind)
	  *norebind = 1;
	else if (serv->flags & SERV_NO_ADDR)
	  flags = F_NXDOMAIN;
	else if (serv->flags & SERV_LITERAL_ADDRESS)
	  { 
	    /* literal address = '#' -> return all-zero address for IPv4 and IPv6 */
	    if ((serv->flags & SERV_USE_RESOLV) && (qtype & (F_IPV6 | F_IPV4)))
	      {
		memset(&zero, 0, sizeof(zero));
		flags = qtype;
		*addrpp = &zero;
	      }
	    else if (sflag & qtype)
	      {
		flags = sflag;
		if (serv->addr.sa.sa_family == AF_INET) 
		  *addrpp = (union all_addr *)&serv->addr.in.sin_addr;
		else
		  *addrpp = (union all_addr *)&serv->addr.in6.sin6_addr;
	      }
	    else if (!flags || (flags & F_NXDOMAIN))
	      flags = F_NOERR;
	  } 
      }
#if defined(DNSQUERY_PORT_BINDING)
    else if (serv->flags & SERV_HAS_BIND_PORT)
      {
	if(port_match(port_src,serv->bindport))
	{
		unsigned short sflag = serv->addr.sa.sa_family == AF_INET ? F_IPV4 : F_IPV6;
		*type = SERV_HAS_BIND_PORT;
		*bindport = serv->bindport;
		if (serv->flags & SERV_NO_ADDR)
			flags = F_NXDOMAIN;
		else if (serv->flags & SERV_LITERAL_ADDRESS)
		{
			if (sflag & qtype)
			{
				flags = sflag;
				if (serv->addr.sa.sa_family == AF_INET)
					*addrpp = (struct all_addr *)&serv->addr.in.sin_addr;
				else
					*addrpp = (struct all_addr *)&serv->addr.in6.sin6_addr;
			}
			else if (!flags || (flags & F_NXDOMAIN))
				flags = F_NOERR;
		}
	}
      }
#elif defined(DNSQUERY_SKBMARK_BINDING)
    else if (serv->flags & SERV_HAS_BIND_SKBMARK)
      {
	if((skb_mark & serv->mask) == serv->mark)
	{
		unsigned short sflag = serv->addr.sa.sa_family == AF_INET ? F_IPV4 : F_IPV6;
		*type = SERV_HAS_BIND_SKBMARK;
		if (serv->flags & SERV_NO_ADDR)
			flags = F_NXDOMAIN;
		else if (serv->flags & SERV_LITERAL_ADDRESS)
		{
			if (sflag & qtype)
			{
				flags = sflag;
				if (serv->addr.sa.sa_family == AF_INET)
					*addrpp = (union all_addr *)&serv->addr.in.sin_addr;
				else
					*addrpp = (union all_addr *)&serv->addr.in6.sin6_addr;
			}
			else if(!flags || (flags & F_NXDOMAIN))
				flags = F_NOERR;
		}
	}
      }
#endif
    else if (serv->flags & SERV_HAS_DOMAIN)
      {
	unsigned int domainlen = strlen(serv->domain);
	char *matchstart = qdomain + namelen - domainlen;
#ifdef CTC_DNS_TUNNEL
	unsigned char reg_match = (serv->flags & SERV_HAS_REG_DOMAIN) ? 1 : 0;
#endif

	if (
#ifdef CTC_DNS_TUNNEL
		(reg_match && hostname_reg_match(qdomain, serv->pregex)) ||
		(!reg_match && namelen >= domainlen) &&
#else
		namelen >= domainlen &&
#endif
	    hostname_isequal(matchstart, serv->domain) &&
	    (domainlen == 0 || namelen == domainlen || *(matchstart-1) == '.' ))
	  {
	    if ((serv->flags & SERV_NO_REBIND) && norebind)	
	      *norebind = 1;
	    else
	      {
		unsigned int sflag = serv->addr.sa.sa_family == AF_INET ? F_IPV4 : F_IPV6;
		/* implement priority rules for --address and --server for same domain.
		   --address wins if the address is for the correct AF
		   --server wins otherwise. */
		if (domainlen != 0 && domainlen == matchlen)
		  {
		    if ((serv->flags & SERV_LITERAL_ADDRESS))
		      {
			if (!(sflag & qtype) && flags == 0)
			  continue;
		      }
		    else
		      {
			if (flags & (F_IPV4 | F_IPV6))
			  continue;
		      }
		  }
		
		if (domainlen >= matchlen)
		  {
		    *type = serv->flags & (SERV_HAS_DOMAIN | SERV_USE_RESOLV | SERV_NO_REBIND | SERV_DO_DNSSEC);
		    *domain = serv->domain;
		    matchlen = domainlen;
		    if (serv->flags & SERV_NO_ADDR)
		      flags = F_NXDOMAIN;
		    else if (serv->flags & SERV_LITERAL_ADDRESS)
		      {
			 /* literal address = '#' -> return all-zero address for IPv4 and IPv6 */
			if ((serv->flags & SERV_USE_RESOLV) && (qtype & (F_IPV6 | F_IPV4)))
			  {			    
			    memset(&zero, 0, sizeof(zero));
			    flags = qtype;
			    *addrpp = &zero;
			  }
			else if (sflag & qtype)
			  {
			    flags = sflag;
			    if (serv->addr.sa.sa_family == AF_INET) 
			      *addrpp = (union all_addr *)&serv->addr.in.sin_addr;
			    else
			      *addrpp = (union all_addr *)&serv->addr.in6.sin6_addr;
			  }
			else if (!flags || (flags & F_NXDOMAIN))
			  flags = F_NOERR;
		      }
		    else
		      flags = 0;
		  } 
	      }
	  }
      }
  
  if (flags == 0 && !(qtype & (F_QUERY | F_DNSSECOK)) && 
      option_bool(OPT_NODOTS_LOCAL) && !strchr(qdomain, '.') && namelen != 0)
    /* don't forward A or AAAA queries for simple names, except the empty name */
    flags = F_NOERR;
  
  if (flags == F_NXDOMAIN && check_for_local_domain(qdomain, now))
    flags = F_NOERR;

  if (flags)
    {
       if (flags == F_NXDOMAIN || flags == F_NOERR)
	 log_query(flags | qtype | F_NEG | F_CONFIG | F_FORWARD, qdomain, NULL, NULL);
       else
	 {
	   /* handle F_IPV4 and F_IPV6 set on ANY query to 0.0.0.0/:: domain. */
	   if (flags & F_IPV4)
	     log_query((flags | F_CONFIG | F_FORWARD) & ~F_IPV6, qdomain, *addrpp, NULL);
	   if (flags & F_IPV6)
	     log_query((flags | F_CONFIG | F_FORWARD) & ~F_IPV4, qdomain, *addrpp, NULL);
	 }
    }
  else if ((*type) & SERV_USE_RESOLV)
    {
      *type = 0; /* use normal servers for this domain */
      *domain = NULL;
    }
  return  flags;
}

#ifdef HAVE_CONNTRACK
static void set_outgoing_mark(struct frec *forward, int fd)
{
  /* Copy connection mark of incoming query to outgoing connection. */
  unsigned int mark;
  if (get_incoming_mark(&forward->frec_src.source, &forward->frec_src.dest, 0, &mark))
    setsockopt(fd, SOL_SOCKET, SO_MARK, &mark, sizeof(unsigned int));
}
#endif

static void log_query_mysockaddr(unsigned int flags, char *name, union mysockaddr *addr, char *arg)
{
  if (addr->sa.sa_family == AF_INET)
    log_query(flags | F_IPV4, name, (union all_addr *)&addr->in.sin_addr, arg);
  else
    log_query(flags | F_IPV6, name, (union all_addr *)&addr->in6.sin6_addr, arg);
}

static void server_send(struct server *server, int fd,
			const void *header, size_t plen, int flags)
{
  while (retry_send(sendto(fd, header, plen, flags,
			   &server->addr.sa,
			   sa_len(&server->addr))));
}

#ifdef HAVE_DNSSEC
static void server_send_log(struct server *server, int fd,
			const void *header, size_t plen, int dumpflags,
			unsigned int logflags, char *name, char *arg)
{
#ifdef HAVE_DUMPFILE
	  dump_packet(dumpflags, (void *)header, (size_t)plen, NULL, &server->addr);
#endif
	  log_query_mysockaddr(logflags, name, &server->addr, arg);
	  server_send(server, fd, header, plen, 0);
}
#endif

static int server_test_type(const struct server *server,
			    const char *domain, int type, int extratype)
{
    return (type == (server->flags & (SERV_TYPE | extratype)) &&
      (type != SERV_HAS_DOMAIN || hostname_isequal(domain, server->domain)) &&
      !(server->flags & (SERV_LITERAL_ADDRESS | SERV_LOOP)));
}

#if defined(DNSQUERY_PORT_BINDING)
int rtk_lannetinfo_get_ifname_by_macaddr(const char *brname, unsigned char *macaddr, char *ifname)
{
	int br_socket_fd = -1, ret = 0;
	unsigned char macZero[6] = {0,0,0,0,0,0};

	if(brname == NULL || macaddr == NULL || ifname == NULL ||
		*brname == '\0' || memcmp(macaddr, macZero, 6) == 0)
		return 0;

	if ((br_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return -1;
	}
	else
	{
		struct __fdb_entry fdb[256];
		unsigned long args[4];
		struct ifreq ifr;
		int ifindices[256];
		int list_num = 0, i, num, offset = 0, fdb_num, found = 0;
		unsigned long list_args[4] = { BRCTL_GET_PORT_LIST, (unsigned long)ifindices, 256, 0 };
		struct ifreq list_ifr;

		memset(&ifr, 0, sizeof(ifr));
		memset(ifindices, 0, sizeof(ifindices));
		strncpy(ifr.ifr_name, brname, IFNAMSIZ);
		ifr.ifr_data = (char *) &list_args;

		list_num = ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);
		if (list_num < 0) {
			close(br_socket_fd);
			return -1;
		}

		memset(&ifr, 0, sizeof(ifr));
		fdb_num = sizeof(fdb)/sizeof(struct __fdb_entry);
		args[0] = BRCTL_GET_FDB_ENTRIES;
		args[1] = (unsigned long)fdb;
		args[2] = fdb_num;
		memcpy(ifr.ifr_name, brname, IFNAMSIZ);
		((unsigned long *)(&ifr.ifr_data))[0] = (unsigned long)args;

		ret = offset = 0;
		do{
			args[3] = offset;
			num = ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);

			for (i = 0; i < num; i++) {
				struct __fdb_entry *f = fdb + i;

				if (memcmp(f->mac_addr, macaddr, 6) == 0) {
					if_indextoname(ifindices[f->port_no], ifname);
					ret = 1;
					break;
				}
			}
			if(num > 0 && num >= fdb_num)
				offset += num;
		} while((num > 0 && num >= fdb_num) && ret == 0);

		close(br_socket_fd);
	}
	return ret;
}

unsigned char rtk_get_logic_port_by_ifname(unsigned char *ifName)
{
	unsigned char conPort = 0;
	int i;

	if(ifName == NULL)
		return conPort;

	if(strstr(ifName, "wlan"))
	{
		for(i=0 ; i<=PMAP_WLAN1_VAP_END-PMAP_WLAN0 ; i++)
		{
			if(!strcmp(ifName, wlan[i]))
			{
				conPort = PMAP_WLAN0+i;
				break;
			}
		}
	}
	else
	{
		for(i=0 ; i<4 ; i++)
		{
			if(!strcmp(ifName, ELANVIF[i]))
			{
				conPort = i;
				break;
			}
		}
	}
	return conPort;
}

int rtk_check_mac_exist_in_bridge(unsigned char *mac)
{
	FILE* tmp_brctl_showmacs_bridgeif=NULL;
	unsigned char brctlPortNo[32] = {0};
	unsigned char brctlMacAddr[32] = {0};
	unsigned char brctlIsLocal[32] = {0};
	unsigned char brctlAgingTimer[32] = {0};
	unsigned char cmd[256] = {0};
	int read = 0, len = 0;
	char *line=NULL;

	if(mac == NULL)
		return 0;

	sprintf(cmd, "brctl showmacs %s > /tmp/brctl_showmacs_bridgeif", (char *)BRIF);
	system(cmd);
	if ((tmp_brctl_showmacs_bridgeif = fopen("/tmp/brctl_showmacs_bridgeif", "r")) == NULL)
	{
		return;
	}

	while ((read = getline(&line, &len, tmp_brctl_showmacs_bridgeif)) != -1)
	{
		if(sscanf(line, "%s %s %s %s", &brctlPortNo[0], &brctlMacAddr[0], &brctlIsLocal[0], &brctlAgingTimer[0]))
		{
			if(!strcmp(mac, brctlMacAddr)) {
				free(line);
				fclose(tmp_brctl_showmacs_bridgeif);
				return 1;
			}
		}
	}

	free(line);
	fclose(tmp_brctl_showmacs_bridgeif);
	return 0;
}

int get_phyport_index_by_IP(union mysockaddr *udpaddr)
{
	int port_index=-1;
	FILE *fp=NULL;

	my_syslog(LOG_DEBUG, _("[%s:%d]"), __func__, __LINE__);

	if (udpaddr->sa.sa_family == AF_INET)
	{
		unsigned char arpIpAddr[32] = {0};
		unsigned char arpHwType[32] = {0};
		unsigned char arpFlags[32] = {0};
		unsigned char arpHwAddress[32] = {0};
		unsigned char arpMask[32] = {0};
		unsigned char arpDevice[32] = {0};
		unsigned int macAddrInt[6]={0};
		unsigned char macAddrChar[6]={0};
		struct in_addr sin_addr;
		char ifName[128];
		int phy_port;
		int read = 0, len = 0, ret;
		char *line=NULL;

		if ((fp = fopen("/proc/net/arp", "r")) == NULL)
			goto done;

		while ((read = getline(&line, &len, fp)) != -1)
		{
			if(sscanf(line, "%s %s %s %s %s %s", &arpIpAddr[0], &arpHwType[0], &arpFlags[0], &arpHwAddress[0], &arpMask[0], &arpDevice[0])== 6
				&& inet_aton(arpIpAddr, &sin_addr) != 0 && sin_addr.s_addr == udpaddr->in.sin_addr.s_addr && (strtoul(arpFlags, NULL,0)==0x2))
			{
				//ip to MAC
				my_syslog(LOG_DEBUG, _("%s %s %s %s %s %s"), arpIpAddr, arpHwType, arpFlags, arpHwAddress, arpMask, arpDevice);
				sscanf(arpHwAddress, "%02x:%02x:%02x:%02x:%02x:%02x", &macAddrInt[0], &macAddrInt[1], &macAddrInt[2], &macAddrInt[3],&macAddrInt[4], &macAddrInt[5]);
				macAddrChar[0] = macAddrInt[0];
				macAddrChar[1] = macAddrInt[1];
				macAddrChar[2] = macAddrInt[2];
				macAddrChar[3] = macAddrInt[3];
				macAddrChar[4] = macAddrInt[4];
				macAddrChar[5] = macAddrInt[5];
				//MAC to ifname
				if(rtk_lannetinfo_get_ifname_by_macaddr(arpDevice, macAddrChar, ifName) != 1) {
					free(line);
					line = NULL;
					continue;
				}
				//ifname to logic port index (use logic port to seprate wifi 2.4g, 5g)
				port_index = rtk_get_logic_port_by_ifname(ifName);
				my_syslog(LOG_DEBUG, _("ifName = %s, port_index=%d"), ifName, port_index);
				break;
			}
			free(line);
			line = NULL;
		}
		if(line) free(line);
		line = NULL;
	}
	else if(udpaddr->sa.sa_family == AF_INET6)//ipv6
	{
		int i,count=0,line=0;
		int mac_valid_idx=-1;
		char buf[256]={0};
		unsigned int macAddrInt[6]={0};
		unsigned char macAddrChar[6]={0};
		char *delim = " ";
		char *p,*ep;
		unsigned char *ipv6;
		struct sockaddr_in6 sa6;
		int ret;
		char ifName[32]={0}, devname[32]={0};

		system("ip -6 neighbor > /tmp/v6_neighbor");
		fp=fopen("/tmp/v6_neighbor","r+");

		if (fp)
		{
			fseek(fp, 0, SEEK_SET);
			while(fgets(buf, sizeof(buf), fp))
			{
				line++;
				ep = buf + strlen(buf);
				if (*(ep - 1) != '\n')
					continue;

				p = strtok(buf,delim);//get ipv6 addr
				//transfer v6 ip string to v6 addr
				inet_pton(AF_INET6, p, &(sa6.sin6_addr));
				ipv6 = (unsigned char *)&(sa6.sin6_addr);
#if 0
				for(i=0;i<16;i++)
					printf("[%02X] ", ipv6[i]);
#endif
				ret = memcmp((void*)&(sa6.sin6_addr),(void*)&(udpaddr->in6.sin6_addr),sizeof(char)*16);
				count=0;
				if(0 == ret)
				{
					while(p = strtok(NULL,delim))
					{
						count++;
						if(count == 2)
							strncpy(devname, p, sizeof(devname));

						if(count == 4)
						{
							//get mac by ip!
							sscanf(p, "%02x:%02x:%02x:%02x:%02x:%02x", &macAddrInt[0], &macAddrInt[1], &macAddrInt[2], &macAddrInt[3], &macAddrInt[4], &macAddrInt[5]);
							macAddrChar[0] = macAddrInt[0];
							macAddrChar[1] = macAddrInt[1];
							macAddrChar[2] = macAddrInt[2];
							macAddrChar[3] = macAddrInt[3];
							macAddrChar[4] = macAddrInt[4];
							macAddrChar[5] = macAddrInt[5];
							my_syslog(LOG_DEBUG, _("p=%s mac %02X:%02X:%02X:%02X:%02X:%02X\n") ,p, macAddrChar[0],macAddrChar[1],macAddrChar[2],macAddrChar[3],macAddrChar[4],macAddrChar[5]);
							break;
						}
					}//while(p = strtok(NULL,delim))
				}
				//MAC to ifname
				if( devname != '\0' && rtk_lannetinfo_get_ifname_by_macaddr(devname, macAddrChar, ifName) != 1)
					continue;

				//ifname to phyport index
				port_index = rtk_get_logic_port_by_ifname(ifName);
				my_syslog(LOG_DEBUG, _("ifName = %s, port_index=%d"), ifName, port_index);
				break;
			}//while(fgets)
		}
	}
	else
		my_syslog(LOG_DEBUG, _("not AF_INET or AF_INET6, port_index=%d"), port_index);

done:
	if(fp)
		fclose(fp);

	return port_index;
}
#endif //DNSQUERY_PORT_BINDING

#if defined(DNSQUERY_SKBMARK_BINDING)
static int forward_query(int udpfd, union mysockaddr *udpaddr,
			union all_addr *dst_addr, unsigned int dst_iface,
			struct dns_header *header, size_t plen, time_t now,
			struct frec *forward, int ad_reqd, int do_bit, unsigned int skb_mark)
#else
static int forward_query(int udpfd, union mysockaddr *udpaddr,
			 union all_addr *dst_addr, unsigned int dst_iface,
			 struct dns_header *header, size_t plen, time_t now, 
			 struct frec *forward, int ad_reqd, int do_bit)
#endif
{
  char *domain = NULL;
  int type = SERV_DO_DNSSEC, norebind = 0;
  union all_addr *addrp = NULL;
  unsigned int flags = 0;
  unsigned int fwd_flags = 0;
  struct server *start = NULL;
  void *hash = hash_questions(header, plen, daemon->namebuff);
#ifdef HAVE_DNSSEC
  int do_dnssec = 0;
#endif
  unsigned int gotname = extract_request(header, plen, daemon->namebuff, NULL);
  unsigned char *oph = find_pseudoheader(header, plen, NULL, NULL, NULL, NULL);
  int old_src = 0;
#ifdef CONFIG_CMCC_2023_NEWREQUIREMENT
  int skipped_domain = 0, retry_skipped_domain = 0;
#endif  
  (void)do_bit;

#if defined(DNSQUERY_PORT_BINDING)
	int port_index=-1;
	char *bindport = NULL;
	if(udpaddr){
  		my_syslog(LOG_DEBUG, _("[%s:%d] udpaddr.sa.sa_family=%d"), __func__,__LINE__,udpaddr->sa.sa_family);
  		port_index=get_phyport_index_by_IP(udpaddr);
	}
#endif
  
  // Mason Yu
  // In nonrecursive mode (the default), queries are sent to the server with the RD (recursion desired) flag set to 0. The server should respond positively to these only if it has the domain cached.
  // If it has not domain cached, drop it.
  if ((header->hb3 & HB3_RD)==0) {
	printf("This is non-recursive query. DROP !!\n");
	return 0;
  }
  
  if (header->hb4 & HB4_CD)
    fwd_flags |= FREC_CHECKING_DISABLED;
  if (ad_reqd)
    fwd_flags |= FREC_AD_QUESTION;
  if (oph)
    fwd_flags |= FREC_HAS_PHEADER;
#ifdef HAVE_DNSSEC
  if (do_bit)
    fwd_flags |= FREC_DO_QUESTION;
#endif
  
  /* Check for retry on existing query */
  if (forward)
    old_src = 1;
  else if ((forward = lookup_frec_by_query(hash, fwd_flags)))
    {
      struct frec_src *src;
      
      for (src = &forward->frec_src; src; src = src->next)
	if (src->orig_id == ntohs(header->id) && 
	    sockaddr_isequal(&src->source, udpaddr))
	  break;
      
      if (src)
	old_src = 1;
      else
	{
	  /* Existing query, but from new source, just add this 
	     client to the list that will get the reply.*/
	  
	  /* Note whine_malloc() zeros memory. */
	  if (!daemon->free_frec_src &&
	      daemon->frec_src_count < daemon->ftabsize &&
	      (daemon->free_frec_src = whine_malloc(sizeof(struct frec_src))))
	    {
	      daemon->frec_src_count++;
	      daemon->free_frec_src->next = NULL;
	    }
	  
	  /* If we've been spammed with many duplicates, return REFUSED. */
	  if (!daemon->free_frec_src)
	    {
	      query_full(now);
	      goto frec_err;
	    }
	  
	  src = daemon->free_frec_src;
	  daemon->free_frec_src = src->next;
	  src->next = forward->frec_src.next;
	  forward->frec_src.next = src;
	  src->orig_id = ntohs(header->id);
	  src->source = *udpaddr;
	  src->dest = *dst_addr;
	  src->log_id = daemon->log_id;
	  src->iface = dst_iface;
	  src->fd = udpfd;

	  /* closely spaced identical queries cannot be a try and a retry, so
	     it's safe to wait for the reply from the first without
	     forwarding the second. */
	  if (difftime(now, forward->time) < 2)
	    return 0;
	}
    }

  /* retry existing query */
  if (forward)
    {
      /* If we didn't get an answer advertising a maximal packet in EDNS,
	 fall back to 1280, which should work everywhere on IPv6.
	 If that generates an answer, it will become the new default
	 for this server */
      forward->flags |= FREC_TEST_PKTSZ;

#ifdef HAVE_DNSSEC
      /* If we've already got an answer to this query, but we're awaiting keys for validation,
	 there's no point retrying the query, retry the key query instead...... */
      if (forward->blocking_query)
	{
	  int fd, is_sign;
	  unsigned char *pheader;
	  
	  forward->flags &= ~FREC_TEST_PKTSZ;
	  
	  while (forward->blocking_query)
	    forward = forward->blocking_query;
	   
	  blockdata_retrieve(forward->stash, forward->stash_len, (void *)header);
	  plen = forward->stash_len;
	  
	  forward->flags |= FREC_TEST_PKTSZ;
	  if (find_pseudoheader(header, plen, NULL, &pheader, &is_sign, NULL) && !is_sign)
	    PUTSHORT(SAFE_PKTSZ, pheader);
	  
	  if ((fd = allocate_rfd(&forward->rfds, forward->sentto)) != -1)
	    server_send_log(forward->sentto, fd, header, plen,
			    DUMP_SEC_QUERY,
			    F_NOEXTRA | F_DNSSEC, "retry", "dnssec");

	  return 1;
	}
#endif

      /* retry on existing query, from original source. Send to all available servers  */
      domain = forward->sentto->domain;
#if defined(DNSQUERY_PORT_BINDING)
	bindport = forward->sentto->bindport;
#endif
      forward->sentto->failed_queries++;
      if (!option_bool(OPT_ORDER) && old_src)
	{
	  forward->forwardall = 1;
	  daemon->last_server = NULL;
	}
      type = forward->sentto->flags & SERV_TYPE;
#ifdef HAVE_DNSSEC
      do_dnssec = forward->sentto->flags & SERV_DO_DNSSEC;
#endif

	if (!(start = forward->sentto->next)){
	start = daemon->servers; /* at end of list, recycle */
	}
      header->id = htons(forward->new_id);
    }
  else 
    {
      /* new query */
      if (gotname)
#if defined(DNSQUERY_PORT_BINDING)
	flags = search_servers(now, &addrp, gotname, daemon->namebuff, &type, &domain, &norebind, &bindport);
#elif defined(DNSQUERY_SKBMARK_BINDING)
	flags = search_servers(now, &addrp, gotname, daemon->namebuff, &type, &domain, &norebind, skb_mark);
#else
	flags = search_servers(now, &addrp, gotname, daemon->namebuff, &type, &domain, &norebind);
#endif
#ifdef HAVE_DNSSEC
      do_dnssec = type & SERV_DO_DNSSEC;
#endif
      type &= ~SERV_DO_DNSSEC;      
      
      /* may be no servers available. */
      if (daemon->servers && !flags)
	forward = get_new_frec(now, NULL, NULL);
      /* table full - flags == 0, return REFUSED */
      
      if (forward)
	{
	  forward->frec_src.source = *udpaddr;
	  forward->frec_src.orig_id = ntohs(header->id);
	  forward->frec_src.dest = *dst_addr;
	  forward->frec_src.iface = dst_iface;
	  forward->frec_src.next = NULL;
	  forward->frec_src.fd = udpfd;
	  forward->new_id = get_id();
	  memcpy(forward->hash, hash, HASH_SIZE);
	  forward->forwardall = 0;
	  forward->flags = fwd_flags;
	  if (norebind)
	    forward->flags |= FREC_NOREBIND;
	  if (header->hb4 & HB4_CD)
	    forward->flags |= FREC_CHECKING_DISABLED;
	  if (ad_reqd)
	    forward->flags |= FREC_AD_QUESTION;
#ifdef HAVE_DNSSEC
	  forward->work_counter = DNSSEC_WORK;
	  if (do_bit)
	    forward->flags |= FREC_DO_QUESTION;
#endif
	  
	  header->id = htons(forward->new_id);
#if defined(DNSQUERY_SKBMARK_BINDING)
	  forward->skb_mark = skb_mark;
#endif
#if defined(CONFIG_DNS_QUERY_TIMEOUT) || defined(DNS_FAILOVER_TIMING_SUPPORT)
	  forward->isreceivereply = 0;
	  forward->timeout = now + (time_t)DNS_QUERY_TIMEOUT;
	  forward->fplen = plen;

	  if (forward->fpacket == NULL)
	  {
		forward->fpacket = malloc(plen+DNS_REPLY_PAD);
		memcpy(forward->fpacket, header, plen);
	  }
	  else
	  {
		free(forward->fpacket);
		forward->fpacket = NULL;
		forward->fpacket = malloc(plen+DNS_REPLY_PAD);
		memcpy(forward->fpacket, header, plen);
	  }
#endif

	  /* In strict_order mode, always try servers in the order 
	     specified in resolv.conf, if a domain is given 
	     always try all the available servers,
	     otherwise, use the one last known to work. */
	  
	  if (type == 0)
	    {
	      if (option_bool(OPT_ORDER))
		start = daemon->servers;
	      else if (!(start = daemon->last_server) ||
		       daemon->forwardcount++ > FORWARD_TEST ||
		       difftime(now, daemon->forwardtime) > FORWARD_TIME)
		{
		  start = daemon->servers;
		  forward->forwardall = 1;
		  daemon->forwardcount = 0;
		  daemon->forwardtime = now;
		}
	    }
	  else
	    {
	      start = daemon->servers;
	      if (!option_bool(OPT_ORDER))
		forward->forwardall = 1;
	    }
#ifdef DNS_FAILOVER_TIMING_SUPPORT
	  forward->isFailoverDone = 0;
	  forward->firstsentto = start;
#endif

	}
    }

  /* check for send errors here (no route to host) 
     if we fail to send to all nameservers, send back an error
     packet straight away (helps modem users when offline)  */
  
  if (!flags && forward)
    {
#ifdef DNS_FAILOVER_TIMING_SUPPORT
      struct server *firstsentto = forward->firstsentto?:start;
#else
      struct server *firstsentto = start;
#endif
      int subnet, cacheable, forwarded = 0;
      size_t edns0_len;
      unsigned char *pheader;
	//check port bind first!!
#if defined(DNSQUERY_PORT_BINDING)
		while(1)
		{
			/* only send to servers dealing with our domain.
			domain may be NULL, in which case server->domain
			must be NULL also. */
			my_syslog(LOG_DEBUG, _("[%s:%d] type=%d, domain=%s,start->domain=%s, start->flags=%x bindport=%s, start->bindport=%s, port_index %d"), __func__,__LINE__,type,
				( type & SERV_HAS_DOMAIN)?domain:"",( start->flags & SERV_HAS_DOMAIN)?start->domain:"",start->flags, (type & SERV_HAS_BIND_PORT)?bindport:"",(start->flags & SERV_HAS_BIND_PORT)?start->bindport:"", port_index);
		  	if (( start->flags != SERV_HAS_DOMAIN) && (start->flags & SERV_HAS_BIND_PORT) && port_match(port_index,start->bindport))
			{
				int fd;
				my_syslog(LOG_DEBUG, _("[%s:%d] start->flags=%x"), __func__,__LINE__,start->flags);
				/* find server socket to use, may need to get random one. */
				if (start->sfd){
					fd = start->sfd->fd;
				}
				else
				{
					if (start->addr.sa.sa_family == AF_INET6)
					{
						if (!forward->rfd6 &&
							!(forward->rfd6 = allocate_rfd(AF_INET6)))
							break;
						daemon->rfd_save = forward->rfd6;
						fd = forward->rfd6->fd;
					}
					else
					{
						if (!forward->rfd4 && !(forward->rfd4 = allocate_rfd(AF_INET))){
							break;
						}
						daemon->rfd_save = forward->rfd4;
						fd = forward->rfd4->fd;
					}
				}

				if (sendto(fd, (char *)header, plen, 0, &start->addr.sa, sa_len(&start->addr)) == -1)
				{
					if (retry_send())
						continue;
				}
				else
				{
					/* Keep info in case we want to re-send this packet */
					daemon->srv_save = start;
					daemon->packet_len = plen;

					if (!gotname)
		    			strcpy(daemon->namebuff, "query");
					if (start->addr.sa.sa_family == AF_INET){
						log_query(F_SERVER | F_IPV4 | F_FORWARD, daemon->namebuff,
							(struct all_addr *)&start->addr.in.sin_addr, NULL);
					}
		  			else{
		    			log_query(F_SERVER | F_IPV6 | F_FORWARD, daemon->namebuff,
			      			(struct all_addr *)&start->addr.in6.sin6_addr, NULL);
		  			}
		  			start->queries++;
		  			forwarded = 1;
		  			forward->sentto = start;
#if 0		  
		  			if (!forward->forwardall)
		    			break;
#endif		  
					my_syslog(LOG_DEBUG, _("[%s:%d] forward->forwardall=%d"), __func__,__LINE__,forward->forwardall);		  
		  			forward->forwardall++;
				}
	    	}

	  		if (!(start = start->next))
 	    		start = daemon->servers;

	  		if (start == firstsentto) {
#ifdef DNS_FAILOVER_TIMING_SUPPORT
				forward->isFailoverDone = 1;
#endif
	    		break;
  			}
			
#ifdef DNS_FAILOVER_TIMING_SUPPORT
			/* xmit to one server done */
			if (forwarded && (daemon->failover_time>0))
				break;
#endif
		}		
		//if port_index match port binding, it can only forward to binding wan
		my_syslog(LOG_DEBUG, _("[%s:%d] forwarded=%d forward->forwardall=%d"), __func__,__LINE__,forwarded,forward->forwardall);		  

		if(forwarded)
			goto HAVE_PORT_BIND;
#endif	
      
      /* If a query is retried, use the log_id for the retry when logging the answer. */
      forward->frec_src.log_id = daemon->log_id;
      
      plen = add_edns0_config(header, plen, ((unsigned char *)header) + PACKETSZ, &forward->frec_src.source, now, &subnet, &cacheable);
      
      if (subnet)
	forward->flags |= FREC_HAS_SUBNET;

      if (!cacheable)
	forward->flags |= FREC_NO_CACHE;

#ifdef HAVE_DNSSEC
      if (option_bool(OPT_DNSSEC_VALID) && do_dnssec)
	{
	  plen = add_do_bit(header, plen, ((unsigned char *) header) + PACKETSZ);
	 	      
	  /* For debugging, set Checking Disabled, otherwise, have the upstream check too,
	     this allows it to select auth servers when one is returning bad data. */
	  if (option_bool(OPT_DNSSEC_DEBUG))
	    header->hb4 |= HB4_CD;

	}
#endif

      if (find_pseudoheader(header, plen, &edns0_len, &pheader, NULL, NULL))
	{
	  /* If there wasn't a PH before, and there is now, we added it. */
	  if (!oph)
	    forward->flags |= FREC_ADDED_PHEADER;

	  /* If we're sending an EDNS0 with any options, we can't recreate the query from a reply. */
	  if (edns0_len > 11)
	    forward->flags |= FREC_HAS_EXTRADATA;

	  /* Reduce udp size on retransmits. */
	  if (forward->flags & FREC_TEST_PKTSZ)
	    PUTSHORT(SAFE_PKTSZ, pheader);
	}
#ifdef CONFIG_CMCC_2023_NEWREQUIREMENT	
SKIPPED_DOMAIN_RETRY:
#endif   
      while (1)
	{ 
	  int fd;

	  /* only send to servers dealing with our domain.
	     domain may be NULL, in which case server->domain 
	     must be NULL also. */
	  
	  if (
#if defined(DNSQUERY_PORT_BINDING)
		(!(start->flags & SERV_HAS_BIND_PORT) || port_match(port_index,start->bindport)) &&
#elif defined(DNSQUERY_SKBMARK_BINDING)
		(!(start->flags & SERV_HAS_BIND_SKBMARK) || ((skb_mark & start->mask) == start->mark)) &&
#endif
		server_test_type(start, domain, type, 0) &&
	      ((fd = allocate_rfd(&forward->rfds, start)) != -1))
	    {
	      
#ifdef HAVE_CONNTRACK
	      /* Copy connection mark of incoming query to outgoing connection. */
	      if (option_bool(OPT_CONNTRACK))
		set_outgoing_mark(forward, fd);
#endif
	      
#ifdef HAVE_DNSSEC
	      if (option_bool(OPT_DNSSEC_VALID) && (forward->flags & FREC_ADDED_PHEADER))
		{
		  /* Difficult one here. If our client didn't send EDNS0, we will have set the UDP
		     packet size to 512. But that won't provide space for the RRSIGS in many cases.
		     The RRSIGS will be stripped out before the answer goes back, so the packet should
		     shrink again. So, if we added a do-bit, bump the udp packet size to the value
		     known to be OK for this server. We check returned size after stripping and set
		     the truncated bit if it's still too big. */		  
		  unsigned char *pheader;
		  int is_sign;
		  if (find_pseudoheader(header, plen, NULL, &pheader, &is_sign, NULL) && !is_sign)
		    PUTSHORT(start->edns_pktsz, pheader);
		}
#endif
#ifdef CONFIG_CMCC_2023_NEWREQUIREMENT
		if(udpaddr && start->addr.sa.sa_family!= udpaddr->sa.sa_family && retry_skipped_domain == 0)
		{
			skipped_domain = 1;
			goto HAVE_NDS_PRIORITY;
		}
#endif
	      if (retry_send(sendto(fd, (char *)header, plen, 0,
				    &start->addr.sa,
				    sa_len(&start->addr))))
		continue;
	    
	      if (errno == 0)
		{
#ifdef HAVE_DUMPFILE
		  dump_packet(DUMP_UP_QUERY, (void *)header, plen, NULL, &start->addr);
#endif
		  
		  /* Keep info in case we want to re-send this packet */
		  daemon->srv_save = start;
		  daemon->packet_len = plen;
		  daemon->fd_save = fd;
		  
		  if (!gotname)
		    strcpy(daemon->namebuff, "query");
		  log_query_mysockaddr(F_SERVER | F_FORWARD, daemon->namebuff,
				       &start->addr, NULL);
		  start->queries++;
		  forwarded = 1;
		  forward->sentto = start;
		  snprintf(forward->namebuff, sizeof(forward->namebuff)-1, "%s", daemon->namebuff);
		  if (!forward->forwardall) 
		    break;
		  forward->forwardall++;
		}
	    }
#ifdef CONFIG_CMCC_2023_NEWREQUIREMENT	
HAVE_NDS_PRIORITY:
#endif	  
	  if (!(start = start->next)){
 	    start = daemon->servers;
#ifdef CONFIG_CMCC_2023_NEWREQUIREMENT
		if(retry_skipped_domain)
			break;
#endif
	  }
	  
	  if (start == firstsentto) {
#ifdef DNS_FAILOVER_TIMING_SUPPORT
 	    forward->isFailoverDone = 1;
#endif
#ifdef CONFIG_CMCC_2023_NEWREQUIREMENT
		if(retry_skipped_domain == 0)
#endif
 	    break;
	  }
			
#ifdef DNS_FAILOVER_TIMING_SUPPORT
	  /* xmit to one server done */
	  if (forwarded && (daemon->failover_time>0))
 	    break;
#endif
	}
#ifdef DNS_FAILOVER_TIMING_SUPPORT
	forward->firstsentto = firstsentto;
#endif
    if (forwarded) {
#ifdef DNS_FAILOVER_TIMING_SUPPORT
 	    forward->timeout = now + (time_t)DNS_QUERY_TIMEOUT;
 	    forward->failover = now + daemon->failover_time;
#endif
 	    return 1;
	}
#ifdef CONFIG_CMCC_2023_NEWREQUIREMENT
	if(forwarded == 0 && skipped_domain == 1 && retry_skipped_domain == 0)
	{
		start = daemon->servers;
		retry_skipped_domain++;
		goto SKIPPED_DOMAIN_RETRY;
	}
#endif      
      /* could not send on, prepare to return */ 
      header->id = htons(forward->frec_src.orig_id);
      free_frec(forward); /* cancel */

    }	  
  
  /* could not send on, return empty answer or address if known for whole domain */
 frec_err:
  if (udpfd != -1)
    {
      plen = setup_reply(header, plen, addrp, flags, daemon->local_ttl);
      if (oph)
	plen = add_pseudoheader(header, plen, ((unsigned char *) header) + PACKETSZ, daemon->edns_pktsz, 0, NULL, 0, do_bit, 0);
      send_from(udpfd, option_bool(OPT_NOWILD) || option_bool(OPT_CLEVERBIND), (char *)header, plen, udpaddr, dst_addr, dst_iface);
    }

  return 0;
}

static size_t process_reply(struct dns_header *header, time_t now, struct server *server, size_t n, int check_rebind, 
			    int no_cache, int cache_secure, int bogusanswer, int ad_reqd, int do_bit, int added_pheader, 
			    int check_subnet, union mysockaddr *query_source)
{
  unsigned char *pheader, *sizep;
  char **sets = 0;
  int munged = 0, is_sign;
  unsigned int rcode = RCODE(header);
  size_t plen; 
  
  (void)ad_reqd;
  (void)do_bit;
  (void)bogusanswer;

#ifdef HAVE_IPSET
  if (daemon->ipsets && extract_request(header, n, daemon->namebuff, NULL))
    {
      /* Similar algorithm to search_servers. */
      struct ipsets *ipset_pos;
      unsigned int namelen = strlen(daemon->namebuff);
      unsigned int matchlen = 0;
      for (ipset_pos = daemon->ipsets; ipset_pos; ipset_pos = ipset_pos->next) 
	{
	  unsigned int domainlen = strlen(ipset_pos->domain);
	  char *matchstart = daemon->namebuff + namelen - domainlen;
	  if (namelen >= domainlen && hostname_isequal(matchstart, ipset_pos->domain) &&
	      (domainlen == 0 || namelen == domainlen || *(matchstart - 1) == '.' ) &&
	      domainlen >= matchlen) 
	    {
	      matchlen = domainlen;
	      sets = ipset_pos->sets;
	    }
	}
    }
#endif

  if ((pheader = find_pseudoheader(header, n, &plen, &sizep, &is_sign, NULL)))
    {
      /* Get extended RCODE. */
      rcode |= sizep[2] << 4;

      if (check_subnet && !check_source(header, plen, pheader, query_source))
	{
	  my_syslog(LOG_WARNING, _("discarding DNS reply: subnet option mismatch"));
	  return 0;
	}
      
      if (!is_sign)
	{
	  if (added_pheader)
	    {
	      /* client didn't send EDNS0, we added one, strip it off before returning answer. */
	      n = rrfilter(header, n, 0);
	      pheader = NULL;
	    }
	  else
	    {
	      unsigned short udpsz;

	      /* If upstream is advertising a larger UDP packet size
		 than we allow, trim it so that we don't get overlarge
		 requests for the client. We can't do this for signed packets. */
	      GETSHORT(udpsz, sizep);
	      if (udpsz > daemon->edns_pktsz)
		{
		  sizep -= 2;
		  PUTSHORT(daemon->edns_pktsz, sizep);
		}

#ifdef HAVE_DNSSEC
	      /* If the client didn't set the do bit, but we did, reset it. */
	      if (option_bool(OPT_DNSSEC_VALID) && !do_bit)
		{
		  unsigned short flags;
		  sizep += 2; /* skip RCODE */
		  GETSHORT(flags, sizep);
		  flags &= ~0x8000;
		  sizep -= 2;
		  PUTSHORT(flags, sizep);
		}
#endif
	    }
	}
    }
  
  /* RFC 4035 sect 4.6 para 3 */
  if (!is_sign && !option_bool(OPT_DNSSEC_PROXY))
     header->hb4 &= ~HB4_AD;
  
  if (OPCODE(header) != QUERY)
    return resize_packet(header, n, pheader, plen);

  if (rcode != NOERROR && rcode != NXDOMAIN)
    {
      union all_addr a;
      a.log.rcode = rcode;
      log_query(F_UPSTREAM | F_RCODE, "error", &a, NULL);
      
      return resize_packet(header, n, pheader, plen);
    }
  
  /* Complain loudly if the upstream server is non-recursive. */
  if (!(header->hb4 & HB4_RA) && rcode == NOERROR &&
      server && !(server->flags & SERV_WARNED_RECURSIVE))
    {
      (void)prettyprint_addr(&server->addr, daemon->namebuff);
      my_syslog(LOG_WARNING, _("nameserver %s refused to do a recursive query"), daemon->namebuff);
      if (!option_bool(OPT_LOG))
	server->flags |= SERV_WARNED_RECURSIVE;
    }  

  if (daemon->bogus_addr && rcode != NXDOMAIN &&
      check_for_bogus_wildcard(header, n, daemon->namebuff, now))
    {
      munged = 1;
      SET_RCODE(header, NXDOMAIN);
      header->hb3 &= ~HB3_AA;
      cache_secure = 0;
    }
  else 
    {
      int doctored = 0;
      
      if (rcode == NXDOMAIN && 
	  extract_request(header, n, daemon->namebuff, NULL) &&
	  check_for_local_domain(daemon->namebuff, now))
	{
	  /* if we forwarded a query for a locally known name (because it was for 
	     an unknown type) and the answer is NXDOMAIN, convert that to NODATA,
	     since we know that the domain exists, even if upstream doesn't */
	  munged = 1;
	  header->hb3 |= HB3_AA;
	  SET_RCODE(header, NOERROR);
	  cache_secure = 0;
	}
      
      if (extract_addresses(header, n, daemon->namebuff, now, sets, is_sign, check_rebind, no_cache, cache_secure, &doctored))
	{
	  my_syslog(LOG_WARNING, _("possible DNS-rebind attack detected: %s"), daemon->namebuff);
	  munged = 1;
	  cache_secure = 0;
	}

      if (doctored)
	cache_secure = 0;
    }
  
#ifdef HAVE_DNSSEC
  if (bogusanswer && !(header->hb4 & HB4_CD) && !option_bool(OPT_DNSSEC_DEBUG))
    {
      /* Bogus reply, turn into SERVFAIL */
      SET_RCODE(header, SERVFAIL);
      munged = 1;
    }

  if (option_bool(OPT_DNSSEC_VALID))
    {
      header->hb4 &= ~HB4_AD;
      
      if (!(header->hb4 & HB4_CD) && ad_reqd && cache_secure)
	header->hb4 |= HB4_AD;
      
      /* If the requestor didn't set the DO bit, don't return DNSSEC info. */
      if (!do_bit)
	n = rrfilter(header, n, 1);
    }
#endif

  /* do this after extract_addresses. Ensure NODATA reply and remove
     nameserver info. */
  
  if (munged)
    {
      header->ancount = htons(0);
      header->nscount = htons(0);
      header->arcount = htons(0);
      header->hb3 &= ~HB3_TC;
    }
  
  /* the bogus-nxdomain stuff, doctor and NXDOMAIN->NODATA munging can all elide
     sections of the packet. Find the new length here and put back pseudoheader
     if it was removed. */
  return resize_packet(header, n, pheader, plen);
}

/* sets new last_server */
void reply_query(int fd, time_t now)
{
  /* packet from peer server, extract data for cache, and send to
     original requester */
  struct dns_header *header;
  union mysockaddr serveraddr;
  struct frec *forward;
  socklen_t addrlen = sizeof(serveraddr);
  ssize_t n = recvfrom(fd, daemon->packet, daemon->packet_buff_sz, 0, &serveraddr.sa, &addrlen);
  size_t nn;
  struct server *server;
  void *hash;

  /* packet buffer overwritten */
  daemon->srv_save = NULL;

  /* Determine the address of the server replying  so that we can mark that as good */
  if (serveraddr.sa.sa_family == AF_INET6)
    serveraddr.in6.sin6_flowinfo = 0;
  
  header = (struct dns_header *)daemon->packet;

  if (n < (int)sizeof(struct dns_header) || !(header->hb3 & HB3_QR))
    return;
  
  /* spoof check: answer must come from known server, */
  for (server = daemon->servers; server; server = server->next)
    if (!(server->flags & (SERV_LITERAL_ADDRESS | SERV_NO_ADDR)) &&
	sockaddr_isequal(&server->addr, &serveraddr))
      break;
  
  if (!server)
    return;

  /* If sufficient time has elapsed, try and expand UDP buffer size again. */
  if (difftime(now, server->pktsz_reduced) > UDP_TEST_TIME)
    server->edns_pktsz = daemon->edns_pktsz;

  hash = hash_questions(header, n, daemon->namebuff);
  
  if (!(forward = lookup_frec(ntohs(header->id), fd, hash)))
    return;
  
#ifdef HAVE_DUMPFILE
  dump_packet((forward->flags & (FREC_DNSKEY_QUERY | FREC_DS_QUERY)) ? DUMP_SEC_REPLY : DUMP_UP_REPLY,
	      (void *)header, n, &serveraddr, NULL);
#endif

  /* log_query gets called indirectly all over the place, so 
     pass these in global variables - sorry. */
  daemon->log_display_id = forward->frec_src.log_id;
  daemon->log_source_addr = &forward->frec_src.source;
  
  if (daemon->ignore_addr && RCODE(header) == NOERROR &&
      check_for_ignored_address(header, n))
    return;

  /* Note: if we send extra options in the EDNS0 header, we can't recreate
     the query from the reply. */
  if ((RCODE(header) == REFUSED || RCODE(header) == SERVFAIL || RCODE(header) == NOTIMP || 
       RCODE(header) == NXDOMAIN || RCODE(header) == NOERROR) &&
      forward->forwardall == 0 &&
      !(forward->flags & FREC_HAS_EXTRADATA))
    /* for broken servers, attempt to send to another one. */
    {
      unsigned char *pheader;
      size_t plen;
      int is_sign;

#ifdef HAVE_DNSSEC
      if (forward->flags & (FREC_DNSKEY_QUERY | FREC_DS_QUERY))
	{
	  struct server *start;
	  
	  blockdata_retrieve(forward->stash, forward->stash_len, (void *)header);
	  plen = forward->stash_len;

	  forward->forwardall = 2; /* only retry once */
	  start = forward->sentto;

	  /* for non-domain specific servers, see if we can find another to try. */
	  if ((forward->sentto->flags & SERV_TYPE) == 0)
	    while (1)
	      {
		if (!(start = start->next))
		  start = daemon->servers;
		if (start == forward->sentto)
		  break;
		
		if ((start->flags & SERV_TYPE) == 0 &&
		    (start->flags & SERV_DO_DNSSEC))
		  break;
	      }
	    
	  
	  if ((fd = allocate_rfd(&forward->rfds, start)) != -1)
	    server_send_log(start, fd, header, plen,
			    DUMP_SEC_QUERY,
			    F_NOEXTRA | F_DNSSEC, "retry", "dnssec");
	  return;
	}
#endif
      
      /* In strict order mode, there must be a server later in the chain
	 left to send to, otherwise without the forwardall mechanism,
	 code further on will cycle around the list forwever if they
	 all return REFUSED. Note that server is always non-NULL before 
	 this executes. */
      if (option_bool(OPT_ORDER))
	for (server = forward->sentto->next; server; server = server->next)
	  if (!(server->flags & (SERV_LITERAL_ADDRESS | SERV_HAS_DOMAIN | SERV_FOR_NODOTS | SERV_NO_ADDR | SERV_LOOP)))
	    break;

      /* recreate query from reply */
      pheader = find_pseudoheader(header, (size_t)n, &plen, NULL, &is_sign, NULL);
      if (!is_sign && server)
	{
	  header->ancount = htons(0);
	  header->nscount = htons(0);
	  header->arcount = htons(0);
	  if ((nn = resize_packet(header, (size_t)n, pheader, plen)))
	    {
	      header->hb3 &= ~(HB3_QR | HB3_AA | HB3_TC);
	      header->hb4 &= ~(HB4_RA | HB4_RCODE | HB4_CD | HB4_AD);
	      if (forward->flags & FREC_CHECKING_DISABLED)
		header->hb4 |= HB4_CD;
	      if (forward->flags & FREC_AD_QUESTION)
		header->hb4 |= HB4_AD;
	      if (forward->flags & FREC_DO_QUESTION)
		add_do_bit(header, nn,  (unsigned char *)pheader + plen);

#ifdef DNS_FAILOVER_TIMING_SUPPORT
		my_syslog(LOG_DEBUG, _("[%s:%d] daemon->failover_time %d"), __func__,__LINE__, daemon->failover_time);
		if(daemon->failover_time > 0){
			my_syslog(LOG_DEBUG, _("[%s:%d] skip forward_query alan"), __func__,__LINE__);
		}
		else
#endif
			{
#if defined(DNSQUERY_SKBMARK_BINDING)
	      forward_query(-1, NULL, NULL, 0, header, nn, now, forward, forward->flags & FREC_AD_QUESTION, forward->flags & FREC_DO_QUESTION, 0);
#else
	      forward_query(-1, NULL, NULL, 0, header, nn, now, forward, forward->flags & FREC_AD_QUESTION, forward->flags & FREC_DO_QUESTION);
#endif
			}
	      return;
	    }
	}
    }   
   
  server = forward->sentto;
  if ((forward->sentto->flags & SERV_TYPE) == 0)
    {
      if (RCODE(header) == REFUSED)
	server = NULL;
      else
	{
	  struct server *last_server;
	  
	  /* find good server by address if possible, otherwise assume the last one we sent to */ 
	  for (last_server = daemon->servers; last_server; last_server = last_server->next)
	    if (!(last_server->flags & (SERV_LITERAL_ADDRESS | SERV_HAS_DOMAIN | SERV_FOR_NODOTS | SERV_NO_ADDR)) &&
		sockaddr_isequal(&last_server->addr, &serveraddr))
	      {
		server = last_server;
		break;
	      }
	} 
      if (!option_bool(OPT_ALL_SERVERS))
	daemon->last_server = server;
    }
 
  /* We tried resending to this server with a smaller maximum size and got an answer.
     Make that permanent. To avoid reduxing the packet size for a single dropped packet,
     only do this when we get a truncated answer, or one larger than the safe size. */
  if (forward->sentto->edns_pktsz > SAFE_PKTSZ && (forward->flags & FREC_TEST_PKTSZ) && 
      ((header->hb3 & HB3_TC) || n >= SAFE_PKTSZ))
    {
      forward->sentto->edns_pktsz = SAFE_PKTSZ;
      forward->sentto->pktsz_reduced = now;
      (void)prettyprint_addr(&forward->sentto->addr, daemon->addrbuff);
      my_syslog(LOG_WARNING, _("reducing DNS packet size for nameserver %s to %d"), daemon->addrbuff, SAFE_PKTSZ);
    }

    
  /* If the answer is an error, keep the forward record in place in case
     we get a good reply from another server. Kill it when we've
     had replies from all to avoid filling the forwarding table when
     everything is broken */
  /* RTK add to fix the dns respond won't get IP if first dns server reply no domain(no such name)
   * Avoid NXDOMAIN to be handle that handle other dns server's response with IP. 2020/01/09 */
  if (forward->forwardall == 0 || --forward->forwardall == 1 || (RCODE(header) != REFUSED
	&& RCODE(header) != SERVFAIL && RCODE(header) != NOTIMP && RCODE(header) != NXDOMAIN) || RCODE(header) == NOERROR
	  )

    {
      int check_rebind = 0, no_cache_dnssec = 0, cache_secure = 0, bogusanswer = 0;
      
      if (option_bool(OPT_NO_REBIND))
	check_rebind = !(forward->flags & FREC_NOREBIND);
      
      /*   Don't cache replies where DNSSEC validation was turned off, either
	   the upstream server told us so, or the original query specified it.  */
      if ((header->hb4 & HB4_CD) || (forward->flags & FREC_CHECKING_DISABLED))
	no_cache_dnssec = 1;
      
#ifdef HAVE_DNSSEC
      if ((forward->sentto->flags & SERV_DO_DNSSEC) && 
	  option_bool(OPT_DNSSEC_VALID) && !(forward->flags & FREC_CHECKING_DISABLED))
	{
	  int status = 0;

	  /* We've had a reply already, which we're validating. Ignore this duplicate */
	  if (forward->blocking_query)
	    return;
	  
	   /* Truncated answer can't be validated.
	      If this is an answer to a DNSSEC-generated query, we still
	      need to get the client to retry over TCP, so return
	      an answer with the TC bit set, even if the actual answer fits.
	   */
	  if (header->hb3 & HB3_TC)
	    status = STAT_TRUNCATED;
	  
	  while (1)
	    {
	      /* As soon as anything returns BOGUS, we stop and unwind, to do otherwise
		 would invite infinite loops, since the answers to DNSKEY and DS queries
		 will not be cached, so they'll be repeated. */
	      if (status != STAT_BOGUS && status != STAT_TRUNCATED && status != STAT_ABANDONED)
		{
		  if (forward->flags & FREC_DNSKEY_QUERY)
		    status = dnssec_validate_by_ds(now, header, n, daemon->namebuff, daemon->keyname, forward->class);
		  else if (forward->flags & FREC_DS_QUERY)
		    status = dnssec_validate_ds(now, header, n, daemon->namebuff, daemon->keyname, forward->class);
		  else
		    status = dnssec_validate_reply(now, header, n, daemon->namebuff, daemon->keyname, &forward->class, 
						   !option_bool(OPT_DNSSEC_IGN_NS) && (forward->sentto->flags & SERV_DO_DNSSEC),
						   NULL, NULL, NULL);
#ifdef HAVE_DUMPFILE
		  if (status == STAT_BOGUS)
		    dump_packet((forward->flags & (FREC_DNSKEY_QUERY | FREC_DS_QUERY)) ? DUMP_SEC_BOGUS : DUMP_BOGUS,
				header, (size_t)n, &serveraddr, NULL);
#endif
		}
	      
	      /* Can't validate, as we're missing key data. Put this
		 answer aside, whilst we get that. */     
	      if (status == STAT_NEED_DS || status == STAT_NEED_KEY)
		{
		  struct frec *new, *orig;
		  
		  /* Free any saved query */
		  if (forward->stash)
		    blockdata_free(forward->stash);
		  
		  /* Now save reply pending receipt of key data */
		  if (!(forward->stash = blockdata_alloc((char *)header, n)))
		    return;
		  forward->stash_len = n;
		  
		  /* Find the original query that started it all.... */
		  for (orig = forward; orig->dependent; orig = orig->dependent);
		  
		  /* Make sure we don't expire and free the orig frec during the
		     allocation of a new one. */
		  if (--orig->work_counter == 0 || !(new = get_new_frec(now, NULL, orig)))
		    status = STAT_ABANDONED;
		  else
		    {
		      int querytype, fd, type = SERV_DO_DNSSEC;
		      struct frec *next = new->next;
		      char *domain;
		      
		      *new = *forward; /* copy everything, then overwrite */
		      new->next = next;
		      new->blocking_query = NULL;

		      /* Find server to forward to. This will normally be the 
			 same as for the original query, but may be another if
			 servers for domains are involved. */		      
		      if (search_servers(now, NULL, F_DNSSECOK, daemon->keyname, &type, &domain, NULL, 0) == 0)
			{
			  struct server *start, *new_server = NULL;
			  start = server = forward->sentto;
			  
			  while (1)
			    {
			      if (server_test_type(start, domain, type, SERV_DO_DNSSEC))
				{
				  new_server = start;
				  if (server == start)
				    {
				      new_server = NULL;
				      break;
				    }
				}
			      
			      if (!(start = start->next))
				start = daemon->servers;
			      if (start == server)
				break;
			    }
			  
			  if (new_server)
			    server = new_server;
			}
		      
		      new->sentto = server;
		      new->rfds = NULL;
		      new->frec_src.next = NULL;
		      new->flags &= ~(FREC_DNSKEY_QUERY | FREC_DS_QUERY | FREC_HAS_EXTRADATA);
		      new->forwardall = 0;
		      
		      new->dependent = forward; /* to find query awaiting new one. */
		      forward->blocking_query = new; /* for garbage cleaning */
		      /* validate routines leave name of required record in daemon->keyname */
		      if (status == STAT_NEED_KEY)
			{
			  new->flags |= FREC_DNSKEY_QUERY; 
			  querytype = T_DNSKEY;
			}
		      else 
			{
			  new->flags |= FREC_DS_QUERY;
			  querytype = T_DS;
			}

		      nn = dnssec_generate_query(header,((unsigned char *) header) + server->edns_pktsz,
						 daemon->keyname, forward->class, querytype, server->edns_pktsz);

		      memcpy(new->hash, hash_questions(header, nn, daemon->namebuff), HASH_SIZE);
		      new->new_id = get_id();
		      header->id = htons(new->new_id);
		      /* Save query for retransmission */
		      new->stash = blockdata_alloc((char *)header, nn);
		      new->stash_len = nn;
		      
		      /* Don't resend this. */
		      daemon->srv_save = NULL;
		      
		      if ((fd = allocate_rfd(&new->rfds, server)) != -1)
			{
#ifdef HAVE_CONNTRACK
			  if (option_bool(OPT_CONNTRACK))
			    set_outgoing_mark(orig, fd);
#endif
			  server_send_log(server, fd, header, nn, DUMP_SEC_QUERY,
					  F_NOEXTRA | F_DNSSEC, daemon->keyname,
					  querystr("dnssec-query", querytype));
			  server->queries++;
			}
		    }		  
		  return;
		}
	  
	      /* Validated original answer, all done. */
	      if (!forward->dependent)
		break;
	      
	      /* validated subsidiary query, (and cached result)
		 pop that and return to the previous query we were working on. */
	      struct frec *prev = forward->dependent;
	      free_frec(forward);
	      forward = prev;
	      forward->blocking_query = NULL; /* already gone */
	      blockdata_retrieve(forward->stash, forward->stash_len, (void *)header);
	      n = forward->stash_len;
	    }
	
	  
	  no_cache_dnssec = 0;
	  
	  if (status == STAT_TRUNCATED)
	    header->hb3 |= HB3_TC;
	  else
	    {
	      char *result, *domain = "result";
	      
	      if (status == STAT_ABANDONED)
		{
		  result = "ABANDONED";
		  status = STAT_BOGUS;
		}
	      else
		result = (status == STAT_SECURE ? "SECURE" : (status == STAT_INSECURE ? "INSECURE" : "BOGUS"));
	      
	      if (status == STAT_BOGUS && extract_request(header, n, daemon->namebuff, NULL))
		domain = daemon->namebuff;
	      
	      log_query(F_SECSTAT, domain, NULL, result);
	    }
	  
	  if (status == STAT_SECURE)
	    cache_secure = 1;
	  else if (status == STAT_BOGUS)
	    {
	      no_cache_dnssec = 1;
	      bogusanswer = 1;
	    }
	}

#endif

      /* restore CD bit to the value in the query */
      if (forward->flags & FREC_CHECKING_DISABLED)
	header->hb4 |= HB4_CD;
      else
	header->hb4 &= ~HB4_CD;

      /* Never cache answers which are contingent on the source or MAC address EDSN0 option,
	 since the cache is ignorant of such things. */
      if (forward->flags & FREC_NO_CACHE)
	no_cache_dnssec = 1;
      
      if ((nn = process_reply(header, now, forward->sentto, (size_t)n, check_rebind, no_cache_dnssec, cache_secure, bogusanswer, 
			      forward->flags & FREC_AD_QUESTION, forward->flags & FREC_DO_QUESTION, 
			      forward->flags & FREC_ADDED_PHEADER, forward->flags & FREC_HAS_SUBNET, &forward->frec_src.source)))
	{
	  struct frec_src *src;

	  header->id = htons(forward->frec_src.orig_id);
	  header->hb4 |= HB4_RA; /* recursion if available */
#ifdef HAVE_DNSSEC
	  /* We added an EDNSO header for the purpose of getting DNSSEC RRs, and set the value of the UDP payload size
	     greater than the no-EDNS0-implied 512 to have space for the RRSIGS. If, having stripped them and the EDNS0
             header, the answer is still bigger than 512, truncate it and mark it so. The client then retries with TCP. */
	  if (option_bool(OPT_DNSSEC_VALID) && (forward->flags & FREC_ADDED_PHEADER) && (nn > PACKETSZ))
	    {
	      header->ancount = htons(0);
	      header->nscount = htons(0);
	      header->arcount = htons(0);
	      header->hb3 |= HB3_TC;
	      nn = resize_packet(header, nn, NULL, 0);
	    }
#endif

	  for (src = &forward->frec_src; src; src = src->next)
	    {
	      header->id = htons(src->orig_id);
	      
#ifdef HAVE_DUMPFILE
	      dump_packet(DUMP_REPLY, daemon->packet, (size_t)nn, NULL, &src->source);
#endif
	      
	      send_from(src->fd, option_bool(OPT_NOWILD) || option_bool (OPT_CLEVERBIND), daemon->packet, nn, 
			&src->source, &src->dest, src->iface);

	      if (option_bool(OPT_EXTRALOG) && src != &forward->frec_src)
		{
		  daemon->log_display_id = src->log_id;
		  daemon->log_source_addr = &src->source;
		  log_query(F_UPSTREAM, "query", NULL, "duplicate");
		}
	    }
	}

      free_frec(forward); /* cancel */
#if defined(CONFIG_DNS_QUERY_TIMEOUT) || defined(DNS_FAILOVER_TIMING_SUPPORT)
	forward->isreceivereply = 1;
#endif
    }
}


void receive_query(struct listener *listen, time_t now)
{
  struct dns_header *header = (struct dns_header *)daemon->packet;
  union mysockaddr source_addr;
  unsigned char *pheader;
  unsigned short type, udp_size = PACKETSZ; /* default if no EDNS0 */
  union all_addr dst_addr;
  struct in_addr netmask, dst_addr_4;
  size_t m;
  ssize_t n;
  int if_index = 0, auth_dns = 0, do_bit = 0, have_pseudoheader = 0;
#ifdef HAVE_AUTH
  int local_auth = 0;
#endif
  struct iovec iov[1];
  struct msghdr msg;
  struct cmsghdr *cmptr;
#if defined(DNSQUERY_SKBMARK_BINDING)
  unsigned int skb_mark = 0;
  union {
	struct cmsghdr align; /* this ensures alignment */
	char control6[CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(sizeof(skb_mark))];
#if defined(HAVE_LINUX_NETWORK)
	char control[CMSG_SPACE(sizeof(struct in_pktinfo)) + CMSG_SPACE(sizeof(skb_mark))];
#elif defined(IP_RECVDSTADDR) && defined(HAVE_SOLARIS_NETWORK)
	char control[CMSG_SPACE(sizeof(struct in_addr)) +
		CMSG_SPACE(sizeof(unsigned int)) + CMSG_SPACE(sizeof(skb_mark))];
#elif defined(IP_RECVDSTADDR)
	char control[CMSG_SPACE(sizeof(struct in_addr)) +
		CMSG_SPACE(sizeof(struct sockaddr_dl)) + CMSG_SPACE(sizeof(skb_mark))];
#endif
  } control_u;
#else
  union {
    struct cmsghdr align; /* this ensures alignment */
    char control6[CMSG_SPACE(sizeof(struct in6_pktinfo))];
#if defined(HAVE_LINUX_NETWORK)
    char control[CMSG_SPACE(sizeof(struct in_pktinfo))];
#elif defined(IP_RECVDSTADDR) && defined(HAVE_SOLARIS_NETWORK)
    char control[CMSG_SPACE(sizeof(struct in_addr)) +
		 CMSG_SPACE(sizeof(unsigned int))];
#elif defined(IP_RECVDSTADDR)
    char control[CMSG_SPACE(sizeof(struct in_addr)) +
		 CMSG_SPACE(sizeof(struct sockaddr_dl))];
#endif
  } control_u;
#endif
  int family = listen->addr.sa.sa_family;
   /* Can always get recvd interface for IPv6 */
  int check_dst = !option_bool(OPT_NOWILD) || family == AF_INET6;

  /* packet buffer overwritten */
  daemon->srv_save = NULL;

  dst_addr_4.s_addr = dst_addr.addr4.s_addr = 0;
  netmask.s_addr = 0;
  
  if (option_bool(OPT_NOWILD) && listen->iface)
    {
      auth_dns = listen->iface->dns_auth;
     
      if (family == AF_INET)
	{
	  dst_addr_4 = dst_addr.addr4 = listen->iface->addr.in.sin_addr;
	  netmask = listen->iface->netmask;
	}
    }
  
  iov[0].iov_base = daemon->packet;
  iov[0].iov_len = daemon->edns_pktsz;
    
  msg.msg_control = control_u.control;
  msg.msg_controllen = sizeof(control_u);
  msg.msg_flags = 0;
  msg.msg_name = &source_addr;
  msg.msg_namelen = sizeof(source_addr);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  
  if ((n = recvmsg(listen->fd, &msg, 0)) == -1)
    return;
  
  if (n < (int)sizeof(struct dns_header) || 
      (msg.msg_flags & MSG_TRUNC) ||
      (header->hb3 & HB3_QR))
    return;

  /* Clear buffer beyond request to avoid risk of
     information disclosure. */
  memset(daemon->packet + n, 0, daemon->edns_pktsz - n);
  
  source_addr.sa.sa_family = family;
  
  if (family == AF_INET)
    {
       /* Source-port == 0 is an error, we can't send back to that. 
	  http://www.ietf.org/mail-archive/web/dnsop/current/msg11441.html */
      if (source_addr.in.sin_port == 0)
	return;
    }
  else
    {
      /* Source-port == 0 is an error, we can't send back to that. */
      if (source_addr.in6.sin6_port == 0)
	return;
      source_addr.in6.sin6_flowinfo = 0;
    }
  
  /* We can be configured to only accept queries from at-most-one-hop-away addresses. */
  if (option_bool(OPT_LOCAL_SERVICE))
    {
      struct addrlist *addr;

      if (family == AF_INET6) 
	{
	  for (addr = daemon->interface_addrs; addr; addr = addr->next)
	    if ((addr->flags & ADDRLIST_IPV6) &&
		is_same_net6(&addr->addr.addr6, &source_addr.in6.sin6_addr, addr->prefixlen))
	      break;
	}
      else
	{
	  struct in_addr netmask;
	  for (addr = daemon->interface_addrs; addr; addr = addr->next)
	    {
	      netmask.s_addr = htonl(~(in_addr_t)0 << (32 - addr->prefixlen));
	      if (!(addr->flags & ADDRLIST_IPV6) &&
		  is_same_net(addr->addr.addr4, source_addr.in.sin_addr, netmask))
		break;
	    }
	}
      if (!addr)
	{
	  static int warned = 0;
	  if (!warned)
	    {
	      my_syslog(LOG_WARNING, _("Ignoring query from non-local network"));
	      warned = 1;
	    }
	  return;
	}
    }
		
  if (check_dst)
    {
      struct ifreq ifr;

      if (msg.msg_controllen < sizeof(struct cmsghdr))
	return;

#if defined(HAVE_LINUX_NETWORK)
      if (family == AF_INET)
	for (cmptr = CMSG_FIRSTHDR(&msg); cmptr; cmptr = CMSG_NXTHDR(&msg, cmptr))
	  if (cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_PKTINFO)
	    {
	      union {
		unsigned char *c;
		struct in_pktinfo *p;
	      } p;
	      p.c = CMSG_DATA(cmptr);
	      dst_addr_4 = dst_addr.addr4 = p.p->ipi_spec_dst;
	      if_index = p.p->ipi_ifindex;
	    }
#if defined(DNSQUERY_SKBMARK_BINDING)
		else if (cmptr->cmsg_level == SOL_SOCKET && cmptr->cmsg_type == SO_RCVMARK) {
			skb_mark = *((unsigned int*)CMSG_DATA(cmptr));
		}
#endif
#elif defined(IP_RECVDSTADDR) && defined(IP_RECVIF)
      if (family == AF_INET)
	{
	  for (cmptr = CMSG_FIRSTHDR(&msg); cmptr; cmptr = CMSG_NXTHDR(&msg, cmptr))
	    {
	      union {
		unsigned char *c;
		unsigned int *i;
		struct in_addr *a;
#ifndef HAVE_SOLARIS_NETWORK
		struct sockaddr_dl *s;
#endif
	      } p;
	       p.c = CMSG_DATA(cmptr);
	       if (cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_RECVDSTADDR)
		 dst_addr_4 = dst_addr.addr4 = *(p.a);
	       else if (cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_RECVIF)
#ifdef HAVE_SOLARIS_NETWORK
		 if_index = *(p.i);
#else
  	         if_index = p.s->sdl_index;
#endif
#if defined(DNSQUERY_SKBMARK_BINDING)
		else if (cmptr->cmsg_level == SOL_SOCKET && cmptr->cmsg_type == SO_RCVMARK) {
			skb_mark = *((unsigned int*)CMSG_DATA(cmptr));
		}
#endif
	    }
	}
#endif
      
      if (family == AF_INET6)
	{
	  for (cmptr = CMSG_FIRSTHDR(&msg); cmptr; cmptr = CMSG_NXTHDR(&msg, cmptr))
	    if (cmptr->cmsg_level == IPPROTO_IPV6 && cmptr->cmsg_type == daemon->v6pktinfo)
	      {
		union {
		  unsigned char *c;
		  struct in6_pktinfo *p;
		} p;
		p.c = CMSG_DATA(cmptr);
		  
		dst_addr.addr6 = p.p->ipi6_addr;
		if_index = p.p->ipi6_ifindex;
	      }
#if defined(DNSQUERY_SKBMARK_BINDING)
		else if (cmptr->cmsg_level == SOL_SOCKET && cmptr->cmsg_type == SO_RCVMARK) {
			skb_mark = *((unsigned int*)CMSG_DATA(cmptr));
		}
#endif
	}
      
#if defined(DNSQUERY_SKBMARK_BINDING)
	my_syslog(LOG_DEBUG, _("[%s:%d] if_index %u skb_mark=0x%x"), __func__,__LINE__, if_index, skb_mark);
#endif
      /* enforce available interface configuration */
      
      if (!indextoname(listen->fd, if_index, ifr.ifr_name))
	return;

      if (!iface_check(family, &dst_addr, ifr.ifr_name, &auth_dns))
	{
	   if (!option_bool(OPT_CLEVERBIND))
	     enumerate_interfaces(0); 
	   if (!loopback_exception(listen->fd, family, &dst_addr, ifr.ifr_name) &&
	       !label_exception(if_index, family, &dst_addr))
	     return;
	}

      if (family == AF_INET && option_bool(OPT_LOCALISE))
	{
	  struct irec *iface;
	  
	  /* get the netmask of the interface which has the address we were sent to.
	     This is no necessarily the interface we arrived on. */
	  
	  for (iface = daemon->interfaces; iface; iface = iface->next)
	    if (iface->addr.sa.sa_family == AF_INET &&
		iface->addr.in.sin_addr.s_addr == dst_addr_4.s_addr)
	      break;
	  
	  /* interface may be new */
	  if (!iface && !option_bool(OPT_CLEVERBIND))
	    enumerate_interfaces(0); 
	  
	  for (iface = daemon->interfaces; iface; iface = iface->next)
	    if (iface->addr.sa.sa_family == AF_INET &&
		iface->addr.in.sin_addr.s_addr == dst_addr_4.s_addr)
	      break;
	  
	  /* If we failed, abandon localisation */
	  if (iface)
	    netmask = iface->netmask;
	  else
	    dst_addr_4.s_addr = 0;
	}
    }
   
  /* log_query gets called indirectly all over the place, so 
     pass these in global variables - sorry. */
  daemon->log_display_id = ++daemon->log_id;
  daemon->log_source_addr = &source_addr;

#ifdef HAVE_DUMPFILE
  dump_packet(DUMP_QUERY, daemon->packet, (size_t)n, &source_addr, NULL);
#endif
	  
  if (extract_request(header, (size_t)n, daemon->namebuff, &type))
    {
#ifdef CTC_DNS_SPEED_LIMIT
		if(DNS_limit_action != (-1) && handle_dns_speed_limit(type, daemon->namebuff, &source_addr, family))
			return; //drop if not return 0
#endif
#ifdef SUPPORT_DNS_FILTER
		char *if_name[IFNAMSIZ] = {0};
		if(if_indextoname(if_index, (char *)if_name) && strcmp((char *)if_name, "lo"))
		{
			int action=-1;
			struct domain_filter *dns_filter = NULL;
			dns_filter = checkDNSFilter(daemon->namebuff, &source_addr, family);
			/*if(dns_filter)
			  {
			  my_syslog(LOG_INFO, _("[%s:%d] dns_filter->domain=%s, dns_filter->mode=%d"), __func__,__LINE__, dns_filter->domain, dns_filter->mode);
			  my_syslog(LOG_INFO, _("[%s:%d] dns_filter->mac=%s, dns_filter->name=%s"), __func__,__LINE__, dns_filter->mac, dns_filter->name);
			  }*/

			if(dns_filter && (action = dns_filter->action) >=0 && action <= 2)
			{
				dns_filter->count++;
				if(action == 1 || action == 2)
				{
					struct in_addr lanIp;
					char lan_ip6_str[INET6_ADDRSTRLEN] = {0};
					struct in6_addr ip6addr;
					union all_addr addr_a;
					unsigned short flags = 0;

					memset(&addr_a, 0, sizeof(addr_a));
					if(action == 1)
					{
						if(type == T_A)
						{
							mib_get_s(MIB_ADSL_LAN_IP,(void *)&lanIp, sizeof(lanIp));
							addr_a.addr4.s_addr=lanIp.s_addr;
							flags=F_IPV4;
						}
						else if(type == T_AAAA)
						{
							mib_get_s(MIB_IPV6_LAN_LLA_IP_ADDR,lan_ip6_str, sizeof(lan_ip6_str));
							inet_pton(PF_INET6, lan_ip6_str, &ip6addr);
							memcpy(&(addr_a.addr6), &ip6addr, sizeof(struct in6_addr));
							flags=F_IPV6;
						}
					}
					else if(action == 2)
						flags=F_NXDOMAIN;

					n = setup_reply(header, n, &addr_a, flags, daemon->local_ttl);
					send_from(listen->fd, option_bool(OPT_NOWILD), (char *)header, n, &source_addr, &dst_addr, if_index);
				}
				updateDNSFilterAllBlocktimes();
				return ;
			}
		}
#endif

#ifdef HAVE_AUTH
      struct auth_zone *zone;
#endif
      char *types = querystr(auth_dns ? "auth" : "query", type);

      log_query_mysockaddr(F_QUERY | F_FORWARD, daemon->namebuff,
			   &source_addr, types);

#ifdef HAVE_AUTH
      /* find queries for zones we're authoritative for, and answer them directly */
      if (!auth_dns && !option_bool(OPT_LOCALISE))
	for (zone = daemon->auth_zones; zone; zone = zone->next)
	  if (in_zone(zone, daemon->namebuff, NULL))
	    {
	      auth_dns = 1;
	      local_auth = 1;
	      break;
	    }
#endif
      
#ifdef HAVE_LOOP
      /* Check for forwarding loop */
      if (detect_loop(daemon->namebuff, type))
	return;
#endif
    }
  
  if (find_pseudoheader(header, (size_t)n, NULL, &pheader, NULL, NULL))
    { 
      unsigned short flags;
      
      have_pseudoheader = 1;
      GETSHORT(udp_size, pheader);
      pheader += 2; /* ext_rcode */
      GETSHORT(flags, pheader);
      
      if (flags & 0x8000)
	do_bit = 1;/* do bit */ 
	
      /* If the client provides an EDNS0 UDP size, use that to limit our reply.
	 (bounded by the maximum configured). If no EDNS0, then it
	 defaults to 512 */
      if (udp_size > daemon->edns_pktsz)
	udp_size = daemon->edns_pktsz;
      else if (udp_size < PACKETSZ)
	udp_size = PACKETSZ; /* Sanity check - can't reduce below default. RFC 6891 6.2.3 */
    }

#ifdef HAVE_AUTH
  if (auth_dns)
    {
      m = answer_auth(header, ((char *) header) + udp_size, (size_t)n, now, &source_addr, 
		      local_auth, do_bit, have_pseudoheader);
      if (m >= 1)
	{
	  send_from(listen->fd, option_bool(OPT_NOWILD) || option_bool(OPT_CLEVERBIND),
		    (char *)header, m, &source_addr, &dst_addr, if_index);
	  daemon->metrics[METRIC_DNS_AUTH_ANSWERED]++;
	}
    }
  else
#endif
    {
      int ad_reqd = do_bit;
       /* RFC 6840 5.7 */
      if (header->hb4 & HB4_AD)
	ad_reqd = 1;

      m = answer_request(header, ((char *) header) + udp_size, (size_t)n, 
			 dst_addr_4, netmask, now, ad_reqd, do_bit, have_pseudoheader);
      
      if (m >= 1)
	{
#ifdef CONFIG_CMCC
	  if(is_dpi_test)
	  {
		usleep(1000*10);
	  }
#endif
	  send_from(listen->fd, option_bool(OPT_NOWILD) || option_bool(OPT_CLEVERBIND),
		    (char *)header, m, &source_addr, &dst_addr, if_index);
	  daemon->metrics[METRIC_DNS_LOCAL_ANSWERED]++;
	}
#if defined(DNSQUERY_SKBMARK_BINDING)
	else if (forward_query(listen->fd, &source_addr, &dst_addr, if_index,
			header, (size_t)n, now, NULL, ad_reqd, do_bit, skb_mark))
#else
      else if (forward_query(listen->fd, &source_addr, &dst_addr, if_index,
			     header, (size_t)n, now, NULL, ad_reqd, do_bit))
#endif
	{
	daemon->metrics[METRIC_DNS_QUERIES_FORWARDED]++;
	}
      else
	daemon->metrics[METRIC_DNS_LOCAL_ANSWERED]++;
    }
}

#ifdef HAVE_DNSSEC
/* Recurse up the key hierarchy */
static int tcp_key_recurse(time_t now, int status, struct dns_header *header, size_t n, 
			   int class, char *name, char *keyname, struct server *server, 
			   int have_mark, unsigned int mark, int *keycount)
{
  int new_status;
  unsigned char *packet = NULL;
  unsigned char *payload = NULL;
  struct dns_header *new_header = NULL;
  u16 *length = NULL;
 
  while (1)
    {
      int type = SERV_DO_DNSSEC;
      char *domain;
      size_t m; 
      unsigned char c1, c2;
      struct server *firstsendto = NULL;
      
      /* limit the amount of work we do, to avoid cycling forever on loops in the DNS */
      if (--(*keycount) == 0)
	new_status = STAT_ABANDONED;
      else if (status == STAT_NEED_KEY)
	new_status = dnssec_validate_by_ds(now, header, n, name, keyname, class);
      else if (status == STAT_NEED_DS)
	new_status = dnssec_validate_ds(now, header, n, name, keyname, class);
      else 
	new_status = dnssec_validate_reply(now, header, n, name, keyname, &class,
					   !option_bool(OPT_DNSSEC_IGN_NS) && (server->flags & SERV_DO_DNSSEC),
					   NULL, NULL, NULL);
      
      if (new_status != STAT_NEED_DS && new_status != STAT_NEED_KEY)
	break;

      /* Can't validate because we need a key/DS whose name now in keyname.
	 Make query for same, and recurse to validate */
      if (!packet)
	{
	  packet = whine_malloc(65536 + MAXDNAME + RRFIXEDSZ + sizeof(u16));
	  payload = &packet[2];
	  new_header = (struct dns_header *)payload;
	  length = (u16 *)packet;
	}
      
      if (!packet)
	{
	  new_status = STAT_ABANDONED;
	  break;
	}

      m = dnssec_generate_query(new_header, ((unsigned char *) new_header) + 65536, keyname, class, 
				new_status == STAT_NEED_KEY ? T_DNSKEY : T_DS, server->edns_pktsz);
      
      *length = htons(m);

      /* Find server to forward to. This will normally be the 
	 same as for the original query, but may be another if
	 servers for domains are involved. */		      
      if (search_servers(now, NULL, F_DNSSECOK, keyname, &type, &domain, NULL, 0) != 0)
	{
	  new_status = STAT_ABANDONED;
	  break;
	}
	
      while (1)
	{
	  int data_sent = 0;
	  
	  if (!firstsendto)
	    firstsendto = server;
	  else
	    {
	      if (!(server = server->next))
		server = daemon->servers;
	      if (server == firstsendto)
		{
		  /* can't find server to accept our query. */
		  new_status = STAT_ABANDONED;
		  break;
		}
	    }
	  
	  if (!server_test_type(server, domain, type, SERV_DO_DNSSEC))
	    continue;

	retry:
	  /* may need to make new connection. */
	  if (server->tcpfd == -1)
	    {
	      if ((server->tcpfd = socket(server->addr.sa.sa_family, SOCK_STREAM, 0)) == -1)
		continue; /* No good, next server */
	      
#ifdef HAVE_CONNTRACK
	      /* Copy connection mark of incoming query to outgoing connection. */
	      if (have_mark)
		setsockopt(server->tcpfd, SOL_SOCKET, SO_MARK, &mark, sizeof(unsigned int));
#endif	
	      
	      if (!local_bind(server->tcpfd,  &server->source_addr, server->interface, 0, 1))
		{
		  close(server->tcpfd);
		  server->tcpfd = -1;
		  continue; /* No good, next server */
		}
	      
#ifdef MSG_FASTOPEN
	      server_send(server, server->tcpfd, packet, m + sizeof(u16), MSG_FASTOPEN);

	      if (errno == 0)
		data_sent = 1;
#endif
	      
	      if (!data_sent && connect(server->tcpfd, &server->addr.sa, sa_len(&server->addr)) == -1)
		{
		  close(server->tcpfd);
		  server->tcpfd = -1;
		  continue; /* No good, next server */
		}
	      
	      server->flags &= ~SERV_GOT_TCP;
	    }
	  
	  if ((!data_sent && !read_write(server->tcpfd, packet, m + sizeof(u16), 0)) ||
	      !read_write(server->tcpfd, &c1, 1, 1) ||
	      !read_write(server->tcpfd, &c2, 1, 1) ||
	      !read_write(server->tcpfd, payload, (c1 << 8) | c2, 1))
	    {
	      close(server->tcpfd);
	      server->tcpfd = -1;
	      /* We get data then EOF, reopen connection to same server,
		 else try next. This avoids DoS from a server which accepts
		 connections and then closes them. */
	      if (server->flags & SERV_GOT_TCP)
		goto retry;
	      else
		continue;
	    }

	  log_query_mysockaddr(F_NOEXTRA | F_DNSSEC, keyname, &server->addr,
		      querystr("dnssec-query", new_status == STAT_NEED_KEY ? T_DNSKEY : T_DS));

	  server->flags |= SERV_GOT_TCP;
	  
	  m = (c1 << 8) | c2;
	  new_status = tcp_key_recurse(now, new_status, new_header, m, class, name, keyname, server, have_mark, mark, keycount);
	  break;
	}
      
      if (new_status != STAT_OK)
	break;
    }
    
  if (packet)
    free(packet);
    
  return new_status;
}
#endif


/* The daemon forks before calling this: it should deal with one connection,
   blocking as necessary, and then return. Note, need to be a bit careful
   about resources for debug mode, when the fork is suppressed: that's
   done by the caller. */
unsigned char *tcp_request(int confd, time_t now,
			   union mysockaddr *local_addr, struct in_addr netmask, int auth_dns)
{
  size_t size = 0;
  int norebind = 0;
#ifdef HAVE_AUTH
  int local_auth = 0;
#endif
  int checking_disabled, do_bit, added_pheader = 0, have_pseudoheader = 0;
  int check_subnet, cacheable, no_cache_dnssec = 0, cache_secure = 0, bogusanswer = 0;
  size_t m;
  unsigned short qtype;
  unsigned int gotname;
  unsigned char c1, c2;
  /* Max TCP packet + slop + size */
  unsigned char *packet = whine_malloc(65536 + MAXDNAME + RRFIXEDSZ + sizeof(u16));
  unsigned char *payload;
  u16 *length;
  struct dns_header *header;
  if(packet) {
  	payload = &packet[2];
  /* largest field in header is 16-bits, so this is still sufficiently aligned */
  	header = (struct dns_header *)payload;
  	length = (u16 *)packet;
  }
  struct server *last_server;
  struct in_addr dst_addr_4;
  union mysockaddr peer_addr;
  socklen_t peer_len = sizeof(union mysockaddr);
  int query_count = 0;
  unsigned char *pheader;
  unsigned int mark = 0;
  int have_mark = 0;

  (void)mark;
  (void)have_mark;

  if (getpeername(confd, (struct sockaddr *)&peer_addr, &peer_len) == -1)
    return packet;

#ifdef HAVE_CONNTRACK
  /* Get connection mark of incoming query to set on outgoing connections. */
  if (option_bool(OPT_CONNTRACK))
    {
      union all_addr local;
		      
      if (local_addr->sa.sa_family == AF_INET6)
	local.addr6 = local_addr->in6.sin6_addr;
      else
	local.addr4 = local_addr->in.sin_addr;
      
      have_mark = get_incoming_mark(&peer_addr, &local, 1, &mark);
    }
#endif	

  /* We can be configured to only accept queries from at-most-one-hop-away addresses. */
  if (option_bool(OPT_LOCAL_SERVICE))
    {
      struct addrlist *addr;

      if (peer_addr.sa.sa_family == AF_INET6) 
	{
	  for (addr = daemon->interface_addrs; addr; addr = addr->next)
	    if ((addr->flags & ADDRLIST_IPV6) &&
		is_same_net6(&addr->addr.addr6, &peer_addr.in6.sin6_addr, addr->prefixlen))
	      break;
	}
      else
	{
	  struct in_addr netmask;
	  for (addr = daemon->interface_addrs; addr; addr = addr->next)
	    {
	      netmask.s_addr = htonl(~(in_addr_t)0 << (32 - addr->prefixlen));
	      if (!(addr->flags & ADDRLIST_IPV6) && 
		  is_same_net(addr->addr.addr4, peer_addr.in.sin_addr, netmask))
		break;
	    }
	}
      if (!addr)
	{
	  my_syslog(LOG_WARNING, _("Ignoring query from non-local network"));
	  return packet;
	}
    }

  while (1)
    {
      if (query_count == TCP_MAX_QUERIES ||
	  !packet ||
	  !read_write(confd, &c1, 1, 1) || !read_write(confd, &c2, 1, 1) ||
	  !(size = c1 << 8 | c2) ||
	  !read_write(confd, payload, size, 1))
       	return packet; 
  
      if (size < (int)sizeof(struct dns_header))
	continue;

      /* Clear buffer beyond request to avoid risk of
	 information disclosure. */
      memset(payload + size, 0, 65536 - size);
      
      query_count++;

      /* log_query gets called indirectly all over the place, so 
	 pass these in global variables - sorry. */
      daemon->log_display_id = ++daemon->log_id;
      daemon->log_source_addr = &peer_addr;
      
      /* save state of "cd" flag in query */
      if ((checking_disabled = header->hb4 & HB4_CD))
	no_cache_dnssec = 1;
       
      if ((gotname = extract_request(header, (unsigned int)size, daemon->namebuff, &qtype)))
	{
#ifdef HAVE_AUTH
	  struct auth_zone *zone;
#endif
	  char *types = querystr(auth_dns ? "auth" : "query", qtype);
	  
	  log_query_mysockaddr(F_QUERY | F_FORWARD, daemon->namebuff,
			       &peer_addr, types);
	  
#ifdef HAVE_AUTH
	  /* find queries for zones we're authoritative for, and answer them directly */
	  if (!auth_dns && !option_bool(OPT_LOCALISE))
	    for (zone = daemon->auth_zones; zone; zone = zone->next)
	      if (in_zone(zone, daemon->namebuff, NULL))
		{
		  auth_dns = 1;
		  local_auth = 1;
		  break;
		}
#endif
	}
      
      if (local_addr->sa.sa_family == AF_INET)
	dst_addr_4 = local_addr->in.sin_addr;
      else
	dst_addr_4.s_addr = 0;
      
      do_bit = 0;

      if (find_pseudoheader(header, (size_t)size, NULL, &pheader, NULL, NULL))
	{ 
	  unsigned short flags;
	  
	  have_pseudoheader = 1;
	  pheader += 4; /* udp_size, ext_rcode */
	  GETSHORT(flags, pheader);
      
	  if (flags & 0x8000)
	    do_bit = 1; /* do bit */ 
	}

#ifdef HAVE_AUTH
      if (auth_dns)
	m = answer_auth(header, ((char *) header) + 65536, (size_t)size, now, &peer_addr, 
			local_auth, do_bit, have_pseudoheader);
      else
#endif
	{
	   int ad_reqd = do_bit;
	   /* RFC 6840 5.7 */
	   if (header->hb4 & HB4_AD)
	     ad_reqd = 1;
	   
	   /* m > 0 if answered from cache */
	   m = answer_request(header, ((char *) header) + 65536, (size_t)size, 
			      dst_addr_4, netmask, now, ad_reqd, do_bit, have_pseudoheader);
	  
	  /* Do this by steam now we're not in the select() loop */
	  check_log_writer(1); 
	  
	  if (m == 0)
	    {
	      unsigned int flags = 0;
	      union all_addr *addrp = NULL;
	      int type = SERV_DO_DNSSEC;
	      char *domain = NULL;
#if defined(DNSQUERY_PORT_BINDING)
		char *bindport = NULL;
#endif
	      unsigned char *oph = find_pseudoheader(header, size, NULL, NULL, NULL, NULL);

	      size = add_edns0_config(header, size, ((unsigned char *) header) + 65536, &peer_addr, now, &check_subnet, &cacheable);

	      if (gotname)
#if defined(DNSQUERY_PORT_BINDING)
		flags = search_servers(now, &addrp, gotname, daemon->namebuff, &type, &domain, &norebind, &bindport);
#elif defined(DNSQUERY_SKBMARK_BINDING)
		flags = search_servers(now, &addrp, gotname, daemon->namebuff, &type, &domain, &norebind, 0);
#else
		flags = search_servers(now, &addrp, gotname, daemon->namebuff, &type, &domain, &norebind);
#endif

#ifdef HAVE_DNSSEC
	      if (option_bool(OPT_DNSSEC_VALID) && (type & SERV_DO_DNSSEC))
		{
		  size = add_do_bit(header, size, ((unsigned char *) header) + 65536);
		  
		  /* For debugging, set Checking Disabled, otherwise, have the upstream check too,
		     this allows it to select auth servers when one is returning bad data. */
		  if (option_bool(OPT_DNSSEC_DEBUG))
		    header->hb4 |= HB4_CD;
		}
#endif

	      /* Check if we added a pheader on forwarding - may need to
		 strip it from the reply. */
	      if (!oph && find_pseudoheader(header, size, NULL, NULL, NULL, NULL))
		added_pheader = 1;

	      type &= ~SERV_DO_DNSSEC;
	      
	      if (type != 0  || option_bool(OPT_ORDER) || !daemon->last_server)
		last_server = daemon->servers;
	      else
		last_server = daemon->last_server;
	      
	      if (!flags && last_server)
		{
		  struct server *firstsendto = NULL;
		  unsigned char hash[HASH_SIZE];
		  memcpy(hash, hash_questions(header, (unsigned int)size, daemon->namebuff), HASH_SIZE);

		  /* Loop round available servers until we succeed in connecting to one.
		     Note that this code subtly ensures that consecutive queries on this connection
		     which can go to the same server, do so. */
		  while (1) 
		    {
		      int data_sent = 0;

		      if (!firstsendto)
			firstsendto = last_server;
		      else
			{
			  if (!(last_server = last_server->next))
			    last_server = daemon->servers;
			  
			  if (last_server == firstsendto)
			    break;
			}
		      
		      /* server for wrong domain */
		      if (!server_test_type(last_server, domain, type, 0))
			continue;

		    retry:
		      *length = htons(size);

		      if (last_server->tcpfd == -1)
			{
			  if ((last_server->tcpfd = socket(last_server->addr.sa.sa_family, SOCK_STREAM, 0)) == -1)
			    continue;
			  
#ifdef HAVE_CONNTRACK
			  /* Copy connection mark of incoming query to outgoing connection. */
			  if (have_mark)
			    setsockopt(last_server->tcpfd, SOL_SOCKET, SO_MARK, &mark, sizeof(unsigned int));
#endif			  
		      
			  if ((!local_bind(last_server->tcpfd,  &last_server->source_addr, last_server->interface, 0, 1)))
			    {
			      close(last_server->tcpfd);
			      last_server->tcpfd = -1;
			      continue;
			    }
			  
#ifdef MSG_FASTOPEN
			    server_send(last_server, last_server->tcpfd, packet, size + sizeof(u16), MSG_FASTOPEN);

			    if (errno == 0)
			      data_sent = 1;
#endif
			    
			    if (!data_sent && connect(last_server->tcpfd, &last_server->addr.sa, sa_len(&last_server->addr)) == -1)
			    {
			      close(last_server->tcpfd);
			      last_server->tcpfd = -1;
			      continue;
			    }
			  
			  last_server->flags &= ~SERV_GOT_TCP;
			}
		      
		      /* get query name again for logging - may have been overwritten */
		      if (!(gotname = extract_request(header, (unsigned int)size, daemon->namebuff, &qtype)))
			strcpy(daemon->namebuff, "query");
		      
		      if ((!data_sent && !read_write(last_server->tcpfd, packet, size + sizeof(u16), 0)) ||
			  !read_write(last_server->tcpfd, &c1, 1, 1) ||
			  !read_write(last_server->tcpfd, &c2, 1, 1) ||
			  !read_write(last_server->tcpfd, payload, (c1 << 8) | c2, 1))
			{
			  close(last_server->tcpfd);
			  last_server->tcpfd = -1;
			  /* We get data then EOF, reopen connection to same server,
			     else try next. This avoids DoS from a server which accepts
			     connections and then closes them. */
			  if (last_server->flags & SERV_GOT_TCP)
			    goto retry;
			  else
			    continue;
			}
		      
		      last_server->flags |= SERV_GOT_TCP;

		      m = (c1 << 8) | c2;

		      log_query_mysockaddr(F_SERVER | F_FORWARD, daemon->namebuff,
					   &last_server->addr, NULL);

#ifdef HAVE_DNSSEC
		      if (option_bool(OPT_DNSSEC_VALID) && !checking_disabled && (last_server->flags & SERV_DO_DNSSEC))
			{
			  int keycount = DNSSEC_WORK; /* Limit to number of DNSSEC questions, to catch loops and avoid filling cache. */
			  int status = tcp_key_recurse(now, STAT_OK, header, m, 0, daemon->namebuff, daemon->keyname, 
						       last_server, have_mark, mark, &keycount);
			  char *result, *domain = "result";
			  
			  if (status == STAT_ABANDONED)
			    {
			      result = "ABANDONED";
			      status = STAT_BOGUS;
			    }
			  else
			    result = (status == STAT_SECURE ? "SECURE" : (status == STAT_INSECURE ? "INSECURE" : "BOGUS"));
			  
			  if (status == STAT_BOGUS && extract_request(header, m, daemon->namebuff, NULL))
			    domain = daemon->namebuff;

			  log_query(F_SECSTAT, domain, NULL, result);
			  
			  if (status == STAT_BOGUS)
			    {
			      no_cache_dnssec = 1;
			      bogusanswer = 1;
			    }

			  if (status == STAT_SECURE)
			    cache_secure = 1;
			}
#endif

		      /* restore CD bit to the value in the query */
		      if (checking_disabled)
			header->hb4 |= HB4_CD;
		      else
			header->hb4 &= ~HB4_CD;
		      
		      /* There's no point in updating the cache, since this process will exit and
			 lose the information after a few queries. We make this call for the alias and 
			 bogus-nxdomain side-effects. */
		      /* If the crc of the question section doesn't match the crc we sent, then
			 someone might be attempting to insert bogus values into the cache by 
			 sending replies containing questions and bogus answers. */
		      if (memcmp(hash, hash_questions(header, (unsigned int)m, daemon->namebuff), HASH_SIZE) != 0)
			{ 
			  m = 0;
			  break;
			}

		      /* Never cache answers which are contingent on the source or MAC address EDSN0 option,
			 since the cache is ignorant of such things. */
		      if (!cacheable)
			no_cache_dnssec = 1;
		      
		      m = process_reply(header, now, last_server, (unsigned int)m, 
					option_bool(OPT_NO_REBIND) && !norebind, no_cache_dnssec, cache_secure, bogusanswer,
					ad_reqd, do_bit, added_pheader, check_subnet, &peer_addr); 
		      
		      break;
		    }
		}
	
	      /* In case of local answer or no connections made. */
	      if (m == 0)
		{
		  m = setup_reply(header, (unsigned int)size, addrp, flags, daemon->local_ttl);
		  if (have_pseudoheader)
		    m = add_pseudoheader(header, m, ((unsigned char *) header) + 65536, daemon->edns_pktsz, 0, NULL, 0, do_bit, 0);
		}
	    }
	}
	  
      check_log_writer(1);
      
      *length = htons(m);
           
      if (m == 0 || !read_write(confd, packet, m + sizeof(u16), 0))
	return packet;
    }
}

static struct frec *allocate_frec(time_t now)
{
  struct frec *f;
  
  if ((f = (struct frec *)whine_malloc(sizeof(struct frec))))
    {
      f->next = daemon->frec_list;
      f->time = now;
      f->sentto = NULL;
      f->rfds = NULL;
      f->flags = 0;
#ifdef HAVE_DNSSEC
      f->dependent = NULL;
      f->blocking_query = NULL;
      f->stash = NULL;
#endif
#if defined(CONFIG_DNS_QUERY_TIMEOUT) || defined(DNS_FAILOVER_TIMING_SUPPORT)
	f->isreceivereply = 0;
	f->timeout = now + (time_t)DNS_QUERY_TIMEOUT;
	f->fpacket = NULL;
	f->fplen = 0;
#endif
#ifdef DNS_FAILOVER_TIMING_SUPPORT
	f->isFailoverDone = 0;
#endif

      daemon->frec_list = f;
    }

  return f;
}

/* return a UDP socket bound to a random port, have to cope with straying into
   occupied port nos and reserved ones. */
static int random_sock(struct server *s)
{
  int fd;

  if ((fd = socket(s->source_addr.sa.sa_family, SOCK_DGRAM, 0)) != -1)
    {
      if (local_bind(fd, &s->source_addr, s->interface, s->ifindex, 0))
	return fd;

      if (s->interface[0] == 0)
	(void)prettyprint_addr(&s->source_addr, daemon->namebuff);
      else
	strcpy(daemon->namebuff, s->interface);

      my_syslog(LOG_ERR, _("failed to bind server socket to %s: %s"),
		daemon->namebuff, strerror(errno));
      close(fd);
    }
  
  return -1;
}

/* compare source addresses and interface, serv2 can be null. */
static int server_isequal(const struct server *serv1,
			 const struct server *serv2)
{
  return (serv2 &&
    serv2->ifindex == serv1->ifindex &&
    sockaddr_isequal(&serv2->source_addr, &serv1->source_addr) &&
    strncmp(serv2->interface, serv1->interface, IF_NAMESIZE) == 0);
}

/* fdlp points to chain of randomfds already in use by transaction.
   If there's already a suitable one, return it, else allocate a 
   new one and add it to the list. 

   Not leaking any resources in the face of allocation failures
   is rather convoluted here.
   
   Note that rfd->serv may be NULL, when a server goes away.
*/
int allocate_rfd(struct randfd_list **fdlp, struct server *serv)
{
  static int finger = 0;
  int i, j = 0;
  struct randfd_list *rfl;
  struct randfd *rfd = NULL;
  int fd = 0;
  
  /* If server has a pre-allocated fd, use that. */
  if (serv->sfd)
    return serv->sfd->fd;
  
  /* existing suitable random port socket linked to this transaction? */
  for (rfl = *fdlp; rfl; rfl = rfl->next)
    if (server_isequal(serv, rfl->rfd->serv))
      return rfl->rfd->fd;

  /* No. need new link. */
  if ((rfl = daemon->rfl_spare))
    daemon->rfl_spare = rfl->next;
  else if (!(rfl = whine_malloc(sizeof(struct randfd_list))))
    return -1;
   
  /* limit the number of sockets we have open to avoid starvation of 
     (eg) TFTP. Once we have a reasonable number, randomness should be OK */
  for (i = 0; i < daemon->numrrand; i++)
    if (daemon->randomsocks[i].refcount == 0)
      {
	if ((fd = random_sock(serv)) != -1)
    	  {
	    rfd = &daemon->randomsocks[i];
	    rfd->serv = serv;
	    rfd->fd = fd;
	    rfd->refcount = 1;
	  }
	break;
      }
  
  /* No free ones or cannot get new socket, grab an existing one */
  if (!rfd)
    for (j = 0; j < daemon->numrrand; j++)
      {
	i = (j + finger) % daemon->numrrand;
	if (daemon->randomsocks[i].refcount != 0 &&
	    server_isequal(serv, daemon->randomsocks[i].serv) &&
	    daemon->randomsocks[i].refcount != 0xfffe)
	  {
	    finger = i + 1;
	    rfd = &daemon->randomsocks[i];
	    rfd->refcount++;
	    break;
	  }
      }

  if (j == daemon->numrrand)
    {
      struct randfd_list *rfl_poll;

      /* there are no free slots, and non with the same parameters we can piggy-back on. 
	 We're going to have to allocate a new temporary record, distinguished by
	 refcount == 0xffff. This will exist in the frec randfd list, never be shared,
	 and be freed when no longer in use. It will also be held on 
	 the daemon->rfl_poll list so the poll system can find it. */

      if ((rfl_poll = daemon->rfl_spare))
	daemon->rfl_spare = rfl_poll->next;
      else
	rfl_poll = whine_malloc(sizeof(struct randfd_list));
      
      if (!rfl_poll ||
	  !(rfd = whine_malloc(sizeof(struct randfd))) ||
	  (fd = random_sock(serv)) == -1)
	{
	  
	  /* Don't leak anything we may already have */
	  rfl->next = daemon->rfl_spare;
	  daemon->rfl_spare = rfl;

	  if (rfl_poll)
	    {
	      rfl_poll->next = daemon->rfl_spare;
	      daemon->rfl_spare = rfl_poll;
	    }
	  
	  if (rfd)
	    free(rfd);
	  
	  return -1; /* doom */
	}

      /* Note rfd->serv not set here, since it's not reused */
      rfd->fd = fd;
      rfd->refcount = 0xffff; /* marker for temp record */

      rfl_poll->rfd = rfd;
      rfl_poll->next = daemon->rfl_poll;
      daemon->rfl_poll = rfl_poll;
    }
  
  rfl->rfd = rfd;
  rfl->next = *fdlp;
  *fdlp = rfl;
  
  return rfl->rfd->fd;
}

void free_rfds(struct randfd_list **fdlp)
{
  struct randfd_list *tmp, *rfl, *poll, *next, **up;
  
  for (rfl = *fdlp; rfl; rfl = tmp)
    {
      if (rfl->rfd->refcount == 0xffff || --(rfl->rfd->refcount) == 0)
	close(rfl->rfd->fd);

      /* temporary overflow record */
      if (rfl->rfd->refcount == 0xffff)
	{
	  free(rfl->rfd);
	  
	  /* go through the link of all these by steam to delete.
	     This list is expected to be almost always empty. */
	  for (poll = daemon->rfl_poll, up = &daemon->rfl_poll; poll; poll = next)
	    {
	      next = poll->next;
	      
	      if (poll->rfd == rfl->rfd)
		{
		  *up = poll->next;
		  poll->next = daemon->rfl_spare;
		  daemon->rfl_spare = poll;
		}
	      else
		up = &poll->next;
	    }
	}

      tmp = rfl->next;
      rfl->next = daemon->rfl_spare;
      daemon->rfl_spare = rfl;
    }

  *fdlp = NULL;
}

static void free_frec(struct frec *f)
{
  struct frec_src *last;
  
  /* add back to freelist if not the record builtin to every frec. */
  for (last = f->frec_src.next; last && last->next; last = last->next) ;
  if (last)
    {
      last->next = daemon->free_frec_src;
      daemon->free_frec_src = f->frec_src.next;
    }
    
  f->frec_src.next = NULL;    
  free_rfds(&f->rfds);
  f->sentto = NULL;
  f->flags = 0;

#ifdef HAVE_DNSSEC
  if (f->stash)
    {
      blockdata_free(f->stash);
      f->stash = NULL;
    }

  /* Anything we're waiting on is pointless now, too */
  if (f->blocking_query)
    free_frec(f->blocking_query);
  f->blocking_query = NULL;
  f->dependent = NULL;
#endif
#if defined(CONFIG_DNS_QUERY_TIMEOUT) || defined(DNS_FAILOVER_TIMING_SUPPORT)
	time_t nowtime;
	nowtime = dnsmasq_time();
	f->isreceivereply = 0;
	f->timeout = nowtime;
	if(f->fpacket)
		free(f->fpacket);
	f->fpacket = NULL;
	f->fplen = 0;
#endif
#ifdef DNS_FAILOVER_TIMING_SUPPORT
	f->firstsentto = NULL;
	f->isFailoverDone = 0;
#endif

}



/* if wait==NULL return a free or older than TIMEOUT record.
   else return *wait zero if one available, or *wait is delay to
   when the oldest in-use record will expire. Impose an absolute
   limit of 4*TIMEOUT before we wipe things (for random sockets).
   If force is non-NULL, always return a result, even if we have
   to allocate above the limit, and never free the record pointed
   to by the force argument. */
struct frec *get_new_frec(time_t now, int *wait, struct frec *force)
{
  struct frec *f, *oldest, *target;
  int count;
  
  if (wait)
    *wait = 0;

  for (f = daemon->frec_list, oldest = NULL, target =  NULL, count = 0; f; f = f->next, count++)
    if (!f->sentto)
      target = f;
    else 
      {
#ifdef HAVE_DNSSEC
	    /* Don't free DNSSEC sub-queries here, as we may end up with
	       dangling references to them. They'll go when their "real" query 
	       is freed. */
	    if (!f->dependent && f != force)
#endif
	      {
		if (difftime(now, f->time) >= 4*TIMEOUT)
		  {
		    free_frec(f);
		    target = f;
		  }
	     
	    
		if (!oldest || difftime(f->time, oldest->time) <= 0)
		  oldest = f;
	      }
      }

  if (target)
    {
      target->time = now;
      return target;
    }
  
  /* can't find empty one, use oldest if there is one
     and it's older than timeout */
  if (!force && oldest && ((int)difftime(now, oldest->time)) >= TIMEOUT)
    { 
      /* keep stuff for twice timeout if we can by allocating a new
	 record instead */
      if (difftime(now, oldest->time) < 2*TIMEOUT && 
	  count <= daemon->ftabsize &&
	  (f = allocate_frec(now)))
	return f;

      if (!wait)
	{
	  free_frec(oldest);
	  oldest->time = now;
	}
      return oldest;
    }
  
  /* none available, calculate time 'till oldest record expires */
  if (!force && count > daemon->ftabsize)
    {
      if (oldest && wait)
	*wait = oldest->time + (time_t)TIMEOUT - now;
      
      query_full(now);
      
      return NULL;
    }
  
  if (!(f = allocate_frec(now)) && wait)
    /* wait one second on malloc failure */
    *wait = 1;

  return f; /* OK if malloc fails and this is NULL */
}

static void query_full(time_t now)
{
  static time_t last_log = 0;
  
  if ((int)difftime(now, last_log) > 5)
    {
      last_log = now;
      my_syslog(LOG_WARNING, _("Maximum number of concurrent DNS queries reached (max: %d)"), daemon->ftabsize);
    }
}


static struct frec *lookup_frec(unsigned short id, int fd, void *hash)
{
  struct frec *f;
  struct server *s;
  int type;
  struct randfd_list *fdl;
  
  for(f = daemon->frec_list; f; f = f->next)
    if (f->sentto && f->new_id == id && 
	(memcmp(hash, f->hash, HASH_SIZE) == 0))
      {
	/* sent from random port */
	for (fdl = f->rfds; fdl; fdl = fdl->next)
	  if (fdl->rfd->fd == fd)
	  return f;
	
	/* Sent to upstream from socket associated with a server. 
	   Note we have to iterate over all the possible servers, since they may
	   have different bound sockets. */
	type = f->sentto->flags & SERV_TYPE;
	s = f->sentto;
	do {
	  if (server_test_type(s, f->sentto->domain, type, 0) &&
	      s->sfd && s->sfd->fd == fd)
	    return f;
	  
	  s = s->next ? s->next : daemon->servers;
	} while (s != f->sentto);
      }
  
  return NULL;
}

static struct frec *lookup_frec_by_query(void *hash, unsigned int flags)
{
  struct frec *f;

  /* FREC_DNSKEY and FREC_DS_QUERY are never set in flags, so the test below 
     ensures that no frec created for internal DNSSEC query can be returned here.
     
     Similarly FREC_NO_CACHE is never set in flags, so a query which is
     contigent on a particular source address EDNS0 option will never be matched. */

#define FLAGMASK (FREC_CHECKING_DISABLED | FREC_AD_QUESTION | FREC_DO_QUESTION \
		  | FREC_HAS_PHEADER | FREC_DNSKEY_QUERY | FREC_DS_QUERY | FREC_NO_CACHE)
  
  for(f = daemon->frec_list; f; f = f->next)
    if (f->sentto &&
	(f->flags & FLAGMASK) == flags &&
	memcmp(hash, f->hash, HASH_SIZE) == 0)
      return f;
  
  return NULL;
}

/* Send query packet again, if we can. */
void resend_query()
{
  if (daemon->srv_save)
    server_send(daemon->srv_save, daemon->fd_save,
		daemon->packet, daemon->packet_len, 0);
}

/* A server record is going away, remove references to it */
void server_gone(struct server *server)
{
  struct frec *f;
  int i;
  
  for (f = daemon->frec_list; f; f = f->next)
    if (f->sentto && f->sentto == server)
      free_frec(f);

  /* If any random socket refers to this server, NULL the reference.
     No more references to the socket will be created in the future. */
  for (i = 0; i < daemon->numrrand; i++)
    if (daemon->randomsocks[i].refcount != 0 && daemon->randomsocks[i].serv == server)
      daemon->randomsocks[i].serv = NULL;
  
  if (daemon->last_server == server)
    daemon->last_server = NULL;
  
  if (daemon->srv_save == server)
    daemon->srv_save = NULL;
}

/* return unique random ids. */
static unsigned short get_id(void)
{
  unsigned short ret = 0;
  struct frec *f;
  
  while (1)
    {
      ret = rand16();

      /* ensure id is unique. */
      for (f = daemon->frec_list; f; f = f->next)
	if (f->sentto && f->new_id == ret)
	  break;

      if (!f)
	return ret;
    }
}

#ifdef CTC_DNS_SPEED_LIMIT
struct dns_log
{
	char *domain;
	int len;
	unsigned int a_count;
	unsigned int aaaa_count;
	struct domain_limit *domain_limit;
	time_t log_time;
	struct dns_log *next;
};
struct dns_log *dns_logs = NULL;

void check_dns_log_timeout(time_t nowtime)
{
	struct dns_log *current = NULL, *pre = NULL, *next = NULL;
	if(dns_logs)
	{
		pre = current = dns_logs;
		while(current)
		{
			if(difftime(nowtime, current->log_time) >= 60)
			{
				next = current->next;;
				if(pre == current)
					pre = next;
				else
					pre->next = next;
				if(current == dns_logs)
					dns_logs = current->next;

				if(current->domain)
					free(current->domain);
				free(current);
				current = next;
			}
			else {
				pre = current;
				current = current->next;
			}
		}
	}
}

void reset_dns_logs(void)
{
	if(dns_logs)
	{
		struct dns_log *current = NULL;
		current = dns_logs;

		while(current)
		{
			dns_logs = dns_logs->next;

			if(current->domain)
				free(current->domain);

			free(current);
			current = dns_logs;
		}
	}
}

static int qname_compare(const char *q, int qlen, const char *p, 
								int plen, unsigned char rmatch)
{
	int j, k;
	for (j=qlen-1,k=plen-1; j>=0 && k>=0; j--,k--) {
		if(*(q+j) != *(p+k))
			break;
	}
	if(k<0 && (j<0 || rmatch)) return 0;
	return 1;
}

struct dns_log *log_dns_query(unsigned short type, char *name)
{
	struct dns_log *log = NULL;

	if(name == NULL)
		return NULL;

	log = dns_logs;
	while(log)
	{
		if(!strcmp(log->domain, name))
			break;
		log = log->next;
	}

	if(log == NULL)
	{
		struct domain_limit *current_filter = domain_limit_list;
		char found = 0;
		int len = strlen(name);
		while (current_filter)
		{
			//fprintf(stderr, "[%s:%d]current_filter->domain = %s, current_filter->limit = %d\n", __func__, __LINE__, current_filter->domain, current_filter->limit);
			if(strcmp(current_filter->domain, "ALL") == 0)
			{
				found = 1;
				break;
			}
			else if(qname_compare(name, len, current_filter->domain, current_filter->len, 1) == 0)
			{
				found = 1;
				break;
			}
			current_filter = current_filter->next;
		}

		if(found){
			log = malloc(sizeof(struct dns_log));
			memset(log, 0, sizeof(struct dns_log));

			log->domain = strdup(name);
			log->len = strlen(log->domain);
			log->domain_limit = current_filter;
			log->log_time = dnsmasq_time();
			// Add to list head
			log->next = dns_logs;
			dns_logs = log;
		}
	}

	if(log){
		if(type == T_A)
			log->a_count++;
		else if(type == T_AAAA)
			log->aaaa_count++;
	}

	return log;
}

int is_dns_limit_overflow(struct dns_log *log)
{
	//fprintf(stderr, "[%s:%d]log->a_count = %d, log->aaaa_count = %d, log->domain_limit->limit =%d\n", __func__, __LINE__, log->a_count, log->aaaa_count, log->domain_limit->limit);
	if(log->domain_limit && ((int)(log->a_count + log->aaaa_count) > log->domain_limit->limit))
		return 1;

	return 0;
}

static int check_duplicated_dev_info(MIB_CE_DNS_LIMIT_DEV_INFO_T *pEntry)
{
	MIB_CE_DNS_LIMIT_DEV_INFO_T target;
	int total, i;

	if(pEntry == NULL)
		return 0;

	total = mib_chain_total(DNS_LIMIT_DEV_INFO_TBL);
	for(i = 0 ; i < total ; i++)
	{
		if(mib_chain_get(DNS_LIMIT_DEV_INFO_TBL, i, &target) == 0)
			continue;

		if(strcmp(target.domain, pEntry->domain) == 0
			&& target.ip_ver == pEntry->ip_ver
			&& memcmp(target.mac, pEntry->mac, MAC_ADDR_LEN) == 0)
		{
			target.record_time = (unsigned int)dnsmasq_time();
			mib_chain_update(DNS_LIMIT_DEV_INFO_TBL, (void *)&target, i);
			return 1;
		}
	}

	return 0;
}

static void macStr2Array(const char* str, char sep, char *mac, int maxBytes, int base)
{
	int i;
	for (i = 0; i < maxBytes; i++) {
		mac[i] = strtoul(str, NULL, base);  // Convert byte
		str = strchr(str, sep);               // Find next separator
		if (str == NULL || *str == '\0') {
			break;                            // No more separators, exit
		}
		str++;                                // Point to next character after separator
	}
}

static void add_dns_limit_dev_info(char *name, union mysockaddr *addr, int family)
{
	int i = 0, total = mib_chain_total(DNS_LIMIT_DEV_INFO_TBL);
	MIB_CE_DNS_LIMIT_DEV_INFO_T entry;

	if(name == NULL || addr == NULL)
		return;

	bzero(&entry, sizeof(entry));
	snprintf(entry.domain, sizeof(entry.domain), "%s", name);

	if(family== AF_INET)
	{
		char buff[ADDRSTRLEN] = {0};

		entry.ip_ver = IPVER_IPV4;
		memcpy(entry.ip_addr, &addr->in.sin_addr, IP_ADDR_LEN);

		inet_ntop(AF_INET, &addr->in.sin_addr, buff, ADDRSTRLEN);
		//fprintf(stderr, "add %s into dev info\n", buff);

		// get mac address
		int s;
		struct arpreq arpreq;
		struct sockaddr_in *sin;

		memset(&arpreq, 0, sizeof(struct arpreq));
		sin = (struct sockaddr_in *) &arpreq.arp_pa;
		sin->sin_family = AF_INET;
		memcpy(&sin->sin_addr, &addr->in.sin_addr, IP_ADDR_LEN);
		strcpy(arpreq.arp_dev, "br0");

		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s < 0)
		{
			perror("[dns_speed_limit] socket");
			return;
		}
		if (ioctl(s, SIOCGARP, &arpreq) < 0)
		{
			perror("[dns_speed_limit] ioctl");
			close(s);
			return;
		}

		if (!(arpreq.arp_flags & ATF_COM))
		{
			close(s);
			return ;
		}

		memcpy(entry.mac, arpreq.arp_ha.sa_data, MAC_ADDR_LEN);
		close(s);
	}
	else if(family == AF_INET6)
	{
		FILE *fp = NULL;
		int count = 0, ret = 0;
		char cmd[32] = {0}, buf[256] = {0};
		unsigned char mac[6] = {0};
		char *delim = " ";
		char *p = NULL, *ep = NULL;
		struct sockaddr_in6 sa6;

		snprintf(cmd, sizeof(cmd), "ip -6 neighbor");
		count = 0;
		while (fp == NULL && count < 5)
		{
			fp = popen(cmd, "r");
			count++;
			usleep(100000); // 100ms
		}

		if (fp)
		{
			while (fgets(buf, sizeof(buf), fp))
			{
				ep = buf + strlen(buf);
				if (*(ep - 1) != '\n')
				{
					break;
				}
				//fprintf(stderr, "[%s:%d] buf = %s\n", __func__, __LINE__, buf);
				p = strtok(buf, delim);
				//fprintf(stderr, "[%s:%d] v6 addr = %s\n", __func__, __LINE__, p);

				inet_pton(AF_INET6, p, &(sa6.sin6_addr));
				ret = memcmp((void*)&(sa6.sin6_addr), (void*)&(addr->in6.sin6_addr), sizeof(char)*16);

				count = 0;
				if (0 == ret)
				{
					while ((p = strtok(NULL, delim)) != NULL)
					{
						count++;
						if (count == 4)
						{
							fprintf(stderr, "[%s:%d] MAC = %s\n", __func__, __LINE__, p);
							macStr2Array((const char *)p, ':', (char *)mac, 6, 16);
							entry.ip_ver = IPVER_IPV6;
							memcpy(entry.ip6_addr, &addr->in6.sin6_addr, IP6_ADDR_LEN);
							memcpy(entry.mac, mac, MAC_ADDR_LEN);
							break;
						}
					}
				}
			}
			pclose(fp);
		}
	}
	else
		return;

	if (entry.ip_ver != 0)
	{
		if (check_duplicated_dev_info(&entry) == 0)
		{
			entry.record_time = (unsigned int)dnsmasq_time();
			if (total == DNS_LIMIT_MAX)
			{
				// replace oldest record
				MIB_CE_DNS_LIMIT_DEV_INFO_T target;
				int oldest_idx = (-1), oldest_time = 0;
				for (i = 0; i < total; i++)
				{
					if (mib_chain_get(DNS_LIMIT_DEV_INFO_TBL, i, &target) == 0)
						continue;

					if (oldest_idx == (-1) && oldest_time == 0)
					{
						oldest_idx = i;
						oldest_time = target.record_time;
					}
					else
					{
						if (target.record_time < (unsigned int)oldest_time)
						{
							oldest_idx = i;
							oldest_time = target.record_time;
						}
					}
				}

				if (oldest_idx >= 0 && oldest_idx < total)
				{
					mib_chain_update(DNS_LIMIT_DEV_INFO_TBL, (void *)&entry, oldest_idx);
				}
			}
			else
			{
				mib_chain_add(DNS_LIMIT_DEV_INFO_TBL, &entry);
			}
			if (DNS_limit_action == DNS_LIMIT_ACTION_ALERT)
			{
				unsigned int vUint = 0;
				mib_get_s(CWMP_INFORM_USER_EVENTCODE,&vUint, sizeof(vUint));
				if (!(vUint & EC_X_CT_COM_DNSLIMITALERT)) {
					vUint = vUint | EC_X_CT_COM_DNSLIMITALERT;
					mib_set(CWMP_INFORM_USER_EVENTCODE, &vUint);
				}
			}
		}
	}

	return;
}

int handle_dns_speed_limit(unsigned short type, char *name, union mysockaddr *addr, int family)
{
	struct dns_log *log = NULL;

	if(type != 1 && type != 28)
		return 0;

	if(name == NULL)
		return 0;

	log = log_dns_query(type, name);
	if(log && is_dns_limit_overflow(log))
	{
		add_dns_limit_dev_info(log->domain, addr, family);
		if(DNS_limit_action == DNS_LIMIT_ACTION_DROP)
			return -1;
	}
	return 0;
}
#endif

#ifdef SUPPORT_DNS_FILTER
void getDeviceMac(union mysockaddr *addr,int family, unsigned char *macstring)
{
	if((addr == NULL) || (macstring == NULL))
		return;
	if(family== AF_INET)
	{
		int s;
		struct arpreq arpreq;
		struct sockaddr_in *sin;
		char buff[ADDRSTRLEN] = {0};

		inet_ntop(AF_INET, &addr->in.sin_addr, buff, ADDRSTRLEN);

		// get mac address
		memset(&arpreq, 0, sizeof(struct arpreq));
		sin = (struct sockaddr_in *) &arpreq.arp_pa;
		sin->sin_family = AF_INET;
		memcpy(&sin->sin_addr, &addr->in.sin_addr, IP_ADDR_LEN);
		strcpy(arpreq.arp_dev, "br0");

		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s < 0)
		{
			perror("[getDeviceMac] socket");
			return;
		}
		if (ioctl(s, SIOCGARP, &arpreq) < 0)
		{
			perror("[getDeviceMac] ioctl");
			close(s);
			return;
		}

		if (!(arpreq.arp_flags & ATF_COM))
		{
			close(s);
			return ;
		}

		memcpy(macstring, arpreq.arp_ha.sa_data, MAC_ADDR_LEN);
		close(s);
	}
	else if(family == AF_INET6)
	{
		FILE *fp = NULL;
		int count = 0, ret = 0;
		char cmd[32] = {0}, buf[256] = {0};
		unsigned char mac[6] = {0};
		char *delim = " ";
		char *p = NULL, *ep = NULL;
		struct sockaddr_in6 sa6;

		snprintf(cmd, sizeof(cmd), "ip -6 neighbor");
		count = 0;
		while (fp == NULL && count < 5)
		{
			fp = popen(cmd, "r");
			count++;
			usleep(100000); // 100ms
		}

		if (fp)
		{
			while (fgets(buf, sizeof(buf), fp))
			{
				ep = buf + strlen(buf);
				if (*(ep - 1) != '\n')
				{
					break;
				}
				//fprintf(stderr, "[%s:%d] buf = %s\n", __func__, __LINE__, buf);
				p = strtok(buf, delim);
				//fprintf(stderr, "[%s:%d] v6 addr = %s\n", __func__, __LINE__, p);

				inet_pton(AF_INET6, p, &(sa6.sin6_addr));
				ret = memcmp((void*)&(sa6.sin6_addr), (void*)&(addr->in6.sin6_addr), sizeof(char)*16);

				count = 0;
				if (0 == ret)
				{
					while ((p = strtok(NULL, delim)) != NULL)
					{
						count++;
						if (count == 4)
						{
							//fprintf(stderr, "[%s:%d] MAC = %s\n", __func__, __LINE__, p);
							macStr2Array((const char *)p, ':', (char *)mac, 6, 16);
							memcpy(macstring, mac, MAC_ADDR_LEN);
							break;
						}
					}
				}
			}
			pclose(fp);
		}
	}
}

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
int checkMatchedDomain(char *domain, char *hostnamelist, char *namebuff)
{
	if(domain == NULL || hostnamelist == NULL || namebuff == NULL)
		return 0;

	if(!strcmp(domain, "*") || strstr(namebuff, domain) || strstr(domain, namebuff))
		return 1;

	if(hostnamelist)
	{
		char dnsname_parse[256]={0};
		char *pch=NULL;
		char *saved=NULL;

		snprintf(dnsname_parse, sizeof(dnsname_parse), "%s", hostnamelist);
		pch = strtok_r(dnsname_parse, ";", &saved);
		while (pch != NULL){
			//fprintf(stderr, "[%s:%d] pch=%s\n", __func__,__LINE__, pch);
			if(strstr(namebuff, pch))
				return 1;
			pch = strtok_r(NULL, ";", &saved);
		}
	}

	return 0;
}

int checkDNSFilterTime(struct domain_filter *dns_filter)
{
	int timecheck = 0;
	int StartTime1 = 0, EndTime1 = 0;
	int StartTime2 = 0, EndTime2 = 0;
	int StartTime3 = 0, EndTime3 = 0;

	if(dns_filter == NULL)
		return 0;

	StartTime1 = getMinutes(dns_filter->start_hr1, dns_filter->start_min1);
	EndTime1 = getMinutes(dns_filter->end_hr1, dns_filter->end_min1);
	StartTime2 = getMinutes(dns_filter->start_hr2, dns_filter->start_min2);
	EndTime2 = getMinutes(dns_filter->end_hr2, dns_filter->end_min2);
	StartTime3 = getMinutes(dns_filter->start_hr3, dns_filter->start_min3);
	EndTime3 = getMinutes(dns_filter->end_hr3, dns_filter->end_min3);
	timecheck = Filter_time_check(StartTime1, EndTime1, StartTime2, EndTime2, StartTime3,EndTime3, dns_filter->WeekDays);

	return timecheck;
}
#endif

struct domain_filter *checkDNSFilter(char *domain, union mysockaddr *addr,int family)
{
	int found=0;
	char mac[20]={0};
	unsigned char DevMac[MAC_ADDR_LEN] ={0};
	struct domain_filter *dns_filter;

	getDeviceMac(addr, family, DevMac);
	snprintf(mac, 20, "%02x:%02x:%02x:%02x:%02x:%02x", DevMac[0], DevMac[1], DevMac[2], DevMac[3], DevMac[4], DevMac[5]);
	dns_filter = domain_filter_list;
	while(dns_filter)
	{
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		if(((strlen(dns_filter->mac) == 0) || (dns_filter->mac[0] && strcasecmp(dns_filter->mac, mac))))
#else
		if(dns_filter->mac[0] && strcasecmp(dns_filter->mac, mac))
#endif
		{
			dns_filter = dns_filter->next;
			continue;
		}

		found = 1;
#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
		//fprintf(stderr, "[%s:%d] domain=%s, dns_filter->domain=%s, dns_filter->hostnamelist=%s, dns_filter->mode=%d\n", 
		//__func__,__LINE__, domain, dns_filter->domain, dns_filter->hostnamelist, dns_filter->mode);
		if(((dns_filter->mode == 0) && (checkMatchedDomain(dns_filter->domain, (char *)dns_filter->hostnamelist, domain) && checkDNSFilterTime(dns_filter)))
			|| ((dns_filter->mode == 1) &&( !checkMatchedDomain(dns_filter->domain, (char *)dns_filter->hostnamelist, domain) || !checkDNSFilterTime(dns_filter))))
#else
		if(strstr(domain, dns_filter->domain))
#endif
		{
			return dns_filter;
		}
		dns_filter = dns_filter->next;
	}

#ifdef SUPPORT_URLFILTER_DNSFILTER_NEW_SPEC
	if((dns_filter == NULL) && (found == 0))
	{
		dns_filter = domain_filter_list;
		while(dns_filter)
		{
			if(strlen(dns_filter->mac))
			{
				dns_filter = dns_filter->next;
				continue;
			}
			//my_syslog(LOG_INFO, _("[%s:%d] dns_filter->domain=%s, dns_filter->mode=%d"), __func__,__LINE__, dns_filter->domain, dns_filter->mode);
			if(((dns_filter->mode == 0) && (checkMatchedDomain(dns_filter->domain, (char *)dns_filter->hostnamelist, domain) && checkDNSFilterTime(dns_filter)))
				|| ((dns_filter->mode == 1) && (!checkMatchedDomain(dns_filter->domain, (char *)dns_filter->hostnamelist, domain) || !checkDNSFilterTime(dns_filter))))
			{
				return dns_filter;
			}
			dns_filter = dns_filter->next;
		}
	}
#endif

	return NULL;
}
#endif

#if defined(CONFIG_DNS_QUERY_TIMEOUT) || defined(DNS_FAILOVER_TIMING_SUPPORT)
void check_DNS_Timeout(void)
{
	struct frec *f;
	time_t nowtime;
	nowtime = dnsmasq_time();
#ifdef CTC_DNS_SPEED_LIMIT
	check_dns_log_timeout(nowtime);
#endif

	for(f = daemon->frec_list; f; f = f->next){
		if(f->isreceivereply == 0){
			if((difftime(nowtime, f->timeout) >= 0) && (f->fpacket != NULL) && (f->frec_src.fd != -1)){
				my_syslog(LOG_DEBUG, _("[%s:%d] timeout~~~~~~~daemon->namebuff=%s"), __func__,__LINE__, f->namebuff);
				f->fpacket->id = htons(f->frec_src.orig_id);
				f->fplen = setup_reply((struct dns_header*)f->fpacket, f->fplen, NULL, F_NXDOMAIN, daemon->local_ttl);
				send_from(f->frec_src.fd, *(daemon->options) & OPT_NOWILD, (char *)f->fpacket, f->fplen, &f->frec_src.source, &f->frec_src.dest, f->frec_src.iface);
				free_frec(f); /* cancel */
				f->isreceivereply = 1;
			}
#ifdef DNS_FAILOVER_TIMING_SUPPORT
			else if ((daemon->failover_time > 0) && (difftime(nowtime, f->failover) >= 0) && (f->fpacket != NULL)) {
				if (f->isFailoverDone || (!daemon->servers))
					continue;
#if defined(DNSQUERY_SKBMARK_BINDING)
				forward_query(f->frec_src.fd, &f->frec_src.source, &f->frec_src.dest, f->frec_src.iface, f->fpacket, f->fplen, nowtime, f, f->flags & FREC_AD_QUESTION, f->flags & FREC_DO_QUESTION, f->skb_mark);
#else
				forward_query(f->frec_src.fd, &f->frec_src.source, &f->frec_src.dest, f->frec_src.iface, f->fpacket, f->fplen, nowtime, f, f->flags & FREC_AD_QUESTION, f->flags & FREC_DO_QUESTION);
#endif
			}
#endif
		}
#ifdef DNS_FAILOVER_TIMING_SUPPORT
		else if ((daemon->failover_time > 0) && (difftime(nowtime, f->failover) >= 0) && (f->fpacket != NULL)) {
			if (f->isFailoverDone || (!daemon->servers))
				continue;
#if defined(DNSQUERY_SKBMARK_BINDING)
			forward_query(f->frec_src.fd, &f->frec_src.source, &f->frec_src.dest, f->frec_src.iface, f->fpacket, f->fplen, nowtime, f, f->flags & FREC_AD_QUESTION, f->flags & FREC_DO_QUESTION, f->skb_mark);
#else
			forward_query(f->frec_src.fd, &f->frec_src.source, &f->frec_src.dest, f->frec_src.iface, f->fpacket, f->fplen, nowtime, f, f->flags & FREC_AD_QUESTION, f->flags & FREC_DO_QUESTION);
#endif
		}
#endif
	}
}
#endif
