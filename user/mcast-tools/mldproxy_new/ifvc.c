/*
**  smcroute - static multicast routing control 
**  Copyright (C) 2001 Carsten Schill <carsten@cschill.de>
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
**  $Id: ifvc.c,v 1.1.1.1 2007-08-06 10:04:43 root Exp $	
**
**  This module manages an interface vector of the machine
**
*/

#include "mclab.h"
#include <linux/sockios.h>
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
#include <signal.h>
#endif

#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
#define  MAXWAN 4
#elif defined(CONFIG_MLDPROXY_MULTIWAN)
#define  MAXWAN 8
#else
#define  MAXWAN 1
#endif

struct IfDesc IfDescVc[ MAX_IF ], *IfDescEp = IfDescVc;

#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
extern int mld_signo;
sigset_t mld_newmask, mld_oldmask, mld_pendmask;
extern int mld_if_sock_old[MAX_IF];
extern int mld_vif_old[MAX_IF];
#endif

extern char mld_down_if_name[IFNAMSIZ];

#if defined(CONFIG_MLDPROXY_MULTIWAN) || defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
extern char mld_up_if_name[MAXWAN][IFNAMSIZ];
extern int mld_up_if_num;
#else
extern char mld_up_if_name[IFNAMSIZ];
#endif

#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
//add name if necessary, update Flags, pif_index, local addr,
//return 1 --> get successfully
int getMldIfInfo(const char *IfName, struct IfDesc * Ifd)
{
	struct sockaddr_in6 addr;
	FILE			*file;
	unsigned int		prefixlen, scope, flags, ifindex;
	char			devname[IFNAMSIZ];
	int retVal = 0;

	if(strcmp(Ifd->Name, "")!=0 && strcmp(Ifd->Name, IfName)!=0)
		return 0;

	file = fopen("/proc/net/if_inet6", "r");
	
	/* We can live without it though */
	if (!file)
	{
		log(LOG_ERR, errno, "exist!Couldn't open /proc/net/if_inet6\n\n");
		//log_msg(LOG_ERR, errno, "Couldn't open /proc/net/if_inet6\n");
		return 0;
	}


		
	while((fscanf( file,
				"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx %x %x %x %x %8s",
				&addr.sin6_addr.s6_addr[ 0], &addr.sin6_addr.s6_addr[ 1], &addr.sin6_addr.s6_addr[ 2], &addr.sin6_addr.s6_addr[ 3],
				&addr.sin6_addr.s6_addr[ 4], &addr.sin6_addr.s6_addr[ 5], &addr.sin6_addr.s6_addr[ 6], &addr.sin6_addr.s6_addr[ 7],
				&addr.sin6_addr.s6_addr[ 8], &addr.sin6_addr.s6_addr[ 9], &addr.sin6_addr.s6_addr[10], &addr.sin6_addr.s6_addr[11],
				&addr.sin6_addr.s6_addr[12], &addr.sin6_addr.s6_addr[13], &addr.sin6_addr.s6_addr[14], &addr.sin6_addr.s6_addr[15],			
				&ifindex, &prefixlen, &scope, &flags, devname)!=EOF))
	{
		if((strncmp(devname, IfName, IFNAMSIZ)==0)&&
		   (IN6_IS_ADDR_LINKLOCAL(addr.sin6_addr.s6_addr32))&&
		   ((flags&IFA_F_TENTATIVE)==0))
		{
			strncpy(Ifd->Name,devname,sizeof( Ifd->Name ));
			memcpy(&Ifd->InAdr,&addr.sin6_addr,sizeof(struct in6_addr));
			Ifd->Flags = flags;
			Ifd->pif_idx=ifindex;
			retVal = 1;
			break;
		}
	}

	fclose(file);
	return retVal;
	
}
//to be check
int getMldVifIndexbyIfName(char *IfName)
{
	int vifIndex = -1;

	if(!strncmp(IfName, "eth1", 4))
		vifIndex = 1;
	else if(!strncmp(IfName, "ppp1", 4))
		vifIndex = 2;
	else if(!strncmp(IfName, "eth8", 4))
		vifIndex = 3;
	else if(!strncmp(IfName, "ppp8", 4))
		vifIndex = 4;
	else if(!strncmp(IfName, "eth9", 4))
		vifIndex = 5;	
	else if(!strncmp(IfName, "ppp9", 4))
		vifIndex = 6;
	else if(!strncmp(IfName, "eth10", 5))
		vifIndex = 7;
	else if(!strncmp(IfName, "ppp10", 5))
		vifIndex = 8;

	return vifIndex;

}

