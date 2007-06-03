#!/bin/bash
set -x

if [[ $(uname -mn ) != "dancer64 x86_64" ]]; then
    echo 'skip this test'
    exit 0;
fi

if [[ "$PBCURRENTCOMMANDLINEOPERATION" = build ]]; then
    echo 'skip this test'
    exit 0;
fi


if echo exit 1 | sudo cowbuilder --login --hookdir /usr/share/doc/pbuilder/examples/workaround/ --basepath /var/cache/pbuilder/base-test.cow; then
    exit 1;
else
    exit 0;
fi
