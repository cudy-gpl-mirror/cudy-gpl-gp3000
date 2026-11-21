ifeq ($(CONFIG_LIB_LIBSSL_3_0_14),y)
OPENSSL = openssl
OPENSSL_VERSION = 3.0.14
OPENSSL_NAME = $(OPENSSL)-$(OPENSSL_VERSION)
OPENSSL_TARBALL = $(OPENSSL_NAME).tar.gz

OPENSSL_TARBALL_DIR = $(ROOTDIR)/lib/libssl/tarball
OPENSSL_BUILD_DIR = $(ROOTDIR)/lib/libssl/build/$(OPENSSL_NAME)
BUILD_DIR ?= $(OPENSSL_BUILD_DIR)
else
BUILD_DIR ?= openssl-1.1.1t
endif

ifeq "$(ARCH)" "arm64"
	OS = linux-aarch64
endif

ifeq "$(ARCH)" "mips"
	OS = linux-mips32
endif

OS ?= linux-generic32
CC := gcc
CXX := g++
AR := ar
RANLIB := ranlib

# cwmp-tr069 build openssl only if we don't enable libssl.
# because cwmp-tr069 always need openssl header file.
# so, we need reset compile flags for checksec
include $(CHECKSEC_LIB_CONFIG)

all: pre
	cd $(BUILD_DIR); \
	if [ ! -f Makefile ]; then \
		./Configure $(OS) shared --prefix="$(TMPFSDIR)"; \
	fi; \
	make && make install

pre:
ifeq ($(CONFIG_LIB_LIBSSL_1_1_1T),y)
	@echo "Using pre-extracted OpenSSL 1.1.1t directory"
else
	@echo "Prepare $(OPENSSL)"
	if [ ! -d $(OPENSSL_BUILD_DIR) ]; then \
		if [ -f $(OPENSSL_TARBALL_DIR)/$(OPENSSL_TARBALL) ]; then \
			echo "Untar $(OPENSSL_TARBALL)"; \
			mkdir -p $(OPENSSL_BUILD_DIR); \
			tar xvf $(OPENSSL_TARBALL_DIR)/$(OPENSSL_TARBALL) --strip-components 1 -C $(OPENSSL_BUILD_DIR); \
		else \
			echo "Can NOT prepare $(OPENSSL)"; \
			exit 1; \
		fi; \
	fi
endif

romfs:
	for x in $(shell cd $(TMPFSDIR)/lib; ls libssl*.so* libssl*.so* libcrypto*.so* libcrypto*.so*); do \
		if [ -L $(TMPFSDIR)/lib/$$x ]; then \
			dest=`readlink $(TMPFSDIR)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(TMPFSDIR)/lib/$$x /lib/$$x; \
		fi; \
	done

clean:
	@if [ -e ./$(BUILD_DIR)/Makefile ]; then \
		make -C $(BUILD_DIR) clean; \
	fi

distclean:
	@if [ -e ./$(BUILD_DIR)/Makefile ]; then \
		make -C $(BUILD_DIR) distclean; \
	fi
