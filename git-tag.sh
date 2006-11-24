VERSION=$(dpkg-parsechangelog | sed -n 's/^Version: //p')
git-tag -s -u dancer@debian.org -m "apt-listbugs release $VERSION" $VERSION

