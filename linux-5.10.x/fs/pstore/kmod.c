// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2012  Google, Inc.
 */

#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/irqflags.h>
#include <linux/percpu.h>
#include <linux/smp.h>
#include <linux/atomic.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/ftrace.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/cache.h>
#include <linux/slab.h>
#include <asm/barrier.h>
#include "internal.h"

#define KMOD_RECORDS_MAX_SIZE (8192)

int dump_modules(void * (*fn)(void *buf, unsigned int *len, char *name, unsigned long addr, unsigned int size), void *buf, int buf_len);

static bool pstore_ftrace_kmod_dumped;
static void *kmod_record_buf = NULL;
static size_t kmod_record_buf_size=0;
static int kmode_offset_generated = 0;

static void kmod_records_update_offsets(struct pstore_kmod_record old[], int old_count,  
                    struct pstore_kmod_record new[], int new_count) {  
	int i,j;
    for (i = 0; i < old_count; ++i) {  
        old[i].offset = 0;
        for (j = 0; j < new_count; ++j) {  
            if (strncmp((char*)old[i].name, (char*)new[j].name, KMOD_MAX_NAME_LEN) == 0) {  
                old[i].offset = new[j].addr - old[i].addr;  
                break;
            }  
        }   
    }  
}  

static void notrace bubble_sort_pstore_kmod_records(struct pstore_kmod_record *records, size_t num_records) {  
    size_t i, j;  
    for (i = 0; i < num_records - 1; i++) {  
        for (j = 0; j < num_records - i - 1; j++) {  
            if (records[j].addr > records[j + 1].addr) {  
                struct pstore_kmod_record temp = records[j];  
                records[j] = records[j + 1];  
                records[j + 1] = temp;  
            }  
        }  
    }  
}  

  
struct pstore_kmod_record *binary_search_kmod_records(struct pstore_kmod_record *records, size_t num_records, unsigned long addr)  
{  
    size_t left = 0;  
    size_t right = num_records - 1;  
  
    while (left <= right) {  
        size_t mid = left + (right - left) / 2;  
        struct pstore_kmod_record *mid_record = &records[mid];  
  
        if (addr >= mid_record->addr && addr < mid_record->addr + mid_record->size) {  
  
            return mid_record;  
        } else if (addr < mid_record->addr) {  

            right = mid - 1;  
        } else {  
          
            left = mid + 1;  
        }  
    }  
  
    return NULL;  
}  

void print_pstore_kmod_records(struct pstore_kmod_record *records, size_t num_records) {  
	size_t i;
    for (i = 0; i < num_records; ++i) {  
        printk("pstore_kmod_record[%zu]:\n", i);  
        printk("  name: %s\n", records[i].name);  
        printk("  addr: 0x%lx\n", records[i].addr);  
        printk("  offset: 0x%lx\n", records[i].offset);  
        printk("  size: %u\n", records[i].size);  
        printk("\n");  
    }  
}   
void notrace pstore_ftrace_kmod_call(unsigned char *name,
								unsigned long addr,
								unsigned int size)
{
	unsigned long flags;
	struct pstore_kmod_record rec = {};
	struct pstore_record  record = {
		.type = PSTORE_TYPE_FTRACE_KMOD,
		.buf =(char *)&rec,
		.size=sizeof(rec),
		.psi= psinfo,
	};
	
	if (unlikely(oops_in_progress))
		return;

	local_irq_save(flags);

	rec.addr = addr;
	rec.size = size;
	strlcpy(rec.name, name, KMOD_MAX_NAME_LEN);
	psinfo->write(&record);

	local_irq_restore(flags);
}


void *pstore_ftrace_kmod_dump_one(void *buf, unsigned int *buf_len, char *name, unsigned long addr, unsigned int size)
{
	struct pstore_kmod_record *record;
	if(NULL == buf)
		return NULL;
	if(*buf_len < sizeof(struct pstore_kmod_record))
		return NULL;
	record = (struct pstore_kmod_record *)buf;
	strlcpy(record->name, name, KMOD_MAX_NAME_LEN);
	record->addr = addr;
	record->size = size;

	*buf_len -= sizeof(struct pstore_kmod_record);
	buf += sizeof(struct pstore_kmod_record);
	return buf;
}

