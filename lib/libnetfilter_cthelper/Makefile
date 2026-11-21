
BUILD := libnetfilter_cthelper-1.0.0
LIB_NAME := libnetfilter_cthelper

all:
	cd $(BUILD); \
	[ -f Makefile ] || \
	autoreconf -f && ./configure \
		--host=$(CROSS_HOST) --target=$(CROSS_TARGET) --prefix=$(TMPFSDIR); \
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
