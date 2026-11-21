PWD = $(shell pwd)

GPTFDISK_VERSION = 1.0.8

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

EXTRA_CXXFLAGS += -I$(INCLUDE_INSTALL_DIR)

GPTFDISK = gptfdisk-$(GPTFDISK_VERSION)
GPTFDISK_TARBALL = $(GPTFDISK).tar.gz
GPTFDISK_WEBURL = https://www.rodsbooks.com/gdisk

PROGS:=sgdisk

.PHONY: prepare_src do_patch $(GPTFDISK)
all: prepare_src do_patch $(GPTFDISK)

prepare_src:
	@echo "Prepare $(PKG_TARBALL_DIR) dir."
	if [ ! -d $(PKG_TARBALL_DIR) ]; then \
		mkdir -p $(PKG_TARBALL_DIR); \
	fi
	@echo "Prepare $(GPTFDISK)"
	if [ ! -d $(GPTFDISK) ]; then \
		if [ ! -f $(PKG_TARBALL_DIR)/$(GPTFDISK_TARBALL) ]; then \
			echo "Get $(GPTFDISK_TARBALL)"; \
			wget -O $(PKG_TARBALL_DIR)/$(GPTFDISK_TARBALL) $(GPTFDISK_WEBURL)/$(GPTFDISK_TARBALL); \
		fi; \
		if [ -f $(PKG_TARBALL_DIR)/$(GPTFDISK_TARBALL) ]; then \
			echo "Untar $(GPTFDISK_TARBALL)"; \
			tar -zxvf $(PKG_TARBALL_DIR)/$(GPTFDISK_TARBALL) -C $(PWD); \
		else \
			echo "Can NOT prepare $(GPTFDISK)"; \
			exit 1; \
		fi; \
	fi

do_patch: prepare_src
	@echo "No patch for $(GPTFDISK)"

$(GPTFDISK): do_patch
	for f in $(PROGS); do \
		$(MAKE) CXXFLAGS+=$(EXTRA_CXXFLAGS) -C $@ $$f ; \
	done

clean:
	cd $(SOURCEDIR) && make distclean

romfs:
	for f in $(PROGS); do \
		$(ROMFSINST) $(GPTFDISK)/$$f /bin ; \
	done

