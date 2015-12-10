SHELL=/bin/bash
BINARY=libcowdancer.so cow-shell cowbuilder qemubuilder cowdancer-ilistcreate \
	cowdancer-ilistdump
INSTALL_DIR=install -d -o root -g root -m 755
INSTALL_FILE=install -o root -g root -m 644
INSTALL_PROGRAM=install -o root -g root -m 755
DESTDIR=
PREFIX=/usr
LIBDIR=$(PREFIX)/lib
CFLAGS := $(CFLAGS) -fno-strict-aliasing
CFLAGS_LFS=$(CFLAGS) $(shell getconf LFS_CFLAGS)
PWD=$(shell pwd)

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

	$(INSTALL_DIR) $(DESTDIR)/usr/share/bash-completion/completions
	$(INSTALL_FILE) bash_completion.qemubuilder $(DESTDIR)/usr/share/bash-completion/completions/qemubuilder
	$(INSTALL_FILE) bash_completion.cowbuilder $(DESTDIR)/usr/share/bash-completion/completions/cowbuilder

libcowdancer.so: cowdancer.lo ilistcreate.lo
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ $^ -ldl

cow-shell: cow-shell.o ilistcreate.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

cowdancer-ilistcreate: cowdancer-ilistcreate.o ilistcreate.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

cowbuilder: cowbuilder.o parameter.o forkexec.o ilistcreate.o main.o cowbuilder_util.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

qemubuilder: qemubuilder.lfso parameter.lfso forkexec.lfso qemuipsanitize.lfso qemuarch.lfso file.lfso main.lfso
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.lo: %.c
	$(CC) $(CFLAGS) -fPIC $< -o $@ -c

%.lfso: %.c
	$(CC) $(CFLAGS_LFS) $< -o $@ -c

%.o: %.c parameter.h
	$(CC) $(CFLAGS) $< -o $@ -c -D LIBDIR="\"${LIBDIR}\""

clean: 
	-rm -f *~ *.o *.lo *.lfso $(BINARY)
	-make -C initrd clean

upload-dist-all:
	scp ../cowdancer_$(VERSION).tar.gz aegis.netfort.gr.jp:public_html/software/downloads

fastcheck:
	set -e; set -o pipefail; for A in ./test_*.c; do echo $$A; \
		./tests/run_c.sh $$A 2>&1 | \
		tee tests/log/$${A/*\//}.log; done

slowcheck: cowdancer-ilistdump qemubuilder cow-shell
	# FIXME: The tests are running installed cowdancer, not the just-built
	set -e; set -o pipefail; for A in tests/[0-9][0-9][0-9]_*.sh; \
	do echo $$A; \
	PATH="$(PWD):$(PWD)/tests:/usr/bin/:/bin" \
	COWDANCER_SO=$(PWD)/libcowdancer.so \
	bash $$A  2>&1 | \
	sed -e's,/tmp/[^/]*,/tmp/XXXX,g' \
	    -e "s,^Current time:.*,Current time: TIME," \
	    -e "s,^pbuilder-time-stamp: .*,pbuilder-time-stamp: XXXX," \
	    -e "s,^Fetched .*B in .*s (.*B/s),Fetched XXXB in Xs (XXXXXB/s)," \
	| tee tests/log/$${A/*\//}.log; done

check: fastcheck slowcheck

check-syntax:
	$(CC) -c $(CFLAGS) $(CHK_SOURCES)  -o/dev/null -D LIBDIR="\"${LIBDIR}\""

.PHONY: clean check upload-dist-all check-syntax fastcheck slowcheck
