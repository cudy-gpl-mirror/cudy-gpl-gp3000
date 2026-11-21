#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>

#if defined(CONFIG_RTL9607C_SERIES)
#include <dal/rtl9607c/dal_rtl9607c_switch.h>
#endif

extern struct proc_dir_entry *realtek_proc;

#ifdef CONFIG_EXTERNAL_SWITCH
#define UNI_PORT_NUMBER (CONFIG_EXTERNAL_SWITCH_PORT_OFFSET+4)
#define PORT_NUMBER 8
#else
#define UNI_PORT_NUMBER 8
#define PORT_NUMBER 8
#endif
int uni_port[UNI_PORT_NUMBER] = {0};
int logical_port[PORT_NUMBER] = {-1};
int port[PORT_NUMBER] = {-1};
int isConfigured = 0;
const char *physical_mapping=NULL;
const char *uni_capability=NULL;
char g_uni_capability[128]={0};
const char *claim_uni_capability=NULL;
char g_claim_uni_capability[128]={0};

#define _DEBUG 0

static int parsePort(char *src, int *output, int num)
{
	int count=0;
	char *strptr = NULL, *tokptr = NULL;
#if _DEBUG
	printk("src %s\n", src);
#endif

	strptr = src;
	tokptr = strsep(&strptr, ";");
	while(strptr!=NULL){
		if(count < num){
			output[count] = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
			printk("output[%d]=%d\n", count, output[count]);
#endif
		}
		else{
			printk("parse port error!!\n");
			return -1;
		}
		count++;
		tokptr = strsep(&strptr, ";");
	}
	if(count < num && tokptr != NULL){
		output[count] = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
		printk("output[%d]=%d\n", count, output[count]);
#endif
		count++;
	}
	return count;
}

#if 0
static int checkSequence(int *src, int num)
{
	int i=0, j, isFound;
	for(i=0; i < num; i++){
		isFound = 0;
		for(j=0; j < num; j++){
			if(src[j]==i){
				isFound = 1;
				break;
			}
		}
		if(isFound==0){
			return 0;
		}
	}
	return 1;
}
#endif

