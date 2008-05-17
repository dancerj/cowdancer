#!/bin/bash
# just test kernel functionality on waitpid.
set -ex

TESTCODE=$(readlink -f tests/022_kernel_waitpid.c)

$TESTCODE
