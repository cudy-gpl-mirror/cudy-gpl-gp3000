#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined(CONFIG_MLDV2_SUPPORT)
#include "mld.h"
#endif
#if defined(CONFIG_IGMPV3_SUPPORT)
#include <linux/in.h>
#include <linux/igmp.h>
#endif

#include "timeout.h"
#include "igmp_mld_utility.h"
#include "igmp_mld_debug.h"
#include "igmp_mld_config.h"


/**********grobal parameters define begin**********/
#ifdef USE_STATIC_ENTRY_BUFFER
struct mcft_entry_en mcft_entry_tbl[MAX_MFCT_ENTRY];
struct mbr_entry_en mbr_entry_tbl[MAX_MBR_ENTRY];
struct ifrec_entry_en ifrec_entry_tbl[MAX_IFREC_ENTRY];
struct src_entry_en src_entry_tbl[MAX_SRC_ENTRY];
#endif
static struct query_timer	startupQueryTimer;
static struct query_timer	generalQueryTimer;
static struct query_timer	linkChangeQueryTimer;
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
struct proxy_timer	igmpV2MldV1Timer[MAX_IFREC_ENTRY];
struct proxy_timer   igmpV1Timer[MAX_IFREC_ENTRY];
int compatibilityStatus[MAX_IFREC_ENTRY] = {0};
#endif

int newGeneralQueryInterval = 0; 

#if defined(CONFIG_MLDV2_SUPPORT)
struct in6_addr MLD_ALL_HOSTS_ADDRESS={{{0xFF,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1}}};
struct in6_addr MLD_ALL_ROUTER_ADDRESS={{{0xFF,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2}}};
struct in6_addr MLD_UNSPECIFIED_ADDRESS = {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}};
#endif

unsigned int ZERO_ADDR[4] = {0};

/**********grobal parameters define end**********/
#ifdef USE_STATIC_ENTRY_BUFFER
void init_static_entry_tbl(void)
{
	memset(mcft_entry_tbl, 0x00, sizeof(struct mcft_entry_en)*MAX_MFCT_ENTRY);
	memset(ifrec_entry_tbl,0x00, sizeof(struct ifrec_entry_en)*MAX_IFREC_ENTRY);
	memset(mbr_entry_tbl, 0x00, sizeof(struct mbr_entry_en)*MAX_MBR_ENTRY);
	memset(src_entry_tbl,0x00, sizeof(struct src_entry_en)*MAX_SRC_ENTRY);
}
struct mcft_entry * find_mcft_entry_from_tbl(void)
{
	int i;
	struct mcft_entry_en *valid_entry;
	
	for(i=0;i<MAX_MFCT_ENTRY;i++){
		valid_entry = &mcft_entry_tbl[i];
		if(valid_entry->valid==0){
			valid_entry->valid=1;
			return (&(valid_entry->entry_mcft_));
		}
	}
	//printf("find_mcft_entry_from_tbl fail\n");
	return NULL;
}

int del_mcft_entry_from_tbl(struct mcft_entry *del_mcft_entry)
{
	
	int i;
	struct mcft_entry_en *valid_entry;
	struct mcft_entry *check_entry;
	//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
	for(i=0;i<MAX_MFCT_ENTRY;i++){
		valid_entry = &mcft_entry_tbl[i];
		check_entry = &(valid_entry->entry_mcft_);
		if(&(valid_entry->entry_mcft_)==del_mcft_entry){
			//printf("delmcft entry\n");
 			memset(valid_entry, 0, sizeof(struct mcft_entry));
			return 1;
		}
	}
	//printf("del_mcft_entry_from_tbl fail\n");
	return 0;
	
}

struct ifrec_entry * find_ifrec_entry_from_tbl(void)
{
	int i;
	struct ifrec_entry_en *valid_entry;
	
	for(i=0;i<MAX_IFREC_ENTRY;i++){
		valid_entry = &ifrec_entry_tbl[i];
		if(valid_entry->valid==0){
			valid_entry->valid=1;
			return (&(valid_entry->entry_ifrec_));
		}
	}
	//printf("find_ifrec_entry_from_tbl fail\n");
	return NULL;
}

int del_ifrec_entry_from_tbl(struct ifrec_entry *del_ifrec_entry)
{
	
	int i;
	struct ifrec_entry_en *valid_entry;
	struct ifrec_entry *check_entry;
	//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
	for(i=0;i<MAX_IFREC_ENTRY;i++){
		valid_entry = &ifrec_entry_tbl[i];
		check_entry = &(valid_entry->entry_ifrec_);
		if(&(valid_entry->entry_ifrec_)==del_ifrec_entry){
			//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
  			memset(valid_entry, 0, sizeof(struct ifrec_entry));
 			return 1;
		}
	}
	//printf("del_ifrec_entry_from_tbl fail\n");
	return 0;
	
}

struct mbr_entry * find_mbr_entry_from_tbl(void)
{
	int i;
	struct mbr_entry_en *valid_entry;
	
	for(i=0;i<MAX_MBR_ENTRY;i++){
		valid_entry = &mbr_entry_tbl[i];
		if(valid_entry->valid==0){
			valid_entry->valid=1;
			//printf("find_mbr_entry_from_tbl success![%s][%d]\n",__FUNCTION__,__LINE__);
			return (&(valid_entry->entry_mbr_));
		}
	}
	//printf("find_mbr_entry_from_tbl fail!!![%s][%d]\n",__FUNCTION__,__LINE__);
	return NULL;
}

int del_mbr_entry_from_tbl(struct mbr_entry *del_mbr_entry)
{
	int i;
	struct mbr_entry_en *valid_entry;
	struct mbr_entry *check_entry;
	//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
	for(i=0;i<MAX_MBR_ENTRY;i++){
		valid_entry = &mbr_entry_tbl[i];
		check_entry = &(valid_entry->entry_mbr_);
		if(&(valid_entry->entry_mbr_)==del_mbr_entry){
			//printf("del mbr entry:user_addr\n");
			memset(valid_entry, 0, sizeof(struct mbr_entry));
			return 1;
		}
	}
	//printf("del_mbr_entry_from_tbl fail\n");
	return 0;
}

struct src_entry * find_src_entry_from_tbl(void)
{
	int i;
	struct src_entry_en *valid_entry;
	
	for(i=0;i<MAX_SRC_ENTRY;i++){
		valid_entry = &src_entry_tbl[i];
		if(valid_entry->valid==0){
			valid_entry->valid=1;
			//printf("find_mbr_entry_from_tbl success![%s][%d]\n",__FUNCTION__,__LINE__);
			return (&(valid_entry->entry_src_));
		}
	}
	//printf("find_mbr_entry_from_tbl fail!!![%s][%d]\n",__FUNCTION__,__LINE__);
	return NULL;
}

int del_src_entry_from_tbl(struct src_entry *del_src_entry)
{
	int i;
	struct src_entry_en *valid_entry;
	struct src_entry *check_entry;
	//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
	for(i=0;i<MAX_MBR_ENTRY;i++){
		valid_entry = &src_entry_tbl[i];
		check_entry = &(valid_entry->entry_src_);
		if(&(valid_entry->entry_src_)==del_src_entry){
			//printf("del src entry:user_addr\n");
			memset(valid_entry, 0, sizeof(struct src_entry));
			return 1;
		}
	}
	//printf("del_mbr_entry_from_tbl fail\n");
	return 0;
}
#endif

struct mcft_entry * add_mcft(unsigned int *grp_addr, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList)
{
	struct mcft_entry *mcp;

#ifndef USE_STATIC_ENTRY_BUFFER 
	mcp = malloc(sizeof(struct mcft_entry));
	if(!mcp)
		return NULL;
#else //static buffer
	mcp =find_mcft_entry_from_tbl();
	if(!mcp){
		return NULL;
	}
#endif

	mcp->ip_version = mCastProxyPara->ipVersion;
	memcpy(mcp->grp_addr,grp_addr,ADDR_SIZE(mcp->ip_version));
	mcp->if_count = 0;
	mcp->iflist = NULL;

	mcp->next = *groupList;
	*groupList = mcp;
	
	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add mcft successfully!\n");
	
	return mcp;
}
int del_mcft(int *grp_addr, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList)
{
	struct mcft_entry **mcq, *mcp;
	struct ifrec_entry *ifn, *ifp;
	struct mbr_entry *gn, *gp;
	struct src_entry *sn, *sp;


	/* Remove the entry from the  list. */
	mcq = groupList;
	mcp = *mcq;
	while(mcp)
	{
		if(memcmp(mcp->grp_addr, grp_addr, ADDR_SIZE(mcp->ip_version))==0)
		{
			//free ifrec list
			*mcq = mcp->next;
			
			ifp = mcp->iflist;
			while(ifp)
			{
				ifn = ifp->next;
				//free group mbr
				gp = ifp->grp_mbr;
				while(gp)
				{
					gn = gp->next;
				#if defined(USE_STATIC_ENTRY_BUFFER)
					del_mbr_entry_from_tbl(gp);
				#else
					free(gp);
				#endif
					gp = gn;
				}

				//free srclist
				sp = ifp->srclist;
				while(sp)
				{
					sn = sp->next;
					//free group mbr
					gp = sp->grp_mbr;
					while(gp)
					{
						gn = gp->next;
					#if defined(USE_STATIC_ENTRY_BUFFER)
						del_mbr_entry_from_tbl(gp);
					#else
						free(gp);
					#endif
						gp = gn;
					}

					//free timeout
					untimeout(&(sp->timer.ch));
				#if defined(USE_STATIC_ENTRY_BUFFER)
					del_src_entry_from_tbl(sp);
				#else
					free(sp);
				#endif
					sp = sn;
				}

				//free timeout
				untimeout(&(ifp->timer.ch));

				#if defined(USE_STATIC_ENTRY_BUFFER)
				del_ifrec_entry_from_tbl(ifp);
				#else
				free(ifp);
				#endif
				ifp = ifn;
			}
				
		#if defined(USE_STATIC_ENTRY_BUFFER)
			del_mcft_entry_from_tbl(mcp);
		#else
			free(mcp)
		#endif
			
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "del mcft successfully!\n");
			return 0;
		}
		mcq = &mcp->next;
		mcp = mcp->next;
	}
	
	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "No mcft!\n");
	return -1;
}
//diff deamon use diff mcp list
int chk_mcft(unsigned int *groupAddr, struct mcft_entry **groupList)
{
	struct mcft_entry *mcp = *groupList;
	while(mcp) 
	{
		if(!memcmp(mcp->grp_addr,groupAddr,ADDR_SIZE(mcp->ip_version)))
			return 1;

		mcp = mcp->next;
	}
	return 0;
}

struct mcft_entry * get_mcft(unsigned int *groupAddr, struct mcft_entry **groupList)
{
	struct mcft_entry *mcp = *groupList;
	while(mcp) 
	{
		if(!memcmp(mcp->grp_addr , groupAddr, ADDR_SIZE(mcp->ip_version)))
		{
			return mcp;
		}
		mcp = mcp->next;
	}
	return NULL;
}
int num_mcft(struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList)
{
	struct mcft_entry *mcp = *groupList;
	int n = 0;
	while(mcp) 
	{
		n++;
		mcp = mcp->next;
	}
	return n;
}

struct ifrec_entry * get_ifrec(struct mcft_entry *mcp,int ifindex)
{
	struct ifrec_entry *if_rec=mcp->iflist;
	while(if_rec) 
	{
		if(if_rec->ifindex==ifindex)
		{
			//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
			return if_rec;
		}
		if_rec = if_rec->next;
	}
	return NULL;
}


//add if
//	0: fail
//	1: duplicate interface
//	2: added new interface successfully
//always get_ifrec first, if ifrec exists not call this function
struct ifrec_entry * add_ifrec(struct mcft_entry *mcp, int ifindex, int proxy_vif, struct mCastProxyConfig *mCastProxyPara)
{
	struct ifrec_entry *if_rec = NULL;
	int ret;
	//printf("ifindex:%d.[%s]:[%d].\n",ifindex,__FUNCTION__,__LINE__);
	
#ifndef USE_STATIC_ENTRY_BUFFER 	
	if_rec = malloc(sizeof(struct ifrec_entry));
#else
	if_rec = find_ifrec_entry_from_tbl();
#endif	
	
	if (!if_rec) 
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "No enough space!\n", mcp->if_count);
		return NULL;
	}

	if_rec->ip_version = mCastProxyPara->ipVersion;
	memcpy(if_rec->grp_addr, mcp->grp_addr, ADDR_SIZE(mcp->ip_version));	
	if_rec->user_count=0;
	if_rec->grp_mbr = NULL;
	if_rec->ifindex = ifindex;
	
	if_rec->filter_mode=MCAST_INCLUDE;
	if_rec->srclist=NULL;
	if_rec->mrt_state=0;
	if_rec->flag = 0;
	if_rec->proxy_ver = (mcp->ip_version==IP_VERSION4)? IGMP_VER_3 : MLD_VER_2;
	if_rec->proxy_vif = proxy_vif;
	memset(&(if_rec->timer), 0, sizeof(struct proxy_timer));


	if_rec->next = mcp->iflist;
	mcp->if_count++;
	mcp->iflist = if_rec;
	
	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add new interface, mcp->if_count:%d!\n", mcp->if_count);
	
	return if_rec;
}

// remove if from group member list
// return: if count
int del_ifrec(struct mcft_entry *mcp,  int ifindex)
{
	struct ifrec_entry **q, *p;
	struct mbr_entry *gc,*gt;
	struct src_entry *s, *sn;

	if (mcp->if_count ==0){
		return 0;
	
	}

	q = &mcp->iflist;
	p = *q;

	while (p) 
	{
		if(p->ifindex==ifindex) 
		{
			*q = p->next;

			//free grp_mbr
			gc = p->grp_mbr;
			while (gc) {
				gt = gc->next;
			#if defined(USE_STATIC_ENTRY_BUFFER)
				del_mbr_entry_from_tbl(gc);
			#else
				free(gc);
			#endif
				gc = gt;
			}

			//free srclist
			s=p->srclist;
			while(s)
			{
				sn=s->next;
			#if defined(USE_STATIC_ENTRY_BUFFER)
				del_src_entry_from_tbl(s);
			#else
				free(s);
			#endif
				s=sn;
			}
			
			untimeout(&p->timer.ch);
		#ifdef USE_STATIC_ENTRY_BUFFER
			del_ifrec_entry_from_tbl(p);
		#else
			free(p);
		#endif
		
			mcp->if_count--;
			return mcp->if_count;
		}

		q = &p->next;
		p = p->next;
	}
	
	return mcp->if_count;
}


int check_src_set(unsigned int *src, struct src_entry *srclist, int ipVersion)
{
	struct src_entry *p;
	for(p=srclist; p!=NULL; p=p->next )
	{
		if(!memcmp(p->srcaddr, src, ADDR_SIZE(ipVersion)))
			return 1;
	}
	return 0;
}

int check_src_exclude_set(unsigned int *src, struct src_entry *srclist, int ipVersion)
{
	struct src_entry *p;
	for(p=srclist; p!=NULL; p=p->next )
	{
		if(!memcmp(p->srcaddr, src, ADDR_SIZE(ipVersion)) && p->timer.lefttime<=0)
			return 1;
	}
	return 0;
}

