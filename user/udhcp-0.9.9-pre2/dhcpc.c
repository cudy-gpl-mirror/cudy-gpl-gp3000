/* dhcpc.c
 *
 * udhcp DHCP client
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/msg.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <errno.h>
#include <sys/wait.h>

#include "dhcpd.h"
#include "dhcpc.h"
#include "options.h"
#include "clientpacket.h"
#include "packet.h"
#include "script.h"
#include "socket.h"
#include "debug.h"
#include "pidfile.h"

/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
#include "arpping.h"
#include <stdlib.h>
//add by ramen
#include <rtk/utility.h>
#include <sys/sysinfo.h>
#ifdef CONFIG_USER_DHCPCLIENT_CHECK_IP_CONFLICT
#include "arpping.h"
#endif
#ifdef CONFIG_LIB_CAP
#include <sys/capability.h>
#endif
#ifdef _PRMT_X_CT_COM_GATEWAY_ARP_DETECT_
#include <pthread.h>
#endif

static int state;

/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
static u_int32_t requested_ip = 0; /* = 0 */
static u_int32_t router_align = 0;

static u_int32_t server_addr;
static u_int32_t relay_addr = 0;
static time_t timeout_dhcpc = 0;
static time_t timeout_dhcpc_bak = 0;
static time_t timeout_signal_usr1 = 0;
static int packet_num; /* = 0 */

#ifdef CONFIG_USER_DHCP_ROSTELECOM_INTERVAL_RETRY
static int russia_discover_num=0;
#endif

static int fd = -1;
static int signal_pipe[2];
int dhcp_debug_on=0;
static char dhcpc_debug_prefix[]="[udhcpc]";
char *dhcp_debug_prefix=dhcpc_debug_prefix;
char dhcpc_debug_interface[16]="";
char *dhcp_debug_interface=dhcpc_debug_interface;

#if defined(CONFIG_CTC_FTTR_SLAVE) || defined(CONFIG_CMCC_FTTR_SLAVE) || defined(CONFIG_CU_FTTR_SLAVE)
static int no_ip_flag = 0;
#endif

#ifdef SEND_RELEASE_BY_RAWSOCK
static unsigned char server_neigh_addr[6] = {0,0,0,0,0,0};
#endif

#define LISTEN_NONE 0
#define LISTEN_KERNEL 1
#define LISTEN_RAW 2
static int listen_mode;
char province[32]="";

#ifdef DHCPC_DELAY_TO_UP
static int dhcpc_delay=1;                  //used to have a random delay before creat a dhcpc wan
#endif
#ifdef DHCPC_ELAPSED_TIME_SUPPORT
int dhcpc_elapsed_time=0;
#endif
#define DEFAULT_SCRIPT "/etc/scripts/udhcpc.sh"
#define DEFAULT_PID_FILE "/var/run/udhcpc.pid"

#ifdef WLAN_RTK_MULTI_AP
#define AUTO_IP_TIMEOUT 120
#else
#define AUTO_IP_TIMEOUT 300
#endif

#ifdef _PRMT_X_CT_COM_GATEWAY_ARP_DETECT_
unsigned int app_type = 0;
int arp_detect_for_renew = 0;
int force_arp_detect = 0;
#endif

struct client_config_t client_config = {
	/* Default options. */
	abort_if_no_lease: 0,
	foreground: 0,
	quit_after_lease: 0,
	background_if_no_lease: 0,
	microsoft_auto_ip_enable: 0,		/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
	interface: "eth0",
	pidfile: DEFAULT_PID_FILE,
	script: DEFAULT_SCRIPT,
	clientid: NULL,
	hostname: NULL,
#ifdef DHCP_OPTION_125
	vi_vendorSpec: NULL,
#endif
	ifindex: 0,
	arp: "\0\0\0\0\0\0",		/* appease gcc-3.0 */
	Hardware_NAT: 1,
};

#ifdef CONFIG_AUTO_DHCP_CHECK
static int auto_dhcp_pid=0;
static unsigned long pre_server_addr=0;
static unsigned long relay_addr_auto=0;
static unsigned char dhcp_server_sHaddr[6];
#endif

#ifdef DHCP_OPTION_125
unsigned char vi_enterprise_data[] = {
	0x01 , 0x06 , 0x30 , 0x30 , 0x30,
	0x31 , 0x30 , 0x32 , 0x02 , 0x10 , 0x30 , 0x30 , 0x30 , 0x31 , 0x30 , 0x32 , 0x2d , 0x34 , 0x32 , 0x38 , 0x32,
	0x38 , 0x38 , 0x38 , 0x32 , 0x39 , 0x03 , 0x11 , 0x43 , 0x44 , 0x52 , 0x6f , 0x75 , 0x74 , 0x65 , 0x72 , 0x20,
	0x56 , 0x6f , 0x49 , 0x50 , 0x20 , 0x41 , 0x54 , 0x41 };
#endif
/*ql:20080926 START: initial MIB_DHCP_CLIENT_OPTION_TBL*/
#if	defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
unsigned int wan_ifIndex;
#endif
/*ql:20080926 END*/
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterNumberEnable_
unsigned int wan_app;
#endif

#ifndef IN_BUSYBOX
static void __attribute__ ((noreturn)) show_usage(void)
{
	printf(
"Usage: udhcpc [OPTIONS]\n\n"
"  -c, --clientid=CLIENTID         Client identifier\n"
"  -H, --hostname=HOSTNAME         Client hostname\n"
"  -h                              Alias for -H\n"
"  -f, --foreground                Do not fork after getting lease\n"
"  -b, --background                Fork to background if lease cannot be\n"
"                                  immediately negotiated.\n"
"  -i, --interface=INTERFACE       Interface to use (default: eth0)\n"
"  -n, --now                       Exit with failure if lease cannot be\n"
"                                  immediately negotiated.\n"
"  -p, --pidfile=file              Store process ID of daemon in file\n"
"  -q, --quit                      Quit after obtaining lease\n"
"  -r, --request=IP                IP address to request (default: none)\n"
"  -s, --script=file               Run file at dhcp events (default:\n"
"                                  " DEFAULT_SCRIPT ")\n"
"  -v, --version                   Display version\n"
"  -a, --auto IP                   Microsoft AUTO IP enable\n"			/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
	);
	exit(0);
}
#else
extern void show_usage(void) __attribute__ ((noreturn));
#endif
char* state_str[8] = {"INIT", "REQUESTING", "BOUND", 
					  "RENEWING", "REBINDING", 
					  "INIT_REBOOT", "RENEW_REQUESTED", "RELEASED"};

/* RTK add update_state() */
static void update_state(int state)
{
	char buf[32];
	FILE * pFile;
	
	sprintf(buf, "%s.%s", DEFAULT_STATE_FILE, client_config.interface);
	pFile = fopen (buf,"w+");
	if (pFile!=NULL)
	{
		//sprintf(tmp, "%s", state_str[state]);
		fputs(state_str[state],pFile);
		fclose(pFile);
	}
}

void write_ipoe_diag_result(int result, unsigned int ip_got, unsigned int gw_got)
{
	FILE *fp = fopen(IPOE_DIAG_RESULT_DHCPC_FILE, "w");

	if(fp == NULL)
	{
		fprintf(stderr, "Open "IPOE_DIAG_RESULT_DHCPC_FILE" failed: %m\n");
		return;
	}

	fprintf(fp, "Result=");
	switch(result)
	{
	case IPOE_DIAG_RESULT_SendDHCPMsgError:
		fprintf(fp, "SendDHCPMsgError\n");
		break;
	case IPOE_DIAG_RESULT_ServerNotFound:
		fprintf(fp, "ServerNotFound\n");
		break;
	case IPOE_DIAG_RESULT_ServerDeny:
		fprintf(fp, "ServerDeny\n");
		break;
	case IPOE_DIAG_RESULT_GetIPAddressTimeout:
		fprintf(fp, "GetIPAddressTimeout\n");
		break;
	case IPOE_DIAG_RESULT_OK:
		fprintf(fp, "OK\n");
		fprintf(fp, "LocalAddress=%d.%d.%d.%d\n",(ntohl(ip_got)>>24)&0xff,(ntohl(ip_got)>>16)&0xff,(ntohl(ip_got)>>8)&0xff,(ntohl(ip_got)>>0)&0xff);
		fprintf(fp, "DefaultGateway=%d.%d.%d.%d\n",(ntohl(gw_got)>>24)&0xff,(ntohl(gw_got)>>16)&0xff,(ntohl(gw_got)>>8)&0xff,(ntohl(gw_got)>>0)&0xff);
		break;
	default:
		fprintf(fp, "Other\n");
		break;
	}


	fclose(fp);
}

/* just a little helper */
static void change_mode(int new_mode)
{
	DEBUG(LOG_INFO, "entering %s listen mode",
		new_mode ? (new_mode == 1 ? "kernel" : "raw") : "none");
	if (fd >= 0) close(fd);
	fd = -1;
	listen_mode = new_mode;
}


