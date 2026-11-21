
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/capability.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/sockios.h>
#include <linux/icmp.h>
#include <linux/if.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/net.h>
#include <linux/in6.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/icmpv6.h>
#include <linux/init.h>
#include <linux/route.h>
#include <linux/rtnetlink.h>
#include <linux/netfilter_ipv6.h>
#include <linux/slab.h>
#include <linux/hash.h>
#include <linux/etherdevice.h>

#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/seg6_iptunnel.h>

#include <net/icmp.h>
#include <net/ip.h>
#include <net/ip_tunnels.h>
#include <net/ipv6.h>
#include <net/ip6_route.h>
#include <net/addrconf.h>
#include <net/ip6_tunnel.h>
#include <net/xfrm.h>
#include <net/dsfield.h>
#include <net/inet_ecn.h>
#include <net/net_namespace.h>
#include <net/netns/generic.h>
#include <net/dst_metadata.h>
#include "rtk_ip6_tunnel_tap.h"

#include <linux/proc_fs.h>

/* Ethernet over IPv6 (mode=ethip6)*/

MODULE_AUTHOR("Realtek");
MODULE_DESCRIPTION("Realtek IPv6 tunneling tap device");
MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK("ip6tnltap");
MODULE_ALIAS_NETDEV("ip6tnltap0");

#define IP6_TUNNEL_HASH_SIZE_SHIFT  5
#define IP6_TUNNEL_HASH_SIZE (1 << IP6_TUNNEL_HASH_SIZE_SHIFT)

static bool log_ecn_error = true;
module_param(log_ecn_error, bool, 0644);
MODULE_PARM_DESC(log_ecn_error, "Log packets received with corrupted ECN");

/* Debug log definition*/
extern struct proc_dir_entry *realtek_proc;
static int ip6tnltap_debug;
#define DBG_PRINT(mode, fmt, ...) ((mode & ip6tnltap_debug) ? printk(fmt, ##__VA_ARGS__) : 0)
enum tun_debug_e {
	DBG_NONE	= 0x0,
	DBG_INFO	= 0x01,
	DBG_RX		= 0x02,
	DBG_TX		= 0x04,
	DBG_ALERT	= 0x08,
	DBG_ERR		= 0x10,
	DBG_ALL		= 0xffffffff
};

char *dbg_name[] = { /* tun_debug_e */
	"INFO", "RX", "TX", "ALERT", "ERR"
};

static u32 HASH(const struct in6_addr *addr)
{
	u32 hash = ipv6_addr_hash(addr);

	return hash_32(hash, IP6_TUNNEL_HASH_SIZE_SHIFT);
}

static int ip6_tnl_dev_init(struct net_device *dev);
static void ip6_tnl_dev_setup(struct net_device *dev);
static struct rtnl_link_ops ip6_link_ops __read_mostly;

static unsigned int ip6_tnl_net_id __read_mostly;
struct ip6_tnl_net {
	/* the IPv6 tunnel fallback device */
	struct net_device *fb_tnl_dev;
	/* lists for storing tunnels in use */
	struct ip6_tnltap __rcu *tnls_r_l[IP6_TUNNEL_HASH_SIZE];
	struct ip6_tnltap __rcu *tnls_wc[1];
	struct ip6_tnltap __rcu **tnls[2];
	struct ip6_tnltap __rcu *collect_md_tun;
};

static struct net_device_stats *ip6_get_stats(struct net_device *dev)
{
	struct pcpu_sw_netstats tmp, sum = { 0 };
	int i;

	for_each_possible_cpu(i) {
		unsigned int start;
		const struct pcpu_sw_netstats *tstats =
						   per_cpu_ptr(dev->tstats, i);

		do {
			start = u64_stats_fetch_begin_irq(&tstats->syncp);
			tmp.rx_packets = tstats->rx_packets;
			tmp.rx_bytes = tstats->rx_bytes;
			tmp.tx_packets = tstats->tx_packets;
			tmp.tx_bytes =  tstats->tx_bytes;
		} while (u64_stats_fetch_retry_irq(&tstats->syncp, start));

		sum.rx_packets += tmp.rx_packets;
		sum.rx_bytes   += tmp.rx_bytes;
		sum.tx_packets += tmp.tx_packets;
		sum.tx_bytes   += tmp.tx_bytes;
	}
	dev->stats.rx_packets = sum.rx_packets;
	dev->stats.rx_bytes   = sum.rx_bytes;
	dev->stats.tx_packets = sum.tx_packets;
	dev->stats.tx_bytes   = sum.tx_bytes;
	return &dev->stats;
}

#define for_each_ip6_tunnel_rcu(start) \
	for (t = rcu_dereference(start); t; t = rcu_dereference(t->next))

/**
 * ip6_tnl_lookup - fetch tunnel matching the end-point addresses
 *   @net: network namespace
 *   @link: ifindex of underlying interface
 *   @remote: the address of the tunnel exit-point
 *   @local: the address of the tunnel entry-point
 *
 * Return:
 *   tunnel matching given end-points if found,
 *   else fallback tunnel if its device is up,
 *   else %NULL
 **/

static struct ip6_tnltap *
ip6_tnl_lookup(struct net *net, int link,
	       const struct in6_addr *remote, const struct in6_addr *local)
{
	unsigned int hash = HASH(local);
	struct ip6_tnltap *t, *cand = NULL;
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	struct in6_addr any;

	for_each_ip6_tunnel_rcu(ip6n->tnls_r_l[hash]) {
		if (!ipv6_addr_equal(local, &t->parms.laddr) ||
		    !(t->dev->flags & IFF_UP))
			continue;

		if (!ipv6_addr_equal(remote, &t->parms.raddr)) {
			struct __ip6_tnl_fmr *fmr;
			bool found = false;

			for (fmr = t->parms.fmrs; fmr; fmr = fmr->next) {
				if (!ipv6_prefix_equal(remote, &fmr->rule.ip6_prefix,
						       fmr->rule.ip6_prefix_len))
					continue;

				found = true;
				break;
			}

			if (!found)
				continue;
		}

		if (link == t->parms.link)
			return t;
		else
			cand = t;
	}

	memset(&any, 0, sizeof(any));
	hash = HASH(local);
	for_each_ip6_tunnel_rcu(ip6n->tnls_r_l[hash]) {
		if (!ipv6_addr_equal(local, &t->parms.laddr) ||
		    !ipv6_addr_any(&t->parms.raddr) ||
		    !(t->dev->flags & IFF_UP))
			continue;

		if (link == t->parms.link)
			return t;
		else if (!cand)
			cand = t;
	}

	hash = HASH(&any);
	for_each_ip6_tunnel_rcu(ip6n->tnls_r_l[hash]) {
		if (!ipv6_addr_equal(remote, &t->parms.raddr) ||
		    !ipv6_addr_any(&t->parms.laddr) ||
		    !(t->dev->flags & IFF_UP))
			continue;

		if (link == t->parms.link)
			return t;
		else if (!cand)
			cand = t;
	}

	if (cand)
		return cand;

	t = rcu_dereference(ip6n->collect_md_tun);
	if (t && t->dev->flags & IFF_UP)
		return t;

	t = rcu_dereference(ip6n->tnls_wc[0]);
	if (t && (t->dev->flags & IFF_UP))
		return t;

	return NULL;
}

/**
 * ip6_tnl_bucket - get head of list matching given tunnel parameters
 *   @p: parameters containing tunnel end-points
 *
 * Description:
 *   ip6_tnl_bucket() returns the head of the list matching the
 *   &struct in6_addr entries laddr and raddr in @p.
 *
 * Return: head of IPv6 tunnel list
 **/

static struct ip6_tnltap __rcu **
ip6_tnl_bucket(struct ip6_tnl_net *ip6n, const struct __ip6_tnltap_parm *p)
{
	const struct in6_addr *remote = &p->raddr;
	const struct in6_addr *local = &p->laddr;
	unsigned int h = 0;
	int prio = 0;

	if (!ipv6_addr_any(remote) || !ipv6_addr_any(local)) {
		prio = 1;
		h = HASH(local);
	}
	return &ip6n->tnls[prio][h];
}

/**
 * ip6_tnl_link - add tunnel to hash table
 *   @t: tunnel to be added
 **/

static void
ip6_tnl_link(struct ip6_tnl_net *ip6n, struct ip6_tnltap *t)
{
	struct ip6_tnltap __rcu **tp = ip6_tnl_bucket(ip6n, &t->parms);

	if (t->parms.collect_md)
		rcu_assign_pointer(ip6n->collect_md_tun, t);
	rcu_assign_pointer(t->next, rtnl_dereference(*tp));
	rcu_assign_pointer(*tp, t);
}

/**
 * ip6_tnl_unlink - remove tunnel from hash table
 *   @t: tunnel to be removed
 **/

static void
ip6_tnl_unlink(struct ip6_tnl_net *ip6n, struct ip6_tnltap *t)
{
	struct ip6_tnltap __rcu **tp;
	struct ip6_tnltap *iter;

	if (t->parms.collect_md)
		rcu_assign_pointer(ip6n->collect_md_tun, NULL);

	for (tp = ip6_tnl_bucket(ip6n, &t->parms);
	     (iter = rtnl_dereference(*tp)) != NULL;
	     tp = &iter->next) {
		if (t == iter) {
			rcu_assign_pointer(*tp, t->next);
			break;
		}
	}
}

static void ip6_dev_free(struct net_device *dev)
{
	struct ip6_tnltap *t = netdev_priv(dev);

	gro_cells_destroy(&t->gro_cells);
	dst_cache_destroy(&t->dst_cache);
	free_percpu(dev->tstats);
}

static int ip6_tnl_create2(struct net_device *dev)
{
	struct ip6_tnltap *t = netdev_priv(dev);
	struct net *net = dev_net(dev);
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	int err;

	t = netdev_priv(dev);
	if (t->parms.proto == IPPROTO_ETHERNET) {
		eth_hw_addr_random(dev);
		ether_setup(dev);
		eth_zero_addr(dev->perm_addr);
	}

	dev->rtnl_link_ops = &ip6_link_ops;
	err = register_netdevice(dev);
	if (err < 0)
		goto out;

	strcpy(t->parms.name, dev->name);

	ip6_tnl_link(ip6n, t);
	return 0;

out:
	return err;
}

/**
 * ip6_tnl_create - create a new tunnel
 *   @net: network namespace
 *   @p: tunnel parameters
 *
 * Description:
 *   Create tunnel matching given parameters.
 *
 * Return:
 *   created tunnel or error pointer
 **/

static struct ip6_tnltap *ip6_tnl_create(struct net *net, struct __ip6_tnltap_parm *p)
{
	struct net_device *dev;
	struct ip6_tnltap *t;
	char name[IFNAMSIZ];
	int err = -E2BIG;

	if (p->name[0]) {
		if (!dev_valid_name(p->name))
			goto failed;
		strlcpy(name, p->name, IFNAMSIZ);
	} else {
		sprintf(name, "ip6tnltap%%d");
	}
	err = -ENOMEM;
	dev = alloc_netdev(sizeof(*t), name, NET_NAME_UNKNOWN,
			   ip6_tnl_dev_setup);
	if (!dev)
		goto failed;

	dev_net_set(dev, net);

	t = netdev_priv(dev);
	t->parms = *p;
	t->net = dev_net(dev);
	err = ip6_tnl_create2(dev);
	if (err < 0)
		goto failed_free;

	return t;

failed_free:
	free_netdev(dev);
failed:
	return ERR_PTR(err);
}

