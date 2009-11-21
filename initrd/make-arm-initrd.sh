#!/bin/bash
# prototype to create initrd for arm.
set -o pipefail
set -e
rm -rf .initrd-arm
mkdir .initrd-arm || true
(
for DEBS in arm/*.deb; do
    echo "dpkg -x $DEBS .initrd-arm"
done
echo "cp ./quick-arm-init.sh .initrd-arm/init"
echo "chmod a+x .initrd-arm/init"
echo "mv .initrd-arm/lib/modules/{2.6.26-2-versatile,linux-live}"
for DIR in root proc dev dev/pts; do
    echo "mkdir $DIR"
done
echo 
echo "(cd .initrd-arm && find . | cpio -H newc -o | gzip -9 > ../initrd.arm.gz.tmp)"
) | fakeroot
mv initrd.arm.gz.tmp initrd.arm.gz

qemu-system-arm -kernel .initrd-arm/boot/vmlinuz-2.6.26-2-versatile \
    -hda /dev/zero \
    -hdb /dev/zero \
    -initrd ./initrd.arm.gz \
    -serial stdio \
    -append "console=ttyAMA0 pbuilderarch=armel init=/bin/bash" \
    -M versatilepb

