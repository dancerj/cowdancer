#!/bin/bash

# quick prototype init program for arm
set -x 
cd / 
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
mount /proc /proc -t proc
mount /dev/pts /dev/pts -t devpts

for MODULE in lib/modules/linux-live/kernel/drivers/ide/ide-core.ko \
lib/modules/linux-live/kernel/drivers/ide/ide-disk.ko \
lib/modules/linux-live/kernel/drivers/ide/ide-generic.ko \
lib/modules/linux-live/kernel/drivers/scsi/scsi_mod.ko \
lib/modules/linux-live/kernel/drivers/scsi/scsi_transport_spi.ko \
lib/modules/linux-live/kernel/drivers/scsi/sym53c8xx_2/sym53c8xx.ko \
lib/modules/linux-live/kernel/drivers/scsi/sd_mod.ko \
lib/modules/linux-live/kernel/fs/mbcache.ko \
lib/modules/linux-live/kernel/fs/jbd/jbd.ko \
lib/modules/linux-live/kernel/fs/ext2/ext2.ko \
lib/modules/linux-live/kernel/fs/ext3/ext3.ko \
lib/modules/linux-live/kernel/drivers/net/smc91x.ko; do
    insmod $MODULE
done
# this process is asynchronous, need to wait about 2 seconds until SCSI device is detected.

echo "Try checking inside this initrd, exit to enter the chroot"
/bin/bash

echo "Entering chroot"
mount /dev/sda /mnt -t ext3
chroot /mnt /sbin/ifconfig eth0 up
chroot /mnt /sbin/dhclient eth0
chroot /mnt /usr/bin/apt-get update
chroot /mnt /bin/bash 
