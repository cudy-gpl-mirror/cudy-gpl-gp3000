#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/netfilter.h>
#include <linux/netfilter_bridge.h>
#include <uapi/linux/netfilter_bridge.h>
#include "../net/bridge/br_private.h"
#include <linux/ptrace.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/init.h>

#if defined(CONFIG_LUNA)
#include <linux/proc_fs.h>
#if defined(CONFIG_RTL8686_SWITCH)
#include <rtk/port.h>
#include <rtk/switch.h>
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
#include <module/intr_bcaster/intr_bcaster.h>
#endif
#ifdef CONFIG_RTL9607C_SERIES
#include <dal/rtl9607c/dal_rtl9607c_switch.h>
#else
#include <dal/apollomp/dal_apollomp_switch.h>
#endif
#endif
#endif
#include <rtk/rt/rt_port.h>


struct lbd_data
{
	unsigned int int_magic;
	unsigned char ifnum;
	unsigned char str_magic[41];	//make data length be 46 bytes
} __attribute__((packed));

#if defined(CONFIG_LUNA)
#if defined(CONFIG_RTL8686_SWITCH)
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
static int lan_port_remapping[4] = {
        0,
        1,
        2,
        3,
};
#endif

static rtk_port_t get_hw_port_id(int intf)
{
	uint32 id, rev, sub;
	int32 port;

#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
	if((intf < 0) || (intf >= 4) || rtk_switch_phyPortId_get(lan_port_remapping[intf], &port) != 0)
	{
		printk("%s rtk_switch_phyPortId_get failed!\n", __FUNCTION__);
		return -1;
	}
#else
	rtk_switch_version_get(&id, &rev, &sub);
	if(id == RTL9602C_CHIP_ID)
	{
		intf ^= 0x1;	/* port remapping */
	}
	else if(id == APOLLOMP_CHIP_ID && (sub == APPOLOMP_CHIP_SUB_TYPE_RTL9602 ||
		    sub == APPOLOMP_CHIP_SUB_TYPE_RTL9603) )
	{
		switch(intf)
		{
		case 0:
			intf = 3;
			break;
		case 1:
			intf = 2;
			break;
		case 2:
			intf = 1;
			break;
		case 3:
			intf = 0;
			break;
		}
	}
	else if(id == RTL9607C_CHIP_ID)
	{
		switch(intf)
		{
		case 0:
			intf = 3;
			break;
		case 1:
			intf = 2;
			break;
		case 2:
			intf = 1;
			break;
		case 3:
			intf = 0;
			break;
		}
	}

	if(rtk_switch_phyPortId_get(intf, &port) != 0 )
	{
		printk("%s rtk_switch_phyPortId_get failed!\n", __FUNCTION__);
		return -1;
	}
#endif

	return port;
}
#endif

void rtk_loopback_disable_intf(int intf)
{
#if defined(CONFIG_RTL8686_SWITCH)
	rtk_port_t port = -1;
    //uint32 value;

	port = get_hw_port_id(intf);
	if(port == -1)
	{
		printk("%s: get_hw_port_id failed!\n", __FUNCTION__);
		return;
	}

#if 1
#ifdef CONFIG_COMMON_RT_API
	rt_port_phyPowerDown_set(port, ENABLED);
#else
	rtk_port_phyPowerDown_set(port, ENABLED);
#endif
#else
	rtk_port_phyReg_get(port, 0, 0, &value);
	value |= 0x0800; /* Power down bit in standard PHY standard register */
	rtk_port_phyReg_set(port, 0, 0, value);
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	//Inform OMCI
	//printk("queue_broadcast is called to alarm\n");
	queue_broadcast(MSG_TYPE_RLDP_LOOP_STATE_CHNG, 1/*RLDP_STS_LOOP_DETECTED*/, port, ENABLED);
#endif
    printk("Loopback detected intf %d \n" , intf);
#endif
}

