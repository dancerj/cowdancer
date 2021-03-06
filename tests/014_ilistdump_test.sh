#!/bin/bash
# test ilist dumping, and content check.
set -ex

TESTDIR=$(mktemp -d )
TESTCODE=$(readlink -f cowdancer-ilistdump)

(
    cd ${TESTDIR}
    mkdir 1/
# make a few files for testing.
    touch 1/a 1/b 1/c 1/d
    ln 1/a 1/aa
    ln 1/b 1/bb
    cp -al 1 2

    dd if=/dev/zero of=1/e bs=512 count=2

    cd 1 
    find -links +1 -type f -printf "%D %i\n" | sort -k 2 -n > ../mylist.1
    cow-shell "${TESTCODE}" ${TESTDIR}/1/.ilist | sort -k 2 -n > ${TESTDIR}/mylist.2
)
RET=$?

diff -u ${TESTDIR}/mylist.1 ${TESTDIR}/mylist.2

rm -rf ${TESTDIR}
exit $RET
