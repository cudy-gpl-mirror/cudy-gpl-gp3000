
ifeq ($(CONFIG_LIB_LIBNL_3_5),y)
BUILD := libnl-3.5
else
BUILD := libnl-3.2.25
endif

LIB_NAME := libnl
# TMPFSDIR := $(shell pwd)/build

all:
	cd $(BUILD);\
	[ -f Makefile ] || \
	autoreconf -vf && \
	./configure \
		--disable-static --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --prefix=$(TMPFSDIR) || exit 1; \
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