void rtk_loopback_enable_intf(int intf)
{
#if defined(CONFIG_RTL8686_SWITCH)
    //uint32 value;
    rtk_port_t port = -1;

	port = (rtk_port_t)get_hw_port_id(intf);
	if(port == -1)
    {
		printk("%s: get_hw_port_id failed!\n", __FUNCTION__);
		return;
    }

#if 1
#ifdef CONFIG_COMMON_RT_API
	rt_port_phyPowerDown_set(port, DISABLED);
#else
	rtk_port_phyPowerDown_set(port, DISABLED);
#endif
#else
    rtk_port_phyReg_get(port, 0, 0, &value);
    value &= ~(0x0800);	/* Power down bit in standard PHY standard register */
    rtk_port_phyReg_set(port, 0, 0, value);
#endif
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
	//Inform OMCI
	//printk("queue_broadcast is called to alarm\n");
	queue_broadcast(MSG_TYPE_RLDP_LOOP_STATE_CHNG, 0/*RLDP_STS_LOOP_REMOVED*/, port, ENABLED);
#endif
	printk("Disable loopback intf %d \n" , intf);
#endif
}

struct proc_dir_entry *procLoopdetect= NULL;
struct proc_dir_entry *procLoopdetectEthType= NULL;
struct proc_dir_entry *procLoopdetectStatus= NULL;
int loopdetect_start = 0;
unsigned short loop_eth_type=0xfffa;
unsigned short loop_intf = 0x0;
unsigned short back_loop_intf = 0x0;
#define LOOP_PAIR_NUM 10
int loop_pair[LOOP_PAIR_NUM]={-1};
#if defined(CONFIG_WLAN)
unsigned short wlan_loop_intf = 0x0;
#endif
static void reset_loop_intf(void)
{
	int i = 0, port_loop = 0;
	loop_intf |= back_loop_intf;
	if(loop_intf == 0) return ;
	for(i=0;i<(sizeof(loop_intf)*8);i++)
	{
		port_loop = (loop_intf & 1<<i);
		if(port_loop)
			rtk_loopback_enable_intf(i);
	}
	loop_intf = 0x0;
	back_loop_intf = 0x0;
	memset(loop_pair, -1, sizeof(int)*LOOP_PAIR_NUM);
#if defined(CONFIG_WLAN)
	wlan_loop_intf = 0x0;
#endif
}

static int loop_detect_read(struct seq_file *seq, void *v)
{
	seq_printf(seq, "%hu\n", loop_intf);
	back_loop_intf |= loop_intf;
	loop_intf = 0;
#if defined(CONFIG_WLAN)
	seq_printf(seq, "%hu\n", wlan_loop_intf);
	wlan_loop_intf = 0x0;
#endif
	return 0;
}

static int loop_detect_open(struct inode *inode, struct file *file)
{
	return single_open(file, loop_detect_read, inode->i_private);
}


static int loop_detect_write(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
	char buffer[64] = {0};

	if (buf && !copy_from_user(buffer, buf, count))
	{	//call from shell
		#define REG32(reg)		(*(volatile unsigned int   *)((unsigned int)reg))
		if(memcmp(buffer , "startbyoam" , 10) == 0 )
		{
			reset_loop_intf();
			loopdetect_start = 2;
			loop_intf = 0x0;
			back_loop_intf = 0x0;
			memset(loop_pair, -1, sizeof(int)*LOOP_PAIR_NUM);
#if defined(CONFIG_WLAN)
			wlan_loop_intf = 0x0;
#endif
			printk("oam enable loopback detect \n");
		}
		else if(memcmp(buffer , "start" , 5) == 0 )
		{
			reset_loop_intf();
			loopdetect_start = 1;
			//loopback_detectd = 0;
			loop_intf = 0x0;
			back_loop_intf = 0x0;
			memset(loop_pair, -1, sizeof(int)*LOOP_PAIR_NUM);
#if defined(CONFIG_WLAN)
			wlan_loop_intf = 0x0;
#endif
			printk("enable loopback detect \n");
		}
		else if(memcmp(buffer, "enableg" , 7) == 0)
		{
			enable_irq(26);
		}
		else if(memcmp(buffer, "disableg" , 8) == 0)
		{
			disable_irq(26);
		}
		else if(memcmp(buffer, "stop" , 4) == 0)
		{
			reset_loop_intf();
			loopdetect_start = 0;
			printk("disable loopback detect \n");
		}
		else if(memcmp(buffer , "enableport" , 10) == 0 )
		{
			if(strlen(buffer) >= 12 && memcmp(buffer+11, "eth0.", 5) == 0 && (*(buffer+16) - '0') >= 0)
			{
				if((back_loop_intf & (1 << (*(buffer+16) - '0' - 2))))
				{
					rtk_loopback_enable_intf((*(buffer+16) - '0' - 2));
					back_loop_intf &= ~(1 << (*(buffer+16) - '0' - 2));
					//loop_intf &= ~(1 << (*(buffer+16) - '0' - 2));
				}
			}
		}
		else
		{
			printk("buffer:%s , len :%d \n" , buffer , strlen(buffer));
			return -EFAULT;
		}
	}
	return count;
}

