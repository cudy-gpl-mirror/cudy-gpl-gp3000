#include <stdio.h>

#include "phruncall.h"
#include "phupdate.h"
#include "log.h"
#include <signal.h>     /* for singal handle */
#ifndef WIN32
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <netdb.h>
#include <unistd.h>     /* for close() */

static char reg_domain[256] = {0};
static char reg_ip[64] = {0};
static char statusFile[256] = {0};

static void create_pidfile(PH_parameter *pa)
{
    FILE *pidfile;

	if(pa->pidfile[0] == 0) return ;
	
    if ((pidfile = fopen(pa->pidfile, "w")) != NULL) {
		fprintf(pidfile, "%d\n", getpid());
		(void) fclose(pidfile);
    } else {
		printf("Failed to create pid file %s: %m", pa->pidfile);
    }
}
#endif

PHGlobal global;
PH_parameter parameter;
// DDNSStatus表示此DDNS的当前的运行或错误状态
//  0表示正常运行, 1表示未启用, 2请填写服务端口表示配置信息不全,
//  3表示连接DDNS服务商平台失败, 4表示DDNS帐号在服务商平台认证失败, 
//  5请填写DDNS用户名, 6请填写DDNS密码, 7请填写DDNS域名, 8请填写DDNS主机名,
//  9表示其它错误,错误码xxx, 10 DDNS服务商拒绝服务,错误码xxx）
int get_status_ret_code(int nCode)
{
	switch (nCode)
	{
	case okConnecting:
	case okConnected:
	case okAuthpassed:
	case okDomainListed:
	case okDomainsRegistered:
	case okKeepAliveRecved:
	case okRetrievingMisc:
	case okNormal:
	case okNoData:
	case okServerER:
	case okRedirecting:
		return 0;
		
	case errorConnectFailed:
	case errorOccupyReconnect:
		return 3;
		
	case errorAuthFailed:
	case errorDomainRegisterFailed:
	case errorAuthBusy:
		return 4;
		
	case errorSocketInitialFailed:
	case errorUpdateTimeout:
	case errorKeepAliveError:
	case errorRetrying:
		return 9;
	
	case errorDomainListFailed:
	case errorStatDetailInfoFailed:
		return 10;
	}

	return 0;
}

static void dumpStatus(int status)
{
	int ret_code = 0;
	FILE *fp = fopen(statusFile, "w");
	if(fp)
	{
		fprintf(fp,"%d\n%s\n%s\n%s\n", 
			get_status_ret_code(status), 
			convert_status_code(status),
			reg_domain,
			reg_ip
			);
		fclose(fp);
	}
}
static void my_handleSIG (int sig)
{
	if (sig == SIGINT)
	{
		printf ("signal = SIGINT\n");
		phddns_stop(&global);
		if(parameter.pidfile) unlink(parameter.pidfile);
		exit(0);
	}
	if (sig == SIGTERM)
	{
		printf ("signal = SIGTERM\n");
		phddns_stop(&global);
		if(parameter.pidfile) unlink(parameter.pidfile);
		exit(0);
	}
	signal (sig, my_handleSIG);
}

//状态更新回调
static void myOnStatusChanged(int status, long data)
{
	printf("myOnStatusChanged %s", convert_status_code(status));
	if (status == okKeepAliveRecved)
	{
		printf(", IP: %d", data);
	}
	if (status == okDomainsRegistered)
	{
		printf(", UserType: %d", data);
	}
	printf("\n");
	
	dumpStatus(status);
}

//域名注册回调
static void myOnDomainRegistered(char *domain)
{
	memset(reg_domain, 0, sizeof(reg_domain));
	strncpy(reg_domain, domain, sizeof(reg_domain)-1);
	printf("myOnDomainRegistered %s\n", domain);
}

//用户信息XML数据回调
static void myOnUserInfo(char *userInfo, int len)
{
	char tmp_ip[64] = {0}, *ptmp_ip;
	unsigned long ip;
	int i = 0;
	if(userInfo && (ptmp_ip = strstr(userInfo, "<ClientIP>"))) {
		sscanf(ptmp_ip, "<ClientIP>%[^<]</ClientIP>%*s", tmp_ip);
		ip = strtoul(tmp_ip, NULL, 0);
		ptmp_ip = reg_ip;
		for(i=0;i<4;i++){
			ptmp_ip += sprintf(ptmp_ip, "%d.", ((ip >> (8*i))&0xFF));
		}
		if(ptmp_ip > reg_ip) ptmp_ip--;
		*ptmp_ip = '\0';
	}
	printf("myOnUserInfo %s\n", userInfo);
}

//域名信息XML数据回调
static void myOnAccountDomainInfo(char *domainInfo, int len)
{
	printf("myOnAccountDomainInfo %s\n", domainInfo);
}

int main(int argc, char *argv[])
{
	void (*ohandler) (int);
#ifdef WIN32
	WORD VersionRequested;		// passed to WSAStartup
	WSADATA  WsaData;			// receives data from WSAStartup
	int error;

	VersionRequested = MAKEWORD(2, 0);

	//start Winsock 2
	error = WSAStartup(VersionRequested, &WsaData);
#endif

	ohandler = signal (SIGINT, my_handleSIG);
	if (ohandler != SIG_DFL) {
		printf ("previous signal handler for SIGINT is not a default handler\n");
		signal (SIGINT, ohandler);
	}
	
	signal (SIGTERM, my_handleSIG);

	init_global(&global);
	init_parameter(&parameter);
	if( checkparameter(argc,argv,&global,&parameter) != 0 ) {
//		printf ("Parameter has an unexpected error,please read the help above.Thank you for your patient!\n");
		printf("Error init phddns !!!\n");
		return -1;
	}

	if( parameter.bDaemon == 1 ) {
#ifndef WIN32
		printf("phddns started as daemon!\n");
		if(0 != daemon(0,0))
		{
			printf("daemon(0,0) failed\n");
			return -1;
		}
#else
		printf("phddns started daemon failed\n");
#endif
	}

	log_open( parameter.logfile, 1);	//empty file will cause we printf to stdout
	create_pidfile(&parameter);
	
	sprintf(statusFile, "/var/tmp/ddns_oray.%s.%s.info", global.szUserID, (parameter.nicName[0])?parameter.nicName:"any");

	set_default_callback(&global);
	
	global.cbOnStatusChanged = myOnStatusChanged;
	global.cbOnDomainRegistered = myOnDomainRegistered;
	global.cbOnUserInfo = myOnUserInfo;
	global.cbOnAccountDomainInfo = myOnAccountDomainInfo;

	//以下key为花生壳官方开源客户端专用，严禁用于其他用途，后果自负！
	global.clientinfo = 0x26A09899; 
	global.challengekey = 0x1E0808B7;
	
	for (;;)
	{
		int next = phddns_step(&global);
		sleep(next);
	}
	phddns_stop(&global);
	return 0;
}
