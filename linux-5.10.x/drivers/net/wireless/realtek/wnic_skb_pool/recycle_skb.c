#ifdef CONFIG_WNIC_RECYCLED_SKB

#include "recycle_conf.h"
#include "recycle_skb.h"

#if defined(WNIC_RING_BUFFER_RECYCLE_SKB)
static __cacheline_aligned_in_smp struct wnic_ring_buffer_recycle_t wrbr[1 << WNIC_POOL_NUM_SHIFT_BITS];
#endif
#if defined(WNIC_DOUBLE_LINK_LIST_RECYCLE_SKB) || defined(WNIC_SINGLE_LINK_LIST_RECYCLE_SKB)
static __cacheline_aligned_in_smp struct sk_buff_head wnic_recycle_skb_queue[1 << WNIC_POOL_NUM_SHIFT_BITS];
#endif
#if defined(WNIC_STRAIGHT_ARRAY_RECYCLE_SKB)
static __cacheline_aligned_in_smp struct wnic_straight_recycle_t wsra[1 << WNIC_POOL_NUM_SHIFT_BITS];
#endif
#if defined(WNIC_LL_LINK_LIST_RECYCLE_SKB)
static __cacheline_aligned_in_smp spinlock_t wnic_llll_read_lock[1 << WNIC_POOL_NUM_SHIFT_BITS];
static __cacheline_aligned_in_smp struct llist_head wnic_llll_skb_head[1 << WNIC_POOL_NUM_SHIFT_BITS];
static __cacheline_aligned_in_smp atomic_t wnic_llll_num[1 << WNIC_POOL_NUM_SHIFT_BITS];
static __cacheline_aligned_in_smp atomic_t wnic_llll_nomem[1 << WNIC_POOL_NUM_SHIFT_BITS];
#endif

static unsigned int wnic_skb_total_num[1 << WNIC_POOL_NUM_SHIFT_BITS] = {0};
static unsigned int wnic_skb_free_num[1 << WNIC_POOL_NUM_SHIFT_BITS] = {0};
static unsigned int wnic_skb_max_size[1 << WNIC_POOL_NUM_SHIFT_BITS] = {0};
#if defined(CONFIG_WNIC_RECYCLED_SKB_ALLOC_MAX)
static unsigned int wnic_skb_max_allocate[1 << WNIC_POOL_NUM_SHIFT_BITS] = {0};
#endif
//static unsigned int wnic_switch_off_recycle_skb[1<<WNIC_POOL_NUM_SHIFT_BITS]={WNIC_RECYCLE_SWITCH_ON};
static unsigned int wnic_pool[1 << WNIC_POOL_NUM_SHIFT_BITS] = {0};
#if defined(WNIC_RING_BUFFER_RECYCLE_SKB)
unsigned int wnic_free_eth_skb_num(void)
{
	if (wrbr.state == RTL_RING_BUF_RECYCLE_FULL) {
		return wrbr.ring_size_mask + 1;
	} else if (wrbr.state == RTL_RING_BUF_RECYCLE_EMPTY) {
		return 0;
	} else {
		if (wrbr.wr_idx > wrbr.rd_idx)
			return wrbr.wr_idx - wrbr.rd_idx;
		else
			return wrbr.ring_size_mask + 1 - wrbr.rd_idx + wrbr.wr_idx;
	}
}

unsigned int wnic_alloc_eth_skb_num(void)
{
	if (wrbr.state == RTL_RING_BUF_RECYCLE_FULL) {
		return 0;
	} else if (wrbr.state == RTL_RING_BUF_RECYCLE_EMPTY) {
		return wrbr.ring_size_mask + 1;
	} else {
		if (wrbr.wr_idx > wrbr.rd_idx)
			return wrbr.ring_size_mask + 1 - wrbr.wr_idx + wrbr.rd_idx;
		else
			return wrbr.rd_idx - wrbr.wr_idx;
	}
}

