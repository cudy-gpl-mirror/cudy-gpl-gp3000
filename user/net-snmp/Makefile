#
# Makefile for net-snmp for the Linux OS
#
PKG_NAME:=net-snmp
PKG_VERSION:=5.9.4

PKG_BUILD_DIR:=$(CURDIR)/build/$(PKG_NAME)-$(PKG_VERSION)
PKG_SRC_TAR:=$(CURDIR)/tarball/$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_PATCH_DIR:=$(CURDIR)/patches/$(PKG_VERSION)
FILE_DIR:=$(CURDIR)/file
BUILD_INSTALL_DIR=$(CURDIR)/build/.install
RC_LEVEL=/etc/init.d/rc6

ifeq ($(ARCH), mips)
CROSS_HOST = mips-linux
CROSS_TARGET = mips-linux
else
CROSS_HOST = armv5-linux
CROSS_TARGET = armv5-linux
endif


SNMP_MIB_MODULES_INCLUDED = \
		agent/extend \
		agentx \
		host/hr_device \
		host/hr_disk \
		host/hr_filesys \
		host/hr_network \
		host/hr_partition \
		host/hr_proc \
		host/hr_storage \
		host/hr_system \
		ieee802dot11 \
		if-mib/ifXTable \
		ip-mib/inetNetToMediaTable \
		mibII/at \
		mibII/icmp \
		mibII/ifTable \
		mibII/ip \
		mibII/snmp_mib \
		mibII/sysORTable \
		mibII/system_mib \
		mibII/tcp \
		mibII/udp \
		mibII/vacm_context \
		mibII/vacm_vars \
		rtk/adsl \
		rtk/adsl2 \
		rtk/atm \
		rtk/pon \
		snmpv3/snmpEngine \
		snmpv3/snmpMPDStats \
		snmpv3/usmConf \
		snmpv3/usmStats \
		snmpv3/usmUser \
		tunnel \
		ucd-snmp/diskio \
		ucd-snmp/dlmod \
		ucd-snmp/extensible \
		ucd-snmp/loadave \
		ucd-snmp/memory \
		ucd-snmp/pass \
		ucd-snmp/pass_persist \
		ucd-snmp/proc \
		ucd-snmp/vmstat \
		util_funcs \
		utilities/execute \

SNMP_MIB_MODULES_EXCLUDED = \
		agent_mibs \
		disman/event \
		disman/schedule \
		hardware \
		host \
		if-mib \
		ip-mib \
		mibII \
		notification \
		notification-log-mib \
		snmpv3mibs \
		target \
		tcp-mib \
		ucd_snmp \
		udp-mib \
		utilities \


#MAKE_FLAGS += DYNEXP= PICFLAG= MODULES=
#CFLAGS += -Os -DMAX_DEBUG_LEVEL=0 -D__location__=\\\"\\\" -ffunction-sections -fdata-sections
#LDFLAGS += -Wl,--gc-sections

CONFIGURE_ARGS += \
		--enable-mfd-rewrites \
		--enable-static \
		--with-endianness=$(if $(CONFIG_BIG_ENDIAN),big,little) \
		--with-logfile=/var/log/snmpd.log \
		--with-persistent-directory=/var/snmp/ \
		--with-default-snmp-version=1 \
		--with-sys-contact=root@localhost \
		--with-sys-location=Unknown \
		--enable-applications \
		--disable-debugging \
		--disable-manuals \
		--disable-scripts \
		--with-out-mib-modules="$(SNMP_MIB_MODULES_EXCLUDED)" \
		--with-mib-modules="$(SNMP_MIB_MODULES_INCLUDED)" \
		--with-out-transports="$(SNMP_TRANSPORTS_EXCLUDED)" \
		--with-transports="$(SNMP_TRANSPORTS_INCLUDED)" \
		--without-openssl \
		--without-libwrap \
		--without-mysql \
		--without-rpm \
		--without-zlib \
		--with-nl \
		--disable-perl-cc-checks \
		--disable-embedded-perl \
		--without-perl-modules

CONFIGURE_VARS += \
		ac_cv_header_netlink_netlink_h=yes \
		ac_cv_func_strlcpy=no


