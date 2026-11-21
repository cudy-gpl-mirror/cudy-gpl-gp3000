#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>
//#include <net/rtl/rtl_types.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/spinlock_types.h>
#include <linux/seq_file.h>
#include <uapi/linux/if.h>
#include <net/sock.h>
#include <linux/netlink.h>
#ifdef CONFIG_RTK_GENERIC_NETLINK_API
#include <net/genetlink.h>
#include <linux/version.h>
#include <net/rtl/rtl_genl.h>

static int wlan_event_rcv(struct sk_buff *skb, struct genl_info *info);
/* genl command ops definition */
static struct genl_ops rtk_wlan_event_indicate_ops[] = {
{
	.cmd = RTL_WLAN_INDICATE_CMD_EVENT,
	.flags = 0,
	.doit = wlan_event_rcv,
},
};

/* genl group definition */
static struct genl_multicast_group rtk_wlan_event_indicate_grp[] = {
{
	.name = RTL_GENL_WLAN_EVENT_GROUP,
},
};

/* genl family definition */
static struct genl_family genl_wlan_indicate_family = {
	.hdrsize = 0,
	.name = RTL_GENL_WLAN_EVENT,
	.version = 1,
	.maxattr = RTL_WLAN_INDICATE_ATTR_MAX,
	.id = 0,
	.ops = rtk_wlan_event_indicate_ops,
	.n_ops = ARRAY_SIZE(rtk_wlan_event_indicate_ops),
	.mcgrps = rtk_wlan_event_indicate_grp,
	.n_mcgrps = ARRAY_SIZE(rtk_wlan_event_indicate_grp),
};

static int get_genl_grp_idx_by_name(struct genl_family *family, const char *name){
	int i=0;
	for(i=0; i < family->n_mcgrps; i++){
		printk(KERN_DEBUG"group name =%s\n", (family->mcgrps+i)->name);
		if(!strcmp((family->mcgrps+i)->name, name))
			return i;
	}

	return -1;
}
#endif /* CONFIG_RTK_GENERIC_NETLINK_API */


#define DRIVER_AUTHOR 	"IulianWu"
#define DRIVER_DESC 	"RealTek WLAN Event Counters"

static struct proc_dir_entry *entry = NULL;
static struct proc_dir_entry *parent;
struct sock *nl_sk=NULL;					//used for hanlde RTK WLAN driver
static int userpid	= 1;

#define PROCFS_WLAN_EVENT_COUNTER			"wlan_event_counter"
#define PROCFS_OSGI_DIR0 					"wlan0_event"
#define PROCFS_OSGI_DIR1 					"wlan1_event"
#define WLAN_EVENT_DISAPPEAR_TIMEOUT		10 //if counter didn't increase in 10secs, adjust the traffic is disappear.
#define MACADDRLEN	6
#define NETLINK_RTK_WLAN_EVENT 29			//used for hanlde RTK WLAN driver
#define GROUP_RTK_WLAN_EVENT			(1<<0)

typedef struct wlan_event_entry_s
{
	int eventID;
	unsigned char mac[MACADDRLEN];
	char ifname[IFNAMSIZ];
	char reason;
	long timer;
    struct list_head list;
} wlan_event_entry_t;

wlan_event_entry_t wlan_event_entry_list;


