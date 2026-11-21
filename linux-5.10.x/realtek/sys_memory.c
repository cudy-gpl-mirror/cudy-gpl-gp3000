#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

extern struct proc_dir_entry *realtek_proc;

static int sys_memory_proc_read(struct seq_file *s, void *v)
{
	#if CONFIG_MIPS
	extern unsigned int memctlc_dram_size(void);
	unsigned int size = memctlc_dram_size();
	seq_printf(s, "%u\n", size >> 10);
	#else
	unsigned long physpages = get_num_physpages();
	seq_printf(s, "%u\n", physpages << (PAGE_SHIFT - 10));
	#endif
	return 0;
}

static int sys_memory_single_open(struct inode *inode, struct file *file)
{
	return(single_open(file, sys_memory_proc_read, NULL));
}


static const struct proc_ops fops_sys_memory_stats = {
	.proc_open           = sys_memory_single_open,
	.proc_read           = seq_read,
	.proc_lseek         = seq_lseek,
	.proc_release        = single_release,
};

static int __init sys_memory_init(void) {
	struct proc_dir_entry *pe;
	
	pe = proc_create("memory", S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH, realtek_proc,
                                                             &fops_sys_memory_stats);

	if (!pe) {
		return -EINVAL;
	}
	
	return 0;
}

static void __exit sys_memory_exit(void) {
	return;
}

late_initcall_sync(sys_memory_init);
module_exit(sys_memory_exit); 

MODULE_LICENSE("GPL"); 
