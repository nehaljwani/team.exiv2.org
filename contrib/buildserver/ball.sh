#!/usr/bin/env bash

max=0
for p in linux macos msvc cygwin mingw solaris freebsd netbsd
do
    (./build.sh "$p" $@ 2>&1 > /dev/null &)
    max=$((max+1))
    if [ $p == linux ]; then max=$((max+1)) ; fi
done

latest=/Users/Shared/Jenkins/Home/userContent/builds/Latest/
rm -rf $latest/*
count=0
while [ $count -lt $max ]; do
   count=$(ls -1 $latest | wc -l)
   echo $(date) ----- $count
   sleep 60
done
ls -1 $latest

# That's all Folks!
##