__always_inline
void wnic_skb_queue_head(struct sk_buff *newsk)
{
	WNIC_RECYCLE_SPINLOCK(&wrbr.lock);
	//printk("[ENQ]wrbr.wr_idx=%d, rd_idx=%d, state=%d\n",wrbr.wr_idx,wrbr.rd_idx,wrbr.state);
	if (likely(wrbr.state != RTL_RING_BUF_RECYCLE_FULL)) {
		wrbr.ring_buffer_recycle_array[wrbr.wr_idx] = newsk;
		wrbr.wr_idx = (wrbr.wr_idx + 1) & wrbr.ring_size_mask;
		smp_mb();
		if (unlikely(wrbr.wr_idx == wrbr.rd_idx))
			wrbr.state = RTL_RING_BUF_RECYCLE_FULL;
		else
			wrbr.state = RTL_RING_BUF_RECYCLE_NORMAL;
		//printk("update wr_idx to %d!! state become to %d\n",wrbr.wr_idx,wrbr.state);
	} else {
		printk("abnormal!! free the skb as dynamically one..\n");
		newsk->recyclable = 0;
		dev_kfree_skb_any(newsk);
	}
	WNIC_RECYCLE_SPINUNLOCK(&wrbr.lock);
}

__always_inline
struct sk_buff *wnic_skb_dequeue(void)
{
	struct sk_buff *skb = NULL;

	WNIC_RECYCLE_SPINLOCK(&wrbr.lock);
	//printk("[DEQ]wrbr.wr_idx=%d, rd_idx=%d, state=%d\n",wrbr.wr_idx,wrbr.rd_idx,wrbr.state);
	if (likely(wrbr.state != RTL_RING_BUF_RECYCLE_EMPTY)) {
		skb = wrbr.ring_buffer_recycle_array[wrbr.rd_idx];
		wrbr.rd_idx = (wrbr.rd_idx + 1) & wrbr.ring_size_mask;
		smp_mb();
		if (unlikely(wrbr.wr_idx == wrbr.rd_idx))
			wrbr.state = RTL_RING_BUF_RECYCLE_EMPTY;
		else
			wrbr.state = RTL_RING_BUF_RECYCLE_NORMAL;
		//printk("update rd_idx to %d!! state become to %d\n",wrbr.rd_idx,wrbr.state);
	}
	WNIC_RECYCLE_SPINUNLOCK(&wrbr.lock);
	return skb;
}
#endif
#if defined(WNIC_DOUBLE_LINK_LIST_RECYCLE_SKB)
unsigned int wnic_free_eth_skb_num(void)
{
	return wnic_recycle_skb_queue.qlen;
}

unsigned int wnic_alloc_eth_skb_num(void)
{
	return wnic_skb_free_num - wnic_recycle_skb_queue.qlen;
}

__always_inline
void wnic_skb_queue_head(struct sk_buff *newsk)
{
	WNIC_RECYCLE_SPINLOCK(&wnic_recycle_skb_queue.lock);
	__skb_queue_head(&wnic_recycle_skb_queue, newsk);
	WNIC_RECYCLE_SPINUNLOCK(&wnic_recycle_skb_queue.lock);
}

__always_inline
struct sk_buff *wnic_skb_dequeue(void)
{
	struct sk_buff *skb = NULL;

	WNIC_RECYCLE_SPINLOCK(&wnic_recycle_skb_queue.lock);
	skb = __skb_dequeue(&wnic_recycle_skb_queue);
	WNIC_RECYCLE_SPINUNLOCK(&wnic_recycle_skb_queue.lock);
	return skb;
}
#endif
#if defined(WNIC_SINGLE_LINK_LIST_RECYCLE_SKB)
unsigned int wnic_free_eth_skb_num(void)
{
	return wnic_recycle_skb_queue.qlen;
}

unsigned int wnic_alloc_eth_skb_num(void)
{
	return wnic_skb_free_num - wnic_recycle_skb_queue.qlen;
}

