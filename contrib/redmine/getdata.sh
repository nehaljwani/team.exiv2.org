#!/bin/bash

for i in {0..9}; do
    curl --silent "http://dev.exiv2.org/projects/exiv2/issues.json?offset=${i}00&limit=100&status_id="'*' > data${i}.json
    wc data${i}.json
done

# That's all Folks!
##

