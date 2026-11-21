
BUILD := pixman-0.38.4

CFLAGS  += -I$(ROOTDIR)/lib/libpng12/tmpfs/include
LDFLAGS += -L$(ROOTDIR)/lib/libpng12/tmpfs/lib
$(info $(CFLAGS))
PATH := $(ROOTDIR)/tools/usr/bin:$(PATH)
all: pkg_conf
	cd $(BUILD) && $(MAKE) all install

pkg_conf:
	if [ ! -f $(BUILD)/Makefile ]; then \
		cd $(BUILD) && ./configure --host=$(CROSS_TARGET) --prefix=$(TMPFSDIR); \
	fi


romfs:
	for x in $(shell cd $(TMPFSDIR)/lib; ls libpixman*.so*); do \
		if [ -L $(TMPFSDIR)/lib/$$x ]; then \
			dest=`readlink $(TMPFSDIR)/lib/$$x`; \
			$(ROMFSINST) -s $$dest /lib/$$x; \
		else \
			$(ROMFSINST) $(TMPFSDIR)/lib/$$x /lib/$$x; \
		fi; \
	done

clean:
	cd $(BUILD) && $(MAKE) distclean
