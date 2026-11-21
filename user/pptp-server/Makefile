
PPTPS_ROOT_DIR=$(shell pwd)
ifeq ($(CONFIG_USER_PPTP_SERVER_V1_4_0), y)
PPTPS_VER:=pptpd-1.4.0
else 
PPTPS_VER:=pptpd-1.4.0
endif
PPTPS_CFLAGS=--host=mips-linux --target=mips-linux --sbindir=/bin --bindir=/bin


all: preconfig pptpserver

preconfig:
	if [ ! -f $(PPTPS_ROOT_DIR)/$(PPTPS_VER)/Makefile ];then \
		cd $(PPTPS_ROOT_DIR)/$(PPTPS_VER); \
		./configure $(PPTPS_CFLAGS); \
		cd -; \
	fi;
	
pptpserver:
	$(MAKE) -C $(PPTPS_ROOT_DIR)/$(PPTPS_VER)

distclean:
	$(MAKE) -C $(PPTPS_ROOT_DIR)/$(PPTPS_VER) distclean

clean:
	$(MAKE) -C $(PPTPS_ROOT_DIR)/$(PPTPS_VER) clean

romfs:
	$(ROMFSINST) $(PPTPS_ROOT_DIR)/$(PPTPS_VER)/pptpd /bin/pptpd
	$(ROMFSINST) $(PPTPS_ROOT_DIR)/$(PPTPS_VER)/pptpctrl /bin/pptpctrl

rcX:

.PHONY: all clean distclean romfs rcX