/**
 * ip6_tnl_locate - find or create tunnel matching given parameters
 *   @net: network namespace
 *   @p: tunnel parameters
 *   @create: != 0 if allowed to create new tunnel if no match found
 *
 * Description:
 *   ip6_tnl_locate() first tries to locate an existing tunnel
 *   based on @parms. If this is unsuccessful, but @create is set a new
 *   tunnel device is created and registered for use.
 *
 * Return:
 *   matching tunnel or error pointer
 **/

static struct ip6_tnltap *ip6_tnl_locate(struct net *net,
		struct __ip6_tnltap_parm *p, int create)
{
	const struct in6_addr *remote = &p->raddr;
	const struct in6_addr *local = &p->laddr;
	struct ip6_tnltap __rcu **tp;
	struct ip6_tnltap *t;
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);

	for (tp = ip6_tnl_bucket(ip6n, p);
	     (t = rtnl_dereference(*tp)) != NULL;
	     tp = &t->next) {
		if (ipv6_addr_equal(local, &t->parms.laddr) &&
		    ipv6_addr_equal(remote, &t->parms.raddr) &&
		    (p->link == t->parms.link || p->link == 0)) {
			if (create)
				return ERR_PTR(-EEXIST);

			return t;
		}
	}
	if (!create)
		return ERR_PTR(-ENODEV);
	return ip6_tnl_create(net, p);
}

/**
 * ip6_tnl_dev_uninit - tunnel device uninitializer
 *   @dev: the device to be destroyed
 *
 * Description:
 *   ip6_tnl_dev_uninit() removes tunnel from its list
 **/

static void
ip6_tnl_dev_uninit(struct net_device *dev)
{
	struct ip6_tnltap *t = netdev_priv(dev);
	struct net *net = t->net;
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);

	if (t->parms.tuninfo)
		kfree(t->parms.tuninfo);
	t->parms.tuninfo = 0;

	while (t->parms.fmrs) {
		struct __ip6_tnl_fmr *next = t->parms.fmrs->next;
		kfree(t->parms.fmrs);
		t->parms.fmrs = next;
	}

	if (dev == ip6n->fb_tnl_dev)
		RCU_INIT_POINTER(ip6n->tnls_wc[0], NULL);
	else
		ip6_tnl_unlink(ip6n, t);
	dst_cache_reset(&t->dst_cache);
	dev_put(dev);
}

static int
ethip6_err(struct sk_buff *skb, struct inet6_skb_parm *opt,
	   u8 type, u8 code, int offset, __be32 info)
{
	return 0;
}

static inline int ethip6_dscp_ecn_decapsulate(const struct ip6_tnltap *t,
				       const struct ipv6hdr *ipv6h,
				       struct sk_buff *skb)
{
	return 0;
}

__u32 ip6_tnltap_get_cap(struct ip6_tnltap *t,
			     const struct in6_addr *laddr,
			     const struct in6_addr *raddr)
{
	struct __ip6_tnltap_parm *p = &t->parms;
	int ltype = ipv6_addr_type(laddr);
	int rtype = ipv6_addr_type(raddr);
	__u32 flags = 0;

	if (ltype == IPV6_ADDR_ANY || rtype == IPV6_ADDR_ANY) {
		flags = IP6_TNL_F_CAP_PER_PACKET;
	} else if (ltype & (IPV6_ADDR_UNICAST|IPV6_ADDR_MULTICAST) &&
		   rtype & (IPV6_ADDR_UNICAST|IPV6_ADDR_MULTICAST) &&
		   !((ltype|rtype) & IPV6_ADDR_LOOPBACK) &&
		   (!((ltype|rtype) & IPV6_ADDR_LINKLOCAL) || p->link)) {
		if (ltype&IPV6_ADDR_UNICAST)
			flags |= IP6_TNL_F_CAP_XMIT;
		if (rtype&IPV6_ADDR_UNICAST)
			flags |= IP6_TNL_F_CAP_RCV;
	}
	return flags;
}

/* called with rcu_read_lock() */
int ip6_tnltap_rcv_ctl(struct ip6_tnltap *t,
				  const struct in6_addr *laddr,
				  const struct in6_addr *raddr)
{
	struct __ip6_tnltap_parm *p = &t->parms;
	int ret = 0;
	struct net *net = t->net;

	if ((p->flags & IP6_TNL_F_CAP_RCV) ||
	    ((p->flags & IP6_TNL_F_CAP_PER_PACKET) &&
	     (ip6_tnltap_get_cap(t, laddr, raddr) & IP6_TNL_F_CAP_RCV))) {
		struct net_device *ldev = NULL;

		if (p->link)
			ldev = dev_get_by_index_rcu(net, p->link);

		if ((ipv6_addr_is_multicast(laddr) ||
		     likely(ipv6_chk_addr_and_flags(net, laddr, ldev, false,
						    0, IFA_F_TENTATIVE))) &&
		    ((p->flags & IP6_TNL_F_ALLOW_LOCAL_REMOTE) ||
		     likely(!ipv6_chk_addr_and_flags(net, raddr, ldev, true,
						     0, IFA_F_TENTATIVE))))
			ret = 1;
	}
	return ret;
}

/**
 * ip4ip6_fmr_calc - calculate target / source IPv6-address based on FMR
 *   @dest: destination IPv6 address buffer
 *   @skb: received socket buffer
 *   @fmr: MAP FMR
 *   @xmit: Calculate for xmit or rcv
 **/
static void ip4ip6_fmr_calc(struct in6_addr *dest,
		const struct iphdr *iph, const uint8_t *end,
		const struct __ip6_tnl_fmr *fmr, bool xmit)
{
	int psidlen = fmr->rule.ea_len - (32 - fmr->rule.ip4_prefix_len);
	u8 *portp = NULL;
	bool use_dest_addr;
	const struct iphdr *dsth = iph;

	if ((u8 *)dsth >= end)
		return;

	/* find significant IP header */
	if (iph->protocol == IPPROTO_ICMP) {
		struct icmphdr *ih = (struct icmphdr *)(((u8 *)dsth) + dsth->ihl * 4);
		if (ih && ((u8 *)&ih[1]) <= end && (
			ih->type == ICMP_DEST_UNREACH ||
			ih->type == ICMP_SOURCE_QUENCH ||
			ih->type == ICMP_TIME_EXCEEDED ||
			ih->type == ICMP_PARAMETERPROB ||
			ih->type == ICMP_REDIRECT))
				dsth = (const struct iphdr *)&ih[1];
	}

	/* in xmit-path use dest port by default and source port only if
		this is an ICMP reply to something else; vice versa in rcv-path */
	use_dest_addr = (xmit && dsth == iph) || (!xmit && dsth != iph);

	/* get dst port */
	if (((u8 *)&dsth[1]) <= end && (
		dsth->protocol == IPPROTO_UDP ||
		dsth->protocol == IPPROTO_TCP ||
		dsth->protocol == IPPROTO_SCTP ||
		dsth->protocol == IPPROTO_DCCP)) {
			/* for UDP, TCP, SCTP and DCCP source and dest port
			follow IPv4 header directly */
			portp = ((u8 *)dsth) + dsth->ihl * 4;

			if (use_dest_addr)
				portp += sizeof(u16);
	} else if (iph->protocol == IPPROTO_ICMP) {
		struct icmphdr *ih = (struct icmphdr *)(((u8 *)dsth) + dsth->ihl * 4);

		/* use icmp identifier as port */
		if (((u8 *)&ih) <= end && (
		    (use_dest_addr && (
		    ih->type == ICMP_ECHOREPLY ||
			ih->type == ICMP_TIMESTAMPREPLY ||
			ih->type == ICMP_INFO_REPLY ||
			ih->type == ICMP_ADDRESSREPLY)) ||
			(!use_dest_addr && (
			ih->type == ICMP_ECHO ||
			ih->type == ICMP_TIMESTAMP ||
			ih->type == ICMP_INFO_REQUEST ||
			ih->type == ICMP_ADDRESS)
			)))
				portp = (u8 *)&ih->un.echo.id;
	}

	if ((portp && &portp[2] <= end) || psidlen == 0) {
		int frombyte = fmr->rule.ip6_prefix_len / 8;
		int fromrem = fmr->rule.ip6_prefix_len % 8;
		int bytes = sizeof(struct in6_addr) - frombyte;
		const u32 *addr = (use_dest_addr) ? &iph->daddr : &iph->saddr;
		u64 eabits = ((u64)ntohl(*addr)) << (32 + fmr->rule.ip4_prefix_len);
		u64 t = 0;

		/* extract PSID from port and add it to eabits */
		u16 psidbits = 0;
		if (psidlen > 0) {
			psidbits = ((u16)portp[0]) << 8 | ((u16)portp[1]);
			psidbits >>= 16 - psidlen - fmr->rule.offset;
			psidbits = (u16)(psidbits << (16 - psidlen));
			eabits |= ((u64)psidbits) << (48 - (fmr->rule.ea_len - psidlen));
		}

		/* rewrite destination address */
		*dest = fmr->rule.ip6_prefix;
		memcpy(&dest->s6_addr[10], addr, sizeof(*addr));
		dest->s6_addr16[7] = htons(psidbits >> (16 - psidlen));

		if (bytes > sizeof(u64))
			bytes = sizeof(u64);

		/* insert eabits */
		memcpy(&t, &dest->s6_addr[frombyte], bytes);
		t = be64_to_cpu(t) & ~(((((u64)1) << fmr->rule.ea_len) - 1)
			<< (64 - fmr->rule.ea_len - fromrem));
		t = cpu_to_be64(t | (eabits >> fromrem));
		memcpy(&dest->s6_addr[frombyte], &t, bytes);
	}
}


static int __ip6_tnltap_rcv(struct ip6_tnltap *tunnel, struct sk_buff *skb,
			 const struct tnl_ptk_info *tpi,
			 struct metadata_dst *tun_dst,
			 int (*dscp_ecn_decapsulate)(const struct ip6_tnltap *t,
						const struct ipv6hdr *ipv6h,
						struct sk_buff *skb),
			 bool log_ecn_err)
{
	struct pcpu_sw_netstats *tstats;
	const struct ipv6hdr *ipv6h = ipv6_hdr(skb);
	int err;

	if ((!(tpi->flags & TUNNEL_CSUM) &&
	     (tunnel->parms.i_flags & TUNNEL_CSUM)) ||
	    ((tpi->flags & TUNNEL_CSUM) &&
	     !(tunnel->parms.i_flags & TUNNEL_CSUM))) {
		tunnel->dev->stats.rx_crc_errors++;
		tunnel->dev->stats.rx_errors++;
		goto drop;
	}

	if (tunnel->parms.i_flags & TUNNEL_SEQ) {
		if (!(tpi->flags & TUNNEL_SEQ) ||
		    (tunnel->i_seqno &&
		     (s32)(ntohl(tpi->seq) - tunnel->i_seqno) < 0)) {
			tunnel->dev->stats.rx_fifo_errors++;
			tunnel->dev->stats.rx_errors++;
			goto drop;
		}
		tunnel->i_seqno = ntohl(tpi->seq) + 1;
	}

	/* Warning: All skb pointers will be invalidated! */
	if (tunnel->dev->type == ARPHRD_ETHER) {
		if (!pskb_may_pull(skb, ETH_HLEN)) {
			tunnel->dev->stats.rx_length_errors++;
			tunnel->dev->stats.rx_errors++;
			goto drop;
		}

		ipv6h = ipv6_hdr(skb);
		skb->protocol = eth_type_trans(skb, tunnel->dev);
		skb_postpull_rcsum(skb, eth_hdr(skb), ETH_HLEN);
	} else {
		skb->dev = tunnel->dev;
	}

	skb_reset_network_header(skb);
	memset(skb->cb, 0, sizeof(struct inet6_skb_parm));

	__skb_tunnel_rx(skb, tunnel->dev, tunnel->net);

	err = dscp_ecn_decapsulate(tunnel, ipv6h, skb);
	if (unlikely(err)) {
		if (log_ecn_err)
			net_info_ratelimited("non-ECT from %pI6 with DS=%#x\n",
					     &ipv6h->saddr,
					     ipv6_get_dsfield(ipv6h));
		if (err > 1) {
			++tunnel->dev->stats.rx_frame_errors;
			++tunnel->dev->stats.rx_errors;
			goto drop;
		}
	}

	tstats = this_cpu_ptr(tunnel->dev->tstats);
	u64_stats_update_begin(&tstats->syncp);
	tstats->rx_packets++;
	tstats->rx_bytes += skb->len;
	u64_stats_update_end(&tstats->syncp);

	skb_scrub_packet(skb, !net_eq(tunnel->net, dev_net(tunnel->dev)));

	if (tun_dst)
		skb_dst_set(skb, (struct dst_entry *)tun_dst);

	gro_cells_receive(&tunnel->gro_cells, skb);
	return 0;

drop:
	if (tun_dst)
		dst_release((struct dst_entry *)tun_dst);
	kfree_skb(skb);
	return 0;
}

