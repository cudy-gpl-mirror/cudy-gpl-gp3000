.EXPORT_ALL_VARIABLES:

MULTICAST_ROOT=$(shell pwd)

IGMPPROXY_DIR=
MLDPROXY_DIR=

ifeq ($(CONFIG_USER_IGMPPROXY),y)
IGMPPROXY_DIR=$(MULTICAST_ROOT)/igmpproxy_new
endif

ifeq ($(CONFIG_USER_MLDPROXY),y)
MLDPROXY_DIR=$(MULTICAST_ROOT)/mldproxy_new
endif

LINK_DIR=$(MULTICAST_ROOT)/common

$(warning -------IGMPPROXY_DIR is $(IGMPPROXY_DIR) -----)
$(warning -------IGMPPROXY_DIR is $(MLDPROXY_DIR) -----)

all:

ifeq ($(CONFIG_USER_IGMPPROXY),y)
ifeq (,$(wildcard $(IGMPPROXY_DIR)/timeout.c))
	ln -s $(LINK_DIR)/timeout.c $(IGMPPROXY_DIR)/timeout.c
endif
ifeq (,$(wildcard $(IGMPPROXY_DIR)/timeout.h))
	ln -s $(LINK_DIR)/timeout.h $(IGMPPROXY_DIR)/timeout.h
endif
ifeq (,$(wildcard $(IGMPPROXY_DIR)/igmp_mld_utility.c))
	ln -s $(LINK_DIR)/igmp_mld_utility.c  $(IGMPPROXY_DIR)/igmp_mld_utility.c
endif
ifeq (,$(wildcard $(IGMPPROXY_DIR)/igmp_mld_debug.c))
	ln -s $(LINK_DIR)/igmp_mld_debug.c $(IGMPPROXY_DIR)/igmp_mld_debug.c
endif
ifeq (,$(wildcard $(IGMPPROXY_DIR)/igmp_mld_config.c))
	ln -s $(LINK_DIR)/igmp_mld_config.c $(IGMPPROXY_DIR)/igmp_mld_config.c
endif
ifeq (,$(wildcard $(IGMPPROXY_DIR)/igmp_mld_utility.h))
	ln -s $(LINK_DIR)/igmp_mld_utility.h $(IGMPPROXY_DIR)/igmp_mld_utility.h
endif
ifeq (,$(wildcard $(IGMPPROXY_DIR)/igmp_mld_debug.h))
	ln -s $(LINK_DIR)/igmp_mld_debug.h $(IGMPPROXY_DIR)/igmp_mld_debug.h
endif
ifeq (,$(wildcard $(IGMPPROXY_DIR)/igmp_mld_config.h))
	ln -s $(LINK_DIR)/igmp_mld_config.h $(IGMPPROXY_DIR)/igmp_mld_config.h
endif
endif

ifeq ($(CONFIG_USER_MLDPROXY),y)
ifeq (,$(wildcard $(MLDPROXY_DIR)/timeout.c))
	ln -s $(LINK_DIR)/timeout.c $(MLDPROXY_DIR)/timeout.c
endif
ifeq (,$(wildcard $(MLDPROXY_DIR)/timeout.h))
	ln -s $(LINK_DIR)/timeout.h $(MLDPROXY_DIR)/timeout.h
endif
ifeq (,$(wildcard $(MLDPROXY_DIR)/igmp_mld_utility.c))
	ln -s $(LINK_DIR)/igmp_mld_utility.c  $(MLDPROXY_DIR)/igmp_mld_utility.c
endif
ifeq (,$(wildcard $(MLDPROXY_DIR)/igmp_mld_debug.c))
	ln -s $(LINK_DIR)/igmp_mld_debug.c $(MLDPROXY_DIR)/igmp_mld_debug.c
endif
ifeq (,$(wildcard $(MLDPROXY_DIR)/igmp_mld_config.c))
	ln -s $(LINK_DIR)/igmp_mld_config.c $(MLDPROXY_DIR)/igmp_mld_config.c
endif
ifeq (,$(wildcard $(MLDPROXY_DIR)/igmp_mld_utility.h))
	ln -s $(LINK_DIR)/igmp_mld_utility.h $(MLDPROXY_DIR)/igmp_mld_utility.h
endif
ifeq (,$(wildcard $(MLDPROXY_DIR)/igmp_mld_debug.h))
	ln -s $(LINK_DIR)/igmp_mld_debug.h $(MLDPROXY_DIR)/igmp_mld_debug.h
endif
ifeq (,$(wildcard $(MLDPROXY_DIR)/igmp_mld_config.h))
	ln -s $(LINK_DIR)/igmp_mld_config.h $(MLDPROXY_DIR)/igmp_mld_config.h
endif
endif


	make -C $(IGMPPROXY_DIR) clean
	make -C $(MLDPROXY_DIR) clean 


	if [ -d $(IGMPPROXY_DIR) ] ; then \
		make -C $(IGMPPROXY_DIR); \
	fi

	if [ -d $(MLDPROXY_DIR) ] ; then \
		make -C $(MLDPROXY_DIR); \
	fi
	
clean:
	make -C $(IGMPPROXY_DIR) clean
	make -C $(MLDPROXY_DIR) clean

romfs:
	if [ -d $(IGMPPROXY_DIR) ] ; then \
		 make -C $(IGMPPROXY_DIR) romfs; \
	fi
 
	if [ -d $(MLDPROXY_DIR) ] ; then \
		make -C $(MLDPROXY_DIR) romfs; \
	fi
