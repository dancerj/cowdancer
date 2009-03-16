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
	-mkdir -p .initrd/lib/modules/linux-live/kernel/drivers/ide/
	-mkdir -p .initrd/lib/modules/linux-live/kernel/drivers/net/
	-mkdir -p .initrd/lib/modules/linux-live/kernel/fs/ext2
	-mkdir -p .initrd/lib/modules/linux-live/kernel/fs/ext3
	-mkdir -p .initrd/lib/modules/linux-live/kernel/fs/jbd
	-ln -sf ../qemuinit-static-klibc .initrd/init
	-ln -sf /usr/lib/klibc/bin/insmod .initrd/bin/insmod
	-ln -sf /usr/lib/klibc/bin/sh.shared .initrd/bin/sh
	-ln -sf /lib/klibc*.so .initrd/lib/
	-ln -sf /lib/klibc*.so .initrd/lib/
	-ln -sf /lib/modules/$(shell uname -r)/kernel/drivers/ide/ide-core.ko .initrd/lib/modules/linux-live/kernel/drivers/ide/
	-ln -sf /lib/modules/$(shell uname -r)/kernel/drivers/ide/ide-disk.ko .initrd/lib/modules/linux-live/kernel/drivers/ide/
	-ln -sf /lib/modules/$(shell uname -r)/kernel/drivers/ide/ide-generic.ko .initrd/lib/modules/linux-live/kernel/drivers/ide/
	-ln -sf /lib/modules/$(shell uname -r)/kernel/drivers/net/8390.ko .initrd/lib/modules/linux-live/kernel/drivers/net/
	-ln -sf /lib/modules/$(shell uname -r)/kernel/drivers/net/ne2k-pci.ko .initrd/lib/modules/linux-live/kernel/drivers/net/
	-ln -sf /lib/modules/$(shell uname -r)/kernel/fs/jbd/jbd.ko .initrd/lib/modules/linux-live/kernel/fs/jbd/
	-ln -sf /lib/modules/$(shell uname -r)/kernel/fs/mbcache.ko .initrd/lib/modules/linux-live/kernel/fs/
	-ln -sf /lib/modules/$(shell uname -r)/kernel/fs/ext2/ext2.ko .initrd/lib/modules/linux-live/kernel/fs/ext2/
	-ln -sf /lib/modules/$(shell uname -r)/kernel/fs/ext3/ext3.ko .initrd/lib/modules/linux-live/kernel/fs/ext3/
	cd .initrd && find . | cpio --dereference -H newc -o > ../$@.1.tmp
	gzip -9 -n < $@.1.tmp > $@.2.tmp
	rm $@.1.tmp
	mv $@.2.tmp $@

check: initrd.gz
	# this assumes you're running on amd64.
	[ $(shell uname -m) = x86_64 ]
	qemu-system-x86_64 -kernel /boot/vmlinuz-$(shell uname -r) \
		-nographic \
		-hda /var/cache/pbuilder/base-i386.qemu \
		-hdb /dev/zero \
		-initrd ./initrd.gz \
		-serial stdio \
		-append "quiet console=ttyS0 pbuilderarch=i386 "

clean:
	rm -f qemuinit-static qemuinit-static-klibc *.o *~ initrd.gz 
	rm -rf .initrd

check-syntax:
	$(CC) -c -O2 -Wall $(CHK_SOURCES)  -o/dev/null -D LIBDIR="\"${LIBDIR}\""

.PHONY: clean check
