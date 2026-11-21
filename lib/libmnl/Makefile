BUILD := libmnl-1.0.4
LIB_NAME := libmnl

all:
	cd $(BUILD); \
	[ -f Makefile ] || \
	autoreconf -f && ./configure \
		--disable-static --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --prefix=$(TMPFSDIR); \
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