__always_inline
void wnic_skb_queue_head(struct sk_buff *newsk)
{
	WNIC_RECYCLE_SPINLOCK(&wnic_recycle_skb_queue.lock);
	newsk->next = wnic_recycle_skb_queue.next;
	newsk->prev = (struct sk_buff *)&wnic_recycle_skb_queue;
	wnic_recycle_skb_queue.next = newsk;
	wnic_recycle_skb_queue.qlen++;
	WNIC_RECYCLE_SPINUNLOCK(&wnic_recycle_skb_queue.lock);
}

__always_inline
void __recycle_skb_peek(struct sk_buff *skb, struct sk_buff_head *list)
{
	struct sk_buff *next, *prev;

	list->qlen--;
	next = skb->next;
	prev = skb->prev;
	next->prev = prev;
	prev->next = next;
}

__always_inline
struct sk_buff *wnic_skb_dequeue(void)
{
	struct sk_buff *skb = NULL;

	WNIC_RECYCLE_SPINLOCK(&wnic_recycle_skb_queue.lock);
	skb = wnic_recycle_skb_queue.next;
	if (likely(skb != (struct sk_buff *)&wnic_recycle_skb_queue))
		__recycle_skb_peek(skb, &wnic_recycle_skb_queue);
	else
		skb = NULL;
	WNIC_RECYCLE_SPINUNLOCK(&wnic_recycle_skb_queue.lock);
	return skb;
}
#endif
#if defined(WNIC_STRAIGHT_ARRAY_RECYCLE_SKB)
unsigned int wnic_free_eth_skb_num(void)
{
	return wsra.idx;
}

unsigned int wnic_alloc_eth_skb_num(void)
{
	return wnic_skb_free_num - wsra.idx;
}

__always_inline
void wnic_skb_queue_head(struct sk_buff *newsk)
{
	WNIC_RECYCLE_SPINLOCK(&wsra.lock);
	wsra.array[wsra.idx] = newsk;
	wsra.idx++;
	WNIC_RECYCLE_SPINUNLOCK(&wsra.lock);
}

__always_inline
struct sk_buff *wnic_skb_dequeue(void)
{
	struct sk_buff *skb = NULL;

	WNIC_RECYCLE_SPINLOCK(&wsra.lock);
	if (likely(wsra.idx)) {
		wsra.idx--;
		skb = wsra.array[wsra.idx];
	}
	WNIC_RECYCLE_SPINUNLOCK(&wsra.lock);
	return skb;
}
#endif
#if defined(WNIC_LL_LINK_LIST_RECYCLE_SKB)
unsigned int wnic_free_eth_skb_num(int pool)
{
	return atomic_read(&wnic_llll_num[pool]);
}

unsigned int wnic_alloc_eth_skb_num(int pool)
{
	return wnic_skb_free_num[pool] - atomic_read(&wnic_llll_num[pool]);
}

__always_inline
void wnic_skb_queue_head(struct sk_buff *newsk, int pool)
{
	llist_add(&newsk->recy_llnode, &wnic_llll_skb_head[pool]);
	atomic_inc(&wnic_llll_num[pool]);
}

__always_inline
struct sk_buff *wnic_skb_dequeue(int pool)
{
	struct llist_node *llll_node;
	struct sk_buff *skb;
	struct spinlock_t *l = &wnic_llll_read_lock[pool];

	WNIC_RECYCLE_SPINLOCK(l);
	llll_node = llist_del_first(&wnic_llll_skb_head[pool]);
	WNIC_RECYCLE_SPINUNLOCK(l);
	if (llll_node == NULL) {
		atomic_inc(&wnic_llll_nomem[pool]);
		return NULL;
	}
	skb = llist_entry(llll_node, struct sk_buff, recy_llnode);
	atomic_dec(&wnic_llll_num[pool]);
	return skb;
}
#endif

#ifdef WNIC_RECYCLE_SKB_PROC
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
struct proc_dir_entry *wnic_recycle_pe;