/*
** sync IfDescVc with mld_down_if_name and mld_up_if name
** return 1: success
** return 0: fail
*/
int buildIfVc(void)
{
	int vcindex = 0, ifindex=0;
	struct IfDesc *Dp=NULL, *Ddp=NULL, *Udp=NULL;
	//block signal
	sigemptyset(&mld_newmask);
	sigaddset(&mld_newmask, mld_signo);
	if(sigprocmask(SIG_BLOCK, &mld_newmask, &mld_oldmask)<0)
	{
		log(LOG_WARNING, errno, "signal block error!\n");
		return 0;
	}

	/*delete interface in IfDescVc but not up/down inf to avoid IfDescVc overflow
	**sock not changed here, record old sock and vif
	*/	
	for(vcindex=0, Dp=IfDescVc; vcindex<MAX_IF; vcindex++, Dp++)
	{
		if(strcmp(Dp->Name, "") == 0)
		{
			mld_if_sock_old[vcindex] = 0;
			mld_vif_old[vcindex] = -1;
			continue;
		}

		mld_if_sock_old[vcindex] = Dp->sock;
		mld_vif_old[vcindex] = Dp->vif_idx;
		
		for(ifindex=0; ifindex<MAXWAN; ifindex++)
		{
			if(strcmp(Dp->Name, mld_up_if_name[ifindex])==0)
				break;
		}

		if(ifindex==MAXWAN && strcmp(Dp->Name, mld_down_if_name)!=0)
		{
			close(Dp->sock);
			memset(Dp, 0, sizeof(struct IfDesc));
		}
	}

	//down if
	Ddp = getIfByName(mld_down_if_name);
	if(Ddp)
	{
		//already exist, update it	
	}
	else
	{
		//not exist, find empty IfDesc
		for(vcindex=0, Dp=IfDescVc; vcindex<MAX_IF; vcindex++, Dp++)
		{
		  	if(strcmp(Dp->Name, "") == 0)
		  	{
				Ddp = Dp;
				break;
		  	}
		}
	}

	if(Ddp)
	{	
		getMldIfInfo(mld_down_if_name, Ddp);
	}
	else
	{
		log(LOG_WARNING, 0, "too much interface!\n");
		return 0;
	}
	//fixed vif index
	Ddp->vif_idx = 0;
	
	for(ifindex=0;ifindex<MAXWAN;ifindex++)
 	{
		if(strcmp(mld_up_if_name[ifindex], "") == 0)
			continue;

		Udp = getIfByName(mld_up_if_name[ifindex]);
		if(Udp)
		{
			//already exist, update it
		}
		else
		{
			for(vcindex=0, Dp=IfDescVc; vcindex<MAX_IF; vcindex++, Dp++)
			{
				if(strcmp(Dp->Name, "") == 0)
				{
					Udp = Dp;
					break;
				}
			}
		}
		
		if(Udp)
		{	
			getMldIfInfo(mld_up_if_name[ifindex], Udp);
		}
		else
		{
			log(LOG_WARNING, 0,"too much interface!\n");
			return 0;
		}

		/*fixed vif index*/
		Udp->vif_idx = getMldVifIndexbyIfName(mld_up_if_name[ifindex]);

	}
	//unblock signal
	if(sigprocmask(SIG_SETMASK, &mld_oldmask, NULL)<0)
	{
		log(LOG_WARNING, errno, "sig set mask error!\n");
		return 0;
	}
	return 1;
}

/*
** Returns a pointer to the IfDesc of the interface 'IfName'
**
** returns: - pointer to the IfDesc of the requested interface
**          - NULL if no interface 'IfName' exists
**          
*/
struct IfDesc *getIfByName( const char *IfName)
{
	struct IfDesc *Dp=NULL;
	int i;
	
