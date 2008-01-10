set -e
WORKDIR=/tmp/oprofwork
rm -rf "$WORKDIR" || true
mkdir "$WORKDIR"
cd "$WORKDIR"
sudo apt-get build-dep apt dpkg
apt-get source apt 
apt-get source dpkg
(cd apt-*; debuild -e DEB_BUILD_OPTIONS=nostrip)
(cd dpkg-*; debuild -e DEB_BUILD_OPTIONS=nostrip)
sudo dpkg -i *.deb