static int wnic_recycleskb_read(struct seq_file *seq, void *v)
{
	int i;

#if	defined(WNIC_RING_BUFFER_RECYCLE_SKB)
	seq_printf(seq, "\nWNIC Recycle SKB: ring-buffer");
#endif
#if	defined(WNIC_DOUBLE_LINK_LIST_RECYCLE_SKB)
	seq_printf(seq, "\nWNIC Recycle SKB: double-link-list");
#endif
#if	defined(WNIC_SINGLE_LINK_LIST_RECYCLE_SKB)
	seq_printf(seq, "\nWNIC Recycle SKB: single-link-list");
#endif
#if defined(WNIC_STRAIGHT_ARRAY_RECYCLE_SKB)
	seq_printf(seq, "\nWNIC Recycle SKB: straight-array");
#endif
#if defined(WNIC_LL_LINK_LIST_RECYCLE_SKB)
	seq_printf(seq, "\nWNIC Recycle SKB: lock_less-link-list");
#endif
	for (i = 0 ; i < (0x1 << WNIC_POOL_NUM_SHIFT_BITS) ; i++) {
		if (wnic_skb_max_size[i] == 0)
			continue;
		seq_printf(seq, "\npool[%d]", i);
		seq_printf(seq, "\nwnic_skb_total_num = %u wnic_skb_free_num = %u, wnic_skb_max_size = %u", wnic_skb_total_num[i], wnic_skb_free_num[i], wnic_skb_max_size[i]);
		seq_printf(seq, "\nrecycle_skb_free_num:\t\t%10d", wnic_free_eth_skb_num(i));
		seq_printf(seq, "\nrecycle_skb_alloc_num:\t\t%10d", wnic_alloc_eth_skb_num(i));
		seq_printf(seq, "\nrecycle_no_mem:      \t\t%10d", atomic_read(&wnic_llll_nomem[i]));
		atomic_set(&wnic_llll_nomem[i], 0);
#if defined(CONFIG_WNIC_RECYCLED_SKB_ALLOC_MAX)
		seq_printf(seq, "\nrecycle_skb_alloc_max:\t\t%10d", wnic_skb_max_allocate[i]);
		wnic_skb_max_allocate[i] = 0;
#endif
	}
	//seq_printf(seq, "\ntop_recycle_skb_alloc_num:\t\t%10d/%d", (wnic_skb_free_num-lowest_wnic_skb_free_num), wnic_skb_free_num);
	//lowest_wnic_skb_free_num=wnic_skb_free_num;
	seq_printf(seq, "\nUsage:\n");
	//seq_printf(seq, "echo on > /proc/realtek/wnic_recycle_skb\n - Switch on recycle skb mechanism.\n");
	//seq_printf(seq, "echo off > /proc/realtek/wnic_recycle_skb\n - Switch off recycle skb mechanism.\n");
#if defined(WNIC_DOUBLE_LINK_LIST_RECYCLE_SKB) || defined(WNIC_SINGLE_LINK_LIST_RECYCLE_SKB) || defined(WNIC_LL_LINK_LIST_RECYCLE_SKB)
	seq_printf(seq, "echo inc <pool> <count> > /proc/realtek/wnic_recycle_skb\n - Increase free skb number of pool, if new skb is available.\n");
	seq_printf(seq, "echo dec <pool> <count> > /proc/realtek/wnic_recycle_skb\n - Decrease free skb number of pool, if free skb enough.\n");
#endif
	return 0;
}

static int wnic_recycleskb_write(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
	int i, operator = 0, pool;
	unsigned int original_state;
	unsigned long operand = 0, flags;
	char tmpbuf[64];
	char *strptr;
	char *tokptr;
	struct sk_buff *skb;

	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count-1] = '\0';
		strptr = tmpbuf;
		tokptr = strsep(&strptr, " ");
		if (tokptr == NULL)
			goto errout;
