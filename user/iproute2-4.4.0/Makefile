SOURCEDIR=src
DIRS=lib ip tc bridge
#misc netem genl
THISDIR=$(shell pwd)/$(SOURCEDIR)
CFLAGS += -DRESOLVE_HOSTNAMES -D_GNU_SOURCE -I$(THISDIR)/include
LIBNETLINK=../lib/libnetlink.a ../lib/libutil.a

ADDLIB=
#options for decnet
ADDLIB+=dnet_ntop.o dnet_pton.o
#options for ipx
ADDLIB+=ipx_ntop.o ipx_pton.o
#options for ip
ADDLIB+=mpls_pton.o mpls_ntop.o
YACCFLAGS=-d -t -v
LDLIBS+= $(LIBNETLINK)
export LIBNETLINK ADDLIB YACCFLAGS LDLIBS

all:
	cd $(SOURCEDIR) && ./configure && for i in $(DIRS); do make -C $$i $@ || exit $?; done

clean:
	cd $(SOURCEDIR) && for i in $(DIRS); do make -C $$i clean; done

romfs:
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_IP_IP  $(SOURCEDIR)/ip/ip /bin/
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_TC_TC $(SOURCEDIR)/tc/tc /bin/
	$(ROMFSINST) -e CONFIG_USER_IPROUTE2_IP_BRIDGE $(SOURCEDIR)/bridge/bridge /bin/
