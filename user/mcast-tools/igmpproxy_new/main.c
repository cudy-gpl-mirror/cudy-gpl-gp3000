#include "mclab.h"
#include "timeout.h"
#include "igmpproxy.h"
#include "igmp_mld_utility.h"
#include "igmp_mld_debug.h"
#include "igmp_mld_config.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include "built_time"

#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

/*
#if defined (CONFIG_ISP_IGMPPROXY_MULTIWAN)
#include <sys/stat.h>
#endif
*/
char *igmp_recv_buf;		/* input packet buffer */
char *igmp_send_buf;		/* output packet buffer */
char *send_buf;

struct msghdr 		sndmh, rcvmh;
struct sockaddr_in 	from;
struct sockaddr_in 	to;
struct iovec 		rcviov[2];
struct iovec 		sndiov[2];
int rcvcmsglen, sndcmsglen;
static u_char   		*rcvcmsgbuf = NULL;
static u_char   		*sndcmsgbuf = NULL;
#define RECV_BUF_SIZE	2048

#define IGMP_RESV_ADDR1 ("224.0.0.2")
#define IGMP_RESV_ADDR2 ("224.0.0.22")


fd_set in_fds;		/* set of fds that wait_input waits for */
int max_in_fd;		/* highest fd set in in_fds */
int IGMP_rx_enable=0;

char igmp_down_if_name[IFNAMSIZ] = {0};
char igmp_down_if_idx;
#if defined(CONFIG_IGMPPROXY_MULTIWAN) || defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
char igmp_up_if_name[MAXWAN][IFNAMSIZ];
#else
char igmp_up_if_name[IFNAMSIZ];
#endif

#if defined(CONFIG_IGMPPROXY_MULTIWAN) || defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
char igmp_up_if_idx[MAXWAN] = {0};
#else
char igmp_up_if_idx;
#endif
#if defined(CONFIG_IGMPPROXY_MULTIWAN) || defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
int igmp_up_if_num = 0;
#endif
#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
int igmp_if_sock_old[MAX_IF] = {0}; // record old sock for remove fd
#endif
char* runPath = "/bin/igmpproxy";
char* pidfile = "/var/run/igmp_pid";

struct mcft_entry *mcpq = NULL;

struct mCastProxyConfig igmpProxyPara;

#define CONFIGSIG 34   //SIGRTMIN+2
struct sigaction igmpConfig;
int sigcofigno=CONFIGSIG;

#if defined(HOST_TRACKING_SUPPORT)
extern int handle_host_timer(struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList);
#endif
extern int chk_local( __u32 addr );
#ifndef CONFIG_ISP_IGMPPROXY_MULTIWAN
extern struct IfDesc *getIfbyPif(int pif_index);
#endif

#ifdef USE_STATIC_ENTRY_BUFFER
extern void init_static_entry_tbl(void);
#endif
extern void calltimeout(void);


extern int rtl_handleIgmpMldQuery(unsigned int *clientAddr, void *l3PktBuf, int l3PktLen, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif);
extern int rtl_handleIgmpv1Join(unsigned int *clientAddr, void *l3PktBuf, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif);
extern int rtl_handleIgmpv2Mldv1Join(unsigned int *clientAddr, void *l3PktBuf, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif);
extern int rtl_handleIgmpv3Mldv2Report(unsigned int *clientAddr, void *l3PktBuf, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif);
extern int rtl_handleIgmpMldLeave(unsigned int *clientAddr, void *l3PktBuf, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif);




void
write_pid()
{
	FILE *fp = fopen(pidfile, "w+");
	if (fp) {
		fprintf(fp, "%d\n", getpid());
		fclose(fp);
	}
	else
	 	printf("Cannot create pid file\n");
}

void
clear_pid()
{
	FILE *fp = fopen(pidfile, "w+");
	if (fp) {
		fprintf(fp, "%d\n", 0);
		fclose(fp);
	}
	else
	 	printf("Cannot create pid file\n");
}

int init_igmpProxyPara(struct mCastProxyConfig *mCastProxyPara)
{	
	mCastProxyPara->ipVersion= IP_VERSION4;
	mCastProxyPara->timeConfig.robustness = ROBUSTNESS_VARIABLE;
	mCastProxyPara->timeConfig.memberQueryInterval = MEMBER_QUERY_INTERVAL;	//MALI
	mCastProxyPara->timeConfig.memberQueryCount = MEMBER_QUERY_COUNT;
	mCastProxyPara->timeConfig.lastMemberInterval = LAST_MEMBER_QUERY_INTERVAL; //LLQI
	mCastProxyPara->timeConfig.lastMemberCount = LAST_MEMBER_QUERY_COUNT;
	mCastProxyPara->timeConfig.setUpGeneralQueryInterval = STARTUP_GENERAL_QUERY_INTERVAL;
	mCastProxyPara->timeConfig.setUpGeneralQueryCount = STARTUP_GENERAL_QUERY_COUNT;
	mCastProxyPara->timeConfig.periodGeneralQueryInterval = PERIODICAL_GENERAL_QUERY_INTERVAL; //qqi
	mCastProxyPara->timeConfig.periodGeneralQueryCount = PERIODICAL_GENERAL_QUERY_COUNT;
	mCastProxyPara->timeConfig.QueryResponseInterval = GENERAL_QUERY_RESPONSE_INTERVAL;
	mCastProxyPara->timeConfig.linkChangeQueryInterval = LINK_CHANGE_QUERY_INTERVAL;
	mCastProxyPara->timeConfig.linkChangeQueryCount = LINK_CHANGE_QUERY_TIMES;
	mCastProxyPara->timeConfig.intervalScale = INTERVAL_SCALE;//mrd in general query=QueryResponseInterval*intervalScale
	mCastProxyPara->timeConfig.oldVersionHostPresentTimeout= OLD_VERSION_HOST_PRESENT_TIMEOUT;
	mCastProxyPara->forceVersion= IGMP_VER_3;
	mCastProxyPara->fastleaveEnable = 0;
	mCastProxyPara->compatibleEnbale = 0;

	config_igmpproxy_para_from_file(&igmpProxyPara);
	write_igmpproxy_para_to_file(&igmpProxyPara);

	//sync time config to driver
	sync_igmpsnooping_para(&igmpProxyPara);

	return 0;
}

/********signal function begin**********/
//#define IGMP_MCAST2UNI
#ifdef	IGMP_MCAST2UNI
static void Update_igmpProxyStateToKernel(int igmpProxyEnable)
{
	//printf("\nUpdate_igmpProxyStateToKernel()\n");
	char *br_igmpProxy_Proc = "/proc/br_igmpProxy";
	char valueBeWirte[2]="01";
	FILE *fp = fopen(br_igmpProxy_Proc, "r+");
	int success;
	if (!fp) {
		printf("igmpProxy-error:Open %s file error.\n", br_igmpProxy_Proc);
		return;
	}

	if(igmpProxyEnable==1){
		success = fputc('1', fp);
	}else{
		success = fputc('0', fp);				
	}
	if(success==EOF) 
		printf("igmpProxy-error:Update_igmpProxyState fail.\n");
	
	fclose(fp);
	
}
#endif

void hup(int signum)
{
	(void)signum;

  igmpproxy_log( LOG_DEBUG, 0, "clean handler called" );
#ifdef	IGMP_MCAST2UNI  
  Update_igmpProxyStateToKernel(0);
#endif
  disableMRouter();

  unlink(pidfile);
  exit(EXIT_SUCCESS);

}

void sigsegv(int signum)
{
	printf("receive a SIGSEGV\n");
	sleep(1);
}

void sigifup(int signum)
{
	(void)signum;
	struct ifreq IfVc[ MAX_IF  ];
	struct ifreq *IfEp, *IfPt;
	struct ifconf IoCtlReq;
	struct IfDesc *Dup, *Ddp;
	int Sock;
	
	syslog(LOG_INFO, "igmpproxy: SIGUSR1 caught\n");
	// get information of all the interfaces
	if( (Sock = socket( AF_INET, SOCK_DGRAM, 0 )) < 0 )
		igmpproxy_log( LOG_ERR, errno, "RAW socket open" );
	
	IoCtlReq.ifc_buf = (void *)IfVc;
	IoCtlReq.ifc_len = sizeof( IfVc );
	
	if( ioctl( Sock, SIOCGIFCONF, &IoCtlReq ) < 0 )
		igmpproxy_log( LOG_ERR, errno, "ioctl SIOCGIFCONF" );
	
	close( Sock );
	IfEp = (void *)((char *)IfVc + IoCtlReq.ifc_len);
#if defined(CONFIG_IGMPPROXY_MULTIWAN) || defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
    int index;
#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
	struct IfDesc *Dp;
	
	// update upstream/downstream interface ip into local database
	for( IfPt = IfVc; IfPt < IfEp; IfPt++ ) {
		for(index = 0, Dp=IfDescVc; index<MAX_IF ;index++, Dp++)
	    {
	       	if(strcmp(Dp->Name, "")==0)
				continue;
			
		  	if (strcmp(IfPt->ifr_name, Dp->Name) == 0) {
				Dp->InAdr = ((struct sockaddr_in *)&IfPt->ifr_addr)->sin_addr;
				//printf("update upstream ip to %s\n", inet_ntoa(Dup->InAdr));
				// Update default multicast interface for this socket.
				setsockopt(Dp->sock, IPPROTO_IP, IP_MULTICAST_IF, 
					(void*)&Dp->InAdr, sizeof(struct in_addr));
		   	}
	    }
	}
#else
	Ddp = getIfByName(igmp_down_if_name);
	if ( Ddp == NULL)
		return;
	
	// update upstream/downstream interface ip into local database
	for( IfPt = IfVc; IfPt < IfEp; IfPt++ ) {
		
		for(index = 0;index<igmp_up_if_num;index++)
	       {
	           Dup = getIfByName(igmp_up_if_name[index]);
		   if ( Dup == NULL)
		       continue;  
		  igmp_up_if_idx[index] = addVIF(Dup);
		  if (!strcmp(IfPt->ifr_name, Dup->Name)) {
			Dup->InAdr = ((struct sockaddr_in *)&IfPt->ifr_addr)->sin_addr;
			//printf("update upstream ip to %s\n", inet_ntoa(Dup->InAdr));
			// Update default multicast interface for this socket.
			setsockopt(Dup->sock, IPPROTO_IP, IP_MULTICAST_IF, 
				(void*)&Dup->InAdr, sizeof(struct in_addr));
		   }
	        }
		
	      if (!strcmp(IfPt->ifr_name, Ddp->Name)) {
			Ddp->InAdr = ((struct sockaddr_in *)&IfPt->ifr_addr)->sin_addr;
			//printf("update downstream ip to %s\n", inet_ntoa(Ddp->InAdr));
			// Update default multicast interface for this socket.
			setsockopt(Ddp->sock, IPPROTO_IP, IP_MULTICAST_IF, 
				(void*)&Ddp->InAdr, sizeof(struct in_addr));
	      }
	}

#endif
#else
	// get descriptors of upstream and downstream interfaces
	Dup = getIfByName(igmp_up_if_name);
	Ddp = getIfByName(igmp_down_if_name);
	if (Dup == NULL || Ddp == NULL)
		return;
	
	// update upstream/downstream interface ip into local database
	for( IfPt = IfVc; IfPt < IfEp; IfPt++ ) {
		if (!strcmp(IfPt->ifr_name, Dup->Name)) {
			Dup->InAdr = ((struct sockaddr_in *)&IfPt->ifr_addr)->sin_addr;
			//printf("update upstream ip to %s\n", inet_ntoa(Dup->InAdr));
			// Update default multicast interface for this socket.
			setsockopt(Dup->sock, IPPROTO_IP, IP_MULTICAST_IF, 
				(void*)&Dup->InAdr, sizeof(struct in_addr));
		}
		else if (!strcmp(IfPt->ifr_name, Ddp->Name)) {
			Ddp->InAdr = ((struct sockaddr_in *)&IfPt->ifr_addr)->sin_addr;
			//printf("update downstream ip to %s\n", inet_ntoa(Ddp->InAdr));
			// Update default multicast interface for this socket.
			setsockopt(Ddp->sock, IPPROTO_IP, IP_MULTICAST_IF, 
				(void*)&Ddp->InAdr, sizeof(struct in_addr));
		}
	}
#endif
}
void callback_usr2(int signum)
{
	(void)signum;
	printf("query interval:%d, macro:%d, [%s:%d]\n", igmpProxyPara.timeConfig.periodGeneralQueryInterval, PERIODICAL_GENERAL_QUERY_INTERVAL, __FUNCTION__, __LINE__);

	set_link_change_query(&igmpProxyPara);	

	
	printf("query interval:%d, macro:%d, [%s:%d]\n", igmpProxyPara.timeConfig.periodGeneralQueryInterval, PERIODICAL_GENERAL_QUERY_INTERVAL, __FUNCTION__, __LINE__);
}

void sigIgmpConfig(int signum, siginfo_t * info, void *ptr)
{
	int para = info->si_int;

	config_igmpproxy_para_from_file(&igmpProxyPara);
	write_igmpproxy_para_to_file(&igmpProxyPara);
	sync_igmpsnooping_para(&igmpProxyPara);
}

#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
int isFileExist(char *file_name)
{
	struct stat status;

	if ( stat(file_name, &status) < 0)
		return 0;

	return 1;
}

int updateIgmpWanUp(void)
{
	FILE *fp = NULL;
	char buf[MAXWAN*(IFNAMSIZ+1)] = {0};
	char *token = NULL, *saveStr = NULL;
	int i;
	
	if(!isFileExist(IGMPPROXY_UP))
	{
		goto err_return;
	}

	fp = fopen(IGMPPROXY_UP, "r");
	if(!fp)
		goto err_return;

	memset(buf, 0, sizeof(buf));
	fgets(buf, sizeof(buf), fp);

	saveStr = buf;

	token = strsep(&saveStr,",");
	
	memset(igmp_up_if_name, 0, MAXWAN*IFNAMSIZ);
	igmp_up_if_num = 0;
	
	while(token)
	{
		strncpy(igmp_up_if_name[igmp_up_if_num], token,sizeof(igmp_up_if_name[igmp_up_if_num]));
		igmp_up_if_num++;
		token = strsep(&saveStr,",");
	}

#if 0
	for(i=0;i<MAXWAN;i++)
	{
		if(strcmp(igmp_up_if_name[i], "")!=0)
		{
			printf("[%s %d] %s!\n", __FUNCTION__, __LINE__, igmp_up_if_name[i]);
		}
	}
#endif
	return 0;
	
err_return:
	printf("[%s %d] update igmp wan interface fail!\n", __FUNCTION__, __LINE__);
	return -1;

}

void sigWanUp(int signum, siginfo_t * info)
{	

	int WanIfFlag;
	//printf("signum = %d, parameter = %x, [%s:%d]\n", signum, info->si_int, __FUNCTION__, __LINE__);
	WanIfFlag = info->si_int;
	if(WanIfFlag&(ADD_WAN_IF|DEL_WAN_IF))
		get_wan_signo = 1;

	updateIgmpWanUp();
}