int check_src_requied_set(unsigned int *src, struct src_entry *srclist, int ipVersion)
{
	struct src_entry *p;
	for(p=srclist; p!=NULL; p=p->next )
	{
		if(!memcmp(p->srcaddr, src, ADDR_SIZE(ipVersion)) && p->timer.lefttime>0)
			return 1;
	}
	return 0;
}

int check_src(unsigned int *src, unsigned int *srcRec, int numsrc, int ipVersion)
{
	int i;
	unsigned int *srcp = srcRec;

	if(src==NULL || srcRec==NULL)
		return 0;

	for (i=0;i<numsrc; i++)
	{
		srcp = srcRec+i*ADDR_SIZE_INT(ipVersion);
		if(!memcmp(srcp, src, ADDR_SIZE(ipVersion)))
			return 1;
	}
	return 0;
}

struct src_entry *get_src_from_srclist(struct ifrec_entry *ifrec, int *srcAddr)
{
	struct src_entry *p;
	
	if(ifrec==NULL) 
		return NULL;

	p = ifrec->srclist;
	while (p) 
	{
		if(!memcmp(p->srcaddr, srcAddr, ADDR_SIZE(ifrec->ip_version)))
			return p;

		p = p->next;
	}

	return NULL;
}

struct src_entry *add_src_to_srclist(struct ifrec_entry *ifrec, int *srcAddr)
{
	struct src_entry *p;
	
	if(ifrec==NULL) 
		return NULL;

	p = ifrec->srclist;
	while (p) 
	{
		if(!memcmp(p->srcaddr, srcAddr, ADDR_SIZE(ifrec->ip_version)))
			return p;

		p = p->next;
	}

	//new src
#ifndef USE_STATIC_ENTRY_BUFFER 
	p = malloc(sizeof(struct src_entry));
#else
	p =find_src_entry_from_tbl();
#endif

	if (!p) 
	{
		return NULL;
	}
	
	memcpy(p->srcaddr, srcAddr, ADDR_SIZE(ifrec->ip_version));
	p->user_count[0] = 0;
	p->user_count[1] = 0;
	p->grp_mbr = NULL;
	memset(&(p->timer), 0, sizeof(struct proxy_timer));

	p->next = ifrec->srclist;
	ifrec->srclist = p;

	return p;
}
int del_src_from_srclist(struct ifrec_entry *ifrec, int *srcAddr)
{
	struct src_entry **q, *p;
	struct mbr_entry *mbrp, *mbrn;

	if(ifrec==NULL) 
		return  -1;
	
	q = &ifrec->srclist;
	p = *q;

	while (p) 
	{
		if(!memcmp(p->srcaddr, srcAddr, ADDR_SIZE(ifrec->ip_version)))
		{
			*q = p->next;

			//free user record
			mbrp = p->grp_mbr;
			while(mbrp)
			{
				mbrn = mbrp->next;
			#if defined(USE_STATIC_ENTRY_BUFFER)
				del_mbr_entry_from_tbl(mbrp);
			#else
				free(mbrp);
			#endif
				mbrp = mbrn;
			}
			
#ifndef USE_STATIC_ENTRY_BUFFER 
			free(p);
#else
			del_src_entry_from_tbl(p);
#endif
			return 0;
		}
		q = &p->next;
		p = p->next;
	}
	
	return 0;
}

#if defined(HOST_TRACKING_SUPPORT)
/*
	return: 	0<-> fail
			1<-> duplicate user
			2<-> add user successfully to ifrec 
*/
struct mbr_entry * add_user_to_ifrec(struct ifrec_entry *ifrec, unsigned int *src)
{
	struct mbr_entry *gcp = NULL;
	// check user
	gcp = ifrec->grp_mbr;
	while (gcp) 
	{
		if(!memcmp(gcp->user_addr, src, ADDR_SIZE(ifrec->ip_version)))
		{
			//gcp->timer.lefttime = ifrec->timer.lefttime;
			//gcp->timer.retry_left = ifrec->timer.retry_left;
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "user %s already listen to group:%s!\n", 
				addr_sprintf(src,ifrec->ip_version),
				addr_sprintf(ifrec->grp_addr,ifrec->ip_version));
			return gcp;
		}
		gcp = gcp->next;
	}
	
#ifndef USE_STATIC_ENTRY_BUFFER 	
	gcp = malloc(sizeof(struct mbr_entry));
#else
	gcp = find_mbr_entry_from_tbl();
#endif	
	if (!gcp) {
		
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add user %s to group:%s fail!\n", 
			addr_sprintf(src,ifrec->ip_version),
			addr_sprintf(ifrec->grp_addr,ifrec->ip_version));
		return NULL;
	}
	
	memcpy(gcp->user_addr, src, ADDR_SIZE(ifrec->ip_version));
	memset(&(gcp->timer), 0, sizeof(struct proxy_timer));
	
	gcp->next = ifrec->grp_mbr;
	ifrec->grp_mbr = gcp;
	ifrec->user_count++;

	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add user %s to group:%s successfully, user count:%d!\n", 
		addr_sprintf(gcp->user_addr,ifrec->ip_version),
		addr_sprintf(ifrec->grp_addr,ifrec->ip_version),
		ifrec->user_count);

	return gcp;
}

/* 
	return: 	user count
*/
int del_user_from_ifrec(struct ifrec_entry *ifrec, unsigned int *src)
{
	struct mbr_entry **q, *p;
	
	/* Remove the entry from the  list. */
	q = &ifrec->grp_mbr;
	p = *q;
	while (p) 
	{
		if(!memcmp(p->user_addr , src, ADDR_SIZE(ifrec->ip_version)))
		{
		
			*q = p->next;

			ifrec->user_count--;

			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "del user %s from group:%s, user count:%d!\n", 
				addr_sprintf(src,ifrec->ip_version),
				addr_sprintf(ifrec->grp_addr,ifrec->ip_version),
				ifrec->user_count);

#ifndef USE_STATIC_ENTRY_BUFFER 				
			free(p);
#else
			del_mbr_entry_from_tbl(p);
#endif			
			return ifrec->user_count;
		}
		q = &p->next;
		p = p->next;
	}
	
	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "user not listen to group:%s!\n", 
		addr_sprintf(ifrec->grp_addr,ifrec->ip_version));

	return ifrec->user_count;
}
/*
	return: 	
	0<-> fail
	1<-> duplicate user, update filter_mode
	2<-> add user successfully to ifrec 
*/
struct mbr_entry *add_user_to_srclist(struct src_entry *srclist, unsigned int *src, int ipVersion, int filter_mode)
{
	struct mbr_entry *gcp = NULL;
	
	// check user
	gcp = srclist->grp_mbr;
	while (gcp) 
	{
		if(!memcmp(gcp->user_addr, src, ADDR_SIZE(ipVersion)))
		{
			//gcp->timer.lefttime = srclist->timer.lefttime;
			//gcp->timer.retry_left = srclist->timer.retry_left;
			if(gcp->filter_mode==MCAST_EXCLUDE && filter_mode==MCAST_INCLUDE)
			{
				srclist->user_count[MCAST_EXCLUDE]--;
				srclist->user_count[MCAST_INCLUDE]++;
				gcp->filter_mode=MCAST_INCLUDE;
			}
			else if(gcp->filter_mode==MCAST_INCLUDE && filter_mode==MCAST_EXCLUDE)
			{
				srclist->user_count[MCAST_INCLUDE]--;
				srclist->user_count[MCAST_EXCLUDE]++;
				gcp->filter_mode=MCAST_EXCLUDE;
			}
			
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "user %s already listen to src:%s, IN user count:%d, EX user count:%d!\n", 
				addr_sprintf(src,ipVersion),
				addr_sprintf(srclist->srcaddr,ipVersion),
				srclist->user_count[MCAST_INCLUDE],
				srclist->user_count[MCAST_EXCLUDE]);

			return gcp;
		}
		gcp = gcp->next;
	}
	
#ifndef USE_STATIC_ENTRY_BUFFER 	
	gcp = malloc(sizeof(struct mbr_entry));
#else
	gcp = find_mbr_entry_from_tbl();
#endif	
	if (!gcp) 
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add user %s to src:%s fail!\n", 
			addr_sprintf(src,ipVersion),
			addr_sprintf(srclist->srcaddr,ipVersion));
		return NULL;
	}
	
	memcpy(gcp->user_addr, src, ADDR_SIZE(ipVersion));
	memset(&(gcp->timer), 0, sizeof(struct proxy_timer));
	gcp->next = srclist->grp_mbr;
	gcp->filter_mode = filter_mode;
	srclist->grp_mbr = gcp;
	srclist->user_count[filter_mode]++;

	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add user %s to src:%s success, IN user count:%d, EX user count:%d!\n", 
		addr_sprintf(src,ipVersion),
		addr_sprintf(srclist->srcaddr,ipVersion),
		srclist->user_count[MCAST_INCLUDE],
		srclist->user_count[MCAST_EXCLUDE]);

	return gcp;
}

int del_user_from_srclist(struct src_entry *srclist, unsigned int *src, int ipVersion)
{
	struct mbr_entry **q, *p;

	/* Remove the entry from the  list. */
	q = &srclist->grp_mbr;
	p = *q;
	while (p) 
	{
		if(!memcmp(p->user_addr , src, ADDR_SIZE(ipVersion)))
		{
		
			*q = p->next;

			srclist->user_count[p->filter_mode]--;
			
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "del user %s from src:%s success, IN user count:%d, EX user count:%d!\n", 
				addr_sprintf(src,ipVersion),
				addr_sprintf(srclist->srcaddr,ipVersion),
				srclist->user_count[MCAST_INCLUDE],
				srclist->user_count[MCAST_EXCLUDE]);
#ifndef USE_STATIC_ENTRY_BUFFER 				
			free(p);
#else
			del_mbr_entry_from_tbl(p);
#endif	
			return 1;
		}
		q = &p->next;
		p = p->next;
	}
	
	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "user not listen to src:%s success, IN user count:%d, EX user count:%d!\n", 
		addr_sprintf(srclist->srcaddr,ipVersion),
		srclist->user_count[MCAST_INCLUDE],
		srclist->user_count[MCAST_EXCLUDE]);

	return 0;
}

int get_user_from_ifrec(struct ifrec_entry *ifrec, unsigned int *src)
{
	struct mbr_entry *gcp = NULL;
	gcp = ifrec->grp_mbr;
	while (gcp) 
	{
		if(!memcmp(gcp->user_addr, src, ADDR_SIZE(ifrec->ip_version)))
		{
			return gcp;
		}
		gcp = gcp->next;
	}

	return NULL;
}
int get_user_from_srclist(struct src_entry *srclist, unsigned int *src, int ipVersion)
{
	struct mbr_entry *gcp = NULL;
	
	// check user
	gcp = srclist->grp_mbr;
	while (gcp) 
	{
		if(!memcmp(gcp->user_addr, src, ADDR_SIZE(ipVersion)))
		{
			return gcp;
		}
		gcp = gcp->next;
	}
	return NULL;
}
int check_no_user_src(struct src_entry *srclist, unsigned int *src, int ipVersion)
{
	struct mbr_entry *gcp = NULL;
	
	gcp = srclist->grp_mbr;
	while (gcp) 
	{
		if(memcmp(gcp->user_addr, src, ADDR_SIZE(ipVersion)))
		{
			//other user_addr exit;
			return 0;
		}
		gcp = gcp->next;
	}

	return 1;
}
int check_no_user_ifrec(struct ifrec_entry *ifrec, unsigned int *src)
{
	struct mbr_entry *gcp = NULL;
	// check user
	gcp = ifrec->grp_mbr;
	while (gcp) 
	{
		if(memcmp(gcp->user_addr, src, ADDR_SIZE(ifrec->ip_version)))
		{
			return 0;
			break;
		}
		gcp = gcp->next;
	}

	return 1;
}
#endif


#if 0
inline int calc_qqic(short qqi)
{
	unsigned char exp=0, mant=0, qqic = 0;
	if(qqi<=0)
		qqic = 0;
	else if(qqi<128)
		qqic = qqi;
	else
	{
		mant = qqi>>3;
		while(mant>31)
		{
			exp++;
			mant = qqi>>(1+exp);
			break;
		}
		mant -= 16;
		qqic = 0x80+(exp<<4)+mant;
	}

	return qqic;
}
#endif
unsigned short calc_interval_code(unsigned int interval, unsigned short th, int exp_len, int mant_len)
{
	unsigned char exp=0, mant=0, code = 0;
	if(interval<=0)
		code = 0;
	else if(interval<th)
		code = interval;
	else
	{
		mant = interval>>exp_len;
		while(mant>((1<<(mant_len+1))-1))
		{
			exp++;
			mant = interval>>(1+exp);
			break;
		}
		mant -= (1<<mant_len);
		code = th+(exp<<mant_len)+mant;
	}

	return code;
}
unsigned short calc_qqic(unsigned int qqi, int ipVesion)
{
	return calc_interval_code(qqi, 0x80, 3, 4);
}

unsigned short calc_mrc(unsigned int mrd, int ipVersion)
{
	return (ipVersion==IP_VERSION4)? calc_interval_code(mrd, 0x80, 3, 4): calc_interval_code(mrd, 0x8000, 3, 12);
}

int send_query(int proxyVersion, unsigned int *dst, unsigned int *grp, unsigned short mrt, unsigned char qqic, unsigned char qrv, unsigned short srcnum, unsigned int *srcfilter, int ipVersion)
{

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "dst:%s, group:%s, mrt:%d, qqic:%d, srcNum:%d!\n", 
		addr_sprintf(dst,ipVersion),addr_sprintf(grp,ipVersion), mrt, qqic, srcnum);

#if defined(CONFIG_IGMPV3_SUPPORT)
	if(ipVersion==IP_VERSION4)
	{
		igmp_send_query(proxyVersion, dst[0], grp[0], mrt, qqic, qrv, srcnum, srcfilter);
	}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	if(ipVersion==IP_VERSION6)
	{
		struct in6_addr grp_addr, dst_addr;
		memcpy(&grp_addr, grp, ADDR_SIZE(ipVersion));
		memcpy(&dst_addr, dst, ADDR_SIZE(ipVersion));
		mld_send_query(proxyVersion, dst_addr, grp_addr, mrt, qqic, qrv, srcnum, srcfilter);
	}
#endif
}

