DESTDIR     = $(shell pwd)/../preinstall
PREFIX      = $(DESTDIR)/usr
ETCDIR      = $(DESTDIR)/etc
SBINDIR     = $(PREFIX)/sbin

CFLAGS += -Os -Wall
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += -s
CFLAGS += -include $(ROOTDIR)/config/autoconf.h

EXEC = mnet 
OBJS = mnet.o
all: BUILT_TIME $(EXEC)

$(EXEC): $(OBJS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)
	$(ROMFSINST) /etc/run.sh

.PHONY: rcX
rcX:
	$(ROMFSINST) -a "mount -t usbfs none /proc/bus/usb" /etc/init.d/rc19
	$(ROMFSINST) -a "mnet -d &" /etc/init.d/rc19

	
clean:
	-rm -f $(EXEC) *.elf *.o built_time

BUILT_TIME:
	@echo  \#define BUILT_TIME \"`TZ=UTC date -u "+%Y.%m.%d-%H:%M%z" `\" > ./built_time
