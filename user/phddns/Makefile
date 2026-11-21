#
# Makefile for OpenvSwitch for the Linux OS
#
PKG_NAME:=phddns
PKG_VERSION:=2.0.5.19225
#PKG_VERSION:=2.4.0
PKG_BUILD_DIR:=$(CURDIR)/build/$(PKG_NAME)-$(PKG_VERSION)
PKG_SRC_TAR:=$(CURDIR)/tarball/$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_PATCH_DIR:=$(CURDIR)/patches/$(PKG_VERSION)
FILE_DIR:=$(CURDIR)/file
BUILD_INSTALL_DIR=$(CURDIR)/build/.install
#BUILD_INSTALL_DIR=$(PKG_BUILD_DIR)/.install
INSTALL_MOD_PATH=$(BUILD_INSTALL_DIR)

ifeq ("$(CROSS_HOST)", "")
CROSS_HOST = $(ARCH)-linux
CROSS_TARGET = $(ARCH)-linux
endif

# build openvswitch kernel module need enable +kmod-stp +kmod-ipv6 +kmod-gre
# +kmod-lib-crc32c => CONFIG_CRYPTO_CRC32C, CONFIG_LIBCRC32C

CONFIGURE_ARG = --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --build=i386-linux

all: check do_patch configure
	make -C $(PKG_BUILD_DIR) CFLAGS="$(CFLAGS)"
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
		touch -d '-20 sec' $(PKG_BUILD_DIR)/*; \
	fi
	if [ -f $(PKG_BUILD_DIR)/aclocal.m4 ]; then \
		touch -d '-10 sec' $(PKG_BUILD_DIR)/aclocal.m4; \
	fi
	if [ -f $(PKG_BUILD_DIR)/Makefile.in ]; then \
		touch $(PKG_BUILD_DIR)/Makefile.in; \
	fi
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
				rsync -rv $(PKG_PATCH_DIR)/* patches/ ; \
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
	rm -rf $(BUILD_INSTALL_DIR)
	#rm -rf $(PKG_BUILD_DIR)
	
.PHONY: rcX romfs
romfs:
	$(ROMFSINST) $(PKG_BUILD_DIR)/src/phddns /bin/phddns
	
rcX:
	
