.EXPORT_ALL_VARIABLES:

-include $(CONFIG_CONFIG)
-include $(CHECKSEC_LIB_CONFIG)

CUPS_ROOT=$(shell pwd)
ifeq ($(CONFIG_USER_CUPS_1_4_6),y)
CUPS_DIR=cups-1.4.6
CUPS_CONFIG_OPTION=--disable-dbus --disable-gssapi
endif

#CC += -Os

all: startcupsd
	echo "CC=" $(CC)
	echo "CFLAGS=" $(CFLAGS) -DDEBUG
	echo "LDFLAGS=" $(LDFLAGS)
	@if [ -d $(CUPS_DIR) ]; then \
		$(MAKE) $(CUPS_DIR)/config.h; \
		$(MAKE) -C $(CUPS_DIR); \
	fi	

$(CUPS_DIR)/config.h: $(CUPS_DIR)/configure 
	cd $(CUPS_DIR); \
	./configure --host=mips-linux $(CUPS_CONFIG_OPTION) CXXFLAGS="-I$(TMPFSDIR)/include" DSOFLAGS="-L$(TMPFSDIR)/lib $(LIB_SECLDFLAGS)" LIBS="-lz";

romfs:
	if [ ! -d $(ROMFSDIR)/etc/cups ]; then \
		mkdir $(ROMFSDIR)/etc/cups ; \
		mkdir $(ROMFSDIR)/etc/cups/conf ; \
	fi
	make -C $(CUPS_DIR) romfs
	$(ROMFSINST) startcupsd /bin/startcupsd

clean:
	make -C $(CUPS_DIR) clean
	make -C $(CUPS_DIR) distclean
	rm -f startcupsd

startcupsd: LDFLAGS += $(LIBMIB) $(LIBVOIP) $(SECLDFLAGS)

RC_LEVEL=/etc/init.d/rc32
.PHONY: rcX
rcX:
	$(ROMFSINST) -a "mkdir -p /var/cups/" $(RC_LEVEL)
	$(ROMFSINST) -a "mkdir -p /var/cups/conf/" $(RC_LEVEL)
	$(ROMFSINST) -a "mkdir -p /var/cups/requests/" $(RC_LEVEL)
	$(ROMFSINST) -a "mkdir -p /var/cups/filter/" $(RC_LEVEL)
	$(ROMFSINST) -a "mkdir -p /var/cups/jobs/" $(RC_LEVEL)
	$(ROMFSINST) -a "cp /etc/cups/conf/* /var/cups/conf/" $(RC_LEVEL)
