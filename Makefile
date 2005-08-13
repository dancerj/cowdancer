BINARY=libcowdancer.so cow-shell
INSTALL_DIR=install -d -o root -g root -m 755
INSTALL_FILE=install -o root -g root -m 644
INSTALL_PROGRAM=install -o root -g root -m 755
DESTDIR=

all: $(BINARY)

install: $(BINARY)
	$(INSTALL_DIR) $(DESTDIR)/usr/lib/cowdancer/
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
	:

check:
	:

.PHONY: clean check upload-dist-all