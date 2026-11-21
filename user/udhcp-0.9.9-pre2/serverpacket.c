/* serverpacket.c
 *
 * Constuct and send DHCP server packets
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#include "packet.h"
#include "debug.h"
#include "dhcpd.h"
#include "options.h"
#include "leases.h"
#include "files.h"
#include "serverpacket.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <rtk/sysconfig.h>
char *if_indextoname(unsigned int ifindex, char *ifname);

#include <rtk/mib.h>
// Added by Mason Yu for Half Bridge
#include <linux/if.h>
#include <linux/sockios.h>
#include <rtk/utility.h>
#if defined(IP_BASED_CLIENT_TYPE)
#include <stdio.h>
#endif
#include <stdlib.h>

#ifdef CTC_DHCP_OPTION43
#define CONFIGVERSTR0     'C'
#define CONFIGVERSTR1     'T'
#define CONFIGVERSTR2     'C'
#define CONFIGVERSTR3     '0'
unsigned char option43[1+1+254]=
	{43, 6, 1, 4, CONFIGVERSTR0, CONFIGVERSTR1, CONFIGVERSTR2, CONFIGVERSTR3,0,0};
#endif
#ifdef CTC_DHCP_OPTION60
unsigned char option60[1+1+254]=
/*jim:20080717 modify option60 according BCM's behavior*/
//{60, 2, 0, 0};
	{0x3c, 0x1F, 0x12 , 0x34, 0x01, 0x07, 'R', 'e', 'a', 'l', 't', 'e', 'k', 0x02, 0x03, 'C', 'P', 'E', 0x03, 0x08, 'E', '8', 'R', 'O', 'U', 'T', 'E', 'R', 0x04, 0x03, '1', '.', '0', 0, 0}; // FOR dhcp ack, we just send enterprise code to clients.
#endif

const char IPTABLES[] = "/bin/iptables";

#include <linux/if_bridge.h>

struct bridge;
struct port;
int br_socket_fd;

struct bridge
{
	struct bridge *next;
	int ifindex;
	char ifname[IFNAMSIZ];
	struct port *firstport;
	struct port *ports[256];
};

int get_ifname_by_mac(const char *brname, u_int8_t *macaddr, char *ifname)
{
	struct __fdb_entry fdb[256];
	int offset = 0;
	unsigned long args[4];
	struct ifreq ifr;

#ifdef DHCPD_MULTI_THREAD_SUPPORT
	sock_mutex_lock();
#endif

	args[0] = BRCTL_GET_FDB_ENTRIES;
	args[1] = (unsigned long)fdb;
	args[2] = 256;
	if ((br_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
#ifdef DHCPD_MULTI_THREAD_SUPPORT
		sock_mutex_unlock();
#endif
		return -1;
	}
	else
	{
		memcpy(ifr.ifr_name, brname, IFNAMSIZ);		
		ifr.ifr_data = (char *) &args;		
		while (1)
		{
			int i = 0;
			int num = 0;

			args[3] = offset;
			num = ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);

			if (!num)
				break;

			for (i = 0; i < num; i++) {
				struct __fdb_entry *f = fdb + i;

				if (memcmp(f->mac_addr, macaddr, 6) == 0) {
					int ifindices[256];
					int list_num = 0;
					unsigned long list_args[4] = { BRCTL_GET_PORT_LIST, (unsigned long)ifindices, 256, 0 };
					struct ifreq list_ifr;

					memset(ifindices, 0, sizeof(ifindices));
					strncpy(list_ifr.ifr_name, brname, IFNAMSIZ);
					list_ifr.ifr_data = (char *) &list_args;

					list_num = ioctl(br_socket_fd, SIOCDEVPRIVATE, &list_ifr);
					if (list_num < 0) {
						close(br_socket_fd);
#ifdef DHCPD_MULTI_THREAD_SUPPORT
						sock_mutex_unlock();
#endif
						return -1;
					}

					if_indextoname(ifindices[f->port_no], ifname);
					break;
				}
			}

			offset += num;
		}
		close(br_socket_fd);
	}
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	sock_mutex_unlock();
#endif
	return 1;
}

#if defined(IP_BASED_CLIENT_TYPE)
#ifdef CONFIG_CU
static char *g_apszCTCDeviceName[] =
{
	"Computer",
	"Camera",
	"HGW",
	"STB",
	"MVT",
	"WlanAP",
#ifdef CONFIG_CU_FTTR_MASTER
	"FTTR_EdgeONT",
#endif
	NULL
};
#else
static char *g_apszCTCDeviceName[] =
{
	"Computer",
	"Camera",
	"HGW",
	"STB",
	"Phone",
	NULL
};
#endif
#endif

// Kaohj --- TR111 Part 1
#ifdef _CWMP_TR111_
struct device_id_t opt125_deviceId;

//#define _TR111_TEST_
#define TR111_DEVICEFILE	"/var/udhcpd/tr111device.txt"

/* Dump the option 125 device Identity to file
 * 0: fail
 * 1: successful
 */
int dump_deviceId(void)
{
	FILE *fp;
	int i;
	char tmp[160];
	struct device_id_t *pdevId;

	fp = fopen( TR111_DEVICEFILE, "w" );
	if (!fp)
		return 0;

	pdevId = opt125_deviceId.next;

	i = 1;
	while (pdevId) {
		if (pdevId->productClass[0])
			snprintf(tmp, sizeof(tmp), "%d %s?%s?%s\n", i, pdevId->oui, pdevId->productClass, pdevId->serialNo);
		else
			snprintf( tmp, sizeof(tmp), "%d %s?%s\n", i, pdevId->oui, pdevId->serialNo);
		fwrite( tmp, 1, strlen(tmp), fp );
		i++;
		pdevId = pdevId->next;
	}

	fclose(fp);
	return 1;
}
/* Device with option 125 has been cached, add device ID into option 125 device list.
 * 0: fail
 * 1: replace
 * 2: add
 */
int add_deviceId(struct device_id_t *deviceId)
{
	struct device_id_t *pdevId;

	pdevId = opt125_deviceId.next;
	// find deviceId
	while (pdevId) {
		if (pdevId->yiaddr == deviceId->yiaddr)
			break;
		pdevId = pdevId->next;
	}

	if (pdevId) { // found, replace it
		//printf("replace deviceId: ip 0x%x\n", pdevId->yiaddr);
		memcpy(pdevId->oui, deviceId->oui, 7);
		memcpy(pdevId->serialNo, deviceId->serialNo, 65);
		memcpy(pdevId->productClass, deviceId->productClass, 65);
		return 1;
	}
	else { // add a new one
		//printf("add new deviceId: ip 0x%x\n", deviceId->yiaddr);
		pdevId = xmalloc(sizeof(struct device_id_t));
		if (pdevId) {
			memcpy(pdevId, deviceId, sizeof(struct device_id_t));
			pdevId->next = opt125_deviceId.next;
			opt125_deviceId.next = pdevId;
			return 2;
		}
		else
			printf("%s: xmalloc fail\n", __FUNCTION__);
	}

	return 0;
}

/* Device iaddr is out, remove its device ID from option 125 device list
 * 0: fail
 * 1: successful
 */
int del_deviceId(u_int32_t iaddr)
{
	struct device_id_t *preId, *curId;

	preId = &opt125_deviceId;
	curId = opt125_deviceId.next;
	// find deviceId
	while (curId) {
		if (curId->yiaddr == iaddr)
			break;
		preId = curId;
		curId = curId->next;
	}

	if (curId) { // found
		preId->next = curId->next;
		free(curId);
		return 1;
	}

	return 0;
}

void clear_all_deviceId(void)
{
	struct device_id_t *curId, *tmpId;

	curId = opt125_deviceId.next;

	while (curId) {
		tmpId = curId;
		curId = curId->next;
		free(tmpId);
	}
}

#ifdef CTC_YUNMESTB_DHCPD_DHCPOPTION
static int find_wanif_by_vlanid(unsigned short latvid, MIB_CE_ATM_VC_T* vc_Entry)
{
	int j, totalEntry;

	int ifIndex=-1;

	totalEntry = mib_chain_total(MIB_ATM_VC_TBL);
	for(j = 0; j < totalEntry; ++j)
	{
		mib_chain_get(MIB_ATM_VC_TBL, j, (void*)vc_Entry);		
		if(vc_Entry->vlan && vc_Entry->vid == latvid)
		{
			ifIndex	= (int)vc_Entry->ifIndex;
			if(vc_Entry->cmode == CHANNEL_MODE_BRIDGE){
				break;
			}
		}
	}
	return ifIndex;
}

static void add_lanport_binding_vlan(struct dhcpMessage *dhcppacket, unsigned char *buff)
{
	int isLanPort; // Base on sourceinterface
	char srcIfName[32] = {0};
	MIB_CE_PORT_BINDING_T pbEntry;
	int port;
	int offset;
	
	isLanPort = 0;
	if (get_ifname_by_mac("br0", dhcppacket->chaddr, srcIfName) > 0) {
		if (strlen(srcIfName) > 0) {
			if (strncmp(srcIfName, "eth", strlen("eth")) == 0) {
				port = srcIfName[5] - '0' - 2;
				isLanPort = 1;
			}
		}
	}

	if(isLanPort)
	{		
		//printf("%s srcIfName[%s] port[%d] \n", __func__, srcIfName, port);
		
		mib_chain_get(MIB_PORT_BINDING_TBL, port, (void*)&pbEntry);

		//is it vlan-mapping lan-port?
		if((unsigned char)VLAN_BASED_MODE == pbEntry.pb_mode)
		{
			int k;
			struct vlan_pair *vid_pair;
			int ifindex;
			unsigned char sub_code;
			
			offset = buff[OPT_LEN] + 2;
			vid_pair = (struct vlan_pair *)&pbEntry.pb_vlan0_a;

			// because there are only 4 pairs~
			for (k=0; k<4; k++)
			{
				//Be sure the content of vlan-mapping exsit!
				if (vid_pair[k].vid_a)
				{
					MIB_CE_ATM_VC_T vc_Entry;
					ifindex = find_wanif_by_vlanid(vid_pair[k].vid_b, &vc_Entry);					

					//this vid_pair does not match any wan interface!
					if(ifindex < 0)
					{
						continue;
					}

					//sub-option code: 10(Internet VID)
					//sub-option code: 11(IPTV VID)
					//sub-option code: 12(TR069 VID)
					//sub-option code: 13(VoIP VID)
					if((vc_Entry.cmode == CHANNEL_MODE_BRIDGE) && (vc_Entry.applicationtype & X_CT_SRV_OTHER))
					{
						sub_code = 11;
						buff[offset] = sub_code;
						buff[offset+1] = 2;
						buff[offset+2] = (vid_pair[k].vid_a & 0xff00) >> 8;
						buff[offset+3] = vid_pair[k].vid_a & 0xff;
						offset += 4;
						//printf("%s sub_code[%d] vid[%d]\n", __func__, sub_code, vid_pair[k].vid_a);
					}
					else
					{
						if(vc_Entry.applicationtype & X_CT_SRV_INTERNET)
						{
							sub_code = 10;
							buff[offset] = sub_code;
							buff[offset+1] = 2;
							buff[offset+2] = (vid_pair[k].vid_a & 0xff00) >> 8;
							buff[offset+3] = vid_pair[k].vid_a & 0xff;
							offset += 4;
							//printf("%s sub_code[%d] vid[%d]\n", __func__, sub_code, vid_pair[k].vid_a);
						}
						if(vc_Entry.applicationtype & X_CT_SRV_VOICE)
						{
							sub_code = 13;
							buff[offset] = sub_code;
							buff[offset+1] = 2;
							buff[offset+2] = (vid_pair[k].vid_a & 0xff00) >> 8;
							buff[offset+3] = vid_pair[k].vid_a & 0xff;
							offset += 4;
							//printf("%s sub_code[%d] vid[%d]\n", __func__, sub_code, vid_pair[k].vid_a);
						}
					}				
				}			
			}
			buff[OPT_LEN] = offset - 2;
			/* size of enterprise content needs to be recalculated */
			buff[OPT_LEN+5] = buff[OPT_LEN] - 5;
			//printf("%s OPT_LEN[%d]\n", __func__, buff[OPT_LEN]);
		}		
	}
}
int add_option_125(struct dhcpMessage *dhcppacket, unsigned char *optionptr, unsigned char *string, int isNormalSTB)
{
	/* CTC YUMME STB need special value and format */
	if(0/*isNormalSTB*/){
		unsigned char buff[]={
		0x7d, 0x13 , 0x00 , 0x00 , 0x10 , 0x53 , 0x48 , 0x43 , 0x54 , 0x43 , 0x49,
		0x50 , 0x54 , 0x56 , 0x44 , 0x48 , 0x43 , 0x50 , 0x41 , 0x41 , 0x41 , 0xff, 0xff};

		int end = end_option(optionptr);
		memcpy(optionptr + end, buff, sizeof(buff));
		return 1;
	}else{
		unsigned char buff[128] = {0};
		int end = end_option(optionptr);

		/* end position + string length + option code/length + end option */
		if (end + string[OPT_LEN] + 2 + 1 >= 308) {
			LOG(LOG_ERR, "Option 0x%02x did not fit into the packet!", string[OPT_CODE]);
			return 0;
		}

		if (string[OPT_LEN] + 2 > sizeof(buff)) {
			LOG(LOG_ERR, "Option length %d exceeds buffer size!", string[OPT_LEN]);
			return 0;
		}

		DEBUG(LOG_INFO, "adding option 0x%02x", string[OPT_CODE]);

		memcpy(buff,string, string[OPT_LEN] + 2);
		add_lanport_binding_vlan(dhcppacket, buff);

		if (end + string[OPT_LEN] + 2 > 308) {
			LOG(LOG_ERR, "Insufficient space in optionptr for the option data!");
			return 0;
		}

		memcpy(optionptr + end, buff, buff[OPT_LEN] + 2);	
		optionptr[end + buff[OPT_LEN] + 2] = DHCP_END;
		return buff[OPT_LEN] + 2;
	}
}
#endif

int handle_tr111(struct dhcpMessage *oldpacket, u_int32_t iaddr)
{
	int ret=0;
	//static int ins_num=1;
	unsigned char *pOpt125=NULL;
#ifndef _TR111_TEST_
	pOpt125 = get_option(oldpacket, DHCP_VI_VENSPEC);
#else
      #if 0
	unsigned char testbuf[]={ 0x00, 0x00, 0x0d, 0xe9, 0x18, 0x01, 0x06, 0x30,
				  0x30, 0x65, 0x30, 0x64, 0x34, 0x02, 0x09, 0x30,
				  0x30, 0x30, 0x30, 0x2d, 0x30, 0x30, 0x30, 0x31,
				  0x03, 0x03, 0x49, 0x47, 0x44, 0x00};
      #else
	unsigned char testbuf[]={
	0x00 , 0x00 , 0x0d , 0xe9 , 0x2d , 0x01 , 0x06 , 0x30 , 0x30 , 0x30,
	0x31 , 0x30 , 0x32 , 0x02 , 0x10 , 0x30 , 0x30 , 0x30 , 0x31 , 0x30 , 0x32 , 0x2d , 0x34 , 0x32 , 0x38 , 0x32,
	0x38 , 0x38 , 0x38 , 0x32 , 0x39 , 0x03 , 0x11 , 0x43 , 0x44 , 0x52 , 0x6f , 0x75 , 0x74 , 0x65 , 0x72 , 0x20,
	0x56 , 0x6f , 0x49 , 0x50 , 0x20 , 0x41 , 0x54 , 0x41 };
      #endif
	pOpt125 = testbuf;
#endif
	struct device_id_t cur_devId={iaddr, 3561, "", "", "", 0};


	if(pOpt125)
	{
		unsigned int ent_num, *pUInt;
		unsigned short data_len;
		unsigned char *GW_OUI=cur_devId.oui, *GW_SN=cur_devId.serialNo, *GW_CLASS=cur_devId.productClass;

		pUInt = (unsigned int*)pOpt125;
		ent_num = ntohl( *pUInt );
		data_len = (unsigned short)pOpt125[4];

		//if more than one enterprise-number in the same option125??
		if( ent_num==3561 )
		{
			unsigned char *pStart;

			//sub-option
			pStart = &pOpt125[5];
			while( data_len>0 )
			{
				unsigned char sub_code, sub_len, *sub_data;

				sub_code = pStart[0];
				sub_len = pStart[1];
				sub_data = &pStart[2];

				if( data_len < sub_len+2 )
					break;

				switch( sub_code )
				{
				case 1://DeviceManufacturerOUI
					if( sub_len<7 )
					{
						strncpy( (char *)GW_OUI, (char *)sub_data, sub_len );
						GW_OUI[sub_len]=0;
					}
					break;
				case 2://DeviceSerialNumber
					if( sub_len<65 )
					{
						strncpy( (char *)GW_SN, (char *)sub_data, sub_len );
						GW_SN[sub_len]=0;
					}
					break;
				case 3://DeviceProductClass
					if( sub_len<65 )
					{
						strncpy( (char *)GW_CLASS, (char *)sub_data, sub_len );
						GW_CLASS[sub_len]=0;
					}
					break;
				default:
					//unknown suboption
					break;
				}


				pStart = pStart+2+sub_len;
				data_len = data_len-sub_len-2;
			}


			//LOG(LOG_ERR,  "%d: oui:%s sn:%s class:%s ", __LINE__,GW_OUI,GW_SN,GW_CLASS  );
			if( *GW_OUI && *GW_SN && *GW_CLASS )
			{
				//sprintf( buf, "%s?%s?%s", GW_OUI,GW_CLASS,GW_SN );
				add_deviceId(&cur_devId);
				dump_deviceId();
			}else if( *GW_OUI && *GW_SN  )
			{
				//sprintf( buf, "%s?%s", GW_OUI,GW_SN );
				add_deviceId(&cur_devId);
				dump_deviceId();
			}else{
				//error
			}
		}
	}

	return ret;
}
#endif // _CWMP_TR111_
#ifdef CONFIG_CMCC
int parse_cmcc_vender_STB(unsigned char* option60, u_int8_t *chaddr,char *province){
	if(!strncmp(province, "ZJ", 2)){
		//ZheJiang
		char key[64]="ZJSTB";
		return parse_cmcc_vender_STB_R(option60, chaddr, key);
	}
	
	else if(!strncmp(province, "HA", 2)){
		//HeNan
		char key[64]="HASTB";
		return parse_cmcc_vender_STB_R(option60, chaddr, key);
	}
	else if(!strncmp(province, "JS", 2)){
		//JiangSu
		return parse_cmcc_vender_STB_JS(option60, chaddr);
	}else //not defined
		return 0;
}
int parse_cmcc_vender_STB_R(unsigned char* option60, u_int8_t *chaddr, char *key)
{
	u_int8_t optlen=*(option60-1);
	u_int16_t enterprise_code=(*option60<<8)+*(option60+1);
	u_int8_t fieldtype=*(option60+2);
	u_int8_t fieldlen=*(option60+3);
	u_int8_t *fieldval=option60+4;
	unsigned long long R=0,STB=0,MAC=0,encrypt=0;
	u_int8_t str[8]={};
	int i;
	
	DEBUG(LOG_INFO,"key=%s", key);
	DEBUG(LOG_INFO,"optlen=%hhu, ecode=%hu, ftype=%hhu, flen=%hhu", optlen, enterprise_code, fieldtype, fieldlen);
	DEBUG(LOG_INFO,"chaddr=%02X:%02X:%02X:%02X:%02X:%02X\n", chaddr[0], chaddr[1], chaddr[2], chaddr[3], chaddr[4], chaddr[5]);
	if(optlen!=(sizeof(enterprise_code)+sizeof(fieldtype)+sizeof(fieldlen)+(sizeof(R)+sizeof(encrypt))))
		return 0;
	if(enterprise_code!=0x0000 || fieldtype!=31 || fieldlen!=(sizeof(R)+sizeof(encrypt)))
		return 0;

	strncpy(str, key, sizeof(str)-1);
	for(i=0; i<8; i++)
	{
		R|=(unsigned long long)fieldval[i]<<((8-i-1)*8);
		STB|=(unsigned long long)str[i]<<((8-i-1)*8);
		MAC|=(unsigned long long)chaddr[i]<<((8-i-1)*8);
		encrypt|=(unsigned long long)fieldval[i+8]<<((8-i-1)*8);
	}
	DEBUG(LOG_INFO,"R=0x%016llX, STB=0x%016llX, MAC=0x%016llX, encrypt=0x%016llX, (R+STB)^MAC=0x%016llX , R+STB=0x%016llX\n", R, STB, MAC, encrypt, (R+STB)^MAC, R+STB);

	if(((R+STB)^MAC) == encrypt)
	{
		DEBUG(LOG_INFO,"STB AUTHED!");
		return 1;
	}
	return 0;
}