ifeq ($(CONFIG_LIB_LIBSSL),y)
LDFLAGS += -L$(TMPFSDIR) -lssl -lcrypto
endif

ifeq ($(CONFIG_USER_RTK_VOIP),y)
LDLAGS += -lvoip_manager
endif




CONFIGURE_ARGS += \
	CFLAGS="$(CFLAGS) $(LIBVOIP) $(LIBMIB) -I$(ROOTDIR)/include" \
	LDFLAGS="$(LDFLAGS) -L$(TMPFSDIR)/lib/" \
	--host=$(CROSS_HOST) --target=$(CROSS_TARGET) --build=i386-linux \
	--prefix=/ \


.PHONY: all check do_patch configure clean romfs rcX

all: check do_patch configure
	make -C $(PKG_BUILD_DIR)/ $(MAKE_FLAGS)

check:
	if [ -f $(PKG_BUILD_DIR)/../.build_version ]; then \
		v=`cat $(PKG_BUILD_DIR)/../.build_version`; \
		if [ ! "$$v" = "$(PKG_VERSION)" ]; then \
			rm -rf $(BUILD_INSTALL_DIR); \
		fi; \
	else \
		rm -rf $(BUILD_INSTALL_DIR); \
	fi
	if [ ! -d $(PKG_BUILD_DIR) -o ! -f $(PKG_BUILD_DIR)/.prepared ]; then \
		rm -rf $(PKG_BUILD_DIR); \
		mkdir -p $(PKG_BUILD_DIR); \
		if [ -f $(PKG_SRC_TAR) ]; then \
			tar -zxvf $(PKG_SRC_TAR) --strip-components 1 -C $(PKG_BUILD_DIR); \
		elif [ -d $(CURDIR)/$(PKG_NAME)-$(PKG_VERSION) ]; then \
			cp -vr $(CURDIR)/$(PKG_NAME)-$(PKG_VERSION)/* $(PKG_BUILD_DIR)/ ; \
		else \
			exit 1; \
		fi; \
		touch $(PKG_BUILD_DIR)/.prepared; \
		echo -n "$(PKG_VERSION)" > $(PKG_BUILD_DIR)/../.build_version; \
	fi

configure: check do_patch
	if [ -d $(PKG_BUILD_DIR)/ ]; then \
		cd $(PKG_BUILD_DIR)/; \
		if [ ! -f Makefile ]; then \
			$(CONFIGURE_VARS) ./configure $(CONFIGURE_ARGS); \
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
	make -C $(PKG_BUILD_DIR)/ distclean
	rm -rf $(BUILD_INSTALL_DIR)

romfs:
	$(ROMFSINST) build/$(PKG_NAME)-$(PKG_VERSION)/agent/.libs/snmpd /bin/snmpd
	$(ROMFSINST) build/$(PKG_NAME)-$(PKG_VERSION)/agent/.libs/libnetsnmpagent.so.40.2.1 /lib/libnetsnmpagent.so.40.2.1
	$(ROMFSINST) -s libnetsnmpagent.so.40.2.1  /lib/libnetsnmpagent.so
	$(ROMFSINST) -s libnetsnmpagent.so.40.2.1  /lib/libnetsnmpagent.so.40
	$(ROMFSINST) build/$(PKG_NAME)-$(PKG_VERSION)/agent/.libs/libnetsnmpmibs.so.40.2.1 /lib/libnetsnmpmibs.so.40.2.1
	$(ROMFSINST) -s libnetsnmpmibs.so.40.2.1  /lib/libnetsnmpmibs.so
	$(ROMFSINST) -s libnetsnmpmibs.so.40.2.1  /lib/libnetsnmpmibs.so.40
	$(ROMFSINST) build/$(PKG_NAME)-$(PKG_VERSION)/snmplib/.libs/libnetsnmp.so.40.2.1 /lib/libnetsnmp.so.40.2.1
	$(ROMFSINST) -s libnetsnmp.so.40.2.1  /lib/libnetsnmp.so
	$(ROMFSINST) -s libnetsnmp.so.40.2.1  /lib/libnetsnmp.so.40
	
.PHONY: rcX
rcX:

