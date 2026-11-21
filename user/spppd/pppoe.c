/*
 * PPPOE.C [RFC 2516]
 *
 */



#include "if_sppp.h"
#include "pppoe.h"
#include <linux/pkt_sched.h>
#include "md5.h"
#include "arpping.h"
#include <stdlib.h>
#include <sys/time.h>
#include <rtk/subr_net.h>

#if 1 //def CONFIG_USER_PPPOE_CTC_DELAY_INT
#define TIMEOUT1(fun, arg1, arg2, handle) 	timeout_spppd(fun,arg1,arg2, &handle)
static struct sppp *find_sp(PPPOE_DRV_CTRL *p);
void discTimeout(PPPOE_DRV_CTRL *p);
#endif

/* convert hex value to string */
static int _pppoe_hex2str(char *sval, unsigned char *hval, int len);

#define OK 0
#define ERROR -1
#define TRUE	1
#define FALSE	0

#if defined(CONFIG_USER_PPPOE_PROXY) || defined(CONFIG_USER_PPPOE_SERVER)
/* Structures used by PPPoE server */
struct ClientSession {
	unsigned int sess;		/* Session number */
	unsigned char ip[4];	/* IP address of peer */
	unsigned char hwaddr[Eid_SZ]; /* Peer's Ethernet address */
	unsigned char isUsingNow;	  /* Now is using this session. */
#ifdef CONFIG_USER_PPPOE_SERVER
	int discoverystate;
#endif
};

char *IFName = "br0";
char *ACName = "RTL_ADSL";
unsigned char localhwAddr[Eid_SZ];
unsigned int LocalIP;

unsigned char ServerIP[4] = {192, 168, 100, 1};
unsigned char ClientIP[4] = {192, 168, 100, 100}; /* Counter STARTS here */

#if defined(CONFIG_USER_PPPOE_PROXY)
#define DEFAULT_MAX_SESSIONS 32
#elif defined(CONFIG_USER_PPPOE_SERVER)
int pppoe_server_onoff = 1;
#define DEFAULT_MAX_SESSIONS 8
#endif
struct ClientSession *Sessions = NULL;

int pppoe_server_socket = -1;
fd_set pppConnSet;
int maxpppConnFd;

/* Random seed for cookie generation */
#define SEED_LEN 16
unsigned char CookieSeed[SEED_LEN];

#ifdef CONFIG_USER_PPPOE_PROXY
#define IPCP_OPT_ADDRESSES		0x01	/* both IP addresses; deprecated */
#define IPCP_OPT_COMPRESSION	0x02	/* IP compression protocol (VJ) */
#define IPCP_OPT_ADDRESS		0x03	/* local IP address */
#define IPCP_OPT_DNS_PRIMARY	0x81	/* primary DNS address */
#define IPCP_OPT_DNS_SECOND		0x83	/* second DNS address */
#endif
#endif

//extern int dbg_printf(const char * fmt, ...);
#ifdef ENABLE_PPP_SYSLOG
#include <time.h>

#define SPPPD_DIAG_RECORD_FILE "/tmp/spppd_diag_record"
#define SPPPD_DIAG_RECORD_SIZE 524288  //512KByte

