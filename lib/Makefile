#
#	Makefile -- Master makefile for all libraries.
#

.EXPORT_ALL_VARIABLES:

#
# Include architecture specific build rules.
#

ifndef ROOTDIR
ROOTDIR=..
endif

UCLINUX_BUILD_LIB=1
include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)
include $(CHECKSEC_LIB_CONFIG)

#
# always build these dir_
#
ifdef CONFIG_USE_RSDK_WRAPPER
dir_y  = libnet libcrypt_old
dir_n  =
dir_   =
LN     = ln
else #CONFIG_USE_RSDK_WRAPPER
dir_y  = $(LIBCDIR) libnet libcrypt_old
dir_n  =
dir_   =

ifeq ($(LIBCDIR),libc)
dir_y += libm
endif
endif #CONFIG_USE_RSDK_WRAPPER

ifeq ($(CONFIG_LUNA),y)
ifeq ($(CONFIG_USER_APOLLO_DIAGSHELL),y)
ifneq ($(CONFIG_GPON_FEATURE)_$(CONFIG_EPON_FEATURE),_)
# for calibration function in rtkbosa tool
dir_y += gsl
endif
endif
endif

ifeq ($(CONFIG_LIB_LIBSSL_DES_OFF),y)
libssl_cfgopt += no-des
endif

ifeq ($(CONFIG_LIB_LIBSSL_SSL23_OFF),y)
libssl_cfgopt = no-ssl2 no-ssl3
endif

ifeq ($(CONFIG_TELMEX_DEV),y)
libssl_cfgopt += no-tls1 no-rc4
endif
#
# and build these if someone needs them
#
dir_$(CONFIG_LIB_LIBFFI)       += libffi
dir_$(CONFIG_LIB_LIBFFI_FORCE) += libffi
dir_$(CONFIG_LIB_SNAPGEAR)     += libsnapgear
dir_$(CONFIG_LIB_LIBAES)       += libaes
dir_$(CONFIG_LIB_LIBAES_FORCE) += libaes
dir_$(CONFIG_LIB_LIBDES)       += libdes
dir_$(CONFIG_LIB_LIBDES_FORCE) += libdes
dir_$(CONFIG_LIB_LIBSSL)       += libssl
dir_$(CONFIG_LIB_LIBSSL_FORCE) += libssl
dir_$(CONFIG_LIB_LIBMATRIXSSL) += libmatrixssl
dir_$(CONFIG_LIB_LIBGMP)       += libgmp
dir_$(CONFIG_LIB_LIBGMP_FORCE) += libgmp
dir_$(CONFIG_LIB_LIBG)         += libg
dir_$(CONFIG_LIB_LIBG_FORCE)   += libg
dir_$(CONFIG_LIB_LIBPCAP)      += libpcap
dir_$(CONFIG_LIB_LIBPCAP_FORCE)+= libpcap
dir_$(CONFIG_LIB_ZLIB)         += zlib
dir_$(CONFIG_LIB_ZLIB_FORCE)   += zlib
dir_$(CONFIG_LIB_LIBJPEG)      += libjpeg
dir_$(CONFIG_LIB_LIBJPEG_FORCE)+= libjpeg
dir_$(CONFIG_LIB_LIBSNAPGEARPP) += libsnapgear++
dir_$(CONFIG_LIB_LIBCCMALLOC)  += libccmalloc
dir_$(CONFIG_LIB_LIBMD5_FORCE) += libmd5
dir_$(CONFIG_LIB_LIBMNL)	   += libmnl
dir_$(CONFIG_LIB_LIBMD5_FORCE) += libmnl
dir_$(CONFIG_LIB_LIBNFNETLINK) += libnfnetlink
dir_$(CONFIG_LIB_LIBNFNETLINK_FORCE) += libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_LOG) += libnetfilter_log #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_LOG_FORCE) += libnetfilter_log #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_QUEUE) += libnetfilter_queue #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_QUEUE_FORCE) += libnetfilter_queue #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_CONNTRACK) += libnetfilter_conntrack #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_CONNTRACK_FORCE) += libnetfilter_conntrack #depends on libnfnetlink
dir_$(CONFIG_LIB_LIBNETFILTER_CTHELPER) += libnetfilter_cthelper #depends on libmnl
dir_$(CONFIG_LIB_LIBNETFILTER_CTHELPER_FORCE) += libnetfilter_cthelper #depends on libmnl
dir_$(CONFIG_LIB_LIBNETFILTER_CTTIMEOUT) += libnetfilter_cttimeout #depends on libmnl
dir_$(CONFIG_LIB_LIBNETFILTER_CTTIMEOUT_FORCE) += libnetfilter_cttimeout #depends on libmnl
# Mason Yu. for ipv6
dir_$(CONFIG_LIB_FLEX) += libflex
dir_$(CONFIG_LIB_NCURSES)      += ncurses
dir_$(CONFIG_LIB_NCURSES_FORCE)+= ncurses
dir_$(CONFIG_LIB_LIBTERMCAP)      += libtermcap
dir_$(CONFIG_LIB_LIBTERMCAP_FORCE)+= libtermcap
dir_$(CONFIG_LIB_LIBUDEV_FORCE) += eudev-3.2
dir_$(CONFIG_LIB_LIBUSB_FORCE) += libusb-1.0.21
dir_$(CONFIG_LIB_XZ)			+= xz-5.0.8
dir_$(CONFIG_LIB_XZ_FORCE)		+= xz-5.0.8
dir_$(CONFIG_LIB_LIBNL) 	+= libnl
dir_$(CONFIG_LIB_LIBNL_FORCE)	+= libnl
dir_$(CONFIG_LIB_LIBEXPAT)      += libexpat
dir_$(CONFIG_LIB_LIBEXPAT_FORCE)      += libexpat
dir_$(CONFIG_LIB_LIBICONV)		+=libiconv
dir_$(CONFIG_LIB_JSON)      += json-c
dir_$(CONFIG_USER_JSON_C)   += json-c
dir_$(CONFIG_LIB_LIBUBOX)	+= libubox
dir_$(CONFIG_LIB_UBUS)	    += ubus
dir_$(CONFIG_LIB_PIXMAN)        += libpixman
dir_$(CONFIG_LIB_LIBUUID)	+= libuuid
dir_$(CONFIG_LIB_LIBUUID_FORCE)	+= libuuid
dir_$(CONFIG_LIB_POPT)		+= popt
dir_$(CONFIG_LIB_POPT_FORCE)	+= popt

