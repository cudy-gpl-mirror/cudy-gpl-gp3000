/*
 * socket.c -- DHCP server client/server socket creation
 *
 * udhcp client/server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *			Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <errno.h>
#include <features.h>
#if __GLIBC__ >=2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#endif

#include "debug.h"
#define USE_SOCK_FILTER
#ifdef USE_SOCK_FILTER
#include <linux/filter.h>
#endif

int read_interface(char *interface, int *ifindex, u_int32_t *addr, unsigned char *arp)
{
	int fd;
	struct ifreq ifr;
	struct sockaddr_in *our_ip;

	memset(&ifr, 0, sizeof(struct ifreq));
	if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
		ifr.ifr_addr.sa_family = AF_INET;
		strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name) - 1);
		ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

		if (addr) { 
			if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
				our_ip = (struct sockaddr_in *) &ifr.ifr_addr;
				*addr = our_ip->sin_addr.s_addr;
				DEBUG(LOG_INFO, "%s (our ip) = %s", ifr.ifr_name, inet_ntoa(our_ip->sin_addr));
			} else {
				LOG(LOG_ERR, "SIOCGIFADDR failed, is the interface up and configured?: %s", 
						strerror(errno));
				close(fd);
				return -1;
			}
		}
		
		if (ioctl(fd, SIOCGIFINDEX, &ifr) == 0) {
			DEBUG(LOG_INFO, "adapter index %d", ifr.ifr_ifindex);
			*ifindex = ifr.ifr_ifindex;
		} else {
			LOG(LOG_ERR, "SIOCGIFINDEX failed!: %s", strerror(errno));
			close(fd);
			return -1;
		}
		if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
			memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);
			DEBUG(LOG_INFO, "adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x",
				arp[0], arp[1], arp[2], arp[3], arp[4], arp[5]);
		} else {
			LOG(LOG_ERR, "SIOCGIFHWADDR failed!: %s", strerror(errno));
			close(fd);
			return -1;
		}
	} else {
		LOG(LOG_ERR, "socket failed!: %s", strerror(errno));
		return -1;
	}
	close(fd);
	return 0;
}


int listen_socket(unsigned int ip, int port, char *inf)
{
	struct ifreq interface={0};
	int fd;
	struct sockaddr_in addr;
	int n = 1;

	DEBUG(LOG_INFO, "Opening listen socket on 0x%08x:%d %s\n", ip, port, inf);
	if ((fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		DEBUG(LOG_ERR, "socket call failed: %s", strerror(errno));
		return -1;
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = ip;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(n)) == -1) {
		close(fd);
		return -1;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *) &n, sizeof(n)) == -1) {
		close(fd);
		return -1;
	}
	memset(interface.ifr_ifrn.ifrn_name, 0, IFNAMSIZ);
	strncpy(interface.ifr_ifrn.ifrn_name, inf, IFNAMSIZ-1);
	if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,(char *)&interface, sizeof(interface)) < 0) {
		close(fd);
		return -1;
	}

	if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1) {
		close(fd);
		return -1;
	}
	
	return fd;
}

#if defined(SEND_RELEASE_BY_RAWSOCK)
int raw_socket(int ifindex)
{
	int fd;
	struct sockaddr_ll sock;
#ifdef USE_SOCK_FILTER
	struct sock_fprog filter;
	/*
	"tcpdump -dd udp dst port 68"
	"-dd: Dump packet-matching code as a C program fragment."
	*/
	struct sock_filter bpf_code[] = {
		{ 0x28, 0, 0, 0x0000000c },
		{ 0x15, 0, 4, 0x000086dd },
		{ 0x30, 0, 0, 0x00000014 },
		{ 0x15, 0, 11, 0x00000011 },
		{ 0x28, 0, 0, 0x00000038 },
		{ 0x15, 8, 9, 0x00000044 },
		{ 0x15, 0, 8, 0x00000800 },
		{ 0x30, 0, 0, 0x00000017 },
		{ 0x15, 0, 6, 0x00000011 },
		{ 0x28, 0, 0, 0x00000014 },
		{ 0x45, 4, 0, 0x00001fff },
		{ 0xb1, 0, 0, 0x0000000e },
		{ 0x48, 0, 0, 0x00000010 },
		{ 0x15, 0, 1, 0x00000044 },
		{ 0x6, 0, 0, 0x0000ffff },
		{ 0x6, 0, 0, 0x00000000 }
	};
#endif

	DEBUG(LOG_INFO, "Opening raw socket on ifindex %d\n", ifindex);
	if ((fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
		DEBUG(LOG_ERR, "socket call failed: %s", strerror(errno));
		return -1;
	}

	sock.sll_family = AF_PACKET;
	sock.sll_protocol = htons(ETH_P_IP);
	sock.sll_ifindex = ifindex;
	if (bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0) {
		DEBUG(LOG_ERR, "bind call failed: %s", strerror(errno));
		close(fd);
		return -1;
	}
#ifdef USE_SOCK_FILTER
	filter.filter = bpf_code;
	filter.len = sizeof(bpf_code)/sizeof(struct sock_filter);;
	if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)) < 0 )
	{
		perror("Cannot attach filter");
		close(fd);
		return -1;
	}
#endif
	return fd;
}
#else
int raw_socket(int ifindex)
{
	int fd;
	struct sockaddr_ll sock;
#ifdef USE_SOCK_FILTER
	struct sock_fprog filter;
	
	/*
	 *	tcpdump -dd udp dst port 68
	 *	-dd: Dump packet-matching code as a C program fragment.
	 *	The packet-matching code is used for RAW socket, this code
	 *	below is modified for IP socket.
	 */
	struct sock_filter bpf_code[] = {
		{ 0x30, 0, 0, 0x00000009 },
		{ 0x15, 0, 6, 0x00000011 },
		{ 0x28, 0, 0, 0x00000006 },
		{ 0x45, 4, 0, 0x00001fff },
		{ 0xb1, 0, 0, 0x00000000 },
		{ 0x48, 0, 0, 0x00000002 },
		{ 0x15, 0, 1, 0x00000044 },
		{ 0x6, 0, 0, 0x0000ffff },
		{ 0x6, 0, 0, 0x00000000 },
	};
#endif

	DEBUG(LOG_INFO, "Opening raw socket on ifindex %d\n", ifindex);
	if ((fd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0) {
		DEBUG(LOG_ERR, "socket call failed: %s", strerror(errno));
		return -1;
	}
	
#ifdef USE_SOCK_FILTER
	filter.filter = bpf_code;
	filter.len = sizeof(bpf_code)/sizeof(struct sock_filter);
	
	if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)) < 0 )
	{
		DEBUG(LOG_ERR, "set sockopt failed: %s", strerror(errno));
		close(fd);
		return -1;
	}
#endif
	memset(&sock, 0, sizeof(sock));
	sock.sll_family = AF_PACKET;
	sock.sll_protocol = htons(ETH_P_IP);
	sock.sll_ifindex = ifindex;
	if (bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0) {
		DEBUG(LOG_ERR, "bind call failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}
#endif