void general_query_timer_expired(void *arg)
{
	struct query_timer  *timerPtr = arg;
	unsigned int mrd = timerPtr->maxDelay;  //
	unsigned int qqi = timerPtr->timerInterval;
	unsigned int qrv = timerPtr->robustValue;
	unsigned short qqic = 0, mrc = 0;
	
	if(timerPtr!=NULL)
	{	
		if((timerPtr->type&PERIODICAL_TIMER_TPYE))
		{
		#if defined(CONFIG_IGMPV3_SUPPORT)
			if(timerPtr->type&IGMP_TIMER)
			{
				qqic = calc_qqic(qqi, IP_VERSION4);
				mrc = calc_mrc(mrd,IP_VERSION4);
				
				if((timerPtr->type&IGMP_TIMER_V1)||(timerPtr->type&IGMP_TIMER_V2))
					igmp_send_query(IGMP_VER_2, ALL_SYSTEMS, 0, mrc, 0, qrv, 0, NULL);
				else
					igmp_send_query(IGMP_VER_3, ALL_SYSTEMS, 0, mrc, qqic, qrv, 0, NULL);
			}
		#endif
		#if defined(CONFIG_MLDV2_SUPPORT)
			if(timerPtr->type&MLD_TIMER)
			{
			
				qqic = calc_qqic(qqi, IP_VERSION6);
				mrc = calc_mrc(mrd,IP_VERSION6);

				if(timerPtr->type&MLD_TIMER_V1)
					mld_send_query(MLD_VER_1, MLD_ALL_HOSTS_ADDRESS, MLD_UNSPECIFIED_ADDRESS, mrc, 0, qrv, 0, NULL);
				else
					mld_send_query(MLD_VER_2, MLD_ALL_HOSTS_ADDRESS, MLD_UNSPECIFIED_ADDRESS, mrc, qqic, qrv, 0, NULL);
			}
		#endif

			if(newGeneralQueryInterval&&(timerPtr==(&generalQueryTimer)))
			{
				generalQueryTimer.timerInterval = newGeneralQueryInterval;
				newGeneralQueryInterval = 0;
			}	
			timeout(general_query_timer_expired , timerPtr, timerPtr->timerInterval, &timerPtr->ch);
		}
		else
		{
			if(timerPtr->retry_left==0) 
			{
				untimeout(&timerPtr->ch);
			}
			else 
			{			
#if defined(CONFIG_IGMPV3_SUPPORT)
				if(timerPtr->type&IGMP_TIMER)
				{
					mrc = calc_mrc(mrd,IP_VERSION4);
					if((timerPtr->type&IGMP_TIMER_V1)||(timerPtr->type&IGMP_TIMER_V2))
						igmp_send_query(IGMP_VER_2, ALL_SYSTEMS, 0, mrc, 0, qrv, 0, NULL);
					else
						igmp_send_query(IGMP_VER_3, ALL_SYSTEMS, 0, mrc, 0, qrv, 0, NULL);
				}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
				if(timerPtr->type&MLD_TIMER)
				{
					mrc = calc_mrc(mrd,IP_VERSION6);
					if(timerPtr->type&MLD_TIMER_V1)
						mld_send_query(MLD_VER_1, MLD_ALL_HOSTS_ADDRESS, MLD_UNSPECIFIED_ADDRESS, mrc, 0, qrv, 0, NULL);
					else
						mld_send_query(MLD_VER_2, MLD_ALL_HOSTS_ADDRESS, MLD_UNSPECIFIED_ADDRESS, mrc, 0, qrv, 0, NULL);
				}
#endif
				timerPtr->retry_left--;
				timeout(general_query_timer_expired , timerPtr, timerPtr->timerInterval, &timerPtr->ch);
			}
		}

	}
}

void init_general_query(struct mCastProxyConfig *mCastProxyPara)
{
	if((mCastProxyPara->ipVersion!=IP_VERSION4) && (mCastProxyPara->ipVersion!=IP_VERSION6))
		return;

	startupQueryTimer.type=LIMIT_RETRY_TIMER_TYPE;
	if(mCastProxyPara->ipVersion==IP_VERSION4)
		startupQueryTimer.type |= IGMP_VER_TO_TIMER_VER(mCastProxyPara->forceVersion);
	else
		startupQueryTimer.type |= MLD_VER_TO_TIMER_VER(mCastProxyPara->forceVersion);
	
	startupQueryTimer.retry_left = mCastProxyPara->timeConfig.setUpGeneralQueryCount;
	startupQueryTimer.timerInterval = mCastProxyPara->timeConfig.setUpGeneralQueryInterval;
	startupQueryTimer.maxDelay = mCastProxyPara->timeConfig.QueryResponseInterval*mCastProxyPara->timeConfig.intervalScale;
	startupQueryTimer.robustValue = mCastProxyPara->timeConfig.robustness;
	timeout(general_query_timer_expired , &startupQueryTimer, startupQueryTimer.timerInterval, &(startupQueryTimer.ch));
	
	/*schedule periodical general query*/
	generalQueryTimer.type=PERIODICAL_TIMER_TPYE;
	if(mCastProxyPara->ipVersion==IP_VERSION4)
		generalQueryTimer.type |= IGMP_VER_TO_TIMER_VER(mCastProxyPara->forceVersion);
	else
		generalQueryTimer.type |= MLD_VER_TO_TIMER_VER(mCastProxyPara->forceVersion);
	generalQueryTimer.retry_left= mCastProxyPara->timeConfig.periodGeneralQueryCount;
	generalQueryTimer.timerInterval = mCastProxyPara->timeConfig.periodGeneralQueryInterval;
	generalQueryTimer.maxDelay = mCastProxyPara->timeConfig.QueryResponseInterval*mCastProxyPara->timeConfig.intervalScale;
	generalQueryTimer.robustValue = mCastProxyPara->timeConfig.robustness;
	timeout(general_query_timer_expired , &generalQueryTimer,  generalQueryTimer.timerInterval, &(generalQueryTimer.ch));

}
void set_link_change_query(struct mCastProxyConfig *mCastProxyPara)
{	
	linkChangeQueryTimer.type=LIMIT_RETRY_TIMER_TYPE;
	if((mCastProxyPara->ipVersion!=IP_VERSION4) && (mCastProxyPara->ipVersion!=IP_VERSION6))
		return;
	
	if(mCastProxyPara->ipVersion==IP_VERSION4)
		linkChangeQueryTimer.type |= IGMP_VER_TO_TIMER_VER(mCastProxyPara->forceVersion);
	else
		linkChangeQueryTimer.type |= MLD_VER_TO_TIMER_VER(mCastProxyPara->forceVersion);
	
	linkChangeQueryTimer.retry_left=mCastProxyPara->timeConfig.linkChangeQueryCount;
	linkChangeQueryTimer.timerInterval=mCastProxyPara->timeConfig.linkChangeQueryInterval;
	linkChangeQueryTimer.maxDelay = mCastProxyPara->timeConfig.QueryResponseInterval*mCastProxyPara->timeConfig.intervalScale;
	linkChangeQueryTimer.robustValue = mCastProxyPara->timeConfig.robustness;
	timeout(general_query_timer_expired , &linkChangeQueryTimer, linkChangeQueryTimer.timerInterval, &(linkChangeQueryTimer.ch));
}

void send_setup_general_query(struct mCastProxyConfig *mCastProxyPara)
{
	int ver = mCastProxyPara->forceVersion;
	//int qqic = calc_qqic(mCastProxyPara->timeConfig.setUpGeneralQueryInterval);
	int mrc = calc_mrc(mCastProxyPara->timeConfig.QueryResponseInterval*mCastProxyPara->timeConfig.intervalScale, mCastProxyPara->ipVersion);

#if defined(CONFIG_IGMPV3_SUPPORT)
	if(mCastProxyPara->ipVersion==IP_VERSION4)
	{
		//if ver!=IGMP_VER_3, qqic will be ignored
		igmp_send_query(ver, ALL_SYSTEMS, 0, mrc, 0, mCastProxyPara->timeConfig.robustness, 0, NULL);
	}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	if(mCastProxyPara->ipVersion==IP_VERSION6)
	{
		//if ver!=MLD_VER_2, qqic will be ignored
		mld_send_query(ver, MLD_ALL_HOSTS_ADDRESS, MLD_UNSPECIFIED_ADDRESS, mrc, 0, mCastProxyPara->timeConfig.robustness, 0, NULL);
	}
#endif
	
}

int handle_group_timer(struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList)
{
	struct mcft_entry *mcfp=NULL, *m_next=NULL;
	struct ifrec_entry *ifp=NULL, *f_next=NULL;
	int mrc;
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)
	mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,mCastProxyPara->ipVersion);
#else
	mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,mCastProxyPara->ipVersion);
#endif
	mcfp = *groupList;
	while(mcfp)
	{
		m_next = mcfp->next;
		ifp = mcfp->iflist;
		while(ifp)
		{
			f_next = ifp->next;
			if(ifp->timer.lefttime)
			{
				ifp->timer.lefttime--;
				if((ifp->timer.lefttime==0) && (ifp->timer.retry_left!=0))
				{
					ifp->timer.retry_left--;
					if(ifp->timer.retry_left)
					{
						DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Group %s TIMEOUT, send query!\n", addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion));

						ifp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
						#if defined(CONFIG_IGMPV3_SUPPORT)
						if(mCastProxyPara->ipVersion==IP_VERSION4)
						{
							igmp_send_query(mCastProxyPara->forceVersion, ALL_SYSTEMS, ifp->grp_addr[0], mrc, 0, mCastProxyPara->timeConfig.robustness, 0, NULL);
						}
						#endif
						#if defined(CONFIG_MLDV2_SUPPORT)
						if(mCastProxyPara->ipVersion==IP_VERSION6)
						{
							struct in6_addr group;
							group.s6_addr32[0] =  ifp->grp_addr[0];
							group.s6_addr32[1] =  ifp->grp_addr[1];
							group.s6_addr32[2] =  ifp->grp_addr[2];
							group.s6_addr32[3] =  ifp->grp_addr[3];
							mld_send_query(mCastProxyPara->forceVersion, MLD_ALL_HOSTS_ADDRESS, group, mrc, 0, mCastProxyPara->timeConfig.robustness, 0, NULL);
						}
						#endif
					}
					
				}

				switch(ifp->filter_mode)
				{
					case MCAST_INCLUDE:
						break;
					case MCAST_EXCLUDE:
						if(ifp->timer.lefttime==0)
						{
							//lefttime==0, retry time=0
							struct src_entry *s, *s_next;
							
							DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Group %s in EXCLUDE mode TIMEOUT!\n", addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion));
							
							s=ifp->srclist;
							while(s)
							{
								s_next=s->next;
								if(s->timer.lefttime==0)
								{
									//remove this source, src !=0, it is impossible to have two mfc entries with same src group but differnet in_vif;
									del_mfc(ifp->grp_addr, s->srcaddr, ifp->proxy_vif,  mCastProxyPara->ipVersion);
									del_src_from_srclist(ifp, s->srcaddr);
								}
								s=s_next;
							}
							
							if(ifp->srclist)
							{
							
								DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Change Group %s to INCLUDE mode!\n", addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion));
								del_mfc(ifp->grp_addr, ZERO_ADDR, ifp->proxy_vif, mCastProxyPara->ipVersion);
								ifp->filter_mode=MCAST_INCLUDE;
								set_srcfilter(ifp, ifp->proxy_vif);
							}
							else
							{					
								DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Remove Group %s!\n", addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion));
								//delete this group record
								del_mfc(ifp->grp_addr, ZERO_ADDR, ifp->proxy_vif, mCastProxyPara->ipVersion);
								del_membership(ifp->grp_addr, ifp->proxy_vif, mCastProxyPara->ipVersion);
								del_ifrec(mcfp,ifp->ifindex);
							}
						}
						break;
					default:
						break;
				}				
			}
			ifp = f_next;
		}
		
		if(mcfp->iflist==NULL)
			del_mcft(mcfp->grp_addr, mCastProxyPara, groupList);
		mcfp = m_next;
	}

	return SUCCESS;
}

/*handle group timer firstly.
   impossible status before handle src timer: INCLUDE(A), EXCLUDE(NULL,Y), EXCLUDE(X,Y)*/
int handle_src_timer(struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList)
{
	struct mcft_entry *mcfp=NULL, *m_next=NULL;
	struct ifrec_entry *ifp=NULL, *f_next=NULL;
	struct src_entry *s=NULL, *s_next;
	int mrc=0, change_sf=0;

	//DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "FUNCTION BEGIN");
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
	mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,mCastProxyPara->ipVersion);
#else
	mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,mCastProxyPara->ipVersion);
#endif
	
	mcfp = *groupList;
	while(mcfp)
	{		
		m_next = mcfp->next;
		ifp = mcfp->iflist;
		while(ifp)
		{
			f_next = ifp->next;
			s = ifp->srclist;
			while(s)
			{
				s_next = s->next;
				
				if( s->timer.lefttime)
				{
					s->timer.lefttime--;
					if( (s->timer.lefttime==0) && (s->timer.retry_left!=0))
					{
						s->timer.retry_left--;
						if(s->timer.retry_left)
						{
							DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Group:%s Src:%s TIMEOUT, send query!\n", 
								addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion),
								addr_sprintf(s->srcaddr, mCastProxyPara->ipVersion));

							s->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
							send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr,mrc,0,mCastProxyPara->timeConfig.robustness,1,s->srcaddr, mCastProxyPara->ipVersion);
						}						
					}
					
						
					switch(ifp->filter_mode)
					{
						case MCAST_INCLUDE:
							if(s->timer.lefttime)
							{
							#if 0
								//forward this src
								if(p->mrt_state)
								{
									IGMPV3LOG("%s> stop all sources for group:%s\n", __FUNCTION__, inet_ntoa(p->grp_addr) );
									igmp_del_mr( p->grp_addr, 0 );
									p->mrt_state=0;
								}
							#endif
							}
							else
							{
								DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Remove Src:%s from Group:%s!\n", 
									addr_sprintf(s->srcaddr, mCastProxyPara->ipVersion),
									addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion));

								//==0, stop this src
								del_mfc(ifp->grp_addr, s->srcaddr, ifp->proxy_vif, mCastProxyPara->ipVersion);
								del_src_from_srclist(ifp,s->srcaddr);

								change_sf=1;
							}
							break;
						case MCAST_EXCLUDE:
						#if 0
							if(s->timer.lefttime)
							{
								//forward this src
								if(p->mrt_state)
								{
									IGMPV3LOG("%s> stop all sources for group:%s\n", __FUNCTION__, inet_ntoa(p->grp_addr) );
									igmp_del_mr( p->grp_addr, 0 );
									p->mrt_state=0;
								}
							}else{
								IGMPV3LOG("%s> stop forwarding src:%s", __FUNCTION__, inet_ntoa(s->srcaddr) );
								IGMPV3LOG(" for group:%s\n", __FUNCTION__, inet_ntoa(p->grp_addr) );

								//==0, stop this src, do not remove record
								igmp_add_mr( p->grp_addr, s->srcaddr, 0 );
								change_sf=1;
							}
						#else
							if(s->timer.lefttime==0)
							{
							
								DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Remove Src:%s to EXCLUDE list, Group:%s!\n", 
									addr_sprintf(s->srcaddr, mCastProxyPara->ipVersion),
									addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion));

								//==0, stop this src, do not remove record
								add_mfc(ifp->grp_addr,s->srcaddr,0,ifp->proxy_vif, ifp->ifindex, mCastProxyPara->ipVersion);
								change_sf=1;
							}
						#endif
							break;
						default:
							break;
					}
				}				
				s = s_next;
			}
			
			//set the new state
			if(change_sf)
			{
				set_srcfilter(ifp, ifp->proxy_vif);

				switch(ifp->filter_mode)
				{
					case MCAST_INCLUDE:
					{
						if(ifp->srclist==NULL)
						{
							DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Delete Group:%s on vif %d!\n",
								addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion), ifp->ifindex);
							del_membership(ifp->grp_addr,ifp->proxy_vif,mCastProxyPara->ipVersion);
							del_ifrec(mcfp,ifp->ifindex);
						}
						break;
					}
					case MCAST_EXCLUDE:
						break;
					default:
						break;
				}
			}
			
			ifp = f_next;
		}
		if(mcfp->iflist==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "None vif listen to Group:%s, DELETE!\n",
				addr_sprintf(mcfp->grp_addr, mCastProxyPara->ipVersion));
			del_mcft(mcfp->grp_addr,mCastProxyPara,groupList);
		}
		mcfp = m_next;
	}
	return SUCCESS;
}
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
int reset_compatible_status(void)
{
	int i=0;
	for(i=0; i++; i<MAX_IFREC_ENTRY)
	{
		compatibilityStatus[i] = COMPATIBILITY_IGMPV3_MLDV2;
		igmpV2MldV1Timer[i].lefttime= 0;
		igmpV1Timer[i].lefttime= 0;
	}
}

