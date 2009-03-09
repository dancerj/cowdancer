#!/bin/bash
# compare file $1 and $2, ignoring the time difference.

diff -u <(
    sed 's/^\[[0-9]\+\] //' < $1
) <(
    sed 's/^\[[0-9]\+\] //' < $2
)

