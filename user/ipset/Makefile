########
# ipset tool need libmnl so you need to build libmnl before building ipset.
########
-include $(CONFIG_CONFIG)

ifeq ($(CONFIG_USER_IPSET_7_5), y)
IPSET_DIR=ipset-7.5
else
IPSET_DIR=
endif

# libmnl/Makefile will symbolic link libmnl.h to $(ROOTDIR)/include/libmnl/
# maintrunk use new lib structure, so we don't need to -I and -L
#CFLAGS += -I$(ROOTDIR)/lib/libmnl/libmnl-1.0.4
#LDFLAGS +=-L$(ROOTDIR)/lib/librtk -lmnl
LDFLAGS += -lmnl

#PKG_CONFIG_PATH="$(ROOTDIR)/lib/libmnl/libmnl-1.0.4"
#PKG_CONFIG_PATH=$(PKG_CONFIG_PATH)

all:
	if [ ! -f $(IPSET_DIR)/Makefile ]; then \
		cd $(IPSET_DIR) && ./autogen.sh && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) libmnl_CFLAGS="$(CFLAGS)" \
		libmnl_LIBS="$(LDFLAGS)" --with-kbuild=$(ROOTDIR)/$(LINUXDIR) --disable-silent-rules \
		&& sed -i 's/hardcode_libdir_flag_spec="\\$$wl-rpath \\$$wl\\$$libdir"/hardcode_libdir_flag_spec="\\$$wl-rpath-link=\\$$libdir"/g' libtool \
		&& cd -; \
	fi; \
	make -C $(IPSET_DIR)

clean:
	make -C $(IPSET_DIR) clean
	make -C $(IPSET_DIR) distclean

romfs:
	$(ROMFSINST) $(IPSET_DIR)/src/ipset  /bin/ipset


.PHONY: all clean 
