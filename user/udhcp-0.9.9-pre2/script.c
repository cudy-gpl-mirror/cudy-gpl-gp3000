/* script.c
 *
 * Functions to call the DHCP client notification scripts
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

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "dhcpd.h"
#include "dhcpc.h"
#include "packet.h"
#include "options.h"
#include "debug.h"
#include <config/autoconf.h>
#include <rtk/options.h>
#include <rtk/utility.h>
#include <time.h>

/* get a rough idea of how long an option will be (rounding up...) */
static int max_option_length[] = {
	[OPTION_IP] =		sizeof("255.255.255.255 "),
	[OPTION_IP_PAIR] =	sizeof("255.255.255.255 ") * 2,
	[OPTION_STRING] =	1,
	[OPTION_BOOLEAN] =	sizeof("yes "),
	[OPTION_U8] =		sizeof("255 "),
	[OPTION_U16] =		sizeof("65535 "),
	[OPTION_S16] =		sizeof("-32768 "),
	[OPTION_U32] =		sizeof("4294967295 "),
	[OPTION_S32] =		sizeof("-2147483684 "),
	[OPTION_120] =	1,
#ifdef _CWMP_TR111_
	[OPTION_125] =	1,
#endif
};


static int upper_length(int length, struct dhcp_option *option)
{
	return max_option_length[option->flags & TYPE_MASK] *
	       (length / option_lengths[option->flags & TYPE_MASK]);
}


static int sprintip(char *dest, char *pre, unsigned char *ip)
{
	return sprintf(dest, "%s%d.%d.%d.%d", pre, ip[0], ip[1], ip[2], ip[3]);
}


/* really simple implementation, just count the bits */
static int mton(struct in_addr *mask)
{
	int i;
	unsigned long bits = ntohl(mask->s_addr);
	/* too bad one can't check the carry bit, etc in c bit
	 * shifting */
	for (i = 0; i < 32 && !((bits >> i) & 1); i++);
	return 32 - i;
}

//wpeng 20120411 added for parsestatic router option
#ifdef CONFIG_USER_DHCP_OPT_33_249
static int sprintpartialip(char *dest, int masklen, int octets, unsigned char *ip)
{
	if(octets == 4)
		return sprintf(dest, "%d.%d.%d.%d/%d", ip[0], ip[1], ip[2], ip[3], masklen);
	else if(octets == 3)
		return sprintf(dest, "%d.%d.%d.%d/%d", ip[0], ip[1], ip[2], 0, masklen);
	else if(octets == 2)
		return sprintf(dest, "%d.%d.%d.%d/%d", ip[0], ip[1], 0, 0, masklen);
	else if(octets == 1)
		return sprintf(dest, "%d.%d.%d.%d/%d", ip[0], 0, 0, 0, masklen);
	else if(octets == 0)
		return sprintf(dest, "%d.%d.%d.%d/%d", 0, 0, 0, 0, masklen);
	return -1;
}

#define PMAP_VC_START	1
#define PMAP_PPP_START	0x10
#define PMAP_NAS_START 0x20
#define PMAP_NAS_PPP_START 0x30

#define CHECK_NAS_IDX(a) (a >> 16)

int caculate_tblid(uint32_t ifid)
{
	int tbl_id;

	uint32_t ifindex;
	
	ifindex = ifid;

	//ifindex of nas0_* is 0x1ff01, 0x1ff02
	//ifindex of ppp* over nas0 is 0x10000, 0x10001
#ifdef CONFIG_ETHWAN
	if(CHECK_NAS_IDX(ifindex))
	{
		if(PPP_INDEX(ifindex) == DUMMY_PPP_INDEX)
			tbl_id = VC_INDEX(ifindex) + PMAP_NAS_START;
		else
			tbl_id = PPP_INDEX(ifindex) + PMAP_NAS_PPP_START;
	}
	else
#endif
	{
	//ifindex of vc* is 0xff01, 0xff02, ...
	//ifindex of ppp* is 0x00, 0x0101, 0x0202 ...
	if (PPP_INDEX(ifindex) == DUMMY_PPP_INDEX)
		tbl_id = VC_INDEX(ifindex) + PMAP_VC_START;
	else
		tbl_id = PPP_INDEX(ifindex) + PMAP_PPP_START;
	}

	return tbl_id;
}

static int aug_itoa(int digital, char* string, uint32_t num)
{
	int rt;

	if(NULL  == string)
		return -1;

	if(0 == num || num > 10)
		return -2;

	rt = snprintf(string, num, "%d", digital);

	return rt;
}

static void addStaticRT(unsigned char *data, int masklen, int octets){
	char ip_mask[18];
	char nxthp[15];
	int i, entrynum;
	MIB_CE_ATM_VC_T Entry;
	int32_t tbl_id;
	char str_tblid[10];
	char ext_ifname[IFNAMSIZ];
	FILE *fp = NULL;
	char static_route_file[64] = {0};
	if (client_config.interface)
	{
		snprintf(static_route_file, sizeof(static_route_file), "%s%s", STATIC_ROUTE_FILE, client_config.interface);
	}
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
	char lanNetAddr[20];
	getPrimaryLanNetAddrInfo(lanNetAddr,sizeof(lanNetAddr));
#endif
	sprintpartialip(ip_mask, masklen, octets, data);
	data += octets;
	sprintip(nxthp, "", data);
	data += 4;
	
	entrynum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entrynum; i++){
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry) || !Entry.enable)
			continue;
		
		ifGetName(Entry.ifIndex,ext_ifname,sizeof(ext_ifname));
		if(!strcmp(ext_ifname, client_config.interface))
			break;
		/*
		if (Entry.ifIndex == client_config.interface)
			break;
		*/	
	}
	if(i == entrynum){
		printf("[%s %d]can not find the interface %s", __func__, __LINE__, client_config.interface);
		return;
	}
#ifdef NEW_PORTMAPPING
	if(Entry.itfGroup == 0){
		//printf("/bin/ip route add %s via %s dev %s\n", ip_mask, nxthp, client_config.interface);
		va_cmd("/bin/ip", 7, 1, "route", "add", ip_mask, "via", nxthp, "dev", client_config.interface);
	}else{
		tbl_id = caculate_tblid(Entry.ifIndex);
		aug_itoa(tbl_id, str_tblid, sizeof(str_tblid));
		//printf("/bin/ip route add %s via %s dev %s table %s\n", ip_mask, nxthp, client_config.interface, str_tblid);
		va_cmd("/bin/ip", 9, 1, "route", "add", ip_mask, "via", nxthp, "dev", client_config.interface, "table", str_tblid);
#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
		va_cmd(BIN_IP, 7, 1, "route", "add", lanNetAddr, "dev", LANIF, "table", str_tblid);
#endif
	}
