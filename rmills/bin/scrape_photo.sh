#!/usr/bin/env bash

u=$(now)                      # updated
T=~/clanmills/2021/Template   # Template
d=default.shtml               # files
o=objects.txt
s=story.txt
p=photos.txt

url=$(grep https $d | cut -d\' -f 2)
echo ----- $PWD === $url -----
scrape $url photos > $o

# That's all folks
##

