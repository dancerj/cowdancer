#!/bin/bash
# just test kernel functionality on waitpid.
set -ex

TESTCODE=$(readlink -f tests/022_kernel_waitpid.c)
RUNC=$(readlink -f tests/run_c.sh)

$RUNC $TESTCODE
