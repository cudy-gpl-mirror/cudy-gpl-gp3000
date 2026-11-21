
IPSEC_ROOT_DIR=$(shell pwd)
ifeq ($(CONFIG_USER_STRONGSWAN_V5_8_4), y)
IPSEC_VER:=strongswan-5.8.4
else 
IPSEC_VER:=strongswan-5.8.4
endif

ifeq ($(ARCH), mips)
CROSS_HOST = mips-linux
CROSS_TARGET = mips-linux
else
ifdef CONFIG_32BIT_IMAGE
CROSS_HOST = arm-linux
CROSS_TARGET = arm-linux
else
CROSS_HOST = aarch64-linux
CROSS_TARGET = asdk64-linux
endif
endif

IPSEC_CFLAGS=--host=$(CROSS_HOST) --prefix=/ --sbindir=/bin --bindir=/bin --target=$(CROSS_TARGET) --enable-gcm CFLAGS="$(CFLAGS) -I$(ROOTDIR)/lib/libgmp" LDFLAGS="$(LDFLAGS) -L$(CONFIG_RSDK_DIR)/lib -L$(ROOTDIR)/lib/libgmp/.libs" --sysconfdir="/etc" --with-ipseclibdir=/usr/lib


all: preconfig strongswan

preconfig:
	if [ ! -f $(IPSEC_ROOT_DIR)/$(IPSEC_VER)/Makefile ];then \
		cd $(IPSEC_ROOT_DIR)/$(IPSEC_VER); \
		autoreconf -ivf && ./configure $(IPSEC_CFLAGS); \
		sed -i 's/hardcode_libdir_flag_spec="\\$$wl-rpath \\$$wl\\$$libdir"/hardcode_libdir_flag_spec="\\$$wl-rpath-link=\\$$libdir"/g' libtool; \
		cd -; \
	fi;
	
strongswan:
	$(MAKE) -C $(IPSEC_ROOT_DIR)/$(IPSEC_VER)

distclean:
	$(MAKE) -C $(IPSEC_ROOT_DIR)/$(IPSEC_VER) distclean

clean:
	$(MAKE) -C $(IPSEC_ROOT_DIR)/$(IPSEC_VER) clean

romfs:	
	$(MAKE) -C $(IPSEC_ROOT_DIR)/$(IPSEC_VER) romfs
	$(ROMFSINST) $(IPSEC_ROOT_DIR)/$(IPSEC_VER)/src/_updown/_updown /etc/scripts/_updown

rcX:

.PHONY: all clean distclean romfs rcX
