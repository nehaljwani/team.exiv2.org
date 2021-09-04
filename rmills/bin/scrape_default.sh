#!/usr/bin/env bash

u=$(now)                  # updated
T=~/clanmills/2021/Test   # Template
d=default.shtml           # files
o=objects.txt
s=story.txt

empty_dir.sh
cp $T/$d  .
cp $T/$s  .
echo PWD = $PWD
leaf=$(basename $PWD)
clanmills="$HOME/clanmills" 
clanmills=${#clanmills}
path=${PWD:$clanmills:100}   # remove /Users/rmills/clanmills
url="https://clanmills.com$path/"       
share=$(grep gl $d  | cut -d\' -f 2)
echo ----- leaf = "$leaf" path = "$path" url = "$url" share = "$share" -----
sed -E -i .bak -e "s/__title__/$leaf/g" $d
sed -E -i .bak -e "s/__updated__/$u/g"  $d
rm  -rf *.bak
scrape $share photos > $o
scrape $url   cols2  > $s
cat $d
ls -l

# That's all folks
##

