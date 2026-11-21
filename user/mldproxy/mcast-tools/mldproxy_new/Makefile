ifeq ($(wildcard $(DIR_LINUX)/.config),)
DIR_LINUX_KERNEL = $(DIR_ROOT)/linux-2.6.30
else
DIR_LINUX_KERNEL = $(DIR_LINUX)
endif
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)gcc
COPT = -Os
MLD_LIBS =
 
CFLAGS += -Werror

ifeq ($(KERNEL_VER),linux-2.6.30)
CFLAGS +=-I$(DIR_LINUX_KERNEL)/include -DCONFIG_DEFAULTS_KERNEL_2_6 -D__BIG_ENDIAN_BITFIELD -DCONFIG_CPU_BIG_ENDIAN $(COPT)
endif
ifeq ($(KERNEL_VER),linux-3.10)
CFLAGS +=-I$(DIR_LINUX_KERNEL)/include/uapi -DCONFIG_DEFAULTS_KERNEL_2_6 -D__BIG_ENDIAN_BITFIELD -DCONFIG_CPU_BIG_ENDIAN $(COPT)
endif
#CFLAGS=-I -Os -DCONFIG_DEFAULTS_KERNEL_2_6 -D__BIG_ENDIAN_BITFIELD -DCONFIG_CPU_BIG_ENDIAN

CFLAGS += $(EXTRA_CFLAGS)
EXEC = mldproxy
OBJS = ip6opt.o mroute-api.o lib.o ifvc.o syslog.o timeout.o main.o

ifeq ($(CONFIG_MLDPROXY_MULTIWAN),y)
CFLAGS += -DCONFIG_MLDPROXY_MULTIWAN
endif
ifeq ($(CONFIG_USER_MLDPROXY_HANDLE_MCFLOW),y)
CFLAGS += -DCONFIG_USER_MLDPROXY_HANDLE_MCFLOW
endif

ifeq ($(CONFIG_MLDV2_SUPPORT),y)
CFLAGS += -DCONFIG_MLDV2_SUPPORT
#OBJS   += 	mldv2.o
DEPH   += mldproxy.h
endif

ifeq ($(CONFIG_IGMP_MLD_COMMON_SUPPORT),y)
CFLAGS += -DCONFIG_IGMP_MLD_COMMON_SUPPORT
OBJS   += igmp_mld_utility.o igmp_mld_debug.o igmp_mld_config.o
endif

ifeq ($(CONFIG_RTK_DEV_AP),y)
CFLAGS += -DCONFIG_MLDPROXY_MULTIWAN
CONFIG_MLDPROXY_MULTIWAN=y
endif

ifeq ($(CONFIG_MLDPROXY_MULTIWAN),y)
ifeq ($(CONFIG_RTK_APP_PORT_BINDING_SUPPORT),y)
OBJS += igmp_mld_lanwan_binding.o
MLD_LIBS += $(LIBMIB)
ifeq ($(CONFIG_USER_RTK_VOIP),y)
MLD_LIBS += $(LIBVOIP)
endif
#CFLAGS += -Werror
endif
endif

ifeq ($(CONFIG_USER_MLDPROXY_HANDLE_MCFLOW),y)
MLD_LIBS += $(LIBMIB)
ifeq ($(CONFIG_USER_RTK_VOIP),y)
MLD_LIBS += $(LIBVOIP)
endif
endif

all: BUILT_TIME $(EXEC)


$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $(INCLUDE) -o $@ $(OBJS) $(LDLIBS$(LDLIBS-$(@))) $(MLD_LIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.o *.bak built_time

BUILT_TIME:
	@echo  \#define BUILT_TIME \"`TZ=UTC date -u "+%Y.%m.%d-%H:%M%z" `\" > ./built_time	
