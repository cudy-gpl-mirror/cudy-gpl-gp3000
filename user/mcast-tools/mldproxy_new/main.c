#include "mclab.h"
#include "timeout.h"
#include "mldproxy.h"
#include "igmp_mld_utility.h"
#include "igmp_mld_debug.h"
#include "igmp_mld_config.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include "built_time"
#include "mld.h"
#if defined (CONFIG_ISP_MLDPROXY_MULTIWAN)
#include <sys/stat.h>
#endif

char *mld6_recv_buf;		/* input packet buffer */
char *mld6_send_buf;		/* output packet buffer */
char *send_buf;

struct msghdr 		sndmh, rcvmh;
struct sockaddr_in6 	from;
struct sockaddr_in6 	to;
struct iovec 		rcviov[2];
struct iovec 		sndiov[2];
int rcvcmsglen, sndcmsglen;
static u_char   		*rcvcmsgbuf = NULL;
char *sndcmsgbuf = NULL;
int ctlbuflen = 0;

#define RECV_BUF_SIZE	2048

fd_set in_fds;		/* set of fds that wait_input waits for */
int max_in_fd;		/* highest fd set in in_fds */
int MLD_rx_enable=0;


#ifndef USE_RFC2292BIS
#define IP6OPT_RTALERT_LEN 4
u_int8_t raopt[IP6OPT_RTALERT_LEN];
#endif 
#define IP6OPT_RTALERT_MLD      0
#define IP6OPT_RTALERT          0x05
#ifndef IP6OPT_ROUTER_ALERT	/* XXX to be compatible older systems */
#define IP6OPT_ROUTER_ALERT IP6OPT_RTALERT
#endif
static u_int16_t rtalert_code;
char mld_down_if_name[IFNAMSIZ]="br0";
char mld_down_if_idx;

#if defined(CONFIG_MLDPROXY_MULTIWAN)||defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
char mld_up_if_name[MAXWAN][IFNAMSIZ];
#else
char mld_up_if_name[IFNAMSIZ]="eth0";
#endif

#if !defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
#if defined(CONFIG_MLDPROXY_MULTIWAN)
char mld_up_if_idx[MAXWAN];
#else
char mld_up_if_idx;
#endif
#endif

int mld_up_if_num=0;
int mld_if_num=0;
char* runPath = "/bin/mldproxy";
char* pidfile = "/var/run/mld_pid";

#ifdef CONFIG_RTK_IPV6_LOGO_TEST
int logoTestMode=0;
#endif

struct mcft_entry *mcpq = NULL;

#define CONFIGSIG 34   //SIGRTMIN+2
struct sigaction mldConfig;
int sigcofigno=CONFIGSIG;

unsigned int mldLogBitMap =0;

#if defined(CONFIG_LUNA) && defined(CONFIG_MLDPROXY_MULTIWAN)
typedef struct mld_lanwanBinding
{
	int lanIfindex;
	int wanIfindex[MAXWAN+1];
	int binding_wan_num;
}MLD_LANWANBINDING_T,*MLD_LANWANBINDING_Tp;

static MLD_LANWANBINDING_T	mld_lanwanBinding;

#define IPV6_RECVPKTINFO_RTK	100
#define IPV6_PKTINFO_RTK		101

struct in6_pktinfo_rtk_mld {
	struct in6_addr	ipi6_addr;
	int		ipi6_ifindex;
	int 	from_dev_ifindex;
	int 	from_dev_ifindex_valid;
};

void mld_set_lanwanBindingInfo(int from_dev_ifindex,const char *ifName)
{
	struct IfDesc *ifUp = getIfByName(ifName);

	if(!ifUp){
		log(LOG_WARNING, 0,"Cannot get IfDesc,ifName=%s\n",ifName);
		return;
	}
	
	mld_lanwanBinding.lanIfindex = from_dev_ifindex;
	mld_lanwanBinding.wanIfindex[mld_lanwanBinding.binding_wan_num] = ifUp->vif_idx;
	mld_lanwanBinding.binding_wan_num++;
	return;
}


int mld_get_lanwanBindingInfo(int from_dev_ifindex)
{
	return mld_lanwanBinding.wanIfindex[0];
}

#endif

void
write_pid()
{
	FILE *fp = fopen(pidfile, "w+");
	if (fp) {
		//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
		fprintf(fp, "%d\n", getpid());
		fclose(fp);
	}
	else
	 	log(LOG_WARNING, 0,"Cannot create pid file\n");
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
	 	log(LOG_WARNING, 0,"Cannot create pid file\n");
}

int init_mldProxyPara(struct mCastProxyConfig *mCastProxyPara)
{
	/* assign default setting */
	mCastProxyPara->ipVersion= IP_VERSION6;
	mCastProxyPara->timeConfig.robustness = ROBUSTNESS_VARIABLE;
	mCastProxyPara->timeConfig.memberQueryInterval = MEMBER_QUERY_INTERVAL;
	mCastProxyPara->timeConfig.memberQueryCount = MEMBER_QUERY_COUNT;
	mCastProxyPara->timeConfig.lastMemberInterval = LAST_MEMBER_QUERY_INTERVAL;
	mCastProxyPara->timeConfig.lastMemberCount = LAST_MEMBER_QUERY_COUNT;
	mCastProxyPara->timeConfig.setUpGeneralQueryInterval = STARTUP_GENERAL_QUERY_INTERVAL;
	mCastProxyPara->timeConfig.setUpGeneralQueryCount = STARTUP_GENERAL_QUERY_COUNT;
	mCastProxyPara->timeConfig.periodGeneralQueryInterval = PERIODICAL_GENERAL_QUERY_INTERVAL;
	mCastProxyPara->timeConfig.periodGeneralQueryCount = PERIODICAL_GENERAL_QUERY_COUNT;
	mCastProxyPara->timeConfig.QueryResponseInterval = GENERAL_QUERY_RESPONSE_INTERVAL;
	mCastProxyPara->timeConfig.linkChangeQueryInterval = LINK_CHANGE_QUERY_INTERVAL;
	mCastProxyPara->timeConfig.linkChangeQueryCount = LINK_CHANGE_QUERY_TIMES;
	mCastProxyPara->timeConfig.intervalScale = INTERVAL_SCALE;//mrd in general query=QueryResponseInterval*intervalScale
	mCastProxyPara->timeConfig.oldVersionHostPresentTimeout = OLD_VERSION_HOST_PRESENT_TIMEOUT;
	mCastProxyPara->forceVersion= MLD_VER_2;
	mCastProxyPara->fastleaveEnable = 1;
	mCastProxyPara->compatibleEnbale = 0;

	config_proxy_para_from_file(&mldProxyPara);

	return 0;
}

/*signal function start*/
void hup(int signum)
{
	(void)signum;

	log( LOG_DEBUG, 0, "clean handler called" );
	disableMRouter();
#if defined (CONFIG_USER_MLDPROXY_HANDLE_MCFLOW)
	del_all_mfc(&mldProxyPara, &mcpq);
	if(isMLDSnoopingEnabled()) //clear both IPv4/IPv6 Mcast flow in snooping module
		//system("/bin/echo DEVIFNAME br0 BR_FLUSH_MDB 1 > /proc/igmp/ctrl/brConfig");
		va_cmd("/bin/sh", 2, 1, "-c", "echo DEVIFNAME br0 BR_FLUSH_MDB 1 > /proc/igmp/ctrl/brConfig");

#endif
	unlink(pidfile);
	exit(EXIT_SUCCESS);

}

#if defined(CONFIG_USER_MLDPROXY_HANDLE_MCFLOW)
void clearMcFlowV6()
{
	printf("=====> %s\n", __FUNCTION__);
	struct mcft_entry *mcp = mcpq;
	while(mcp)
	{
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
		if (mcp->ip_version == IP_VERSION6)
			rtk_fc_ipv6_multicast_flow_delete(mcp->grp_addr);
#endif
		mcp = mcp->next;
	}
}

void updateMcFlowV6(void)
{
	if(isMLDSnoopingEnabled())
	{
		clearMcFlowV6();
		send_setup_general_query(&mldProxyPara);
	}
	else
	{
		printf("=====> %s\n", __FUNCTION__);

		struct mcft_entry *mcp = mcpq;
		while(mcp)
		{
#if defined(CONFIG_COMMON_RT_API) && defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
			if (mcp->ip_version == IP_VERSION6)
				rtk_fc_ipv6_multicast_flow_add(mcp->grp_addr);
#endif
			mcp = mcp->next;
		}
	}
}


void sig_doUpdate(int signum)
{
	(void)signum;
	log( LOG_DEBUG, 0, "sig_doUpdate handler called" );
	updateMcFlowV6();
}
#endif

// Comes here because upstream or downstream interface ip changed
// Usually, it is used by dynamic interface to sync its interface with
// the mldproxy local database.
void sigifup(int signum)
{
	(void)signum;
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN) || defined(CONFIG_MLDPROXY_MULTIWAN)
    int index;
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	struct IfDesc *Dp=NULL;
#endif
#endif
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	struct ifreq IfVc[ MAX_IF  ];
	struct ifreq *IfEp, *IfPt;
	struct ifconf IoCtlReq;
	int ifindex;
	int Sock;

	//syslog(LOG_INFO, "mldproxy: SIGUSR1 caught\n");
	log(LOG_WARNING, 0,"mldproxy: SIGUSR1 caught, [%s]:[%d].\n",__FUNCTION__,__LINE__);
	// get information of all the interfaces
	if( (Sock = socket( AF_INET6, SOCK_DGRAM, 0 )) < 0 ){
		printf("RAW socket open![%s]:[%d].\n",__FUNCTION__,__LINE__);
		log( LOG_ERR, errno, "RAW socket open" );
		return errno;
	}
	IoCtlReq.ifc_buf = (void *)IfVc;
	IoCtlReq.ifc_len = sizeof( IfVc );
	//ioctl»ñÈ¡interfaceµÄÐÅÏ¢ipv6?Ö»ÊÊÓÃÓÚipv4,ipv6¿ÉÄÜ²»ÄÜ»ñÈ¡
	if( ioctl( Sock, SIOCGIFCONF, &IoCtlReq ) < 0 ){
		printf("ioctl SIOCGIFCONF Fail![%s]:[%d].\n",__FUNCTION__,__LINE__);
		//coverity:negative_returns
		close(Sock);
		log( LOG_ERR, errno, "ioctl SIOCGIFCONF" );
		return errno;
	}
	close( Sock );
	IfEp = (void *)((char *)IfVc + IoCtlReq.ifc_len);

	for( IfPt = IfVc; IfPt < IfEp; IfPt++ ) 
	{

		for(index=0, Dp=IfDescVc; index<MAX_IF; index++, Dp++)
		{
			if(strcmp(Dp->Name, IfPt->ifr_name)==0)
			{
				//if ifindex will be changed???
				Dp->pif_idx = (int)IfPt->ifr_ifindex;
				ifindex=(int)IfPt->ifr_ifindex;	//init when build ifvc
				//printf("update upstream ip to %s\n", inet_ntoa(Dup->InAdr));
				// Update default multicast interface for this socket.
				if(setsockopt(Dp->sock, IPPROTO_IPV6 , IPV6_MULTICAST_IF, 
					(void*)&ifindex, sizeof(int))<0)
				{
					//fix coverity: should check_return;
					log(LOG_WARNING, errno,"setsockopt Dup->sock Failed!![%s]:[%d].\n",__FUNCTION__,__LINE__);
				}
			}

		}
	}
