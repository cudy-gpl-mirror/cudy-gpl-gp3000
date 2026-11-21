XUPNPD_DIR = xupnpd

.PHONY: all xupnpd clean romfs

all: xupnpd

xupnpd:
	make -C $(XUPNPD_DIR) all

clean:
	make -C $(XUPNPD_DIR) clean

romfs:
	make -C $(XUPNPD_DIR) romfs

