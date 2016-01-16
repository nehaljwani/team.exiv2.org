#!/bin/bash

##
# redmine does not respect limit>100 Probably a global parameter in the server configuration file
# So, pull down 10 files on 100 issues
# Use status_id=* to get closed issues (default is to only show open issues)
# ./progress.py understand the 10 file approach and handles it almost instantly
for i in {0..9}; do
    curl --silent "http://dev.exiv2.org/projects/exiv2/issues.json?offset=${i}00&limit=100&status_id="'*' > data${i}.json
    wc data${i}.json
done

# That's all Folks!
##