#else
	//printf("/bin/ip route add %s via %s dev %s\n", ip_mask, nxthp, client_config.interface);
	va_cmd("/bin/ip", 7, 1, "route", "add", ip_mask, "via", nxthp, "dev", client_config.interface);
#endif

	fp = fopen(static_route_file, "a+");
	if (fp)
	{
		fprintf(fp, "%s via %s\n", ip_mask, nxthp);
		fclose(fp);
	}
}


void dealStaticRTOption(unsigned char *data, int code){
	int optlen = 0, masklen = 0, octets = 0;
	optlen = data[OPT_LEN - 2];
	
	//printf("[%s %d]code = %d\n", __func__, __LINE__, code);
	
	if(DHCP_CLASSLESS_RT == code || DHCP_OPT_249 == code){
		while(optlen > 0){
			masklen = data[0];
			octets = masklen>24?4:(masklen>16?3:(masklen>8?2:(masklen>0?1:0)));
			data++;
			addStaticRT(data, masklen, octets);
			data += (4+octets);
			optlen -= (5+octets);
		}
	}else{
		while(optlen > 0){
			addStaticRT(data, 32, 4);
			data += 8;
			optlen -= 8;
		}
	}
}
#endif
//end of add

#ifdef RECEIVE_DHCP_OPTION_125
#ifdef CONFIG_CMCC
unsigned int check_option125 = 0;
#endif
int dealOption125(unsigned char *data){
	// V-I Vendor-specific Information
	int ret=1;     // default is accept
	u_int32_t optlen = 0, datalen = 0;	
	MIB_CE_ATM_VC_T entry = {0};
	int i, option=1;
	unsigned char *backup = data;	
#ifdef SUPPORT_NON_SESSION_IPOE
	unsigned char serverid[129]={0};
#endif
#if	!defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	unsigned int wan_ifIndex;
	wan_ifIndex = getIfIndexByName(client_config.interface);
#endif
	int mibtotal,mib_index;
	mibtotal = mib_chain_total(MIB_ATM_VC_TBL);
	for(mib_index=0;mib_index<mibtotal;mib_index++)
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, mib_index, (void *)&entry))
			continue;
		if(entry.ifIndex == wan_ifIndex)
			break;
	}

#ifdef CONFIG_CMCC
	if(check_option125 == 0){
		if(!strncmp(province, "ZJ", 2)){
            char value[] = "CMCC.ZJ.HGU";
			entry.dhcp_opt125_enable[0] = 1;
			entry.dhcp_opt125_type[0] = 2;
			memcpy(entry.dhcp_opt125_value[0], value, strlen(value));
			mib_chain_update(MIB_ATM_VC_TBL, (void *)&entry, mib_index);
		}

		check_option125 = 1;
	}
#endif	
	if(mib_index==mibtotal)
	{		
		DEBUG(LOG_ERR, "0x%x is not a WAN interface. Do not check CT-COM option 125", wan_ifIndex);
		return 1;
	}
	
	for(i = 0 ; i < 4 ; i++) {		
		option = 1;
		data = backup;
		optlen = data[OPT_LEN - 2];		
		
		// match sub_code + sub_data
		if(entry.dhcp_opt125_enable[i] && entry.dhcp_opt125_type[i] == 1)
		{	
			ret = 0;
			while(optlen > 0){
				datalen = data[0+4];  // enterprise(4bytes)
				DEBUG(LOG_INFO, "[%s %d]match sub_code : option %d, datalen=%d\n", __func__,__LINE__, option, datalen);
				
				data +=5;  // enterprise(4bytes) + datalen(1bytes)				
				if (strlen((char*)(entry.dhcp_opt125_sub_data[i])) == data[1]) {
					if(!strncmp((char*)data+2, (char*)(entry.dhcp_opt125_sub_data[i]), strlen((char*)(entry.dhcp_opt125_sub_data[i])))) {
						printf("Find the V-I Vendor-specific by matching sub_code & sub_data. Accept it.\n");
						ret = 1;
						return ret;
					}		
				}				
				data += datalen;
				optlen -= (5+datalen);  // enterprise(4bytes) + datalen(1bytes)
				option++;
			}
		}
	}
	
	for(i = 0 ; i < 4 ; i++) {		
		option = 1;
		data = backup;
		optlen = data[OPT_LEN - 2];		
		
		// match value
		if(entry.dhcp_opt125_enable[i] && entry.dhcp_opt125_type[i] == 2)
		{	
			ret = 0;
			while(optlen > 0){
				datalen = data[0+4];  // enterprise(4bytes)
				DEBUG(LOG_INFO, "[%s %d]match value: option %d, datalen=%d\n", __func__,__LINE__, option, datalen);
				
				data +=5;  // enterprise(4bytes) + datalen(1bytes)	
#ifdef CONFIG_CMCC			
				if(!strncmp(province, "JS", 2)){
					if (i==0 && strlen((char*)entry.dhcp_opt125_value[i]) == 0) {
						//need init for JS province
						strcpy((char*)entry.dhcp_opt125_value[i],"CMCCJSHGU");
						mib_chain_update(MIB_ATM_VC_TBL, &entry, mib_index);
					}
				}
#endif
				if (strlen((char*)(entry.dhcp_opt125_value[i])) == datalen) {
					if(!strncmp((char*)data, (char*)(entry.dhcp_opt125_value[i]), strlen((char*)(entry.dhcp_opt125_value[i])))) {
						printf("Find the V-I Vendor-specific by matching value. Accept it.\n");
						ret = 1;
						return ret;
					}		
				}				
				data += datalen;
				optlen -= (5+datalen);  // enterprise(4bytes) + datalen(1bytes)
				option++;
			}
		}
	}