	for(i=0, Dp=IfDescVc; i<MAX_IF; i++, Dp++)
	{
		if(strcmp(IfName, Dp->Name)==0)
		{
			//printf("Find dp:name:%s,vif:%d,[%s]:[%d].\n",Dp->Name,(Dp-IfDescVc),__FUNCTION__,__LINE__);
			return Dp;
		}
	}
		
	log(LOG_DEBUG, 0, "[%s]:[%d].failed! IfName:%s\n\n",__FUNCTION__,__LINE__,IfName);
	return NULL;
}

/*
** Returns a pointer to the IfDesc of the interface 'IfName'
**
** returns: - pointer to the IfDesc of the requested interface
**          - NULL if no interface 'Pix' exists
**          
*/
struct IfDesc *getIfByPix( int pix)
{
	struct IfDesc *Dp=NULL;
	int i;
	
	for(i=0, Dp=IfDescVc; i<MAX_IF; i++, Dp++)
	{
		if(pix==((int)Dp->pif_idx))  
		{
			//printf("Find dp:name:%s,vif:%d,[%s]:[%d].\n",Dp->Name,(Dp-IfDescVc),__FUNCTION__,__LINE__);
			return Dp;
		}
	}
		
	log(LOG_DEBUG, 0, "[%s]:[%d].failed! Pix:%d\n\n",__FUNCTION__,__LINE__,pix);
	return NULL;
}

/*
** Returns a pointer to the IfDesc of the interface 'Ix'
**
** returns: - pointer to the IfDesc of the requested interface
**          - NULL if no interface 'Ix' exists
**          
*/
struct IfDesc *getIfByIx( unsigned Ix )
{
	struct IfDesc *Dp=NULL;

	if(Ix>=MAX_IF)
		return NULL;

	Dp = &IfDescVc[ Ix ];
	return Dp;
}

/*
** return 1: is local address
**		0: is not
**          
*/
int chk_local( struct in6_addr addr )
{
	struct IfDesc *Dp;
	int i;
	
	for(i=0, Dp=IfDescVc; i<MAX_IF; i++, Dp++)
	{
	  	if( !memcmp(&(Dp->InAdr),&(addr),sizeof(struct in6_addr)))
  		return 1;
	}
	return 0;
}

#else
void initIfDesc(void)
{
	int i = 0;
	memset(IfDescVc, 0, sizeof( IfDescVc ));
	for(i=0; i<MAX_IF; i++)
	{
		IfDescVc[i].vif_idx = NO_VIF;
	}
/*	Due to VifDescVc array will reference IfDescVc array, 
 *	when /proc/net/if_inet6 interface order change,it will got wrong interface information,
 *	so we fix interface index 
 */
#if defined(CONFIG_MLDPROXY_MULTIWAN)
	i=0;
	strncpy(IfDescVc[i].Name, mld_down_if_name,sizeof( IfDescVc[i].Name ));

	for(i=0; i<mld_up_if_num; i++){
		strncpy(IfDescVc[i+1].Name, mld_up_if_name[i],sizeof( IfDescVc[i+1].Name ));
	}
#else
	strncpy(IfDescVc[0].Name, mld_down_if_name,sizeof( IfDescVc[0].Name ));
	strncpy(IfDescVc[1].Name, mld_up_if_name,sizeof( IfDescVc[1].Name ));
#endif
}