int parse_cmcc_vender_STB_JS(unsigned char* option60, u_int8_t *chaddr)
{
#if 1
	u_int8_t optlen=*(option60-1);
	u_int16_t enterprise_code=(*option60<<8)+*(option60+1);
	u_int8_t fieldtype=*(option60+2);
	u_int8_t fieldlen=*(option60+3);
	u_int8_t *fieldval=option60+4;
    int len = 0;
	//unsigned long long R=0,ZJSTB=0,MAC=0,encrypt=0;
	u_int8_t str[8]={};
	int i;
    char TS[8] = {0};
    char R[8] = {0};
    char DESC[16] = {0};
    char AUTH;
    char O;
    char username[40] = {0};
    char password[40] = {0};
    char buf_username[100];
    char buf_password[100];
    char encrypt[16];
    //fieldval: 4A 53 43 4D 43 43 2D 4F 54 54
    //00 01
    //92 5B BF 32 4B 06 95 6B
    //00 00 00 00 5F D2 EB 8A 
    //B1 53 46 E9 BC DC 0F E7 65 C1 87 8E 69 97 12 99 
    //88 08 8A F3 00 C7 27 F1 15 77 66 91 DB 95 10 5B 

	printf("--parse_cmcc_vender_STB_JS---optlen=%hhu, ecode=%hu, ftype=%hhu, flen=%hhu\n", optlen, enterprise_code, fieldtype, fieldlen);
    
	if(optlen!=(sizeof(enterprise_code)+sizeof(fieldtype)+sizeof(fieldlen)+(sizeof(R)+sizeof(TS)+sizeof(encrypt)))+10+1+1+16)
	{
		DEBUG(LOG_ERR, "parse option 60 length error");
		return 0;
	}
	if(enterprise_code!=0x0002 || fieldtype!=31 || fieldlen!=(sizeof(R)+sizeof(TS)+sizeof(encrypt)+10+1+1+16))
	{
		DEBUG(LOG_ERR, "parse option 60 content error");
		return 0;
    }
    int k;

    output_hex("fieldval: ",fieldval,fieldlen);

    memcpy(DESC,fieldval,10);
    len += 10;

    if(!strncmp(DESC,"JSCMCC-ITV",10))
        return -1;

    if(strncmp(DESC,"JSCMCC-OTT",10))
        return 0;

    AUTH = fieldval[10];
    O = fieldval[11];

    if( AUTH != 0 && AUTH !=1)
        return 0;
    
    if(O != 1)
        return 0;
    
    len += 2;
    memcpy(R,fieldval + len,8);
    len += 8;
    memcpy(TS,fieldval + len,8);
    len += 8;

    memcpy(buf_password,fieldval+len,16);
    len += 16;

    char ottAccount[128] = "JSCMCC-OTT";
    char ottPassword[128] = "";

    mib_get_s(PROVINCE_OTT_ACCOUNT,ottAccount, sizeof(ottAccount));
    mib_get_s(PROVINCE_OTT_PASSWORD,ottPassword, sizeof(ottPassword));


    memcpy(buf_username,fieldval+len,fieldlen-len);
    output_hex("before username: ",buf_username,fieldlen-len);
    int username_len = fieldlen-len;
    len += fieldlen-len;


    unsigned char mode;
	mib_get_s(PROVINCE_LAN_STB_AUTH,&mode, sizeof(mode));
    int ret = 0;
    if( !mode)
    {
        return 1;
    }
    else if( 1 == mode || ( 3 == mode && !AUTH))
    {
        get_decrypt_text(buf_username, R, TS, username,1);
        if(strncmp(ottAccount,username,strlen(ottAccount)))
        {
            return -1;
        }
    }
    else if( 2 == mode || ( 3 == mode))
    {
        get_decrypt_text(buf_username, R, TS, username,1);

        if(strncmp(ottAccount,username,strlen(ottAccount)))
            return -1;
        
        get_key(ottPassword, R, TS, password);
        output_hex("after encrypted: ",password,16);
        output_hex("before encrypted: ",buf_password,16);
        if(strncmp(password,buf_password,16))
            return -1;
    }
    else
    {
        return -1;
    }
#endif
    return 1;

}
void add_cmcc_option125_subdata(char *value, u_int8_t code, char *subdata,u_int8_t subdata_len){
	value[0] = code;
	value[1] = subdata_len;
	memcpy(value+2, subdata, subdata_len);
}

int add_cmcc_option_125(struct dhcpMessage *oldpacket, struct dhcpMessage *packet, char type, char *province){
	unsigned char option[1+1+256];
	int offset,opt_data_len=0;
	char *value;
	short vlan[4]={0x2000,0x2000,0x2000,0x2000};
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	u_int8_t data_len=0;
	char data[250]={0};
	char subdata[64];
	u_int8_t subdata_len;
	int other_wan_running=0;
	
	unsigned char enterprise[4] = {0x00, 0x00, 0x00, 0x00};
	if(type == DHCPOFFER){
		entryNum = mib_chain_total(MIB_ATM_VC_TBL);
		for (i = 0; i < entryNum; i++)
		{
			if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
				continue;
			if(Entry.applicationtype & X_CT_SRV_INTERNET && Entry.vlan == 1)
				vlan[0] = Entry.vid;
			if(Entry.applicationtype & X_CT_SRV_OTHER && Entry.vlan == 1){
				vlan[1] = Entry.vid;
				char ifname[IFNAMSIZ];
			
				if(Entry.mVid != 0 && ifGetName(Entry.ifIndex, ifname, IFNAMSIZ))
				{
					int flags_found,flags;
					struct in_addr inAddr;
					flags_found = getInFlags(ifname, &flags);
					if (flags_found && (flags & IFF_UP)&&getInAddr(ifname, IP_ADDR, &inAddr) == 1){
						other_wan_running = 1;
					}else break;
				}
			}
			if(Entry.applicationtype & X_CT_SRV_TR069 && Entry.vlan == 1)
				vlan[2] = Entry.vid;
			if(Entry.applicationtype & X_CT_SRV_VOICE && Entry.vlan == 1)
				vlan[3] = Entry.vid;
		}
		if(other_wan_running==0){
			printf("other wan not running, do not add option 125\n");
			return -1;
		}

		value = data;
		//subopt_code 1 manufaturer
		memset(subdata, 0 , 64);
		subdata_len = 0;
		mib_get_s(MIB_HW_CWMP_MANUFACTURER,& subdata, sizeof(subdata));
		//sub-opt max len 32
		if(strlen(subdata)> 32)
			subdata_len = 32;
		else subdata_len = strlen(subdata);

		add_cmcc_option125_subdata(value, 1, subdata,subdata_len);
		
		data_len += subdata_len+2;
		value += subdata_len+2;
		
		//subopt_code 2 DHCP server Type,default "HGW-CT"
		memset(subdata, 0 , 64);
		subdata_len = strlen("HGW_CT");
		memcpy(subdata, "HGW_CT", subdata_len);

		add_cmcc_option125_subdata(value, 2, subdata,subdata_len);
		
		data_len += subdata_len+2;
		value += subdata_len+2;
		//subopt_code 3 model name
		memset(subdata, 0 , 64);
		subdata_len = 0;
		mib_get_s(MIB_GPON_ONU_MODEL,& subdata, sizeof(subdata));
		//sub-opt max len 32
		if(strlen(subdata)> 32)
			subdata_len = 32;
		else subdata_len = strlen(subdata);

		add_cmcc_option125_subdata(value, 3, subdata,subdata_len);
		
		data_len += subdata_len+2;
		value += subdata_len+2;

		//subopt_code 10~13 wan vlan
		for(i = 0;i< 4;i++){
			memset(subdata, 0 , 64);
			subdata_len = 2;
			memcpy(subdata, (void *)&vlan[i], sizeof(short));

			add_cmcc_option125_subdata(value, 10+i, subdata,subdata_len);

			data_len += subdata_len+2;
			value += subdata_len+2;		
		}

		//subopt_code 74 CMCCJSHGU"
		memset(subdata, 0 , 64);
		subdata_len = strlen("CMCCJSHGU");
		memcpy(subdata, "CMCCJSHGU", subdata_len);

		add_cmcc_option125_subdata(value, 74, subdata,subdata_len);

		data_len += subdata_len+2;
		value += subdata_len+2;

		if(data_len > 250){
			printf("error option len exceed max length 250!!");
			return -1;
		}
		option[OPT_CODE] = DHCP_VI_VENSPEC;
		opt_data_len = data_len+5;
		option[OPT_LEN] = opt_data_len;
		offset= OPT_DATA;
	
		value = option + OPT_DATA;
		memcpy(value, enterprise, 4);

		value += 4;

		memcpy(value, &data_len, 1);
		value += 1;

		memcpy(value, data, data_len);
#if 0
		{
		int i;
		printf("dump option 125\n");
		for(i = 0;i<opt_data_len+2;i++){
			printf("%02x ",option[i]);
			if(i%16==0&&i!=0)
				printf("\n");
		}
		printf("\ndump option 125 end\n");
		}
#endif		
		add_option_string(packet->options, option);
	}
	return 0;
}
#endif	

#if defined(IP_BASED_CLIENT_TYPE)
int parse_CTC_Vendor_Class(unsigned char* option60, struct dhcp_ctc_client_info *pstClientInfo)
{
	unsigned short enterprise_code=(*option60<<8)+*(option60+1);
	unsigned char *fieldtype;
	unsigned char fieldlength;
	int iOptlen, i;
	struct server_config_t *pDhcp;
#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
#if defined(CONFIG_CU)
	char tempSzVendorClassID[DHCP_VENDORCLASSID_LEN];
#endif
#endif

	DEBUG_CHN_TEL("__FUNCTION__, enterprise_code=%04x\n", enterprise_code);

	pstClientInfo->iCategory = NULL;

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	// Magicia: Every device other than default type is set to Computer.
	for (pDhcp=&server_config; pDhcp; pDhcp=pDhcp->next)
	{
		if(pDhcp->vendorclass && !strcmp(pDhcp->vendorclass, "Computer"))
		{
			pstClientInfo->iCategory = pDhcp->clientRange;
			break;
		}
	}
#endif
	pstClientInfo->category = CTC_Computer;

	fieldtype=(option60+2);
	iOptlen = *(option60-1);

#if 0
#ifdef CONFIG_CU_FTTR_MASTER
	if(!strncmp(option60,"dslforum.org", iOptlen)){
		printf("%s:%d fttr sub connect!\n",__func__,__LINE__);
		pstClientInfo->category = CTC_FTTRsub;
		return 0;
	}
#endif
#endif

#ifdef CONFIG_CU
	if(enterprise_code!=0x0001)
#else
	if(enterprise_code!=0x0000)
#endif		
	{
		//ql 20090119 START: for imagenio STB string should be "[IAL]"
/*ping_zhang:20090312 START:add STB checkbox for designate STB device type*/
	#if 0 //mark for STB with different option60
		if (strstr(option60, "[IAL]"))
			pstClientInfo->category = CTC_STB;
		else
			pstClientInfo->category = CTC_UNKNOWN;
	#endif
/*ping_zhang:20090312 END*/
#ifdef IMAGENIO_IPTV_SUPPORT
		struct client_category_t *pDhcp;
		for (pDhcp=server_config.clientRange; pDhcp; pDhcp=pDhcp->next) {
			if (strstr(option60, pDhcp->option60)) {
/*ping_zhang:20090312 START:add STB checkbox for designate STB device type*/
				pstClientInfo->category = pDhcp->device_type;
/*ping_zhang:20090312 END*/
				pstClientInfo->iCategory = pDhcp;

				return 0;
			}
		}
#endif
#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
		unsigned char val = 1;
#if !defined(CONFIG_CU)
		mib_get_s(PROVINCE_SICHUAN_SUPPER_DHCP_LEASE,&val, sizeof(val));	
#endif
		if(val)
		{
			snprintf(pstClientInfo->szVendor, sizeof(pstClientInfo->szVendor), "%s", option60);
#ifdef CONFIG_CU
			/* option60 format of phone: (3C) (len) (android-xxx/dhcp-xxx)*/
			if (strstr(pstClientInfo->szVendor, "android") || strstr(pstClientInfo->szVendor, "dhcp"))
				pstClientInfo->category = CTC_PHONE;
			else if (strstr(pstClientInfo->szVendor, "MSFT") || strstr(pstClientInfo->szVendor, "computer"))
				pstClientInfo->category = CTC_Computer;
			else if (strstr(pstClientInfo->szVendor, "STB"))
				pstClientInfo->category = CTC_STB;
			else if (strstr(pstClientInfo->szVendor, "MVT"))
				pstClientInfo->category = CTC_MVT;
#ifdef CONFIG_CU_FTTR_MASTER
			else if (strstr(pstClientInfo->szVendor, "FTTR_EdgeONT"))
				pstClientInfo->category = CTC_FTTRsub;
#endif
#endif
			return 0;
		}
		else
			return -1;
#else
		//ql 20090119 END
		return -1;// not china telecom enterprise code..
#endif 
	}

#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
	pstClientInfo->isCtcVendor = 1;
#endif

	while (iOptlen > 0)
	{
		fieldlength = *(fieldtype + 1);
		switch (*fieldtype)
		{
			case Vendor:
				if ((fieldlength < DHCP_CTC_MIN_FIELD_LEN) || (fieldlength > DHCP_CTC_MAX_FIELD_LEN))
				{
				  return -1;
				}

				memcpy(pstClientInfo->szVendor, fieldtype + 2, fieldlength);
				pstClientInfo->szVendor[fieldlength] = 0;
				break;

			case Category:
				if ((fieldlength < DHCP_CTC_MIN_FIELD_LEN) || (fieldlength > DHCP_CTC_MAX_FIELD_LEN))
				{
					return -1;
				}
#if defined(CONFIG_CU)
				memcpy(pstClientInfo->szcategory, fieldtype + 2, fieldlength);
				pstClientInfo->szcategory[fieldlength] = 0;
#endif
#ifdef CONFIG_CU_FTTR_MASTER
				if ((fieldlength == strlen("FTTR_EdgeONT")) && (0 == memcmp(fieldtype + 2, "FTTR_EdgeONT", fieldlength)))
				{
					printf("%s:%d fttr sub connect!\n",__func__,__LINE__);
					pstClientInfo->category = CTC_FTTRsub;
				}
#endif
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
//				for (pDhcp=server_config.clientRange; pDhcp; pDhcp=pDhcp->next)
				for (pDhcp=&server_config; pDhcp; pDhcp=pDhcp->next)
				{
					if(pDhcp->vendorclass && strstr((char*)fieldtype + 2, pDhcp->vendorclass))
					{
						pstClientInfo->iCategory = pDhcp->clientRange;
						break;
					}
				}

				for (i = 0; g_apszCTCDeviceName[i]; i++)
				{
					if ((fieldlength == strlen(g_apszCTCDeviceName[i])) && (0 == memcmp(fieldtype + 2, g_apszCTCDeviceName[i], fieldlength)))
					{
						pstClientInfo->category = i;
						break;
					}
				}
#endif
				//pstClientInfo->iCategory = NULL;
				break;
			case Model:
				if ((fieldlength < DHCP_CTC_MIN_FIELD_LEN) || (fieldlength > DHCP_CTC_MAX_FIELD_LEN))
				{
					return -1;
				}

				memcpy(pstClientInfo->szModel, fieldtype + 2, fieldlength);
				pstClientInfo->szModel[fieldlength] = 0;
				break;
			case Version:
				if ((fieldlength < DHCP_CTC_MIN_FIELD_LEN) || (fieldlength > DHCP_CTC_MAX_FIELD_LEN))
				{
				return -1;
				}

				memcpy(pstClientInfo->szVersion, fieldtype + 2, fieldlength);
				pstClientInfo->szVersion[fieldlength] = 0;
				break;
			case ProtocolType:
				memcpy((char *)(&pstClientInfo->stPortForwarding.usProtocol), fieldtype + 2, sizeof(unsigned short));
				memcpy((char *)(&pstClientInfo->stPortForwarding.usPort), fieldtype + 4, sizeof(unsigned short));
				break;
			default:
				break;
		}
		iOptlen -= fieldlength + 2;
		fieldtype += fieldlength + 2;
	}

#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
#if defined(CONFIG_CU)
	snprintf(tempSzVendorClassID, DHCP_VENDORCLASSID_LEN, "%s:%s:%s", pstClientInfo->szVendor, pstClientInfo->szcategory, pstClientInfo->szModel);
	//snprintf(pstClientInfo->szVendorClassID, DHCP_VENDORCLASSID_LEN, "%s", tempSzVendorClassID);
	snprintf(pstClientInfo->szVendorClassID, DHCP_VENDORCLASSID_LEN, "%s", tempSzVendorClassID);
#endif
#endif

	return 0;
}