static const struct tnl_ptk_info tpi_teb = {
	/* no tunnel info required for ipxip6. */
	.proto = htons(ETH_P_TEB),
};

static int tapip6_rcv(struct sk_buff *skb, u8 ipproto,
		      const struct tnl_ptk_info *tpi,
		      int (*dscp_ecn_decapsulate)(const struct ip6_tnltap *t,
						  const struct ipv6hdr *ipv6h,
						  struct sk_buff *skb))
{
	struct ip6_tnltap *t;
	const struct ipv6hdr *ipv6h = ipv6_hdr(skb);
	struct metadata_dst *tun_dst = NULL;
	int ret = -1;

	rcu_read_lock();
	t = ip6_tnl_lookup(dev_net(skb->dev), skb->dev->ifindex, &ipv6h->saddr, &ipv6h->daddr);

	if (t) {
		u8 tproto = READ_ONCE(t->parms.proto);

		DBG_PRINT(DBG_RX, "RX packet in tapip6 tunnel \'%s\': local %pI6c remote %pI6c\n", t->dev->name, &t->parms.laddr, &t->parms.raddr);
		if (tproto != ipproto && tproto != 0)
			goto drop;
		if (!xfrm6_policy_check(NULL, XFRM_POLICY_IN, skb))
			goto drop;
		ipv6h = ipv6_hdr(skb);
		if (!ip6_tnltap_rcv_ctl(t, &ipv6h->daddr, &ipv6h->saddr))
			goto drop;
		if (iptunnel_pull_header(skb, 0, tpi->proto, false))
			goto drop;
		if (t->parms.collect_md) {
			tun_dst = ipv6_tun_rx_dst(skb, 0, 0, 0);
			if (!tun_dst)
				goto drop;
		}
		ret = __ip6_tnltap_rcv(t, skb, tpi, tun_dst, dscp_ecn_decapsulate,
				    log_ecn_error);
	}

	rcu_read_unlock();

	return ret;

drop:
	rcu_read_unlock();
	kfree_skb(skb);
	return 0;
}

static int ethip6_rcv(struct sk_buff *skb)
{
	return tapip6_rcv(skb, IPPROTO_ETHERNET, &tpi_teb,
			  ethip6_dscp_ecn_decapsulate);
}

struct ipv6_tel_txoption {
	struct ipv6_txoptions ops;
	__u8 dst_opt[8];
};

static void init_tel_txopt(struct ipv6_tel_txoption *opt, __u8 encap_limit)
{
	memset(opt, 0, sizeof(struct ipv6_tel_txoption));

	opt->dst_opt[2] = IPV6_TLV_TNL_ENCAP_LIMIT;
	opt->dst_opt[3] = 1;
	opt->dst_opt[4] = encap_limit;
	opt->dst_opt[5] = IPV6_TLV_PADN;
	opt->dst_opt[6] = 1;

	opt->ops.dst1opt = (struct ipv6_opt_hdr *) opt->dst_opt;
	opt->ops.opt_nflen = 8;
}

/**
 * ip6_tnl_addr_conflict - compare packet addresses to tunnel's own
 *   @t: the outgoing tunnel device
 *   @hdr: IPv6 header from the incoming packet
 *
 * Description:
 *   Avoid trivial tunneling loop by checking that tunnel exit-point
 *   doesn't match source of incoming packet.
 *
 * Return:
 *   1 if conflict,
 *   0 else
 **/

static inline bool
ip6_tnl_addr_conflict(const struct ip6_tnltap *t, const struct ipv6hdr *hdr)
{
	return ipv6_addr_equal(&t->parms.raddr, &hdr->saddr);
}

int ip6_tnltap_xmit_ctl(struct ip6_tnltap *t,
		     const struct in6_addr *laddr,
		     const struct in6_addr *raddr)
{
	struct __ip6_tnltap_parm *p = &t->parms;
	int ret = 0;
	struct net *net = t->net;

	if (t->parms.collect_md)
		return 1;

	if ((p->flags & IP6_TNL_F_CAP_XMIT) ||
	    ((p->flags & IP6_TNL_F_CAP_PER_PACKET) &&
	     (ip6_tnltap_get_cap(t, laddr, raddr) & IP6_TNL_F_CAP_XMIT))) {
		struct net_device *ldev = NULL;

		rcu_read_lock();
		if (p->link)
			ldev = dev_get_by_index_rcu(net, p->link);

		if (unlikely(!ipv6_chk_addr_and_flags(net, laddr, ldev, false,
						      0, IFA_F_TENTATIVE)))
			pr_warn("%s xmit: Local address not yet configured!\n",
				p->name);
		else if (!(p->flags & IP6_TNL_F_ALLOW_LOCAL_REMOTE) &&
			 !ipv6_addr_is_multicast(raddr) &&
			 unlikely(ipv6_chk_addr_and_flags(net, raddr, ldev,
							  true, 0, IFA_F_TENTATIVE)))
			pr_warn("%s xmit: Routing loop! Remote address found on this node!\n",
				p->name);
		else
			ret = 1;
		rcu_read_unlock();
	}
	return ret;
}

/**
 * ip6_tnltap_xmit - encapsulate packet and send
 *   @skb: the outgoing socket buffer
 *   @dev: the outgoing tunnel device
 *   @dsfield: dscp code for outer header
 *   @fl6: flow of tunneled packet
 *   @encap_limit: encapsulation limit
 *   @pmtu: Path MTU is stored if packet is too big
 *   @proto: next header value
 *
 * Description:
 *   Build new header and do some sanity checks on the packet before sending
 *   it.
 *
 * Return:
 *   0 on success
 *   -1 fail
 *   %-EMSGSIZE message too big. return mtu in this case.
 **/

