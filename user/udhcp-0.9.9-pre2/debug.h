#ifndef _DEBUG_H
#define _DEBUG_H

#include "libbb_udhcp.h"

#include <stdio.h>
#ifdef SYSLOG
#include <syslog.h>
#endif

#ifdef DEBUG_ONOFF_BY_MIB
extern int dhcp_debug_on;
extern char *dhcp_debug_prefix;
extern char *dhcp_debug_interface;
# define DEBUG(level, str, args...) LOG(level, str, ## args)
# define LOG(level, str, args...)	if(dhcp_debug_on!=0){	\
										printf(dhcp_debug_prefix); \
										printf(dhcp_debug_interface);\
										printf(str, ## args); \
										printf("\n"); \
								 	}
# define OPEN_LOG(name) do {;} while(0)
#define CLOSE_LOG() do {;} while(0)

#define _DHCP_DEBUG_ON_OFF_		1		//used for enabling dhcp_debug dynamically
#if defined(_DHCP_DEBUG_ON_OFF_)
enum {
	DHCPDEBUG_FLAG_WRITE=0,
	DHCPDEBUG_FLAG_READ,
};
void dhcp_debug_onoff_by_file(int flag);
#endif

#else//i.e DEBUG_ONOFF_BY_MIB not defined
#ifdef SYSLOG
# define LOG(level, str, args...) do { printf(str, ## args); \
				printf("\n"); \
				syslog(level, str, ## args); } while(0)
# define OPEN_LOG(name) openlog(name, 0, 0)
#define CLOSE_LOG() closelog()
#else
# define LOG_EMERG	"EMERGENCY!"
# define LOG_ALERT	"ALERT!"
# define LOG_CRIT	"critical!"
# define LOG_WARNING	"warning"
# define LOG_ERR	"error"
# define LOG_INFO	"info"
# define LOG_DEBUG	"debug"
# define LOG(level, str, args...) do { printf("%s, ", level); \
				printf(str, ## args); \
				printf("\n"); } while(0)
# define OPEN_LOG(name) do {;} while(0)
#define CLOSE_LOG() do {;} while(0)
#endif

#ifdef DEBUG
# undef DEBUG
# define DEBUG(level, str, args...) LOG(level, str, ## args)
# define DEBUGGING
#else
# define DEBUG(level, str, args...) do {;} while(0)
#endif

#endif
#endif
