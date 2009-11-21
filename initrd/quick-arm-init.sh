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
lib/modules/linux-live/kernel/fs/mbcache.ko \
lib/modules/linux-live/kernel/fs/jbd/jbd.ko \
lib/modules/linux-live/kernel/fs/ext2/ext2.ko \
lib/modules/linux-live/kernel/fs/ext3/ext3.ko \
lib/modules/linux-live/kernel/drivers/net/smc91x.ko; do
    insmod $MODULE
done

/bin/bash