#for RTK created
dir_$(CONFIG_LIB_LIBRTK_TIMERLIST)			+= librtk_timerlist
dir_$(CONFIG_LIB_LIBRTK_TIMERLIST_FORCE)	+= librtk_timerlist
dir_$(CONFIG_LIB_LIBRTK_LED)			+= librtk_led
dir_$(CONFIG_LIB_TIRPC_FORCE) += libtirpc
dir_$(CONFIG_LIB_BINUTILS)  += binutils
dir_$(CONFIG_LIB_ELF)       += libelf
dir_$(CONFIG_LIB_ELF_FORCE) += libelf
dir_$(CONFIG_LIB_BPF)       += libbpf
dir_$(CONFIG_LIB_BPF_FORCE) += libbpf
dir_$(CONFIG_LIB_EVENT)     += libevent2
dir_$(CONFIG_LIB_EVENT_FORCE) += libevent2
dir_$(CONFIG_LIB_CJSON) += cJSON
dir_$(CONFIG_LIB_EV) += libev
dir_$(CONFIG_LIB_JSONRPC) += jsonrpc-c

ifeq ($(or $(CONFIG_LIB_LIBSSL_1_1_1D),$(CONFIG_LIB_LIBSSL_1_1_1I)),y)
OPTIONS_SSL=no-ec_nistp_64_gcc_128 no-md2 no-rc5 no-rfc3779 no-sctp no-zlib no-zlib-dynamic no-static-engine no-tests
else ifeq ($(CONFIG_LIB_LIBSSL_1_1_0H), y)
OPTIONS_SSL=no-ssl3 no-rc4 no-ec_nistp_64_gcc_128 no-md2 no-rc5 no-rfc3779 no-sctp no-zlib no-zlib-dynamic no-static-engine
else
OPTIONS_SSL=no-ssl2 no-ssl3 no-rc4 no-ec_nistp_64_gcc_128 no-gmp no-jpake no-krb5 no-md2 no-rc5 no-rfc3779 no-sctp no-store no-zlib no-zlib-dynamic no-static-engine
endif

