.EXPORT_ALL_VARIABLES:
DIR_SOURCE=libusb-1.0.21
LIBUSB_ARCH=mips
ifdef CROSS_HOST #arm/arm64
CONF_OPTS = --host=$(CROSS_HOST) \
	--target=$(CROSS_TARGET) \
	--enable-udev=no
else
CONF_OPTS = --host=$(LIBUSB_ARCH)-linux \
	--target=$(LIBUSB_ARCH)-linux \
	--enable-udev=no
endif

all: libusb

libusb:
	if [ ! -f $(DIR_SOURCE)/Makefile ]; then \
		cd $(DIR_SOURCE) && ./configure $(CONF_OPTS); \
	fi 
	$(MAKE) -C $(DIR_SOURCE)
	$(CC) -o $(DIR_SOURCE)/examples/testlibusb $(DIR_SOURCE)/examples/testlibusb.c -I$(DIR_SOURCE)/libusb -L$(DIR_SOURCE)/libusb/.libs -lusb-1.0
	install -m 644 $(DIR_SOURCE)/libusb/.libs/libusb-1.0.so.0.1.0 $(ROOTDIR)/lib/librtk/libusb-1.0.so.0.1.0
	ln -sf libusb-1.0.so.0.1.0 $(ROOTDIR)/lib/librtk/libusb-1.0.so
	ln -sf libusb-1.0.so.0.1.0 $(ROOTDIR)/lib/librtk/libusb-1.0.so.0
	install -m 644 $(DIR_SOURCE)/libusb/libusb.h $(ROOTDIR)/include/rtk/libusb.h

clean:
	$(MAKE) -C $(DIR_SOURCE) distclean
	
distclean:
	$(MAKE) -C $(DIR_SOURCE) distclean

romfs:
	$(ROMFSINST) $(DIR_SOURCE)/libusb/.libs/libusb-1.0.so.0.1.0 /lib/libusb-1.0.so.0.1.0
	$(ROMFSINST) -s libusb-1.0.so.0.1.0 /lib/libusb-1.0.so
	$(ROMFSINST) -s libusb-1.0.so.0.1.0 /lib/libusb-1.0.so.0
	$(ROMFSINST) $(DIR_SOURCE)/examples/testlibusb /bin/testlibusb

.PHONY: all clean distclean romfs