int ip6_tnltap_xmit(struct sk_buff *skb, struct net_device *dev, __u8 dsfield,
		 struct flowi6 *fl6, int encap_limit, __u32 *pmtu,
		 __u8 proto)
{
	struct ip6_tnltap *t = netdev_priv(dev);
	struct net *net = t->net;
	struct net_device_stats *stats = &t->dev->stats;
	struct ipv6hdr *ipv6h;
	struct ipv6_tel_txoption opt;
	struct dst_entry *dst = NULL, *ndst = NULL;
	struct net_device *tdev;
	int mtu;
	unsigned int eth_hlen = t->dev->type == ARPHRD_ETHER ? ETH_HLEN : 0;
	unsigned int psh_hlen = sizeof(struct ipv6hdr) + t->encap_hlen;
	unsigned int max_headroom = psh_hlen;
	bool use_cache = false;
	u8 hop_limit;
	int err = -1;

	if (t->parms.collect_md) {
		hop_limit = skb_tunnel_info(skb)->key.ttl;
		goto route_lookup;
	} else {
		hop_limit = t->parms.hop_limit;
	}

	/* NBMA tunnel */
	if (ipv6_addr_any(&t->parms.raddr)) {
		if (skb->protocol == htons(ETH_P_IPV6)) {
			struct in6_addr *addr6;
			struct neighbour *neigh;
			int addr_type;

			if (!skb_dst(skb))
				goto tx_err_link_failure;

			neigh = dst_neigh_lookup(skb_dst(skb),
						 &ipv6_hdr(skb)->daddr);
			if (!neigh)
				goto tx_err_link_failure;

			addr6 = (struct in6_addr *)&neigh->primary_key;
			addr_type = ipv6_addr_type(addr6);

			if (addr_type == IPV6_ADDR_ANY)
				addr6 = &ipv6_hdr(skb)->daddr;

			memcpy(&fl6->daddr, addr6, sizeof(fl6->daddr));
			neigh_release(neigh);
		}
	} else if (t->parms.proto != 0 && !(t->parms.flags &
					    (IP6_TNL_F_USE_ORIG_TCLASS |
					     IP6_TNL_F_USE_ORIG_FWMARK))) {
		/* enable the cache only if neither the outer protocol nor the
		 * routing decision depends on the current inner header value
		 */
		use_cache = true;
	}

	if (use_cache)
		dst = dst_cache_get(&t->dst_cache);

	if (!ip6_tnltap_xmit_ctl(t, &fl6->saddr, &fl6->daddr))
		goto tx_err_link_failure;

	if (!dst) {
route_lookup:
		/* add dsfield to flowlabel for route lookup */
		fl6->flowlabel = ip6_make_flowinfo(dsfield, fl6->flowlabel);

//#ifdef CONFIG_RTK_IPV6_ETHERNET_TUNNEL
		if (t->parms.tuninfo == NULL)
			dst = ip6_route_output(net, NULL, fl6);
		else {
			struct flowi6 newfl6;
			struct ipv6_sr_hdr *osrh;

			osrh = t->parms.tuninfo->srh;
			memcpy(&newfl6, fl6, sizeof(struct flowi6));
			/* With SRH, outer dip would be the first sid */
			newfl6.daddr = osrh->segments[osrh->first_segment];
			dst = ip6_route_output(net, NULL, &newfl6);
		}
//#endif

		if (dst->error)
			goto tx_err_link_failure;
		dst = xfrm_lookup(net, dst, flowi6_to_flowi(fl6), NULL, 0);
		if (IS_ERR(dst)) {
			err = PTR_ERR(dst);
			dst = NULL;
			goto tx_err_link_failure;
		}
		if (t->parms.collect_md && ipv6_addr_any(&fl6->saddr) &&
		    ipv6_dev_get_saddr(net, ip6_dst_idev(dst)->dev,
				       &fl6->daddr, 0, &fl6->saddr))
			goto tx_err_link_failure;
		ndst = dst;
	}

	tdev = dst->dev;

	if (tdev == dev) {
		stats->collisions++;
		net_warn_ratelimited("%s: Local routing loop detected!\n",
				     t->parms.name);
		goto tx_err_dst_release;
	}
	mtu = dst_mtu(dst) - eth_hlen - psh_hlen - t->tun_hlen;
	if (encap_limit >= 0) {
		max_headroom += 8;
		mtu -= 8;
	}
	mtu = max(mtu, skb->protocol == htons(ETH_P_IPV6) ?
		       IPV6_MIN_MTU : IPV4_MIN_MTU);

	skb_dst_update_pmtu_no_confirm(skb, mtu);
	if (skb->len - t->tun_hlen - eth_hlen > mtu && !skb_is_gso(skb)) {
		*pmtu = mtu;
		err = -EMSGSIZE;
		goto tx_err_dst_release;
	}

	if (t->err_count > 0) {
		if (time_before(jiffies,
				t->err_time + IP6TUNNEL_ERR_TIMEO)) {
			t->err_count--;

			dst_link_failure(skb);
		} else {
			t->err_count = 0;
		}
	}

	skb_scrub_packet(skb, !net_eq(t->net, dev_net(dev)));

	/*
	 * Okay, now see if we can stuff it in the buffer as-is.
	 */
	max_headroom += LL_RESERVED_SPACE(tdev);

	if (skb_headroom(skb) < max_headroom || skb_shared(skb) ||
	    (skb_cloned(skb) && !skb_clone_writable(skb, 0))) {
		struct sk_buff *new_skb;

		new_skb = skb_realloc_headroom(skb, max_headroom);
		if (!new_skb)
			goto tx_err_dst_release;

		if (skb->sk)
			skb_set_owner_w(new_skb, skb->sk);
		consume_skb(skb);
		skb = new_skb;
	}

	if (t->parms.collect_md) {
		if (t->encap.type != TUNNEL_ENCAP_NONE)
			goto tx_err_dst_release;
	} else {
		if (use_cache && ndst)
			dst_cache_set_ip6(&t->dst_cache, ndst, &fl6->saddr);
	}
	skb_dst_set(skb, dst);

	if (hop_limit == 0) {
		if (skb->protocol == htons(ETH_P_IP))
			hop_limit = ip_hdr(skb)->ttl;
		else if (skb->protocol == htons(ETH_P_IPV6))
			hop_limit = ipv6_hdr(skb)->hop_limit;
		else
			hop_limit = ip6_dst_hoplimit(dst);
	}

	/* Calculate max headroom for all the headers and adjust
	 * needed_headroom if necessary.
	 */
	max_headroom = LL_RESERVED_SPACE(dst->dev) + sizeof(struct ipv6hdr)
			+ dst->header_len + t->hlen;
	if (max_headroom > dev->needed_headroom)
		dev->needed_headroom = max_headroom;

	err = ip6_tnltap_encap(skb, t, &proto, fl6);
	if (err)
		return err;

	if (encap_limit >= 0) {
		init_tel_txopt(&opt, encap_limit);
		ipv6_push_frag_opts(skb, &opt.ops, &proto);
	}

//#ifdef CONFIG_RTK_IPV6_ETHERNET_TUNNEL
	/* Added for SRH */
	if (t->parms.tuninfo == NULL) {
//#endif
		skb_push(skb, sizeof(struct ipv6hdr));
		skb_reset_network_header(skb);
		ipv6h = ipv6_hdr(skb);
		ip6_flow_hdr(ipv6h, dsfield,
				ip6_make_flowlabel(net, skb, fl6->flowlabel, true, fl6));
		ipv6h->hop_limit = hop_limit;
		ipv6h->nexthdr = proto;
		ipv6h->saddr = fl6->saddr;
		ipv6h->daddr = fl6->daddr;
//#ifdef CONFIG_RTK_IPV6_ETHERNET_TUNNEL
	} else { /* Insert SRH */
		int hdrlen, tot_len;
		struct ipv6_sr_hdr *osrh, *isrh;

		osrh = t->parms.tuninfo->srh;
		hdrlen = (osrh->hdrlen + 1) << 3;
		tot_len = hdrlen + sizeof(struct ipv6hdr);
		err = skb_cow_head(skb, tot_len);
		if (unlikely(err))
			return err;
		skb_push(skb, tot_len);
		skb_reset_network_header(skb);
		ipv6h = ipv6_hdr(skb);
		ip6_flow_hdr(ipv6h, dsfield,
			ip6_make_flowlabel(net, skb, fl6->flowlabel, true, fl6));
		ipv6h->hop_limit = hop_limit;
		isrh = (void *)ipv6h + sizeof(*ipv6h);
		memcpy(isrh, osrh, hdrlen);
		isrh->nexthdr = proto;
		ipv6h->nexthdr = NEXTHDR_ROUTING;
		#if 1
		/* remote ip as last_segment */
		isrh->segments[0] = fl6->daddr;
		ipv6h->saddr = fl6->saddr;
		ipv6h->daddr = isrh->segments[isrh->first_segment];
		#else
		/* remote ip as first_segment */
		isrh->segments[isrh->first_segment] = fl6->daddr;
		ipv6h->saddr = fl6->saddr;
		ipv6h->daddr = fl6->daddr;
		#endif
	}
//#endif
	ip6tunnel_xmit(NULL, skb, dev);
	return 0;
tx_err_link_failure:
	stats->tx_carrier_errors++;
	dst_link_failure(skb);
tx_err_dst_release:
	dst_release(dst);
	return err;
}

static inline int
tapip6_tnl_xmit(struct sk_buff *skb, struct net_device *dev,
		u8 protocol)
{
	struct ip6_tnltap *t = netdev_priv(dev);
	struct __ip6_tnl_fmr *fmr;
	struct ipv6hdr *ipv6h;
	const struct iphdr  *iph;
	int encap_limit = -1;
	__u16 offset;
	struct flowi6 fl6;
	__u8 dsfield, orig_dsfield;
	__u32 mtu;
	u8 tproto;
	int err;

	tproto = READ_ONCE(t->parms.proto);
	if (tproto != protocol && tproto != 0)
		return -1;

	if (t->parms.collect_md) {
		struct ip_tunnel_info *tun_info;
		const struct ip_tunnel_key *key;

		tun_info = skb_tunnel_info(skb);
		if (unlikely(!tun_info || !(tun_info->mode & IP_TUNNEL_INFO_TX) ||
			     ip_tunnel_info_af(tun_info) != AF_INET6))
			return -1;
		key = &tun_info->key;
		memset(&fl6, 0, sizeof(fl6));
		fl6.flowi6_proto = protocol;
		fl6.saddr = key->u.ipv6.src;
		fl6.daddr = key->u.ipv6.dst;
		fl6.flowlabel = key->label;
		dsfield =  key->tos;
		switch (protocol) {
		case IPPROTO_IPIP:
			iph = ip_hdr(skb);
			orig_dsfield = ipv4_get_dsfield(iph);
			break;
		case IPPROTO_IPV6:
			ipv6h = ipv6_hdr(skb);
			orig_dsfield = ipv6_get_dsfield(ipv6h);
			break;
		default:
			orig_dsfield = dsfield;
			break;
		}
	} else {
		if (!(t->parms.flags & IP6_TNL_F_IGN_ENCAP_LIMIT))
			encap_limit = t->parms.encap_limit;
		if (protocol == IPPROTO_IPV6) {
			offset = ip6_tnl_parse_tlv_enc_lim(skb,
						skb_network_header(skb));
			/* ip6_tnl_parse_tlv_enc_lim() might have
			 * reallocated skb->head
			 */
			if (offset > 0) {
				struct ipv6_tlv_tnl_enc_lim *tel;

				tel = (void *)&skb_network_header(skb)[offset];
				if (tel->encap_limit == 0) {
					icmpv6_ndo_send(skb, ICMPV6_PARAMPROB,
							ICMPV6_HDR_FIELD, offset + 2);
					return -1;
				}
				encap_limit = tel->encap_limit - 1;
			}
		}

		memcpy(&fl6, &t->fl.u.ip6, sizeof(fl6));
		fl6.flowi6_proto = protocol;

		if (t->parms.flags & IP6_TNL_F_USE_ORIG_FWMARK)
			fl6.flowi6_mark = skb->mark;
		else
			fl6.flowi6_mark = t->parms.fwmark;
		switch (protocol) {
		case IPPROTO_IPIP:
			iph = ip_hdr(skb);
			orig_dsfield = ipv4_get_dsfield(iph);
			if (t->parms.flags & IP6_TNL_F_USE_ORIG_TCLASS)
				dsfield = orig_dsfield;
			else
				dsfield = ip6_tclass(t->parms.flowinfo);
			break;
		case IPPROTO_IPV6:
			ipv6h = ipv6_hdr(skb);
			orig_dsfield = ipv6_get_dsfield(ipv6h);
			if (t->parms.flags & IP6_TNL_F_USE_ORIG_TCLASS)
				dsfield = orig_dsfield;
			else
				dsfield = ip6_tclass(t->parms.flowinfo);
			if (t->parms.flags & IP6_TNL_F_USE_ORIG_FLOWLABEL)
				fl6.flowlabel |= ip6_flowlabel(ipv6h);
			break;
		default:
			orig_dsfield = dsfield = ip6_tclass(t->parms.flowinfo);
			break;
		}
	}

	fl6.flowi6_uid = sock_net_uid(dev_net(dev), NULL);
	dsfield = INET_ECN_encapsulate(dsfield, orig_dsfield);

	/* try to find matching FMR */
	for (fmr = t->parms.fmrs; fmr; fmr = fmr->next) {
		unsigned mshift = 32 - fmr->rule.ip4_prefix_len;
		if (ntohl(fmr->rule.ip4_prefix.s_addr) >> mshift ==
				ntohl(ip_hdr(skb)->daddr) >> mshift)
			break;
	}

	/* change dstaddr according to FMR */
	if (fmr)
		ip4ip6_fmr_calc(&fl6.daddr, ip_hdr(skb), skb_tail_pointer(skb), fmr, true);

	if (iptunnel_handle_offloads(skb, SKB_GSO_IPXIP6))
		return -1;

	skb_set_inner_ipproto(skb, protocol);

	err = ip6_tnltap_xmit(skb, dev, dsfield, &fl6, encap_limit, &mtu,
			   protocol);
	if (err != 0) {
		/* XXX: send ICMP error even if DF is not set. */
		if (err == -EMSGSIZE)
			switch (protocol) {
			case IPPROTO_IPIP:
				icmp_ndo_send(skb, ICMP_DEST_UNREACH,
					      ICMP_FRAG_NEEDED, htonl(mtu));
				break;
			case IPPROTO_IPV6:
				icmpv6_ndo_send(skb, ICMPV6_PKT_TOOBIG, 0, mtu);
				break;
			default:
				break;
			}
		return -1;
	}

	return 0;
}

static netdev_tx_t
ip6_tnl_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ip6_tnltap *t = netdev_priv(dev);
	struct net_device_stats *stats = &t->dev->stats;
	u8 tproto;
	int ret;

	if (!pskb_inet_may_pull(skb))
		goto tx_err;

	/* IPPROTO_ETHERNET */
	tproto = READ_ONCE(t->parms.proto);

	DBG_PRINT(DBG_TX, "TX packet in tapip6 tunnel \'%s\': local %pI6c remote %pI6c\n", dev->name, &t->parms.laddr, &t->parms.raddr);
	ret = tapip6_tnl_xmit(skb, dev, tproto);
	if (ret < 0)
		goto tx_err;

	return NETDEV_TX_OK;

tx_err:
	stats->tx_errors++;
	stats->tx_dropped++;
	kfree_skb(skb);
	return NETDEV_TX_OK;
}