#else	// CONFIG_ISP_MLDPROXY_MULTIWAN
	struct IfDesc *IfDp=NULL;
	int flags;

	/* Step 1: update interface information */
	if(!buildIfVc()) 
		return;

	/* Step 2: update VIF information */
#if defined(CONFIG_MLDPROXY_MULTIWAN)
	IfDp = getIfByName(mld_down_if_name);
	if ((IfDp == NULL) || (IfDp->pif_idx == 0))
		return;

	mld_down_if_idx = addVIF(IfDp);
	if((IfDp->vif_idx != NO_VIF) && IfDp->sock <= 0){
		//new interface
		mld_inf_create(IfDp);

		add_fd(IfDp->sock);
		flags = fcntl(IfDp->sock, F_GETFL);
		if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
			log(LOG_WARNING, 0,"Couldn't set sock of %s to nonblock\n",IfDp->Name);
	}

	for(index = 0;index<mld_up_if_num;index++)
	{
		IfDp = getIfByName(mld_up_if_name[index]);

		if ((IfDp == NULL) || (IfDp->pif_idx == 0))
			continue;  

		mld_up_if_idx[index] = addVIF(IfDp);
		if((IfDp->vif_idx != NO_VIF) && IfDp->sock <= 0){
			//new interface
			mld_inf_create(IfDp);

			add_fd(IfDp->sock);
			flags = fcntl(IfDp->sock, F_GETFL);
			if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
				log(LOG_WARNING, 0,"Couldn't set sock of %s to nonblock\n",IfDp->Name);

			/* TODO: sync exist mfc to new vif */
		}
	}
#else	// CONFIG_MLDPROXY_MULTIWAN
	IfDp = getIfByName(mld_down_if_name);
	if ((IfDp == NULL) || (IfDp->pif_idx == 0))
		return;

	mld_down_if_idx = addVIF(IfDp);
	if((IfDp->vif_idx != NO_VIF) && IfDp->sock <= 0){
		//new interface
		mld_inf_create(IfDp);

		add_fd(IfDp->sock);
		flags = fcntl(IfDp->sock, F_GETFL);
		if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
			log(LOG_WARNING, 0,"Couldn't set sock of %s to nonblock\n",IfDp->Name);
	}

	IfDp = getIfByName(mld_up_if_name);
	if ((IfDp == NULL) || (IfDp->pif_idx == 0))
		return;

	mld_up_if_idx = addVIF(IfDp);

	/* following should not be used, because initMRoute() will wait upstream interface ready */
	if((IfDp->vif_idx != NO_VIF) && IfDp->sock <= 0){
		//new interface
		mld_inf_create(IfDp);

		add_fd(IfDp->sock);
		flags = fcntl(IfDp->sock, F_GETFL);
		if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
			log(LOG_WARNING, 0,"Couldn't set sock of %s to nonblock\n",IfDp->Name);

		/* TODO: sync exist mfc to new vif */
	}
#endif	// CONFIG_MLDPROXY_MULTIWAN
	send_setup_general_query(&mldProxyPara);
#endif	// CONFIG_ISP_MLDPROXY_MULTIWAN
}
void segv(void)
{
	log(LOG_ERR, 0 ,"get SIGSEGV [%s]:[%d].\n",__FUNCTION__,__LINE__);
	//exit(-1);
}
void callback_usr2()
{	
	set_link_change_query(&mldProxyPara);	
}

void sigmldConfig(int signum, siginfo_t * info)
{
	int para = info->si_int;
	config_proxy_para_from_file(&mldProxyPara);
}

#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
int isFileExist(char *file_name)
{
	struct stat status;

	if ( stat(file_name, &status) < 0)
		return 0;

	return 1;
}
int updateMldWanUp(void)
{
	FILE *fp = NULL;
	char buf[MAXWAN*(IFNAMSIZ+1)] = {0};
	char *token = NULL, *saveStr = NULL;
	int i;
	
	if(!isFileExist(MLDPROXY_UP))
	{
		goto err_return;
	}

	fp = fopen(MLDPROXY_UP, "r");
	if(!fp)
		goto err_return;

	memset(buf, 0, sizeof(buf));
	fgets(buf, sizeof(buf), fp);

	saveStr = buf;

	token = strsep(&saveStr,",");
	
	memset(mld_up_if_name, 0, MAXWAN*IFNAMSIZ);
	mld_up_if_num = 0;
	
	while(token)
	{
		strncpy(mld_up_if_name[mld_up_if_num], token,sizeof(mld_up_if_name[mld_up_if_num]));
		mld_up_if_num++;
		token = strsep(&saveStr,",");
	}
	mld_if_num = mld_up_if_num+1;
#if 0
	for(i=0;i<MAXWAN;i++)
	{
		if(strcmp(mld_up_if_name[i], "")!=0)
		{
			printf("[%s %d] %s!\n", __FUNCTION__, __LINE__, mld_up_if_name[i]);
		}
	}
#endif
	fclose(fp);
	return 0;
	
err_return:
	log(LOG_WARNING, 0,"[%s %d] update mld wan interface fail!\n", __FUNCTION__, __LINE__);
	return -1;

}

void sigMldWanUp(int signum, siginfo_t * info)
{	

	int WanIfFlag;
	log(LOG_DEBUG, 0,"signum = %d, parameter = %x, [%s:%d]\n", signum, info->si_int, __FUNCTION__, __LINE__);
	WanIfFlag = info->si_int;
	if(WanIfFlag&(ADD_WAN_IF|DEL_WAN_IF))
		mld_update_wan = 1;

	updateMldWanUp();
}
#endif

void init_signals(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGHUP);
    sigaddset(&sa.sa_mask, SIGUSR1);
	sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGUSR2);
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	sigaddset(&sa.sa_mask, mld_signo);
#endif
	sigaddset(&sa.sa_mask, sigcofigno);


    sa.sa_handler = hup;
    sigaction(SIGTERM, &sa, NULL);
#if defined (CONFIG_USER_MLDPROXY_HANDLE_MCFLOW)
	sa.sa_handler = sig_doUpdate;
#else
	sa.sa_handler = hup;
#endif
    sigaction(SIGHUP, &sa, NULL);

	sa.sa_handler = sigifup;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = segv;
    sigaction(SIGSEGV, &sa, NULL);
	
    sa.sa_handler = callback_usr2;
    sigaction(SIGUSR2, &sa, NULL);
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	sa.sa_flags =SA_SIGINFO;	
	sa.sa_handler = sigMldWanUp;
	sigaction(mld_signo, &sa, NULL);
