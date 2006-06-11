#!/bin/bash
set -x

: > tests/log/901_test_pbuilder_create_distributions.log

for DEBOOTSTRAP in debootstrap cdebootstrap; do
    for DISTRIBUTION in etch sid; do
	sudo rm -rf /var/cache/pbuilder/base-test.cow
	if [ $DEBOOTSTRAP = cdebootstrap ]; then
	    DEBUG="--debootstrapopts --debug"
	else
	    DEBUG=
	fi
	if sudo cowbuilder --create $DEBUG --distribution $DISTRIBUTION --debootstrap $DEBOOTSTRAP --basepath /var/cache/pbuilder/base-test.cow; then
	    echo "[OK]   $DEBOOTSTRAP $DISTRIBUTION" >> tests/log/901_test_pbuilder_create_distributions.log
	else
	    echo "[FAIL] $DEBOOTSTRAP $DISTRIBUTION" >> tests/log/901_test_pbuilder_create_distributions.log
	fi
    done
done
