/* dhcpd.c
 *
 * udhcp Server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *			Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
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

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>

#include "debug.h"
#include "dhcpd.h"
#include "arpping.h"
#include "socket.h"
#include "options.h"
#include "files.h"
#include "leases.h"
#include "packet.h"
#include "serverpacket.h"
#include "pidfile.h"
#include <rtk/mib.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include <rtk/sysconfig.h>
//#ifdef DHCPD_MULTI_THREAD_SUPPORT
#include <pthread.h>
//#endif
#ifdef CONFIG_LIB_CAP
#include <sys/capability.h>
#endif

#if 0
#ifdef CONFIG_CTC_E8_CLIENT_LIMIT
#include <signal.h>
#include <sys/time.h>
#endif
#endif

/* globals */
struct dhcpOfferedAddr *leases;
struct server_config_t server_config;
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
struct server_config_t* p_serverpool_config;
#endif
#ifdef CONFIG_CMCC
char province[32]="";
#endif

//ql
char g_dhcp_mode=0; // Mason Yu
char g_server_ip[16]="1.1.1.1";

int dhcp_debug_on=0;
static char dhcpd_debug_prefix[]="[udhcpd]";
char *dhcp_debug_prefix=dhcpd_debug_prefix;
char dhcpd_debug_interface[16]="[br0]";
char *dhcp_debug_interface=dhcpd_debug_interface;

#ifdef RTK_DHCP_REPLY_NAK
const char DHCPSERVERREPLYNAK[] = "/var/udhcpd/udhcpd_reply_nak";
#endif

#ifdef DHCPD_MULTI_THREAD_SUPPORT
static pthread_mutex_t g_threadMutex;
static pthread_mutex_t g_hostsMutex;
static pthread_mutex_t g_arppingMutex;
static pthread_mutex_t g_socketMutex;
#endif

#ifdef DHCPD_MULTI_THREAD_SUPPORT
static void init_pthread_mutex(void)
{
	pthread_mutex_init(&g_threadMutex, NULL);
	pthread_mutex_init(&g_arppingMutex, NULL);
	pthread_mutex_init(&g_hostsMutex, NULL);
	pthread_mutex_init(&g_socketMutex, NULL);
}

void thread_mutex_lock(void)
{
	pthread_mutex_lock(&g_threadMutex);
}

void thread_mutex_unlock(void)
{
	pthread_mutex_unlock(&g_threadMutex);
}

void host_mutex_lock(void)
{
	pthread_mutex_lock(&g_hostsMutex);
}

void host_mutex_unlock(void)
{
	pthread_mutex_unlock(&g_hostsMutex);
}

void sock_mutex_lock(void)
{
	pthread_mutex_lock(&g_socketMutex);
}

void sock_mutex_unlock(void)
{
	pthread_mutex_unlock(&g_socketMutex);
}

void arpping_mutex_lock(void)
{
	pthread_mutex_lock(&g_arppingMutex);
}

void arpping_mutex_unlock(void)
{
	pthread_mutex_unlock(&g_arppingMutex);
}

static void start_arpping_thread(void)
{
	pthread_t ptID;

	pthread_create(&ptID, NULL, arpping_thread, NULL);
	pthread_detach(ptID);
}

#ifndef CONFIG_TELMEX_DEV
static void start_keepalive_thread(void)
{
	pthread_t ptID;

	pthread_create(&ptID, NULL, keepalive_thread, NULL);
	pthread_detach(ptID);
}
#endif
#endif

/* Exit and cleanup */
//static void exit_server(int retval)   // try123
void exit_server(int retval)
{
	pidfile_delete(server_config.pidfile);
	CLOSE_LOG();
	clear_all_lease();
// Kaohj --- TR111 Part 1
#ifdef _CWMP_TR111_
	clear_all_deviceId();
#endif
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	{
		struct server_config_t* servingpool_p = p_serverpool_config;
		while(servingpool_p != NULL)
		{
			struct server_config_t* p_servingpool_tmp = servingpool_p;
#ifdef IP_BASED_CLIENT_TYPE
			struct client_category_t *p;
			while (p_servingpool_tmp->clientRange)
			{
				p = p_servingpool_tmp->clientRange;

				p_servingpool_tmp->clientRange = p_servingpool_tmp->clientRange->next;
				free(p);
			}
#endif
			servingpool_p = servingpool_p->next;
			free(p_servingpool_tmp);
		}
	}
#else
#ifdef IP_BASED_CLIENT_TYPE
		//ql, clear clientrange
		{
			struct client_category_t *p;
			while (server_config.clientRange)
			{
				p = server_config.clientRange;
	
				server_config.clientRange = server_config.clientRange->next;
				free(p);
			}
		}
#endif //IP_BASED_CLIENT_TYPE
#endif
    /* remove_force_portal(); */
	exit(retval);
}
void print_trace (int sig)
{
    void *array[100];
    size_t size;
    char **strings;
    size_t i;
	FILE *fp=NULL;
	
  	fp = fopen("/tmp/udhcpd.trace", "w");
	if (fp == NULL){
		fprintf (stderr,"udhcpd open trace file fail!\n");
	}
    printf ("udhcpd recv signal %d\n", sig);
	if(fp)
		fprintf(fp,"udhcpd recv signal %d\n", sig);
    size = backtrace (array, 100);
    strings = backtrace_symbols (array, size);
      
    if (strings == NULL) {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }
    printf ("Obtained %zd stack frames.\n", size);
	if(fp)
		fprintf(fp,"Obtained %zd stack frames.\n", size);
  
    for (i = 0; i < size; i++){
        printf ("%s\n", strings[i]);
		if(fp)
			fprintf(fp,"%s\n", strings[i]);
    }
  
    free(strings);
	if(fp) fclose(fp);
}

