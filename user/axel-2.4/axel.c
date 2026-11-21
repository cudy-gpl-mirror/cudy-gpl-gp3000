  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* Main control								*/

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License with
  the Debian GNU/Linux distribution in file /usr/doc/copyright/GPL;
  if not, write to the Free Software Foundation, Inc., 59 Temple Place,
  Suite 330, Boston, MA  02111-1307  USA
*/

#include "axel.h"
#include <netinet/tcp.h>
#include <math.h>

SSL_CTX *g_ssl_ctx = NULL;
int      g_use_ssl = 0;

/* Axel */
static void save_state( axel_t *axel );
static void *setup_thread( void * );
static void axel_message( axel_t *axel, char *format, ... );
static void axel_divide( axel_t *axel );

#ifdef CONFIG_RTK_HOST_SPEEDUP
void setSpeedupInfo( axel_t *axel );
#endif

static char *buffer = NULL;

/* Create a new axel_t structure					*/
axel_t *axel_new( conf_t *conf, int count, void *url )
{
    SSL_library_init();
    g_ssl_ctx = SSL_CTX_new(SSLv23_client_method());

	search_t *res;
	axel_t *axel;
	url_t *u;
	char *s;
	int i;
	int err = 0;

	axel = malloc( sizeof( axel_t ) );
	memset( axel, 0, sizeof( axel_t ) );
	*axel->conf = *conf;
	axel->conn = malloc( sizeof( conn_t ) * axel->conf->num_connections );
	memset( axel->conn, 0, sizeof( conn_t ) * axel->conf->num_connections );
	if( axel->conf->max_speed > 0 )
	{
		if( (float) axel->conf->max_speed / axel->conf->buffer_size < 0.5 )
		{
			if( axel->conf->verbose >= 2 )
				axel_message( axel, _("Buffer resized for this speed.") );
			axel->conf->buffer_size = axel->conf->max_speed;
		}
		axel->delay_time = (int) ( (float) 1000000 / axel->conf->max_speed * axel->conf->buffer_size * axel->conf->num_connections );
	}
	if( buffer == NULL )
		buffer = malloc( max( MAX_STRING, axel->conf->buffer_size ) );

	if( count == 0 )
	{
		axel->url = malloc( sizeof( url_t ) );
		axel->url->next = axel->url;
		strncpy( axel->url->text, (char *) url, MAX_STRING );
	}
	else
	{
		res = (search_t *) url;
		u = axel->url = malloc( sizeof( url_t ) );
		for( i = 0; i < count; i ++ )
		{
			strncpy( u->text, res[i].url, MAX_STRING );
			u->valid = 1;
			if( i < count - 1 )
			{
				u->next = malloc( sizeof( url_t ) );
				u = u->next;
			}
			else
			{
				u->next = axel->url;
			}
		}
	}

	axel->error = eTR143_None;
	axel->conn[0].conf = axel->conf;

	/* QL 20180417 when more url is set for downloading, then we should search a valid url to get reasonable result */
	axel->conn[0].local_if = axel->conf->interfaces->text;
	axel->conf->interfaces = axel->conf->interfaces->next;
	u = axel->url;
	do {
		err = 0;
		
		if( !conn_set( &axel->conn[0], u->text ) )
		{
			axel_message( axel, _("Could not parse URL.\n") );
			//axel->ready = -1;
			err = eTR143_Error_InitConnectionFailed;
			axel->error = eTR143_Error_InitConnectionFailed;
			//return( axel );
			/* check next url for more valid info */
			fprintf(stderr, "%s %d url %s is invalid!\n", __func__, __LINE__, u->text);
			u->valid = 0;
			u = u->next;
			continue;
		}

		/* QL I dont want to modify interfaces info for next url, so move below code before do...while statement */
		//axel->conn[0].local_if = axel->conf->interfaces->text;
		//axel->conf->interfaces = axel->conf->interfaces->next;

		strncpy( axel->filename, axel->conn[0].file, MAX_STRING );
		http_decode( axel->filename );
		if( *axel->filename == 0 )	/* Index page == no fn		*/
			strncpy( axel->filename, axel->conf->default_filename, MAX_STRING );
		if( ( s = strchr( axel->filename, '?' ) ) != NULL && axel->conf->strip_cgi_parameters )
			*s = 0;		/* Get rid of CGI parameters		*/
#if 0//def CONFIG_RTK_HOST_SPEEDUP
		if(axel->conf->speedtest)
		{
			axel->conn->supported = 1;
			axel->conn->size = INT_MAX;
		}
		else
		{
#endif
		if( !conn_init( &axel->conn[0] ) )
		{
			axel_message( axel, axel->conn[0].message );
			axel->error = eTR143_Error_InitConnectionFailed;
			err = eTR143_Error_InitConnectionFailed;
			//axel->ready = -1;
			//return( axel );
			fprintf(stderr, "%s %d url %s is invalid!\n", __func__, __LINE__, u->text);
			u->valid = 0;
			u = u->next;
			continue;
		}

		/* This does more than just checking the file size, it all depends
		   on the protocol used.					*/
		if( !conn_info( &axel->conn[0] ) )
		{
			axel_message( axel, axel->conn[0].message );
		}
#if 0//def CONFIG_RTK_HOST_SPEEDUP
		}
#endif

		s = conn_url( axel->conn );
		strncpy( u->text, s, MAX_STRING );
		if( ( axel->size = axel->conn[0].size ) != INT_MAX )
		{
			if( axel->conf->verbose > 0 )
				axel_message( axel, _("File size: %lld bytes"), axel->size );
		}

		/* Wildcards in URL --> Get complete filename			*/
		if( strchr( axel->filename, '*' ) || strchr( axel->filename, '?' ) )
			strncpy( axel->filename, axel->conn[0].file, MAX_STRING );
		
		break;
	} while (u != axel->url);

	if (err != 0)
	{
		fprintf(stderr, "%s fail!\n", __func__);
		axel->ready = -1;
		return( axel );
	}
	
	return( axel );
}

