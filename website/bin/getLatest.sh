#!/bin/bash

builds=~/Jenkins/builds/Latest
out=builds

##
# clean up anything from last time
cd  $out
rm -rf *

##
# copy the builds and chmod them
cp -p "$builds"/* .
echo ++++++++++++++++++
chmod 0444 *
ls -lh     .

# That's all folks!
##
