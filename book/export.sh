#!/usr/bin/env bash
for i in $(find . -type f -depth 1); do cp -v $i ~/clanmills/exiv2/book ; done
cd ~/clanmills
mv exiv2/book/IMaEA.html exiv2/book/index.html
./syncup exiv2

# That's all Folks!
##
