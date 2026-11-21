#include "nand_info_proc.h"


#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>


struct nand_info_s spi_nand_info;

static int nand_info_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%-15s: %llu\n", "nand_size"   , spi_nand_info.nand_size);
	seq_printf(m, "%-15s: %llu\n", "chip_size"   , spi_nand_info.chip_size);
	seq_printf(m, "%-15s: %u\n" , "block_size"  , spi_nand_info.block_size);
	seq_printf(m, "%-15s: %u\n" , "chunk_size"  , spi_nand_info.page_size);
	seq_printf(m, "%-15s: %u\n" , "oob_size"    , spi_nand_info.oob_size);
	seq_printf(m, "%-15s: %u\n" , "oob_avail"   , spi_nand_info.oob_avail);
	seq_printf(m, "%-15s: %u\n" , "PagePerBlock", spi_nand_info.PagePerBlock);
	seq_printf(m, "%-15s: %s\n" , "model", spi_nand_info.model);
	return 0;
}

static ssize_t nand_info_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *data)
{
	return count;
}

static int nand_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, nand_info_show, inode->i_private);
}

static const struct proc_ops nand_info_proc = {
	.proc_open       = nand_info_open,
	.proc_read       = seq_read,
	.proc_lseek     = seq_lseek,
	.proc_write      = nand_info_write,
	.proc_release = single_release,
};

#define NAND_INFO_NAME "nandinfo"
int __init nand_info_init(void)
{
	if (!proc_create(NAND_INFO_NAME, 0644, NULL, &nand_info_proc)) {
		printk(KERN_WARNING "create proc " NAND_INFO_NAME " failed!\n");
		return -1;
	}
	return 0;
}

module_init(nand_info_init);