void ftrace_kmod_gen_offset(void *record_buf, size_t size)
{
	int new_num, old_num;
	void *buffer;
	unsigned int length = KMOD_RECORDS_MAX_SIZE;

	if(kmode_offset_generated)
		return;

	/*collect new kmod info*/
	buffer = kzalloc(length, GFP_KERNEL);
	if(NULL == buffer) {
		printk("No Mem\n");
		return;
	}
	old_num = size/sizeof(struct pstore_kmod_record);
	new_num=dump_modules(pstore_ftrace_kmod_dump_one, buffer, length);
	kmod_records_update_offsets((struct pstore_kmod_record *)record_buf, old_num, 
		(struct pstore_kmod_record *)buffer, new_num);
	kfree(buffer);

	kmod_record_buf = kzalloc(size, GFP_KERNEL);
	if(kmod_record_buf == NULL) {
		printk("No Mem\n");
		return ;
	}
	memcpy(kmod_record_buf, record_buf, size);
	kmod_record_buf_size = size;
	kmode_offset_generated = 1;
}

unsigned long pstore_ftrace_kmod_addr(unsigned long addr)
{
	struct pstore_kmod_record *record;
	if(0 == kmode_offset_generated)
		return addr;

	if(is_kernel(addr))
		return addr;

	record = binary_search_kmod_records(kmod_record_buf, 
		kmod_record_buf_size/sizeof(struct pstore_kmod_record), addr);
	if(record)
		return addr + record->offset;

	return addr;
}




static ssize_t pstore_ftrace_kmod_knob_write(struct file *f, const char __user *buf,
					size_t count, loff_t *ppos)
{
	u8 on;
	ssize_t ret;
	int number, i;
	void *buffer = NULL;
	struct pstore_kmod_record *record = NULL;
	unsigned int length = KMOD_RECORDS_MAX_SIZE;
	ret = kstrtou8_from_user(buf, count, 2, &on);
	if (ret)
		return ret;

	if (!on ^ pstore_ftrace_kmod_dumped)
		goto out;

	/*dump kmod info to pstore*/
	buffer = kzalloc(length, GFP_KERNEL);
	if(NULL == buffer)
		goto out;

	number=dump_modules(pstore_ftrace_kmod_dump_one, buffer, length);

	if(0 == number)
		goto out;

	record = (struct pstore_kmod_record *) buffer;
	bubble_sort_pstore_kmod_records((struct pstore_kmod_record *)buffer, number);
	//print_pstore_kmod_records((struct pstore_kmod_record *)buffer, number);
	
	for(i=0;i<number;i++)
	{
		//printk("i %d name:%s addr: 0x%lx size: %u\n", i, record[i].name, record[i].addr, record[i].size);
		pstore_ftrace_kmod_call(record[i].name, record[i].addr, record[i].size);
	}

	kfree(buffer);

	pstore_ftrace_kmod_dumped = on;

out:
	ret = count;
	return ret;
}

static ssize_t pstore_ftrace_kmod_knob_read(struct file *f, char __user *buf,
				       size_t count, loff_t *ppos)
{
	char val[] = { '0' + pstore_ftrace_kmod_dumped, '\n' };

	return simple_read_from_buffer(buf, count, ppos, val, sizeof(val));
}

static const struct file_operations pstore_kmod_knob_fops = {
	.open	= simple_open,
	.read	= pstore_ftrace_kmod_knob_read,
	.write	= pstore_ftrace_kmod_knob_write,
};

static struct dentry *pstore_ftrace_kmod_dir;

void pstore_register_ftrace_kmod(void)
{
	if (!psinfo->write)
		return;
	
	pstore_ftrace_kmod_dir = debugfs_lookup("pstore", NULL);
	if(NULL == pstore_ftrace_kmod_dir)
		pstore_ftrace_kmod_dir = debugfs_create_dir("pstore", NULL);
	
	debugfs_create_file("record_ftrace_kmod", 0600, pstore_ftrace_kmod_dir, NULL,
			    &pstore_kmod_knob_fops);

}

void pstore_unregister_ftrace_kmod(void)
{
	pstore_ftrace_kmod_dumped = false;
	if(kmod_record_buf)
		kfree(kmod_record_buf);
	debugfs_remove_recursive(pstore_ftrace_kmod_dir);
}
