#!/bin/bash
# test chown function handling.
set -ex

TESTDIR=$(mktemp -d )
TESTCODE=$(readlink -f tests/012_test_chown.c)
RUNC=$(readlink -f tests/run_c.sh)
set -- $(id -G)

#export COWDANCER_DEBUG=yes

# assumes that this user has multiple groups.
ORIGID=$1
NEWID=$2

if (( $# < 3 )); then
    echo W: Needs multiple groups for user $(id) for this test to succeed, test skipped
    exit 0
fi

cd ${TESTDIR}
mkdir 1/

# make a few files for testing.
touch 1/a 1/b 1/c 1/d
dd if=/dev/zero of=1/e bs=512 count=2
chgrp $ORIGID 1/a 1/b 1/c 1/d
ls -li 1/ > ls.prev

sleep 1s

cp -al 1/ 2

echo "   2/ before"
ls -li 2/ 
cow-shell $RUNC $TESTCODE $NEWID 2/a 2/b 2/c
echo "   2/ after"
ls -li 2/

rm -rf 2/

ls -li 1/ > ls.after


echo "   1/ differences; should not exist"
diff -u ls.prev ls.after

rm -rf ${TESTDIR}