#endif
/********signal function end**********/

/*
** Inits the necessary resources for MRouter.
**
*/
static int initMRouter(void)
{
	int Err;
	int i;
	struct IfDesc *Ddp, *Dup;
	int proxyUp = 0;
	int ret;

#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
	memset(IfDescVc, 0, sizeof(struct IfDesc)*(MAX_IF));
#endif
	
	buildIfVc();    

	switch( Err = enableMRouter() ) {
		case 0: break;
		case EADDRINUSE: igmpproxy_log( LOG_ERR, EADDRINUSE, "MC-Router API already in use" ); break;
		default: igmpproxy_log( LOG_ERR, Err, "MRT_INIT failed" );
	}

	if (!igmp_recv_buf && (igmp_recv_buf = malloc(RECV_BUF_SIZE)) == NULL)
	    igmpproxy_log(LOG_ERR, 0, "malloc failed");
    if (!igmp_send_buf && (igmp_send_buf = malloc(RECV_BUF_SIZE)) == NULL)
	    igmpproxy_log(LOG_ERR, 0, "malloc failed");

    rcvcmsglen = CMSG_SPACE(sizeof(struct in_pktinfo)) +
	    CMSG_SPACE(sizeof(int));
    if (rcvcmsgbuf == NULL && (rcvcmsgbuf = malloc(rcvcmsglen)) == NULL)
	    igmpproxy_log(LOG_ERR, 0,"malloc failed");

#if defined(CONFIG_IGMPPROXY_MULTIWAN) || defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
	struct IfDesc *Dp;
	for(i=0, Dp=IfDescVc; i<MAX_IF; i++, Dp++)
	{
		if(strcmp(Dp->Name, igmp_down_if_name)==0)
		{
			igmp_down_if_idx = addVIF(Dp);
		}
		else if(strcmp(Dp->Name, "") !=0)
		{
			proxyUp = 1;
			addVIF(Dp);
		}
	}
#else
    
	/* add upstream interface */
	 for(i = 0;i<igmp_up_if_num;i++)
	{
		Dup = getIfByName(igmp_up_if_name[i]);
		if (( Dup == NULL) || (Dup->InAdr.s_addr == 0))
			continue;  
		else
			proxyUp = 1;
		
		ret = addVIF(Dup);
		if(ret == -1)
			return 0;
		Dup->vif_idx = ret;
		igmp_up_if_idx[i] = ret;
    }

	/* add downstream interface */
	Ddp = getIfByName(igmp_down_if_name);
	if (Ddp==NULL )
		return 0;
	
	ret = addVIF(Ddp);
	if(ret == -1)
		return 0;
	Ddp->vif_idx = ret;
	igmp_down_if_idx = ret;
	printf("----igmp_down_if_idx = %d,Dup->pif_idx =%d,Dup->vif_idx=%d----\n",igmp_down_if_idx,Ddp->pif_idx,Ddp->vif_idx);

#endif

	if(proxyUp == 0)
	{		
		return 0;
	}
#else
	Ddp = getIfByName(igmp_down_if_name);
	Dup = getIfByName(igmp_up_if_name);
	if (Ddp==NULL || Dup==NULL)
		return 0;
	
	/* add downstream interface */
	igmp_down_if_idx = addVIF(Ddp);

	/* add upstream interface */
	igmp_up_if_idx = addVIF(Dup);
#endif

/* initialize msghdr for receiving packets */
    rcviov[0].iov_base = (caddr_t) igmp_recv_buf;
    rcviov[0].iov_len = RECV_BUF_SIZE;
    rcvmh.msg_name = (caddr_t) & from;
    rcvmh.msg_namelen = sizeof(from);
    rcvmh.msg_iov = rcviov;
    rcvmh.msg_iovlen = 1;
    rcvmh.msg_control = (caddr_t) rcvcmsgbuf;
    rcvmh.msg_controllen = rcvcmsglen;

    /* initialize msghdr for sending packets */
    sndiov[0].iov_base = (caddr_t)igmp_send_buf;
	sndmh.msg_name = (caddr_t) & to;
    sndmh.msg_namelen =sizeof(to);
    sndmh.msg_iov = sndiov;
    sndmh.msg_iovlen = 1;

	

	signal(SIGTERM, hup);

#if 0//defined (CONFIG_QUERIER_SELECTION)
	initQuerierSelection();
#endif

  //atexit( clean );
  	return 1;
}

/*
 * add_fd - add an fd to the set that wait_input waits for.
 */
void add_fd(int fd)
{
    FD_SET(fd, &in_fds);
    if (fd > max_in_fd)
	max_in_fd = fd;
}

/*
 * remove_fd - remove an fd from the set that wait_input waits for.
 */
void remove_fd(int fd)
{
    FD_CLR(fd, &in_fds);
}

/*add igmp socket*/
#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
int igmp_inf_create(struct IfDesc *dp)
{
	int i;
	int ret;
	struct ip_mreqn mreqn;
	
	if(strcmp(dp->Name, "")==0)
		return 0;
		
	mreqn.imr_ifindex = dp->pif_idx;
	memcpy(&mreqn.imr_address, &dp->InAdr, sizeof(struct in_addr));

	if ((dp->sock = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP)) < 0) 
	{
		igmpproxy_log(LOG_ERR, errno, "IGMP socket");
		return -1;//fix coverity:NEGATIVE_RETURNS
	}
	{	
		/* Set router alert option*/
		char ra[4];
		ra[0] = 148;
		ra[1] = 4;
		ra[2] = 0;
		ra[3] = 0;
		//setsockopt(dp->sock, IPPROTO_IP, IP_OPTIONS, ra, 4);
		ret=setsockopt(dp->sock, IPPROTO_IP, IP_OPTIONS, ra, 4);
		if(ret)
			printf("setsockopt IP_OPTIONS error!\n");//fix coverity:CHECKED_RETURN
	}
	
	/* init igmp */ 
	/* Set reuseaddr, ttl, loopback and set outgoing interface */
	i = 1;
	ret = setsockopt(dp->sock, SOL_SOCKET, SO_REUSEADDR, (void*)&i, sizeof(i));
	if(ret)
		printf("setsockopt SO_REUSEADDR error!\n");
	i = 1;
	ret = setsockopt(dp->sock, IPPROTO_IP, IP_MULTICAST_TTL, 
		(void*)&i, sizeof(i));
	if(ret)
		printf("setsockopt IP_MULTICAST_TTL error!\n");

	//eddie disable LOOP
	i = 0;
	ret = setsockopt(dp->sock, IPPROTO_IP, IP_MULTICAST_LOOP, 
		(void*)&i, sizeof(i));
	if(ret)
		printf("setsockopt IP_MULTICAST_LOOP error!\n");
		
	ret = setsockopt(dp->sock, IPPROTO_IP, IP_MULTICAST_IF, 
		(void*)&mreqn, sizeof(mreqn));
	if(ret)
		printf("setsockopt IP_MULTICAST_IF error!\n");
	
	/* In linux use IP_PKTINFO */
	//IP_RECVIF returns the interface of received datagram
	i = 1;
	ret = setsockopt(dp->sock, IPPROTO_IP, IP_PKTINFO, &i, sizeof(i));
	if(ret)
		printf("setsockopt IP_PKTINFO error!\n");
	
	return 0;

}
#else
int igmp_inf_create(char *ifname)
{
	struct ip_mreq mreq;
	int i;
	int ret;
	struct IfDesc *dp;


	dp = getIfByName(ifname);
	if(dp==NULL)
		return 0;
	
    if ((dp->sock = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP)) < 0) 
    {
		igmpproxy_log(LOG_ERR, errno, "IGMP socket");
		return -1;
		//fix coverity:NEGATIVE_RETURNS
    }

	{	
		/* Set router alert option*/
		char ra[4];
		ra[0] = 148;
		ra[1] = 4;
		ra[2] = 0;
		ra[3] = 0;
		//setsockopt(dp->sock, IPPROTO_IP, IP_OPTIONS, ra, 4);
		ret=setsockopt(dp->sock, IPPROTO_IP, IP_OPTIONS, ra, 4);
		if(ret)
			printf("setsockopt IP_OPTIONS error!\n");//fix coverity:CHECKED_RETURN
	}

	/* init igmp */	
	/* Set reuseaddr, ttl, loopback and set outgoing interface */
	i = 1;
	ret = setsockopt(dp->sock, SOL_SOCKET, SO_REUSEADDR, (void*)&i, sizeof(i));
	if(ret)
		printf("setsockopt SO_REUSEADDR error!\n");
	i = 1;
	ret = setsockopt(dp->sock, IPPROTO_IP, IP_MULTICAST_TTL, 
		(void*)&i, sizeof(i));
	if(ret)
		printf("setsockopt IP_MULTICAST_TTL error!\n");
	//eddie disable LOOP
	i = 0;
	ret = setsockopt(dp->sock, IPPROTO_IP, IP_MULTICAST_LOOP, 
		(void*)&i, sizeof(i));
	if(ret)
		printf("setsockopt IP_MULTICAST_LOOP error!\n");
	ret = setsockopt(dp->sock, IPPROTO_IP, IP_MULTICAST_IF, 
		(void*)&dp->InAdr, sizeof(struct in_addr));
	if(ret)
		printf("setsockopt IP_MULTICAST_IF error!\n");

	/* In linux use IP_PKTINFO */
	//IP_RECVIF returns the interface of received datagram
	i = 1;
	ret = setsockopt(dp->sock, IPPROTO_IP, IP_PKTINFO, &i, sizeof(i));
	if(ret)
		printf("setsockopt IP_PKTINFO error!\n");

	//ret = fcntl(dp->sock, F_SETFL, O_NONBLOCK);
	//if(ret)
	//	printf("fcntl O_NONBLOCK error!\n");
	#if 0
	if (setsockopt(dp->sock, SOL_SOCKET, SO_BINDTODEVICE, ifname,strlen(ifname) + 1) == -1)
	{
		printf("bind to %s error", ifname);
	}
	#endif
	return 0;
	
}
#endif