static int uni_capability_proc_read(struct seq_file *s, void *v)
{
	char capability[128]="\0";
	char omci_claim_capability[128] = "\0";
	int i = 0, k = 0;
	char tmpbuf[128] = "\0";
	char tmpstr[128] = "\0";
	int logical_num = 0;

#if defined(CONFIG_RTL9607C_SERIES)
	if(!strcmp(CONFIG_UNI_CAPABILITY,"auto"))
	{
		unsigned int chipId, rev, subType;
		rtk_switch_version_get(&chipId, &rev, &subType);
		switch(chipId)
		{
			case RTL9607C_CHIP_ID:
				switch(subType)
				{
					case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA5:
					case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA6:
					case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA7:
						printk("RTL9607C\n");
					    strcpy(capability,"2;2;2;2;0");
					break;
					case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA4:
					case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA5:
					case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA6:
						printk("RTL9603C\n");
#ifdef CONFIG_00R0
						strncpy(capability,"0;0;0;1;2",strlen("0;0;0;1;2"));
#else
					    strcpy(capability,"0;1;1;1;2");
#endif
					break;
					default:
						printk("unsupport subType:0x%x\n",subType);
					return -1;
				}
				break;
			case RTL9603CVD_CHIP_ID:
				printk("Realtek RTL9603CVD\n");
#ifdef CONFIG_00R0
				strncpy(capability,"0;0;1;2;0",strlen("0;0;1;2;0"));
#else
				strcpy(capability,"1;1;1;2;0");
#endif
				break;
			default:
				printk("unsupport chipId:0x%x\n",chipId);
				return -1;
		}
	}
	else
#endif
	{
		if(uni_capability != NULL){
			strncpy(capability,uni_capability,sizeof(capability)-1);
		}
		else{
			strncpy(capability,CONFIG_UNI_CAPABILITY,sizeof(capability));
		}
	}

	if(claim_uni_capability != NULL){
		strncpy(omci_claim_capability,claim_uni_capability,sizeof(omci_claim_capability)-1);
	}
	else{
#ifdef CONFIG_OMCI_CLAIM_UNI_CAPABILITY
		strncpy(omci_claim_capability, CONFIG_OMCI_CLAIM_UNI_CAPABILITY, sizeof(omci_claim_capability));
#else
		strncpy(omci_claim_capability, capability, sizeof(omci_claim_capability));
#endif
	}

	seq_puts(s, "# port capability\n");
	seq_puts(s, "# From left, physical uni 0, uni 1, uni 2...\n");
	seq_puts(s, "# 40GE 7, 25GE 6, 10GE 5, 5GE 4, 2.5GE 3, GE 2, FE 1, None 0\n");
	seq_printf(s, "%s\n", capability);
	seq_puts(s, "# OMCI claim port capability\n");
	seq_puts(s, "# From left, physical uni 0, uni 1, uni 2...\n");
	seq_puts(s, "# 40GE 7, 25GE 6, 10GE 5, 5GE 4, 2.5GE 3, GE 2, FE 1, None 0\n");
	seq_printf(s, "%s\n", omci_claim_capability);
	seq_puts(s, "# logical port to physical port remapping\n");
	seq_puts(s, "# -1: invalid port\n");
	if(isConfigured==0){
		//parse uni port string
		memset(uni_port, 0, sizeof(int)*UNI_PORT_NUMBER);
		strcpy(tmpbuf, capability);
		//tmpbuf[strlen(tmpbuf)]=';';
		//tmpbuf[strlen(tmpbuf)+1]='\0';
		tmpbuf[strlen(tmpbuf)]='\0';
		parsePort(tmpbuf, uni_port, UNI_PORT_NUMBER);

		//parse logical port string
		if(physical_mapping != NULL){
			memset(tmpbuf, 0, sizeof(tmpbuf));
			sprintf(tmpbuf, "%s", physical_mapping);
			tmpbuf[strlen(tmpbuf)+1]='\0';
		}
		else{
			memset(tmpbuf, 0, sizeof(tmpbuf));
			for(i=0; i < UNI_PORT_NUMBER; i++){
				if(uni_port[i] != 0){
					sprintf(tmpstr, "%d", i);
				}
				else{
					sprintf(tmpstr, "-1");	
				}
				tmpstr[strlen(tmpstr)+1]='\0';
				strcat(tmpbuf, tmpstr);
				if(i<UNI_PORT_NUMBER-1){
					sprintf(tmpstr, ";");
				}
				else{
					*tmpstr = '\0';
				}
				tmpstr[strlen(tmpstr)+1]='\0';
				strcat(tmpbuf, tmpstr);
			}
		}
		//printk("%s:%d tmpbuf %s\n", __FUNCTION__, __LINE__, tmpbuf);
		logical_num = parsePort(tmpbuf, port, PORT_NUMBER);
		
		//fill logical port
		memset(logical_port, -1, sizeof(int)*PORT_NUMBER);
		for (i = 0; i < PORT_NUMBER; i++) {
			if (port[i] != -1) {
				logical_port[k] = port[i];
				k++;
			}
		}
		isConfigured = 1;
	}

	for (i = 0; i < PORT_NUMBER; i++) {
		if (i < PORT_NUMBER-1)
			seq_printf(s, "%d;", logical_port[i]);
		else
			seq_printf(s, "%d\n", logical_port[i]);
	}

	return 0;
}

static void __safe_strcpy(char *dst, unsigned int dst_len, const char *src)
{
	strncpy(dst, src, dst_len);
	dst[dst_len-1] = '\0';
}

static int uni_capability_single_open(struct inode *inode, struct file *file)
{
	return(single_open(file, uni_capability_proc_read, NULL));
}