#ifdef SUPPORT_NON_SESSION_IPOE
	for(i = 0 ; i < 4 ; i++) {		
		option = 1;
		data = backup;
		optlen = data[OPT_LEN - 2];		
		
		// match value
		if(entry.dhcp_opt125_enable[i] && entry.dhcp_opt125_type[i] == 3)
		{	
			ret = 0;
			while(optlen > 0){
				datalen = data[0+4];  // enterprise(4bytes)
				//printf("match value: option %d, datalen=%d\n", option, datalen);
				
				data +=5;  // enterprise(4bytes) + datalen(1bytes)				
				if (parse_dhcp_server_auth_opt_serverid(data, datalen, entry.dhcp_opt125_SharedKey[i], serverid) > 0) {
					if(strcmp(serverid, entry.dhcp_opt125_ServerID[i]) == 0)
					{
						printf("[%s %d]opt125 serverid match!\n",__func__,__LINE__);
						return 1;
					}
				}				
				data += datalen;
				optlen -= (5+datalen);  // enterprise(4bytes) + datalen(1bytes)
				option++;
			}
		}
	}
#endif
	return ret;
}
#endif

#ifdef _CONFIG_DHCPC_OPTION43_ACSURL_
int dealOption43(unsigned char *data){	
	int i;     
	int optlen = 0;		
	char acsurl[256+1]={0};
	unsigned char cwmp_ProvisioningCode[64];	
	unsigned char dynamic_acs_url_selection = 0;
	mib_get_s(CWMP_DYNAMIC_ACS_URL_SELECTION, &dynamic_acs_url_selection, sizeof(dynamic_acs_url_selection));

	optlen = data[OPT_LEN - 2];
	if (optlen >= sizeof(acsurl)) {
		LOG(LOG_ERR, "Option length %d exceeds buffer size %zu!", optlen, sizeof(acsurl));
		optlen = sizeof(acsurl)-1;
	}
	memcpy(acsurl, data, optlen);
	printf("Option43 acsurl=%s\n", acsurl);

	MIB_CE_ATM_VC_T entry;
	if(getATMVCEntryByIfName(client_config.interface, &entry) == NULL
		|| !(entry.applicationtype & X_CT_SRV_TR069))
		return 1;

	if (dynamic_acs_url_selection)
	{
		mib_set(RS_DHCP_TR069_IF_NAME, client_config.interface);
		if(mib_set(RS_DHCP_ACS_URL, (void *)acsurl))
		{
			unsigned char from = CWMP_ACS_FROM_DHCP;

			mib_set(RS_CWMP_USED_ACS_FROM, &from);
			mib_set(RS_CWMP_USED_ACS_URL, acsurl);
		}
		else
		{
			printf("Set RS_DHCP_ACS_URL error!\n");
			return 0;
		}
	}
	else
	{
		if(!mib_set(CWMP_ACS_URL, (void *)acsurl))
		{
			printf("Set CWMP_ACS_URL error!\n");
			return 0;
		}
	}

			// update DNS info
#ifdef CONFIG_00R0
	restart_dnsrelay_ex("all", 0);
	sleep(1);       //wait a second for DNS updating
#else
	reload_dnsrelay("all");
#endif

#if 0
	for (i=0; i<2; i++)
	{
		optlen = data[1];
		if (data[0] == 1 )      // for ACS URL
		{
			if (optlen >= sizeof(acsurl)) {
				LOG(LOG_ERR, "Option length %d exceeds buffer size %zu!", optlen, sizeof(acsurl));
				optlen = sizeof(acsurl)-1;
			}
			memcpy(acsurl, data+2, optlen);
			printf("Option43 acsurl=%s\n", acsurl);

			MIB_CE_ATM_VC_T entry;
			if(getATMVCEntryByIfName(client_config.interface, &entry) == NULL
				|| !(entry.applicationtype & X_CT_SRV_TR069))
				return 1;

			if (dynamic_acs_url_selection)
			{
				mib_set(RS_DHCP_TR069_IF_NAME, client_config.interface);
				if(mib_set(RS_DHCP_ACS_URL, (void *)acsurl))
				{
					unsigned char from = CWMP_ACS_FROM_DHCP;

					mib_set(RS_CWMP_USED_ACS_FROM, &from);
					mib_set(RS_CWMP_USED_ACS_URL, acsurl);
				}
				else
				{
					printf("Set RS_DHCP_ACS_URL error!\n");
					return 0;
				}
			}
			else
			{
				if(!mib_set(CWMP_ACS_URL, (void *)acsurl))
				{
					printf("Set CWMP_ACS_URL error!\n");
					return 0;
				}
			}

			// update DNS info
#ifdef CONFIG_00R0
			restart_dnsrelay_ex("all", 0);
			sleep(1);       //wait a second for DNS updating
#else
			reload_dnsrelay("all");
#endif
		}
		else if (data[0] == 2 )  // for ProvisioningCode
		{
			if (optlen >= sizeof(cwmp_ProvisioningCode)) {
				LOG(LOG_ERR, "Option length %d exceeds buffer size %zu!", optlen, sizeof(cwmp_ProvisioningCode));
				optlen = sizeof(cwmp_ProvisioningCode)-1;
			}
			memcpy(cwmp_ProvisioningCode, data+2, optlen);
			//printf("cwmp_ProvisioningCode=%s\n", cwmp_ProvisioningCode);
			if(!mib_set(CWMP_PROVISIONINGCODE, (void *)cwmp_ProvisioningCode))
			{
				printf("Set CWMP_PROVISIONINGCODE error!\n");
				return 0;
			}
		}
			
		data +=	2+optlen;
	}	
#endif
	Commit();
	return 1;
}
#endif


int dealOption20(unsigned char *data){	 // Enable/Disable source routing
	int temp=data[0];
	char tmpStr[64];

	sprintf(tmpStr, "/bin/echo %d > /proc/sys/net/ipv4/conf/all/accept_source_route", temp);
	system(tmpStr);

	return 1;
}