/*
 * Add 224.0.0.2 (all routers) and 224.0.0.22 (IGMPv3) membership
 * on downstream interface in order to receive IGMP messages.
 */
static int add_IGMPMembership()
{
	struct IfDesc *dp;
	struct ip_mreq imr;	/* Ip multicast membership */
	
	dp = getIfByName(igmp_down_if_name);
	if(dp==NULL)
		return 0;
	
	/* setting imr structure */
	imr.imr_multiaddr.s_addr = inet_addr(IGMP_RESV_ADDR1);
	/*imr.imr_interface.s_addr = htonl(INADDR_ANY);*/
	imr.imr_interface.s_addr = dp->InAdr.s_addr;
	//printf("add membership at %s\n", inet_ntoa(dp->InAdr));
	
	if (setsockopt(MRouterFD, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&imr, sizeof(struct ip_mreq)) < 0)
	{
		syslog(LOG_ERR, "%s(%d): IGMP IP_ADD_MEMBERSHIP %s failed.", __FUNCTION__, __LINE__, inet_ntoa(imr.imr_multiaddr));
		return -1;
	}
	
	imr.imr_multiaddr.s_addr = inet_addr(IGMP_RESV_ADDR2);
	if (setsockopt(MRouterFD, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&imr, sizeof(struct ip_mreq)) < 0)
	{
		syslog(LOG_ERR, "%s(%d): IGMP IP_ADD_MEMBERSHIP %s failed.", __FUNCTION__, __LINE__, inet_ntoa(imr.imr_multiaddr));
		return -1;
	}
	
	return 0;
}


int init_igmp(void)
{
	int val;

	send_buf = malloc(RECV_BUF_SIZE);

	FD_ZERO(&in_fds);
	max_in_fd = 0;

	

#if defined(CONFIG_IGMPPROXY_MULTIWAN) || defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
	int idx;
#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
	struct IfDesc *Dp;
	for(idx=0, Dp=IfDescVc; idx<MAX_IF; idx++, Dp++)
	{
		igmp_inf_create(Dp);
	}
#else
	igmp_inf_create(igmp_down_if_name);

	for(idx=0;idx<igmp_up_if_num;idx++)
		igmp_inf_create(igmp_up_if_name[idx]);
#endif
#else
	igmp_inf_create(igmp_down_if_name);
	igmp_inf_create(igmp_up_if_name);
#endif
	printf("query interval:%d, macro:%d, [%s:%d]\n", igmpProxyPara.timeConfig.periodGeneralQueryInterval, PERIODICAL_GENERAL_QUERY_INTERVAL, __FUNCTION__, __LINE__);
	init_general_query(&igmpProxyPara);
	
	printf("query interval:%d, macro:%d, [%s:%d]\n", igmpProxyPara.timeConfig.periodGeneralQueryInterval, PERIODICAL_GENERAL_QUERY_INTERVAL, __FUNCTION__, __LINE__);

	// Kaohj --- enable IGMP rx

	add_IGMPMembership(); // Add 224.0.0.2, 224.0.0.22 for IGMP receiving
	IGMP_rx_enable = 1;
	return 0;
}
/*
 * u_short in_cksum(u_short *addr, int len)
 *
 * Compute the inet checksum
 */
