#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <uapi/linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/types.h>
#include <linux/inet.h>
#include <net/rtl/rtl_types.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/spinlock_types.h>

#define DRIVER_AUTHOR 	"ql"
#define DRIVER_DESC 	"Traffic Mirror"

extern struct proc_dir_entry *realtek_proc;
struct net_device *mirrored_dev=NULL, *mirroring_dev=NULL;

#define PROCFS_TRAFFIC_MIRROR		"traffic_mirror"

void rtk_mirror_tx(struct sk_buff *skb)
{
	unsigned int size = skb_end_offset(skb) + skb->data_len;
	int headerlen = skb_headroom(skb);
	struct sk_buff *skb2;

	if (!mirroring_dev || !mirrored_dev)
		return;

	if (skb->dev != mirrored_dev)
		return;

	skb2 = dev_alloc_skb(size);
	if (skb2) {
		/* Set the data pointer */
		skb_reserve(skb2, headerlen);
		/* Set the tail pointer and length */
		skb_put(skb2, skb->len);
	
		memcpy(skb2->data, skb->data, skb->len);
		skb2->dev = mirroring_dev;

		skb2->dev->netdev_ops->ndo_start_xmit(skb2, skb2->dev);
	}
}

EXPORT_SYMBOL(rtk_mirror_tx);

void rtk_mirror_rx(struct sk_buff *skb)
{
	unsigned int size = skb_end_offset(skb) + skb->data_len;
	int headerlen = skb_headroom(skb);
	struct sk_buff *skb2;

	if (!mirroring_dev || !mirrored_dev)
		return;

	if (skb->dev != mirrored_dev)
		return;
	
	skb2 = dev_alloc_skb(size);
	if (skb2) {
		/* Set the data pointer */
		skb_reserve(skb2, headerlen);
		/* Set the tail pointer and length */
		skb_put(skb2, skb->len);

		skb_push(skb2, ETH_HLEN);
		memcpy(skb2->data, skb->data-ETH_HLEN, skb->len+ETH_HLEN);
		
		skb2->dev = mirroring_dev;

		skb2->dev->netdev_ops->ndo_start_xmit(skb2, skb2->dev);
	}
}

EXPORT_SYMBOL(rtk_mirror_rx);

static int proc_traffic_mirror_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[512] = "\0", *strptr = NULL, *split_str = NULL, *val;
	struct net_device *dev = NULL;
	
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		if((val = strchr(tmpbuf, '\r'))) *val='\0';
		if((val = strchr(tmpbuf, '\n'))) *val='\0';

		strptr = tmpbuf;
		while(strptr != NULL)
		{
			split_str=strsep(&strptr," ");
			if (!strcmp(split_str, "mirroring"))
			{
				if (strptr == NULL) break;
				split_str = strsep(&strptr, " ");

				if (!strcasecmp(split_str, "null")) {
					if (mirroring_dev)
						dev_put(mirroring_dev);
					mirroring_dev = NULL;

					if (mirrored_dev)
						dev_put(mirrored_dev);
					mirrored_dev = NULL;
					
					return count;
				}
				
				dev = dev_get_by_name(&init_net, split_str);
				if(!dev)
				{
					printk("error device %s!\n", split_str);
					return -1;
				}
				
				if (mirroring_dev)
					dev_put(mirroring_dev);
				mirroring_dev = dev;
			}
			else if (!strcmp(split_str, "mirrored"))
			{
				if (strptr == NULL) break;
				split_str = strsep(&strptr, " ");
				
				dev = dev_get_by_name(&init_net, split_str);
				if(!dev)
				{
					printk("error device %s!\n", split_str);
					return -1;
				}
				
				if (mirrored_dev)
					dev_put(mirrored_dev);
				mirrored_dev = dev;
			}
			else {
				printk("echo \"mirroring wlan0 mirrored eth0.2\" > proc/realtek/traffic_mirror\n");
				return -1;
			}
		}
		return count;
	}
	return count;
}

static int proc_traffic_mirror_read(struct seq_file *seq, void *v)
{
	if (mirroring_dev && mirrored_dev)
		printk("mirror from %s to %s\n", mirrored_dev->name, mirroring_dev->name);
	else
		printk("no mirror\n");
	
	return 0;
}

static int proc_traffic_mirror_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_traffic_mirror_read, inode->i_private);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
static const struct proc_ops traffic_mirror_fops = {
        .proc_open           = proc_traffic_mirror_open,
        .proc_write          = proc_traffic_mirror_write,
        .proc_read           = seq_read,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};
#else
static const struct file_operations traffic_mirror_fops = {
	    .owner          = THIS_MODULE,
	    .open           = proc_traffic_mirror_open,
	    .read           = seq_read,
	    .write          = proc_traffic_mirror_write,
	    .llseek         = seq_lseek,
	    .release        = single_release,
};
#endif

static int __init init_main(void)
{
	struct proc_dir_entry *entry = NULL;
	
	entry = proc_create(PROCFS_TRAFFIC_MIRROR, 0644, realtek_proc, &traffic_mirror_fops);
	if ( entry == NULL){
		printk("%s: Register to /proc/realtek/%s failed \n", __FUNCTION__, PROCFS_TRAFFIC_MIRROR);
	}

	return 0;
}

static void __exit cleanup_main(void)
{
    remove_proc_entry(PROCFS_TRAFFIC_MIRROR, realtek_proc);
}

module_init(init_main);
module_exit(cleanup_main);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

