#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/netlink.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include "built_time"

//DONE: H21
//TODO: apmib should be shared library
//DONE: libusb should be shared library
//TODO: get usb info via libusb
//DONE: tune ppp interface MTU, refer set_wan.c
		//RunSystemCmd(NULL_FILE, "ifconfig", "ppp0", "mtu", tmp_args, "txqueuelen", "25",NULL_STR);
//DONE: special parameter in usb3g.chat & enable.pin, enable.userpass
//DONE: not execute, while web apply reboot
//DONE: remove mac entry in the web -> wan -> status

//--------------------------------------------------------------
#define DEMON		1
#define OUTPUT		0

//--------------------------------------------------------------
#define FAILURE		0
#define SUCCESS		1

#define DEV_NONE    0
#define DEV_ADD		1
#define DEV_REMOVE	2

#define VENDOR		"Vendor"
#define PRODID		"ProdID"
#define LINUX_VID	"1d6b"

#define PPP_OPTION	"/var/usb3g.option"
#define PPP_CHAT	"/var/usb3g.chat"

#define MAX_RETRY   30
#define OFFHUB_THD  3

#define BUSYWAIT(X)	{while (X) { ; }}

enum functions {
	FUNC_NONE = 0,
	FUNC_SCSI,
	FUNC_TTY,
	FUNC_MAX
};

//int connect_type = 0;
//int g_pwroff     = 0;
char vid[5], pid[5];
//--------------------------------------------------------------
//#define DEF_BH_SHOWFLOW 1

#if DEF_BH_SHOWFLOW
#define BH_SHOWFLOW printf
#else
#define BH_SHOWFLOW(format, args...)
#endif

#define THIS_FILE "mnet.c"

//#define BH_DBG printf
#define BH_DBG(format, args...)
//BH_DBG("[bruce:%s:%d]\n", __FUNCTION__, __LINE__);


#define WORKAROUND_FOR_MPS 1
//--------------------------------------------------------------
struct usb3g_info_s {
	char pin[5];
	char apn[20];
	char dialnum[12];
	char user[32];
	char pass[32];
	char conntype[2];
	char idle[6];
	char mtu[6];
};

//--------------------------------------------------------------
int isFileExisted(char *filename)
{
	struct stat fst;
	if ( stat(filename, &fst) < 0)
		return FAILURE; //not exist
	return SUCCESS;
}

int isPPPRunning(void)
{
	FILE   *fp;
	char buffer[8];
	/* check ppp pid */
	fp = fopen("/var/run/ppp0.pid", "r");
	if (fp ==NULL)
		return FAILURE;
		
	if (fgets(buffer, sizeof(buffer),fp))
		if (-1 != getpgid(atoi(buffer)))
			return FAILURE;

	return SUCCESS;
}

int onoff_usbhub(void)
{
	int sys_ret = 0;

	BH_SHOWFLOW("[%s:%s:%d] turn off & on the power of hub\n", THIS_FILE, __FUNCTION__, __LINE__);
#if 1
    sys_ret = system("hub-ctrl -b 1 -d 1 -P 1 -p");
	sleep(1);
    sys_ret = system("hub-ctrl -b 1 -d 1 -P 1 -p 1");
	sleep(1);
#elif 0
	sys_ret = system("hub-ctrl -P 1");
	sleep(1);
	sys_ret = system("hub-ctrl -P 1 -p 1");
	sleep(1);
#else
	//system("iwpriv wlan0 write_mem dw,b8021054,1,100000");
#endif
	return SUCCESS;
}

int need_a_poweroff(void)
{
	BH_SHOWFLOW("[%s:%s:%d]\n", THIS_FILE, __FUNCTION__, __LINE__);
	//g_pwroff = 0;
	onoff_usbhub();
	return 0;
}

#define USB3G_CONN   1
#define USB3G_DISC   2
#define USB3G_INIT   3
#define USB3G_DIAL   4
#define USB3G_REMOVE 5

