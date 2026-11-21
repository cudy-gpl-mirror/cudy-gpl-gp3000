THISDIR := $(shell pwd)
SOURCEDIR := $(THISDIR)/dosfstools-4.1
BUILDDIR := $(THISDIR)/build

CROSS_TARGET ?= mips-linux
CROSS_COMPILE ?= $(CROSS_TARGET)-

all:
	[ -f timestamp ] || touch timestamp
	touch -rtimestamp $(SOURCEDIR)/config* $(SOURCEDIR)/aclocal* $(SOURCEDIR)/Makefile*
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR) && \
	$(SOURCEDIR)/configure \
		--host=$(CROSS_TARGET) \
		--target=$(CROSS_TARGET) \
		--prefix=$(ROMFSDIR); \
	$(MAKE)
	
romfs:
	$(ROMFSINST) $(BUILDDIR)/src/fsck.fat /bin/dosfsck	
	
clean:
	rm -rf $(BUILDDIR)
#	make -C $(SOURCEDIR) clean