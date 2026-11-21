include $(CONFIG_CONFIG)
PWDDIR =$(shell pwd)
.EXPORT_ALL_VARIABLES:

ifeq ($(CONFIG_LIB_LIBICONV_1_15), y)
LIB_SOURCE=libiconv-1.15
endif

ifeq ($(CONFIG_LIB_LIBICONV_1_14), y)
LIB_SOURCE=libiconv-1.14
endif

ifdef CROSS_HOST #arm/arm64, config.arch will export CROSS_HOST and CROSS_TARGET
LIB_OPTS = --host=$(CROSS_HOST)-linux \
	--target=$(CROSS_TARGET)-linux
else
ifeq ($(ARCH), mips)
LIBUSB_ARCH=mips
LIB_OPTS = --host=$(LIBUSB_ARCH)-linux \
	--target=$(LIBUSB_ARCH)-linux 
endif
endif

include $(CHECKSEC_CONFIG)

all: iconv

iconv:
	if [ ! -f $(LIB_SOURCE)/Makefile ]; then \
		cd $(LIB_SOURCE) && ./configure --prefix=$(PWDDIR)/$(LIB_SOURCE)/tmp --enable-m4_pattern_allow --disable-nls $(LIB_OPTS); \
		if [ "$(LIB_SOURCE)" == "libiconv-1.15" ]; then \
			sed -i 's/hardcode_libdir_flag_spec="\\$$wl-rpath \\$$wl\\$$libdir"/hardcode_libdir_flag_spec="\\$$wl-rpath-link=\\$$libdir"/g' libtool; \
		elif [ "$(LIB_SOURCE)" == "libiconv-1.14" ]; then \
			sed -i 's/hardcode_libdir_flag_spec="\\$${wl}-rpath \\$${wl}\\$$libdir"/hardcode_libdir_flag_spec="\\$${wl}-rpath-link=\\$$libdir"/g' libtool; \
		fi; \
	fi
	$(MAKE) -C $(LIB_SOURCE)
	$(MAKE) -C $(LIB_SOURCE) install
	cp -a $(PWDDIR)/$(LIB_SOURCE)/tmp/lib/* $(ROOTDIR)/lib/librtk
	cp -a $(PWDDIR)/$(LIB_SOURCE)/tmp/include/* $(ROOTDIR)/include/rtk/
clean:
	$(MAKE) -C $(LIB_SOURCE) clean
	[ ! -e $(LIB_SOURCE)/Makefile ] || rm $(LIB_SOURCE)/Makefile;
	
distclean:
	$(MAKE) -C $(LIB_SOURCE) distclean

romfs:
	cp -a $(PWDDIR)/$(LIB_SOURCE)/tmp/lib/* $(ROOTDIR)/romfs/lib
	cp -a $(PWDDIR)/$(LIB_SOURCE)/tmp/bin/* $(ROOTDIR)/romfs/bin

.PHONY: all clean distclean romfs
