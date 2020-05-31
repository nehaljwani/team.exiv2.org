#!/bin/bash

builds=~/Jenkins/builds/Latest
out=builds

if [ ! -d $out ]; then
    echo "*** create a directory called $out ***"
    exit
fi
if [ ! -d ~/Jenkins/builds/Latest ]; then
    echo "*** ~/Jenkins/builds/Latest does not exist ***"
    exit
fi

##
# clean up anything from last time
rm -rf $out/*

##
# copy the builds, remove the UNIX builds and chmod the others
cp -p "$builds"/* $out
rm -rf  $out/*SunOS* $out/*BSD*
echo ++++++++++++++++++
chmod 0444 $out/*
ls -lh     $out/*

# That's all folks!
##