//int check_type(u_int32_t addr, enum DeviceType devicetype)
int check_type(u_int32_t addr_n, struct client_category_t *deviceCategory, struct server_config_t *server_config_p)
{
	int ret=0;
	u_int32_t addr=ntohl(addr_n);

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	if (server_config_p != p_serverpool_config)
	{
		if ((addr<ntohl(server_config_p->start)) || (addr>ntohl(server_config_p->end)))
			ret = 1;
	}
	else
#endif
	{
		if (deviceCategory == NULL)
		{
			if ((addr<ntohl(server_config_p->start)) || (addr>ntohl(server_config_p->end)))
				ret = 1;
		}
		else
		{
			if ((addr<ntohl(deviceCategory->ipstart)) ||
				(addr>ntohl(deviceCategory->ipend)))
				ret = 1;
		}
	}
#if 0
	switch(devicetype)
	{
		case CTC_Computer:
			if(addr<ntohl(server_config.pcstart)||addr>ntohl(server_config.pcend))
				ret=1;
			break;
		case CTC_Camera:
			if(addr<ntohl(server_config.cmrstart)||addr>ntohl(server_config.cmrend))
				ret=1;
			break;
		case CTC_STB:
			if(addr<ntohl(server_config.stbstart)||addr>ntohl(server_config.stbend))
				ret=1;
			break;
		case CTC_PHONE:
			if(addr<ntohl(server_config.phnstart)||addr>ntohl(server_config.phnend))
				ret=1;
			break;
		case CTC_HGW:
			if(addr<ntohl(server_config.hgwstart)||addr>ntohl(server_config.hgwend))
				ret=1;
			break;
		case CTC_UNKNOWN:
			if(addr<ntohl(server_config.start)||addr>ntohl(server_config.end))
				ret=1;
			if((addr>ntohl(server_config.pcstart)&&addr<ntohl(server_config.pcend))||
			   	(addr>ntohl(server_config.cmrstart)&&addr<ntohl(server_config.cmrend))||
				(addr>ntohl(server_config.stbstart)&&addr<ntohl(server_config.stbend))||
				(addr>ntohl(server_config.phnstart)&&addr<ntohl(server_config.phnend)))
				ret=1;
			break;
		default:
			break;
	}
#endif

	return ret;
}

#endif
//added by jim luo to support china telecom e8 spec.
#ifdef CONFIG_CTC_E8_CLIENT_LIMIT
#if 0
/*
static int acceptedAllowedNumbers(struct CTC_Clients *clients, int range);
static void blockingClient(unsigned int ip)
{
	unsigned char buffer[128];
		//va_cmd("/bin/iptables"  6, 1, "-I", "FORWARD", "-s", ip, "-j", "DROP");
	sprintf(buffer, "/bin/iptables -I FORWARD -s %d.%d.%d.%d -j DROP",
					(ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip>>0)&0xFF);

	system(buffer);
	printf("blocking client %08x\n", ip);
}
static void unblockingClient(unsigned int ip)
{
	unsigned char buffer[128];
	//va_cmd("/bin/iptables"  6, 1, "-D", "FORWARD", "-s", ip, "-j", "DROP");
	sprintf(buffer, "/bin/iptables -D FORWARD -s %d.%d.%d.%d -j DROP",
					(ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip>>0)&0xFF);

	system(buffer);
	printf("unblocking client %08x\n", ip);
}

//push out the unused elements in list.....
static void reorderClientsList(struct CTC_Clients *clients, int range)
{
	int i;
	for(i=0;  i<range; i++)
	{
		if(clients[i].ip==0)
		{	//copress
			int j,k;
			j=i;
			for(k=0; k<range-i-1; k++, j++)
				memcpy(&clients[j], &clients[j+1], sizeof(struct CTC_Clients));
		}
	}
}
static void removeUnexistClients(struct CTC_Clients *clients, int range)
{
	int i;
	for(i=0; i< range; i++)
	{
		if((clients[i].ip!=0)&&(clients[i].detected==0))
		{	// the blocked connection is not detected, then unblock it from iptables.
			if(clients[i].state==0) //blocked
				unblockingClient(clients[i].ip); //remove the iptables blocking entry
			clients[i].ip=0;
			clients[i].state=0;
		}
		if(clients[i].ip==0)
		{	//copress
			int j,k;
			j=i;
			for(k=0; k<range-i-1; k++, j++)
				memcpy(&clients[j], &clients[j+1], sizeof(struct CTC_Clients));
		}

	}
	return ;
}
static int updateClientsState(struct CTC_Clients *clients, int range, int maxclients)
{
	int clientsAllowedAlready;
	int clientsCanAllowed;
	int clientsAllowedFromBlocking=0;
	int i,j;
	clientsAllowedAlready=acceptedAllowedNumbers(clients, range);
	clientsCanAllowed=maxclients-clientsAllowedAlready;   //we can allow some blocked connections to be allowed to access WAN..
	for(i=0,j=0; i< clientsCanAllowed; i++)
	{
		for( ; j<range; j++)
		{
			if(clients[j].ip && clients[j].state==0)  //blocked state
			{
				clients[j].state=1; // allowed state.
				unblockingClient(clients[j].ip);
				clientsAllowedFromBlocking++;
				break;
			}
		}
	}
	return clientsAllowedAlready+clientsAllowedFromBlocking; // return the updated allow/block connection nunbers to caller.
}
static int clientInList(struct CTC_Clients *clients, int range,  unsigned long ip, int * state)
{
	int i;
	for(i=0; i< range; i++)
	{
		if(clients[i].ip==0)
			return -1;
		if(clients[i].ip==ip)
		{
			*state=clients[i].state;
			//clients[i].detected=1; // set detected flags.
			return i; // found..
		}
	}
	return -1; //not found..
}
static int acceptedAllowedNumbers(struct CTC_Clients *clients, int range)
{
	int i;
	int numbers=0;
	for(i=0; i<range; i++)
	{
		if( ( clients[i].ip!=0) &&(clients[i].state==1)) //unblocking
			numbers++;
	}
	return numbers;
}
static int insertClients(struct CTC_Clients  *clients, int range, unsigned int ip, int state)
{
	int i;
	for(i=0; i<range; i++)
	{
		if(clients[i].ip==ip || clients[i].ip==0)
		{
			clients[i].ip=ip;
			clients[i].state=state;
			return 0;
		}
	}
	return -1;
}
static struct CTC_Clients *getNextClient(struct CTC_Clients * clients, int *range)
{
	int i;
	if(*range==0)
		return NULL;
	for(i=0;i<*range; i++)
	{
		if(clients->ip!=0)
		{
			*range=*range-i-1;
			return clients;
		}
		clients++;
	}
	return NULL;
}


static int checkClients(unsigned int ip, enum DeviceType devtype)
{
	//using Arp request to detect the accepted-clients is still here.
	struct CTC_Clients * client;
	unsigned char buffer[1024];
	unsigned char recv_buffer[1024];
	unsigned char *curptr;
	int len,i;
	unsigned int ipparts[4];
	int range;
	int pcNum=0;
	int cameraNum=0;
	int pcNumAllowed=0;
	int pcNumBlocked=0;
	int cameraNumAllowed=0;
	int cameraNumBlocked=0;
	FILE * fd;
	curptr=buffer;
	//printf("checkClients in\n");
	fd=fopen(CLIENTSMONITOR, "r+");
	if(fd==NULL)
		return -1;
	//printf("checkClients in 2\n");
	memset(buffer, 0, sizeof(buffer));
	// send Arp Request to eavy accepted-clients
	client=accepted_PC_Clients;
	range=MAXPCCLIENTS;
	len=sprintf(curptr, "1 "); // set detect flag...
	if(len < 0)
		goto err;
	curptr+=len;

	while(client=getNextClient(client, &range))
	{
		//sendArpRequest(*client);
		len=sprintf(curptr, "%d.%d.%d.%d ", (client->ip>>24)&0xFF, (client->ip>>16)&0xFF,
										(client->ip>>8)&0xFF, (client->ip>>0)&0xFF); // set ip to detect...
		if(len < 0)
			goto err;
		curptr+=len;
		client++; //next client
	}
	//printf("checkClients in 3\n");
	client=accepted_Camera_Clients;
	range=MAXCAMERACLIENTS;
	while(client=getNextClient(client, &range))
	{
		//sendArpRequest(*client);
		len=sprintf(curptr, "%d.%d.%d.%d ", (client->ip>>24)&0xFF, (client->ip>>16)&0xFF,
										(client->ip>>8)&0xFF, (client->ip>>0)&0xFF); // set ip to detect...
		if(len < 0)
			goto err;
		curptr+=len;
		client++;
	}
	//printf("checkClients in 4\n");
	// before gather detected clients info, zero detected flag...
	for(i=0; i< MAXPCCLIENTS; i++)
	{
		accepted_PC_Clients[i].detected=0;
	}
	for(i=0; i< MAXCAMERACLIENTS; i++)
	{
		accepted_Camera_Clients[i].detected=0;
	}


// do 3 times try detect...
{
	int localcount=0;
retry:
	//printf("retry count\n");
	fclose(fd);
	fd=fopen(CLIENTSMONITOR, "r+");
	fwrite(buffer, 1, strlen(buffer),fd);
	fflush(fd);
	//though china telecom spec need 3 times detect to determined if still existed, but one time is enough...
	usleep(200000);// wait arp reply
	memset(recv_buffer, 0, sizeof(recv_buffer));
	curptr=recv_buffer;
	//printf("checkclients: read\n");
	len=fread(curptr, 1,  1024, fd);
	//fscanf(fd, "%-16s 0x%-8x0x%-6x%s"
	//						"   *     %s\n",
	if(len < 0)
		goto err;
	//printf("checkClients in 5\n");
	printf("%s\n", curptr);

	for(i=0; i< MAXCAMERACLIENTS+MAXPCCLIENTS; i++)
	{
		unsigned long ip;
		int state;
		int index;
		if(sscanf(curptr, "%d.%d.%d.%d", &ipparts[0],&ipparts[1],&ipparts[2],&ipparts[3])<=0)
			break;
		ip=(ipparts[0]<<24) |( ipparts[1]<<16) |( ipparts[2]<<8) |( ipparts[3]<<0);
		if((index=clientInList(accepted_PC_Clients, MAXPCCLIENTS, ip, &state))>=0)
		{
			accepted_PC_Clients[index].detected=1;
		}else if((index=clientInList(accepted_Camera_Clients, MAXCAMERACLIENTS, ip, &state))>=0)
		{
			accepted_Camera_Clients[index].detected=1;
		}
		//search next ip.
		while((*curptr != ' ') && (*curptr != 0))
		{
			curptr++;
		}
		if(*curptr==0)
			break;
		curptr++;
	}
	localcount++;
	if(localcount<3)
		goto retry;
}
	// now all the clients-connected have been polled.
	removeUnexistClients(accepted_PC_Clients,MAXPCCLIENTS);
	removeUnexistClients(accepted_Camera_Clients, MAXCAMERACLIENTS);
	pcNumAllowed=updateClientsState(accepted_PC_Clients,MAXPCCLIENTS, maxPCClients);
	cameraNumAllowed=updateClientsState(accepted_Camera_Clients,MAXCAMERACLIENTS, maxCameraClients);
	if(devtype == CTC_Carema)
	{
		int state;
		if(cameraNumAllowed<maxCameraClients)
			state=1;   //allowed
		else
		{
			state=0;
			blockingClient(ip);
		}
		insertClients(accepted_Camera_Clients,MAXCAMERACLIENTS, ip, state);


	}else // Computers device default
	{
		int state;
		if(pcNumAllowed<maxPCClients)
			state=1;   //allowed
		else
		{
			state=0;
			blockingClient(ip);
		}
		insertClients(accepted_PC_Clients,MAXPCCLIENTS, ip, state);
	}

//reset arp-detecting to idle
	memset(buffer, 0, sizeof(buffer));
	curptr=buffer;
	len=sprintf(curptr, "0 "); // set detect flag...
	if(len < 0)
		goto err;
	//printf("checkClients in 6\n");
	fwrite(buffer, 1, strlen(buffer),fd);
	fclose(fd);
	return 0;
err:
	//printf("checkClients err out\n");
	fclose(fd);
	return -1;


}*/

static void handle_clients_limit(struct dhcpMessage *packet,unsigned long yiaddr)
{
	unsigned char *classVendor;
	enum DeviceType      devicetype;
	char pass_buffer[256];
	char *curptr=pass_buffer;
	int len;
	FILE * fd;
    struct dhcp_ctc_client_info stClientInfo;
/*ping_zhang:20090313 START:Telefonica DHCP option new request*/
	unsigned char vendor_len = 0;
	unsigned char classVendorStr[256] = {0};
/*ping_zhang:20090313 END*/
	//printf("checkClients in\n");

	//check the device type: PC or Camera...
	printf("handle_clients_limit in\n");
	if(!(classVendor=get_option(packet, DHCP_VENDOR)))
		//default : PC clients....
		devicetype=CTC_Computer;
	else
	{
/*ping_zhang:20090313 START:Telefonica DHCP option new request*/
		vendor_len=*(unsigned char*)(classVendor-OPT_LEN);
		memcpy(classVendorStr,classVendor,vendor_len);
		classVendorStr[vendor_len]=0;
/*ping_zhang:20090313 END*/
        	memset(&stClientInfo, 0, sizeof(struct dhcp_ctc_client_info));
/*ping_zhang:20090313 START:Telefonica DHCP option new request*/
		//parse_CTC_Vendor_Class(classVendor, &stClientInfo);
		parse_CTC_Vendor_Class(classVendorStr, &stClientInfo);
/*ping_zhang:20090313 END*/
        devicetype = (enum DeviceType)(stClientInfo.iCategory);
	}
	// send it to kernel...
	fd=fopen(CLIENTSMONITOR, "r+");
	if(fd==NULL)
	{
		printf("no such proc file %s\n", CLIENTSMONITOR);
		return;
	}
	memset(pass_buffer, 0, sizeof(pass_buffer));

//debug management path through proc/ClientsMonitor.....
if(0) // test pass, below path is OK....
{
	len=sprintf(curptr, "flagLimitOnAll "); // set ip to detect...
	if(len < 0)
		goto err;
	curptr+=len;
	len=sprintf(curptr, "%d\n", 0);
	curptr+=len;

	len=sprintf(curptr, "limitOnAll "); // set ip to detect...
	if(len < 0)
		goto err;
	curptr+=len;
	len=sprintf(curptr, "%d\n", 252);
	curptr+=len;

	len=sprintf(curptr, "computerLimitEnable "); // set ip to detect...
	if(len < 0)
		goto err;
	curptr+=len;
	len=sprintf(curptr, "%d\n", 0);
	curptr+=len;

	len=sprintf(curptr, "limitOnComputer "); // set ip to detect...
	if(len < 0)
		goto err;
	curptr+=len;
	len=sprintf(curptr, "%d\n", 201);
	curptr+=len;

	len=sprintf(curptr, "cameraLimitEnable "); // set ip to detect...
	if(len < 0)
		goto err;
	curptr+=len;
	len=sprintf(curptr, "%d\n", 1);
	curptr+=len;

	len=sprintf(curptr, "limitOnCamera "); // set ip to detect...
	if(len < 0)
		goto err;
	curptr+=len;
	len=sprintf(curptr, "%d\n", 39);
	curptr+=len;
}
	//the format should conformance with kernel arp.c
	len=sprintf(curptr, "IP              Types\n"); // set ip to detect...
	if(len < 0)
		goto err;
	curptr+=len;
	len=sprintf(curptr, "%d.%d.%d.%d %d\n", (yiaddr>>24)&0xFF, (yiaddr>>16)&0xFF,
										(yiaddr>>8)&0xFF, (yiaddr>>0)&0xFF, (int)devicetype); // set ip to detect...
	if(len < 0)
		goto err;
	curptr+=len;
	len=fwrite(pass_buffer, 1, 2+(unsigned)curptr-(unsigned)pass_buffer, fd);	//some trail zero should be included.
	fclose(fd);
	printf("handle_clients_limit exit len=%d\n", len);
	if(1)
	{
			printf("proc wirrten: %s", pass_buffer);
	}
	return;
err:
	fclose(fd);
	return;

}
#endif
#endif
/* send a packet to giaddr using the kernel ip stack */
static int send_packet_to_relay(struct dhcpMessage *payload)
{
	DEBUG(LOG_INFO, "Forwarding packet to relay");

	return kernel_packet(payload, server_config.server, SERVER_PORT,
			payload->giaddr, SERVER_PORT);
}


