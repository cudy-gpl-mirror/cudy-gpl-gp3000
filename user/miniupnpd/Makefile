EXEC=miniupnpd
UPNP_ROOT_DIR=$(shell pwd)
ifeq ($(CONFIG_USER_MINIUPNP_V2_1), y)
UPNP_VER:=miniupnpd-2.1.20200510
else 
UPNP_VER:=miniupnpd-1.3.2008
endif

DEP_LIBS=
COMMON_LIBS_CFLAGS:=--host=$(CROSS_HOST) --target=$(CROSS_TARGET) --prefix=$(TMPFSDIR)

################### UPNP Configure Option ###########################
ifeq ($(CONFIG_USER_MINIUPNP_V2_1), y)
UPNP_CFLAGS=--leasefile --vendorcfg --firewall=iptables

ifeq ($(CONFIG_USER_IPTABLES_IPTABLES_1_4_18), y)
IPTABLES_DIR=$(ROOTDIR)/user/iptables-1.4.18
else ifeq ($(CONFIG_USER_IPTABLES_IPTABLES_1_4_21), y)
IPTABLES_DIR=$(ROOTDIR)/user/iptables-1.4.21
else
IPTABLES_DIR=$(ROOTDIR)/user/iptables
endif
UPNP_CFLAGS+=--iptablespath=$(IPTABLES_DIR)

ifeq ($(CONFIG_IPV6), y)
UPNP_CFLAGS+=--ipv6
endif
ifeq ($(CONFIG_USER_MINIUPNPD_IGD2), y)
UPNP_CFLAGS+=--igd2
endif
UPNP_CFLAGS+=--debug

CFLAGS+=-I$(IPTABLES_DIR)/include
LDFLAGS+=-L$(IPTABLES_DIR)/libiptc/.libs/ -lip4tc \
         -L$(ROOTDIR)/user/boa/src/LINUX 
ifeq ($(CONFIG_IPV6), y)
LDFLAGS+=-lip6tc
CFLAGS+=-include $(ROOTDIR)/config/autoconf.h
endif

########### libuuid configure #################
UUID_LIB=libuuid-1.0.3
UUID_CFLAGS=--disable-static
DEP_LIBS+=$(UUID_LIB)
endif	# CONFIG_USER_MINIUPNP_V2_1

all: libs preconfig
	$(MAKE) -C $(UPNP_ROOT_DIR)/$(UPNP_VER);

preconfig:
	if [ ! -f $(UPNP_ROOT_DIR)/$(UPNP_VER)/Makefile ];then \
		cd $(UPNP_ROOT_DIR)/$(UPNP_VER); \
		./configure $(UPNP_CFLAGS); \
		cd -; \
	fi;

libs: $(DEP_LIBS)
	for i in $(DEP_LIBS); do \
		if [ ! -f $(UPNP_ROOT_DIR)/$$i/Makefile ];then \
			if [ "$$i" = "$(UUID_LIB)" ]; then \
				cd $$i;autoreconf -ivf && ./configure $(COMMON_LIBS_CFLAGS) $(UUID_CFLAGS);cd -; \
			fi; \
		fi; \
		$(MAKE) -C $(UPNP_ROOT_DIR)/$$i;$(MAKE) -C $(UPNP_ROOT_DIR)/$$i install; \
	done;

clean:
	$(MAKE) -C $(UPNP_ROOT_DIR)/$(UPNP_VER) clean;
ifeq ($(CONFIG_USER_MINIUPNP_V2_1), y)
	rm -f $(UPNP_ROOT_DIR)/$(UPNP_VER)/Makefile;	# for initial miniupnp
endif

romfs:
	for x in $(shell cd $(TMPFSDIR)/lib; ls libuuid*.so*); do \
		if [ -L $(TMPFSDIR)/lib/$$x ]; then \
			dest=`readlink $(TMPFSDIR)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(TMPFSDIR)/lib/$$x /lib/$$x; \
		fi; \
	done;
	$(ROMFSINST)  $(UPNP_ROOT_DIR)/$(UPNP_VER)/$(EXEC) /bin/$(EXEC);