#ifdef TIME_ZONE
int dealOption42(unsigned char *data){	
	int optlen = 0;	
	char ipaddr[16]={0};
	
	optlen = data[OPT_LEN - 2];

#ifdef CONFIG_00R0
	unsigned char ValGet;
	int pri = 0;
	mib_get_s(MIB_NTP_EXT_ITF_PRI, (void *)&pri, sizeof(pri));

	if ( pri > NTP_EXT_ITF_PRI_MID)
	{
		printf("Skip NTP option because now controled by TR-069.\n");
		return 1;
	}

	pri = NTP_EXT_ITF_PRI_MID;
	if(!mib_set(MIB_NTP_EXT_ITF_PRI, &pri))
	{
		printf("Set MIB_NTP_EXT_ITF error!\n");
		return 0;
	}

	if(!mib_set(MIB_NTP_EXT_ITF, &wan_ifIndex))
	{
		printf("Set MIB_NTP_EXT_ITF error!\n");
		return 0;
	}
#endif

	if (optlen > 0) {
		sprintf(ipaddr, "%d.%d.%d.%d", data[0],data[1],data[2],data[3]);
		if(!mib_set(MIB_NTP_SERVER_HOST2, (void *)ipaddr))
		{
			printf("Set MIB_NTP_SERVER_HOST2 error!\n");
			return 0;
		}
	}
	/* Allow to support multiple NTP server*/
	if (optlen > 4) {
		sprintf(ipaddr, "%d.%d.%d.%d", data[4],data[5],data[6],data[7]);

		if(!mib_set(MIB_NTP_SERVER_HOST1, (void *)ipaddr))
		{
			printf("Set MIB_NTP_SERVER_HOST1 error!\n");
			return 0;
		}
	}

#ifdef CONFIG_00R0
	mib_get_s(MIB_NTP_ENABLED, (void *)&ValGet, sizeof(ValGet));
	if(ValGet){
		stopNTP();
		startNTP();
	} else {
		stopNTP();
	}
#endif
	Commit();
	return 1;
}
#endif

/*determine if dns change,previous dns info is read from /var/udhcpc/resolve_interface eg:172.29.17.10@192.168.2.200 */
#if defined(CONFIG_USER_DNSMASQ_DNSMASQ) || defined(CONFIG_USER_DNSMASQ_DNSMASQ245)
#define MAX_DNS_COUNT 10
int dealOption6(unsigned char *data){
	int optlen = 0;
	FILE *dnsfp= NULL;
	char tempDns[32]={0};
	char line[128]={0};
	char *endptr=NULL;
	int dnsNeedChange = 0;
	MIB_CE_ATM_VC_T Entry, *pEntry = &Entry;
	int num_entry, idx;
	char if_name[IFNAMSIZ];
	int needCompareDns=0;
	char resolveName[64]={0};
	struct in_addr preDnsAddr[MAX_DNS_COUNT];
	struct in_addr newDnsAddr[MAX_DNS_COUNT];
	int dnsIdx=0, newDnsNum=0;
	int preDnsNum=0;
	int i,j=0;

	optlen = data[OPT_LEN - 2];

	num_entry = mib_chain_total(MIB_ATM_VC_TBL);
	for( idx = 0; idx < num_entry; idx++ )
	{
		if(!mib_chain_get(MIB_ATM_VC_TBL, idx, (void *)pEntry))
			continue;

		ifGetName(pEntry->ifIndex, if_name, IFNAMSIZ);

		if(!strcmp(if_name, client_config.interface))
		{
			if(pEntry->dnsMode == REQUEST_DNS)
			{
				needCompareDns=1;
			}
			break;
		}
	}

	if(needCompareDns)
	{
		//store dns info from packet
		newDnsNum = optlen>>2;
		for (dnsIdx=0; dnsIdx<newDnsNum; dnsIdx++)
		{
			newDnsAddr[dnsIdx].s_addr = ntohl(*(unsigned int *)data);
			data+=4;
		}

		//store dns info from resolve_interface file
		snprintf(resolveName, 64, "%s.%s", (char *)DNS_RESOLV, client_config.interface);
		dnsfp=fopen(resolveName,"r");
		if(dnsfp)
		{
			while(!feof(dnsfp))
			{
				memset(line,0,sizeof(line));
				fgets(line,sizeof(line),dnsfp);

				if((strlen(line)==0))
					break;

				endptr = strchr(line, '@');
				if (endptr)
				{
					memcpy(tempDns, line, endptr-line);
					tempDns[endptr-line] = '\0';
					if((preDnsAddr[preDnsNum].s_addr = inet_addr(tempDns))==INADDR_NONE)
					{
						break;
					}
					preDnsNum++;
					if(preDnsNum>=MAX_DNS_COUNT)
					{
						break;
					}
				}
			}
			fclose(dnsfp);
		}

		if(newDnsNum!=preDnsNum)
		{
			dnsNeedChange =1;
		}
		else
		{
			for(i=0;i<newDnsNum;i++)
			{
				for(j=0;j<preDnsNum;j++)
				{
					if(newDnsAddr[i].s_addr==preDnsAddr[j].s_addr)
						break;
				}
				if (j >= preDnsNum)
				{
					dnsNeedChange = 1;
					break;
				}
			}
		}
	}

	return dnsNeedChange;
}
#endif

int dealOption66(unsigned char *data){	
	int optlen = 0;	
	char domainName[254+1]={0};
	
	optlen = data[OPT_LEN - 2];
    if (optlen >= sizeof(domainName)) {
        LOG(LOG_ERR, "Option length %d exceeds buffer size!", optlen);
        optlen = sizeof(domainName)-1;
    }
	memcpy(domainName, data, optlen);
	domainName[optlen] = '\0';
	if(!mib_set(MIB_TFTP_SERVER_ADDR, (void *)domainName))
	{
		printf("Set TFTP_SERVER_ADDR error!\n");
		return 0;
	}
	Commit();
	return 1;
}

int dealOption67(unsigned char *data){	
	int optlen = 0;	
	char domainName[254+1]={0};
	
	optlen = data[OPT_LEN - 2];
    if (optlen >= sizeof(domainName)) {
        LOG(LOG_ERR, "Option length %d exceeds buffer size!", optlen);
        optlen = sizeof(domainName)-1;
    }
	memcpy(domainName, data, optlen);
	domainName[optlen] = '\0';
	if(!mib_set(MIB_BOOT_FILENAME, (void *)domainName))
	{
		printf("Set MIB_BOOT_FILENAME error!\n");
		return 0;
	}
	Commit();
	return 1;
}


int dealOption100(unsigned char *data){	
	int optlen = 0;	
	char domainName[254+1]={0};
	
	optlen = data[OPT_LEN - 2];
    if (optlen >= sizeof(domainName)) {
        LOG(LOG_ERR, "Option length %d exceeds buffer size!", optlen);
        optlen = sizeof(domainName)-1;
    }
	memcpy(domainName, data, optlen);
	domainName[optlen] = '\0';
	if(!mib_set(MIB_POSIX_TZ_STRING, (void *)domainName))
	{
		printf("Set Posix-TZ-String error!\n");
		return 0;
	}
	Commit();
	return 1;
}