/* send a packet to a specific arp address and ip address by creating our own ip packet */
static int send_packet_to_client(struct dhcpMessage *payload, int force_broadcast)
{
	unsigned char *chaddr;
	u_int32_t ciaddr;

	if (force_broadcast) {
		DEBUG(LOG_INFO, "broadcasting packet to client (NAK)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	} else if (payload->ciaddr) {
		DEBUG(LOG_INFO, "unicasting packet to client ciaddr");
		ciaddr = payload->ciaddr;
		chaddr = payload->chaddr;
	} else if (ntohs(payload->flags) & BROADCAST_FLAG) {
		DEBUG(LOG_INFO, "broadcasting packet to client (requested)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	} else {
		DEBUG(LOG_INFO, "unicasting packet to client yiaddr");
		ciaddr = payload->yiaddr;
		chaddr = payload->chaddr;
	}
	return raw_packet(payload, server_config.server, SERVER_PORT,
			ciaddr, CLIENT_PORT, chaddr, server_config.ifindex);
}


/* send a dhcp packet, if force broadcast is set, the packet will be broadcast to the client */
static int send_packet(struct dhcpMessage *payload, int force_broadcast)
{
	int ret;

#ifdef DHCPD_MULTI_THREAD_SUPPORT
	sock_mutex_lock();
#endif
	if (payload->giaddr)
		ret = send_packet_to_relay(payload);
	else ret = send_packet_to_client(payload, force_broadcast);
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	sock_mutex_unlock();
#endif
	return ret;
}


static void init_packet(struct dhcpMessage *packet, struct dhcpMessage *oldpacket, char type, struct server_config_t *server_config_p)
{
	init_header(packet, type);
	packet->xid = oldpacket->xid;
	memcpy(packet->chaddr, oldpacket->chaddr, 16);
	packet->flags = oldpacket->flags;
	packet->giaddr = oldpacket->giaddr;
	packet->ciaddr = oldpacket->ciaddr;
	add_simple_option(packet->options, DHCP_SERVER_ID, server_config_p->server);
}


/* add in the bootp options */
static void add_bootp_options(struct dhcpMessage *packet, struct server_config_t *server_config_p)
{
	packet->siaddr = server_config_p->siaddr;
	if (server_config_p->sname)
		strncpy((char *)packet->sname, server_config_p->sname, sizeof(packet->sname) - 1);
	if (server_config_p->boot_file)
		strncpy((char *)packet->file, server_config_p->boot_file, sizeof(packet->file) - 1);
}


// Added by Mason Yu for MAC Base assignment
unsigned long find_IP_by_Mac(unsigned char * packet_chaddr)
{
	char tmp_mac[6][3];
	char chaddr[19];
	FILE *fp;
	char temps[0x100];
	unsigned long value;
	char *str;
	struct in_addr matchIp;
	int offset;
	int i;

	for (i=0; i<6; i++) {
		if ( packet_chaddr[i] <= 0xf )
			sprintf(tmp_mac[i], "0%x",  packet_chaddr[i]);
		else
			sprintf(tmp_mac[i], "%x",  packet_chaddr[i]);
	}

	sprintf(chaddr, "%s-%s-%s-%s-%s-%s:", tmp_mac[0], tmp_mac[1], tmp_mac[2], tmp_mac[3], tmp_mac[4], tmp_mac[5]);
	//printf("chaddr=%s\n", chaddr);

	value = 0;

	if ((fp = fopen("/var/dhcpdMacBase.txt", "r")) == NULL)
	{
		printf("Open file /var/dhcpdMacBase.txt fail !\n");
		return 0;
	}

	while (fgets(temps,0x100,fp))
	{
		if (temps[strlen(temps)-1]=='\n')
			temps[strlen(temps)-1] = 0;

		if ((str=strstr(temps, chaddr)) != NULL)
		{
			offset = strlen(chaddr)+1;
			//printf("The string is %s\n", str+offset);

			inet_aton(str+offset, &matchIp);
			value = matchIp.s_addr;
			//printf("value=0x%x\n", value);
			break;

		} else {
			continue;

	        }

	}

  fclose(fp);
	return value;
}

//star add: for static ip based Mac
int find_Mac_by_IP(u_int32_t ipaddr)
{
	struct in_addr matchIp;
	char ip[20];
	char * str=0;
	FILE *fp;
	int value=0;
	char temps[0x100];

	matchIp.s_addr = ipaddr;
	if (strlen(inet_ntoa(matchIp)) < sizeof(ip))
		strcpy(ip, inet_ntoa(matchIp));
	else
		strncpy(ip, inet_ntoa(matchIp), sizeof(ip));

	if ((fp = fopen("/var/dhcpdMacBase.txt", "r")) == NULL)
	{
		printf("Open file /var/dhcpdMacBase.txt fail !\n");
		return 0;
	}

	while (fgets(temps,0x100,fp))
	{
		if (temps[strlen(temps)-1]=='\n')
			temps[strlen(temps)-1] = 0;

		if ((str=strstr(temps, ip)) != NULL)
		{
			value = 1;
			break;

		} else {
			continue;
		}
	}
	//jim: we should release all resource, fixed by jim
	fclose(fp);
	return value;

}

#ifdef SUPPORT_DHCP_RESERVED_IPADDR
static const char DHCPReservedIPAddrFile[] = "/var/udhcpd/DHCPReservedIPAddr.txt";
int isReservedIPAddress(u_int32_t ipaddr, struct server_config_t *server_config_p)
{
	FILE *fp;
	int ret=0, step=0;
	char buf[32];
	unsigned int instnum=0;

	fp=fopen( DHCPReservedIPAddrFile, "r" );
	if(!fp) return ret;

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	instnum = server_config_p->cwmpinstnum;
#endif //_PRMT_X_TELEFONICA_ES_DHCPOPTION_

	//LOG(LOG_INFO, "isReservedIPAddress(): ipaddr=0x%08x, instnum=%u", ipaddr, instnum );
	while( fgets(buf, 32, fp) )
	{
		char *p;

		p=strchr( buf, '\n' );
		if(p) *p=0;
		p=strchr( buf, '\r' );
		if(p) *p=0;

		//LOG(LOG_INFO, "isReservedIPAddress(): got (buf=%s)", buf );
		if(step==0)
		{
			if( (strncmp( buf, "START ", 6 )==0) && ( strlen(&buf[6])>0 ) )
			{
				unsigned int cur_instnum;
				cur_instnum=atoi( &buf[6] );
				if(cur_instnum==instnum)
					step=1;
			}
		}else{
			struct in_addr matchIP;

			if( strncmp( buf, "END", 3 )==0 ) break;
			if( (inet_aton(buf,&matchIP)!=0) && (matchIP.s_addr==ipaddr) )
			{
				//LOG(LOG_INFO, "isReservedIPAddress(): found ipaddr=0x%08x", ipaddr );
				ret=1;
				break;
			}
		}
	}
	fclose(fp);
	//LOG(LOG_INFO, "isReservedIPAddress(): end ret=%d", ret );
	return ret;
}
#endif //SUPPORT_DHCP_RESERVED_IPADDR

// Added by Mason Yu for PPP Half Bridge
int PPP_HALFBRIDGE_GET_WANIP = FALSE;
//int PPP_HALFBRIDGE_GET_WANIP = TRUE;
int IPOA_HALFBRIDGE_GET_WANIP = FALSE;
int FIRST_SEND_PUBLIC_IP = FALSE;
int FIRST_SEND_ACK = TRUE;
int RENEW_PUBLIC_IP = FALSE;

#ifdef CTC_YUNMESTB_DHCPD_DHCPOPTION 
/*check normal setupbox or intelligent TV
return 1 ---> normal setup box
return 0 ---> others intelligent TV belong to this.
*/
int check_option60_enterprise_code(unsigned char* option60)
{

	unsigned short enterprise_code=(*option60<<8)+*(option60+1);
	unsigned char *fieldtype;
	unsigned char fieldlength;
	int iOptlen, i;
	//DHCPD_PRT("enterprise_code=%x\n",enterprise_code);
	if(enterprise_code != 0x0000)
		return 0;
	//00001f --> normal setup box. add normal option 125
	//if(enterprise_code = 0x0000)
	{
		fieldtype=(option60+2);
		iOptlen = *(option60-1);
//DHCPD_PRT("fieldtype=%d iOptlen=%d\n",*fieldtype,iOptlen);
		while(iOptlen > 0)
		{
			fieldlength = *(fieldtype + 1);
			switch (*fieldtype)
			{
				case ShangHaiSTB0:
				case ShangHaiSTB1:				
					return 1;
				break;
				default:
					break;
			}
			iOptlen -= fieldlength + 2;
			fieldtype += fieldlength + 2;			
		}

	}
	return 0;

}
#endif
/* send a DHCP OFFER to a DHCP DISCOVER */
int sendOffer(struct dhcpMessage *oldpacket, struct server_config_t *server_config_p)
{
	struct dhcpMessage packet;
	struct dhcpOfferedAddr *lease = NULL;
	u_int32_t req_align, lease_time_align = server_config.lease;
	unsigned char *req, *lease_time;
	struct option_set *curr;
	struct in_addr addr;
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	unsigned int old_valid_lease_time = 0;
#endif
#if defined CONFIG_CMCC
	int cmcc_STB_check = 0;
	unsigned char stb_auth = 0;
#endif
	// Added by Mason Yu for Half Bridge
	// Added by Mason Yu. Access internet fail.
	unsigned long myipaddr=0, hisipaddr, dnsipaddr;
	FILE *fp;
	//static int skfd = -1;
	//struct sockaddr_in *addrtemp;
	//struct in_addr pAddr;
	//char incoming_route[100];

	init_packet(&packet, oldpacket, DHCPOFFER, server_config_p);

	// Added by Mason Yu for Half Bridge, Start
	DEBUG_CHN_TEL("server_config.ippt = %d\n", server_config.ippt);
	if ( server_config.ippt )  {
		/*
		if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("creat socket fail");
			return (0);
		}
		*/
         	// If we can get WAN IP, that is the WAN interface is up.
         	/*
         	if (skfd >= 0) {
             		strcpy(ifr.ifr_name, "ppp0");
             		ifr.ifr_addr.sa_family = AF_INET;
             		if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0) {
             			PPP_HALFBRIDGE_GET_WANIP = TRUE;
            		 }
         	}
         	*/

         	// Read global variables from sppp process.
	 	fp = fopen ("/tmp/PPPHalfBridge", "r");
         	if (fp) {
			if (fread(&myipaddr, 4, 1, fp) != 1) {
				fclose(fp);
				return -1;
			}
			if (fread(&hisipaddr, 4, 1, fp) != 1) {
				fclose(fp);
				return -1;
			}
			// Added by Mason Yu. Access internet fail.
			if (fread(&dnsipaddr, 4, 1, fp) != 1) {
				fclose(fp);
				return -1;
			}
			fclose(fp);
			PPP_HALFBRIDGE_GET_WANIP = TRUE;
         	} else {
         		PPP_HALFBRIDGE_GET_WANIP = FALSE;
         	}

         	// Read global variables from goahead process.
	 	fp = fopen ("/tmp/IPoAHalfBridge", "r");
         	if (fp) {
			if (fread(&myipaddr, 4, 1, fp) != 1) {
				fclose(fp);
				return -1;
			}
			if (fread(&hisipaddr, 4, 1, fp) != 1) {
				fclose(fp);
				return -1;
			}
			fclose(fp);
			IPOA_HALFBRIDGE_GET_WANIP = TRUE;
         	} else {
         		IPOA_HALFBRIDGE_GET_WANIP = FALSE;
         	}

         	// If we can not find public IP in the release table, we should distribute the public IP to DHCP client.
         	if (( (!(lease = find_lease_by_yiaddr(myipaddr)) ||
         	      /* or it expired and we are checking for expired leases */
	 	    	  (lease_expired(lease))) &&

	 	    	  /* and it isn't on the network */
				(!check_ip(myipaddr, oldpacket->chaddr, server_config_p))  ) ||
	 	          ((lease = find_lease_by_yiaddr(myipaddr))&&
	 	          !memcmp(lease->chaddr,packet.chaddr,16))) {
  //       		  printf("No host use the public IP\n");
         		  FIRST_SEND_PUBLIC_IP = TRUE;
         	}
    	}
#ifdef IMAGENIO_IPTV_SUPPORT
	enum DeviceType devicetype;
	devicetype = getDeviceType();
#elif defined(IP_BASED_CLIENT_TYPE)
//	enum DeviceType devicetype;
	struct client_category_t *deviceCategory = NULL;
	unsigned char *classVendor=NULL;
	struct dhcp_ctc_client_info stClientInfo;
	/*ping_zhang:20090313 START:Telefonica DHCP option new request*/
	unsigned char len = 0;
	unsigned char classVendorStr[256] = {0};
	/*ping_zhang:20090313 END*/

#ifdef _PRMT_X_CT_COM_DHCP_
	unsigned char check_opt_60;

	mib_get_s(CWMP_CT_DHCPS_CHECK_OPT_60,&check_opt_60, sizeof(check_opt_60));
	if(!check_opt_60 || !(classVendor=get_option(oldpacket, DHCP_VENDOR)))
#else
	if(!(classVendor=get_option(oldpacket, DHCP_VENDOR)))
#endif
	{
		struct server_config_t *pDhcp;
		//default : PC clients....
//		devicetype = CTC_Computer;
		deviceCategory=NULL;
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
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
		/*ping_zhang:20090313 START:Telefonica DHCP option new request*/
		len=*(unsigned char*)(classVendor-OPT_LEN);
		if (len < sizeof(classVendorStr))
			memcpy(classVendorStr, classVendor, len);
		else
			memcpy(classVendorStr, classVendor, sizeof(classVendorStr));
		classVendorStr[len]=0;
		/*ping_zhang:20090313 END*/
		memset(&stClientInfo, 0, sizeof(struct dhcp_ctc_client_info));
		/*ping_zhang:20090313 START:Telefonica DHCP option new request*/
		parse_CTC_Vendor_Class(classVendor, &stClientInfo);
//		parse_CTC_Vendor_Class(classVendorStr, &stClientInfo);
		/*ping_zhang:20090313 END*/
		/*ping_zhang:20090319 START:replace ip range with serving pool of tr069*/
#if 0
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
		devicetype = (enum DeviceType)(stClientInfo.category);
#else
		devicetype = getDeviceType();
#endif
#endif
		/*ping_zhang:20090319 END*/
		deviceCategory = stClientInfo.iCategory;
	}
#endif  // End #elif defined(IP_BASED_CLIENT_TYPE)

	// Modified by Mason Yu for Half Bridge
	// If user choose Half Bridge feature and the DHCP server send offer is the first time
	// after WAN interface is up, we should distribute the WAN IP address to DHCP client.
	if ( ((server_config.ippt == 1) && (PPP_HALFBRIDGE_GET_WANIP == TRUE) && (FIRST_SEND_PUBLIC_IP == TRUE)) ||
		((server_config.ippt == 1) && (IPOA_HALFBRIDGE_GET_WANIP == TRUE) && (FIRST_SEND_PUBLIC_IP == TRUE))) {
		/* Put the ip address into packet that WAN get from PPPoX Server  */
		packet.yiaddr = myipaddr;
	}
	else {
		/* ADDME: if static, short circuit */
		/* the client is in our lease/offered table */
		u_int32_t ip_addr;
		ip_addr = find_IP_by_Mac(oldpacket->chaddr);

		if ((lease = find_lease_by_chaddr(oldpacket->chaddr))
			 	// Mason Yu. Req
			&& ( (ip_addr ==0 && !(find_Mac_by_IP(packet.ciaddr)) ) || ((ip_addr != 0) && (ip_addr == lease->yiaddr)))
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
			&& ((ip_addr != 0) || ((ip_addr == 0) && (isReservedIPAddress(lease->yiaddr, server_config_p)==0)))
#endif //SUPPORT_DHCP_RESERVED_IPADDR
			&& (!check_ip(lease->yiaddr, oldpacket->chaddr, server_config_p))
#ifdef IP_BASED_CLIENT_TYPE
			&& ((lease->yiaddr==myipaddr)||((ip_addr != 0) || (!check_type(lease->yiaddr, deviceCategory, server_config_p))))
#endif
			// Mason Yu
			&& ( ntohl(lease->yiaddr) >= ntohl(server_config_p->start) )
			&& ( ntohl(lease->yiaddr) <= ntohl(server_config_p->end) )
			)
			{
				if (!lease_expired(lease))
					lease_time_align = lease->expires - get_uptime();

				packet.yiaddr = lease->yiaddr;
				
#ifdef DHCPD_MULTI_THREAD_SUPPORT
				/* because each dhcp packet is processed by different pthread, so DISCOVER packet maybe processed later than REQUEST packet,
				 * then lease expire time maybe shrinked unexpectedly.
				 */
				if ((lease->expires-get_uptime()) > server_config_p->offer_time)
					old_valid_lease_time = lease->expires - get_uptime();
#endif
	        	 /* Or the client has a requested ip */
				// jim: starzhang added it to support ip/mac binding.
			}
#if defined(IP_BASED_CLIENT_TYPE) && defined(IMAGENIO_IPTV_SUPPORT)
			else if ((packet.yiaddr = find_IP_by_Mac(packet.chaddr)) && (!check_type(packet.yiaddr, deviceCategory, server_config_p)))
#else
			else if((packet.yiaddr = find_IP_by_Mac(packet.chaddr)) != 0)
#endif
			/*ql 20090119 START: for ip/mac binding condition, if ip is not in right range, it should not be distributed.*/
//#ifndef IMAGENIO_IPTV_SUPPORT
//	         	else if(packet.yiaddr = find_IP_by_Mac(packet.chaddr))
//#else
//			else if ((packet.yiaddr = find_IP_by_Mac(packet.chaddr)) && (!check_type(packet.yiaddr, deviceCategory)))
//#endif
			{
				goto getip;
			}
			else if ((req = get_option(oldpacket, DHCP_REQUESTED_IP))
				/* Don't look here (ugly hackish thing to do) */
				&& memcpy(&req_align, req, 4) 

				/* and the ip is in the lease range */
				&& ntohl(req_align) >= ntohl(server_config_p->start) 
				&& ntohl(req_align) <= ntohl(server_config_p->end) 
				&& (!find_Mac_by_IP(req_align)) 
				/*add by star, for static IP based Mac*/
				/* and its not already taken/offered */ /* ADDME: check that its not a static lease */
#ifdef SUPPORT_DHCP_RESERVED_IPADDR
				&& (isReservedIPAddress(req_align, server_config_p)==0)
#endif //SUPPORT_DHCP_RESERVED_IPADDR
#if defined(IP_BASED_CLIENT_TYPE)
				&& (!check_type(packet.yiaddr, deviceCategory, server_config_p))
#endif				
				)
			{
#ifdef DHCPD_MULTI_THREAD_SUPPORT
				thread_mutex_lock();
#endif
				lease = find_lease_by_yiaddr(req_align);
				if (!lease || lease_expired(lease))
				{
#ifdef DHCPD_MULTI_THREAD_SUPPORT
					//double check if the client has been offered an address.
					lease = find_lease_by_chaddr(oldpacket->chaddr);
					if (!lease || lease_expired(lease))
						add_lease(packet.chaddr, req_align, server_config_p->offer_time-1);
					else
						req_align = lease->yiaddr;
					thread_mutex_unlock();
#endif
					if (!check_ip(req_align, oldpacket->chaddr, server_config_p))
					{
						packet.yiaddr = req_align; /* FIXME: oh my, is there a host using this IP? */
						/* otherwise, find a free IP */ /*ADDME: is it a static lease? */
					}
					else
					{
						if (!packet.yiaddr){
#ifdef IP_BASED_CLIENT_TYPE
							packet.yiaddr = find_address(0, deviceCategory, oldpacket->chaddr, server_config_p );
#else
							packet.yiaddr = find_address(0, oldpacket->chaddr, server_config_p );
#endif
						}
		
						/* try for an expired lease */
						if (!packet.yiaddr){
#ifdef IP_BASED_CLIENT_TYPE
							packet.yiaddr = find_address(1, deviceCategory, oldpacket->chaddr, server_config_p );
#else
							packet.yiaddr = find_address(1, oldpacket->chaddr, server_config_p );
#endif
						}
					}
				}
				else
				{
#ifdef DHCPD_MULTI_THREAD_SUPPORT
					thread_mutex_unlock();
#endif
					if (!packet.yiaddr){
#ifdef IP_BASED_CLIENT_TYPE
						packet.yiaddr = find_address(0, deviceCategory, oldpacket->chaddr, server_config_p );
#else
						packet.yiaddr = find_address(0, oldpacket->chaddr, server_config_p );
#endif
					}
	
					/* try for an expired lease */
					if (!packet.yiaddr){
#ifdef IP_BASED_CLIENT_TYPE
						packet.yiaddr = find_address(1, deviceCategory, oldpacket->chaddr, server_config_p );
#else
						packet.yiaddr = find_address(1, oldpacket->chaddr, server_config_p );
#endif
					}
				}
			}
			else {
				// Mason Yu. Find IP base on Mac
	//			packet.yiaddr = find_IP_by_Mac(packet.chaddr);

		        	// Modified by Mason Yu
		        	//packet.yiaddr = find_address(0, oldpacket->chaddr);
				if (!packet.yiaddr){
#ifdef IP_BASED_CLIENT_TYPE
					packet.yiaddr = find_address(0, deviceCategory, oldpacket->chaddr, server_config_p );
#else
					packet.yiaddr = find_address(0, oldpacket->chaddr, server_config_p );
#endif
				}

				/* try for an expired lease */
				if (!packet.yiaddr){
#ifdef IP_BASED_CLIENT_TYPE
					packet.yiaddr = find_address(1, deviceCategory, oldpacket->chaddr, server_config_p );
#else
					packet.yiaddr = find_address(1, oldpacket->chaddr, server_config_p );
#endif
				}
			}
		}
getip:

	if(!packet.yiaddr) {
		LOG(LOG_WARNING, "no IP addresses to give -- OFFER abandoned");
#ifdef DHCPD_MULTI_THREAD_SUPPORT
		forceKeepAliveChk = 1;
#endif
		return -1;
	}

#ifdef DHCPD_MULTI_THREAD_SUPPORT
	thread_mutex_lock();

	/* client may send multi DISCOVER, following DISCOVER packets maybe processed later the first REQUEST packet, 
	 * then udhcpd may offer a new ip address for one client here, it will remove the valid lease which has been offered.
	 */
	lease = find_lease_by_chaddr(oldpacket->chaddr);
	if (lease && !lease_expired(lease) && (packet.yiaddr!=lease->yiaddr))
	{
		//yiaddr is get from find_address(); it should be released here
		clear_lease(blank_chaddr, packet.yiaddr);
#if defined(IP_BASED_CLIENT_TYPE)
		if(!check_type(lease->yiaddr, deviceCategory, server_config_p))
#endif	
		{
			if(find_IP_by_Mac(packet.chaddr) == 0)
				packet.yiaddr = lease->yiaddr;
			if ((lease->expires-get_uptime()) > server_config_p->offer_time)
				old_valid_lease_time = lease->expires - get_uptime();
		}
	}
	if (!add_lease(packet.chaddr, packet.yiaddr, (old_valid_lease_time>0)?old_valid_lease_time:(server_config_p->offer_time+10))) {
		thread_mutex_unlock();
		LOG(LOG_WARNING, "lease pool is full -- OFFER abandoned");
		return -1;
	}
	thread_mutex_unlock();
#else
	if (!add_lease(packet.chaddr, packet.yiaddr, server_config.offer_time)) {
		LOG(LOG_WARNING, "lease pool is full -- OFFER abandoned");
		return -1;
	}
#endif

	// Modified by Mason Yu for Half Bridge
	// Asigned Lease Time
	if ( (((server_config.ippt == 1) && (PPP_HALFBRIDGE_GET_WANIP == TRUE) && (FIRST_SEND_PUBLIC_IP == TRUE)) ||
	      ((server_config.ippt == 1) && (PPP_HALFBRIDGE_GET_WANIP == TRUE) && (RENEW_PUBLIC_IP == TRUE))) ||
	     (((server_config.ippt == 1) && (IPOA_HALFBRIDGE_GET_WANIP == TRUE) && (FIRST_SEND_PUBLIC_IP == TRUE)) ||
	      ((server_config.ippt == 1) && (IPOA_HALFBRIDGE_GET_WANIP == TRUE) && (RENEW_PUBLIC_IP == TRUE))) )  {
		// Asigned Lease Time for Half Bridge
            	//printf("server_config.ipptlt(sendOffer) = %d\n", server_config.ipptlt);
            	//server_config.ipptlt = 30;
            	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(server_config.ipptlt));

            	// Added by Mason Yu
#ifdef DHCPD_MULTI_THREAD_SUPPORT
				thread_mutex_lock();
            	if (!add_lease(packet.chaddr, packet.yiaddr, (old_valid_lease_time>0)?old_valid_lease_time:server_config.ipptlt)) {
					thread_mutex_unlock();
					LOG(LOG_WARNING, "IPPT: lease pool is full -- OFFER abandoned");
					return -1;
				}
				thread_mutex_unlock();
#else
				if (!add_lease(packet.chaddr, packet.yiaddr, server_config.ipptlt)) {
					LOG(LOG_WARNING, "IPPT: lease pool is full -- OFFER abandoned");
					return -1;
				}
#endif
    	}else {
	       if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) {
	       		memcpy(&lease_time_align, lease_time, 4);
	       		lease_time_align = ntohl(lease_time_align);
	       		if (lease_time_align > server_config_p->lease)
	       			lease_time_align = server_config_p->lease;
	       }

	       /* Make sure we aren't just using the lease time from the previous offer */
	       if (lease_time_align < server_config_p->min_lease)
	       		lease_time_align = server_config_p->lease;
	       /* ADDME: end of short circuit */
	       add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));
    	}

	curr = server_config_p->options;
	// Modified by Mason Yu for Half Bridge
	// The following two situations, we should (1) set 0xffffffff as submet and
	// (2) set PPPoX server IP address as Gateway.
	// (1) The user choose Half Bridge feature and DHCP server send offer first time.
	// (2) The user choose Half Bridge feature and user renew public IP.
	if ( (((server_config.ippt == 1) && (PPP_HALFBRIDGE_GET_WANIP == TRUE) && (FIRST_SEND_PUBLIC_IP == TRUE)) ||
	      ((server_config.ippt == 1) && (PPP_HALFBRIDGE_GET_WANIP == TRUE) && (RENEW_PUBLIC_IP == TRUE))) ||
	     (((server_config.ippt == 1) && (IPOA_HALFBRIDGE_GET_WANIP == TRUE) && (FIRST_SEND_PUBLIC_IP == TRUE)) ||
	      ((server_config.ippt == 1) && (IPOA_HALFBRIDGE_GET_WANIP == TRUE) && (RENEW_PUBLIC_IP == TRUE))) )  {
		//printf("Set gw and subnet by Half Bridge(sendoffer)\n");
		while (curr) {
	       		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)  {
		        	//(1) Set 0xffffffff as submet of the option on this offer.
		        	if (curr->data[OPT_CODE] == DHCP_SUBNET) {
		                  	add_simple_option(packet.options, DHCP_SUBNET, 0xffffffff );

		        	// (2) Set WAN interface address as Gateway of the option on this offer.
		        	}else if ( curr->data[OPT_CODE] == DHCP_ROUTER ) {
		                  	add_simple_option(packet.options, DHCP_ROUTER, hisipaddr );

		                // Added by Mason Yu. Access internet fail.
		                // (3) Set Dns Server of the option on this offer.
		        	}else if ( (PPP_HALFBRIDGE_GET_WANIP) && (curr->data[OPT_CODE] == DHCP_DNS_SERVER) ) {
		                  	add_simple_option(packet.options, DHCP_DNS_SERVER, dnsipaddr );

		        	}else {
					add_option_string(packet.options, curr->data);
		        	}
	        	}
	        	curr = curr->next;
	      	}

	      	FIRST_SEND_PUBLIC_IP = FALSE;
	      	RENEW_PUBLIC_IP = FALSE;

	} else {
	while (curr) {
		if(curr->data[OPT_CODE] == DHCP_VI_VENSPEC) 
		{
#ifdef CTC_YUNMESTB_DHCPD_DHCPOPTION 
			int isNormalSTB=0;
#ifdef IP_BASED_CLIENT_TYPE
			if(classVendor != NULL)
				isNormalSTB = check_option60_enterprise_code(classVendor);
#endif
//DHCPD_PRT("isNormalSTB=%d\n",isNormalSTB);
//for CTC Yueme, Test Case 4.1.2 vlan binding to lan1,3,4
//setup box attach to lan3, intelligent TV attach to lan4
//we need to check option60 value from client to do padding option125 or not.
			add_option_125(&packet, packet.options, curr->data, isNormalSTB);
#elif defined CONFIG_CMCC
#ifdef IP_BASED_CLIENT_TYPE
			mib_get_s(MIB_STB_L3_MC_AUTH, &stb_auth, sizeof(stb_auth));
			if(!stb_auth)
				cmcc_STB_check = 0;
			else if(classVendor != NULL)
				cmcc_STB_check = parse_cmcc_vender_STB(classVendor, oldpacket->chaddr, province);
			if(cmcc_STB_check==0){
				//nomal option 125
				add_option_string(packet.options, curr->data);
			}else if(cmcc_STB_check == 1){
				char ifname[IFNAMSIZ]={};
				//customise option 125
				get_ifname_by_mac("br0", oldpacket->chaddr, ifname);
				add_stb_white_list(ifname, oldpacket->chaddr);
				DEBUG(LOG_INFO,"device is STB");
				if(!strncmp(province, "JS", 2)){
					if(add_cmcc_option_125(oldpacket, &packet,DHCPOFFER,province)<0){
						//add cutomise option fail, add normal option
						add_option_string(packet.options, curr->data);
					}
				}
			}else{
				//auth fail do not send offer at all
				DEBUG(LOG_INFO,"option 60 AUTH fail host %02x-%02x-%02x-%02x-%02x-%02x!!"
				, packet.chaddr[0], packet.chaddr[1], packet.chaddr[2], packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);
				return -1;
			}
#endif
#endif
			curr = curr->next;
			continue;
		}
		//ql 20090119 START: for imagenio service, dns must be 172.26.23.3
#ifndef IMAGENIO_IPTV_SUPPORT
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
#else
		if ((curr->data[OPT_CODE] != DHCP_LEASE_TIME) &&
		    ((devicetype != CTC_STB) || (curr->data[OPT_CODE] != DHCP_DNS_SERVER)))
		    add_option_string(packet.options, curr->data);
#endif
			//ql 20090119 END
			curr = curr->next;
		}
	}
