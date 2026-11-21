PWD = $(shell pwd)

PARTED_VERSION = 3.4

PKG_TARBALL_DIR = $(PWD)/tarball
PKG_PATCH_DIR = $(PWD)/patches
TMPFS_DIR = $(PWD)/tmpfs
PKG_CONFIG_DIR=$(TMPFS_DIR)/lib/pkgconfig
INCLUDE_INSTALL_DIR = $(ROOTDIR)/include
LIBRTK_INSTALL_DIR = $(ROOTDIR)/lib/librtk

ifeq ("$(CROSS_HOST)", "")
CROSS_HOST = $(ARCH)-linux
CROSS_TARGET = $(ARCH)-linux
endif

PARTED =parted-$(PARTED_VERSION)
PARTED_TARBALL = $(PARTED).tar.xz
PARTED_WEBURL = https://ftp.gnu.org/gnu/parted/
PARTED_OPTS = --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --prefix=$(TMPFS_DIR) PKG_CONFIG_PATH=$(PKG_CONFIG_DIR) \
	--disable-device-mapper --without-readline

.PHONY: prepare_src do_patch $(PARTED)
all: prepare_src do_patch $(PARTED)

prepare_src:
	@echo "Prepare $(PKG_TARBALL_DIR) dir."
	if [ ! -d $(PKG_TARBALL_DIR) ]; then \
		mkdir -p $(PKG_TARBALL_DIR); \
	fi
	@echo "Prepare $(PARTED)"
	if [ ! -d $(PARTED) ]; then \
		if [ ! -f $(PKG_TARBALL_DIR)/$(PARTED_TARBALL) ]; then \
			echo "Get $(PARTED_TARBALL)"; \
			wget -O $(PKG_TARBALL_DIR)/$(PARTED_TARBALL) $(PARTED_WEBURL)/$(PARTED_TARBALL); \
		fi; \
		if [ -f $(PKG_TARBALL_DIR)/$(PARTED_TARBALL) ]; then \
			echo "Untar $(PARTED_TARBALL)"; \
			tar -Jxvf $(PKG_TARBALL_DIR)/$(PARTED_TARBALL) -C $(PWD); \
		else \
			echo "Can NOT prepare $(PARTED)"; \
			exit 1; \
		fi; \
	fi

do_patch: prepare_src
	@echo "No patch for $(PARTED)"

$(PARTED): do_patch
	cd $@ && \
	touch aclocal.m4 ; sleep 1 && \
	touch configure ; sleep 1 && \
	touch `find . -name Makefile.in -print` && \
	[ -f Makefile ] || (./configure $(PARTED_OPTS))
	$(MAKE) -C $@
	$(MAKE) -C $@ install

clean:
	cd $(SOURCEDIR) && make distclean

romfs:
	$(ROMFSINST) $(TMPFS_DIR)/sbin/parted /bin/
	for x in $(shell cd $(TMPFS_DIR)/lib; ls libparted*.so*); do \
		if [ -L $(TMPFS_DIR)/lib/$$x ]; then \
			dest=`readlink $(TMPFS_DIR)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(TMPFS_DIR)/lib/$$x /lib/$$x; \
		fi; \
	done

