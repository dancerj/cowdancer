KLCC=klcc

all: qemuinit-static qemuinit-static-klibc initrd.gz

%-static: %.o ../forkexec.o ../file.o
	$(CC) -static -O2 -Wall -o $@ $^

%.o: %.c
	$(CC) -c -O2 -Wall -o $@ $^

%-static-klibc: %.c ../forkexec.c ../file.c
	$(KLCC) -Wall -o $@ $^

initrd.gz: qemuinit-static-klibc Makefile
	-mkdir .initrd
	-mkdir .initrd/bin
	-mkdir .initrd/lib
	-mkdir .initrd/etc
	-ln -sf ../qemuinit-static-klibc .initrd/init
	-ln -sf /usr/lib/klibc/bin/insmod .initrd/bin/insmod
	-ln -sf /usr/lib/klibc/bin/sh.shared .initrd/bin/sh
	-ln -sf /lib/klibc*.so .initrd/lib/
	cd .initrd && find . | cpio --dereference -H newc -o > ../$@.1.tmp
	gzip -9 -n < $@.1.tmp > $@.2.tmp
	rm $@.1.tmp
	mv $@.2.tmp $@

check: initrd.gz
	# this assumes you're running on amd64.
	[ $(shell uname -m) = x86_64 ]
	qemu-system-x86_64 -kernel /boot/vmlinuz-2.6.26-1-amd64 \
		-nographic \
		-hda /dev/zero \
		-initrd ./initrd.gz \
		-serial stdio \
		-append "console=ttyS0 pbuilderarch=i386"

clean:
	rm -f qemuinit-static qemuinit-static-klibc *.o *~ initrd.gz 
	rm -rf .initrd

check-syntax:
	$(CC) -c -O2 -Wall $(CHK_SOURCES)  -o/dev/null -D LIBDIR="\"${LIBDIR}\""

.PHONY: clean check
