.EXPORT_ALL_VARIABLES:

#include ./../.config
include ./../luna_mldproxy_config
MULTICAST_ROOT=$(shell pwd)
IGMPPROXY_OLD_DIR=$(MULTICAST_ROOT)/igmpproxy
IGMPPROXY_NEW_DIR=$(MULTICAST_ROOT)/igmpproxy_new
MLDPROXY_OLD_DIR=$(MULTICAST_ROOT)/mldproxy
MLDPROXY_NEW_DIR=$(MULTICAST_ROOT)/mldproxy_new
IGMPPROXY_EXEC=
MLDPROXY_EXEC=
IGMPPROXY_DIR=invalid
MLDPROXY_DIR=invalid

ifeq ($(CONFIG_APP_IGMPPROXY),y)
ifeq ($(CONFIG_IGMP_MLD_COMMON_SUPPORT),y)
ifeq ($(CONFIG_IGMPV2_SUPPORT), y)
IGMPPROXY_DIR=$(IGMPPROXY_OLD_DIR)
else
IGMPPROXY_DIR=$(IGMPPROXY_NEW_DIR)
endif
else
IGMPPROXY_DIR=$(IGMPPROXY_OLD_DIR)
endif
endif

ifeq ($(CONFIG_APP_MLDPROXY),y)
ifeq ($(CONFIG_IGMP_MLD_COMMON_SUPPORT),y)
ifeq ($(CONFIG_MLDV1_SUPPORT), y)
MLDPROXY_DIR=$(MLDPROXY_OLD_DIR)
else
MLDPROXY_DIR=$(MLDPROXY_NEW_DIR)
endif
else
MLDPROXY_DIR=$(MLDPROXY_OLD_DIR)
endif
endif

all:
	make -C $(MLDPROXY_NEW_DIR) clean
	if [ -d $(MLDPROXY_DIR) ] ; then \
		make -C $(MLDPROXY_DIR); \
	fi
clean:
	make -C $(MLDPROXY_NEW_DIR) clean
	
romfs:
	if [ -d $(IGMPPROXY_DIR) ] ; then \
                make -C $(IGMPPROXY_DIR) romfs; \
        fi
	if [ -d $(MLDPROXY_DIR) ] ; then \
                make -C $(MLDPROXY_DIR) romfs; \
        fi
