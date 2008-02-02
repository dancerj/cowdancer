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

: > tests/log/901_test_pbuilder_create_distributions.log

FAIL=n

for DEBOOTSTRAP in debootstrap cdebootstrap; do
    for DISTRIBUTION in etch sid; do
	sudo rm -rf /var/cache/pbuilder/base-test.cow
	if [ $DEBOOTSTRAP = cdebootstrap ]; then
	    DEBUG="--debootstrapopts --verbose"
	else
	    DEBUG=
	fi
	if sudo cowbuilder --create $DEBUG  --hookdir /usr/share/doc/pbuilder/examples/workaround/ --distribution $DISTRIBUTION --debootstrap $DEBOOTSTRAP --basepath /var/cache/pbuilder/base-test.cow; then
	    echo "[OK]   $DEBOOTSTRAP $DISTRIBUTION" >> tests/log/901_test_pbuilder_create_distributions.log
	else
	    echo "[FAIL] $DEBOOTSTRAP $DISTRIBUTION" >> tests/log/901_test_pbuilder_create_distributions.log
	    FAIL=y
	fi
    done
done

if [ "$FAIL" = y ]; then
    exit 1;
fi
