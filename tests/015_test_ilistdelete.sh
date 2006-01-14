#!/bin/bash
# test if ilist deletion is handled gracefully
set -ex

exit 0; # disable this check

TESTDIR=$(mktemp -d )
TESTCODE=$(readlink -f tests/015_test_ilistdelete.c)

(
    cd ${TESTDIR}
    mkdir 1/
# make a few files for testing.
    touch 1/a 1/b 1/c 1/d
    dd if=/dev/zero of=1/e bs=512 count=2

    cow-shell $TESTCODE
)

rm -rf ${TESTDIR}
