#!/bin/bash

syntax() {
    echo "usage: $0 { --help | -? | -h | platform | option value | switch }+ "
    echo "platform:        all | cygwin[32] | linux[32]| macosx    | mingw[32] | msvc[32]"
    echo "switch:    --publish | --status   | --clone  | --debug   | --static  | --video  | --nonls | --clang"
    echo "msvc:         --2015 | --2017     | --2013   | --2012    | --2010    | --2008"
    echo "option:   --branch A | --server B | --user C | --builds D"
}

announce()
{
    if [ "$status" != "1" ]; then
        echo ++++++++++++++++++++++++++++++++
        echo $* ${build_type}${library_type}
        echo ++++++++++++++++++++++++++++++++
    fi
}

bomb() {
    echo "*** $1 requires an argument ***" >&2
    exit 1
}

# write tag into the build directory (in Unix format without \r)
writeTag()
{
    # $1 = server name (eg rmillsmm-w7)
    # $2 = command for ssh (eg msys64 or bash)
    # $3 = destination of tag
    # $4 = tag
    echo "echo tag=$4 > $3" | ssh ${user}@$1 $2
}

# if we're asked to clone, we remove the old build directory
prepareToClone()
{
    # $1 = server name (eg rmillsmm-w7)
    # $2 = command to remove directory 'buildserver' ("rmdir/s/q ${cd}buildserver")
    if [ "$clone" == "1" ]; then
        echo "$2" | ssh ${user}@$1 ${command} 2>/dev/null
    fi
}

# list the current build
reportStatus()
{
    # $1 = server (eg rmillsmm-w7)
    # $2 = program to run (eg msys64 or bash)
    # $3 = string to execute (eg cd .../buildserver/build ; ls -alt *.tar *.zip)
    echo "$3" | ssh ${user}@$1 $2
}

unixBuild()
{
    # $1 = server                   (eg rmillsmm-w7)
    # $2 = string for announcement  (eg 'MinGW 64' )
    announce  $1 $2
    if [ "$status" == "1" ]; then
        reportStatus $1 $command "cd ${cd}/buildserver/build; ls -alt *.tar.gz | sed -E -e 's/\+ / /g'"
    else
        # remove the buildserver directory if we are to clone
        prepareToClone $1 "rm -rf ${cd}/buildserver"
        ! ssh ${user}@$1 ${command} <<EOF
PATH="/usr/local/bin/:/usr/bin:/mingw64/bin:$PATH"
cd ${cd}
if [ $clang == 1 ]; then
   if [ $(uname) != Darwin ]; then
	 export CC=$(which clang)
	 export CXX=$(which clang++)
  fi
fi
if [ ! -e buildserver ]; then
    git clone --branch $branch https://github.com/exiv2/exiv2 buildserver --depth 1
fi
cd  buildserver
git fetch --unshallow
git pull  --rebase
git       status
mkdir -p  build
cd        build
if [ -e logs ]; then rm -rf logs ; fi
mkdir  -p logs
cmake .. -G "Unix Makefiles" -DEXIV2_TEAM_PACKAGING=O -DBUILD_SHARED_LIBS=${shared} -DEXIV2_ENABLE_VIDEO=${video} -DEXIV2_BUILD_PO=$nls -DEXIV2_ENABLE_NLS=$nls -DCMAKE_BUILD_TYPE=${config} 2>&1 | tee - > logs/build.txt
make                            2>&1      | tee -a logs/build.txt
make                            2>&1      | tee -a logs/build.txt
make tests                      2>&1      | tee -a logs/test.txt
ls -alt *.tar.gz | sed -E -e 's/\+ / /g'  | tee -a logs/test.txt
make package
EOF
        writeTag $1 $command ${cd}buildserver/build/tag $tag
    fi
}

msvcBuild()
{
    # $1 = server                   (eg rmillsmm-w7)
    cd=c:\\Users\\rmills\\gnu\\github\\exiv2\\

    profile=msvc${edition}${config}${bits}
    case "$edition" in
      2017) generator='Visual Studio 15 2017' ;;
      2015) generator='Visual Studio 14 2015' ;;
      2013) generator='Visual Studio 12 2013' ;;
      2012) generator='Visual Studio 11 2012' ;;
      2010) generator='Visual Studio 10 2010' ;;
      2008) generator='Visual Studio 9 2008'  ;;
    esac

    if [ "$bits" == "64" ]; then
         generator="${generator} Win64"
    fi
    generator="\"${generator}\""
    announce  $1 ${profile}

    if [ "$status" == "1" ]; then
        reportStatus $1 msys64 "cd ${cd}\\buildserver\\build ; ls -alt *.zip | sed -E -e 's/\+ / /g'"
    else
        prepareToClone $1 "rmdir/s/q ${cd}buildserver"
        ! ssh ${user}@$1 cmd64 <<EOF
