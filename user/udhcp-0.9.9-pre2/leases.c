/*
 * leases.c -- tools to manage DHCP leases
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 */

#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "debug.h"
#include "dhcpd.h"
#include "files.h"
#include "options.h"
#include "leases.h"
#include "arpping.h"

// Mason Yu
#include <errno.h>
#include "serverpacket.h"
#include <rtk/sysconfig.h>
#include <rtk/utility.h>
#include <unistd.h>
#include <linux/unistd.h>       /* for _syscallX macros/related stuff */
#include <linux/kernel.h>       /* for struct sysinfo */
#include <sys/sysinfo.h>
#ifdef DHCPD_MULTI_THREAD_SUPPORT
#include <sys/prctl.h>
#include <pthread.h>
#include <assert.h>
#endif

unsigned char blank_chaddr[] = {[0 ... 15] = 0};

#ifdef DHCPD_MULTI_THREAD_SUPPORT
LIST_HEAD(arppinglist);
int arppingSockId = -1;
int forceKeepAliveChk = 0;
#endif

struct dhcpOfferedAddr *find_lease_by_chaddr_and_yiaddr(u_int8_t *chaddr, u_int32_t yiaddr);
long get_uptime()
{
    struct sysinfo s_info;
    int error = sysinfo(&s_info);
    if(error != 0)
    {
        printf("code error = %d\n", error);
    }
    return s_info.uptime;
}

void lease_timer()
{
    int iUpdateForcePortal = 0;
    unsigned long i;

	for (i = 0; i < server_config.max_leases; i++)
    {
		if ((leases[i].yiaddr != 0)
            && (lease_expired(&(leases[i]))))
        {
        	// Kaohj
#ifdef _CWMP_TR111_
		// Remove option 125 device when lease expired
		if (del_deviceId(leases[i].yiaddr))
			dump_deviceId();
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_NAT44
    {
        unsigned char functype=0;
        mib_get_s(AWIFI_PROVINCE_CODE,&functype, sizeof(functype));
        if(functype == AWIFI_ZJ){
            if(IsAddrInDhcpPool(leases[i].yiaddr))
            {
                if(leases[i].stClientInfo.ulDevice)
                    delNAT44Rules(leases[i].yiaddr);
            }
        }
    }
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_TCPOPT
    {
        unsigned char functype=0;
        mib_get_s(AWIFI_PROVINCE_CODE,&functype, sizeof(functype));
        if(functype == AWIFI_ZJ){
            if(leases[i].stClientInfo.ulDevice)
                delTCPOPTRules(leases[i].yiaddr, leases[i].chaddr);
        }
    }
#endif

            if (NULL != leases[i].stClientInfo.pfLeaseExpired)
            {
                leases[i].stClientInfo.pfLeaseExpired(&leases[i]);
            }

            if (FP_InvalidDevice != leases[i].stClientInfo.ulDevice)
            {
                iUpdateForcePortal = 1;
            }

            //LOG(LOG_INFO, "lease_timer of %s",  inet_ntoa(*((struct in_addr *)&(leases[i].yiaddr))));
            memset(&leases[i].stClientInfo, 0, sizeof(struct dhcpClientInfo));
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ)
#ifdef DHCPD_MULTI_THREAD_SUPPORT
			host_mutex_lock();
#endif
            delete_hosts( (unsigned char *)inet_ntoa(*((struct in_addr *)&(leases[i].yiaddr))));
#ifdef DHCPD_MULTI_THREAD_SUPPORT
			host_mutex_unlock();
#endif
#endif
		}
	}

    if (iUpdateForcePortal)
    {
        /* update_force_portal(); */
    }

    return;
}

void clear_all_lease()
{
    unsigned long i;

	for (i = 0; i < server_config.max_leases; i++)
    {
		if (leases[i].yiaddr != 0)
        {
#ifdef CONFIG_CT_AWIFI_JITUAN_NAT44
			if(IsAddrInDhcpPool(leases[i].yiaddr))
				delNAT44Rules(leases[i].yiaddr);
#endif
						
#ifdef CONFIG_CT_AWIFI_JITUAN_TCPOPT
			delTCPOPTRules(leases[i].yiaddr,leases[i].chaddr);
#endif

            if (NULL != leases[i].stClientInfo.pfLeaseExpired)
            {
                leases[i].stClientInfo.pfLeaseExpired(&leases[i]);
            }
		}
	}

    return;
}

