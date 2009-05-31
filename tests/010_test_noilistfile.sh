#!/bin/bash
# test ILISTFILE=NULL case

# original error message example:
#LD_PRELOAD=/usr/lib/cowdancer/libcowdancer.so dd if=/dev/zero of=/dev/null count=1 bs=512
#cannot open ilistfile (null)
#dd: opening `/dev/zero': 不正なアドレスです

set -e

[ -n "${COWDANCER_SO}" ] # sanity check if COWDANCER_SO is set.

unset COWDANCER_ILISTFILE
if LD_PRELOAD="${COWDANCER_SO}" dd if=/dev/zero of=/dev/null count=1 bs=512; then
    exit 1;
else
    true;
fi

if COWDANCER_ILISTFILE=./nonexistent-file LD_PRELOAD="${COWDANCER_SO}" dd if=/dev/zero of=/dev/null count=1 bs=512; then
    exit 1;
else
    true;
fi

