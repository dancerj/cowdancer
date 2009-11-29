#!/bin/bash
# prototype to create initrd for arm.
set -o pipefail
set -e
rm -rf .initrd-arm
mkdir .initrd-arm || true
(
for DIR in etc root proc dev dev/pts mnt; do
    echo "mkdir .initrd-arm/$DIR"
done
for DEBS in arm/*.deb; do
    echo "dpkg -x $DEBS .initrd-arm"
done
for DEV in "dev/sda b 8 0" "dev/sdb b 8 16"; do
    echo "mknod .initrd-arm/${DEV}"
done
echo "cp ./quick-arm-init.sh .initrd-arm/init"
echo "chmod a+x .initrd-arm/init"
echo "mv .initrd-arm/lib/modules/{2.6.26-2-versatile,linux-live}"
echo "touch .initrd-arm/etc/fstab"
echo "(cd .initrd-arm && find . | cpio -H newc -o | gzip -9 > ../initrd.arm.gz.tmp)"
# for debugging
# echo "find .initrd-arm/ -ls"
echo "ls -l .initrd-arm/dev"
) | fakeroot
mv initrd.arm.gz.tmp initrd.arm.gz

# this requires base-armel.qemu image accessible from user
qemu-system-arm -kernel .initrd-arm/boot/vmlinuz-2.6.26-2-versatile \
    -hda ~/tmp/base-armel.qemu \
    -hdb /dev/zero \
    -initrd ./initrd.arm.gz \
    -serial stdio \
    -append "console=ttyAMA0 pbuilderarch=armel init=/bin/bash" \
    -M versatilepb

