#!/bin/bash
# test if there is much memory leaking.
set -ex

TESTDIR=$(mktemp -d )
TESTCODE=$(readlink -f tests/016_memleakcheck.c)
gcc ${TESTCODE} -o ${TESTDIR}/tracer -g 

(
    set -ex
    cd ${TESTDIR}
    mkdir 1/
# make a few files for testing.
    touch 1/a 1/b 1/c 1/d
    dd if=/dev/zero of=1/e bs=512 count=2

    cp -al 1 2 

    MALLOC_TRACE="log" cow-shell ${TESTDIR}/tracer
)
RESULT=$?

mtrace ${TESTDIR}/tracer ${TESTDIR}/log
rm -rf ${TESTDIR}

exit $RESULT

