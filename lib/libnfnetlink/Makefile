BUILD := libnfnetlink-1.0.1
LIB_NAME := libnfnetlink

all:
	cd $(BUILD); \
	[ -f Makefile ] || \
	aclocal && autoreconf -f && ./configure \
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
	$(MAKE) -C $(BUILD) clean


.PHONY: all clean romfs