unsigned short in_cksum(unsigned short *addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    if (nleft == 1) {
        *(unsigned char*)(&answer) = *(unsigned char*)w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    answer = ~sum;
    return (answer);
}

int igmp_make_query(int version, unsigned int grp, unsigned char mrt, unsigned char qqic, unsigned char qrv, short srcnum, unsigned int *srcfilter, unsigned char *buf)
{
	int totalsize = 0, i=0;
	
	if(buf == NULL)
	{
		printf("No buff to make query!\n");
		return 0;
	}

	if(srcnum && srcfilter==NULL)
	{
		printf("Error Input Parameter, [%s:%d]\n", __FUNCTION__, __LINE__);
		return 0;
	}
	
	if(version==IGMP_VER_3)
	{
		struct igmpv3_query *igmpv3 = (struct igmpv3_query *)buf;
		igmpv3->type	  = 0x11;
		igmpv3->code	  = mrt;
		igmpv3->csum	  = 0;
		igmpv3->group	  = grp;
		igmpv3->resv	  = 0;
		igmpv3->suppress  = 1;
		igmpv3->qrv 	  = qrv;
		igmpv3->qqic	  = qqic;
		igmpv3->nsrcs	  = htons(srcnum);
	    for(i=0;i<srcnum;i++)
	    {
	    	igmpv3->srcs[i] = srcfilter[i];
	    }
		totalsize		  = sizeof(struct igmpv3_query)+ srcnum*sizeof(__u32);
		igmpv3->csum	  = in_cksum((u_short *)igmpv3, totalsize);
		
	}
	else if(version==IGMP_VER_2 || version==IGMP_VER_1)
	{
		struct igmphdr *igmp = (struct igmphdr *)buf;
	    igmp->type         = 0x11;
	    igmp->code         = mrt;
	    igmp->group 	   = grp;
	    igmp->csum        = 0;
	    igmp->csum        = in_cksum((u_short *)igmp, IGMP_MINLEN);
		totalsize = IGMP_MINLEN;
	}
	else
	{
		totalsize = 0;
	}

	return totalsize;
}

/*
 * igmp_query - send an IGMP Query packet to downstream interface
 *
 * int igmp_query(i__u32 dst, __u32 grp,__u8 mrt, )
 * Where:
 *  dst		destination address
 *  grp		query group address
 *  MRT		Max Response Time in IGMP header (in 1/10 second unit)
 *
 * Returns:
 *	0	if unable to send
 *	1	packet was sent successfully
 */
int igmp_send_query(int version, unsigned int dst, unsigned int grp, unsigned char mrt, unsigned char qqic, unsigned char qrv, unsigned short srcnum, unsigned int *srcfilter)
{
	int totalsize = 0;
    struct sockaddr_in sdst;
    struct IfDesc *dp = getIfByName(igmp_down_if_name);

#if 0//defined (CONFIG_QUERIER_SELECTION)
	if(querierSelectionResult==QUERIER_IS_OTHER)
	{
		return 0;
	}	
#endif

	if(dp == NULL)
	{
		printf("get if(%s) failed\n",igmp_down_if_name);
		return 0;
	}
	
	totalsize = igmp_make_query(version,grp,mrt,qqic,qrv,srcnum,srcfilter,(unsigned char*)igmp_send_buf);

	if(totalsize==0)
	{
		printf("make query fail!\n");
		return 0;
	}

	DBG_IGMP_MLD_PRK(IGMP_PROTO, "dst:%s, group:%s, mrt:%d, qqic:%d, srcNum:%d!\n", 
		addr_sprintf(&dst,4),addr_sprintf(&grp,4), mrt, qqic, srcnum);

	bzero(&sdst, sizeof(struct sockaddr_in));
    sdst.sin_family = AF_INET;
    sdst.sin_addr.s_addr = dst;
	
    if (sendto(dp->sock, igmp_send_buf, totalsize, 0, (struct sockaddr *)&sdst, sizeof(sdst)) < 0)
    {
		printf("send igmp v%d query error, from 0x%x ", version, dp->InAdr.s_addr);
		printf("to 0x%x\n", sdst.sin_addr.s_addr);
    }

    return 0;
	
}

static struct timeval start_time={0,0};
static int init_stat_time=0;
extern int handle_compatibility_timer(struct mCastProxyConfig *mCastProxyPara);

void igmpv3_timer(void)
{
	struct timeval cur_time;
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)
	unsigned char time_scale;
	int diff_time_ms;
#endif	
	if( init_stat_time==0 )
	{
		gettimeofday( &start_time, NULL );
		init_stat_time = 1;
		return;
	}
	
	gettimeofday( &cur_time, NULL );
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)
	time_scale = igmpProxyPara.timeConfig.intervalScale;
	diff_time_ms = (cur_time.tv_sec*time_scale+cur_time.tv_usec*time_scale/1000000)-(start_time.tv_sec*time_scale+start_time.tv_usec*time_scale/1000000);
	
	if (diff_time_ms >= 1 || diff_time_ms<0)
#else
	if( (cur_time.tv_sec > (start_time.tv_sec+1)) ||
    ((cur_time.tv_sec == (start_time.tv_sec+1)) && (cur_time.tv_usec >=start_time.tv_usec)) )
#endif
	{
		//suppose 1 second passed
		//printf( "." );fflush(NULL);
	#if defined(HOST_TRACKING_SUPPORT)
		handle_host_timer(&igmpProxyPara, &mcpq);
	#else
		handle_group_timer(&igmpProxyPara, &mcpq);
		handle_src_timer(&igmpProxyPara, &mcpq);
	#endif
	#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
		handle_compatibility_timer(&igmpProxyPara);
	#endif
		
		start_time.tv_sec = cur_time.tv_sec;
		start_time.tv_usec = cur_time.tv_usec;
	}

	return;
}
//exclude NULL
//test ok, 2017.06.4
int igmp_add_group(unsigned int group, int vif)
{
	struct ip_mreqn mreq;
	struct IfDesc *dp = getIfbyVif(vif);

	if(dp == NULL)
		return -1;

	/* join multicast group */
	mreq.imr_multiaddr.s_addr = group;
	mreq.imr_address.s_addr = dp->InAdr.s_addr;
	mreq.imr_ifindex = dp->pif_idx;
	
	DBG_IGMP_MLD_PRK(IGMP_MEMBERSHIP, "add group %s on %s(vif:%d)\n",
		inet_ntoa(mreq.imr_multiaddr), dp->Name, dp->vif_idx);

#if defined(CONFIG_RTL_NEW_IGMP_REPORT_BEHAVIOR)
	if(setsockopt(dp->sock, IPPROTO_IP, IP_ADD_IN_MEMBERSHIP, (void*)&mreq, sizeof(mreq))<0)
#else
	if(setsockopt(dp->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&mreq, sizeof(mreq))<0)
#endif
	{
		if(errno!=EADDRINUSE)
		{
			printf("setsockopt IP_ADD_MEMBERSHIP %s error %d!\n", inet_ntoa(mreq.imr_multiaddr), errno);
			return -1;
		}
	}
	
	DBG_IGMP_MLD_PRK(IGMP_MEMBERSHIP, "add group %s successfully\n", inet_ntoa(mreq.imr_multiaddr));

	return 0;		
}

//test ok, 2017.06.4
int igmp_del_group(unsigned int group, int vif)
{
	struct ip_mreqn mreq;
	struct IfDesc *dp = getIfbyVif(vif);

	if(dp == NULL)
		return -1;

	/* drop multicast group */
	mreq.imr_multiaddr.s_addr = group;
	mreq.imr_address.s_addr = dp->InAdr.s_addr;
	mreq.imr_ifindex = dp->pif_idx;

	
	DBG_IGMP_MLD_PRK(IGMP_MEMBERSHIP, "Leave group %s on %s(vif:%d)\n",
		inet_ntoa(mreq.imr_multiaddr), dp->Name, dp->vif_idx);

	if(setsockopt(dp->sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void*)&mreq, sizeof(mreq))<0)
	{
		if(errno != EADDRNOTAVAIL)
		{
			printf("setsockopt IP_DROP_MEMBERSHIP %s error %d!\n", inet_ntoa(mreq.imr_multiaddr), errno);
			return -1;
		}
	}

	
	DBG_IGMP_MLD_PRK(IGMP_MEMBERSHIP, "Leave group %s successfully!\n", inet_ntoa(mreq.imr_multiaddr));

	return 0;
}

/*	
	must add group membership first. 
	vif: on which membership learned
*/
//test ok, 2017.06.4
int igmp_set_srcfilter(unsigned int group, int filterMode, int srcNum, unsigned int *igmpSrcList, int vif)
{
	struct ip_msfilter *imsfp;
	int size;
	//struct IfDesc *up_dp = getIfByName(igmp_up_if_name);
	struct IfDesc *dp = getIfbyVif(vif);

	if(dp == NULL) 
		return -1;

	imsfp = (struct ip_msfilter *)send_buf;
	imsfp->imsf_multiaddr = group;
	imsfp->imsf_interface = dp->InAdr.s_addr;
	imsfp->imsf_fmode = filterMode;
	imsfp->imsf_numsrc = srcNum;

	if(srcNum)
		memcpy(imsfp->imsf_slist, igmpSrcList, srcNum*sizeof(unsigned int));
	else
		imsfp->imsf_slist[0] = 0;
	

	size = IP_MSFILTER_SIZE(srcNum);

	if (setsockopt(dp->sock, IPPROTO_IP, IP_MSFILTER, imsfp, size) < 0)
	{
		printf("setsockopt IP_MSFILTER error:%d\n", errno); 
		return -1;
	}

	DBG_IGMP_MLD_PRK(IGMP_MEMBERSHIP, "Set srcfilter %x successfully!\n", imsfp->imsf_multiaddr);

	return 0;
}