#if defined(CTC_DHCP_OPTION43)
		// jim should attach option  43 to dhcp ack packet.
	add_option_string(packet.options, option43);
#endif
#if defined(CTC_DHCP_OPTION60)
		// jim should attach option  60 to dhcp ack packet.
	add_option_string(packet.options, option60);
#endif
/*star:20080926 START: add for telefornica tr069 request*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	addDhcpdOption(&packet, server_config_p);
#endif
/*star:20080826 END*/

	add_option_61(oldpacket, &packet);
#ifndef CTC_YUNMESTB_DHCPD_DHCPOPTION
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	host_mutex_lock();
#endif
	add_option_12(oldpacket, &packet, DHCPOFFER);
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	host_mutex_unlock();
#endif
#endif
	
#if 0
/*ping_zhang:20090319 START:replace ip range with serving pool of tr069*/
#ifndef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	/*ql 20090119 START: add options 240 for STB Discover*/
#ifdef IMAGENIO_IPTV_SUPPORT
	if (CTC_STB == devicetype)
		addStbOption(&packet);
/*ping_zhang:20090313 START:Telefonica DHCP option new request*/
	else {
		if(deviceCategory && deviceCategory->optionCode!=DHCP_END)
			addReservedOption(&packet,deviceCategory);
	}
/*ping_zhang:20090313 END*/
#endif
	/*ql 20090119 END*/
#endif
/*ping_zhang:20090319 END*/
#endif

	add_bootp_options(&packet, server_config_p);

	addr.s_addr = packet.yiaddr;
	LOG(LOG_INFO, "sending OFFER of %s to %x-%x-%x-%x-%x-%x xid=%#x", inet_ntoa(addr), packet.chaddr[0], packet.chaddr[1], packet.chaddr[2],
					packet.chaddr[3], packet.chaddr[4], packet.chaddr[5], ntohl(packet.xid));
	return send_packet(&packet, 0);
}


int sendNAK(struct dhcpMessage *oldpacket, struct server_config_t *server_config_p)
{
	struct dhcpMessage packet;

	init_packet(&packet, oldpacket, DHCPNAK, server_config_p);

	DEBUG(LOG_INFO, "sending NAK to %x-%x-%x-%x-%x-%x xid=%#x", packet.chaddr[0], packet.chaddr[1], packet.chaddr[2],
					packet.chaddr[3], packet.chaddr[4], packet.chaddr[5], ntohl(packet.xid));
	return send_packet(&packet, 1);
}

#ifdef IP_BASED_CLIENT_TYPE
int set_CTC_port_forwarding(int iSet, u_int32_t ulIp, u_int8_t usProtocol, u_int8_t usPort)
{
	char szAction[4];
	char szProtocol[4];
	unsigned char aucPort[16];
	unsigned char aucDstIP[32];

	switch (usProtocol)
	{
		case PF_UDP:
		case PF_TCP:
			snprintf(szProtocol, sizeof(szProtocol), "%s", PF_UDP == usProtocol ? "udp" : "tcp");
			break;
		case PF_TCP_UDP:
			if (set_CTC_port_forwarding(iSet, ulIp, PF_UDP, usPort))
			{
				if (!set_CTC_port_forwarding(iSet, ulIp, PF_TCP, usPort))
				{
					if (iSet)
					{
						/* undo udp setting */
						set_CTC_port_forwarding(!iSet, ulIp, PF_UDP, usPort);
					}
				}
				else
				{
					/* success */
					return 1;
				}
			}
			else
			{
				if (!iSet)
				{
					/* Clear */
					set_CTC_port_forwarding(iSet, ulIp, PF_TCP, usPort);
				}
			}
			return 0;

		default:
			return 0;
	}

	snprintf(szAction, sizeof(szAction), "%s", iSet ? "-A" : "-D");
	snprintf((char*)aucPort, sizeof(aucPort), "%u", usPort);
	snprintf((char*)aucDstIP, sizeof(aucDstIP), "%s:%s", inet_ntoa(*((struct in_addr *)(&ulIp))), aucPort);

	if (0 == va_cmd(IPTABLES, 15, 1, "-t", "nat", szAction, "PREROUTING", "!", "-i", server_config.interface, "-p", szProtocol, "--dport",
		aucPort, "-j", "DNAT", "--to-destination", aucDstIP))
	{
		/* Set ip filter */
		snprintf(szAction, sizeof(szAction), "%s", iSet ? "-I" : "-D");
		if (0 != va_cmd(IPTABLES, 13, 1, szAction, "ipfilter", "!", "-i", server_config.interface, "-o", server_config.interface, "-p", szProtocol,
			"--dport", aucPort, "-j", "RETURN"))
		{
			if (iSet)
			{
				/* undo nat setting */
				va_cmd (IPTABLES, 15, 1, "-t", "nat", "-D", "PREROUTING", "!", "-i", server_config.interface, "-p", szProtocol, "--dport", aucPort,
					"-j", "DNAT", "--to-destination", aucDstIP);
			}
		}
		else
		{
			/* success */
			return 1;
		}
	}
	else
	{
		if (!iSet)
		{
			/* go on clear ipfilter */
			va_cmd (IPTABLES, 13, 1, szAction, "ipfilter", "!", "-i", server_config.interface, "-o", server_config.interface, "-p", szProtocol, "--dport",
				aucPort, "-j", "RETURN");
		}
	}

	return 0;
}