void clear_one_lease(struct dhcpOfferedAddr *pstLease)
{
    int iUpdateForcePortal = 0;

    if (NULL != pstLease->stClientInfo.pfLeaseExpired)
    {
        pstLease->stClientInfo.pfLeaseExpired(pstLease);
    }

    if (FP_InvalidDevice != pstLease->stClientInfo.ulDevice)
    {
        iUpdateForcePortal = 1;
    }

    memset(pstLease, 0, sizeof(struct dhcpOfferedAddr));

    if (iUpdateForcePortal)
    {
        /* update_force_portal(); */
    }

    return;
}

/* clear every lease out that chaddr OR yiaddr matches and is nonzero */
void clear_lease(u_int8_t *chaddr, u_int32_t yiaddr)
{
	unsigned int i, j;

	for (j = 0; j < 16 && !chaddr[j]; j++);

	for (i = 0; i < server_config.max_leases; i++)
		if ((j != 16 && !memcmp(leases[i].chaddr, chaddr, 16)) ||
		    (yiaddr && leases[i].yiaddr == yiaddr)) {
		    
#ifdef CONFIG_CT_AWIFI_JITUAN_NAT44
    {
        unsigned char functype=0;
        mib_get_s(AWIFI_PROVINCE_CODE,&functype, sizeof(functype));
        if(functype == AWIFI_ZJ){
			if(IsAddrInDhcpPool(leases[i].yiaddr))
				delNAT44Rules(leases[i].yiaddr);
        }
    }
#endif
			
#ifdef CONFIG_CT_AWIFI_JITUAN_TCPOPT
    {
        unsigned char functype=0;
        mib_get_s(AWIFI_PROVINCE_CODE,&functype, sizeof(functype));
        if(functype == AWIFI_ZJ){
			delTCPOPTRules(leases[i].yiaddr,leases[i].chaddr);
        }
    }
#endif
			clear_one_lease(&(leases[i]));
		}
}


/* add a lease into the table, clearing out any old ones */
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, unsigned long lease)
{
	struct dhcpOfferedAddr *oldest;

#ifdef DHCPD_MULTI_THREAD_SUPPORT
	/* when multi-thread enabled, discover and request may arrive out of order, then lease will be modified unexpectedly */
	oldest = find_lease_by_chaddr_and_yiaddr(chaddr, yiaddr);
	if (oldest)
	{
		if ( lease == 0xffffffff )
			oldest->expires = 0xffffffff;
		else if ((get_uptime()+lease) > oldest->expires)
			oldest->expires = get_uptime() + lease;
		
		return oldest;
	}
#endif

	/* clean out any old ones */
	clear_lease(chaddr, yiaddr);

	oldest = oldest_expired_lease();

	if (oldest) {
        clear_one_lease(oldest);

		if (chaddr != NULL && sizeof(chaddr) <= 16) {
			memcpy(oldest->chaddr, chaddr, 16);
		} else {
			printf("Invalid chaddr size\n");
			return NULL;
		}

		oldest->yiaddr = yiaddr;
		// Mason Yu
		if ( lease == 0xffffffff ){
			//printf("DHCPD Lease time is overflow!!\n");
			oldest->expires = 0xffffffff;
		}else
			oldest->expires = get_uptime() + lease;
	}

	return oldest;
}


/* true if a lease has expired */
int lease_expired(struct dhcpOfferedAddr *lease)
{
	return (lease->expires < (unsigned long) get_uptime());
}


/* Find the oldest expired lease, NULL if there are no expired leases */
struct dhcpOfferedAddr *oldest_expired_lease(void)
{
	struct dhcpOfferedAddr *oldest = NULL;
	unsigned long oldest_lease = get_uptime();
	unsigned int i;


	for (i = 0; i < server_config.max_leases; i++)
		if (oldest_lease > leases[i].expires) {
			oldest_lease = leases[i].expires;
			oldest = &(leases[i]);
		}
	return oldest;

}


