/*
 * arpping .h
 */

#ifndef ARPPING_H
#define ARPPING_H

#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netinet/in.h>

#ifdef CONFIG_USER_DHCPCLIENT_CHECK_IP_CONFLICT
#define ARP_PROBE_NUM           3   /*number of probe packets*/
#define ARP_PROBE_MIN           1   /*minimum time between Probe packets */
#define ARP_PROBE_MAX           2   /*maximum time between Probe packets */
#define ARP_ANNOUNCEMENT_NUM    1   /*number of Announcement packets*/
#define ARP_ANNOUNCE_INTERVAL   2   /*time between Announcement packets*/
#endif

struct arpMsg {
	struct ethhdr ethhdr;	 		/* Ethernet header */
	u_short htype;				/* hardware type (must be ARPHRD_ETHER) */
	u_short ptype;				/* protocol type (must be ETH_P_IP) */
	u_char  hlen;				/* hardware address length (must be 6) */
	u_char  plen;				/* protocol address length (must be 4) */
	u_short operation;			/* ARP opcode */
	u_char  sHaddr[6];			/* sender's hardware address */
	u_char  sInaddr[4];			/* sender's IP address */
	u_char  tHaddr[6];			/* target's hardware address */
	u_char  tInaddr[4];			/* target's IP address */
	u_char  pad[18];			/* pad for min. Ethernet payload (60 bytes) */
};

/* function prototypes */
int arpping(u_int32_t yiaddr, u_int32_t ip, unsigned char *arp, char *interface);
int checkGWARP(u_int32_t ip);
#ifdef CONFIG_USER_DHCPCLIENT_CHECK_IP_CONFLICT
int do_one_arp_probe(u_int32_t yiaddr, unsigned char *mac, char *interface);
int do_one_arp_announce(u_int32_t iip, unsigned char *mac, char *interface);
#endif
#endif