int handle_compatibility_timer(struct mCastProxyConfig *mCastProxyPara)
{
	int i=0;
	
	if(mCastProxyPara->compatibleEnbale)
	{
		for(i=0; i<MAX_IFREC_ENTRY; i++)
		{
			switch(compatibilityStatus[i])
			{
				case COMPATIBILITY_IGMPV1:
					if(igmpV1Timer[i].lefttime>0)
						igmpV1Timer[i].lefttime--;
					if(igmpV2MldV1Timer[i].lefttime>0)
						igmpV2MldV1Timer[i].lefttime--;

					if(igmpV1Timer[i].lefttime==0)
					{
						if(igmpV2MldV1Timer[i].lefttime==0)
							compatibilityStatus[i]=COMPATIBILITY_IGMPV3_MLDV2;
						else
							compatibilityStatus[i]=COMPATIBILITY_IGMPV2_MLDV1;
					#if defined(DBG_IGMP_MLD)
						show_compatibility_status(mCastProxyPara);
					#endif
					}
					break;
				case COMPATIBILITY_IGMPV2_MLDV1:
					if(igmpV2MldV1Timer[i].lefttime>0)
						igmpV2MldV1Timer[i].lefttime--;

					if(igmpV2MldV1Timer[i].lefttime==0)
					{
						compatibilityStatus[i]=COMPATIBILITY_IGMPV3_MLDV2;					
					#if defined(DBG_IGMP_MLD)
						show_compatibility_status(mCastProxyPara);
					#endif
					}
					break;
				case COMPATIBILITY_IGMPV3_MLDV2:
					break;
				default:
					break;
			}
		}
	}
	return SUCCESS;
}
#endif
#if defined(HOST_TRACKING_SUPPORT)
int handle_host_timer(struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList)
{
	struct mcft_entry *mcfp=NULL, *m_next=NULL;
	struct ifrec_entry *ifp=NULL, *f_next=NULL;
	struct src_entry *s=NULL, *s_next;
	struct mbr_entry *mbrp=NULL, *mbr_next;
	int mrc = 0;
	int need_query_group = 0, change_sf = 0;

#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
	mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,mCastProxyPara->ipVersion);
#else
	mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,mCastProxyPara->ipVersion);
#endif
	mcfp = *groupList;
	while(mcfp)
	{		
		m_next = mcfp->next;
		ifp = mcfp->iflist;
		while(ifp)
		{
			f_next = ifp->next;
			if(ifp->user_count>0)
			{
				mbrp = ifp->grp_mbr;
				while(mbrp)
				{
					mbr_next = mbrp->next;
					if(mbrp->timer.lefttime>0)
					{
						mbrp->timer.lefttime--;
						if((mbrp->timer.lefttime==0)&&(mbrp->timer.retry_left>0))
						{
							mbrp->timer.retry_left--;
							if(mbrp->timer.retry_left>0)
							{
								mbrp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
								need_query_group = 1;
							}
						}

						if(mbrp->timer.lefttime==0)
						{
							del_user_from_ifrec(ifp,mbrp->user_addr);
							change_sf = 1;
						}
					}
					mbrp = mbr_next;
				}
			}

			s = ifp->srclist;
			while(s)
			{
				s_next = s->next;
				if(s->user_count[MCAST_EXCLUDE]>0 || s->user_count[MCAST_INCLUDE]>0)
				{
					mbrp = s->grp_mbr;
					while(mbrp)
					{
						mbr_next = mbrp->next;
						if(mbrp->timer.lefttime>0)
						{
							mbrp->timer.lefttime--;
							if((mbrp->timer.lefttime==0)&&(mbrp->timer.retry_left>0))
							{
								mbrp->timer.retry_left--;
								if(mbrp->timer.retry_left>0)
								{
									mbrp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
									need_query_group = 1;
								}
							}
					
							if(mbrp->timer.lefttime==0)
							{
								del_user_from_srclist(s,mbrp->user_addr,mCastProxyPara->ipVersion);
								change_sf = 1;
							}
						}
						mbrp = mbr_next;
					}

				}
				
				if((s->user_count[MCAST_EXCLUDE]==0)&&(s->user_count[MCAST_INCLUDE]==0))
				{
					del_mfc(ifp->grp_addr,s->srcaddr,ifp->proxy_vif,ifp->ip_version);
					del_src_from_srclist(ifp,s->srcaddr);
					change_sf = 1;
				}
				
				s = s_next;
			}
			
			if(need_query_group)
				send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr, mrc, 0, mCastProxyPara->timeConfig.robustness, 0, NULL, ifp->ip_version);

			if(ifp->user_count==0&&ifp->srclist==NULL)
			{
				DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Delete Group:%s on vif %d!\n",
					addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion), ifp->ifindex);
				del_mfc(ifp->grp_addr, ZERO_ADDR, ifp->proxy_vif, mCastProxyPara->ipVersion);
				del_membership(ifp->grp_addr,ifp->proxy_vif,mCastProxyPara->ipVersion);
				del_ifrec(mcfp,ifp->ifindex);
			}
			else
			{
				if(change_sf)
				{
					set_srcfilter(ifp,ifp->proxy_vif);
					update_mfc(ifp);
				}
			}
				
			ifp = f_next;
		}
		if(mcfp->iflist==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "None vif listen to Group:%s, DELETE!\n",
				addr_sprintf(mcfp->grp_addr, mCastProxyPara->ipVersion));
			del_mcft(mcfp->grp_addr,mCastProxyPara,groupList);
		}
		mcfp = m_next;
	}
	return SUCCESS;
}

#endif
int check_group_validity(unsigned int *groupAddr, int ipVersion)
{
	unsigned int group[4]={0};
	
#if defined(CONFIG_IGMPV3_SUPPORT)
	if(ipVersion==IP_VERSION4)
	{
		group[0] = ntohl(groupAddr[0]);
		if(!IN_MULTICAST(group[0]))
			return FALSE;

		if(groupAddr[0]==ALL_PRINTER)	/* It's MS-Windows UPNP all printers notify */
			return FALSE;

		if(IN_MULTICAST_RESV1(group[0]))
			return FALSE;
		
	}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	if(ipVersion==IP_VERSION6)
	{
		group[0] = ntohl(groupAddr[0]);
		group[1] = ntohl(groupAddr[1]);
		group[2] = ntohl(groupAddr[2]);
		group[3] = ntohl(groupAddr[3]);
		if(!IS_IPV6_MULTICAST(group))
			return FALSE;
		
		if(IS_IPV6_RESVER_0_ADDRESS(group))
			return FALSE;
	}
#endif

	return TRUE;
}
int add_membership(unsigned int *groupAddr, int vif, int ipVersion)
{
#if defined(CONFIG_IGMPV3_SUPPORT)
	if(ipVersion==IP_VERSION4)
	{
		return igmp_add_group(groupAddr[0], vif);
	}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	if(ipVersion==IP_VERSION6)
	{
		struct in6_addr group;
		group.s6_addr32[0] = groupAddr[0];
		group.s6_addr32[1] = groupAddr[1];
		group.s6_addr32[2] = groupAddr[2];
		group.s6_addr32[3] = groupAddr[3];
		return mld_add_group(group, vif);
	}
#endif
}
int del_membership(unsigned int *groupAddr, int vif, int ipVersion)
{
#if defined(CONFIG_IGMPV3_SUPPORT)
	if(ipVersion==IP_VERSION4)
	{
		return igmp_del_group(groupAddr[0], vif);
	}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	if(ipVersion==IP_VERSION6)
	{
		struct in6_addr group;
		group.s6_addr32[0] = groupAddr[0];
		group.s6_addr32[1] = groupAddr[1];
		group.s6_addr32[2] = groupAddr[2];
		group.s6_addr32[3] = groupAddr[3];
		return mld_del_group(group, vif);
	}
#endif
}
#if 0
int add_addr_to_list(unsigned int *addr, unsigned int *addrList, int ipVersion)
{
#if defined(CONFIG_IGMPV3_SUPPORT)
	if(ipVersion==IP_VERSION4)
	{
		struct in_addr * igmpSrcList = (struct in_addr *)addrList;
		igmpSrcList->s_addr = addr[0];
	}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	if(ipVersion==IP_VERSION6)
	{
		struct in6_addr * mldSrcList = (struct in6_addr *)addrList;
		mldSrcList->s6_addr32[0] = addr[0];
		mldSrcList->s6_addr32[1] = addr[1];
		mldSrcList->s6_addr32[2] = addr[2];
		mldSrcList->s6_addr32[3] = addr[3];
	}
#endif
}
#endif
#if defined(HOST_TRACKING_SUPPORT)
int set_srcfilter(struct ifrec_entry *ifp, int vif)
{
	int inSrcNum=0, exSrcNum=0, miscSrcNum=0;
	struct src_entry *s = NULL;
#if defined(CONFIG_IGMPV3_SUPPORT)
	unsigned int igmpInSrcListTmp[MAX_SRC_ENTRY]={0};
	unsigned int igmpExSrcListTmp[MAX_SRC_ENTRY]={0};
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	struct in6_addr mldInSrcListTmp[MAX_SRC_ENTRY];
	struct in6_addr mldExSrcListTmp[MAX_SRC_ENTRY];	
#if defined(CONFIG_MLDV2_SUPPORT)
	struct in6_addr group;
#endif
#endif

	if(ifp==NULL)
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "ERROR input parameter!\n");
		return FAILED;
	}

#if defined(CONFIG_MLDV2_SUPPORT)
	memset(mldInSrcListTmp, 0, MAX_SRC_ENTRY*sizeof(struct in6_addr));
	memset(mldExSrcListTmp, 0, MAX_SRC_ENTRY*sizeof(struct in6_addr));
#endif
	
	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "group:%s!\n", addr_sprintf(ifp->grp_addr, ifp->ip_version));

	if(ifp->user_count>0)
	{
		//EXClUDE NULL
		ifp->filter_mode = MCAST_EXCLUDE;
#if defined(CONFIG_IGMPV3_SUPPORT)
		igmp_set_srcfilter(ifp->grp_addr[0], MCAST_EXCLUDE, 0, NULL, vif);
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
		group.s6_addr32[0] = ifp->grp_addr[0];
		group.s6_addr32[1] = ifp->grp_addr[1];
		group.s6_addr32[2] = ifp->grp_addr[2];
		group.s6_addr32[3] = ifp->grp_addr[3];
		mld_set_srcfilter(group, MCAST_EXCLUDE, 0, NULL, vif);
#endif
	}
	else
	{
		s = ifp->srclist;
		while(s)
		{
			if(s->user_count[MCAST_EXCLUDE]==0)
			{
			#if defined(CONFIG_IGMPV3_SUPPORT)
				igmpInSrcListTmp[inSrcNum] = s->srcaddr[0];

			#endif
			#if defined(CONFIG_MLDV2_SUPPORT)
				mldInSrcListTmp[inSrcNum].s6_addr32[0] = s->srcaddr[0];
				mldInSrcListTmp[inSrcNum].s6_addr32[1] = s->srcaddr[1];
				mldInSrcListTmp[inSrcNum].s6_addr32[2] = s->srcaddr[2];
				mldInSrcListTmp[inSrcNum].s6_addr32[3] = s->srcaddr[3];
			#endif
				inSrcNum++;
			}
			else if(s->user_count[MCAST_INCLUDE]==0)
			{
			#if defined(CONFIG_IGMPV3_SUPPORT)
				igmpExSrcListTmp[exSrcNum] = s->srcaddr[0];

			#endif
			#if defined(CONFIG_MLDV2_SUPPORT)
				mldExSrcListTmp[exSrcNum].s6_addr32[0] = s->srcaddr[0];
				mldExSrcListTmp[exSrcNum].s6_addr32[1] = s->srcaddr[1];
				mldExSrcListTmp[exSrcNum].s6_addr32[2] = s->srcaddr[2];
				mldExSrcListTmp[exSrcNum].s6_addr32[3] = s->srcaddr[3];
			#endif
				exSrcNum++;
			}
			else
			{
				miscSrcNum++;
			}
			s = s->next;
		}
		if((exSrcNum+miscSrcNum)>0)
		{
			ifp->filter_mode = MCAST_EXCLUDE;
		#if defined(CONFIG_IGMPV3_SUPPORT)
			igmp_set_srcfilter(ifp->grp_addr[0], MCAST_EXCLUDE, exSrcNum, igmpExSrcListTmp, vif);
		#endif
		#if defined(CONFIG_MLDV2_SUPPORT)
			group.s6_addr32[0] = ifp->grp_addr[0];
			group.s6_addr32[1] = ifp->grp_addr[1];
			group.s6_addr32[2] = ifp->grp_addr[2];
			group.s6_addr32[3] = ifp->grp_addr[3];
			mld_set_srcfilter(group, MCAST_EXCLUDE, exSrcNum, mldExSrcListTmp, vif);
		#endif
		}
		else
		{
			ifp->filter_mode = MCAST_INCLUDE;
		#if defined(CONFIG_IGMPV3_SUPPORT)
			igmp_set_srcfilter(ifp->grp_addr[0], MCAST_INCLUDE, inSrcNum, igmpInSrcListTmp, vif);
		#endif
		#if defined(CONFIG_MLDV2_SUPPORT)
			group.s6_addr32[0] = ifp->grp_addr[0];
			group.s6_addr32[1] = ifp->grp_addr[1];
			group.s6_addr32[2] = ifp->grp_addr[2];
			group.s6_addr32[3] = ifp->grp_addr[3];
			mld_set_srcfilter(group, MCAST_INCLUDE, inSrcNum, mldInSrcListTmp, vif);
		#endif
		}
	}
	return SUCCESS;
}
int update_mfc(struct ifrec_entry *ifp)
{
	struct src_entry *s = NULL;

	if(ifp==NULL)
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "ERROR input parameter!\n");
		return FAILED;
	}

	if(ifp->filter_mode==MCAST_EXCLUDE)
		add_mfc(ifp->grp_addr,ZERO_ADDR,1,ifp->proxy_vif,ifp->ifindex,ifp->ip_version);
	else
		del_mfc(ifp->grp_addr,ZERO_ADDR,ifp->proxy_vif, ifp->ip_version);

	s = ifp->srclist;
	while(s)
	{
		if(s->user_count[MCAST_INCLUDE]==0)
		{
			if(ifp->user_count==0)
			{
				add_mfc(ifp->grp_addr,s->srcaddr,0,ifp->proxy_vif,ifp->ifindex,ifp->ip_version);
			}
			else
				del_mfc(ifp->grp_addr,s->srcaddr,ifp->proxy_vif,ifp->ip_version);
		}
		else
		{
			add_mfc(ifp->grp_addr,s->srcaddr,1,ifp->proxy_vif,ifp->ifindex,ifp->ip_version);
		}
		
		s = s->next;
	}

	
}
#else
int set_srcfilter(struct ifrec_entry *ifp, int vif)
{
	int srcNum = 0;
	struct src_entry *s = NULL;
#if defined(CONFIG_IGMPV3_SUPPORT)
	unsigned int igmpSrcListTmp[MAX_SRC_ENTRY];
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	struct in6_addr mldSrcListTmp[MAX_SRC_ENTRY];
#endif

	if(ifp==NULL)
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "ERROR input parameter!\n");
		return FAILED;
	}

	s = ifp->srclist;
	
	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "group:%s!\n", addr_sprintf(ifp->grp_addr, ifp->ip_version));

	while(s)
	{
		if(((ifp->filter_mode==MCAST_INCLUDE) && (s->timer.lefttime>0)) ||
			((ifp->filter_mode==MCAST_EXCLUDE) && (s->timer.lefttime==0)))
		{
	
#if defined(CONFIG_IGMPV3_SUPPORT)
			if(ifp->ip_version==IP_VERSION4)
			{
				igmpSrcListTmp[srcNum] = s->srcaddr[0];
				
				DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add src %s to srclist[%d]!\n", addr_sprintf(s->srcaddr, ifp->ip_version), srcNum);
				srcNum++;
			}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
			if(ifp->ip_version==IP_VERSION6)
			{
				mldSrcListTmp[srcNum].s6_addr32[0] = s->srcaddr[0];
				mldSrcListTmp[srcNum].s6_addr32[1] = s->srcaddr[1];
				mldSrcListTmp[srcNum].s6_addr32[2] = s->srcaddr[2];
				mldSrcListTmp[srcNum].s6_addr32[3] = s->srcaddr[3];
				
				DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add src %s to srclist[%d]!\n", addr_sprintf(s->srcaddr, ifp->ip_version));
				srcNum++;
			}
#endif
		}
		s = s->next;
	}
	
