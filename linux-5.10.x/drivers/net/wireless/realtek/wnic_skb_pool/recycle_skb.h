#ifndef _WNIC_RECYCLESKB_H_
#define _WNIC_RECYCLESKB_H_

#include <linux/skbuff.h>
#include <linux/interrupt.h>
#ifdef CONFIG_RTK_PTOOL_CPU_PERF
#include <linux/ptool.h>		/* performance */
#endif
#include <linux/kernel.h>		/* dump_stack */
#include <linux/mm.h>			/* get total memory size */
#include <linux/vmstat.h>
#include <linux/highmem.h>
#include <linux/netdevice.h>	/* dev_kfree */
#include <common/rt_type.h>
#include "module/intr_bcaster/intr_bcaster.h"

#define WNIC_RECYCLE_SKB_PROC	1

enum {
	WNIC_RECYCLE_SWITCH_ON = 0x0,
	WNIC_RECYCLE_SWITCH_OFF = 0x1,
};

#define WNIC_RESERVED_HEADER_SIZE 0

#if defined(WNIC_RING_BUFFER_RECYCLE_SKB)
enum {
	WNIC_RING_BUF_RECYCLE_NORMAL = 0x0,
	WNIC_RING_BUF_RECYCLE_EMPTY,
	WNIC_RING_BUF_RECYCLE_FULL,
};

struct wnic_ring_buffer_recycle_t {
	unsigned int rd_idx;
	unsigned int wr_idx;
	unsigned int state;	/* 0: normal, 1:empty, 2:full */
	unsigned int ring_size_mask;

	struct sk_buff **ring_buffer_recycle_array;
	spinlock_t	lock;
};
#endif
#if defined(WNIC_STRAIGHT_ARRAY_RECYCLE_SKB)
struct wnic_straight_recycle_t {
	unsigned int idx;
	struct sk_buff **array;
	spinlock_t	lock;
};
#endif

#if defined(CONFIG_PREEMPT)
#define WNIC_RECYCLE_SPINLOCK(x) spin_lock_bh(x)
#define WNIC_RECYCLE_SPINUNLOCK(x) spin_unlock_bh(x)
#else
#define WNIC_RECYCLE_SPINLOCK(x) spin_lock(x)
#define WNIC_RECYCLE_SPINUNLOCK(x) spin_unlock(x)
#endif

#if defined(CONFIG_LUNA_G3_SERIES)
#define __wnic_recycle_rtl_dev_alloc_skb(size)		__netdev_alloc_skb(NULL, (size), GFP_ATOMIC | GFP_DMA)
#else
#define __wnic_recycle_rtl_dev_alloc_skb(size)		__netdev_alloc_skb(NULL, (size) + RTL_RESERVED_HEADER_SIZE, GFP_ATOMIC | GFP_DMA)
#endif

int init_recycle_wnic_skb_buf(unsigned int ring_size, unsigned int max_size);
void deinit_recycle_wnic_skb_buf(int pool);
struct sk_buff *dev_alloc_skb_recy_wnic(unsigned int size, int pool);
struct sk_buff *wnic_recycle_skb_swap(struct sk_buff *skb);
unsigned int wnic_free_eth_skb_num(int pool);
unsigned int wnic_alloc_eth_skb_num(int pool);

extern int min_free_kbytes;
extern struct proc_dir_entry *realtek_proc;

static inline void __init_skb_recy_wnic(struct sk_buff *skb)
{
	skb->data = skb->head + WNIC_RESERVED_HEADER_SIZE;		/* reserve header room */
	skb_reset_tail_pointer(skb);
}

static inline void __wnic_recycle_skb_copy(struct sk_buff *skb, struct sk_buff *new_skb)
{
	unsigned char *head;
	sk_buff_data_t end;
	unsigned int truesize;
	bool pfmemalloc, cloned;

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	if (skb->fcIngressData.doLearning)
		memcpy(&new_skb->fcIngressData, &skb->fcIngressData, sizeof(rtk_fc_ingress_data_t));
#endif
	head = new_skb->head;
	end = new_skb->end;
	truesize = new_skb->truesize;
	cloned = new_skb->cloned;
	pfmemalloc = new_skb->pfmemalloc;

	new_skb->head = skb->head;
	new_skb->data = skb->data;
	new_skb->tail = skb->tail;
	new_skb->end = skb->end;
	new_skb->truesize = skb->truesize;
	new_skb->cloned = skb->cloned;
	new_skb->pfmemalloc = skb->pfmemalloc;
	new_skb->protocol = skb->protocol;
	new_skb->mac_header = skb->mac_header;
	new_skb->len = skb->len;
	new_skb->dev = skb->dev;
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
	new_skb->priority = skb->priority;
#endif
	new_skb->mark = skb->mark;
#ifdef CONFIG_RTK_SKB_MARK2
	new_skb->mark2 = skb->mark2;
#endif
#ifdef CONFIG_RTL8192CD
	new_skb->cb[0] = skb->cb[0];
#endif
#ifdef CONFIG_SKB_EXTENSIONS
	/* only useable after checking ->active_extensions != 0 */
	new_skb->extensions = skb->extensions;
	new_skb->active_extensions = skb->active_extensions;
#endif
	/* keep original ip checksum status to protocol stack */
	new_skb->ip_summed = skb->ip_summed;

	skb->head = head;
	skb->end = end;
	skb->truesize = truesize;
	skb->cloned = cloned;
	skb->pfmemalloc = pfmemalloc;
	/*for jumbo packet should clear it here*/
	skb->data_len = 0;
}


#endif /*_WNIC_RECYCLESKB_H_*/