static const struct proc_ops loopdetect_ops = {
	.proc_open				= loop_detect_open,
	.proc_read				= seq_read,
	.proc_write				= loop_detect_write,
	.proc_lseek				= seq_lseek,
	.proc_release			= single_release,
};

static int loop_detect_etype_read(struct seq_file *seq, void *v)
{
	seq_printf(seq, "loop eth_type=0x%x\n", loop_eth_type);
	seq_printf(seq, "loopdetect_start=%d\n", loopdetect_start);
	return 0;
}

static int loop_detect_etype_write(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
	char tmpbuf[16] = {0};
	int len = (count > 15) ? 15 : count;

	if (buf && !copy_from_user(tmpbuf, buf, count))
	{
		loop_eth_type = (unsigned short)simple_strtoul(tmpbuf, NULL, 16);
		printk("loop_eth_type=0x%x\n",loop_eth_type);
		return count;
	}
	return -EFAULT;

}

static int loop_detect_etype_open(struct inode *inode, struct file *file)
{
	return single_open(file, loop_detect_etype_read, inode->i_private);
}

static const struct proc_ops loopdetect_ethtype_ops = {
	.proc_open		= loop_detect_etype_open,
	.proc_read		= seq_read,
	.proc_write		= loop_detect_etype_write,
	.proc_lseek 	= seq_lseek,
	.proc_release	= single_release,
};

static int loop_detect_status_read(struct seq_file *seq, void *v)
{
	int i = 0;
	unsigned short loop_intf_bitmask = 0x0;
	
	for(i=0 ; i < LOOP_PAIR_NUM; i++){
		if(loop_pair[i] != -1){
			loop_intf_bitmask = 0x0;
			loop_intf_bitmask |= (1 << i);
			loop_intf_bitmask |= (1 << loop_pair[i]);
			seq_printf(seq, "loop_intf_bitmask=%d\n", loop_intf_bitmask);
		}
	}

	return 0;
}

static int loop_detect_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, loop_detect_status_read, inode->i_private);
}

static const struct proc_ops loopdetect_status_ops = {
	.proc_open		= loop_detect_status_open,
	.proc_read		= seq_read,
	.proc_write		= seq_write,
	.proc_lseek 	= seq_lseek,
	.proc_release	= single_release,
};

#if defined(CONFIG_RTL8686_SWITCH)
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
static struct ctl_table loopdetect_table[] = {
	{
		.procname		= "lan_port_remapping",
		.data			= lan_port_remapping,
		.maxlen 		= 4*sizeof(int),
		.mode			= 0644,
		.proc_handler	= proc_dointvec,
	},
	{}
};

static struct ctl_table loopdetect_root[] = {
	{
        .procname       = "loopback_detect",
        .mode           = 0555,
        .child          = loopdetect_table,
	},
	{}
};

static struct ctl_table_header *loopdetect_tbl_hdr = NULL;
#endif
#endif
#if defined(CONFIG_WLAN)
static DECLARE_WAIT_QUEUE_HEAD(lbd_kick_wait);
static DEFINE_SPINLOCK(lbd_kick_lock);
struct task_struct *pLbdTask;
int wlan_loop_ifnum = 0;