static void ip6_tnl_link_config(struct ip6_tnltap *t)
{
	struct net_device *dev = t->dev;
	struct net_device *tdev = NULL;
	struct __ip6_tnltap_parm *p = &t->parms;
	struct flowi6 *fl6 = &t->fl.u.ip6;
	unsigned int mtu;
	int t_hlen;

	/* Set up flowi template */
	fl6->saddr = p->laddr;
	fl6->daddr = p->raddr;
	fl6->flowi6_oif = p->link;
	fl6->flowlabel = 0;

	if (!(p->flags&IP6_TNL_F_USE_ORIG_TCLASS))
		fl6->flowlabel |= IPV6_TCLASS_MASK & p->flowinfo;
	if (!(p->flags&IP6_TNL_F_USE_ORIG_FLOWLABEL))
		fl6->flowlabel |= IPV6_FLOWLABEL_MASK & p->flowinfo;

	p->flags &= ~(IP6_TNL_F_CAP_XMIT|IP6_TNL_F_CAP_RCV|IP6_TNL_F_CAP_PER_PACKET);
	p->flags |= ip6_tnltap_get_cap(t, &p->laddr, &p->raddr);

	t->tun_hlen = 0;
	t->hlen = t->encap_hlen + t->tun_hlen;
	t_hlen = t->hlen + sizeof(struct ipv6hdr);

	if (p->flags & IP6_TNL_F_CAP_XMIT) {
		int strict = (ipv6_addr_type(&p->raddr) &
			      (IPV6_ADDR_MULTICAST|IPV6_ADDR_LINKLOCAL));

		struct rt6_info *rt = rt6_lookup(t->net,
						 &p->raddr, &p->laddr,
						 p->link, NULL, strict);
		if (rt) {
			tdev = rt->dst.dev;
			ip6_rt_put(rt);
		}

		if (!tdev && p->link)
			tdev = __dev_get_by_index(t->net, p->link);

		if (tdev) {
			dev->hard_header_len = tdev->hard_header_len + t_hlen;
			mtu = min_t(unsigned int, tdev->mtu, IP6_MAX_MTU);

			dev->mtu = mtu - t_hlen;
			if (p->proto == IPPROTO_ETHERNET)
				dev->mtu -= ETH_HLEN;
			if (!(t->parms.flags & IP6_TNL_F_IGN_ENCAP_LIMIT))
				dev->mtu -= 8;

			if (dev->mtu < IPV6_MIN_MTU)
				dev->mtu = IPV6_MIN_MTU;
		}
	}
}

/**
 * ip6_tnl_change - update the tunnel parameters
 *   @t: tunnel to be changed
 *   @p: tunnel configuration parameters
 *
 * Description:
 *   ip6_tnl_change() updates the tunnel parameters
 **/

static int
ip6_tnl_change(struct ip6_tnltap *t, const struct __ip6_tnltap_parm *p)
{
	t->parms.laddr = p->laddr;
	t->parms.raddr = p->raddr;
	t->parms.flags = p->flags;
	t->parms.hop_limit = p->hop_limit;
	t->parms.encap_limit = p->encap_limit;
	t->parms.flowinfo = p->flowinfo;
	t->parms.link = p->link;
	t->parms.proto = p->proto;
	t->parms.fwmark = p->fwmark;

	if (t->parms.tuninfo)
		kfree(t->parms.tuninfo);
	t->parms.tuninfo = p->tuninfo;

	while (t->parms.fmrs) {
		struct __ip6_tnl_fmr *next = t->parms.fmrs->next;
		kfree(t->parms.fmrs);
		t->parms.fmrs = next;
	}
	t->parms.fmrs = p->fmrs;

	dst_cache_reset(&t->dst_cache);
	ip6_tnl_link_config(t);
	return 0;
}

static int ip6_tnl_update(struct ip6_tnltap *t, struct __ip6_tnltap_parm *p)
{
	struct net *net = t->net;
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	int err;

	ip6_tnl_unlink(ip6n, t);
	synchronize_net();
	err = ip6_tnl_change(t, p);
	ip6_tnl_link(ip6n, t);
	netdev_state_change(t->dev);
	return err;
}

static int ip6_tnl0_update(struct ip6_tnltap *t, struct __ip6_tnltap_parm *p)
{
	/* for default tnl0 device allow to change only the proto */
	t->parms.proto = p->proto;
	netdev_state_change(t->dev);
	return 0;
}

static void
ip6_tnl_parm_from_user(struct __ip6_tnltap_parm *p, const struct ip6_tnl_parm *u)
{
	p->laddr = u->laddr;
	p->raddr = u->raddr;
	p->flags = u->flags;
	p->hop_limit = u->hop_limit;
	p->encap_limit = u->encap_limit;
	p->flowinfo = u->flowinfo;
	p->link = u->link;
	p->proto = u->proto;
	p->fmrs = NULL;
	p->tuninfo = NULL;
	memcpy(p->name, u->name, sizeof(u->name));
}

static void
ip6_tnl_parm_to_user(struct ip6_tnl_parm *u, const struct __ip6_tnltap_parm *p)
{
	u->laddr = p->laddr;
	u->raddr = p->raddr;
	u->flags = p->flags;
	u->hop_limit = p->hop_limit;
	u->encap_limit = p->encap_limit;
	u->flowinfo = p->flowinfo;
	u->link = p->link;
	u->proto = p->proto;
	memcpy(u->name, p->name, sizeof(u->name));
}

/**
 * ip6_tnl_ioctl - configure ipv6 tunnel-tap from userspace
 *   @dev: virtual device associated with tunnel
 *   @ifr: parameters passed from userspace
 *   @cmd: command to be performed
 *
 * Description:
 *   ip6_tnl_ioctl() is used for managing IPv6 tunnel-tap
 *   from userspace.
 *
 *   The possible commands are the following:
 *     %SIOCGETTUNNEL: get tunnel parameters for device
 *     %SIOCADDTUNNEL: add tunnel matching given tunnel parameters
 *     %SIOCCHGTUNNEL: change tunnel parameters to those given
 *     %SIOCDELTUNNEL: delete tunnel
 *
 *   The fallback device "ip6tnltap0", created during module
 *   initialization, can be used for creating other tunnel devices.
 *
 * Return:
 *   0 on success,
 *   %-EFAULT if unable to copy data to or from userspace,
 *   %-EPERM if current process hasn't %CAP_NET_ADMIN set
 *   %-EINVAL if passed tunnel parameters are invalid,
 *   %-EEXIST if changing a tunnel's parameters would cause a conflict
 *   %-ENODEV if attempting to change or delete a nonexisting device
 **/

static int
ip6_tnl_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	int err = 0;
	struct ip6_tnl_parm p;
	struct __ip6_tnltap_parm p1;
	struct ip6_tnltap *t = netdev_priv(dev);
	struct net *net = t->net;
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	struct ipv6_tnl_fmr fmrInfo;
	struct __ip6_tnl_fmr *fmrs_next, *fmrP;

	memset(&p1, 0, sizeof(p1));

	switch (cmd) {
	case SIOCGETTUNNEL:
		if (dev == ip6n->fb_tnl_dev) {
			if (copy_from_user(&p, ifr->ifr_ifru.ifru_data, sizeof(p))) {
				err = -EFAULT;
				break;
			}
			ip6_tnl_parm_from_user(&p1, &p);
			t = ip6_tnl_locate(net, &p1, 0);
			if (IS_ERR(t))
				t = netdev_priv(dev);
		} else {
			memset(&p, 0, sizeof(p));
		}
		ip6_tnl_parm_to_user(&p, &t->parms);
		if (copy_to_user(ifr->ifr_ifru.ifru_data, &p, sizeof(p))) {
			err = -EFAULT;
		}
		break;
	case SIOCADDTUNNEL:
	case SIOCCHGTUNNEL:
		err = -EPERM;
		if (!ns_capable(net->user_ns, CAP_NET_ADMIN))
			break;
		err = -EFAULT;
		if (copy_from_user(&p, ifr->ifr_ifru.ifru_data, sizeof(p)))
			break;
		err = -EINVAL;
		if (p.proto != IPPROTO_ETHERNET && p.proto != 0)
			break;
		ip6_tnl_parm_from_user(&p1, &p);
		t = ip6_tnl_locate(net, &p1, cmd == SIOCADDTUNNEL);
		if (cmd == SIOCCHGTUNNEL) {
			if (!IS_ERR(t)) {
				if (t->dev != dev) {
					err = -EEXIST;
					break;
				}
			} else
				t = netdev_priv(dev);
			if (dev == ip6n->fb_tnl_dev)
				err = ip6_tnl0_update(t, &p1);
			else
				err = ip6_tnl_update(t, &p1);
		}
		if (!IS_ERR(t)) {
			err = 0;
			ip6_tnl_parm_to_user(&p, &t->parms);
			if (copy_to_user(ifr->ifr_ifru.ifru_data, &p, sizeof(p)))
				err = -EFAULT;

		} else {
			err = PTR_ERR(t);
		}
		break;
	case SIOCDELTUNNEL:
		err = -EPERM;
		if (!ns_capable(net->user_ns, CAP_NET_ADMIN))
			break;

		if (dev == ip6n->fb_tnl_dev) {
			err = -EFAULT;
			if (copy_from_user(&p, ifr->ifr_ifru.ifru_data, sizeof(p)))
				break;
			err = -ENOENT;
			ip6_tnl_parm_from_user(&p1, &p);
			t = ip6_tnl_locate(net, &p1, 0);
			if (IS_ERR(t))
				break;
			err = -EPERM;
			if (t->dev == ip6n->fb_tnl_dev)
				break;
			dev = t->dev;
		}
		err = 0;
		/*free FMRS*/
		while (t->parms.fmrs) {
			fmrs_next = t->parms.fmrs->next;
			kfree(t->parms.fmrs);
			t->parms.fmrs = fmrs_next;
		}
		/*free tuninfo*/
		if (t->parms.tuninfo)
			kfree(t->parms.tuninfo);
		t->parms.tuninfo = 0;

		unregister_netdevice(dev);
		break;
	case SIOCSETFMRSRULE: /*SET FMRS*/
		err = -EPERM;
		if (copy_from_user(&fmrInfo, ifr->ifr_ifru.ifru_data, sizeof(fmrInfo)))
			break;
		memcpy(&p1.laddr, &fmrInfo.laddr, sizeof(p1.laddr));
		memcpy(&p1.raddr, &fmrInfo.raddr, sizeof(p1.raddr));
		t = ip6_tnl_locate(net, &p1, 0);
		if (IS_ERR(t))
			break;

		fmrP = (struct __ip6_tnl_fmr *)kmalloc(sizeof(struct __ip6_tnl_fmr), GFP_KERNEL);
		if (fmrP == NULL) {
			printk("[%s:%d][FMRS]:kmalloc for struct __ip6_tnl_fmr failed!!\n", __FUNCTION__, __LINE__);
			while (t->parms.fmrs) {
				fmrs_next = t->parms.fmrs->next;
				kfree(t->parms.fmrs);
				t->parms.fmrs = fmrs_next;
			}
			break;
		}
		memset(fmrP, 0x00, sizeof(struct __ip6_tnl_fmr));
		memcpy(&fmrP->rule, &fmrInfo.rule, sizeof(struct ip6_tnl_fmr_rule));
		if (t->parms.fmrs == NULL) {
			t->parms.fmrs = fmrP;
		} else {
			fmrs_next = t->parms.fmrs;
			while (fmrs_next->next)
				fmrs_next = fmrs_next->next;
			fmrs_next->next = fmrP;
		}
		err = 0;
		break;

	default:
		err = -EINVAL;
	}
	return err;
}