int spppd_dial_record(unsigned char *buff)
{
	static FILE *spppd_fp = NULL;
	static unsigned char spppd_file_no = 0;
    time_t t;
    struct tm * lt;
	struct stat record_stat;
	char file_name[32];

	snprintf(file_name, 31, "%s%d", SPPPD_DIAG_RECORD_FILE, spppd_file_no);

    time(&t);
    lt = localtime(&t);

	if(NULL == spppd_fp){
		//Truncate  file  to zero length
		spppd_fp = fopen(file_name, "w");
		if(spppd_fp)
			fclose(spppd_fp);
	
		spppd_fp = fopen(file_name, "a");
		if(NULL != spppd_fp){
			fprintf(spppd_fp, "[%d/%d/%d %d:%d:%d start]\n",
					lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
		}
	}

	if(NULL != spppd_fp){
	    fprintf(spppd_fp, "[%d:%d:%d]", lt->tm_hour, lt->tm_min, lt->tm_sec);

		fprintf(spppd_fp, buff);
	}
	else
		return -1;

	if(0 == stat(file_name, &record_stat)){
		if(record_stat.st_size > SPPPD_DIAG_RECORD_SIZE){
			if(spppd_file_no)
				spppd_file_no = 0;
			else
				spppd_file_no = 1;	
			printf("spppd dial record over %d, change file to %s%d\n", 
					SPPPD_DIAG_RECORD_SIZE, SPPPD_DIAG_RECORD_FILE,spppd_file_no);
			fclose(spppd_fp);
			spppd_fp = NULL;
		}
	}

	return 0;
}

int dbg_printf(const char * fmt, ...)
{
	va_list args;
	unsigned char vChar, buff[256];
	unsigned char spppd_debug;

	va_start(args, fmt);
	vsprintf(buff, fmt, args);
	va_end(args);
	fflush(NULL);

	mib_get_s(MIB_SYSLOG,(void *)&vChar, sizeof(vChar));
	if (vChar==1 && dbg_syslog == 1)
		syslog(LOG_INFO, buff);
	else
		printf(buff);
	//support record ppp diag info to /tmp/spppd_diag_recordx
	mib_get_s(MIB_SPPPD_DEBUG,(void *)&spppd_debug, sizeof(spppd_debug));
	if(spppd_debug){
		//do not recored LCP keeplive pkt.
		if(0 == strncmp(buff+5, "LCP: ", 5)){
			if(0 == strncmp(buff+12, "ECHO", 4))
				goto end;
		}

		spppd_dial_record(buff);
	}

end:	
	return 0;
}
#endif

#ifdef ENABLE_PPP_SYSLOG
#define addlog		dbg_printf
#else
#define addlog		printf
#endif
#define	PPPOE_FMT		"%s%d : "
#define	PPPOE_OPT		"%s%d :    "
#define	PPPOE_ARGS(unit)	"pppoe", unit

// Kaohj
#define RECV_LEN	1024

//2014/3/4 Some PPPoE server do not accept End-Of-List Tag, so revmoe it.
//         According to PPPoE RFC 2516, End-Of-List Tag is not required.
//#define PPPOE_EOL_TAG 1

//N_SPPP has defined in if_sppp.h
//#define N_SPPP 8
unsigned int disc_counter[N_SPPP] = {0};	// Jenny, for discovery timeout counter
char pre_PADT_flag[N_SPPP] = {0};
char pre_PADT_RETRY_COUNT[N_SPPP] = {0};
char sendPADR_flag[N_SPPP] = {0};	// Jenny, for PADR flag
#if defined(CONFIG_USER_PPPOE_CTC_DELAY_INT) || defined (CONFIG_USER_PPPOE_2_INTERVALS_RETRY)
long send_padi_time[N_SPPP] = {0};
long send_padi_mtime[N_SPPP] = {0};
long send_padt_time[N_SPPP] = {0};
long send_padt_mtime[N_SPPP] = {0};
#endif
unsigned int padrsent[N_SPPP] = {0};
#ifdef PPPOE_UNIVERSAL_RETRY_INTERVAL
int padi_start_time[N_SPPP] = {0};
#endif

#ifdef ENABLE_PPP_SYSLOG
int dbg_syslog = 0;	// Jenny, for debug message output ; 1: syslog, 0: console
#endif
int pppoe_output(PPPOE_DRV_CTRL *p, void *buf, int data_len);

#define PPPOE_LOWER_SEND(p,m,l)    pppoe_output(p,m,l)
#define PPPOE_UPPER_INPUT(ifp,m)

//static void _pppoe_print_mac(const unsigned char *mac);
static void _pppoe_mac(char *result, const unsigned char *mac);
//const char *_pppoe_state_name(int state);
//static void pppoe_session_update(PPPOE_DRV_CTRL *p);
//void pre_release_pppoe(char* interface_name);
int pppoe_init(PPPOE_DRV_CTRL *pppoe_p);
void pppoe_down(PPPOE_DRV_CTRL *p);


//PPPOE_DRV_CTRL pppoe_pool[N_PPPOE];
unsigned char bcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#if defined(CONFIG_USER_PPPOE_PROXY) || defined(CONFIG_USER_PPPOE_SERVER)
void parsePADITags(PPPOEPROXY_DRV_CTRL *p, struct PPPoEPacket *packet)
{
	unsigned char *curTag, *tagValue;
	unsigned short tagType, tagLen;
	char temp[100];
	char *rpfcn = "[parsePADITags]";
	int debug = p->debug;
	int maxlen, pktlen;

	memset(p, 0, sizeof(PPPOEPROXY_DRV_CTRL));
	memcpy(p->peerEthAddr, &packet->ehdr.from, ETH_ALEN);
	pktlen = ntohs(packet->length);
	maxlen = (pktlen < (RECV_LEN-6)? pktlen:(RECV_LEN-6));
	if (maxlen < pktlen)
		printf("PPPoE payload overflow: %d\n", pktlen);

	curTag = packet->payload;
	while((curTag - packet->payload) < maxlen)
	{
		int mylen;
		tagType = (((unsigned short) curTag[0]) << 8) + (unsigned short) curTag[1];
		tagLen = (((unsigned short) curTag[2]) << 8) + (unsigned short) curTag[3];
		if (tagType == TAG_END_OF_LIST) return;
		tagValue = curTag+4;

		mylen = curTag+4+tagLen - packet->payload;
		if (mylen > maxlen) {
			break;
		}

		switch(tagType){
			case TAG_SERVICE_NAME:
				if (tagLen > 0 && tagLen < 100){
					memcpy(p->ServiceName, (char *)tagValue, tagLen);
				}
				break;
			case TAG_RELAY_SESSION_ID:
				if (tagLen > 0 && tagLen < 100){
					p->relayId.type = htons(TAG_RELAY_SESSION_ID);
					p->relayId.length = htons(tagLen);
					memcpy(p->relayId.payload, (char *)tagValue, tagLen);
				}
				break;
			case TAG_HOST_UNIQ:
				if (tagLen > 0 && tagLen < 100){
					p->hostUniq.type = htons(TAG_HOST_UNIQ);
					p->hostUniq.length = htons(tagLen);
					memcpy(p->hostUniq.payload, (char *)tagValue, tagLen);
				}
				break;
		}
		curTag = curTag + 4 + tagLen;
	}

}
#endif

#ifndef EMBED
static char g_huniq[3] = {0x00, 0xe0, 0x4c};
#endif
/**********************************************************************
*%FUNCTION: parsePADOTags
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data
* extra -- extra user data.  Should point to a PacketCriteria structure
*          which gets filled in according to selected AC name and service
*          name.
*%RETURNS: Nothing
*%DESCRIPTION: Picks interesting tags out of a PADO packet
***********************************************************************/

void parsePADOTags(PPPOE_DRV_CTRL *p, struct PPPoEPacket *packet, struct PacketCriteria *pc)
{
	unsigned char *curTag, *tagValue;
	unsigned short tagType, tagLen;
	char temp[100];
	char huniq_str[64];
	char *rpfcn = "[parsePADOTags]";
	int debug = p->debug;
	#ifdef EMBED
	unsigned char devAddr[MAC_ADDR_LEN];
	#endif
	char hustr[MAC_ADDR_LEN+1];
	// Kaohj
	int maxlen, pktlen;
	char tmpServiceName[STR_LEN]={0};

	// Kaohj ---- 6: pppoe header length
	pktlen = ntohs(packet->length);
	maxlen = (pktlen < (RECV_LEN-Ehdr_SZ-6)? pktlen:(RECV_LEN-Ehdr_SZ-6));
	if (maxlen < pktlen)
		printf("PPPoE payload overflow: %d\n", pktlen);
	//printf("maxlen=%d\n", maxlen);
	curTag = packet->payload;
	// Kaohj
	//while((curTag - packet->payload) < packet->length)
	while((curTag - packet->payload) < maxlen)
	{
		// Kaohj
		int mylen;
		tagType = (((unsigned short) curTag[0]) << 8) + (unsigned short) curTag[1];
		tagLen = (((unsigned short) curTag[2]) << 8) + (unsigned short) curTag[3];
		if (tagType == TAG_END_OF_LIST) return;
		tagValue = curTag+4;
		// Kaohj
		mylen = curTag+4+tagLen - packet->payload;
		if (mylen > maxlen) {
			//printf("mylen exceeds\n");
			break;
		}

		switch(tagType)
		{
			case TAG_AC_NAME:
				if (tagLen > 0 && tagLen < 100)
				{
					memcpy(temp, (char *)tagValue, tagLen);
					temp[tagLen] = 0;
					strncpy(p->ACName, temp, sizeof(p->ACName)-1);
					if(debug)
						addlog(PPPOE_OPT"AC-Name: %s\n", PPPOE_ARGS(p->ifno), temp);
				}
//				if (strlen(p->ACName))
//					if (tagLen == strlen(p->ACName) && !strncmp((char *) tagValue, p->ACName, tagLen))
				if (strlen(p->DesiredACName)) {
					if (tagLen == strlen(p->DesiredACName) && !strncmp((char *) tagValue, p->DesiredACName, tagLen))
						pc->acNameOK = 1;
				}
				else
					pc->acNameOK = 1;
				break;
			case TAG_SERVICE_NAME:
				// Service name NULL issue
//				if (strlen(p->ServiceName) != 0) // Jenny
//					break;
				if (tagLen > 0 && tagLen < 100)
				{
					memcpy(temp, (char *)tagValue, tagLen);
					temp[tagLen] = 0;
					// Service name NULL issue
					if (!strlen(p->ServiceName))
						strncpy(tmpServiceName, temp, sizeof(tmpServiceName)-1);
					if(debug)
						addlog(PPPOE_OPT"Service-Name: %s\n", PPPOE_ARGS(p->ifno), temp);
				}
				if (strlen(p->ServiceName)) {
					if (tagLen == strlen(p->ServiceName) && !strncmp((char *) tagValue, p->ServiceName, tagLen))
						pc->serviceNameOK = 1;
				}
				else
					pc->serviceNameOK = 1;
				break;
			case TAG_AC_COOKIE:
				p->cookie.type = htons(tagType);
				p->cookie.length = htons(tagLen);
				memcpy(p->cookie.payload, tagValue, tagLen);
				break;
			case TAG_RELAY_SESSION_ID:
				p->relayId.type = htons(tagType);
				p->relayId.length = htons(tagLen);
				memcpy(p->relayId.payload, tagValue, tagLen);
				break;
			case TAG_SERVICE_NAME_ERROR:
				syslog(LOG_INFO, "spppd: ppp%d: PPPoE service name error: %.*s\n", p->ifno, (int) tagLen, tagValue);
				if(debug)
					addlog(PPPOE_OPT"Service-Name-Error: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				return;
//				break;
			case TAG_AC_SYSTEM_ERROR:
				syslog(LOG_INFO, "spppd: ppp%d: PPPoE AC system error: %.*s\n", p->ifno, (int) tagLen, tagValue);
				if(debug)
					addlog(PPPOE_OPT"System-Error: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				return;
//				break;
			case TAG_GENERIC_ERROR:
				syslog(LOG_INFO, "spppd: ppp%d: PPPoE generic error: %.*s\n", p->ifno, (int) tagLen, tagValue);
				if(debug)
					addlog(PPPOE_OPT"Generic-Error: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				return;
//				break;
			case TAG_HOST_UNIQ:		// Jenny, parse TAG_HOST_UNIQ to verify packets
				memcpy(temp, (char *)tagValue, tagLen);
				temp[tagLen] = 0;
				#ifdef EMBED
				mib_get_s(MIB_ELAN_MAC_ADDR,(void *)devAddr, sizeof(devAddr));
				memset(hustr, 0, sizeof(hustr));
				memcpy(hustr, devAddr, MAC_ADDR_LEN);
				hustr[MAC_ADDR_LEN] = p->ifno;
				#else
				memcpy(hustr, g_huniq, 3);
				#endif
				if (debug) {
					_pppoe_hex2str(huniq_str, temp, tagLen);
					addlog(PPPOE_OPT"Host-Uniq: %s\n", PPPOE_ARGS(p->ifno), huniq_str);
				}
				if (memcmp(temp, hustr, tagLen))
					return;
		}
		curTag = curTag + 4 + tagLen;
	}
	if(pc->acNameOK && pc->serviceNameOK && (strlen(p->ServiceName) == 0))
		strncpy(p->ServiceName, tmpServiceName, sizeof(p->ServiceName)-1);
}

#if defined(CONFIG_USER_PPPOE_PROXY) || defined(CONFIG_USER_PPPOE_SERVER)
void parsePADRTags(PPPOEPROXY_DRV_CTRL *p, struct PPPoEPacket *packet)
{
	unsigned char *curTag, *tagValue;
	unsigned short tagType, tagLen;
	char temp[100];
	char *rpfcn = "[parsePADRTags]";
	int debug = p->debug;
	int maxlen, pktlen;

	memset(p, 0, sizeof(PPPOEPROXY_DRV_CTRL));
	memcpy(p->peerEthAddr, packet->ehdr.from.octet, ETH_ALEN);
	pktlen = ntohs(packet->length);
	maxlen = (pktlen < (RECV_LEN-Ehdr_SZ-6)? pktlen:(RECV_LEN-Ehdr_SZ-6));
	if (maxlen < pktlen)
		printf("PPPoE payload overflow: %d\n", pktlen);

	curTag = packet->payload;
	while((curTag - packet->payload) < maxlen)
	{
		int mylen;
		tagType = (((unsigned short) curTag[0]) << 8) + (unsigned short) curTag[1];
		tagLen = (((unsigned short) curTag[2]) << 8) + (unsigned short) curTag[3];
		if (tagType == TAG_END_OF_LIST) return;
		tagValue = curTag+4;

		mylen = curTag+4+tagLen - packet->payload;
		if (mylen > maxlen) {
			break;
		}

		switch(tagType){
			case TAG_SERVICE_NAME:
				if (tagLen > 0 && tagLen < 100){
					memcpy(p->ServiceName, (char *)tagValue, tagLen);
				}
				break;
			case TAG_RELAY_SESSION_ID:
				if (tagLen > 0 && tagLen < 100){
					p->relayId.type = htons(TAG_RELAY_SESSION_ID);
					p->relayId.length = htons(tagLen);
					memcpy(p->relayId.payload, (char *)tagValue, tagLen);
				}
				break;
			case TAG_HOST_UNIQ:
				if (tagLen > 0 && tagLen < 100){
					p->hostUniq.type = htons(TAG_HOST_UNIQ);
					p->hostUniq.length = htons(tagLen);
					memcpy(p->hostUniq.payload, (char *)tagValue, tagLen);
				}
				break;
			case TAG_AC_COOKIE:
				if (tagLen > 0 && tagLen < 100){
					p->cookie.type = htons(TAG_AC_COOKIE);
					p->cookie.length = htons(tagLen);
					memcpy(p->cookie.payload, (char *)tagValue, tagLen);
				}
				break;
		}
		curTag = curTag + 4 + tagLen;
	}

}
#endif

/**********************************************************************
*%FUNCTION: parsePADSTags
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data
* extra -- extra user data
*%RETURNS: 1 correct, 0 something error
*%DESCRIPTION: Picks interesting tags out of a PADS packet
***********************************************************************/

int parsePADSTags(PPPOE_DRV_CTRL *p, struct PPPoEPacket *packet)
{
	unsigned char *curTag, *tagValue;
	unsigned short tagType, tagLen;
	int debug = p->debug;
	char *rpfcn = "[parsePADSTags]";
	#ifdef EMBED
	unsigned char devAddr[MAC_ADDR_LEN];
	#endif
	char hustr[MAC_ADDR_LEN+1], temp[100], huniq_str1[64], huniq_str2[64];
	// Kaohj
	int maxlen, pktlen;
	int gotCorrectHostUniq = 0;

	// Kaohj ---- 6: pppoe header length
	pktlen = ntohs(packet->length);
	maxlen = (pktlen < (RECV_LEN-Ehdr_SZ-6)? pktlen:(RECV_LEN-Ehdr_SZ-6));
	if (maxlen < pktlen)
		printf("PPPoE payload overflow: %d\n", pktlen);
	//printf("maxlen=%d\n", maxlen);
	curTag = packet->payload;
	// Kaohj
	//while((curTag - packet->payload) < packet->length)
	while((curTag - packet->payload) < maxlen)
	{
		// Kaohj
		int mylen;
		tagType = (((unsigned short) curTag[0]) << 8) + (unsigned short) curTag[1];
		tagLen = (((unsigned short) curTag[2]) << 8) + (unsigned short) curTag[3];
		if (tagType == TAG_END_OF_LIST)
		{
			if(gotCorrectHostUniq==0)
			{
				if(debug)
					addlog(PPPOE_OPT"Host-Uniq-Error: Not got Host-Uniq Tag\n", PPPOE_ARGS(p->ifno));
				return 0;
			}
			break;
		}

		tagValue = curTag+4;
		// Kaohj
		mylen = curTag+4+tagLen - packet->payload;
		if (mylen > maxlen) {
			//printf("mylen exceeds\n");
			break;
		}

		switch(tagType) {
			case TAG_AC_NAME:
				if(debug)
					addlog(PPPOE_OPT"AC-Name: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				break;
			case TAG_SERVICE_NAME:
				if(debug)
					addlog(PPPOE_OPT"Service-Name: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				break;
			case TAG_SERVICE_NAME_ERROR:
				syslog(LOG_INFO, "spppd: ppp%d: PPPoE service name error: %.*s\n", p->ifno, (int) tagLen, tagValue);
				if(debug)
					addlog(PPPOE_OPT"Service-Name-Error: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				//curTag = curTag + 4 + tagLen;
				return 0;
				break;
			case TAG_AC_SYSTEM_ERROR:
				syslog(LOG_INFO, "spppd: ppp%d: PPPoE AC system error: %.*s\n", p->ifno, (int) tagLen, tagValue);
				if(debug)
					addlog(PPPOE_OPT"System-Error: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				//curTag = curTag + 4 + tagLen;

				if(strstr(tagValue, "No client slots available"))
				{
					if(p->sppp)
					{
						syslog(LOG_INFO, "spppd: ppp%d: kick out lan pppoe dial account:%s\n", p->ifno, p->sppp->myauth.name);
#ifdef CONFIG_PPPOE_MONITOR
						kick_pppoe_monitor_client(p->name, p->sppp->myauth.name);
#endif
					}
					struct sppp *sp=0;
					sp = find_sp(p);
					if(sp) {
						sppp_last_connection_error(sp->if_unit, ERROR_SERVER_OUT_OF_RESOURCES, NULL);
					}
				}

				return 0;
				break;
			case TAG_GENERIC_ERROR:
				syslog(LOG_INFO, "spppd: ppp%d: PPPoE generic error: %.*s\n", p->ifno, (int) tagLen, tagValue);
				if(debug)
					addlog(PPPOE_OPT"Generic-Error: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				//curTag = curTag + 4 + tagLen;
				return 0;
				break;
			case TAG_RELAY_SESSION_ID:
				p->relayId.type = htons(tagType);
				p->relayId.length = htons(tagLen);
				memcpy(p->relayId.payload, tagValue, tagLen);
				break;
			case TAG_HOST_UNIQ:		// Jenny, parse TAG_HOST_UNIQ to verify packets
				memcpy(temp, (char *)tagValue, tagLen);
				temp[tagLen] = 0;
				#ifdef EMBED
				mib_get_s(MIB_ELAN_MAC_ADDR,(void *)devAddr, sizeof(devAddr));
				memset(hustr, 0, sizeof(hustr));
				memcpy(hustr, devAddr, MAC_ADDR_LEN);
				hustr[MAC_ADDR_LEN] = p->ifno;
				#else
				memcpy(hustr, g_huniq, 3);
				#endif
				if (debug) {
					_pppoe_hex2str(huniq_str1, temp, tagLen);
					addlog(PPPOE_OPT"Host-Uniq: %s\n", PPPOE_ARGS(p->ifno), huniq_str1);
				}
				if (memcmp(temp, hustr, tagLen)) {
					if (debug) {
						_pppoe_hex2str(huniq_str2, hustr, tagLen);
						addlog(PPPOE_OPT"Host-Uniq not match: [%s] vs [%s]\n", PPPOE_ARGS(p->ifno), huniq_str1, huniq_str2);
					}
					return 0;
				}
				else
					gotCorrectHostUniq = 1;
				break;
		}
		curTag = curTag + 4 + tagLen;
	}
	return 1;
}

/**********************************************************************
*%FUNCTION: parseLogErrs
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data
* extra -- extra user data
*%RETURNS: Nothing
*%DESCRIPTION: Picks error tags out of a packet and logs them.
***********************************************************************/
void parsePADTTags(PPPOE_DRV_CTRL *p, struct PPPoEPacket *packet)
{
	unsigned char *curTag, *tagValue;
	unsigned short tagType, tagLen;
	int debug = p->debug;
	char *rpfcn = "[parsePADTTags]";
	// Kaohj
	int maxlen, pktlen;

	// Kaohj ---- 6: pppoe header length
	pktlen = ntohs(packet->length);
	maxlen = (pktlen < (RECV_LEN-Ehdr_SZ-6)? pktlen:(RECV_LEN-Ehdr_SZ-6));
	if (maxlen < pktlen)
		printf("PPPoE payload overflow: %d\n", pktlen);
	//printf("maxlen=%d\n", maxlen);
	curTag = packet->payload;
	// Kaohj
	//while((curTag - packet->payload) < packet->length)
	while((curTag - packet->payload) < maxlen)
	{
		// Kaohj
		int mylen;
		tagType = (((unsigned short) curTag[0]) << 8) + (unsigned short) curTag[1];
		tagLen = (((unsigned short) curTag[2]) << 8) + (unsigned short) curTag[3];
		if (tagType == TAG_END_OF_LIST) return;
		tagValue = curTag+4;
		// Kaohj
		mylen = curTag+4+tagLen - packet->payload;
		if (mylen > maxlen) {
			//printf("mylen exceeds\n");
			break;
		}

		switch(tagType)
		{
			case TAG_SERVICE_NAME_ERROR:
				if(debug)
					addlog(PPPOE_OPT"Service-Name-Error: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				return;
//				break;
			case TAG_AC_SYSTEM_ERROR:
				if(debug)
					addlog(PPPOE_OPT"System-Error: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				return;
//				break;
			case TAG_GENERIC_ERROR:
				if(debug)
					addlog(PPPOE_OPT"Generic-Error: %.*s\n", PPPOE_ARGS(p->ifno), (int) tagLen, tagValue);
				return;
//				break;
		}
		curTag = curTag + 4 + tagLen;
	}
}

int pppoe_change_state(PPPOE_DRV_CTRL *p, int state)
{
	p->DiscoveryState = state;
	ppp_status();
#ifdef CONFIG_USER_PPPOE_SERVER
	if (pppoe_server_onoff == 1)
		session_status();
#endif
	return 0;
}

#if defined(CONFIG_USER_PPPOE_PROXY) || defined(CONFIG_USER_PPPOE_SERVER)
void genCookie(unsigned char const *peerEthAddr,
	  unsigned char const *myEthAddr,
	  unsigned char const *seed,
	  unsigned char *cookie)
{
    struct MD5Context ctx;

    MD5Init(&ctx);
    MD5Update(&ctx, peerEthAddr, ETH_ALEN);
    MD5Update(&ctx, myEthAddr, ETH_ALEN);
    MD5Update(&ctx, seed, SEED_LEN);
    MD5Final(cookie, &ctx);
}
#endif

#ifdef CONFIG_NEC_PPP_CUSTOMIZATION
int PADI_INTERVAL[] = {0, 5, 10, 20, 27};
#endif

/***********************************************************************
*%FUNCTION: sendPADI
*%ARGUMENTS: None
*%RETURNS: 0 success, 1 fail
*%DESCRIPTION: Sends a PADI packet
***********************************************************************/
void sendPADI(PPPOE_DRV_CTRL *p)
{
//	unsigned char pkt_buf[256];
	unsigned char pkt_buf[RECV_LEN] = {0};
	struct PPPoEPacket *packet;
	unsigned char *cursor;
	struct PPPoETag *svc;
#ifdef PPPOE_EOL_TAG
	struct PPPoETag eol;	//EOL
#endif
	struct PPPoETag acName;

	unsigned short namelen = 0;
	unsigned short plen=0;
	int len;
	int debug = p->debug;
	char *rpfcn = "[sendPADI]";
	char hustr[MAC_ADDR_LEN+1];

	//DEBUG_SIMU("[%s %d]---->Send PADI(%s)<----\n", __func__, __LINE__, p->name);
	packet = (struct PPPoEPacket *)pkt_buf;
	svc = (struct PPPoETag *)packet->payload;
	cursor = packet->payload;
//	svc = (struct PPPoETag *)(&packet->payload);

	if (p->ServiceName)	// Jenny
		namelen = (unsigned short)strlen(p->ServiceName);
	plen = TAG_HDR_SIZE + namelen;
	CHECK_ROOM(cursor, packet->payload, plen);	// Jenny

	memset(p->remote.sll_addr, 0xff, ETH_ALEN);
	memcpy(&packet->ehdr.to, bcastAddr, Eid_SZ);
	memcpy(&packet->ehdr.from, p->MyEthAddr, Eid_SZ);
//	bcopy((char*)bcastAddr, (char*)&packet->ehdr.to, Eid_SZ);
//	bcopy((char*)p->MyEthAddr, (char*)&packet->ehdr.from, Eid_SZ);
	packet->ehdr.type = htons(ETH_PPPOE_DISCOVERY);

	packet->ver_type = 0x11;
	packet->code = CODE_PADI;
	packet->session = 0;

	svc->type = htons(TAG_SERVICE_NAME);
	svc->length = htons(namelen);
	CHECK_ROOM(cursor, packet->payload, namelen + TAG_HDR_SIZE);	// Jenny
	if (p->ServiceName)	// Jenny
		memcpy(svc->payload, p->ServiceName, namelen);
//	bcopy((char*)p->ServiceName, (char*)svc->payload, strlen(p->ServiceName));
	cursor += namelen + TAG_HDR_SIZE;
	
	//add AC-Name
	if (p->DesiredACName)
	{
		namelen = (unsigned short)strlen(p->DesiredACName);
		if(namelen>0)
		{
			acName.type=htons(TAG_AC_NAME);
			acName.length=htons(namelen);
			memcpy(acName.payload, p->DesiredACName, namelen);
			CHECK_ROOM(cursor, packet->payload, namelen + TAG_HDR_SIZE);
			memcpy(cursor, &acName, namelen + TAG_HDR_SIZE);
			cursor += namelen + TAG_HDR_SIZE;
			plen += namelen + TAG_HDR_SIZE;			
		}
	}

	// Jenny, add TAG_HOST_UNIQ to better support multiple PPPoE sessions
	{
		#ifdef EMBED
		unsigned char devAddr[MAC_ADDR_LEN];
		#endif
		struct PPPoETag hostUniq;
		unsigned short hulen = 0;

		hostUniq.type = htons(TAG_HOST_UNIQ);
		#ifdef EMBED
		mib_get_s(MIB_ELAN_MAC_ADDR,(void *)devAddr, sizeof(devAddr));
		memset(hustr, 0, sizeof(hustr));
		memcpy(hustr, devAddr, MAC_ADDR_LEN);
		hustr[MAC_ADDR_LEN] = p->ifno;
		hulen = MAC_ADDR_LEN + 1;
		#else
		memcpy(hustr, g_huniq, 3);
		hulen = 3;
		#endif
		hostUniq.length = htons((unsigned int)hulen);
		memcpy(hostUniq.payload, hustr, hulen);
		CHECK_ROOM(cursor, packet->payload, hulen + TAG_HDR_SIZE);
		memcpy(cursor, &hostUniq, hulen + TAG_HDR_SIZE);
		cursor += hulen + TAG_HDR_SIZE;
		plen += hulen + TAG_HDR_SIZE;
	}

    // If we're using Host-Uniq, copy it over
    /*if (p->optUseHostUnique) {
		struct PPPoETag hostUniq;
		hostUniq.type = htons(TAG_HOST_UNIQ);
		hostUniq.length = htons(sizeof(cur_task));
		memcpy(hostUniq.payload, &cur_task, sizeof(cur_task));
		memcpy(cursor, &hostUniq, sizeof(cur_task) + TAG_HDR_SIZE);
		plen += sizeof(cur_task) + TAG_HDR_SIZE;
    }*/

#ifdef PPPOE_EOL_TAG	// EOL
    	// Jenny, support Tag End-Of-List
	eol.type = htons(TAG_END_OF_LIST);
	eol.length = htons(0);
	memcpy(cursor, &eol, TAG_HDR_SIZE);
	plen += TAG_HDR_SIZE;
#endif

	packet->length = htons(plen);
	//len = (int) (plen + PPPOE_HDR_SIZE);
	len = (int) (plen + PPPOE_OVERHEAD);
	//PPPOE_LOWER_SEND(p, packet, len);
	PPPOE_LOWER_SEND(p, (unsigned char*)packet+Ehdr_SZ, len);

#if defined(CONFIG_USER_PPPOE_CTC_DELAY_INT) || defined (CONFIG_USER_PPPOE_2_INTERVALS_RETRY)
	struct timeval timenow;
	gettimeofday(&timenow, NULL);
	send_padi_time[p->ifno] = timenow.tv_sec;
	send_padi_mtime[p->ifno] = timenow.tv_usec/1000;
	send_padt_time[p->ifno] = 0;
	send_padt_mtime[p->ifno] = 0;
#endif

#ifdef PPPOE_UNIVERSAL_RETRY_INTERVAL
	padi_start_time[p->ifno] = getSYSInfoTimer();
#endif

#ifdef CONFIG_NEC_PPP_CUSTOMIZATION
	disc_counter[p->ifno]++;

	int idx = disc_counter[p->ifno];
	if (idx >= 4)
		p->timeout = PADI_INTERVAL[4];
	else
		p->timeout = PADI_INTERVAL[idx];

	TIMEOUT((void *)sendPADI, (void *)(p), p->timeout, p->ch);	

#else
#if  !defined(CONFIG_USER_PPPOE_2_INTERVALS_RETRY) && !defined(CONFIG_USER_PPPOE_CTC_DELAY_INT) && !defined(PPPOE_UNIVERSAL_RETRY_INTERVAL)
	TIMEOUT((void *)sendPADI, (void *)(p), p->timeout, p->ch);
#ifdef CONFIG_TELMEX_DEV
	if (p->timeout < 32)
		p->timeout *= 2;
#else
	p->timeout *= 2;
	if (p->timeout >= 16)
		p->timeout = 1;
#endif
#endif
#endif

	if (debug)
	{
		/*addlog(PPPOE_FMT"O PADI [%s] from ", PPPOE_ARGS(p->ifno), _pppoe_state_name(p->DiscoveryState));
		_pppoe_print_mac((char*)&packet->ehdr.from);
		addlog("\n");*/
		char mac[18];
		char huniq_str[64];
		mac[0] = 0;
		_pppoe_mac(mac, (char*)&packet->ehdr.from);
		addlog(PPPOE_FMT"O PADI [%s] from %s\n", PPPOE_ARGS(p->ifno), _pppoe_state_name(p->DiscoveryState), mac);
		addlog(PPPOE_OPT"Service-Name: %s\n", PPPOE_ARGS(p->ifno), p->ServiceName);
		#ifdef EMBED
		_pppoe_hex2str(huniq_str, hustr, MAC_ADDR_LEN+1);
		#else
		_pppoe_hex2str(huniq_str, hustr, 3);
		#endif
		addlog(PPPOE_OPT"Host-Uniq: %s\n", PPPOE_ARGS(p->ifno), huniq_str);
	}

#ifdef CONFIG_CMCC
	//when start send PADI, record ppp_error_log to ppp_error_log_old
	va_cmd("/sbin/cp", 2, 1, "/tmp/ppp_error_log", "/tmp/ppp_error_log_old");
#endif
	
	pppoe_change_state(p, PPPOE_STATE_SENT_PADI);
#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingNumberEnable_) \
|| defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingErrorEnable_)
	pppoe_dialing_info_update(DIALING_NUMBER);
#endif
	return;
}

#if defined(CONFIG_USER_PPPOE_PROXY) || defined(CONFIG_USER_PPPOE_SERVER)
void sendPADO(PPPOEPROXY_DRV_CTRL *p, struct sockaddr_ll *addr)
{
	unsigned char pkt_buf[RECV_LEN];
	struct PPPoEPacket *packet;
	struct PPPoETag acname;
    struct PPPoETag servname;
    struct PPPoETag cookie;
    unsigned short acname_len;
	unsigned short servname_len;
	unsigned char *cursor;
	unsigned short plen = 0;
	unsigned short len = 0;
	char *rpfcn = "[sendPADO]";
	int ret;

	packet = (struct PPPoEPacket *)pkt_buf;
	cursor =  packet->payload;

	memcpy(&packet->ehdr.to, p->peerEthAddr, Eid_SZ);
	memcpy(&packet->ehdr.from, localhwAddr, Eid_SZ);

	packet->ehdr.type = htons(ETH_PPPOE_DISCOVERY);
	packet->ver_type = 0x11;
	packet->code = CODE_PADO;
	packet->session = 0;

	acname.type = htons(TAG_AC_NAME);
    acname_len = strlen(ACName);
    acname.length = htons(acname_len);
    memcpy(acname.payload, ACName, sizeof(acname.payload));

	if (p->ServiceName)
		servname_len = (unsigned short)strlen(p->ServiceName);
	servname.type = htons(TAG_SERVICE_NAME);
	servname.length = htons(servname_len);
	memcpy(servname.payload, p->ServiceName, servname_len);

	cookie.type = htons(TAG_AC_COOKIE);
    cookie.length = htons(16);		/* MD5 output is 16 bytes */
    genCookie(packet->ehdr.to.octet, localhwAddr, CookieSeed, cookie.payload);

	CHECK_ROOM(cursor, packet->payload, acname_len+TAG_HDR_SIZE);
    memcpy(cursor, &acname, acname_len + TAG_HDR_SIZE);
    cursor += acname_len + TAG_HDR_SIZE;
	plen += TAG_HDR_SIZE + acname_len;

    CHECK_ROOM(cursor, packet->payload, servname_len+TAG_HDR_SIZE);
    memcpy(cursor, &servname, TAG_HDR_SIZE);
    cursor += TAG_HDR_SIZE;
    plen += TAG_HDR_SIZE;

    CHECK_ROOM(cursor, packet->payload, TAG_HDR_SIZE + 16);
    memcpy(cursor, &cookie, TAG_HDR_SIZE + 16);
    cursor += TAG_HDR_SIZE + 16;
    plen += TAG_HDR_SIZE + 16;

	if (p->relayId.type) {
		CHECK_ROOM(cursor, packet->payload, ntohs(p->relayId.length) + TAG_HDR_SIZE);
		memcpy(cursor, &p->relayId, ntohs(p->relayId.length) + TAG_HDR_SIZE);
		cursor += ntohs(p->relayId.length) + TAG_HDR_SIZE;
		plen += ntohs(p->relayId.length) + TAG_HDR_SIZE;
    }

    if (p->hostUniq.type) {
		CHECK_ROOM(cursor, packet->payload, ntohs(p->hostUniq.length)+TAG_HDR_SIZE);
		memcpy(cursor, &p->hostUniq, ntohs(p->hostUniq.length) + TAG_HDR_SIZE);
		cursor += ntohs(p->hostUniq.length) + TAG_HDR_SIZE;
		plen += ntohs(p->hostUniq.length) + TAG_HDR_SIZE;
		unsigned char *c = cursor - 12;
    }

    packet->length = htons(plen);
	len = (int) (plen + PPPOE_HDR_SIZE);

	ret = sendto(pppoe_server_socket, packet, len+Ehdr_SZ, 0, (struct sockaddr*)addr, sizeof(struct sockaddr_ll));
	if(ret < 0){
		printf("send PADO error! ret = %d\n", ret);
		perror("Could not send data: ");
	}

}
#endif

/***********************************************************************
*%FUNCTION: sendPADR
*%ARGUMENTS: None
*%RETURNS: 0 success 1 fail
*%DESCRIPTION: Sends a PADR packet
***********************************************************************/
void sendPADR(PPPOE_DRV_CTRL *p)
{
//	unsigned char pkt_buf[256];
	unsigned char pkt_buf[RECV_LEN] = {0};
	struct PPPoEPacket *packet;
	struct PPPoETag *svc;
	unsigned char *cursor;
	unsigned short namelen = 0;
	unsigned short plen=0;
	int len;
	struct PPPoETag acName;

	int debug = p->debug;
	char *rpfcn = "[sendPADR]";
	char hustr[MAC_ADDR_LEN+1];

	packet = (struct PPPoEPacket *)pkt_buf;
	svc = (struct PPPoETag *)packet->payload;
	cursor =  packet->payload;
	if (p->ServiceName)	// Jenny
		namelen = (unsigned short)strlen(p->ServiceName);
	plen = TAG_HDR_SIZE + namelen;
	CHECK_ROOM(cursor, packet->payload, plen);	// Jenny

	memcpy(&packet->ehdr.to, p->ACEthAddr, Eid_SZ);
	memcpy(&packet->ehdr.from, p->MyEthAddr, Eid_SZ);
//	bcopy((char*)p->ACEthAddr, (char*)&packet->ehdr.to, Eid_SZ);
//	bcopy((char*)p->MyEthAddr, (char*)&packet->ehdr.from, Eid_SZ);

	packet->ehdr.type = htons(ETH_PPPOE_DISCOVERY);
	packet->ver_type = 0x11;
	packet->code = CODE_PADR;
	packet->session = 0;

	svc->type = htons(TAG_SERVICE_NAME);
	svc->length = htons(namelen);
	if (p->ServiceName)	// Jenny
		memcpy(svc->payload, p->ServiceName, namelen);
//	bcopy((char*)p->ServiceName, (char*)svc->payload, strlen(p->ServiceName));

	cursor += namelen + TAG_HDR_SIZE;
	//add AC-Name
	if (p->DesiredACName)
	{
		namelen = (unsigned short)strlen(p->DesiredACName);
		if(namelen>0)
		{
			acName.type=htons(TAG_AC_NAME);
			acName.length=htons(namelen);
			memcpy(acName.payload, p->DesiredACName, namelen);
			CHECK_ROOM(cursor, packet->payload, namelen + TAG_HDR_SIZE);
			memcpy(cursor, &acName, namelen + TAG_HDR_SIZE);
			cursor += namelen + TAG_HDR_SIZE;
			plen += namelen + TAG_HDR_SIZE; 		
		}
	}

	// Jenny, add TAG_HOST_UNIQ to better support multiple PPPoE sessions
	{
		#ifdef EMBED
		unsigned char devAddr[MAC_ADDR_LEN];
		#endif
		struct PPPoETag hostUniq;
		unsigned short hulen = 0;

		hostUniq.type = htons(TAG_HOST_UNIQ);
		#ifdef EMBED
		mib_get_s(MIB_ELAN_MAC_ADDR,(void *)devAddr, sizeof(devAddr));
		memset(hustr, 0, sizeof(hustr));
		memcpy(hustr, devAddr, MAC_ADDR_LEN);
		hustr[MAC_ADDR_LEN] = p->ifno;
		hulen = MAC_ADDR_LEN + 1;
		#else
		memcpy(hustr, g_huniq, 3);
		hulen = 3;
		#endif
		hostUniq.length = htons((unsigned int)hulen);
		memcpy(hostUniq.payload, hustr, hulen);
		CHECK_ROOM(cursor, packet->payload, hulen + TAG_HDR_SIZE);
		memcpy(cursor, &hostUniq, hulen + TAG_HDR_SIZE);
		cursor += hulen + TAG_HDR_SIZE;
		plen += hulen + TAG_HDR_SIZE;
	}

    /* If we're using Host-Uniq, copy it over */
    /*if (p->optUseHostUnique) {
		struct PPPoETag hostUniq;
		hostUniq.type = htons(TAG_HOST_UNIQ);
		hostUniq.length = htons(sizeof(cur_task));
		memcpy(hostUniq.payload, &cur_task, sizeof(cur_task));
		memcpy(cursor, &hostUniq, sizeof(cur_task) + TAG_HDR_SIZE);
		plen += sizeof(cur_task) + TAG_HDR_SIZE;
    }*/

	/* Copy cookie and relay-ID if needed */
	if (p->cookie.type) {
		CHECK_ROOM(cursor, packet->payload, ntohs(p->cookie.length) + TAG_HDR_SIZE);	// Jenny
		memcpy(cursor, &p->cookie, ntohs(p->cookie.length) + TAG_HDR_SIZE);
		cursor += ntohs(p->cookie.length) + TAG_HDR_SIZE;
		plen += ntohs(p->cookie.length) + TAG_HDR_SIZE;
	}

	if (p->relayId.type) {
		CHECK_ROOM(cursor, packet->payload, ntohs(p->relayId.length) + TAG_HDR_SIZE);	// Jenny
		memcpy(cursor, &p->relayId, ntohs(p->relayId.length) + TAG_HDR_SIZE);
		cursor += ntohs(p->relayId.length) + TAG_HDR_SIZE;
		plen += ntohs(p->relayId.length) + TAG_HDR_SIZE;
	}

	packet->length = htons(plen);
	//len = (int) (plen + PPPOE_HDR_SIZE);
	len = (int) (plen + PPPOE_OVERHEAD);
	//PPPOE_LOWER_SEND(p, packet, len);
	PPPOE_LOWER_SEND(p, (unsigned char*)packet+Ehdr_SZ, len);
	TIMEOUT((void *)sendPADR, (void *)p, p->timeout, p->ch);

	if (debug)
	{
		/*addlog(PPPOE_FMT"O PADR [%s] from ", PPPOE_ARGS(p->ifno), _pppoe_state_name(p->DiscoveryState));
		_pppoe_print_mac((char*)&packet->ehdr.from);
		addlog(" to ");
		_pppoe_print_mac((char*)&packet->ehdr.to);
		addlog("\n");*/
		char mac1[18], mac2[18], huniq_str[64];
		mac1[0] = mac2[0] = 0;
		_pppoe_mac(mac1, (char*)&packet->ehdr.from);
		_pppoe_mac(mac2, (char*)&packet->ehdr.to);
		addlog(PPPOE_FMT"O PADR [%s] from %s to %s\n", PPPOE_ARGS(p->ifno), _pppoe_state_name(p->DiscoveryState), mac1, mac2);
		addlog(PPPOE_OPT"Service-Name: %s\n", PPPOE_ARGS(p->ifno), p->ServiceName);
		#ifdef EMBED
		_pppoe_hex2str(huniq_str, hustr, MAC_ADDR_LEN+1);
		#else
		_pppoe_hex2str(huniq_str, hustr, 3);
		#endif
		addlog(PPPOE_OPT"Host-Uniq: %s\n", PPPOE_ARGS(p->ifno), huniq_str);
	}

	sendPADR_flag[p->ifno] = 1;
	pppoe_change_state(p, PPPOE_STATE_SENT_PADR);

	p->timeout *= 2;

	/* when open CONFIG_USER_PPPOE_2_INTERVALS_RETRY|CONFIG_USER_PPPOE_CTC_DELAY_INT|PPPOE_UNIVERSAL_RETRY_INTERVAL
	   do not call pppoe_down when client always can not get PADS from server. 
	   because if p->disc_ch's function (discTimeout) have not excute when pppoe_down delete timer p->disc_ch, 
	   p->disc_timeout will always equal 0, and pppoe never send PADI in process_poe again!
	*/
#if  !defined(CONFIG_USER_PPPOE_2_INTERVALS_RETRY) && !defined(CONFIG_USER_PPPOE_CTC_DELAY_INT) && !defined(PPPOE_UNIVERSAL_RETRY_INTERVAL)
	if (p->timeout >= 16) {
		padrsent[p->ifno] = 1;
		p->timeout = 1;
#ifdef CONFIG_SUPPORT_AUTO_DIAG
		if(0 == p->simulation)
#endif	
		{
			pppoe_change_state(p, PPPOE_STATE_IDLE);
		}
		UNTIMEOUT(0, 0, p->ch);
#ifdef CONFIG_SUPPORT_AUTO_DIAG
		if(0 == p->simulation)
#endif	
		{
			TIMEOUT((void *)pppoe_redial, (void *)p, p->timeout, p->ch);
		}
	}
#endif

}

#if defined(CONFIG_USER_PPPOE_PROXY) || defined(CONFIG_USER_PPPOE_SERVER)
void sendErrorPADS(PPPOEPROXY_DRV_CTRL *p, struct sockaddr_ll *dst, int errorTag, char *errorMsg)
	{
	unsigned char pkt_buf[sizeof(struct PPPoEPacket)]={0};
	struct PPPoEPacket *pads;
	unsigned char *cursor;
	unsigned char payload[ET_MAXLEN]={0};
	unsigned int plen = 0;
	unsigned int len = 0;
	struct PPPoETag err={0};
	int ret;

	int elen = strlen(errorMsg);
	pads = (struct PPPoEPacket *)pkt_buf;
	cursor = payload;
	memcpy(&pads->ehdr.to, p->peerEthAddr, Eid_SZ);
	memcpy(&pads->ehdr.from, localhwAddr, Eid_SZ);
	pads->ehdr.type = htons(ETH_PPPOE_DISCOVERY);
	pads->ver_type = 0x11;
	pads->code = CODE_PADS;

	pads->session = htons(0);
	plen = 0;

	err.type = htons(errorTag);
	err.length = htons(elen);

	memcpy(err.payload, errorMsg, sizeof(err.payload));
	memcpy(cursor, &err, (TAG_HDR_SIZE+elen)>=ET_MAXLEN?ET_MAXLEN:(TAG_HDR_SIZE+elen));
	cursor += TAG_HDR_SIZE + elen;
	plen += TAG_HDR_SIZE + elen;
	if (p->relayId.type) {
		len = plen + ntohs(p->relayId.length) + TAG_HDR_SIZE;
		if (len <= ET_MAXLEN)
		{
			memcpy(cursor, &p->relayId, ntohs(p->relayId.length) + TAG_HDR_SIZE);
			cursor += ntohs(p->relayId.length) + TAG_HDR_SIZE;
			plen += ntohs(p->relayId.length) + TAG_HDR_SIZE;
		}
		else
			perror("Could not send data: relayId");
	}

	if (p->hostUniq.type) {
		len = plen + ntohs(p->hostUniq.length) + TAG_HDR_SIZE;
		if (len <= ET_MAXLEN)
		{
			memcpy(cursor, &p->hostUniq, ntohs(p->hostUniq.length) + TAG_HDR_SIZE);
			cursor += ntohs(p->hostUniq.length) + TAG_HDR_SIZE;
			plen += ntohs(p->hostUniq.length) + TAG_HDR_SIZE;
		}
		else
			perror("Could not send data: hostUniq");
	}

	if(plen + PPPOE_HDR_SIZE + Ehdr_SZ < sizeof(struct PPPoEPacket))
	{
		memcpy(pads->payload, cursor, plen<ET_MAXLEN?plen:ET_MAXLEN);
		pads->length = htons(plen);
		len = plen + PPPOE_HDR_SIZE + Ehdr_SZ;
		if (len > sizeof(struct PPPoEPacket))
			len = sizeof(struct PPPoEPacket);
		ret = sendto(pppoe_server_socket, pads, len, 0, (struct sockaddr*)dst, sizeof(struct sockaddr_ll));
		if(ret < 0){
			printf("send errorPADS error! ret = %d\n", ret);
			perror("Could not send data: ");
		}
	}
	else
	{
		printf("send errorPADS error! oversize packets\n");
		perror("Could not send data: oversize packets!");
	}
}

void sendServerPADT(struct sppp *sp)
{
	unsigned char pkt_buf[RECV_LEN];
	struct PPPoEPacket *packet;
#ifdef PPPOE_EOL_TAG
	struct PPPoETag eol;	// EOL
#endif
	unsigned char *cursor;
	unsigned short plen = 0;
	int len;
	char *rpfcn = "[sendServerPADT]";
	int ret;
	PPPOEPROXY_DRV_CTRL *p = sp->pp_lowerp;

	if (!p->session) {
		return ;
	}

	packet = (struct PPPoEPacket *)pkt_buf;
	cursor = packet->payload;

	memcpy(&packet->ehdr.to, p->peerEthAddr, Eid_SZ);
	memcpy(&packet->ehdr.from, localhwAddr, Eid_SZ);

	packet->ehdr.type = htons(ETH_PPPOE_DISCOVERY);
	packet->ver_type = 0x11;
	packet->code = CODE_PADT;
	packet->session = htons(p->session);

#ifdef CONFIG_USER_PPPOE_SERVER
	freeSession(p->session);
#endif

	/* Reset Session to zero so there is no possibility of
       recursive calls to this function by any signal handler */
	p->session = 0;

	/* Copy cookie and relay-ID if needed */
	if (p->cookie.type) {
		CHECK_ROOM(cursor, packet->payload, ntohs(p->cookie.length) + TAG_HDR_SIZE);
		memcpy(cursor, &p->cookie, ntohs(p->cookie.length) + TAG_HDR_SIZE);
		cursor += ntohs(p->cookie.length) + TAG_HDR_SIZE;
		plen += ntohs(p->cookie.length) + TAG_HDR_SIZE;
	}

	if (p->relayId.type) {
		CHECK_ROOM(cursor, packet->payload, ntohs(p->relayId.length) + TAG_HDR_SIZE);
		memcpy(cursor, &p->relayId, ntohs(p->relayId.length) + TAG_HDR_SIZE);
		cursor += ntohs(p->relayId.length) + TAG_HDR_SIZE;
		plen += ntohs(p->relayId.length) + TAG_HDR_SIZE;
	}

#ifdef PPPOE_EOL_TAG
	eol.type = htons(TAG_END_OF_LIST);
	eol.length = htons(0);
	memcpy(cursor, &eol, TAG_HDR_SIZE);
	plen += TAG_HDR_SIZE;
#endif

	packet->length = htons(plen);
	len = (int) (plen + PPPOE_HDR_SIZE);

	ret = sendto(pppoe_server_socket, packet, len+Ehdr_SZ, 0, (struct sockaddr*)&p->client, sizeof(struct sockaddr_ll));
	if(ret < 0){
		printf("send serverPADT error! ret = %d\n", ret);
		perror("Could not send data: ");
	}

	p->session = PPPOE_STATE_IDLE;
}

void sendPADS(PPPOEPROXY_DRV_CTRL *p, unsigned int sess, struct sockaddr_ll *addr)
{

	unsigned char pkt_buf[RECV_LEN];
	struct PPPoEPacket *packet;
	struct PPPoETag servname;
	struct PPPoETag cookie;
	unsigned short servname_len;
	unsigned char *cursor;
	unsigned short plen = 0;
	unsigned short len = 0;
	char *rpfcn = "[sendPADS]";
	int ret;

	packet = (struct PPPoEPacket *)pkt_buf;
	cursor =  packet->payload;

	memcpy(&packet->ehdr.to, p->peerEthAddr, Eid_SZ);
	memcpy(&packet->ehdr.from, localhwAddr, Eid_SZ);

	packet->ehdr.type = htons(ETH_PPPOE_DISCOVERY);
	packet->ver_type = 0x11;
	packet->code = CODE_PADS;
	packet->session = htons(sess);

	if (p->ServiceName)
		servname_len = (unsigned short)strlen(p->ServiceName);
	servname.type = htons(TAG_SERVICE_NAME);
	servname.length = htons(servname_len);
	memcpy(servname.payload, p->ServiceName, servname_len);

	cookie.type = htons(TAG_AC_COOKIE);
	cookie.length = htons(16);		/* MD5 output is 16 bytes */
	genCookie(packet->ehdr.to.octet, localhwAddr, CookieSeed, cookie.payload);

	CHECK_ROOM(cursor, packet->payload, servname_len+TAG_HDR_SIZE);
	memcpy(cursor, &servname, TAG_HDR_SIZE);
	cursor += TAG_HDR_SIZE + servname_len;
	plen += TAG_HDR_SIZE + servname_len;

	CHECK_ROOM(cursor, packet->payload, TAG_HDR_SIZE + 16);
	memcpy(cursor, &cookie, TAG_HDR_SIZE + 16);
	cursor += TAG_HDR_SIZE + 16;
	plen += TAG_HDR_SIZE + 16;

	if (p->relayId.type) {
		CHECK_ROOM(cursor, packet->payload, ntohs(p->relayId.length) + TAG_HDR_SIZE);
		memcpy(cursor, &p->relayId, ntohs(p->relayId.length) + TAG_HDR_SIZE);
		cursor += ntohs(p->relayId.length) + TAG_HDR_SIZE;
		plen += ntohs(p->relayId.length) + TAG_HDR_SIZE;
	}

	if (p->hostUniq.type) {
		CHECK_ROOM(cursor, packet->payload, ntohs(p->hostUniq.length)+TAG_HDR_SIZE);
		memcpy(cursor, &p->hostUniq, ntohs(p->hostUniq.length) + TAG_HDR_SIZE);
		cursor += ntohs(p->hostUniq.length) + TAG_HDR_SIZE;
		plen += ntohs(p->hostUniq.length) + TAG_HDR_SIZE;
	}

	packet->length = htons(plen);
	len = (int) (plen + PPPOE_HDR_SIZE);

	ret = sendto(pppoe_server_socket, packet, len+Ehdr_SZ, 0, (struct sockaddr*)addr, sizeof(struct sockaddr_ll));
	if(ret < 0){
		printf("send PADS error! ret = %d\n", ret);
		perror("Could not send data: ");
	}
}
#endif

/***********************************************************************
*%FUNCTION: sendPADT
*%ARGUMENTS: None
*%RETURNS: 0 success, 1 fail
*%DESCRIPTION: Sends a PADT packet
***********************************************************************/
void sendPADT(PPPOE_DRV_CTRL *p)
{
//	unsigned char pkt_buf[256];
	unsigned char pkt_buf[RECV_LEN] = {0};
	struct PPPoEPacket *packet;
#ifdef PPPOE_EOL_TAG
	struct PPPoETag eol;	// EOL
#endif
	unsigned char *cursor;
	unsigned short plen = 0;
	int len;
	int debug = p->debug;
	char *rpfcn = "[sendPADT]";

	if (!p->session)
	{
//		addlog(PPPOE_FMT"%s null session id\n", PPPOE_ARGS(p->ifno));
		return ;
	}

	packet = (struct PPPoEPacket *)pkt_buf;
	cursor = packet->payload;

	memcpy(&packet->ehdr.to, p->ACEthAddr, Eid_SZ);
	memcpy(&packet->ehdr.from, p->MyEthAddr, Eid_SZ);
//	bcopy((char*)p->ACEthAddr, (char*)&packet->ehdr.to, Eid_SZ);
//	bcopy((char*)p->MyEthAddr, (char*)&packet->ehdr.from, Eid_SZ);

	packet->ehdr.type = htons(ETH_PPPOE_DISCOVERY);
	packet->ver_type = 0x11;
	packet->code = CODE_PADT;
	packet->session = htons(p->session);

	/* Reset Session to zero so there is no possibility of
       recursive calls to this function by any signal handler */
	p->session = 0;

	/* Copy cookie and relay-ID if needed */
	if (p->cookie.type) {
		CHECK_ROOM(cursor, packet->payload, ntohs(p->cookie.length) + TAG_HDR_SIZE);	// Jenny
		memcpy(cursor, &p->cookie, ntohs(p->cookie.length) + TAG_HDR_SIZE);
//		bcopy((char*)&p->cookie, (char*)cursor, ntohs(p->cookie.length) + TAG_HDR_SIZE);
		cursor += ntohs(p->cookie.length) + TAG_HDR_SIZE;
		plen += ntohs(p->cookie.length) + TAG_HDR_SIZE;
	}

	if (p->relayId.type) {
		CHECK_ROOM(cursor, packet->payload, ntohs(p->relayId.length) + TAG_HDR_SIZE);	// Jenny
		memcpy(cursor, &p->relayId, ntohs(p->relayId.length) + TAG_HDR_SIZE);
//		bcopy((char*)&p->relayId, (char*)cursor, ntohs(p->relayId.length) + TAG_HDR_SIZE);
		cursor += ntohs(p->relayId.length) + TAG_HDR_SIZE;
		plen += ntohs(p->relayId.length) + TAG_HDR_SIZE;
	}

#ifdef PPPOE_EOL_TAG	// EOL
    	// Jenny, support Tag End-Of-List
	eol.type = htons(TAG_END_OF_LIST);
	eol.length = htons(0);
	memcpy(cursor, &eol, TAG_HDR_SIZE);
	plen += TAG_HDR_SIZE;
#endif

	packet->length = htons(plen);
	//len = (int) (plen + PPPOE_HDR_SIZE);
	len = (int) (plen + PPPOE_OVERHEAD);
	//PPPOE_LOWER_SEND(p, packet, len);
	PPPOE_LOWER_SEND(p, (unsigned char*)packet+Ehdr_SZ, len);
	p->session = PPPOE_STATE_IDLE;

	if (debug)
		addlog(PPPOE_FMT"O PADT\n", PPPOE_ARGS(p->ifno));

#ifdef CONFIG_USER_PPPOE_CTC_DELAY_INT
	struct timeval timenow;
	gettimeofday(&timenow, NULL);
	send_padt_time[p->ifno] = timenow.tv_sec;
	send_padt_mtime[p->ifno] = timenow.tv_usec/1000;
#endif
}

void pppoe_up(PPPOE_DRV_CTRL *p)
{
	int debug = p->debug;

	if(debug)
		addlog(PPPOE_FMT"Phase is Discovery\n", PPPOE_ARGS(p->ifno));
	p->timeout = PADI_TIMEOUT;
	sendPADI(p);
}

void pppoe_to(PPPOE_DRV_CTRL *p)
{
	p->timeout = PADI_TIMEOUT;
	sendPADR(p);
}

//int pppoe_send(struct ifnet *ifp);
//void pppoe_if_up(struct sppp *sp);
//#define PPP_PAP		0xc023		/* Password Authentication Protocol */
//#define PPP_CHAP	0xc223		/* Challenge-Handshake Auth Protocol */
//#include "if_sppp.h"
//extern struct sppp sppp_softc[];

int pppoe_open(PPPOE_DRV_CTRL *p)
{
//struct sppp *sp = &sppp_softc[p->ifno];
int debug = p->debug;
int retval;

	if(debug)
		addlog(PPPOE_FMT"Phase is Session\n", PPPOE_ARGS(p->ifno));

    p->sp.sa_family = AF_PPPOX;
    p->sp.sa_protocol = PX_PROTO_OE;
    p->sp.sa_addr.pppoe.sid = htons(p->session);
    memcpy(p->sp.sa_addr.pppoe.dev,p->name, IFNAMSIZ);
    memcpy(p->sp.sa_addr.pppoe.remote, p->remote.sll_addr, ETH_ALEN);

    //printf("Connecting PPPoE socket: %04x %s %p\n",
	//     p->sp.sa_addr.pppoe.sid,
	//     p->sp.sa_addr.pppoe.dev,p);

    retval = connect(p->fd, (struct sockaddr*)&p->sp,
		  sizeof(struct sockaddr_pppox));


    if( retval < 0 ){
		printf("Failed to connect PPPoE socket: %d %m",retval);
		return retval;
    }

#if defined(CONFIG_LUNA)
	//printf pppoe information for diag cmd user: Chuck
	printf("############PPPoE INFO:###############\n");
	printf("###SessionID: %d\n", p->session);
	printf("###dev_name: %s\n",p->name);

	printf("###gateway Mac: %02x:%02x:%02x:%02x:%02x:%02x\n",p->remote.sll_addr[0],p->remote.sll_addr[1],p->remote.sll_addr[2],p->remote.sll_addr[3],p->remote.sll_addr[4],p->remote.sll_addr[5]);
	printf("###  message@ %s(%s:%d)\n",__FILE__,__func__,__LINE__);
	printf("######################################\n");
#endif

	padrsent[p->ifno] = 0;
#ifdef CONFIG_PPPOE_OUT_OF_ORDER
	if(0 != p->sess_in_disc_sock)
		pppoe_remove_fd(p->sess_in_disc_sock);

	if((debug) && (p->lcp_conn_req_len))
	{
		addlog(PPPOE_FMT"p->lcp_conn_req_len is %d, p->lcp_session is 0x%x, p->session is 0x%x\n", PPPOE_ARGS(p->ifno),
		  p->lcp_conn_req_len, p->lcp_session, p->session);
	}

	//lcp request out of order will deal in sppp_lcp_scr->sppp_lcp_out_of_order.
#endif
	return 0;
}

void pppoe_down(PPPOE_DRV_CTRL *p)
{
	int debug = p->debug;

	UNTIMEOUT(0, 0, p->ch);
	UNTIMEOUT(0, 0, p->disc_ch);
	if(debug)
		addlog(PPPOE_FMT"State is Terminated, session ID: 0x%x\n", PPPOE_ARGS(p->ifno), p->session);
	pppoe_change_state(p, PPPOE_STATE_TERMINATED);
	disc_counter[p->ifno] = 0;

#ifdef CONFIG_SUPPORT_AUTO_DIAG 
	if(0 == p->simulation)
#endif
	{
		//Simulation ppp connection does not need re-dail
		pppoe_init(p);
		//pppoe_init(p->ifno, "");
		//sppp_init(p->ifno);
	}
}

void pppoe_redial(PPPOE_DRV_CTRL *p)
{
	pppoe_down(p);
	
#ifdef CONFIG_SUPPORT_AUTO_DIAG 
	if(0 == p->simulation)
#endif
	{
		//do not send PADI immediately when open PPPOE_UNIVERSAL_RETRY_INTERVAL
#if !defined(CONFIG_USER_PPPOE_CTC_DELAY_INT ) && !defined(CONFIG_USER_PPPOE_2_INTERVALS_RETRY) && !defined(PPPOE_UNIVERSAL_RETRY_INTERVAL)
		pppoe_up(p);
#endif
	}
}

extern struct sppp *spppq;
static struct sppp *find_sp(PPPOE_DRV_CTRL *p)
{
	struct sppp *sp;
	sp = spppq;
	while(sp) {
		if ((sp->over == SPPP_PPPOE) && sp->pp_lowerp == p)
			return sp;
		sp = sp->pp_next;
	}
	return sp;
}

void pppoe_cp_input(PPPOE_DRV_CTRL *p, struct PPPoEPacket *packet)
{

	struct PacketCriteria pc;
	int debug = p->debug;
	char *rpfcn = "[pppoe_cp_input]";
	char mac[18];
	#ifdef EMBED
	MIB_CE_ATM_VC_T entry;
	unsigned int totalEntry; /* get chain record size */
	#endif
	int i;

//	memset(&pc, 0, sizeof pc);
	memset(&pc, 0, sizeof(struct PacketCriteria));

	mac[0] = 0;
	_pppoe_mac(mac, (char*)&packet->ehdr.from);

	/*****************************************************************
	 * Martin ZHU--2017-01-17
	 *  When pppoe is in session state, should not receive PADO and
	 *��PADS, especially PADS.	 
	 *  For example:
	 * 		we sent 2 PADR Packets(1st is timeout), Server will 
	 *  	reply 2 PADS Packets and 2nd PADS will update p->session.
	 * 		so, MIB_PPPOE_SESSION_TBL and rg pppoe table will be error.
	 *
	 *****************************************************************/
	if ( (CODE_PADT != packet->code)&&(PPPOE_STATE_SESSION==p->DiscoveryState) )
	{
		if (debug)
		{
			printf("[%s %d]spppd: p->session=%d, packet->session=%d.\n", __func__, __LINE__, p->session, packet->session); 
			printf("[%s %d]spppd: In SESSION state, do not handle PADO and PADS.\n", __func__, __LINE__);
		}
		return;
	}
	
	switch (packet->code)
	{
		case CODE_PADO:
			if (debug)
			{
				/*addlog(PPPOE_FMT"I PADO [%s] from ", PPPOE_ARGS(p->ifno), _pppoe_state_name(p->DiscoveryState));
				_pppoe_print_mac((char*)&packet->ehdr.from);
				addlog("\n");*/
				addlog(PPPOE_FMT"I PADO [%s] from %s\n", PPPOE_ARGS(p->ifno), _pppoe_state_name(p->DiscoveryState), mac);
			}
			// Jenny, check if PADR has been sent
			if (!sendPADR_flag[p->ifno]) {
				parsePADOTags(p, packet, &pc);
				if (pc.acNameOK && pc.serviceNameOK) {
					UNTIMEOUT(sendPADI, p, p->ch);
					pppoe_change_state(p, PPPOE_STATE_RECEIVED_PADO);
					memcpy(p->ACEthAddr, &packet->ehdr.from, Eid_SZ);
					memcpy(p->remote.sll_addr, &packet->ehdr.from, Eid_SZ);
					pppoe_to(p);
				}
			}
			break;
		case CODE_PADS:
			if (debug)
			{
				/*addlog(PPPOE_FMT"I PADS [%s] from ", PPPOE_ARGS(p->ifno), _pppoe_state_name(p->DiscoveryState));
				_pppoe_print_mac((char*)&packet->ehdr.from);
				addlog("\n");*/
				addlog(PPPOE_FMT"I PADS [%s] from %s\n", PPPOE_ARGS(p->ifno), _pppoe_state_name(p->DiscoveryState), mac);
			}
			if (memcmp(&packet->ehdr.from, p->ACEthAddr, Eid_SZ))
			{
				if(debug)
					addlog(PPPOE_OPT"Invalid source addr, discard\n", PPPOE_ARGS(p->ifno));
			}
			else
			{
				if (parsePADSTags(p, packet))
				{
					UNTIMEOUT(sendPADR, p, p->ch);
					p->session = ntohs(packet->session);
					//pppoe_session_update(p);
					pppoe_change_state(p, PPPOE_STATE_SESSION);
					if (debug)
					{
						addlog(PPPOE_OPT"Session ID = %04X\n", PPPOE_ARGS(p->ifno), (int)(p->session));
						if (p->session == 0)
							addlog(PPPOE_FMT"Access concentrator used a session value of zero\n", \
								PPPOE_ARGS(p->ifno));
					}
					// for some broadband ras bug
					// it should delay msecs here
					pppoe_open(p);
					
					// update session id when get PADS
					PPPOE_SESSION_INFO p_session_info;
					p_session_info.uifno = p->uifno;
					memcpy(&p_session_info.remote,(void *) &p->remote, sizeof(struct sockaddr_ll));
					p_session_info.session= p->session;
				#ifdef CONFIG_NEC_PPP_CUSTOMIZATION
					p_session_info.disconnect_flag = 1;
					p->disconnect_flag = 1;
				#endif
					mib_set_PPPoE(CMD_MIB_SAVE_PPPOE, &p_session_info, sizeof(PPPOE_SESSION_INFO));
				}
				else
				{
					//pppoe_change_state(p, PPPOE_STATE_TERMINATED);
					if (padrsent[p->ifno] == 1) {
						pppoe_down(p);
#if !defined(CONFIG_USER_PPPOE_CTC_DELAY_INT ) && !defined(CONFIG_USER_PPPOE_2_INTERVALS_RETRY) && !defined(PPPOE_UNIVERSAL_RETRY_INTERVAL)
						pppoe_up(p);
#endif
					}
				}
			}
			break;
		case CODE_PADT:
			if (debug)
			{
				/*addlog(PPPOE_FMT"I PADT [%s] sid = %04X from ", PPPOE_ARGS(p->ifno), \
					_pppoe_state_name(p->DiscoveryState), packet->session);
					_pppoe_print_mac((char*)&packet->ehdr.from);
				addlog("\n");*/
				addlog(PPPOE_FMT"I PADT [%s] sid = %04X from %s\n", PPPOE_ARGS(p->ifno), \
					_pppoe_state_name(p->DiscoveryState), ntohs(packet->session), mac);
			}
			if (memcmp(&packet->ehdr.from, p->ACEthAddr, Eid_SZ))
			{
				if (debug)
					addlog(PPPOE_OPT"Invalid source addr, discard\n", PPPOE_ARGS(p->ifno));
			}
			else
			{
				if (ntohs(packet->session) != p->session)
				{
					if (debug)
						addlog(PPPOE_OPT"Error session ID, discard\n", PPPOE_ARGS(p->ifno));
				}
				else
				{
					// Receive PADT, force uplayer ppp down
					struct sppp *sp=0;
					sp = find_sp(p);
					if (sp) {
						if (sp->pp_phase <= PHASE_AUTHENTICATE && !auth_fail_counter[sp->unit]) {	// Jenny, for authentication fail counter
							extern unsigned int auth_fail_counter[N_SPPP];
							auth_fail_counter[sp->unit] ++;
						}
						sp->pp_down(sp);
					}
					parsePADTTags(p, packet);
					pppoe_change_state(p, PPPOE_STATE_TERMINATED);
#ifdef CONFIG_SUPPORT_AUTO_DIAG 
					if(sp != NULL)
					{
						if((SPPP_PPPOE == sp->over)&&(0 == sp->role)&&(0 != sp->simulation))
						{
							int result=DIAG_PARAMNEGOFAIL;
							int errNum=ERROR_ISP_DISCONNECT;

							if((sp->ipType==0)&&(sp->pp_phase == PHASE_NETWORK))
							{
								result = DIAG_PARAMNEGOFAIL_IPV4;
								errNum = ERROR_ISP_DISCONNECT_IPv4;
							}
							else if((sp->ipType==1)&&(sp->pp_phase == PHASE_NETWORK))
							{
								result = DIAG_PARAMNEGOFAIL_IPV6;
								errNum = ERROR_ISP_DISCONNECT_IPv6;
							}
							else if(sp->ipType==2)
							{
								if(sp->dualFlag==1)
								{
									result = DIAG_PARAMNEGOFAIL_IPV6;
									errNum = ERROR_ISP_DISCONNECT_IPv6;
								}
								else if(sp->dualFlag==2)
								{
									result = DIAG_PARAMNEGOFAIL_IPV4;
									errNum = ERROR_ISP_DISCONNECT_IPv4;
								}
							}

							if(((sp->pp_error == ERROR_NONE)&&(sp->ipType<2))
								||((sp->pp_phase<PHASE_NETWORK)&&(1!=sp->simulation)&&(16!=sp->simulation)))
							{
								//pppoe dial has finished, do nothing.
							}
							else
							{
								sppp_last_connection_error(sp->if_unit, errNum, NULL);

								if(sp->simulation<=2)
								{
									char cmd[64];	

									DEBUG_SIMU("[%s %d]state=%d, pp_error=%d\n", __func__, __LINE__, result, sp->pp_error);
									ppp_manualdiag_log(sp, result, sp->pp_error, 0, NULL);
									sp->simulation *= 2;

									DEBUG_SIMU("[%s %d]del_ppp(unit=%d).\n", __func__, __LINE__, sp->unit);
									snprintf(cmd, 64, "/bin/spppctl del %d", sp->if_unit);
									system(cmd);
								}
							}
						}
					}
#endif
					pppoe_down(p);
					//PADT should terminate PPP too
					if(sp != NULL) ppp_if_down(sp);
				}
			}
			break;
	}
}

// 1 Invalid packet, 0 valid packet
int ErrorPacket(struct PPPoEPacket *packet, PPPOE_DRV_CTRL *p)
{
	int debug = p->debug;
	char *rpfcn = "[ErrorPacket]";

	if (packet->ver_type != 0x11)
	{
		if (debug)
			addlog(PPPOE_FMT"%s error ver and type =%02x\n", PPPOE_ARGS(p->ifno), packet->ver_type);
		return 1;
	}

	if (packet->length > ET_MAXLEN - 6) {
		if(debug)
			addlog(PPPOE_FMT"%s invalid PPPoE packet length %u\n", PPPOE_ARGS(p->ifno), packet->length);
		return 1;
	}

	if (memcmp(&packet->ehdr.to, p->MyEthAddr, Eid_SZ)) {
		if(debug)
			addlog(PPPOE_FMT"%s invalid dest addr %02x:%02x:%02x:%02x:%02x:%02x<->%02x:%02x:%02x:%02x:%02x:%02x\n\n", PPPOE_ARGS(p->ifno),
				   (unsigned) packet->ehdr.to.octet[0], (unsigned) packet->ehdr.to.octet[1], (unsigned) packet->ehdr.to.octet[2],
				   (unsigned) packet->ehdr.to.octet[3], (unsigned) packet->ehdr.to.octet[4], (unsigned) packet->ehdr.to.octet[5],
				   (unsigned) p->MyEthAddr[0], (unsigned) p->MyEthAddr[1], (unsigned) p->MyEthAddr[2],
				   (unsigned) p->MyEthAddr[3], (unsigned) p->MyEthAddr[4], (unsigned) p->MyEthAddr[5]);
		return 1;
	}

	return 0;
}


int pppoe_init(PPPOE_DRV_CTRL *pppoe_p)
{
	char *rpfcn = "[pppoe_init]";
//	int debug;

//	pppoe_p->debug = 1;
	pppoe_change_state(pppoe_p, PPPOE_STATE_IDLE);
	pppoe_p->optUseHostUnique = 0;
	pppoe_p->session = 0;
	strcpy(pppoe_p->ServiceName, "");
	strcpy(pppoe_p->DesiredACName, "");
	strcpy(pppoe_p->ACName, "");
	memset(pppoe_p->name, 0, sizeof(pppoe_p->name));
	memset(pppoe_p->ACEthAddr, 0, Eid_SZ);
	pppoe_p->cookie.type = 0;               // Added by Jenny
	pppoe_p->cookie.length= 0;
	memset(pppoe_p->cookie.payload, 0, ET_MAXLEN);
	pppoe_p->relayId.type = 0;
	pppoe_p->relayId.length= 0;
	memset(pppoe_p->relayId.payload, 0, ET_MAXLEN);
	sendPADR_flag[pppoe_p->ifno] = 0;
	padrsent[pppoe_p->ifno] = 0;
	//pppoe_p->fd = -1;
	//pppoe_p->disc_sock = -1;
	//disc_counter[pppoe_p->ifno] = 0;
	return 1;
}

int pppoe_shut(PPPOE_DRV_CTRL *pppoe_p)
{
	char *rpfcn = "[pppoe_shut]";

	sendPADT(pppoe_p);
	pppoe_change_state(pppoe_p, PPPOE_STATE_TERMINATED);
	return 1;
}

/*static void _pppoe_print_mac(const unsigned char *mac)
{
	addlog("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}*/

static void _pppoe_mac(char *result, const unsigned char *mac)
{
	sprintf(result, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static int _pppoe_hex2str(char *sval, unsigned char *hval, int len)
{
	int i;
	
	if (len>16) len=16;
	sval[0] = '\0';
	for (i=0; i<len; i++) {
		sprintf(sval+strlen(sval), "%02X", (unsigned char)hval[i]);
	}
	return 0;
}

#ifdef CONFIG_USER_PPPOE_SERVER
static void _pppoe_server_mac(const char *result, const unsigned char *mac)
{
	sprintf(result, "%02x-%02x-%02x-%02x-%02x-%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

const char * _pppoe_server_state_name(int state)
{
	switch (state) {
		case SERVER_STATE_IDLE:				return "Idle";
		case SERVER_STATE_SENT_PADO:		return "PADO-Sent";
		case SERVER_STATE_RECEIVED_PADR:	return "PADR-Received";
		case SERVER_STATE_SESSION:			return "Session";
		case SERVER_STATE_TERMINATED:		return "Terminated";
	}
	return "illegal";
}
#endif

const char *
_pppoe_state_name(int state)
{
	switch (state) {
		case PPPOE_STATE_IDLE:				return "Idle";
		case PPPOE_STATE_SENT_PADI:			return "PADI-Sent";
		case PPPOE_STATE_RECEIVED_PADO:	return "PADO-Received";
		case PPPOE_STATE_SENT_PADR:		return "PADR-Sent";
		case PPPOE_STATE_SESSION:			return "Session";
		case PPPOE_STATE_TERMINATED:		return "Terminated";
	}
	return "illegal";
}


//extern fd_set	in_fds;
//extern int	max_in_fd;
// Kaohj
fd_set poe_in_fds;
int max_poe_in_fd;

int pppoe_recv(PPPOE_DRV_CTRL *pppoe_p)
{
	// Kaohj
	//unsigned char pkt_buf[256];
	unsigned char pkt_buf[RECV_LEN];
	struct Ehdr *ehdr;
	struct PPPoEPacket *packet;
	int	len;
	char *rpfcn = "[pppoe_recv]";


	packet = (struct PPPoEPacket *)pkt_buf;
	ehdr = &(packet->ehdr);

	{
		struct sockaddr_ll	from_addr;
		fd_set in;
		struct timeval tv;
		int ret;

		//in = in_fds;
		in = poe_in_fds;

		/*some BRAS require between LCP request and LCP reply time less than 20ms, 
		  do not waste long time to wait PADT after ppp established.
		 */
		tv.tv_sec = 0;
		tv.tv_usec = 0;

		//ret = select(max_in_fd+1, &in, NULL, NULL, &tv);
		ret = select(max_poe_in_fd+1, &in, NULL, NULL, &tv);

		if (ret<=0)
			return 0;

		len = sizeof(struct sockaddr_ll);

		if (!FD_ISSET( pppoe_p->disc_sock, &poe_in_fds))	// Jenny
			return 0;
		// Kaohj
		//ret = recvfrom( pppoe_p->disc_sock, pkt_buf+Ehdr_SZ, 256, 0,
		//	(struct sockaddr*)&from_addr, &len);
		ret = recvfrom( pppoe_p->disc_sock, pkt_buf+Ehdr_SZ, RECV_LEN-Ehdr_SZ, 0,
			(struct sockaddr*)&from_addr, &len);
		if (ret<0)		// Jenny
			return 0;

		memcpy(&packet->ehdr.from, from_addr.sll_addr, ETH_ALEN);
		ehdr->type = from_addr.sll_protocol;

	}

	//if (ErrorPacket(packet, pppoe_p))  goto RelBuf;

	switch (ntohs(ehdr->type))
	{
		case ETH_PPPOE_DISCOVERY:
			// Casey
			//pppoe_p = _pppoe_get_by_discovery();
			//if(!pppoe_p && packet->code == CODE_PADT)
			//	pppoe_p = _pppoe_get_by_session(packet->session);
			//if(pppoe_p)
			//
			pppoe_cp_input(pppoe_p, packet);
			/* release buffer */
			goto pppoeReceiveError;

		case ETH_PPPOE_SESSION:
			if(packet->code != CODE_SESS)
				goto pppoeReceiveError;

			/* demux pppoe connection by session number */
			// Casey
			//pppoe_p = _pppoe_get_by_session(packet->session);
			//
			if(pppoe_p->DiscoveryState != PPPOE_STATE_SESSION)
				goto pppoeReceiveError;

			if (bcmp((char*)&packet->ehdr.from, (char*)pppoe_p->ACEthAddr, Eid_SZ)) {
				/* debug */
				if(pppoe_p->debug)
					addlog(PPPOE_FMT"Session packet, invalid source addr\n", PPPOE_ARGS(pppoe_p->ifno));
				goto pppoeReceiveError;
			}

			/* point to the ppp header, and adjust the length */
			len	-= (Ehdr_SZ + PPPOE_OVERHEAD);
			//PPPOE_UPPER_INPUT(packet->payload, len);
			printf("pass session packet to upper %x %d\n",packet->payload, len);
			break;

		default:
			addlog(PPPOE_FMT"Error ether type = 0x%04X\n", PPPOE_ARGS(pppoe_p->ifno), ntohs(ehdr->type));
			goto pppoeReceiveError;
    }
    return (TRUE);

pppoeReceiveError:

    return (TRUE);

}

#ifdef CONFIG_PPPOE_OUT_OF_ORDER
#define CONF_REQ	1		/* PPP configure request */
#define CONF_ACK	2		/* PPP configure acknowledge */
#define CONF_NAK	3		/* PPP configure negative ack */
#define CONF_REJ	4		/* PPP configure reject */
#define TERM_REQ	5		/* PPP terminate request */
#define TERM_ACK	6		/* PPP terminate acknowledge */
#define CODE_REJ	7		/* PPP code reject */
#define PROTO_REJ	8		/* PPP protocol reject */
#define ECHO_REQ	9		/* PPP echo request */
#define ECHO_REPLY	10		/* PPP echo reply */
#define DISC_REQ	11		/* PPP discard request */

static const char *
pppoe_lcp_type_name(u_char type)
{
	static char buf[12];
	switch (type) {
	case CONF_REQ:   return "CONF-REQ";
	case CONF_ACK:   return "CONF-ACK";
	case CONF_NAK:   return "CONF-NAK";
	case CONF_REJ:   return "CONF-REJ";
	case TERM_REQ:   return "TERM-REQ";
	case TERM_ACK:   return "TERM-ACK";
	case CODE_REJ:   return "CODE-REJ";
	case PROTO_REJ:  return "PROTO-REJ";
	case ECHO_REQ:   return "ECHO-REQ";
	case ECHO_REPLY: return "ECHO-REPLY";
	case DISC_REQ:   return "DISCARD-REQ";
	}
	sprintf (buf, "cp/0x%02x", type);
	return buf;
}

struct lcp_header {
	u_char type;
	u_char ident;
	u_short len;
};

int pppoe_recv_session(PPPOE_DRV_CTRL *pppoe_p)
{
	unsigned char pkt_buf[RECV_LEN];
	struct Ehdr *ehdr;
	struct PPPoEPacket *packet;
	int	len;
	int ret;

	packet = (struct PPPoEPacket *)pkt_buf;
	ehdr = &(packet->ehdr);

	{
		struct sockaddr_ll	from_addr;
		fd_set in;
		struct timeval tv;
		int ret;

		//in = in_fds;
		in = poe_in_fds;

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

		//ret = select(max_in_fd+1, &in, NULL, NULL, &tv);
		ret = select(max_poe_in_fd+1, &in, NULL, NULL, &tv);

		if (ret<=0){
			return 0;
		}

		len = sizeof(struct sockaddr_ll);

		if (!FD_ISSET( pppoe_p->sess_in_disc_sock, &poe_in_fds))	 //Jenny
			return 0;
		ret = recvfrom(pppoe_p->sess_in_disc_sock, pkt_buf+Ehdr_SZ, RECV_LEN-Ehdr_SZ, 0,
						(struct sockaddr*)&from_addr, &len);
		if (ret<0)
			return 0;

		memcpy(&packet->ehdr.from, from_addr.sll_addr, ETH_ALEN);
		ehdr->type = from_addr.sll_protocol;
	}

	switch (ntohs(ehdr->type))
	{
		case ETH_PPPOE_SESSION:
		
			if(packet->code != CODE_SESS)
				goto pppoeReceiveError;

			if(!pppoe_p)
				goto pppoeReceiveError;
			{
				int ppp_len = 0;
				ppp_len = ntohs(packet->length);
				struct lcp_header *lcph;
				u_short protocol;

				memcpy((char *)&protocol, packet->payload, sizeof(short));
				protocol = ntohs(protocol);

				pppoe_p->lcp_session = packet->session;
				if(pppoe_p->debug){
					addlog(PPPOE_FMT"(lcp out of order) session is 0x%x, protocol is 0x%2x, len is %d\n", 
							PPPOE_ARGS(pppoe_p->ifno), pppoe_p->lcp_session, protocol,  ppp_len);
				}

				if(protocol != PPP_LCP)
					goto pppoeReceiveError;

				lcph = (struct lcp_header*)(packet->payload+2);
				if(pppoe_p->debug){
					addlog(PPPOE_FMT"(lcp out of order) lcp info: type %s, id 0x%02x, len %d\n", 
							PPPOE_ARGS(pppoe_p->ifno), pppoe_lcp_type_name(lcph->type), lcph->ident, htons(lcph->len));
				}

				if(lcph->type != CONF_REQ)
					goto pppoeReceiveError;

				if(ppp_len >= PPPOE_LCP_CONN_REQ_PKT_LEN)
				{
					printf("[pppoe]get ppp pkt in discover state, but len %d is over %d, ignore\n", 
							ppp_len, PPPOE_LCP_CONN_REQ_PKT_LEN);
					goto pppoeReceiveError;
				}
				else
				{
					pppoe_p->lcp_conn_req_len = ppp_len;
					memcpy(pppoe_p->lcp_conn_req_pkt, packet->payload, ppp_len);
					printf(PPPOE_FMT"lcp before PADS\n", PPPOE_ARGS(pppoe_p->ifno));
					if(pppoe_p->debug)
						addlog(PPPOE_FMT"I lcp pkt in discover state, cache it\n", PPPOE_ARGS(pppoe_p->ifno));
				}
			}
			
			break;
		default:
			addlog(PPPOE_FMT"Error ether type = 0x%04X\n", PPPOE_ARGS(pppoe_p->ifno), ntohs(ehdr->type));
			goto pppoeReceiveError;
    }
    return (TRUE);

pppoeReceiveError:

    return (TRUE);

}
#endif

/**********************************************************************
* FUNCTION: pppoeInit
* ARGUMENTS:
* 	unit 		-- pppoe unit number
* 	pDevice		-- attach to ethernet device
* 	dunit 		-- attach to ethernet device number
* 	local_addr 	-- local ip address, 0 as dynamic
* 	remote_addr -- remote ip address, 0 as dynamic
* 	username	-- username for authentication
* 	secret      -- password for authentication
* RETURNS: STATUS
* DESCRIPTION: Initial PPPoE unit and do dial up
***********************************************************************/
int pppoeInit(char *dev_name)
{
	//i have checked the place calling this func, they do not use the returned value.
    //i just add this sentence to make it look like a standard function.
    return 0;
}

#ifdef CONFIG_PPPOE_OUT_OF_ORDER
int pppoe_init_sess_in_disc_sock(PPPOE_DRV_CTRL *pppoe)
{
	int flags;

	pppoe->sess_in_disc_sock = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_PPP_SES));
	if (pppoe->sess_in_disc_sock < 0) {
		printf("%s:%d, Cannot create PF_PACKET socket for PPPoE session in discovery\n", __FUNCTION__, __LINE__);
		return -1;
	}

	flags = fcntl(pppoe->sess_in_disc_sock, F_GETFL);

	if ((flags == -1) || fcntl(pppoe->sess_in_disc_sock, F_SETFL, flags | O_NONBLOCK) == -1)
	{
	   	printf("%s:%d, Couldn't set PPPoE discovery socket to nonblock\n", __FUNCTION__, __LINE__);
		return -1;
	}

	printf("[pppoe]init sock of session in discover state. sock is %d \n", pppoe->sess_in_disc_sock);

	return 0;
}
#endif

int pppoe_client_init(struct sppp *sppp)
{
	//struct pppoe_param_s *poer = sppp->dev;
	struct pppoe_param_s poer = {0};
	PPPOE_DRV_CTRL *p;
	int retval, flags;
	char dev[IFNAMSIZ+1]={0};
	int addr[ETH_ALEN]={0};
	int sid;
	struct ifreq ifr;
	int i=0, ret=0;
	int sockbufpriority = TC_PRIO_CONTROL;
	char ifname[IFNAMSIZ]={0};
	char ACName_str[STR_LEN]={0}, ServiceName_str[STR_LEN]={0};

	memset(&ifr, 0, sizeof(struct ifreq));
	p = (PPPOE_DRV_CTRL *)malloc(sizeof(PPPOE_DRV_CTRL));
	if (!p)
		return -1;
	memset (p, 0, sizeof(PPPOE_DRV_CTRL));

	p->fd = -1;
	p->disc_sock = -1;
	p->ifno = sppp->if_unit;
#ifdef CONFIG_SUPPORT_AUTO_DIAG	
	p->simulation = sppp->simulation;
#endif
	pppoe_init(p);
	memcpy(&poer, sppp->dev, sizeof(struct pppoe_param_s));
	snprintf(ifname, sizeof(ifname)-1, "%s", poer.dev_name);
	if(strlen(ifname)>0)
		strncpy(p->name, ifname, sizeof(p->name)-1);
	else
		strncpy(p->name, "", sizeof(p->name)-1);
	//strncpy(p->name, poer->dev_name, sizeof(p->name)-1);

	snprintf(ACName_str, sizeof(ACName_str)-1, "%s", poer.ACName);
	if(strlen(ACName_str)>0)
		strncpy(p->DesiredACName, ACName_str, sizeof(p->DesiredACName)-1);
	else
		strncpy(p->DesiredACName, "", sizeof(p->DesiredACName)-1);
	//strncpy(p->DesiredACName, poer->ACName), sizeof(ACName_str)-1);
	snprintf(ServiceName_str, sizeof(ServiceName_str)-1, "%s", poer.ServiceName);
	if(strlen(ServiceName_str)>0)
		strncpy(p->ServiceName, ServiceName_str, sizeof(p->ServiceName)-1);
	else
		strncpy(p->ServiceName, "", sizeof(p->ServiceName)-1);
	//strncpy(p->ServiceName, poer->ServiceNam, sizeof(p->ServiceName)-1);

	p->vpi = poer.vpi;
	p->vci = poer.vci;
#if defined(CONFIG_LUNA)
	p->NoHardware_NAT = poer.NoHardware_NAT;
#endif
	if(sppp->pp_lowerp) free(sppp->pp_lowerp);
	sppp->pp_lowerp = p;
	p->sppp = sppp;
	p->disc_sock = socket(PF_PACKET, SOCK_DGRAM, 0);
	if (p->disc_sock < 0) {
		printf("Cannot create PF_PACKET socket for PPPoE discovery\n");
		return -1;
	}

	if (setsockopt(p->disc_sock, SOL_SOCKET, SO_PRIORITY, (void *) &sockbufpriority, sizeof(sockbufpriority)) == -1)	// Jenny
		printf("set PPPoE (SO_PRIORITY) error! ");

	ret = fcntl(p->disc_sock, F_SETFD, fcntl(p->disc_sock, F_GETFD) | FD_CLOEXEC); // FD_CLOEXEC: Let child process won't inherit this fd
	if (ret < 0)
		printf("PPPoE (fcntl) error! ");
	flags = fcntl(p->disc_sock, F_GETFL);	// Jenny
	if (flags == -1 || fcntl(p->disc_sock, F_SETFL, flags | O_NONBLOCK) == -1)
	   	printf("Couldn't set PPPoE discovery socket to nonblock\n");

	{
		strncpy(ifr.ifr_name, p->name, sizeof(ifr.ifr_name)-1);

		retval = ioctl(p->disc_sock , SIOCGIFINDEX, &ifr);
		if (retval < 0) {
			printf("Bad device name: %s  (%m)",p->name);
			return retval;
//			return 0;
	    	}

		p->local.sll_ifindex = ifr.ifr_ifindex;

		retval = ioctl (p->disc_sock, SIOCGIFHWADDR, &ifr);
		if (retval < 0) {
			printf("Bad device name: %s  (%m)",p->name);
			return retval;
//			return 0;
    		}

		if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
			printf("Interface %s is not Ethernet!", p->name);
			return -1;
//			return 0;
		}

		p->local.sll_family	= AF_PACKET;
		p->local.sll_protocol= ntohs(ETH_P_PPP_DISC);
		p->local.sll_hatype	= ARPHRD_ETHER;
		p->local.sll_pkttype = PACKET_BROADCAST;
		p->local.sll_hatype	= ETH_ALEN;
		memcpy(p->local.sll_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

		memcpy(&p->remote, &p->local, sizeof(struct sockaddr_ll));
		memset(p->remote.sll_addr, 0xff, ETH_ALEN);
	}
	memcpy(p->MyEthAddr, p->local.sll_addr, ETH_ALEN);

	retval = bind( p->disc_sock , (struct sockaddr*)&p->local, sizeof(struct sockaddr_ll));
	if (retval < 0) {
		printf("bind to PF_PACKET socket failed: %m");
		return retval;		// Jenny
	}

#ifdef CONFIG_PPPOE_OUT_OF_ORDER
	pppoe_init_sess_in_disc_sock(p);
#endif

	p->fd = socket(AF_PPPOX,SOCK_STREAM,PX_PROTO_OE);
	if (p->fd < 0)	{
		printf("Failed to create PPPoE socket: %m");
		return p->fd;	// Jenny
	}

	ret = fcntl(p->fd, F_SETFD, fcntl(p->fd, F_GETFD) | FD_CLOEXEC); // FD_CLOEXEC: Let child process won't inherit this fd
	if (ret < 0)
		printf("PPPoE (fcntl) error! ");

	p->sp.sa_family = AF_PPPOX;
	p->sp.sa_protocol = PX_PROTO_OE;
	p->sp.sa_addr.pppoe.sid = 0;
	memcpy(p->sp.sa_addr.pppoe.dev,p->name, IFNAMSIZ);

/*	retval = connect(p->fd, (struct sockaddr*)&p->sp, sizeof(struct sockaddr_pppox));
	if (retval < 0) {
		printf("Failed to connect PPPoE socket: %d %m",retval);
		return retval;
	}*/

	flags = fcntl(p->fd, F_GETFL);	// Jenny
	if (flags == -1 || fcntl(p->fd, F_SETFL, flags | O_NONBLOCK) == -1)
	   	printf("Couldn't set PPPoE socket to nonblock\n");

	disc_counter[sppp->if_unit] = 0;
	return p->fd;
}

// Kaohj
//static struct callout disc_ch;
//static int disc_timeout;
void discTimeout(PPPOE_DRV_CTRL *p)
{
//	disc_timeout = 1;
	printf("pppoe%d discTimeout\n", p->ifno);
	p->disc_timeout = 1;
}

/*
 * pppoe_add_fd - add an fd to the set that wait_input waits for.
 */
void pppoe_add_fd(int fd)
{
	FD_SET(fd, &poe_in_fds);
	if (fd > max_poe_in_fd)
		max_poe_in_fd = fd;
}

/*
 * pppoe_remove_fd - remove an fd from the set that wait_input waits for.
 */
void pppoe_remove_fd(int fd)
{
	FD_CLR(fd, &poe_in_fds);
}

//extern int in_pppoe_disc;
// Jenny, change PPPoE discovery mechanism from sequential to simultaneous, to shorten multiple PPPoE connecting time

#if defined(CONFIG_USER_PPPOE_PROXY) || defined(CONFIG_USER_PPPOE_SERVER)
int createPppoeServerSocket()
{
	int opt = 1, ret = 0;
	int optval=1;
	int flags;
	struct ifreq ifr;
	struct sockaddr_ll address;

	/* create the server socket and check it worked */
	if ((pppoe_server_socket = socket(PF_PACKET, SOCK_RAW, ETH_PPPOE_DISCOVERY)) < 0)
		return -1;

	if (setsockopt(pppoe_server_socket, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
		return -2;
    }

	ret = fcntl(pppoe_server_socket, F_SETFD, fcntl(pppoe_server_socket, F_GETFD) | FD_CLOEXEC); // FD_CLOEXEC: Let child process won't inherit this fd
	if (ret < 0)
		printf("%s (fcntl) error! ", __FUNCTION__);

	flags = fcntl(pppoe_server_socket, F_GETFL);
	if (flags == -1 || fcntl(pppoe_server_socket, F_SETFL, flags | O_NONBLOCK) == -1)
	   	printf("Couldn't set pppoe server socket to nonblock\n");

	/* set up socket */
	memset(&address, 0, sizeof(address));

	address.sll_family	= AF_PACKET;
	address.sll_protocol= ntohs(ETH_P_PPP_DISC);
	address.sll_hatype	= ARPHRD_ETHER;
	address.sll_pkttype = PACKET_BROADCAST;
	address.sll_hatype	= ETH_ALEN;
	memcpy(address.sll_addr, localhwAddr, ETH_ALEN);

	/* bind the socket to the pptp port */
	if (bind(pppoe_server_socket, (struct sockaddr *) &address, sizeof(address)) < 0)
		return -3;

	FD_SET(pppoe_server_socket, &pppConnSet);
	maxpppConnFd = pppoe_server_socket;

	return 0;
}

void pppoe_server_init()
{
	int i;
	FILE *fp;
	int ret = 0;

	if(-1 != pppoe_server_socket)
		return;

	//get server's ip and hwaddr
	read_interface(IFName, &LocalIP, localhwAddr);

	createPppoeServerSocket();

	//init CookieSeed
	fp = fopen("/dev/urandom", "r");
    if (fp) {
		ret = fread(&CookieSeed, 1, SEED_LEN, fp);
		if(ret < 0)
		{
			printf("Read /dev/urandom error! \n");
			return;
		}
		fclose(fp);
    } else {
		CookieSeed[0] = getpid() & 0xFF;
		CookieSeed[1] = (getpid() >> 8) & 0xFF;
		for (i=2; i<SEED_LEN; i++) {
	  		CookieSeed[i] = (rand() >> (i % 9)) & 0xFF;
		}
    }

	if (Sessions) {
		free(Sessions);
		Sessions = NULL;
	}

	//init Session
	Sessions = calloc(DEFAULT_MAX_SESSIONS, sizeof(struct ClientSession));
	for(i = 0; i < DEFAULT_MAX_SESSIONS; i++){
		Sessions[i].sess = i+1;
		Sessions[i].isUsingNow = 0;
		#ifdef CONFIG_USER_PPPOE_SERVER
		Sessions[i].discoverystate = SERVER_STATE_IDLE;
		#endif
		memcpy(Sessions[i].ip, ClientIP, sizeof(ClientIP));

		/* Increment IP */
		ClientIP[3]++;
		if (!ClientIP[3] || ClientIP[3] == 255) {
		    ClientIP[3] = 1;
		    ClientIP[2]++;
		    if (!ClientIP[2] || ClientIP[2] == 255) {
				ClientIP[2] = 1;
				ClientIP[1]++;
				if (!ClientIP[1] || ClientIP[1] == 255) {
					ClientIP[1] = 1;
				    ClientIP[0]++;
				}
		    }
		}
	}
	#ifdef CONFIG_USER_PPPOE_SERVER
	session_status();
	#endif
}

struct ClientSession * getSession(){
	int i;
	static int usedSession=0;

	for(i = usedSession; i < DEFAULT_MAX_SESSIONS; i++){
		usedSession++;
		if(usedSession>=DEFAULT_MAX_SESSIONS)
			usedSession=0;
		if (Sessions[i].isUsingNow != 1){
			printf("\nPrepared session id :%d, with IP 0x%08x.\n", Sessions[i].sess, *((int *)(&Sessions[i].ip)));
			return &Sessions[i];
		}
		else
			printf("\nThe IP 0x%08x has been used\n", *((int *)(&Sessions[i].ip)));
	}
	return NULL;
}

void freeSession(int sessionID){
	int i;

	if(Sessions == NULL){
		return;
	}
	
	for(i = 0; i < DEFAULT_MAX_SESSIONS; i++){
		if (Sessions[i].sess == sessionID)
		{
			Sessions[i].isUsingNow=0;
			printf("Free PPPoE session id :%d, with IP 0x%08x\n",sessionID,*((int *)(&Sessions[i].ip)));
#ifdef CONFIG_USER_PPPOE_SERVER
			Sessions[i].discoverystate = SERVER_STATE_IDLE;
			memset(Sessions[i].hwaddr, 0, Eid_SZ);
#endif
		}
	}
}

void freeAllSession() {
	int i;

	printf("Free All PPPoE server/proxy Session!\n");
	if(Sessions == NULL){
		return;
	}
	for (i = 0; i < DEFAULT_MAX_SESSIONS; i++) {
			Sessions[i].isUsingNow=0;
#ifdef CONFIG_USER_PPPOE_SERVER
			Sessions[i].discoverystate = SERVER_STATE_IDLE;
			memset(Sessions[i].hwaddr, 0, Eid_SZ);
#endif
	}
}

#ifdef CONFIG_USER_PPPOE_PROXY
#ifdef CONFIG_USER_LANNETINFO
#ifndef Host_Ethernet
#define Host_Ethernet 0
#endif
#ifndef Host_802_11
#define Host_802_11 1
#endif

static int client_alive_in_lan(unsigned int intfGrp, int debug)
{
	int idx;
	int ret = 0;
	unsigned int portIdx;
	unsigned int phyPort;
	lanHostInfo_t *pLANNetInfoData = NULL; 
	int Count = 0;

	ret = get_lan_net_info(&pLANNetInfoData, &Count);
	if(ret != 0 || pLANNetInfoData == NULL || Count <= 0) {
		if(debug) addlog("[PPPoE Proxy] No LAN Host !!!, ret = %d\n", ret);
		goto done;
	}
	
	if(debug) addlog("[PPPoE Proxy] intGrp: 0x%x, LAN Host Count: %d\n", intfGrp, Count);
	
	ret = 0;
	for (idx=0; idx<Count; idx++)
	{
		if(!pLANNetInfoData[idx].disConnect)
		{
			if(debug) 
				addlog("[PPPoE Proxy] MAC: %x-%x-%x-%x-%x-%x, ConnType: %s, Port: %d, Disconnect: %d\n", 
					pLANNetInfoData[idx].mac[0], pLANNetInfoData[idx].mac[1], pLANNetInfoData[idx].mac[2],
					pLANNetInfoData[idx].mac[3], pLANNetInfoData[idx].mac[4], pLANNetInfoData[idx].mac[5],
					(pLANNetInfoData[idx].connectionType == Host_Ethernet) ? "Ethernet" : "WIFI", 
					pLANNetInfoData[idx].port,
					pLANNetInfoData[idx].disConnect);
			if(intfGrp > 0)
			{
				portIdx = pLANNetInfoData[idx].port;
				if(pLANNetInfoData[idx].connectionType == Host_Ethernet) 
				{
					phyPort = portIdx - 1;
					if((intfGrp & (1<<phyPort)))
					{
						ret = 1;
						break;
					}
				}
#ifdef WLAN_SUPPORT
				else if(pLANNetInfoData[idx].connectionType == Host_802_11)
				{
					if(portIdx > WLAN_SSID_NUM)
						phyPort = PMAP_WLAN1 + (portIdx - WLAN_SSID_NUM -1);
					else
						phyPort = PMAP_WLAN0 + (portIdx -1);
					if((intfGrp & (1<<phyPort)))
					{
						ret = 1;
						break;
					}
				}
#endif
			}
			else{
				ret = 1;
				break;
			}
		}
	}
	
done:
	if(pLANNetInfoData)
	{
		free(pLANNetInfoData);
	}
	if(debug) addlog("[PPPoE Proxy] %s LAN Host exist !!!  ret = %d\n", (ret)?"Have":"No have", ret);
	
	return ret;
}
#endif
/**
 * FUNC: when pppoe client is off, we should check if all lan ports binded with pppoe proxy wan disconnect with ONU.
 * PARA: sp - point to wan (which is pppoe client)
 */
#include <sys/sysinfo.h>
int pppoe_proxy_clients_are_all_down(struct sppp *sp)
{
	int debug = sp->debug;
	int ret=0,Count=0,isLanhostExist=0;
	struct sysinfo info;
	static long _lastUptime = 0;
	/* 1. check if pppoe client is disconnected */
	if (sp->currUser != 0)
		return 0;
	
	sysinfo(&info);
	if((info.uptime - _lastUptime) < 3) return 0;

#ifdef CONFIG_USER_LANNETINFO
	/* check if no l2 entry matched with this port */
	if (client_alive_in_lan(sp->itfGroup, debug))
	{
		isLanhostExist = 1;
	}
#endif

	sysinfo(&info);
	_lastUptime = info.uptime;

	return (isLanhostExist) ? 0 : 1;
}

int is_pppoe_proxy_up(void)
{
	struct sppp *sp;
	
	sp = spppq;
	while (sp)
	{
		if ((1 == sp->role) && (SPPP_PPPOE == sp->over))
		{
			if (1 == get_ipcpstate(sp))
			{
				fprintf(stderr, "pppoe proxy is alive!\n");
				return 1;
			}
		}
		
		sp = sp->pp_next;
	}
	
	return 0;
}

void pppoe_server_rcv()
{
	struct sppp *sp, *sq, *spq;
	PPPOE_DRV_CTRL *pppoe_p;
	PPPOEPROXY_DRV_CTRL *proxy_p, proxy_req;
	fd_set in;
	struct timeval tv;
	unsigned char pkt_buf[RECV_LEN];
	struct Ehdr *ehdr;
	struct PPPoEPacket *packet;
	struct sockaddr_ll from_addr;
	unsigned char cookieBuffer[16];
	struct ClientSession *cliSession;
	struct ifreq ifreq;
	int	len;
	int ret;
	int i;
	int match = 0;
	int flags;
	int sock_fd;
	struct SW_INFO swinfo;

	in = pppConnSet;
	tv.tv_sec = 0;
	tv.tv_usec = 10000;

	if (select(maxpppConnFd + 1, &in, NULL, NULL, &tv) <= 0) {
		/* check if proxy wan should be forced down */
		sp = spppq;
		while(sp)
		{
			if ((1 == sp->enable_pppoe_proxy) && (0 == sp->currUser) && (sp->pp_phase != PHASE_DEAD))
			{
				//if (pppoe_proxy_clients_are_all_down(sp))
				{
					if(sp->debug) addlog("[PPPoE Proxy] All clients is down, %s is going down\n", sp->if_name);
					pppoe_change_state(sp->pp_lowerp, PPPOE_STATE_TERMINATED);
					pppoe_down(sp->pp_lowerp);
#ifdef CONFIG_USER_PPPOE_CTC_DELAY_INT
					ppp_if_down(sp);
#else
					pppoe_up(sp->pp_lowerp);
#endif
				}
			}
			sp = sp->pp_next;
		}
		return;
	}

	if (FD_ISSET(pppoe_server_socket, &in)) {	/* A call came! */

		packet = (struct PPPoEPacket *)pkt_buf;
		ehdr = &(packet->ehdr);
		len = sizeof(struct sockaddr_ll);
		ret = recvfrom(pppoe_server_socket, pkt_buf, RECV_LEN, 0,
			(struct sockaddr*)&from_addr, &len);
		if (ret<0){		// Jenny
			perror("recvfrom error: ");
			return;
		}

		if (0x11 != packet->ver_type){
			printf("ver_type error! ver_type = 0x%x\n", packet->ver_type);
			return;
		}

		if(ETH_PPPOE_DISCOVERY != ntohs(ehdr->type)){
			printf("packet type should be discovery!, type = 0x%x\n", ntohs(ehdr->type));
			return;
		}

		switch(packet->code) {
			case CODE_PADI:
				parsePADITags(&proxy_req, packet);
				sp = spppq;
				while(sp){
					if((1 == sp->enable_pppoe_proxy) && (sp->currUser < sp->maxUser)){
						sendPADO(&proxy_req, &from_addr);
						break;
					}
					sp = sp->pp_next;
				}
				break;
			case CODE_PADR:
				parsePADRTags(&proxy_req, packet);
				if(!proxy_req.cookie.type){
					printf("Received PADR packet without cookie tag\n");
					sendErrorPADS(&proxy_req, &from_addr, TAG_GENERIC_ERROR, "No cookie.  Me hungry!");
					return;
				}
				if (proxy_req.cookie.length != htons(16)) {
					printf("Received PADR packet with invalid cookie tag length\n");
 					sendErrorPADS(&proxy_req, &from_addr, TAG_GENERIC_ERROR, "Cookie wrong size.");
					return;
			    }

				genCookie(from_addr.sll_addr, localhwAddr, CookieSeed, cookieBuffer);
			    if (memcmp(proxy_req.cookie.payload, cookieBuffer, 16)){
					printf("Received PADR packet with invalid cookie tag\n");
					sendErrorPADS(&proxy_req, &from_addr, TAG_GENERIC_ERROR, "Bad cookie.  Me have tummy-ache.");
					return;
			    }

				sp = spppq;
				while(sp){
					if((1 == sp->role)){
						proxy_p = sp->pp_lowerp;
						if(!memcmp(proxy_p->peerEthAddr, from_addr.sll_addr, Eid_SZ))
							return; //already response PADS and start pppd
					}
					sp = sp->pp_next;
				}

#if !defined(CONFIG_LUNA)
#ifdef NEW_PORTMAPPING
				sock_fd = socket(AF_INET, SOCK_DGRAM, 0);  //sock_fd for query swport
			    if (sock_fd < 0)
					printf("Couldn't create IP socket!\n");

				ret = fcntl(sock_fd, F_SETFD, fcntl(sock_fd, F_GETFD) | FD_CLOEXEC); // FD_CLOEXEC: Let child process won't inherit this fd
				if (ret < 0)
					printf("%s (fcntl) error! ", __FUNCTION__);
				memcpy(ifreq.ifr_name, IFName, IFNAMSIZ);
				memcpy(swinfo.hwaddr, from_addr.sll_addr, Eid_SZ);
				swinfo.swport = 255;
				ifreq.ifr_data = (void *)&swinfo;
				if(ioctl(sock_fd, SIOCGSWPORT, &ifreq) < 0){
					printf("ioctl SIOCGSWPORT error!\n");
					return;
				}
				close(sock_fd);
#endif
#endif
				sp = spppq;
				while(sp){
					if((1 == sp->enable_pppoe_proxy) && (sp->currUser < sp->maxUser)){
						if( TRUE //(1 == get_ipcpstate(sp))
#if !defined(CONFIG_LUNA)
#ifdef NEW_PORTMAPPING
						 && (sp->itfGroup == 0 || ((sp->itfGroup & (1<<swinfo.swport)) != 0))
#endif
#endif
						){
							match = 1;
							break;
						}
					}
					sp = sp->pp_next;
				}

				if(!match){
					printf("no server can response\n");
					return;
				}

				cliSession = getSession();
				if(NULL == cliSession){
					printf("Can not get session.\n");
					sendErrorPADS(&proxy_req, &from_addr, TAG_AC_SYSTEM_ERROR, "No session supplied");
					return;
			    }
				packet->session = cliSession->sess;

				sendPADS(&proxy_req, cliSession->sess, &from_addr);

				if((sq = (struct sppp *)add_pppd(SPPP_PPPOE)) != NULL){
					sq->debug = sp->debug;
					sq->ipcp.myipaddr = *((int *)(&ServerIP));
					sq->ipcp.hisipaddr = *((int *)(&cliSession->ip));
					sq->ipcp.opts = (1 << IPCP_OPT_ADDRESS);
					//sq->ipcp.primary_dns = LocalIP; //if PPPoE Proxy, remote end is 192.168.100.1, instead of 192.168.1.1
													  // and DNS relay may not work for 192.168.100.1, so just give the DNS
													  // IP addre got from ISP
					sq->ipcp.primary_dns = (sp->ipcp.primary_dns==0)?LocalIP:sp->ipcp.primary_dns;
//					sq->ipcp.second_dns = sp->ipcp.second_dns;
					sq->ipcp.dns_opts = IPCP_OPT_DNS_PRIMARY;
					sq->myauth.aumode = sp->myauth.aumode;
					sq->myauth.proto = sp->myauth.proto;
					if(0x0 == sq->myauth.proto)
						sq->myauth.proto = PPP_PAP;
					memcpy(sq->myauth.name, sp->myauth.name, AUTHNAMELEN);
					memcpy(sq->myauth.secret, sp->myauth.secret, AUTHNAMELEN);
					
					//init proxy_p
					proxy_p = (PPPOEPROXY_DRV_CTRL *)malloc(sizeof(PPPOEPROXY_DRV_CTRL));
					if (!proxy_p){
						printf("alloc proxy_p error!\n");
						return;
					}
					memset (proxy_p, 0, sizeof(PPPOEPROXY_DRV_CTRL));

					memcpy(proxy_p->peerEthAddr, from_addr.sll_addr, Eid_SZ);
					memcpy(&proxy_p->client, &from_addr, sizeof(struct sockaddr_ll));
					memcpy(&proxy_p->cookie, &proxy_req.cookie, sizeof(struct PPPoETag));
					memcpy(&proxy_p->relayId, &proxy_req.relayId, sizeof(struct PPPoETag));
					proxy_p->sp = sp;
					proxy_p->session = htons(cliSession->sess);
					memcpy(proxy_p->name, IFName, IFNAMSIZ);
					sp->currUser++;
					sq->pp_lowerp = proxy_p;

					proxy_p->fd = socket(AF_PPPOX,SOCK_STREAM,PX_PROTO_OE);
					if (proxy_p->fd < 0)	{
						printf("Failed to create PPPOE server socket\n");
						return ;
					}

					ret = fcntl(proxy_p->fd, F_SETFD, fcntl(proxy_p->fd, F_GETFD) | FD_CLOEXEC); // FD_CLOEXEC: Let child process won't inherit this fd
					if (ret < 0)
						printf("%s (fcntl) error! ", __FUNCTION__);
					proxy_p->localaddr.sa_family = AF_PPPOX;
				    proxy_p->localaddr.sa_protocol = PX_PROTO_OE;
				  	proxy_p->localaddr.sa_addr.pppoe.sid = proxy_p->session;
				    memcpy(proxy_p->localaddr.sa_addr.pppoe.dev, proxy_p->name, IFNAMSIZ);
				    memcpy(proxy_p->localaddr.sa_addr.pppoe.remote, localhwAddr, Eid_SZ);

					memcpy(&proxy_p->peeraddr, &proxy_p->localaddr, sizeof(struct sockaddr_pppox));
					memcpy(proxy_p->peeraddr.sa_addr.pppoe.remote, from_addr.sll_addr, Eid_SZ);

				    ret = connect(proxy_p->fd, (struct sockaddr*)&proxy_p->peeraddr, sizeof(struct sockaddr_pppox));
					if( ret < 0 ){
						printf("Failed to connect PPPoE socket, ret = %d",ret);
						perror("connect error: ");
						return ;
				    }

					flags = fcntl(proxy_p->fd, F_GETFL);
					if (flags == -1 || fcntl(proxy_p->fd, F_SETFL, flags | O_NONBLOCK) == -1)
						printf("Couldn't set PPPOE server socket to nonblock\n");

					sq->fd = proxy_p->fd;
					cliSession->isUsingNow = 1;
					start_pppd_real(sq);
				}

				break;
			case CODE_PADT:
				printf("Received PADT packet with session %d\n", ntohs(packet->session));
				sp = spppq;
				while(sp)
				{
					if((1 == sp->role))
					{
						char cmd[64];
						proxy_p = sp->pp_lowerp;
						if((!memcmp(proxy_p->peerEthAddr, from_addr.sll_addr, Eid_SZ) || get_lcpstate(sp) == 0)
							&&(proxy_p->session==packet->session))
						{
							//sp receive a PADT.
							printf("Got a sp\n");
							snprintf(cmd, 64, "/bin/spppctl delproxyunit %d", sp->if_unit);
							system(cmd);
							break;
						}
					}
					sp = sp->pp_next;
				}
				break;	
			default:
				break;
		}
	}
}
#elif defined(CONFIG_USER_PPPOE_SERVER)
int session_status(void)
{
	char buff[256];
	int i;
	FILE *fp;
#ifdef FILE_LOCK
	struct flock flpoe, flpoa;
	int fdpoe, fdpoa;
#endif
	char mac[17];

	//signal(SIGUSR2, SIG_IGN);
#ifdef FILE_LOCK
	fdpoe = open("/var/ppp/session.conf", O_RDWR);
	if (fdpoe != -1) {
		flpoe.l_type = F_WRLCK;
		flpoe.l_whence = SEEK_SET;
		flpoe.l_start = 0;
		flpoe.l_len = 0;
		flpoe.l_pid = getpid();
		if (fcntl(fdpoe, F_SETLKW, &flpoe) == -1)
			printf("pppoe write lock failed\n");
	}
#endif //FILE_LOCK

	fp = fopen("/var/ppp/session.conf", "w");
	if (fp) {
		sprintf(buff, "%-10s  %-12s  %-17s  %-8s  %s\n", \
			"SessionNo", "ClientIP", "ClientMAC", "isUsing", "phase");
		fputs(buff, fp);
		for (i = 0; i < DEFAULT_MAX_SESSIONS; i++) {
			_pppoe_server_mac(mac, (char*)&Sessions[i].hwaddr);
			//printf("================%s(%d) %-10d  0x%08x  %s  %-8d  %s\n", __FUNCTION__, __LINE__, \
			//	Sessions[i].sess, *((int *)(&Sessions[i].ip)), mac, Sessions[i].isUsingNow, _pppoe_server_state_name(Sessions[i].discoverystate));
			sprintf(buff, "%-10d  0x%10x  %-17s  %-8d  %s\n", \
				Sessions[i].sess, *((int *)(&Sessions[i].ip)), mac, Sessions[i].isUsingNow, _pppoe_server_state_name(Sessions[i].discoverystate));
			fputs(buff, fp);
		}
		fclose(fp);
	}

#ifdef FILE_LOCK
	if (fdpoe != -1) {
		flpoe.l_type = F_UNLCK;
		if (fcntl(fdpoe, F_SETLK, &flpoe) == -1)
			printf("pppoe write unlock failed\n");
		close(fdpoe);
	}
#endif //FILE_LOCK

	//signal(SIGUSR2, session_status);
	return 0;
}

void stopPppoeServer()
{
	if (Sessions == NULL)
		return;

	freeAllSession();
	session_status();

	if (pppoe_server_socket == -1)
		return;

	if (FD_ISSET(pppoe_server_socket, &pppConnSet))
		FD_CLR(pppoe_server_socket, &pppConnSet);
	if (pppoe_server_socket >= 0) {
		close(pppoe_server_socket);
		pppoe_server_socket = -1;
	}

	if (Sessions) {
		free(Sessions);
		Sessions = NULL;
	}
}

void pppoe_server_rcv()
{
	PPPOE_DRV_CTRL *pppoe_p;
	PPPOEPROXY_DRV_CTRL pppoe_s_req;
	fd_set in;
	struct timeval tv;
	unsigned char pkt_buf[RECV_LEN];
	struct Ehdr *ehdr;
	struct PPPoEPacket *packet;
	struct sockaddr_ll from_addr;
	unsigned char cookieBuffer[16];
	struct ClientSession *cliSession;
	struct ifreq ifreq;
	int	len;
	int ret;
	int i;
	int match = 0;
	int flags;
	int sock_fd;
	struct SW_INFO swinfo;
	char mac[17];

	in = pppConnSet;
	tv.tv_sec = 0;
	tv.tv_usec = 10000;

	if (select(maxpppConnFd + 1, &in, NULL, NULL, &tv) <= 0) {
		return;
	}

	if (FD_ISSET(pppoe_server_socket, &in)) {	/* A call came! */

		packet = (struct PPPoEPacket *)pkt_buf;
		ehdr = &(packet->ehdr);
		len = sizeof(struct sockaddr_ll);
		ret = recvfrom(pppoe_server_socket, pkt_buf, RECV_LEN, 0,
			(struct sockaddr*)&from_addr, &len);
		//printf("===================%s(%d) recvfrom ret = %d\n", __FUNCTION__, __LINE__, ret);
		if (ret<0){		// Jenny
			perror("recvfrom error: ");
			return;
		}

		//printf("===================%s(%d) ver_type = 0x%x\n", __FUNCTION__, __LINE__, packet->ver_type);
		if (0x11 != packet->ver_type){
			printf("ver_type error! ver_type = 0x%x\n", packet->ver_type);
			return;
		}

		//printf("===================%s(%d) packet type = 0x%x\n", __FUNCTION__, __LINE__, ntohs(ehdr->type));
		if (ETH_PPPOE_DISCOVERY != ntohs(ehdr->type)){
			printf("packet type should be discovery!, type = 0x%x\n", ntohs(ehdr->type));
			return;
		}

		//printf("===================%s(%d) packet code = 0x%x \n", __FUNCTION__, __LINE__, packet->code);
		switch(packet->code) {
			case CODE_PADI:
				parsePADITags(&pppoe_s_req, packet);
				sendPADO(&pppoe_s_req, &from_addr);
				session_status();
				break;
			case CODE_PADR:
				parsePADRTags(&pppoe_s_req, packet);
				if(!pppoe_s_req.cookie.type){
					printf("Received PADR packet without cookie tag\n");
					sendErrorPADS(&pppoe_s_req, &from_addr, TAG_GENERIC_ERROR, "No cookie.  Me hungry!");
					return;
				}
				if (pppoe_s_req.cookie.length != htons(16)) {
					printf("Received PADR packet with invalid cookie tag length\n");
					sendErrorPADS(&pppoe_s_req, &from_addr, TAG_GENERIC_ERROR, "Cookie wrong size.");
					return;
				}

				genCookie(from_addr.sll_addr, localhwAddr, CookieSeed, cookieBuffer);
				if (memcmp(pppoe_s_req.cookie.payload, cookieBuffer, 16)){
					printf("Received PADR packet with invalid cookie tag\n");
					sendErrorPADS(&pppoe_s_req, &from_addr, TAG_GENERIC_ERROR, "Bad cookie.  Me have tummy-ache.");
					return;
				}

				cliSession = getSession();
				if (NULL == cliSession){
					printf("Can not get session.\n");
					sendErrorPADS(&pppoe_s_req, &from_addr, TAG_AC_SYSTEM_ERROR, "No session supplied");
					return;
				}
				packet->session = cliSession->sess;

				sendPADS(&pppoe_s_req, cliSession->sess, &from_addr);
				cliSession->isUsingNow = 1;
				cliSession->discoverystate = SERVER_STATE_SESSION;
				memcpy(cliSession->hwaddr, from_addr.sll_addr, Eid_SZ);
				_pppoe_server_mac(mac, (char*)&packet->ehdr.from);
				session_status();
				//printf("===================%s(%d) session id = 0x%x, client mac address  %s\n", __FUNCTION__, __LINE__, packet->session, mac);

				break;
			case CODE_PADT:
				//printf("===================%s(%d) Received PADT packet with session %d\n", __FUNCTION__, __LINE__, packet->session);
				freeSession(packet->session);
				session_status();
				break;
			default:
				break;
		}
	}
}
#endif
#endif

#ifdef CONFIG_USER_PPPOE_2_INTERVALS_RETRY
int get_poe_timeout(struct sppp *sp, int state)
{
	int timeoutOrg = 0;
	int timeoutval = 0;
	int timediff = 0;
	//rosetelecom spec 2019
	int retry_threshold = 6;	//t1=6
	int retry_threshold_2 = 16;	//t2=16
	int retry_delay = 10;		//d1=10
	int retry_delay_2 = 30;		//d2=30
	int retry_delay_long = 300;	//d3=300

	if(state == 1)
	{
		// if auth fail , the counter should minus 1 and return relative timeout value
		if( (sp->ctc_retry_cnt+1) >= retry_threshold_2) timeoutOrg=retry_delay_long;
		else if((sp->ctc_retry_cnt+1) >= retry_threshold) timeoutOrg=retry_delay_2;
		else timeoutOrg=retry_delay;
	}
	else
	{
		if((sp->ctc_retry_cnt+1) >= retry_threshold_2) timeoutOrg=retry_delay_long;
		else if((sp->ctc_retry_cnt+1)>= retry_threshold) timeoutOrg=retry_delay_2;
		else timeoutOrg=retry_delay;
		sp->ctc_retry_cnt++;

	}
	timeoutval = timeoutOrg;
	timediff = (((send_padt_time[sp->if_unit]-send_padi_time[sp->if_unit])*1000+(send_padt_mtime[sp->if_unit]-send_padi_mtime[sp->if_unit])+500)/1000);
	timeoutval = timeoutval - timediff;
	if(timeoutval > timeoutOrg || timeoutval <=0 ) {
		timeoutval=timeoutOrg;
	}

	return timeoutval;
}
#endif

#ifdef PPPOE_UNIVERSAL_RETRY_INTERVAL
extern pppoe_retry_cycle_t g_retry_cycle;

int get_retry_timeout(struct sppp *sp, int state)
{
	int interval = 0;
	int real_interval = 0;
	int time_aleary_use = 0;
	int timenow = 0;

	if(state == PPPOE_RETRY_DISCOVER)
	{
		switch(g_retry_cycle.cycle_type)
		{
			case PPPOE_RETRY_TYPE_MACRO_CYCLE:
				if(sp->retry_cnt > g_retry_cycle.interval_cnt){
					sp->retry_cnt = 0;
				}
				break;
			case PPPOE_RETRY_TYPE_LONG_TAIL:
				//do not reset, sp->retry_cnt will set 0 after ppp sussess.
				break;
		}
	
		if(sp->retry_cnt < g_retry_cycle.interval_cnt){
			interval = g_retry_cycle.interval[sp->retry_cnt];
			sp->retry_cnt++;
		}
		else{
			interval = g_retry_cycle.big_cycle;
			sp->retry_cnt++;
		}

		real_interval = interval;
	}
	else if(state == PPPOE_RETRY_SESSION)
	{
		if(0 == sp->retry_cnt){
			/* when in session state, when ppp get IP, sp->retry_cnt will set to 0 */
			interval = g_retry_cycle.interval[sp->retry_cnt];
		}
		else if((sp->retry_cnt - 1) < g_retry_cycle.interval_cnt){
			interval = g_retry_cycle.interval[sp->retry_cnt - 1];
		}
		else{
			interval = g_retry_cycle.big_cycle;
		}

		timenow = getSYSInfoTimer();
		time_aleary_use = timenow - padi_start_time[sp->if_unit];
		real_interval = (time_aleary_use > interval)?0:(interval - time_aleary_use);
	}
	
	if(sp->debug){
		addlog(PPPOE_OPT "mode:%s, state: %s, get retry timeout interval %d, real_interval %d, retry_cnt is %d now\n", 
			PPPOE_ARGS(sp->if_unit), (g_retry_cycle.cycle_type == PPPOE_RETRY_TYPE_MACRO_CYCLE)?"MACRO_CYCLE":"LONG_TAIL",
			(state == PPPOE_RETRY_DISCOVER)?"DISCOVER":"SESSION", 
			interval, real_interval, sp->retry_cnt);
	}
	
	return real_interval;
}
#endif

extern int ppp_up_flag[N_SPPP];
void process_poe()
{
	struct sppp *sp;
	PPPOE_DRV_CTRL *p;
	unsigned char spppd_padt_retry=0;

#if defined(CONFIG_USER_PPPOE_PROXY) || defined(CONFIG_USER_PPPOE_SERVER)
	//printf("===================%s(%d) pppoe_server_socket = %d\n", __FUNCTION__, __LINE__, pppoe_server_socket);
	if(-1 != pppoe_server_socket){
		pppoe_server_rcv();
	}
#endif


	sp = spppq;
	while(sp) {
		if ((sp->over != SPPP_PPPOE) || (sp->role == 1)) {
			goto next;
		}
		if (!(p = sp->pp_lowerp)) {
			goto next;
		}
		if (p->in_pppoe_disc) {
			if (p->DiscoveryState == PPPOE_STATE_SESSION) {
				p->in_pppoe_disc = 0;
				disc_counter[p->ifno] = 0;
				UNTIMEOUT(0, 0, p->disc_ch);
				//printf("pppoe on session: %s\n", sp->if_name);
				printf("pppoe on session:: %s: Session ID %d\n", sp->if_name, p->session);
				syslog(LOG_INFO, "spppd: %s: AC MAC address %02X:%02X:%02X:%02X:%02X:%02X\n"
					, sp->if_name, p->ACEthAddr[0], p->ACEthAddr[1], p->ACEthAddr[2], p->ACEthAddr[3], p->ACEthAddr[4], p->ACEthAddr[5]);
				syslog(LOG_INFO, "spppd: %s: PPPoE Session ID %d\n", sp->if_name, p->session);
				start_ppp_real(sp);
				goto next;
			}
			if (!FD_ISSET(p->disc_sock, &poe_in_fds)) {
				// pppoe been deleted
				p->in_pppoe_disc = 0;
				goto next;
			}
			if (p->disc_timeout) {
#ifdef CONFIG_SUPPORT_AUTO_DIAG
				int curState = p->DiscoveryState;
#endif
				int pp_phase = sp->pp_phase;
				
				pppoe_shut(p);
				pre_PADT_flag[p->ifno] = 0;
				sendPADR_flag[p->ifno] = 0;
				disc_counter[p->ifno] ++;	// Jenny, for counting discovery timeout
#if defined(CONFIG_USER_PPPOE_CTC_DELAY_INT) || defined (CONFIG_USER_PPPOE_2_INTERVALS_RETRY)
				send_padi_time[p->ifno] = 0;
				send_padi_mtime[p->ifno] = 0;
				send_padt_time[p->ifno] = 0;
				send_padt_mtime[p->ifno] = 0;
#endif

				//pre_release_pppoe();	// Jenny, send PADT to disconnect previous session after discovery timeout
				sppp_last_connection_error(sp->if_unit, ERROR_ISP_TIME_OUT, NULL);	// Jenny, send PADI, no reponse from PPPoE server
				
				sp->pp_phase = PHASE_DEAD;
				if(pp_phase != sp->pp_phase) {
					ppp_status();
				}
				
				UNTIMEOUT(0, 0, p->ch);
#ifdef PPP_DELAY_UP
				//UNTIMEOUT(pre_release_pppoe, 0, sp->pppoe_up_delay);
				UNTIMEOUT(sppp_delay_up, 0, sp->ppp_up_delay);
#endif
#ifdef CONFIG_SUPPORT_AUTO_DIAG
#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingNumberEnable_) \
|| defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingErrorEnable_)
				if(curState <= PPPOE_STATE_SENT_PADI)
					pppoe_dialing_info_update(DIALING_ERROR_678);
				else
					pppoe_dialing_info_update(DIALING_ERROR_676);
#endif
#endif
				if (FD_ISSET(p->disc_sock, &poe_in_fds))
					pppoe_remove_fd(p->disc_sock);
#ifdef CONFIG_PPPOE_OUT_OF_ORDER
				if (FD_ISSET(p->sess_in_disc_sock, &poe_in_fds))
					pppoe_remove_fd(p->sess_in_disc_sock);
#endif				
				p->in_pppoe_disc = 0;
				//printf("disc timeout\n");
#ifdef CONFIG_SUPPORT_AUTO_DIAG
				if((SPPP_PPPOE == sp->over)&&(0 == sp->role)&&(0 != sp->simulation))
				{
					char cmd[64];				

					if(curState > PPPOE_STATE_SENT_PADI)
					{
						sppp_last_connection_error(sp->if_unit, ERROR_ISP_DISCONNECT, NULL);
					}
					if(1 == sp->simulation)
					{
						if(curState <= PPPOE_STATE_SENT_PADI)
						{
							//send padi multi times, and no pado reply
							DEBUG_SIMU("[%s %d]state=%d, pp_error=%d\n", __func__, __LINE__, DIAG_TIMEOUT, sp->pp_error);
							ppp_manualdiag_log(sp, DIAG_TIMEOUT, sp->pp_error, 0, NULL);
							sp->simulation *= 2;
							printf("[%s %d]Timeout at PADI(unit=%d).\n", __func__, __LINE__, sp->unit);
						}
						else
						{
							//receive pado and send padr multitimes, no pads reply.
							DEBUG_SIMU("[%s %d]state=%d, pp_error=%d\n", __func__, __LINE__, DIAG_PARAMNEGOFAIL, sp->pp_error);
							ppp_manualdiag_log(sp, DIAG_PARAMNEGOFAIL, sp->pp_error, 0, NULL);
							sp->simulation *= 2;
							printf("[%s %d]Timeout at PADO(unit=%d).\n", __func__, __LINE__, sp->unit);
						}
					}
					
					DEBUG_SIMU("[%s %d]del_ppp(unit=%d).\n", __func__, __LINE__, sp->unit);
					snprintf(cmd, 64, "/bin/spppctl del %d", sp->if_unit);
					system(cmd);
				}
#endif
			}
			pppoe_recv(p);
#ifdef CONFIG_PPPOE_OUT_OF_ORDER
			pppoe_recv_session(p);
#endif
		}
		else { // not in PPPoE Discovery stage
			if (sp->up_flag && ppp_up_flag[sp->if_unit] && (p->DiscoveryState == PPPOE_STATE_IDLE || p->DiscoveryState == PPPOE_STATE_TERMINATED)) {
				int pp_phase = sp->pp_phase;

#ifdef CONFIG_SUPPORT_AUTO_DIAG
				if((SPPP_PPPOE == sp->over)&&(0 != sp->simulation)&&(p->DiscoveryState == PPPOE_STATE_TERMINATED))
				{
					//simulation ppp connection does not need re-dial
					char cmd[64];
					printf("[%s %d]del_ppp(unit=%d).\n", __func__, __LINE__, sp->unit);
					sp->simulation *= 2;
					snprintf(cmd, 64, "/bin/spppctl del %d", sp->if_unit);
					system(cmd);
					goto next;
				}
#endif
				mib_get_s(MIB_SPPPD_PADT_RETRY,(void *)&spppd_padt_retry, sizeof(spppd_padt_retry));
			#ifdef CONFIG_NEC_PPP_CUSTOMIZATION
				if (pre_PADT_flag[sp->if_unit] == 0)
					pre_PADT_RETRY_COUNT[sp->if_unit] = 0;
			#endif
				if ( pre_PADT_flag[sp->if_unit] || ((spppd_padt_retry!=0) && (pre_PADT_RETRY_COUNT[sp->if_unit] > spppd_padt_retry))) 
				{
					int ifindex=0;
					int timeVal;
					
#ifdef CONFIG_USER_PPPOE_PROXY
					if (sp->enable_pppoe_proxy && !is_pppoe_proxy_up())
						goto next;
#endif
					if (!FD_ISSET(p->disc_sock, &poe_in_fds))
						pppoe_add_fd(p->disc_sock);
#ifdef CONFIG_PPPOE_OUT_OF_ORDER
					if (!FD_ISSET(p->sess_in_disc_sock, &poe_in_fds)){
						//printf("pppoe_add_fd p->sess_in_disc_sock\n");
						pppoe_add_fd(p->sess_in_disc_sock);	
					}
#endif

					p->in_pppoe_disc = 1;
					p->disc_timeout = 0;
					sp->lastErrState = 0;
#ifndef CONFIG_NEC_PPP_CUSTOMIZATION
#if defined(CONFIG_USER_PPPOE_2_INTERVALS_RETRY)
					timeVal = get_poe_timeout(sp,0);
#elif defined(CONFIG_USER_PPPOE_CTC_DELAY_INT)
					timeVal = get_poe_ctcto(sp,0);
#elif defined(PPPOE_UNIVERSAL_RETRY_INTERVAL)
					timeVal = get_retry_timeout(sp, PPPOE_RETRY_DISCOVER);
#elif defined(CONFIG_TELMEX_DEV)
					timeVal = 64;
#else
					timeVal = 10;
#endif
					TIMEOUT((void *)discTimeout, (void *)p, timeVal, p->disc_ch);
#endif
					p->debug = sp->debug;
					p->ifno = sp->if_unit;

					#ifdef EMBED
					if(strncmp(p->name,ALIASNAME_NAS,3)==0)
					{
						sscanf(p->name, "nas0_%d", &ifindex);
						p->uifno = TO_IFINDEX(MEDIA_ETH, sp->if_unit, ETH_INDEX(ifindex));
					}
					else if(strncmp(p->name,ALIASNAME_VC,2)==0)
					{
						sscanf(p->name, "vc%d", &ifindex);
						p->uifno = TO_IFINDEX(MEDIA_ATM, sp->if_unit, VC_INDEX(ifindex));
					}
					else if(strncmp(p->name,ALIASNAME_PTM,3)==0)
					{
						sscanf(p->name, "ptm%d", &ifindex);
						p->uifno = TO_IFINDEX(MEDIA_PTM, sp->if_unit, PTM_INDEX(ifindex));
					}
					else if(strncmp(p->name,"wlan", 4)==0)
					{
						//sscanf(p->name, "ptm%d", &ifindex);
						ifindex = 0;
						p->uifno = TO_IFINDEX(MEDIA_WLAN, sp->if_unit, ETH_INDEX(ifindex));
					}
					else
						printf("Error!, unknown device\n");
					#else
					p->uifno = 0x1234;
					#endif
#ifdef PPP_DELAY_UP
#ifdef PPP_UP_MAX_DELAY
					sp->ppp_delay = PPP_UP_MAX_DELAY;
#else
					sp->ppp_delay = 5;
#endif
					TIMEOUT((void *)sppp_delay_up, (void *)sp, sp->ppp_delay, sp->ppp_up_delay);
#else
					sp->pp_phase = PHASE_INITIALIZE;
					if(pp_phase != sp->pp_phase) {
						ppp_status();
					}
					pppoe_up(p);
#endif
					//printf("start disc: %s\n", sp->if_name);
				}
				else{
					pre_release_pppoe(p->name);
				}
			}
			else if (sp->up_flag && ppp_up_flag[sp->if_unit] && (p->DiscoveryState == PPPOE_STATE_SESSION)) {
				// in case of receiving PADT in session
#ifdef CONFIG_USER_PPPOE_CTC_DELAY_INT	
				if(auth_fail_counter[sp->unit] && sp->pp_phase != PHASE_TERMINATE)
				{
					if(sp->debug) printf("[%s %d] ppp%d, auth fail, going to down !!!\n", __FUNCTION__, __LINE__, sp->unit);
					extern void sppp_cp_terminate(void *cookie);
					sppp_cp_terminate(sp);
				}
#endif	
#if 0 //def CONFIG_SUPPORT_AUTO_DIAG
				if((0 != sp->simulation) && ( hasSessionPacket()<=0))
				{
					pppoe_recv(p);
				}
				else
#endif
				pppoe_recv(p);
			}
#ifdef CONFIG_SUPPORT_AUTO_DIAG
			if((ppp_up_flag[sp->if_unit]==0)&&(SPPP_PPPOE == sp->over)&&(0 != sp->simulation))
			{
				char cmd[64];
				sppp_last_connection_error(sp->if_unit, ERROR_ISP_DISCONNECT, NULL);
				DEBUG_SIMU("[%s %d]state=%d, pp_error=%d\n", __func__, __LINE__, DIAG_TIMEOUT, sp->pp_error);
				ppp_manualdiag_log(sp, DIAG_TIMEOUT, ERROR_ISP_DISCONNECT, 0, NULL);//ERROR_ISP_TIME_OUT=1
				sp->simulation *= 2;
				DEBUG_SIMU("[%s %d]del_ppp(unit=%d).\n", __func__, __LINE__, sp->unit);
				snprintf(cmd, 64, "/bin/spppctl del %d", sp->if_unit);
				system(cmd);
			}
#endif
		}
next:
		sp = sp->pp_next;
	}
   	return;
}

#ifdef CONFIG_PPPOE_OUT_OF_ORDER
void pppoe_delete_sess_in_disc_sock(PPPOE_DRV_CTRL *p)
{
	if (FD_ISSET( p->sess_in_disc_sock, &poe_in_fds))
		pppoe_remove_fd(p->sess_in_disc_sock);

	if (p->sess_in_disc_sock >= 0) {
		if(p->debug){
			printf("[pppoe]close sock of session in discover state. sock is %d \n", p->sess_in_disc_sock);
		}
		close(p->sess_in_disc_sock);
		p->sess_in_disc_sock = -1;
	}	

	return;
}
#endif

/**********************************************************************
* FUNCTION: pppoeDelete
* ARGUMENTS:
* 	unit		-- pppoe unit number
* 	pDevice		-- attach to ethernet device
* 	dunit 		-- attach to ethernet device number
* RETURNS: STATUS
* DESCRIPTION: Delete and detach PPPoE unit
***********************************************************************/
int pppoeDelete(struct sppp *sp)
{
	int retval;

	if(sp == NULL)
		return -1;
	if(sp->pp_lowerp == NULL)
		return -1;

	PPPOE_DRV_CTRL *p = sp->pp_lowerp;
//	printf("pppoeDelete..\n");

	if (p != NULL) {
		/* send PADT will cause server terminate immediatly */
		sendPADT(p);

		UNTIMEOUT(0, 0, p->ch);
		UNTIMEOUT(0, 0, p->disc_ch);

		p->sp.sa_addr.pppoe.sid = 0;

	// Kaohj
    //retval = connect(p->fd, (struct sockaddr*)&p->sp,
	//	  sizeof(struct sockaddr_pppox));
		if(p->fd >=0){
		close(p->fd);
		p->fd = -1;
		}

	/* test direct mode */
		//if (FD_ISSET( p->disc_sock, &poe_in_fds) || p->disc_sock >= 0) {
		if (FD_ISSET( p->disc_sock, &poe_in_fds))
			pppoe_remove_fd(p->disc_sock);
		if (p->disc_sock >= 0) {
			close(p->disc_sock);
			p->disc_sock = -1;
		}

#ifdef CONFIG_PPPOE_OUT_OF_ORDER
		pppoe_delete_sess_in_disc_sock(p);	
#endif
		
		free(p);
		sp->pp_lowerp = NULL;
	}
	//sp->pp_lowerp = NULL;
	//
	return (0);
}


int pppoe_output(PPPOE_DRV_CTRL *p, void *buf, int data_len)
{
	int error;
	error = sendto(p->disc_sock, buf, data_len, 0,
		   (struct sockaddr*) &p->remote,
		   sizeof(struct sockaddr_ll));
	return error;
}

// Jenny, get PPPoE session information
static void pppoe_session_get(PPPOE_DRV_CTRL *p)
{
	#ifdef EMBED
	unsigned int totalEntry;
	MIB_CE_PPPOE_SESSION_T Entry;
	int i, found=0, tmpEntry=-1;

	p->session = 0;
#ifdef CONFIG_NEC_PPP_CUSTOMIZATION
	p->disconnect_flag = 0;
#endif
	totalEntry = mib_chain_total(MIB_PPPOE_SESSION_TBL); /* get chain record size */
	for (i=0; i<totalEntry; i++) {
		if (!mib_chain_get(MIB_PPPOE_SESSION_TBL, i, (void *)&Entry)) {
			printf("Get chain record error!\n");
			return;
		}

		if (Entry.uifno == p->uifno ) {
   		    memcpy((unsigned char *)p->ACEthAddr, (unsigned char *)Entry.acMac, 6);
			p->session = Entry.sessionId;
		#ifdef CONFIG_NEC_PPP_CUSTOMIZATION
			p->disconnect_flag = Entry.disconnect_flag;
		#endif
			//printf("uifno = 0x%x, Got session id 0x%x\n",p->uifno,p->session);
			return;
		}
	}
	#else
	char fakeAC[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
	p->session = 0;
	memcpy((unsigned char *)p->ACEthAddr, fakeAC, 6);
	#endif
}


int get_ifindex(char *dev)
{
	int s = 0;
	struct ifreq req = {0};
	int err = 0;
	strncpy(req.ifr_name, dev, sizeof(req.ifr_name)-1);
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("[%s %d]:socket() error.\n", __func__, __LINE__);
		exit(1);
	}
	err = ioctl(s, SIOCGIFINDEX, &req);
	close(s);
	if (err == -1)
	{
		printf("[%s %d]:ioctl error.\n", __func__, __LINE__);
		exit(1);
	}

	return req.ifr_ifindex;
}

//send LCP terminate Request after reboot
void sendLcpTeminateReq(PPPOE_DRV_CTRL *p, struct sppp *sp, char* interface_name)
{
	struct sockaddr_ll my_etheraddr;
	int ppp_socket = -1;
	unsigned char pkt_buf_lcp[1400]={0};
	struct PPPoEPacket *packet_lcp;
	unsigned char LCP_terminate[6] = {0xc0,0x21,0x05,0x01,0x00,0x04};
	unsigned short plen = 0;
	int len, ret=0;
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(struct ifreq));

	strncpy(ifr.ifr_name, interface_name, sizeof(ifr.ifr_name)-1);
	ret = ioctl (p->disc_sock, SIOCGIFHWADDR, &ifr);
	if (ret < 0) {
		printf("Bad device name: %s  (%m)",p->name);
		return;
	}
	memset(&my_etheraddr, 0, sizeof(my_etheraddr));
	my_etheraddr.sll_family = AF_PACKET;
	my_etheraddr.sll_protocol = htons(ETH_P_PPP_SES);
	my_etheraddr.sll_hatype = htons(ARPHRD_ETHER);
	my_etheraddr.sll_pkttype = PACKET_BROADCAST;
	my_etheraddr.sll_halen = ETH_ALEN;
	my_etheraddr.sll_ifindex = get_ifindex(interface_name);
	memcpy(&my_etheraddr.sll_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	ppp_socket = socket(PF_PACKET, SOCK_DGRAM, 0);

	if (ppp_socket < 0)
	{
		printf("[%s %d]:open socket error\n", __func__, __LINE__);
		return;
	}
	/* bind to socket */
	if (bind(ppp_socket, (struct sockaddr*)&my_etheraddr, sizeof(my_etheraddr)))
	{
		printf("[%s %d]:bind socket error\n", __func__, __LINE__);
		close(ppp_socket);
		ppp_socket = -1;
		return;
	}

	//send LCP terminate request
	memset(pkt_buf_lcp, 0, sizeof(pkt_buf_lcp));
	packet_lcp = (struct PPPoEPacket *)pkt_buf_lcp;
	packet_lcp->ver_type = 0x1;
	packet_lcp->code = CODE_SESS;
	packet_lcp->session = htons(p->session);
	plen += TAG_HDR_SIZE;

	plen = format_lcp_packet(PPP_LCP, 5, ++sp->pp_seq[IDX_LCP], 0, packet_lcp->payload);

	packet_lcp->length = htons(plen);

	len = (int) (plen + PPPOE_OVERHEAD);
	memcpy(&packet_lcp->payload, LCP_terminate, sizeof(LCP_terminate));
	if (packet_lcp->session != 0)
	{
		memcpy(&my_etheraddr.sll_addr, &p->ACEthAddr, Eid_SZ);
		ret = sendto(ppp_socket, (void *)packet_lcp+Ehdr_SZ, len, 0, (struct sockaddr *)&my_etheraddr, sizeof(my_etheraddr));
		if(ret < 0){
			printf("send LCP term error! ret = %d\n", ret);
			perror("Could not send data: ");
			close(ppp_socket);
			ppp_socket = -1;
			return;
		}
	}

	if (sp->debug) {
		char mac1[18], mac2[18];
		mac1[0] = mac2[0] = 0;
		_pppoe_mac(mac1, (char*)&p->MyEthAddr);
		_pppoe_mac(mac2, (char*)&p->ACEthAddr);
		addlog(PPPOE_FMT"O LCP TermReq Session ID=%04X from %s to %s\n", PPPOE_ARGS(p->ifno), ntohs(packet_lcp->session), mac1, mac2);
	}

	close(ppp_socket);
}

// Jenny, send PADT packet to release existed PPPoE connection
void pre_release_pppoe(char* interface_name)
{
	PPPOE_DRV_CTRL *p;
	struct sppp *sp;
	unsigned char pkt_buf[256];
	struct PPPoEPacket *packet;
#ifdef PPPOE_EOL_TAG
	struct PPPoETag eol;	// EOL
	unsigned char *cursor;	// EOL
#endif
	unsigned short plen = 0;
	int len;
	#ifdef EMBED
	unsigned int totalEntry;
	MIB_CE_PPPOE_SESSION_T Entry;
	#endif
	int i = 0,ifindex = 0, ret = 0;
	unsigned int uifno;
	unsigned char spppd_padt_retry=0;
	mib_get_s(MIB_SPPPD_PADT_RETRY,(void *)&spppd_padt_retry, sizeof(spppd_padt_retry));

	for (sp=spppq; sp; sp=sp->pp_next) {
		if (1 == sp->role)//VPN Server has nothing to do with wan interface link status
			continue;
		if (sp->over != SPPP_PPPOE)
			continue;
		if (pre_PADT_flag[sp->if_unit] || ((spppd_padt_retry!=0) && (pre_PADT_RETRY_COUNT[sp->if_unit] > spppd_padt_retry)))
			continue;
		p = sp->pp_lowerp;
		if (!p)
			continue;
		if(strcmp(p->name, interface_name)) {
			//printf("%s %d\n", __func__, __LINE__);
			continue;
		}

		p->ifno = sp->if_unit;

		#ifdef EMBED
		if(strncmp(p->name,ALIASNAME_NAS,3)==0)
		{
			sscanf(p->name, "nas0_%d", &ifindex);
			p->uifno = TO_IFINDEX(MEDIA_ETH, sp->if_unit, ETH_INDEX(ifindex));
		}
		else if(strncmp(p->name,ALIASNAME_VC,2)==0)
		{
			sscanf(p->name, "vc%d", &ifindex);
			p->uifno = TO_IFINDEX(MEDIA_ATM, sp->if_unit, VC_INDEX(ifindex));
		}
		else if(strncmp(p->name,ALIASNAME_PTM,3)==0)
		{
			sscanf(p->name, "ptm%d", &ifindex);
			p->uifno = TO_IFINDEX(MEDIA_PTM, sp->if_unit, PTM_INDEX(ifindex));
		}
		else if(strncmp(p->name,"wlan", 4)==0)
		{
			//sscanf(p->name, "wlan%d", &ifindex);
			ifindex = 0;
			p->uifno = TO_IFINDEX(MEDIA_WLAN, sp->if_unit, ETH_INDEX(ifindex));
		}
		else
			printf("Error!, unknown device\n");
		#else
		p->uifno = 0x1234;
		#endif

		pppoe_session_get(p);

#ifdef CONFIG_NEC_PPP_CUSTOMIZATION
		if (p->disconnect_flag)
		{
			if (!rtk_link_up_send_pppoe_disconnect(p->name))
				return;
			p->disconnect_flag = 0;
			p->session = 0;
		}
#else
		// send LCP  terminate request after reboot
		sendLcpTeminateReq(p, sp, p->name);
		packet = (struct PPPoEPacket *)pkt_buf;
#ifdef PPPOE_EOL_TAG
		cursor = packet->payload;	// EOL
#endif
		memcpy(&packet->ehdr.to, p->ACEthAddr, Eid_SZ);
		memcpy(&packet->ehdr.from, p->MyEthAddr, Eid_SZ);
		//Modify to send PADT by broadcast
		//memcpy(p->remote.sll_addr, &packet->ehdr.to, Eid_SZ);
		memset(p->remote.sll_addr, 0xff, Eid_SZ);

//		bcopy((char*)p->ACEthAddr, (char*)&packet->ehdr.to, Eid_SZ);
//		bcopy((char*)p->MyEthAddr, (char*)&packet->ehdr.from, Eid_SZ);
		packet->ehdr.type = htons(ETH_PPPOE_DISCOVERY);
		packet->ver_type = 0x11;
		packet->code = CODE_PADT;
		packet->session = htons(p->session);
		p->session = 0;
#ifdef PPPOE_EOL_TAG	// EOL
		eol.type = htons(TAG_END_OF_LIST);
		eol.length = htons(0);
		memcpy(cursor, &eol, TAG_HDR_SIZE);
		plen += TAG_HDR_SIZE;
#endif

		packet->length = htons(plen);
		//len = (int) (plen + PPPOE_HDR_SIZE);
		len = (int) (plen + PPPOE_OVERHEAD);
		if (packet->session != 0)
		{
			//printf("Send PADT with session id 0x%x\n",packet->session);
			//PPPOE_LOWER_SEND(p, packet, len);
			// broadcast for PADT
			ret = PPPOE_LOWER_SEND(p, (unsigned char*)packet+Ehdr_SZ, len);
			if(ret < 0){
					printf("[%s:%d] send broadcast for PADT failed ! ret = %d\n", __FUNCTION__, __LINE__, ret);
			}
			else
			{
				// unicast for PADT, previous PPPoE session not release after reboot
				memcpy(p->remote.sll_addr, &packet->ehdr.to, Eid_SZ);
				ret = PPPOE_LOWER_SEND(p, (unsigned char*)packet+Ehdr_SZ, len);
				if(ret < 0){
					printf("[%s:%d] send unicast for PADT failed ! ret = %d\n", __FUNCTION__, __LINE__, ret);
				}
			}
		}
#endif
		p->session = PPPOE_STATE_IDLE;
		pre_PADT_flag[p->ifno] = 1;
		pre_PADT_RETRY_COUNT[p->ifno]++;
		//sleep for 1 second. for inter-op with ISP, make sure previous session is closed.
		//sleep(1);
		if (sp->debug) {
			/*addlog(PPPOE_FMT"O PADT Session ID=%04X from ", PPPOE_ARGS(p->ifno), packet->session);
			_pppoe_print_mac((char*)&packet->ehdr.from);
			addlog(" to ");
			_pppoe_print_mac((char*)&packet->ehdr.to);
			addlog("\n");*/
			char mac1[18], mac2[18];
			mac1[0] = mac2[0] = 0;
			_pppoe_mac(mac1, (char*)&packet->ehdr.from);
			_pppoe_mac(mac2, (char*)&packet->ehdr.to);
			addlog(PPPOE_FMT"O PADT Session ID=%04X from %s to %s\n", PPPOE_ARGS(p->ifno), ntohs(packet->session), mac1, mac2);
		}
	}
}


const char PPPOE_UPFLAG[] = "/tmp/ppp_up_log";
void pppoe_up_log(int ifno, int value)
{
	char buff[10]="";
	FILE *fp;
	struct sppp *sp;

	fp = fopen(PPPOE_UPFLAG, "w");
	if(fp !=NULL)
	{
		sp = spppq;
		while(sp) {
			if(sp->over == SPPP_PPPOATM) {
				sprintf(buff, "%d 1\n", sp->if_unit);
			}
#ifdef CONFIG_USER_PPPOMODEM
			else if(sp->over == SPPP_PPPOMODEM) {
				//sp->up_flag = 1;//????
				sprintf(buff, "%d 1\n", sp->if_unit);
			}
#endif //CONFIG_USER_PPPOMODEM
			else {
				sprintf(buff, "%d %d\n", sp->if_unit, sp->up_flag);
			}
			fputs(buff, fp);
			sp = sp->pp_next;
		}
		fclose(fp);
	}
	else
		printf("[%s:%d] open file failed! \n", __FUNCTION__, __LINE__);
}

#if defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingNumberEnable_)\
|| defined(_PRMT_X_CT_COM_PERFORMANCE_REPORT_SUBITEM_DialingErrorEnable_)
const char PPPOE_DIALING_INFO[] = "/tmp/ppp_dialing_info";
#define PPPOE_DIALING_INFO_LOCKFILE		"/var/run/pppoe_dialing_info.lock"

/*
*10:2:3:1:1:1	//dialing number:DialingError678:DialingError676:DialingError691:DialingError999:padiSendFlag
*/
void pppoe_dialing_info_update(int flag)
{
	char buff[64]="";
	char tmp[64]="";
	FILE *fp,*fp1;
	int DialingNumber = 0,Error678= 0,Error676= 0,Error691= 0,Error999= 0,padiSendFlag=0;
	//printf("[ppp_dialing_info debug]00\n");
	int lockfd = 0;

	if ((lockfd = lock_file_by_flock(PPPOE_DIALING_INFO_LOCKFILE, 1)) == -1)
	{
		printf("%s, the file %s have been locked\n", __FUNCTION__, PPPOE_DIALING_INFO_LOCKFILE);
		return;
	}

	if (fp = fopen(PPPOE_DIALING_INFO, "r")) {
		
		//printf("[ppp_dialing_info debug]11\n");
		while (fgets(tmp, sizeof(tmp), fp) != NULL) {
			//printf("[ppp_dialing_info debug]22 tmp  %s\n",tmp);
			sscanf(tmp, "%d:%d:%d:%d:%d:%d", &DialingNumber,&Error678,&Error676,&Error691,&Error999,&padiSendFlag);
			break;
		}
		fclose(fp);
	} 

	fp1 = fopen(PPPOE_DIALING_INFO, "w");
	if(fp1)
	{
		//printf("[ppp_dialing_info debug]3 flag %d\n",flag);
		//printf("%d:%d:%d:%d:%d:%d\n",DialingNumber,Error678,Error676,Error691,Error999,padiSendFlag);
		if(flag == DIALING_NUMBER)
		{			
			DialingNumber++;
			padiSendFlag=1;
		}else if(padiSendFlag == 1)
		{
			switch(flag)
			{
				case DIALING_ERROR_678:
					Error678++;
					break;
				case DIALING_ERROR_676:
					Error676++;
					break;
				case DIALING_ERROR_691:
					Error691++;
					break;
				case DIALING_ERROR_999:
					Error999++;
					break;
				default:
					break;
			}
			padiSendFlag=0;
		}
		sprintf(buff,"%d:%d:%d:%d:%d:%d",DialingNumber,Error678,Error676,Error691,Error999,padiSendFlag);
		//printf("[ppp_dialing_info debug]4 buff %s\n",buff);
		fputs(buff, fp1);
		fclose(fp1);
	}
	unlock_file_by_flock(lockfd);
}
#endif