void CTC_lease_expired(struct dhcpOfferedAddr *pstLease)
{
    struct dhcp_ctc_client_info *pstClientInfo;

    pstClientInfo = (struct dhcp_ctc_client_info *)(pstLease->stClientInfo.pvDHCPClientData);
    if ((0 != pstClientInfo->stPortForwarding.usPort)
        && (pstClientInfo->stPortForwarding.iSet))
    {
        set_CTC_port_forwarding(0, pstLease->yiaddr,
                                 pstClientInfo->stPortForwarding.usProtocol,
                                 pstClientInfo->stPortForwarding.usPort);
    }

    free(pstClientInfo);

    return;
}
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_NAT44
int IsAddrInDhcpPool(u_int32_t addr)
{
	unsigned int i,numpool, dhcpStart, dhcpEnd;
	DHCPS_SERVING_POOL_T entrypool;
	numpool = mib_chain_total( MIB_DHCPS_SERVING_POOL_AWIFI_TBL );
	for( i=0; i<numpool;i++ )
	{
		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_AWIFI_TBL, i, (void*)&entrypool ) )
			continue;

		if( entrypool.enable==0)
			continue;

		if(strcmp(entrypool.poolname,"awifi"))
			continue;

		if(addr >= *(unsigned int *)entrypool.startaddr
			    && addr <= *(unsigned int *)entrypool.endaddr)
			return 1;    
	}

	mib_get_s(MIB_DHCP_POOL_START,(void *)&dhcpStart, sizeof(dhcpStart));
	mib_get_s(MIB_DHCP_POOL_END,(void *)&dhcpEnd, sizeof(dhcpEnd));

	if(addr >= dhcpStart && addr <= dhcpEnd)
		return 1;

	return 0;
}

void addNAT44Rules(u_int32_t addr)
{
	int port_min, port_max, ip4;
	char nat_cmd[256];
	ip4 = addr&0x000000ff;
	
	port_min = (ip4-2)*256+1024;
	port_max = port_min + 255;
	
	sprintf(nat_cmd, "iptables -t nat -I POSTROUTING -s %s -p tcp  -j MASQUERADE --to-ports %d-%d", inet_ntoa(*((struct in_addr *)&addr)),
				port_min, port_max);
	system(nat_cmd);
	
	sprintf(nat_cmd, "iptables -t nat -I POSTROUTING -s %s -p udp  -j MASQUERADE --to-ports %d-%d", inet_ntoa(*((struct in_addr *)&addr)),
				port_min, port_max);
	system(nat_cmd);
}

void delNAT44Rules(u_int32_t addr)
{
	int port_min, port_max, ip4;
	char nat_cmd[256];
	ip4 = addr&0x000000ff;
	
	port_min = (ip4-2)*256+1024;
	port_max = port_min + 255;
	
	sprintf(nat_cmd, "iptables -t nat -D POSTROUTING -s %s -p tcp  -j MASQUERADE --to-ports %d-%d", inet_ntoa(*((struct in_addr *)&addr)),
				port_min, port_max);
	system(nat_cmd);
	
	sprintf(nat_cmd, "iptables -t nat -D POSTROUTING -s %s -p udp  -j MASQUERADE --to-ports %d-%d", inet_ntoa(*((struct in_addr *)&addr)),
				port_min, port_max);
	system(nat_cmd);
}
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_TCPOPT
int IsAddrInAwifiDhcpPool(u_int32_t addr)
{
	unsigned int i,numpool;
	DHCPS_SERVING_POOL_T entrypool;
	numpool = mib_chain_total( MIB_DHCPS_SERVING_POOL_AWIFI_TBL );
	for( i=0; i<numpool;i++ )
	{
		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_AWIFI_TBL, i, (void*)&entrypool ) )
			continue;

		if( entrypool.enable==0)
			continue;

		if(strcmp(entrypool.poolname,"awifi"))
			continue;

		if(addr >= *(unsigned int *)entrypool.startaddr
			    && addr <= *(unsigned int *)entrypool.endaddr)
			return 1;    
	}

	return 0;
}

void addTCPOPTRules(u_int32_t addr,u_int8_t *pchaddr)
{
	char nat_cmd[256];
	char macinfo[32]={0};
	char IfName[32] = {0};
	
	unsigned char macadd[MAC_ADDR_LEN];
	mib_get_s(MIB_ELAN_MAC_ADDR,(void *)macadd, sizeof(macadd));
	snprintf(macinfo, 32, "%02x%02x%02x%02x%02x%02xFFFF%02x%02x%02x%02x%02x%02x", macadd[0], macadd[1],macadd[2], macadd[3], macadd[4], macadd[5],
		pchaddr[0], pchaddr[1], pchaddr[2], pchaddr[3], pchaddr[4], pchaddr[5]);

	if (IsAddrInAwifiDhcpPool(addr))
	{
		sprintf(nat_cmd, "iptables -I FORWARD -s %s -p tcp --syn -j TCPTERMAC --set-termac %s",inet_ntoa(*((struct in_addr *)&addr)), macinfo);
		system(nat_cmd);
	}
	
}

void delTCPOPTRules(u_int32_t addr, u_int8_t *pchaddr)
{
	char nat_cmd[256];
	char macinfo[32]={0};
	char IfName[32] = {0};
	
	unsigned char macadd[MAC_ADDR_LEN];
	mib_get_s(MIB_ELAN_MAC_ADDR,(void *)macadd, sizeof(macadd));
	snprintf(macinfo, 32, "%02x%02x%02x%02x%02x%02xFFFF%02x%02x%02x%02x%02x%02x", macadd[0], macadd[1],macadd[2], macadd[3], macadd[4], macadd[5],
		pchaddr[0], pchaddr[1], pchaddr[2], pchaddr[3], pchaddr[4], pchaddr[5]);

	if (IsAddrInAwifiDhcpPool(addr))
	{
		sprintf(nat_cmd, "iptables -D FORWARD -s %s -p tcp --syn -j TCPTERMAC --set-termac %s", inet_ntoa(*((struct in_addr *)&addr)), macinfo);
		system(nat_cmd);
	}
}
#endif

#ifdef CONFIG_TELMEX_DEV
void addClientOptionInfo(struct dhcpMessage *oldpacket)
{
	int i, num;
	CWMP_DHCP_POOL_CLIENT_OPT_T entry;
	unsigned char *optionptr;
	unsigned char tag, len;
	int skiplen = 0;

	if(!oldpacket)
	{
		//printf("[%s %d]oldpacket is NULL\n", __func__, __LINE__);
		return;
	}
	
	num = mib_chain_total(MIB_CWMP_DHCP_POOL_CLIENT_OPT_TBL);
	for(i=num-1; i>=0; i--)
	{
		if(!mib_chain_get(MIB_CWMP_DHCP_POOL_CLIENT_OPT_TBL, i, (void*)&entry))
		{
			continue;
		}

		if(0==((oldpacket->chaddr[0]^entry.macAddr[0])||(oldpacket->chaddr[1]^entry.macAddr[1])||(oldpacket->chaddr[2]^entry.macAddr[2])
			||(oldpacket->chaddr[3]^entry.macAddr[3])||(oldpacket->chaddr[4]^entry.macAddr[4])||(oldpacket->chaddr[5]^entry.macAddr[5])))
		{
			//printf("[%s %d]delete old entry with same MAC\n", __func__, __LINE__);
			mib_chain_delete(MIB_CWMP_DHCP_POOL_CLIENT_OPT_TBL, i);
		}
	}

	optionptr = oldpacket->options;
	while(skiplen < 308)	//options max len is 308
	{
		tag = optionptr[OPT_CODE + skiplen];
		len = optionptr[OPT_LEN + skiplen];

		//printf("[%s %d]tag=%d, len=%d, skiplen=%d\n", __func__, __LINE__, tag, len, skiplen);
		if(0x35 == tag)
		{
			skiplen += 3;	//skip DHCP_Message_Type
			continue;
		}
		if(0xFF == tag)
			break;

		memset(&entry, 0, sizeof(entry));
		entry.tag = tag;
		entry.len = len;
		memcpy(entry.value, &optionptr[OPT_DATA + skiplen], len);
		memcpy(entry.macAddr, oldpacket->chaddr, MAC_ADDR_LEN);
		mib_chain_add(MIB_CWMP_DHCP_POOL_CLIENT_OPT_TBL, (void*)&entry);
		skiplen += (len+2);
	}
}
#endif

#if defined(CONFIG_CU)
void do_gbk2utf8(char *src, char *dst, int size)
{
	int result = rtk_iconv_scan_multi_bytes(src);
	if(!result){
		memcpy(dst, src, size);
	}
	else{
		if(gbk_check(src, size) == 0)
		{//if not gbk, do nothing
			memcpy(dst, src, size);
			return;
		}
		
		if(rtk_iconv_gbk_to_utf8(src, dst, size))
		{
			memcpy(dst, src, size);
			return;
		}
	}
	return;
}
#endif


int sendACK(struct dhcpMessage *oldpacket, u_int32_t yiaddr, struct server_config_t *server_config_p)
{
	struct dhcpMessage packet;
	struct option_set *curr;
	unsigned char *lease_time;
	u_int32_t lease_time_align = server_config.lease;
	struct in_addr addr;
    	// Added by Mason Yu for Half Bridge
    	// Added by Mason Yu. Access internet fail.
	unsigned long myipaddr, hisipaddr, dnsipaddr;
	FILE *fp;
	struct dhcpOfferedAddr *pstLease;
	//ql 20090119 START: check if it is STB device
#ifdef IMAGENIO_IPTV_SUPPORT
//	enum DeviceType devicetype;
#elif defined(IP_BASED_CLIENT_TYPE) //IMAGENIO_IPTV_SUPPORT
//	enum DeviceType devicetype;
#if defined(CONFIG_CU)
	struct client_category_t *deviceCategory = NULL;
#endif
	unsigned char *classVendor=NULL;
	struct dhcp_ctc_client_info stClientInfo;
/*ping_zhang:20090316 START:Fix the DHCP_VENDOR string bugs*/
	unsigned char len = 0;
	unsigned char classVendorStr[256] = {0};
/*ping_zhang:20090316 END*/
#ifdef _PRMT_X_CT_COM_DHCP_
	unsigned char check_opt_60;
#endif
#endif
	//ql 20090119 END
	myipaddr = 0;

	//ql 20090119 START: check if it is STB device
#ifdef IMAGENIO_IPTV_SUPPORT
//	devicetype = getDeviceType();
#elif defined(IP_BASED_CLIENT_TYPE) //&& defined(IMAGENIO_IPTV_SUPPORT)

#ifdef _PRMT_X_CT_COM_DHCP_
	mib_get_s(CWMP_CT_DHCPS_CHECK_OPT_60,&check_opt_60, sizeof(check_opt_60));
	if(!check_opt_60 || !(classVendor=get_option(oldpacket, DHCP_VENDOR)))
#else
	if(!(classVendor=get_option(oldpacket, DHCP_VENDOR)))
#endif
	{
		struct server_config_t *pDhcp;
		//default : PC clients....
//		devicetype = CTC_Computer;
//		deviceCategory=NULL;
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
		// Magicia: Every device other than default type is set to Computer.
		for (pDhcp=&server_config; pDhcp; pDhcp=pDhcp->next)
		{
			if(pDhcp->vendorclass && !strcmp(pDhcp->vendorclass, "Computer"))
			{
//				deviceCategory = pDhcp->clientRange;
				break;
			}
		}
#endif
	}
	else
	{
/*ping_zhang:20090316 START:Fix the DHCP_VENDOR string bugs*/
		len=*(unsigned char*)(classVendor-OPT_LEN);
		memcpy(classVendorStr,classVendor,len);
		classVendorStr[len]=0;
/*ping_zhang:20090316 END*/
		memset(&stClientInfo, 0, sizeof(struct dhcp_ctc_client_info));
/*ping_zhang:20090316 START:Fix the DHCP_VENDOR string bugs*/
		parse_CTC_Vendor_Class(classVendor, &stClientInfo);
//		parse_CTC_Vendor_Class(classVendorStr, &stClientInfo);
/*ping_zhang:20090316 END*/
//		devicetype = (enum DeviceType)(stClientInfo.category);
//		deviceCategory = stClientInfo.iCategory;
	}
#endif
	//ql 20090119 END

	// Added by Mason Yu for Half Bridge
	if ( server_config.ippt )  {

		fp = fopen ("/tmp/PPPHalfBridge", "r");
		if (fp) {
			if (fread(&myipaddr, 4, 1, fp) != 1) {
				fclose(fp);
				return -1;
			}
			if (fread(&hisipaddr, 4, 1, fp) != 1) {
				fclose(fp);
				return -1;
			}
			// Added by Mason Yu. Access internet fail.
			if (fread(&dnsipaddr, 4, 1, fp) != 1) {
				fclose(fp);
				return -1;
			}

			fclose(fp);
		} else {
			PPP_HALFBRIDGE_GET_WANIP = FALSE;
		}

		fp = fopen ("/tmp/IPoAHalfBridge", "r");
		if (fp) {
			if (fread(&myipaddr, 4, 1, fp) != 1) {
				fclose(fp);
				return -1;
			}
			if (fread(&hisipaddr, 4, 1, fp) != 1) {
				fclose(fp);
				return -1;
			}

			fclose(fp);
		} else {
			IPOA_HALFBRIDGE_GET_WANIP = FALSE;
		}


		if ( yiaddr == myipaddr )  {
			RENEW_PUBLIC_IP = TRUE;
		// Added by Mason Yu. Access internet fail.
		} else if ( !((ntohl(server_config.start) <= ntohl(yiaddr)) && (ntohl(yiaddr)<= ntohl(server_config.end))) ) {
			printf("sendACK: This ip is not on DHCP Pool for IP Passthrough\n");
			sendNAK(oldpacket, server_config_p);
			return -1;
		}
	#if 0
	// Kaohj
	// ippt and bridged IP not yet leased
	else if (myipaddr != 0 && !(pstLease = find_lease_by_yiaddr(myipaddr))) {
		// reject it and let this client be able to discover the bridged IP
        	sendNAK(oldpacket, server_config_p);
         	return -1;
	}
	#endif
	goto init_ack;

    }

	// Added by Mason Yu
#if defined(CONFIG_CU) && defined(IP_BASED_CLIENT_TYPE)
	if(check_type(yiaddr, deviceCategory, server_config_p))
	{
		printf("sendACK: This ip is not on DHCP Pool\n");
		sendNAK(oldpacket, server_config_p);
		return -1;
	}
#else	
	if ( !((ntohl(server_config_p->start) <= ntohl(yiaddr)) && (ntohl(yiaddr)<= ntohl(server_config_p->end))) ) {
		printf("sendACK: This ip is not on DHCP Pool\n");
		sendNAK(oldpacket, server_config_p);
		return -1;
	}
#endif	

init_ack:

	init_packet(&packet, oldpacket, DHCPACK, server_config_p);
	packet.yiaddr = yiaddr;

	// Modified by Mason Yu for Half Bridge
	// Asigned Lease Time
	if ( (((server_config.ippt == 1) && (PPP_HALFBRIDGE_GET_WANIP == TRUE) && (FIRST_SEND_ACK == TRUE)) ||
	      ((server_config.ippt == 1) && (PPP_HALFBRIDGE_GET_WANIP == TRUE) && (RENEW_PUBLIC_IP == TRUE))) ||
	     (((server_config.ippt == 1) && (IPOA_HALFBRIDGE_GET_WANIP == TRUE) && (FIRST_SEND_ACK == TRUE)) ||
	      ((server_config.ippt == 1) && (IPOA_HALFBRIDGE_GET_WANIP == TRUE) && (RENEW_PUBLIC_IP == TRUE))) )  {
            // Asigned Lease Time for Half Bridge
            //printf("server_config.ipptlt(sendAck) = %d\n", server_config.ipptlt);
            add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(server_config.ipptlt));
            lease_time_align = server_config.ipptlt;
    }else {
	        if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) {
	       	     memcpy(&lease_time_align, lease_time, 4);
	       	     lease_time_align = ntohl(lease_time_align);
	       	     if (lease_time_align > server_config_p->lease)
	       		     lease_time_align = server_config_p->lease;
	       	     else if (lease_time_align < server_config_p->min_lease)
	       		     lease_time_align = server_config_p->lease;
	       }

	       add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));
	}

	curr = server_config_p->options;
	// Modified by Mason Yu for Half Bridge
	if((((server_config.ippt == 1) && (PPP_HALFBRIDGE_GET_WANIP == TRUE) && (FIRST_SEND_ACK == TRUE)) ||
		((server_config.ippt == 1) && (PPP_HALFBRIDGE_GET_WANIP == TRUE) && (RENEW_PUBLIC_IP == TRUE))) ||
		(((server_config.ippt == 1) && (IPOA_HALFBRIDGE_GET_WANIP == TRUE) && (FIRST_SEND_ACK == TRUE)) ||
		((server_config.ippt == 1) && (IPOA_HALFBRIDGE_GET_WANIP == TRUE) && (RENEW_PUBLIC_IP == TRUE))))	{
		while (curr) {
			if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)  {
				//(1) Set 0xffffffff as submet of the option on this offer.
				if (curr->data[OPT_CODE] == DHCP_SUBNET) {
					add_simple_option(packet.options, DHCP_SUBNET, 0xffffffff );

				// (2) Set WAN interface address as Gateway of the option on this offer.
				} else if ( curr->data[OPT_CODE] == DHCP_ROUTER ) {
					add_simple_option(packet.options, DHCP_ROUTER, hisipaddr );

				// Added by Mason Yu. Access internet fail.
				// (3) Set Dns Server of the option on this offer.
				}else if ( (PPP_HALFBRIDGE_GET_WANIP) && (curr->data[OPT_CODE] == DHCP_DNS_SERVER) ) {
					add_simple_option(packet.options, DHCP_DNS_SERVER, dnsipaddr );

				}else {
					add_option_string(packet.options, curr->data);
				}
			}
			curr = curr->next;
		}

		FIRST_SEND_ACK = FALSE;
		RENEW_PUBLIC_IP = FALSE;

	} else  {
	      while (curr) {
#ifdef CTC_YUNMESTB_DHCPD_DHCPOPTION 
		if(curr->data[OPT_CODE] == DHCP_VI_VENSPEC) 
		{
			int isNormalSTB=0;
#ifdef IP_BASED_CLIENT_TYPE
			if(classVendor != NULL)
				isNormalSTB = check_option60_enterprise_code(classVendor);
#endif
//DHCPD_PRT("isNormalSTB=%d\n",isNormalSTB);
//for CTC Yueme, Test Case 4.1.2 vlan binding to lan1,3,4
//setup box attach to lan3, intelligent TV attach to lan4
//we need to check option60 value from client to do padding option125 or not.
			add_option_125(&packet, packet.options, curr->data, isNormalSTB);		
			curr = curr->next;
			continue;
		}
#endif
		//ql 20090119 START: for imagenio service, dns must be 172.26.23.3
#ifndef IMAGENIO_IPTV_SUPPORT
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
	      		add_option_string(packet.options, curr->data);
#else
		if ((curr->data[OPT_CODE] != DHCP_LEASE_TIME) &&
		    ((devicetype != CTC_STB) || (curr->data[OPT_CODE] != DHCP_DNS_SERVER)))
		    add_option_string(packet.options, curr->data);
#endif
		//ql 20090119 END
	      	curr = curr->next;
	      }
	}
