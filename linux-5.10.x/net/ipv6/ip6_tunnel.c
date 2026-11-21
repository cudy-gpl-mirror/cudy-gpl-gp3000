// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *	IPv6 tunneling device
 *	Linux INET6 implementation
 *
 *	Authors:
 *	Ville Nuorvala		<vnuorval@tcs.hut.fi>
 *	Yasuyuki Kozakai	<kozakai@linux-ipv6.org>
 *
 *      Based on:
 *      linux/net/ipv6/sit.c and linux/net/ipv4/ipip.c
 *
 *      RFC 2473
 *
 *      Changes:
 *      Steven Barth <cyrus@openwrt.org>:           MAP-E FMR support
 */

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

#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
#include <net/netfilter/nf_conntrack.h>
#endif

MODULE_AUTHOR("Ville Nuorvala");
MODULE_DESCRIPTION("IPv6 tunneling device");
MODULE_LICENSE("GPL");
MODULE_ALIAS_RTNL_LINK("ip6tnl");
MODULE_ALIAS_NETDEV("ip6tnl0");

#define IP6_TUNNEL_HASH_SIZE_SHIFT  5
#define IP6_TUNNEL_HASH_SIZE (1 << IP6_TUNNEL_HASH_SIZE_SHIFT)

static bool log_ecn_error = true;
module_param(log_ecn_error, bool, 0644);
MODULE_PARM_DESC(log_ecn_error, "Log packets received with corrupted ECN");

#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
static int mape_portset_init(struct mape_psid_info *psid_info, struct ip6_tnl *t);
#endif

#if 1//RTK
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/inet.h>

struct proc_dir_entry *proc_ip6tnl_fmr = NULL;
static int ip6tnl_fmr_proc_init(struct net *net)
{
	printk("[%s:%d] Create /proc/net/ip6_tunnel_fmr\n",__FUNCTION__,__LINE__);
	proc_ip6tnl_fmr = proc_mkdir("ip6_tunnel_fmr", net->proc_net);
	if (!proc_ip6tnl_fmr)
	{
		printk("[%s:%d] Error: Can't create /proc/net/ip6_tunnel_fmr proc\n",__FUNCTION__,__LINE__);
		return (-1);
	}
	return (0);
}

static int ip6tnl_fmr_proc_exit(struct net *net)
{
	printk("[%s:%d] Remove /proc/net/ip6_tunnel_fmr\n",__FUNCTION__,__LINE__);
	remove_proc_entry("ip6_tunnel_fmr", net->proc_net);
	return (0);
}

static int ip6tnl_fmr_rfc_ver_set(struct ip6_tnl* t, char* cmd)
{
	char *ptr = cmd + strlen("rfc");

	while (*ptr == ' ' || *ptr == ':')
		ptr++;
	if (!ptr) return -EINVAL;

	if (strncasecmp(ptr, "rfc7597", strlen("rfc7597")) == 0)
		t->parms.mape_rfc7597 = 1; //rfc7597
	else
		t->parms.mape_rfc7597 = 0; //draft03

	return (0);
}

static char *ip6tnl_parse_strings(char *tag, char *pstr)
{
	char *str  = NULL;
	char *val   = NULL;
	char *delim = 0;

	str = strstr(pstr, tag);
	if (str) {
		str = strchr(str, '=');
		if (!str)
			return NULL;
		val = str + 1;
		delim = strchr(str, ',');
		if (delim)
			*delim = '\0';
		return val;
	}
	return NULL;
}

//echo 'fmr ip6_prefix=%s/%d,ip4_prefix=%s/%d,ea_len=%u,offset=%u' > /proc/net/ip6_tunnel_fmr/%s
static int ip6tnl_fmr_add_rule(struct ip6_tnl *t, char *cmd)
{
	struct __ip6_tnl_fmr *fmr = NULL;
	char *ip4_prefix     = NULL,
	     *ip6_prefix     = NULL,
	     *ip4_prefix_len = NULL,
	     *ip6_prefix_len = NULL,
	     *ea_len         = NULL,
	     *offset         = NULL;

	char *p;
	char *tag = "";
	int  ret;
	unsigned long kval;
	char cmd_buf[256] = {0};
	int len = (sizeof(cmd_buf) > strlen(cmd)) ? strlen(cmd) : sizeof(cmd_buf);

	fmr = kzalloc(sizeof(*fmr), GFP_KERNEL);
	if (!fmr)
		return (-ENOMEM);

	// command parse start
	// 1st - ip6_prefix/ip6_prefix_len
	memcpy(cmd_buf, cmd, len);
	tag = "ip6_prefix";

	ip6_prefix = ip6tnl_parse_strings(tag, cmd_buf);
	if (!ip6_prefix) {
		ret = -EINVAL;
		goto err;
	}
	p = strchr(ip6_prefix, '/');
	if (p) {
		*p = 0;
		p++;
		ip6_prefix_len = p;
	}
	ret = in6_pton(ip6_prefix, -1, (u8 *)&fmr->rule.ip6_prefix, -1, NULL);
	if (ret == 0) {
		ret = -EINVAL;
		goto err;
	}
	tag = "ip6_prefix_len";
	if (kstrtol(ip6_prefix_len, 10, &kval) != 0 || kval < 1 || kval > 128) {
		ret = -EINVAL;
		goto err;
	}
	fmr->rule.ip6_prefix_len = (__u8)kval;

	// 2nd - ip4_prefix/ip4_prefix_len,
	memcpy(cmd_buf, cmd, len);
	tag = "ip4_prefix";
	ip4_prefix = ip6tnl_parse_strings(tag, cmd_buf);
	if (!ip4_prefix) {
		ret = -EINVAL;
		goto err;
	}
	p = strchr(ip4_prefix, '/');
	if (p) {
		*p = 0;
		p++;
		ip4_prefix_len = p;
	}
	ret = in4_pton(ip4_prefix, -1, (u8 *)&fmr->rule.ip4_prefix, -1, NULL);
	if (ret == 0) {
		ret = -EINVAL;
		goto err;
	}
	tag = "ip4_prefix_len";
	if (kstrtol(ip4_prefix_len, 10, &kval) != 0 || kval < 1 || kval > 32) {
		ret = -EINVAL;
		goto err;
	}
	fmr->rule.ip4_prefix_len = (__u8)kval;

	// 3rd - ea_length
	memcpy(cmd_buf, cmd, len);
	tag = "ea_len";
	ea_len = ip6tnl_parse_strings(tag, cmd_buf);
	if (!ea_len) {
		ret = -EINVAL;
		goto err;
	}
	if (kstrtol(ea_len, 10, &kval) != 0 || kval > 64) {
		ret = -EINVAL;
		goto err;
	}
	fmr->rule.ea_len = (__u8)kval;

	// 4th - offset
	memcpy(cmd_buf, cmd, len);
	tag = "offset";
	offset = ip6tnl_parse_strings(tag, cmd_buf);
	if (!offset) {
		ret = -EINVAL;
		goto err;
	}
	if (kstrtol(offset, 10, &kval) != 0 || kval > 64) {
		ret = -EINVAL;
		goto err;
	}
	fmr->rule.offset = (__u8)kval;

	rcu_read_lock();
	fmr->next = t->parms.fmrs;
	t->parms.fmrs = fmr;
	rcu_read_unlock();
	return 0;
err:
	pr_err("[%s:%d] Error: invalid %s\n", __func__, __LINE__, tag);
	kfree(fmr);
	return ret;
}

static ssize_t ip6tnl_fmr_seq_write(struct file* file, const char __user * buf,
	size_t count, loff_t *ppos)
{
	struct net_device* dev = (struct net_device*)PDE_DATA(file_inode(file));
	struct ip6_tnl* t;
	char cmdbuf[256];
	int ret = 0;

	if ((t = netdev_priv(dev)) == NULL) return (-EIO);
	if (count >= sizeof(cmdbuf)) return (-EINVAL);
	if (copy_from_user(cmdbuf, buf, count)) return (-EFAULT);

	cmdbuf[count] = 0;
	if (strncasecmp(cmdbuf, "rfc", strlen("rfc")) == 0)
		ret = ip6tnl_fmr_rfc_ver_set(t, cmdbuf);
	else if (strncasecmp(cmdbuf, "fmr", strlen("fmr")) == 0)
		ret = ip6tnl_fmr_add_rule(t, cmdbuf + strlen("fmr"));
	if(ret != 0)
		return (ret);
	return (count);
}

static int ip6tnl_fmr_seq_show(struct seq_file *m, void *v)
{
	struct net_device* dev = (struct net_device*)m->private;
	struct __ip6_tnl_fmr *fmr;
	struct ip6_tnl* t;
	u32 fmr_cnt = 0;

	if (!dev) return (-EIO);
	if ((t = netdev_priv(dev)) == NULL)
		return (-EIO);

	seq_printf(m, "[TUN] %s %s\n", dev->name, (t->parms.mape_rfc7597) ? "rfc7597" : "draft03");

	if (t->parms.fmrs) {
		seq_puts(m, "[FMR] ip6_prefix/len ip4_prefix/len ea_len offset\n");
		for (fmr = t->parms.fmrs; fmr; fmr = fmr->next) {
			seq_printf(m, "[%u]: %pI6/%u %pI4/%u %u %u\n", fmr_cnt,
				   &fmr->rule.ip6_prefix, fmr->rule.ip6_prefix_len,
				   &fmr->rule.ip4_prefix, fmr->rule.ip4_prefix_len,
				   fmr->rule.ea_len, fmr->rule.offset);
			fmr_cnt++;
		}
	} else {
		seq_puts(m, "[FMR] No FMR rules\n");
	}
	return (0);
}

static int ip6tnl_fmr_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, ip6tnl_fmr_seq_show, PDE_DATA(inode));
}

struct proc_ops ip6tnl_fmr_seq_fops= {
	.proc_open           = ip6tnl_fmr_seq_open,
	.proc_write          = ip6tnl_fmr_seq_write,
	.proc_read           = seq_read,
	.proc_lseek         = seq_lseek,
	.proc_release        = single_release,
};

