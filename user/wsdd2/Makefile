.EXPORT_ALL_VARIABLES:
LIB_SOURCE=wsdd2-1.8

all: eudev

eudev:
	$(MAKE) -C $(LIB_SOURCE)
	$(MAKE) -C $(LIB_SOURCE) install DESTDIR=tmp

clean:
	$(MAKE) -C $(LIB_SOURCE) distclean
	
distclean:
	$(MAKE) -C $(LIB_SOURCE) distclean

romfs:
	$(ROMFSINST) $(LIB_SOURCE)/tmp/usr/sbin/wsdd2 /bin/wsdd2

.PHONY: all clean distclean romfs