static void unlink_pid_file(void){
	int ret = 0;
	int tries = 0;
try:
	ret=unlink(server_config.pidfile);
	if(ret==-1){
		if(errno==EBUSY){
			tries++;
			printf("Somebody is using %s,try %d times!\n",server_config.pidfile,tries);
			sleep(1);//1s		
			if(tries<100)
				goto try;			
		}
	}
}
static void sig_process_quit(int sig)
{
	//printf("process sig %d\n",sig);
	print_trace(sig);
	unlink_pid_file();
	signal(sig, SIG_DFL);
	raise(sig);
}

static void init_signals(void)
{
	struct sigaction sa;
	sa.sa_flags = 0;

	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGABRT);
	sigaddset(&sa.sa_mask, SIGSEGV);
	sigaddset(&sa.sa_mask, SIGBUS);
	sigaddset(&sa.sa_mask, SIGFPE);
	sigaddset(&sa.sa_mask, SIGQUIT);
	sigaddset(&sa.sa_mask, SIGINT);

	sa.sa_handler = sig_process_quit;
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	//sa.sa_handler = sig_process_quit;
	sigaction(SIGINT, &sa, NULL);
}
static void exit_hook(void){
    unlink_pid_file();
}



void usage(char *s)
{
   printf("usage: %s [-h] {-S|-R|-B} {ServerIP|configFile}\n"
	  "\t-h:print this help\n"
	  "\t-S:run in the Server function\n"
	  "\t-R:run in the Relay fuction\n"
	  "\t-B:run in the Server and Relay function\n"
	  "\tServerIP:It is an IP Address. The CPE will relay packet to this IP\n"
	  "\tAddress.\n"
	  "\tconfigFile:It is config File for DHCP Server\n"
	  ,s);
   exit(1);
}

static void sig_handler_do_nothing(int signum)
{
	DEBUG(LOG_INFO,	"udhcpd Get signal: %d\n", signum);
	LOG(LOG_INFO, "udhcpd Get signal: %d\n", signum);

	return;
}


#ifdef COMBINED_BINARY
int udhcpd_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{

/* Because we combine the dhcpd and dhcprelay, it will cause signal register too late.
 * If we register signal not yet, that time other send signal to us. With kernel behavior,
 * It will make dhcpd exit. So we need register signal first to avoid it. */
#ifdef CONFIG_LIB_CAP
	set_process_capability();
#endif 

#ifdef COMBINE_DHCPD_DHCRELAY
	struct sigaction sigact;

	printf("Start udhcpd server!\n");
	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_flags = 0;
	sigact.sa_handler = sig_handler_do_nothing;	
	sigaction(SIGUSR1, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGALRM, &sigact, NULL);
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGUSR2, &sigact, NULL);
	sigaction(38, &sigact, NULL);
#endif
	//fd_set rfds;
	//struct timeval tv;
	//int server_socket = -1;
	//int bytes, retval;
	//struct dhcpMessage packet;
	//unsigned char *state;
	//unsigned char *server_id, *requested;
	//u_int32_t server_id_align, requested_align;
	//unsigned long timeout_end;
	struct option_set *option;
	//struct dhcpOfferedAddr *lease;
	int pid_fd;
	//int max_sock;
	//int sig;
#ifndef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	unsigned long num_ips;
#endif
	struct itimerval stTimerInter;
	//unsigned char *classVendor;
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	struct server_config_t* p_servingpool_tmp;
	//DHCPS_SERVING_POOL_T *pDHCPSPEntry, DHCPSPEntry;
	//int entry_num, i;
	//char isDroped = 0;
#endif
	int c;
	sigset_t  tmpset;
	extern char *optarg;
	extern int optind;

	OPEN_LOG("udhcpd");
	LOG(LOG_INFO, "udhcp server/relay (v%s) started", VERSION);


#ifdef RTK_DHCP_REPLY_NAK
	unlink(DHCPSERVERREPLYNAK);
#endif
	/* block all signal to avoid process exit unexpectedly */
	sigfillset(&tmpset);
	sigprocmask(SIG_BLOCK, &tmpset, NULL);

	memset(&server_config, 0, sizeof(struct server_config_t));

