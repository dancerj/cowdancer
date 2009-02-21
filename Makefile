BINARY=libcowdancer.so cow-shell cowbuilder qemubuilder cowdancer-ilistcreate \
	cowdancer-ilistdump
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
	$(INSTALL_FILE)  cowdancer-ilistcreate.1 $(DESTDIR)/usr/share/man/man1/cowdancer-ilistcreate.1
	$(INSTALL_FILE)  cowdancer-ilistdump.1 $(DESTDIR)/usr/share/man/man1/cowdancer-ilistdump.1
	$(INSTALL_FILE)  cowbuilder.8 $(DESTDIR)/usr/share/man/man8/cowbuilder.8
	$(INSTALL_FILE)  qemubuilder.8 $(DESTDIR)/usr/share/man/man8/qemubuilder.8
	$(INSTALL_FILE)  libcowdancer.so $(DESTDIR)${LIBDIR}/cowdancer/libcowdancer.so
	$(INSTALL_PROGRAM) cow-shell $(DESTDIR)/usr/bin/cow-shell
	$(INSTALL_PROGRAM) cowbuilder $(DESTDIR)/usr/sbin/cowbuilder
	$(INSTALL_PROGRAM) qemubuilder $(DESTDIR)/usr/sbin/qemubuilder
	$(INSTALL_PROGRAM) cowdancer-ilistcreate $(DESTDIR)/usr/bin/cowdancer-ilistcreate
	$(INSTALL_PROGRAM) cowdancer-ilistdump $(DESTDIR)/usr/bin/cowdancer-ilistdump

	$(INSTALL_DIR) $(DESTDIR)/etc/bash_completion.d
	$(INSTALL_FILE) bash_completion.qemubuilder $(DESTDIR)/etc/bash_completion.d/qemubuilder
	$(INSTALL_FILE) bash_completion.cowbuilder $(DESTDIR)/etc/bash_completion.d/cowbuilder

libcowdancer.so: cowdancer.lo ilistcreate.lo
	$(CC) -O2 -Wall -ldl -shared -o $@ $^

cow-shell: cow-shell.o ilistcreate.o
	$(CC) -O2 -Wall -o $@ $^

cowdancer-ilistcreate: cowdancer-ilistcreate.o ilistcreate.o
	$(CC) -O2 -Wall -o $@ $^

cowbuilder: cowbuilder.o parameter.o forkexec.o ilistcreate.o
	$(CC) -O2 -Wall -o $@ $^

qemubuilder: qemubuilder.o parameter.o forkexec.o qemuipsanitize.o qemuarch.o file.o
	$(CC) -O2 -Wall -o $@ $^

%.lo: %.c 
	$(CC) -D_REENTRANT -fPIC $< -o $@ -c -Wall -O2 -g

%.o: %.c parameter.h
	$(CC) $< -o $@ -c -Wall -O2 -g -fno-strict-aliasing -D LIBDIR="\"${LIBDIR}\""

clean: 
	-rm -f *~ *.o *.lo $(BINARY)
	-make -C initrd clean

upload-dist-all:
	scp ../cowdancer_$(VERSION).tar.gz aegis.netfort.gr.jp:public_html/software/downloads

fastcheck:
	set -e; set -o pipefail; for A in ./test_*.c; do echo $$A; $$A 2>&1 | \
		tee tests/log/$${A/*\//}.log; done

slowcheck:
	set -e; set -o pipefail; for A in tests/???_*.sh; do echo $$A; bash $$A  2>&1 | \
	sed -e's,/tmp/[^/]*,/tmp/XXXX,g' \
	    -e "s,^Current time:.*,Current time: TIME," \
	    -e "s,^pbuilder-time-stamp: .*,pbuilder-time-stamp: XXXX," \
	    -e "s,^Fetched .*B in .*s (.*B/s),Fetched XXXB in Xs (XXXXXB/s)," \
	| tee tests/log/$${A/*\//}.log; done

check: fastcheck slowcheck

check-syntax:
	$(CC) -c -O2 -Wall $(CHK_SOURCES)  -o/dev/null -D LIBDIR="\"${LIBDIR}\""

.PHONY: clean check upload-dist-all check-syntax fastcheck slowcheck
