EXEC = igmpproxy
OBJS = mroute-api.o lib.o ifvc.o syslog.o timeout.o main.o
IGMPPROXY_LIBS=
CURRENT_DIR = $(shell pwd)
LINK_DIR = $(CURRENT_DIR)/../common

$(warning -----CURRENT_DIR is $(CURRENT_DIR),LINK_DIR is $(LINK_DIR)------ )

#ifeq ($(CONFIG_IGMPPROXY_MULTIWAN),y)
#CFLAGS += -DCONFIG_IGMPPROXY_MULTIWAN
#endif

ifeq ($(CONFIG_IGMP_MLD_COMMON_SUPPORT),y)
CFLAGS += -DCONFIG_IGMP_MLD_COMMON_SUPPORT
OBJS   += igmp_mld_utility.o igmp_mld_debug.o igmp_mld_config.o
ifeq ($(CONFIG_IGMPV3_SUPPORT),y)
CFLAGS += -DCONFIG_IGMPV3_SUPPORT
endif
ifeq ($(CONFIG_RTL_LAN_COMPATIBILITY),y)
CFLAGS += -DCONFIG_RTL_LAN_COMPATIBILITY
endif
ifeq ($(CONFIG_RTL_NEW_IGMP_REPORT_BEHAVIOR),y)
CFLAGS += -DCONFIG_RTL_NEW_IGMP_REPORT_BEHAVIOR
endif
else
ifeq ($(CONFIG_IGMPV3_SUPPORT),y)
CFLAGS += -DCONFIG_IGMPV3_SUPPORT
OBJS   += igmpv3.o
DEPH   += igmpproxy.h
endif
endif


#ap
#ifeq ($(CONFIG_RTK_DEV_AP),y)
#ifeq ($(CONFIG_RTL_MULTI_ETH_WAN),y)
#CFLAGS += -D MULTICAST_LANWAN_BIND
#IGMPPROXY_LIBS += $(LIBMIB)
#endif
#endif

CFLAGS += -Wall -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

all: BUILT_TIME $(EXEC) 


#LINK_FILE:

#ifeq (,$(wildcard $(CURRENT_DIR)/timeout.c))
#	ln -s $(LINK_DIR)/timeout.c $(CURRENT_DIR)/timeout.c
#endif
#ifeq (,$(wildcard $(CURRENT_DIR)/timeout.h))
#	ln -s $(LINK_DIR)/timeout.h $(CURRENT_DIR)/timeout.h
#endif
#ifeq (,$(wildcard igmp_mld_utility.c))
#	ln -s $(LINK_DIR)/igmp_mld_utility.c  $(CURRENT_DIR)/igmp_mld_utility.c
#endif
#ifeq (,$(wildcard $(CURRENT_DIR)/igmp_mld_debug.c))
#	ln -s $(LINK_DIR)/igmp_mld_debug.c $(CURRENT_DIR)/igmp_mld_debug.c
#endif
#ifeq (,$(wildcard $(CURRENT_DIR)/igmp_mld_config.c))
#	ln -s $(LINK_DIR)/igmp_mld_config.c $(CURRENT_DIR)/igmp_mld_config.c
#endif
#ifeq (,$(wildcard $(CURRENT_DIR)/igmp_mld_utility.h))
#	ln -s $(LINK_DIR)/igmp_mld_utility.h $(CURRENT_DIR)/igmp_mld_utility.h
#endif
#ifeq (,$(wildcard $(CURRENT_DIR)/igmp_mld_debug.h))
#	ln -s $(LINK_DIR)/igmp_mld_debug.h $(CURRENT_DIR)/igmp_mld_debug.h
#endif
#ifeq (,$(wildcard $(CURRENT_DIR)/igmp_mld_config.h))
#	ln -s $(LINK_DIR)/igmp_mld_config.h $(CURRENT_DIR)/igmp_mld_config.h
#endif

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS$(LDLIBS-$(@))) $(LIBMIB) $(LIBVOIP) $(IGMPPROXY_LIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.o *.bak built_time

BUILT_TIME:
	@echo  \#define BUILT_TIME \"`TZ=UTC date -u "+%Y.%m.%d-%H:%M%z" `\" > ./built_time	

