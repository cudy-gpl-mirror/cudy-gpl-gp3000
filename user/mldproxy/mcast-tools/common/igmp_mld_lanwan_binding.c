#include <rtk/utility.h>
#include <linux/mroute6.h>
#include <linux/version.h>
#include <rtk/mib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef void (igmp_mld_set_lanwanBindingInfo)(int,const char*); 
typedef void (igmp_mld_log)(int,int,const char*, ... );

typedef struct igmp_mld_hook_ops
{
	igmp_mld_set_lanwanBindingInfo	*set_lanwanBindingInfo;
	igmp_mld_log					*log;
}IGMP_MLD_HOOK_OPS_T,*IGMP_MLD_HOOK_OPS_Tp;
static IGMP_MLD_HOOK_OPS_T	igmp_mld_hook;

int isValidMedia(unsigned int ifIndex)
{
	MEDIA_TYPE_T mType;

	mType = MEDIA_INDEX(ifIndex);
	if (1
	#ifdef CONFIG_RTL8672_SAR
		&& mType!=MEDIA_ATM
	#endif
	#ifdef CONFIG_PTMWAN
		&& mType!=MEDIA_PTM
	#endif /*CONFIG_PTMWAN*/
	#ifdef CONFIG_ETHWAN
		&& mType!=MEDIA_ETH
	#endif
	#ifdef WLAN_WISP
		&& mType!=MEDIA_WLAN
	#endif
	)
		return 0;
	return 1;
}

char *ifGetName(int ifindex, char *buffer, unsigned int len)
{
	MEDIA_TYPE_T mType;

	if ( ifindex == DUMMY_IFINDEX )
		return 0;
	if (PPP_INDEX(ifindex) == DUMMY_PPP_INDEX)
	{
		mType = MEDIA_INDEX(ifindex);
		if (mType == MEDIA_ATM) {
#ifdef CONFIG_RTL_MULTI_PVC_WAN
			if (VC_MINOR_INDEX(ifindex) == DUMMY_VC_MINOR_INDEX) // major vc devname
				snprintf( buffer, len,  "%s%u", ALIASNAME_VC, VC_MAJOR_INDEX(ifindex));
			else
				snprintf( buffer, len,  "%s%u_%u", ALIASNAME_VC, VC_MAJOR_INDEX(ifindex),  VC_MINOR_INDEX(ifindex));
#else
			snprintf( buffer, len,  "%s%u", ALIASNAME_VC, VC_INDEX(ifindex) );
#endif
		}
		else if (mType == MEDIA_ETH)
#ifdef CONFIG_RTL_MULTI_ETH_WAN
			snprintf( buffer, len, "%s%d", ALIASNAME_MWNAS, ETH_INDEX(ifindex));
#else
			snprintf( buffer, len,  "%s%u", ALIASNAME_NAS, ETH_INDEX(ifindex) );
#endif
#ifdef CONFIG_PTMWAN
		else if (mType == MEDIA_PTM)
			snprintf( buffer, len, "%s%d", ALIASNAME_MWPTM, PTM_INDEX(ifindex));
#endif /*CONFIG_PTMWAN*/
#ifdef WLAN_WISP
		else if (mType == MEDIA_WLAN)
			snprintf( buffer, len, "wlan%d-vxd", ETH_INDEX(ifindex));
#endif
		else if (mType == MEDIA_IPIP)    // Mason Yu. Add VPN ifIndex
			snprintf( buffer, len, "ipip%u", IPIP_INDEX(ifindex));
		else
			return 0;

	}else{
		snprintf( buffer, len,  "ppp%u", PPP_INDEX(ifindex) );

	}

	return buffer;
}

int getIfIndexVidByName(char *pIfname,unsigned int *ifIndex,unsigned short *vid)
{
	unsigned int entryNum, i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];

	entryNum = mib_chain_total(MIB_ATM_VC_TBL);
	for (i=0; i<entryNum; i++) {
		if (!mib_chain_get(MIB_ATM_VC_TBL, i, (void *)&Entry))
		{
  			printf("Get chain record error!\n");
			return -1;
		}

		if (!Entry.enable || !isValidMedia(Entry.ifIndex))
			continue;

		ifGetName(Entry.ifIndex,ifname,sizeof(ifname));

		if(!strcmp(ifname,pIfname)){
			*ifIndex = Entry.ifIndex;
			*vid = Entry.vid;
			if(igmp_mld_hook.log)
				igmp_mld_hook.log(LOG_INFO,0,"default binding to %s,ifIndex=%d,vid=%d\n",ifname,Entry.ifIndex,Entry.vid);
			return 0;
		}
	}

	return -1;
}

