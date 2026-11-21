/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _NET_IP6_TUNNEL_TAP_H
#define _NET_IP6_TUNNEL_TAP_H

struct __ip6_tnltap_parm {
	char name[IFNAMSIZ];	/* name of tunnel device */
	int link;		/* ifindex of underlying L2 interface */
	__u8 proto;		/* tunnel protocol */
	__u8 encap_limit;	/* encapsulation limit for tunnel */
	__u8 hop_limit;		/* hop limit for tunnel */
	bool collect_md;
	__be32 flowinfo;	/* traffic class and flowlabel for tunnel */
	__u32 flags;		/* tunnel flags */
	struct in6_addr laddr;	/* local tunnel end-point address */
	struct in6_addr raddr;	/* remote tunnel end-point address */
	struct __ip6_tnl_fmr *fmrs;	/* FMRs */
	/* Add for SRH */
	struct seg6_iptunnel_encap *tuninfo;

	__be16			i_flags;
	__be16			o_flags;
	__be32			i_key;
	__be32			o_key;

	__u32			fwmark;
	__u32			index;	/* ERSPAN type II index */
	__u8			erspan_ver;	/* ERSPAN version */
	__u8			dir;	/* direction */
	__u16			hwid;	/* hwid */
};

/* IPv6 tunnel tap */
struct ip6_tnltap {
	struct ip6_tnltap __rcu *next;	/* next tunnel in list */
	struct net_device *dev;	/* virtual device associated with tunnel */
	struct net *net;	/* netns for packet i/o */
	struct __ip6_tnltap_parm parms;	/* tunnel configuration parameters */
	struct flowi fl;	/* flowi template for xmit */
	struct dst_cache dst_cache;	/* cached dst */
	struct gro_cells gro_cells;

	int err_count;
	unsigned long err_time;

	/* These fields used only by GRE */
	__u32 i_seqno;	/* The last seen seqno	*/
	__u32 o_seqno;	/* The last output seqno */
	int hlen;       /* tun_hlen + encap_hlen */
	int tun_hlen;	/* Precalculated header length */
	int encap_hlen; /* Encap header length (FOU,GUE) */
	struct ip_tunnel_encap encap;
	int mlink;
};

#ifdef CONFIG_INET

static inline int ip6_tnltap_encap(struct sk_buff *skb, struct ip6_tnltap *t,
				u8 *protocol, struct flowi6 *fl6)
{
	const struct ip6_tnl_encap_ops *ops;
	int ret = -EINVAL;

	if (t->encap.type == TUNNEL_ENCAP_NONE)
		return 0;

	if (t->encap.type >= MAX_IPTUN_ENCAP_OPS)
		return -EINVAL;

	rcu_read_lock();
	ops = rcu_dereference(ip6tun_encaps[t->encap.type]);
	if (likely(ops && ops->build_header))
		ret = ops->build_header(skb, &t->encap, protocol, fl6);
	rcu_read_unlock();

	return ret;
}

int ip6_tnltap_rcv_ctl(struct ip6_tnltap *t, const struct in6_addr *laddr,
		const struct in6_addr *raddr);
int ip6_tnltap_xmit_ctl(struct ip6_tnltap *t, const struct in6_addr *laddr,
		     const struct in6_addr *raddr);
int ip6_tnltap_xmit(struct sk_buff *skb, struct net_device *dev, __u8 dsfield,
		 struct flowi6 *fl6, int encap_limit, __u32 *pmtu, __u8 proto);
__u32 ip6_tnltap_get_cap(struct ip6_tnltap *t, const struct in6_addr *laddr,
			     const struct in6_addr *raddr);
struct net *ip6_tnltap_get_link_net(const struct net_device *dev);
int ip6_tnltap_get_iflink(const struct net_device *dev);
int ip6_tnltap_change_mtu(struct net_device *dev, int new_mtu);

#endif

#endif