void export_status(int stat)
{
	switch (stat) {
	case USB3G_CONN:
		system("echo \"conn\" > /var/usb3g.stat");
		break;
	case USB3G_DISC:
		system("echo \"disc\" > /var/usb3g.stat");
		break;
	case USB3G_INIT:
		system("echo \"init\" > /var/usb3g.stat");
		break;
	case USB3G_DIAL:
		system("echo \"dial\" > /var/usb3g.stat");
		break;
	case USB3G_REMOVE:
		system("echo \"remove\" > /var/usb3g.stat");
		if ( 0 == access("/var/mnet_fork", 0))
			system("rm /var/mnet_fork >/dev/null 2>&1");
		break;
	}
}

#if 0
void kill_ppp_inet(void)
{
	system("killall ppp_inet >/dev/null 2>&1");
	system("killall pppd >/dev/null 2>&1");
	system("rm /etc/ppp/connectfile >/dev/null 2>&1");
}
#endif

int get_tty(char *ttyitf)
{
    FILE   *fp;
    char   buffer[64];
	int    sys_ret = 0;

	//printf("[%s] ttyitf = [%s]\n", __FUNCTION__, ttyitf);
	sys_ret = system("ls /sys/bus/usb-serial/devices > /var/tty-devices");
	fp = fopen("/var/tty-devices", "r");
	if (fp !=NULL && NULL != fgets(buffer, sizeof(buffer),fp)) {
		char *ptr = strstr(buffer, " ");
		if (ptr != NULL)
			return FAILURE;

		ptr = '\0';
		strcpy(ttyitf, buffer);
		BH_SHOWFLOW("[%s:%s:%d] first one is /dev/%s\n", THIS_FILE, __FUNCTION__, __LINE__, ttyitf);
		sys_ret = system("rm /var/tty-devices >/dev/null 2>&1");
		fclose(fp);

		return SUCCESS;
	}

	sys_ret = system("rm /var/tty-devices >/dev/null 2>&1");
	fclose(fp);
	return FAILURE;
}

int switch_to_modem(char *ttyitf)
{
    FILE   *fp;
    char   buffer[128];
	int    sys_ret = 0, idx = 0;

	if (FAILURE == isFileExisted("/var/usb_modeswitch.conf"))
		return FAILURE;

	export_status(USB3G_INIT);
	sys_ret = system("usb_modeswitch -c /var/usb_modeswitch.conf");

	if (get_tty(ttyitf)) {
		fp = fopen("/var/usb3g.tty", "r");
		if (fp==NULL)
			return FAILURE;

		fgets(buffer, sizeof(buffer),fp);
		sscanf(ttyitf, "ttyUSB%d", &idx);
		sprintf(ttyitf,"ttyUSB%d", idx+atoi(buffer));
		BH_SHOWFLOW("usb_modeswitch done! and ttyitf is [%s], idx[%d]\n", ttyitf, idx);
		sys_ret = system("rm /var/usb_modeswitch.conf >/dev/null 2>&1");
		fclose(fp);
		return SUCCESS;
	}

	return FAILURE;
}

/* TODO: apmib should be shared library */
int get_mib(char *val, char *mib)
{
	FILE *fp;
 	char buf[128];

	snprintf(buf, sizeof(buf), "mib get %s", mib);
	fp = popen(buf, "r");
	if (fp==NULL)
		return FAILURE;

	if (NULL == fgets(buf, sizeof(buf),fp)) {
		pclose(fp);
		return FAILURE;
	}

	if(strstr(buf, "="))
		strcpy(val, strstr(buf, "=")+1);
	pclose(fp);
	return SUCCESS;
}

