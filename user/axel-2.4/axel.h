  /********************************************************************\
  * Axel -- A lighter download accelerator for Linux and other Unices. *
  *                                                                    *
  * Copyright 2001 Wilmer van der Gaast                                *
  \********************************************************************/

/* Main include file							*/

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

#define _GNU_SOURCE

#include "config.h"

#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>

#ifndef	NOGETOPTLONG
#include <getopt.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <openssl/ssl.h>
#ifdef EMBED
#include <linux/config.h>
#include <rtk/options.h>
#include <config/autoconf.h>
#else
#include "../../../../include/linux/autoconf.h"
#include "../../../../config/autoconf.h"
#include "options.h"
#endif

#if defined(CONFIG_LUNA_G3_SERIES) && defined(CONFIG_RTL8277C_SERIES) && (CONFIG_NR_CPUS > 1) 
#include "rt_acl_ext.h"
#include "common/error.h"
#define RTK_AXEL_RULE_ACL_FILE "/var/axel_rule_acl_file"

#if defined(CONFIG_RTL_PE_OFFLOAD) && defined(CONFIG_RTK_FC_MAINCPU_ONE_CPU_PORT) 
#define SPECIAL_FASTFWD_CPU_NUM 2
#elif defined(CONFIG_RTL_PE_OFFLOAD)
#define SPECIAL_FASTFWD_CPU_NUM 3
#else
#define SPECIAL_FASTFWD_CPU_NUM ((CONFIG_NR_CPUS>4)?4:CONFIG_NR_CPUS)
#endif
struct SPECIAL_CPU_PORT_s {
	unsigned short cpu_port;
	char *irq_name;
	int irq_num;
	unsigned int org_irq_mask;
	unsigned int new_irq_mask;
};
extern struct SPECIAL_CPU_PORT_s SPECIAL_CPU_PORT[SPECIAL_FASTFWD_CPU_NUM];
#define DO_ACL_CPU_ISOLATE_SPEEDUP 1
#endif

/* Internationalization							*/
#ifdef I18N
#define PACKAGE			"axel"
#define _( x )			gettext( x )
#include <libintl.h>
#include <locale.h>
#else
#define _( x )			x
#endif

/* Compiled-in settings							*/
#define MAX_STRING		1024
#define MAX_ADD_HEADERS	10
#define MAX_REDIR		5
#define AXEL_VERSION_STRING	"2.4"
#define DEFAULT_USER_AGENT	"Axel " AXEL_VERSION_STRING " (" ARCH ")"

typedef struct
{
	void *next;
	int valid;
	char text[MAX_STRING];
} message_t;

typedef message_t url_t;
typedef message_t if_t;

#include "conf.h"
#include "tcp.h"
#include "ftp.h"
#include "http.h"
#include "conn.h"
#include "search.h"

#define min( a, b )		( (a) < (b) ? (a) : (b) )
#define max( a, b )		( (a) > (b) ? (a) : (b) )

/* TR143 HTTP Download/Upload Diagnostics*/
enum
{
	eTR143_None=0,
	eTR143_Requested,
	eTR143_Completed,
	eTR143_Error_InitConnectionFailed,
	eTR143_Error_NoResponse,
	eTR143_Error_PasswordRequestFailed,
	eTR143_Error_LoginFailed,
	eTR143_Error_NoTransferMode,
	eTR143_Error_NoPASV,
	//download
	eTR143_Error_TransferFailed,
	eTR143_Error_IncorrectSize,
	eTR143_Error_Timeout,
	//upload
	eTR143_Error_NoCWD,
	eTR143_Error_NoSTOR,
	eTR143_Error_NoTransferComplete,

	eTR143_End /*last one*/
};

#ifdef CONFIG_USER_CTMANAGEDEAMON
enum
{
	eBUCPE_None=0,
	eBUCPE_Requested,
	eBUCPE_Completed,
	eBUCPE_InitError_ConnectionFailed,
	eBUCPE_DNSError_Timeout,
	eBUCPE_DNSError_NoSuchDomain,
	eBUCPE_NetError_Timeout,
	eBUCPE_NetError_DestAddrUnreachable,
	eBUCPE_NetError_DestPortUnreachable,
	eBUCPE_HTTPError_Code,
};
#endif

#ifdef CONFIG_USER_CUSPEEDTEST
enum
{
	eSpeedTest_None=0,
	eSpeedTest_Requested,
	eSpeedTest_Completed,

	eSpeedTest_STAT_PPPOE_DIALING,
	eSpeedTest_STAT_SERVER_CONNECTING,
	eSpeedTest_STAT_SERVER_CONNECTED,
	eSpeedTest_STAT_SPEED_TESTING,
	eSpeedTest_STAT_UPLOAD_TEST_RESULT,
	eSpeedTest_STAT_TEST_FINISHED,

	eSpeedTest_End /*last one*/
};
#endif

#define MAX_SAMPLED_VALUES 30

extern SSL_CTX *g_ssl_ctx;
extern int      g_use_ssl;

typedef struct
{
	conn_t *conn;
	conf_t conf[1];
	char filename[MAX_STRING];
	double start_time;
	int next_state, finish_time;
	long long bytes_done, start_byte, size;
	int bytes_per_second;
	int delay_time;
	int outfd;
	int ready;
	message_t *message;
	url_t *url;

	/*Added for TR-143*/
	int error;
	double EOMTime;
	int sample_cnt;
	int sample_values[MAX_SAMPLED_VALUES];
	int total_sample_values[MAX_SAMPLED_VALUES];
#ifdef CONFIG_USER_CUSPEEDTEST
	long long bytes_done_pre,totalbytes_done_pre;
	int bytes_one_sec[MAX_SAMPLED_VALUES];
	int totalbytes_one_sec[MAX_SAMPLED_VALUES];
#endif
	unsigned long long total_bytes_start;
	unsigned long long total_bytes_end;
} axel_t;

axel_t *axel_new( conf_t *conf, int count, void *url );
int axel_open( axel_t *axel );
void axel_start( axel_t *axel );
void axel_do( axel_t *axel );
void axel_close( axel_t *axel );

double gettime();
//#if 1
#ifdef CONFIG_USER_CTMANAGEDEAMON
extern void write_BUCPE_test_error(int type);
#endif