#endif
	sa.sa_flags =SA_SIGINFO;	
	sa.sa_handler = sigmldConfig;
	sigaction(sigcofigno, &sa, NULL);
}
/*signal function end*/
static int initMRouter(void)
/*
** Inits the necessary resources for MRouter.
** return 0: fail
**          1: success
**
*/
{
	int Err;
	int i;
	struct IfDesc *Ddp, *Dup;
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	struct IfDesc *Dp = NULL;
	int mld_up_if_idx_tmp = 0;
#endif
	int proxyUp = 0;
	// get information of interface  
	if(!buildIfVc()) 
		return 0;

// check information of interface 
#if defined(CONFIG_MLDPROXY_MULTIWAN) && defined(CONFIG_RTK_DEV_AP)

	for(i=0;i<mld_up_if_num;i++)
	{
		//printf("mld_up_if_name[i] is %s\n",mld_up_if_name[i]);
		if(getIfByName(mld_up_if_name[i]) == NULL)
			return 0;
	}	
#endif

	rtalert_code = htons(IP6OPT_RTALERT_MLD);
    if (!mld6_recv_buf && (mld6_recv_buf = malloc(RECV_BUF_SIZE)) == NULL)
	    log(LOG_ERR, 0, "malloc failed");
    if (!mld6_send_buf && (mld6_send_buf = malloc(RECV_BUF_SIZE)) == NULL)
	    log(LOG_ERR, 0, "malloc failed");

    rcvcmsglen = CMSG_SPACE(sizeof(struct in6_pktinfo)) +
	    CMSG_SPACE(sizeof(int));
    if (rcvcmsgbuf == NULL && (rcvcmsgbuf = malloc(rcvcmsglen)) == NULL)
	    log(LOG_ERR, 0,"malloc failed");
    
    
	switch( Err = enableMRouter() ) {
		case 0: break;
		case EADDRINUSE: log( LOG_ERR, EADDRINUSE, "MC-Router API already in use" ); break;
		default: log( LOG_ERR, Err, "MRT_INIT failed" );
	}

	//set mld_socket option
	{
	int on=1;
	int ret;
	
	ret = setsockopt(MLD_Socket, IPPROTO_IPV6 , IPV6_MULTICAST_HOPS, 
		(void*)&on, sizeof(on));
	if(ret)
		log(LOG_ERR, errno,"setsockopt IPV6_MULTICAST_HOPS error!\n");
	on=1;
#ifdef IPV6_RECVPKTINFO
    if (setsockopt(MLD_Socket, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on,
		   sizeof(on)) < 0)
		log(LOG_ERR, errno, "setsockopt(IPV6_RECVPKTINFO)");
#else  /* old adv. API */
    if (setsockopt(MLD_Socket, IPPROTO_IPV6, IPV6_PKTINFO, &on,
		   sizeof(on)) < 0)
		log(LOG_ERR, errno, "setsockopt(IPV6_PKTINFO)");
#endif 
#if defined(CONFIG_LUNA) && defined(CONFIG_MLDPROXY_MULTIWAN)
	/* IPV6_PKTINFO_RTK */
	on=1;
    if (setsockopt(MLD_Socket, IPPROTO_IPV6, IPV6_RECVPKTINFO_RTK, &on,
		   sizeof(on)) < 0)
		log(LOG_ERR, errno, "setsockopt(IPV6_RECVPKTINFO)");
#endif
	 on = 1;
    /* specify to tell value of hoplimit field of received IP6 hdr */
#ifdef IPV6_RECVHOPLIMIT
    if (setsockopt(MLD_Socket, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on,
		   sizeof(on)) < 0)
		log(LOG_ERR, errno, "setsockopt(IPV6_RECVHOPLIMIT)");
#else  /* old adv. API */
    if (setsockopt(MLD_Socket, IPPROTO_IPV6, IPV6_HOPLIMIT, &on,
		   sizeof(on)) < 0)
		log(LOG_ERR, errno, "setsockopt(IPV6_HOPLIMIT)");
#endif 
	    /* initialize msghdr for receiving packets */
    rcviov[0].iov_base = (caddr_t) mld6_recv_buf;
    rcviov[0].iov_len = RECV_BUF_SIZE;
    rcvmh.msg_name = (caddr_t) & from;
    rcvmh.msg_namelen = sizeof(from);
    rcvmh.msg_iov = rcviov;
    rcvmh.msg_iovlen = 1;
    rcvmh.msg_control = (caddr_t) rcvcmsgbuf;
    rcvmh.msg_controllen = rcvcmsglen;

    /* initialize msghdr for sending packets */
    sndiov[0].iov_base = (caddr_t)mld6_send_buf;
	sndmh.msg_name = (caddr_t) & to;
    sndmh.msg_namelen =sizeof(to);
    sndmh.msg_iov = sndiov;
    sndmh.msg_iovlen = 1;
    /* specifiy to insert router alert option in a hop-by-hop opt hdr. */
#ifndef USE_RFC2292BIS
    raopt[0] = IP6OPT_ROUTER_ALERT;
    raopt[1] = IP6OPT_RTALERT_LEN - 2;
    memcpy(&raopt[2], (caddr_t) & rtalert_code, sizeof(u_int16_t));
#endif 
	
	/* In linux use IP_PKTINFO */
	//IP_RECVIF returns the interface of received datagram
	
	}

#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	mld_down_if_idx = -1;
	for(i=0, Dp=IfDescVc; i<MAX_IF; i++, Dp++)
	{
		if(strcmp(Dp->Name, mld_down_if_name)==0)
		{
			mld_down_if_idx = addVIF(Dp);
			
		}
		else if(strcmp(Dp->Name, "")!=0)
		{
			//wan interface
			mld_up_if_idx_tmp = addVIF(Dp);
			if(mld_up_if_idx_tmp == -1)
				break;
		}
	}
	if((i!=MAX_IF) ||(mld_down_if_idx==-1))
	{
		return 0;
	}
	else
	{
		proxyUp = 1;
	}
	
#elif defined(CONFIG_MLDPROXY_MULTIWAN)
    Ddp = getIfByName(mld_down_if_name);
	
	if ((Ddp==NULL) || (Ddp->pif_idx == 0))
		return 0;
	
	/* add downstream interface */
	mld_down_if_idx = addVIF(Ddp);

	/* add upstream interface */
	for(i = 0;i<mld_up_if_num;i++)
	{
		Dup = getIfByName(mld_up_if_name[i]);
		if ((Dup == NULL) || (Dup->pif_idx == 0))
		   continue;  
		else
			proxyUp = 1;
		mld_up_if_idx[i] = addVIF(Dup);
	 }

	if(proxyUp == 0)
	{		
		return 0;
	}
#else
	Ddp = getIfByName(mld_down_if_name);
	Dup = getIfByName(mld_up_if_name);
	if ((Ddp==NULL) || (Ddp->pif_idx == 0))
		return 0;
	if ((Dup==NULL) || (Dup->pif_idx == 0))
		return 0;
	/*
	printf("add down VIF: Interface %s ,pif_idx:%d, Flags: 0x%04x",
		   Ddp->Name,
		   Ddp->pif_idx,
		   Ddp->Flags );
	printf( "addr:%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
	Ddp->InAdr.s6_addr16[ 0],Ddp->InAdr.s6_addr16[ 1],Ddp->InAdr.s6_addr16[ 2],
	Ddp->InAdr.s6_addr16[ 3],Ddp->InAdr.s6_addr16[ 4],Ddp->InAdr.s6_addr16[ 5],
	Ddp->InAdr.s6_addr16[ 6],Ddp->InAdr.s6_addr16[ 7]);
	printf("add up VIF: Interface %s ,pif_idx:%d, Flags: 0x%04x",
		   Dup->Name,
		   Dup->pif_idx,
		   Dup->Flags );
	printf( "addr:%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
	Dup->InAdr.s6_addr16[ 0],Dup->InAdr.s6_addr16[ 1],Dup->InAdr.s6_addr16[ 2],
	Dup->InAdr.s6_addr16[ 3],Dup->InAdr.s6_addr16[ 4],Dup->InAdr.s6_addr16[ 5],
	Dup->InAdr.s6_addr16[ 6],Dup->InAdr.s6_addr16[ 7]);
	*/
	/* add downstream interface */
	mld_down_if_idx = addVIF(Ddp);
	//printf("[%s]:[%d].mld_down_if_idx:%d\n",__FUNCTION__,__LINE__,mld_down_if_idx);
	/* add upstream interface */
	mld_up_if_idx = addVIF(Dup);
	//printf("[%s]:[%d].mld_up_if_idx:%d\n",__FUNCTION__,__LINE__,mld_down_if_idx);
#endif
	
	signal(SIGTERM, hup);
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
/*
	return 0: fail
	return 1: success
*/
#define ALL_ROUTERS_MLD2   "FF02::16"

int mld_inf_create(char *ifname)
{
	struct ipv6_mreq mreq;
	int i;
	int ret;
	struct IfDesc *dp;
	struct in6_addr all_mld2_routers_in6_addr;
	struct in6_pktinfo pktinfo;

	dp = getIfByName(ifname);
	if(dp==NULL)
		return 0;
#if !defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	if(dp->pif_idx == 0)
		return 0;		// ignore invalid interface
	if(dp->sock > 0)
		return 0;		// sock already create
#endif
    if ((dp->sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0) 
	{
		//fix coverity
		log(LOG_ERR, errno, "ICMPv6 socket");
		return errno;
    }
	#if 0
	{	
		/* Set router alert option*/
		char ra[4];
		ra[0] = 148;
		ra[1] = 4;
		ra[2] = 0;
		ra[3] = 0;
		setsockopt(dp->sock, IPPROTO_IPV6, IP_OPTIONS, ra, 4);//IP_OPTIONSÕâ¸öÑ¡Ïî??
	}
	#endif
	/* init mld */	
	/* Set reuseaddr, hops, loopback and set outgoing interface */
	i = 1;
	ret = setsockopt(dp->sock, SOL_SOCKET, SO_REUSEADDR, (void*)&i, sizeof(i));
	if(ret)
		log(LOG_ERR, errno, "setsockopt SO_REUSEADDR error!\n");
	i = 1;
	/*IPv6 hops*/
	ret = setsockopt(dp->sock, IPPROTO_IPV6 , IPV6_MULTICAST_HOPS, 
		(void*)&i, sizeof(i));
	if(ret)
		log(LOG_ERR, errno, "setsockopt IPV6_MULTICAST_HOPS error!\n");
	
	i = 0;
	ret = setsockopt(dp->sock, IPPROTO_IPV6 , IPV6_MULTICAST_LOOP, 
		(void*)&i, sizeof(i));
	if(ret)
		log(LOG_ERR, errno, "setsockopt IPV6_MULTICAST_LOOP error!\n");

	i = 1;
	memset(&pktinfo, 0, sizeof(struct in6_pktinfo));
	memcpy(&pktinfo, &i, sizeof(i));
	ret = setsockopt(dp->sock, IPPROTO_IPV6 , IPV6_PKTINFO, (void*)&pktinfo, sizeof(struct in6_pktinfo));
	if(ret)
		log(LOG_ERR, errno,"setsockopt IPV6_PKTINFO error!\n");
	
	int ifindex=(int)(dp->pif_idx);
	ret = setsockopt(dp->sock, IPPROTO_IPV6 , IPV6_MULTICAST_IF, 
		(void*)&ifindex, sizeof(int));
	if(ret){
		//printf("setsockopt IPV6_MULTICAST_IF error:%d!\n", errno);
		log( LOG_ERR, errno, "IPV6_MULTICAST_IF" );
	}	
	inet_pton(AF_INET6, ALL_ROUTERS_MLD2, &all_mld2_routers_in6_addr);
	memset(&mreq, 0, sizeof(mreq));
	mreq.ipv6mr_multiaddr = all_mld2_routers_in6_addr;
	mreq.ipv6mr_ifindex   = ifindex;
	setsockopt(dp->sock,IPPROTO_IPV6,IPV6_ADD_MEMBERSHIP,&mreq,sizeof(mreq));
	return 0;
}

/*
** return 0: success
** return -1: fail
*/
int init_mld(void)
{
	int val;
	send_buf = malloc(RECV_BUF_SIZE);

	FD_ZERO(&in_fds);
	max_in_fd = 0;
#if defined(CONFIG_MLDPROXY_MULTIWAN) || defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	int idx;
#endif
	mld_inf_create(mld_down_if_name);

#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	for(idx=0; idx<MAXWAN; idx++)
	{
		if(strcmp(mld_up_if_name[idx], "") !=0)
			mld_inf_create(mld_up_if_name[idx]);
	}
#elif defined(CONFIG_MLDPROXY_MULTIWAN)	
	for(idx=0;idx<mld_up_if_num;idx++)
		mld_inf_create(mld_up_if_name[idx]);

#else
	mld_inf_create(mld_up_if_name);
#endif

	init_general_query(&mldProxyPara);
	MLD_rx_enable = 1;

	return 0;
}
int mld_make_query(int version, struct in6_addr grp, unsigned short mrt, unsigned char qqic, unsigned char qrv, unsigned short srcnum, struct in6_addr *srcfilter, unsigned char *buf)
{
	int totalsize = 0, i=0;
	
	if(buf == NULL)
		return 0;

	if(srcnum && srcfilter==NULL)
		return 0;
	
	if(version==MLD_VER_2)
	{
		struct mld2_query *mldv2 = (struct mld2_query *)buf;
		memset(mldv2,0,sizeof(struct mld2_query));
		
		mldv2->mld2q_type= ICMPV6_MGM_QUERY;
		mldv2->mld2q_code= 0;
		mldv2->mld2q_mrc = htons(mrt);
		memcpy(&(mldv2->mld2q_mca), &grp, sizeof(struct in6_addr));
		mldv2->mld2q_cksum		= 0;
		mldv2->mld2q_resv1	   = 0;
		mldv2->mld2q_suppress  = 1;
		mldv2->mld2q_qrv	   = qrv;
		mldv2->mld2q_qqic	   = qqic;
		mldv2->mld2q_nsrcs	   = htons(srcnum);
		totalsize = sizeof(struct mld2_query)+ srcnum*sizeof(struct in6_addr);
		for(i=0;i<srcnum;i++)
  		{
			memcpy(&mldv2->mld2q_srcs[i],&srcfilter[i],sizeof(struct in6_addr));
    	}
	}
	else if(version==MLD_VER_1)
	{
		struct mld_msg *mldv1=(struct mld_msg *)buf;
		memset(mldv1,0,sizeof(struct mld_msg));
		mldv1->mld_type 	=ICMPV6_MGM_QUERY;
		mldv1->mld_code 	=0;//set to 1 when send and ingnore it when receive
		mldv1->mld_cksum	=0;
		mldv1->mld_maxdelay =htons(mrt);
		memcpy(&(mldv1->mld_mca), &grp, sizeof(struct in6_addr));
		totalsize = sizeof(struct mld_msg);
	}
	else
	{
		totalsize = 0;
	}

	return totalsize;
}

int mld_send_query(int version, struct in6_addr dst, struct in6_addr grp, unsigned short mrt, unsigned char qqic, unsigned char qrv, unsigned short srcnum, struct in6_addr *srcfilter)
{
	struct sockaddr_in6 dst_sa;
	int totalsize = 0;
	struct IfDesc *down_dp=getIfByName(mld_down_if_name);
	struct IfDesc *dp[mld_if_num], *dp_tmp;
	int i;

	if(down_dp==NULL)
	{
		log(LOG_WARNING, 0,"get if(%s) failed\n",mld_down_if_name);
		return 0;
	}

	totalsize = mld_make_query(version,grp,mrt,qqic,qrv,srcnum,srcfilter,mld6_send_buf);

	if(totalsize==0)
		return 0;

	dp[0]=down_dp;

#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	for(i=0,dp_tmp=IfDescVc; i<MAX_IF; i++, dp_tmp++)
#else
	for(i=0;i< mld_if_num;i++)
#endif
	{
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
		/*if readlogo is disabled, only send query to down interface.
		   else send query to all interface*/
		if((logoTestMode==0)&&(i!=0))
			break;
#endif
	#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
		if(strcmp(dp_tmp->Name, "")==0)
			continue;
	#elif defined(CONFIG_MLDPROXY_MULTIWAN)
		if(i<mld_if_num-1)
			dp[i+1]= getIfByName(mld_up_if_name[i]);
		dp_tmp = dp[i];
		if(dp_tmp == NULL)
		{
			log(LOG_WARNING, 0,"get if(%d) failed\n",i);
			return 0;
		}
	#else
		if(i==0)
			dp[i+1]= getIfByName(mld_up_if_name);
		dp_tmp = dp[i];
		if(dp_tmp == NULL)
		{
			log(LOG_WARNING, 0,"get if(%d) failed\n",i);
			return 0;
		}
	#endif
#if !defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
		// ignore invalid interface
		if(dp_tmp->pif_idx == 0)
			return 0;
		// sock already create
		if(dp_tmp->sock <= 0)
			return 0;
#endif
		
		memset(&to, 0,sizeof(struct sockaddr_in6));
		to.sin6_family= AF_INET6;
		memcpy(&(to.sin6_addr) ,&dst,sizeof(struct in6_addr));

		sndmh.msg_name = (caddr_t) & to;
		sndmh.msg_namelen =sizeof(to);

		
		int ctllen=0; 
		int hbhlen = 0;
		int alert=1;
		int ifindex=(int)dp_tmp->pif_idx;
		struct in6_addr src;
		memcpy(&src,&(dp_tmp->InAdr),sizeof(struct in6_addr));

		sndiov[0].iov_len = totalsize;

		ctllen += CMSG_SPACE(sizeof(struct in6_pktinfo));
		if (alert) 
		{
#ifdef USE_RFC2292BIS
			if ((hbhlen = inet6_opt_init(NULL, 0)) == -1)
				log(LOG_ERR, 0, "inet6_opt_init(0) failed");
			if ((hbhlen = inet6_opt_append(NULL, 0, hbhlen, IP6OPT_ROUTER_ALERT, 2,
							   2, NULL)) == -1)
				log(LOG_ERR, 0, "inet6_opt_append(0) failed");
			if ((hbhlen = inet6_opt_finish(NULL, 0, hbhlen)) == -1)
				log(LOG_ERR, 0, "inet6_opt_finish(0) failed");
			ctllen += CMSG_SPACE(hbhlen);
#else  /* old advanced API */
			hbhlen = inet6_option_space(sizeof(raopt));
			ctllen += hbhlen;
#endif
		}/*end if (alert)*/
		log(LOG_DEBUG, 0, " extend ancillary data space\n");
		/* extend ancillary data space (if necessary) */
		if (ctlbuflen < ctllen) 
		{
			//printf("ctlbuflen=%d,ctllen=%d,[%s]:[%d].\n",ctlbuflen,ctllen,__FUNCTION__,__LINE__);
			if (sndcmsgbuf)
				free(sndcmsgbuf);
			if ((sndcmsgbuf = malloc(ctllen)) == NULL)
				log(LOG_ERR, 0, "make_mld6_msg: malloc failed"); /* assert */
			ctlbuflen = ctllen;
		}
		log(LOG_DEBUG, 0, " store ancillary data\n");
		/* store ancillary data */
		if ((sndmh.msg_controllen = ctllen) > 0) 
		{
			struct cmsghdr *cmsgp=NULL;

			sndmh.msg_control = sndcmsgbuf;
			cmsgp = CMSG_FIRSTHDR(&sndmh);
			log(LOG_DEBUG, 0, " ifindex\n");
			if (dp_tmp) 
			{
				struct in6_pktinfo *pktinfo;

				cmsgp->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
				cmsgp->cmsg_level = IPPROTO_IPV6;
				cmsgp->cmsg_type = IPV6_PKTINFO;
				pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsgp);
				memset((caddr_t)pktinfo, 0, sizeof(*pktinfo));
				
				pktinfo->ipi6_ifindex = ifindex;
				
				memcpy(&pktinfo->ipi6_addr, &src,sizeof(struct in6_addr));
				cmsgp = CMSG_NXTHDR(&sndmh, cmsgp);
			}
			
			log(LOG_DEBUG, 0, " alert\n");
			if (cmsgp) 
			{
#ifdef USE_RFC2292BIS
				int currentlen;
				void *hbhbuf, *optp = NULL;

				cmsgp->cmsg_len = CMSG_LEN(hbhlen);
				cmsgp->cmsg_level = IPPROTO_IPV6;
				cmsgp->cmsg_type = IPV6_HOPOPTS;
				hbhbuf = CMSG_DATA(cmsgp);
				log(LOG_DEBUG, 0, " inet6_opt_init\n");
				if ((currentlen = inet6_opt_init(hbhbuf, hbhlen)) == -1)
					log(LOG_ERR, 0, "inet6_opt_init(len = %d) failed",hbhlen);
				log(LOG_DEBUG, 0, " inet6_opt_append\n");
				if ((currentlen = inet6_opt_append(hbhbuf, hbhlen,
								   currentlen,
								   IP6OPT_ROUTER_ALERT, 2,
								   2, &optp)) == -1)
					log(LOG_ERR, 0,
					"inet6_opt_append(len = %d/%d) failed",
					currentlen, hbhlen);
				log(LOG_DEBUG, 0, " inet6_opt_set_val\n");
				(void)inet6_opt_set_val(optp, 0, &rtalert_code,
							sizeof(rtalert_code));
				log(LOG_DEBUG, 0, " inet6_opt_finish\n");
				if (inet6_opt_finish(hbhbuf, hbhlen, currentlen) == -1)
					log(LOG_ERR, 0, "inet6_opt_finish(buf) failed");
#else  /* old advanced API */
				if (inet6_option_init((void *)cmsgp, &cmsgp, IPV6_HOPOPTS))
					log(LOG_ERR, 0, /* assert */
					"make_mld6_msg: inet6_option_init failed");
				if (inet6_option_append(cmsgp, raopt, 4, 0))
					log(LOG_ERR, 0, /* assert */
					"make_mld6_msg: inet6_option_append failed");
#endif 
				cmsgp = CMSG_NXTHDR(&sndmh, cmsgp);
			}
		}
		else
		{
			//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
			sndmh.msg_control = NULL; /* clear for safety */
		}
		log(LOG_DEBUG, 0, "end make_mld6_msg\n");
		//printf("end make_mld6_msg![%s]:[%d].\n",__FUNCTION__,__LINE__);
		/*
		struct sockaddr_in6 *dstp=(struct sockaddr_in6 *)sndmh.msg_name;
		printf( "dstaddr:%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
		dstp->sin6_addr.s6_addr16[ 0],dstp->sin6_addr.s6_addr16[ 1],dstp->sin6_addr.s6_addr16[ 2],dstp->sin6_addr.s6_addr16[ 3],
		dstp->sin6_addr.s6_addr16[ 4],dstp->sin6_addr.s6_addr16[ 5],dstp->sin6_addr.s6_addr16[ 6],dstp->sin6_addr.s6_addr16[ 7]);
		*/
		//printf("send mld_query![%s]:[%d].\n",__FUNCTION__,__LINE__);
		
		if ((dp_tmp->sock != 0) && (sendmsg(dp_tmp->sock, &sndmh, 0) < 0))
		{
			//printf("if:%s,errno:%d,SEND FAIL!!![%s]:[%d].\n",dp_tmp->Name,errno,__FUNCTION__,__LINE__);
			if (errno == ENETDOWN || errno==ENODEV || errno==EINVAL){
				//check_vif_state();
#if defined(CONFIG_MLDV2_SUPPORT)
				log(LOG_WARNING, errno, "SEND FAIL on if:%s,src address = %s.!!![%s]:[%d].\n",dp_tmp->Name,ip6_sprintf(&src), __FUNCTION__,__LINE__);
#else
				log(LOG_WARNING, errno, "SEND FAIL on if:%s,src address = 0x%x-%x-%x-%x.!!![%s]:[%d].\n",dp_tmp->Name,src.s6_addr32[0],src.s6_addr32[1],src.s6_addr32[2],src.s6_addr32[3], __FUNCTION__,__LINE__);			
#endif
			}
			else
				log(LOG_ERR, errno, "MLD6 sendmsg.");
			return 0;
		}
	}

    return SUCCESS;
}

static struct timeval start_time={0,0};
static int init_stat_time=0;
void mldv2_timer(void)
{
	struct timeval cur_time;
	//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
	if( init_stat_time==0 )
	{
		gettimeofday( &start_time, NULL );
		init_stat_time = 1;
		return;
	}


	
	gettimeofday( &cur_time, NULL );
	if( (cur_time.tv_sec > (start_time.tv_sec+1)) ||
	    ((cur_time.tv_sec == (start_time.tv_sec+1)) && (cur_time.tv_usec >=start_time.tv_usec)) )
	{
		//suppose 1 second passed
#if defined(HOST_TRACKING_SUPPORT)
		handle_host_timer(&mldProxyPara, &mcpq);
#else
		handle_group_timer(&mldProxyPara, &mcpq);
		handle_src_timer(&mldProxyPara, &mcpq);	
#endif
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
		handle_compatibility_timer(&mldProxyPara);
#endif

		start_time.tv_sec = cur_time.tv_sec;
		start_time.tv_usec = cur_time.tv_usec;
	}
	
	return;
}

/*vif: on which membership learned*/
int mld_add_group( struct in6_addr group, int vif)
{
	struct ipv6_mreq mreq;
	struct IfDesc *dp = getIfbyVif(vif);

	if (dp==NULL)
		return -1;

	memcpy(&(mreq.ipv6mr_multiaddr),&(group),sizeof(struct in6_addr));
	mreq.ipv6mr_ifindex = dp->pif_idx;

	

#if defined(CONFIG_MLDV2_SUPPORT)
	DBG_IGMP_MLD_PRK(MLD_MEMBERSHIP, "add group %s on %s(vif:%d, pif_idx=%d)\n", 
		ip6_sprintf(&group), dp->Name, dp->vif_idx, dp->pif_idx);
#else
	DBG_IGMP_MLD_PRK(MLD_MEMBERSHIP, "add group 0x%x-%x-%x-%x on %s(vif:%d, pif_idx=%d)\n", 
		group.s6_addr32[0],group.s6_addr32[1],group.s6_addr32[2],group.s6_addr32[3], dp->Name, dp->vif_idx, dp->pif_idx);			
#endif
#if defined(CONFIG_RTL_NEW_IGMP_REPORT_BEHAVIOR)
	if(setsockopt(dp->sock, IPPROTO_IPV6 , IPV6_ADD_IN_MEMBERSHIP, (void*)&mreq, sizeof(mreq))<0)
#else
	if(setsockopt(dp->sock, IPPROTO_IPV6 , IPV6_ADD_MEMBERSHIP, (void*)&mreq, sizeof(mreq))<0)
#endif
	{
		printf("errno:%d, EADDRINUSE:%d, [%s:%d]\n", errno, EADDRINUSE, __FUNCTION__, __LINE__);
		if(errno!=EADDRINUSE)
		{
#if defined(CONFIG_MLDV2_SUPPORT)
			printf("setsockopt IPV6_ADD_MEMBERSHIP %s error!\n", ip6_sprintf(&group));
#else
			printf("setsockopt IPV6_ADD_MEMBERSHIP 0x%x-%x-%x-%x error!\n", group.s6_addr32[0],group.s6_addr32[1],group.s6_addr32[2],group.s6_addr32[3]);		
#endif
			
			return -1;
		}
	}

#if defined(CONFIG_MLDV2_SUPPORT)
	DBG_IGMP_MLD_PRK(MLD_MEMBERSHIP, "add group %s successfully!\n", ip6_sprintf(&group));
#else
	DBG_IGMP_MLD_PRK(MLD_MEMBERSHIP, "add group 0x%x-%x-%x-%x successfully!\n", group.s6_addr32[0],group.s6_addr32[1],group.s6_addr32[2],group.s6_addr32[3]);			
#endif

	return 0;
}

int mld_del_group( struct in6_addr group,int vif)
{
	struct ipv6_mreq mreq;
	struct IfDesc *dp = getIfbyVif(vif);

	if (dp==NULL)
		return -1;
	

	memcpy(&(mreq.ipv6mr_multiaddr),&(group),sizeof(struct in6_addr));
	mreq.ipv6mr_ifindex =dp->pif_idx;

#if defined(CONFIG_MLDV2_SUPPORT)
	DBG_IGMP_MLD_PRK(MLD_MEMBERSHIP, "leave group %s on %s(vif:%d)\n", 
		ip6_sprintf(&group), dp->Name, dp->vif_idx);
#else
	DBG_IGMP_MLD_PRK(MLD_MEMBERSHIP, "leave group 0x%x-%x-%x-%x on %s(vif:%d)\n", 
		group.s6_addr32[0],group.s6_addr32[1],group.s6_addr32[2],group.s6_addr32[3], dp->Name, dp->vif_idx);	
#endif
	

	if(setsockopt(dp->sock, IPPROTO_IPV6 , IPV6_DROP_MEMBERSHIP, (void*)&mreq, sizeof(mreq))<0)
	{
		printf("errno:%d, EADDRNOTAVAIL:%d, [%s:%d]\n", errno, EADDRNOTAVAIL, __FUNCTION__, __LINE__);
		if(errno!=-EADDRNOTAVAIL)
		{
#if defined(CONFIG_MLDV2_SUPPORT)
			printf("setsockopt IPV6_DROP_MEMBERSHIP %s error!\n", ip6_sprintf(&group));
#else
			printf("setsockopt IPV6_DROP_MEMBERSHIP  0x%x-%x-%x-%x  error!\n", group.s6_addr32[0],group.s6_addr32[1],group.s6_addr32[2],group.s6_addr32[3]);	
#endif
			
			return -1;
		}
	}
#if defined(CONFIG_MLDV2_SUPPORT)
	DBG_IGMP_MLD_PRK(MLD_MEMBERSHIP, "leave group %s successfully!\n", ip6_sprintf(&group));
#else
	DBG_IGMP_MLD_PRK(MLD_MEMBERSHIP, "leave group 0x%x-%x-%x-%x successfully!\n", group.s6_addr32[0],group.s6_addr32[1],group.s6_addr32[2],group.s6_addr32[3]);	
#endif
	
	
	return 0;
}

/*
	add membership first(EXCLUDE NULL)
	vif: on which membership learned;
*/
int mld_set_srcfilter(struct in6_addr group, int filterMode, int srcNum, struct in6_addr *mldSrcList, int vif)
{
	int size,i;

	struct group_filter *gsf;
	struct sockaddr_in6 sock_addr;
	
	struct IfDesc *dp = getIfbyVif(vif);

	if(dp==NULL)
		return -1;

	gsf = (struct group_filter *)send_buf;

	//to check if the interface is the pif????
	gsf->gf_interface = dp->pif_idx;
	gsf->gf_fmode = filterMode;

    memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin6_family = AF_INET6;
	memcpy(&sock_addr.sin6_addr, &group, sizeof(struct in6_addr));
	memcpy(&gsf->gf_group, &sock_addr, sizeof(struct sockaddr_in6));

	gsf->gf_numsrc = srcNum;
	for(i = 0; i<srcNum; i++)
	{
		memcpy(&sock_addr.sin6_addr, &mldSrcList[i], sizeof(struct in6_addr));
		memcpy(&gsf->gf_slist[i], &sock_addr, sizeof(struct sockaddr_in6));
	}

	size = GROUP_FILTER_SIZE(srcNum);
	
	if (setsockopt(dp->sock, IPPROTO_IPV6, MCAST_MSFILTER, (char *)gsf,size) < 0 )
	{
		printf("setsockopt MCAST_MSFILTER error!!\n");
		log( LOG_WARNING, errno, "MCAST_LEAVE_SOURCE_GROUP" );
		return -1;
	}

	return 0;

}
int is_mld_unspecified_addr(struct in6_addr addr)
{
	return ((addr.s6_addr32[0]==0)&&(addr.s6_addr32[1]==0)&&(addr.s6_addr32[2]==0)&&(addr.s6_addr32[3]==0));
}
int mld_add_mr(struct in6_addr group, struct in6_addr src, int enable ,int in_vif, int out_vif)
{
	struct MRouteDesc	mrd;
	/* add multicast routing entry */
	memcpy(&(mrd.OriginAdr),&(src),sizeof(struct in6_addr));
#if !defined(CONFIG_MLDPROXY_MULTIWAN) && !defined(CONFIG_ISP_MLDPROXY_MULTIWAN) &&  defined(CONFIG_RTK_IPV6_LOGO_TEST)
	if((logoTestMode)&&(is_mld_unspecified_addr(mrd.OriginAdr)))
		mrd.OriginAdr.s6_addr[15] = in_vif;
#endif

	memset(&(mrd.SubsAdr),0,sizeof(struct in6_addr));
	memcpy(&(mrd.McAdr),&(group),sizeof(struct in6_addr));
	memset(mrd.TtlVc, 0, sizeof(mrd.TtlVc));

	mrd.InVif = in_vif;
	mrd.TtlVc[out_vif] = enable;
	mrd.TtlVc[in_vif] = enable;

	addMRoute(&mrd);

	return 1;
}

int mld_del_mr( struct in6_addr group, struct in6_addr src,int invif)
{
	struct MRouteDesc	mrd;

	/* delete multicast routing entry */
	memcpy(&(mrd.OriginAdr),&src,sizeof(struct in6_addr));
#if !defined(CONFIG_MLDPROXY_MULTIWAN) && !defined(CONFIG_ISP_MLDPROXY_MULTIWAN) &&  defined(CONFIG_RTK_IPV6_LOGO_TEST)
	if((logoTestMode)&&(is_mld_unspecified_addr(mrd.OriginAdr)))
		mrd.OriginAdr.s6_addr[15] = invif;
#endif
	memcpy(&(mrd.McAdr),&group,sizeof(struct in6_addr));
	memset(mrd.TtlVc, 0, sizeof(mrd.TtlVc));

	mrd.InVif = invif;
	
	delMRoute(&mrd);

	return 1;
}

#ifdef CONFIG_RTK_IPV6_LOGO_TEST
/*To avoid received report of self to send out*/
int checkisLocalIP(struct in6_addr ip6)
{
	FILE *fp;
	struct in6_addr		addr;
	unsigned int		ifindex = 0;
	unsigned int		prefixlen, scope, flags;
	unsigned char		scope_value;
	char			devname[IFNAMSIZ];
	char 			buf[1024];
	int exist = 0;

	/* Get link local addresses from /proc/net/if_inet6 */
	fp = fopen("/proc/net/if_inet6", "r");
	if (!fp)
		return 0;

	while (fgets(buf, sizeof(buf), fp))
	{
		//printf("buf= %s\n", buf);
		if (21 != sscanf( buf,
			"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx %x %x %x %x %8s",
			&addr.s6_addr[ 0], &addr.s6_addr[ 1], &addr.s6_addr[ 2], &addr.s6_addr[ 3],
			&addr.s6_addr[ 4], &addr.s6_addr[ 5], &addr.s6_addr[ 6], &addr.s6_addr[ 7],
			&addr.s6_addr[ 8], &addr.s6_addr[ 9], &addr.s6_addr[10], &addr.s6_addr[11],
			&addr.s6_addr[12], &addr.s6_addr[13], &addr.s6_addr[14], &addr.s6_addr[15],
			&ifindex, &prefixlen, &scope, &flags, devname))
		{
			//printf("/proc/net/if_inet6 has a broken line, ignoring");
			continue;
		}
		if(memcmp(&addr,&ip6,sizeof(struct in6_addr))==0)
		{
			exist = 1;
			break;
		}
	}

	fclose(fp);
	return exist;
}
#endif

int mldv2_accept(int recvlen)

{
	struct in6_addr *dst = NULL, src;
	struct cmsghdr *cm;
	struct in6_pktinfo *pi = NULL;
#if defined(CONFIG_LUNA) && defined(CONFIG_MLDPROXY_MULTIWAN)
	struct in6_pktinfo_rtk_mld *pi_rtk = NULL;
	int from_dev_ifindex = -1;
#endif
	int *hlimp = NULL;
	struct sockaddr_in6 *sock_src = (struct sockaddr_in6 *) rcvmh.msg_name;

	int ifindex = -1;
	int in_vif = -1, out_vif = -1;
	struct IfDesc *inDesc = NULL;
	unsigned char *l3PktBuf = NULL;
	int l3PktLen = 0;
	int type = 0;
#if defined(CONFIG_MLDPROXY_MULTIWAN)
	int index=0;
#endif
#if defined(CONFIG_LUNA) && defined(CONFIG_MLDPROXY_MULTIWAN)
	int binding_out_vif=-1;
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	struct mrt6msg *msg=NULL;
	struct mcft_entry *mymcp = NULL;
	struct src_entry *srcp=NULL;
	struct mbr_entry *mbrp=NULL;
#endif	/* CONFIG_MLDV2_SUPPORT */
	
	//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
	if (recvlen < sizeof(struct mld_hdr))
	{
		log(LOG_WARNING, 0,
		    "received packet too short (%u bytes) for MLD header",
		    recvlen);
		//printf("recvlen:%d,sizelimit:%d",recvlen,sizeof(struct mld_hdr));
		return 0;
	}
	
	l3PktBuf = (unsigned char *) rcvmh.msg_iov[0].iov_base;
	memcpy(&src, &sock_src->sin6_addr, sizeof(struct in6_addr));

	type = ((struct mld_hdr *)l3PktBuf)->mld_icmp6_hdr.icmp6_type;
#if defined(CONFIG_MLDV2_SUPPORT)
	/* Special Case:
	 * Due to first byte of struct mrt6msg must be zero, and there is no zero for icmpv6 type
	 *	- https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol_for_IPv6
	 *	- please check kernel/include/uapi/linux/mroute6.h
	 * So, We assume type 0 is IPv6 cache report
	 */
	if(type == IPV6_CACHE_REPORT){
		msg = (struct mrt6msg*)l3PktBuf;
		if(!msg){
			DBG_IGMP_MLD_PRK(MLD_RECEIVE, "ERROR: null mrt6msg\n");
			return -1;
		}
	}
#endif	/* CONFIG_MLDV2_SUPPORT */

	//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
/*To avoid received report of self to send out*/
	if(checkisLocalIP(src))
	{
		DBG_IGMP_MLD_PRK(MLD_RECEIVE, "receive a mld using self src_ip\n");
		return 0;
	}
#endif

	/* extract optional information via Advanced API */
	for (cm = (struct cmsghdr *) CMSG_FIRSTHDR(&rcvmh);
	     cm;
	     cm = (struct cmsghdr *) CMSG_NXTHDR(&rcvmh, cm))
	{
		
		//printf("get information via Advanced API[%s]:[%d], leval:%d, type:%d.\n",__FUNCTION__,__LINE__, cm->cmsg_level, cm->cmsg_type);	
		if (cm->cmsg_level == IPPROTO_IPV6 &&
			cm->cmsg_type == IPV6_PKTINFO &&
			cm->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo)))
		{
			//printf("get information via Advanced API[%s]:[%d].\n",__FUNCTION__,__LINE__);	
			pi = (struct in6_pktinfo *) (CMSG_DATA(cm));
			ifindex = pi->ipi6_ifindex;
			dst = &pi->ipi6_addr;
		}
		if (cm->cmsg_level == IPPROTO_IPV6 &&
		    cm->cmsg_type == IPV6_HOPLIMIT &&
		    cm->cmsg_len == CMSG_LEN(sizeof(int)))
			hlimp = (int *) CMSG_DATA(cm);

#if defined(CONFIG_LUNA) && defined(CONFIG_MLDPROXY_MULTIWAN)
		if (cm->cmsg_level == IPPROTO_IPV6 &&
			cm->cmsg_type == IPV6_PKTINFO_RTK &&
			cm->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo_rtk_mld)))
		{
			//printf("get information via Advanced API[%s]:[%d].\n",__FUNCTION__,__LINE__);	
			pi_rtk = (struct in6_pktinfo_rtk_mld *) (CMSG_DATA(cm));
			if(pi_rtk->from_dev_ifindex_valid){
				from_dev_ifindex = pi_rtk->from_dev_ifindex;
			}
		}
