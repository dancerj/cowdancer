#!/bin/bash
# check qemubuilder commnad-line option parsing.
#
# Apparently, this test requires config files to exist in order not to
# die of error, so we need to have qemubuilder installed.
set -ex

[ "$(./qemubuilder --inputfile one --inputfile two --dumpconfig | grep inputfile)" = \
"  inputfile[0]: one
  inputfile[1]: two" ]