static int wlan_event_send(int pid, int eventID, char *data, int data_len)
{
	struct sk_buff *skb;
#ifdef CONFIG_RTK_GENERIC_NETLINK_API
		int rc = 0, grp_idx=-1;
		void *head = NULL;
		size_t genl_payload = 0;
#else
	struct nlmsghdr *nlh;
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */
	char netlink_data[sizeof(int) + (MACADDRLEN+1+IFNAMSIZ+2)]={0};
	memcpy(netlink_data, &eventID, sizeof(int));
	memcpy(netlink_data + sizeof(int), data, data_len);

#ifdef CONFIG_RTK_GENERIC_NETLINK_API
	genl_payload = nla_total_size(sizeof(netlink_data)); /* total length of attribute including padding */
	printk(KERN_DEBUG"[%s] genl payload len=%d\n", __func__, genl_payload);
	/* create a new netlink msg */
	skb = genlmsg_new(genl_payload, GFP_ATOMIC);
#else	/* CONFIG_RTK_GENERIC_NETLINK_API */
	skb = nlmsg_new(sizeof(netlink_data), GFP_ATOMIC);
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */

	printk(KERN_DEBUG"Enter %s \n", __func__);

	if(!skb)
	{
		printk(KERN_ERR"Failed to alloc skb\n");
		return 0;
	}

#ifdef CONFIG_RTK_GENERIC_NETLINK_API
	/* Add a new netlink message to an skb */
	genlmsg_put(skb, pid, 0, &genl_wlan_indicate_family, 0, RTL_WLAN_INDICATE_CMD_EVENT);

	/* add a netlink attribute to a socket buffer */
	if ((rc = nla_put(skb, RTL_WLAN_INDICATE_ATTR_MSG, data_len+sizeof(int), netlink_data)) != 0) {
		printk(KERN_ERR"nla_put fail\n");
		nlmsg_free(skb);
		return rc;
	}

	head = genlmsg_data(nlmsg_data(nlmsg_hdr(skb)));
	genlmsg_end(skb, head);

#if 0	// for unicast
	printk(KERN_ERR"Sent to pid %d message\n", pid);
	rc = genlmsg_unicast(&init_net, skb, pid);
	if (rc < 0) {
		printk(KERN_ERR"Failed to unicast skb\n");
		return rc;
	}
#endif
	grp_idx = get_genl_grp_idx_by_name(&genl_wlan_indicate_family, RTL_GENL_WLAN_EVENT_GROUP);
	if(-1 == grp_idx){
		printk(KERN_ERR"get group offset Failed(%s)\n", RTL_GENL_WLAN_EVENT_GROUP);
		nlmsg_free(skb);
		return -1;
	}

	printk(KERN_DEBUG"Sent to group (%d) message\n", (genl_wlan_indicate_family.mcgrp_offset+grp_idx));
	rc = genlmsg_multicast(&genl_wlan_indicate_family, skb, 0, grp_idx, GFP_ATOMIC);		// for first group
	if (rc < 0) {
		printk(KERN_ERR"Failed to multicast skb: ret=%d\n", rc);
		return rc;
	}
#else	/* CONFIG_RTK_GENERIC_NETLINK_API */
	// put into skb
	nlh = nlmsg_put(skb, 0, 0, 0, data_len+sizeof(int), 0);

	memcpy(nlmsg_data(nlh), netlink_data, data_len+sizeof(int));

	NETLINK_CB(skb).portid = 0; /* from kernel */
	NETLINK_CB(skb).dst_group = GROUP_RTK_WLAN_EVENT;

	printk(KERN_DEBUG"Sent to Group %d message\n", GROUP_RTK_WLAN_EVENT);
	if (netlink_broadcast(nl_sk, skb, 0, GROUP_RTK_WLAN_EVENT, GFP_ATOMIC) < 0)
	{
		printk(KERN_ERR"Failed to broadcast skb\n");
		return 0;
	}
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */

	return 1;
}


void rtk_eventd_netlink_send(int pid, struct sock *nl_sk, int eventID, char *ifname, char *data, int data_len)
{
	//if(userpid!=1){
		wlan_event_send(userpid, eventID, data, data_len);
	//}
}
EXPORT_SYMBOL(rtk_eventd_netlink_send);

int get_nl_eventd_pid(void)
{
    return userpid;
}
EXPORT_SYMBOL(get_nl_eventd_pid);

struct sock *get_nl_eventd_sk(void)
{
    return nl_sk;
}
EXPORT_SYMBOL(get_nl_eventd_sk);

struct sock *rtk_eventd_netlink_init(void)
{
    return nl_sk;
}
EXPORT_SYMBOL(rtk_eventd_netlink_init);

static void *wlan_event_seq_idx(loff_t pos)
{
	wlan_event_entry_t *node = NULL;
	list_for_each_entry(node, &(wlan_event_entry_list.list), list)
	if (pos-- == 0)
		return node;
	return NULL;
}

static void *wlan_event_seq_start(struct seq_file *s, loff_t *pos)
{
#if __DEBUG
	printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos);
#endif
	return *pos ? wlan_event_seq_idx(*pos - 1)	: SEQ_START_TOKEN;
}
static void *wlan_event_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	++*pos;
#if __DEBUG
	printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos);
#endif
	return wlan_event_seq_start(s, pos);
}

static void wlan_event_seq_stop(struct seq_file *s, void *v)
{
#if __DEBUG
	printk("[%s:%d] \n", __FUNCTION__, __LINE__);
#endif
	wlan_event_entry_t *node = NULL, *tmp = NULL;
	list_for_each_entry_safe(node, tmp, &(wlan_event_entry_list.list), list)
	{
		list_del(&node->list);
		kfree(node);
	}
}

static int wlan_event_seq_show(struct seq_file *seq, void *v)
{
	if (v == SEQ_START_TOKEN) {
		seq_puts(seq,  "eventID ifname mac reason\n");
	}
	else {
		//struct timespec now;
		//long curr_time;
		wlan_event_entry_t *node = v;
		//now = current_kernel_time();
		//curr_time = now.tv_sec;
#if __DEBUG
		printk("[%s:%d] %d %s %02x:%02x:%02x:%02x:%02x:%02x %d\n", __FUNCTION__, __LINE__,
				node->eventID, node->ifname, node->mac[0], node->mac[1], node->mac[2],
				node->mac[3], node->mac[4], node->mac[5], node->reason);
#endif
		seq_printf(seq, "%d %s %02x:%02x:%02x:%02x:%02x:%02x %d\n",
				node->eventID, node->ifname, node->mac[0], node->mac[1], node->mac[2],
				node->mac[3], node->mac[4], node->mac[5], node->reason);

	}
	return 0;
}

static struct seq_operations wlan_event_counter_ops = {
	.start			= wlan_event_seq_start,
	.next			= wlan_event_seq_next,
	.stop			= wlan_event_seq_stop,
	.show			= wlan_event_seq_show
};