int rtk_binding_all_untag_wan(int from_dev_ifindex)
{
	int total,i;
	MIB_CE_ATM_VC_T Entry;
	char ifname[IFNAMSIZ];
	struct IfDesc *tmp_bind_up;
	
	total = mib_chain_total(MIB_ATM_VC_TBL);
	if (total > MAX_VC_NUM){
		if(igmp_mld_hook.log)
				igmp_mld_hook.log(LOG_WARNING, 0, "MIB_ATM_VC_TBL num=%d exceed %d",total,MAX_VC_NUM);
		return -1;
	}

	for(i = 0; i < total; i++) {
		mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&Entry);
		if (!Entry.enable || !isValidMedia(Entry.ifIndex))
			continue;

		if(Entry.vid == 0){
			ifGetName(Entry.ifIndex,ifname,sizeof(ifname));

			if(igmp_mld_hook.set_lanwanBindingInfo)
				igmp_mld_hook.set_lanwanBindingInfo(from_dev_ifindex,ifname);

			if(igmp_mld_hook.log)
				igmp_mld_hook.log(LOG_INFO,0,"binding to untag wan %s",ifname);
		}
	}
	
	return 0;
}

int rtk_get_binding_wan_info(char *name,unsigned int *ifindex,unsigned short *vid)
{
	int total,i,j;
	unsigned short itfGroup;
	MIB_CE_ATM_VC_T Entry;
	
	total = mib_chain_total(MIB_ATM_VC_TBL);
	if (total > MAX_VC_NUM){
		if(igmp_mld_hook.log)
			igmp_mld_hook.log(LOG_WARNING, 0, "MIB_ATM_VC_TBL num=%d exceed %d",total,MAX_VC_NUM);
		return -1;
	}

	for(i = 0; i < total; i++) {
		mib_chain_get(MIB_ATM_VC_TBL, i, (void*)&Entry);
		if (!Entry.enable || !isValidMedia(Entry.ifIndex))
			continue;
		itfGroup = Entry.itfGroup;
		for(j = PMAP_ETH0_SW0; j <= PMAP_ETH0_SW3 && j < ELANVIF_NUM; ++j)
		{
			if((itfGroup & (1<<j)) != 0)
				if(!strcmp((char *)ELANVIF[j],name)){
					if(igmp_mld_hook.log)
						igmp_mld_hook.log(LOG_INFO, 0, "Get LAN-WAN binding, lan interface=%s, wan ifIndex=%d,vid=%d\n",name,Entry.ifIndex,Entry.vid);
					*vid = Entry.vid;
					*ifindex = Entry.ifIndex;
					
					return 0;
				}
		}
		
#ifdef WLAN_SUPPORT
		{
			for (j=PMAP_WLAN0; j<=PMAP_WLAN0_VAP3; j++) {
				if(wlan_en[j-PMAP_WLAN0])
					if((itfGroup & (1<<j)) != 0)
						if(!strcmp((char *)wlan[j-PMAP_WLAN0],name)){
							if(igmp_mld_hook.log)
								igmp_mld_hook.log(LOG_INFO, 0, "Get LAN-WAN binding, lan interface=%s, wan ifIndex=%d,vid=%d\n",name,Entry.ifIndex,Entry.vid);
							*vid = Entry.vid;
							*ifindex = Entry.ifIndex;
							return 0;
						}
			}
		}

		{
			for (j=PMAP_WLAN1; j<=PMAP_WLAN1_VAP3; j++) {
				if(wlan_en[j-PMAP_WLAN0])
					if((itfGroup & (1<<j)) != 0)
						if(!strcmp((char *)wlan[j-PMAP_WLAN0],name)){
							if(igmp_mld_hook.log)
								igmp_mld_hook.log(LOG_INFO, 0, "Get LAN-WAN binding, lan interface=%s, wan ifIndex=%d,vid=%d\n",name,Entry.ifIndex,Entry.vid);
							*vid = Entry.vid;
							*ifindex = Entry.ifIndex;
							return 0;
						}	
			}
		}

#endif // of WLAN_SUPPORT	
	}

	return 0;
}


int rtk_igmp_mld_get_lanwanBindingInfo(int raw_socket,int from_dev_ifindex)
{
	struct ifreq ifr;
	unsigned int ifindex;
	unsigned short vid;
	char ifname[IFNAMSIZ];

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_ifindex=from_dev_ifindex;  
	if (ioctl(raw_socket, SIOCGIFNAME, &ifr) < 0)
	{
		if(igmp_mld_hook.log)
			igmp_mld_hook.log(LOG_WARNING,0,"ioctl SIOCGIFNAME fail");
		return -1;
	}

	if(rtk_get_binding_wan_info(ifr.ifr_name,&ifindex,&vid) < 0)
		return -1;

	if(!vid){
		if(rtk_binding_all_untag_wan(from_dev_ifindex) < 0)
			return -1;
	}else{
		ifGetName(ifindex,ifname,sizeof(ifname));

		if(igmp_mld_hook.set_lanwanBindingInfo){
			igmp_mld_hook.set_lanwanBindingInfo(from_dev_ifindex,ifname);
		}
		if(igmp_mld_hook.log)
			igmp_mld_hook.log(LOG_INFO,0,"binding to tag wan %s",ifname);
	}

	return 0;
}


void rtk_igmp_mld_hook_register(void (*igmp_mld_set_lanwanBindingInfo)(int,const char*),void (*igmp_mld_log)(int,int,const char*, ... ))
{
	igmp_mld_hook.set_lanwanBindingInfo = igmp_mld_set_lanwanBindingInfo;
	igmp_mld_hook.log = igmp_mld_log;
	return;
}