int get_mib_para(struct usb3g_info_s *info)
{
	memset(info, 0x0, sizeof(struct usb3g_info_s));
	get_mib(info->pin,	"WAN_3G_TBL.0.pin");
	get_mib(info->apn,	"WAN_3G_TBL.0.apn");
	get_mib(info->dialnum,	"WAN_3G_TBL.0.dial");
	get_mib(info->user,	"WAN_3G_TBL.0.username");
	get_mib(info->pass,	"WAN_3G_TBL.0.password");
	get_mib(info->conntype, "WAN_3G_TBL.0.ctype");
	get_mib(info->idle,	"WAN_3G_TBL.0.idletime");
	get_mib(info->mtu,	"WAN_3G_TBL.0.mtu");
	
	return SUCCESS;
}

void gen_ppp_option(char *ttyitf)
{
    FILE *fp;
	struct usb3g_info_s info;
    char buf[128];
	char commentAPN[4];
#if WORKAROUND_FOR_MPS
	char commentCIMI[4];
	char vendorID[6];
	char productID[6];
#endif

	get_mib_para(&info);
	BH_SHOWFLOW("[%s:%s:%d] PPP parameter: apn[%s], dialnum[%s]\n", THIS_FILE, __FUNCTION__, __LINE__, info.apn, info.dialnum);

#if WORKAROUND_FOR_MPS
	get_device_info_from_proc(vendorID, productID);
#endif
	
    buf[0] = '\0';
	//if (!strcmp(info.dialnum, "#777"))
#if WORKAROUND_FOR_MPS
	if (vendorID && strcmp(vendorID, "feed")) /* for mobilepeak */
#endif
		sprintf(buf+strlen(buf), "'OK' \"ATQ0 V1 E1  S0=0 &C0 &D2\"\n");
    if (strlen(info.pin) != 0)
        sprintf(buf+strlen(buf), "'OK' 'AT+CPIN=%s'\n", info.pin);
#if WORKAROUND_FOR_MPS
    if (vendorID && !strcmp(vendorID, "feed")) /* for mobilepeak */
        sprintf(buf+strlen(buf), "'OK' 'AT%%B'\n");//TODO
#endif
    if (buf == NULL)
        sprintf(buf, "\n");

	commentAPN[0] = '\0';
	if (info.dialnum && !strcmp(info.dialnum, "#777"))
		sprintf(commentAPN, "#");

#if WORKAROUND_FOR_MPS
	commentCIMI[0] = '\0';
	if (vendorID && !strcmp(vendorID, "feed")) /* for mobilepeak */
		sprintf(commentCIMI, "#");
#endif

	fp = fopen(PPP_CHAT, "w");
	fprintf(fp,
			"ABORT 'NO DIAL TONE'\n"
			"ABORT 'NO ANSWER'\n"
			"ABORT 'NO CARRIER'\n"
			"ABORT DELAYED\n"
			"ABORT 'COMMAND NOT SUPPORT'\n"
			"\n"
			"'' ''\n"
			"'' 'ATZ'\n"
            "%s" //buf: PIN
			"\n"
			"SAY 'show device infomation...\\n'\n"
			"'OK' 'ATI'\n"
			"\n"
			"SAY 'show SIM CIMI...\\n'\n"
	#if WORKAROUND_FOR_MPS
			"%s" //commentCIMI, mobilepeak doesn't have cimi yet
	#endif
			"'OK' 'AT+CIMI'\n"
			"\n"
			"SAY 'set APN...\\n'\n"
			"%s" //commentAPN: enable APN or not
			"'OK' 'AT+CGDCONT=1,\"IP\",\"%s\"'\n"
			"\n"
			"SAY 'dial...\\n\\n'\n"
			"'OK' 'ATD%s'\n"
			"'CONNECT' ''\n",
            buf,
	#if WORKAROUND_FOR_MPS
			commentCIMI,
	#endif
			commentAPN, info.apn, info.dialnum);

	fclose(fp);

    buf[0] = '\0';
	if (info.conntype && atoi(info.conntype) == 1) {
        sprintf(buf+strlen(buf), "demand\n");
        sprintf(buf+strlen(buf), "idle %s\n", info.idle);
    }
    if (strlen(info.user) != 0)
        sprintf(buf+strlen(buf), "user %s\n", info.user);
    if (strlen(info.pass) != 0)
        sprintf(buf+strlen(buf), "password %s\n", info.pass);
    if (strlen(info.mtu) != 0) {
        sprintf(buf+strlen(buf), "mtu %s\n", info.mtu);
		sprintf(buf+strlen(buf), "mru %s\n", info.mtu);
	}
    if (buf == NULL)
        sprintf(buf, "\n");

	fp = fopen(PPP_OPTION, "w");
	fprintf(fp,
			//"-detach\n"
			"hide-password\n"
			"persist\n"
			"nodetach\n"
			"lcp-echo-interval 20\n"
			"lcp-echo-failure 3\n"
			"holdoff 2\n"
			"connect-delay 100\n"
			"noauth\n"
			"/dev/%s\n"
			"115200\n"
			"debug\n"
			"defaultroute\n"
			"ipcp-accept-local\n"
			"ipcp-accept-remote\n"
			"usepeerdns\n"
			"crtscts\n"
			"lock\n"
			"noccp\n"
			"noipdefault\n"
            "%s" //buf: ondemand, user, password, mtu
			"connect '/bin/chat -v -t10 -f %s'\n",
			ttyitf, buf, PPP_CHAT);

    fclose(fp);
}