/* perform a renew */
static void perform_renew(void)
{
#ifdef DHCPC_DELAY_TO_UP
	srand(time(NULL));
	int InFlags=1;
	
	if(dhcpc_delay == 1)
	{
		getInFlags(client_config.interface, &InFlags);
		if(InFlags & IFF_RUNNING)
		{
			sleep(rand()%4);
			dhcpc_delay=0;
		}
	}
#endif
	LOG(LOG_INFO, "Performing a DHCP renew");
	switch (state) {
	case BOUND:
		change_mode(LISTEN_KERNEL);
		/* fall through */
	case RENEWING:
	case REBINDING:
#ifdef DHCPC_BROADCAST_RENEW_SUPPORT
		if(!strncmp(province, "fuj", 3)){
			change_mode(LISTEN_RAW);
			state = REQUESTING;
#ifdef _PRMT_X_CT_COM_GATEWAY_ARP_DETECT_
			if(app_type & (X_CT_SRV_TR069|X_CT_SRV_VOICE))
				arp_detect_for_renew = 1;
#endif
		}
		else
#endif
		state = RENEW_REQUESTED;
		break;
	case RENEW_REQUESTED: /* impatient are we? fine, square 1 */
		run_script(NULL, "deconfig");
		/* fall through */
	case REQUESTING:
	case RELEASED:
		change_mode(LISTEN_RAW);
		state = INIT_SELECTING;
		break;
	case INIT_SELECTING:
	/*
	 * We shouldn't reset timeout_dhcpc at state INIT_SELECTING. It will make udhcpc send 2 DISCOVER message
	 * if we got SIGUSR2 at INIT_SELECTING state.
	 */
#if defined(CONFIG_CTC_FTTR_SLAVE) || defined(CONFIG_CMCC_FTTR_SLAVE) || defined(CONFIG_CU_FTTR_SLAVE)
	 	if(no_ip_flag)
			break;
#endif
		update_state(state);
#ifdef CONFIG_RTK_DEV_AP
		packet_num = 0;
		timeout_dhcpc = 0;
#endif
		return;
	default:
		break;			/* Dick Tam, 2003-05-16, in order to eliminate compile warning */
	}

	/* start things over */
	packet_num = 0;

	/* Kill any timeouts because the user wants this to hurry along */
	timeout_dhcpc = 0;
	update_state(state);
}


/* perform a release */
static void perform_release(void)
{
	char buffer[64];
	struct in_addr temp_addr;

	/* send release packet */
	if (state == BOUND || state == RENEWING || state == REBINDING) {
		temp_addr.s_addr = server_addr;
		sprintf(buffer, "%s", inet_ntoa(temp_addr));
		temp_addr.s_addr = requested_ip;
		LOG(LOG_INFO, "Unicasting a release of %s to %s",
				inet_ntoa(temp_addr), buffer);
#ifdef SEND_RELEASE_BY_RAWSOCK
		send_raw_release(server_addr, requested_ip, server_neigh_addr); /* unicast */
#else
		send_release(server_addr, requested_ip); /* unicast */
#endif
		snprintf(buffer, sizeof(buffer)-1, "/tmp/udhcpc_release_%s", client_config.interface);
		if (unlink(buffer) == 0) {
			// printf("%s:%d Sending DHCP release(%s)\n", __FUNCTION__, __LINE__, client_config.interface);
		} else if (errno == ENOENT) {
			run_script(NULL, "deconfig");
		} else {
			LOG(LOG_ERR, "Failed to unlink file: %s", strerror(errno));
		}
	}
	LOG(LOG_INFO, "Entering released state");

	change_mode(LISTEN_NONE);
	state = RELEASED;
	timeout_dhcpc = 0x7fffffff;
	update_state(state);
}


/* Exit and cleanup */
static void exit_client(int retval)
{
	pidfile_delete(client_config.pidfile);
	/*ql:20080926 START: when stop process, clear mib tbl*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	//stopDhcpc();
#endif
	/*ql:20080926 END*/
	CLOSE_LOG();
	exit(retval);
}


/* Signal handler */
static void signal_handler(int sig)
{
	time_t now;
	struct sysinfo s_info;
#ifdef CONFIG_USER_RTK_BRIDGE_MODE
	if (sig == SIGUSR1) {
	#if defined(CONFIG_RTK_DEV_AP)
		if (strcmp(client_config.interface, "br0")==0 && (state == RENEW_REQUESTED))
	#else
		if (strcmp(client_config.interface, "br0")==0 && (state == RENEW_REQUESTED || state == INIT_SELECTING))
	#endif
		{
			//printf("\n%s:%d state=%d there is no need to do renew!!!\n",__FUNCTION__,__LINE__,state);
			return ;
		}	
	}
#endif

	sysinfo(&s_info);
	now = s_info.uptime;
	if(sig == SIGUSR1)
	{
		if((now-timeout_signal_usr1) < 2)
			return ;
	}

	if(sig == SIGUSR1 && client_config.delay)
	{
		LOG(LOG_INFO, "SIGUSR1 got, set delay to 0\n");
		client_config.delay = 0;
	}
	else if (send(signal_pipe[1], &sig, sizeof(sig), MSG_DONTWAIT) < 0) {
		LOG(LOG_ERR, "Could not send signal: %s",
			strerror(errno));
	}
}


static void background(void)
{
#if 0
	int pid_fd;

	pid_fd = pidfile_acquire(client_config.pidfile); /* hold lock during fork. */
	while (pid_fd >= 0 && pid_fd < 3) pid_fd = dup(pid_fd); /* don't let daemon close it */
	if (daemon(0, 0) == -1) {
		perror("fork");
		exit_client(1);
	}
	client_config.foreground = 1; /* Do not fork again. */
	client_config.background_if_no_lease = 0;
	pidfile_write_release(pid_fd);
#endif
}

/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
static struct in_addr rt_microsoft_auto_ip_generate(unsigned char * mac, unsigned int count)
{
	unsigned long seed;
	unsigned char addr[4];
	struct in_addr temp;
	unsigned int i;

	seed = mac[0];
	seed = (seed << 2) ^ mac[1];
	seed = (seed << 2) ^ mac[2];
	seed = (seed << 2) ^ mac[3];
	seed = (seed << 2) ^ mac[4];
	seed = (seed << 2) ^ mac[5];

	srandom(seed);

	for(i=0;i<=count;i++)
	{
		seed = (unsigned long) random();
	}

	// network byte order
	addr[0] = 0xA9;	// 169
	addr[1] = 0xFE;	// 254
	addr[2] = (seed >> 8) & 0xFF;
	addr[3] = seed & 0xFF;

	if((addr[3] == 0x00) || (addr[3] == 0xFE) || (addr[3] == 0xFF))
	{
		// X.X.X.0, X.X.X.255, X.X.X.254 is NOT allowed
		addr[3] = 0x01;
	}

	memcpy(&seed, addr, 4);
	temp.s_addr  =seed;

	return temp;
}

static int rt_microsoft_auto_ip_start(struct in_addr *pResult)
{
	unsigned int i;
	struct in_addr addr;

	for(i=0;i<10;i++)
	{
		addr = rt_microsoft_auto_ip_generate(client_config.arp, i);

		if(arpping(addr.s_addr, 0, client_config.arp, client_config.interface) != 0)
		{	// ARP no response
			*pResult = addr;
			return 0;
		}
	}

	return -1;
}

#ifdef CONFIG_AUTO_DHCP_CHECK
static int save_auto_dhcpc_addr(void)
{
	FILE *fp;
	if ((fp = fopen("/var/udhcpc/auto_dhcp", "w")) == NULL)
	{
		printf("***** Open file /var/udhcpc/auto_dhcp failed !\n");
		return -1;
	}
	
	fprintf(fp, "%s %lu\n", "server_ip", server_addr);
	//fprintf(fp, "%s %x %x %x %x %x %x\n", "client_mac", client_config.arp[0],client_config.arp[1],client_config.arp[2],client_config.arp[3],client_config.arp[4],client_config.arp[5]);
	fprintf(fp, "%s %lu\n", "client_ip", requested_ip);		
	fprintf(fp, "%s %lu\n", "relay_addr_auto", relay_addr_auto);
	fclose(fp);

	//printf("sun test dhcpc get ip = %lu, requested_ip = %lu\n", server_addr, requested_ip);

	return 0;
					
}

#endif