/* Open a local file to store the downloaded data			*/
int axel_open( axel_t *axel )
{
	int i, fd;
	long long int j;

	if( axel->conf->verbose > 0 )
		axel_message( axel, _("Opening output file %s"), axel->filename );
	snprintf( buffer, MAX_STRING, "%s.st", axel->filename );

	axel->outfd = -1;
#if 0//def CONFIG_RTK_HOST_SPEEDUP
	/* when file is small, single thread is better for speedtest, while multi-thread is better for great file download */
	if (axel->conf->speedtest && (axel->size < (200*1024*1024)))
		axel->conf->num_connections = 1;
#endif
#ifdef CONFIG_RTK_HOST_SPEEDUP
	//always multiple thread download for host speedtest
	if(axel->conf->speedtest)
		axel->conn[0].supported = 1;
#endif
	/* Check whether server knows about RESTart and switch back to
	   single connection download if necessary			*/
	if( !axel->conn[0].supported )
	{
		axel_message( axel, _("Server unsupported, "
			"starting from scratch with one connection.") );
		fprintf(stderr,"Server unsupported, "
			"starting from scratch with one connection.");
		axel->conf->num_connections = 1;
		axel->conn = realloc( axel->conn, sizeof( conn_t ) );
		axel_divide( axel );
	}
	else if( ( fd = open( buffer, O_RDONLY ) ) != -1 )
	{
		read( fd, &axel->conf->num_connections, sizeof( axel->conf->num_connections ) );

		axel->conn = realloc( axel->conn, sizeof( conn_t ) * axel->conf->num_connections );
		memset( axel->conn + 1, 0, sizeof( conn_t ) * ( axel->conf->num_connections - 1 ) );

		axel_divide( axel );

		read( fd, &axel->bytes_done, sizeof( axel->bytes_done ) );
		for( i = 0; i < axel->conf->num_connections; i ++ )
			read( fd, &axel->conn[i].currentbyte, sizeof( axel->conn[i].currentbyte ) );

		axel_message( axel, _("State file found: %lld bytes downloaded, %lld to go."),
			axel->bytes_done, axel->size - axel->bytes_done );

		close( fd );

		if( ( axel->outfd = open( axel->filename, O_WRONLY, 0666 ) ) == -1 )
		{
			axel_message( axel, _("Error opening local file") );
			return( 0 );
		}
	}

	/* If outfd == -1 we have to start from scrath now		*/
	if( axel->outfd == -1 )
	{
		axel_divide( axel );

		if( ( axel->outfd = open( axel->filename, O_CREAT | O_WRONLY, 0666 ) ) == -1 )
		{
			axel_message( axel, _("Error opening local file") );
			return( 0 );
		}

        if (axel->conf->speedtest)//while do speed test, dont write any real data to output file
    	{
    		//setSpeedupInfo(axel);
            goto DONE;
    	}

		/* And check whether the filesystem can handle seeks to
		   past-EOF areas.. Speeds things up. :) AFAIK this
		   should just not happen:				*/
		if( lseek( axel->outfd, axel->size, SEEK_SET ) == -1 && axel->conf->num_connections > 1 )
		{
			/* But if the OS/fs does not allow to seek behind
			   EOF, we have to fill the file with zeroes before
			   starting. Slow..				*/
			axel_message( axel, _("Crappy filesystem/OS.. Working around. :-(") );
			lseek( axel->outfd, 0, SEEK_SET );
			memset( buffer, 0, axel->conf->buffer_size );
			j = axel->size;
			while( j > 0 )
			{
				write( axel->outfd, buffer, min( j, axel->conf->buffer_size ) );
				j -= axel->conf->buffer_size;
			}
		}
	}

DONE:
	return( 1 );
}

/* Start downloading							*/
void axel_start( axel_t *axel )
{
	url_t *u;
	int i;

	/* HTTP might've redirected and FTP handles wildcards, so
	   re-scan the URL for every conn				*/
	for( i = 0; i < axel->conf->num_connections; i ++ )
	{
		u = axel->url;
		do {
			if (u->valid)
				break;
			u = u->next;
		} while (u != axel->url);
		fprintf(stderr,"pass url:%s\n",u->text);
		conn_set( &axel->conn[i], u->text );
		axel->url = u->next;
		axel->conn[i].local_if = axel->conf->interfaces->text;
		axel->conf->interfaces = axel->conf->interfaces->next;
		axel->conn[i].conf = axel->conf;
		if( i ) axel->conn[i].supported = 1;
	}

	if( axel->conf->verbose > 0 )
		axel_message( axel, _("Starting download") );

	for( i = 0; i < axel->conf->num_connections; i ++ )
	if( axel->conn[i].currentbyte <= axel->conn[i].lastbyte )
	{
		if( axel->conf->verbose >= 2 )
		{
			axel_message( axel, _("Connection %i downloading from %s:%i using interface %s"),
		        	      i, axel->conn[i].host, axel->conn[i].port, axel->conn[i].local_if );
		}

		axel->conn[i].state = 1;
		if( pthread_create( axel->conn[i].setup_thread, NULL, setup_thread, &axel->conn[i] ) != 0 )
		{
			axel_message( axel, _("pthread error!!!") );
			axel->ready = -1;
		}
		else
		{
#if (CONFIG_NR_CPUS > 1)
			cpu_set_t set;
			int cpu = (i%2)+1;

			CPU_ZERO(&set);
			CPU_SET(cpu, &set);	// run on cpu1 and cpu2
			if (pthread_setaffinity_np(*axel->conn[i].setup_thread, sizeof(set), &set) < 0)
				fprintf(stderr, "pthread_setaffinity_np on connection %d failed\n", i);

//			pthread_getaffinity_np(*axel->conn[i].setup_thread, sizeof(set), &set);
//			axel_message(axel, "Setting on connection %d tid %lu on cpu %lu, is_set=%d", i, *axel->conn[i].setup_thread, cpu, CPU_ISSET(cpu, &set));
#endif
			axel->conn[i].last_transfer = gettime();
		}
	}

	/* The real downloading will start now, so let's start counting	*/
	axel->start_time = gettime();
	get_total_wan_stats(NULL, &axel->total_bytes_start);
	axel->ready = 0;
}

