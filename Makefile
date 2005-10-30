BINARY=libcowdancer.so cow-shell
INSTALL_DIR=install -d -o root -g root -m 755
INSTALL_FILE=install -o root -g root -m 644
INSTALL_PROGRAM=install -o root -g root -m 755
DESTDIR=
VERSION=0.6

all: $(BINARY)

install: $(BINARY)
	$(INSTALL_DIR) $(DESTDIR)/usr/lib/cowdancer/
	$(INSTALL_DIR) $(DESTDIR)/usr/share/man/man1/
	$(INSTALL_FILE)  cow-shell.1 $(DESTDIR)/usr/share/man/man1/cow-shell.1
	$(INSTALL_FILE)  libcowdancer.so $(DESTDIR)/usr/lib/cowdancer/libcowdancer.so
	$(INSTALL_PROGRAM) cow-shell $(DESTDIR)/usr/bin/cow-shell

libcowdancer.so: cowdancer.lo
	gcc -ldl -shared -o $@ $<

cow-shell: cow-shell.o

%.lo: %.c
	gcc -fPIC $< -o $@ -c -Wall -O2 -g

%.o: %.c
	gcc $< -o $@ -c -Wall -O2 -g 

clean: 
	-rm *~ *.o *.lo $(BINARY)

upload-dist-all:
	scp ../cowdancer_$(VERSION).tar.gz viper2.netfort.gr.jp:public_html/software/downloads

check:
	set -e; for A in tests/???_*.sh; do echo $$A; bash $$A  > tests/log/$${A/*\//}.log 2>&1; done


.PHONY: clean check upload-dist-all