#ifdef CONFIG_USER_DHCP_OPT_GUI_60
static void update_wan_custom_dhcp_options(MIB_CE_ATM_VC_T *pEntry)
{
	int len;

	if(pEntry->enable_opt_60)
	{
		len = strlen(pEntry->opt60_val);
		if(len > 0)
			updateDhcpcOptionTbl(DHCP_VENDOR, (unsigned char *)pEntry->opt60_val, len);
	}

	if(pEntry->enable_opt_61)
	{
/**
Format of option 61. From RFC-4361:

		Code  Len  Type  IAID                DUID
       +----+----+-----+----+----+----+----+----+----+---
       | 61 | n  | 255 | i1 | i2 | i3 | i4 | d1 | d2 |...
       +----+----+-----+----+----+----+----+----+----+---

	DUID format is defined in RFC-3315.
*/
		char data[256] = {255, 0};
		char *cur = data + 1;
		unsigned short type = pEntry->duid_type;
		unsigned short hw_type = 1;

		memcpy(cur, (char *)&pEntry->iaid, 4);
		cur += 4;

		memcpy(cur, (char *)&type, 2);
		cur += 2;

		len = 7;

		switch(type)
		{
		case 1:	//type 1: Link-Layer address plus time
			{
				struct sockaddr hwaddr;
				time_t t;
				char ifname[IFNAMSIZ] = {0};

				//HW type Ethernet (1), no ATM currently
				memcpy(cur, (char *)&hw_type, 2);
				len += 2;
				cur += 2;

				t = time(NULL);
				memcpy(cur, (char *)&t, 4);
				len += 4;
				cur += 4;

				ifGetName(pEntry->ifIndex, ifname, IFNAMSIZ);
				getInAddr(ifname, HW_ADDR, &hwaddr);
				memcpy(cur, hwaddr.sa_data, 6);
				len += 6;

				updateDhcpcOptionTbl(DHCP_CLIENT_ID, (unsigned char *)data, len);
			}
			break;
		case 2:	//type 2: Enterprise Number and Identifier
			{
				int id_len = strlen(pEntry->duid_id);

				memcpy(cur, (char *)&pEntry->duid_ent_num, 4);
				len += 4;
				cur += 4;

				memcpy(cur, pEntry->duid_id, id_len);
				len += id_len;

				updateDhcpcOptionTbl(DHCP_CLIENT_ID, (unsigned char *)data, len);
			}
			break;
		case 3:	//type 3: Link-Layer address
			{
				struct sockaddr hwaddr;
				char ifname[IFNAMSIZ] = {0};

				//HW type Ethernet (1), no ATM currently
				memcpy(cur, (char *)&hw_type, 2);
				len += 2;
				cur += 2;

				ifGetName(pEntry->ifIndex, ifname, IFNAMSIZ);
				getInAddr(ifname, HW_ADDR, &hwaddr);
				memcpy(cur, hwaddr.sa_data, 6);
				len += 6;

				updateDhcpcOptionTbl(DHCP_CLIENT_ID, (unsigned char *)data, len);
			}
			break;
		default:
			fprintf(stderr, "Invalid duid_type %d\n", pEntry->duid_type);
			break;
		}
	}

	/**
		Option 125 is implemented by output of D-Link DSL-6740C.
		Maybe not correct because the output is not fit TR-069 spec.
	*/
	if(pEntry->enable_opt_125)
	{
		char data[256] = {0};
		char *cur = data;
		unsigned char data_len = 0;
		const unsigned int ent_num = 3561;	/* BBF enterprise code */
		unsigned char sub_code;

		memcpy(cur, (char *) &ent_num, 4);
		/*Also skip data length field.*/
		cur += 5;

		for(sub_code = 1 ; sub_code <= 4 ; sub_code++)
		{
			unsigned char sub_len = 0;

			memcpy(cur++, (char *)&sub_code, 1);

			switch(sub_code)
			{
			case 1:
				sub_len = strlen(pEntry->manufacturer);
				memcpy(cur++, (char *)&sub_len, 1);
				memcpy(cur, pEntry->manufacturer, sub_len);
				break;
			case 2:
				sub_len = strlen(pEntry->product_class);
				memcpy(cur++, (char *)&sub_len, 1);
				memcpy(cur, pEntry->product_class, sub_len);
				break;
			case 3:
				sub_len = strlen(pEntry->model_name);
				memcpy(cur++, (char *)&sub_len, 1);
				memcpy(cur, pEntry->model_name, sub_len);
				break;
			case 4:
				sub_len = strlen(pEntry->serial_num);
				memcpy(cur++, (char *)&sub_len, 1);
				memcpy(cur, pEntry->serial_num, sub_len);
				break;
			}
			cur += sub_len;
			data_len += sub_len + 2;
		}

		memcpy(data + 4, (char *)&data_len, 1);
		if(updateDhcpcOptionTbl(DHCP_VI_VENSPEC, (unsigned char *)data, data_len + 5) == 0)
		{
			// add new entry if update failed.
			addDhcpcOptionSent(DHCP_VI_VENSPEC, (unsigned char *)data, data_len + 5);
		}
	}
}
#endif


#ifdef DHCP_OPTION_125
// Kaohj --- calculate VI Vendor-Specific option data
static void get_vendorSpec()
{
	int data_len, option_len;
	struct vendor_info_t *vendor_data;

	if (client_config.vi_vendorSpec) free(client_config.vi_vendorSpec);
	// data-len
	data_len = sizeof(vi_enterprise_data);
	// option-len
	option_len = data_len+4+1;	// enterprise-number(4)+data-len(1)
	client_config.vi_vendorSpec = xmalloc(option_len + 2); // option-code+option-len
	client_config.vi_vendorSpec[OPT_CODE] = DHCP_VI_VENSPEC;
	client_config.vi_vendorSpec[OPT_LEN] = option_len;
	vendor_data = (struct vendor_info_t *)&client_config.vi_vendorSpec[OPT_DATA];
	vendor_data->ent_num = htonl(3561);
	vendor_data->data_len = data_len;
	memcpy(client_config.vi_vendorSpec + OPT_DATA+4+1, vi_enterprise_data, data_len);
}
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
int dhcp_request_interval[3][10]={
	{10,10,60,60,60,60,60,60,60},	//type normal
	{4,4,8,16,32,60,60,60,60,60},	//type fujian
	{10,10,10,60,60,60,60,60,60}	//type sichuan
};
enum
{
	Normal,
	FUJIAN,
	SICHUAN
}PROVINCE_TYPE_T;
char REQ_TYPE=0; //0:normal dhcp req interval, 1:customerized dhcp req interval for fujian province voip
unsigned char province_type=0;
//0: normal wan
//1: voip wan
unsigned int wan_app;
int sent;
#endif

#ifdef CONFIG_00R0
int update_rtc_option60(MIB_CE_ATM_VC_T *pEntry)
{
	int len = 0;
	int province = 0;

	mib_get_s(MIB_ROSTELECOM_PROVINCES, &province, sizeof(int));
	if(province && pEntry->applicationtype & X_CT_SRV_VOICE)
	{
		if(!queryDhcpcOptionTbl(DHCP_VENDOR, &len))
		{
			updateDhcpcOptionTbl(DHCP_VENDOR, "VOIP_Reutov", strlen("VOIP_Reutov"));
			return 0;
		}
	}

	 if(!queryDhcpcOptionTbl(DHCP_VENDOR, &len)) {  /* Iulian Wu, If already have option 60 value, use it.*/
		/* Rosetelecom request option 60 format : VNDR-MODELNAME_SRV */
		unsigned char vendor[256]={0};
		unsigned char modelName[256]={0};
		unsigned char service[256]={0};
		unsigned char buf[256]={0};

		mib_get_s(MIB_PON_VENDOR_ID,(void *) vendor, sizeof(vendor));
		mib_get_s(MIB_SNMP_SYS_NAME,(void *) modelName, sizeof(modelName));

		if((pEntry->applicationtype & X_CT_SRV_VOICE) &&
		!(pEntry->applicationtype & X_CT_SRV_TR069) &&
		!(pEntry->applicationtype & X_CT_SRV_INTERNET))
			strcpy(service, "_VOIP");
		else if ((pEntry->applicationtype & X_CT_SRV_TR069) &&
		!(pEntry->applicationtype & X_CT_SRV_VOICE) &&
		!(pEntry->applicationtype & X_CT_SRV_INTERNET))
			strcpy(service, "_MGMT");
		else if ((pEntry->applicationtype & X_CT_SRV_INTERNET) &&
		!(pEntry->applicationtype & X_CT_SRV_TR069) &&
		!(pEntry->applicationtype & X_CT_SRV_VOICE))
			strcpy(service, "_INTERNET");
		else
			strcpy(service, "");

		if(service[0])
		{
			sprintf(buf, "%s-%s%s", vendor, modelName, service);
			len = strlen(buf);
		}
		if(len > 0)
			updateDhcpcOptionTbl(DHCP_VENDOR, (unsigned char *)buf, len);
	}
}
#endif

#ifdef CONFIG_USER_DHCP_ROSTELECOM_INTERVAL_RETRY
void delete_tmp_option43_file(void)
{
	char opt43_file[64]={0};

	snprintf(opt43_file, sizeof(opt43_file), "/var/option43_%s", client_config.interface);
	if(isFileExist(opt43_file))
		unlink(opt43_file);

	return;
}

void create_tmp_option43_file(void)
{
	char cmdbuf[128]={0};
	snprintf(cmdbuf, sizeof(cmdbuf), "echo 1 > /var/option43_%s", client_config.interface);
	system(cmdbuf);
	return;
}
#endif

#ifdef _PRMT_X_CT_COM_GATEWAY_ARP_DETECT_
void *thread_gw_arp_detect(void *data)
{
	unsigned int arp_detect_timeout = 0;
	unsigned int arp_detect_interval = (unsigned int)data;
	char cmd[256], res_file[64], gw_addr[64];
	int check_arp_detect_result = 0;
	FILE* fp;
	int send_cnt = 0, reply_cnt = 0;
	int sig = SIGUSR1;

	snprintf(res_file, sizeof(res_file), "/tmp/dhcpc_gw_detect_result.%s", client_config.interface);
	while(1) {
		if(state == BOUND) {
			arp_detect_timeout++;
		}
		else if(state != RENEWING && state != RENEW_REQUESTED)
			arp_detect_timeout = 0;
		if(((arp_detect_timeout >= arp_detect_interval) || force_arp_detect) && (router_align != 0)) {
			if(inet_ntop(AF_INET, &router_align, gw_addr, sizeof(gw_addr))){
				LOG(LOG_INFO, "udhcp start gw arp detect for %s(%s)", gw_addr, client_config.interface);
				unlink(res_file);
				snprintf(cmd, sizeof(cmd), "/bin/neighdetect -b -f -c 3 -w 5 -i %s %s > %s &", client_config.interface, gw_addr, res_file);
				system(cmd);
				check_arp_detect_result = 1;
				arp_detect_timeout = 0;
				force_arp_detect = 0;
			}
		}
		else if(check_arp_detect_result) {
			if(fp = fopen(res_file, "r")) {
				send_cnt = 0;
				reply_cnt = 0;
				while(fgets(cmd, sizeof(cmd), fp)) {
					if(sscanf(cmd, "detectresult:send=%d recv=%d", &send_cnt, &reply_cnt) == 2)
						break;
				}
				fclose(fp);
				LOG(LOG_INFO, "udhcp start gw arp detect result: send=%d recv=%d", send_cnt, reply_cnt);
				if(send_cnt > 0) {
					if(reply_cnt == 0){
						//perform renew;
						if (send(signal_pipe[1], &sig, sizeof(sig), MSG_DONTWAIT) < 0) {
							LOG(LOG_ERR, "GW arp detect could not send signal: %s",
								strerror(errno));
						}
					}
					check_arp_detect_result = 0;
				}
			}
		}
		sleep(1);
	}
}