#endif
	}

#if defined(CONFIG_LUNA) && defined(CONFIG_MLDPROXY_MULTIWAN)
	if(from_dev_ifindex == -1){
		log(LOG_WARNING, 0,
			"not get lan-wan binding info,in_vif name=%s",inDesc->Name);
		return 0;
	}

	memset(&mld_lanwanBinding,0,sizeof(mld_lanwanBinding));
	if(rtk_igmp_mld_get_lanwanBindingInfo(MLD_Socket,from_dev_ifindex) < 0){
		log(LOG_WARNING, 0,
			"get lan-wan binding info fail,in_vif name=%s",inDesc->Name);

		return 0;
	}
#endif

#if defined(CONFIG_MLDV2_SUPPORT)
	/*	IPv6 cache report is not related to input interface, 
	 *	we just care streaming interface (msg->im6_mif).
	 */
	if(type != IPV6_CACHE_REPORT){
#endif
		inDesc = getIfByPix(ifindex);
		if(inDesc==NULL)
		{
			DBG_IGMP_MLD_PRK(MLD_RECEIVE, "receive a mld on a unknown vif\n");
			return -1;
		}

		in_vif = inDesc->vif_idx;
#if defined(CONFIG_LUNA) && defined(CONFIG_MLDPROXY_MULTIWAN)
		binding_out_vif = mld_get_lanwanBindingInfo(from_dev_ifindex);
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
		/* Readylogo case check if mld is from wan interface
		 * Note: loopback packet from_dev is NULL =>from_dev_ifindex=-1,So, mldproxy will not handle loopback packet in Multi-Wan mode
		 */
		int i=0;
		if(logoTestMode){
			for (i=0; i<MAXWAN;i++) 	{
				if(strcmp(inDesc->Name, mld_up_if_name[i])==0){
					binding_out_vif = mld_down_if_idx;
					break;
				}
			}
		}
#endif
#endif

		if(in_vif==NO_VIF)
		{
			DBG_IGMP_MLD_PRK(MLD_RECEIVE, "receive a mld on a wrong in_vif\n");
			return -1;
		}
#if defined(CONFIG_MLDV2_SUPPORT)
	}
