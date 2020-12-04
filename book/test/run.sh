#!/usr/bin/env bash

pass=0
fail=0

# Create reference and tmp directories
if [ ! -e ../test/data ]; then mkdir ../test/data ; fi
if [ ! -e ../test/tmp  ]; then mkdir ../test/tmp  ; fi

report()
{
    stub=$1
    # if there's no reference file, create one
    # (make it easy to add tests or delete and rewrite all reference files)
    if [ ! -e "../test/data/$stub" ]; then
        cp "../test/tmp/$stub" ../test/data
    fi
    
    diff -q "../test/tmp/$stub" "../test/data/$stub" >/dev/null 
    if [ "$?" == "0" ]; then
        echo "$stub passed";
        pass=$((pass+1))
    else
        echo "$stub failed"
        fail=$((fail+1))
    fi
}

# test every file in ../files
for i in $( ls ../files/* | sort --ignore-case ) ; do
    stub=$(basename $i)
    # dmpf and csv are utility tests
    if [ $stub == dmpf -o $stub == csv -o $stub == args ]; then
        ./$stub ../files/$stub 2>&1 > "../test/tmp/$stub"
    else 
        ./tvisitor -pRU "$i"   2&>1 > "../test/tmp/$stub"
    fi
    report $stub
done

echo -------------------
echo Passed $pass Failed $fail
echo -------------------

# That's all Folks
##