int dial_ppp(void)
{
    char cmd[32], buf[4];
	int  sys_ret = 0, recheck = 5, connect_type = -1;
	
	get_mib(buf, "WAN_3G_TBL.0.ctype");
	connect_type = atoi(buf);

	//sleep(1);
	
    if (connect_type == 2)
        return SUCCESS;

	export_status(USB3G_DIAL);
	//TODO: check ppp_inet
	//kill_ppp_inet();
	sprintf(cmd, "ppp_inet -t 16 -c %d -x", connect_type);
printf("---> %s\n", cmd);
	sys_ret = system(cmd);

	/* check connect */
	while (recheck--)
	{
		if (SUCCESS == isFileExisted("/etc/ppp/link"))
			break;
		else
			sleep(1);
	}

	if (recheck<=0)
		export_status(USB3G_DISC);

	return SUCCESS;
}

int get_device_info_from_proc(char *vendor, char *prodid)
{
	FILE *fp = NULL;
	char buffer[128];
    char *ptr  = NULL;

#ifdef CONFIG_DEFAULTS_KERNEL_2_6
    fp = popen("cat /proc/bus/usb/devices", "r");
#else
	fp = popen("cat /sys/kernel/debug/usb/devices", "r");
#endif 	
	if (fp==NULL)
		return FAILURE;

    while (NULL != fgets(buffer, sizeof(buffer),fp))
    {
        ptr = strstr(buffer, VENDOR);
        if (ptr) {
            if (strstr(buffer, LINUX_VID))
                /* if vender == LINUX, get next */
                continue;
            else {
                sscanf(ptr, "Vendor=%s ProdID=%s Rev*", vendor, prodid);
                return SUCCESS;
            }
        }
    }
    pclose(fp);

	return FAILURE;
}