//vif: outvif, not support multiwan yet
//in_vif, out_vif : for date, is opposite to igmp packets for proxy
//test ok, 2017.06.4
int igmp_add_mr(unsigned int group, unsigned int src, int enable, int in_vif, int out_vif)
{
	struct MRouteDesc	mrd;

	/* add multicast routing entry */
	if(src)
	{
		//(src, group)
		mrd.OriginAdr.s_addr = src;
	}
	else
	{
		//(*, group)
		mrd.OriginAdr.s_addr = htonl(in_vif);
	}

	mrd.SubsAdr.s_addr = 0;   //not used yet
	mrd.McAdr.s_addr = group;

	mrd.InVif = in_vif;
	memset(mrd.TtlVc, 0, sizeof(mrd.TtlVc));
	mrd.TtlVc[out_vif] = enable;	

	DBG_IGMP_MLD_PRK(IGMP_MFC, "add MFC, src:%x group:%x from %d to %d\n",
		mrd.OriginAdr.s_addr, mrd.McAdr.s_addr, in_vif, out_vif);
	

	return (addMRoute(&mrd));
}
//test ok, 2017.06.4
int igmp_del_mr(unsigned int group, unsigned int src, int in_vif)
{
	struct MRouteDesc	mrd;
	int ret=0;

	/* delete multicast routing entry */
	if(src)
	{
		mrd.OriginAdr.s_addr = src;
	}
	else
	{
		mrd.OriginAdr.s_addr = htonl(in_vif);
	}

	mrd.McAdr.s_addr = group;
	mrd.InVif = in_vif;
	memset(mrd.TtlVc, 0, sizeof(mrd.TtlVc));

	DBG_IGMP_MLD_PRK(IGMP_MFC, "remove MFC, src:%x group:%x from %d\n",
		mrd.OriginAdr.s_addr, mrd.McAdr.s_addr, in_vif);

	return(delMRoute(&mrd));	
}


