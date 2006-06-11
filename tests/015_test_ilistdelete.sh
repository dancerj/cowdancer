#!/bin/bash
# test if ilist deletion is handled gracefully
# 0.9 used to hang if pthread was used and ilist was not available.
set -ex

TESTDIR=$(mktemp -d )
TESTCODE=$(readlink -f tests/015_test_ilistdelete.c)

(
    cd ${TESTDIR}
    mkdir 1/
# make a few files for testing.
    touch 1/a 1/b 1/c 1/d
    cp -al 1 2
    dd if=/dev/zero of=1/e bs=512 count=2

    cow-shell $TESTCODE
)
RET=$?
rm -rf ${TESTDIR}

exit $RET