#!/bin/bash
set -x

if echo exit 1 | sudo cowbuilder --login --basepath /var/cache/pbuilder/base-test.cow; then
    exit 1;
else
    exit 0;
fi