/* Find the first lease that matches chaddr, NULL if no match */
struct dhcpOfferedAddr *find_lease_by_chaddr(u_int8_t *chaddr)
{
	unsigned int i;

	for (i = 0; i < server_config.max_leases; i++)
		if (!memcmp(leases[i].chaddr, chaddr, 16)) return &(leases[i]);

	return NULL;
}


/* Find the first lease that matches yiaddr, NULL is no match */
struct dhcpOfferedAddr *find_lease_by_yiaddr(u_int32_t yiaddr)
{
	unsigned int i;

	for (i = 0; i < server_config.max_leases; i++)
		if (leases[i].yiaddr == yiaddr) return &(leases[i]);

	return NULL;
}

// Mason Yu
/* Find the first lease that matches chaddr and yiaddr, NULL if no match */
struct dhcpOfferedAddr *find_lease_by_chaddr_and_yiaddr(u_int8_t *chaddr, u_int32_t yiaddr)
{
	unsigned int i;

	for (i = 0; i < server_config.max_leases; i++) {
		if (!memcmp(leases[i].chaddr, chaddr, 6) && leases[i].yiaddr == yiaddr) {
			return &(leases[i]);
		}
	}

	return NULL;
}

// Mason Yu
int arpping_for_Server(u_int32_t yiaddr, u_int32_t ip, unsigned char *mac, char *interface, u_int8_t *chaddr)
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
#if defined (CONFIG_RTL9607C_SERIES) && defined (CONFIG_KERNEL_5_10_x)
	__time64_t	prevTime;
#else
	time_t	prevTime;
#endif
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
	if (strlen(interface) < sizeof(addr.sa_data))
		strcpy(addr.sa_data, interface);
	else
		strncpy(addr.sa_data, interface, sizeof(addr.sa_data));
	if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
		rv = 0;

	/* wait arp reply, and check it */
	tm.tv_usec = 0;
	prevTime = get_uptime();
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
			    // Mason Yu
			    //*((u_int *) arp.sInaddr) == yiaddr) {
			    u32_srcIP == yiaddr && find_lease_by_chaddr_and_yiaddr(arp.sHaddr, yiaddr)==NULL) {
			    if (bcmp(arp.sHaddr, chaddr, 6))
		    	{
				    printf("\nconflict address mac=%x-%x-%x-%x-%x-%x\n",arp.sHaddr[0],arp.sHaddr[1],arp.sHaddr[2],arp.sHaddr[3],arp.sHaddr[4],arp.sHaddr[5]);
					DEBUG(LOG_INFO, "Valid arp reply receved for this address");
					rv = 0;
					break;
		    	}
			}
		}
		timeout -= (get_uptime() - prevTime);
		prevTime = get_uptime();
	}
	close(s);
	DEBUG(LOG_INFO, "%salid arp replies for this address", rv ? "No v" : "V");
	return rv;
}

#ifdef DHCPD_MULTI_THREAD_SUPPORT
static struct ArpPingInfo * add_arpping_host(u_int32_t yiaddr, u_int8_t *chaddr , unsigned char *dmac, u_int32_t ip, unsigned char *smac, char *interface, int checkTimes)
{
	struct ArpPingInfo *pEntry;

	pEntry = (struct ArpPingInfo *)malloc(sizeof(struct ArpPingInfo));
	if (NULL == pEntry)
	{
		fprintf(stderr, "%s malloc fail!\n", __func__);
		return NULL;
	}

	memset(pEntry, 0, sizeof(struct ArpPingInfo));
	pEntry->yiaddr = yiaddr;
	pEntry->ip = ip;
	pEntry->checkTimes = checkTimes;
	memcpy(pEntry->lmac, smac, 6);
	memcpy(pEntry->dmac, dmac, 6);
	if (chaddr != NULL)
		memcpy(pEntry->chaddr, chaddr, 6);
	else
		bzero(pEntry->chaddr, 6);
	snprintf(pEntry->ifname, sizeof(pEntry->ifname), "%s", interface);
	INIT_LIST_HEAD(&pEntry->list);

	arpping_mutex_lock();
	list_add_tail(&pEntry->list, &arppinglist);
	arpping_mutex_unlock();

	return pEntry;
}

