.EXPORT_ALL_VARIABLES:

ifeq ($(CONFIG_USER_UTIl_LINUX_2_39), y)
UTIL_LINUX = util-linux-2.39
else
UTIL_LINUX = util-linux-2.29
endif

LDFLASGS += -lm
UTIL_LINUX_PREFIX=$(shell pwd)/tmp
ifeq ($(ARCH)$(CONFIG_KERNEL_4_4_x),mipsy)
ifneq (,$(findstring g2.23,$(CONFIG_RSDK_DIR)))
ARCH_KERNEL=$(ARCH)-4.4-glibc
else
ARCH_KERNEL=$(ARCH)-4.4
endif
else
ARCH_KERNEL=$(ARCH)
endif
UTIL_LINUX_BINARY=$(shell pwd)/binary/$(ARCH_KERNEL)

ifeq ("$(CROSS_HOST)", "")
CROSS_HOST = $(ARCH)-linux
CROSS_TARGET = $(ARCH)-linux
endif

UTIL_LINUX_OPTS= --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --build=i386-linux \
	--without-python \
	--without-tinfo \
	--disable-libfdisk \
	--disable-fsck \
	--disable-setterm \
	--disable-mountpoint \
	--disable-eject \
	--disable-agetty \
	--disable-colors-default \
	--disable-ul \
	--disable-use-tty-group \
	--disable-logger \
	--disable-pylibmount \
	--disable-libmount \
	--disable-more \
	--without-utempter \
	--disable-last \
	--enable-newgrp \
	--disable-utmpdump \
	--disable-cal

all:
	if [ ! -d $(UTIL_LINUX_BINARY) ]; then \
		$(MAKE) remake_all; \
	else \
		if [ ! -d $(UTIL_LINUX_PREFIX) ]; then \
			mkdir -p $(UTIL_LINUX_PREFIX); \
			rsync -ar $(UTIL_LINUX_BINARY)/* $(UTIL_LINUX_PREFIX); \
		fi; \
		echo $(UTIL_LINUX_BINARY); \
		echo "Directory is existed. If you want to rebuild, please remove it."; \
	fi

remake_all:
ifeq ($(CONFIG_USER_UTIl_LINUX_2_39), y)
	cd $(UTIL_LINUX); \
	[ -f Makefile ] || (./autogen.sh && ./configure $(UTIL_LINUX_OPTS))
else
	cd $(UTIL_LINUX); \
	[ -f Makefile ] || (autoreconf -vf && chmod +x configure && ./configure $(UTIL_LINUX_OPTS))
endif
	$(MAKE) -C $(UTIL_LINUX)
	$(MAKE) -C $(UTIL_LINUX) install DESTDIR=$(UTIL_LINUX_PREFIX)

romfs:
	#$(MAKE) -C $(UTIL_LINUX) romfs
	$(ROMFSINST) $(UTIL_LINUX_PREFIX)/sbin/blkid /bin/blkid2
	$(ROMFSINST) $(UTIL_LINUX_PREFIX)/usr/bin/prlimit /bin/prlimit
	cp -a  $(UTIL_LINUX_PREFIX)/lib/libblkid.so* $(ROOTDIR)/romfs/lib/
	cp -a  $(UTIL_LINUX_PREFIX)/lib/libuuid.so* $(ROOTDIR)/romfs/lib/
	cp -a  $(UTIL_LINUX_PREFIX)/lib/libsmartcols.so* $(ROOTDIR)/romfs/lib/
	$(ROMFSINST) $(UTIL_LINUX_PREFIX)/usr/bin/unshare /usr/bin/unshare
	$(ROMFSINST) $(UTIL_LINUX_PREFIX)/usr/bin/nsenter /usr/bin/nsenter

sync:
	mkdir -p $(UTIL_LINUX_BINARY)
	rsync -ar --exclude=share $(UTIL_LINUX_PREFIX)/* $(UTIL_LINUX_BINARY)

clean:
	rm -rf $(UTIL_LINUX_PREFIX)
	[ -f $(UTIL_LINUX)/Makefile ] && $(MAKE) -C $(UTIL_LINUX) clean && rm -f $(UTIL_LINUX)/Makefile

