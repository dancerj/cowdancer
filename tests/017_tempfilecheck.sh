#!/bin/bash
# test the case where there is already a tilde file.
# 0.10 uses mkstemp, so it should work.
set -ex

TESTDIR=$(mktemp -d )

(
    set -ex
    cd ${TESTDIR}
    mkdir 1/
# make a few files for testing.
    touch 1/a 1/b 1/c 1/d 1/a~~
    dd if=/dev/zero of=1/e bs=512 count=2
    cp -al 1 2
    cow-shell touch 1/a
# 0.9 deleted this file, not good. Test that it works now.
    ls 1/a~~
) 
RESULT=$?

rm -rf ${TESTDIR}

exit $RESULT