ifeq ($(CONFIG_USER_RTK_NESSUS_SCAN_FIXED),y)
OPTIONS_SSL += no-rc4
endif

.PHONY: all
all: pre_compile
ifeq ($(findstring glibc,$(LIBCDIR)),glibc)
	[ -d $(LIBCDIR) ] || mkdir $(LIBCDIR)
	if [ ! -f $(LIBCDIR)/config.cache ]; then \
		cd $(LIBCDIR); \
		export CFLAGS="$(VENDOR_CFLAGS) -DEMBED -O2"; \
		unset LDFLAGS; \
		../configure  --prefix= \
			$(GLIBC_TARGET_OPTS) --build=i386-redhat-linux \
			--with-headers=$(ROOTDIR)/$(LINUXDIR)/include \
			--without-cvs --without-gd \
			--disable-profile --disable-force-install \
			--enable-add-ons ; \
	fi
endif
ifdef CONFIG_USE_RSDK_WRAPPER
	[ -d $(ROOTDIR)/lib/librtk ] || mkdir -p $(ROOTDIR)/lib/librtk
	[ -d $(ROOTDIR)/include/rtk ] || mkdir -p $(ROOTDIR)/include/rtk
	[ -d $(ROOTDIR)/include/rtk/linux ] || mkdir -p $(ROOTDIR)/include/rtk/linux
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/sockmark_define.h $(ROOTDIR)/include/rtk/sockmark_define.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/options.h $(ROOTDIR)/include/rtk/options.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/mib.h $(ROOTDIR)/include/rtk/mib.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/mibtbl.h $(ROOTDIR)/include/rtk/mibtbl.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/sysconfig.h $(ROOTDIR)/include/rtk/sysconfig.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/utility.h $(ROOTDIR)/include/rtk/utility.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/rtk_interface.h $(ROOTDIR)/include/rtk/rtk_interface.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/adslif.h $(ROOTDIR)/include/rtk/adslif.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/adsl_drv.h $(ROOTDIR)/include/rtk/adsl_drv.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_dhcpv6.h $(ROOTDIR)/include/rtk/subr_dhcpv6.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/ipv6_info.h $(ROOTDIR)/include/rtk/ipv6_info.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_wlan.h $(ROOTDIR)/include/rtk/subr_wlan.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_net.h $(ROOTDIR)/include/rtk/subr_net.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_multiap.h $(ROOTDIR)/include/rtk/subr_multiap.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/rtk_timer.h $(ROOTDIR)/include/rtk/rtk_timer.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_led.h $(ROOTDIR)/include/rtk/subr_led.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_lannetinfo.h $(ROOTDIR)/include/rtk/subr_lannetinfo.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/portald.h $(ROOTDIR)/include/rtk/portald.h
        ifdef CONFIG_USER_XDSL_SLAVE
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_nfbi_api.h $(ROOTDIR)/include/rtk/subr_nfbi_api.h
        endif #CONFIG_USER_XDSL_SLAVE
	$(LN) -fs $(ROOTDIR)/user/boa/src/msgq.h $(ROOTDIR)/include/rtk/msgq.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/tz.h $(ROOTDIR)/include/rtk/tz.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/cJSON.h $(ROOTDIR)/include/rtk/cJSON.h
ifdef CONFIG_YUEME
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/slre.h $(ROOTDIR)/include/rtk/slre.h
endif
ifdef CONFIG_CU_BASEON_YUEME
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/slre.h $(ROOTDIR)/include/rtk/slre.h
endif
ifdef CONFIG_CU
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_cu.h $(ROOTDIR)/include/rtk/subr_cu.h
endif
ifdef CONFIG_E8B
	$(LN) -fs $(ROOTDIR)/user/boa/src/defs_e8.h $(ROOTDIR)/include/rtk/defs_e8.h