#endif

#if defined(CONFIG_MLDPROXY_MULTIWAN)
	for(index=0;index<mld_up_if_num;index++)
	{
#if defined(CONFIG_MLDV2_SUPPORT)
		if(type == IPV6_CACHE_REPORT){
			out_vif = mld_up_if_idx[index];
			if(out_vif != msg->im6_mif)
				continue;
		}else{
#endif	/* CONFIG_MLDV2_SUPPORT */
			out_vif = getOutVif(in_vif, index);
			if(out_vif == NO_VIF)
				continue;

#if defined(CONFIG_LUNA) && defined(CONFIG_MLDPROXY_MULTIWAN)
			if(binding_out_vif != out_vif)
				continue;
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
		}
#endif
#else	//CONFIG_MLDPROXY_MULTIWAN
#if defined(CONFIG_MLDV2_SUPPORT)
	if(type == IPV6_CACHE_REPORT)
		out_vif == msg->im6_mif;
	else
		out_vif = getOutVif(in_vif);
#else
	out_vif = getOutVif(in_vif);
#endif
#endif	//CONFIG_MLDPROXY_MULTIWAN

	if(out_vif==NO_VIF)
	{
		DBG_IGMP_MLD_PRK(MLD_RECEIVE, "receive a mld on a wrong out_vif\n");
		return -1;
	}

	if (((hlimp == NULL)||(*hlimp!=1))
#if defined(CONFIG_MLDV2_SUPPORT)
		&& type != IPV6_CACHE_REPORT
#endif
	)
	{
		//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
		log(LOG_WARNING, 0,
		    "failed to get receiving hop limit,in_vif name=%s",inDesc->Name);
		return 0;
	}

	//shirley: to do, change general query interval
	l3PktLen = 0;
	
	switch(type)
	{

		case ICMPV6_MGM_QUERY:
			rtl_handleIgmpMldQuery((unsigned int *)(&src), l3PktBuf, l3PktLen, &mldProxyPara, &mcpq, in_vif, out_vif);
			break;
			
		case ICMPV6_MGM_REPORT:
			rtl_handleIgmpv2Mldv1Join((unsigned int *)(&src), l3PktBuf, &mldProxyPara, &mcpq, in_vif, out_vif);
			break;
			
		case ICMPV6_MGM_REDUCTION:	
			rtl_handleIgmpMldLeave((unsigned int *)(&src), l3PktBuf, &mldProxyPara, &mcpq, in_vif, out_vif);
			break;
			
		case ICMPV6_MLD2_REPORT:
			rtl_handleIgmpv3Mldv2Report((unsigned int *)(&src), l3PktBuf, &mldProxyPara, &mcpq, in_vif, out_vif);
			break;
#if defined(CONFIG_MLDV2_SUPPORT)
		case IPV6_CACHE_REPORT:
		{
			msg = (struct mrt6msg*)l3PktBuf;
			if(msg->im6_msgtype != MRT6MSG_NOCACHE)
				break;	// ignore unknown type

			
#if defined(CONFIG_MLDV2_SUPPORT)
			DBG_IGMP_MLD_PRK(MLD_RECEIVE, "receive MRT6MSG_NOCACHE, traffic vif=%d, Group:%s, Src=%s\n", msg->im6_mif, ip6_sprintf(&msg->im6_dst), ip6_sprintf(&msg->im6_src));
#else
			DBG_IGMP_MLD_PRK(MLD_RECEIVE, "receive MRT6MSG_NOCACHE, traffic vif=%d, Group:%0x%x-%x-%x-%x, Src=0x%x-%x-%x-%x\n", \
				msg->im6_mif, msg->im6_dst.s6_addr32[0], msg->im6_dst.s6_addr32[1], msg->im6_dst.s6_addr32[2], msg->im6_dst.s6_addr32[3], \
				msg->im6_src.s6_addr32[0],msg->im6_src.s6_addr32[1],msg->im6_src.s6_addr32[2],msg->im6_src.s6_addr32[3]);
#endif

			mymcp=get_mcft(&(msg->im6_dst), &mcpq);
			if(mymcp && mymcp->iflist){
				struct ifrec_entry *tmp=mymcp->iflist;
				while(tmp){
					if(tmp->proxy_vif != msg->im6_mif){
						tmp=tmp->next;
						continue;
					}
					DBG_IGMP_MLD_PRK(MLD_RECEIVE, "found intf record for vif=%d, filter mode=%s\n", tmp->proxy_vif, (tmp->filter_mode==MCAST_INCLUDE)?"include":"exclude");
					srcp=mymcp->iflist->srclist;
					/*  RFC3810 Section 7.3. https://tools.ietf.org/html/rfc3810#section-7.3
					 *  INCLUDE mode:
					 *   - if src timer > 0, then Suggest to forward traffic from source.
					 *   - if src timer = 0, then Suggest to stop forward traffic from source and remove source record.
					 *  EXCLUDE mode:
					 *   - if src timer > 0, then Suggest to forward traffic from source.
					 *   - if src timer = 0, then Suggest to stop forward traffic from source.
					 *   - if no source element: Suggest to forward traffic from all sources
					 */
					if(check_src_set(&(msg->im6_src), srcp, mldProxyPara.ipVersion)){
						DBG_IGMP_MLD_PRK(MLD_RECEIVE, "found src record with type=%d, timerInterval=%d, retry_left=%d, lefttime=%d\n",
							srcp->timer.type, srcp->timer.timerInterval, srcp->timer.retry_left, srcp->timer.lefttime);

						mbrp = srcp->grp_mbr;
						while(mbrp)
						{
							DBG_IGMP_MLD_PRK(MLD_RECEIVE, "check member %s with type=%d, timerInterval=%d, retry_left=%d, lefttime=%d\n", addr_sprintf(mbrp->user_addr, mldProxyPara.ipVersion),
								mbrp->timer.type, mbrp->timer.timerInterval, mbrp->timer.retry_left, mbrp->timer.lefttime);

							if(mbrp->timer.lefttime > 0){
								DBG_IGMP_MLD_PRK(MLD_RECEIVE, "add mfc for this member, in_vif=%d, out_vif=%d\n", tmp->ifindex, tmp->proxy_vif);
								mld_add_mr(msg->im6_dst, msg->im6_src, 1, tmp->proxy_vif, tmp->ifindex);
								break;
							}

							mbrp = mbrp->next;
						}
						break;
					}else{
						DBG_IGMP_MLD_PRK(MLD_RECEIVE, "No Src found\n");
						if(tmp->filter_mode==MCAST_EXCLUDE){
							DBG_IGMP_MLD_PRK(MLD_RECEIVE, "add mfc for in_vif=%d, out_vif=%d\n", tmp->ifindex, tmp->proxy_vif);
							mld_add_mr(msg->im6_dst, msg->im6_src, 1, tmp->proxy_vif, tmp->ifindex);
							break;
						}
					}

					tmp=tmp->next;
				}
			}
			break;
		}
#endif /* CONFIG_MLDV2_SUPPORT */
		default:
			DBG_IGMP_MLD_PRK(MLD_RECEIVE, "receive a mld with unknown type!\n");
			break;
	}

#if defined(CONFIG_MLDPROXY_MULTIWAN) 
	}