#if defined(CTC_DHCP_OPTION43)
		// jim should attach option  43 to dhcp ack packet.
	add_option_string(packet.options, option43);
#endif
/*jim:20080717 START according bcm samples behavior, option60 is unnecessary for ack, so mask off it*/
//#if defined(CTC_DHCP_OPTION60)
		// jim should attach option  60 to dhcp ack packet.
//	add_option_string(packet.options, option60);
//#endif
/*jim:20080717 END*/
/*star:20080926 START: add for telefornica tr069 request*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
	addDhcpdOption(&packet, server_config_p);
#endif
/*star:20080826 END*/

	add_option_61(oldpacket, &packet);
#ifndef CTC_YUNMESTB_DHCPD_DHCPOPTION
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	host_mutex_lock();
#endif
	add_option_12(oldpacket, &packet, DHCPACK);
#ifdef DHCPD_MULTI_THREAD_SUPPORT
	host_mutex_unlock();
#endif
#endif

	add_bootp_options(&packet, server_config_p);

	addr.s_addr = packet.yiaddr;
	LOG(LOG_INFO, "sending ACK to %s ifindex %d for %x-%x-%x-%x-%x-%x xid=%#x", inet_ntoa(addr), server_config.ifindex, 
					packet.chaddr[0], packet.chaddr[1], packet.chaddr[2],
					packet.chaddr[3], packet.chaddr[4], packet.chaddr[5], ntohl(packet.xid));

	if (send_packet(&packet, 0) < 0)
		return -1;
#ifdef CONFIG_CMCC_ENTERPRISE
	char cmd[128];
	unsigned int ArpCheatEn = 0;

	mib_get_s(MIB_ARP_CHEAT_ENABLED, (void *)&ArpCheatEn, sizeof(ArpCheatEn));
	if (ArpCheatEn) {
		snprintf(cmd, sizeof(cmd), "/bin/arp -d %s", inet_ntoa(addr));
		system(cmd);

		snprintf(cmd, sizeof(cmd), "/bin/arp -s %s %02x:%02x:%02x:%02x:%02x:%02x", inet_ntoa(addr), packet.chaddr[0], packet.chaddr[1], packet.chaddr[2],
				packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);
		system(cmd);
	}
#endif

#ifdef DHCPD_MULTI_THREAD_SUPPORT
	thread_mutex_lock();
#endif
	pstLease = add_lease(packet.chaddr, packet.yiaddr, lease_time_align);

#ifdef CONFIG_TELMEX_DEV
	addClientOptionInfo(oldpacket);
#endif

#ifdef DHCPD_MULTI_THREAD_SUPPORT
	thread_mutex_unlock();
#endif

    if (NULL != pstLease)
    {
		unsigned char opt12_len = 0;
		unsigned char *opt12_hostname = NULL;
#ifdef GET_VENDOR_CLASS_ID
		unsigned char *opt60_vendor = NULL;
		unsigned char opt60_len = 0;
#endif
		char srcIfName[32] = {0};
		//time_t now;

		pstLease->interfaceType = Host_Ethernet; // default type
		if (get_ifname_by_mac("br0", packet.chaddr, srcIfName) > 0) {
			if (strlen(srcIfName) > 0 && strncmp(srcIfName, "wlan", strlen("wlan")) == 0) {
				pstLease->interfaceType = Host_802_11;
			}
#ifdef CONFIG_CU_FTTR_MASTER			
			else if (strlen(srcIfName) > 0 && strncmp(srcIfName, "eth0.", strlen("eth0.")) == 0) {
				int lanport = 0;
				sscanf(srcIfName, "eth0.%d.0", &lanport);
				if((lanport >= 6) && (lanport < (CONFIG_EXTERNAL_VIRTUAL_PORT_ONU_NUM+6)))
					pstLease->interfaceType = Host_PON;
			}
#endif			
		}

#if 1 //!defined(CTC_YUNMESTB_DHCPD_DHCPOPTION) || defined(CONFIG_00R0)
		opt12_hostname = get_option(oldpacket, DHCP_HOST_NAME);
		if(opt12_hostname){
			opt12_len = *(unsigned char*)(opt12_hostname - OPT_LEN);

			if (opt12_len > sizeof(pstLease->hostName) - 1) {
				opt12_len = sizeof(pstLease->hostName) - 1;
			}

			memset(pstLease->hostName, 0, sizeof(pstLease->hostName));
			if(opt12_len > 0)
			{
				memcpy(pstLease->hostName, opt12_hostname, opt12_len);
				pstLease->hostName[opt12_len] = '\0';
				
#if defined(CONFIG_CU)
				// for glib required utf8 as input, if gbk do convert
				char tmp_hostname[64]={0};
				memcpy(tmp_hostname, pstLease->hostName, sizeof(tmp_hostname));
				memset(pstLease->hostName, 0, 64);
				do_gbk2utf8(tmp_hostname, (char *)pstLease->hostName, sizeof(pstLease->hostName));
#endif
			}
		}
#endif		

		pstLease->stClientInfo.ulDevice = FP_Computer;

#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
		do {
			struct dhcp_ctc_client_info *pstClientInfo = NULL;
			unsigned char *classVendor, *userClassID, *clientID;
			unsigned char optLen;

			if (NULL == pstLease->stClientInfo.pvDHCPClientData)
			{
				pstClientInfo = malloc(sizeof(struct dhcp_ctc_client_info));
				if (NULL != pstClientInfo)
				{
					memset(pstClientInfo, 0, sizeof(struct dhcp_ctc_client_info));
					pstLease->stClientInfo.pvDHCPClientData = pstClientInfo;
				}
			}
			else
				pstClientInfo = pstLease->stClientInfo.pvDHCPClientData;

			if (NULL == pstClientInfo)
				break;
			
			classVendor = get_option(oldpacket, DHCP_VENDOR);
			if (classVendor != NULL)
			{
				optLen = *(unsigned char*)(classVendor-OPT_LEN);
				memcpy(pstClientInfo->szVendor, classVendor, optLen);
				pstClientInfo->szVendor[optLen] = 0;
			}
			else
				memset(pstClientInfo->szVendor, 0, sizeof(pstClientInfo->szVendor));
			
			userClassID = get_option(oldpacket, DHCP_USER_ID);
			if (userClassID != NULL)
			{
				optLen = *(unsigned char*)(userClassID-OPT_LEN);
				memcpy(pstClientInfo->szUserClass, userClassID, optLen);
				pstClientInfo->szUserClass[optLen] = 0;
			}
			else
				memset(pstClientInfo->szUserClass, 0, sizeof(pstClientInfo->szUserClass));

			clientID = get_option(oldpacket, DHCP_CLIENT_ID);
			if (clientID != NULL)
			{
				unsigned char type;
				
				optLen = *(unsigned char*)(clientID-OPT_LEN);
				type = clientID[0];
				if (1 == type)//hw address
				{
					snprintf(pstClientInfo->szClientID, sizeof(pstClientInfo->szClientID), "%02x:%02x:%02x:%02x:%02x:%02x", 
							clientID[1], clientID[2], clientID[3], clientID[4], clientID[5], clientID[6]);
				}
				else
				{
					memcpy(pstClientInfo->szClientID, clientID, optLen);
					pstClientInfo->szClientID[optLen] = 0;
				}
			}
			else
				memset(pstClientInfo->szClientID, 0, sizeof(pstClientInfo->szClientID));
		} while(0);
#endif//end of _PRMT_X_CT_SUPPER_DHCP_LEASE_SC

		//time(&now);
		//pstLease->active_time = (unsigned int)now;
		pstLease->active_time = get_uptime();

#ifdef GET_VENDOR_CLASS_ID
		opt60_vendor = get_option(oldpacket, DHCP_VENDOR);
		if(opt60_vendor){
			opt60_len = *(unsigned char*)(opt60_vendor - OPT_LEN);

			memset(pstLease->vendor_class_id, 0, 64);
			if(opt60_len){
				memcpy(pstLease->vendor_class_id, opt60_vendor, opt60_len);
				pstLease->vendor_class_id[opt60_len] = 0;
			}
		}	
#endif

#ifdef IP_BASED_CLIENT_TYPE
        struct dhcp_ctc_client_info *pstClientInfo = NULL;
		unsigned char *classVendor;
/*ping_zhang:20090316 START:Fix the DHCP_VENDOR string bugs*/
		unsigned char len = 0;
		unsigned char classVendorStr[256] = {0};
/*ping_zhang:20090316 END*/


#ifdef _PRMT_X_CT_COM_DHCP_
		unsigned char check_opt_60;

		mib_get_s(CWMP_CT_DHCPS_CHECK_OPT_60,&check_opt_60, sizeof(check_opt_60));
		if (check_opt_60 && NULL != (classVendor=get_option(oldpacket, DHCP_VENDOR)))
#else
		if (NULL != (classVendor=get_option(oldpacket, DHCP_VENDOR)))
#endif
		{
/*ping_zhang:20090316 START:Fix the DHCP_VENDOR string bugs*/
		len=*(unsigned char*)(classVendor-OPT_LEN);
		if (len >= sizeof(classVendorStr)) {
			fprintf(stderr, "Error: Data length %d exceeds buffer size %zu!\n", len, sizeof(classVendorStr));
			len = sizeof(classVendorStr) - 1;
		}
		memcpy(classVendorStr,classVendor,len);
		classVendorStr[len]='\0';
/*ping_zhang:20090316 END*/
            pstClientInfo = malloc(sizeof(struct dhcp_ctc_client_info));
            if (NULL != pstClientInfo)
            {
                memset(pstClientInfo, 0, sizeof(struct dhcp_ctc_client_info));
/*ping_zhang:20090316 START:Fix the DHCP_VENDOR string bugs*/
                parse_CTC_Vendor_Class(classVendor, pstClientInfo);
//                parse_CTC_Vendor_Class(classVendorStr, pstClientInfo);
/*ping_zhang:20090316 END*/

                pstLease->stClientInfo.pfLeaseExpired = CTC_lease_expired;
                pstLease->stClientInfo.pvDHCPClientData = (void *)pstClientInfo;

                if ((0 != pstClientInfo->stPortForwarding.usPort)
                    && (!pstClientInfo->stPortForwarding.iSet))
                {
                    if (set_CTC_port_forwarding(1, pstLease->yiaddr,
                                                 pstClientInfo->stPortForwarding.usProtocol,
                                                 pstClientInfo->stPortForwarding.usPort))
                    {
                        pstClientInfo->stPortForwarding.iSet = 1;
                    }
                }

		switch (pstClientInfo->category)
                {
#ifdef CONFIG_CU
					case CTC_Camera:
						pstLease->stClientInfo.ulDevice = FP_Camera;
						break;
					case CTC_HGW:
						pstLease->stClientInfo.ulDevice = FP_HGW;
						break;
					case CTC_STB:
						pstLease->stClientInfo.ulDevice = FP_STB;
						break;
					case CTC_MVT:
						pstLease->stClientInfo.ulDevice = FP_MVT;
						break;
					case CTC_WlanAP:
						pstLease->stClientInfo.ulDevice = FP_AP;
						break;
#ifdef CONFIG_CU_FTTR_MASTER
					case CTC_FTTRsub:
						pstLease->stClientInfo.ulDevice = FP_FTTRsub;
						break;
#endif
					case CTC_PHONE:
						pstLease->stClientInfo.ulDevice = FP_MOBILE;
						break;
#else
                    case CTC_STB:
                        pstLease->stClientInfo.ulDevice = FP_STB;
                        break;

                    case CTC_PHONE:
                        pstLease->stClientInfo.ulDevice = FP_MOBILE;
                        break;
#endif

                    default:
                        break;
                }
            }
		}

#ifdef _PRMT_X_CT_SUPPER_DHCP_LEASE_SC
		//piyajee_chen: make sure pstLease->stClientInfo.pvDHCPClientData is not NULL.
		if(NULL == pstClientInfo)
		{
			pstClientInfo = malloc(sizeof(struct dhcp_ctc_client_info));
			if(NULL != pstClientInfo)
			{
				pstLease->stClientInfo.pfLeaseExpired = CTC_lease_expired;   //use to free pvDHCPClientData
				memset(pstClientInfo, 0, sizeof(struct dhcp_ctc_client_info));
				pstLease->stClientInfo.pvDHCPClientData = (void *)pstClientInfo;
			}
		}
		
		unsigned char supper_lease;	
		mib_get_s(PROVINCE_SICHUAN_SUPPER_DHCP_LEASE,&supper_lease, sizeof(supper_lease));

		if(supper_lease)
		{		
			unsigned char opt81_len = 0, fqdn_len = 0;
			unsigned char *opt81_fqdn = NULL;
			opt81_fqdn = get_option(oldpacket, DHCP_FQDN);
			if(opt81_fqdn){
	            if (NULL != pstClientInfo)
	            {					
					opt81_len = *(unsigned char*)(opt81_fqdn - OPT_LEN);

					//printf("opt81_len is %d\n", opt81_len);

					if(opt81_len > 3)
					{
						fqdn_len = opt81_len - 3;
						if(fqdn_len >= DHCP_CTC_FQDN_LEN)
							fqdn_len = DHCP_CTC_FQDN_LEN-1;
						memcpy(pstClientInfo->szFQDN, opt81_fqdn+3, fqdn_len);
						pstClientInfo->szFQDN[fqdn_len] = 0;
					}
            	}
			}
		}
#endif

#ifdef CONFIG_CTC_E8_CLIENT_LIMIT
        update_client_limit(pstLease);
#endif
#endif

#ifdef _PRMT_X_CT_COM_PORTALMNT_
        /* update_force_portal(); */
        update_force_portal(pstLease);
#endif
    }

#ifdef _CWMP_TR111_
	handle_tr111( oldpacket, yiaddr );
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_NAT44
    {
        unsigned char functype=0;
        mib_get_s(AWIFI_PROVINCE_CODE,&functype, sizeof(functype));
        if(functype == AWIFI_ZJ){
            if(IsAddrInDhcpPool(yiaddr))
                addNAT44Rules(yiaddr);
        }
    }
#endif

#ifdef CONFIG_CT_AWIFI_JITUAN_TCPOPT
    {
        unsigned char functype=0;
        mib_get_s(AWIFI_PROVINCE_CODE,&functype, sizeof(functype));
        if(functype == AWIFI_ZJ){
	addTCPOPTRules(yiaddr, packet.chaddr);
        }
    }
#endif
#if 0
#ifdef CONFIG_CTC_E8_CLIENT_LIMIT
	handle_clients_limit(oldpacket,yiaddr);
#endif
#endif

	//dump_leases();

	return 0;
}


int send_inform(struct dhcpMessage *oldpacket, struct server_config_t *server_config_p)
{
	struct dhcpMessage packet;
	struct option_set *curr;

	init_packet(&packet, oldpacket, DHCPACK, server_config_p);

	curr = server_config_p->options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}