static int get_arpping_result(struct ArpPingInfo *hostEntry)
{
	int ret = 0;

	usleep(20000);
	while (1)
	{
		if (MODE_ARP_REPLY_RCVD == hostEntry->status)
		{
			DEBUG(LOG_DEBUG, "Valid arp reply receved for this address\n");
			ret = 0;
			break;
		}
		else if (MODE_ARP_REPLY_TIMEOUT == hostEntry->status)
		{
			ret = 1;
			break;
		}
		else {
			if (MODE_ARP_IDLE == hostEntry->status)
			{
				DEBUG(LOG_DEBUG, "host %02x:%02x:%02x:%02x:%02x:%02x is not checked by dhcpd thread\n", hostEntry->lmac[0], hostEntry->lmac[1], 
					hostEntry->lmac[2], hostEntry->lmac[3], hostEntry->lmac[4], hostEntry->lmac[5]);
				hostEntry->lastReqTime = get_uptime();
			}

			if ((get_uptime() - hostEntry->lastReqTime) >= (hostEntry->checkTimes*ARP_REQUEST_INTERVAL))
			{
				ret = 1;
				break;
			}
			
			usleep(10000);
		}
	}

	DEBUG(LOG_DEBUG, "%salid arp replies for address %s\n", ret ? "No v" : "V", inet_ntoa(*((struct in_addr *)&hostEntry->yiaddr)));

	arpping_mutex_lock();
	list_del(&hostEntry->list);
	arpping_mutex_unlock();
	free(hostEntry);

	return ret;
}

