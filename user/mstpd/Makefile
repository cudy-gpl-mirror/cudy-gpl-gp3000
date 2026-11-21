
DIR_SOURCE = mstpd-0.1.0

all: mstpd

mstpd:
	cd $(DIR_SOURCE) && ./autogen.sh && ./configure --host=mips-linux --target=mips-linux --disable-zlib
	$(MAKE) -C $(DIR_SOURCE)

distclean:
	$(MAKE) -C $(DIR_SOURCE) distclean

clean:
	$(MAKE) -C $(DIR_SOURCE) clean

romfs:
	$(ROMFSINST) $(DIR_SOURCE)/mstpd /bin/mstpd
	$(ROMFSINST) $(DIR_SOURCE)/mstpctl /bin/mstpctl
	$(ROMFSINST) $(DIR_SOURCE)/bridge-stp /bin/bridge-stp

rcX:

.PHONY: all clean distclean romfs rcX