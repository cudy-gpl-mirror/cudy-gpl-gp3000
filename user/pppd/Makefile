#
# Makefile for pppd for the Linux OS
#
PKG_NAME:=ppp

PKG_VERSION:=2.4.7
ifeq ($(CONFIG_USER_PPPD_2_4_7), y)
	PKG_VERSION:=2.4.7
endif

PKG_BUILD_DIR:=$(CURDIR)/build/$(PKG_NAME)-$(PKG_VERSION)
PKG_SRC_TAR:=$(CURDIR)/tarball/$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_PATCH_DIR:=$(CURDIR)/patches/$(PKG_VERSION)
FILE_DIR:=$(CURDIR)/file
BUILD_INSTALL_DIR=$(CURDIR)/build/.install

CONFIGURE_ARG = 
#CFLAGS = 
ifeq ($(CONFIG_RTK_PPP_IF_NS),y)
CFLAGS += -DCONFIG_RTK_PPP_IF_NS
endif
export CFLAGS

all: check do_patch configure
	make -C $(PKG_BUILD_DIR) 
	make -C $(PKG_BUILD_DIR) install DESTDIR=$(BUILD_INSTALL_DIR)

check:
	if [ -f $(PKG_BUILD_DIR)/../.build_version ]; then \
		v=`cat $(PKG_BUILD_DIR)/../.build_version`; \
		if [ ! "$$v" = "$(PKG_VERSION)" ]; then \
			rm -rf $(BUILD_INSTALL_DIR); \
		fi; \
	else \
		rm -rf $(BUILD_INSTALL_DIR); \
	fi
	if [ ! -d $(PKG_BUILD_DIR) ]; then \
		mkdir -p $(PKG_BUILD_DIR); \
		if [ -f $(PKG_SRC_TAR) ]; then \
			tar -zxvf $(PKG_SRC_TAR) --strip-components 1 -C $(PKG_BUILD_DIR); \
		else \
			exit 1; \
		fi; \
		echo -n "$(PKG_VERSION)" > $(PKG_BUILD_DIR)/../.build_version; \
	fi

configure: do_patch
	if [ -d $(PKG_BUILD_DIR) ]; then \
		cd $(PKG_BUILD_DIR); \
		if [ ! -f Makefile ]; then \
			./configure $(CONFIGURE_ARG); \
		fi; \
	fi
	
do_patch: check
	if [ -d $(PKG_BUILD_DIR) ]; then \
		cd $(PKG_BUILD_DIR); \
		if [ ! -f .patches ]; then \
			mkdir -p patches; \
			if [ -d $(PKG_PATCH_DIR) ]; then \
				cp -rf $(PKG_PATCH_DIR)/* patches/ ; \
				type quilt > /dev/null ; \
				if [ "$$?" = "0" ]; then \
					echo "" > patches/series; \
					for i in `ls patches/*.patch`; \
					do \
						b=`basename $$i`; \
						echo $$b >> patches/series; \
					done; \
					quilt push -a || exit 1; \
					touch .quilt_patches; \
				else \
					for i in `ls patches/*.patch`; \
					do \
						patch -p1 -i $$i || exit 1; \
					done; \
				fi; \
			fi; \
			touch .patches; \
		fi; \
	fi
	
clean:
	make -C $(PKG_BUILD_DIR) clean
	#rm -rf $(PKG_BUILD_DIR)
	rm -rf $(BUILD_INSTALL_DIR)

romfs:
	mkdir -p $(ROMFSDIR)/lib/pppd/plugins
	mkdir -p $(ROMFSDIR)/bin
	mkdir -p $(ROMFSDIR)/etc/pppd
	$(ROMFSINST) -o $(CONFIG_PPPOE) $(BUILD_INSTALL_DIR)/lib/pppd/$(PKG_VERSION)/rp-pppoe.so  /lib/pppd/plugins/rp-pppoe.so
	$(ROMFSINST) -o $(CONFIG_PPPOATM) $(BUILD_INSTALL_DIR)/lib/pppd/$(PKG_VERSION)/pppoatm.so  /lib/pppd/plugins/pppoatm.so
	$(ROMFSINST) -o $(CONFIG_PPPOL2TP) $(BUILD_INSTALL_DIR)/lib/pppd/$(PKG_VERSION)/pppol2tp.so  /lib/pppd/plugins/pppol2tp.so
	$(ROMFSINST) -o $(CONFIG_PPTP) $(BUILD_INSTALL_DIR)/lib/pppd/$(PKG_VERSION)/pptp.so  /lib/pppd/plugins/pptp.so
	$(ROMFSINST) $(BUILD_INSTALL_DIR)/sbin/pppd /bin/pppd
ifeq ($(CONFIG_USER_PPPD),y)
	$(ROMFSINST) $(BUILD_INSTALL_DIR)/sbin/chat /bin/chat
endif
	
	$(ROMFSINST) -p 755 $(FILE_DIR)/ip-up.local /etc/pppd/ip-up.local
	$(ROMFSINST) -p 755 $(FILE_DIR)/ip-down.local /etc/pppd/ip-down.local
	$(ROMFSINST) -p 755 $(FILE_DIR)/ipv6-up /etc/pppd/ipv6-up 
	$(ROMFSINST) -p 755 $(FILE_DIR)/ipv6-down /etc/pppd/ipv6-down

.PHONY: rcX
rcX:



