PWD = $(shell pwd)

E2FSPROGS_VERSION = 1.46.2

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

E2FSPROGS = e2fsprogs-$(E2FSPROGS_VERSION)
E2FSPROGS_TARBALL = $(E2FSPROGS).tar.gz
E2FSPROGS_WEBURL = https://mirrors.edge.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v$(E2FSPROGS_VERSION)
E2FSPROGS_OPTS = --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --prefix=$(TMPFS_DIR) PKG_CONFIG_PATH=$(PKG_CONFIG_DIR) \
	--with-udev-rules-dir=no --with-crond-dir=no --with-systemd-unit-dir=no

PROGS:=fsck.ext4 mkfs.ext4

.PHONY: prepare_src do_patch $(E2FSPROGS)
all: prepare_src do_patch $(E2FSPROGS)

prepare_src:
	@echo "Prepare $(PKG_TARBALL_DIR) dir."
	if [ ! -d $(PKG_TARBALL_DIR) ]; then \
		mkdir -p $(PKG_TARBALL_DIR); \
	fi
	@echo "Prepare $(E2FSPROGS)"
	if [ ! -d $(E2FSPROGS) ]; then \
		if [ ! -f $(PKG_TARBALL_DIR)/$(E2FSPROGS_TARBALL) ]; then \
			echo "Get $(E2FSPROGS_TARBALL)"; \
			wget -O $(PKG_TARBALL_DIR)/$(E2FSPROGS_TARBALL) $(E2FSPROGS_WEBURL)/$(E2FSPROGS_TARBALL); \
		fi; \
		if [ -f $(PKG_TARBALL_DIR)/$(E2FSPROGS_TARBALL) ]; then \
			echo "Untar $(E2FSPROGS_TARBALL)"; \
			tar -zxvf $(PKG_TARBALL_DIR)/$(E2FSPROGS_TARBALL) -C $(PWD); \
		else \
			echo "Can NOT prepare $(E2FSPROGS)"; \
			exit 1; \
		fi; \
	fi

do_patch: prepare_src
	@echo "No patch for $(E2FSPROGS)"

$(E2FSPROGS): do_patch
	cd $@; \
	[ -f Makefile ] || (./configure $(E2FSPROGS_OPTS))
	$(MAKE) -C $@
	$(MAKE) -C $@ install

clean:
	cd $(SOURCEDIR) && make distclean

romfs:
	for f in $(PROGS); do \
		$(ROMFSINST) $(TMPFS_DIR)/sbin/$$f /bin ; \
	done
#	$(ROMFSINST) $(TMPFS_DIR)/sbin/blkid /bin/blkid2

