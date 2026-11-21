/*
 * arpping.c
 *
 * Mostly stolen from: dhcpcd - DHCP client daemon
 * by Yoichi Hariguchi <yoichi@fore.com>
 */

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>


#include "dhcpd.h"
#include "debug.h"
#include "arpping.h"

/* args:	yiaddr - what IP to ping
 *		ip - our ip
 *		mac - our arp address
 *		interface - interface to use
 * retn: 	1 addr free
 *		0 addr used
 *		-1 error 
 */  

/* FIXME: match response against chaddr */
int arpping(u_int32_t yiaddr, u_int32_t ip, unsigned char *mac, char *interface)
{

//	int	timeout = 2;
	int   timeout = 1;
	int 	optval = 1;
	int	s;			/* socket */
	int	rv = 1;			/* return value */
	struct sockaddr addr;		/* for interface name */
	struct arpMsg	arp;
	fd_set		fdset;
	struct timeval	tm;
	time_t		prevTime;
	u_int32_t u32_srcIP;

	if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
		LOG(LOG_ERR, "Could not open raw socket");
		return -1;
	}
	
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
		LOG(LOG_ERR, "Could not setsocketopt on raw socket");
		close(s);
		return -1;
	}

	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6);	/* MAC DA */
	memcpy(arp.ethhdr.h_source, mac, 6);		/* MAC SA */
	arp.ethhdr.h_proto = htons(ETH_P_ARP);		/* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER);		/* hardware type */
	arp.ptype = htons(ETH_P_IP);			/* protocol type (ARP message) */
	arp.hlen = 6;					/* hardware address length */
	arp.plen = 4;					/* protocol address length */
	arp.operation = htons(ARPOP_REQUEST);		/* ARP op code */
	memcpy(arp.sInaddr, &ip, sizeof(ip));		/* source IP address */
	memcpy(arp.sHaddr, mac, 6);			/* source hardware address */
	memcpy(arp.tInaddr, &yiaddr, sizeof(yiaddr));	/* target IP address */
	
	memset(&addr, 0, sizeof(addr));
	snprintf(addr.sa_data, sizeof(addr.sa_data), "%s", interface);
	if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
		rv = 0;
	
	/* wait arp reply, and check it */
	tm.tv_usec = 0;
	time(&prevTime);
	while (timeout > 0) {
		FD_ZERO(&fdset);
		FD_SET(s, &fdset);
		tm.tv_sec = timeout;
		if (select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) < 0) {
			DEBUG(LOG_ERR, "Error on ARPING request: %s", strerror(errno));
			if (errno != EINTR) rv = 0;
		} else if (FD_ISSET(s, &fdset)) {
			if (recv(s, &arp, sizeof(arp), 0) < 0 ) rv = 0;
			memcpy(&u32_srcIP, arp.sInaddr, sizeof(u32_srcIP));
			
			if (arp.operation == htons(ARPOP_REPLY) && 
			    bcmp(arp.tHaddr, mac, 6) == 0 && 
			    u32_srcIP == yiaddr) {
			    printf("\nconflict address mac=%x-%x-%x-%x-%x-%x\n",arp.sHaddr[0],arp.sHaddr[1],arp.sHaddr[2],arp.sHaddr[3],arp.sHaddr[4],arp.sHaddr[5]);
				DEBUG(LOG_INFO, "Valid arp reply receved for this address");
				rv = 0;
				break;
			}
		}
		timeout -= time(NULL) - prevTime;
		time(&prevTime);
	}
	close(s);
	DEBUG(LOG_INFO, "%salid arp replies for this address", rv ? "No v" : "V");	 
	return rv;
}
int checkGWARP(u_int32_t ip)
{
	FILE *fp;
	char  buf[256];
	char ip_str[20];
	u_int32_t ip_tmp;
	char mac_str[32];
	char dev_str[32];
	const char incompleteMac[32]="00:00:00:00:00:00";
	int found = 0;

	//printf("search for ip=0x%x\n",ip);
	fflush(stdout);
	fp = fopen("/proc/net/arp", "r");
	if (fp == NULL)
		printf("read arp file fail!\n");
	else {
		fgets(buf, 256, fp);//first line!?
		while(fgets(buf, 256, fp) != NULL) {
			//printf("buf=%s\n",buf);
			sscanf(buf,"%s	%*s %*s %s %*s %s ", ip_str, mac_str,dev_str);
			//printf("ip %s found\n", ip_str);
			inet_aton(ip_str, (struct in_addr *)&ip_tmp);
			//printf("ip 0x%x found\n", ip_tmp);
			if (ip == ip_tmp && strcmp(mac_str, incompleteMac)){
				//printf("found!!!");
				found = 1;
				break;
			}
		}
		fclose(fp);
	}
	return found;
}

