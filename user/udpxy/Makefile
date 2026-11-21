UDPXY_DIR = udpxy-1.0.23-12

.PHONY: all udpxy clean romfs

all: udpxy

udpxy:
	make -C $(UDPXY_DIR) release

clean:
	make -C $(UDPXY_DIR) clean

romfs:
	$(ROMFSINST) $(UDPXY_DIR)/udpxy /bin/udpxy

