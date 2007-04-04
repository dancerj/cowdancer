#!/bin/bash
# some way to download the kernel. --- prototype idea?
# 
# references: crosshurd: /usr/share/crosshurd/functions

# TODO: actually think about implementation.

DEB_TARGET_ARCH=$1
shift
apt_options="--option Debug::Nolocking=true --option APT::Architecture=$DEB_TARGET_ARCH --option APT::Get::Force-Yes=true --option APT::Get::Download-Only=true"
apt-get $apt_options "$@"

# ./crossapt.sh update 
# ./crossapt.sh install (fails, because too many things are missing.)