int exec_action_for_tty(int action, char *ttyitf)
{
	char buffer[64];
    char vender[6];
    char prodid[6];
	FILE *fp;
	int idx =0;

	if (action == DEV_ADD) {
		if (get_tty(ttyitf)) {
			if (FAILURE == isFileExisted("/var/usb3g.tty"))
			{
				get_device_info_from_proc(vender, prodid);
				sprintf(buffer, "usb_modeswitch -f %s:%s -c /etc/usb_modeswitch.d/%s_%s", vender, prodid, vender, prodid);
				system(buffer);
				if (SUCCESS == isFileExisted("/var/usb3g.tty")) {
					fp = fopen("/var/usb3g.tty", "r");
					fgets(buffer, sizeof(buffer),fp);
					sscanf(ttyitf, "ttyUSB%d", &idx);
					sprintf(ttyitf,"ttyUSB%d", idx+atoi(buffer));
					fclose(fp);
				}
			}
			else {
				fp = fopen("/var/usb3g.tty", "r");
				fgets(buffer, sizeof(buffer),fp);
				sscanf(ttyitf, "ttyUSB%d", &idx);
				sprintf(ttyitf,"ttyUSB%d", idx+atoi(buffer));		
				fclose(fp);
			}
		}

		system("mdev -s");//
		sleep(2);//

		BH_SHOWFLOW("ttyitf[%s], idx[%d]\n", ttyitf, idx);
		gen_ppp_option(ttyitf);
		//dial_ppp();
#if defined(CONFIG_USER_RTK_SYSCONF) && defined(CONFIG_USER_PPPOMODEM) && defined(CONFIG_USER_PPPD)
		char cmd[256]={0};
		snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_dialup_modem %s", ttyitf);
		system(cmd);
#endif
		return SUCCESS;
	}
    else if (action == DEV_REMOVE) {
        BH_SHOWFLOW("[%s:%s:%d] do nothing\n", THIS_FILE, __FUNCTION__, __LINE__);
		//kill_ppp_inet();
#if defined(CONFIG_USER_RTK_SYSCONF) && defined(CONFIG_USER_PPPOMODEM) && defined(CONFIG_USER_PPPD)
		char cmd[256]={0};
		snprintf(cmd, sizeof(cmd), "sysconf send_unix_sock_message /var/run/systemd.usock do_modem_link_down");
		system(cmd);
#endif
		export_status(USB3G_REMOVE);
        return SUCCESS;
    }

	return FAILURE;
}

int exec_action(int action, int retry, char *ttyitf)
{
	/* MUST BE storage device !!! */
	if (action == DEV_ADD) {
		BH_SHOWFLOW("[%s:%s:%d] action == DEV_ADD\n", THIS_FILE, __FUNCTION__, __LINE__);
		if (switch_to_modem(ttyitf)) {
			gen_ppp_option(ttyitf);
			return dial_ppp();
		}
	}
	else if (action == DEV_REMOVE) {
		BH_SHOWFLOW("[%s:%s:%d] do nothing\n", THIS_FILE, __FUNCTION__, __LINE__);
		export_status(USB3G_REMOVE);
		return SUCCESS;
	}

	return FAILURE;
}

int find_profile(char *profile)
{
    FILE *fp;
    char buffer[128];
	int sys_ret = 0;
	//printf("[%s][%d] profile = [%s]\n", __FUNCTION__, __LINE__, profile);
    fp = popen("ls -a /etc/usb_modeswitch.d", "r");
    while (NULL != fgets(buffer, sizeof(buffer),fp))
    {
		//printf("[%s][%d] buffer = [%s]\n", __FUNCTION__, __LINE__, buffer);
        if (strstr(buffer, profile))
        {
			//printf("[%s][%d]\n", __FUNCTION__, __LINE__);
            sprintf(buffer, "cp /etc/usb_modeswitch.d/%s /var/usb_modeswitch.conf", profile);
			//printf("[%s][%d] buffer = [%s]\n", __FUNCTION__, __LINE__, buffer);
            sys_ret = system(buffer);

			memset(buffer, 0x0, 128);
			
            sprintf(buffer, "echo DefaultVendor=0x%s >> /var/usb_modeswitch.conf", vid);
			//printf("[%s][%d] buffer = [%s]\n", __FUNCTION__, __LINE__, buffer);
            sys_ret = system(buffer);

			memset(buffer, 0x0, 128);
			
            sprintf(buffer, "echo DefaultProduct=0x%s >> /var/usb_modeswitch.conf", pid);
			//printf("[%s][%d] buffer = [%s]\n", __FUNCTION__, __LINE__, buffer);
            sys_ret = system(buffer);
			
            pclose(fp);
			return SUCCESS;
        }
    }
    pclose(fp);
    return FAILURE;
}

