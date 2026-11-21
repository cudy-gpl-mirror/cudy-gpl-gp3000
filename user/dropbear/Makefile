
DIR_SOURCE = dropbear-2019.78

all: dropbear

dropbear:
	cd $(DIR_SOURCE) && ./configure --host=mips-linux --target=mips-linux --disable-zlib
	$(MAKE) -C $(DIR_SOURCE)

distclean:
	$(MAKE) -C $(DIR_SOURCE) distclean

clean:
	$(MAKE) -C $(DIR_SOURCE) clean

romfs:	
	[ -d $(ROMFSDIR)/etc/dropbear ] || mkdir -p $(ROMFSDIR)/etc/dropbear
	$(ROMFSINST) $(DIR_SOURCE)/dropbear_rsa_host_key /etc/dropbear/dropbear_rsa_host_key
	$(ROMFSINST) $(DIR_SOURCE)/dropbear /bin/dropbear

rcX:

.PHONY: all clean distclean romfs rcX