endif
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_ctcsdn.h $(ROOTDIR)/include/rtk/subr_ctcsdn.h
        ifdef CONFIG_LUNA
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/chip_deps.h $(ROOTDIR)/include/rtk/chip_deps.h
        ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/fc_api.h $(ROOTDIR)/include/rtk/fc_api.h
        endif
        endif
ifdef CONFIG_CU_SECURITY_FUNC
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/security_func.h $(ROOTDIR)/include/rtk/security_func.h
endif
ifneq ($(CONFIG_ATM),)
	$(LN) -fs $(ROOTDIR)/$(LINUXDIR)/include/net/rtl/rtl_atm.h $(ROOTDIR)/include/rtk/linux/rtl_atm.h
endif
ifdef CONFIG_RTL_SMUX_DEV
	$(LN) -fs $(ROOTDIR)/$(LINUXDIR)/drivers/soc/realtek/rtk_smux.h $(ROOTDIR)/include/rtk/linux/rtk_smux.h
endif
ifdef CONFIG_CMCC
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/network_core.h $(ROOTDIR)/include/rtk/network_core.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/accessservices.h $(ROOTDIR)/include/rtk/accessservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/addressservices.h $(ROOTDIR)/include/rtk/addressservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/commservices.h $(ROOTDIR)/include/rtk/commservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/lanservices.h $(ROOTDIR)/include/rtk/lanservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/transferservices.h $(ROOTDIR)/include/rtk/transferservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/voipservices.h $(ROOTDIR)/include/rtk/voipservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/mangement.h $(ROOTDIR)/include/rtk/mangement.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/permission.h $(ROOTDIR)/include/rtk/permission.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/rtkdebug.h $(ROOTDIR)/include/rtk/rtkdebug.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/osgidebug.h $(ROOTDIR)/include/rtk/osgidebug.h
	$(LN) -fs $(ROOTDIR)/user/libosgi_ipc/osgi_ipc.h $(ROOTDIR)/include/rtk/osgi_ipc.h
endif
ifdef CONFIG_CU_BASEON_CMCC
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/network_core.h $(ROOTDIR)/include/rtk/network_core.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/accessservices.h $(ROOTDIR)/include/rtk/accessservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/addressservices.h $(ROOTDIR)/include/rtk/addressservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/commservices.h $(ROOTDIR)/include/rtk/commservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/lanservices.h $(ROOTDIR)/include/rtk/lanservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/transferservices.h $(ROOTDIR)/include/rtk/transferservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/voipservices.h $(ROOTDIR)/include/rtk/voipservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/mangement.h $(ROOTDIR)/include/rtk/mangement.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/permission.h $(ROOTDIR)/include/rtk/permission.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/rtkdebug.h $(ROOTDIR)/include/rtk/rtkdebug.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/osgidebug.h $(ROOTDIR)/include/rtk/osgidebug.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_cu.h $(ROOTDIR)/include/rtk/subr_cu.h
	$(LN) -fs $(ROOTDIR)/user/libosgi_ipc/osgi_ipc.h $(ROOTDIR)/include/rtk/osgi_ipc.h
endif
ifdef CONFIG_CTC_FTTR_MASTER
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/network_core.h $(ROOTDIR)/include/rtk/network_core.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/accessservices.h $(ROOTDIR)/include/rtk/accessservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/addressservices.h $(ROOTDIR)/include/rtk/addressservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/commservices.h $(ROOTDIR)/include/rtk/commservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/lanservices.h $(ROOTDIR)/include/rtk/lanservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/transferservices.h $(ROOTDIR)/include/rtk/transferservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/voipservices.h $(ROOTDIR)/include/rtk/voipservices.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/mangement.h $(ROOTDIR)/include/rtk/mangement.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/permission.h $(ROOTDIR)/include/rtk/permission.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/rtkdebug.h $(ROOTDIR)/include/rtk/rtkdebug.h
	$(LN) -fs $(ROOTDIR)/user/cmcc/libnetwork_core/include/osgidebug.h $(ROOTDIR)/include/rtk/osgidebug.h
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_segment_speedtest.h $(ROOTDIR)/include/rtk/subr_segment_speedtest.h
endif
ifdef CONFIG_CMCC_FTTR_MASTER
	$(LN) -fs $(ROOTDIR)/user/boa/src/LINUX/subr_segment_speedtest.h $(ROOTDIR)/include/rtk/subr_segment_speedtest.h