#if 0
	// Because we do not input config file for Relay, so we should set the server_config.pidfile with "/var/run/udhcpd.pid" for Relay.
	// After read_config() on Server , the server_config.pidfile will be set again.
	server_config.pidfile = strdup("/var/run/udhcpd.pid");
	pid_fd = pidfile_acquire(server_config.pidfile);
	atexit(exit_hook);
	init_signals();
	pidfile_write_release(pid_fd);
#endif
#ifdef CONFIG_CMCC
	mib_get_s(MIB_HW_PROVINCE_NAME, province, sizeof(province));
#endif

	if (argc>2) {
    		while ((c = getopt(argc, argv,"B:S:R:e")) !=EOF) {
    			//printf("optind: %d\n", optind);
      			switch (c) {
      				case 'S':
					g_dhcp_mode = LAN_SERVER;
					printf("Run Server function only\n");
					if ( optarg != NULL ) {
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
						p_serverpool_config=NULL;
						//pDHCPSPEntry = &DHCPSPEntry;
#endif
						read_config(optarg);
					} else
						usage(argv[0]);
					break;
     				case 'R':
					g_dhcp_mode = LAN_RELAY;
					printf("Run Relay function only\n");
					if ( optarg != NULL ) {
						if (strlen(optarg) < sizeof(g_server_ip)) {
							strncpy(g_server_ip, optarg, strlen(optarg));
							g_server_ip[strlen(optarg)] = '\0';
						} else {
							strncpy(g_server_ip, optarg, sizeof(g_server_ip));
							g_server_ip[sizeof(g_server_ip) - 1] = '\0';
						}
					} else
						usage(argv[0]);
					server_config.pidfile = strdup("/var/run/udhcpd.pid");
					break;
				case 'B':
					g_dhcp_mode = LAN_SERVER_AND_RELAY;
					printf("Run Server and Relay functions.\n");
					if ( optarg != NULL ) {
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
						p_serverpool_config=NULL;
						//pDHCPSPEntry = &DHCPSPEntry;
#endif
						read_config(optarg);
					} else
						usage(argv[0]);
					break;
				case 'e':
					dhcp_debug_on = 1;
#if defined(_DHCP_DEBUG_ON_OFF_)
					dhcp_debug_onoff_by_file(DHCPDEBUG_FLAG_WRITE);
#endif
					break;
      				case 'h':
      				default:
					usage(argv[0]);
      			}
      		}
      	}
  	else
    		usage(argv[0]);

    	if (argc != optind) {
    		usage(argv[0]);
    	}

	pid_fd = pidfile_acquire(server_config.pidfile);
	atexit(exit_hook);
	init_signals();
	pidfile_write_release(pid_fd);
	if ( g_dhcp_mode == LAN_SERVER || g_dhcp_mode == LAN_SERVER_AND_RELAY ) {
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
		p_servingpool_tmp=p_serverpool_config;
		while(p_servingpool_tmp!=NULL){
			unsigned int serverip;
			if ((option = find_option(p_servingpool_tmp->options, DHCP_LEASE_TIME))) {
				memcpy(&(p_servingpool_tmp->lease), option->data + 2, 4);
				p_servingpool_tmp->lease = ntohl(p_servingpool_tmp->lease);
			}
			else p_servingpool_tmp->lease = LEASE_TIME;

		#if 1
			p_servingpool_tmp->max_leases = 254; /*every pool shares the same lease num/structure*/
		#else
			num_ips = ntohl(p_servingpool_tmp->end) - ntohl(p_servingpool_tmp->start) + 1;
			if (p_servingpool_tmp->max_leases > num_ips) {
				p_servingpool_tmp->max_leases = num_ips;
			}
		#endif
			//LOG(LOG_INFO, "p_servingpool_tmp->max_leases=%d", p_servingpool_tmp->max_leases);
#if 0
			if (read_interface(p_servingpool_tmp->interface, &p_servingpool_tmp->ifindex,
				   &serverip, p_servingpool_tmp->arp) < 0)
				exit_server(1);
#else
			while (read_interface(p_servingpool_tmp->interface, &p_servingpool_tmp->ifindex,
				   &serverip, p_servingpool_tmp->arp) < 0) {
				printf("DHCPv4 Server: Interface %s is not ready\n", p_servingpool_tmp->interface);
				sleep(1);
			}
#endif
			//ql add
			if (0 == p_servingpool_tmp->server) {
				p_servingpool_tmp->server = serverip;
			}

			p_servingpool_tmp=p_servingpool_tmp->next;
		}
		p_serverpool_config->pidfile = server_config.pidfile;
		memcpy(&server_config,p_serverpool_config,sizeof(struct server_config_t));

		/*every pool shares the same lease num/structure*/
		LOG(LOG_INFO, "create leases server_config.max_leases=%lu, size=%lu", server_config.max_leases, sizeof(struct dhcpOfferedAddr) * server_config.max_leases );
		leases = xmalloc(sizeof(struct dhcpOfferedAddr) * server_config.max_leases);
		memset(leases, 0, sizeof(struct dhcpOfferedAddr) * server_config.max_leases);
		read_leases(server_config.lease_file);
#else

		if ((option = find_option(server_config.options, DHCP_LEASE_TIME))) {
			memcpy(&server_config.lease, option->data + 2, 4);
			server_config.lease = ntohl(server_config.lease);
		}
		else server_config.lease = LEASE_TIME;

		/* Sanity check */
		// Kaohj -- num_ips should include both start and end ips
		//num_ips = ntohl(server_config.end) - ntohl(server_config.start);
		num_ips = ntohl(server_config.end) - ntohl(server_config.start) + 1;
		if (server_config.max_leases > num_ips) {
	// david, disable message. 2003-5-21
	//		LOG(LOG_ERR, "max_leases value (%lu) not sane, setting to %lu instead",
	//			server_config.max_leases, num_ips);
			server_config.max_leases = num_ips;
		}

		leases = xmalloc(sizeof(struct dhcpOfferedAddr) * server_config.max_leases);
		memset(leases, 0, sizeof(struct dhcpOfferedAddr) * server_config.max_leases);
		read_leases(server_config.lease_file);
	{
		unsigned int serverip;
#if 0
		if (read_interface(server_config.interface, &server_config.ifindex,
				   &serverip, server_config.arp) < 0)
			exit_server(1);
#else
		while (read_interface(server_config.interface, &server_config.ifindex,
			&serverip, server_config.arp) < 0) {
			printf("DHCPv4 Server: Interface %s is not ready\n", server_config.interface);
			sleep(1);
		}
#endif
		if (0 == server_config.server)
			server_config.server = serverip;
	}
#endif //_PRMT_X_TELEFONICA_ES_DHCPOPTION_
	} // if ( g_dhcp_mode == LAN_SERVER || g_dhcp_mode == LAN_SERVER_AND_RELAY ) {

#if 0
#ifndef DEBUGGING
	pid_fd = pidfile_acquire(server_config.pidfile); /* hold lock during fork. */
	if (daemon(0, 0) == -1) {
		perror("fork");
		exit_server(1);
	}
	pidfile_write_release(pid_fd);
#endif
#endif


		// If created by tty, stdin, stdout and stderr may be associated to tty.
		freopen("/dev/null", "r", stdin);
		freopen("/dev/console", "w", stdout);
		freopen("/dev/console", "w", stderr);

		stTimerInter.it_value.tv_sec = DHCP_LEASE_TIMER_INTERVAL;
		stTimerInter.it_value.tv_usec = 0;
		stTimerInter.it_interval = stTimerInter.it_value;
		setitimer(ITIMER_REAL, &stTimerInter, NULL);
		
		#if 0
		if ( g_dhcp_mode == LAN_SERVER || g_dhcp_mode == LAN_SERVER_AND_RELAY ) {
			timeout_end = get_uptime() + server_config.auto_time;
		}
		#endif

//jim luo
#if 0
#ifdef CONFIG_CTC_E8_CLIENT_LIMIT
	//memset(accepted_PC_Clients, 0, sizeof(accepted_PC_Clients));
	//memset(accepted_Camera_Clients, 0, sizeof(accepted_Camera_Clients));
	printf("install signal SIGALRM hander read_proc_handler\n");
	{
		pid_t pid;
		pid=fork();
		if(pid==-1)
		{
			printf("fork error\n");
			return;
		}
		if(pid==0)
		{	//child process
			//no need to
			/*
			signal(SIGALRM, read_proc_handler);
			read_proc_interval;
			read_proc_interval.it_value.tv_sec=1;
			read_proc_interval.it_value.tv_usec=0;
			read_proc_interval.it_interval=read_proc_interval.it_value;
			setitimer(ITIMER_REAL, &read_proc_interval, NULL );
			while(1);*/
			while(1)
			{
				sleep(1);
				read_proc_handler(0);
			}
		}
		if(pid)
		{
			printf("child process pid=%d created in dhcpd\n ", pid);
		}
	}
#endif
#endif
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	init_pthread_mutex();
	start_arpping_thread();
#ifndef CONFIG_TELMEX_DEV	
	start_keepalive_thread();
#endif
#endif

#ifdef CONFIG_USER_MONITORD
	update_monitor_list_file("udhcpd", 1);