#if defined(CONFIG_IGMPV3_SUPPORT)
	if(ifp->ip_version==IP_VERSION4)
	{
		unsigned int group = ifp->grp_addr[0];
		igmp_set_srcfilter(group, ifp->filter_mode, srcNum, igmpSrcListTmp, vif);
	}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	if(ifp->ip_version==IP_VERSION6)
	{
		struct in6_addr group;
		group.s6_addr32[0] = ifp->grp_addr[0];
		group.s6_addr32[1] = ifp->grp_addr[1];
		group.s6_addr32[2] = ifp->grp_addr[2];
		group.s6_addr32[3] = ifp->grp_addr[3];
		mld_set_srcfilter(group, ifp->filter_mode, srcNum, mldSrcListTmp, vif);
	}
#endif

	return SUCCESS;
}
#endif
//here in vif is the date in interface, is opposite to igmp
add_mfc(unsigned int *groupAddr, unsigned int *srcAddr, int enable, int in_vif, int out_vif, int ipVersion)
{
#if defined(CONFIG_IGMPV3_SUPPORT)
	if(ipVersion==IP_VERSION4)
	{
		return igmp_add_mr(groupAddr[0], srcAddr[0], enable, in_vif, out_vif);
	}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	if(ipVersion==IP_VERSION6)
	{
		struct in6_addr group, src;
		group.s6_addr32[0] = groupAddr[0];
		group.s6_addr32[1] = groupAddr[1];
		group.s6_addr32[2] = groupAddr[2];
		group.s6_addr32[3] = groupAddr[3];		
		src.s6_addr32[0] = srcAddr[0];
		src.s6_addr32[1] = srcAddr[1];
		src.s6_addr32[2] = srcAddr[2];
		src.s6_addr32[3] = srcAddr[3];
		return mld_add_mr(group, src, enable, in_vif, out_vif);
	}
#endif

}
del_mfc(unsigned int *groupAddr, unsigned int *srcAddr, int in_vif, int ipVersion)
{
#if defined(CONFIG_IGMPV3_SUPPORT)
	if(ipVersion==IP_VERSION4)
	{
		return igmp_del_mr(groupAddr[0], srcAddr[0], in_vif);
	}
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	if(ipVersion==IP_VERSION6)
	{
		struct in6_addr group, src;
		group.s6_addr32[0] = groupAddr[0];
		group.s6_addr32[1] = groupAddr[1];
		group.s6_addr32[2] = groupAddr[2];
		group.s6_addr32[3] = groupAddr[3];		
		src.s6_addr32[0] = srcAddr[0];
		src.s6_addr32[1] = srcAddr[1];
		src.s6_addr32[2] = srcAddr[2];
		src.s6_addr32[3] = srcAddr[3];
		return mld_del_mr(group, src, in_vif);
	}
#endif
}
int rtl_handleIgmpv3Mldv2isIn(unsigned int *groupAddr, unsigned int *clientAddr, int srcNum, unsigned int *srcRecord, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	struct mcft_entry *mcfp = NULL;
	struct ifrec_entry *ifp = NULL;
	int ret = 0;
	int i = 0;
	struct src_entry *s, *old_set, *s_next;
	int *srcp = NULL;

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "receive a igmp/mld IS_IN: group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
		
	mcfp = get_mcft(groupAddr,groupList);
	if(mcfp==NULL)
	{
		//new group, and include(null), not join this group;
		if(srcNum==0)
			return SUCCESS;

		mcfp = add_mcft(groupAddr, mCastProxyPara, groupList);
		if(mcfp==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add mcft fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			return FAILED;
		}
	}

	ifp = get_ifrec(mcfp, in_vif);
	if(ifp==NULL)
	{
		//new group, and include(null), not join this group;
		if(srcNum==0)
			return SUCCESS;
		
		ifp = add_ifrec(mcfp, in_vif, out_vif, mCastProxyPara);
		if(ifp==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add ifrec fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			return FAILED;
		}
		ret = add_membership(groupAddr, out_vif, mCastProxyPara->ipVersion);
		if(ret==-1)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add membership fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			del_ifrec(mcfp, ifp->ifindex);
			if(mcfp->iflist==NULL)
				del_mcft(mcfp->grp_addr, mCastProxyPara, groupList);

			return FAILED;
		}
	}

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "group:%s, client:%s, srcNum:%d, origin filter mode:%s!\n", 
		addr_sprintf(groupAddr,mCastProxyPara->ipVersion),
		addr_sprintf(clientAddr,mCastProxyPara->ipVersion),srcNum,
		(ifp->filter_mode==MCAST_INCLUDE)?"include":"exclude");

#if defined(HOST_TRACKING_SUPPORT)
	struct mbr_entry *ifmbrp=NULL, *smbrp=NULL;
	ifmbrp = get_user_from_ifrec(ifp, clientAddr);
	if(ifmbrp)
	{
		del_user_from_ifrec(ifp,clientAddr);
		for(i=0; i<srcNum; i++)
		{	
			srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
			s = add_src_to_srclist(ifp, srcp);
			if(s)
			{
				smbrp = add_user_to_srclist(s, clientAddr, ifp->ip_version, MCAST_INCLUDE);
				smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
				smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				//src entry must be misc or all in.
				//add_mfc(ifp->grp_addr,s->srcaddr,1,out_vif, in_vif, ifp->ip_version);
			}
		}
	}
	else
	{
		old_set = ifp->srclist;
		for(i=0; i<srcNum; i++)
		{	
			srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
			s = add_src_to_srclist(ifp, srcp);
			if(s)
			{
				smbrp = add_user_to_srclist(s, clientAddr, ifp->ip_version, MCAST_INCLUDE);
				smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
				smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				//src entry must be misc or all in
				//add_mfc(ifp->grp_addr,s->srcaddr,1,out_vif, in_vif, ifp->ip_version);
			}
		}

		s = old_set;
		while(s)
		{
			s_next = s->next;
			if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
			{
				smbrp = get_user_from_srclist(s,clientAddr,ifp->ip_version);
				if(smbrp)
				{
					del_user_from_srclist(s,clientAddr,ifp->ip_version);
					
					if(s->user_count[MCAST_EXCLUDE]==0&&s->user_count[MCAST_INCLUDE]==0)
					{
						del_mfc(ifp->grp_addr,s->srcaddr,out_vif,ifp->ip_version);
						del_src_from_srclist(ifp,s->srcaddr);
					}						
				}
			}
			s = s_next;
		}
	}

	if(ifp->srclist==NULL && ifp->user_count==0)
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Remove Group %s!\n", addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion));
		del_mfc(ifp->grp_addr, ZERO_ADDR, out_vif, mCastProxyPara->ipVersion);
		del_membership(ifp->grp_addr, out_vif, mCastProxyPara->ipVersion);
		del_ifrec(mcfp,ifp->ifindex);
	}
	else
	{
		set_srcfilter(ifp,out_vif);
		update_mfc(ifp);
	}
#else
	switch(ifp->filter_mode)
	{
		case MCAST_INCLUDE:
			{				
				/*	IN(A), IN(B) => IN(A+B)
					(A, group, in_vif, ou_vif) => (A+B, group, in_vif, out_vif)
				*/
				old_set = ifp->srclist;
				for(i=0; i<srcNum; i++)
				{	
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
					//(B)=GMI
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{
						if (check_src_set(srcp, old_set, ifp->ip_version)==0)
						{
							//B-A
							add_mfc(ifp->grp_addr, srcp, 1, out_vif, in_vif, ifp->ip_version);
						}
						
						s->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
						s->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
					}
				}
			
				//set the new state
				ifp->filter_mode = MCAST_INCLUDE;
				set_srcfilter(ifp, out_vif);
			}
			break;
		case MCAST_EXCLUDE:
			{
				/*
					EX(X,Y), IS_IN(A) => EX(X+A, Y-A)
					(Y,group,in_vif, NULL)(X,group,in_vif, out_vif) --> (Y-A, group,in_vif, NULL)(X+A, group, in_vif, out_vif)
				*/

				old_set = ifp->srclist;
				//(A)=GMI
				for(i=0;i<srcNum;i++)
				{
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{	
						s->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
						s->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
						add_mfc(ifp->grp_addr, srcp, 1, out_vif, in_vif, ifp->ip_version);
					}
				}
				
				//set the new state
				ifp->filter_mode = MCAST_EXCLUDE;
				set_srcfilter(ifp, out_vif);
			}
			break;
		default:
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Unknown Filter Mode!\n");
			break;
	}
#endif
	return SUCCESS;
}


int rtl_handleIgmpv3Mldv2isEx(unsigned int *groupAddr, unsigned int *clientAddr, int srcNum, unsigned int *srcRecord, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	struct mcft_entry *mcfp = NULL;
	struct ifrec_entry *ifp = NULL;
	int ret = 0;
	int i;
	struct src_entry *s, *old_set,*s_next;
	int *srcp = NULL;
	

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "receive a igmp/mld IS_EX: group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));

#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
	if(mCastProxyPara->compatibleEnbale==0)
	{
		//if compatibility function disable, ignore
		DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "For compatibility purpose, IGNORE!\n");
		return SUCCESS;
	}
#endif

	mcfp = get_mcft(groupAddr,groupList);
	if(mcfp==NULL)
	{
		//new group
		mcfp = add_mcft(groupAddr, mCastProxyPara, groupList);
		if(mcfp==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add mcft fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			return FAILED;
		}
	}

	ifp = get_ifrec(mcfp, in_vif);
	if(ifp==NULL)
	{
		ifp = add_ifrec(mcfp, in_vif, out_vif, mCastProxyPara);
		if(ifp==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add ifrec fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			return FAILED;
		}
		ret = add_membership(groupAddr, out_vif, mCastProxyPara->ipVersion);		
		if(ret==-1)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add membership fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			del_ifrec(mcfp, ifp->ifindex);
			if(mcfp->iflist==NULL)
				del_mcft(mcfp->grp_addr, mCastProxyPara, groupList);

			return FAILED;
		}
	}

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "group:%s, client:%s, srcNum:%d, origin filter mode:%s!\n", 
		addr_sprintf(groupAddr,mCastProxyPara->ipVersion),
		addr_sprintf(clientAddr,mCastProxyPara->ipVersion),srcNum,
		(ifp->filter_mode==MCAST_INCLUDE)?"include":"exclude");

#if defined(HOST_TRACKING_SUPPORT)	
	struct mbr_entry *ifmbrp=NULL, *smbrp=NULL;
	ifmbrp = get_user_from_ifrec(ifp, clientAddr);
	if(ifmbrp)
	{
		if(srcNum==0)
		{
			//exclude NULL => exclude NULL
			ifmbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
			ifmbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
		}
		else
		{
			del_user_from_ifrec(ifp,clientAddr);
			old_set = ifp->srclist;
			for(i=0; i<srcNum; i++)
			{	
				srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
				s = add_src_to_srclist(ifp, srcp);
				if(s)
				{
					smbrp = add_user_to_srclist(s, clientAddr, ifp->ip_version, MCAST_EXCLUDE);
					smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
					smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				#if 0
					if((check_src_set(srcp, old_set, ifp->ip_version)==0)&&(ifp->user_count==0))
						add_mfc(ifp->grp_addr,s->srcaddr,0,out_vif, in_vif, ifp->ip_version);
				#endif
				}
			}
		}
	}
	else
	{
		if(srcNum==0)
		{
			ifmbrp = add_user_to_ifrec(ifp,clientAddr);
			ifmbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
			ifmbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
		}
			
		old_set = ifp->srclist;
		for(i=0; i<srcNum; i++)
		{	
			srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
			s = add_src_to_srclist(ifp, srcp);
			if(s)
			{
				smbrp = add_user_to_srclist(s, clientAddr, ifp->ip_version, MCAST_EXCLUDE);
				smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
				smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
			#if 0
				if(check_src_set(srcp, old_set, ifp->ip_version)==0)
					add_mfc(ifp->grp_addr,s->srcaddr,0,out_vif, in_vif, ifp->ip_version);
			#endif
			}
		}
			
		s = old_set;
		while(s)
		{
			s_next = s->next;
			if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
			{
				smbrp = get_user_from_srclist(s,clientAddr,ifp->ip_version);
				if(smbrp)
				{
					del_user_from_srclist(s,clientAddr,ifp->ip_version);
						
					if(s->user_count[MCAST_EXCLUDE]==0&&s->user_count[MCAST_INCLUDE]==0)
					{
						del_mfc(ifp->grp_addr,s->srcaddr,out_vif,ifp->ip_version);
						del_src_from_srclist(ifp,s->srcaddr);
					}
				#if 0
					else if(s->user_count[MCAST_INCLUDE]==0)
					{
						add_mfc(ifp->grp_addr, s->srcaddr, 0, out_vif, in_vif, ifp->ip_version);
					}
				#endif
				}
			}
			s = s_next;
		}
	}

	set_srcfilter(ifp,out_vif);
	update_mfc(ifp);
