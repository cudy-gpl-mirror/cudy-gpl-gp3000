.EXPORT_ALL_VARIABLES:
QUAGGA = quagga-1.0.20160315

ifeq ($(CONFIG_CMCC), y)
CONF_OPTS += --enable-user=CMCCAdmin
else
CONF_OPTS += --enable-user=admin
endif

CONF_OPTS += -host=$(CROSS_HOST) \
	--target=$(CROSS_TARGET) \
	--disable-shared \
	--enable-group=root \
	--disable-vtysh \
	--disable-ospf6d \
	--disable-pimd \
	--disable-babeld \
	--disable-watchquagga \
	--disable-solaris \
	--disable-doc \
	--enable-zebra \
	--disable-opaque-lsa

ifeq ($(CONFIG_USER_QUAGGA_RIPNGD),y)
CONF_OPTS += --enable-ripngd
endif
ifeq ($(CONFIG_USER_QUAGGA_RIP),y)
CONF_OPTS += --enable-ripd
CONF_OPTS += --enable-linux
endif

all: quagga

quagga:
	cd $(QUAGGA)  && ./configure $(CONF_OPTS)
	$(MAKE) -C $(QUAGGA)

distclean:
	$(MAKE) -C $(QUAGGA) distclean

clean:
	$(MAKE) -C $(QUAGGA) distclean

romfs:
	$(ROMFSINST) $(QUAGGA)/zebra/zebra /bin/zebra
	$(ROMFSINST) $(QUAGGA)/ripngd/ripngd /bin/ripngd
	$(ROMFSINST) $(QUAGGA)/ripngd/libripng.a /lib/libripng.a
	$(ROMFSINST) $(QUAGGA)/ripd/librip.a /lib/librip.a
	$(ROMFSINST) $(QUAGGA)/ripd/ripd /bin/ripd
	
.PHONY: all clean distclean romfs rcX

