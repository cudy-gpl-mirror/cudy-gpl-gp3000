#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>

extern struct proc_dir_entry *realtek_proc;
struct proc_dir_entry *sys_capability_proc;

int num_of_fe_port=0;
int num_of_ge_port=0;
int num_of_voip_port=0;
int num_of_usb_port=0;
int num_of_wlan24_port=0;
int num_of_wlan5_port=0;
int num_of_lan_pon_port=0;
int num_of_2dot5_ge_port=0;
int num_of_5ge_port=0;
int num_of_10ge_port=0;
int num_of_25ge_port=0;
int num_of_40ge_port=0;

#define _DEBUG 0

static int sys_capability_proc_read(struct seq_file *s, void *v)
{
	seq_printf(s, "%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n", 
		num_of_fe_port, num_of_ge_port, num_of_voip_port, num_of_usb_port, num_of_wlan24_port, 
		num_of_wlan5_port, num_of_lan_pon_port, num_of_2dot5_ge_port, num_of_5ge_port, num_of_10ge_port, 
		num_of_25ge_port, num_of_40ge_port);
	
	return 0;
}

static int sys_capability_single_open(struct inode *inode, struct file *file)
{
	return(single_open(file, sys_capability_proc_read, NULL));
}


static const struct proc_ops fops_sys_capability_stats = {
	.proc_open           = sys_capability_single_open,
	.proc_read_iter = seq_read_iter,
	.proc_lseek         = seq_lseek,
	.proc_release        = single_release,
};

static int sys_capability_num_of_ge_port_proc_read(struct seq_file *s, void *v)
{
	seq_printf(s, "num_of_ge_port: %d\n", num_of_ge_port);
	
	return 0;
}

static int sys_capability_num_of_ge_port_single_open(struct inode *inode, struct file *file)
{
	return(single_open(file, sys_capability_num_of_ge_port_proc_read, NULL));
}

static int str_valid(const char *s)
{
	const char *p;

	for (p = s; *p != '\0' && *p != '\r' && *p != '\n'; p++) {
		if ((*p < 32) || (*p > 126))
			return 0;
	}
	return 1;
}

static ssize_t sys_capability_num_of_ge_port_single_write(struct file *filp, const char __user *buffer, size_t count, loff_t *offp)
{
	size_t len;
	long v;
	char tmpbuf[128] = "\0";

	len = min(count, sizeof(tmpbuf) - 1);

	if (buffer && !copy_from_user(tmpbuf, buffer, len)) {
		tmpbuf[len] = '\0';
		if (str_valid(tmpbuf) && !kstrtol(tmpbuf, 0, &v))
			num_of_ge_port = (int)v;
		printk("num_of_ge_port: %d\n", num_of_ge_port);
	}
	return count;
}

static const struct proc_ops fops_sys_capability_num_of_ge_port_stats = {
	.proc_open           = sys_capability_num_of_ge_port_single_open,
	.proc_write          = sys_capability_num_of_ge_port_single_write,
	.proc_read_iter = seq_read_iter,
	.proc_lseek         = seq_lseek,
	.proc_release        = single_release,
};

static int sys_capability_num_of_2dot5_ge_port_proc_read(struct seq_file *s, void *v)
{
	seq_printf(s, "num_of_2dot5_ge_port: %d\n", num_of_2dot5_ge_port);
	
	return 0;
}

static int sys_capability_num_of_2dot5_ge_port_single_open(struct inode *inode, struct file *file)
{
	return(single_open(file, sys_capability_num_of_2dot5_ge_port_proc_read, NULL));
}

static ssize_t sys_capability_num_of_2dot5_ge_port_single_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	size_t len;
	long v;
	char tmpbuf[128] = "\0";

	len = min(count, sizeof(tmpbuf) - 1);

	if (buffer && !copy_from_user(tmpbuf, buffer, len)) {
		tmpbuf[len] = '\0';
		if (str_valid(tmpbuf) && !kstrtol(tmpbuf, 0, &v))
			num_of_2dot5_ge_port = (int)v;
		printk("num_of_2dot5_ge_port: %d\n", num_of_2dot5_ge_port);
	}
	return count;
}

static const struct proc_ops fops_sys_capability_num_of_2dot5_ge_port_stats = {
	.proc_open           = sys_capability_num_of_2dot5_ge_port_single_open,
	.proc_write          = sys_capability_num_of_2dot5_ge_port_single_write,
	.proc_read_iter = seq_read_iter,
	.proc_lseek         = seq_lseek,
	.proc_release        = single_release,
};

static int __init sys_capability_init(void) {
	struct proc_dir_entry *pe;
	char capability[128] = "\0";
	char *strptr = NULL, *tokptr = NULL;

	strncpy(capability, CONFIG_SYSTEM_CAPABILITY ,sizeof(capability));
	printk("CONFIG_SYSTEM_CAPABILITY is %s\n", capability);
	
	strptr = capability;
	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_fe_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_fe_port is %d\n", num_of_fe_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_ge_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_ge_port is %d\n", num_of_ge_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_voip_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_voip_port is %d\n", num_of_voip_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_usb_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_usb_port is %d\n", num_of_usb_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_wlan24_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_wlan24_port is %d\n", num_of_wlan24_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_wlan5_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_wlan5_port is %d\n", num_of_wlan5_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_lan_pon_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_lan_pon_port is %d\n", num_of_lan_pon_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_2dot5_ge_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_2dot5_ge_port is %d\n", num_of_2dot5_ge_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_5ge_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_5ge_port is %d\n", num_of_5ge_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_10ge_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_10ge_port is %d\n", num_of_10ge_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_25ge_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_25ge_port is %d\n", num_of_25ge_port);
#endif
	}

	tokptr = strsep(&strptr, ";");
	if (tokptr){
		num_of_40ge_port = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("num_of_40ge_port is %d\n", num_of_40ge_port);
#endif
	}
	
	pe = proc_create("sys_capability", S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH, realtek_proc,
                                                             &fops_sys_capability_stats);

	if (!pe) {
		return -EINVAL;
	}

	sys_capability_proc = proc_mkdir("sys_capability_mod", realtek_proc);
	
	pe = proc_create("num_of_ge", S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH, sys_capability_proc,
                                                             &fops_sys_capability_num_of_ge_port_stats);

	if (!pe) {
		return -EINVAL;
	}

	pe = proc_create("num_of_2dot5_ge", S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH, sys_capability_proc,
                                                             &fops_sys_capability_num_of_2dot5_ge_port_stats);

	if (!pe) {
		return -EINVAL;
	}
	
	return 0;
}

static void __exit sys_capability_exit(void) {
	return;
}

late_initcall_sync(sys_capability_init);
module_exit(sys_capability_exit); 

MODULE_LICENSE("GPL"); 