#ifdef DEMON
char *id_pend_zero(char *id, char *buf)
{
	char *ptr = NULL;
	int pend_zero = 0, i;

	ptr = strstr(buf, "/");
	if (ptr) {
		ptr[0] = '\0';	

		pend_zero = 4-strlen(buf);
		strcpy(id+pend_zero, buf);
		if (pend_zero>0) {
			for (i = 0; i<pend_zero; i++)
				id[i] = '0';
		}
	}
	return ptr;
}


struct cooling_device {
    char action;
    const char *devpath;
    const char *subsystem;
    int major;
    int minor;
    const char *devname;
    const char *devtype;
    const char *product;
};

static void parse_uevent(const char *msg, int len, struct cooling_device *cdev)
{
    char *pStart = msg;
    while (*msg) {
        if((msg - pStart + strlen(msg)) >= len)
            break;
        //printf("%s\n", msg);
        if (!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
	    if(!strncmp(msg, "add", 3)){
	        cdev->action = DEV_ADD;
	    }else if(!strncmp(msg, "remove", 6)){
	        cdev->action = DEV_REMOVE;
	    }else{
	        cdev->action = DEV_NONE;
	    }
        } else if (!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            cdev->devpath = msg;
        } else if (!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            cdev->subsystem = msg;
        } else if (!strncmp(msg, "MAJOR=", 6)) {
            msg += 6;
            cdev->major = atoi(msg);
        } else if (!strncmp(msg, "MINOR=", 6)) {
            msg += 6;
            cdev->minor = atoi(msg);
        } else if (!strncmp(msg, "DEVNAME=", 8)) {
            msg += 8;
            cdev->devname = msg;
        } else if (!strncmp(msg, "DEVTYPE=", 8)) {
            msg += 8;
            cdev->devtype = msg;
        } else if (!strncmp(msg, "PRODUCT=", 8)) {
            msg += 8;
            cdev->product = msg;
        }

        while(*msg++);
        if((msg - pStart)>=len)
            break;
    }

    BH_SHOWFLOW("event { action=%d, devpath=%s, subsystem=%s, major=%d, minor=%d, devname=%s, devtype=%s, product=%s}\n", 
		    cdev->action,
		    cdev->devpath,
		    cdev->subsystem,
		    cdev->major,
		    cdev->minor,
		    cdev->devname,
		    cdev->devtype,
		    cdev->product);
}

#ifdef CONFIG_USER_NETLINK_HOTPLUG
static int isHotplugDev(struct cooling_device *cdev){
	if(!cdev) return 0;
	if(cdev->subsystem && !strncmp(cdev->subsystem, "block", strlen("block")+1) && cdev->devname){
		if(!strncmp(cdev->devname, "sd", 2) && isalpha(*(cdev->devname+2)) && isdigit(*(cdev->devname+3))){
			return 1;
		}
	}

	if(cdev->subsystem && !strncmp(cdev->subsystem, "block", strlen("block")+1) && cdev->devname){
		if(!strncmp(cdev->devname, "sr", 2) && isdigit(*(cdev->devname+2))){
			return 2;
		}
	}

	if(cdev->subsystem && !strncmp(cdev->subsystem, "usbmisc", strlen("usbmisc")+1) && cdev->devname){
		if(!strncmp(cdev->devname, "usb/lp", 6) && isdigit(*(cdev->devname+6))){
			return 3;
		}
	}
	return 0;
}
#endif

