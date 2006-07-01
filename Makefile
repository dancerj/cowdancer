BINARY=libcowdancer.so cow-shell cowbuilder
INSTALL_DIR=install -d -o root -g root -m 755
INSTALL_FILE=install -o root -g root -m 644
INSTALL_PROGRAM=install -o root -g root -m 755
DESTDIR=
export VERSION=$(shell sed -n '1s/.*(\(.*\)).*$$/\1/p' < debian/changelog )

all: $(BINARY)

install: $(BINARY)
	$(INSTALL_DIR) $(DESTDIR)/usr/lib/cowdancer/
	$(INSTALL_DIR) $(DESTDIR)/usr/share/man/man1/
	$(INSTALL_DIR) $(DESTDIR)/usr/share/man/man8/
	$(INSTALL_FILE)  cow-shell.1 $(DESTDIR)/usr/share/man/man1/cow-shell.1
	$(INSTALL_FILE)  cowbuilder.8 $(DESTDIR)/usr/share/man/man8/cowbuilder.8
	$(INSTALL_FILE)  libcowdancer.so $(DESTDIR)/usr/lib/cowdancer/libcowdancer.so
	$(INSTALL_PROGRAM) cow-shell $(DESTDIR)/usr/bin/cow-shell
	$(INSTALL_PROGRAM) cowbuilder $(DESTDIR)/usr/sbin/cowbuilder

libcowdancer.so: cowdancer.lo
	gcc -ldl -shared -o $@ $<

cow-shell: cow-shell.o

cowbuilder: cowbuilder.o

%.lo: %.c
	gcc -D_REENTRANT -fPIC $< -o $@ -c -Wall -O2 -g

%.o: %.c
	gcc $< -o $@ -c -Wall -O2 -g 

clean: 
	-rm -f *~ *.o *.lo $(BINARY)

upload-dist-all:
	scp ../cowdancer_$(VERSION).tar.gz viper2.netfort.gr.jp:public_html/software/downloads

check:
	set -e; set -o pipefail; for A in tests/???_*.sh; do echo $$A; bash $$A  2>&1 | tee tests/log/$${A/*\//}.log; done

.PHONY: clean check upload-dist-all