static int init_arpping_sock(void)
{
	int optval = 1;

	if ((arppingSockId = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
		fprintf(stderr, "%s Could not open raw socket\n", __func__);
		return -1;
	}

	if (fcntl(arppingSockId, F_SETFL, O_NONBLOCK) == -1) {
		fprintf(stderr, "%s Could not set noblock\n", __func__);
		goto err_out;
	}

	if (setsockopt(arppingSockId, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
		fprintf(stderr, "%s Could not setsocketopt on raw socket\n", __func__);
		goto err_out;
	}
	return 0;
	
err_out:
	close(arppingSockId);
	arppingSockId = -1;
	return -1;
}

static int send_arp_request(u_int32_t yiaddr, unsigned char *dmac, u_int32_t ip, unsigned char *smac, char *interface)
{
	struct sockaddr addr;		/* for interface name */
	struct arpMsg	arp;

	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memcpy(arp.ethhdr.h_dest, dmac, 6);	/* MAC DA */
	memcpy(arp.ethhdr.h_source, smac, 6);		/* MAC SA */
	arp.ethhdr.h_proto = htons(ETH_P_ARP);		/* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER);		/* hardware type */
	arp.ptype = htons(ETH_P_IP);			/* protocol type (ARP message) */
	arp.hlen = 6;					/* hardware address length */
	arp.plen = 4;					/* protocol address length */
	arp.operation = htons(ARPOP_REQUEST);		/* ARP op code */
	memcpy(arp.sInaddr, &ip, sizeof(ip));		/* source IP address */
	memcpy(arp.sHaddr, smac, 6);			/* source hardware address */
	memcpy(arp.tInaddr, &yiaddr, sizeof(yiaddr));	/* target IP address */

	memset(&addr, 0, sizeof(addr));
	snprintf(addr.sa_data, sizeof(addr.sa_data), "%s", interface);
	DEBUG(LOG_DEBUG, "send arp request for %s\n", inet_ntoa(*((struct in_addr *)&yiaddr)));
	if (sendto(arppingSockId, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
		return -1;

	return 0;
}

static void arpping_send(void)
{
	struct ArpPingInfo *pEntry, *pNextEntry;

	if (list_empty(&arppinglist))
		return;

	list_for_each_entry_safe(pEntry, pNextEntry, &arppinglist, list)
	{
		if (MODE_ARP_IDLE == pEntry->status)
		{
			send_arp_request(pEntry->yiaddr, pEntry->dmac, pEntry->ip, pEntry->lmac, pEntry->ifname);
			pEntry->status = MODE_ARP_REQ_SENT;
			pEntry->lastReqTime = get_uptime();
			pEntry->arpCnt++;
		}
		else if (MODE_ARP_REQ_SENT == pEntry->status)
		{
			if ((get_uptime() - pEntry->lastReqTime) >= ARP_REQUEST_INTERVAL)
			{
				if (pEntry->arpCnt >= pEntry->checkTimes)
					pEntry->status = MODE_ARP_REPLY_TIMEOUT;
				else
				{
					send_arp_request(pEntry->yiaddr, pEntry->dmac, pEntry->ip, pEntry->lmac, pEntry->ifname);
					pEntry->lastReqTime = get_uptime();
					pEntry->arpCnt++;
				}
			}
		}
	}
}

int is_valid_address(const char *ip_addr) {
    struct in_addr ipv4_addr;
    struct in6_addr ipv6_addr;

    if (inet_pton(AF_INET, ip_addr, &ipv4_addr) == 1) {
        return 1;
    }

    if (inet_pton(AF_INET6, ip_addr, &ipv6_addr) == 1) {
        return 1;
    }

    return 0;
}

static int arpping_recv(void)
{
	struct ArpPingInfo *pEntry, *pNextEntry;
	struct arpMsg	arp;
	u_int32_t u32_srcIP;
	ssize_t bytes_received;
	
	bytes_received = recv(arppingSockId, &arp, sizeof(arp), 0);
	if (bytes_received < 0 )
		return -1;
	if (bytes_received < sizeof(arp)) {
		LOG(LOG_ERR, "Received incomplete ARP message");
		return -1;
	}
	if (is_valid_address(arp.sInaddr)) {
		if (bytes_received >= sizeof(arp.sInaddr)) {
			memcpy(&u32_srcIP, arp.sInaddr, sizeof(u32_srcIP));
		}
		else
		{
			LOG(LOG_ERR, "Invalid ARP address size");
			return -1;
		}
	}
	else
	{
		LOG(LOG_ERR, "Received invalid ARP address");
		return -1;
	}
	arpping_mutex_lock();
	list_for_each_entry_safe(pEntry, pNextEntry, &arppinglist, list)
	{
		if (arp.operation == htons(ARPOP_REPLY) &&
			bcmp(arp.tHaddr, pEntry->lmac, 6) == 0 &&
			u32_srcIP == pEntry->yiaddr)
		{
			if (find_lease_by_chaddr_and_yiaddr(arp.sHaddr, pEntry->yiaddr)==NULL)
			{
				if ((0 != (pEntry->chaddr[0] | pEntry->chaddr[1] | pEntry->chaddr[2] | pEntry->chaddr[3] | pEntry->chaddr[4] | pEntry->chaddr[5])) &&
					!bcmp(arp.sHaddr, pEntry->chaddr, 6))
				{
					/* arp received from dhcp IP requester, ignore it */
					continue;
				}
					
				printf("\nconflict address mac=%x-%x-%x-%x-%x-%x\n",arp.sHaddr[0],arp.sHaddr[1],arp.sHaddr[2],arp.sHaddr[3],arp.sHaddr[4],arp.sHaddr[5]);
				DEBUG(LOG_DEBUG, "Valid arp reply received for this address");
				pEntry->status = MODE_ARP_REPLY_RCVD;
			}
			else if (bcmp(arp.sHaddr, pEntry->dmac, 6) == 0)
			{
				/* reply for unicast arp request, check if client is alive */
				DEBUG(LOG_DEBUG, "Valid arp reply received for this address");
				pEntry->status = MODE_ARP_REPLY_RCVD;
			}
		}
	}
	arpping_mutex_unlock();
	
	return 0;
}

void * arpping_thread(void *data)
{
	struct timeval tm;
	fd_set fdset;

	assert(data==NULL);
	prctl(PR_SET_NAME, "arpping");
	
	init_arpping_sock();
	
	while (1)
	{
		arpping_mutex_lock();
		arpping_send();
		arpping_mutex_unlock();

		FD_ZERO(&fdset);
		FD_SET(arppingSockId, &fdset);
		tm.tv_sec = 0;
		tm.tv_usec = 10000;
		if (select(arppingSockId + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) < 0)
		{
			/* do nothing */
		}
		else if (FD_ISSET(arppingSockId, &fdset))
		{
			while (1)
			{
				if (arpping_recv() < 0)
					break;
			}
		}
	}

	// We can use pthread_exit() or return to terminate this thread.
	//pthread_exit(NULL);
}

void * keepalive_thread(void *data)
{
	struct ArpPingInfo *arpEntry;
	unsigned long startTime, currTime;
    unsigned long i;
	int ret = -1;

	startTime = get_uptime();
	while (1)
	{
		currTime = get_uptime();
		if (((currTime-startTime) >= KEEPALIVE_TIME) || forceKeepAliveChk)
		{
			startTime = currTime;
			forceKeepAliveChk = 0;
			
			for (i = 0; i < server_config.max_leases; i++)
			{
				if ((leases[i].yiaddr != 0) && !lease_expired(&leases[i]))
				{
					arpEntry = add_arpping_host(leases[i].yiaddr, NULL, leases[i].chaddr, server_config.server, server_config.arp, server_config.interface, ARP_REQUEST_MAX_TIMES);
					if (arpEntry != NULL)
						ret = get_arpping_result(arpEntry);
					
					if (ret != 0)
					{
						DEBUG(LOG_DEBUG, "%s no reply from %s, force lease timeout\n",
								__func__, inet_ntoa(*((struct in_addr *)&leases[i].yiaddr)));
						//client is off line, this host should be deleted!
						DEBUG(LOG_INFO, "[%s] lease %x-%x-%x-%x-%x-%x is timeout", __func__, 
								leases[i].chaddr[0], leases[i].chaddr[1], leases[i].chaddr[2],
								leases[i].chaddr[3], leases[i].chaddr[4], leases[i].chaddr[5]);
						leases[i].expires = currTime-1;
					}
				}
			}
		}

		sleep(10);
	}
	
	//pthread_exit(NULL);
}
#endif

/* find an assignable address, it check_expired is true, we check all the expired leases as well.
 * Maybe this should try expired leases by age... */
#ifdef IP_BASED_CLIENT_TYPE
//u_int32_t find_address(int check_expired, enum DeviceType devicetype )
u_int32_t find_address(int check_expired, struct client_category_t *deviceCategory, u_int8_t *chaddr, struct server_config_t *server_config_p )
#else
u_int32_t find_address(int check_expired, u_int8_t *chaddr, struct server_config_t *server_config_p)
#endif
{
	u_int32_t addr, ret;
	u_int32_t addrstart, addrend, addrhash, random_one, hash;
	int i;
	struct dhcpOfferedAddr *lease = NULL;
	
	random_one = server_config.random_one;
	if(random_one) {
		for (hash = 0, i = 0; i < 6; i++)
			hash = chaddr[i] + (hash << 6) + (hash << 16) - hash;
	}
	else hash = 0;

#ifdef IP_BASED_CLIENT_TYPE
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	thread_mutex_lock();
#endif
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	if (server_config_p != p_serverpool_config) {
			addr = addrstart= ntohl(server_config_p->start);
			addrend = ntohl(server_config_p->end);
	} else
#endif
	{
		if (deviceCategory == NULL) {
			addr = addrstart = ntohl(server_config_p->start);
			addrend = ntohl(server_config_p->end);
		} else {
			addr = addrstart = ntohl(deviceCategory->ipstart);
			addrend = ntohl(deviceCategory->ipend);
		}
	}
	
	if(random_one) {
		addr = addrhash = addrstart + hash % (1 + addrend - addrstart);
		if(addr == addrstart)
			random_one = 0;
	}

rechk:
	for (;addr <= addrend; addr++) {
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
		if (server_config_p == p_serverpool_config)
#endif
		{
			if (deviceCategory == NULL) {
				struct client_category_t *pDhcp;
				for (pDhcp=server_config_p->clientRange; pDhcp; pDhcp=pDhcp->next) {
					if ((addr >= ntohl(pDhcp->ipstart)) && (addr <= ntohl(pDhcp->ipend)))
						break;
				}
				if (!pDhcp)
					continue;
			}
		}

		/* ie, 192.168.55.0 */
		if (!(addr & 0xFF)) continue;

		/* ie, 192.168.55.255 */
		if ((addr & 0xFF) == 0xFF) continue;
//jim added by star zhang
		if(find_Mac_by_IP(htonl(addr))) continue;// star add: for static ip based Mac
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
		if( isReservedIPAddress(htonl(addr), server_config_p) ) continue;
#endif //SUPPORT_DHCP_RESERVED_IPADDR

		/* lease is not taken */
		ret = htonl(addr);
#ifdef DHCPD_MULTI_THREAD_SUPPORT
		if (!(lease = find_lease_by_yiaddr(ret)))
		{
			/* when multi-client request addresses at the same time, we should add a forged lease entry to forbid more than 2 clients get the same address */
			add_lease(blank_chaddr, ret, server_config_p->offer_time-1);
			thread_mutex_unlock();

			if (!check_ip(ret, chaddr, server_config_p))
				return ret;
			
			thread_mutex_lock();
		}
		else if (check_expired  && lease_expired(lease))
		{
			add_lease(blank_chaddr, ret, server_config_p->offer_time-1);
			thread_mutex_unlock();

			if (!check_ip(ret, chaddr, server_config_p))
				return ret;

			thread_mutex_lock();
		}
#else
		if ((!(lease = find_lease_by_yiaddr(ret)) ||
			/* or it expired and we are checking for expired leases */
			(check_expired  && lease_expired(lease))) &&
			/* and it isn't on the network */
			!check_ip(ret, chaddr, server_config_p)) {
				return ret;
				break;
		}
#endif
	}

	if(random_one && addr > addrend) {
		addr = addrstart;
		addrend = addrhash - 1;
		random_one = 0;
		goto rechk;
	}

#ifdef DHCPD_MULTI_THREAD_SUPPORT
	thread_mutex_unlock();
#endif

#else  // IP_BASED_CLIENT_TYPE
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	thread_mutex_lock();
#endif
	addr = addrstart = ntohl(server_config_p->start); /* addr is in host order here */
	addrend = ntohl(server_config_p->end);

	if(random_one) {
		addr = addrhash = addrstart + hash % (1 + addrend - addrstart);
		if(addr == addrstart)
			random_one = 0;
	}

rechk:
	for (;addr <= addrend; addr++) {

		/* ie, 192.168.55.0 */
		if (!(addr & 0xFF)) continue;

		/* ie, 192.168.55.255 */
		if ((addr & 0xFF) == 0xFF) continue;
//jim added by star zhang
		if(find_Mac_by_IP(htonl(addr))) continue;// star add: for static ip based Mac
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
		if( isReservedIPAddress(htonl(addr), server_config_p) ) continue;
#endif //SUPPORT_DHCP_RESERVED_IPADDR

		/* lease is not taken */
		ret = htonl(addr);

#ifdef DHCPD_MULTI_THREAD_SUPPORT
		if (!(lease = find_lease_by_yiaddr(ret)))
		{
			/* when multi-client request addresses at the same time, we should add a forged lease entry to forbid more than 2 clients get the same address */
			add_lease(blank_chaddr, ret, server_config_p->offer_time-1);
			thread_mutex_unlock();

			if (!check_ip(ret, chaddr, server_config_p))
				return ret;
			
			thread_mutex_lock();
		}
		else if (check_expired  && lease_expired(lease))
		{
			add_lease(blank_chaddr, ret, server_config_p->offer_time-1);
			thread_mutex_unlock();

			if (!check_ip(ret, chaddr, server_config_p))
				return ret;

			thread_mutex_lock();
		}
		
#else
		if ((!(lease = find_lease_by_yiaddr(ret)) ||

		     /* or it expired and we are checking for expired leases */
		     (check_expired  && lease_expired(lease))) &&

		     /* and it isn't on the network */
	    	     !check_ip(ret, chaddr, server_config_p)) {
			return ret;
			break;
		}
#endif
	}
	
	if(random_one && addr > addrend) {
		addr = addrstart;
		addrend = addrhash - 1;
		random_one = 0;
		goto rechk;
	}
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	thread_mutex_unlock();
#endif
#endif
	return 0;
}


/* check is an IP is taken, if it is, add it to the lease table */
int check_ip(u_int32_t addr, u_int8_t *chaddr, void *config)
{
#if 0//!defined(CONFIG_RTK_DEV_AP) && !defined(CONFIG_CMCC_FTTR)
	// Remove ARP req to speedup dhcp flow
	// Because it may cause timeout in tests with many dhcp clients.
	// Client should do it itself.
	//LOG(LOG_DEBUG, "skip check arp for 0x%x",addr);
	(void)(addr);(void)(chaddr);
	return 0;
#else
#ifndef DHCPD_MULTI_THREAD_SUPPORT
	struct in_addr temp;

	// Mason Yu
	//if (arpping(addr, server_config.server, server_config.arp, server_config.interface) == 0) {
	if (arpping_for_Server(addr, server_config.server, server_config.arp, server_config.interface, chaddr) == 0) {
		temp.s_addr = addr;
	 	LOG(LOG_INFO, "%s belongs to someone, reserving it for %ld seconds",
	 		inet_ntoa(temp), server_config.conflict_time);
		add_lease(blank_chaddr, addr, server_config.conflict_time);
		return 1;
	} else return 0;
#else
	struct ArpPingInfo *arpEntry;
	struct dhcpOfferedAddr *lease;
	struct in_addr temp;
	int ret = 0;
	struct server_config_t *server_config_p = (struct server_config_t *)config;

	if(!server_config_p)
		return 0;

	DEBUG(LOG_DEBUG, "%s check %s\n", __func__, inet_ntoa(*((struct in_addr *)&addr)));
	arpEntry = add_arpping_host(addr, chaddr, MAC_BCAST_ADDR, server_config_p->server, server_config_p->arp, server_config_p->interface, ARP_REQUEST_MIN_TIMES);
	if (arpEntry != NULL)
		ret = get_arpping_result(arpEntry);
	
	if (0 == ret)
	{
		temp.s_addr = addr;
		LOG(LOG_INFO, "%s belongs to someone, reserving it for %ld seconds",
			inet_ntoa(temp), server_config_p->conflict_time);
		thread_mutex_lock();
		lease = find_lease_by_yiaddr(addr);
		/* only 2 case should we forbid a ip in address pool 
		 * 1. no lease exist and find conflict address, this case means a static ip client exist.
		 * 2. lease->expires is less than offer_time, which means this lease is not valid.
		 */
		if (!lease || ((lease->expires-get_uptime()) < server_config_p->offer_time))
		{
			DEBUG(LOG_DEBUG, "%s %d add lease %s expires %d\n", __func__, __LINE__, inet_ntoa(*((struct in_addr *)&addr)), (int)server_config_p->conflict_time);
			add_lease(blank_chaddr, addr, server_config_p->conflict_time);
		}
		thread_mutex_unlock();
		return 1;
	}
	else
		return 0;
#endif
#endif
}

//add by piyajee_chen 20171227 debug the leases info.
void dump_leases(void)
{

	unsigned int i;
	struct in_addr addr;

	printf("==========udhcpd dump all leases info==========\n");

	for (i = 0; i < server_config.max_leases; i++)
	{
		if (leases[i].yiaddr == 0)
			continue;
	
		printf("=======================%d=======================\n", i);
		printf("MAC:        %02X:%02X:%02X:%02X:%02X:%02X\n", leases[i].chaddr[0],leases[i].chaddr[1],leases[i].chaddr[2],
				leases[i].chaddr[3],leases[i].chaddr[4],leases[i].chaddr[5]);
		addr.s_addr = leases[i].yiaddr;
		printf("IP:         %s\n", inet_ntoa(addr));
		printf("HostName    %s\n", leases[i].hostName);
		printf("active time    %u\n", leases[i].active_time);
#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
		struct dhcp_ctc_client_info *pstClientInfo = NULL;
		pstClientInfo = (struct dhcp_ctc_client_info *)(leases[i].stClientInfo.pvDHCPClientData);
		if(NULL != pstClientInfo)
		{
			printf("Category    %d\n", pstClientInfo->category);
			printf("Vendor      %s\n", pstClientInfo->szVendor);
			printf("UserClassID %s\n", pstClientInfo->szUserClass);
			printf("ClientID    %s\n", pstClientInfo->szClientID);
			printf("FQDN        %s\n", pstClientInfo->szFQDN);	
#if defined(CONFIG_CU)
			printf("VendorClassID   %s\n", pstClientInfo->szVendorClassID);
#endif
		}
		else
			printf("\n");
#endif
	}

	return;
}