int lbd_trigger_thread(void *data)
{
	while (true) {
		wait_event_freezable(lbd_kick_wait, wlan_loop_ifnum != 0);
		
		spin_lock(&lbd_kick_lock);
		if (wlan_loop_ifnum) {
			char cmdStr[128];
			char *argv[] = {
				"/bin/sh",
				"-c",
				cmdStr,
				NULL
			};
			char *envp[] = {
				"HOME=/",
				NULL
			};
			int rc;
			char ifname[32];
			
#if defined(CONFIG_CU_FTTR_MASTER)
			snprintf(ifname, sizeof(ifname), "wlan%d-vap0", wlan_loop_ifnum-16);
			snprintf(cmdStr, sizeof(cmdStr), "/bin/hostapd_cli -i %s deauthenticate ff:ff:ff:ff:ff:ff;/bin/iwpriv %s set_mib func_off=1", ifname, ifname);
#else
			snprintf(ifname, sizeof(ifname), "wlan%d-vxd", wlan_loop_ifnum-16);
			snprintf(cmdStr, sizeof(cmdStr), "/bin/iwpriv %s vxd_disassoc; ifconfig %s down", ifname, ifname);
#endif
			/* call userspace STP and report program errors */
			printk("[%s] call %s!\n", __func__, argv[2]);
			rc = call_usermodehelper(argv[0], argv, envp, UMH_KILLABLE);
			if (rc > 0) {
				if (rc & 0xff)
					printk("%s %s received signal %d\n", __func__, argv[2], rc & 0x7f);
				else
					printk("%s %s exited with code %d\n", __func__, argv[2], (rc >> 8) & 0xff);
			}
			else {
				printk("%s %s cmd ret=%d\n", __func__, argv[2], rc);
			}
			
			wlan_loop_ifnum = 0;
		}
		spin_unlock(&lbd_kick_lock);
	}

	return 0;
}
#endif
unsigned int rtk_loopdetect_by_skb_in_bridge(struct sk_buff *skb,
				      struct net_bridge_port *p)
{
	struct net_device *loop_dev;
	struct lbd_data *lbd_h;

	if (p == NULL)
		return NF_ACCEPT;

