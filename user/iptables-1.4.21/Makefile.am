# -*- Makefile -*-

ACLOCAL_AMFLAGS  = -I m4
AUTOMAKE_OPTIONS = foreign subdir-objects

SUBDIRS          = libiptc libxtables
if ENABLE_DEVEL
SUBDIRS         += include
endif
if ENABLE_LIBIPQ
SUBDIRS         += libipq
endif
SUBDIRS         += utils
# Depends on libxtables:
SUBDIRS         += extensions
# Depends on extensions/libext.a:
SUBDIRS         += iptables

.PHONY: tarball
tarball:
	rm -Rf /tmp/${PACKAGE_TARNAME}-${PACKAGE_VERSION};
	pushd ${top_srcdir} && git archive --prefix=${PACKAGE_TARNAME}-${PACKAGE_VERSION}/ HEAD | tar -C /tmp -x && popd;
	pushd /tmp/${PACKAGE_TARNAME}-${PACKAGE_VERSION} && ./autogen.sh && popd;
	tar -C /tmp -cjf ${PACKAGE_TARNAME}-${PACKAGE_VERSION}.tar.bz2 --owner=root --group=root ${PACKAGE_TARNAME}-${PACKAGE_VERSION}/;
	rm -Rf /tmp/${PACKAGE_TARNAME}-${PACKAGE_VERSION};

config.status: extensions/GNUmakefile.in \
	include/xtables-version.h.in include/iptables/internal.h.in

romfs:
	$(ROMFSINST) iptables/.libs/xtables-multi /bin/xtables-multi
	$(ROMFSINST) libiptc/.libs/libip4tc.so.0 /lib/libip4tc.so.0
	$(ROMFSINST) libiptc/.libs/libip6tc.so.0 /lib/libip6tc.so.0
	$(ROMFSINST) libxtables/.libs/libxtables.so.10 /lib/libxtables.so.10
	-cd $(ROMFSDIR)/bin && \
	ln -fs xtables-multi iptables && \
	ln -fs xtables-multi ip6tables && \
	cd -