setlocal
cd ${cd}
IF NOT EXIST buildserver git clone --branch ${branch} https://github.com/exiv2/exiv2 buildserver --depth 1
cd buildserver
git fetch --unshallow
git pull  --rebase
git status
if NOT EXIST build mkdir build
cd           build
if     EXIST logs  rmdir/s/q logs
mkdir logs
echo test log for $tag                                  2>&1 | c:\msys64\usr\bin\tee -a logs\test.txt
echo ++++++++++++++++++++++++++++++                     2>&1 | c:\msys64\usr\bin\tee -a logs\build.txt
set                                                     2>&1 | c:\msys64\usr\bin\tee -a logs\build.txt
echo ++++++++++++++++++++++++++++++                     2>&1 | c:\msys64\usr\bin\tee -a logs\build.txt
conan install .. --profile ${profile} --build missing   2>&1 | c:\msys64\usr\bin\tee -a logs\build.txt
cmake         .. -G ${generator} -DCMAKE_BUILD_TYPE=${config} -DBUILD_SHARED_LIBS=${shared} -DEXIV2_ENABLE_VIDEO=${video} -DEXIV2_TEAM_PACKAGING=On -DCMAKE_INSTALL_PREFIX=..\dist\${profile}  2>&1 | c:\msys64\usr\bin\tee -a  logs\build.txt
cmake --build .  --config ${config}                     2>&1 | c:\msys64\usr\bin\tee -a logs\build.txt
cd    ..\test
set EXIV2_EXT=.exe
set OLD_PATH=%PATH%
set PATH=c:\\Python34;c:\\msys64\\usr\\bin;%PATH%;
make test EXIV2_BINDIR=c:\Users\rmills\gnu\github\exiv2\buildserver\build\bin  2>&1 | c:\msys64\usr\bin\tee -a  ..\build\logs\test.txt
if  NOT %ERRORLEVEL% 1 set RESULT=ignored
set PATH=%OLD_PATH%
cd    ..\build
cmake --build .  --config ${config} --target package
exit 0
EOF
        writeTag $1 msys64 ${cd}buildserver\\build\\tag $tag
    fi
}

##
# assign defaults
cygwin=0
cygwin32=0
linux=0
linux32=0
macosx=0
mingw=0
mingw32=0
msvc=0
msvc32=0
help=0
publish=0
test=0
edition=2017
video=0
branch=0.27-RC3
clang=0
clone=0
config=Release
server=rmillsmm
user=rmills
status=0
shared=1
all=0
nls=1
shared=1
builds=/Users/rmills/Jenkins/builds

tag=$(date '+%Y:%m:%d_%H:%M:%S')
if [ "$#" == "0" ]; then help=1; fi

##
# parse command line
while [ "$#" != "0" ]; do
    arg="$1"
    shift
    case "$arg" in
      -h|--help|-\?) help=1    ;;
      all)       all=1         ;;
      cygwin)    cygwin=1;     ;;
      cygwin32)  cygwin32=1;   ;;
      linux)     linux=1;      ;;
      linux32)   linux32=1;    ;;
      macosx)    macosx=1      ;;
      mingw)     mingw=1;      ;;
      mingw32)   mingw32=1;    ;;
      msvc)      msvc=1        ;;
      msvc32)    msvc32=1      ;;
      --test)    test=1        ;;
      --publish) publish=1     ;;
      --clang    clang=1       ;;
      --clone)   clone=1       ;;
      --debug)   config=Debug  ;;
      --2017)    edition=2017  ;;
      --2015)    edition=2015  ;;
      --2013)    edition=2013  ;;
      --2012)    edition=2012  ;;
      --2010)    edition=2010  ;;
      --2008)    edition=2008  ;;
      --status)  status=1      ;;
      --static)  shared=0      ;;
      --video)   video=0       ;;
      --nonls)   nls=0         ;;
      --server)  if [ $# -gt 0 ]; then server="$1"  ; shift; else bomb $arg ; fi ;;
      --branch)  if [ $# -gt 0 ]; then branch="$1"  ; shift; else bomb $arg ; fi ;;
      --user)    if [ $# -gt 0 ]; then user="$1"    ; shift; else bomb $arg ; fi ;;
      --builds)  if [ $# -gt 0 ]; then builds="$1"  ; shift; else bomb $arg ; fi ;;
      *)         echo "*** invalid option: $arg ***" 1>&2; help=1; ;;
    esac