#ifdef CONFIG_IPV6_SIT_6RD
int dealOption212(unsigned char *data){
	MIB_CE_ATM_VC_T vc_entry = {0};
	int wan_ifIndex = -1;

	/*
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|  OPTION_6RD   | option-length |  IPv4MaskLen  |  6rdPrefixLen |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	|                           6rdPrefix                           |
	|                          (16 octets)                          |
	|                                                               |
	|                                                               |
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                     6rdBRIPv4Address(es)                      |
	.                                                               .
	.                                                               .
	.                                                               .
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	*/

	if(getWanEntrybyIfname(client_config.interface, &vc_entry, &wan_ifIndex) == 0) {
		vc_entry.SixrdIPv4MaskLen = data[0];                                // IPv4MaskLen
		vc_entry.SixrdPrefixLen = data[1];                                  // 6rdPrefixLen
		memcpy(vc_entry.SixrdPrefix, &data[2], IP6_ADDR_LEN);               // 6rdPrefix
		memcpy(vc_entry.SixrdBRv4IP, &data[2+IP6_ADDR_LEN], IP_ADDR_LEN);   // 6rdBRIPv4Address
		mib_chain_update(MIB_ATM_VC_TBL, (void *)&vc_entry, wan_ifIndex);
	}
	return 1;
}
#endif

#ifdef CONFIG_USER_RTK_VOIP
#define SIP_PROXY_NUM 2
int dealOption120(unsigned char *data)
{
	int optlen = 0, tmplen=0;	
	char url[SIP_PROXY_NUM][256+1]={"", ""};
	char ip[SIP_PROXY_NUM][IP_ADDR_LEN]={"", ""};
	char ipStr[SIP_PROXY_NUM][16]={"", ""};	
	char *current, *backup;
	int count=0, urllen, encode=0;
#ifdef CONFIG_00R0
	int i=0;
#endif	
	optlen = data[OPT_LEN - 2];
	//  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	//  |120|27 | 0 | 7 |'e'|'x'|'a'|'m'|'p'|'l'|'e'| 3 |'c'|'o'|'m'| 0 |
	//  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	//  +---+---+---+---+---+---+---+---+---+---+---+---+---+ | 7
	//  |'e'|'x'|'a'|'m'|'p'|'l'|'e'| 3 |'n'|'e'|'t'| 0 | +---+---+---
	//  +---+---+---+---+---+---+---+---+---+---+
	// Get domain name
	if (data[0] == 0)
	{
		current = (char *)data+1;
		optlen = optlen-1;
		count = 0;
		encode = 0;
		
		while (optlen>0)
		{
			backup = current;
			urllen = 0;
			
			if (count >=SIP_PROXY_NUM)
				break;
				
			while (*current != 0)
			{
				tmplen = *current;
				current[0] = '.';
				current = current+tmplen+1;
				urllen = urllen+tmplen+1;
			}
			if (urllen >= 257) {
				LOG(LOG_ERR, "Option length %d exceeds buffer size 256!", urllen);
				urllen = 256;
			}
			memcpy(url[count], &backup[1], urllen);
			url[count][urllen] = '\0';
			optlen = optlen-urllen-1;
			count++;
			current++;
		}
	}
	
	// Code   Len   enc   Address 1               Address 2
	// +-----+-----+-----+-----+-----+-----+-----+-----+--
	// | 120 |  n  |  1  | a1  | a2  | a3  | a4  | a1  |  ...
	// +-----+-----+-----+-----+-----+-----+-----+-----+--
	// Get IP address
	if (data[0] == 1)
	{
		current = (char *)data+1;
		optlen = optlen-1;
		count = 0;
		encode = 1;
		
		while (optlen>0)
		{	
			struct in_addr addr;
			
			if (count >=SIP_PROXY_NUM)
				break;
			memcpy(ip[count], current, IP_ADDR_LEN);
			memcpy(&addr.s_addr, ((struct in_addr *)ip[count]), 4);
			strncpy(ipStr[count], inet_ntoa(addr), 16);
			//strncpy(ipStr[count], inet_ntoa(*((struct in_addr *)ip[count])), 16);
			ipStr[count][15] = '\0';
			optlen = optlen-IP_ADDR_LEN;
			count++;
			current = current+IP_ADDR_LEN;
		}
	}
#ifdef CONFIG_00R0
	for(i=0;i<=count;i++)
	{
		if (encode == 0)
			voip_SetDhcpOption120(url[i],i);
		else if (encode == 1)
			voip_SetDhcpOption120(ipStr[i],i);
	}
#else	
	if (encode == 0)
		voip_SetDhcpOption120(url[0]);
	else if (encode == 1)
		voip_SetDhcpOption120(ipStr[0]);
#endif
	return 1;
}
#endif

int dealOption15(unsigned char *data)
{
	int optlen = 0;	
	char domainName[256+1]={0};
	
	optlen = data[OPT_LEN - 2];

	if (optlen >= sizeof(domainName)) {
		LOG(LOG_ERR, "Option length %d exceeds buffer size!", optlen);
		optlen = sizeof(domainName)-1;
	}

	memcpy(domainName, data, optlen);
	domainName[optlen] = '\0';
	DEBUG(LOG_INFO, "domainName=%s\n", domainName );


#ifdef CONFIG_USER_RTK_VOIP 
	//need check 
#ifdef CONFIG_00R0
	voip_SetDhcpOption15(domainName);
#else
	voip_SetDhcpOption15();
#endif
#endif
	return 1;
}

#ifdef _PRMT_X_CT_COM_DHCP_
unsigned int check_option60 = 0;

