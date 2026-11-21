all: tools/cert_create  tools/fiptool  tools/encrypt_fw

tools/cert_create:
	make PLAT=venus -C $@

tools/fiptool:
	make CPPFLAGS="-DVERSION='\"v1.5(release)\"'" -C $@

tools/encrypt_fw:
	make HOSTCC=gcc -C $@

clean:
	$(MAKE) PLAT=venus -C tools/cert_create clean
	$(MAKE) CPPFLAGS="-DVERSION='\"v1.5(release)\"'" -C tools/fiptool clean
	$(MAKE) -C tools/encrypt_fw clean

install: | ${DEST}
	install -m 0755 tools/cert_create/cert_create ${DEST}
	install -m 0755 tools/fiptool/fiptool ${DEST}
#	install -m 0755 tools/fip_create/fip_create ${DEST}
	install -m 0755 tools/encrypt_fw/encrypt_fw ${DEST}

${DEST}:
	-mkdir -p $@

.PHONY: tools/cert_create tools/fiptool tools/encrypt_fw

