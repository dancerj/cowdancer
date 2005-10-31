#!/bin/bash
# system test:
# test pbuilder work.
# assume /var/cache/pbuilder/base.tgz exists
# /var/cache/pbuilder/build/cow/{orig,work$$}/xxxx 

# this test requires root privs for pbuilder.


set -ex

if [ ! -x /usr/sbin/pbuilder ]; then
    echo Skipping pbuilder test.
    exit 0;
fi

ORIG=/var/cache/pbuilder/build/cow/orig
WORK=/var/cache/pbuilder/build/cow/work$$
BASETGZ=/var/cache/pbuilder/base.tgz
COWDEB=$(readlink -f ../cowdancer_0.6_amd64.deb)

sudo rm -rf /var/cache/pbuilder/build/cow
sudo mkdir -p $ORIG

cd $ORIG
sudo tar xfzp $BASETGZ
sudo cp $COWDEB $ORIG/tmp/
sudo apt-get source -d dsh
sudo chroot $ORIG dpkg -i tmp/cowdancer*.deb
sudo find $ORIG -ls > /tmp/ls-before
sudo cp -al $ORIG $WORK-1
sudo cp var/log/dpkg.log /tmp/a
sudo pbuilder update --buildplace $WORK-1 --no-targz --internal-chrootexec "chroot $WORK-1 cow-shell" 
sudo pbuilder build --buildplace $WORK-1 --no-targz --internal-chrootexec "chroot $WORK-1 cow-shell" dsh*.dsc
sudo diff -u /tmp/a $ORIG/var/log/dpkg.log
sudo rm -rf $WORK-1

sudo find $ORIG -ls > /tmp/ls-after

cd ..
sudo rm -rf /var/cache/pbuilder/build/cow

# /etc/passwd, /etc/group are due to 'pbuilder-buildpackage-funcs/createbuilduser'
diff -u /tmp/ls-before /tmp/ls-after

echo end

