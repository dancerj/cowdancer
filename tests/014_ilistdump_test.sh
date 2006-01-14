#!/bin/bash
# test ilist dumping, and content check.
set -ex

TESTDIR=$(mktemp -d )
TESTCODE=$(readlink -f tests/014_ilistdump_test.c)

(
    cd ${TESTDIR}
    mkdir 1/
# make a few files for testing.
    touch 1/a 1/b 1/c 1/d
    dd if=/dev/zero of=1/e bs=512 count=2

    cd 1 
    find -printf "%D %i\n" | sort +2 -n > ../mylist.1
    cow-shell echo hello
)
RET=$?

$TESTCODE < ${TESTDIR}/1/.ilist > ${TESTDIR}/mylist.2

diff -u ${TESTDIR}/mylist.1 ${TESTDIR}/mylist.2

rm -rf ${TESTDIR}
exit $RET
