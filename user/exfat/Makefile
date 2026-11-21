.EXPORT_ALL_VARIABLES:
EXFAT_INSTALL=$(shell pwd)

ifeq ($(CONFIG_USER_FUSE_2_9_7), y)
LIBFUSE = fuse-2.9.7
endif
ifeq ($(CONFIG_USER_FUSE_3_16_2), y)
LIBFUSE = fuse-3.16.2
endif

ifeq ($(CONFIG_USER_EXFAT_1_2_5), y)
EXFAT_DIR = exfat_1_2_5
endif

EXFAT_CFLAGS+=-I$(EXFAT_INSTALL)/$(LIBFUSE)/include/
ifeq ($(CONFIG_USER_FUSE_2_9_7), y)
EXFAT_LDFLAGS+=-L$(EXFAT_INSTALL)/$(LIBFUSE)/lib/.libs
EXFAT_LIBS=-lfuse
endif

ifeq ($(CONFIG_USER_FUSE_3_16_2), y)
EXFAT_CFLAGS+=-I$(EXFAT_INSTALL)/$(LIBFUSE)/build/
EXFAT_LDFLAGS+=-L$(EXFAT_INSTALL)/$(LIBFUSE)/build/lib
EXFAT_LIBS=-lfuse3
endif

all:
ifeq ($(CONFIG_USER_FUSE_2_9_7), y)
ifdef CROSS_TARGET
	cd $(LIBFUSE); \
	[ -f Makefile ] || ( autoreconf -ivf && chmod +x configure && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) CC="$(CROSS_COMPILE)gcc" )
else
	cd $(LIBFUSE); \
	[ -f Makefile ] || ( autoreconf -ivf && chmod +x configure && ./configure --host=mips-linux --target=mips-linux CC="$(CROSS_COMPILE)gcc" )
endif
	make -C $(LIBFUSE)
endif

ifeq ($(CONFIG_USER_FUSE_3_16_2), y)
	cd $(LIBFUSE)/build; \
	sed -i 's#/share/rlx/asdk-10.2.0-a55-EL-5.10-g2.30-a96-210331/bin/aarch64-linux-gcc#$(CROSS_COMPILE)gcc#g' build.ninja
	cd $(LIBFUSE)/build; \
	sed -i 's#/share/rlx/asdk-10.2.0-a55-EL-5.10-g2.30-a96-210331/bin/aarch64-linux-g++#$(CROSS_COMPILE)g++#g' build.ninja
	cd $(LIBFUSE)/build; \
	sed -i 's#/share/rlx/asdk-10.2.0-a55-EL-5.10-g2.30-a96-210331/aarch64-linux-gnu#$(RSDK)/$(CROSS_TARGET)-gnu#g' build.ninja
	cd $(LIBFUSE)/build; \
	[ -d $(EXFAT_INSTALL)/$(LIBFUSE)/build ] && $(EXFAT_INSTALL)/$(LIBFUSE)/ninja -C $(EXFAT_INSTALL)/$(LIBFUSE)/build -t clean
	$(EXFAT_INSTALL)/$(LIBFUSE)/ninja -C $(EXFAT_INSTALL)/$(LIBFUSE)/build -f $(EXFAT_INSTALL)/$(LIBFUSE)/build/build.ninja
endif

ifdef CROSS_TARGET
	cd $(EXFAT_DIR); \
	[ -f Makefile ] || ( autoreconf --install && chmod +x configure && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) CC="$(CROSS_COMPILE)gcc"  CFLAGS="$(CFLAGS) $(EXFAT_CFLAGS)" LDFLAGS="$(LDFLAGS) $(EXFAT_LDFLAGS)" LIBS="$(EXFAT_LIBS)" )
else
	cd $(EXFAT_DIR); \
	[ -f Makefile ] || ( autoreconf --install && chmod +x configure && ./configure --host=mips-linux --target=mips-linux CC="$(CROSS_COMPILE)gcc"  CFLAGS="$(CFLAGS) $(EXFAT_CFLAGS)" LDFLAGS="$(LDFLAGS) $(EXFAT_LDFLAGS)" LIBS="$(EXFAT_LIBS)" )
endif
	make -C $(EXFAT_DIR)

romfs:	
ifeq ($(CONFIG_USER_FUSE_2_9_7), y)
	$(ROMFSINST)	$(EXFAT_INSTALL)/$(LIBFUSE)/lib/.libs/libfuse.so.2.9.7 /lib/libfuse.so.2.9.7
	ln -fs libfuse.so.2.9.7	$(ROOTDIR)/romfs/lib/libfuse.so
	ln -fs libfuse.so.2.9.7	$(ROOTDIR)/romfs/lib/libfuse.so.2
	
	$(ROMFSINST)	$(EXFAT_INSTALL)/$(LIBFUSE)/lib/.libs/libulockmgr.a /lib/libulockmgr.a
	$(ROMFSINST)	$(EXFAT_INSTALL)/$(LIBFUSE)/lib/libulockmgr.la /lib/libulockmgr.la
	$(ROMFSINST)	$(EXFAT_INSTALL)/$(LIBFUSE)/lib/.libs/libulockmgr.so.1.0.1 /lib/libulockmgr.so.1.0.1
	
	ln -fs libulockmgr.so.1.0.1	$(ROOTDIR)/romfs/lib/libulockmgr.so
	ln -fs libulockmgr.so.1.0.1	$(ROOTDIR)/romfs/lib/libulockmgr.so.1
endif

ifeq ($(CONFIG_USER_FUSE_3_16_2), y)
	$(ROMFSINST)	$(EXFAT_INSTALL)/$(LIBFUSE)/build/lib/libfuse3.so.3.16.2 /lib/libfuse3.so.3.16.2
	ln -fs libfuse3.so.3.16.2	$(ROOTDIR)/romfs/lib/libfuse3.so
	ln -fs libfuse3.so.3.16.2	$(ROOTDIR)/romfs/lib/libfuse3.so.3
endif
	
	$(ROMFSINST)	$(EXFAT_INSTALL)/$(EXFAT_DIR)/fuse/mount.exfat-fuse	/bin/mount.exfat
	$(ROMFSINST)	$(EXFAT_INSTALL)/$(EXFAT_DIR)/fsck/exfatfsck	/bin/exfatfsck
	$(ROMFSINST)	$(EXFAT_INSTALL)/$(EXFAT_DIR)/mkfs/mkexfatfs	/bin/mkexfatfs
	$(ROMFSINST)	$(EXFAT_INSTALL)/$(EXFAT_DIR)/label/exfatlabel	/bin/exfatlabel
	#make -C $(EXFAT_DIR) romfs

clean:
ifeq ($(CONFIG_USER_FUSE_2_9_7), y)
	if [ -f $(LIBFUSE)/Makefile ]; then \
		cd $(LIBFUSE); \
		make clean; \
		rm Makefile; \
	fi;
endif
	if [ -f $(EXFAT_DIR)/Makefile ]; then \
		cd $(EXFAT_DIR); \
		make clean; \
		rm Makefile; \
	fi;
	
