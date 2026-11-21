
DIR_SOURCE = openssh-9.8p1

all: openssh

openssh:
	cd $(DIR_SOURCE) && ./configure --host=mips-linux --target=mips-linux
	rm -f $(DIR_SOURCE)/openbsd-compat/log.h
	cd $(DIR_SOURCE)/openbsd-compat && ln -s ../log.h log.h
	$(MAKE) -C $(DIR_SOURCE)

distclean:
	$(MAKE) -C $(DIR_SOURCE) distclean

clean:
	$(MAKE) -C $(DIR_SOURCE) clean

romfs:	
ifeq ($(CONFIG_USER_OPENSSH_SFTP),y)
	[ -d $(ROMFSDIR)/usr/libexec ] || mkdir -p $(ROMFSDIR)/usr/libexec
	$(ROMFSINST) $(DIR_SOURCE)/sftp-server /usr/libexec/sftp-server
endif

rcX:

.PHONY: all clean distclean romfs rcX