void setup_gw_arp_detect(void)
{
	pthread_t t_id=0;
	unsigned int arp_detect_interval = 0;

	if(strncmp(province, "fuj", 3) || !(app_type & (X_CT_SRV_TR069|X_CT_SRV_VOICE)))
		return;

	mib_get_s(MIB_CWMP_GATEWAY_DETECT_INTERVAL, &arp_detect_interval, sizeof(arp_detect_interval));
	if(arp_detect_interval > 10) {
		LOG(LOG_INFO, "udhcp start gw arp detect thread");
		pthread_create(&t_id, NULL, thread_gw_arp_detect, arp_detect_interval);
		pthread_detach(t_id);
	}
}
#endif

#ifdef COMBINED_BINARY
int udhcpc_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	unsigned char *temp, *message;
	unsigned long t1 = 0, t2 = 0, xid = 0;
	unsigned long start = 0, lease;
	fd_set rfds;
	int retval;
	struct timeval tv;
	int c, len;
	struct dhcpMessage packet;
	struct in_addr temp_addr;
	int pid_fd;
	time_t now;
	int max_fd;
	int sig;
	//int useClaesslessRT;
	int ip_changed=0;
	char if_name[IFNAMSIZ];
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
	int dns_changed=0;
#endif

#ifdef DHCPC_DELAY_TO_UP
	int InFlags=1;
#endif
#if defined(CONFIG_USER_DHCP_OPT_GUI_60) || defined(CONFIG_00R0) || defined(_PRMT_X_CT_COM_GATEWAY_ARP_DETECT_)
	MIB_CE_ATM_VC_T Entry, *pEntry = &Entry;
	int num_entry, idx;
#endif
#ifdef CONFIG_USER_DHCP_OPT_33_249
	char static_route_file[64] = {0};
#endif
	char cmd[64]={0};

	struct sysinfo s_info;
	unsigned char arp_on_bound=0;
	unsigned int arp_times = 0;
	int status;

	static struct option arg_options[] = {
		{"clientid",	required_argument,	0, 'c'},
		{"foreground",	no_argument,		0, 'f'},
		{"background",	no_argument,		0, 'b'},
		{"hostname",	required_argument,	0, 'H'},
		{"hostname",    required_argument,      0, 'h'},
		{"interface",	required_argument,	0, 'i'},
		{"now", 	no_argument,		0, 'n'},
		{"pidfile",	required_argument,	0, 'p'},
		{"quit",	no_argument,		0, 'q'},
		{"request",	required_argument,	0, 'r'},
		{"script",	required_argument,	0, 's'},		
		{"dignostics",	no_argument,	0, 'd'},
		{"version",	no_argument,		0, 'v'},
		{"help",	no_argument,		0, '?'},
		{"autoip",	no_argument,		0, 'a'},		/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
		{"delay",	required_argument,	0, 'D'},	// Delay before send first discover
		{0, 0, 0, 0}
	};

#ifdef CONFIG_LIB_CAP
	set_process_capability();
#endif

	/* If we receive SIGUSR1 or SIGUSR2 during initialization, udhcpc will be crashed. */
	/* So we ignore these 2 signal first. */
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	mib_get_s(MIB_HW_PROVINCE_NAME, province, sizeof(province));
	/* get options */
	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "c:fbH:h:i:np:qr:s:dv:aeWD:", arg_options, &option_index);		/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */

		if (c == -1) break;

		switch (c) {
		case 'c':
			len = strlen(optarg) > 255 ? 255 : strlen(optarg);
			if (client_config.clientid) free(client_config.clientid);
			client_config.clientid = xmalloc(len + 2);
			client_config.clientid[OPT_CODE] = DHCP_CLIENT_ID;
			client_config.clientid[OPT_LEN] = len;
			client_config.clientid[OPT_DATA] = '\0';
			strncpy((char *)client_config.clientid + OPT_DATA, optarg, len);
			break;
		case 'f':
			client_config.foreground = 1;
			break;
		case 'b':
			client_config.background_if_no_lease = 1;
			break;
		case 'h':
		case 'H':
			len = strlen(optarg) > 255 ? 255 : strlen(optarg);
			if (client_config.hostname) free(client_config.hostname);
			client_config.hostname = xmalloc(len + 2);
			client_config.hostname[OPT_CODE] = DHCP_HOST_NAME;
			client_config.hostname[OPT_LEN] = len;
			strncpy((char *)client_config.hostname + 2, optarg, len);
			break;
		case 'i':
			client_config.interface =  optarg;
// Mason Yu. Use -i instead od -w
#if	defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) 
			wan_ifIndex = getIfIndexByName(client_config.interface);
			fprintf(stderr,"%s %d wan_ifIndex=%d client_config.interface=%s\n",__FUNCTION__,__LINE__,wan_ifIndex,client_config.interface);
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
			wan_app = getapplicationtypeByName(client_config.interface);
#endif
			snprintf(dhcp_debug_interface,16,"[%s]",client_config.interface);
			break;
		case 'n':
			client_config.abort_if_no_lease = 1;
			break;
		case 'p':
			client_config.pidfile = optarg;
			break;
		case 'q':
			client_config.quit_after_lease = 1;
			break;
		case 'r':
			requested_ip = inet_addr(optarg);
			break;
		case 's':
			client_config.script = optarg;
			break;
		case 'd':
			client_config.is_ipoe_diag = 1;
			break;
		case 'v':
			printf("udhcpcd, version %s\n\n", VERSION);
			exit_client(0);
			break;
		/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
		case 'a':
			client_config.microsoft_auto_ip_enable = 1;
			break;
		case 'W':
			client_config.Hardware_NAT = 0;
			break;
		case 'D':
			client_config.delay = atoi(optarg);
			break;
		case 'e':
			dhcp_debug_on = 1;
#if defined(_DHCP_DEBUG_ON_OFF_)
			dhcp_debug_onoff_by_file(DHCPDEBUG_FLAG_WRITE);
#endif
			break;
		default:
			show_usage();
		}
	}

	//use /tmp/.udhcpc.renew.%s to sure dhcpc receiced USR1 signanl
	snprintf(cmd, sizeof(cmd), "rm %s_%s", DHCPC_RENEWFILE, client_config.interface);
	system(cmd);

	// Add by Dick
	// No daemon DHCP client, only foreground for ucLinux
	client_config.foreground = 1;

	if (client_config.is_ipoe_diag && client_config.interface)
	{
		unsigned int major = 0, minor = 0, simu = 0;
		char real_ifname[16] = {0};
		if (sscanf(client_config.interface, "nas%u_%u_%u", &major, &minor, &simu) == 3)
		{
			sprintf(real_ifname, "nas%u_%u", major, minor);
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) 
			wan_ifIndex = getIfIndexByName(real_ifname);
			fprintf(stderr,"(is_ipoe_diag) wan_ifIndex = %u, client_config.interface = %s, real_ifname = %s\n", wan_ifIndex, client_config.interface, real_ifname);
#endif
		}
	}

	OPEN_LOG("udhcpc");
	//LOG(LOG_INFO, "udhcp client (v%s) started", VERSION);
	printf("[udhcpc]udhcp client (v%s) started, interface=%s\n", VERSION, client_config.interface);

#ifdef CONFIG_E8B
	mib_get_s(PROVINCE_DHCPC_ARP_ON_BOUND,&arp_on_bound, sizeof(arp_on_bound));
#endif
	if(arp_on_bound)
		DEBUG(LOG_INFO, "enable arp on bound, go back to init state when lost gateway arp");

	// Install signal handler before creating pid file to prevent potential crash.
	// The default action for SIGUSR1 and SIGUSR2 is Term.
	/* setup signal handlers */
	socketpair(AF_UNIX, SOCK_STREAM, 0, signal_pipe);
	signal(SIGUSR1, signal_handler);
	signal(SIGCHLD, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	
	pid_fd = pidfile_acquire(client_config.pidfile);
	pidfile_write_release(pid_fd);

	if (client_config.interface) {
		if (read_interface(client_config.interface, &client_config.ifindex,
				   NULL, client_config.arp) < 0)
			exit_client(1);
	}

	if (!client_config.clientid) {
		client_config.clientid = xmalloc(6 + 3);
		client_config.clientid[OPT_CODE] = DHCP_CLIENT_ID;
		client_config.clientid[OPT_LEN] = 7;
		client_config.clientid[OPT_DATA] = 1;
		memcpy(client_config.clientid + 3, client_config.arp, 6);
	}

#ifdef DHCP_OPTION_125
	get_vendorSpec();
#endif
	/*ql:20080926 START: initial MIB_DHCP_CLIENT_OPTION_TBL*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	initialDhcpcOptionTbl();
#endif
	/*ql:20080926 END*/

	/*ql:20080925 START: modify entry in MIB_DHCP_CLIENT_OPTION_TBL*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	unsigned char clientid[10];
	clientid[0] = 1;
	memcpy(clientid+1, client_config.arp, 6);
	updateDhcpcOptionTbl(DHCP_CLIENT_ID, clientid, 7);
#endif
	/*ql:20080925 END*/

#if defined(CONFIG_USER_DHCP_OPT_GUI_60) || defined(CONFIG_00R0) || defined(_PRMT_X_CT_COM_GATEWAY_ARP_DETECT_)
	num_entry = mib_chain_total(MIB_ATM_VC_TBL);
	for( idx = 0; idx < num_entry; idx++ )
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, idx, (void *)pEntry))
			continue;

		ifGetName(pEntry->ifIndex, if_name, IFNAMSIZ);
		if(!strcmp(if_name, client_config.interface))
		{
#ifdef _PRMT_X_CT_COM_GATEWAY_ARP_DETECT_
			app_type = pEntry->applicationtype;
#endif
#if defined(CONFIG_USER_DHCP_OPT_GUI_60) || defined(CONFIG_00R0)
#if 0
			update_rtc_option60(pEntry);
#else
			update_wan_custom_dhcp_options(pEntry);
#endif
#endif
			break;
		}
	}