#if 0
	if((orig_filter_mode==MCAST_INCLUDE) && (ifp->filter_mode==MCAST_EXCLUDE))
	{
		add_mfc(ifp->grp_addr, ZERO_ADDR, 1, out_vif, in_vif, mCastProxyPara->ipVersion);
	}
#endif
#else
	switch(ifp->filter_mode)
	{
		case MCAST_INCLUDE:
			{
				/*
					spec:
						IN(A), IS_EX(B) => EX(A*B, B-A)
				*/
				old_set = ifp->srclist;
				for(i=0; i<srcNum; i++)
				{	
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{
						if (check_src_set(srcp, old_set, ifp->ip_version)==0)
						{
							//B-A
							s->timer.lefttime = 0;
							s->timer.retry_left = 0;
							add_mfc(ifp->grp_addr, srcp, 0, out_vif, in_vif, ifp->ip_version);
						}
					}
				}
				
				s = old_set;
				while(s)
				{
					s_next=s->next;
					if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
					{
						//A-B
						del_mfc(ifp->grp_addr, s->srcaddr, out_vif, ifp->ip_version);
						del_src_from_srclist(ifp, s->srcaddr);

					}
					s = s_next;
				}

				//Group Timer=GMI
				ifp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
				ifp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				
				//set the new state
				ifp->filter_mode = MCAST_EXCLUDE;
				set_srcfilter(ifp, out_vif);

				//for exclude mode, mfc(0:out_vif,group) is always needed.
				add_mfc(ifp->grp_addr, ZERO_ADDR, 1, out_vif, in_vif, ifp->ip_version);
			}
			break;
		case MCAST_EXCLUDE:
			{
				/*
					spec:
						EX(X,Y), IS_EX(A) => EX(A-Y, Y*A)
				*/
				
				old_set = ifp->srclist;
				for(i=0; i<srcNum; i++)
				{
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);					
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{
						if(check_src_set(srcp, old_set, ifp->ip_version)==0)
						{
							// (A-X-Y)=GMI
							s->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
							s->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
							add_mfc(ifp->grp_addr, srcp, 1, out_vif, in_vif, ifp->ip_version);
						}
					}
				}
				
				s = old_set;
				while(s)
				{
					s_next =s->next;
					
					//Delete (X-A), Delete(Y-A)
					if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
					{
						del_mfc(ifp->grp_addr, s->srcaddr, out_vif, ifp->ip_version);
						del_src_from_srclist(ifp, s->srcaddr );
					}
					s = s_next;
				}
								
				//Group Timer=GMI
				ifp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
				ifp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;

				//set the new state
				ifp->filter_mode = MCAST_EXCLUDE;
				set_srcfilter(ifp, out_vif);
			}
			break;
		default:
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Unknown Filter Mode!\n");
			break;
	}
#endif	
	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "handle IN_EX successfully!\n");
	return SUCCESS;
}
int rtl_handleIgmpv3Mldv2toIn(unsigned int *groupAddr, unsigned int *clientAddr, int srcNum, unsigned int *srcRecord, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	struct mcft_entry *mcfp = NULL;
	struct ifrec_entry *ifp = NULL;
	struct src_entry *s=NULL, *old_set=NULL, *s_next=NULL;
	int *srcp = NULL;
	unsigned int srcListTmp[MAX_SRC_ENTRY*4];
	unsigned short mrc=0;
	int ret = 0;
	int i;
	
	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "receive a igmp/mld TO_IN: group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));

#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
	if(mCastProxyPara->compatibleEnbale==0 ||
	   compatibilityStatus[in_vif]==COMPATIBILITY_IGMPV1)
	{
		//if compatibility function disable, ignore
		DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "For compatibility purpose, IGNORE!\n");
		return SUCCESS;
	}
#endif

	mcfp = get_mcft(groupAddr,groupList);
	if(mcfp==NULL)
	{
		//new group
		if(srcNum==0)
			return SUCCESS;
		
		mcfp = add_mcft(groupAddr, mCastProxyPara, groupList);
		if(mcfp==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add mcft fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			return FAILED;
		}
	}

	ifp = get_ifrec(mcfp, in_vif);
	if(ifp==NULL)
	{
		if(srcNum==0)
			return SUCCESS;
		
		ifp = add_ifrec(mcfp, in_vif, out_vif, mCastProxyPara);
		if(ifp==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add ifrec fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			return FAILED;
		}
		ret = add_membership(groupAddr, out_vif, mCastProxyPara->ipVersion);
		if(ret==-1)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add membership fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			del_ifrec(mcfp, ifp->ifindex);
			if(mcfp->iflist==NULL)
				del_mcft(mcfp->grp_addr, mCastProxyPara, groupList);

			return FAILED;
		}
	}

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "group:%s, client:%s, srcNum:%d, origin filter mode:%s!\n", 
		addr_sprintf(groupAddr,mCastProxyPara->ipVersion),
		addr_sprintf(clientAddr,mCastProxyPara->ipVersion),srcNum,
		(ifp->filter_mode==MCAST_INCLUDE)?"include":"exclude");

#if defined(HOST_TRACKING_SUPPORT)
	struct mbr_entry *ifmbrp=NULL, *smbrp=NULL;
	int need_query_group = 0;
	ifmbrp = get_user_from_ifrec(ifp, clientAddr);
	if(ifmbrp)
	{
		//exclude NULL => include(A)
		if(mCastProxyPara->fastleaveEnable==0 || !check_no_user_ifrec(ifp, clientAddr))
		{
			ifmbrp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
			ifmbrp->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
			need_query_group = 1;
		}
		else
		{
			//no listen group, listen to A
			del_user_from_ifrec(ifp,clientAddr);
			for(i=0; i<srcNum; i++)
			{	
				srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
				s = add_src_to_srclist(ifp, srcp);
				if(s)
				{
					smbrp = add_user_to_srclist(s, clientAddr, ifp->ip_version, MCAST_INCLUDE);
					smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
					smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
					//src entry must be misc or all in.
					//add_mfc(ifp->grp_addr,s->srcaddr,1,out_vif, in_vif, ifp->ip_version);
				}
			}
		}
	}
	else
	{
		old_set = ifp->srclist;
		for(i=0; i<srcNum; i++)
		{	
			srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
			s = add_src_to_srclist(ifp, srcp);
			if(s)
			{
				smbrp = add_user_to_srclist(s, clientAddr, ifp->ip_version, MCAST_INCLUDE);
				smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
				smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				//src entry must be misc or all in
				//add_mfc(ifp->grp_addr,s->srcaddr,1,out_vif, in_vif, ifp->ip_version);
			}
		}

		i=0;
		s = old_set;
		while(s)
		{
			s_next = s->next;
			if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
			{
				smbrp = get_user_from_srclist(s,clientAddr,ifp->ip_version);
				if(smbrp)
				{
					if(smbrp->filter_mode==MCAST_EXCLUDE)
					{
						del_user_from_srclist(s,clientAddr,ifp->ip_version);
						
						if(s->user_count[MCAST_EXCLUDE]==0&&s->user_count[MCAST_INCLUDE]==0)
						{
							del_mfc(ifp->grp_addr,s->srcaddr,out_vif,ifp->ip_version);
							del_src_from_srclist(ifp,s->srcaddr);
						}
					}
					else
					{
						if(mCastProxyPara->fastleaveEnable==0 || !check_no_user_src(s,clientAddr,ifp->ip_version))
						{
							//may be used late for special group and source query.
							srcp = srcListTmp+i*ADDR_SIZE_INT(ifp->ip_version);
							memcpy(srcp, s->srcaddr, ADDR_SIZE(ifp->ip_version));
							i++;
							
							smbrp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
							smbrp->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
							need_query_group=1;
						}
						else
						{
							del_user_from_srclist(s,clientAddr,ifp->ip_version);
							if(s->user_count[MCAST_EXCLUDE]==0&&s->user_count[MCAST_INCLUDE]==0)
							{
								//in src --> null src
								del_mfc(ifp->grp_addr,s->srcaddr,out_vif,ifp->ip_version);
								del_src_from_srclist(ifp,s->srcaddr);
							}
						}
					}						
				}
			}
			s = s_next;
		}
	}

	if(need_query_group)
	{
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
		mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,ifp->ip_version);
#else
		mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,ifp->ip_version);
#endif
		send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr, mrc, 0, mCastProxyPara->timeConfig.robustness, 0, NULL, ifp->ip_version);
	}

	if(ifp->srclist==NULL && ifp->user_count==0)
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Remove Group %s!\n", addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion));
		del_mfc(ifp->grp_addr, ZERO_ADDR, out_vif, mCastProxyPara->ipVersion);
		del_membership(ifp->grp_addr, out_vif, mCastProxyPara->ipVersion);
		del_ifrec(mcfp,ifp->ifindex);
	}
	else
	{
		set_srcfilter(ifp,out_vif);
		update_mfc(ifp);
	}
#else
	switch(ifp->filter_mode)
	{
		case MCAST_INCLUDE:
			{
				/*
					IN(A), TO_IN(B) => IN(A+B)
					(A, group, in_vif, ou_vif) => (A+B, group, in_vif, out_vif)
				*/
				old_set = ifp->srclist;
				//(B)=GMI
				for(i=0; i<srcNum; i++)
				{	
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{
						if (check_src_set(srcp, old_set, ifp->ip_version)==0)
						{
							//B-A
							add_mfc(ifp->grp_addr, srcp, 1, out_vif, in_vif, ifp->ip_version);
						}
						s->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
						s->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
					}
				}
				
				//send Q(MA,A-B)
				i=0;
				s = old_set;
				while(s)
				{
					s_next=s->next;
					if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
					{
						//A-B
						srcp = srcListTmp+i*ADDR_SIZE_INT(ifp->ip_version);
						memcpy(srcp, s->srcaddr, ADDR_SIZE(ifp->ip_version));

						s->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
						s->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
						
						i++;
					}
					s = s_next;
				}
				
				if(i>0)
				{
				#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
					mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,ifp->ip_version);
				#else
					mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,ifp->ip_version);
				#endif
					send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr, mrc, 0, mCastProxyPara->timeConfig.robustness, i, srcListTmp, ifp->ip_version);
				}
				
				//set the new state
				ifp->filter_mode = MCAST_INCLUDE;
				set_srcfilter(ifp, out_vif);
			}
			break;
		case MCAST_EXCLUDE:
			{
				/*
					EX(X,Y), TO_IN(A) => EX(X+A, Y-A)
					(Y,group,in_vif, NULL)(X,group,in_vif, out_vif) --> (Y-A, group,in_vif, NULL)(X+A, group, in_vif, out_vif)
				*/

				old_set = ifp->srclist;
				//(A)=GMI
				for(i=0;i<srcNum;i++)
				{
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{
						s->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
						s->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
						
						add_mfc(ifp->grp_addr, srcp, 1, out_vif, in_vif, ifp->ip_version);
					}
				}

				i = 0;
				s = old_set;
				while(s)
				{
					s_next=s->next;
					if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
					{
						if(s->timer.lefttime>0)
						{
							srcp = srcListTmp+i*ADDR_SIZE_INT(ifp->ip_version);
							memcpy(srcp, s->srcaddr, ADDR_SIZE(ifp->ip_version));
					
							s->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
							s->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
							i++;

						}
					}					
					s = s_next;
				}				
			
				//send Q(G,X-A)
			#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
				mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,ifp->ip_version);
			#else
				mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,ifp->ip_version);
			#endif
				if(i>0)
				{
					send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr, mrc, 0, mCastProxyPara->timeConfig.robustness, i, srcListTmp, ifp->ip_version);
				}

				//send Q(G) here to continue;
				ifp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
				ifp->timer.retry_left= mCastProxyPara->timeConfig.lastMemberCount;

				send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr, mrc, 0, mCastProxyPara->timeConfig.robustness, 0, NULL, ifp->ip_version);
				
				//set the new state
				ifp->filter_mode = MCAST_EXCLUDE;
				set_srcfilter(ifp, out_vif);

			}
			break;
		default:
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Unknown Filter Mode!\n");
			break;
	}
#endif	
	return SUCCESS;
}

int rtl_handleIgmpv3Mldv2toEx(unsigned int *groupAddr, unsigned int *clientAddr, int srcNum, unsigned int *srcRecord, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	struct mcft_entry *mcfp = NULL;
	struct ifrec_entry *ifp = NULL;
	struct src_entry *s, *old_set,*s_next;
	int *srcp = NULL, *srclistp = NULL;
	unsigned int srcListTmp[MAX_SRC_ENTRY*4];
	unsigned short mrc = 0;
	int ret = 0;
	int i=0, j=0;

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "receive a igmp/mld TO_EX: group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
	if(mCastProxyPara->compatibleEnbale==0)
	{
		//if compatibility function disable, ignore
		DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "For compatibility purpose, IGNORE!\n");
		return SUCCESS;
	}

	if(compatibilityStatus[in_vif]!=COMPATIBILITY_IGMPV3_MLDV2)
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "For compatibility purpose, change to TO_EX(*)!\n");
		srcNum = 0;
		srcRecord = NULL;
	}
#endif

	mcfp = get_mcft(groupAddr,groupList);
	if(mcfp==NULL)
	{
		//new group
		mcfp = add_mcft(groupAddr, mCastProxyPara, groupList);
		if(mcfp==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add mcft fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			return FAILED;
		}
	}

	ifp = get_ifrec(mcfp, in_vif);
	if(ifp==NULL)
	{
		ifp = add_ifrec(mcfp, in_vif, out_vif, mCastProxyPara);
		if(ifp==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add ifrec fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			return FAILED;
		}
		ret = add_membership(groupAddr, out_vif, mCastProxyPara->ipVersion);
		if(ret==-1)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add membership fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			del_ifrec(mcfp, ifp->ifindex);
			if(mcfp->iflist==NULL)
				del_mcft(mcfp->grp_addr, mCastProxyPara, groupList);

			return FAILED;
		}
	}

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "group:%s, client:%s, srcNum:%d, origin filter mode:%s!\n", 
		addr_sprintf(groupAddr,mCastProxyPara->ipVersion),
		addr_sprintf(clientAddr,mCastProxyPara->ipVersion),srcNum,
		(ifp->filter_mode==MCAST_INCLUDE)?"include":"exclude");