#if 0
static int wlan_event_counter_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &wlan_event_counter_ops);
};

static struct file_operations wlan_event_counter_fops = {
	.owner 			= THIS_MODULE,
	.open			= wlan_event_counter_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= seq_release
};
#endif

static void wlan_event_entry_list_init(void)
{
    INIT_LIST_HEAD(&wlan_event_entry_list.list);
}

#ifdef CONFIG_RTK_GENERIC_NETLINK_API
static int wlan_event_rcv(struct sk_buff *skb, struct genl_info *info)
#else
static void wlan_event_rcv(struct sk_buff *skb)
#endif
{
	struct nlmsghdr *nlh = NULL;
#ifdef CONFIG_RTK_GENERIC_NETLINK_API
	struct genlmsghdr *genlhdr = NULL;
	struct nlattr *nla = NULL;
	unsigned int i = 0, nla_len = 0;
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */

	printk(KERN_DEBUG"Enter %s\n", __func__);

	if(skb == NULL)
	{
		printk(KERN_ERR"skb is NULL\n");
#ifdef CONFIG_RTK_GENERIC_NETLINK_API
		return 0;
#else
		return;
#endif
	}
	nlh = (struct nlmsghdr *)skb->data;

	printk(KERN_DEBUG"%s:received message from pid %d: %s\n", __FUNCTION__, nlh->nlmsg_pid, (char *)NLMSG_DATA(nlh));

	userpid = nlh->nlmsg_pid;
#ifdef CONFIG_RTK_GENERIC_NETLINK_API
	genlhdr = nlmsg_data(nlh);
	if(genl_wlan_indicate_family.hdrsize){
		printk(KERN_ERR"[%s:%d] parser user specific header heae.\n", __FUNCTION__, __LINE__);
	}

	nla = genlmsg_data(genlhdr) + genl_wlan_indicate_family.hdrsize;
	nla_len = genlmsg_len(genlhdr) - genl_wlan_indicate_family.hdrsize; 		  //len of attributes

	printk(KERN_DEBUG"%s:received message from pid %d, genl_cmd=%d\n", __FUNCTION__, nlh->nlmsg_pid, genlhdr->cmd);
	for (i = 0; nla_ok(nla, nla_len); nla = nla_next(nla, &nla_len), ++i)
	{
		printk(KERN_DEBUG"%s: [%d]nla_type=%d\n", __FUNCTION__, i, nla->nla_type);
	}

	return 1;
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */
}

static int __init wlan_event_init_main(void)
{
#ifdef CONFIG_RTK_GENERIC_NETLINK_API
	int ret = 0;
#else	/* CONFIG_RTK_GENERIC_NETLINK_API */
    struct netlink_kernel_cfg cfg = {
        .input = wlan_event_rcv,
	};
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */

	parent = proc_mkdir(PROCFS_OSGI_DIR0, NULL);
	entry = proc_create_seq_data(PROCFS_WLAN_EVENT_COUNTER, 0444, parent, &wlan_event_counter_ops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR0, PROCFS_WLAN_EVENT_COUNTER);
	}
	wlan_event_entry_list_init();

#ifdef CONFIG_RTK_GENERIC_NETLINK_API
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0)
	ret = genl_register_family_with_ops_groups(&genl_wlan_indicate_family, rtk_wlan_event_indicate_ops, rtk_wlan_event_indicate_grp);
#else
	ret = genl_register_family(&genl_wlan_indicate_family);
#endif
	if(0 != ret){
		printk(KERN_ERR"[%s] netlink_kernel_create failed !! \n", __FUNCTION__);
	}
	nl_sk = 1;
#else	/* CONFIG_RTK_GENERIC_NETLINK_API */
	nl_sk = netlink_kernel_create(&init_net, NETLINK_RTK_WLAN_EVENT, &cfg);//used for hanlde RTK WLAN driver
	if(!nl_sk)
		printk("%s:%d] netlink_kernel_create failed !! \n", __FUNCTION__, __LINE__);
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */

	printk("%s: Successfully inserted a hook into kernel\n", __FUNCTION__);
	return 0;
}

static void __exit wlan_event_cleanup_main(void)
{
	wlan_event_entry_t *node = NULL, *tmp = NULL;
	list_for_each_entry_safe(node, tmp, &(wlan_event_entry_list.list), list)
	{
		list_del(&node->list);
		kfree(node);
	}

	remove_proc_entry(PROCFS_WLAN_EVENT_COUNTER, parent);
	proc_remove(parent);
#ifdef CONFIG_RTK_GENERIC_NETLINK_API
	genl_unregister_family(&genl_wlan_indicate_family);
#else	/* CONFIG_RTK_GENERIC_NETLINK_API */
	netlink_kernel_release(nl_sk);
#endif	/* CONFIG_RTK_GENERIC_NETLINK_API */
	printk("Successfully unloaded the hook\n");
}


late_initcall(wlan_event_init_main);
module_exit(wlan_event_cleanup_main);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);