static int ip6tnl_create_fmr_proc(struct net_device *dev)
{
	struct ip6_tnl *t;
	struct proc_dir_entry *p;

	if (!proc_ip6tnl_fmr || !dev)
		return -EINVAL;

	t = netdev_priv(dev);
	if (!t) return -EINVAL;

	t->parms.mape_fmr_proc = NULL;
	p = proc_create_data(dev->name, S_IRUGO | S_IWUGO, proc_ip6tnl_fmr, &ip6tnl_fmr_seq_fops, dev);
	if (!p) return -ENOMEM;

	t->parms.mape_fmr_proc = p;
	return (0);
}

static int ip6tnl_destroy_fmr_proc(struct net_device *dev)
{
	struct ip6_tnl *t;
	if (!proc_ip6tnl_fmr || !dev) return -EINVAL;

	t = netdev_priv(dev);
	if (!t) return -EINVAL;

	if (t->parms.mape_fmr_proc)
		proc_remove(t->parms.mape_fmr_proc);
	t->parms.mape_fmr_proc = NULL;

	return (0);
}
#endif

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
	struct ip6_tnl __rcu *tnls_r_l[IP6_TUNNEL_HASH_SIZE];
	struct ip6_tnl __rcu *tnls_wc[1];
	struct ip6_tnl __rcu **tnls[2];
	struct ip6_tnl __rcu *collect_md_tun;
};

static inline int ip6_tnl_mpls_supported(void)
{
	return IS_ENABLED(CONFIG_MPLS);
}

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