int buildIfVc(void)
/*
** Builds up a vector with the interface of the machine. Calls to the other functions of 
** the module will fail if they are called before the vector is build.
**          
*/
{
	int i=0;
	struct sockaddr_in6 addr;
	FILE *file=NULL;
	unsigned int prefixlen=0, scope=0, flags=0, ifindex = 0;
	char devname[IFNAMSIZ]={0};
	char FmtBu[128]={0};
	file = fopen("/proc/net/if_inet6", "r");

	/* We can live without it though */
	if (!file)
	{
		log(LOG_ERR, errno,"error!Couldn't open /proc/net/if_inet6\n\n");
		//log_msg(LOG_ERR, errno, "Couldn't open /proc/net/if_inet6\n");
		return 0;
	}

	IfDescEp = IfDescVc;
			
	/*
	 * Loop through all of the interfaces.
	 */
	while ((fscanf( file,
			"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx %x %x %x %x %8s",
			&addr.sin6_addr.s6_addr[ 0], &addr.sin6_addr.s6_addr[ 1], &addr.sin6_addr.s6_addr[ 2], &addr.sin6_addr.s6_addr[ 3],
			&addr.sin6_addr.s6_addr[ 4], &addr.sin6_addr.s6_addr[ 5], &addr.sin6_addr.s6_addr[ 6], &addr.sin6_addr.s6_addr[ 7],
			&addr.sin6_addr.s6_addr[ 8], &addr.sin6_addr.s6_addr[ 9], &addr.sin6_addr.s6_addr[10], &addr.sin6_addr.s6_addr[11],
			&addr.sin6_addr.s6_addr[12], &addr.sin6_addr.s6_addr[13], &addr.sin6_addr.s6_addr[14], &addr.sin6_addr.s6_addr[15],			
			&ifindex, &prefixlen, &scope, &flags, devname)!=EOF)){
#if 0
			printf("ifa->name:%s\n",devname);
			printf( "ifindex:%x,prefixlen:%x,scope:%x,flags:%x\n",ifindex,prefixlen,scope,flags);
			printf( "addr:%08x:%08x:%08x:%08x\n",
				addr.sin6_addr.s6_addr32[0],addr.sin6_addr.s6_addr32[1],addr.sin6_addr.s6_addr32[2],addr.sin6_addr.s6_addr32[ 3]);
#endif

		/* get If vector
		**  RFC 2710 mandates the use of a link-local IPv6 source address for the transmission of MLD message
		**  so we just need to record the link-local address
		*/
		if (IN6_IS_ADDR_LINKLOCAL(addr.sin6_addr.s6_addr32))
		{
			if(flags&IFA_F_TENTATIVE){
				log(LOG_WARNING, 0,"%s IFA_F_TENTATIVE is set!\n",devname);
				//if ((strcmp(devname,"eth1")==0)||(strcmp(devname,"br0")==0))
				if (strcmp(devname,mld_down_if_name)==0)
                {
                    fclose(file);
					return 0;
                }
				#ifndef CONFIG_MLDPROXY_MULTIWAN
				if (strcmp(devname,mld_up_if_name)==0)
                {
                    fclose(file);
					return 0;
                }
				#else
				for(i=0;i<MAXWAN;i++){
					if (strcmp(devname,mld_up_if_name[i])==0)
                    {
                        fclose(file);
						return 0;
                    }
				}
				#endif
			}

			for( i=0; i<MAX_IF; i++ )
			{
				IfDescEp=IfDescVc+i;
			
				if (IfDescEp->Name[0] && !strcmp(IfDescEp->Name, devname)){
					memcpy(&IfDescEp->InAdr,&addr.sin6_addr,sizeof(struct in6_addr));
					IfDescEp->Flags = flags;
					IfDescEp->pif_idx=ifindex;
					IfDescEp->vif_idx = getVifIx(IfDescEp);
#if 0
					printf("buildIfVc: Interface %s pif_idx:%d, Flags: 0x%04x",
						   IfDescEp->Name,
						   IfDescEp->pif_idx,
						  IfDescEp->Flags );
					printf( "addr:%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
					IfDescEp->InAdr.s6_addr16[ 0],IfDescEp->InAdr.s6_addr16[ 1],IfDescEp->InAdr.s6_addr16[ 2],
					IfDescEp->InAdr.s6_addr16[ 3],IfDescEp->InAdr.s6_addr16[ 4],IfDescEp->InAdr.s6_addr16[ 5],
					IfDescEp->InAdr.s6_addr16[ 6],IfDescEp->InAdr.s6_addr16[ 7]);
#endif 
					break;
				}
			}
		}
	}

	fclose(file);
	return 1;
}