int appendOption60(unsigned char *value) {

	MIB_CE_ATM_VC_T entry = {0};
	int i;
	int total_len = 0;
	unsigned char field_len=0;
	int tag = 0;
#if	!defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	unsigned int wan_ifIndex;
	wan_ifIndex = getIfIndexByName(client_config.interface);
#endif
#ifdef CONFIG_CMCC1
	unsigned char enterprise[2] = {0x00, 0x02};
#elif defined(CONFIG_CU) || defined(CONFIG_CU_AP)
	unsigned char enterprise[2] = {0x00, 0x01};
#else
	unsigned char enterprise[2] = {0x00, 0x00};
#endif
#ifdef CONFIG_CU_BASEON_CMCC
	unsigned char vChar;
	char RMS_TJ[6] = "RMS_TJ", IPTV_TJ[7] = "IPTV_TJ";
	mib_get_s( CU_SRVMGT_TIANJIN_SPEC_ENABLE, (void *)&vChar, sizeof(vChar));
#endif
	char account[MAX_IPOE_NAME_LEN+1]={0};
	char passwd[MAX_IPOE_PWD_LEN+1]={0};
#if defined(CONFIG_SUPPORT_HQOS_APPLICATIONTYPE)
	unsigned char hqos_enable = 0;
	mib_get_s(MIB_HQOS_APPLICATIONTYPE_ENABLE,&hqos_enable, sizeof(hqos_enable));
#endif

	tag = (client_config.is_ipoe_diag) ? (MIB_SIMU_ATM_VC_TBL):(MIB_ATM_VC_TBL);

	int idx, total = mib_chain_total(tag);
	for(idx = 0 ; idx < total ; idx++)
	{		
		if(mib_chain_get(tag, idx, &entry) == 0)
			continue;

		if(entry.ifIndex == wan_ifIndex)
			break;
	}
	
	if(idx >= total)
	{		
		DEBUG(LOG_ERR, "appendOption60: 0x%x is not a WAN interface. Do not append CT-COM option 60", wan_ifIndex);
		return 0;
	}

#ifdef CONFIG_CU_BASEON_CMCC
	if (vChar == 1)
	{
		if  (entry.applicationtype & X_CT_SRV_TR069)
		{
			total_len = strlen(RMS_TJ);
			memcpy(value, RMS_TJ, total_len);
			return total_len;
		}
		else if (entry.applicationtype & OTHER_IPTV_TYPE)
		{
			total_len = strlen(IPTV_TJ);
			memcpy(value, IPTV_TJ, total_len);
			return total_len;
		}
	}
#elif defined(CONFIG_CMCC) 
	char ottAccount[128] = {0};
	char ottPassword[128] = {0};
	if(check_option60==0){
        if(!strncmp(province, "ZJ", 2))
        {
            char username[] = "ZJ.HGU@HGU";
            char password[] = "CMCCZJHGU";
            check_option60 = 1;
			mib_set(PROVINCE_OTT_ACCOUNT, username);
			mib_set(PROVINCE_OTT_PASSWORD, password);
            if(entry.dhcp_opt60_enable[0] == 0){
                entry.dhcp_opt60_enable[0] = 1;
                entry.dhcp_opt60_type[0]=0x1f;
                //memcpy(entry.dhcp_opt60_account[0], username, strlen(username));
                //memcpy(entry.dhcp_opt60_password[0], password, strlen(password));
            }
        }
        else if(!strncmp(province, "JS", 2))
        {
			mib_get_s(PROVINCE_OTT_ACCOUNT,ottAccount, sizeof(ottAccount));
			mib_get_s(PROVINCE_OTT_PASSWORD,ottPassword, sizeof(ottPassword));
            if(!strlen(ottAccount))
            {
				DEBUG(LOG_INFO, "JS province:OTT ACCOUNT empty, skip option 60");
                return 2;
            }
            
            //memcpy(entry.dhcp_opt60_account[0], ottAccount, strlen(ottAccount));
            if(!strlen(ottPassword))
            {
                char password[] = "JS@HGU";
                memcpy(ottPassword, password, strlen(password));
                mib_set(PROVINCE_OTT_PASSWORD, (void *)password);
            }
            //memcpy(entry.dhcp_opt60_password[0], ottPassword, strlen(ottPassword));
            
            check_option60 = 1;
            //if(entry.dhcp_opt60_enable[0] == 0){
            //    entry.dhcp_opt60_enable[0] = 1;
            //    entry.dhcp_opt60_type[0]=0x1f;
            //}
        }
	}
	mib_get_s(PROVINCE_OTT_ACCOUNT,ottAccount, sizeof(ottAccount));
	mib_get_s(PROVINCE_OTT_PASSWORD,ottPassword, sizeof(ottPassword));
#endif

	// (1) 2 bytes for Enterprise Code
	memcpy(value, enterprise, 2);
	total_len +=2;
	value +=2;
	
#if defined(CONFIG_SUPPORT_HQOS_APPLICATIONTYPE)
	if(hqos_enable && (entry.applicationtype & X_CT_SRV_INTERNET) && (entry.othertype == OTHER_HQOS_TYPE) && strlen(entry.ipoeName) && strlen(entry.ipoePassword))
	{
		char buf[100] = {0};
		unsigned char dhcp_opt60_type = HQOS_OPTION60_FIELD_TYPE;
#if 0
		field_len = gen_ctcom_dhcp_opt(dhcp_opt60_type, buf, 100, entry.ipoeName, entry.ipoePassword);
#else
		field_len = gen_cu_dhcp_opt(dhcp_opt60_type, buf, 100, entry.ipoeName, entry.ipoePassword);
#endif

		if(field_len > 0)
		{
			// (2) 1 bytes for Field type
			memcpy(value, &dhcp_opt60_type, 1);
			total_len +=1;
			value +=1;

			// (3) 1 bytes for Field Length
			memcpy(value, &field_len, 1);
			total_len +=1;
			value +=1;

			// (4) n bytes for Field Value
#if defined(CONFIG_CMCC) 
			if(!strncmp(province, "JS", 2)){
				char DESC_JS[]="JSCMCC-WAN";
				memcpy(value, DESC_JS, strlen(DESC_JS));
				total_len += strlen(DESC_JS)+1;
				value += strlen(DESC_JS);
				*value = 0;
				value += 1;
			}
#endif
			memcpy(value, buf, field_len);
			total_len += field_len;
			value += field_len;
		}
	}
#endif

	// create data string with leading colon
	for (i = 0; i < 4; i++)
	{
		if(entry.dhcp_opt60_enable[i])
		{
			printf("#######%d option60 is enable\n", i+1);
			if(entry.dhcp_opt60_type[i] == 34 && entry.dhcp_opt60_value_mode[i] != 2)
				continue;

			if(entry.dhcp_opt60_type[i] == 32 && entry.dhcp_opt60_value_mode[i] != 3)
				continue;

#ifdef SUPPORT_CLOUD_VR_SERVICE
			if(entry.dhcp_opt60_type[i] == 35 && entry.dhcp_opt60_value_mode[i] != 2)
				continue;
#endif

			printf("%d option60 is enable\n", i+1);

			if(entry.dhcp_opt60_value_mode[i] == 0)
			{
				field_len = (unsigned char) strlen((char*)(entry.dhcp_opt60_value[i]));

				if(field_len > 0)
				{
					// (2) 1 bytes for Field type
					memcpy(value, &entry.dhcp_opt60_type[i], 1);
					total_len +=1;
					value +=1;

					// (3) 1 bytes for Field Length		
					memcpy(value, &field_len, 1);
					total_len +=1;
					value +=1;
			
					// (4) n bytes for Field Value
					memcpy(value, entry.dhcp_opt60_value[i], field_len);
					total_len += field_len;
					value += field_len;
				}
			}
#ifdef _PRMT_X_CT_COM_DHCP_
			else if((entry.dhcp_opt60_type[i] == 34 && entry.dhcp_opt60_value_mode[i] == 2) ||
				(entry.dhcp_opt60_type[i] == 32 && entry.dhcp_opt60_value_mode[i] == 3) ||
				(entry.dhcp_opt60_type[i] == 35 && entry.dhcp_opt60_value_mode[i] == 2) ||
#ifdef CONFIG_CMCC
				((!strncmp(province, "ZJ", 2) || !strncmp(province, "JS", 2)) && entry.dhcp_opt60_type[i] == 31) ||
#endif
				(entry.dhcp_opt60_type[i] != 34 && entry.dhcp_opt60_type[i] != 32 && entry.dhcp_opt60_type[i] != 35 && entry.dhcp_opt60_value_mode[i] == 1
			))
			{
				char buf[100] = {0};

#ifdef CONFIG_CMCC
				field_len = gen_ctcom_dhcp_opt(entry.dhcp_opt60_type[i], buf, 100, ottAccount, ottPassword);
#else
				field_len = gen_ctcom_dhcp_opt(entry.dhcp_opt60_type[i], buf, 100,NULL,NULL);
#endif

				if(field_len > 0)
				{
					// (2) 1 bytes for Field type
					memcpy(value, &entry.dhcp_opt60_type[i], 1);
					total_len +=1;
					value +=1;

					// (3) 1 bytes for Field Length
					memcpy(value, &field_len, 1);
					total_len +=1;
					value +=1;
				
					// (4) n bytes for Field Value
					memcpy(value, buf, field_len);
					total_len += field_len;
					value += field_len;

					memcpy(entry.dhcp_opt60_value[i], buf, 100);
					mib_chain_update(tag, &entry, idx);
				}				
			}
#if defined(CONFIG_CMCC)
			else if (entry.dhcp_opt60_type[i] == 31)
			{
				char buf[100] = {0};
				strncpy(account, entry.dhcp_opt60_account, sizeof(entry.dhcp_opt60_account));
				strncpy(passwd, entry.dhcp_opt60_password, sizeof(entry.dhcp_opt60_password));

				field_len = gen_cmcc_dhcp_opt(entry.dhcp_opt60_type[i], buf, 100, account, passwd);

				if(field_len > 0)
				{
					// (2) 1 bytes for Field type
					memcpy(value, &entry.dhcp_opt60_type[i], 1);
					total_len +=1;
					value +=1;

					// (3) 1 bytes for Field Length
					memcpy(value, &field_len, 1);
					total_len +=1;
					value +=1;
				
					// (4) n bytes for Field Value
					memcpy(value, buf, field_len);
					total_len += field_len;
					value += field_len;

					memcpy(entry.dhcp_opt60_value[i], buf, 100);
					mib_chain_update(tag, &entry, idx);
				}				
			}
#endif
#endif
		}
	}
	
	return total_len;
}
#endif