endif
ifdef CONFIG_DEV_xDSL
	$(LN) -fs $(ROOTDIR)/$(LINUXDIR)/include/net/rtl/rtl_sar.h $(ROOTDIR)/include/rtk/linux/rtl_sar.h
endif

ifdef CONFIG_ATM
	$(LN) -fs $(ROOTDIR)/$(LINUXDIR)/include/net/rtl/rtl_atm.h $(ROOTDIR)/include/rtk/linux/rtl_atm.h
endif
	$(LN) -fs $(ROOTDIR)/$(LINUXDIR)/include/net/rtl/rtl_genl.h $(ROOTDIR)/include/rtk/linux/rtl_genl.h

ifdef CONFIG_RTK_BOARDID
	$(LN) -fs $(ROOTDIR)/$(LINUXDIR)/include/soc/boardid.h $(ROOTDIR)/include/rtk/linux/boardid.h
endif

endif #CONFIG_USE_RSDK_WRAPPER
	for i in $(dir_y) ; do \
		if [ -d "$$i" ]; then \
			if [ ! -f $$i/Makefile ]; then \
				cd $$i && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
			fi; \
			$(MAKE) -C $$i; \
		fi \
	done
ifeq ($(CONFIG_LIB_LIBSSL_FORCE),y)
	if [ -e libssl/libcrypto.so ]; then \
		cp libssl/libcrypto.so librtk; \
	fi
	if [ -e libssl/libssl.so ]; then \
		cp libssl/libssl.so librtk; \
	fi
endif
ifeq ($(or $(CONFIG_LIB_LIBSSL_1_1_1D),$(CONFIG_LIB_LIBSSL_1_1_1I)),y)
	if [ ! -d libssl/lib ];then \
		mkdir libssl/lib; \
	fi
	if [ -e libssl/libcrypto.so ]; then \
		cp libssl/libcrypto.so libssl/lib; \
		cp libssl/libcrypto.so librtk; \
	fi
	if [ -e libssl/libssl.so ]; then \
		cp libssl/libssl.so libssl/lib; \
		cp libssl/libssl.so librtk; \
		cp -d libssl/libssl.so* $(TMPFSDIR)/lib/;	\
	fi
endif