static ssize_t uni_capability_single_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[128] = "\0";
	char tmpbuf1[128] = "\0";
	char key[20] = "\0";
	char *strptr = NULL, *tokptr = NULL;
	int i = 0, k = 0;
	int logical_num = 0;
	char capability[128]="\0";
	int speed = 0, claim=0;
	size_t len;

	len = min(count, sizeof(tmpbuf) - 1);

	if (buffer && !copy_from_user(tmpbuf, buffer, len)) {
		tmpbuf[len] = '\0';
		strptr = tmpbuf;
		tokptr = strsep(&strptr, " ");
#if _DEBUG
		printk("tokptr is %s\n", tokptr);
#endif
		if (!tokptr)
			key[0] = '\0';
		else {
			__safe_strcpy(key, sizeof(key), tokptr);
		}
		
		if( strcmp(key, "REMAP")!=0 && strcmp(key, "SPEED")!=0 && strcmp(key, "CLAIM")!=0){
			printk("parameter error, The logical port number is not matched!!\n");
			printk("echo [CMD SPEED]  [Logical port speed UNI0,UNI1,...,UNI3] > /proc/Realtek/uni_capability\n");
			printk("# 40GE 7, 25GE 6, 10GE 5, 5GE 4, 2.5GE 3, GE 2, FE 1, None 0\n");
			printk("e.g. echo SPEED 2;0;1;3 > /proc/realtek/uni_capability\n");
			printk("echo [CMD CLAIM]  [Logical port speed UNI0,UNI1,...,UNI3] > /proc/Realtek/uni_capability\n");
			printk("# 40GE 7, 25GE 6, 10GE 5, 5GE 4, 2.5GE 3, GE 2, FE 1, None 0\n");
			printk("e.g. echo CLAIM 2;0;1;3 > /proc/realtek/uni_capability\n");
			printk("echo [CMD REMAP]  [Logical port remppaing UNI0,UNI1,...,UNI3] > /proc/Realtek/uni_capability\n");
			printk("e.g. echo REMAP 2;0;1;3 > /proc/realtek/uni_capability\n");
			return count;
		}
		else if(strcmp(key, "SPEED")==0){
			speed = 1;
		}
		else if(strcmp(key, "CLAIM")==0){
			claim = 1;
		}
		tokptr = strsep(&strptr, "\n");
#if _DEBUG
		printk("tokptr is %s\n", tokptr);
#endif
		if(speed == 1){
			__safe_strcpy(g_uni_capability, sizeof(g_uni_capability), tokptr);
			uni_capability = &g_uni_capability[0];
		}
		else if(claim == 1){
			__safe_strcpy(g_claim_uni_capability, sizeof(g_claim_uni_capability), tokptr);
			claim_uni_capability = &g_claim_uni_capability[0];
		}
		else{
			//parse logical port string
			__safe_strcpy(tmpbuf1, sizeof(tmpbuf1), tokptr);
			//tmpbuf1[strlen(tmpbuf1)]=';';
			//tmpbuf1[strlen(tmpbuf1)+1]='\0';

			logical_num = parsePort(tmpbuf1, port, PORT_NUMBER);
	#if _DEBUG
			printk("logical_num is %d\n", logical_num);
	#endif
			//check logical port sequence
	#if 0
			if(checkSequence(port, logical_num)==0){
				printk("parameter error, The logical port sequence is not int the order!!\n");
				return count;
			}
	#endif
	#if defined(CONFIG_RTL9607C_SERIES)
			if(!strcmp(CONFIG_UNI_CAPABILITY,"auto"))
			{
				unsigned int chipId, rev, subType;
				rtk_switch_version_get(&chipId, &rev, &subType);
				switch(chipId)
				{
					case RTL9607C_CHIP_ID:
						switch(subType)
						{
							case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA5:
							case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA6:
							case RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA7:
								printk("RTL9607C\n");
								strncat(capability, "2;2;2;2;0", sizeof(capability)-1);
							break;
							case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA4:
							case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA5:
							case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA6:
									printk("RTL9603C\n");
	#ifdef CONFIG_00R0
								strncat(capability, "0;0;0;1;2", sizeof(capability)-1);
	#else
								strncat(capability, "0;1;1;1;2", sizeof(capability)-1);
	#endif
							break;
							default:
								printk("unspport subType:0x%x\n",subType);
							return -1;
						}
						break;
					case RTL9603CVD_CHIP_ID:
						printk("Realtek RTL9603CVD\n");
	#ifdef CONFIG_00R0
						strncat(capability, "0;0;1;2;0", sizeof(capability)-1);
	#else
						strncat(capability, "1;1;1;2;0", sizeof(capability)-1);
	#endif
						break;
					default:
							printk("unspport chipId:0x%x\n",chipId);
						return -1;
				}
			}
			else
	#endif
			{
				if(uni_capability != NULL){
					strncat(capability, uni_capability, sizeof(capability)-1);
				}
				else{
					strncat(capability, CONFIG_UNI_CAPABILITY, sizeof(capability)-1);
				}
			}

			//parse uni port string
			memset(uni_port, 0, sizeof(int)*UNI_PORT_NUMBER);
			__safe_strcpy(tmpbuf1, sizeof(tmpbuf1), capability);
			//tmpbuf1[strlen(tmpbuf1)]=';';
			//tmpbuf1[strlen(tmpbuf1)+1]='\0';

			parsePort(tmpbuf1, uni_port, UNI_PORT_NUMBER);

			//fill logical port
			memset(logical_port, -1, sizeof(int)*PORT_NUMBER);
			for (i = 0; i < PORT_NUMBER; i++) {
				if (port[i] != -1) {
					logical_port[k] = port[i];
					k++;
				}
			}
			isConfigured = 1;
		}
	}
	return count;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