#if defined(WNIC_DOUBLE_LINK_LIST_RECYCLE_SKB) || defined(WNIC_SINGLE_LINK_LIST_RECYCLE_SKB) || defined(WNIC_LL_LINK_LIST_RECYCLE_SKB)
		else if (strcasecmp(tokptr, "inc") == 0)
			operator = 1;
		else if (strcasecmp(tokptr, "dec") == 0)
			operator = -1;
		else
			goto errout;

		tokptr = strsep(&strptr, " ");
		if (tokptr == NULL)
			goto errout;

		if (kstrtoul(tokptr, 0, &pool))
			goto errout;

		tokptr = strsep(&strptr, " ");
		if (tokptr == NULL)
			goto errout;

		if (kstrtoul(tokptr, 0, &operand))
			goto errout;

		local_irq_save(flags);
		if (operator < 0 && operand > wnic_free_eth_skb_num(pool)) {
			local_irq_restore(flags);
			printk("dec over free skb num (%lu)\n", operand);
			goto errout;
		}

		for (i = 0 ; i < operand ; i++) {
			if (operator > 0) {
				skb = dev_alloc_skb(wnic_skb_max_size[pool] + WNIC_RESERVED_HEADER_SIZE);
				if (skb) {
					__init_skb_recy_wnic(skb);
					skb->wnic_recyclable = pool;
					wnic_skb_queue_head(skb, pool);
					wnic_skb_free_num[pool]++;
				}
			} else {
				skb = wnic_skb_dequeue(pool);
				if (skb) {
					skb->wnic_recyclable = 0;
					dev_kfree_skb_any(skb);
					wnic_skb_free_num[pool]--;
				}
			}
		}
		local_irq_restore(flags);
		printk("recycle_skb_free_num[%d]:%d\n", pool, wnic_free_eth_skb_num(pool));
#endif
	}
errout:
	return count;
}

