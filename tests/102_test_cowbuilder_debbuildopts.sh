#!/bin/bash
# check cowbuilder command-line parsing, for debbuildopts.
#
# Do not yet check for failure or success because it's broken right now.
set -ex

CONFIGFILE=tests/102_test_cowbuilder_debbuildopts.config

[ "$(./cowbuilder --configfile ${CONFIGFILE} --dumpconfig | grep debbuildopts:)" = \
    "  debbuildopts: -j2 -I" ]

[ "$(./cowbuilder --configfile ${CONFIGFILE} --dumpconfig | grep kernel_image:)" = \
    "  kernel_image: /boot/vmlinuz-x.y.z" ]