static const struct proc_ops fops_uni_capability_stats = {
    .proc_open           = uni_capability_single_open,
    .proc_read_iter = seq_read_iter,
    .proc_write          = uni_capability_single_write,
    .proc_lseek         = seq_lseek,
    .proc_release        = single_release,
};
#else
static const struct file_operations fops_uni_capability_stats = {
	.open           = uni_capability_single_open,
	.read           = seq_read,
	.write          = uni_capability_single_write,
	.llseek         = seq_lseek,
	.release        = single_release,
};
#endif

static int rtk_uni_capability_platform_driver_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
	struct device_node *np =dev->of_node;
    int ret = 0;
	
	ret = of_property_read_string(np, "uni_capability", &uni_capability);
	if (ret < 0){
		uni_capability = NULL;
	}
	
	ret = of_property_read_string(np, "physical_mapping", &physical_mapping);
	if (ret < 0){
		return ret;
	}
	
#if _DEBUG
	if(uni_capability != NULL) printk("%s:%d uni_capability %s\n", __FUNCTION__, __LINE__, uni_capability);
	if(physical_mapping != NULL) printk("%s:%d physical_mapping %s\n", __FUNCTION__, __LINE__, physical_mapping);
#endif
	return 0;
}

static int rtk_uni_capability_platform_driver_remove(struct platform_device *pdev)
{
        return 0;
}

static const struct of_device_id rtk_uni_capability_match[] = {
        { .compatible = "realtek,uni_capability" },
        { },
};

static struct platform_driver rtk_uni_capability_driver = {
        .driver = {
                .name           = "rtk_uni_capability",
                .owner          = THIS_MODULE,
                .of_match_table = rtk_uni_capability_match,
        },
        .probe = rtk_uni_capability_platform_driver_probe,
        .remove = rtk_uni_capability_platform_driver_remove,
};

static int __init uni_capability_init(void) {
	struct proc_dir_entry *pe;
	
	platform_driver_register(&rtk_uni_capability_driver);
	
	if(strlen(CONFIG_UNI_CAPABILITY)!=0){
		pe = proc_create("uni_capability", S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH, realtek_proc,
                                                             &fops_uni_capability_stats);
		if (!pe) {
			return -EINVAL;
		}
	}
	
	return 0;
}

static void __exit uni_capability_exit(void) {
	platform_driver_unregister(&rtk_uni_capability_driver);
	return;
}

module_init(uni_capability_init);
module_exit(uni_capability_exit); 

MODULE_LICENSE("GPL"); 
