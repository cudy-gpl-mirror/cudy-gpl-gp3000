###############################################################################################################
# You will encouter the "Invalid configuration `aarch64-linux': machine `aarch64' not recognize" compile error
# Please download latest config.guess and config.sub, then rm build-aux/config.guess build-aux/config.sub
# to update two file
###############################################################################################################

BUILD := libnetfilter_log-1.0.1
LIB_NAME := libnetfilter_log
PKG_CONFIG_PATH := $(TMPFSDIR)/lib/pkgconfig

all:
	cd $(BUILD); \
	[ -f Makefile ] || \
	aclocal && autoreconf -f && ./configure \
		--host=$(CROSS_HOST) --target=$(CROSS_TARGET) --prefix=$(TMPFSDIR) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH); \
	$(MAKE) && \
	$(MAKE) install


romfs:
	for x in $(shell cd $(TMPFSDIR)/lib; ls $(LIB_NAME)*.so*); do \
		if [ -L $(TMPFSDIR)/lib/$$x ]; then \
			dest=`readlink $(TMPFSDIR)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(TMPFSDIR)/lib/$$x /lib/$$x; \
		fi; \
	done

clean:
	$(MAKE) -C $(BUILD) distclean


.PHONY: all clean romfs
