#!/usr/bin/env bash

result=0
d=default.shtml
count=$(find . -name $d | wc -l)
if [ "$count" != "1" ]; then
    echo "***" loop in $PWD "***"
    result=1
    echo $PWD >> /tmp/scrape_loops
fi
exit $result

# That's all folks
##

