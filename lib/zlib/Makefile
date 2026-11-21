PWD = $(shell pwd)
SOURCEDIR := zlib-1.2.11
PKG_PATCH_DIR := $(CURDIR)/patches/zlib-1.2.11
PKG_OPTS = --prefix=/
LIB_NAME = libz

all: do_patch configure
	$(MAKE) -C $(SOURCEDIR)
	$(MAKE) -C $(SOURCEDIR) install DESTDIR=$(TMPFSDIR)
	$(MAKE) -C $(SOURCEDIR) install DESTDIR=$(PWD)/.install
ifeq ($(CONFIG_USER_MINIZIP), y)
	$(MAKE) -C $(SOURCEDIR)/contrib/minizip CC=$(CC) 
endif

do_patch:
	if [ -d $(SOURCEDIR) ]; then \
		cd $(SOURCEDIR); \
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
	fi;

configure:
	if [ -d $(SOURCEDIR) ]; then \
		cd $(SOURCEDIR); \
		if [ ! -f zlib.pc ]; then \
			./configure $(PKG_OPTS); \
		fi; \
	fi
	
romfs:
	cp -fpR $(PWD)/.install/lib/$(LIB_NAME)*.so $(ROMFSDIR)/lib
	cp -fpR $(PWD)/.install/lib/$(LIB_NAME)*.so.* $(ROMFSDIR)/lib
ifeq ($(CONFIG_USER_MINIZIP), y)
	cp -fpR $(SOURCEDIR)/contrib/minizip/minizip $(ROMFSDIR)/bin
endif

clean:
	- rm -rf $(PWD)/.install
	- [ -f $(SOURCEDIR)/Makefile ] && $(MAKE) -C $(SOURCEDIR) uninstall DESTDIR=$(TMPFSDIR)
	- [ -f $(SOURCEDIR)/Makefile ] && $(MAKE) -C $(SOURCEDIR) distclean
	- $(MAKE) -C $(SOURCEDIR)/contrib/minizip clean
