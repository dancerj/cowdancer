#!/bin/bash
set -x

if sudo echo exit 1 | cowbuilder --login --basepath /var/cache/pbuilder/base-test.cow; then
    exit 1;
else
    exit 0;
fi