#if defined(HOST_TRACKING_SUPPORT)	
	struct mbr_entry *ifmbrp=NULL, *smbrp=NULL;
	int need_query_group = 0;
	ifmbrp = get_user_from_ifrec(ifp, clientAddr);
	if(ifmbrp)
	{
		if(srcNum==0)
		{
			//exclude NULL => exclude NULL
			ifmbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
			ifmbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
		}
		else
		{
			//exclude NULL =>exclude(A)
			if(mCastProxyPara->fastleaveEnable==0 || !check_no_user_ifrec(ifp, clientAddr))
			{
				ifmbrp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
				ifmbrp->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
				need_query_group = 1;
			}
			else
			{
				del_user_from_ifrec(ifp,clientAddr);
				old_set = ifp->srclist;
				for(i=0; i<srcNum; i++)
				{	
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{
						smbrp = add_user_to_srclist(s, clientAddr, ifp->ip_version, MCAST_EXCLUDE);
						smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
						smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
					}
				}
			}
		}
	}
	else
	{
		if(srcNum==0)
		{
			ifmbrp = add_user_to_ifrec(ifp,clientAddr);
			ifmbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
			ifmbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
		}
			
		old_set = ifp->srclist;
		for(i=0; i<srcNum; i++)
		{	
			srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
			s = add_src_to_srclist(ifp, srcp);
			if(s)
			{
				smbrp = add_user_to_srclist(s, clientAddr, ifp->ip_version, MCAST_EXCLUDE);
				smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
				smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
			}
		}
			
		s = old_set;
		while(s)
		{
			s_next = s->next;
			if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
			{
				smbrp = get_user_from_srclist(s,clientAddr,ifp->ip_version);
				if(smbrp)
				{
					del_user_from_srclist(s,clientAddr,ifp->ip_version);
						
					if(s->user_count[MCAST_EXCLUDE]==0&&s->user_count[MCAST_INCLUDE]==0)
					{
						del_mfc(ifp->grp_addr,s->srcaddr,out_vif,ifp->ip_version);
						del_src_from_srclist(ifp,s->srcaddr);
					}
				}
			}
			s = s_next;
		}
	}

	if(need_query_group)
	{
	#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
		mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,ifp->ip_version);
	#else
		mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,ifp->ip_version);
	#endif
		send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr, mrc, 0, mCastProxyPara->timeConfig.robustness, 0, NULL, ifp->ip_version);
	}

	set_srcfilter(ifp,out_vif);
	update_mfc(ifp);
#else
	switch(ifp->filter_mode)
	{
		case MCAST_INCLUDE:
			{
				/* spec:
				   	IN(A), TO_EX(B) => EX(A*B, B-A)
				*/
				old_set = ifp->srclist;
				j=0;
				for(i=0; i<srcNum; i++)
				{	
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{
						if(check_src_set(srcp, old_set, ifp->ip_version)==0)
						{
							//(B-A) = 0;
							s->timer.lefttime = 0;
							s->timer.retry_left = 0;
							add_mfc(ifp->grp_addr, srcp, 0, out_vif, in_vif, ifp->ip_version);
						}
						else
						{
							//A*B
							
							/*add A*B to special query srclist*/
							srclistp = srcListTmp + j*ADDR_SIZE_INT(ifp->ip_version);
							memcpy(srclistp, s->srcaddr, ADDR_SIZE(ifp->ip_version));
						
							/*lower A*B timer to LMQT*/
							s->timer.lefttime=mCastProxyPara->timeConfig.lastMemberInterval;
							s->timer.retry_left=mCastProxyPara->timeConfig.lastMemberCount;
							
							j++;
						}
					}
				}
				
				s = old_set;
				while(s)
				{
					s_next=s->next;
					if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
					{
						//Delete (A-B)
						del_mfc(ifp->grp_addr, s->srcaddr, out_vif, ifp->ip_version);
						del_src_from_srclist(ifp, s->srcaddr);
					}

					s = s_next;
				}

				if(j>0)
				{
			#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
					mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,ifp->ip_version);
			#else
					mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,ifp->ip_version);
			#endif
					send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr,mrc,0, mCastProxyPara->timeConfig.robustness,j,srcListTmp,ifp->ip_version);
				}
				
				//Group Timer=GMI
				ifp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
				ifp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				
				//set the new state
				ifp->filter_mode = MCAST_EXCLUDE;
				set_srcfilter(ifp, out_vif);

				//for exclude mode, mfc(0:out_vif,group) is always needed.
				add_mfc(ifp->grp_addr, ZERO_ADDR, 1, out_vif, in_vif, ifp->ip_version);
				
			}
			break;
		case MCAST_EXCLUDE:
			{				
				/*
					spec:
						EX(X,Y), TO_EX(A) => EX(A-Y, Y*A)
				*/
				old_set = ifp->srclist;
				j=0;
				for(i=0; i<srcNum; i++)
				{
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);					
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{
						if(check_src_set(srcp, old_set, ifp->ip_version)==0)
						{
							/*A-X-Y*/
							srclistp = srcListTmp + j*ADDR_SIZE_INT(ifp->ip_version);
							memcpy(srclistp, s->srcaddr, ADDR_SIZE(ifp->ip_version));
							
							/*lower src timer to LMQT*/
							s->timer.lefttime=mCastProxyPara->timeConfig.lastMemberInterval;
							s->timer.retry_left=mCastProxyPara->timeConfig.lastMemberCount;

							j++;
							
							add_mfc(ifp->grp_addr, srcp, 1, out_vif, in_vif, ifp->ip_version);
							
						}

						else
						{
							if(s->timer.lefttime>0)
							{
								//A*X								
								srclistp = srcListTmp + j*ADDR_SIZE_INT(ifp->ip_version);
								memcpy(srclistp, s->srcaddr, ADDR_SIZE(ifp->ip_version));
								
								/*lower src timer to LMQT*/
								s->timer.lefttime=mCastProxyPara->timeConfig.lastMemberInterval;
								s->timer.retry_left=mCastProxyPara->timeConfig.lastMemberCount;
								
								j++;
							}
						}
					}
				}
				
				s = old_set;
				while(s)
				{
					s_next =s->next;
					
					if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
					{
						//Delete (X-A), Delete(Y-A)
						del_mfc(ifp->grp_addr, s->srcaddr, out_vif, ifp->ip_version);
						del_src_from_srclist(ifp, s->srcaddr );
					}
					s = s_next;
				}
								
				//send Q(MA,A-Y)
				if(j>0)
				{
				#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
					mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,ifp->ip_version);
				#else
					mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,ifp->ip_version);
				#endif
					send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr,mrc,0, mCastProxyPara->timeConfig.robustness,j,srcListTmp,ifp->ip_version);
				}
				//Group Timer=GMI
				ifp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
				ifp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;

				//set the new state
				ifp->filter_mode = MCAST_EXCLUDE;
				set_srcfilter(ifp, out_vif);
			}			
			break;
		default:
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Unknown Filter Mode!\n");
			break;
	}
#endif	
	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "handle TO_EX successfully!\n");
	return SUCCESS;
}
int rtl_handleIgmpv3Mldv2Allow(unsigned int *groupAddr, unsigned int *clientAddr, int srcNum, unsigned int *srcRecord, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	struct mcft_entry *mcfp = NULL;
	struct ifrec_entry *ifp = NULL;
	int ret = 0;
	int i = 0;
	struct src_entry *s, *old_set, *s_next;
	int *srcp = NULL;

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "receive a igmp/mld ALLOW: group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));

	if(srcNum==0)
		return SUCCESS;

	mcfp = get_mcft(groupAddr,groupList);
	if(mcfp==NULL)
	{
		//new group
		mcfp = add_mcft(groupAddr, mCastProxyPara, groupList);
		if(mcfp==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add mcft fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			return FAILED;
		}
	}

	ifp = get_ifrec(mcfp, in_vif);
	if(ifp==NULL)
	{
		ifp = add_ifrec(mcfp, in_vif, out_vif, mCastProxyPara);
		if(ifp==NULL)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add ifrec fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			return FAILED;
		}
		ret = add_membership(groupAddr, out_vif, mCastProxyPara->ipVersion);
		if(ret==-1)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "add membership fail, group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
			del_ifrec(mcfp, ifp->ifindex);
			if(mcfp->iflist==NULL)
				del_mcft(mcfp->grp_addr, mCastProxyPara, groupList);
			return FAILED;
		}	
	}

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "group:%s, client:%s, srcNum:%d, origin filter mode:%s!\n", 
		addr_sprintf(groupAddr,mCastProxyPara->ipVersion),
		addr_sprintf(clientAddr,mCastProxyPara->ipVersion),srcNum,
		(ifp->filter_mode==MCAST_INCLUDE)?"include":"exclude");

#if defined(HOST_TRACKING_SUPPORT)
	struct mbr_entry *ifmbrp=NULL, *smbrp=NULL;
	int need_query_group = 0;
	int mrc;
	
	ifmbrp = get_user_from_ifrec(ifp, clientAddr);
	if(ifmbrp)
	{
		//unlikely
		ifmbrp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
		ifmbrp->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
		need_query_group = 0;
	}
	else
	{
		old_set = ifp->srclist;
		for(i=0; i<srcNum; i++)
		{	
			srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);					
			//(B)=GMI
			s = add_src_to_srclist(ifp, srcp);
			if(s)
			{
				smbrp = get_user_from_srclist(s, clientAddr, ifp->ip_version);
				if(smbrp)
				{
					if(smbrp->filter_mode==MCAST_EXCLUDE)
						del_user_from_srclist(s,clientAddr,ifp->ip_version);
					else
					{
						//unlikely.
						smbrp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
						smbrp->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
						need_query_group = 1;
					}

					if(s->user_count[MCAST_EXCLUDE]==0&&s->user_count[MCAST_INCLUDE]==0)
					{
						del_mfc(ifp->grp_addr,s->srcaddr,out_vif,ifp->ip_version);
						del_src_from_srclist(ifp,s->srcaddr);
					}
						
				}
				else
				{
					smbrp = add_user_to_srclist(s,clientAddr,ifp->ip_version, MCAST_INCLUDE);
					smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
					smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				}
			}
		}

		s = old_set;
		while(s)
		{
			s_next = s->next;
			if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
			{
				smbrp = get_user_from_srclist(s,clientAddr,ifp->ip_version);
				if(smbrp)
				{
					smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
					smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				}
			}
			s = s_next;
		}
	}

	if(need_query_group)
	{
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
		mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,ifp->ip_version);
#else
		mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,ifp->ip_version);
#endif
		send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr, mrc, 0,  mCastProxyPara->timeConfig.robustness,0, NULL, ifp->ip_version);
	}

	if(ifp->srclist==NULL)
	{			
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Change to EX NULL group:%s!\n", addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion));
		ifmbrp = add_user_to_ifrec(ifp,clientAddr);
		ifmbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
		ifmbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
	}

	set_srcfilter(ifp,out_vif);
	update_mfc(ifp);
#else
	switch(ifp->filter_mode)
	{
		case MCAST_INCLUDE:
			{				
				/*
					IN(A), ALLOW(B) => IN(A+B)
					(A, group, in_vif, ou_vif) => (A+B, group, in_vif, out_vif)
				*/
				
				old_set = ifp->srclist;
				for(i=0; i<srcNum; i++)
				{	
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);					
					//(B)=GMI
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{
						if (check_src_set(srcp, old_set, ifp->ip_version)==0)
						{
							add_mfc(ifp->grp_addr, srcp, 1, out_vif, in_vif, ifp->ip_version);
						}
						s->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
						s->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
					}
				}
			
				//set the new state
				ifp->filter_mode = MCAST_INCLUDE;
				set_srcfilter(ifp, out_vif);			
			}
			break;
		case MCAST_EXCLUDE:
			{
				//EX(X,Y), ALLOW(A) => EX(X+A, Y-A)
				//(Y,group,in_vif, NULL)(X,group,in_vif, out_vif) --> (Y-A, group,in_vif, NULL)(X+A, group, in_vif, out_vif)
				old_set = ifp->srclist;
				//(A)=GMI
				for(i=0;i<srcNum;i++)
				{
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{	
						s->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
						s->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
						
						add_mfc(ifp->grp_addr, srcp, 1, out_vif, in_vif, ifp->ip_version);
					}
				}
				
				//set the new state
				ifp->filter_mode = MCAST_EXCLUDE;
				set_srcfilter(ifp, out_vif);
			}
			break;
		default:
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Unknown Filter Mode!\n");
			break;
	}
#endif
	return SUCCESS;
}

int rtl_handleIgmpv3Mldv2Block(unsigned int *groupAddr, unsigned int *clientAddr, int srcNum, unsigned int *srcRecord, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	struct mcft_entry *mcfp = NULL;
	struct ifrec_entry *ifp = NULL;
	unsigned int srcListTmp[MAX_SRC_ENTRY*4];
	unsigned short mrc = 0;
	int i=0, j=0;
	struct src_entry *s, *old_set,*s_next;
	int *srcp = NULL, *srclistp = NULL;
	

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "receive a igmp/mld BLOCK: group:%s!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));

	if(srcNum==0)
		return SUCCESS;
	
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
	if(mCastProxyPara->compatibleEnbale)
	{
		//if compatibility function disable, ignore		
		if(compatibilityStatus[in_vif]!=COMPATIBILITY_IGMPV3_MLDV2)
		{
			DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "For compatibility purpose, IGNORE!\n");
			return SUCCESS;
		}
	}

#endif
	mcfp = get_mcft(groupAddr,groupList);
	if(mcfp==NULL)
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "group %s not exist!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion));
		return SUCCESS;
	}

	ifp = get_ifrec(mcfp, in_vif);
	if(ifp==NULL)
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "group %s not exist on interface %d!\n", addr_sprintf(groupAddr,mCastProxyPara->ipVersion), in_vif);
		return SUCCESS;
	}

	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "group:%s, client:%s, srcNum:%d, origin filter mode:%s!\n", 
		addr_sprintf(groupAddr,mCastProxyPara->ipVersion),
		addr_sprintf(clientAddr,mCastProxyPara->ipVersion),srcNum,
		(ifp->filter_mode==MCAST_INCLUDE)?"include":"exclude");

#if defined(HOST_TRACKING_SUPPORT)
	struct mbr_entry *ifmbrp=NULL, *smbrp=NULL;
	int need_query_group = 0;
	ifmbrp = get_user_from_ifrec(ifp,clientAddr);
	if(ifmbrp)
	{
		//exclude NULL =>exclude(B)
		if(mCastProxyPara->fastleaveEnable==0 || !check_no_user_ifrec(ifp, clientAddr))
		{
			ifmbrp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
			ifmbrp->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
			need_query_group = 1;
		}
		else
		{
			del_user_from_ifrec(ifp,clientAddr);
			for(i=0; i<srcNum; i++)
			{	
				srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
				s = add_src_to_srclist(ifp, srcp);
				if(s)
				{
					smbrp = add_user_to_srclist(s, clientAddr, ifp->ip_version, MCAST_EXCLUDE);
					smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
					smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				}
			}
		}		
	}
	else
	{
		old_set = ifp->srclist;
		for(i=0; i<srcNum; i++)
		{	
			srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
			s = add_src_to_srclist(ifp, srcp);
			if(s)
			{
				smbrp = get_user_from_srclist(s,clientAddr,ifp->ip_version);
				if(smbrp)
				{
					if(smbrp->filter_mode==MCAST_INCLUDE)
					{
						if(mCastProxyPara->fastleaveEnable==0 || !check_no_user_src(s,clientAddr,ifp->ip_version))
						{
							smbrp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
							smbrp->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
							need_query_group = 1;
						}
						else
						{
							del_user_from_srclist(s,clientAddr,ifp->ip_version);
							if(s->user_count[MCAST_EXCLUDE]==0&&s->user_count[MCAST_INCLUDE]==0)
							{
								del_mfc(ifp->grp_addr,s->srcaddr,out_vif,ifp->ip_version);
								del_src_from_srclist(ifp,s->srcaddr);
							}
						}
					}
					else
					{
						//unlikely
						smbrp->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
						smbrp->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
						need_query_group = 1;
					}
				}
				else
				{
					smbrp = add_user_to_srclist(s, clientAddr, ifp->ip_version, MCAST_EXCLUDE);
					smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
					smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				}
			}
		}
			
		s = old_set;
		while(s)
		{
			s_next = s->next;
			if(check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)==0)
			{
				smbrp = get_user_from_srclist(s,clientAddr,ifp->ip_version);
				if(smbrp)
				{
					smbrp->timer.lefttime = mCastProxyPara->timeConfig.memberQueryInterval;
					smbrp->timer.retry_left = mCastProxyPara->timeConfig.memberQueryCount;
				}
			}
			s = s_next;
		}
	}

	if(need_query_group)
	{
#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
		mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,ifp->ip_version);
