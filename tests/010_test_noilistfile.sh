#!/bin/bash
# test ILISTFILE=NULL case

#LD_PRELOAD=/usr/lib/cowdancer/libcowdancer.so dd if=/dev/zero of=/dev/null count=1 bs=512
#cannot open ilistfile (null)
#dd: opening `/dev/zero': �����ʥ��ɥ쥹�Ǥ�
set -e

unset ILISTFILE
if LD_PRELOAD=/usr/lib/cowdancer/libcowdancer.so dd if=/dev/zero of=/dev/null count=1 bs=512; then
    exit 1;
else
    true;
fi

if COWDANCER_ILISTFILE=./nonexistent-file LD_PRELOAD=/usr/lib/cowdancer/libcowdancer.so dd if=/dev/zero of=/dev/null count=1 bs=512; then
    exit 1;
else
    true;
fi

