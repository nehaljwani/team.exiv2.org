#!/bin/bash

input="/tmp/tvisitor-runall.txt"
testfiles=/Users/Shared/Jenkins/Home/userContent/testfiles
count=0
ignored=0
errors=0

# set -x

syntax() {
    echo "usage: ./runall.sh  { help | dryrun | verbose | list | program+ | option+ | section+ | name+ }+ "
    echo -n "sections: "  
    ( cd  "$testfiles" 
      for s in $(find . -maxdepth 1 -type d | sort --ignore-case)
      do   echo -n "$(basename $s) "
      done
    )
    echo
}

bomb() {
    echo "*** $1 requires an argument ***" >&2
    exit 1
}

##
# parse command line
if [ "$#" == "0" ]; then help=1; fi
while [ "$#" != "0" ]; do
    arg="$1"
    shift
    case "$arg" in
      -d|--dryrun|-dryrun)    dryrun=1      ;;
      -h|--help|-help|-\?)    help=1        ;;
      -l|--list|-list)        list=1        ;;
      -n|--name|-name)        if [ $# -gt 0 ]; then name="-iname $1"         ; shift; else bomb $arg ; fi ;;
      -o|--option|-option)    if [ $# -gt 0 ]; then option="$option $1"     ; shift; else bomb $arg ; fi ;;
      -p|--program|-program)  if [ $# -gt 0 ]; then program="$1"            ; shift; else bomb $arg ; fi ;;
      -s|--section|-section)  if [ $# -gt 0 ]; then section="$1"            ; shift; else bomb $arg ; fi ;;
      -v|--verbose|-verbose)  verbose=1     ;;
#     *)                      section=$1    ;;
      *)             echo "*** invalid option: $arg ***" 1>&2; help=1; ;;
    esac
done

# inspect/modify options
if [ ! -z "$help"      ]; then syntax ; exit 0      ; fi
if [ ! -z "$section"   ]; then testfiles="$testfiles/$section" ; fi
if [ ! -d "$testfiles" ]; then  >&2 echo "directory $testfiles does not exit" ; exit 1 ; fi
if [   -z "$program"   ]; then program=tvisitor     ; fi
if [ ! -z "$dryrun"    ]; then verbose=1            ; fi

if [ ! -z $verbose   ]; then
    echo program = $program
    echo section = $section
    echo option  = $option
    echo name    = $name
    echo "** command =  " find $testfiles -type f $name -print0 \| xargs -0 $program $option
fi
if [ ! -z $dryrun ]; then exit ; exit ; fi

if [ ! -z $list ]; then
    if [ -z "$name" ]; then
        cd "$testfiles"
        ls -1 | sort
    else
        find "$testfiles" -type f $name
    fi
    exit 0
fi

if [ 1 == 1 ]; then
    find "$testfiles" -type f $name -print0 | xargs -0 $program $option 2>/dev/null
else
    find "$testfiles" -type f $name | sort > "$input"
    while IFS= read -r file
    do
        filename=$(basename -- "$file")
        ext="${filename##*.}"
        ext=$(echo "$ext" | tr '[:lower:]' '[:upper:]')
        file "$file" | grep -q -e "image data" -e "ISO Media" 2>&1 > /dev/null 
        if [ "$?" == "0" -o "$ext" == CRW -o "$ext" == CR2 -o "$ext" == RAF -o "$ext" == RW2 -o "$ext" == RAW -o "$ext" == EXV ]; then
            count=$((count+1))
            $program "$@" $option "$file"
            if [ "$?" != "0" ]; then 
                >&2 echo FAILED $file
                errors=$((errors+1))
            fi
        else
            ignored=$((ignored+1))
            # don't report obvious duds.
            if [ "$ext" != ZIP -a "$ext" != 7Z -a "$ext" != TXT -a "$ext" != RAR -a "$ext" != XZ ]; then
                >&2 echo IGNORE $file
            fi
        fi
    done < "$input"
fi

if [ "$count" != "0" ] ; then  
  >&2 echo -------------------------------
  >&2 echo count $count failed $errors ignored $ignored
  >&2 echo -------------------------------
fi

# That's all Folks!
##