done

if [ $help == 1 ]; then
    syntax;
    exit 0;
fi

if [ "$all" == "1" ]; then
    cygwin=1; linux=1; macosx=1; mingw=1; msvc=1;
fi

publishBundle()
{
    # $1 = server    (eg rmillsmm-w7)
    # $2 = command   (eg msys32)
    # $3 = path      (eg /c/msys32/home/rmills/gnu/github/exiv2/buildserver/build)
    # $4 = extension (eg tar.gz or zip)

    # find the build tag left during the build
    tag_saved=$tag
    if [ -e tag ]; then rm -rf tag ; fi
    scp -q "$user@$1:$3/tag" . 2>/dev/null                      # silently collect build tag file
    if [ -e tag ]; then source tag; fi                          # and read it!

    files=$(echo ls -1 $3/*$4 | ssh $user@$1 $2 2>/dev/null)    # find the names of the bundles
    # echo +++ files = $files # after ssh $user@$1 $2 ls -1 $3/\*$4
    for file in $files; do
        if [ ! -z $file ]; then                                 # copy to builds/all and merge the tag into the filename
            echo scp -pq "$user@$1:$file" $builds/all/$(basename $file $4)-$tag$4
                 scp -pq "$user@$1:$file" $builds/all/$(basename $file $4)-$tag$4
        fi
    done
    tag=$tag_saved
}

if [ $publish == 1 ]; then
    ## create the source package
    echo "+++++++++++++++++++++++++++++++++++++++++"
    echo "+++ build Source in exiv2/buildserver +++"
    pushd ~/gnu/github/exiv2/buildserver/build >/dev/null
    make  package_source
    ls    -alt *Source.tar.gz|sed -E -e 's/\+ / /g'
    popd >/dev/null
    echo "+++++++++++++++++++++++++++++++++++++++++"
    publishBundle $server                      bash       /Users/$user/gnu/github/exiv2/buildserver/build             '.tar.gz'
    cygwin=0; linux=0; macosx=0; mingw=0; msvc=0; # don't build anything
    cygwin32=0; linux32=0; mingw32=0; msvc32=0;
    $(dirname $0)/categorize.py  $builds
fi

libary_type=''
if [ $shared != 1 ]; then library_type=Static; fi

##
# perform builds
if [ $cygwin == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='cygwin64'
    unixBuild ${server}-w7 Cygwin64
    publishBundle $server-w7                 ${command}   /c/cygwin64/home/$user/gnu/github/exiv2/buildserver/build   '.tar.gz'
fi
if [ $cygwin32 == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='cygwin32' ;
    unixBuild    ${server}-w7 Cygwin32
    publishBundle $server-w7                 ${command}   /c/cygwin32/home/$user/gnu/github/exiv2/buildserver/build   '.tar.gz'
fi

if [ $linux == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='bash'
    unixBuild     ${server}-ubuntu Linux64
    publishBundle ${server}-ubuntu           ${command}   /home/$user/gnu/github/exiv2/buildserver/build              '.tar.gz'
fi
if [ $linux32 == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='bash'
    unixBuild     ${server}-ubuntu32 Linux32
    publishBundle ${server}-ubuntu32         ${command}   /home/$user/gnu/github/exiv2/buildserver/build              '.tar.gz'
fi

if [ $macosx == 1 ]; then
    cd=/Users/rmills/gnu/github/exiv2/
    command='bash'
    unixBuild         ${server} MacOSX
    publishBundle     ${server}              ${command}   /Users/$user/gnu/github/exiv2/buildserver/build             '.tar.gz'
fi

if [ $mingw == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='msys64'
    unixBuild         ${server}-w7 MinGW/64
    publishBundle     ${server}-w7           ${command}   /c/msys64/home/$user/gnu/github/exiv2/buildserver/build     '.tar.gz'
fi
if [ $mingw32 == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='msys32'
    unixBuild         ${server}-w7 MinGW/32
    publishBundle     ${server}-w7           ${command}   /c/msys32/home/$user/gnu/github/exiv2/buildserver/build     '.tar.gz'
fi

if [ $msvc == 1 ]; then
    command='cmd64'
    bits=64
    msvcBuild     ${server}-w7
    publishBundle ${server}-w7                msys64      /c/Users/$user/gnu/github/exiv2/buildserver/build           '.zip'
fi
if [ $msvc32 == 1 ]; then
    command='cmd64'
    bits=''
    msvcBuild     ${server}-w7
    publishBundle ${server}-w7                msys64      /c/Users/$user/gnu/github/exiv2/buildserver/build           '.zip'
fi

# That's all Folks
##