#endif

	// Mason Yu. signal123
	listen_from_relay();

	return 0;
}
int mac_autobinding_record_mac(struct dhcpMessage *packet){	
	MIB_CE_CMCC_DHCPCLIENT_MAC_T entry;
	int i = 0;
	int total = mib_chain_total(MIB_CMCC_DHCPCLIENT_MAC_TBL);
	printf("%s %d received DHCP from %02x:%02x:%02x:%02x:%02x:%02x\n",__FUNCTION__,__LINE__,
				packet->chaddr[0],packet->chaddr[1],packet->chaddr[2],packet->chaddr[3],packet->chaddr[4],packet->chaddr[5]);
	for(i=0;i<total;i++){
		if(mib_chain_get(MIB_CMCC_DHCPCLIENT_MAC_TBL,i,&entry)){
			if(!memcmp(entry.Mac,packet->chaddr,sizeof(entry.Mac))){
				printf("%s %d has existed!\n",__FUNCTION__,__LINE__);
				return 1;
			}
		}
	}
	memcpy(entry.Mac,packet->chaddr,sizeof(entry.Mac));
	mib_chain_add(MIB_CMCC_DHCPCLIENT_MAC_TBL,&entry);
	mac_autobinding_setup_mac_accept_rule(entry.Mac);

	return 0;
}
// Mason Yu. try123
int server_func(struct dhcpMessage *packet, struct server_config_t *server_config_p)
{
	unsigned char *state;
	unsigned char *server_id, *requested;
	u_int32_t server_id_align, requested_align;	
	struct dhcpOfferedAddr *lease;
#ifdef IP_BASED_CLIENT_TYPE
	unsigned char *classVendor;
#endif
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	//struct server_config_t* p_servingpool_tmp;
	//DHCPS_SERVING_POOL_T *pDHCPSPEntry, DHCPSPEntry;
	//int entry_num, i;
	//char isDroped = 0;
#endif
/*ping_zhang:20090316 START:Fix the DHCP_VENDOR string bugs*/	
#ifdef IP_BASED_CLIENT_TYPE
	unsigned char len = 0;
	unsigned char classVendorStr[256] = {0};
#endif
/*ping_zhang:20090316 END*/

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	//pDHCPSPEntry = &DHCPSPEntry;
#endif

	if ((state = get_option(packet, DHCP_MESSAGE_TYPE)) == NULL) {
		DEBUG(LOG_ERR, "couldn't get option from packet, ignoring");
		//continue;
		return 0;
	}

		/* ADDME: look for a static lease */
		lease = find_lease_by_chaddr(packet->chaddr);
		//jim: added by star_zhang
		u_int32_t ip_addr;

		do{
			unsigned char mib_auto_mac_binding_iptv_bridge_wan_mode=0;
			if(mib_get_s(MIB_AUTO_MAC_BINDING_IPTV_BRIDGE_WAN,&mib_auto_mac_binding_iptv_bridge_wan_mode, sizeof(mib_auto_mac_binding_iptv_bridge_wan_mode))&&mib_auto_mac_binding_iptv_bridge_wan_mode==2){
				//for guizhou
				mac_autobinding_record_mac(packet);
			}
		}while(0);

		LOG(LOG_DEBUG, "DHCPD message type=%c",state[0]);
		switch (state[0]) {
		case DHCPDISCOVER:
			DEBUG(LOG_INFO,"received DISCOVER from %x-%x-%x-%x-%x-%x, xid=%#x", packet->chaddr[0], packet->chaddr[1], packet->chaddr[2],
					packet->chaddr[3], packet->chaddr[4], packet->chaddr[5], ntohl(packet->xid));
			//tylo, wait for 1 sec. to force PC get IP from WAN prior to CPE if there is DHCP server in WAN side
			// Kaohj -- not for general release (should be customized as necessary)
			//sleep(1);

#ifdef RTK_DHCP_REPLY_NAK
			if(isFileExist(DHCPSERVERREPLYNAK))
				break;
#endif
// Magician: TR-069 DHCP Serving Pool.
#if 0 // Mason Yu. try123
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
			isDroped = 0;
			if(classVendor=get_option(packet, DHCP_VENDOR))
			{
				len=*(unsigned char*)(classVendor-OPT_LEN);
				memcpy(classVendorStr,classVendor,len);
				classVendorStr[len]=0;

				entry_num = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);
				for( i = 0; i < entry_num; i++ )
				{
					if( !mib_chain_get(MIB_DHCPS_SERVING_POOL_TBL, i, (void*)pDHCPSPEntry) )
						continue;
						//return 0;

					if(strstr(classVendorStr, pDHCPSPEntry->vendorclass))
					{
						isDroped = !pDHCPSPEntry->localserved;
						break;
					}
				}
			}

			if(!isDroped)
#endif
#endif  // try123
// The end of TR-069 DHCP Serving Pool.
			if (sendOffer(packet, server_config_p) < 0) 
			{
				LOG(LOG_ERR, "send OFFER failed");
			}
			break;

 		case DHCPREQUEST:
// Magician: TR-069 DHCP Serving Pool.
#if 0 // Mason Yu. try123
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
			isDroped = 0;

			if(classVendor=get_option(packet, DHCP_VENDOR))
			{
				len=*(unsigned char*)(classVendor-OPT_LEN);
				memcpy(classVendorStr,classVendor,len);
				classVendorStr[len]=0;

				entry_num = mib_chain_total(MIB_DHCPS_SERVING_POOL_TBL);
				for( i = 0; i < entry_num; i++ )
				{
					if( !mib_chain_get(MIB_DHCPS_SERVING_POOL_TBL, i, (void*)pDHCPSPEntry) )
						continue;
						//return 0;

					if(strstr(classVendorStr, pDHCPSPEntry->vendorclass))
					{
						isDroped = !pDHCPSPEntry->localserved;
						break;
					}
				}
			}

			if(isDroped)
				//continue;
				return 0;
#endif
#endif
			// The end of TR-069 DHCP Serving Pool.
			DEBUG(LOG_INFO, "received REQUEST from %x-%x-%x-%x-%x-%x xid=%#x", packet->chaddr[0], packet->chaddr[1], packet->chaddr[2],
					packet->chaddr[3], packet->chaddr[4], packet->chaddr[5], ntohl(packet->xid));
			requested = get_option(packet, DHCP_REQUESTED_IP);
			server_id = get_option(packet, DHCP_SERVER_ID);

#ifdef RTK_DHCP_REPLY_NAK
			if(isFileExist(DHCPSERVERREPLYNAK))
			{
				sendNAK(packet, server_config_p);
				break;
			}
#endif

#ifdef DHCP_OPTION_43_ENABLE
			OPT43_VendorSpecificInformation(get_option(packet, DHCP_VENDOR_SPECIFIC_INFO));
#endif
#ifdef DHCP_OPTION_82_ENABLE
			OPT82_RelayAgentInformation(get_option(packet, DHCP_RELAY_AGENT_INFO));
#endif

#ifdef IP_BASED_CLIENT_TYPE
//			enum DeviceType devicetype;
			struct client_category_t *deviceCategory;
			struct dhcp_ctc_client_info stClientInfo;

#ifdef _PRMT_X_CT_COM_DHCP_
			unsigned char check_opt_60;

			mib_get_s(CWMP_CT_DHCPS_CHECK_OPT_60,&check_opt_60, sizeof(check_opt_60));
			if(!check_opt_60 || !(classVendor=get_option(packet, DHCP_VENDOR)))
#else
			if(!(classVendor=get_option(packet, DHCP_VENDOR)))
#endif
			{
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
				struct server_config_t *pDhcp;
				//default : PC clients....
//				devicetype = CTC_Computer;
				deviceCategory=NULL;
				// Magicia: Every device other than default type is set to Computer.
				for (pDhcp=&server_config; pDhcp; pDhcp=pDhcp->next)
				{
					if(pDhcp->vendorclass && !strcmp(pDhcp->vendorclass, "Computer"))
					{
						deviceCategory = pDhcp->clientRange;
						break;
					}
				}
#endif
			}
			else
			{
/*ping_zhang:20090316 START:Fix the DHCP_VENDOR string bugs*/
				len=*(unsigned char*)(classVendor-OPT_LEN);
				if (len < sizeof(classVendorStr)) {
					memcpy(classVendorStr, classVendor, len);
					classVendorStr[len]=0;
				} else {
					memcpy(classVendorStr, classVendor, sizeof(classVendorStr));
					classVendorStr[sizeof(classVendorStr) - 1] = 0;
				}
/*ping_zhang:20090316 END*/
				memset(&stClientInfo, 0, sizeof(struct dhcp_ctc_client_info));
/*ping_zhang:20090316 START:Fix the DHCP_VENDOR string bugs*/
				parse_CTC_Vendor_Class(classVendor, &stClientInfo);
//				parse_CTC_Vendor_Class(classVendorStr, &stClientInfo);
/*ping_zhang:20090316 END*/
//				devicetype = (enum DeviceType)(stClientInfo.category);
				deviceCategory = stClientInfo.iCategory;
			}
#endif

			if (requested) memcpy(&requested_align, requested, 4);
			if (server_id) memcpy(&server_id_align, server_id, 4);

			if (lease) { /*ADDME: or static lease */
				if (server_id) {
					/* SELECTING State */
					DEBUG(LOG_INFO, "[%s:%d] server_id = %08x", __FUNCTION__,__LINE__, ntohl(server_id_align));
					if (server_id_align == server_config_p->server && requested &&
					    requested_align == lease->yiaddr) {
						sendACK(packet, lease->yiaddr, server_config_p);
					}
				} else {
					ip_addr = find_IP_by_Mac(packet->chaddr);
					if (requested) {
						/* INIT-REBOOT State */
						if ((lease->yiaddr == requested_align)
							// Mason Yu. Req
							&& ( (ip_addr ==0 && !(find_Mac_by_IP(packet->ciaddr)) ) || ((ip_addr != 0) && (ip_addr == lease->yiaddr)))
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
							&& ((ip_addr != 0) || ((ip_addr == 0) && (isReservedIPAddress(lease->yiaddr, server_config_p)==0)))
#endif //SUPPORT_DHCP_RESERVED_IPADDR
							&&  !check_ip(requested_align, packet->chaddr, server_config_p)
				#ifdef IP_BASED_CLIENT_TYPE
							&& ((ip_addr != 0) || (!check_type(lease->yiaddr, deviceCategory, server_config_p)))
/*star:20080927 START add for dhcp ip range check*/
				#else
							&& ((ip_addr !=0) || (ntohl(lease->yiaddr) >= ntohl(server_config.start) && ntohl(lease->yiaddr) <= ntohl(server_config.end)))
/*star:20080927 END*/
				#endif
						)
							sendACK(packet, lease->yiaddr, server_config_p);
						else {
							sendNAK(packet, server_config_p);
						}
					} else {
						/* RENEWING or REBINDING State */
						if ((lease->yiaddr == packet->ciaddr)
						// Mason Yu. Req
						&& ( (ip_addr ==0 && !(find_Mac_by_IP(packet->ciaddr)) ) || ((ip_addr != 0) && (ip_addr == lease->yiaddr)))
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
						&& ((ip_addr != 0) || ((ip_addr == 0) && (isReservedIPAddress(lease->yiaddr, server_config_p)==0)))
#endif //SUPPORT_DHCP_RESERVED_IPADDR
				#ifdef IP_BASED_CLIENT_TYPE
						&& ((ip_addr != 0) || (!check_type(lease->yiaddr, deviceCategory, server_config_p)))
/*star:20080927 START add for dhcp ip range check*/
				#else
						&& ((ip_addr !=0) || (ntohl(lease->yiaddr) >= ntohl(server_config.start) && ntohl(lease->yiaddr) <= ntohl(server_config.end)))
/*star:20080927 END*/
				#endif
						)
							sendACK(packet, lease->yiaddr, server_config_p);
						else {
							/* don't know what to do!!!! */
							sendNAK(packet, server_config_p);
						}
					}
				}

			/* what to do if we have no record of the client */
			}
			else if (server_id) {
				/* SELECTING State */
			}
			else if (requested) {
				DEBUG(LOG_INFO, "[%s:%d] requested = %x", __FUNCTION__,__LINE__, ntohl(requested_align));
				/* INIT-REBOOT State */
				ip_addr = find_IP_by_Mac(packet->chaddr);
#ifdef DHCPD_MULTI_THREAD_SUPPORT
				thread_mutex_lock();
#endif
				if ((lease = find_lease_by_yiaddr(requested_align))) {
					if (lease_expired(lease)) {
						/* probably best if we drop this lease */
						clear_one_lease(lease);
#ifdef DHCPD_MULTI_THREAD_SUPPORT
						/* bug fix: any other thread will get the same lease or get the same ip */
						add_lease(packet->chaddr, requested_align, server_config_p->lease);
						thread_mutex_unlock();
#endif

						// Mason Yu. Req
						sendACK(packet, requested_align, server_config_p);
					/* make some contention for this address */
					} else {
#ifdef DHCPD_MULTI_THREAD_SUPPORT
						thread_mutex_unlock();
#endif
						sendNAK(packet, server_config_p);
					}
				}
				//jim: added by star_zhang
				else if(ip_addr != 0){
					if(ip_addr!=requested_align)
					{
#ifdef DHCPD_MULTI_THREAD_SUPPORT
						thread_mutex_unlock();
#endif
						sendNAK(packet, server_config_p);
					}
					// Mason Yu. Req
					else {
#ifdef DHCPD_MULTI_THREAD_SUPPORT
						/* bug fix: any other thread will get the same lease or get the same ip */
						add_lease(packet->chaddr, requested_align, server_config_p->lease);
						thread_mutex_unlock();
#endif
						sendACK(packet, requested_align, server_config_p);
					}
				}
				else if (
#ifdef IP_BASED_CLIENT_TYPE
					(ip_addr == 0) && (check_type(requested_align, deviceCategory, server_config_p))
#else
					(ip_addr == 0) && (ntohl(requested_align) < ntohl(server_config_p->start) || ntohl(requested_align) > ntohl(server_config_p->end))
#endif
				) {
#ifdef DHCPD_MULTI_THREAD_SUPPORT
					thread_mutex_unlock();
#endif
					sendNAK(packet, server_config_p);
				} /* else remain silent */
				else if(find_Mac_by_IP(requested_align)) {
#ifdef DHCPD_MULTI_THREAD_SUPPORT
					thread_mutex_unlock();
#endif
					sendNAK(packet, server_config_p);
				}
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
				else if( isReservedIPAddress(requested_align, server_config_p) ) {
#ifdef DHCPD_MULTI_THREAD_SUPPORT
					thread_mutex_unlock();
#endif
					sendNAK(packet, server_config_p);
				}
#endif //SUPPORT_DHCP_RESERVED_IPADDR
				// Mason Yu. Req
				else if ( (ip_addr == 0) && (ntohl(requested_align) > ntohl(server_config_p->start) || ntohl(requested_align) < ntohl(server_config_p->end)) ) {
#ifdef DHCPD_MULTI_THREAD_SUPPORT
					lease = add_lease(packet->chaddr, requested_align, server_config_p->offer_time-1);
					thread_mutex_unlock();
#endif
					if (!check_ip(requested_align, packet->chaddr, server_config_p)) {
						sendACK(packet, requested_align, server_config_p);
					}
					else {
#ifdef DHCPD_MULTI_THREAD_SUPPORT
						thread_mutex_lock();
						clear_one_lease(lease);
						thread_mutex_unlock();
#endif
						sendNAK(packet, server_config_p);
					}
				}
				else{
#ifdef DHCPD_MULTI_THREAD_SUPPORT
					thread_mutex_unlock();
#endif
					//LOG(LOG_INFO, "sending NACK!\n");
					sendNAK(packet, server_config_p);
				}
			}
			else {
				 /* RENEWING or REBINDING State */
//jim: modified by starzhang
//				sendACK(packet, packet->ciaddr);	// Jenny, ack to client renewal request after CPE reboot
				ip_addr = find_IP_by_Mac(packet->chaddr);  //star, for static ip based Mac
				// Mason Yu. Req
				if ( ((ip_addr == 0 && !(find_Mac_by_IP(packet->ciaddr))) || ((ip_addr != 0) && (ip_addr == packet->ciaddr)))

#ifdef SUPPORT_DHCP_RESERVED_IPADDR
					&& ((ip_addr != 0) || ((ip_addr == 0) && (isReservedIPAddress(packet->ciaddr, server_config_p)==0)))
#endif //SUPPORT_DHCP_RESERVED_IPADDR
				#ifdef IP_BASED_CLIENT_TYPE
					&& ((ip_addr != 0) || (!check_type(packet->ciaddr, deviceCategory, server_config_p)))
/*star:20080927 START add for dhcp ip range check*/
				#else
					&& ((ip_addr != 0) || (ntohl(packet->ciaddr) >= ntohl(server_config_p->start) && ntohl(packet->ciaddr) <= ntohl(server_config_p->end)))
/*star:20080927 END*/
				#endif
				) {
#ifdef DHCPD_MULTI_THREAD_SUPPORT
					thread_mutex_lock();
					if ((lease = find_lease_by_yiaddr(packet->ciaddr))) {
						if (lease_expired(lease)) {
							/* probably best if we drop this lease */
							clear_one_lease(lease);
							add_lease(packet->chaddr, packet->ciaddr, server_config_p->lease);
							thread_mutex_unlock();
							
							sendACK(packet, packet->ciaddr, server_config_p);
						} else {
							thread_mutex_unlock();
							
							sendNAK(packet, server_config_p);
						}
					}
					else {
						add_lease(packet->chaddr, packet->ciaddr, server_config_p->lease);
						thread_mutex_unlock();

						sendACK(packet, packet->ciaddr, server_config_p);
					}
#else
					sendACK(packet, packet->ciaddr, server_config_p);
#endif
				}
				else
					sendNAK(packet, server_config_p);

			}
			break;
		case DHCPDECLINE:
			DEBUG(LOG_INFO,"received DECLINE from %x-%x-%x-%x-%x-%x xid=%#x", packet->chaddr[0], packet->chaddr[1], packet->chaddr[2],
					packet->chaddr[3], packet->chaddr[4], packet->chaddr[5], ntohl(packet->xid));
			if (lease) {
				memset(lease->chaddr, 0, 16);
				lease->expires = get_uptime() + server_config_p->decline_time;
			}
			break;
		case DHCPRELEASE:
			DEBUG(LOG_INFO,"received RELEASE from %x-%x-%x-%x-%x-%x xid=%#x", packet->chaddr[0], packet->chaddr[1], packet->chaddr[2],
					packet->chaddr[3], packet->chaddr[4], packet->chaddr[5], ntohl(packet->xid));
			if (lease) lease->expires = get_uptime();
#ifdef CONFIG_USER_LANNETINFO
			int set_lanhost_dhcp_release_mac(char *mac);
			set_lanhost_dhcp_release_mac((char *)packet->chaddr);
#endif
#ifdef _CWMP_TR111_
			if (lease) {
				del_deviceId(lease->yiaddr);
				dump_deviceId();
			}
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_NAT44
    {
        unsigned char functype=0;
        mib_get_s(AWIFI_PROVINCE_CODE,&functype, sizeof(functype));
        if(functype == AWIFI_ZJ){
			if (lease) {
				if(IsAddrInDhcpPool(lease->yiaddr))
					delNAT44Rules(lease->yiaddr);
			}
        }
    }
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_TCPOPT
    {
        unsigned char functype=0;
        mib_get_s(AWIFI_PROVINCE_CODE,&functype, sizeof(functype));
        if(functype == AWIFI_ZJ){
			if (lease) {
				delTCPOPTRules(lease->yiaddr, lease->chaddr);
			}
        }
    }
#endif
			break;
		case DHCPINFORM:
			DEBUG(LOG_INFO,"received INFORM from %x-%x-%x-%x-%x-%x xid=%#x", packet->chaddr[0], packet->chaddr[1], packet->chaddr[2],
					packet->chaddr[3], packet->chaddr[4], packet->chaddr[5],ntohl(packet->xid));
			send_inform(packet, server_config_p);

			break;

		default:
			LOG(LOG_WARNING, "unsupported DHCP message (%02x) -- ignoring", state[0]);
		}

		return 0;
}

