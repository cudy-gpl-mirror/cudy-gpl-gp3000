all: tools/cert_create  tools/fiptool

tools/cert_create:
	make PLAT=saturn_fpga -C $@

tools/fiptool:
	make CPPFLAGS="-DVERSION='\"v1.3(release):75a83df\"'" -C $@

clean:
	$(MAKE) PLAT=saturn_fpga -C tools/cert_create clean
	$(MAKE) CPPFLAGS="-DVERSION='\"v1.3(release):75a83df\"'" -C tools/fiptool clean

install: | ${DEST}
	install -m 0755 tools/fip_create/fip_create ${DEST}
	install -m 0755 tools/cert_create/cert_create ${DEST}
	install -m 0755 tools/fiptool/fiptool ${DEST}

${DEST}:
	-mkdir -p $@

.PHONY: tools/cert_create tools/fiptool