#endif

	// Move upward to install before creating pid file.
	#if 0
	/* setup signal handlers */
	socketpair(AF_UNIX, SOCK_STREAM, 0, signal_pipe);
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);
	signal(SIGTERM, signal_handler);
	#endif

	state = INIT_SELECTING;
	run_script(NULL, "deconfig");
	change_mode(LISTEN_RAW);
#ifdef DHCPC_DELAY_TO_UP
	srand(time(NULL));
	getInFlags(client_config.interface, &InFlags);
	if(InFlags & IFF_RUNNING)
	{
		sleep(rand()%4);
		dhcpc_delay=0;
	}
#endif

	// wait for timeout or SIGUSR1 to continue
	LOG(LOG_INFO, "udhcp client wait for %d seconds or SIGUSR1 to start\n", client_config.delay);
	while(client_config.delay > 0)
		client_config.delay = sleep(client_config.delay);

	// delay install SIGUSR2 to prevent device cannot get IP address on startup
	// start delay --> sigusr1 and set delay to 0 --> sigusr2 --> failed to get IP becasue first sigusr1 is used to set delay to 0
	signal(SIGUSR2, signal_handler);
#ifdef _PRMT_X_CT_COM_GATEWAY_ARP_DETECT_
	setup_gw_arp_detect();
#endif

#ifdef CONFIG_USER_DHCP_ROSTELECOM_INTERVAL_RETRY
	delete_tmp_option43_file();
#endif
	for (;;) {
#ifdef CONFIG_USER_DHCPCLIENT_CHECK_IP_CONFLICT
		unsigned char ip_conflict;
#endif
		struct in_addr inAddr={0};
#if defined(CONFIG_USER_CWMP_TR069)
		int cwmp_msgid = 0;
		struct cwmp_message cwmpmsg;
#endif

#if defined(_DHCP_DEBUG_ON_OFF_)
		/* Enable dhcp_debug dynamically */
		dhcp_debug_onoff_by_file(DHCPDEBUG_FLAG_READ);
#endif

		ip_changed = 0;
		sysinfo(&s_info);
		tv.tv_sec = timeout_dhcpc - s_info.uptime;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);
#if defined(CONFIG_CTC_FTTR_SLAVE) || defined(CONFIG_CMCC_FTTR_SLAVE) || defined(CONFIG_CU_FTTR_SLAVE)
		LOG(LOG_DEBUG, "line-%d packet_num=%d,timeout_dhcpc=%d,tv.tv_sec=%d\n",__LINE__,packet_num,timeout_dhcpc,tv.tv_sec);