static int wnic_recycleskb_open(struct inode *inode, struct file *file)
{
	return single_open(file, wnic_recycleskb_read, inode->i_private);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops wnic_recycleskb_ops = {
	.proc_open           = wnic_recycleskb_open,
	.proc_write          = wnic_recycleskb_write,
	.proc_read           = seq_read,
	.proc_lseek          = seq_lseek,
	.proc_release        = single_release,
};
#else
static const struct file_operations wnic_recycleskb_ops = {
	.owner          = THIS_MODULE,
	.open           = wnic_recycleskb_open,
	.write          = wnic_recycleskb_write,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};
#endif

#endif //WNIC_RECYCLE_SKB_PROC

int init_recycle_wnic_skb_buf(unsigned int ring_size, unsigned int max_size)
{
	int i, pool = -1;
	int first_invalid = -1;
	struct sk_buff *skb;

	for (i = 1 ; i < (0x1 << WNIC_POOL_NUM_SHIFT_BITS) ; i++) {
		if (wnic_pool[i] == 0) {
			if (first_invalid < 0)
				first_invalid = i;
			continue;
		} else if (wnic_skb_max_size[i] == max_size) {
			pool = i;
			break;
		}
	}
	if (pool < 0)
		pool = first_invalid;

	//memset(recycle_skb_buf, '\0', sizeof(struct recycle_skb_buf)*MAX_ETH_SKB_NUM);
	wnic_skb_free_num[pool] += ring_size;
	wnic_skb_total_num[pool] += ring_size;		//keep original total number

#if defined(WNIC_RING_BUFFER_RECYCLE_SKB)
	wrbr.rd_idx = 0;
	wrbr.wr_idx = 0;
	wrbr.state = WNIC_RING_BUF_RECYCLE_EMPTY;
	spin_lock_init(&wrbr.lock);

	for (i = 0 ; (0x1 << i) < wnic_skb_free_num ; i++)
		;
	if (i >= 32) {
		printk("pool is oversized...failed!\n");
		return;
	}

	wnic_skb_free_num = (0x1 << i);
	wrbr.ring_size_mask = wnic_skb_free_num - 1;

	wrbr.ring_buffer_recycle_array = (struct sk_buff *)kmalloc(wnic_skb_free_num * sizeof(struct sk_buff *), GFP_ATOMIC);
	if (!wrbr.ring_buffer_recycle_array) {
		printk("ring buffer recycle array allocate failed!\n");
		return;
	}
#endif
#if defined(WNIC_DOUBLE_LINK_LIST_RECYCLE_SKB) || defined(WNIC_SINGLE_LINK_LIST_RECYCLE_SKB)
	skb_queue_head_init(&wnic_recycle_skb_queue);
#endif
#if defined(WNIC_STRAIGHT_ARRAY_RECYCLE_SKB)
	wsra.idx = 0;
	spin_lock_init(&wsra.lock);

	wsra.array = (struct sk_buff *)kmalloc(wnic_skb_free_num * sizeof(struct sk_buff *), GFP_ATOMIC);
	if (!wsra.array) {
		printk("straight recycle array allocate failed!\n");
		return;
	}
#endif
#if defined(WNIC_LL_LINK_LIST_RECYCLE_SKB)
	init_llist_head(&wnic_llll_skb_head[pool]);
	spin_lock_init(&wnic_llll_read_lock[pool]);
	atomic_set(&wnic_llll_num[pool], 0);
	atomic_set(&wnic_llll_nomem[pool], 0);
#endif

	wnic_skb_max_size[pool] = max_size;
	wnic_pool[pool] = 1;

	for (i = 0 ; i < ring_size ; i++) {
		skb = dev_alloc_skb(max_size + WNIC_RESERVED_HEADER_SIZE);
		if (skb) {
			__init_skb_recy_wnic(skb);
			skb->wnic_recyclable = pool;
			wnic_skb_queue_head(skb, pool);
		}
	}

#ifdef WNIC_RECYCLE_SKB_PROC
	if (!wnic_recycle_pe) {
	    wnic_recycle_pe = proc_create_data("wnic_recycle_skb",
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
				realtek_proc, &wnic_recycleskb_ops, NULL);
	    if (!wnic_recycle_pe)
			printk("can't create proc entry for recycle_skb\n");
    }
#endif

	return pool;
}
EXPORT_SYMBOL(init_recycle_wnic_skb_buf);

void deinit_recycle_wnic_skb_buf(int pool)
{
	int i;
	struct sk_buff *skb;

	while (wnic_free_eth_skb_num(pool)) {
		skb = wnic_skb_dequeue(pool);
		if (skb) {
			skb->wnic_recyclable = 0;
			dev_kfree_skb_any(skb);
		}
	}

	wnic_skb_total_num[pool] = 0;
	wnic_skb_free_num[pool] = 0;
	wnic_skb_max_size[pool] = 0;

#if defined(WNIC_RING_BUFFER_RECYCLE_SKB)
	if (wrbr[pool].ring_buffer_recycle_array)
		kfree(wrbr[pool].ring_buffer_recycle_array);
#endif
#if defined(WNIC_STRAIGHT_ARRAY_RECYCLE_SKB)
	if (wsra[pool].array)
		kfree(wsra[pool].array);
#endif

	wnic_pool[pool] = 0;

#ifdef WNIC_RECYCLE_SKB_PROC
	if (wnic_recycle_pe) {
		for (i = ((0x1 << WNIC_POOL_NUM_SHIFT_BITS) - 1) ; i >= 1 ; i--) {
			if (wnic_pool[i] > 0)
				break;
		}
		if (i < 0 && wnic_recycle_pe)
			proc_remove(wnic_recycle_pe);
	}
#endif

}
EXPORT_SYMBOL(deinit_recycle_wnic_skb_buf);

void wnic_recycle_skb_clean(struct sk_buff *skb)
{
	struct skb_shared_info *shinfo;
	bool pfmemalloc;
	sk_buff_data_t		end;
	unsigned char		*head;
	unsigned int		truesize;
	unsigned char		recyclable;

	/* keep these fields before clean */
	pfmemalloc = skb->pfmemalloc;
	head = skb->head;
	end = skb->end;
	truesize = skb->truesize;
	recyclable = skb->wnic_recyclable;

	memset(skb, 0, offsetof(struct sk_buff, tail));
#if 1 /* clear RTK private header */
	memset(&skb->__private_header_start, 0, offsetof(struct sk_buff, __private_header_end) -
											offsetof(struct sk_buff, __private_header_start));
#endif
	/* Account for allocated memory : skb + skb->head */
	skb->truesize = truesize;
	atomic_set(&skb->users, 1);
	skb->head = head;
	skb->end = end;
	skb->pfmemalloc = pfmemalloc;
	skb->mac_header = (typeof(skb->mac_header))~0U;
	skb->transport_header = (typeof(skb->transport_header))~0U;
	skb->wnic_recyclable = recyclable;

	/* make sure we initialize shinfo sequentially */
	shinfo = skb_shinfo(skb);
	memset(shinfo, 0, offsetof(struct skb_shared_info, dataref));
	atomic_set(&shinfo->dataref, 1);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
	kmemcheck_annotate_variable(shinfo->destructor_arg);
#endif
}
EXPORT_SYMBOL(wnic_recycle_skb_clean);

int recycle_wnic_buf(struct sk_buff *skb)
{
	__u16 pool = skb->wnic_recyclable;

	if (likely((pool > 0))) {
		if (likely(wnic_pool[pool])) {
			/* clean cloned skb */
			if (skb->cloned)
				wnic_recycle_skb_clean(skb);

			wnic_skb_queue_head(skb, pool);
			return 1;
		} else {
			printk(KERN_ERR "[WNIC] recycle skb but pool[%d] deinit!!\n", pool);
			/* clear recyclable and let kernel free it */
			skb->wnic_recyclable = 0;
		}
	}
	return 0;
}
EXPORT_SYMBOL(recycle_wnic_buf);

struct sk_buff *dev_alloc_skb_recy_wnic(unsigned int size, int pool)
{
	/* first argument is not used */
	if (unlikely(size > wnic_skb_max_size[pool])) {
		return NULL;
	} else {
		/* dequeue one skb or null if empty */
		struct sk_buff *skb = wnic_skb_dequeue(pool);

		/* for wifi, _os_pkt_buf_alloc_rx will always reset data and tail before use. */
		if (likely(skb)) {
#if defined(CONFIG_WNIC_RECYCLED_SKB_ALLOC_MAX)
			unsigned int alloc_num = wnic_alloc_eth_skb_num(pool);

			if (alloc_num > wnic_skb_max_allocate[pool])
				wnic_skb_max_allocate[pool] = alloc_num;
#endif
			__init_skb_recy_wnic(skb);
		}
		return skb;
	}
}
EXPORT_SYMBOL(dev_alloc_skb_recy_wnic);

/* default set 0, let userspace to adjust it */
static unsigned long min_limit_pages;
module_param(min_limit_pages, ulong, 0644);
static unsigned int mem_usage_status; /* 0: normal, 1: no memory */

#define K(x) ((x) << (PAGE_SHIFT - 10))
static struct sk_buff *wnic_alloc_skb_limit(unsigned int size)
{
	unsigned long free_pages, limit_pages;

	limit_pages = (min_free_kbytes > min_limit_pages) ? min_free_kbytes : min_limit_pages;
	limit_pages += (size / 512);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	free_pages = global_zone_page_state(NR_FREE_PAGES);
#else
	free_pages = global_page_state(NR_FREE_PAGES);
#endif

#ifdef CONFIG_HIGHMEM
	free_pages -= nr_free_highpages();
#endif
	free_pages = K(free_pages);
	if (free_pages > limit_pages) {
		if (mem_usage_status == 1) {
			mem_usage_status = 0;
			queue_broadcast(MSG_TYPE_NIC_EVENT, NIC_EVENT_TYPE_MEM_RECOVERD, 0, ENABLED);
		}
		return __wnic_recycle_rtl_dev_alloc_skb(size);
	}

	printk_ratelimited("[%s %d] %lu, %d\n", __func__, __LINE__, free_pages, min_free_kbytes);
	if (mem_usage_status == 0) {
		mem_usage_status = 1;
		queue_broadcast(MSG_TYPE_NIC_EVENT, NIC_EVENT_TYPE_NO_MEM, 0, ENABLED);
	}

	return NULL;
}

struct sk_buff *wnic_recycle_skb_swap(struct sk_buff *skb)
{
	/* allocate new skb, swap skb, enqueue old_skb back to queue, continue to protocol stack */
	struct sk_buff *new_skb = NULL;
	int pool = skb->wnic_recyclable;

	/* check for jumbo packet */
	if (skb_shinfo(skb)->frag_list) {
		struct sk_buff *segs = skb_shinfo(skb)->frag_list;

		new_skb = wnic_alloc_skb_limit(wnic_skb_max_size[pool] + WNIC_RESERVED_HEADER_SIZE);
		if (new_skb) {
			struct sk_buff *frag_skb = wnic_alloc_skb_limit(wnic_skb_max_size[pool] + WNIC_RESERVED_HEADER_SIZE);

			if (frag_skb) {
				struct sk_buff *tmp_skb = skb_shinfo(skb)->frag_list->next;

				if (tmp_skb) {
					/* loop for next */
					struct sk_buff *swap_skb = wnic_alloc_skb_limit(wnic_skb_max_size[pool] + WNIC_RESERVED_HEADER_SIZE);
					if (swap_skb) {
						struct sk_buff *next_skb = tmp_skb->next;

						frag_skb->next = swap_skb;
						while (next_skb) {
							struct sk_buff *next = next_skb->next;
							struct sk_buff *alloc = NULL;

							alloc = wnic_alloc_skb_limit(wnic_skb_max_size[pool] + WNIC_RESERVED_HEADER_SIZE);
							if (alloc) {
								swap_skb->next = alloc;
								swap_skb = alloc;
							} else {
								swap_skb = NULL;
								break;
							}
							next_skb = next;
						}
						if (swap_skb) {
							/* copy new_skb,frag_skb and all next_skb */
							swap_skb = frag_skb->next;
							next_skb = skb_shinfo(skb)->frag_list->next;
							while (swap_skb) {
								struct sk_buff *swap_next = swap_skb->next;
								struct sk_buff *next_skb_next = next_skb->next;

								__wnic_recycle_skb_copy(next_skb, swap_skb);
								swap_skb = swap_next;
								next_skb = next_skb_next;
							}
							new_skb->data_len = skb->data_len;
							__wnic_recycle_skb_copy(skb, new_skb);
							__wnic_recycle_skb_copy(skb_shinfo(new_skb)->frag_list, frag_skb);
							skb_shinfo(skb)->frag_list = skb_shinfo(new_skb)->frag_list;
							skb_shinfo(new_skb)->frag_list = frag_skb;
						} else {
							/* no memory for swap */
							swap_skb = frag_skb->next;
							while (swap_skb) {
								struct sk_buff *next = swap_skb->next;

								dev_kfree_skb_any(swap_skb);
								swap_skb = next;
							}
							dev_kfree_skb_any(frag_skb);
							dev_kfree_skb_any(new_skb);
							new_skb = NULL;
						}
					} else {
						/* no memory for swap */
						dev_kfree_skb_any(frag_skb);
						dev_kfree_skb_any(new_skb);
						new_skb = NULL;
					}
				} else {
					/* copy new_skb and frag_skb */
					new_skb->data_len = skb->data_len;
					__wnic_recycle_skb_copy(skb, new_skb);
					__wnic_recycle_skb_copy(skb_shinfo(new_skb)->frag_list, frag_skb);
					skb_shinfo(skb)->frag_list = skb_shinfo(new_skb)->frag_list;
					skb_shinfo(new_skb)->frag_list = frag_skb;
				}
			} else {
				/* no memory for frag_skb */
				dev_kfree_skb_any(new_skb);
				new_skb = NULL;
			}
		}
		/* free old skb */
		while (segs) {
			struct sk_buff *next = segs->next;

			dev_kfree_skb_any(segs);
			segs = next;
		}
		skb_shinfo(skb)->frag_list = NULL;
	} else {
		new_skb = wnic_alloc_skb_limit(wnic_skb_max_size[pool] + WNIC_RESERVED_HEADER_SIZE);
		if (new_skb) {
			__wnic_recycle_skb_copy(skb, new_skb);
		}
	}
	dev_kfree_skb_any(skb);
	return new_skb;
}
EXPORT_SYMBOL(wnic_recycle_skb_swap);

#endif // CONFIG_WNIC_RECYCLED_SKB