static struct ip6_tnl *
ip6_tnl_lookup(struct net *net, int link,
	       const struct in6_addr *remote, const struct in6_addr *local)
{
	unsigned int hash = HASH(local);
	struct ip6_tnl *t, *cand = NULL;
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

static struct ip6_tnl __rcu **
ip6_tnl_bucket(struct ip6_tnl_net *ip6n, const struct __ip6_tnl_parm *p)
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
ip6_tnl_link(struct ip6_tnl_net *ip6n, struct ip6_tnl *t)
{
	struct ip6_tnl __rcu **tp = ip6_tnl_bucket(ip6n, &t->parms);

	if (t->parms.collect_md)
		rcu_assign_pointer(ip6n->collect_md_tun, t);
	rcu_assign_pointer(t->next , rtnl_dereference(*tp));
	rcu_assign_pointer(*tp, t);
}

/**
 * ip6_tnl_unlink - remove tunnel from hash table
 *   @t: tunnel to be removed
 **/

static void
ip6_tnl_unlink(struct ip6_tnl_net *ip6n, struct ip6_tnl *t)
{
	struct ip6_tnl __rcu **tp;
	struct ip6_tnl *iter;

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
	struct ip6_tnl *t = netdev_priv(dev);

	gro_cells_destroy(&t->gro_cells);
	dst_cache_destroy(&t->dst_cache);
	free_percpu(dev->tstats);
}

static int ip6_tnl_create2(struct net_device *dev)
{
	struct ip6_tnl *t = netdev_priv(dev);
	struct net *net = dev_net(dev);
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	int err;

	t = netdev_priv(dev);

	dev->rtnl_link_ops = &ip6_link_ops;
	err = register_netdevice(dev);
	if (err < 0)
		goto out;

	strcpy(t->parms.name, dev->name);

	ip6_tnl_link(ip6n, t);

#if 1//RTK
	ip6tnl_create_fmr_proc(dev);
#endif

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

static struct ip6_tnl *ip6_tnl_create(struct net *net, struct __ip6_tnl_parm *p)
{
	struct net_device *dev;
	struct ip6_tnl *t;
	char name[IFNAMSIZ];
	int err = -E2BIG;

	if (p->name[0]) {
		if (!dev_valid_name(p->name))
			goto failed;
		strlcpy(name, p->name, IFNAMSIZ);
	} else {
		sprintf(name, "ip6tnl%%d");
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

static struct ip6_tnl *ip6_tnl_locate(struct net *net,
		struct __ip6_tnl_parm *p, int create)
{
	const struct in6_addr *remote = &p->raddr;
	const struct in6_addr *local = &p->laddr;
	struct ip6_tnl __rcu **tp;
	struct ip6_tnl *t;
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
	struct ip6_tnl *t = netdev_priv(dev);
	struct net *net = t->net;
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);

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

/**
 * parse_tvl_tnl_enc_lim - handle encapsulation limit option
 *   @skb: received socket buffer
 *
 * Return:
 *   0 if none was found,
 *   else index to encapsulation limit
 **/

__u16 ip6_tnl_parse_tlv_enc_lim(struct sk_buff *skb, __u8 *raw)
{
	const struct ipv6hdr *ipv6h = (const struct ipv6hdr *)raw;
	unsigned int nhoff = raw - skb->data;
	unsigned int off = nhoff + sizeof(*ipv6h);
	u8 nexthdr = ipv6h->nexthdr;

	while (ipv6_ext_hdr(nexthdr) && nexthdr != NEXTHDR_NONE) {
		struct ipv6_opt_hdr *hdr;
		u16 optlen;

		if (!pskb_may_pull(skb, off + sizeof(*hdr)))
			break;

		hdr = (struct ipv6_opt_hdr *)(skb->data + off);
		if (nexthdr == NEXTHDR_FRAGMENT) {
			optlen = 8;
		} else if (nexthdr == NEXTHDR_AUTH) {
			optlen = ipv6_authlen(hdr);
		} else {
			optlen = ipv6_optlen(hdr);
		}

		if (!pskb_may_pull(skb, off + optlen))
			break;

		hdr = (struct ipv6_opt_hdr *)(skb->data + off);
		if (nexthdr == NEXTHDR_FRAGMENT) {
			struct frag_hdr *frag_hdr = (struct frag_hdr *)hdr;

			if (frag_hdr->frag_off)
				break;
		}
		if (nexthdr == NEXTHDR_DEST) {
			u16 i = 2;

			while (1) {
				struct ipv6_tlv_tnl_enc_lim *tel;

				/* No more room for encapsulation limit */
				if (i + sizeof(*tel) > optlen)
					break;

				tel = (struct ipv6_tlv_tnl_enc_lim *)(skb->data + off + i);
				/* return index of option if found and valid */
				if (tel->type == IPV6_TLV_TNL_ENCAP_LIMIT &&
				    tel->length == 1)
					return i + off - nhoff;
				/* else jump to next option */
				if (tel->type)
					i += tel->length + 2;
				else
					i++;
			}
		}
		nexthdr = hdr->nexthdr;
		off += optlen;
	}
	return 0;
}
EXPORT_SYMBOL(ip6_tnl_parse_tlv_enc_lim);

/**
 * ip6_tnl_err - tunnel error handler
 *
 * Description:
 *   ip6_tnl_err() should handle errors in the tunnel according
 *   to the specifications in RFC 2473.
 **/

static int
ip6_tnl_err(struct sk_buff *skb, __u8 ipproto, struct inet6_skb_parm *opt,
	    u8 *type, u8 *code, int *msg, __u32 *info, int offset)
{
	const struct ipv6hdr *ipv6h = (const struct ipv6hdr *)skb->data;
	struct net *net = dev_net(skb->dev);
	u8 rel_type = ICMPV6_DEST_UNREACH;
	u8 rel_code = ICMPV6_ADDR_UNREACH;
	__u32 rel_info = 0;
	struct ip6_tnl *t;
	int err = -ENOENT;
	int rel_msg = 0;
	u8 tproto;
	__u16 len;

	/* If the packet doesn't contain the original IPv6 header we are
	   in trouble since we might need the source address for further
	   processing of the error. */

	rcu_read_lock();
	t = ip6_tnl_lookup(dev_net(skb->dev), skb->dev->ifindex, &ipv6h->daddr, &ipv6h->saddr);
	if (!t)
		goto out;

	tproto = READ_ONCE(t->parms.proto);
	if (tproto != ipproto && tproto != 0)
		goto out;

	err = 0;

	switch (*type) {
	case ICMPV6_DEST_UNREACH:
		net_dbg_ratelimited("%s: Path to destination invalid or inactive!\n",
				    t->parms.name);
		rel_msg = 1;
		break;
	case ICMPV6_TIME_EXCEED:
		if ((*code) == ICMPV6_EXC_HOPLIMIT) {
			net_dbg_ratelimited("%s: Too small hop limit or routing loop in tunnel!\n",
					    t->parms.name);
			rel_msg = 1;
		}
		break;
	case ICMPV6_PARAMPROB: {
		struct ipv6_tlv_tnl_enc_lim *tel;
		__u32 teli;

		teli = 0;
		if ((*code) == ICMPV6_HDR_FIELD)
			teli = ip6_tnl_parse_tlv_enc_lim(skb, skb->data);

		if (teli && teli == *info - 2) {
			tel = (struct ipv6_tlv_tnl_enc_lim *) &skb->data[teli];
			if (tel->encap_limit == 0) {
				net_dbg_ratelimited("%s: Too small encapsulation limit or routing loop in tunnel!\n",
						    t->parms.name);
				rel_msg = 1;
			}
		} else {
			net_dbg_ratelimited("%s: Recipient unable to parse tunneled packet!\n",
					    t->parms.name);
		}
		break;
	}
	case ICMPV6_PKT_TOOBIG: {
		__u32 mtu;

		ip6_update_pmtu(skb, net, htonl(*info), 0, 0,
				sock_net_uid(net, NULL));
		mtu = *info - offset;
		if (mtu < IPV6_MIN_MTU)
			mtu = IPV6_MIN_MTU;
		len = sizeof(*ipv6h) + ntohs(ipv6h->payload_len);
		if (len > mtu) {
			rel_type = ICMPV6_PKT_TOOBIG;
			rel_code = 0;
			rel_info = mtu;
			rel_msg = 1;
		}
		break;
	}
	case NDISC_REDIRECT:
		ip6_redirect(skb, net, skb->dev->ifindex, 0,
			     sock_net_uid(net, NULL));
		break;
	}

	*type = rel_type;
	*code = rel_code;
	*info = rel_info;
	*msg = rel_msg;

out:
	rcu_read_unlock();
	return err;
}

static int
ip4ip6_err(struct sk_buff *skb, struct inet6_skb_parm *opt,
	   u8 type, u8 code, int offset, __be32 info)
{
	__u32 rel_info = ntohl(info);
	const struct iphdr *eiph;
	struct sk_buff *skb2;
	int err, rel_msg = 0;
	u8 rel_type = type;
	u8 rel_code = code;
	struct rtable *rt;
	struct flowi4 fl4;

	err = ip6_tnl_err(skb, IPPROTO_IPIP, opt, &rel_type, &rel_code,
			  &rel_msg, &rel_info, offset);
	if (err < 0)
		return err;

	if (rel_msg == 0)
		return 0;

	switch (rel_type) {
	case ICMPV6_DEST_UNREACH:
		if (rel_code != ICMPV6_ADDR_UNREACH)
			return 0;
		rel_type = ICMP_DEST_UNREACH;
		rel_code = ICMP_HOST_UNREACH;
		break;
	case ICMPV6_PKT_TOOBIG:
		if (rel_code != 0)
			return 0;
		rel_type = ICMP_DEST_UNREACH;
		rel_code = ICMP_FRAG_NEEDED;
		break;
	default:
		return 0;
	}

	if (!pskb_may_pull(skb, offset + sizeof(struct iphdr)))
		return 0;

	skb2 = skb_clone(skb, GFP_ATOMIC);
	if (!skb2)
		return 0;

	skb_dst_drop(skb2);

	skb_pull(skb2, offset);
	skb_reset_network_header(skb2);
	eiph = ip_hdr(skb2);

	/* Try to guess incoming interface */
	rt = ip_route_output_ports(dev_net(skb->dev), &fl4, NULL, eiph->saddr,
				   0, 0, 0, IPPROTO_IPIP, RT_TOS(eiph->tos), 0);
	if (IS_ERR(rt))
		goto out;

	skb2->dev = rt->dst.dev;
	ip_rt_put(rt);

	/* route "incoming" packet */
	if (rt->rt_flags & RTCF_LOCAL) {
		rt = ip_route_output_ports(dev_net(skb->dev), &fl4, NULL,
					   eiph->daddr, eiph->saddr, 0, 0,
					   IPPROTO_IPIP, RT_TOS(eiph->tos), 0);
		if (IS_ERR(rt) || rt->dst.dev->type != ARPHRD_TUNNEL6) {
			if (!IS_ERR(rt))
				ip_rt_put(rt);
			goto out;
		}
		skb_dst_set(skb2, &rt->dst);
	} else {
		if (ip_route_input(skb2, eiph->daddr, eiph->saddr, eiph->tos,
				   skb2->dev) ||
		    skb_dst(skb2)->dev->type != ARPHRD_TUNNEL6)
			goto out;
	}

	/* change mtu on this route */
	if (rel_type == ICMP_DEST_UNREACH && rel_code == ICMP_FRAG_NEEDED) {
		if (rel_info > dst_mtu(skb_dst(skb2)))
			goto out;

		skb_dst_update_pmtu_no_confirm(skb2, rel_info);
	}

	icmp_send(skb2, rel_type, rel_code, htonl(rel_info));

out:
	kfree_skb(skb2);
	return 0;
}

static int
ip6ip6_err(struct sk_buff *skb, struct inet6_skb_parm *opt,
	   u8 type, u8 code, int offset, __be32 info)
{
	__u32 rel_info = ntohl(info);
	int err, rel_msg = 0;
	u8 rel_type = type;
	u8 rel_code = code;

	err = ip6_tnl_err(skb, IPPROTO_IPV6, opt, &rel_type, &rel_code,
			  &rel_msg, &rel_info, offset);
	if (err < 0)
		return err;

	if (rel_msg && pskb_may_pull(skb, offset + sizeof(struct ipv6hdr))) {
		struct rt6_info *rt;
		struct sk_buff *skb2 = skb_clone(skb, GFP_ATOMIC);

		if (!skb2)
			return 0;

		skb_dst_drop(skb2);
		skb_pull(skb2, offset);
		skb_reset_network_header(skb2);

		/* Try to guess incoming interface */
		rt = rt6_lookup(dev_net(skb->dev), &ipv6_hdr(skb2)->saddr,
				NULL, 0, skb2, 0);

		if (rt && rt->dst.dev)
			skb2->dev = rt->dst.dev;

		icmpv6_send(skb2, rel_type, rel_code, rel_info);

		ip6_rt_put(rt);

		kfree_skb(skb2);
	}

	return 0;
}

static int
mplsip6_err(struct sk_buff *skb, struct inet6_skb_parm *opt,
	    u8 type, u8 code, int offset, __be32 info)
{
	__u32 rel_info = ntohl(info);
	int err, rel_msg = 0;
	u8 rel_type = type;
	u8 rel_code = code;

	err = ip6_tnl_err(skb, IPPROTO_MPLS, opt, &rel_type, &rel_code,
			  &rel_msg, &rel_info, offset);
	return err;
}

static int ip4ip6_dscp_ecn_decapsulate(const struct ip6_tnl *t,
				       const struct ipv6hdr *ipv6h,
				       struct sk_buff *skb)
{
	__u8 dsfield = ipv6_get_dsfield(ipv6h) & ~INET_ECN_MASK;

	if (t->parms.flags & IP6_TNL_F_RCV_DSCP_COPY)
		ipv4_change_dsfield(ip_hdr(skb), INET_ECN_MASK, dsfield);

	return IP6_ECN_decapsulate(ipv6h, skb);
}

static int ip6ip6_dscp_ecn_decapsulate(const struct ip6_tnl *t,
				       const struct ipv6hdr *ipv6h,
				       struct sk_buff *skb)
{
	if (t->parms.flags & IP6_TNL_F_RCV_DSCP_COPY)
		ipv6_copy_dscp(ipv6_get_dsfield(ipv6h), ipv6_hdr(skb));

	return IP6_ECN_decapsulate(ipv6h, skb);
}

static inline int mplsip6_dscp_ecn_decapsulate(const struct ip6_tnl *t,
					       const struct ipv6hdr *ipv6h,
					       struct sk_buff *skb)
{
	/* ECN is not supported in AF_MPLS */
	return 0;
}

__u32 ip6_tnl_get_cap(struct ip6_tnl *t,
			     const struct in6_addr *laddr,
			     const struct in6_addr *raddr)
{
	struct __ip6_tnl_parm *p = &t->parms;
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
EXPORT_SYMBOL(ip6_tnl_get_cap);

/* called with rcu_read_lock() */
int ip6_tnl_rcv_ctl(struct ip6_tnl *t,
				  const struct in6_addr *laddr,
				  const struct in6_addr *raddr)
{
	struct __ip6_tnl_parm *p = &t->parms;
	int ret = 0;
	struct net *net = t->net;

	if ((p->flags & IP6_TNL_F_CAP_RCV) ||
	    ((p->flags & IP6_TNL_F_CAP_PER_PACKET) &&
	     (ip6_tnl_get_cap(t, laddr, raddr) & IP6_TNL_F_CAP_RCV))) {
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
EXPORT_SYMBOL_GPL(ip6_tnl_rcv_ctl);


#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
/**	
 *					 0                   1
 *                 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 *                 +-----------+-----------+-------+
 *  Ports in       |     A     |    PSID   |   j   |
 *the CE port set  |    > 0    |           |       |
 *                 +-----------+-----------+-------+
 *                 |  a bits   |  k bits   |m bits |
                 
 * a bits:  The number of offset bits -- 6 by default, as this excludes the system ports (0-1023).
 *
 * A:  Selects the range of the port number. For 'a' > 0, A MUST be larger than 0. 
 		This ensures that the algorithm excludes the system ports.
		For the default value of 'a' (6), the system ports are excluded by requiring that A be greater than 0. 
		Smaller values of 'a' exclude a larger initial range, e.g., 'a' = 4 will exclude ports 0-4095. 
 *
 * k bits:  The length in bits of the PSID field.
 *
 * PSID:  The Port Set Identifier (PSID).
 *
 * m bits:  The number of contiguous ports is given by 2^m.
 *
 * j:  Selects the specific port within a particular range specified by the concatenation of A and the PSID.
 **/

#define MAPE_PORT_BIT_CLEAR(val, mask) (__u16)(val&((__u16)(~mask)))
#define MAPE_PORT_BIT_SET(val, mask) (__u16)(val|mask)
#define MAPE_PORT_BIT_GET(val, mask)	(__u16)(val&mask)

#define MAPE_PORT_A_PART_MASK	(__u16)(0xFFFF<<(k+m))
#define MAPE_PORT_PSID_PART_MASK	(__u16)((__u16)(0xFFFF<<(16-k))>>a)
#define MAPE_PORT_j_PART_MASK	(__u16)((__u16)(0xFFFF<<(a+k))>>(a+k))

#define MAPE_PORT_A_PART_GET(val)	MAPE_PORT_BIT_GET(val, MAPE_PORT_A_PART_MASK)
#define MAPE_PORT_PSID_PART_GET(val) MAPE_PORT_BIT_GET(val, MAPE_PORT_PSID_PART_MASK)
#define MAPE_PORT_j_PART_GET(val)	MAPE_PORT_BIT_GET(val, MAPE_PORT_j_PART_MASK)

#define MAPE_PORT_A_VAL_GET(val)	MAPE_PORT_A_PART_GET(val)>>(k+m)
#define MAPE_PORT_j_VAL_GET(val)	MAPE_PORT_j_PART_GET(val)

#define MAPE_PORT_BIT_LEFT_SHIFT(x,y)	(__u16)((__u16)x<<(y))	

#define MAPE_PORT_A_VAL_MAX	MAPE_PORT_BIT_LEFT_SHIFT(1,a)
#define MAPE_PORT_j_VAL_MAX	MAPE_PORT_BIT_LEFT_SHIFT(1,m)

#define MAPE_PORT_A_PART_MIN	MAPE_PORT_BIT_LEFT_SHIFT(1,(k+m))
#define MAPE_PORT_A_VAL_INCREASE(val)	(__u16)((__u16)(MAPE_PORT_A_VAL_GET(val))+1)
#define MAPE_PORT_A_PART_INCREASE(val)	(__u16)((__u16)(MAPE_PORT_A_VAL_INCREASE(val))<<(k+m))

#define MAPE_PORT_PSID_HEX_CONVERT(val) (__u16)((__u16)(val<<(16-k))>>a)

#define MAPE_PORT_HIGHEST_BIT_SET 0x8000

#define MAPE_PORT_DIV(a,b) (__u16)(a/b)
#define MAPE_PORT_MOD(a,b) (__u16)(a%b)

static void mape_portset_field_len_convert(struct __ip6_tnl_psid *mape_psid, __u8 *a, __u8 *k, __u8 *m){
	*a = mape_psid->psid_info.psid_offset;
	*k = mape_psid->psid_info.psid_len;
	*m = 16-(*a)-(*k);
}

static int mape_portset_init(struct mape_psid_info *psid_info, struct ip6_tnl *t){
	struct __ip6_tnl_psid *mape_psid=NULL;

	if (!psid_info || !t)
		return 0;

	if ((psid_info->psid_offset+psid_info->psid_len) > 16){
		printk("[%s:%d] psid info invalid!\n",__FUNCTION__,__LINE__);
		return 0;
	}

	mape_psid = &(t->parms.mape_psid);
	memcpy(&(mape_psid->psid_info), psid_info, sizeof(struct mape_psid_info));
	mape_psid->kernel_ipid = 0;
	mape_psid->fc_ipid = 0;

	if (mape_psid->set == 0){
		rwlock_init(&mape_psid->portset_id_lock);
		mape_psid->set = 1;
	}
	return 1;
}

/**
 * mape_tun_dev_get_by_name - Get the tunnel device by the tunnel name.
 *	@tun_name: the tunnel device name
 *	@tnl_dev: the tunnel net_device
 *	@tnl_t: the tunnel device
 **/
static bool mape_tun_dev_get_by_name(char *tun_name, struct net_device **tnl_dev, struct ip6_tnl **tnl_t){
	struct net_device *dev;
	struct ip6_tnl *t;
	
	dev = dev_get_by_name(&init_net, tun_name); 
	if (!dev){
		goto failed;
	}

	t = netdev_priv(dev);
	if (IS_ERR(t) || (!t->parms.mape_psid.set)){
		goto failed;
	}

	*tnl_dev = dev;
	*tnl_t = t;
	return true;
	
failed:
	if (dev)
		dev_put(dev);
	return false;
}

bool is_mape_session(void *nf_ct){
	struct nf_conn *ct = (struct nf_conn *)nf_ct;
	if (!ct || !(ct->out_mape_tunnel_name[0]))
		return false;

	return true;
}
EXPORT_SYMBOL(is_mape_session);

static bool mape_is_port_in_portset_range(char *tun_name, __u16 port){
	struct net_device *dev=NULL;
	struct ip6_tnl *t;
	struct __ip6_tnl_psid *mape_psid;
	__u8 a, k, m;
	__u16 PSID_hex,PSID_mask;
	bool ret=false;

	if (mape_tun_dev_get_by_name(tun_name, &dev, &t) == false)
		return false;

	if (t->parms.mape_psid.set == 0){
		ret = false;
		goto ret_func;
	}
	
	mape_psid = &(t->parms.mape_psid);
	mape_portset_field_len_convert(mape_psid, &a, &k, &m);
	PSID_hex = MAPE_PORT_PSID_HEX_CONVERT(mape_psid->psid_info.psid); //0x0340
	PSID_mask = MAPE_PORT_PSID_PART_MASK; //0x0FF0
	
	if ((port&PSID_mask) == PSID_hex)
		ret = true;
	else
		ret = false;

ret_func:
	if (dev)
		dev_put(dev);
	return ret;
}


/**
 *	mape_portset_get_nearest_port - Get the smallest port in port-set larger than the sport
 *
 * Return:
 *   1) If the sport has already been in the port-set range, return the sport itself
 		eg: 0x2340 ->0x2340
 *   2) otherwise, return the smallest port in port-set larger than the sport
 		eg: 0x0221->0x1340, 0x1221->0x1340, 0x1451->0x2340, 0xF331->0xF340, 0xF450 ->0x1340
 **/
static __u16 mape_portset_get_nearest_port(char *tun_name, int sport){
	struct net_device *dev=NULL;
	struct ip6_tnl *t;
	struct __ip6_tnl_psid *mape_psid;
	__u8 a, k, m;
	__u16 PSID_hex,PSID_mask;
	__u16 sport_A_part, sport_PSID_part;
	__u16 dport_A_part, dport;

	if (mape_tun_dev_get_by_name(tun_name, &dev, &t) == false)
		return 0;

	if (t->parms.mape_psid.set == 0){
		dport = 0;
		goto ret_func;
	}
	
	if (mape_is_port_in_portset_range(tun_name, sport) == 1){
		dport = sport;
		goto ret_func;
	}
	
	mape_psid = &(t->parms.mape_psid);
	mape_portset_field_len_convert(mape_psid, &a, &k, &m);
	PSID_hex = MAPE_PORT_PSID_HEX_CONVERT(mape_psid->psid_info.psid); //0x0340
	PSID_mask = MAPE_PORT_PSID_PART_MASK; //0x0FF0

	sport_A_part = MAPE_PORT_A_PART_GET(sport); // A part of sport
	sport_PSID_part = MAPE_PORT_PSID_PART_GET(sport);//PSID bits part of sport

	if (sport_PSID_part > PSID_hex){//if PSID part of sport > PSID, then carry bit to A part
		if ( MAPE_PORT_A_VAL_INCREASE(sport_A_part) >= MAPE_PORT_A_VAL_MAX ){
			dport_A_part = MAPE_PORT_A_PART_MIN; //A start from 1
		}else {
			dport_A_part = MAPE_PORT_A_PART_INCREASE(sport_A_part); //carry bit to A part
		}
	}else{
		dport_A_part = sport_A_part;
		if (sport_A_part == 0){
			dport_A_part = MAPE_PORT_A_PART_MIN; //A part should be larger than 0
		}
	}

	dport = MAPE_PORT_BIT_SET(MAPE_PORT_BIT_CLEAR(dport_A_part, PSID_mask), PSID_hex); //set the PSID bits
	/*printk("[%s:%d] sport=0x%x , dport=0x%x\n", __FUNCTION__,__LINE__,sport, dport);*/

ret_func:
	if (dev)
		dev_put(dev);	  
	return dport;
}


__u16 ftp_alg_get_mape_available_port(void *nf_ct, __u16 port){
	struct nf_conn * ct;
	if (is_mape_session(nf_ct) == false)
		return 0;

	ct = (struct nf_conn * )nf_ct;
	return mape_portset_get_nearest_port(ct->out_mape_tunnel_name, port);
}
EXPORT_SYMBOL(ftp_alg_get_mape_available_port);

/**
 *	mape_portset_get_delta_port - Get the delta-th port after sport in port-set. 
 *		eg: 
 			sport: 0x1340, delta: 5, then dport: 0x1345
 			sport: 0x134A, delte: 10, then dport: 0x2344
 *
 *	@sport: the FTP port in control link after doing SNAT
 *	@delta: the delta of FTP port between the control link and data link before doing SNAT
 **/
static __u16 mape_portset_get_delta_port(char *tun_name, int sport, int delta){
	struct net_device *dev=NULL;
	struct ip6_tnl *t;
	struct __ip6_tnl_psid *mape_psid;
	__u8 a, k, m; 
	__u16 PSID_hex;
	__u16 sport_A_val, sport_j_val, dport_A_val, dport_j_val;
	__u16 total_delta, carry_bits, dport;

	if (tun_name == NULL)
		return 0;

	if (mape_tun_dev_get_by_name(tun_name, &dev, &t) == false)
		return 0;

	if (t->parms.mape_psid.set == 0){
		dport = 0;
		goto ret_func;
	}
	
	mape_psid = &(t->parms.mape_psid);
	mape_portset_field_len_convert(mape_psid, &a, &k, &m);
	PSID_hex = MAPE_PORT_PSID_HEX_CONVERT(mape_psid->psid_info.psid); //0x0340

	sport_A_val = MAPE_PORT_A_VAL_GET(sport);//A part of sport
	sport_j_val = MAPE_PORT_j_VAL_GET(sport); //j part of sport

	if (delta < 0)
		delta = -delta;

	total_delta = delta + sport_j_val;
	carry_bits = MAPE_PORT_DIV(total_delta, MAPE_PORT_j_VAL_MAX);
	dport_j_val = MAPE_PORT_MOD(total_delta, MAPE_PORT_j_VAL_MAX);
	dport_A_val = MAPE_PORT_MOD((carry_bits + sport_A_val), MAPE_PORT_A_VAL_MAX);
	if (dport_A_val == 0)
		dport_A_val = 1;	//the A needs > 0
		
	dport = (__u16)((dport_A_val<<(k+m)) | PSID_hex | dport_j_val);

	/* printk("[%s:%d] sport=0x%x, delta=%u, dport=0x%x\n\n", __FUNCTION__,__LINE__, sport, delta, dport); */
	
ret_func:

	if (dev)
		dev_put(dev);	  
	return dport;
}


__u16 ftp_alg_get_mape_delta_port(void *nf_ct, __u16 port, __u16 delta){
	struct nf_conn * ct;
	if (is_mape_session(nf_ct) == false)
		return 0;

	ct = (struct nf_conn * )nf_ct;
	if(mape_is_port_in_portset_range(ct->out_mape_tunnel_name, port) == false)
		return 0;
	
	return mape_portset_get_delta_port(ct->out_mape_tunnel_name, port, delta);
}
EXPORT_SYMBOL(ftp_alg_get_mape_delta_port);

#define TCP_CONNTRACK_TIME_WAIT_TIMEOUT 120 * HZ

__u32 ftp_alg_get_new_ct_timeout(void *nf_ct, __u8 state, __u32 ct_timeout){
	struct nf_conn * ct;
	__u32 new_timeout = TCP_CONNTRACK_TIME_WAIT_TIMEOUT;
	ct = (struct nf_conn * )nf_ct;
	
	if (!ct)
		return ct_timeout;

	if(is_mape_session(ct->master) && ct->proto.tcp.state == TCP_CONNTRACK_TIME_WAIT && ct_timeout < TCP_CONNTRACK_TIME_WAIT_TIMEOUT)
		return new_timeout;
	else
		return ct_timeout;
}
EXPORT_SYMBOL(ftp_alg_get_new_ct_timeout);

#ifdef CONFIG_IPV6_MAPE_IPID_ADJUST
typedef enum{KERNEL_PATH, FC_PATH} MAPE_PATH;

/**
 * mape_get_ipid - Rewrite the the IPv4 identifier to port range.
 *	@tun_name: the tunnel device name
 *	@path_type: forwarded by kernel or FC
 *
 * Description:
 *	Modify IPv4 identifier of upstream  packets to port-set assigned to CE.
 *	And use the PSID offset highest bit to separate the kernel path and FC path when a > 1.
 *	Kernel path: set the PSID offset highest bit to 0
 *		eg: 0x1340~0x134F, 0x2340~0x234F, ..., 0x7340~0x723F,0x1340~0x134F...
 *	FC path: set the PSID offset highest bit to 1
 *		eg: 0x8340~0x834F, 0x9340~0x934F, ..., 0xF340~0xF34F,0x8340~0x834F, ...
 *		
 * Reason:
 *	If two IPv4 hosts behind two different MAP CEs with the same IPv4
 *   address send fragments to an IPv4 destination host outside the
 *   domain, those hosts may use the same IPv4 fragmentation identifier,
 *   resulting in incorrect reassembly of the fragments at the destination
 *   host.  Given that the IPv4 fragmentation identifier is a 16-bit
 *   field, it could be used similarly to port ranges.  A MAP CE could
 *   rewrite the IPv4 fragmentation identifier to be within its allocated
 *   port set.
 *
 **/
__u16 mape_get_ipid(char *tun_name, MAPE_PATH path_type){
	struct net_device *dev=NULL;
	struct ip6_tnl *t;
	struct __ip6_tnl_psid *mape_psid=NULL;
	__u8 a, k, m;
	__u16 *portset_id;
	__u16 PSID_hex, PSID_mask;
	__u16 A_part=0, tmp_port;
	__u8 distinguish_path=0;

	if (!(path_type == KERNEL_PATH || path_type == FC_PATH)){
		return 0;
	}
	
	if (mape_tun_dev_get_by_name(tun_name, &dev, &t) == false){
		return 0;
	}

	if (t->parms.mape_psid.set == 0){
		tmp_port = 0;
		goto ret_failed;
	}

	mape_psid = &(t->parms.mape_psid);
	mape_portset_field_len_convert(mape_psid, &a, &k, &m);
	PSID_hex = MAPE_PORT_PSID_HEX_CONVERT(mape_psid->psid_info.psid); //0x0340
	PSID_mask = MAPE_PORT_PSID_PART_MASK; //0x0FF0

	if (a > 1) //Only when a>1, the kernel path and FC path can be distinguished by the highest bit
		distinguish_path = 1;

	read_lock(&(mape_psid->portset_id_lock));
	if (path_type == KERNEL_PATH){
		portset_id = &(mape_psid->kernel_ipid);
	}else if (path_type == FC_PATH){
		portset_id = &(mape_psid->fc_ipid);
	}	
	tmp_port = *portset_id; 
	read_unlock(&(mape_psid->portset_id_lock)); 
	
	if (tmp_port == 0){
		if (path_type == KERNEL_PATH){ /*start from the smallest port in portset, and the A (offset part)should > 0 to exclude the system ports*/
			tmp_port = MAPE_PORT_A_PART_MIN; //the offset bits part need start from 1
		}
		goto ret_ipid;
	}else {
		tmp_port = MAPE_PORT_BIT_CLEAR(tmp_port, PSID_mask); //clear the PSID bit
		tmp_port++;
	}

	if (tmp_port&PSID_mask){//If carry bit
		tmp_port = MAPE_PORT_BIT_CLEAR(tmp_port, PSID_mask); //clear the PSID bit
		
		if ((distinguish_path == 1) && (path_type == FC_PATH)){
			tmp_port = MAPE_PORT_BIT_CLEAR(tmp_port, MAPE_PORT_HIGHEST_BIT_SET); //clear the highest bit for FC path
		}

		A_part = MAPE_PORT_A_PART_INCREASE(tmp_port); //carry bit to A part
		if (distinguish_path == 1){
			if (A_part & MAPE_PORT_HIGHEST_BIT_SET){ //If carry bit is the highest bit, reset to the smallest port in port-set
				if (path_type == KERNEL_PATH){
					A_part = MAPE_PORT_A_PART_MIN; //The highest bit of A part for kernel path should be 0
				}else if (path_type == FC_PATH){
					A_part = MAPE_PORT_HIGHEST_BIT_SET; //The highest bit of A part for FC path should be 1
				}			
			}
		}else {
			if (MAPE_PORT_A_VAL_INCREASE(tmp_port) >= MAPE_PORT_A_VAL_MAX){
				A_part = MAPE_PORT_A_PART_MIN; //kernel should start from 0x1340
			}
		}
		
		tmp_port = A_part;
	}

ret_ipid:	
	if (distinguish_path == 1){
		if (path_type == KERNEL_PATH){
			tmp_port = MAPE_PORT_BIT_CLEAR(tmp_port, MAPE_PORT_HIGHEST_BIT_SET); //clear the highest bit for Kernel path
		}else if(path_type == FC_PATH){
			tmp_port = MAPE_PORT_BIT_SET(tmp_port, MAPE_PORT_HIGHEST_BIT_SET); //set the highest bit for FC path
		}
	}

	tmp_port = MAPE_PORT_BIT_SET(MAPE_PORT_BIT_CLEAR(tmp_port, PSID_mask), PSID_hex); //set the PSID bits

	write_lock(&(mape_psid->portset_id_lock));	
	if(distinguish_path == 1){
		*portset_id = tmp_port;
	}else{
		mape_psid->kernel_ipid = tmp_port;
		mape_psid->fc_ipid = tmp_port;
	}
	write_unlock(&(mape_psid->portset_id_lock));	

	/*
	printk("[%s:%d] path_type=%s, *portset_id=%d\n",
		__FUNCTION__,__LINE__, path_type == KERNEL_PATH?"kernel":"fc", *portset_id);
	*/

ret_failed:
	if (dev)
		dev_put(dev);	
	return tmp_port;
}


__u16 kernel_get_mape_ipid(char *tun_name){
	return mape_get_ipid(tun_name, KERNEL_PATH);
}
EXPORT_SYMBOL(kernel_get_mape_ipid);

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
__u16 fc_get_mape_ipid(char *tun_name){
	return mape_get_ipid(tun_name, FC_PATH);
}
EXPORT_SYMBOL(fc_get_mape_ipid);
#endif
#endif

#endif

/**
 * ip4ip6_fmr_calc - calculate target / source IPv6-address based on FMR
 *   @dest: destination IPv6 address buffer
 *   @skb: received socket buffer
 *   @fmr: MAP FMR
 *   @xmit: Calculate for xmit or rcv
 **/
static void ip4ip6_fmr_calc(struct in6_addr *dest,
		const struct iphdr *iph, const uint8_t *end,
		const struct __ip6_tnl_fmr *fmr, bool xmit, struct ip6_tnl *tunnel)
{
	int psidlen = fmr->rule.ea_len - (32 - fmr->rule.ip4_prefix_len);
	u8 *portp = NULL;
	bool use_dest_addr;
	const struct iphdr *dsth = iph;
	u8 addr4_offset;
	u16 psid16;

	if ((u8*)dsth >= end)
		return;

	/* find significant IP header */
	if (iph->protocol == IPPROTO_ICMP) {
		struct icmphdr *ih = (struct icmphdr*)(((u8*)dsth) + dsth->ihl * 4);
		if (ih && ((u8*)&ih[1]) <= end && (
			ih->type == ICMP_DEST_UNREACH ||
			ih->type == ICMP_SOURCE_QUENCH ||
			ih->type == ICMP_TIME_EXCEEDED ||
			ih->type == ICMP_PARAMETERPROB ||
			ih->type == ICMP_REDIRECT))
				dsth = (const struct iphdr*)&ih[1];
	}

	/* in xmit-path use dest port by default and source port only if
		this is an ICMP reply to something else; vice versa in rcv-path */
	use_dest_addr = (xmit && dsth == iph) || (!xmit && dsth != iph);

	/* get dst port */
	if (((u8*)&dsth[1]) <= end && (
		dsth->protocol == IPPROTO_UDP ||
		dsth->protocol == IPPROTO_TCP ||
		dsth->protocol == IPPROTO_SCTP ||
		dsth->protocol == IPPROTO_DCCP)) {
			/* for UDP, TCP, SCTP and DCCP source and dest port
			follow IPv4 header directly */
			portp = ((u8*)dsth) + dsth->ihl * 4;

			if (use_dest_addr)
				portp += sizeof(u16);
	} else if (iph->protocol == IPPROTO_ICMP) {
		struct icmphdr *ih = (struct icmphdr*)(((u8*)dsth) + dsth->ihl * 4);

		/* use icmp identifier as port */
		if (((u8*)&ih) <= end && (
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
				portp = (u8*)&ih->un.echo.id;
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
		if (tunnel->parms.mape_rfc7597 == 1)
			addr4_offset = 10; //rfc7597
		else
			addr4_offset = 9; //draft03
		memcpy(&dest->s6_addr[addr4_offset], addr, sizeof(*addr));
		psid16 = htons(psidbits >> (16 - psidlen));
		memcpy(&dest->s6_addr[addr4_offset + 4], &psid16, 2);

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


static int __ip6_tnl_rcv(struct ip6_tnl *tunnel, struct sk_buff *skb,
			 const struct tnl_ptk_info *tpi,
			 struct metadata_dst *tun_dst,
			 int (*dscp_ecn_decapsulate)(const struct ip6_tnl *t,
						const struct ipv6hdr *ipv6h,
						struct sk_buff *skb),
			 bool log_ecn_err)
{
	const struct ipv6hdr *ipv6h;
	int nh, err;

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

	skb->protocol = tpi->proto;

	/* Warning: All skb pointers will be invalidated! */
	if (tunnel->dev->type == ARPHRD_ETHER) {
		if (!pskb_may_pull(skb, ETH_HLEN)) {
			tunnel->dev->stats.rx_length_errors++;
			tunnel->dev->stats.rx_errors++;
			goto drop;
		}

		skb->protocol = eth_type_trans(skb, tunnel->dev);
		skb_postpull_rcsum(skb, eth_hdr(skb), ETH_HLEN);
	} else {
		skb->dev = tunnel->dev;
	}

	/* Save offset of outer header relative to skb->head,
	 * because we are going to reset the network header to the inner header
	 * and might change skb->head.
	 */
	nh = skb_network_header(skb) - skb->head;

	skb_reset_network_header(skb);

	if (!pskb_inet_may_pull(skb)) {
		DEV_STATS_INC(tunnel->dev, rx_length_errors);
		DEV_STATS_INC(tunnel->dev, rx_errors);
		goto drop;
	}

	/* Get the outer header. */
	ipv6h = (struct ipv6hdr *)(skb->head + nh);

	memset(skb->cb, 0, sizeof(struct inet6_skb_parm));

	if (tpi->proto == htons(ETH_P_IP) && tunnel->parms.fmrs &&
		!ipv6_addr_equal(&ipv6h->saddr, &tunnel->parms.raddr)) {
			/* Packet didn't come from BR, so lookup FMR */
			struct __ip6_tnl_fmr *fmr;
			struct in6_addr expected = tunnel->parms.raddr;
			for (fmr = tunnel->parms.fmrs; fmr; fmr = fmr->next)
				if (ipv6_prefix_equal(&ipv6h->saddr,
					&fmr->rule.ip6_prefix, fmr->rule.ip6_prefix_len))
						break;

			/* Check that IPv6 matches IPv4 source to prevent spoofing */
			if (fmr)
				ip4ip6_fmr_calc(&expected, ip_hdr(skb),
						skb_tail_pointer(skb), fmr, false, tunnel);

			if (!ipv6_addr_equal(&ipv6h->saddr, &expected)) {
				//rcu_read_unlock();
				goto drop;
			}
	}

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

	dev_sw_netstats_rx_add(tunnel->dev, skb->len);

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

int ip6_tnl_rcv(struct ip6_tnl *t, struct sk_buff *skb,
		const struct tnl_ptk_info *tpi,
		struct metadata_dst *tun_dst,
		bool log_ecn_err)
{
	int (*dscp_ecn_decapsulate)(const struct ip6_tnl *t,
				    const struct ipv6hdr *ipv6h,
				    struct sk_buff *skb);

	dscp_ecn_decapsulate = ip6ip6_dscp_ecn_decapsulate;
	if (tpi->proto == htons(ETH_P_IP))
		dscp_ecn_decapsulate = ip4ip6_dscp_ecn_decapsulate;

	return __ip6_tnl_rcv(t, skb, tpi, tun_dst, dscp_ecn_decapsulate,
			     log_ecn_err);
}
EXPORT_SYMBOL(ip6_tnl_rcv);

static const struct tnl_ptk_info tpi_v6 = {
	/* no tunnel info required for ipxip6. */
	.proto = htons(ETH_P_IPV6),
};

static const struct tnl_ptk_info tpi_v4 = {
	/* no tunnel info required for ipxip6. */
	.proto = htons(ETH_P_IP),
};

static const struct tnl_ptk_info tpi_mpls = {
	/* no tunnel info required for mplsip6. */
	.proto = htons(ETH_P_MPLS_UC),
};

static int ipxip6_rcv(struct sk_buff *skb, u8 ipproto,
		      const struct tnl_ptk_info *tpi,
		      int (*dscp_ecn_decapsulate)(const struct ip6_tnl *t,
						  const struct ipv6hdr *ipv6h,
						  struct sk_buff *skb))
{
	struct ip6_tnl *t;
	const struct ipv6hdr *ipv6h = ipv6_hdr(skb);
	struct metadata_dst *tun_dst = NULL;
	int ret = -1;

	rcu_read_lock();
	t = ip6_tnl_lookup(dev_net(skb->dev), skb->dev->ifindex, &ipv6h->saddr, &ipv6h->daddr);

	if (t) {
		u8 tproto = READ_ONCE(t->parms.proto);

		if (tproto != ipproto && tproto != 0)
			goto drop;
		if (!xfrm6_policy_check(NULL, XFRM_POLICY_IN, skb))
			goto drop;
		ipv6h = ipv6_hdr(skb);
		if (!ip6_tnl_rcv_ctl(t, &ipv6h->daddr, &ipv6h->saddr))
			goto drop;
		if (iptunnel_pull_header(skb, 0, tpi->proto, false))
			goto drop;
		if (t->parms.collect_md) {
			tun_dst = ipv6_tun_rx_dst(skb, 0, 0, 0);
			if (!tun_dst)
				goto drop;
		}
		ret = __ip6_tnl_rcv(t, skb, tpi, tun_dst, dscp_ecn_decapsulate,
				    log_ecn_error);
	}

	rcu_read_unlock();

	return ret;

drop:
	rcu_read_unlock();
	kfree_skb(skb);
	return 0;
}

static int ip4ip6_rcv(struct sk_buff *skb)
{
	return ipxip6_rcv(skb, IPPROTO_IPIP, &tpi_v4,
			  ip4ip6_dscp_ecn_decapsulate);
}

static int ip6ip6_rcv(struct sk_buff *skb)
{
	return ipxip6_rcv(skb, IPPROTO_IPV6, &tpi_v6,
			  ip6ip6_dscp_ecn_decapsulate);
}

static int mplsip6_rcv(struct sk_buff *skb)
{
	return ipxip6_rcv(skb, IPPROTO_MPLS, &tpi_mpls,
			  mplsip6_dscp_ecn_decapsulate);
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
ip6_tnl_addr_conflict(const struct ip6_tnl *t, const struct ipv6hdr *hdr)
{
	return ipv6_addr_equal(&t->parms.raddr, &hdr->saddr);
}

int ip6_tnl_xmit_ctl(struct ip6_tnl *t,
		     const struct in6_addr *laddr,
		     const struct in6_addr *raddr)
{
	struct __ip6_tnl_parm *p = &t->parms;
	int ret = 0;
	struct net *net = t->net;

	if (t->parms.collect_md)
		return 1;

	if ((p->flags & IP6_TNL_F_CAP_XMIT) ||
	    ((p->flags & IP6_TNL_F_CAP_PER_PACKET) &&
	     (ip6_tnl_get_cap(t, laddr, raddr) & IP6_TNL_F_CAP_XMIT))) {
		struct net_device *ldev = NULL;

		rcu_read_lock();
		if (p->link)
			ldev = dev_get_by_index_rcu(net, p->link);

		if (unlikely(!ipv6_chk_addr_and_flags(net, laddr, ldev, false,
						      0, IFA_F_TENTATIVE)))
			pr_warn_ratelimited("%s xmit: Local address not yet configured!\n",
					    p->name);
		else if (!(p->flags & IP6_TNL_F_ALLOW_LOCAL_REMOTE) &&
			 !ipv6_addr_is_multicast(raddr) &&
			 unlikely(ipv6_chk_addr_and_flags(net, raddr, ldev,
							  true, 0, IFA_F_TENTATIVE)))
			pr_warn_ratelimited("%s xmit: Routing loop! Remote address found on this node!\n",
					    p->name);
		else
			ret = 1;
		rcu_read_unlock();
	}
	return ret;
}
EXPORT_SYMBOL_GPL(ip6_tnl_xmit_ctl);

/**
 * ip6_tnl_xmit - encapsulate packet and send
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

int ip6_tnl_xmit(struct sk_buff *skb, struct net_device *dev, __u8 dsfield,
		 struct flowi6 *fl6, int encap_limit, __u32 *pmtu,
		 __u8 proto)
{
	struct ip6_tnl *t = netdev_priv(dev);
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

	if (!ip6_tnl_xmit_ctl(t, &fl6->saddr, &fl6->daddr))
		goto tx_err_link_failure;

	if (!dst) {
route_lookup:
		/* add dsfield to flowlabel for route lookup */
		fl6->flowlabel = ip6_make_flowinfo(dsfield, fl6->flowlabel);

		dst = ip6_route_output(net, NULL, fl6);

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

#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
	if (t->parms.mape_psid.set != 0){
		struct nf_conn *ct;
		ct = (struct nf_conn*)skb_nfct(skb);
		if (ct){
			//printk("[%s:%d] xmit ct=%p (l3num=0x%x)\n", __FUNCTION__,__LINE__,ct, nf_ct_l3num(ct));
			if (!ct->out_mape_tunnel_name[0]){
				memcpy(ct->out_mape_tunnel_name, dev->name, IFNAMSIZ);
			}
		}
	}	
#endif

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
	if (max_headroom > READ_ONCE(dev->needed_headroom))
		WRITE_ONCE(dev->needed_headroom, max_headroom);

	err = ip6_tnl_encap(skb, t, &proto, fl6);
	if (err)
		return err;

	if (encap_limit >= 0) {
		init_tel_txopt(&opt, encap_limit);
		ipv6_push_frag_opts(skb, &opt.ops, &proto);
	}

	skb_push(skb, sizeof(struct ipv6hdr));
	skb_reset_network_header(skb);
	ipv6h = ipv6_hdr(skb);
	ip6_flow_hdr(ipv6h, dsfield,
		     ip6_make_flowlabel(net, skb, fl6->flowlabel, true, fl6));
	ipv6h->hop_limit = hop_limit;
	ipv6h->nexthdr = proto;
	ipv6h->saddr = fl6->saddr;
	ipv6h->daddr = fl6->daddr;
	ip6tunnel_xmit(NULL, skb, dev);
	return 0;
tx_err_link_failure:
	stats->tx_carrier_errors++;
	dst_link_failure(skb);
tx_err_dst_release:
	dst_release(dst);
	return err;
}
EXPORT_SYMBOL(ip6_tnl_xmit);

static inline int
ipxip6_tnl_xmit(struct sk_buff *skb, struct net_device *dev,
		u8 protocol)
{
	struct ip6_tnl *t = netdev_priv(dev);
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
		ip4ip6_fmr_calc(&fl6.daddr, ip_hdr(skb), skb_tail_pointer(skb), fmr, true, t);

	if (iptunnel_handle_offloads(skb, SKB_GSO_IPXIP6))
		return -1;

	skb_set_inner_ipproto(skb, protocol);

	err = ip6_tnl_xmit(skb, dev, dsfield, &fl6, encap_limit, &mtu,
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
	struct ip6_tnl *t = netdev_priv(dev);
	struct net_device_stats *stats = &t->dev->stats;
	u8 ipproto;
	int ret;

	if (!pskb_inet_may_pull(skb))
		goto tx_err;

	switch (skb->protocol) {
	case htons(ETH_P_IP):
		ipproto = IPPROTO_IPIP;
		break;
	case htons(ETH_P_IPV6):
		if (ip6_tnl_addr_conflict(t, ipv6_hdr(skb)))
			goto tx_err;
		ipproto = IPPROTO_IPV6;
		break;
	case htons(ETH_P_MPLS_UC):
		ipproto = IPPROTO_MPLS;
		break;
	default:
		goto tx_err;
	}

	ret = ipxip6_tnl_xmit(skb, dev, ipproto);
	if (ret < 0)
		goto tx_err;

	return NETDEV_TX_OK;

tx_err:
	stats->tx_errors++;
	stats->tx_dropped++;
	kfree_skb(skb);
	return NETDEV_TX_OK;
}

static void ip6_tnl_link_config(struct ip6_tnl *t)
{
	struct net_device *dev = t->dev;
	struct net_device *tdev = NULL;
	struct __ip6_tnl_parm *p = &t->parms;
	struct flowi6 *fl6 = &t->fl.u.ip6;
	int t_hlen;
	int mtu;

	memcpy(dev->dev_addr, &p->laddr, sizeof(struct in6_addr));
	memcpy(dev->broadcast, &p->raddr, sizeof(struct in6_addr));

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
	p->flags |= ip6_tnl_get_cap(t, &p->laddr, &p->raddr);

	if (p->flags&IP6_TNL_F_CAP_XMIT && p->flags&IP6_TNL_F_CAP_RCV)
		dev->flags |= IFF_POINTOPOINT;
	else
		dev->flags &= ~IFF_POINTOPOINT;

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
			dev->needed_headroom = tdev->hard_header_len +
				tdev->needed_headroom + t_hlen;
			mtu = min_t(unsigned int, tdev->mtu, IP6_MAX_MTU);

			mtu = mtu - t_hlen;
			if (!(t->parms.flags & IP6_TNL_F_IGN_ENCAP_LIMIT))
				mtu -= 8;

			if (mtu < IPV6_MIN_MTU)
				mtu = IPV6_MIN_MTU;
			WRITE_ONCE(dev->mtu, mtu);
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
ip6_tnl_change(struct ip6_tnl *t, const struct __ip6_tnl_parm *p)
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

static int ip6_tnl_update(struct ip6_tnl *t, struct __ip6_tnl_parm *p)
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

static int ip6_tnl0_update(struct ip6_tnl *t, struct __ip6_tnl_parm *p)
{
	/* for default tnl0 device allow to change only the proto */
	t->parms.proto = p->proto;
	netdev_state_change(t->dev);
	return 0;
}

static void
ip6_tnl_parm_from_user(struct __ip6_tnl_parm *p, const struct ip6_tnl_parm *u)
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
#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
	memset(&(p->mape_psid), 0x00, sizeof(p->mape_psid));
#endif
	memcpy(p->name, u->name, sizeof(u->name));
}

static void
ip6_tnl_parm_to_user(struct ip6_tnl_parm *u, const struct __ip6_tnl_parm *p)
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
 * ip6_tnl_ioctl - configure ipv6 tunnels from userspace
 *   @dev: virtual device associated with tunnel
 *   @ifr: parameters passed from userspace
 *   @cmd: command to be performed
 *
 * Description:
 *   ip6_tnl_ioctl() is used for managing IPv6 tunnels
 *   from userspace.
 *
 *   The possible commands are the following:
 *     %SIOCGETTUNNEL: get tunnel parameters for device
 *     %SIOCADDTUNNEL: add tunnel matching given tunnel parameters
 *     %SIOCCHGTUNNEL: change tunnel parameters to those given
 *     %SIOCDELTUNNEL: delete tunnel
 *
 *   The fallback device "ip6tnl0", created during module
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
	struct __ip6_tnl_parm p1;
	struct ip6_tnl *t = netdev_priv(dev);
	struct net *net = t->net;
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	struct ipv6_tnl_fmr fmrInfo;
	struct __ip6_tnl_fmr *fmrs_next, *fmrP;
#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
	struct mape_psid_info psid_info;
#endif

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
		if (p.proto != IPPROTO_IPV6 && p.proto != IPPROTO_IPIP &&
		    p.proto != 0)
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

	#if 1//RTK
		ip6tnl_destroy_fmr_proc(dev);
	#endif
		/*free FMRS*/
		while(t->parms.fmrs){
			fmrs_next = t->parms.fmrs->next;
			kfree(t->parms.fmrs);
			t->parms.fmrs = fmrs_next;
		}
		unregister_netdevice(dev);
		break;
	case SIOCSETFMRSRULE: /*SET FMRS*/
		err = -EPERM;
		if (copy_from_user(&fmrInfo, ifr->ifr_ifru.ifru_data, sizeof(fmrInfo)))
			break;
		memcpy(&p1.laddr, &fmrInfo.laddr ,sizeof(p1.laddr));
		memcpy(&p1.raddr, &fmrInfo.raddr ,sizeof(p1.raddr));
		t = ip6_tnl_locate(net, &p1, 0);
		if (IS_ERR(t))
			break;
			
		fmrP = (struct __ip6_tnl_fmr *)kmalloc(sizeof(struct __ip6_tnl_fmr), GFP_KERNEL);
		if(fmrP == NULL){
			printk("[%s:%d][FMRS]:kmalloc for struct __ip6_tnl_fmr failed!!\n",__FUNCTION__,__LINE__);
			while(t->parms.fmrs){
				fmrs_next = t->parms.fmrs->next;
				kfree(t->parms.fmrs);
				t->parms.fmrs = fmrs_next;
			}
			break;
		}
		memset(fmrP, 0x00, sizeof(struct __ip6_tnl_fmr));
		memcpy(&fmrP->rule, &fmrInfo.rule, sizeof(struct ip6_tnl_fmr_rule));
		if(t->parms.fmrs == NULL){
			t->parms.fmrs = fmrP;
		}else{
			fmrs_next = t->parms.fmrs;
			while(fmrs_next->next)
				fmrs_next = fmrs_next->next;
			fmrs_next->next = fmrP;
		}	
		err = 0;
		break;

#ifdef CONFIG_IPV6_MAPE_PSID_KERNEL_HOOK
	case SIOCSETPSID: /*Set MAPE PSID*/
		err = -EPERM;
		if (copy_from_user(&psid_info, ifr->ifr_ifru.ifru_data, sizeof(psid_info)))
			break;
		
		memcpy(&p1.laddr, &psid_info.laddr, sizeof(p1.laddr));
		memcpy(&p1.raddr, &psid_info.raddr,sizeof(p1.raddr));
		t = ip6_tnl_locate(net, &p1, 0);
		if (IS_ERR(t)){
			break;
		}	

		if (mape_portset_init(&psid_info, t) == 1){
			err = 0;		
		}
		break;
#endif

	default:
		err = -EINVAL;
	}
	return err;
}

/**
 * ip6_tnl_change_mtu - change mtu manually for tunnel device
 *   @dev: virtual device associated with tunnel
 *   @new_mtu: the new mtu
 *
 * Return:
 *   0 on success,
 *   %-EINVAL if mtu too small
 **/

int ip6_tnl_change_mtu(struct net_device *dev, int new_mtu)
{
	struct ip6_tnl *tnl = netdev_priv(dev);
	int t_hlen;

	t_hlen = tnl->hlen + sizeof(struct ipv6hdr);
	if (tnl->parms.proto == IPPROTO_IPV6) {
		if (new_mtu < IPV6_MIN_MTU)
			return -EINVAL;
	} else {
		if (new_mtu < ETH_MIN_MTU)
			return -EINVAL;
	}
	if (tnl->parms.proto == IPPROTO_IPV6 || tnl->parms.proto == 0) {
		if (new_mtu > IP6_MAX_MTU - dev->hard_header_len - t_hlen)
			return -EINVAL;
	} else {
		if (new_mtu > IP_MAX_MTU - dev->hard_header_len - t_hlen)
			return -EINVAL;
	}
	dev->mtu = new_mtu;
	return 0;
}
EXPORT_SYMBOL(ip6_tnl_change_mtu);

int ip6_tnl_get_iflink(const struct net_device *dev)
{
	struct ip6_tnl *t = netdev_priv(dev);

	return t->parms.link;
}
EXPORT_SYMBOL(ip6_tnl_get_iflink);

int ip6_tnl_encap_add_ops(const struct ip6_tnl_encap_ops *ops,
			  unsigned int num)
{
	if (num >= MAX_IPTUN_ENCAP_OPS)
		return -ERANGE;

	return !cmpxchg((const struct ip6_tnl_encap_ops **)
			&ip6tun_encaps[num],
			NULL, ops) ? 0 : -1;
}
EXPORT_SYMBOL(ip6_tnl_encap_add_ops);

int ip6_tnl_encap_del_ops(const struct ip6_tnl_encap_ops *ops,
			  unsigned int num)
{
	int ret;

	if (num >= MAX_IPTUN_ENCAP_OPS)
		return -ERANGE;

	ret = (cmpxchg((const struct ip6_tnl_encap_ops **)
		       &ip6tun_encaps[num],
		       ops, NULL) == ops) ? 0 : -1;

	synchronize_net();

	return ret;
}
EXPORT_SYMBOL(ip6_tnl_encap_del_ops);

int ip6_tnl_encap_setup(struct ip6_tnl *t,
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
EXPORT_SYMBOL_GPL(ip6_tnl_encap_setup);

static const struct net_device_ops ip6_tnl_netdev_ops = {
	.ndo_init	= ip6_tnl_dev_init,
	.ndo_uninit	= ip6_tnl_dev_uninit,
	.ndo_start_xmit = ip6_tnl_start_xmit,
	.ndo_do_ioctl	= ip6_tnl_ioctl,
	.ndo_change_mtu = ip6_tnl_change_mtu,
	.ndo_get_stats	= ip6_get_stats,
	.ndo_get_iflink = ip6_tnl_get_iflink,
};

#define IPXIPX_FEATURES (NETIF_F_SG |		\
			 NETIF_F_FRAGLIST |	\
			 NETIF_F_HIGHDMA |	\
			 NETIF_F_GSO_SOFTWARE |	\
			 NETIF_F_HW_CSUM)

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
	struct ip6_tnl *t = netdev_priv(dev);
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

	dev->type = ARPHRD_TUNNEL6;
	dev->mtu = ETH_DATA_LEN - t_hlen;
	if (!(t->parms.flags & IP6_TNL_F_IGN_ENCAP_LIMIT))
		dev->mtu -= 8;
	dev->min_mtu = ETH_MIN_MTU;
	dev->max_mtu = IP6_MAX_MTU - dev->hard_header_len - t_hlen;

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
	struct ip6_tnl *t = netdev_priv(dev);
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
	struct ip6_tnl *t = netdev_priv(dev);
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
	if (proto != IPPROTO_IPV6 &&
	    proto != IPPROTO_IPIP &&
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

static void ip6_tnl_netlink_parms(struct nlattr *data[],
				  struct __ip6_tnl_parm *parms)
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

	if (data[IFLA_IPTUN_COLLECT_METADATA])
		parms->collect_md = true;

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
	struct ip6_tnl *nt, *t;
	int err;

	nt = netdev_priv(dev);

	if (ip6_tnl_netlink_encap_parms(data, &ipencap)) {
		err = ip6_tnl_encap_setup(nt, &ipencap);
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
		ip6_tnl_change_mtu(dev, nla_get_u32(tb[IFLA_MTU]));

	return err;
}

static int ip6_tnl_changelink(struct net_device *dev, struct nlattr *tb[],
			      struct nlattr *data[],
			      struct netlink_ext_ack *extack)
{
	struct ip6_tnl *t = netdev_priv(dev);
	struct __ip6_tnl_parm p;
	struct net *net = t->net;
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	struct ip_tunnel_encap ipencap;

	if (dev == ip6n->fb_tnl_dev)
		return -EINVAL;

	if (ip6_tnl_netlink_encap_parms(data, &ipencap)) {
		int err = ip6_tnl_encap_setup(t, &ipencap);

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
	const struct ip6_tnl *t = netdev_priv(dev);
#ifdef MAPE_GIVE_FMR_INFO
	struct __ip6_tnl_fmr *c;
	int fmrs = 0;
	for (c = t->parms.fmrs; c; c = c->next)
		++fmrs;
#endif
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
		nla_total_size(0) +
		/* IFLA_IPTUN_FWMARK */
		nla_total_size(4) +
#ifdef MAPE_GIVE_FMR_INFO
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
#endif
		0;
}

static int ip6_tnl_fill_info(struct sk_buff *skb, const struct net_device *dev)
{
	struct ip6_tnl *tunnel = netdev_priv(dev);
	struct __ip6_tnl_parm *parm = &tunnel->parms;
#ifdef MAPE_GIVE_FMR_INFO
	struct __ip6_tnl_fmr *c;
	int fmrcnt = 0;
	struct nlattr *fmrs;
#endif
	if (nla_put_u32(skb, IFLA_IPTUN_LINK, parm->link) ||
	    nla_put_in6_addr(skb, IFLA_IPTUN_LOCAL, &parm->laddr) ||
	    nla_put_in6_addr(skb, IFLA_IPTUN_REMOTE, &parm->raddr) ||
	    nla_put_u8(skb, IFLA_IPTUN_TTL, parm->hop_limit) ||
	    nla_put_u8(skb, IFLA_IPTUN_ENCAP_LIMIT, parm->encap_limit) ||
	    nla_put_be32(skb, IFLA_IPTUN_FLOWINFO, parm->flowinfo) ||
	    nla_put_u32(skb, IFLA_IPTUN_FLAGS, parm->flags) ||
	    nla_put_u8(skb, IFLA_IPTUN_PROTO, parm->proto) ||
#ifdef MAPE_GIVE_FMR_INFO
	    !(fmrs = nla_nest_start(skb, IFLA_IPTUN_FMRS)) ||
#endif
	    nla_put_u32(skb, IFLA_IPTUN_FWMARK, parm->fwmark))
		goto nla_put_failure;
#ifdef MAPE_GIVE_FMR_INFO
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
#endif
	if (nla_put_u16(skb, IFLA_IPTUN_ENCAP_TYPE, tunnel->encap.type) ||
	    nla_put_be16(skb, IFLA_IPTUN_ENCAP_SPORT, tunnel->encap.sport) ||
	    nla_put_be16(skb, IFLA_IPTUN_ENCAP_DPORT, tunnel->encap.dport) ||
	    nla_put_u16(skb, IFLA_IPTUN_ENCAP_FLAGS, tunnel->encap.flags))
		goto nla_put_failure;

	if (parm->collect_md)
		if (nla_put_flag(skb, IFLA_IPTUN_COLLECT_METADATA))
			goto nla_put_failure;

	return 0;

nla_put_failure:
	return -EMSGSIZE;
}

struct net *ip6_tnl_get_link_net(const struct net_device *dev)
{
	struct ip6_tnl *tunnel = netdev_priv(dev);

	return tunnel->net;
}
EXPORT_SYMBOL(ip6_tnl_get_link_net);

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
	[IFLA_IPTUN_COLLECT_METADATA]	= { .type = NLA_FLAG },
	[IFLA_IPTUN_FWMARK]		= { .type = NLA_U32 },
	[IFLA_IPTUN_FMRS]		= { .type = NLA_NESTED },
};

static struct rtnl_link_ops ip6_link_ops __read_mostly = {
	.kind		= "ip6tnl",
	.maxtype	= IFLA_IPTUN_MAX,
	.policy		= ip6_tnl_policy,
	.priv_size	= sizeof(struct ip6_tnl),
	.setup		= ip6_tnl_dev_setup,
	.validate	= ip6_tnl_validate,
	.newlink	= ip6_tnl_newlink,
	.changelink	= ip6_tnl_changelink,
	.dellink	= ip6_tnl_dellink,
	.get_size	= ip6_tnl_get_size,
	.fill_info	= ip6_tnl_fill_info,
	.get_link_net	= ip6_tnl_get_link_net,
};

static struct xfrm6_tunnel ip4ip6_handler __read_mostly = {
	.handler	= ip4ip6_rcv,
	.err_handler	= ip4ip6_err,
	.priority	=	1,
};

static struct xfrm6_tunnel ip6ip6_handler __read_mostly = {
	.handler	= ip6ip6_rcv,
	.err_handler	= ip6ip6_err,
	.priority	=	1,
};

static struct xfrm6_tunnel mplsip6_handler __read_mostly = {
	.handler	= mplsip6_rcv,
	.err_handler	= mplsip6_err,
	.priority	=	1,
};

static void __net_exit ip6_tnl_destroy_tunnels(struct net *net, struct list_head *list)
{
	struct ip6_tnl_net *ip6n = net_generic(net, ip6_tnl_net_id);
	struct net_device *dev, *aux;
	int h;
	struct ip6_tnl *t;

	for_each_netdev_safe(net, dev, aux)
		if (dev->rtnl_link_ops == &ip6_link_ops){
			#if 1//RTK
			ip6tnl_fmr_proc_exit(net);
			#endif
			unregister_netdevice_queue(dev, list);
		}

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
	struct ip6_tnl *t = NULL;
	int err;

	ip6n->tnls[0] = ip6n->tnls_wc;
	ip6n->tnls[1] = ip6n->tnls_r_l;

	if (!net_has_fallback_tunnels(net))
		return 0;
	err = -ENOMEM;
	ip6n->fb_tnl_dev = alloc_netdev(sizeof(struct ip6_tnl), "ip6tnl0",
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

#if 1//RTK
	ip6tnl_fmr_proc_init(net);
#endif

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

/**
 * ip6_tunnel_init - register protocol and reserve needed resources
 *
 * Return: 0 on success
 **/

static int __init ip6_tunnel_init(void)
{
	int  err;

	if (!ipv6_mod_enabled())
		return -EOPNOTSUPP;

	err = register_pernet_device(&ip6_tnl_net_ops);
	if (err < 0)
		goto out_pernet;

	err = xfrm6_tunnel_register(&ip4ip6_handler, AF_INET);
	if (err < 0) {
		pr_err("%s: can't register ip4ip6\n", __func__);
		goto out_ip4ip6;
	}

	err = xfrm6_tunnel_register(&ip6ip6_handler, AF_INET6);
	if (err < 0) {
		pr_err("%s: can't register ip6ip6\n", __func__);
		goto out_ip6ip6;
	}

	if (ip6_tnl_mpls_supported()) {
		err = xfrm6_tunnel_register(&mplsip6_handler, AF_MPLS);
		if (err < 0) {
			pr_err("%s: can't register mplsip6\n", __func__);
			goto out_mplsip6;
		}
	}

	err = rtnl_link_register(&ip6_link_ops);
	if (err < 0)
		goto rtnl_link_failed;

	return 0;

rtnl_link_failed:
	if (ip6_tnl_mpls_supported())
		xfrm6_tunnel_deregister(&mplsip6_handler, AF_MPLS);
out_mplsip6:
	xfrm6_tunnel_deregister(&ip6ip6_handler, AF_INET6);
out_ip6ip6:
	xfrm6_tunnel_deregister(&ip4ip6_handler, AF_INET);
out_ip4ip6:
	unregister_pernet_device(&ip6_tnl_net_ops);
out_pernet:
	return err;
}

/**
 * ip6_tunnel_cleanup - free resources and unregister protocol
 **/

static void __exit ip6_tunnel_cleanup(void)
{
	rtnl_link_unregister(&ip6_link_ops);
	if (xfrm6_tunnel_deregister(&ip4ip6_handler, AF_INET))
		pr_info("%s: can't deregister ip4ip6\n", __func__);

	if (xfrm6_tunnel_deregister(&ip6ip6_handler, AF_INET6))
		pr_info("%s: can't deregister ip6ip6\n", __func__);

	if (ip6_tnl_mpls_supported() &&
	    xfrm6_tunnel_deregister(&mplsip6_handler, AF_MPLS))
		pr_info("%s: can't deregister mplsip6\n", __func__);
	unregister_pernet_device(&ip6_tnl_net_ops);

}

module_init(ip6_tunnel_init);
module_exit(ip6_tunnel_cleanup);