#else
		mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,ifp->ip_version);
#endif
		send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr, mrc, 0, mCastProxyPara->timeConfig.robustness, 0, NULL, ifp->ip_version);
	}

	if(ifp->srclist==NULL && ifp->user_count==0)
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Remove Group %s!\n", addr_sprintf(ifp->grp_addr, mCastProxyPara->ipVersion));
		del_mfc(ifp->grp_addr, ZERO_ADDR, out_vif, mCastProxyPara->ipVersion);
		del_membership(ifp->grp_addr, out_vif, mCastProxyPara->ipVersion);
		del_ifrec(mcfp,ifp->ifindex);
	}
	else
	{
		set_srcfilter(ifp,out_vif);
		update_mfc(ifp);
	}
#else
	switch(ifp->filter_mode)
	{
		case MCAST_INCLUDE:
			{
				/*
					IN(A), BLOCK(B) => IN(A)  do noting
					Send Q(MA, A*B)
				*/
				j=0;
				s = ifp->srclist;
				while(s)
				{	
					s_next = s->next;
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
					if (check_src(s->srcaddr, srcRecord, srcNum, ifp->ip_version)!=0)
					{
						srclistp = srcListTmp+j*ADDR_SIZE_INT(ifp->ip_version);
						memcpy(srclistp, s->srcaddr, ADDR_SIZE(ifp->ip_version));
					
						/*lower A*B timer to LMQT*/
						s->timer.lefttime=mCastProxyPara->timeConfig.lastMemberInterval;
						s->timer.retry_left=mCastProxyPara->timeConfig.lastMemberCount;						
						j++;
					}
					s = s_next;
				}
				if(j>0)
				{
			#if defined(SMALLER_TIMER_INTERVAL_SUPPORT)	
					mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval,ifp->ip_version);
			#else
					mrc = calc_mrc(mCastProxyPara->timeConfig.lastMemberInterval*mCastProxyPara->timeConfig.intervalScale,ifp->ip_version);
			#endif
					send_query(mCastProxyPara->forceVersion, ifp->grp_addr, ifp->grp_addr,mrc,0, mCastProxyPara->timeConfig.robustness,j,srcListTmp,ifp->ip_version);
				}			
				
			}
			break;
		case MCAST_EXCLUDE:
			{				
				//EX(X,Y), BLOCK(A) => EX(X+(A-Y), Y)
				old_set = ifp->srclist;
				j=0;
				for(i=0; i<srcNum; i++)
				{
					srcp = srcRecord+i*ADDR_SIZE_INT(ifp->ip_version);
					s = add_src_to_srclist(ifp, srcp);
					if(s)
					{
						if(check_src_set(srcp, old_set, ifp->ip_version)==0)
						{
							//A-X-Y
							srclistp = srcListTmp+j*ADDR_SIZE_INT(ifp->ip_version);
							memcpy(srclistp, s->srcaddr, ADDR_SIZE(ifp->ip_version));
							
							s->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
							s->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
							
							add_mfc(ifp->grp_addr, srcp, 1, out_vif, in_vif, ifp->ip_version);
							j++;
						}
						else
						{
							if(s->timer.lefttime>0)
							{
								srclistp = srcListTmp+j*ADDR_SIZE_INT(ifp->ip_version);
								memcpy(srclistp, s->srcaddr, ADDR_SIZE(ifp->ip_version));
							
								s->timer.lefttime = mCastProxyPara->timeConfig.lastMemberInterval;
								s->timer.retry_left = mCastProxyPara->timeConfig.lastMemberCount;
								j++;
							}
						}
					}
				}

				//set the new state
				ifp->filter_mode = MCAST_EXCLUDE;
				set_srcfilter(ifp, out_vif);
			}
			break;
		default:
			DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "Unknown Filter Mode!\n");
			break;
	}
#endif	
	DBG_IGMP_MLD_PRK(IGMP_MLD_PROTO, "handle BLOCK successfully!\n");
	return SUCCESS;
}

int rtl_handleIgmpMldQuery(unsigned int *clientAddr, void *l3PktBuf, int l3PktLen, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	unsigned int *groupAddr= NULL;
	int qqi = 0, qqic = 0;
	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "receive a igmp/mld Query!\n");

	/*query selection--todo*/
#if defined(CONFIG_IGMPV3_SUPPORT)
	//only support for igmp proxy
	if(l3PktLen>=IGMPV3_MLDV2_QUERY_SIZE(mCastProxyPara->ipVersion))
	{
		//igmpv3 query
		groupAddr = (unsigned int *)(&((struct igmpv3_query *)l3PktBuf)->group);
		qqic = ((struct igmpv3_query *)l3PktBuf)->qqic;
		if(memcmp(groupAddr, ZERO_ADDR, ADDR_SIZE(mCastProxyPara->ipVersion))==0&&qqic!=0)
		{
			//rececive a general query,  change general query interval
			qqi = IGMPV3_MLDV2_QQIC(mCastProxyPara->ipVersion, qqic);
			//printf("qqic:%d, qqi:%d\n", qqic, qqi);
			mCastProxyPara->timeConfig.periodGeneralQueryInterval = qqi/2;
			mCastProxyPara->timeConfig.memberQueryInterval = mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval;
			mCastProxyPara->timeConfig.oldVersionHostPresentTimeout = mCastProxyPara->timeConfig.robustness*mCastProxyPara->timeConfig.periodGeneralQueryInterval+mCastProxyPara->timeConfig.QueryResponseInterval;
			newGeneralQueryInterval = qqi/2;
			write_proxy_para_to_file(mCastProxyPara);
			sync_snooping_para(mCastProxyPara);
		}
		
	}
#endif
	return SUCCESS;
}
int rtl_handleIgmpv1Join(unsigned int *clientAddr, void *l3PktBuf, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "receive a igmpv1 Join!\n");
	unsigned int *groupAddr= NULL;
	int ret = FAILED, valid_group = FALSE;

#if defined(CONFIG_IGMPV3_SUPPORT)
	groupAddr = (unsigned int *)(&((struct igmphdr *)l3PktBuf)->group);
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	groupAddr = (unsigned int *)(&((struct mld_hdr *)l3PktBuf)->mld_addr);
#endif
	valid_group = check_group_validity(groupAddr, mCastProxyPara->ipVersion);
	if(valid_group==FALSE)
		return FAILED;
	
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
	if(mCastProxyPara->compatibleEnbale==0)
	{
		//if compatibility function disable, ignore
		DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "For compatibility purpose, IGNORE!\n");
		return SUCCESS;
	}

	compatibilityStatus[in_vif]=COMPATIBILITY_IGMPV1;
	igmpV1Timer[in_vif].lefttime = mCastProxyPara->timeConfig.oldVersionHostPresentTimeout;
#if defined(DBG_IGMP_MLD)
	show_compatibility_status(mCastProxyPara);
#endif
#endif
	ret = rtl_handleIgmpv3Mldv2toEx(groupAddr, clientAddr,0, NULL, mCastProxyPara, groupList, in_vif, out_vif);
	
	return ret;
}
int rtl_handleIgmpv2Mldv1Join(unsigned int *clientAddr, void *l3PktBuf, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "receive a igmp/mld Join!\n");
	unsigned int *groupAddr= NULL;
	int ret = FAILED, valid_group = FALSE;

#if defined(CONFIG_IGMPV3_SUPPORT)
	groupAddr = (unsigned int *)(&((struct igmphdr *)l3PktBuf)->group);
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	groupAddr = (unsigned int *)(&((struct mld_hdr *)l3PktBuf)->mld_addr);
#endif

	valid_group = check_group_validity(groupAddr, mCastProxyPara->ipVersion);
	if(valid_group==FALSE)
		return FAILED;

	#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
	if(mCastProxyPara->compatibleEnbale==0)
	{
		//if compatibility function disable, ignore
		DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "For compatibility purpose, IGNORE!\n");
		return SUCCESS;
	}

	igmpV2MldV1Timer[in_vif].lefttime= mCastProxyPara->timeConfig.oldVersionHostPresentTimeout;
	if(compatibilityStatus[in_vif]==COMPATIBILITY_IGMPV3_MLDV2)
		compatibilityStatus[in_vif]=COMPATIBILITY_IGMPV2_MLDV1;
#endif
	ret = rtl_handleIgmpv3Mldv2toEx(groupAddr, clientAddr,0, NULL, mCastProxyPara, groupList, in_vif, out_vif);
	
	return ret;
}
int rtl_handleIgmpMldLeave(unsigned int *clientAddr, void *l3PktBuf, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "receive a igmp/mld Leave!\n");
	unsigned int *groupAddr= NULL;
	int ret = FAILED, valid_group=FALSE;

#if defined(CONFIG_IGMPV3_SUPPORT)
	groupAddr = (unsigned int *)(&((struct igmphdr *)l3PktBuf)->group);
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	groupAddr = (unsigned int *)(&((struct mld_hdr *)l3PktBuf)->mld_addr);
#endif
	valid_group = check_group_validity(groupAddr, mCastProxyPara->ipVersion);
	if(valid_group==FALSE)
		return FAILED;
	
#if defined(CONFIG_RTL_LAN_COMPATIBILITY)
	if(mCastProxyPara->compatibleEnbale==0)
	{
		//if compatibility function disable, ignore
		DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "For compatibility purpose, IGNORE!\n");
		return SUCCESS;
	}

	igmpV2MldV1Timer[in_vif].lefttime= mCastProxyPara->timeConfig.oldVersionHostPresentTimeout;
	if(compatibilityStatus[in_vif]==COMPATIBILITY_IGMPV1)
	{
		DBG_IGMP_MLD_PRK(IGMP_MLD_RECEIVE, "For compatibility purpose, IGNORE!\n");
		return SUCCESS;
	}

	compatibilityStatus[in_vif]=COMPATIBILITY_IGMPV2_MLDV1;
#endif
	ret = rtl_handleIgmpv3Mldv2toIn(groupAddr, clientAddr, 0, NULL, mCastProxyPara, groupList, in_vif, out_vif);
	
	return SUCCESS;
}
int rtl_handleIgmpv3Mldv2Report(unsigned int *clientAddr, void *l3PktBuf, struct mCastProxyConfig *mCastProxyPara, struct mcft_entry **groupList, int in_vif, int out_vif)
{
	int recId=0, grecNum=0, srcNum=0;
	unsigned char type=0;
	unsigned int *groupAddr=NULL;
	unsigned char *groupRecords=NULL;
	unsigned int *srcRecord=NULL;
	int valid_group = TRUE;

#if defined(CONFIG_IGMPV3_SUPPORT)
	grecNum = ntohs(((struct igmpv3_report *)l3PktBuf)->ngrec);
	if(grecNum>0)
		groupRecords = (unsigned char *)(&(((struct igmpv3_report *)l3PktBuf)->grec[0]));
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
	grecNum = ntohs(((struct mld2_report *)l3PktBuf)->mld2r_ngrec);
	if(grecNum>0)
		groupRecords = (unsigned char *)(&(((struct mld2_report *)l3PktBuf)->mld2r_grec[0]));
#endif

	while(recId < grecNum)
	{
#if defined(CONFIG_IGMPV3_SUPPORT)
		srcNum=ntohs(((struct igmpv3_grec *)groupRecords)->grec_nsrcs);
		groupAddr = (unsigned int *)(&((struct igmpv3_grec *)groupRecords)->grec_mca);
		type = ((struct igmpv3_grec *)groupRecords)->grec_type;
		if(srcNum>0)
			srcRecord = (unsigned int *)(&(((struct igmpv3_grec *)groupRecords)->grec_src[0]));
		else
			srcRecord = NULL;
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
		srcNum=ntohs(((struct mld2_grec *)groupRecords)->grec_nsrcs);
		groupAddr = (unsigned int *)(&(((struct mld2_grec *)groupRecords)->grec_mca));
		type = ((struct mld2_grec *)groupRecords)->grec_type;
		if(srcNum>0)
			srcRecord = (unsigned int*)(&(((struct mld2_grec *)groupRecords)->grec_src[0]));
		else
			srcRecord = NULL;
#endif	
		valid_group = check_group_validity(groupAddr, mCastProxyPara->ipVersion);

		if(valid_group==FALSE)
			goto next_record;

		switch(type)
		{
			case MODE_IS_INCLUDE:
				rtl_handleIgmpv3Mldv2isIn(groupAddr, clientAddr, srcNum, srcRecord, mCastProxyPara, groupList, in_vif, out_vif);
				break;
			case MODE_IS_EXCLUDE:
				rtl_handleIgmpv3Mldv2isEx(groupAddr, clientAddr, srcNum, srcRecord, mCastProxyPara, groupList, in_vif, out_vif);
				break;
			case CHANGE_TO_INCLUDE: 
				rtl_handleIgmpv3Mldv2toIn(groupAddr, clientAddr, srcNum, srcRecord, mCastProxyPara, groupList, in_vif, out_vif);
				break;
			case CHANGE_TO_EXCLUDE: 
				rtl_handleIgmpv3Mldv2toEx(groupAddr, clientAddr, srcNum, srcRecord, mCastProxyPara, groupList, in_vif, out_vif);
				break;
			case ALLOW_NEW_SOURCES:
				rtl_handleIgmpv3Mldv2Allow(groupAddr, clientAddr, srcNum, srcRecord, mCastProxyPara, groupList, in_vif, out_vif);
				break;
			case BLOCK_OLD_SOURCES:
				rtl_handleIgmpv3Mldv2Block(groupAddr, clientAddr, srcNum, srcRecord, mCastProxyPara, groupList, in_vif, out_vif);
				break;
			default:
				
				DBG_IGMP_MLD_PRK(IGMP_RECEIVE, "receive a igmpv3/mldv2 report with unknown type\n");
				break;
		}

	next_record:		
		recId++;
		/*shift pointer to another group record*/
#if defined(CONFIG_IGMPV3_SUPPORT)
		groupRecords=groupRecords+sizeof(struct igmpv3_grec)+(srcNum+((struct igmpv3_grec *)groupRecords)->grec_auxwords)*sizeof(unsigned int);
#endif
#if defined(CONFIG_MLDV2_SUPPORT)
		groupRecords=groupRecords+sizeof(struct mld2_grec)+(srcNum*4+((struct mld2_grec *)(groupRecords))->grec_auxwords)*sizeof(unsigned int);
#endif
	}
	return SUCCESS;
}
