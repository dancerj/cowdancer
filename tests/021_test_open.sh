#!/bin/bash
# test close(0); open() pair will create FD=0
set -ex

TESTDIR=$(mktemp -d )
TESTCODE=$(readlink -f tests/021_test_open.c)

cd "${TESTDIR}"
mkdir 1/
# make a few files for testing.
touch 1/a 1/b 1/c 1/d
ln -s a 1/f
ln -s b 1/g
cp -al 1/ 2
# first, exclude non-cowdancer problem
"${TESTCODE}" "${TESTCODE}"
#check that cowdancer works.
cow-shell "${TESTCODE}" "${TESTCODE}"
RET=$?
echo $RET

rm -rf ${TESTDIR}