#endif
		if (listen_mode != LISTEN_NONE && fd < 0) {
			if (listen_mode == LISTEN_KERNEL)
				fd = listen_socket(INADDR_ANY, CLIENT_PORT, client_config.interface);
			else
				fd = raw_socket(client_config.ifindex);
			if (fd < 0) {
				LOG(LOG_ERR, "FATAL: couldn't listen on socket, %s", strerror(errno));
				exit_client(0);
			}
		}
		if (fd >= 0) FD_SET(fd, &rfds);
		FD_SET(signal_pipe[0], &rfds);

		if (tv.tv_sec > 0) {
			DEBUG(LOG_INFO, "Waiting on select...\n");
			max_fd = signal_pipe[0] > fd ? signal_pipe[0] : fd;
			retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
		} else retval = 0; /* If we already timed out, fall through */

		sysinfo(&s_info);
		now = s_info.uptime;
		if (retval == 0) {
			/* timeout dropped to zero */
			LOG(LOG_DEBUG, "<line-%d:timeout> client state=%d", __LINE__, state);
			switch (state) {
			case INIT_SELECTING:
#if defined(CONFIG_E8B) && !defined(FAST_SEND_DISCOVER)
				if (packet_num < 2) 
#else
#ifdef FAST_SEND_DISCOVER
				if (packet_num < 30) 
#else
				if (packet_num < 3) 
#endif
#endif
				{
					if (packet_num == 0)
					{
#ifdef DHCPC_ELAPSED_TIME_SUPPORT
						if(!strncmp(province, "fuj",3)){
							dhcpc_elapsed_time = 0;
							start = now;
						}
#endif
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterNumberEnable_
						if(wan_app & X_CT_SRV_TR069){
							printf("dhcpc Register!! consider packets with the same xid as one register\n");
							addDHCPRegisterNumber();
						}
					
#endif
						mib_get_s(PROVINCE_VOICE_REGISTER_DHCP_INTERVAL,(void *)&province_type, sizeof(province_type));
						LOG(LOG_DEBUG, "line-%d wan_app=%d REQ_TYPE=%d packet_num=%d province_type=%d\n",__LINE__,wan_app,REQ_TYPE,packet_num,province_type);
						if(province_type > 0)
						{
							//setup for fujian/sichuan special case dhcp request
							if(wan_app & (X_CT_SRV_VOICE|X_CT_SRV_TR069))
							{
								REQ_TYPE = province_type;
							//change dhcp request time interval for fujian 
							}
							else
							{
								REQ_TYPE = Normal;
							}
						}
						else
						{
							//normal mode
							REQ_TYPE = Normal;
						}
#endif						
						xid = random_xid();
					}
#ifdef DHCPC_ELAPSED_TIME_SUPPORT
					else {
						if(!strncmp(province, "fuj",3)){
							dhcpc_elapsed_time = now - start;
						}
					}
#endif

					/* send discover packet */
#ifndef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
					send_discover(xid, requested_ip); /* broadcast */
#else
					send_discover(xid); /* broadcast */
#endif

#if defined(CONFIG_E8B) && !defined(FAST_SEND_DISCOVER)
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
					if(!REQ_TYPE)
					{
						timeout_dhcpc = now + DHCPC_DISCOVERY_TIMEOUT_PHASE_1;
					}
					else
					{
						LOG(LOG_DEBUG, "line-%d REQ_TYPE=%d, packet_num=%d interval=%d\n",__LINE__,REQ_TYPE,packet_num,dhcp_request_interval[REQ_TYPE][packet_num]);					
						timeout_dhcpc = now + dhcp_request_interval[REQ_TYPE][packet_num];					
					}
#else
					timeout_dhcpc = now + DHCPC_DISCOVERY_TIMEOUT_PHASE_1;
#endif
#else
#ifdef FAST_SEND_DISCOVER					
					timeout_dhcpc = now + 2;
#else
					timeout_dhcpc = now + ((packet_num == 2) ? 4 : 2);
#endif
#endif
					packet_num++;
#ifdef CONFIG_USER_DHCP_ROSTELECOM_INTERVAL_RETRY   //interval 0,2,4,8,16,32,32,32,32,32....
					russia_discover_num++;
					if(russia_discover_num<6)
						timeout_dhcpc = now + (1<<russia_discover_num);
					else
						timeout_dhcpc = now + 32;
#endif
#if defined(CONFIG_CTC_FTTR_SLAVE) || defined(CONFIG_CMCC_FTTR_SLAVE) || defined(CONFIG_CU_FTTR_SLAVE)
					no_ip_flag = 0;
#endif
				} else {
					/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
					struct in_addr auto_addr;

					if(client_config.microsoft_auto_ip_enable &&
						(requested_ip != 0) && 	(router_align != 0) &&
						(arpping(requested_ip, 0, client_config.arp, client_config.interface) != 0) &&
						(arpping(router_align, requested_ip, client_config.arp, client_config.interface) == 0)) {
						// Microsoft AUTO IP Procedure success, with valid lease
						struct dhcpMessage packet;
						auto_addr.s_addr = requested_ip;

						rt_prepare_microsoft_auto_ip_dhcpack(&packet, auto_addr.s_addr);
						run_script(&packet, "bound");

						LOG(LOG_INFO, "Microsoft AUTO IP success with valid lease %s",inet_ntoa(auto_addr));

						if (client_config.quit_after_lease)
							exit_client(0);
						if (!client_config.foreground)
							background();

						/* wait to try again */
						packet_num = 0;
#ifdef FAST_LEASE_DEBUG
						timeout_dhcpc = now + 30;
#else
#ifdef FAST_SEND_DISCOVER
						timeout_dhcpc = now + 5;
#else
						timeout_dhcpc = now + AUTO_IP_TIMEOUT;
#endif
#endif
#ifdef CONFIG_USER_DHCP_ROSTELECOM_INTERVAL_RETRY   //interval 0,2,4,8,16,32,32,32,32,32....
						if(russia_discover_num<6)
							timeout_dhcpc = now + (1<<russia_discover_num);
						else
							timeout_dhcpc = now + 32;
#endif
					} else if(client_config.microsoft_auto_ip_enable &&
						(rt_microsoft_auto_ip_start(&auto_addr) != -1)){
						// Microsoft AUTO IP Procedure success
						struct dhcpMessage packet;

						rt_prepare_microsoft_auto_ip_dhcpack(&packet, auto_addr.s_addr);
						run_script(&packet, "bound");

						LOG(LOG_INFO, "Microsoft AUTO IP success %s",inet_ntoa(auto_addr));

						if (client_config.quit_after_lease)
							exit_client(0);
						if (!client_config.foreground)
							background();

						router_align = 0;
						requested_ip = auto_addr.s_addr;

						/* wait to try again */
						packet_num = 0;
#ifdef FAST_LEASE_DEBUG
						timeout_dhcpc = now + 30;
#else
#ifdef FAST_SEND_DISCOVER
						timeout_dhcpc = now + 5;
#else
						timeout_dhcpc = now + AUTO_IP_TIMEOUT;
#endif
#endif
#ifdef CONFIG_USER_DHCP_ROSTELECOM_INTERVAL_RETRY   //interval 0,2,4,8,16,32,32,32,32,32....
						if(russia_discover_num<6)
							timeout_dhcpc = now + (1<<russia_discover_num);
						else
							timeout_dhcpc = now + 32;
#endif
					} else {	// non-AUTO IP case
						if(client_config.is_ipoe_diag)
						{
							write_ipoe_diag_result(IPOE_DIAG_RESULT_ServerNotFound, 0, 0);
							exit_client(0);
						}
						if (client_config.background_if_no_lease) {
							LOG(LOG_INFO, "No lease, forking to background.");
							background();
						} else if (client_config.abort_if_no_lease) {
							LOG(LOG_INFO, "No lease, failing.");
							exit_client(1);
					  	}
						/* wait to try again */
						
#ifdef FAST_LEASE_DEBUG
						timeout_dhcpc = now + 30;
						packet_num=0;
#elif defined(CONFIG_E8B)
#if defined(CONFIG_YUEME) || defined(CONFIG_CU_BASEON_YUEME)
						//LOG(LOG_DEBUG, "line-%d wan_app=%d REQ_TYPE=%d packet_num=%d FUJIAN=%d\n",__LINE__,wan_app,REQ_TYPE,packet_num,FUJIAN);					
						if(REQ_TYPE)
						{
							if(packet_num >= 9)//reste dhcp to init state
								packet_num = 0;
							timeout_dhcpc = now + dhcp_request_interval[REQ_TYPE][packet_num];					
							LOG(LOG_DEBUG, "line-%d REQ_TYPE=%d, packet_num=%d interval=%d\n",__LINE__,REQ_TYPE,packet_num,dhcp_request_interval[REQ_TYPE][packet_num]);					
						}
						else
#endif							
							timeout_dhcpc = now + DHCPC_DISCOVERY_TIMEOUT_PHASE_2;
#ifdef DHCPC_ELAPSED_TIME_SUPPORT
						if(!strncmp(province, "fuj",3)) {
							dhcpc_elapsed_time = now - start;
						}
#endif
#ifndef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
						send_discover(xid, requested_ip); /* broadcast */
#else
						send_discover(xid); /* broadcast */
#endif
						packet_num++;
#ifdef DHCPC_ELAPSED_TIME_SUPPORT
						if(!strncmp(province, "fuj",3)) {
							if(dhcpc_elapsed_time >= 304)
							{
								dhcpc_elapsed_time = 0;
								start = now;
								packet_num = 0;
							}
						}
#endif
#else
//						timeout = now + 60;
						timeout_dhcpc = now + DHCPC_DISCOVERY_TIMEOUT_PHASE_2;
						packet_num=0;
#endif
#if defined(CONFIG_CTC_FTTR_SLAVE) || defined(CONFIG_CMCC_FTTR_SLAVE) || defined(CONFIG_CU_FTTR_SLAVE)
						no_ip_flag = 1;
#endif

#ifdef CONFIG_USER_DHCP_ROSTELECOM_INTERVAL_RETRY   //interval 0,2,4,8,16,32,32,32,32,32....
					russia_discover_num++;
					if(russia_discover_num<6)
						timeout_dhcpc = now + (1<<russia_discover_num);
					else
						timeout_dhcpc = now + 32;
#endif
//						LOG(LOG_INFO, "No lease, wait 60 second to retry.");
//						LOG(LOG_INFO, "No lease, wait 30 second to retry.");
					}
				}
				
	#ifdef CONFIG_USER_ANDLINK_FST_BOOT_MONITOR	
				LOG(LOG_DEBUG,"[%s:%d] interface=%s, packet_num=%d", __FUNCTION__,__LINE__,client_config.interface,packet_num);
				notify_fstBootMonitor(client_config.interface, packet_num, 0);			
	#endif
	
				break;
			case RENEW_REQUESTED:
			case REQUESTING:
				if (packet_num < 3) {
#ifdef DHCPC_ELAPSED_TIME_SUPPORT
					if(!strncmp(province, "fuj",3)) {
						dhcpc_elapsed_time = 0;
					}
#endif
					/* send request packet */
					if (state == RENEW_REQUESTED)
						send_renew(xid, server_addr, requested_ip); /* unicast */
					else send_selecting(xid, server_addr, requested_ip); /* broadcast */

					timeout_dhcpc = now + ((packet_num == 2) ? 10 : 2);
					packet_num++;
				} else {
					/* timed out, go back to init state */
#ifdef CONFIG_00R0
//Ruby Hack here, because Rostelecom's server may delay the reply packet
					packet_num = 0;
#else
#ifdef _PRMT_X_CT_COM_GATEWAY_ARP_DETECT_
					arp_detect_for_renew = 0;
#endif
#ifdef DHCPC_BROADCAST_RENEW_SUPPORT
					if ((state == RENEW_REQUESTED) || (strncmp(province, "fuj", 3) == 0)) {
						run_script(NULL, "deconfig");
					}
#else
					if (state == RENEW_REQUESTED) run_script(NULL, "deconfig");
#endif
					state = INIT_SELECTING;
					timeout_dhcpc = now;
					packet_num = 0;
					change_mode(LISTEN_RAW);
#endif
				}
				break;
			case BOUND:
				if(router_align && arp_times)
				{//check arp when arp_times not 0
					//printf("cannot receive arp reply anyway");
					//arpping(server_addr, requested_ip, client_config.arp, client_config.interface);
					va_cmd("/bin/ping",7,0,inet_ntoa(*((struct in_addr *)&router_align)), "-c", "1", "-w","1", "-I", client_config.interface);
					if(checkGWARP(router_align)==0){
						//no arp found
						timeout_dhcpc=now+1;
						arp_times--;
						if(arp_times == 0){//max arp time reach and still
							LOG(LOG_INFO,"max arp time reach and still not GW arp, return to INIT_SELECTING, time=%lu", time(0));
							run_script(NULL, "deconfig");
							state = INIT_SELECTING;
							timeout_dhcpc = now;
							requested_ip = 0;
							relay_addr = 0;
							packet_num = 0;
							change_mode(LISTEN_RAW);
							sleep(1); 	
							continue;
						}
					}
					else//arp found 
					{
						LOG(LOG_INFO,"found arp, recover timeout: timeout_dhcpc = %lu,timeout_dhcpc_bak=%lu\n",timeout_dhcpc,timeout_dhcpc_bak);
						arp_times=0;
						timeout_dhcpc = timeout_dhcpc_bak;
					}
					continue;
				}
				/* Lease is starting to run out, time to enter renewing state */
				state = RENEWING;
				change_mode(LISTEN_KERNEL);
				DEBUG(LOG_INFO, "Entering renew state");
				/* fall through */
				/* fall right through */
			case RENEWING:
				/* Either set a new T1, or enter REBINDING state */
				if ((t2 - t1) <= (lease / 14400 + 1)) {
					/* timed out, enter rebinding state */
					state = REBINDING;
					timeout_dhcpc = now + (t2 - t1);
					// Mason Yu. it should change listen mode to raw
					change_mode(LISTEN_RAW);
					DEBUG(LOG_INFO, "Entering rebinding state");
				} else {
					/* send a request packet */
					send_renew(xid, server_addr, requested_ip); /* unicast */

					t1 = (t2 - t1) / 2 + t1;
					timeout_dhcpc = t1 + start;
				}
				break;
			case REBINDING:
				/* Either set a new T2, or enter INIT state */
				if ((lease - t2) <= (lease / 14400 + 1)) {
					/* timed out, enter init state */
					state = INIT_SELECTING;
					LOG(LOG_INFO, "Lease lost, entering init state");
					run_script(NULL, "deconfig");
					timeout_dhcpc = now;
					packet_num = 0;
					change_mode(LISTEN_RAW);
				} else {
					/* send a request packet */
					send_renew(xid, 0, requested_ip); /* broadcast */

					t2 = (lease - t2) / 2 + t2;
					timeout_dhcpc = t2 + start;
				}
				break;
			case RELEASED:
				/* yah, I know, *you* say it would never happen */
				timeout_dhcpc = 0x7fffffff;
				break;
			}
		} else if (retval > 0 && listen_mode != LISTEN_NONE && FD_ISSET(fd, &rfds)) {
			/* a packet is ready, read it */
			LOG(LOG_DEBUG, "A packet is ready, read it");
			//useClaesslessRT = 0;
			if (listen_mode == LISTEN_KERNEL)
				len = get_packet(&packet, fd);
			else 
#if defined(SEND_RELEASE_BY_RAWSOCK)
				len = get_raw_packet(&packet, fd, &server_neigh_addr[0]);
#else
				len = get_raw_packet(&packet, fd);
#endif

			if (len == -1 && errno != EINTR) {
				DEBUG(LOG_INFO, "error on read, %s, reopening socket", strerror(errno));
				change_mode(listen_mode); /* just close and reopen */
			}
			if (len < 0) continue;

			if (packet.xid != xid) {
				DEBUG(LOG_INFO, "Ignoring XID %lx (our xid is %lx)",
					(unsigned long) packet.xid, xid);
#ifndef CONFIG_00R0
//Ruby Hack here, ingore XID mismatch
//TODO: if not this issue, need to modify back
				continue;
#endif
			}

#ifdef DHCPC_RIGOROUSNESS_SUPPORT
			//check if Client MAC is matched
			if (memcmp(packet.chaddr, client_config.arp, 6)) {
				DEBUG(LOG_INFO, "Ignoring Client MAC %02x-%02x-%02x-%02x-%02x-%02x (our Client MAC is %02x-%02x-%02x-%02x-%02x-%02x)",
					packet.chaddr[0], packet.chaddr[1], packet.chaddr[2], packet.chaddr[3], packet.chaddr[4], packet.chaddr[5], 
					client_config.arp[0], client_config.arp[1], client_config.arp[2], client_config.arp[3], client_config.arp[4], client_config.arp[5]);
				continue;
			}
#endif

			if ((message = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL) {
				DEBUG(LOG_ERR, "couldnt get option from packet -- ignoring");
				continue;
			}

			LOG(LOG_DEBUG, "<line-%d:read packet> client state=%d", __LINE__, state);		//record the state
			switch (state) {
			case INIT_SELECTING:
				/* Must be a DHCPOFFER to one of our xid's */
				if (*message == DHCPOFFER) {
#ifdef RECEIVE_DHCP_OPTION_125
					// DealOption125
					if((temp = get_option(&packet, DHCP_VI_VENSPEC))){						
						if (!dealOption125(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_VI_VENSPEC option from packet -- ignoring");
							continue;
						}
					}
#endif

					if ((temp = get_option(&packet, DHCP_SERVER_ID))) {
						memcpy(&server_addr, temp, 4);
						xid = packet.xid;
						requested_ip = packet.yiaddr;

						/* enter requesting state */
						state = REQUESTING;
						timeout_dhcpc = now;
						packet_num = 0;
					} else {
						DEBUG(LOG_ERR, "No server ID in message");
					}
					relay_addr = packet.giaddr;
					if(packet.giaddr!=0)
						DEBUG(LOG_INFO, "relay agent aligned in message");
	
				}
				break;
			case RENEW_REQUESTED:
			case REQUESTING:
			case RENEWING:
			case REBINDING:
				if (*message == DHCPACK) {
					LOG(LOG_INFO, "Received DHCP ACK");
#ifdef DHCPC_RIGOROUSNESS_SUPPORT
					//check if DHCP Server Identifier is matched
					if ((temp = get_option(&packet, DHCP_SERVER_ID))) {
						unsigned int serverID;

						memcpy(&serverID, temp, 4);
						if (serverID != server_addr) {
							DEBUG(LOG_ERR, "Ignoring DHCP Server Identifier %u (our ID is %u)",
								serverID, server_addr);
							continue;
						}
					}
#endif
#ifdef CONFIG_USER_DHCPCLIENT_CHECK_IP_CONFLICT
					mib_get_s(MIB_DHCPC_IP_CONFLICT, (void *)&ip_conflict, sizeof(ip_conflict));
					if (ip_conflict == 1) {
						if(!(state == RENEWING || state == REBINDING))
						{
							int probe_sent = 0, conflict = 0;
							/*
							* RFC5227: should then send PROBE_NUM(default 3) probe packets.
							*/
							while(probe_sent < ARP_PROBE_NUM)
							{
								if (do_one_arp_probe(packet.yiaddr, client_config.arp, client_config.interface) == 0)
								{
									conflict = 1;
									break;
								}
								probe_sent++;
							}
							if(conflict)
							{
								send_decline();
								state = INIT_SELECTING;
								LOG(LOG_INFO, "Lease lost, entering init state");
								run_script(NULL, "deconfig");
								timeout_dhcpc = now;
								packet_num = 0;
								change_mode(LISTEN_RAW);
								break;
							}
						}
					}
#endif
#ifdef RECEIVE_DHCP_OPTION_125
					// DealOption125
					if((temp = get_option(&packet, DHCP_VI_VENSPEC))){						
						if (!dealOption125(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_VI_VENSPEC option from packet -- ignoring");
							continue;
						}
					}
#endif
#ifdef _CONFIG_DHCPC_OPTION43_ACSURL_
#ifdef CONFIG_USER_DHCP_ROSTELECOM_INTERVAL_RETRY
					delete_tmp_option43_file();
#endif
					if((temp = get_option(&packet, DHCP_VENDOR_SPECIFIC_INFO))){
						if (!dealOption43(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_VENDOR_SPECIFIC_INFO option from packet -- ignoring");
							continue;
						}
#ifdef CONFIG_USER_DHCP_ROSTELECOM_INTERVAL_RETRY
						create_tmp_option43_file();
#endif
					}
#endif
#ifdef CONFIG_USER_RTK_VOIP
					if((temp = get_option(&packet, DHCP_SIP_SERVER))){
						if (!dealOption120(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_SIP_SERVER option from packet -- ignoring");
							continue;
						}
					}
#endif
					if((temp = get_option(&packet, DHCP_DOMAIN_NAME))){
						if (!dealOption15(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_DOMAIN_NAME option from packet -- ignoring");
							continue;
						}
					}
					
					if (!(temp = get_option(&packet, DHCP_LEASE_TIME))) {
						LOG(LOG_ERR, "No lease time with ACK, using 1 hour lease");
						lease = 60 * 60;
					} else {
						memcpy(&lease, temp, 4);
						lease = ntohl(lease);
					}

					// DealOption20
					if((temp = get_option(&packet, DHCP_SOURCE_ROUTE))){
						if (!dealOption20(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_SOURCE_ROUTE option from packet -- ignoring");
							continue;
						}								
					}

#ifdef TIME_ZONE
					// DealOption42
					if((temp = get_option(&packet, DHCP_NTP_SERVER))){
						if (!dealOption42(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_NTP_SERVER option from packet -- ignoring");
							continue;
						}
					}
#endif					
					// DealOption66
					if((temp = get_option(&packet, DHCP_TFTP_SERVER))){
						if (!dealOption66(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_TFTP_SERVER option from packet -- ignoring");
							continue;
						}
					}

					// DealOption67
					if((temp = get_option(&packet, DHCP_BOOT_FILENAME))){
						if (!dealOption67(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_BOOT_FILENAME option from packet -- ignoring");
							continue;
						}
					}

					
					if((temp = get_option(&packet, DHCP_POSIX_TZ_STRING))){
						if (!dealOption100(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_POSIX_TZ_STRING option from packet -- ignoring");
							continue;
						}
					}
#ifdef CONFIG_IPV6_SIT_6RD
					if((temp = get_option(&packet, DHCP_6RD))){
						if (!dealOption212(temp)) {
							DEBUG(LOG_ERR, "couldnt get DHCP_6RD option from packet -- ignoring");
							continue;
						}
					}
#endif
#ifdef _PRMT_X_CT_COM_GATEWAY_ARP_DETECT_
					if(arp_detect_for_renew == 1) {
						char cmd[256] = {0}, res_file[64] = {0}, gateway_addr[32] = {0};
						FILE *fp;
						int send_cnt = 0, reply_cnt = 0;

						if((temp = get_option(&packet, DHCP_ROUTER)) && inet_ntop(AF_INET, temp, gateway_addr, sizeof(gateway_addr))) {
							snprintf(res_file, sizeof(res_file), "/tmp/dhcpc_gw_detect_result.%s", client_config.interface);
							snprintf(cmd, sizeof(cmd), "/bin/neighdetect -b -c 1 -w 5 -i %s %s > %s", client_config.interface, gateway_addr, res_file);
							system(cmd);
							LOG(LOG_INFO,"check gateway arp for %s\n", gateway_addr);
							if(fp = fopen(res_file, "r")) {
								fgets(cmd, sizeof(cmd), fp);
								fclose(fp);
								if(sscanf(cmd, "detectresult:send=%d recv=%d", &send_cnt, &reply_cnt) == 2) {
									if((send_cnt > 0) && (reply_cnt == 0)) {
										LOG(LOG_ERR, "ARP detect for gateway %s fail! Ignore DHCP ack!", gateway_addr);
										continue;
									}
								}
							}
						}
						arp_detect_for_renew = 0;
					}
					else if(state == REQUESTING)
						force_arp_detect = 1;
#endif
#ifdef CONFIG_TELMEX_DEV
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
					if ((temp = get_option(&packet, DHCP_DNS_SERVER))) {
						if (dealOption6(temp)) {
							dns_changed = 1;
						}
					}
#endif
#endif

#ifdef FAST_LEASE_DEBUG
					lease = 30;
#endif
					/* enter bound state */
					t1 = lease / 2;

					/* little fixed point for n * .875 */
					t2 = (lease * 0x7) >> 3;
					temp_addr.s_addr = packet.yiaddr;
					LOG(LOG_INFO, "Lease of %s obtained, lease time %ld",
					inet_ntoa(temp_addr), lease);
					start = now;
					timeout_dhcpc = t1 + start;
					requested_ip = packet.yiaddr;
#ifdef DHCPC_ELAPSED_TIME_SUPPORT
					dhcpc_elapsed_time = 0;
#endif
#ifdef CONFIG_AUTO_DHCP_CHECK
					relay_addr_auto=packet.giaddr;
#if 0
					if((0 == get_server_mac_addr())&&(0 == strcmp(client_config.interface, "br0")))
					{   
						/* save server addr and client addr to file */
						save_auto_dhcpc_addr();					
					}
#endif
					//DUT first get IP or server changes
					if((!(state == RENEWING || state == REBINDING)) || (pre_server_addr!=server_addr))
					{
						if(strcmp(client_config.interface, "br0")==0)
						{	
							save_auto_dhcpc_addr();
							auto_dhcp_pid=read_pid("/var/run/auto_dhcp.pid");
							if(auto_dhcp_pid>0)
							{
								//printf("\nsend signal SIGUSR1 to auto dhcp daemon!\n");
								kill(auto_dhcp_pid, SIGUSR1);
							}
						}						
					}
					pre_server_addr=server_addr;
#endif

					/*ql: 20080926 START: get request option value*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
					updateReqDhcpcOptionValue(&packet);
#endif
					/*ql: 20080926 END*/

					/* Dick Tam, 2003-05-16, Microsoft AUTO IP procedure */
					// store router IP for Microsoft AUTO IP Procedure
					if(arp_on_bound)
					{
						LOG(LOG_INFO,"check gateway arp at lease 3 times\n");
						arp_times = 3;
						timeout_dhcpc_bak = timeout_dhcpc;
						timeout_dhcpc = now;
						if(router_align != 0){
							LOG(LOG_INFO, "delete old router arp entry");
							va_cmd("/bin/arp",2,0,"-d",inet_ntoa(*((struct in_addr *)&router_align)));
						}
					}
					if((temp = get_option(&packet, DHCP_ROUTER)))
					{
						unsigned int routerAddr;

						memcpy(&routerAddr, temp, 4);
						if (routerAddr != router_align)
							ip_changed = 1;
						memcpy(&router_align, temp, 4);
					}
					#ifdef CONFIG_TR069_DNS_ISOLATION
					 set_tr069_dns_isolation(1);
					#endif

					if(getInAddr(client_config.interface,IP_ADDR,(void *)&inAddr)==0)
						ip_changed = 1;
					else{
						//printf("0x%x\n",inAddr.s_addr);
						if(inAddr.s_addr!=requested_ip){//Iulian Wu, didn't run script when get the same IP, to avoid of the throughput draw.
							ip_changed = 1;
						}
					}
					if(ip_changed
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
						|| dns_changed
#endif
					)
					{
						run_script(&packet,((state == RENEWING || state == REBINDING) ? "renew" : "bound"));
					}
//if dns change && ip do not change,dns config should renew
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
					if(dns_changed && (!ip_changed))
					{
						restart_dnsrelay();
					}
#endif

					//wpeng 20120411 added for parsestatic router option
#ifdef CONFIG_USER_DHCP_OPT_33_249
					if (client_config.interface)
					{
						snprintf(static_route_file, sizeof(static_route_file), "%s%s", STATIC_ROUTE_FILE, client_config.interface);
						unlink(static_route_file);
					}

					if((temp = get_option(&packet, DHCP_CLASSLESS_RT))){
						//useClaesslessRT = 1;
						dealStaticRTOption(temp, DHCP_CLASSLESS_RT);
					}
					if((temp = get_option(&packet, DHCP_OPT_249))){
						//useClaesslessRT = 1;
						dealStaticRTOption(temp, DHCP_OPT_249);
					}
					//if(useClaesslessRT != 1 && (temp = get_option(&packet, DHCP_STATIC_RT)))//ignore ooption 33 when has option 121/249
					if((temp = get_option(&packet, DHCP_STATIC_RT)))
						dealStaticRTOption(temp, DHCP_STATIC_RT);
#endif
					//end of add
#ifdef _PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_RegisterSuccessNumberEnable_
					if(wan_app & X_CT_SRV_TR069){
						//printf("dhcpc success!! but renew&rebind do not count, state=%s\n", state_str[state]);
						if(state == REQUESTING)
							addDHCPSuccessNumber();
					}
					
#endif
					state = BOUND;
					change_mode(LISTEN_NONE);
#ifdef CONFIG_USER_DHCPCLIENT_CHECK_IP_CONFLICT
					mib_get_s(MIB_DHCPC_IP_CONFLICT, (void *)&ip_conflict, sizeof(ip_conflict));
					if (ip_conflict == 1)
					{
					    /*
					     * RFC5227:MUST then announce that it is commencing to use this address 
					     * by broadcasting ANNOUNCE_NUM ARP Announcements, spaced ANNOUNCE_INTERVAL seconds apart
					     */
					    int announce_sent = 0;
					    while(announce_sent < ARP_ANNOUNCEMENT_NUM)
					    {
					        do_one_arp_announce(packet.yiaddr, client_config.arp, client_config.interface);
					        announce_sent++;
					        sleep(ARP_ANNOUNCE_INTERVAL);
					    }
					}
#endif
					if(client_config.is_ipoe_diag)
					{
						write_ipoe_diag_result(IPOE_DIAG_RESULT_OK, requested_ip, router_align);
						exit_client(0);
					}
					if (client_config.quit_after_lease)
						exit_client(0);
					if (!client_config.foreground)
						background();
					//add by ramen for the dns bind pvc
					//DnsBindPvcRoute(1);//delete the route about dns bind pvc
					//DnsBindPvcRoute(0);//add the route about dns bind pvc
#ifdef CONFIG_TR069_DNS_ISOLATION
					set_tr069_dns_isolation(0);
#endif
#ifdef CONFIG_USER_RTK_VOIP
					if(ip_changed) {
						restart_voip_callmanager();
					}
#endif
#if defined(CONFIG_USER_CWMP_TR069)
					if (ip_changed) {
						if ((cwmp_msgid = msgget((key_t)1234, 0)) > 0) {
							cwmpmsg.msg_type = MSG_ACTIVE_NOTIFY;
							msgsnd(cwmp_msgid, (void *)&cwmpmsg, MSG_SIZE, 0);
						}
					}
#endif
				} else if (*message == DHCPNAK) {
					//check if DHCP Server Identifier is matched
					if ((temp = get_option(&packet, DHCP_SERVER_ID))) {
						unsigned int serverID;

						memcpy(&serverID, temp, 4);
						if (serverID != server_addr) {
							DEBUG(LOG_ERR, "Ignoring DHCP Server Identifier %u (our ID is %x)",
								serverID, server_addr);
							continue;
						}
					}
					if(packet.giaddr != relay_addr){
						DEBUG(LOG_ERR, "Ignoring DHCP Relay Agent %x (our Relay Agent is %x)",
								packet.giaddr, relay_addr);
						continue;
					}
#ifdef CONFIG_USER_DHCP_OPT_33_249
					if (client_config.interface)
					{
						snprintf(static_route_file, sizeof(static_route_file), "%s%s", STATIC_ROUTE_FILE, client_config.interface);
						unlink(static_route_file);
					}
#endif
					/* return to init state */
					LOG(LOG_INFO, "Received DHCP NAK");
					run_script(&packet, "nak");
					if (state != REQUESTING)
						run_script(NULL, "deconfig");
					state = INIT_SELECTING;
					timeout_dhcpc = now;
					requested_ip = 0;
					packet_num = 0;
					change_mode(LISTEN_RAW);
					if(client_config.is_ipoe_diag)
						write_ipoe_diag_result(IPOE_DIAG_RESULT_ServerDeny, 0, 0);
					sleep(3); /* avoid excessive network traffic */
				}
				break;
			/* case BOUND, RELEASED: - ignore all packets */
			}
		} else if (retval > 0 && FD_ISSET(signal_pipe[0], &rfds)) {
			if (read(signal_pipe[0], &sig, sizeof(sig)) < 0) {
				DEBUG(LOG_ERR, "Could not read signal: %s",
					strerror(errno));
				continue; /* probably just EINTR */
			}
			switch (sig) {
			case SIGUSR1:
				printf("[udhcpc]received SIGUSR1");
				snprintf(cmd, sizeof(cmd), "touch %s_%s", DHCPC_RENEWFILE, client_config.interface);
				system(cmd);
				timeout_signal_usr1 = now;
				perform_renew();
				break;
			case SIGUSR2:
				printf("[udhcpc]received SIGUSR2");
				snprintf(cmd, sizeof(cmd), "rm %s_%s", DHCPC_RENEWFILE, client_config.interface);
				system(cmd);
				perform_release();
				break;
			case SIGCHLD:
				printf("[udhcpc]received SIGCHLD");
				while(waitpid(-1, &status, WNOHANG) > 0);  
				break;				
			case SIGTERM:
			case SIGHUP:
				perform_release();
				snprintf(cmd, sizeof(cmd), "rm %s_%s", DHCPC_RENEWFILE, client_config.interface);
				system(cmd);
				LOG(LOG_INFO, "Received %s", (SIGTERM==sig)?"SIGTERM":"SIGHUP");
				printf("[udhcpc]received SIGTERM or SIGHUP");
				exit_client(0);
			}
		} else if (retval == -1 && errno == EINTR) {
			/* a signal was caught */
		} else {
			/* An error occured */
			DEBUG(LOG_ERR, "Error on select");
		}
		update_state(state);
	}
	return 0;
}