/**
 * ip6_tnltap_change_mtu - change mtu manually for tunnel device
 *   @dev: virtual device associated with tunnel
 *   @new_mtu: the new mtu
 *
 * Return:
 *   0 on success,
 *   %-EINVAL if mtu too small
 **/

int ip6_tnltap_change_mtu(struct net_device *dev, int new_mtu)
{
	struct ip6_tnltap *tnl = netdev_priv(dev);

	if (tnl->parms.proto == IPPROTO_IPV6) {
		if (new_mtu < IPV6_MIN_MTU)
			return -EINVAL;
	} else {
		if (new_mtu < ETH_MIN_MTU)
			return -EINVAL;
	}
	if (tnl->parms.proto == IPPROTO_IPV6 || tnl->parms.proto == 0) {
		if (new_mtu > IP6_MAX_MTU - dev->hard_header_len)
			return -EINVAL;
	} else {
		if (new_mtu > IP_MAX_MTU - dev->hard_header_len)
			return -EINVAL;
	}
	dev->mtu = new_mtu;
	return 0;
}

int ip6_tnltap_get_iflink(const struct net_device *dev)
{
	struct ip6_tnltap *t = netdev_priv(dev);

	return t->parms.link;
}

int ip6_tnltap_encap_setup(struct ip6_tnltap *t,
			struct ip_tunnel_encap *ipencap)
{
	int hlen;

	memset(&t->encap, 0, sizeof(t->encap));

	hlen = ip6_encap_hlen(ipencap);
	if (hlen < 0)
		return hlen;

	t->encap.type = ipencap->type;
	t->encap.sport = ipencap->sport;
	t->encap.dport = ipencap->dport;
	t->encap.flags = ipencap->flags;

	t->encap_hlen = hlen;
	t->hlen = t->encap_hlen + t->tun_hlen;

	return 0;
}

static const struct net_device_ops ip6_tnl_netdev_ops = {
	.ndo_init	= ip6_tnl_dev_init,
	.ndo_uninit	= ip6_tnl_dev_uninit,
	.ndo_start_xmit = ip6_tnl_start_xmit,
	.ndo_do_ioctl	= ip6_tnl_ioctl,
	.ndo_change_mtu = ip6_tnltap_change_mtu,
	.ndo_set_mac_address	= eth_mac_addr,
	.ndo_get_stats	= ip6_get_stats,
	.ndo_get_iflink = ip6_tnltap_get_iflink,
};

#if 1
//Avoid tunnel dev(ip6tunX) and pppX(PPPoE over IPv6) TSO default is enable.
#define IPXIPX_FEATURES (NETIF_F_SG |		\
			 NETIF_F_FRAGLIST |	\
			 NETIF_F_HIGHDMA |	\
			 NETIF_F_GSO_SOFTWARE)
#else
#define IPXIPX_FEATURES (NETIF_F_SG |		\
			 NETIF_F_FRAGLIST |	\
			 NETIF_F_HIGHDMA |	\
			 NETIF_F_GSO_SOFTWARE |	\
			 NETIF_F_HW_CSUM)
#endif

/**
 * ip6_tnl_dev_setup - setup virtual tunnel device
 *   @dev: virtual device associated with tunnel
 *
 * Description:
 *   Initialize function pointers and device parameters
 **/

static void ip6_tnl_dev_setup(struct net_device *dev)
{
	dev->netdev_ops = &ip6_tnl_netdev_ops;
	dev->header_ops = &ip_tunnel_header_ops;
	dev->needs_free_netdev = true;
	dev->priv_destructor = ip6_dev_free;

	dev->type = ARPHRD_TUNNEL6;
	dev->flags |= IFF_NOARP;
	dev->addr_len = sizeof(struct in6_addr);
	dev->features |= NETIF_F_LLTX;
	netif_keep_dst(dev);

	dev->features		|= IPXIPX_FEATURES;
	dev->hw_features	|= IPXIPX_FEATURES;

	/* This perm addr will be used as interface identifier by IPv6 */
	dev->addr_assign_type = NET_ADDR_RANDOM;
	eth_random_addr(dev->perm_addr);
}


/**
 * ip6_tnl_dev_init_gen - general initializer for all tunnel devices
 *   @dev: virtual device associated with tunnel
 **/

static inline int
ip6_tnl_dev_init_gen(struct net_device *dev)
{
	struct ip6_tnltap *t = netdev_priv(dev);
	int ret;
	int t_hlen;

	t->dev = dev;
	t->net = dev_net(dev);
	dev->tstats = netdev_alloc_pcpu_stats(struct pcpu_sw_netstats);
	if (!dev->tstats)
		return -ENOMEM;

	ret = dst_cache_init(&t->dst_cache, GFP_KERNEL);
	if (ret)
		goto free_stats;

	ret = gro_cells_init(&t->gro_cells, dev);
	if (ret)
		goto destroy_dst;

	t->tun_hlen = 0;
	t->hlen = t->encap_hlen + t->tun_hlen;
	t_hlen = t->hlen + sizeof(struct ipv6hdr);

	/* dev->type = ARPHRD_TUNNEL6; */
	dev->hard_header_len = LL_MAX_HEADER + t_hlen;
	dev->mtu = ETH_DATA_LEN - t_hlen;
	if (!(t->parms.flags & IP6_TNL_F_IGN_ENCAP_LIMIT))
		dev->mtu -= 8;
	dev->min_mtu = ETH_MIN_MTU;
	dev->max_mtu = IP6_MAX_MTU - dev->hard_header_len;

	dev_hold(dev);
	return 0;

destroy_dst:
	dst_cache_destroy(&t->dst_cache);
free_stats:
	free_percpu(dev->tstats);
	dev->tstats = NULL;

	return ret;
}

/**
 * ip6_tnl_dev_init - initializer for all non fallback tunnel devices
 *   @dev: virtual device associated with tunnel
 **/

static int ip6_tnl_dev_init(struct net_device *dev)
{
	struct ip6_tnltap *t = netdev_priv(dev);
	int err = ip6_tnl_dev_init_gen(dev);

	if (err)
		return err;
	ip6_tnl_link_config(t);
	if (t->parms.collect_md)
		netif_keep_dst(dev);
	return 0;
}

/**
 * ip6_fb_tnl_dev_init - initializer for fallback tunnel device
 *   @dev: fallback device
 *
 * Return: 0
 **/

static int __net_init ip6_fb_tnl_dev_init(struct net_device *dev)
{
	struct ip6_tnltap *t = netdev_priv(dev);
	struct net *net = dev_net(dev);
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);

	t->parms.proto = IPPROTO_IPV6;

	rcu_assign_pointer(ip6n->tnls_wc[0], t);
	return 0;
}

static int ip6_tnl_validate(struct nlattr *tb[], struct nlattr *data[],
			    struct netlink_ext_ack *extack)
{
	u8 proto;

	if (!data || !data[IFLA_IPTUN_PROTO])
		return 0;

	proto = nla_get_u8(data[IFLA_IPTUN_PROTO]);
	if (proto != IPPROTO_ETHERNET &&
	    proto != 0)
		return -EINVAL;

	return 0;
}

static const struct nla_policy ip6_tnl_fmr_policy[IFLA_IPTUN_FMR_MAX + 1] = {
	[IFLA_IPTUN_FMR_IP6_PREFIX] = { .len = sizeof(struct in6_addr) },
	[IFLA_IPTUN_FMR_IP4_PREFIX] = { .len = sizeof(struct in_addr) },
	[IFLA_IPTUN_FMR_IP6_PREFIX_LEN] = { .type = NLA_U8 },
	[IFLA_IPTUN_FMR_IP4_PREFIX_LEN] = { .type = NLA_U8 },
	[IFLA_IPTUN_FMR_EA_LEN] = { .type = NLA_U8 },
	[IFLA_IPTUN_FMR_OFFSET] = { .type = NLA_U8 }
};

static const struct nla_policy seg6_iptunnel_policy[SEG6_IPTUNNEL_MAX + 1] = {
	[SEG6_IPTUNNEL_SRH]	= { .type = NLA_BINARY },
};

/*
 * The ip6tnltap segs option allows last sid to be the same as remote ip to
 * support single sid in SRH.
 * This function merge the last sid with remote ip if they are the same.
 */
static int tuninfo_scrub_srh(struct ipv6_sr_hdr *srh, int *tuninfo_len, struct in6_addr *raddr)
{
	int i;

	/* Check if the last sid is the same as remote ip */
	if (srh->segments_left >= 1 && ipv6_addr_cmp(&srh->segments[1], raddr) == 0) {
		/* merge duplicate last segment */
		DBG_PRINT(DBG_INFO, "Merge last sid with remote ip\n");
		/* Segment 1 is the same as segment 0 (remote address). */
		/* We remove segment 0 (merged with segment 1) */
		for (i = 0; i < srh->segments_left; i++)
			memcpy(&srh->segments[i], &srh->segments[i+1], sizeof(struct in6_addr));
		srh->segments_left--;
		srh->first_segment--;
		*tuninfo_len -= 16;
	}
	return 0;
}

static void ip6_tnl_netlink_parms(struct nlattr *data[],
				  struct __ip6_tnltap_parm *parms)
{
	memset(parms, 0, sizeof(*parms));

	if (!data)
		return;

	if (data[IFLA_IPTUN_LINK])
		parms->link = nla_get_u32(data[IFLA_IPTUN_LINK]);

	if (data[IFLA_IPTUN_LOCAL])
		parms->laddr = nla_get_in6_addr(data[IFLA_IPTUN_LOCAL]);

	if (data[IFLA_IPTUN_REMOTE])
		parms->raddr = nla_get_in6_addr(data[IFLA_IPTUN_REMOTE]);

	if (data[IFLA_IPTUN_TTL])
		parms->hop_limit = nla_get_u8(data[IFLA_IPTUN_TTL]);

	if (data[IFLA_IPTUN_ENCAP_LIMIT])
		parms->encap_limit = nla_get_u8(data[IFLA_IPTUN_ENCAP_LIMIT]);

	if (data[IFLA_IPTUN_FLOWINFO])
		parms->flowinfo = nla_get_be32(data[IFLA_IPTUN_FLOWINFO]);

	if (data[IFLA_IPTUN_FLAGS])
		parms->flags = nla_get_u32(data[IFLA_IPTUN_FLAGS]);

	if (data[IFLA_IPTUN_PROTO])
		parms->proto = nla_get_u8(data[IFLA_IPTUN_PROTO]);

	/* ip6tnltap use IFLA_IPTUN_COLLECT_METADATA to nested SEG6_IPTUNNEL_SRH
	if (data[IFLA_IPTUN_COLLECT_METADATA])
		parms->collect_md = true;
	*/
	/* Use IFLA_IPTUN_COLLECT_METADATA to nested SRH attribute */
	if (data[IFLA_IPTUN_COLLECT_METADATA]) {
		struct nlattr *seg6info[SEG6_IPTUNNEL_MAX + 1];
		struct seg6_iptunnel_encap *tuninfo;
		int tuninfo_len, min_size;
		int err;

		err = nla_parse_nested_deprecated(seg6info, SEG6_IPTUNNEL_MAX, data[IFLA_IPTUN_COLLECT_METADATA],
					  seg6_iptunnel_policy, NULL);
		if (err < 0)
			pr_err("%s: parse SEG6_IPTUNNEL fail\n", __func__);
		if (seg6info[SEG6_IPTUNNEL_SRH]) {
			tuninfo = nla_data(seg6info[SEG6_IPTUNNEL_SRH]);
			tuninfo_len = nla_len(seg6info[SEG6_IPTUNNEL_SRH]);

			tuninfo_scrub_srh(tuninfo->srh, &tuninfo_len, &parms->raddr);
			/* tuninfo must contain at least the iptunnel encap structure,
			 * the SRH and one segment
			 */
			min_size = sizeof(*tuninfo) + sizeof(struct ipv6_sr_hdr) +
				sizeof(struct in6_addr);
			if (tuninfo_len < min_size)
				pr_err("%s: not enough tuninfo_len\n", __func__);

			/* verify that SRH is consistent */
			/* skip this to avoid 'unknown symbol in module' when loading as kernel module. */
			/*
			if (!seg6_validate_srh(tuninfo->srh, tuninfo_len - sizeof(*tuninfo), false))
				pr_err("%s: SRH verification fail\n", __func__);
			*/

			parms->tuninfo = kzalloc(tuninfo_len, GFP_KERNEL);
			memcpy(parms->tuninfo, tuninfo, tuninfo_len);
		}
	}