#ifdef CONFIG_RTK_HOST_SPEEDUP
#include "rtk/ip_speedup.h"

static pthread_mutex_t speed_lock = PTHREAD_MUTEX_INITIALIZER;

unsigned int getRecvDataLen(conn_t *conn)
{
	struct sockaddr_in saddr, daddr;
	struct sockaddr_in6 saddr6, daddr6;
	socklen_t sock_len = sizeof(struct sockaddr_in);
	int fd1;
	struct speedtest_arg pass_arg;
	int sock_familly = AF_INET;
	socklen_t len = sizeof(sock_familly);

	if(conn->ip_familly == 0)
	{
		if(getsockopt(conn->fd, SOL_SOCKET, SO_DOMAIN, &sock_familly, &len) < 0)
			return 0;
		conn->ip_familly = sock_familly;
	}

	if (0 == conn->sip[0])
	{
		if(sock_familly == AF_INET)
		{
			sock_len = sizeof(saddr);
			if (getsockname(conn->fd, (struct sockaddr*) &saddr, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getsockname fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->sip, &(saddr.sin_addr), IP_ADDR_LEN);
			conn->sport = saddr.sin_port;
		}
		else if(sock_familly == AF_INET6)
		{
			sock_len = sizeof(saddr6);
			if (getsockname(conn->fd, (struct sockaddr*) &saddr6, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getsockname fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->sip, &(saddr6.sin6_addr), IP6_ADDR_LEN);
			conn->sport = saddr6.sin6_port;
		}
	}

	if (0 == conn->dip[0])
	{
		if(sock_familly == AF_INET)
		{
			sock_len = sizeof(daddr);
			if (getpeername(conn->fd, (struct sockaddr*) &daddr, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getpeername fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->dip, &(daddr.sin_addr), IP_ADDR_LEN);
			conn->dport = daddr.sin_port;
		}
		else if(sock_familly == AF_INET6)
		{
			sock_len = sizeof(daddr6);
			if (getpeername(conn->fd, (struct sockaddr*) &daddr6, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getpeername fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->dip, &(daddr6.sin6_addr), IP6_ADDR_LEN);
			conn->dport = daddr6.sin6_port;
		}
	}
	 
	pass_arg.cmd=1;
	memcpy(pass_arg.sip, conn->sip, IP6_ADDR_LEN);
	memcpy(pass_arg.dip, conn->dip, IP6_ADDR_LEN);
	pass_arg.sport = conn->sport;
	pass_arg.dport = conn->dport;
	pass_arg.ip_familly = conn->ip_familly;

	fd1 = open(DEV_SPEEDUP_PATH, O_RDWR);
  	if(fd1 < 0){
		fprintf(stderr, "Error!Socket create fail in ethctl.\n");
		return 0;
  	}

	if (ioctl(fd1, SIOCSPEEDTEST, &pass_arg) < 0) {
		fprintf(stderr, "Error ioctl for SIOCETHTEST\n");
		close(fd1);
		return 0;
	}
	close(fd1);
	//fprintf(stderr,"[%s %d]datalen:%u\n",__func__,__LINE__,pass_arg.ip_familly);
	return (unsigned int)pass_arg.ip_familly;

err:
	memset(conn->sip, 0, IP6_ADDR_LEN);
	memset(conn->dip, 0, IP6_ADDR_LEN);
	conn->sport = 0;
	conn->dport = 0;
	conn->ip_familly = 0;
	return 0;
}

#if defined(DO_ACL_CPU_ISOLATE_SPEEDUP)
int SreamRuleCPUID = 0;
#endif
void addSpeedupSreamRule(conn_t *conn)
{
	//FILE *fp = NULL;
	struct sockaddr_in saddr, daddr;
	struct sockaddr_in6 saddr6, daddr6;
	socklen_t sock_len = sizeof(struct sockaddr_in);
	char sipStr[48], dipStr[48];
	char cmdStr[1024];
	int sock_familly = AF_INET;
	socklen_t len = sizeof(sock_familly);
	struct tcp_info info;
	unsigned int snd_wscale, rcv_wscale;
	unsigned int snd_mss;
	
	pthread_mutex_lock(&speed_lock);
	if(conn->ip_familly == 0)
	{
		if(getsockopt(conn->fd, SOL_SOCKET, SO_DOMAIN, &sock_familly, &len) < 0)
			goto err;
		conn->ip_familly = sock_familly;
	}

	if (0 == conn->sip[0])
	{
		if(sock_familly == AF_INET)
		{
			sock_len = sizeof(saddr);
			if (getsockname(conn->fd, (struct sockaddr*) &saddr, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getsockname fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->sip, &(saddr.sin_addr), IP_ADDR_LEN);
			conn->sport = saddr.sin_port;
		}
		else if(sock_familly == AF_INET6)
		{
			sock_len = sizeof(saddr6);
			if (getsockname(conn->fd, (struct sockaddr*) &saddr6, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getsockname fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->sip, &(saddr6.sin6_addr), IP6_ADDR_LEN);
			conn->sport = saddr6.sin6_port;
		}
	}

	if (0 == conn->dip[0])
	{
		if(sock_familly == AF_INET)
		{
			sock_len = sizeof(daddr);
			if (getpeername(conn->fd, (struct sockaddr*) &daddr, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getpeername fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->dip, &(daddr.sin_addr), IP_ADDR_LEN);
			conn->dport = daddr.sin_port;
		}
		else if(sock_familly == AF_INET6)
		{
			sock_len = sizeof(daddr6);
			if (getpeername(conn->fd, (struct sockaddr*) &daddr6, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getpeername fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->dip, &(daddr6.sin6_addr), IP6_ADDR_LEN);
			conn->dport = daddr6.sin6_port;
		}
	}
	inet_ntop(conn->ip_familly,conn->sip, sipStr,sizeof(sipStr));
	inet_ntop(conn->ip_familly,conn->dip, dipStr,sizeof(dipStr));

	#if 0
	fp = fopen("/proc/HostSpeedUP-STREAM", "w");
	if (fp) {
		//fprintf(stderr, "add sip %s sport %d dip %s dport %d\n", sipStr, conn->sport, dipStr, conn->dport);
		fprintf(fp, "add sip %s sport %d dip %s dport %d\n", sipStr, conn->sport, dipStr, conn->dport);
		fclose(fp);
	}
	#endif
	
	len = sizeof(info);
	memset(&info, 0, sizeof(info));
	getsockopt(conn->fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
	snd_wscale = pow(2, info.tcpi_snd_wscale);
	rcv_wscale = pow(2, info.tcpi_rcv_wscale);
	snd_mss = info.tcpi_snd_mss;

	if(conn->local_if)
		snprintf(cmdStr, 1024, "echo \"add intf %s sip %s sport %d dip %s dport %d snd_wscale %u rcv_wscale %u snd_mss %u\" > /proc/HostSpeedUP-STREAM", 
				conn->local_if, sipStr, ntohs(conn->sport), dipStr, ntohs(conn->dport), snd_wscale, rcv_wscale, snd_mss);
	else
		snprintf(cmdStr, 1024, "echo \"add sip %s sport %d dip %s dport %d snd_wscale %u rcv_wscale %u snd_mss %u\" > /proc/HostSpeedUP-STREAM", 
			sipStr, ntohs(conn->sport), dipStr, ntohs(conn->dport), snd_wscale, rcv_wscale, snd_mss);
	fprintf(stderr, "%s\n", cmdStr);
	system(cmdStr);

#ifdef CONFIG_USER_CUSPEEDTEST
	if(isIPv6Addr(sipStr)){
		write_wan_ipv6_addr(sipStr);
	}
#endif	

#if defined(DO_ACL_CPU_ISOLATE_SPEEDUP)
	int aclIdx = 0, ret = 0;
	FILE *fp = NULL;

	fp = fopen(RTK_AXEL_RULE_ACL_FILE, "a");
	if (fp)
	{
		rt_acl_filterAndQos_t aclRule;
		memset(&aclRule, 0, sizeof(rt_acl_filterAndQos_t));
		aclRule.acl_weight = 65535;

		aclRule.filter_fields |= RT_ACL_INGRESS_PORT_MASK_BIT;
		aclRule.ingress_port_mask = rtk_port_get_wan_phyMask();

		aclRule.filter_fields |= RT_ACL_INGRESS_L4_TCP_BIT;

		if (sock_familly == AF_INET)
		{
			struct in_addr sin_addr_s = {0}, sin_addr_c = {0};

			aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_SIP_RANGE_BIT;
			inet_pton(AF_INET, dipStr, &sin_addr_s);
			aclRule.ingress_src_ipv4_addr_start = aclRule.ingress_src_ipv4_addr_end = ntohl(sin_addr_s.s_addr);

			aclRule.filter_fields |= RT_ACL_INGRESS_IPV4_DIP_RANGE_BIT;
			inet_pton(AF_INET, sipStr, &sin_addr_c);
			aclRule.ingress_dest_ipv4_addr_start = aclRule.ingress_dest_ipv4_addr_end = ntohl(sin_addr_c.s_addr);
		}
		else if(sock_familly == AF_INET6)
		{
			unsigned char ip6_addr_s[IP6_ADDR_LEN] = {0}, ip6_addr_c[IP6_ADDR_LEN] = {0}, ip6_mask[IP6_ADDR_LEN] = {0};

			inet_pton(PF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", (void *)ip6_mask);

			aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_SIP_BIT;
			inet_pton(PF_INET6, dipStr, (void *)ip6_addr_s);
			memcpy(aclRule.ingress_src_ipv6_addr, ip6_addr_s, IPV6_ADDR_LEN);
			memcpy(aclRule.ingress_src_ipv6_addr_mask, ip6_mask, IPV6_ADDR_LEN);

			aclRule.filter_fields |= RT_ACL_INGRESS_IPV6_DIP_BIT;
			inet_pton(PF_INET6, sipStr, (void *)ip6_addr_c);
			memcpy(aclRule.ingress_dest_ipv6_addr, ip6_addr_c, IPV6_ADDR_LEN);
			memcpy(aclRule.ingress_dest_ipv6_addr_mask, ip6_mask, IPV6_ADDR_LEN);
		}

		aclRule.filter_fields |= RT_ACL_INGRESS_L4_SPORT_RANGE_BIT;
		aclRule.ingress_src_l4_port_start = aclRule.ingress_src_l4_port_end = ntohs(conn->dport);

		aclRule.filter_fields |= RT_ACL_INGRESS_L4_DPORT_RANGE_BIT;
		aclRule.ingress_dest_l4_port_start = aclRule.ingress_dest_l4_port_end = ntohs(conn->sport);

		aclRule.action_fields = RT_ACL_ACTION_FORWARD_GROUP_REDIRECT_BIT;
		aclRule.action_forward_group_redirect_port_idx = SPECIAL_CPU_PORT[(SreamRuleCPUID % SPECIAL_FASTFWD_CPU_NUM)].cpu_port;
		SreamRuleCPUID++;

		if ((ret = rt_acl_filterAndQos_add(aclRule, &aclIdx)) == 0)
			fprintf(fp, "%d\n", aclIdx);

		fclose(fp);
	}
#endif

	pthread_mutex_unlock(&speed_lock);
	
	return;

err:
	pthread_mutex_unlock(&speed_lock);
	memset(conn->sip, 0, IP6_ADDR_LEN);
	memset(conn->dip, 0, IP6_ADDR_LEN);
	conn->sport = 0;
	conn->dport = 0;
	conn->ip_familly = 0;
}

void delSpeedupSreamRule(conn_t *conn)
{
	//FILE *fp = NULL;
	struct sockaddr_in saddr, daddr;
	struct sockaddr_in6 saddr6, daddr6;
	socklen_t sock_len = sizeof(struct sockaddr_in);
	char sipStr[48], dipStr[48];
	char cmdStr[1024];
	
	int sock_familly = AF_INET;
	socklen_t len = sizeof(sock_familly);

	pthread_mutex_lock(&speed_lock);
	if(conn->ip_familly == 0)
	{
		if(getsockopt(conn->fd, SOL_SOCKET, SO_DOMAIN, &sock_familly, &len) < 0)
			goto err;
		conn->ip_familly = sock_familly;
	}

	if (0 == conn->sip[0])
	{
		if(sock_familly == AF_INET)
		{
			sock_len = sizeof(saddr);
			if (getsockname(conn->fd, (struct sockaddr*) &saddr, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getsockname fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->sip, &(saddr.sin_addr), IP_ADDR_LEN);
			conn->sport = saddr.sin_port;
		}
		else if(sock_familly == AF_INET6)
		{
			sock_len = sizeof(saddr6);
			if (getsockname(conn->fd, (struct sockaddr*) &saddr6, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getsockname fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->sip, &(saddr6.sin6_addr), IP6_ADDR_LEN);
			conn->sport = saddr6.sin6_port;
		}
	}

	if (0 == conn->dip[0])
	{
		if(sock_familly == AF_INET)
		{
			sock_len = sizeof(daddr);
			if (getpeername(conn->fd, (struct sockaddr*) &daddr, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getpeername fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->dip, &(daddr.sin_addr), IP_ADDR_LEN);
			conn->dport = daddr.sin_port;
		}
		else if(sock_familly == AF_INET6)
		{
			sock_len = sizeof(daddr6);
			if (getpeername(conn->fd, (struct sockaddr*) &daddr6, &sock_len) != 0)
			{
				fprintf(stderr, "%s %d getpeername fail\n", __func__, __LINE__);
				goto err;
			}
			memcpy(conn->dip, &(daddr6.sin6_addr), IP6_ADDR_LEN);
			conn->dport = daddr6.sin6_port;
		}
	}
	
	inet_ntop(conn->ip_familly,conn->sip, sipStr,sizeof(sipStr));
	inet_ntop(conn->ip_familly,conn->dip, dipStr,sizeof(dipStr));

	#if 0
	fp = fopen("/proc/HostSpeedUP-STREAM", "w");
	if (fp) {
		//fprintf(stderr, "del sip %s sport %d dip %s dport %d\n", sipStr, conn->sport, dipStr, conn->dport);
		fprintf(fp, "del sip %s sport %d dip %s dport %d\n", sipStr, conn->sport, dipStr, conn->dport);
		fclose(fp);
	}
	#else
	snprintf(cmdStr, 1024, "echo del sip %s sport %d dip %s dport %d > /proc/HostSpeedUP-STREAM", 
			sipStr, ntohs(conn->sport), dipStr, ntohs(conn->dport));
	fprintf(stderr, "%s\n", cmdStr);
	system(cmdStr);
	#endif
	
	pthread_mutex_unlock(&speed_lock);
	return;
	
err:
	pthread_mutex_unlock(&speed_lock);
	memset(conn->sip, 0, IP6_ADDR_LEN);
	memset(conn->dip, 0, IP6_ADDR_LEN);
	conn->sport = 0;
	conn->dport = 0;
	conn->ip_familly = 0;
}

void learnSockAddrforConn(conn_t *conn)
{
	struct sockaddr_in saddr, daddr;
	struct sockaddr_in6 saddr6, daddr6;
	socklen_t sock_len = sizeof(struct sockaddr_in);
	int sock_familly = AF_INET;
	socklen_t len = sizeof(sock_familly);

	if(conn->ip_familly == 0)
	{
		if(getsockopt(conn->fd, SOL_SOCKET, SO_DOMAIN, &sock_familly, &len) < 0)
			return;
		conn->ip_familly = sock_familly;
	}
	if(sock_familly == AF_INET)
	{
		sock_len = sizeof(saddr);
		if (getsockname(conn->fd, (struct sockaddr*) &saddr, &sock_len) != 0)
		{
			fprintf(stderr, "%s %d getsockname fail\n", __func__, __LINE__);
			return;
		}
		if (getpeername(conn->fd, (struct sockaddr*) &daddr, &sock_len) != 0)
		{
			fprintf(stderr, "%s %d getpeername fail\n", __func__, __LINE__);
			return;
		}
		memcpy(conn->sip, &(saddr.sin_addr), IP_ADDR_LEN);
		memcpy(conn->dip, &(daddr.sin_addr), IP_ADDR_LEN);	
		conn->sport = saddr.sin_port;
		conn->dport = daddr.sin_port;
	}
	else if(sock_familly == AF_INET6)
	{
		sock_len = sizeof(saddr6);
		if (getsockname(conn->fd, (struct sockaddr*) &saddr6, &sock_len) != 0)
		{
			fprintf(stderr, "%s %d getsockname fail\n", __func__, __LINE__);
			return;
		}
		if (getpeername(conn->fd, (struct sockaddr*) &daddr6, &sock_len) != 0)
		{
			fprintf(stderr, "%s %d getpeername fail\n", __func__, __LINE__);
			return;
		}
		memcpy(conn->sip, &(saddr6.sin6_addr), IP6_ADDR_LEN);
		memcpy(conn->dip, &(daddr6.sin6_addr), IP6_ADDR_LEN);
		conn->sport = saddr6.sin6_port;
		conn->dport = daddr6.sin6_port;
	}
}

void setSpeedupSackAttr(int fd, char *serverAddr, char*lip)
{
	struct tcp_info info;
	int len=sizeof(info);
	FILE *fp = NULL;
	
	fp = fopen("/proc/HostSpeedUP-Info", "w");
	if (fp)
	{
		getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
		if (info.tcpi_options & TCPI_OPT_SACK)
			fprintf(fp, "add rip %s rport %d lip %s lport %d sack 1\n", serverAddr, 0, lip, 0);
		else
			fprintf(fp, "add rip %s rport %d lip %s lport %d sack 0\n", serverAddr, 0, lip, 0);

		fclose(fp);
	}
}
#endif

/* Main 'loop'								*/
void axel_do( axel_t *axel )
{
	fd_set fds[1];
	int hifd, i;
	long long int remaining,size;
	struct timeval timeval[1];
	double curtime=0;
	/* if axel is in idle time over 10s, we should change state to ready and quit the job */
	unsigned int maxWaitTimes = 5 * axel->conf->num_connections;
	static int timeoutCnt = 0;

	/* Create statefile if necessary				*/
	if( gettime() > axel->next_state )
	{
		save_state( axel );
		axel->next_state = gettime() + axel->conf->save_state_interval;
	}

	/* Wait for data on (one of) the connections			*/
	FD_ZERO( fds );
	hifd = 0;
	for( i = 0; i < axel->conf->num_connections; i ++ )
	{
		if( axel->conn[i].enabled )
			FD_SET( axel->conn[i].fd, fds );
		hifd = max( hifd, axel->conn[i].fd );
	}
	if( hifd == 0 )
	{
		/* No connections yet. Wait...				*/
		usleep( 100000 );
		goto conn_check;
	}
	else
	{
		timeval->tv_sec = 0;
		timeval->tv_usec = 100000;
		/* A select() error probably means it was interrupted
		   by a signal, or that something else's very wrong...	*/
		if( select( hifd + 1, fds, NULL, NULL, timeval ) == -1 )
		{
			axel->ready = -1;
			return;
		}
	}

	/* Handle connections which need attention			*/
	for( i = 0; i < axel->conf->num_connections; i ++ )
	if( axel->conn[i].enabled ) {
	if( FD_ISSET( axel->conn[i].fd, fds ) )
	{
		timeoutCnt = 0;
		
		axel->conn[i].last_transfer = gettime();
        if (g_use_ssl) {
		    size = SSL_read( axel->conn[i].http->ssl, buffer, axel->conf->buffer_size );
        }
        else {
		size = read( axel->conn[i].fd, buffer, axel->conf->buffer_size );
        }
		if( size == -1 )
		{
#ifdef CONFIG_RTK_HOST_SPEEDUP
			if(axel->conf->speedtest /*&& g_use_ssl*/ && (errno==EAGAIN || errno==EWOULDBLOCK))
				continue;
#endif
			if( axel->conf->verbose )
			{
				axel_message( axel, _("Error on connection %i! "
					"Connection closed"), i );
			}
			axel->conn[i].enabled = 0;
#ifdef CONFIG_RTK_HOST_SPEEDUP
			if (axel->conn[i].speedup)
				delSpeedupSreamRule(&axel->conn[i]);
#endif
			conn_disconnect( &axel->conn[i] );
			continue;
		}
		else if( size == 0 )
		{
			if( axel->conf->verbose )
			{
				/* Only abnormal behaviour if:		*/
				if( axel->conn[i].currentbyte < axel->conn[i].lastbyte && axel->size != INT_MAX )
				{
					axel_message( axel, _("Connection %i unexpectedly closed"), i );
				}
				else
				{
					axel_message( axel, _("Connection %i finished"), i );
				}
			}
			if( !axel->conn[0].supported )
			{
				axel->ready = 1;
			}
			axel->conn[i].enabled = 0;
#ifdef CONFIG_RTK_HOST_SPEEDUP
			if (axel->conn[i].speedup)
				delSpeedupSreamRule(&axel->conn[i]);
#endif
			conn_disconnect( &axel->conn[i] );
			continue;
		}

recv_done:
		/* remaining == Bytes to go					*/
		remaining = axel->conn[i].lastbyte - axel->conn[i].currentbyte + 1;
#if 0//def CONFIG_RTK_HOST_SPEEDUP
		if(size==0 && axel->conf->speedtest)//finish download regardless of pkts loss
			size = remaining;
#endif
		if( remaining < size )
		{
			if( axel->conf->verbose )
			{
				axel_message( axel, _("Connection %i finished"), i );
			}
			axel->conn[i].enabled = 0;
#ifdef CONFIG_RTK_HOST_SPEEDUP
			if (axel->conn[i].speedup)
				delSpeedupSreamRule(&axel->conn[i]);
#endif
			conn_disconnect( &axel->conn[i] );
			size = remaining;
			/* Don't terminate, still stuff to write!	*/
		}
		/* This should always succeed..				*/
        if (!axel->conf->speedtest)
        {//when do speed test, dont write any data to output file
    		lseek( axel->outfd, axel->conn[i].currentbyte, SEEK_SET );
    		if( write( axel->outfd, buffer, size ) != size )
    		{

    			axel_message( axel, _("Write error!") );
    			axel->ready = -1;
    			return;
    		}
        }
		axel->conn[i].currentbyte += size;
		axel->bytes_done += size;
	}
	else
	{
		if(axel->conf->speedtest)
		{
			int j;
			axel->bytes_done = 0;
			for(j = 0 ; j < axel->conf->num_connections ; j++)
				axel->bytes_done += axel->conn[j].currentbyte;
		}

		if( gettime() > axel->conn[i].last_transfer + axel->conf->connection_timeout )
		{
#ifdef CONFIG_RTK_HOST_SPEEDUP
			if (axel->conf->speedtest)
			{
				axel->conn[i].last_transfer = gettime();
				size = getRecvDataLen(&axel->conn[i]);
				if (size > 0)
				{
					timeoutCnt = 0;
					goto recv_done;
				}
				else if(size == 0)
				{
					timeoutCnt++;
					if (timeoutCnt >= maxWaitTimes)
					{
						fprintf(stderr, "=====>>%s bytes_done %lld total_size %lld size %lld timeoutCnt %d\n", __func__, axel->bytes_done, 
								axel->size, size, timeoutCnt);
						//axel->bytes_done = axel->size;
						axel->ready = 1;
						break;
					}
					continue;
				}
			}
#endif
			if( axel->conf->verbose )
				axel_message( axel, _("Connection %i timed out"), i );
#ifdef CONFIG_RTK_HOST_SPEEDUP
			if (axel->conn[i].speedup)
				delSpeedupSreamRule(&axel->conn[i]);
#endif
			conn_disconnect( &axel->conn[i] );
			axel->conn[i].enabled = 0;
		}
	} }

	if( axel->ready )
		return;

conn_check:
	/* Look for aborted connections and attempt to restart them.	*/
	for( i = 0; i < axel->conf->num_connections; i ++ )
	{
#ifdef CONFIG_RTK_HOST_SPEEDUP
		if (axel->conf->speedtest)
		{
			break;
			if (!axel->conn[i].enabled && axel->conn[i].currentbyte >= axel->conn[i].lastbyte)
			{
				if ( *axel->conn[i].setup_thread != 0 )
					pthread_cancel( *axel->conn[i].setup_thread );

				axel->conn[i].speedup = 0;
				axel->conn[i].state = 0;
				if (0 == i)
					axel->conn[0].currentbyte = 0;
				else
					axel->conn[i].currentbyte = axel->conn[i-1].lastbyte + 1;
			}
		}
#endif
		if( !axel->conn[i].enabled && axel->conn[i].currentbyte < axel->conn[i].lastbyte )
		{
			url_t *u;
			
			if( axel->conn[i].state == 0 )
			{
				// Wait for termination of this thread
				pthread_join(*(axel->conn[i].setup_thread), NULL);

				u = axel->url;
				do {
					if (u->valid)
						break;
					u = u->next;
				} while (u != axel->url);
				
				conn_set( &axel->conn[i], u->text );
				axel->url = u->next;
				/* axel->conn[i].local_if = axel->conf->interfaces->text;
				axel->conf->interfaces = axel->conf->interfaces->next; */
				if( axel->conf->verbose >= 2 )
					axel_message( axel, _("Connection %i downloading from %s:%i using interface %s"),
				        	      i, axel->conn[i].host, axel->conn[i].port, axel->conn[i].local_if );

				axel->conn[i].state = 1;
				if( pthread_create( axel->conn[i].setup_thread, NULL, setup_thread, &axel->conn[i] ) == 0 )
				{
					axel->conn[i].last_transfer = gettime();
				}
				else
				{
					axel_message( axel, _("pthread error!!!") );
					axel->ready = -1;
				}
			}
			else
			{
				if( gettime() > axel->conn[i].last_transfer + axel->conf->reconnect_delay )
				{
					pthread_cancel( *axel->conn[i].setup_thread );
					axel->conn[i].state = 0;
				}
			}
		}
	}

	/* Calculate current average speed and finish_time		*/
	curtime=gettime();
	if((curtime - axel->start_time)>0 )
		axel->bytes_per_second = (int) ( (double) ( axel->bytes_done - axel->start_byte ) / (curtime - axel->start_time ) );
	if(axel->bytes_per_second>0)
		axel->finish_time = (int) ( axel->start_time + (double) ( axel->size - axel->start_byte ) / axel->bytes_per_second );

	/* Check speed. If too high, delay for some time to slow things
	   down a bit. I think a 5% deviation should be acceptable.	*/
	if( axel->conf->max_speed > 0 )
	{
		if( (float) axel->bytes_per_second / axel->conf->max_speed > 1.05 )
			axel->delay_time += 10000;
		else if( ( (float) axel->bytes_per_second / axel->conf->max_speed < 0.95 ) && ( axel->delay_time >= 10000 ) )
			axel->delay_time -= 10000;
		else if( ( (float) axel->bytes_per_second / axel->conf->max_speed < 0.95 ) )
			axel->delay_time = 0;
		usleep( axel->delay_time );
	}
	if( axel->conf->speedtest_loop == 1)
		return;
	/* Ready?							*/
#ifdef CONFIG_RTK_HOST_SPEEDUP
	//all connections timeout mean download over, now timeout time is one second
	if(axel->conf->speedtest)
	{
		for( i = 0; i < axel->conf->num_connections; i ++ )
		{
			if(curtime < axel->conn[i].last_transfer+100 )
				break;
		}
		if(i >= axel->conf->num_connections)
			axel->ready = 1;
		if( axel->bytes_done >= ((axel->size)*(axel->conf->num_connections)) )
			axel->ready = 1;
		return;
	}
#endif
	if( axel->bytes_done == axel->size )
		axel->ready = 1;
}

/* Close an axel connection						*/
void axel_close( axel_t *axel )
{
	int i;
	message_t *m;

	/* Terminate any thread still running				*/
	for( i = 0; i < axel->conf->num_connections; i ++ )
		/* don't try to kill non existing thread */
		if ( *axel->conn[i].setup_thread != 0 )
			pthread_cancel( *axel->conn[i].setup_thread );

	/* Delete state file if necessary				*/
	if( axel->ready == 1 || axel->conf->speedtest)
	{
		snprintf( buffer, MAX_STRING, "%s.st", axel->filename );
		unlink( buffer );
	}
	/* Else: Create it.. 						*/
	else if( axel->bytes_done > 0 )
	{
		save_state( axel );
	}

	/* Delete any message not processed yet				*/
	while( axel->message )
	{
		m = axel->message;
		axel->message = axel->message->next;
		free( m );
	}

	/* Close all connections and local file				*/
	close( axel->outfd );
	for( i = 0; i < axel->conf->num_connections; i ++ )
	{
#ifdef CONFIG_RTK_HOST_SPEEDUP
		if (axel->conn[i].speedup)
			delSpeedupSreamRule(&axel->conn[i]);
#endif
		conn_disconnect( &axel->conn[i] );
	}

	free( axel->conn );
	free( axel );
}

/* time() with more precision						*/
double gettime()
{
	struct timeval time[1];

	gettimeofday( time, 0 );
	return( (double) time->tv_sec + (double) time->tv_usec / 1000000 );
}

/* Save the state of the current download				*/
void save_state( axel_t *axel )
{
	int fd, i;
	char fn[MAX_STRING+4];

	/* No use for such a file if the server doesn't support
	   resuming anyway..						*/
	if( !axel->conn[0].supported )
		return;

	snprintf( fn, MAX_STRING, "%s.st", axel->filename );
	if( ( fd = open( fn, O_CREAT | O_TRUNC | O_WRONLY, 0666 ) ) == -1 )
	{
		return;		/* Not 100% fatal..			*/
	}
	write( fd, &axel->conf->num_connections, sizeof( axel->conf->num_connections ) );
	write( fd, &axel->bytes_done, sizeof( axel->bytes_done ) );
	for( i = 0; i < axel->conf->num_connections; i ++ )
	{
		write( fd, &axel->conn[i].currentbyte, sizeof( axel->conn[i].currentbyte ) );
	}
	close( fd );
}

/* Thread used to set up a connection					*/
void *setup_thread( void *c )
{
	conn_t *conn = c;
	int oldstate;

	/* Allow this thread to be killed at any time.			*/
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &oldstate );
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate );

	if( conn_setup( conn ) )
	{
#ifdef CONFIG_RTK_HOST_SPEEDUP
		if(conn->conf->speedtest)
		learnSockAddrforConn(conn);
#endif
		conn->last_transfer = gettime();
#ifdef CONFIG_RTK_HOST_SPEEDUP
		if (conn->conf->speedtest) 
		{
			conn->speedup = 1;
			addSpeedupSreamRule(conn);
		}
#endif
		if( conn_exec( conn ) )
		{
#ifdef CONFIG_RTK_HOST_SPEEDUP
			if(conn->conf->speedtest)
			{				
				conn->size = http_size( conn->http );
				//if(g_use_ssl)
				{
					int flags;
					//SSL_read will block for packet go speedup, so set speedtest fd to nonblock
					if ((flags = fcntl(conn->fd, F_GETFL, 0)) == -1)
				        flags = 0;
					fcntl(conn->fd, F_SETFL, flags | O_NONBLOCK);
				}
			}
#endif
			conn->last_transfer = gettime();
			conn->enabled = 1;
			conn->state = 0;
			if(conn->conf->speedtest)
				conn_download(conn);
			return( NULL );
		}
	}

	conn_disconnect( conn );
	conn->state = 0;
	return( NULL );
}

/* Add a message to the axel->message structure				*/
static void axel_message( axel_t *axel, char *format, ... )
{
	message_t *m = malloc( sizeof( message_t ) ), *n = axel->message;
	va_list params;

	memset( m, 0, sizeof( message_t ) );
	va_start( params, format );
	vsnprintf( m->text, MAX_STRING, format, params );
	va_end( params );

	if( axel->message == NULL )
	{
		axel->message = m;
	}
	else
	{
		while( n->next != NULL )
			n = n->next;
		n->next = m;
	}
}

/* Divide the file and set the locations for each connection		*/
static void axel_divide( axel_t *axel )
{
	int i;
#ifdef CONFIG_RTK_HOST_SPEEDUP
	if(axel->conf->speedtest)
	{
	for( i = 0; i < axel->conf->num_connections; i ++ )
	{
		axel->conn[i].currentbyte = 0;
		axel->conn[i].lastbyte = axel->size - 1;
	}
		return;
	}
#endif
	axel->conn[0].currentbyte = 0;
	axel->conn[0].lastbyte = axel->size / axel->conf->num_connections - 1;
	for( i = 1; i < axel->conf->num_connections; i ++ )
	{
#ifdef DEBUG
		printf( "Downloading %lld-%lld using conn. %i\n", axel->conn[i-1].currentbyte, axel->conn[i-1].lastbyte, i - 1 );
#endif
		axel->conn[i].currentbyte = axel->conn[i-1].lastbyte + 1;
		axel->conn[i].lastbyte = axel->conn[i].currentbyte + axel->size / axel->conf->num_connections;
	}
	axel->conn[axel->conf->num_connections-1].lastbyte = axel->size - 1;
#ifdef DEBUG
	printf( "Downloading %lld-%lld using conn. %i\n", axel->conn[i-1].currentbyte, axel->conn[i-1].lastbyte, i - 1 );
#endif
}
