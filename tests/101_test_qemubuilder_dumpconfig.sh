#!/bin/bash
#check qemubuilder commnad-line option parsing.
set -ex

[ "$(./qemubuilder.c --inputfile one --inputfile two --dumpconfig | grep inputfile)" = \
"  inputfile[0]: one
  inputfile[1]: two" ]