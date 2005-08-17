#!/bin/bash
# test symlink handling.

set -ex

TESTDIR=$(mktemp -d )

cd ${TESTDIR}
mkdir 1/

# make a few files for testing.
touch 1/b 1/c 1/d 1/a.real
dd if=/dev/zero of=1/e bs=512 count=2
ln -s a.real 1/a
ls -li 1/ > ls.prev

sleep 1s

cp -al 1/ 2/ 

echo "   2/ before"
ls -li 2/ 
echo "cd 2 && echo a > a && mv b c && touch c && dd if=e of=d" | cow-shell
echo "   2/ after"
ls -li 2/

rm -rf 2/

ls -li 1/ > ls.after


echo "   1/ differences; should not exist"
diff -u ls.prev ls.after

rm -rf ${TESTDIR}
