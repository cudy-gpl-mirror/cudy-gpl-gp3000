ifdef CONFIG_LIB_LIBFFI_3_2_1
BUILD := libffi-3.2.1
endif
ifdef CONFIG_LIB_LIBFFI_3_3
BUILD := libffi-3.3
endif

LIB_NAME := libffi

all:
	cd $(BUILD);\
	[ -f Makefile ] || \
	sed -e '/^includesdir/ s/$$(libdir).*$$/$$(includedir)/' -i include/Makefile.in && \
	sed -e '/^includedir/ s/=.*$$/=@includedir@/' -e 's/^Cflags: -I$${includedir}/Cflags:/' -i libffi.pc.in && \
	./configure \
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