/* Fill dest with the text of option 'option'. */
static void fill_options(char *dest, unsigned char *option, struct dhcp_option *type_p)
{
	int type, optlen;
	u_int16_t val_u16;
	int16_t val_s16;
	u_int32_t val_u32;
	int32_t val_s32;
	int len = option[OPT_LEN - 2];

	dest += sprintf(dest, "%s=", type_p->name);

	type = type_p->flags & TYPE_MASK;
	optlen = option_lengths[type];
	for(;;) {
		switch (type) {
		case OPTION_IP_PAIR:
			dest += sprintip(dest, "", option);
			*(dest++) = '/';
			option += 4;
			optlen = 4;
			/* fall through */
		case OPTION_IP:	/* Works regardless of host byte order. */
			dest += sprintip(dest, "", option);
 			break;
		case OPTION_BOOLEAN:
			dest += sprintf(dest, *option ? "yes" : "no");
			break;
		case OPTION_U8:
			dest += sprintf(dest, "%u", *option);
			break;
		case OPTION_U16:
			memcpy(&val_u16, option, 2);
			dest += sprintf(dest, "%u", ntohs(val_u16));
			break;
		case OPTION_S16:
			memcpy(&val_s16, option, 2);
			dest += sprintf(dest, "%d", ntohs(val_s16));
			break;
		case OPTION_U32:
			memcpy(&val_u32, option, 4);
			dest += sprintf(dest, "%lu", (unsigned long) ntohl(val_u32));
			break;
		case OPTION_S32:
			memcpy(&val_s32, option, 4);
			dest += sprintf(dest, "%ld", (long) ntohl(val_s32));
			break;
		case OPTION_STRING:
			memcpy(dest, option, len);
			dest[len] = '\0';
			return;	 /* Short circuit this case */
		}
		option += optlen;
		len -= optlen;
		if (len <= 0) break;
		dest += sprintf(dest, " ");
	}
}


static char *find_env(const char *prefix, char *defaultstr)
{
	extern char **environ;
	char **ptr;
	const int len = strlen(prefix);

	for (ptr = environ; *ptr != NULL; ptr++) {
		if (strncmp(prefix, *ptr, len) == 0)
			return *ptr;
	}
	return defaultstr;
}