#endif
    return 0;
}

/*
 *	A FIFO special file is created by init_fifo() and recv_fifo() is managed
 *	to receive command and process it.
 */
#ifdef EMBED
char MLD_FIFO[]="/var/run/mld.fifo";
#else
char MLD_FIFO[]="mld.fifo";
#endif
int fifofd;

/*
 * Create a FIFO special file IGMP_FIFO[] and open it for reading with
 * nonblocking mode.
 */
int init_fifo(void)
{
	if (access(MLD_FIFO, F_OK) == -1)
	{
		if (mkfifo(MLD_FIFO, 0644) == -1)
			printf("access %s failed: %s\n", MLD_FIFO, strerror(errno));
	}
	
	fifofd = open(MLD_FIFO, O_RDWR);
	if (fifofd == -1)
		printf("open %s failed\n", MLD_FIFO);
	return fifofd;
}

int remove_fifo(void)
{
	if (fifofd >=0) {
		close(fifofd);
		unlink(MLD_FIFO);
	}
	return 0;
}

struct command
{
	char	*cmd;
	int	(*func)(char *arg);
	char	*help;
};

static struct command cmds[];

static int show_usage(char *arg)
{
	int i;
	
	printf("Usage:\n");
	printf("\techo [cmd] > %s\n", MLD_FIFO);
	for (i=0; cmds[i].cmd!=NULL; i++) {
		printf("\t%s\n", cmds[i].help);
	}
	return 0;
}

