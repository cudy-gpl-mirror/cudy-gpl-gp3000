
DIR_SOURCE = conntrack-tools-1.4.6

all: conntrack-tools

ifeq ("$(CROSS_HOST)", "")
CROSS_HOST = $(ARCH)-linux
CROSS_TARGET = $(ARCH)-linux
endif

conntrack-tools:
	if [ ! -f $(DIR_SOURCE)/Makefile ]; then \
		cd $(DIR_SOURCE); \
		autoreconf -ivf && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --prefix=$(TMPFSDIR) --disable-silent-rules; \
		sed -i 's/hardcode_libdir_flag_spec="\\$$wl-rpath \\$$wl\\$$libdir"/hardcode_libdir_flag_spec="\\$$wl-rpath-link=\\$$libdir"/g' libtool; \
		cd -; \
	fi
	$(MAKE) -C $(DIR_SOURCE)

distclean:
	$(MAKE) -C $(DIR_SOURCE) distclean

clean:
	$(MAKE) -C $(DIR_SOURCE) clean

romfs:
	$(ROMFSINST) $(DIR_SOURCE)/src/conntrack /bin/conntrack
	$(ROMFSINST) $(DIR_SOURCE)/src/conntrackd /bin/conntrackd
	$(ROMFSINST) $(DIR_SOURCE)/src/nfct /bin/nfct

rcX:

.PHONY: all clean distclean romfs rcX