int igmpv3_accept(int recvlen, struct IfDesc *dp)
{
	unsigned int src, dst, group;
	struct iphdr *ip;
	unsigned char *l3PktBuf;
	int ipdatalen, iphdrlen;
	struct mcft_entry *mymcp;
	struct in_pktinfo *info;
	int in_ifindex = -1;
	int in_vif = -1, out_vif = -1;
	struct IfDesc *inDesc = NULL;
	int type = 0;
	int l3PktLen = 0;
#if defined(CONFIG_IGMPPROXY_MULTIWAN)
	int index, i;
#endif
	
	
	if (recvlen < sizeof(struct iphdr)) 
	{
		igmpproxy_log(LOG_WARNING, 0, "received packet too short (%u bytes) for IP header", recvlen);
		return 0;
	}

	struct cmsghdr *cm;
	ip = (struct iphdr *) rcvmh.msg_iov[0].iov_base;

	src = ip->saddr;
	dst = ip->daddr;

	if(!IN_MULTICAST(ntohl(dst)))
		return -1; 

	if(chk_local(src)) 	/* It's our report looped back */
		return -1;

	//if(dst == ALL_PRINTER)	/* It's MS-Windows UPNP all printers notify */
	//	return -1;
		
	for (cm = (struct cmsghdr *) CMSG_FIRSTHDR(&rcvmh);
	     cm;
	     cm = (struct cmsghdr *) CMSG_NXTHDR(&rcvmh, cm))
	{
		if (cm->cmsg_level ==IPPROTO_IP&&cm->cmsg_type == IP_PKTINFO &&
	    	cm->cmsg_len == CMSG_LEN(sizeof(struct in_pktinfo)))
		{
			//printf("get information via Advanced API[%s]:[%d].\n",__FUNCTION__,__LINE__);
			info = (struct in_pktinfo *) (CMSG_DATA(cm));
			//this ifindex is physical index
			in_ifindex = info->ipi_ifindex;
		}
	}

	inDesc = getIfbyPif(in_ifindex);
	if(inDesc==NULL)
	{
		DBG_IGMP_MLD_PRK(IGMP_RECEIVE, "receive a igmp from %x to %x on a unknown vif, in_ifindex =%d\n", src, dst,in_ifindex);
		return -1;
	}

	DBG_IGMP_MLD_PRK(IGMP_RECEIVE, "receive a igmp from %x to %x on a vif  =%d,inDesc->vif_idx =%d\n", src, dst,in_ifindex,inDesc->vif_idx);
		
	in_vif = inDesc->vif_idx;
	if(in_vif==NO_VIF)
	{
		DBG_IGMP_MLD_PRK(IGMP_RECEIVE, "receive a igmp from %x to %x on a wrong vif\n", src, dst);
		return -1;
	}
	

#if defined(CONFIG_IGMPPROXY_MULTIWAN)
	for(index=0;index<igmp_up_if_num;index++)
	{
		out_vif = getOutVif(in_vif,index);
		if(out_vif==NO_VIF )
			continue;
#else
	out_vif = getOutVif(inDesc);
	if(out_vif==NO_VIF )
	{
		DBG_IGMP_MLD_PRK(IGMP_RECEIVE, "receive a igmp from %x to %x on a wrong vif\n", src, dst);
		return -1;
	}

	{
#endif
		
		
		iphdrlen  = ip->ihl << 2;	
		ipdatalen = ntohs(ip->tot_len);	
		l3PktLen = ipdatalen-iphdrlen;
		l3PktBuf = (rcvmh.msg_iov[0].iov_base + iphdrlen);
		type = ((struct igmphdr *)l3PktBuf)->type;

		if((IS_IGMP_QUERY(type)&&in_vif==igmp_down_if_idx))
		{
		
			DBG_IGMP_MLD_PRK(IGMP_RECEIVE, "receive query from from lan side, IGNORE\n");
			return -1;
		}

#if defined(CONFIG_IGMPPROXY_MULTIWAN)
		for(i = 0; i < igmp_up_if_num;i++)
		{
			if(IS_IGMP_REPORT(type)&&in_vif==igmp_up_if_idx[i])
			{
				DBG_IGMP_MLD_PRK(IGMP_RECEIVE, "receive receive report from Wan side, IGNORE\n");
				return -1;
			}
			//DBG_IGMP_MLD_PRK(IGMP_RECEIVE, "receive receive report %d ,in_vif =%d,igmp_up_if_idx[%d] =%d\n",IS_IGMP_REPORT(type),in_vif,i,igmp_up_if_idx[i]);
		}
#else
		if(IS_IGMP_REPORT(type)&&in_vif==igmp_up_if_idx)
		{
			DBG_IGMP_MLD_PRK(IGMP_RECEIVE, "receive receive report from Wan side, IGNORE\n");
			return -1;
		}
#endif

		
		switch(type)
		{
			case IGMP_HOST_MEMBERSHIP_QUERY:
				rtl_handleIgmpMldQuery(&src, l3PktBuf, l3PktLen, &igmpProxyPara, &mcpq, in_vif, out_vif);
				break;
				
			case IGMP_HOST_MEMBERSHIP_REPORT:
				rtl_handleIgmpv1Join(&src, l3PktBuf, &igmpProxyPara, &mcpq, in_vif, out_vif);
				break;

			case IGMPV2_HOST_MEMBERSHIP_REPORT:
				rtl_handleIgmpv2Mldv1Join(&src, l3PktBuf, &igmpProxyPara, &mcpq, in_vif, out_vif);
				break;
				
			case IGMPV3_HOST_MEMBERSHIP_REPORT:
				rtl_handleIgmpv3Mldv2Report(&src, l3PktBuf, &igmpProxyPara, &mcpq, in_vif, out_vif);
				break;
				
			case IGMP_HOST_LEAVE_MESSAGE:
				rtl_handleIgmpMldLeave(&src, l3PktBuf, &igmpProxyPara, &mcpq, in_vif, out_vif);
				break;
			default:
				DBG_IGMP_MLD_PRK(IGMP_RECEIVE, "receive a igmp with unknown type\n");
				break;
		}
#if defined(DBG_IGMP_MLD)
		show_mfct(mcpq);
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
		show_compatibility_status(&igmpProxyPara);
#endif
#endif
	}
    return 0;
}


int main(int argc, char **argv)
{
	int _argc = 0;
#if defined(CONFIG_IGMPPROXY_MULTIWAN)|| defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
    char *_argv[12];
	int index;		//	
	char *ifStr=NULL;
	char *saveptr=NULL;
#else
	char *_argv[5];
#endif
	pid_t pid;
	int execed = 0;
	char cmdBuffer[256];//Brad add 20080605
	struct IfDesc *IfDp;
	int flags;
#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
	char UpInfCmd[128] = {0};
	char UpInf[128] = {0};
	int index_tmp = 0;
#endif	
 	int i;
	char *upstreamIfStr=NULL,*downstreamIfStr=NULL,*igmpVersonConfigStr=NULL;
#if defined(CONFIG_IGMPPROXY_MULTIWAN)|| defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
     if (argc >= 12)
#else
     if (argc >= 5) 
#endif
	{
		fprintf(stderr, "To many arguments ,argc = %d\n",argc);
		exit(1);
	}
	
	if (strcmp(argv[argc-1], "-D") == 0) {
		argc--;
		execed = 1;
	}

	while ((i = getopt(argc, argv, "c:d:u:f:")) != EOF)
	{
		switch(i)
		{
			case 'd':
				downstreamIfStr= strdup(optarg);
				break;
			case 'u':
				upstreamIfStr= strdup(optarg);
				break;
			case 'c':
				igmpVersonConfigStr = strdup(optarg);
				break;
#ifdef CONFIG_USER_IGMPPROXY_SUPPORT_CFG_FILE
			case 'f':
				cfg_path=strdup(optarg);
				break;
#endif
			default:
				printf("Error! igmpproxy not support this argument! -%c\n ",i);
				break;
		}
	}
	

	if (!execed) {
		if ((pid = vfork()) < 0) {
			fprintf(stderr, "vfork failed\n");
			exit(1);
		} else if (pid != 0) {
			exit(0);
		}
		
		for (_argc=0; _argc < argc; _argc++ )
			_argv[_argc] = argv[_argc];
		_argv[0] = runPath;
		_argv[argc++] = "-D";
		_argv[argc++] = NULL;
		execv(_argv[0], _argv);
		/* Not reached */
		fprintf(stderr, "Couldn't exec\n");
		_exit(1);

	} else {
		setsid();
	}

	memset(igmp_up_if_name,0,sizeof(igmp_up_if_name));

#ifdef CONFIG_IGMPPROXY_MULTIWAN
	igmp_up_if_num = 0;
	index =0;

	ifStr =  strtok_r(upstreamIfStr,",\0", &saveptr);
	while(ifStr !=NULL)
	{
		strncpy(igmp_up_if_name[index], ifStr,sizeof(igmp_up_if_name[index]));
		//printf("igmp_up_if_name is %s \n",igmp_up_if_name[index]);
		index++;
		igmp_up_if_num++;
		ifStr = strtok_r(NULL,",",&saveptr);
		
	}
#else
	strncpy(igmp_up_if_name, upstreamIfStr,sizeof(igmp_up_if_name));
#endif

	strncpy(igmp_down_if_name, downstreamIfStr,sizeof(igmp_down_if_name));
	//printf("igmp_up_if_num =%d,igmp_down_if_name is %s\n",igmp_up_if_num,igmp_down_if_name);
	

#if defined(CONFIG_IGMPPROXY_MULTIWAN)|| defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
	for(index=0, index_tmp=0;index<MAXWAN;index++)
	{
		if(strcmp(igmp_up_if_name[index], "") !=0)
		{
			if(index_tmp==0)
			{
				//first wan interface
 				strncpy(UpInf, igmp_up_if_name[index],IFNAMSIZ);
				index_tmp++;
			}
			else
			{
				strncat(UpInf, ",",1);
				strncat(UpInf, igmp_up_if_name[index],IFNAMSIZ);
			}
		}
	}
	if(strcmp(UpInf, ""))
	{
		sprintf(UpInfCmd, "echo %s > /var/igmp_up", UpInf);
		system(UpInfCmd);
	}
#else
	for(index=0;index<igmp_up_if_num;index++)
	{
		memset(cmdBuffer, '\0',sizeof(cmdBuffer));
		sprintf(cmdBuffer, "echo %s > /var/igmp_up", igmp_up_if_name[index]);
		system(cmdBuffer);
	}
#endif

	/*proc/igmp_proxy_wan_dev not used yet*/
#else

	//Brad add 20080605			
	memset(cmdBuffer, '\0',sizeof(cmdBuffer));
	sprintf(cmdBuffer, "echo %s > /var/igmp_up", igmp_up_if_name);
	system(cmdBuffer);

	//jwj add 20140709
	memset(cmdBuffer, '\0',sizeof(cmdBuffer));
	sprintf(cmdBuffer, "echo %s > /proc/igmp_proxy_wan_dev", igmp_up_if_name);
	system(cmdBuffer);
#endif

	if(upstreamIfStr)
		free(upstreamIfStr);
	if(downstreamIfStr)
		free(downstreamIfStr);
	if(igmpVersonConfigStr)
		free(igmpVersonConfigStr);

	
	write_pid();
/*
	#if defined(CONFIG_IGMPV3_SUPPORT)
	system("echo 3 > /proc/br_igmpVersion");
	#else
	system("echo 2 > /proc/br_igmpVersion");
	#endif
*/
	/*here to set igmp proxy daemon pid to kernel*/
	{
		int br_socket_fd = -1;
		unsigned long arg[3];

		arg[0] = BRCTL_SET_IGMPPROXY_PID;	//shoulde be BRCTL_SET_MESH_PATHSELPID
		arg[1] = getpid();
		arg[2] = 0;

		if ((br_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("create socket to connect bridge fail! \n");
			
		}
		else 
		{
			ioctl(br_socket_fd, SIOCGIFBR, arg);
		}
		close(br_socket_fd);
	}

	init_igmpProxyPara(&igmpProxyPara);

/*install signal begin*/	
	signal(SIGHUP, hup);
	signal(SIGTERM, hup);
	signal(SIGSEGV, sigsegv);
	signal(SIGUSR1, sigifup);
	signal(SIGUSR2, callback_usr2);
	
	igmpConfig.sa_flags=SA_SIGINFO;	
	igmpConfig.sa_sigaction=sigIgmpConfig;
  
	if(sigaction(sigcofigno, &igmpConfig, NULL)<0)  
	{  
		printf("signal is installed unsuccessfully\n");  
	} 
	else
	{	
		printf("signal is installed successfully\n");
	}

/*install signal end*/
	while (!initMRouter())
	{
		// Kaohj, polling every 2 seconds
		sleep(2);
	}

	init_igmp();
#ifdef	IGMP_MCAST2UNI	
	Update_igmpProxyStateToKernel(1);
#endif

	//hyking:recv the sock for avoid dst cache refcnt issue.
	//2010-8-3
	IfDp = getIfByName(igmp_down_if_name);
	if(IfDp && IfDp->sock > 0)
	{
		add_fd(IfDp->sock);
		flags = fcntl(IfDp->sock, F_GETFL);
		if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
		   	printf("Couldn't set %s to nonblock\n",igmp_down_if_name);

	}

#if defined(CONFIG_IGMPPROXY_MULTIWAN)|| defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN) 
	
	for(index=0, IfDp=IfDescVc; index<MAX_IF; index++, IfDp++)
	{
		if(strcmp(IfDp->Name, "")!=0 && strcmp(IfDp->Name, igmp_down_if_name)!=0 && IfDp->sock > 0)
		{
			add_fd(IfDp->sock);
			flags = fcntl(IfDp->sock, F_GETFL);
			if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
			   	printf("Couldn't set sock of %s to nonblock\n",IfDp->Name);
		}
	}
#else
	for(index=0;index<igmp_up_if_num;index++)
	{
		IfDp = getIfByName(igmp_up_if_name[index]);
		if(IfDp && IfDp->sock > 0)
		{
			add_fd(IfDp->sock);
			flags = fcntl(IfDp->sock, F_GETFL);
			if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
			   	printf("Couldn't set sock of %s to nonblock\n",igmp_up_if_name[index]);
		}
	}
#endif
#else
	IfDp = getIfByName(igmp_up_if_name);
	if(IfDp && IfDp->sock > 0)
	{
		add_fd(IfDp->sock);
		flags = fcntl(IfDp->sock, F_GETFL);
		if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
		   	printf("Couldn't set sock of %s to nonblock\n",igmp_up_if_name);
	}
#endif
	
	if(MRouterFD>0)
	{
		add_fd(MRouterFD);
		flags = fcntl(MRouterFD, F_GETFL);
		if (flags == -1 || fcntl(MRouterFD, F_SETFL, flags | O_NONBLOCK) == -1)
		   	printf("Couldn't set MRouterFD to nonblock\n");
	}



	//if (setsockopt(MRouterFD, IPPROTO_IP, IP_PKTINFO_RTK, &on,sizeof(on)) < 0){
	//	printf( "Couldn't enable IP_PKTINFO_RTK\n");
	//}

	//printf( "-------------- enable IP_PKTINFO_RTK,on =%d-------------\n",on);

#if defined(USE_STATIC_ENTRY_BUFFER)
	init_static_entry_tbl();
#endif
	
	send_setup_general_query(&igmpProxyPara);

	DISPLAY_BANNER;

	/* process loop */
	while(1)
	{
		fd_set in;
		struct timeval tv;
		int ret;
		int recvlen;

#ifdef CONFIG_IGMPV3_SUPPORT
		igmpv3_timer();
#endif
		calltimeout();		

		tv.tv_sec = 0;
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)
		tv.tv_usec = 100000/igmpProxyPara.timeConfig.intervalScale;
#else
		tv.tv_usec = 100000;
#endif

#ifdef CONFIG_ISP_IGMPPROXY_MULTIWAN
	//update wan interface info, block signal
		if(get_wan_signo)
		{
			buildIfVc();

			//sync vif
			for(index=0, IfDp=IfDescVc; index<MAX_IF; index ++, IfDp++)
			{
				if(strcmp(IfDp->Name, igmp_down_if_name) ==0)
					continue;
				
				addVIF(IfDp);
				if(strcmp(IfDp->Name, "")!=0 && IfDp->sock <= 0)
				{
					igmp_inf_create(IfDp);

					add_fd(IfDp->sock);

					flags = fcntl(IfDp->sock, F_GETFL);
					if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
				   		printf("Couldn't set sock of %s to nonblock\n",IfDp->Name);
				}
				else if(igmp_if_sock_old[index] >0 && strcmp(IfDp->Name, "")==0 && FD_ISSET(igmp_if_sock_old[index], &in_fds))
					remove_fd(igmp_if_sock_old[index]);
			}

			if(need_del_vif)
			{
				// only dhcp eth1.x disconnect need to del vif 2x-1,
				
				for(index=1; index<MAX_IF; index++)
				{
					for(index_tmp=0, IfDp=IfDescVc; index_tmp<MAX_IF; index_tmp ++, IfDp++)
					{
						if(strcmp(IfDp->Name, "")!=0 && (IfDp->vif_idx == 2*index-1))
							break;
					}
					if(index_tmp == MAX_IF)
					{
						delVIf(2*index-1);
					}
				}
			}
			
		}
		get_wan_signo = 0;
		need_del_vif = 0;
#endif		
		in = in_fds;
		
		ret = select(max_in_fd+1, &in, NULL, NULL, &tv);

		if( ret <= 0 ){
			//printf("igmp: timeout\n");
			continue;
		} 	

		if(FD_ISSET(MRouterFD, &in_fds)) {
						
		    recvlen = recvmsg(MRouterFD, &rcvmh, 0);
			if (recvlen > 0) 
			{
				IfDp = getIfByName(igmp_down_if_name);
				if (IfDp && IGMP_rx_enable)
				{
					
					igmpv3_accept(recvlen, IfDp);
					
				}
			}
			else
			{
		    	if (errno != EINTR && errno !=EAGAIN) 
					igmpproxy_log(LOG_ERR, errno, "recvfrom multicast route sock");
		    			
			}
			
	     }

		//hyking:recv the sock for avoid dst cache refcnt issue.
		//2010-8-3
		IfDp =  getIfByName(igmp_down_if_name);
		if(IfDp && FD_ISSET(IfDp->sock, &in_fds))
		{
		
			recvlen = recvmsg(IfDp->sock, &rcvmh, 0);
			
			if (recvlen < 0) 
			{
		    	if (errno != EINTR && errno !=EAGAIN) 
					igmpproxy_log(LOG_ERR, errno, "recvfrom down interface");
			}
		}
#if defined(CONFIG_IGMPPROXY_MULTIWAN) || defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
#if defined(CONFIG_ISP_IGMPPROXY_MULTIWAN)
		for(index=0, IfDp=IfDescVc; index<MAX_IF; index++, IfDp++)
		{		
			if(strcmp(IfDp->Name, "")!=0 && FD_ISSET(IfDp->sock, &in_fds))
			{
				recvlen = recvmsg(IfDp->sock, &rcvmh, 0);
				if (recvlen < 0) 
				{
			    		if (errno != EINTR && errno !=EAGAIN) igmpproxy_log(LOG_ERR, errno, "recvfrom up interface");
				}
			}
		}
#else
		for(index=0;index<igmp_up_if_num;index++)
		{
			IfDp =	getIfByName(igmp_up_if_name[index]);		
			if(IfDp && FD_ISSET(IfDp->sock, &in_fds))
			{
				recvlen = recvmsg(IfDp->sock, &rcvmh, 0);
				if (recvlen < 0) 
				{
						if (errno != EINTR && errno !=EAGAIN) igmpproxy_log(LOG_ERR, errno, "recvfrom up interface");
				}
			}
		}
#endif
#else
		IfDp =  getIfByName(igmp_up_if_name);		
		if(IfDp && FD_ISSET(IfDp->sock, &in_fds))
		{
			recvlen = recvmsg(IfDp->sock, &rcvmh, 0);
			if (recvlen < 0) 
			{
			    if (errno != EINTR && errno !=EAGAIN) igmpproxy_log(LOG_ERR, errno, "recvfrom up interface");
			}
		}
		
#endif
	}
	return 0;
}

