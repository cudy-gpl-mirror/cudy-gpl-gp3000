PWDDIR = $(shell pwd)

ifneq ($(CONFIG_USER_WPAS_VERSION),)
HOSTAPD = wpa_supplicant-$(CONFIG_USER_WPAS_VERSION)/wpa_supplicant
else
HOSTAPD = wpa_supplicant-2.9/wpa_supplicant
endif
FILES = files
PROG = wpa_supplicant
PROG_CLI = wpa_cli

LIBNLDIR = $(TMPFSDIR)
export PKG_CONFIG_PATH = $(LIBNLDIR)/lib/pkgconfig

CFLAGS += -Wno-error=implicit-function-declaration
CFLAGS += -I$(ROOTDIR)/lib/libssl/include

ifeq ("$(CROSS_HOST)", "")
CROSS_HOST = $(ARCH)-linux
CROSS_TARGET = $(ARCH)-linux
endif

.PHONY: all wpa_supplicant

all: wpa_supplicant

wpa_supplicant:
	@echo -n "Building $(HOSTAPD) ......"
	if [ ! -f $(HOSTAPD)/.config ]; then \
		cp $(FILES)/defconfig $(HOSTAPD)/.config; \
		echo "CFLAGS += -I$(LIBNLDIR)/include/libnl3" >> $(HOSTAPD)/.config; \
		echo "LIBS += -L$(LIBNLDIR)/lib" >> $(HOSTAPD)/.config; \
	fi
	$(MAKE) -C $(HOSTAPD)

romfs:
	@echo "Install $(HOSTAPD) ......."
	$(ROMFSINST) $(HOSTAPD)/$(PROG) /bin/$(PROG)
	$(ROMFSINST) $(HOSTAPD)/$(PROG_CLI) /bin/$(PROG_CLI)
	@cp -a $(FILES)/conf/. $(ROMFSDIR)/etc/conf
	#$(ROMFSINST) $(FILES)/wpa_supplicant_wps.sh /etc/wpa_supplicant_wps.sh
	for script in $(shell cd $(FILES); ls *.sh) ; do \
		$(ROMFSINST) $(FILES)/$$script /etc/$$script; \
	done

rcX:

clean:
	$(MAKE) -C $(HOSTAPD) clean

distclean:
	$(MAKE) -C $(HOSTAPD) clean
	rm $(HOSTAPD)/.config