romfs:
	for i in $(dir_y) ; do \
		[ ! -d $$i ] || $(MAKE) -C $$i romfs || exit $$? ; \
	done
	-$(STRIP) $(ROMFSDIR)/lib/*
	
rcX:
	for i in $(dir_y) ; do \
		if [ "`grep "^rcX:" $$i/Makefile`" = "rcX:" ]; then \
			 $(MAKE) -C $$i rcX;\
		fi \
	done

%_menuconfig:
	[ ! -d "$(@:_menuconfig=)" ] || $(MAKE) -C $(@:_menuconfig=) menuconfig

%_only:
	if [ -d "$(@:_only=)" ]; then \
		if [ ! -f $(@:_only=)/Makefile ]; then \
			cd $(@:_only=) && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
		fi; \
		$(MAKE) -C $(@:_only=); \
	fi

%_romfs:
	[ ! -d "$(@:_romfs=)" ] || $(MAKE) -C $(@:_romfs=) romfs

%_clean:
	[ ! -d "$(@:_clean=)" ] || $(MAKE) -C $(@:_clean=) clean
#if define CONFIG_LIB_LIBSSL or CONFIG_LIB_LIBSSL_FORCE
ifeq ($(or $(CONFIG_LIB_LIBSSL),$(CONFIG_LIB_LIBSSL_FORCE)), y)
	if [ "$(@:_clean=)" = "libssl" -a -f libssl/config_done ]; then \
		rm libssl/config_done ; \
	fi
endif

clean:
ifdef CONFIG_USE_RSDK_WRAPPER
	-rm -rf $(ROOTDIR)/lib/librtk
endif #CONFIG_USE_RSDK_WRAPPER
ifeq ($(findstring glibc,$(LIBCDIR)),glibc)
	-rm -rf $(LIBCDIR)
endif
	-for i in $(dir_y) $(dir_n) $(dir_); do \
		[ ! -d $$i ] || $(MAKE) -C $$i clean ; \
	done
#if define CONFIG_LIB_LIBSSL or CONFIG_LIB_LIBSSL_FORCE
ifeq ($(or $(CONFIG_LIB_LIBSSL),$(CONFIG_LIB_LIBSSL_FORCE)), y)
	[ ! -f libssl/config_done ] || rm libssl/config_done;
endif

ifeq ($(or $(CONFIG_LIB_LIBPCAP),$(CONFIG_LIB_LIBPCAP_FORCE)), y)
	[ ! -f libpcap/Makefile ] || rm libpcap/Makefile;
endif

pre_compile:
ifeq ($(CONFIG_LIB_LIBSSL),y)
ifeq ($(or $(CONFIG_LIB_LIBSSL_1_1_1D),$(CONFIG_LIB_LIBSSL_1_1_1I)),y)
	if [ ! -f libssl/config_done ]; then \
		cd libssl; \
		./Configure shared $(OPTIONS_SSL) uClinux-dist $(libssl_cfgopt) CC=gcc LD=ld AR=ar LDFLAGS="$(LDFLAGS) -pthread"; \
		cat Makefile.rtk >> Makefile; \
		$(MAKE) depend;touch config_done; \
		cd - ;\
	fi
	# remove -pie for shared library when security options set
	sed -i 's/LIB_LDFLAGS=-shared -Wl,-Bsymbolic $$(CNF_LDFLAGS) $$(LDFLAGS)/LIB_LDFLAGS=-shared -Wl,-Bsymbolic $$(CNF_LDFLAGS) $$(filter-out -pie,$$(LDFLAGS))/' libssl/Makefile
	sed -i 's/DSO_LDFLAGS=-shared -Wl,-Bsymbolic $$(CNF_LDFLAGS) $$(LDFLAGS)/DSO_LDFLAGS=-shared -Wl,-Bsymbolic $$(CNF_LDFLAGS) $$(filter-out -pie,$$(LDFLAGS))/' libssl/Makefile
else
	if [ ! -f libssl/config_done ]; then \
	cd libssl;./Configure shared $(OPTIONS_SSL) uClinux-dist $(libssl_cfgopt) -lpthread; \
	$(MAKE) depend;touch config_done; \
	fi
endif
endif

ifeq ($(CONFIG_LIB_LIBSSL_FORCE),y)
ifeq ($(or $(CONFIG_LIB_LIBSSL_1_1_1D),$(CONFIG_LIB_LIBSSL_1_1_1I)),y)
	if [ ! -f libssl/config_done ]; then \
		cd libssl; \
		./Configure shared  $(OPTIONS_SSL) uClinux-dist $(libssl_cfgopt) CC=gcc LD=ld AR=ar LDFLAGS=-pthread; \
		cat Makefile.rtk >> Makefile; \
		$(MAKE) depend;touch config_done; \
	fi
else
	if [ ! -f libssl/config_done ]; then \
	cd libssl;./Configure shared $(OPTIONS_SSL) uClinux-dist $(libssl_cfgopt) -lpthread; \
	$(MAKE) depend;touch config_done; \
	fi
endif
endif
	@echo "finished pre_compile"

#cd libssl;./Configure no-zlib no-asm no-hw no-dso no-comp no-engine shared no-ssl2 no-ssl3 no-sse2 no-aes no-bf no-cast no-dh  no-dsa no-ec no-ecdh no-ecdsa no-idea no-md2 no-md4 no-mdc2 no-rc2 no-rc5 no-ripemd no-rmd160 no-krb5  mips-linux;$(MAKE) depend;touch config_done; \