#if defined(CTC_DHCP_OPTION43)
		// jim should attach option  43 to dhcp ack packet.
	add_option_string(packet.options, option43);
#endif
#if defined(CTC_DHCP_OPTION60)
		// jim should attach option  60 to dhcp ack packet.
	add_option_string(packet.options, option60);
#endif
	add_bootp_options(&packet, server_config_p);

#ifdef _CWMP_TR111_
	handle_tr111( oldpacket, oldpacket->yiaddr );
#endif

	return send_packet(&packet, 0);
}

/*ql: 20090119 START: add for imagenio service*/
#ifdef IMAGENIO_IPTV_SUPPORT
void addStbOption(struct dhcpMessage *packet)
{
	unsigned char option[120];
/*ping_zhang:20090930 START:add for Telefonica new option 240*/
#if 0
	option[OPT_CODE] = DHCP_DNS_SERVER;
	option[OPT_LEN] = 4;
	memcpy(option+OPT_DATA, &server_config.stbdns1, 4);
	if (server_config.stbdns2) {
		memcpy(option+option[OPT_LEN]+2, &server_config.stbdns2, 4);
		option[OPT_LEN] = 8;
	}
	if (!add_option_string(packet->options, option))
		printf("add option DNS fail\n");

	option[OPT_CODE] = DHCP_OPT_240;
	sprintf(option+OPT_DATA, ":::::%s:%d", inet_ntoa(*(struct in_addr *)&server_config.opchaddr), server_config.opchport);
	option[OPT_LEN] = strlen(option+OPT_DATA);
	if (!add_option_string(packet->options, option))
		printf("add option 240 fail\n");
#else
	struct in_addr stbDnsAdd1,stbDnsAdd2;

	stbDnsAdd1.s_addr = 0xffffffff;
	stbDnsAdd2.s_addr = 0xffffffff;
	mib_get_s(MIB_IMAGENIO_DNS1,(void *)&stbDnsAdd1, sizeof(stbDnsAdd1));
	mib_get_s(MIB_IMAGENIO_DNS2,(void *)&stbDnsAdd2, sizeof(stbDnsAdd2));
	option[OPT_CODE] = DHCP_DNS_SERVER;
	option[OPT_LEN] = 4;

	memcpy(option+OPT_DATA, &stbDnsAdd1.s_addr, 4);
	if (stbDnsAdd2.s_addr!=0xffffffff) {
		memcpy(option+option[OPT_LEN]+2, &stbDnsAdd2.s_addr, 4);
		option[OPT_LEN] = 8;
	}
	if (!add_option_string(packet->options, option))
		printf("add option DNS fail\n");
#endif
/*ping_zhang:20090930 END*/
}

#if 0
/*ping_zhang:20090313 START:Telefonica DHCP option new request*/
addReservedOption(struct dhcpMessage *packet,struct client_category_t *deviceCategory)
{
	unsigned char option[120];

	option[OPT_CODE] = deviceCategory->optionCode;
	strcpy(option+OPT_DATA, deviceCategory->optionStr);
	option[OPT_LEN] = strlen(option+OPT_DATA);
	if (!add_option_string(packet->options, option))
		printf("add option %d fail\n",deviceCategory->optionCode);
}
/*ping_zhang:20090313 END*/
#endif
#endif
/*ql 20090119 END*/

/*star:20080926 START: add for telefornica tr069 request*/
#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
/*ping_zhang:20090319 START:replace ip range with serving pool of tr069*/
unsigned char getDeviceType(void)
{
	DHCPS_SERVING_POOL_T entry;
	unsigned int i,num;
	unsigned char deviceType = CTC_UNKNOWN;

	num = mib_chain_total( MIB_DHCPS_SERVING_POOL_TBL );
	for( i=0; i<num;i++ )
	{
		if( !mib_chain_get( MIB_DHCPS_SERVING_POOL_TBL, i, (void*)&entry ) )
			continue;

		if( entry.InstanceNum==server_config.cwmpinstnum)
		{
			deviceType =  entry.deviceType;
			break;
		}
	}
	return deviceType;
}
/*ping_zhang:20090319 END*/

void addDhcpdOption(struct dhcpMessage *packet, struct server_config_t *server_config_p)
{
	MIB_CE_DHCP_OPTION_T entry;
	int i, entrynum;
	unsigned char option[DHCP_OPT_VAL_LEN+2];

/*ping_zhang:20090319 START:replace ip range with serving pool of tr069*/
	//check if is STB device type
#ifdef IMAGENIO_IPTV_SUPPORT
	if(getDeviceType() == CTC_STB)
		addStbOption(packet);
#endif //IMAGENIO_IPTV_SUPPORT
/*ping_zhang:20090319 END*/

	entrynum = mib_chain_total(MIB_DHCP_SERVER_OPTION_TBL);
	for (i=0; i<entrynum; i++)
	{
		if (!mib_chain_get(MIB_DHCP_SERVER_OPTION_TBL, i, (void *)&entry) || !entry.enable)
			continue;
//#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
		if (entry.dhcpConSPInstNum!=server_config_p->cwmpinstnum)
			continue;
//#endif
		option[OPT_CODE] = (unsigned char)entry.tag;
		option[OPT_LEN] = entry.len;
		memcpy(option+OPT_DATA, entry.value, entry.len);

		add_option_string(packet->options, option);
	}
}

void find_match_serving_pool(struct dhcpMessage *dhcppacket, struct server_config_t **server_config_p){

	struct server_config_t* p_servingpool_tmp=p_serverpool_config->next;
	unsigned char vendorclass[256]={0},clientid[256]={0},userclass[256]={0};
	unsigned char *tmpstr;
	unsigned char len=0;
	unsigned char vendormachflag=0,clientidmachflag=0,userclassmatchflag=0,chaddrmatchflag=0, srcIntfflag=0/*base on sourceinterface*/;
	unsigned char chaddr[6],chaddrmask[6],chaddrmaskresult[6];
	int i;
	unsigned int srcIntf; // Base on sourceinterface
	unsigned char check_opt_60 = 1;
	char srcIfName[32] = {0};

#ifdef _PRMT_X_CT_COM_DHCP_
	mib_get_s(CWMP_CT_DHCPS_CHECK_OPT_60,&check_opt_60, sizeof(check_opt_60));
#endif

	if(check_opt_60)
	{
		tmpstr=get_option(dhcppacket, DHCP_VENDOR);
		if(tmpstr!=NULL){
			len=*(unsigned char*)(tmpstr-OPT_LEN);
			if (len >= sizeof(vendorclass)) {
				len = sizeof(vendorclass)-1;
			}
			memcpy(vendorclass, tmpstr, len);
			vendorclass[len] = '\0';
		}
	}
	tmpstr=get_option(dhcppacket, DHCP_CLIENT_ID);
	if(tmpstr!=NULL){
		len=*(unsigned char*)(tmpstr-OPT_LEN);
		if (len >= sizeof(clientid)) {
			len = sizeof(clientid)-1;
		}
			memcpy(clientid, tmpstr, len);
			clientid[len] = '\0';
	}
	tmpstr=get_option(dhcppacket, DHCP_USER_ID);
	if(tmpstr!=NULL){
		len=*(unsigned char*)(tmpstr-OPT_LEN);
		if (len >= sizeof(userclass)) {
			len = sizeof(userclass)-1;
		}
			memcpy(userclass, tmpstr, len);
			userclass[len] = '\0';
	}

	srcIntf = 0;
	if (get_ifname_by_mac("br0", dhcppacket->chaddr, srcIfName) > 0) {
		if (strlen(srcIfName) > 0) {
			if (strncmp(srcIfName, "eth", strlen("eth")) == 0) {
				srcIntf = (1 << (srcIfName[5] - '0' - 2));
			}
			else if (strncmp(srcIfName, "wlan", strlen("wlan")) == 0) {
#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
                unsigned char functype=0;
                mib_get_s(AWIFI_PROVINCE_CODE,&functype, sizeof(functype));
                if(functype == AWIFI_ZJ){
                    if(!strcmp(srcIfName,"wlan0"))
                        srcIntf = 0x10;
                    else
                        srcIntf = (1 << (5+(srcIfName[9] - '0')));
                }else
#endif
                {
	            #ifdef CONFIG_RTK_DEV_AP
					/*
						wlan0: 1<<4(elan_num:4), wlan0-vap0: 1<<(4+i+1)
						wlan1: 1<<9(elan_num:4+wlan0+wlan0_ap_num:4), wlan1-vap0: 1<<(9+i+1)
					*/
					int wlan_idx, vap_idx, port_bit_mark;
					char *tmp_ptr = NULL;
				#if (CONFIG_MLO_INTF_NUM>0)
					if (strstr(srcIfName, "wlanmlo"))
					{
						port_bit_mark = PMAP_WLANMLO;
					}else
				#endif
					{
						wlan_idx = srcIfName[4]-'0';
						if (wlan_idx == 0)
							port_bit_mark = PMAP_WLAN0;
						else if (wlan_idx == 1)
							port_bit_mark = PMAP_WLAN1;
					}
					if (strstr(srcIfName, "vap"))
					{
						tmp_ptr = strstr(srcIfName, "vap") + strlen("vap");
						vap_idx = tmp_ptr - '0';//start from vap0
						port_bit_mark = port_bit_mark+vap_idx+1;
					}
					srcIntf = (1<<port_bit_mark);
	            #else
					srcIntf = 0x10;
	            #endif
	            }
			}
			//fprintf(stderr, "[%s:%d] srcIfName=%s, srcIntf=0x%x\n", __FUNCTION__,__LINE__,srcIfName, srcIntf);
		}
	}

	while(p_servingpool_tmp!=NULL){

		vendormachflag=0;
		clientidmachflag=0;
		userclassmatchflag=0;
		chaddrmatchflag=0;
		srcIntfflag=0; // Base on sourceinterface

		if(p_servingpool_tmp->vendorclass==NULL
			&& p_servingpool_tmp->sourceinterface==0 // Base on sourceinterface
			&& p_servingpool_tmp->clientid==NULL
			&& p_servingpool_tmp->userclass==NULL
			&& p_servingpool_tmp->chaddr==NULL){
			p_servingpool_tmp=p_servingpool_tmp->next;
			continue;
		}
#ifdef CONFIG_CT_AWIFI_JITUAN_FEATURE
                unsigned char functype=0;
                mib_get_s(AWIFI_PROVINCE_CODE,&functype, sizeof(functype));
                if(functype == AWIFI_DEFAULT && (!strcmp(p_servingpool_tmp->poolname, "awifi")))
					continue;
#endif
		

		if(p_servingpool_tmp->vendorclass){
			if(p_servingpool_tmp->vendorclassflag==0){
				int classlen=0,tmplen=0;
				char *classtmp=0;
				if(!strcmp(p_servingpool_tmp->vendorclassmode,"Exact")){
					if(!strcmp(p_servingpool_tmp->vendorclass,(const char *)vendorclass))
						vendormachflag=1;
				}else if(!strcmp(p_servingpool_tmp->vendorclassmode,"Prefix")){
					classtmp=strstr((const char *)vendorclass,p_servingpool_tmp->vendorclass);
					if((unsigned char *)classtmp==vendorclass)
						vendormachflag=1;
				}else if(!strcmp(p_servingpool_tmp->vendorclassmode,"Suffix")){
					classlen=strlen(p_servingpool_tmp->vendorclass);
					classtmp=strstr((const char *)vendorclass,p_servingpool_tmp->vendorclass);
					if (classtmp != NULL) {
						tmplen=strlen(classtmp);
						if(tmplen==classlen)
							vendormachflag=1;
					}
				}else{ //Substring
					classtmp=strstr((const char *)vendorclass,p_servingpool_tmp->vendorclass);
					if(classtmp!=NULL)
						vendormachflag=1;
				}
			}
			else{
				if(strcmp(p_servingpool_tmp->vendorclass, (const char *)vendorclass))
					vendormachflag=1;
			}
		}else
			vendormachflag=1;

		if(p_servingpool_tmp->clientid){
			if(p_servingpool_tmp->clientidflag==0){
				if(!strcmp(p_servingpool_tmp->clientid, (const char *)clientid))
					clientidmachflag=1;
			}
			else{
				if(strcmp(p_servingpool_tmp->clientid, (const char *)clientid))
					clientidmachflag=1;
			}
		}else
			clientidmachflag=1;

		if(p_servingpool_tmp->userclass){
			if(p_servingpool_tmp->userclassflag==0){
				if(!strcmp(p_servingpool_tmp->userclass,( const char *)userclass))
					userclassmatchflag=1;
			}
			else{
				if(strcmp(p_servingpool_tmp->userclass, (const char *)userclass))
					userclassmatchflag=1;
			}
		}else
			userclassmatchflag=1;

		if(p_servingpool_tmp->chaddr){
			if(!p_servingpool_tmp->chaddrmask)
				memset(chaddrmask,0xff,6);
			else
				rtk_string_to_hex(p_servingpool_tmp->chaddrmask,chaddrmask,12);
			rtk_string_to_hex(p_servingpool_tmp->chaddr,chaddr,12);
			for(i=0;i<6;i++){
				chaddrmaskresult[i]=(dhcppacket->chaddr[i])&chaddrmask[i];
			}
			if(p_servingpool_tmp->chaddrflag==0){
				if(!memcmp(chaddr,chaddrmaskresult,6))
					chaddrmatchflag=1;
			}
			else{
				if(memcmp(chaddr,chaddrmaskresult,6))
					chaddrmatchflag=1;
			}
		}else
			chaddrmatchflag=1;

		// Mason Yu. base on sourceinterface
		if(p_servingpool_tmp->sourceinterface !=0 ){
			if ( ( srcIntf!=0 && (p_servingpool_tmp->sourceinterface&srcIntf)==srcIntf) ) {
				//printf("Find match source Interface. p_servingpool_tmp->sourceinterface=%d, srcIntf=%d\n", p_servingpool_tmp->sourceinterface, srcIntf);
				srcIntfflag=1;
			}
		}else
			srcIntfflag=1;

		if(vendormachflag==1 && clientidmachflag==1 && userclassmatchflag==1 && chaddrmatchflag==1 && srcIntfflag==1)
			break;

		p_servingpool_tmp=p_servingpool_tmp->next;
	}

	if(p_servingpool_tmp==NULL) {
		p_servingpool_tmp=p_serverpool_config;
	}

	DEBUG_CHN_TEL("\ndhcp server find servingpool:%s\n",p_servingpool_tmp->poolname);
	*server_config_p = p_servingpool_tmp;
}
/*
void dumpservingpool()
{
	struct server_config_t* p_servingpool_tmp=p_serverpool_config;

	while(p_servingpool_tmp!=NULL){
		printf("\ntmp=%x",p_servingpool_tmp);
		printf("\npoolname=%s",p_servingpool_tmp->poolname);
		printf("\nstart=%x",p_servingpool_tmp->start);
		printf("\nend=%x",p_servingpool_tmp->end);
		printf("\npoolorder=%d",p_servingpool_tmp->poolorder);
		printf("\nvendorclass=%s",p_servingpool_tmp->vendorclass);
		printf("\nvendorclassflag=%d",p_servingpool_tmp->vendorclassflag);
		printf("\nvendorclassmode=%s",p_servingpool_tmp->vendorclassmode);
		printf("\nclientid=%s",p_servingpool_tmp->clientid);
		printf("\nclientidflag=%d",p_servingpool_tmp->clientidflag);
		printf("\nuserclass=%s",p_servingpool_tmp->userclass);
		printf("\nuserclassflag=%d",p_servingpool_tmp->userclassflag);

		p_servingpool_tmp=p_servingpool_tmp->next;
	}

}
*/
#endif
/*star:20080926 END*/

#ifdef DHCP_OPTION_43_ENABLE
int OPT43_VendorSpecificInformation(unsigned char *optvalue)
{
	int vendor_info_len, i;
	char *subopt = optvalue;

	if(optvalue == NULL)
		return 0;

	vendor_info_len = *(unsigned char *)(optvalue - OPT_LEN);

	while(subopt < optvalue + vendor_info_len)
	{
		unsigned char subcode = *subopt;
		int len = *(subopt+1);
		char *subvalue = (char *)malloc(len+1);

		memcpy(subvalue, subopt+2, len);

		subvalue[len] = 0;

		printf("Option 43 - Vendor-specific Information:\n");
		printf("Sub code=%d\n", subcode);
		printf("Sub info=%s\n", subvalue);
		free(subvalue);
		subopt += 2 + len;
	}

	return 1;
}
#endif

#ifdef DHCP_OPTION_82_ENABLE
int OPT82_RelayAgentInformation(unsigned char *optvalue)
{
	int vendor_info_len, i;
	char *subopt = optvalue;

	if(optvalue == NULL)
		return 0;

	vendor_info_len = *(unsigned char *)(optvalue - OPT_LEN);

	while(subopt < optvalue + vendor_info_len)
	{
		unsigned char subcode = *subopt;
		int len = *(subopt+1);
		char *subvalue = (char *)malloc(len+1);

		memcpy(subvalue, subopt+2, len);

		subvalue[len] = 0;

		printf("Option 82 - Relay Agent Infomation:\n");
		switch(subcode)
		{
			case 1: // Agent Circuit ID Sub-option
				printf("Sub code=%d(Agent Circuit ID Sub-option)\n", subcode);
				break;
			case 2: // Agent Remote ID Sub-option
				printf("Sub code=%d(Agent Remote ID Sub-option)\n", subcode);
				break;
			default:
				printf("Sub code=%d\n", subcode);
				break;
		}
		printf("Sub info=%s\n", subvalue);
		free(subvalue);
		subopt += 2 + len;
	}

	return 1;
}
#endif