	loop_dev = p->br->dev;
	if(loop_dev != NULL)
	{
#if defined(CONFIG_CU_FTTR)
		if(skb->protocol == htons(loop_eth_type) 
			&& (!memcmp(loop_dev->dev_addr,eth_hdr(skb)->h_source,ETH_ALEN) || !memcmp(p->dev->dev_addr,eth_hdr(skb)->h_source,ETH_ALEN)))
#else
		if(skb->protocol == htons(loop_eth_type) && !memcmp(loop_dev->dev_addr,eth_hdr(skb)->h_source,ETH_ALEN))
#endif
		{
			//unsigned char *eth = eth_hdr(skb)->h_source;
			//printk("br=%02x:%02x:%02x:%02x:%02x:%02x\n",loop_dev->dev_addr[0],loop_dev->dev_addr[1],loop_dev->dev_addr[2],loop_dev->dev_addr[3],loop_dev->dev_addr[4],loop_dev->dev_addr[5]);
			//printk("eth=%02x:%02x:%02x:%02x:%02x:%02x\n",eth[0],eth[1],eth[2],eth[3],eth[4],eth[5]);
			if((loopdetect_start == 1 || loopdetect_start == 2)&& (loop_intf == 0) && skb->protocol == htons(loop_eth_type))
			{
#ifdef CONFIG_RTL_MULTI_LAN_DEV
				lbd_h = (struct lbd_data *)skb_network_header(skb);
#if defined(CONFIG_CU_FTTR)
#if defined(CONFIG_WLAN)
				int wlan_ifnum=-1;
				if((lbd_h->ifnum > LOOP_PAIR_NUM)
#if defined(CONFIG_CU_FTTR_MASTER)
					|| (memcmp(p->dev->name , "wlan", 4) == 0 && sscanf(p->dev->name, "wlan%d-vap0", &wlan_ifnum) == 1)
#else
					|| (memcmp(p->dev->name , "wlan", 4) == 0 && sscanf(p->dev->name, "wlan%d-vxd", &wlan_ifnum) == 1)
#endif
					)
				{
					printk("%s %d -->received packet on %s(intf %d) eth-type=0x%X loop-intf=%d skb->dev->name %s with own address as source address from intf %d\n",__func__,__LINE__,p->dev->name, (p->dev->name[5] - '0' - 2), htons(skb->protocol),loop_intf, skb->dev->name, lbd_h->ifnum);
					if (spin_trylock(&lbd_kick_lock)) {
						if(wlan_ifnum >= 0)
							wlan_loop_ifnum = (wlan_ifnum+16);
						else
							wlan_loop_ifnum = lbd_h->ifnum;
						wlan_loop_intf |= (1 << (wlan_loop_ifnum-16));
						wake_up(&lbd_kick_wait);
						spin_unlock(&lbd_kick_lock);
					}
				}
				else
#endif
				if(memcmp(p->dev->name , "eth0.", 5) == 0 && (p->dev->name[5] - '0') >= 0)
				{
					unsigned char ifnum;
					ifnum = (p->dev->name[5] - '0' - 2);
					if(ifnum >= 4 && lbd_h->ifnum <4)
					{
						if(!(loop_intf & (1 << lbd_h->ifnum)))
						{
							loop_intf |= (1 << lbd_h->ifnum);
							rtk_loopback_disable_intf(lbd_h->ifnum);
							if(lbd_h->ifnum < LOOP_PAIR_NUM){
								loop_pair[lbd_h->ifnum] = ifnum;
							}
						}
					}
					else if(!(loop_intf & (1 << ifnum)))
					{
						loop_intf |= (1 << ifnum);
						rtk_loopback_disable_intf(ifnum);
						if(ifnum < LOOP_PAIR_NUM){
							loop_pair[ifnum] = lbd_h->ifnum;
						}
					}
					printk("%s %d -->received packet on %s(intf %d) eth-type=0x%X loop-intf=%d skb->dev->name %s with own address as source address from intf %d\n",__func__,__LINE__,p->dev->name, ifnum, htons(skb->protocol),loop_intf, skb->dev->name, lbd_h->ifnum);
				}
				else
				{
					if(!(loop_intf & (lbd_h->ifnum)))
					{
						loop_intf |= (1 << lbd_h->ifnum);
						rtk_loopback_disable_intf(lbd_h->ifnum);
						printk("%s %d -->received packet on %s(intf %d) eth-type=0x%X loop-intf=%d skb->dev->name %s with own address as source address from intf %d\n",__func__,__LINE__,p->dev->name, lbd_h->ifnum, htons(skb->protocol),loop_intf, skb->dev->name, lbd_h->ifnum);
					}
				}
#else
				if(memcmp(p->dev->name , "eth0.", 5) == 0 && (p->dev->name[5] - '0') >= 0)
				{
					unsigned char ifnum;
					ifnum = (p->dev->name[5] - '0' - 2);
					if (lbd_h->ifnum > LOOP_PAIR_NUM)
					{
						printk("%s %d -->received packet on %s(intf %d) eth-type=0x%X loop-intf=%d skb->dev->name %s with own address as source address from intf %d\n",__func__,__LINE__,p->dev->name, (p->dev->name[5] - '0' - 2), htons(skb->protocol),loop_intf, skb->dev->name, lbd_h->ifnum);
#if defined(CONFIG_WLAN)
						if (spin_trylock(&lbd_kick_lock)) {
							wlan_loop_ifnum = lbd_h->ifnum;
							wlan_loop_intf |= (1 << (wlan_loop_ifnum-16));
							wake_up(&lbd_kick_wait);
							spin_unlock(&lbd_kick_lock);
						}
#endif
					}
					else
					{
						if(!(loop_intf & (1 << ifnum)))
						{
							loop_intf |= (1 << ifnum);
							rtk_loopback_disable_intf(ifnum);
							if(ifnum < LOOP_PAIR_NUM){
								loop_pair[ifnum] = lbd_h->ifnum;
							}
						}
						printk("%s %d -->received packet on %s(intf %d) eth-type=0x%X loop-intf=%d skb->dev->name %s with own address as source address from intf %d\n",__func__,__LINE__,p->dev->name, ifnum, htons(skb->protocol),loop_intf, skb->dev->name, lbd_h->ifnum);
					}
				}
				else if (!memcmp(p->dev->name, "nas0", 4))
				{
					lbd_h = (struct lbd_data *)skb_network_header(skb);
					if (lbd_h->ifnum > LOOP_PAIR_NUM)
					{
						printk("-->received packet on %s eth-type=0x%X loop-intf=%d skb->dev->name %s with own address as source address from intf %d\n",
								p->dev->name, htons(skb->protocol),loop_intf, skb->dev->name, lbd_h->ifnum);
#if defined(CONFIG_WLAN)
						if (spin_trylock(&lbd_kick_lock)) {
							wlan_loop_ifnum = lbd_h->ifnum;
							wake_up(&lbd_kick_wait);
							spin_unlock(&lbd_kick_lock);
						}
#endif
					}
				}
#endif
#else
				if(strcmp(p->dev->name, "eth0") == 0)
				{
					loop_intf |= 1;
				}
				printk("-->received packet on %s eth-type=0x%X loop-intf=%d with own address as source address\n",p->dev->name, htons(skb->protocol),loop_intf);
#endif
			}
		}
	}