#ifdef CONFIG_USER_DHCPCLIENT_CHECK_IP_CONFLICT
int do_one_arp_probe(u_int32_t yiaddr, unsigned char *mac, char *interface)
{
	int optval = 1;
	int	s;			    /* socket */
	int	rv = 1;			/* return value */
	struct sockaddr addr;		/* for interface name */
	struct arpMsg	arp;
	fd_set		fdset;
	struct timeval	tm;
	struct timespec orgtime, aftertime;
	u_int32_t u32_srcIP, u32_tarIP;
	const time_t MICROSEC = 1000000;
	time_t elapsed=0, remaining=0, timeout=0; /* unit: microsecond */
	time_t rand_min = MICROSEC*ARP_PROBE_MIN, rand_max = MICROSEC*ARP_PROBE_MAX; /* unit: microsecond */

	if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
		LOG(LOG_ERR, "Could not open raw socket");
		return -1;
	}
	
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
		LOG(LOG_ERR, "Could not setsocketopt on raw socket");
		close(s);
		return -1;
	}

	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6);	/* MAC DA */
	memcpy(arp.ethhdr.h_source, mac, 6);		    /* MAC SA */
	arp.ethhdr.h_proto = htons(ETH_P_ARP);		    /* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER);		        /* hardware type */
	arp.ptype = htons(ETH_P_IP);			        /* protocol type (ARP message) */
	arp.hlen = 6;					                /* hardware address length */
	arp.plen = 4;					                /* protocol address length */
	arp.operation = htons(ARPOP_REQUEST);		    /* ARP op code */
	memcpy(arp.sHaddr, mac, 6);			            /* sender hardware address */
	memcpy(arp.tInaddr, &yiaddr, sizeof(yiaddr));	/* target IP address */
	
	memset(&addr, 0, sizeof(addr));
	strcpy(addr.sa_data, interface);
	if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
		rv = 0;

	/* random interval in range [rand_min, rand_max] */
	srandom(time(NULL));
	timeout = random()%(rand_max+1-rand_min) + rand_min;
        
	/* wait arp reply, and check it */
	tm.tv_sec = timeout/MICROSEC;
	tm.tv_usec = timeout%MICROSEC;
	remaining = timeout - elapsed;
	clock_gettime(CLOCK_MONOTONIC, &orgtime);
	while (elapsed <= timeout) {
		FD_ZERO(&fdset);
		FD_SET(s, &fdset);
		tm.tv_sec = remaining/MICROSEC;
		tm.tv_usec = remaining%MICROSEC;
		if (select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) < 0) {
			DEBUG(LOG_ERR, "Error on ARPING request: %s", strerror(errno));
			if (errno != EINTR) rv = 0;
		} else if (FD_ISSET(s, &fdset)) {
			if (recv(s, &arp, sizeof(arp), 0) < 0 ) rv = 0;
			memcpy(&u32_srcIP, arp.sInaddr, sizeof(u32_srcIP));
			memcpy(&u32_tarIP, arp.tInaddr, sizeof(u32_tarIP));
			
			if (arp.operation == htons(ARPOP_REPLY) && 
			    bcmp(arp.tHaddr, mac, 6) == 0 && 
			    u32_srcIP == yiaddr) {
				printf("\nconflict address mac=%x-%x-%x-%x-%x-%x\n",arp.sHaddr[0],arp.sHaddr[1],arp.sHaddr[2],arp.sHaddr[3],arp.sHaddr[4],arp.sHaddr[5]);
				DEBUG(LOG_INFO, "Valid arp reply receved for this address");
				rv = 0;
				break;
			}else if(arp.operation == htons(ARPOP_REQUEST) && 
			    bcmp(arp.sHaddr, mac, 6) != 0 && 
			    u32_tarIP == yiaddr &&
			    u32_srcIP == 0) {
				//ARP Probe from someone else with same targetIP
				printf("\nconflict arp-probe mac=%x-%x-%x-%x-%x-%x\n",arp.sHaddr[0],arp.sHaddr[1],arp.sHaddr[2],arp.sHaddr[3],arp.sHaddr[4],arp.sHaddr[5]);
				DEBUG(LOG_INFO, "Another arp probe with same target IP address receved");
				rv = 0;
				break;
			}
		}
		clock_gettime(CLOCK_MONOTONIC, &aftertime);
		elapsed = (aftertime.tv_sec-orgtime.tv_sec)*MICROSEC + (aftertime.tv_nsec-orgtime.tv_nsec)/1000;
		remaining = timeout - elapsed;
	}
	close(s);
	DEBUG(LOG_INFO, "%salid arp replies for this address", rv ? "No v" : "V");	 
	return rv;
}

int do_one_arp_announce(u_int32_t iip, unsigned char *mac, char *interface)
{
    int optval = 1;
    int s;          /* socket */
    int rv = 0;         /* return value */
    struct sockaddr addr;       /* for interface name */
    struct arpMsg   arp;

    if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
        LOG(LOG_ERR, "Could not open raw socket");
        return -1; 
    }   

    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
        LOG(LOG_ERR, "Could not setsocketopt on raw socket");
        close(s);
        return -1; 
    }   

    /* send arp request */
    memset(&arp, 0, sizeof(arp));
    memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6);   /* MAC DA */
    memcpy(arp.ethhdr.h_source, mac, 6);        /* MAC SA */
    arp.ethhdr.h_proto = htons(ETH_P_ARP);      /* protocol type (Ethernet) */
    arp.htype = htons(ARPHRD_ETHER);            /* hardware type */
    arp.ptype = htons(ETH_P_IP);                /* protocol type (ARP message) */
    arp.hlen = 6;                               /* hardware address length */
    arp.plen = 4;                               /* protocol address length */
    arp.operation = htons(ARPOP_REQUEST);       /* ARP op code */
    memcpy(arp.sInaddr, &iip, sizeof(iip));     /* source IP address */
    memcpy(arp.sHaddr, mac, 6);                 /* source hardware address */
    memcpy(arp.tInaddr, &iip, sizeof(iip));     /* target IP address */
    
    memset(&addr, 0, sizeof(addr));
    strcpy(addr.sa_data, interface);//interface);
    if (sendto(s, &arp, sizeof(arp)-sizeof(arp.pad), 0, &addr, sizeof(addr)) < 0)
        rv = -1; 

    close(s);
    return rv; 
}
#endif