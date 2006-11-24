# run debuild with .git ignore.
debuild -us -uc -I.git
git push --tags costa
git push costa