int run_fork(int func, int action)
{
	int retry = MAX_RETRY;
	pid_t pid = 0;
	char ttyitf[8];

	BH_SHOWFLOW("[%s:%s:%d]\n", THIS_FILE, __FUNCTION__, __LINE__);
	if ((pid = fork())< 0)
		printf("process[%d] fail to fork child\n", getpid());
	else if (pid == 0) {
		if ((pid = fork())< 0)
			printf("process[%d] fail to fork child\n", getpid());
		else if (pid > 0)
			exit(0);
		else {
			if (SUCCESS == isFileExisted("/var/mnet_fork")) {
				if ( action == DEV_REMOVE && 0 == access("/var/mnet_fork", 0))
					system("rm /var/mnet_fork >/dev/null 2>&1");
				BH_SHOWFLOW("[%s:%s:%d] /var/mnet_fork is existed, exit(0)\n", THIS_FILE, __FUNCTION__, __LINE__);
				exit(0);
			}else
				system("echo \"\" > /var/mnet_fork >/dev/null 2>&1");
			
			if (func == FUNC_SCSI)
				exec_action(action, retry, ttyitf);
			else if (func == FUNC_TTY)
				exec_action_for_tty(action, ttyitf);

			system("rm /var/mnet_fork >/dev/null 2>&1");
			exit(0);
		}
	}
	waitpid(pid, NULL, 0);
	return SUCCESS;
}

int run_fork_modeswitch(void)
{
	pid_t pid = 0;
	char vender[6];
	char prodid[6];
	char buffer[32];

	if ((pid = fork())< 0)
		printf("process[%d] fail to fork child\n", getpid());
	else if (pid == 0) {
		if ((pid = fork())< 0)
			printf("process[%d] fail to fork child\n", getpid());
		else if (pid > 0)
			exit(0);
		else {
			get_device_info_from_proc(vender, prodid);
			sprintf(buffer, "usb_modeswitch -c /etc/usb_modeswitch.d/%s_%s  >/dev/null 2>&1", vender, prodid);
			system(buffer);
			exit(0);
		}
	}
	waitpid(pid, NULL, 0);
	return SUCCESS;
}

int demon(void)
{
	struct sockaddr_nl nls;
	struct pollfd pfd;
	char buf[512], devinfo[20];
	int action = DEV_NONE;
	int func   = 0;
	int len    = 0;
	struct cooling_device cdev;
	char *tmp=NULL;
	char found_profile=FAILURE;
	char devType=0;

	daemon(1,1);
	memset(&nls, 0, sizeof(nls));
	nls.nl_family = AF_NETLINK;
	nls.nl_pid = getpid();
	nls.nl_groups = -1;

	pfd.events = POLLIN;
	pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (pfd.fd == -1) {
		printf("Not root\n");
		exit(1);
	}
	//printf("create socket ok\n");
	
	if (bind(pfd.fd, (void*)&nls, sizeof(nls))) {
		printf("bind failed\n");
		exit(1);
	}
	//printf("bind ok\n");

	//if (g_pwroff == 1) {
	//	run_fork_modeswitch();
		//need_a_poweroff();
	//}
	//printf("g_pwroff is true\n");
	
	while (-1 != poll(&pfd, 1, -1)) {
		func = FUNC_NONE;
		len  = recv(pfd.fd, buf, sizeof(buf), MSG_DONTWAIT);
		if (len == -1) {
			printf("recv failed\n");
			//exit(1);
		}

		found_profile = FAILURE;
		memset(&cdev, 0x0, sizeof(cdev));
		parse_uevent(buf, len, &cdev);

		if (cdev.product && cdev.devtype && !strncmp(cdev.devtype, "usb_device", strlen("usb_device")+1)) {
			memset(vid, 0, sizeof(vid));
			memset(pid, 0, sizeof(pid));

			strncpy(devinfo, cdev.product, strlen(cdev.product));
			tmp = id_pend_zero(vid, devinfo);
			tmp = id_pend_zero(pid, tmp+1);
			memset(devinfo, 0, sizeof(devinfo));
			sprintf(devinfo, "%s_%s\0", vid, pid);
			BH_SHOWFLOW("vid=[%s], pid=[%s], devinfo=%s\n", vid, pid, devinfo);
			found_profile=find_profile(devinfo);
		}

		if(found_profile == SUCCESS){
			func = FUNC_SCSI;
		}else if(cdev.subsystem && !strncmp(cdev.subsystem, "tty", strlen("tty")+1) && cdev.major == 188){
			func = FUNC_TTY;
		}
#ifdef CONFIG_USER_NETLINK_HOTPLUG
		else if((cdev.action == DEV_ADD || cdev.action == DEV_REMOVE) && (devType=isHotplugDev(&cdev))){
			/* For mass storage device
			 * 1. subsystem="block"
			 *    devname match following rule
			 *    - sd[a-z][0-9]
			 *    - sr[0-9]
			 * 2. subsystem="usbmisc"
			 *    devname match following rule
			 *    - usb/lp[0-9]
			 * */

			BH_SHOWFLOW("[%s:%d] handle hotplug event(devname=%s)\n", __FUNCTION__, __LINE__, cdev.devname);
			setenv("DEVPATH",cdev.devpath,1);
			if(cdev.action==DEV_ADD){
				setenv("ACTION","add",1);
			}else if(cdev.action==DEV_REMOVE){
				setenv("ACTION","remove",1);
			}

			if(devType==1 || devType==2){
				system("/bin/usbmount");
			}else if(devType==3){
				system("/bin/startcupsd");
			}
		}
#endif

		BH_SHOWFLOW("Device %s[%d], func=%d***\n", devinfo, cdev.action, func);
		if (func==FUNC_SCSI || func==FUNC_TTY) {
			run_fork(func, cdev.action);
		}
	}
	printf("mnet demon end...\n");

	return SUCCESS;
}
#endif /* #ifdef DEMON */

