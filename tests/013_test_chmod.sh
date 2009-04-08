#!/bin/bash
# test chmod function handling.
set -ex

TESTDIR=$(mktemp -d )
TESTCODE=$(readlink -f tests/013_test_chmod.c)
RUNC=$(readlink -f tests/run_c.sh)

cd ${TESTDIR}
mkdir 1/

# make a few files for testing.
touch 1/a 1/b 1/c 1/d
dd if=/dev/zero of=1/e bs=512 count=2
chmod 600 1/a 1/b 1/c 1/d
ls -li 1/ > ls.prev

sleep 1s

cp -al 1/ 2

echo "   2/ before"
ls -li 2/ 
cow-shell $RUNC $TESTCODE 2/a 2/b
echo "   2/ after"
ls -li 2/

rm -rf 2/

ls -li 1/ > ls.after


echo "   1/ differences; should not exist"
diff -u ls.prev ls.after

rm -rf ${TESTDIR}