/* put all the paramaters into an environment */
static char **fill_envp(struct dhcpMessage *packet)
{
	int num_options = 0;
	int i, j;
	char **envp;
	unsigned char *temp;
	struct in_addr subnet, netip, wildcard,broadcast;
	char over = 0;

	if (packet == NULL)
		num_options = 0;
	else {
		for (i = 0; options[i].code; i++){
			if (get_option(packet, options[i].code)){
				num_options++;

				if(options[i].code == DHCP_SUBNET)
					num_options+=3;
				else if (options[i].code == DHCP_LEASE_TIME)
					num_options++;
			}
		}
		if (packet->siaddr) num_options++;
		if ((temp = get_option(packet, DHCP_OPTION_OVER)))
			over = *temp;
		if (!(over & FILE_FIELD) && packet->file[0]) num_options++;
		if (!(over & SNAME_FIELD) && packet->sname[0]) num_options++;
	}

	envp = xmalloc((num_options + 8) * sizeof(char *));
	j = 0;
	envp[j++] = xmalloc(sizeof("interface=") + strlen(client_config.interface));
	sprintf(envp[0], "interface=%s", client_config.interface);
	envp[j++] = find_env("PATH", "PATH=/bin:/usr/bin:/sbin:/usr/sbin");
	envp[j++] = find_env("HOME", "HOME=/");

	if (packet == NULL) {
		envp[j++] = NULL;
		return envp;
	}

	envp[j] = xmalloc(sizeof("ip=255.255.255.255"));
	sprintip(envp[j++], "ip=", (unsigned char *) &packet->yiaddr);


	for (i = 0; options[i].code; i++) {
		if (!(temp = get_option(packet, options[i].code)))
			continue;
		envp[j] = xmalloc(upper_length(temp[OPT_LEN - 2], &options[i]) + strlen(options[i].name) + 2);
		fill_options(envp[j++], temp, &options[i]);

		/* Fill in a subnet bits option for things like /24 */
		if (options[i].code == DHCP_SUBNET) {
			envp[j] = xmalloc(sizeof("mask=32"));
			memcpy(&subnet, temp, 4);
			sprintf(envp[j++], "mask=%d", mton(&subnet));
			// Kaohj -- Add subnet IP. ex. 192.168.4.0
			envp[j] = xmalloc(sizeof("netip=255.255.255.255"));
			netip.s_addr = packet->yiaddr&subnet.s_addr;
			sprintip(envp[j++], "netip=", (unsigned char *) &netip.s_addr);

			envp[j] = xmalloc(sizeof("broadcast=255.255.255.255"));
			wildcard.s_addr = ~subnet.s_addr;
			broadcast.s_addr = netip.s_addr | wildcard.s_addr;
			sprintip(envp[j++], "broadcast=", (unsigned char *) &broadcast.s_addr);
		}

		// For calulating lease time remains in cwmp-tr069
		if (options[i].code == DHCP_LEASE_TIME)
		{
			unsigned int lease = 0;
			envp[j] = xmalloc(sizeof("expire=") + 24);
			memcpy(&lease, temp, 4);
			sprintf(envp[j++], "expire=%ld", time(0) + (long int)lease);
		}
	}
	if (packet->ciaddr) {
		envp[j] = xmalloc(sizeof("ciaddr=255.255.255.255"));
		sprintip(envp[j++], "ciaddr=", (unsigned char *) &packet->ciaddr);
	}
	if (packet->siaddr) {
		envp[j] = xmalloc(sizeof("siaddr=255.255.255.255"));
		sprintip(envp[j++], "siaddr=", (unsigned char *) &packet->siaddr);
	}
	if (!(over & FILE_FIELD) && packet->file[0]) {
		/* watch out for invalid packets */
		packet->file[sizeof(packet->file) - 1] = '\0';
		envp[j] = xmalloc(sizeof("boot_file=") + strlen((char *)packet->file));
		sprintf(envp[j++], "boot_file=%s", packet->file);
	}
	if (!(over & SNAME_FIELD) && packet->sname[0]) {
		/* watch out for invalid packets */
		packet->sname[sizeof(packet->sname) - 1] = '\0';
		envp[j] = xmalloc(sizeof("sname=") + strlen((char *)packet->sname));
		sprintf(envp[j++], "sname=%s", packet->sname);
	}
	envp[j] = NULL;
	return envp;
}


/* Call a script with a par file and env vars */
void run_script(struct dhcpMessage *packet, const char *name)
{
	int pid;
	char **envp;

	if (client_config.script == NULL)
		return;
	/* call script */
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
#if 0
// Magician: Copy from Realsil E8B
		envp = fill_envp(packet);	

 // W.H. Hung: Don't need to send GWINFO
/*star:20090122 START message must be sent to boa after dhcpc script execute */
#ifdef IP_ROUTED_PVC_POLICY
		char *pRouter=NULL;
		char *pInterface=NULL;
		char *pIp=NULL;
		char *pMask=NULL;
		char **envpx=envp;
		while((*envpx)!=NULL)
		{
			printf("envpx=%s\n",*envpx);
			if(!pRouter) pRouter=strstr(*envpx,"router=");
			if(!pInterface) pInterface=strstr(*envpx,"interface=");
			if(!pIp) pIp=strstr(*envpx,"ip=");
			if(!pMask) pMask=strstr(*envpx,"subnet=");
			envpx++;
		}

		if(pRouter)
		{
			pRouter+=strlen("router=");
			printf("pRouter=%s\n",pRouter);
		}

		if(pInterface)
		{
			pInterface+=strlen("interface=");
			printf("pInterface=%s\n",pInterface);
		}

		if(pMask)
		{
			pMask+=strlen("subnet=");
			printf("pMask=%s\n",pMask);
		}

		if(pIp)
		{
			pIp+=strlen("ip=");
			printf("ip=%s\n",pIp);
		}

		if(pRouter&&pInterface&&pMask&&pIp)
		{
			GWINFO p;
			struct in_addr gwip;
			strcpy(p.name,pInterface);
			//gwip
			inet_aton(pRouter, &gwip);
			p.gwip=gwip.s_addr;
			//ip
			inet_aton(pIp, &gwip);
			p.ip=gwip.s_addr;
			//mask
			inet_aton(pMask, &gwip);
			p.mask=gwip.s_addr;
			sendMsg2Boa(CMD_PPPOE_DEFAULT_GW,& p, sizeof(GWINFO));
		}
#endif
/*star:20090122 END*/
#endif
// End Magician: Copy from Realsil E8B
		return;
	} else if (pid == 0) {
		envp = fill_envp(packet);

		/* close fd's? */

		/* exec script */
		DEBUG(LOG_INFO, "execle'ing %s", client_config.script);
		execle(client_config.script, client_config.script,
		       name, NULL, envp);
		LOG(LOG_ERR, "script %s failed: %s",
		    client_config.script, strerror(errno));
		exit(1);
	}
}