struct IfDesc *getIfByName( const char *IfName )
/*
** Returns a pointer to the IfDesc of the interface 'IfName'
**
** returns: - pointer to the IfDesc of the requested interface
**          - NULL if no interface 'IfName' exists
**          
*/
{
	struct IfDesc *Dp=NULL;
	int i;
	for( i=0; i<MAX_IF; i++ ) {
		Dp=IfDescVc+i;
		if( ! strcmp( IfName, Dp->Name ) ) {
			//printf("Find dp:name:%s,vif:%d,[%s]:[%d].\n",Dp->Name,Dp->vif_idx,__FUNCTION__,__LINE__);
			return Dp;
		}
	}
		
	log(LOG_DEBUG, 0,"[%s]:[%d].failed! IfName:%s\n\n",__FUNCTION__,__LINE__,IfName);
	return NULL;
}
struct IfDesc *getIfByPix( int pix)
/*
** Returns a pointer to the IfDesc of the interface 'IfName'
**
** returns: - pointer to the IfDesc of the requested interface
**          - NULL if no interface 'Pix' exists
**          
*/
{
	struct IfDesc *Dp=NULL;
	int i;
	for( i=0; i<MAX_IF; i++ ) {
		Dp=IfDescVc+i;
		
		log(LOG_DEBUG,"Find dp:name:%s,vif:%d, pif_idx=%d,[%s]:[%d].\n",Dp->Name,(Dp->vif_idx), Dp->pif_idx,__FUNCTION__,__LINE__);

		if( pix==((int)Dp->pif_idx))  {
			log(LOG_DEBUG,"Find dp:name:%s,vif:%d,[%s]:[%d].\n",Dp->Name,(Dp->vif_idx),__FUNCTION__,__LINE__);
			return Dp;
		}
	}
		
	log( LOG_DEBUG, 0,"[%s]:[%d].failed! Pix:%d\n\n",__FUNCTION__,__LINE__,pix);
	return NULL;
}

struct IfDesc *getIfbyVif(int vif)
{
	struct IfDesc *Dp=NULL;
	int i;
	for( i=0; i<MAX_IF; i++ ) 
	{
		Dp=IfDescVc+i;
		
		log(LOG_DEBUG,"Find dp:name:%s,vif:%d,[%s]:[%d].\n",Dp->Name,Dp->vif_idx,__FUNCTION__,__LINE__);
		if(vif ==((int)Dp->vif_idx))  
		{
			log(LOG_DEBUG,"Find dp:name:%s,vif:%d,[%s]:[%d].\n",Dp->Name,Dp->vif_idx,__FUNCTION__,__LINE__);
			return Dp;
		}
	}
		
	log( LOG_DEBUG, 0,"[%s]:[%d].failed! vif:%d\n\n",__FUNCTION__,__LINE__,vif);
	return NULL;
}

struct IfDesc *getIfByIx( unsigned Ix )
/*
** Returns a pointer to the IfDesc of the interface 'Ix'
**
** returns: - pointer to the IfDesc of the requested interface
**          - NULL if no interface 'Ix' exists
**          
*/
{
	struct IfDesc *Dp = &IfDescVc[ Ix ];
	return Dp < IfDescEp ? Dp : NULL;
}


int chk_local( struct in6_addr addr )
/*
** Returns a pointer to the IfDesc of the interface 'IfName'
**
** returns: - pointer to the IfDesc of the requested interface
**          - NULL if no interface 'IfName' exists
**          
*/
{
	struct IfDesc *Dp;
	int i;
	for(i=0; i<MAX_IF; i++) 
	{
		Dp=IfDescVc+i;
  		if( !memcmp(&(Dp->InAdr),&(addr),sizeof(struct in6_addr) ))
  			return 1;
	}

	return 0;
}

#if defined(CONFIG_MLDPROXY_MULTIWAN)
int getOutVif(int in_vif, int out_index)
#else
int getOutVif(int in_vif)
#endif
{
	int outvif = NO_VIF;
#if defined(CONFIG_MLDPROXY_MULTIWAN)
	int i=0;
#endif
	
	if(in_vif==NO_VIF)
		return NO_VIF;

#if defined(CONFIG_MLDPROXY_MULTIWAN)
	if(in_vif == mld_down_if_idx){
		// Report from LAN
		return mld_up_if_idx[out_index];
	}else{
		for(i=0; i<mld_up_if_num;i++){
			if(in_vif == mld_up_if_idx[i]){
				// Report from WAN
				return mld_down_if_idx;
			}
		}
	}

	return NO_VIF;
#else
	if(in_vif == mld_up_if_idx)
		outvif = mld_down_if_idx;
	else if(in_vif == mld_down_if_idx)
		outvif = mld_up_if_idx;

	return outvif;
#endif
}
#endif