	if (data[IFLA_IPTUN_FWMARK])
		parms->fwmark = nla_get_u32(data[IFLA_IPTUN_FWMARK]);

	if (data[IFLA_IPTUN_FMRS]) {
		unsigned rem;
		struct nlattr *fmr;
		nla_for_each_nested(fmr, data[IFLA_IPTUN_FMRS], rem) {
			struct nlattr *fmrd[IFLA_IPTUN_FMR_MAX + 1], *c;
			struct __ip6_tnl_fmr *nfmr;

			nla_parse_nested(fmrd, IFLA_IPTUN_FMR_MAX,
				fmr, ip6_tnl_fmr_policy, NULL);

			if (!(nfmr = kzalloc(sizeof(*nfmr), GFP_KERNEL)))
				continue;

			nfmr->rule.offset = 6;

			if ((c = fmrd[IFLA_IPTUN_FMR_IP6_PREFIX]))
				nla_memcpy(&nfmr->rule.ip6_prefix, fmrd[IFLA_IPTUN_FMR_IP6_PREFIX],
					sizeof(nfmr->rule.ip6_prefix));

			if ((c = fmrd[IFLA_IPTUN_FMR_IP4_PREFIX]))
				nla_memcpy(&nfmr->rule.ip4_prefix, fmrd[IFLA_IPTUN_FMR_IP4_PREFIX],
					sizeof(nfmr->rule.ip4_prefix));

			if ((c = fmrd[IFLA_IPTUN_FMR_IP6_PREFIX_LEN]))
				nfmr->rule.ip6_prefix_len = nla_get_u8(c);

			if ((c = fmrd[IFLA_IPTUN_FMR_IP4_PREFIX_LEN]))
				nfmr->rule.ip4_prefix_len = nla_get_u8(c);

			if ((c = fmrd[IFLA_IPTUN_FMR_EA_LEN]))
				nfmr->rule.ea_len = nla_get_u8(c);

			if ((c = fmrd[IFLA_IPTUN_FMR_OFFSET]))
				nfmr->rule.offset = nla_get_u8(c);

			nfmr->next = parms->fmrs;
			parms->fmrs = nfmr;
		}
	}
}

static bool ip6_tnl_netlink_encap_parms(struct nlattr *data[],
					struct ip_tunnel_encap *ipencap)
{
	bool ret = false;

	memset(ipencap, 0, sizeof(*ipencap));

	if (!data)
		return ret;

	if (data[IFLA_IPTUN_ENCAP_TYPE]) {
		ret = true;
		ipencap->type = nla_get_u16(data[IFLA_IPTUN_ENCAP_TYPE]);
	}

	if (data[IFLA_IPTUN_ENCAP_FLAGS]) {
		ret = true;
		ipencap->flags = nla_get_u16(data[IFLA_IPTUN_ENCAP_FLAGS]);
	}

	if (data[IFLA_IPTUN_ENCAP_SPORT]) {
		ret = true;
		ipencap->sport = nla_get_be16(data[IFLA_IPTUN_ENCAP_SPORT]);
	}

	if (data[IFLA_IPTUN_ENCAP_DPORT]) {
		ret = true;
		ipencap->dport = nla_get_be16(data[IFLA_IPTUN_ENCAP_DPORT]);
	}

	return ret;
}

static int ip6_tnl_newlink(struct net *src_net, struct net_device *dev,
			   struct nlattr *tb[], struct nlattr *data[],
			   struct netlink_ext_ack *extack)
{
	struct net *net = dev_net(dev);
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	struct ip_tunnel_encap ipencap;
	struct ip6_tnltap *nt, *t;
	int err;

	nt = netdev_priv(dev);

	if (ip6_tnl_netlink_encap_parms(data, &ipencap)) {
		err = ip6_tnltap_encap_setup(nt, &ipencap);
		if (err < 0)
			return err;
	}

	ip6_tnl_netlink_parms(data, &nt->parms);

	if (nt->parms.collect_md) {
		if (rtnl_dereference(ip6n->collect_md_tun))
			return -EEXIST;
	} else {
		t = ip6_tnl_locate(net, &nt->parms, 0);
		if (!IS_ERR(t))
			return -EEXIST;
	}

	err = ip6_tnl_create2(dev);
	if (!err && tb[IFLA_MTU])
		ip6_tnltap_change_mtu(dev, nla_get_u32(tb[IFLA_MTU]));

	return err;
}

static int ip6_tnl_changelink(struct net_device *dev, struct nlattr *tb[],
			      struct nlattr *data[],
			      struct netlink_ext_ack *extack)
{
	struct ip6_tnltap *t = netdev_priv(dev);
	struct __ip6_tnltap_parm p;
	struct net *net = t->net;
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	struct ip_tunnel_encap ipencap;

	if (dev == ip6n->fb_tnl_dev)
		return -EINVAL;

	if (ip6_tnl_netlink_encap_parms(data, &ipencap)) {
		int err = ip6_tnltap_encap_setup(t, &ipencap);

		if (err < 0)
			return err;
	}
	ip6_tnl_netlink_parms(data, &p);
	if (p.collect_md)
		return -EINVAL;

	t = ip6_tnl_locate(net, &p, 0);
	if (!IS_ERR(t)) {
		if (t->dev != dev)
			return -EEXIST;
	} else
		t = netdev_priv(dev);

	return ip6_tnl_update(t, &p);
}

static void ip6_tnl_dellink(struct net_device *dev, struct list_head *head)
{
	struct net *net = dev_net(dev);
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);

	if (dev != ip6n->fb_tnl_dev)
		unregister_netdevice_queue(dev, head);
}

static size_t ip6_tnl_get_size(const struct net_device *dev)
{
	const struct ip6_tnltap *t = netdev_priv(dev);
	int tuninfo_len = 0;
	struct __ip6_tnl_fmr *c;
	int fmrs = 0;
	for (c = t->parms.fmrs; c; c = c->next)
		++fmrs;

	if (t->parms.tuninfo) {
		int srhlen;

		srhlen = (t->parms.tuninfo->srh->hdrlen + 1) << 3;
		tuninfo_len = sizeof(struct seg6_iptunnel_encap)+srhlen;
	}
	return
		/* IFLA_IPTUN_LINK */
		nla_total_size(4) +
		/* IFLA_IPTUN_LOCAL */
		nla_total_size(sizeof(struct in6_addr)) +
		/* IFLA_IPTUN_REMOTE */
		nla_total_size(sizeof(struct in6_addr)) +
		/* IFLA_IPTUN_TTL */
		nla_total_size(1) +
		/* IFLA_IPTUN_ENCAP_LIMIT */
		nla_total_size(1) +
		/* IFLA_IPTUN_FLOWINFO */
		nla_total_size(4) +
		/* IFLA_IPTUN_FLAGS */
		nla_total_size(4) +
		/* IFLA_IPTUN_PROTO */
		nla_total_size(1) +
		/* IFLA_IPTUN_ENCAP_TYPE */
		nla_total_size(2) +
		/* IFLA_IPTUN_ENCAP_FLAGS */
		nla_total_size(2) +
		/* IFLA_IPTUN_ENCAP_SPORT */
		nla_total_size(2) +
		/* IFLA_IPTUN_ENCAP_DPORT */
		nla_total_size(2) +
		/* IFLA_IPTUN_COLLECT_METADATA */
		/*
		nla_total_size(0) +
		*/
		nla_total_size(tuninfo_len) +
		/* IFLA_IPTUN_FWMARK */
		nla_total_size(4) +
		/* IFLA_IPTUN_FMRS */
		nla_total_size(0) +
		(
			/* nest */
			nla_total_size(0) +
			/* IFLA_IPTUN_FMR_IP6_PREFIX */
			nla_total_size(sizeof(struct in6_addr)) +
			/* IFLA_IPTUN_FMR_IP4_PREFIX */
			nla_total_size(sizeof(struct in_addr)) +
			/* IFLA_IPTUN_FMR_EA_LEN */
			nla_total_size(1) +
			/* IFLA_IPTUN_FMR_IP6_PREFIX_LEN */
			nla_total_size(1) +
			/* IFLA_IPTUN_FMR_IP4_PREFIX_LEN */
			nla_total_size(1) +
			/* IFLA_IPTUN_FMR_OFFSET */
			nla_total_size(1)
		) * fmrs +
		0;
}

/* Copy from net/ipv6/seg6_iptunnel.c */
static int nla_put_srh(struct sk_buff *skb, int attrtype,
		       struct seg6_iptunnel_encap *tuninfo)
{
	struct seg6_iptunnel_encap *data;
	struct nlattr *nla;
	int len;

	len = SEG6_IPTUN_ENCAP_SIZE(tuninfo);

	nla = nla_reserve(skb, attrtype, len);
	if (!nla)
		return -EMSGSIZE;

	data = nla_data(nla);
	memcpy(data, tuninfo, len);

	return 0;
}

