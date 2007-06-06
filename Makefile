BINARY=libcowdancer.so cow-shell cowbuilder qemubuilder
INSTALL_DIR=install -d -o root -g root -m 755
INSTALL_FILE=install -o root -g root -m 644
INSTALL_PROGRAM=install -o root -g root -m 755
DESTDIR=
PREFIX=/usr
LIBDIR=$(PREFIX)/lib
export VERSION=$(shell sed -n '1s/.*(\(.*\)).*$$/\1/p' < debian/changelog )

all: $(BINARY)

install: $(BINARY)
	$(INSTALL_DIR) $(DESTDIR)${PREFIX}/bin
	$(INSTALL_DIR) $(DESTDIR)${LIBDIR}/cowdancer
	$(INSTALL_DIR) $(DESTDIR)${PREFIX}/share/man/man1
	$(INSTALL_DIR) $(DESTDIR)${PREFIX}/share/man/man8
	$(INSTALL_FILE)  cow-shell.1 $(DESTDIR)/usr/share/man/man1/cow-shell.1
	$(INSTALL_FILE)  cowbuilder.8 $(DESTDIR)/usr/share/man/man8/cowbuilder.8
	$(INSTALL_FILE)  libcowdancer.so $(DESTDIR)${LIBDIR}/cowdancer/libcowdancer.so
	$(INSTALL_PROGRAM) cow-shell $(DESTDIR)/usr/bin/cow-shell
	$(INSTALL_PROGRAM) cowbuilder $(DESTDIR)/usr/sbin/cowbuilder
	$(INSTALL_PROGRAM) qemubuilder $(DESTDIR)/usr/sbin/qemubuilder
	$(INSTALL_FILE)  qemubuilder.8 $(DESTDIR)/usr/share/man/man8/qemubuilder.8

libcowdancer.so: cowdancer.lo
	gcc -ldl -shared -o $@ $<

cow-shell: cow-shell.o

cowbuilder: cowbuilder.o parameter.o

qemubuilder: qemubuilder.o parameter.o

%.lo: %.c 
	gcc -D_REENTRANT -fPIC $< -o $@ -c -Wall -O2 -g

%.o: %.c parameter.h
	gcc $< -o $@ -c -Wall -O2 -g -fno-strict-aliasing -D LIBDIR="\"${LIBDIR}\""

clean: 
	-rm -f *~ *.o *.lo $(BINARY)

upload-dist-all:
	scp ../cowdancer_$(VERSION).tar.gz aegis.netfort.gr.jp:public_html/software/downloads

check:
	set -e; set -o pipefail; for A in tests/???_*.sh; do echo $$A; bash $$A  2>&1 | \
	sed -e's,/tmp/[^/]*,/tmp/XXXX,g' \
	    -e "s,^Current time:.*,Current time: TIME," \
	    -e "s,^pbuilder-time-stamp: .*,pbuilder-time-stamp: XXXX," \
	    -e "s,^Fetched .*B in .*s (.*B/s),Fetched XXXB in Xs (XXXXXB/s)," \
	| tee tests/log/$${A/*\//}.log; done

.PHONY: clean check upload-dist-all