static int set_loglevel(char *arg)
{
	int num;
	
	if (arg==NULL)
		printf("Log2Stderr = %d\n", Log2Stderr);
	else {
		num = atoi(arg);
		if (num > 0) {
			Log2Stderr = num;
			printf("Set Log2Stderr to %d\n", Log2Stderr);
		}
	}
	return 0;
}

static int set_mld_logBitMap(char *arg)
{
	unsigned int num;
	
	if (arg==NULL)
		printf("mldLogBitMap = 0x%x\n", mldLogBitMap);
	else {
		num = strtoul(arg,NULL,0);
		if (num >= 0) {
			mldLogBitMap = num;
			printf("Set mldLogBitMap to %d\n", mldLogBitMap);
		}
	}
	return 0;
}

static int show_mcft_list(char *arg){
	show_mfct(mcpq);
    return SUCCESS;
}

static int show_mldProxyPara(char *arg){
	printf("\nshow mldProxyPara\n");
	printf("%s=%s\n", cfg_param[CFG_IP_VER],mldProxyPara.ipVersion == IP_VERSION6 ? "IPV6":"IPV4");
	printf("%s=%s\n", cfg_param[CFG_FORCE_VER],mldProxyPara.forceVersion == MLD_VER_2 ? "MLDV2":"MLDV1");
	printf("%s=%d\n", cfg_param[CFG_FAST_LEAVE],mldProxyPara.fastleaveEnable);
	printf("%s=%d\n", cfg_param[CFG_LAN_CONPATIBLE],mldProxyPara.compatibleEnbale);
	printf("%s=%d\n", cfg_param[CFG_ROBUST],mldProxyPara.timeConfig.robustness);
	printf("%s=%d\n", cfg_param[CFG_MEMBER_QUERY_INTERVAL],mldProxyPara.timeConfig.memberQueryInterval);
	printf("%s=%d\n", cfg_param[CFG_MEMBER_QUERY_COUNT],mldProxyPara.timeConfig.memberQueryCount);
	printf("%s=%d\n", cfg_param[CFG_STARTUP_QUERY_INTERVAL],mldProxyPara.timeConfig.setUpGeneralQueryInterval);
	printf("%s=%d\n", cfg_param[CFG_STARTUP_QUERY_COUNT],mldProxyPara.timeConfig.setUpGeneralQueryCount);
	printf("%s=%d\n", cfg_param[CFG_LAST_MEMBER_QUERY_INTERVAL],mldProxyPara.timeConfig.lastMemberInterval);
	printf("%s=%d\n", cfg_param[CFG_LAST_MEMBER_QUERY_COUNT],mldProxyPara.timeConfig.lastMemberCount);
	printf("%s=%d\n", cfg_param[CFG_QUERY_INTERVAL],mldProxyPara.timeConfig.periodGeneralQueryInterval);
	printf("%s=%d\n", cfg_param[CFG_QUERY_COUNT],mldProxyPara.timeConfig.periodGeneralQueryCount);
	printf("%s=%d\n", cfg_param[CFG_QUERY_RESPONSE_INTERVAL],mldProxyPara.timeConfig.QueryResponseInterval);
	printf("%s=%d\n", cfg_param[CFG_LINK_CHANGE_QUERY_INTERVAL],mldProxyPara.timeConfig.linkChangeQueryInterval);
	printf("%s=%d\n", cfg_param[CFG_LINK_CHANGE_QUERY_COUNT],mldProxyPara.timeConfig.linkChangeQueryCount);
	printf("%s=%d\n", cfg_param[CFG_INTERVAL_SCALE],mldProxyPara.timeConfig.intervalScale);
	printf("%s=%d\n", cfg_param[CFG_HOST_PRESERT_INTERVAL],mldProxyPara.timeConfig.oldVersionHostPresentTimeout);
    return SUCCESS;
}

static int reloadMldProxyCfg(char *arg){
	return init_mldProxyPara(&mldProxyPara);
}

static struct command cmds[] = {
	{"help", show_usage, "[help] Show usage."},
	{"log", set_loglevel, "[log <level>] set debug loglevel; show loglevel if <level> not specified."},
	{"mld_log", set_mld_logBitMap, "[mld_log <BitMap>] set mld logBitMap; show mld logBitMap if <BitMap> not specified."},
	{"mcft", show_mcft_list, "[mcft] show local mcft list ."},
	{"dumpcfg", show_mldProxyPara, "[dumpcfg] show mldProxyPara."},
	{"reloadcfg", reloadMldProxyCfg, "[reloadcfg] reload mldProxyPara."},
	{NULL, NULL, ""}
};

static int parse_cmd(char *cmd)
{
	char *s;
	int i;
	
	s = strtok(cmd, " \n");
	for (i=0; cmds[i].cmd!=NULL; i++) {
		if (!strcmp(s, cmds[i].cmd)) {
			s = strtok(NULL, " \n");
			cmds[i].func(s);
			break;
		}
	}
	return 0;
}

// Read fifo command and process it.
// 
void recv_fifo(void)
{
	char buffer[256];
	int n;
	n = read(fifofd, buffer, sizeof(buffer));
	if (n > 0) {
		buffer[sizeof(buffer)-1] = '\0';
		parse_cmd(buffer);
	}
}


void set_parameter(struct mCastProxyConfig *mCastProxyPara)
{
	mCastProxyPara->fastleaveEnable = 1;
	return;
}

void usage(void)
{
	printf ("mldproxy -i {upstream intf} -o {downstream intf} [OPTION]...\n");
	printf ("Usage:\n");
	printf ("  -i        upstream interfaces, Ex: -i nas0_0 nas0_1\n");
	printf ("  -o        downstream interfaces, Ex: -o br0\n");
#if defined(CONFIG_MLDV2_SUPPORT)
	printf ("  -mldfile	 mld configure file, Ex: -mldfile /var/mld_config\n");
#endif
#if defined(CONFIG_IGMPV3_SUPPORT)
	printf ("  -igmpfile igmp configure file, Ex: -igmpfile /var/igmp_config\n");
#endif
#if defined(CONFIG_RTK_IPV6_LOGO_TEST)
	printf ("-L Logo test mode,  0:none  1:Readylogo Router  2: Readylogo Host  3.CE-Router\n");
#endif

	return 0;
}

static const char use_noarg[] = "No argument for command: %s";

int main(int argc, char **argv)
{
	int i = 0;
	int _argc = 0;
#if defined(CONFIG_MLDPROXY_MULTIWAN) || defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
    char *_argv[12];
	int index;		
#else
	char *_argv[7];
#endif
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	int ret_update=-1;
#endif
	pid_t pid;
	int execed = 0;
	char cmdBuffer[50];//Brad add 20080605
	struct IfDesc *IfDp;
	int flags;
#if defined(CONFIG_MLDPROXY_MULTIWAN) || defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	if (argc >= 12) 
#else
	if (argc >= 7) 
#endif
	{
		fprintf(stderr, "To many arguments \n");
		exit(1);
	}
	
	if (strcmp(argv[argc-1], "-D") == 0) {
		argc--;
		execed = 1;
	}

	if (!execed) {
		if ((pid = vfork()) < 0) {
			printf("[%s]:[%d].vfork failed\n",__FUNCTION__,__LINE__);
			fprintf(stderr, "vfork failed\n");
			exit(1);
		} else if (pid != 0) {
			//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
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

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-o")) {
			if (++i == argc){
				printf(use_noarg, argv[i-1]);
				exit(1);
			}
			snprintf(mld_down_if_name, sizeof(mld_down_if_name), "%s", argv[i]);
		} 
		else if (!strcmp(argv[i], "-i")) {
			if (++i == argc){
				printf(use_noarg, argv[i-1]);
				exit(1);
			}
#if defined(CONFIG_MLDPROXY_MULTIWAN)
			memset(mld_up_if_name, 0, MAXWAN*IFNAMSIZ);
			for(mld_up_if_num=0; mld_up_if_num<MAXWAN; mld_up_if_num++){
				snprintf(mld_up_if_name[mld_up_if_num], IFNAMSIZ, "%s", argv[i]);
				if(argv[i+1][0] == '-'){
					mld_up_if_num++;
					break;
				}else{
					i++;
				}
			}
#else
			snprintf(mld_up_if_name, IFNAMSIZ, "%s", argv[i]);
			mld_up_if_num = 1;
#endif
		}
#if defined(CONFIG_MLDV2_SUPPORT)
		else if (!strcmp(argv[i], "-mldfile")) {
			if (++i == argc){
				printf(use_noarg, argv[i-1]);
				exit(1);
			}
			snprintf(config_mld_file, sizeof(config_mld_file), "%s", argv[i]);
		}
#endif
#if defined(CONFIG_IGMPV3_SUPPORT)
		else if (!strcmp(argv[i], "-igmpfile")) {
			if (++i == argc){
				printf(use_noarg, argv[i-1]);
				exit(1);
			}
			snprintf(config_igmp_file, sizeof(config_igmp_file), "%s", argv[i]);
		}
#endif
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
		else if (!strcmp(argv[i], "-L")) {
			if (++i == argc){
				printf(use_noarg, argv[i-1]);
				exit(1);
			}
			logoTestMode=atoi(argv[i]);
		}
#endif

		else{
			printf("unknow parameter(%s)\n", argv[i]);
			usage();
			exit(FAILED);
		}
	}

#if defined(CONFIG_MLDPROXY_MULTIWAN) || defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	if(!mld_up_if_num){
		fprintf(stderr, "No upstream interface, exit process\n");
		exit(FAILED);
	}

	mld_if_num = mld_up_if_num+1;
