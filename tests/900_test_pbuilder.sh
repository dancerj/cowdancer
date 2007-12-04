#!/bin/bash
# system test:
# test pbuilder work.
# assume /var/cache/pbuilder/base.tgz exists
# /var/cache/pbuilder/build/cow/{orig,work$$}/xxxx 

# this test requires root privs for pbuilder.

if [[ $(uname -mn ) != "dancer64 x86_64" ]]; then
    echo 'skip this test'
    exit 0;
fi

if [[ "$PBCURRENTCOMMANDLINEOPERATION" = build ]]; then
    echo 'skip this test'
    exit 0;
fi

set -ex

if [ ! -x /usr/sbin/pbuilder ]; then
    echo Skipping pbuilder test.
    exit 0;
fi

sudo rm -rf /var/cache/pbuilder/base-test.cow
sudo cowbuilder --create --hookdir /usr/share/doc/pbuilder/examples/workaround/ --debootstrap debootstrap --debootstrapopts --debug --basepath /var/cache/pbuilder/base-test.cow
sudo cowbuilder --update --hookdir /usr/share/doc/pbuilder/examples/workaround/ --basepath /var/cache/pbuilder/base-test.cow
pdebuild --pbuilder cowbuilder -- --basepath /var/cache/pbuilder/base-test.cow

echo end
