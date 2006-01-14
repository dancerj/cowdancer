#!/bin/bash
# test the case with C++ source
set -ex

TESTDIR=$(mktemp -d )
TESTCODE=$(readlink -f tests/018_testcxx.cc)

(
    set -ex
    cd ${TESTDIR}
    mkdir 1/
    touch 1/a 1/b 1/c 1/d 
    dd if=/dev/zero of=1/e bs=512 count=2
    cp -al 1 0
    cow-shell $TESTCODE
    cat 1/a
    cat 0/a
    ! diff -u 1/a 0/a
) 
RESULT=$?

rm -rf ${TESTDIR}

exit $RESULT
