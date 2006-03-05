#!/bin/bash
# test that things are working as expected, with filename with space
# add a test for quote as well.
#
# Requires a installed cowdancer, and will test the installed cowdancer.

set -ex

TESTDIR=$(mktemp -d )

cd ${TESTDIR}
mkdir 1/

# make a few files for testing.
touch "1/a'f" "1/b g" "1/c h" "1/d i"
dd if=/dev/zero of="1/e j" bs=512 count=2

ls -li 1/ > ls.prev

sleep 1s

cp -al 1/ 2

echo "   2/ before"
ls -li 2/ 
echo "cd 2 && echo a > \"a'f\" && mv 'b g' 'c h' && touch 'c h' && dd if='e j' of='d i'" | cow-shell
echo "   2/ after"
ls -li 2/

rm -rf 2/

ls -li 1/ > ls.after


echo "   1/ differences; should not exist"
diff -u ls.prev ls.after

rm -rf ${TESTDIR}
