// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C)2003,2004 USAGI/WIDE Project
 *
 * Authors	Mitsuru KANDA  <mk@linux-ipv6.org>
 *		YOSHIFUJI Hideaki <yoshfuji@linux-ipv6.org>
 */

#define pr_fmt(fmt) "IPv6: " fmt

#include <linux/icmpv6.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <net/ipv6.h>
#include <net/protocol.h>
#include <net/xfrm.h>

/* Ethernet over IPv6 (mode=ethip6)*/
static struct xfrm6_tunnel __rcu *tunnele6_handlers __read_mostly;
static DEFINE_MUTEX(tunnel6_mutex);

int xfrm6_tunnel_tap_register(struct xfrm6_tunnel *handler, unsigned short family)
{
	struct xfrm6_tunnel __rcu **pprev;
	struct xfrm6_tunnel *t;
	int ret = -EEXIST;
	int priority = handler->priority;

	mutex_lock(&tunnel6_mutex);

	switch (family) {
	case AF_BRIDGE:
		pprev = &tunnele6_handlers;
		break;
	default:
		goto err;
	}

	for (; (t = rcu_dereference_protected(*pprev,
			lockdep_is_held(&tunnel6_mutex))) != NULL;
	     pprev = &t->next) {
		if (t->priority > priority)
			break;
		if (t->priority == priority)
			goto err;
	}

	handler->next = *pprev;
	rcu_assign_pointer(*pprev, handler);

	ret = 0;

err:
	mutex_unlock(&tunnel6_mutex);

	return ret;
}
EXPORT_SYMBOL(xfrm6_tunnel_tap_register);

int xfrm6_tunnel_tap_deregister(struct xfrm6_tunnel *handler, unsigned short family)
{
	struct xfrm6_tunnel __rcu **pprev;
	struct xfrm6_tunnel *t;
	int ret = -ENOENT;

	mutex_lock(&tunnel6_mutex);

	switch (family) {
	case AF_BRIDGE:
		pprev = &tunnele6_handlers;
		break;
	default:
		goto err;
	}

	for (; (t = rcu_dereference_protected(*pprev,
			lockdep_is_held(&tunnel6_mutex))) != NULL;
	     pprev = &t->next) {
		if (t == handler) {
			*pprev = handler->next;
			ret = 0;
			break;
		}
	}

err:
	mutex_unlock(&tunnel6_mutex);

	synchronize_net();

	return ret;
}
EXPORT_SYMBOL(xfrm6_tunnel_tap_deregister);

#define for_each_tunnel_rcu(head, handler)		\
	for (handler = rcu_dereference(head);		\
	     handler != NULL;				\
	     handler = rcu_dereference(handler->next))	\

static int tunnele6_rcv(struct sk_buff *skb)
{
	struct xfrm6_tunnel *handler;

	if (!pskb_may_pull(skb, sizeof(struct ethhdr)))
		goto drop;

	for_each_tunnel_rcu(tunnele6_handlers, handler)
		if (!handler->handler(skb))
			return 0;

	icmpv6_send(skb, ICMPV6_DEST_UNREACH, ICMPV6_PORT_UNREACH, 0);

drop:
	kfree_skb(skb);
	return 0;
}

static int tunnele6_err(struct sk_buff *skb, struct inet6_skb_parm *opt,
			 u8 type, u8 code, int offset, __be32 info)
{
	struct xfrm6_tunnel *handler;

	for_each_tunnel_rcu(tunnele6_handlers, handler)
		if (!handler->err_handler(skb, opt, type, code, offset, info))
			return 0;

	return -ENOENT;
}

static const struct inet6_protocol tunnele6_protocol = {
	.handler	= tunnele6_rcv,
	.err_handler	= tunnele6_err,
	.flags          = INET6_PROTO_NOPOLICY|INET6_PROTO_FINAL,
};

static int __init rtk_tunnel6_tap_init(void)
{
	if (inet6_add_protocol(&tunnele6_protocol, IPPROTO_ETHERNET)) {
		pr_err("%s: can't add protocol\n", __func__);
		return -EAGAIN;
	}
	return 0;
}

static void __exit rtk_tunnel6_tap_fini(void)
{
	if (inet6_del_protocol(&tunnele6_protocol, IPPROTO_ETHERNET))
		pr_err("%s: can't remove protocol\n", __func__);
}

module_init(rtk_tunnel6_tap_init);
module_exit(rtk_tunnel6_tap_fini);
MODULE_LICENSE("GPL");