static int ip6_tnl_fill_info(struct sk_buff *skb, const struct net_device *dev)
{
	struct ip6_tnltap *tunnel = netdev_priv(dev);
	struct __ip6_tnltap_parm *parm = &tunnel->parms;
	struct __ip6_tnl_fmr *c;
	int fmrcnt = 0;
	struct nlattr *fmrs;

	if (nla_put_u32(skb, IFLA_IPTUN_LINK, parm->link) ||
	    nla_put_in6_addr(skb, IFLA_IPTUN_LOCAL, &parm->laddr) ||
	    nla_put_in6_addr(skb, IFLA_IPTUN_REMOTE, &parm->raddr) ||
	    nla_put_u8(skb, IFLA_IPTUN_TTL, parm->hop_limit) ||
	    nla_put_u8(skb, IFLA_IPTUN_ENCAP_LIMIT, parm->encap_limit) ||
	    nla_put_be32(skb, IFLA_IPTUN_FLOWINFO, parm->flowinfo) ||
	    nla_put_u32(skb, IFLA_IPTUN_FLAGS, parm->flags) ||
	    nla_put_u8(skb, IFLA_IPTUN_PROTO, parm->proto) ||
	    nla_put_u32(skb, IFLA_IPTUN_FWMARK, parm->fwmark) ||
	    !(fmrs = nla_nest_start(skb, IFLA_IPTUN_FMRS)))
		goto nla_put_failure;

	for (c = parm->fmrs; c; c = c->next) {
		struct nlattr *fmr = nla_nest_start(skb, ++fmrcnt);
		if (!fmr ||
			nla_put(skb, IFLA_IPTUN_FMR_IP6_PREFIX,
				sizeof(c->rule.ip6_prefix), &c->rule.ip6_prefix) ||
			nla_put(skb, IFLA_IPTUN_FMR_IP4_PREFIX,
				sizeof(c->rule.ip4_prefix), &c->rule.ip4_prefix) ||
			nla_put_u8(skb, IFLA_IPTUN_FMR_IP6_PREFIX_LEN, c->rule.ip6_prefix_len) ||
			nla_put_u8(skb, IFLA_IPTUN_FMR_IP4_PREFIX_LEN, c->rule.ip4_prefix_len) ||
			nla_put_u8(skb, IFLA_IPTUN_FMR_EA_LEN, c->rule.ea_len) ||
			nla_put_u8(skb, IFLA_IPTUN_FMR_OFFSET, c->rule.offset))
				goto nla_put_failure;

		nla_nest_end(skb, fmr);
	}
	nla_nest_end(skb, fmrs);

	if (nla_put_u16(skb, IFLA_IPTUN_ENCAP_TYPE, tunnel->encap.type) ||
	    nla_put_be16(skb, IFLA_IPTUN_ENCAP_SPORT, tunnel->encap.sport) ||
	    nla_put_be16(skb, IFLA_IPTUN_ENCAP_DPORT, tunnel->encap.dport) ||
	    nla_put_u16(skb, IFLA_IPTUN_ENCAP_FLAGS, tunnel->encap.flags))
		goto nla_put_failure;

	if (parm->collect_md)
		if (nla_put_flag(skb, IFLA_IPTUN_COLLECT_METADATA))
			goto nla_put_failure;

	/* push SEG6_IPTUNNEL_SRH attribute into nested IFLA_IPTUN_COLLECT_METADATA */
	if (parm->tuninfo) {
		struct nlattr *tuninfo;

		tuninfo = nla_nest_start(skb, IFLA_IPTUN_COLLECT_METADATA);
		nla_put_srh(skb, SEG6_IPTUNNEL_SRH, parm->tuninfo);
		nla_nest_end(skb, tuninfo);
	}

	return 0;

nla_put_failure:
	return -EMSGSIZE;
}

struct net *ip6_tnltap_get_link_net(const struct net_device *dev)
{
	struct ip6_tnltap *tunnel = netdev_priv(dev);

	return tunnel->net;
}

static const struct nla_policy ip6_tnl_policy[IFLA_IPTUN_MAX + 1] = {
	[IFLA_IPTUN_LINK]		= { .type = NLA_U32 },
	[IFLA_IPTUN_LOCAL]		= { .len = sizeof(struct in6_addr) },
	[IFLA_IPTUN_REMOTE]		= { .len = sizeof(struct in6_addr) },
	[IFLA_IPTUN_TTL]		= { .type = NLA_U8 },
	[IFLA_IPTUN_ENCAP_LIMIT]	= { .type = NLA_U8 },
	[IFLA_IPTUN_FLOWINFO]		= { .type = NLA_U32 },
	[IFLA_IPTUN_FLAGS]		= { .type = NLA_U32 },
	[IFLA_IPTUN_PROTO]		= { .type = NLA_U8 },
	[IFLA_IPTUN_ENCAP_TYPE]		= { .type = NLA_U16 },
	[IFLA_IPTUN_ENCAP_FLAGS]	= { .type = NLA_U16 },
	[IFLA_IPTUN_ENCAP_SPORT]	= { .type = NLA_U16 },
	[IFLA_IPTUN_ENCAP_DPORT]	= { .type = NLA_U16 },
	/*
	[IFLA_IPTUN_COLLECT_METADATA]	= { .type = NLA_FLAG },
	*/
	[IFLA_IPTUN_COLLECT_METADATA]	= { .type = NLA_NESTED },
	[IFLA_IPTUN_FWMARK]		= { .type = NLA_U32 },
	[IFLA_IPTUN_FMRS]		= { .type = NLA_NESTED },
};

static struct rtnl_link_ops ip6_link_ops __read_mostly = {
	.kind		= "ip6tnltap",
	.maxtype	= IFLA_IPTUN_MAX,
	.policy		= ip6_tnl_policy,
	.priv_size	= sizeof(struct ip6_tnltap),
	.setup		= ip6_tnl_dev_setup,
	.validate	= ip6_tnl_validate,
	.newlink	= ip6_tnl_newlink,
	.changelink	= ip6_tnl_changelink,
	.dellink	= ip6_tnl_dellink,
	.get_size	= ip6_tnl_get_size,
	.fill_info	= ip6_tnl_fill_info,
	.get_link_net	= ip6_tnltap_get_link_net,
};

static struct xfrm6_tunnel ethip6_handler __read_mostly = {
	.handler	= ethip6_rcv,
	.err_handler	= ethip6_err,
	.priority	=	1,
};

static void __net_exit ip6_tnl_destroy_tunnels(struct net *net, struct list_head *list)
{
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	struct net_device *dev, *aux;
	int h;
	struct ip6_tnltap *t;

	for_each_netdev_safe(net, dev, aux)
		if (dev->rtnl_link_ops == &ip6_link_ops)
			unregister_netdevice_queue(dev, list);

	for (h = 0; h < IP6_TUNNEL_HASH_SIZE; h++) {
		t = rtnl_dereference(ip6n->tnls_r_l[h]);
		while (t) {
			/* If dev is in the same netns, it has already
			 * been added to the list by the previous loop.
			 */
			if (!net_eq(dev_net(t->dev), net))
				unregister_netdevice_queue(t->dev, list);
			t = rtnl_dereference(t->next);
		}
	}

	t = rtnl_dereference(ip6n->tnls_wc[0]);
	while (t) {
		/* If dev is in the same netns, it has already
		 * been added to the list by the previous loop.
		 */
		if (!net_eq(dev_net(t->dev), net))
			unregister_netdevice_queue(t->dev, list);
		t = rtnl_dereference(t->next);
	}
}

static int __net_init ip6_tnl_init_net(struct net *net)
{
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	struct ip6_tnltap *t = NULL;
	int err;

	ip6n->tnls[0] = ip6n->tnls_wc;
	ip6n->tnls[1] = ip6n->tnls_r_l;

	if (!net_has_fallback_tunnels(net))
		return 0;
	err = -ENOMEM;
	ip6n->fb_tnl_dev = alloc_netdev(sizeof(struct ip6_tnltap), "ip6tnltap0",
					NET_NAME_UNKNOWN, ip6_tnl_dev_setup);

	if (!ip6n->fb_tnl_dev)
		goto err_alloc_dev;
	dev_net_set(ip6n->fb_tnl_dev, net);
	ip6n->fb_tnl_dev->rtnl_link_ops = &ip6_link_ops;
	/* FB netdevice is special: we have one, and only one per netns.
	 * Allowing to move it to another netns is clearly unsafe.
	 */
	ip6n->fb_tnl_dev->features |= NETIF_F_NETNS_LOCAL;

	err = ip6_fb_tnl_dev_init(ip6n->fb_tnl_dev);
	if (err < 0)
		goto err_register;

	err = register_netdev(ip6n->fb_tnl_dev);
	if (err < 0)
		goto err_register;

	t = netdev_priv(ip6n->fb_tnl_dev);

	strcpy(t->parms.name, ip6n->fb_tnl_dev->name);
	return 0;

err_register:
	free_netdev(ip6n->fb_tnl_dev);
err_alloc_dev:
	return err;
}

static void __net_exit ip6_tnl_exit_batch_net(struct list_head *net_list)
{
	struct net *net;
	LIST_HEAD(list);

	rtnl_lock();
	list_for_each_entry(net, net_list, exit_list)
		ip6_tnl_destroy_tunnels(net, &list);
	unregister_netdevice_many(&list);
	rtnl_unlock();
}

static struct pernet_operations ip6_tnl_net_ops = {
	.init = ip6_tnl_init_net,
	.exit_batch = ip6_tnl_exit_batch_net,
	.id   = &ip6_tnl_net_id,
	.size = sizeof(struct ip6_tnl_net),
};

/*
 *	Read ip6tnltap_debug
 */
static int ip6tnltap_debug_proc_read(struct seq_file *sf, void *v)
{
	int bit = 0;

	seq_printf(sf, "Debug mode: 0x%x\n", ip6tnltap_debug);
	for (bit = 0; bit < ARRAY_SIZE(dbg_name); bit++) {
		if (strlen(dbg_name[bit]))
			seq_printf(sf, "[0x%x:%s=%s]\n", (1<<bit), dbg_name[bit], (ip6tnltap_debug&(1<<bit)) ? "\033[1;33mon\033[0m":"off");
	}
	return 0;
}

static int ip6tnltap_debug_single_open(struct inode *inode, struct file *file)
{
	return single_open(file, ip6tnltap_debug_proc_read, NULL);
}

#define DATA_SIZE 64
/*
 *	Write ip6tnltap_debug
 */
static ssize_t ip6tnltap_debug_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
	char buf[DATA_SIZE];
	size_t len;
	long res;

	len = (count >= (DATA_SIZE-1))?(DATA_SIZE-1):count;

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtol(buf, 0, &res))
		return -EINVAL;
	ip6tnltap_debug = (int)res;
	printk("Set debug to 0x%x\n", ip6tnltap_debug);
	return count;
}

static struct proc_ops fops_ip6tnltap_debug = {
	.proc_open		= ip6tnltap_debug_single_open,
	.proc_read		= seq_read,
	.proc_write		= ip6tnltap_debug_proc_write,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};

extern int xfrm6_tunnel_tap_register(struct xfrm6_tunnel *handler, unsigned short family);
extern int xfrm6_tunnel_tap_deregister(struct xfrm6_tunnel *handler, unsigned short family);

/**
 * rtk_ip6_tunnel_tap_init - register protocol and reserve needed resources
 *
 * Return: 0 on success
 **/

static int __init rtk_ip6_tunnel_tap_init(void)
{
	int  err;
	struct proc_dir_entry *pe;

	pr_info("Realtek Ethernet over ipv6 tunnel.\n");
	if (!ipv6_mod_enabled())
		return -EOPNOTSUPP;

	err = register_pernet_device(&ip6_tnl_net_ops);
	if (err < 0)
		goto out_pernet;

	err = xfrm6_tunnel_tap_register(&ethip6_handler, AF_BRIDGE);
	if (err < 0) {
		pr_err("%s: can't register ethip6\n", __func__);
		goto out_ethip6;
	}

	err = rtnl_link_register(&ip6_link_ops);
	if (err < 0)
		goto rtnl_link_failed;

	/* Add /proc/realtek/ip6tnltap_debug */
	pe = proc_create_data("ip6tnltap_debug",
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
			realtek_proc, &fops_ip6tnltap_debug, NULL);
	if (!pe)
		return -EINVAL;

	return 0;

rtnl_link_failed:
	xfrm6_tunnel_tap_deregister(&ethip6_handler, AF_BRIDGE);
out_ethip6:
	unregister_pernet_device(&ip6_tnl_net_ops);
out_pernet:
	return err;
}

/**
 * rtk_ip6_tunnel_tap_cleanup - free resources and unregister protocol
 **/

static void __exit rtk_ip6_tunnel_tap_cleanup(void)
{
	rtnl_link_unregister(&ip6_link_ops);

	if (xfrm6_tunnel_tap_deregister(&ethip6_handler, AF_BRIDGE))
		pr_info("%s: can't deregister ethip6\n", __func__);
	unregister_pernet_device(&ip6_tnl_net_ops);
}

module_init(rtk_ip6_tunnel_tap_init);
module_exit(rtk_ip6_tunnel_tap_cleanup);