#ifdef OUTPUT
int output(void)
{
    struct sockaddr_nl nls;
    struct pollfd pfd;
    char buf[512];

	memset(&nls, 0, sizeof(nls));
	nls.nl_family = AF_NETLINK;
	nls.nl_pid = getpid();
	nls.nl_groups = -1;

	pfd.events = POLLIN;
	pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (pfd.fd == -1) {
		printf("Not root\n");
		exit(1);
	}

	if (bind(pfd.fd, (void*)&nls, sizeof(nls))) {
		printf("bind failed\n");
		exit(1);
	}
	while (-1 != poll(&pfd, 1, -1)) {
		int i, len = recv(pfd.fd, buf, sizeof(buf), MSG_DONTWAIT);
		if (len == -1) {
			printf("recv failed\n");
			//exit(1);
		}
		i = 0;
		while (i < len) {
			printf("%s\n", buf + i);
			i += strlen(buf+i) + 1;
		}
	}
	printf("mnet output end...\n");

	return SUCCESS;
}
#endif /* #ifdef OUTPUT */

int main(int argc, char *argv[])
{
	printf("\nmnet built on %s\n\n", BUILT_TIME);

#ifdef DEMON
	//printf("%s:%d\n", __FUNCTION__, __LINE__);
	if (argc > 1 && !strcmp(argv[1], "-d")) {
		//printf("%s:%d\n", __FUNCTION__, __LINE__);
		char ttyitf[8];
		//char buf[4];

		system("mdev -s");
		sleep(2);
        
		/* if usb was learnt as tty device at bootup */
		if (SUCCESS == get_tty(ttyitf)) {
			run_fork(FUNC_TTY, DEV_ADD);
			sleep(1);
			BUSYWAIT(FAILURE == isFileExisted("/var/mnet_fork")); // wait here, till mnet_fork does not exist
  		}
		//if (FAILURE == isFileExisted("/var/run/ppp0.pid")) {
		//	g_pwroff=1;
		//}
		//printf("prepare enter demon");
		demon();
	}
#endif

#ifdef OUTPUT
	if (argc > 1 && !strcmp(argv[1], "-o"))
		output();
#endif

	if (argc > 1 && !strcmp(argv[1], "-p"))
		onoff_usbhub();

	//if (argc > 1 && !strcmp(argv[1], "-r")) {
		//if (isPPPRunning()) {
			//system("killall pppd >/dev/null 2>&1");
			//kill_ppp_inet();
			//sleep(3);
		//}
		//system("iwpriv wlan0 write_mem dw,b8021054,1,100000");
	//}

	return 0;
}