#endif

#if defined(CONFIG_MLDPROXY_MULTIWAN)	
	for(index=0;index<mld_up_if_num;index++)
	{
		memset(cmdBuffer, '\0',sizeof(cmdBuffer));
		sprintf(cmdBuffer, "echo %s > /var/mld_up", mld_up_if_name[index]);
		//system(cmdBuffer);
		va_cmd("/bin/sh" , 2, 1, "-c", cmdBuffer);
	}
#else
	//Brad add 20080605
	memset(cmdBuffer, '\0',sizeof(cmdBuffer));
	sprintf(cmdBuffer, "echo %s > /var/mld_up", mld_up_if_name);
	//system(cmdBuffer);
	va_cmd("/bin/sh" , 2, 1, "-c", cmdBuffer);
#endif

	write_pid();

	if(FAILED == init_mldProxyPara(&mldProxyPara)){
		fprintf(stderr, "init parameter fail\n");
#if defined(CONFIG_MLDV2_SUPPORT)
		fprintf(stderr, "  mld_config=%s\n", config_mld_file);
#endif
#if defined(CONFIG_IGMPV3_SUPPORT)
		fprintf(stderr, "  igmp_config=%s\n", config_igmp_file);
#endif
#ifdef CONFIG_RTK_IPV6_LOGO_TEST
		if(!logoTestMode)
#endif
		exit(FAILED);
	}
	
	/*here to set mld proxy daemon pid to kernel*/
	{
		int br_socket_fd = -1;
		unsigned long arg[3];

		arg[0] = BRCTL_SET_MLDPROXY_PID;	
		arg[1] = getpid();
		arg[2] = 0;

		if ((br_socket_fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		{
			log(LOG_ERR, errno,"create socket to connect bridge fail! \n");
			return -1;
		}
		else 
		{	
			ioctl(br_socket_fd, SIOCGIFBR, arg);
		}
		close(br_socket_fd);
	}

	init_signals();

	mld_down_if_idx=NO_VIF;
#if !defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	initIfDesc();
#if defined(CONFIG_MLDPROXY_MULTIWAN)
	for(i = 0;i<mld_up_if_num;i++)
	{
		mld_up_if_idx[i]=NO_VIF;
	}
#else
	mld_up_if_idx=NO_VIF;
#endif
#endif

	while (!initMRouter())
	{
		// Kaohj, polling every 2 seconds
		//printf("initMRouter fail\n");
		sleep(2);
	}

	init_mld();

	//hyking:recv the sock for avoid dst cache refcnt issue.
	//2010-8-3
	//printf("[%s]:[%d]\n",__FUNCTION__,__LINE__);
	IfDp = getIfByName(mld_down_if_name);
	if((IfDp!=NULL)&&(IfDp->sock > 0))
	{
		add_fd(IfDp->sock);
		flags = fcntl(IfDp->sock, F_GETFL);
		if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
		   	log(LOG_WARNING, 0 ,"Couldn't set %s to nonblock\n",mld_down_if_name);

	}
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
	for(index=0, IfDp= IfDescVc; index<MAX_IF;index++, IfDp++)
	{
		if(strcmp(IfDp->Name, "")==0 || strcmp(IfDp->Name, mld_down_if_name)==0 || IfDp->sock<=0)
			continue;
		
		add_fd(IfDp->sock);
		flags = fcntl(IfDp->sock, F_GETFL);
		if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
			log(LOG_WARNING, 0,"Couldn't set sock of %s to nonblock\n",IfDp->Name);
	}

#elif defined(CONFIG_MLDPROXY_MULTIWAN)
	for(index=0;index<mld_up_if_num;index++)
	{
		IfDp = getIfByName(mld_up_if_name[index]);
		if((IfDp!=NULL)&&(IfDp->sock > 0))
		{
			add_fd(IfDp->sock);
			flags = fcntl(IfDp->sock, F_GETFL);
			if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
			   	log(LOG_WARNING, 0,"Couldn't set sock of %s to nonblock\n",mld_up_if_name[index]);
		}
	}
		
#else
	IfDp = getIfByName(mld_up_if_name);
	if((IfDp!=NULL)&&(IfDp->sock > 0))
	{
		add_fd(IfDp->sock);
		flags = fcntl(IfDp->sock, F_GETFL);
		if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
		   	log(LOG_WARNING, 0,"Couldn't set sock of %s to nonblock\n",mld_up_if_name);
	}
#endif
	
	if(MLD_Socket>0)
	{
		add_fd(MLD_Socket);
		flags = fcntl(MLD_Socket, F_GETFL);
		if (flags == -1 || fcntl(MLD_Socket, F_SETFL, flags | O_NONBLOCK) == -1)
		   	log(LOG_WARNING, 0,"Couldn't set MLD_Socket to nonblock\n");

	}

	
	init_fifo();
	if (fifofd >= 0)
		add_fd(fifofd);

#if defined(USE_STATIC_ENTRY_BUFFER)
	init_static_entry_tbl();
#endif

#if defined(CONFIG_LUNA) && defined(CONFIG_MLDPROXY_MULTIWAN)
	rtk_igmp_mld_hook_register(mld_set_lanwanBindingInfo,log);
#endif

	send_setup_general_query(&mldProxyPara);

	DISPLAY_MLD_BANNER;

	while(1)
	{
	
		fd_set in;
		struct timeval tv;
		int ret;
		int recvlen;
		int    mld6_recvlen;
#ifdef CONFIG_MLDV2_SUPPORT
		mldv2_timer();
#endif
		calltimeout();		

		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
		//update wan interface info, block signal
		if(mld_update_wan)
		{
			buildIfVc();

			//sync vif
			for(index=0, IfDp=IfDescVc; index<MAX_IF; index ++, IfDp++)
			{
				if(strcmp(IfDp->Name, mld_down_if_name) ==0)
					continue;

				if(strcmp(IfDp->Name, "")!=0)
				{
					//current mld up interface
					//eth1.xxx and eth1 has the same vif index
					delVIf(IfDp->vif_idx);
					addVIF(IfDp);

					if(IfDp->sock <= 0)
					{
						//new interface
						mld_inf_create(IfDp);

						add_fd(IfDp->sock);
						flags = fcntl(IfDp->sock, F_GETFL);
						if (flags == -1 || fcntl(IfDp->sock, F_SETFL, flags | O_NONBLOCK) == -1)
							log(LOG_WARNING, 0,"Couldn't set sock of %s to nonblock\n",IfDp->Name);
					}					
				}
				else
				{
					//old interface be deleted 
					if(mld_if_sock_old[index] >0 && FD_ISSET(mld_if_sock_old[index], &in_fds))
					{
						remove_fd(mld_if_sock_old[index]);
					}

					if(mld_vif_old[index] >= 0)
						delVIf(mld_vif_old[index]);
				}
			}

		}
		mld_update_wan = 0;
#endif		
		in = in_fds;
		
		ret = select(max_in_fd+1, &in, NULL, NULL, &tv);

		if( ret <= 0 ){
			//printf("mld: timeout\n");
			continue;
		} 
		IfDp=NULL;
		//printf("ret=%d,[%s]:[%d]\n",ret,__FUNCTION__,__LINE__);
		if(FD_ISSET(MLD_Socket, &in)) 
		{

		    rcvmh.msg_controllen=CMSG_SPACE(sizeof(struct in6_pktinfo)) +CMSG_SPACE(sizeof(int));
#if defined(CONFIG_LUNA) && defined(CONFIG_MLDPROXY_MULTIWAN)
			rcvmh.msg_controllen += CMSG_SPACE(sizeof(struct in6_pktinfo_rtk_mld));
#endif
		    mld6_recvlen = recvmsg(MLD_Socket, &rcvmh, 0);

		    if (mld6_recvlen < 0)
		    {
		    	//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
				if (errno != EINTR && errno != EAGAIN)
			    	log(LOG_ERR, errno, "MLD6 recvmsg");

				continue;
		    }
			
			mldv2_accept(mld6_recvlen);
			//printf("mld6_recvlen:%d\n",mld6_recvlen);
		}

		if(FD_ISSET(fifofd, &in))
			recv_fifo(); // read commands from fifo

		//hyking:recv the sock for avoid dst cache refcnt issue.
		//2010-8-3
		IfDp =  getIfByName(mld_down_if_name);
		if((IfDp!=NULL)&&(IfDp->sock)&&(FD_ISSET(IfDp->sock, &in)))
		{
		
			mld6_recvlen = recvmsg(IfDp->sock, &rcvmh, 0);
			if (mld6_recvlen < 0) 
			{
		    	if (errno != EINTR && errno !=EAGAIN) 
					log(LOG_ERR, errno, "recvfrom down interface");
			}
			//printf("recvlen=%d,Receive pkts[%s]:[%d]\n",recvlen,__FUNCTION__,__LINE__);
		}
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
		for(index=0, IfDp= IfDescVc; index<MAX_IF;index++, IfDp++)
		{
			if(strcmp(IfDp->Name, "")==0 || strcmp(IfDp->Name, mld_down_if_name)==0 || !(FD_ISSET(IfDp->sock, &in_fds)))
				continue;

			mld6_recvlen = recvmsg(IfDp->sock, &rcvmh, 0);
			if (mld6_recvlen < 0) 
			{
				if (errno != EINTR && errno !=EAGAIN) 
					log(LOG_ERR, errno, "recvfrom up interface");
			}
		}
#elif defined(CONFIG_MLDPROXY_MULTIWAN)
		for(index=0;index<mld_up_if_num;index++)
		{
			IfDp =  getIfByName(mld_up_if_name[index]);	
			if((IfDp!=NULL)&&(IfDp->sock > 0)&&(FD_ISSET(IfDp->sock, &in)))
			{
				mld6_recvlen = recvmsg(IfDp->sock, &rcvmh, 0);
				if (mld6_recvlen < 0) 
				{
			    	if (errno != EINTR && errno !=EAGAIN) 
						log(LOG_ERR, errno, "recvfrom up interface");
				}
			}
		}
			
#else
		IfDp =  getIfByName(mld_up_if_name);		
		if((IfDp!=NULL)&&(IfDp->sock > 0)&&(FD_ISSET(IfDp->sock, &in)))
		{
			mld6_recvlen = recvmsg(IfDp->sock, &rcvmh, 0);
			if (mld6_recvlen < 0) 
			{
		    	if (errno != EINTR && errno !=EAGAIN) 
					log(LOG_ERR, errno, "recvfrom up interface");
			}
		}
		
#endif
	}
	log(LOG_WARNING, 0,"!!!JUMP out of loop!!!\n[%s]:[%d]\n",__FUNCTION__,__LINE__);
	remove_fifo();
	return 0;
}

