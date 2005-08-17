#!/bin/bash
# test pbuilder work.
# assume /var/cache/pbuilder/base.tgz exists
# /var/cache/pbuilder/build/cow/{orig,work$$}/xxxx 

# this test requires root privs for pbuilder.

set -ex

ORIG=/var/cache/pbuilder/build/cow/orig
WORK=/var/cache/pbuilder/build/cow/work$$
BASETGZ=/var/cache/pbuilder/base.tgz
COWDEB=$(readlink -f ../cowdancer_0.3_i386.deb)

sudo rm -rf /var/cache/pbuilder/build/cow
sudo mkdir -p $ORIG

cd $ORIG
sudo tar xfzp $BASETGZ
sudo cp $COWDEB $ORIG/tmp/
sudo chroot $ORIG dpkg -i tmp/cowdancer*.deb
sudo find $ORIG -ls > /tmp/ls-before
sudo cp -al $ORIG $WORK-1
sudo cp var/log/dpkg.log /tmp/a
sudo pbuilder update --buildplace $WORK-1 --no-targz --internal-chrootexec "chroot $WORK-1 cow-shell" 
sudo pbuilder build --buildplace $WORK-1 --no-targz --internal-chrootexec "chroot $WORK-1 cow-shell" /home/dancer/pending/20050813/cowdancer_0.2.dsc

sudo diff -u /tmp/a $ORIG/var/log/dpkg.log
sudo rm -rf $WORK-1

sudo find $ORIG -ls > /tmp/ls-after

cd ..
sudo rm -rf /var/cache/pbuilder/build/cow

# /etc/passwd, /etc/group are due to 'pbuilder-buildpackage-funcs/createbuilduser'
diff -u /tmp/ls-before /tmp/ls-after

echo end