	return NF_ACCEPT;
}
EXPORT_SYMBOL(rtk_loopdetect_by_skb_in_bridge);

static unsigned int rtk_loopdetect_pre_routing(void *priv,
				      struct sk_buff *skb,
				      const struct nf_hook_state *state)
{
	struct net_bridge_port *p;

	p = br_port_get_rcu(state->in);

	return rtk_loopdetect_by_skb_in_bridge(skb, p);
}
static struct nf_hook_ops rtk_loopdetect_hook = {
	.hook = rtk_loopdetect_pre_routing,
	.pf = NFPROTO_BRIDGE,
	.hooknum = NF_BR_PRE_ROUTING,
	.priority = NF_BR_PRI_FIRST,
};
#endif

static int __init rtk_loopdetect_init(void)
{

#if defined(CONFIG_LUNA)
	procLoopdetect= proc_create("loopback_detect", 0644, NULL, &loopdetect_ops);
	procLoopdetectEthType= proc_create("loopback_detect_ethtype", 0644, NULL, &loopdetect_ethtype_ops);
	procLoopdetectStatus= proc_create("loopback_detect_status", 0644, NULL, &loopdetect_status_ops);

#if defined(CONFIG_RTL8686_SWITCH)
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
	loopdetect_tbl_hdr = register_sysctl_table(loopdetect_root);
#endif
#endif
#if defined(CONFIG_NETFILTER_FAMILY_BRIDGE)
	if (nf_register_net_hook(&init_net, &rtk_loopdetect_hook) != 0 ) {
		printk(KERN_WARNING "rtk_loopdetect register hook error!\n");
	}
#endif
#endif
#if defined(CONFIG_WLAN)
	pLbdTask = kthread_run(lbd_trigger_thread, NULL, "lbd_kick");
	if (!pLbdTask || IS_ERR(pLbdTask)) {
		pr_err("%s Setting up thread lbd_kick failed\n", __func__);
		pLbdTask = NULL;
	}
#endif
	return 0;
}

static void __exit rtk_loopdetect_deinit(void)
{
#if defined(CONFIG_LUNA)
#if defined(CONFIG_NETFILTER_FAMILY_BRIDGE)
	nf_unregister_net_hook(&init_net, &rtk_loopdetect_hook);
#endif

	if(procLoopdetect)
	{
		remove_proc_entry("loopback_detect", NULL);
		procLoopdetect = NULL;
	}
	if(procLoopdetectEthType)
	{
		remove_proc_entry("loopback_detect_ethtype", NULL);
		procLoopdetectEthType = NULL;
	}
	if(procLoopdetectStatus)
	{
		remove_proc_entry("loopback_detect_status", NULL);
		procLoopdetectStatus = NULL;
	}
#if defined(CONFIG_RTL8686_SWITCH)
#if defined(CONFIG_RTL9607C_SERIES) || defined(CONFIG_LUNA_G3_SERIES)
	if(loopdetect_tbl_hdr){
		unregister_sysctl_table(loopdetect_tbl_hdr);
		loopdetect_tbl_hdr = NULL;
	}
#endif
#endif
#endif
}
module_init(rtk_loopdetect_init)
module_exit(rtk_loopdetect_deinit)
MODULE_LICENSE("GPL");

