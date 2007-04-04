#!/bin/bash
# extract kernel file from DEB file.

# actually implement it.

FILENAMES=$(dpkg-deb --fsys-tarfile "$1" | tar xf - --wildcards ./boot/vmlinu\* )
echo extracting $FILENAMES
for f in $FILENAMES; do
    dpkg-deb --fsys-tarfile "$1" | tar xf - -O $f $(basename $f) > $f
done
