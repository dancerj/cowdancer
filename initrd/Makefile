KLCC=klcc

all: qemuinit-static qemuinit-static-klibc initrd.gz

%-static: %.o ../forkexec.o
	$(CC) -static -O2 -Wall -o $@ $^

%.o: %.c
	$(CC) -c -O2 -Wall -o $@ $^

%-static-klibc: %.c ../forkexec.c 
	$(KLCC) -Wall -o $@ $^

# try running with something like: 
# qemu-system-x86_64  -kernel /boot/vmlinuz-2.6.26-1-amd64 -hda /dev/zero -initrd ./initrd.gz
initrd.gz: qemuinit-static-klibc
	-ln -sf qemuinit-static-klibc init
	echo init | cpio --dereference --quiet -H newc -o | gzip -9 -n > $@.tmp
	mv $@.tmp $@

clean:
	rm -f qemuinit-static qemuinit-static-klibc *.o *~ init initrd.gz

check-syntax:
	$(CC) -c -O2 -Wall $(CHK_SOURCES)  -o/dev/null -D LIBDIR="\"${LIBDIR}\""

.PHONY: clean