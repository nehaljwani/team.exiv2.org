#!/bin/bash

builds=~/Jenkins/builds/Latest
out=builds

for b in "$builds"/* ; do
	# b=$(echo $b)
	b=$(basename $b)
	ext=$(echo $b | cut -d. -f 5-6)
	echo $b | grep MinGW >/dev/null
	if [ "$?" == "0" ]; then
	   B=$(echo $b | cut -d- -f 1-4)
    else
	   B=$(echo $b | cut -d- -f 1-3)
	fi
	B=$B.$ext
	# echo -e $b '\t' $B
	cp -p $builds/$b $out/$B
done
# echo ++++++++++++++++++
chmod 0444 $out/*
ls -lh  $out

# That's all folks!
##
