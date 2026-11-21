.EXPORT_ALL_VARIABLES:
LIB_SOURCE=eudev-3.2
LIBUSB_ARCH=mips
ifdef CROSS_HOST #arm/arm64, config.arch will export CROSS_HOST and CROSS_TARGET
LIB_OPTS = --host=$(CROSS_HOST) \
	--target=$(CROSS_TARGET) \
	--disable-manpages \
	--disable-introspection \
	--disable-blkid \
	--disable-selinux \
	--disable-kmod \
	--disable-hwdb \
	--disable-rule-generator
else
LIB_OPTS = --host=$(LIBUSB_ARCH)-linux \
	--target=$(LIBUSB_ARCH)-linux \
	--disable-manpages \
	--disable-introspection \
	--disable-blkid \
	--disable-selinux \
	--disable-kmod \
	--disable-hwdb \
	--disable-rule-generator
endif
all: eudev

eudev:
	if [ ! -f $(LIB_SOURCE)/Makefile ]; then \
		cd $(LIB_SOURCE) && ./configure $(LIB_OPTS); \
	fi
	$(MAKE) -C $(LIB_SOURCE)
	$(CC) -o udev_example udev_example.c -I$(LIB_SOURCE)/src/libudev -L$(LIB_SOURCE)/src/libudev/.libs -ludev
	install -m 644 $(LIB_SOURCE)/src/libudev/.libs/libudev.so.1.6.3 $(ROOTDIR)/lib/librtk/libudev.so.1.6.3
	ln -sf libudev.so.1.6.3 $(ROOTDIR)/lib/librtk/libudev.so
	ln -sf libudev.so.1.6.3 $(ROOTDIR)/lib/librtk/libudev.so.1
	install -m 644 $(LIB_SOURCE)/src/libudev/libudev.h $(ROOTDIR)/include/rtk/libudev.h

clean:
	$(MAKE) -C $(LIB_SOURCE) distclean
	
distclean:
	$(MAKE) -C $(LIB_SOURCE) distclean

romfs:
	$(ROMFSINST) $(LIB_SOURCE)/src/libudev/.libs/libudev.so.1.6.3 /lib/libudev.so.1.6.3
	$(ROMFSINST) -s libudev.so.1.6.3 /lib/libudev.so
	$(ROMFSINST) -s libudev.so.1.6.3 /lib/libudev.so.1
	$(ROMFSINST) udev_example /bin/udev_example

.PHONY: all clean distclean romfs
