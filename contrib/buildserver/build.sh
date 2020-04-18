#!/usr/local/bin/env bash

syntax() {
    echo "usage: $0   { --help | -?  |   -h | platform   | switch     | option     | location value }+ "
    echo "platform:    all[32] | cygwin     | freebsd    | linux[32]  | macos|unix | mingw | msvc[32] | netbsd | solaris"
    echo "switch:     --source | --clone    | --debug    | --static   | --clang"
    echo "options:   --[no]nls | --video    | --asan     | --webready | --unit     | --status"
    echo "msvc:         --2019 | --2017     | --2015     | --2013     | --2012     | --2010    | --2008"
    echo "location: --server B | --user C   | --builds D | --stamp stamp"
    echo "location: --github {rmillsmm,github,E}         | { --branch branch | --tag tag } "
}
this="$0"
announce()
{
    if [ "$status" != "1" ]; then
        compiler=''
        options=''
        if [ $clang    == 1 ]; then compiler=Clang ; fi
        if [ $video    == 1 ]; then options="${options}Video "    ; fi
        if [ $webready == 1 ]; then options="${options}Webready " ; fi
        echo ++++++++++++++++++++++++++++++++
        echo $* ${compiler}${config}${library_type} ${options} ${branch}
        echo ++++++++++++++++++++++++++++++++
    fi
}

bomb() {
    echo "*** $1 requires an argument ***" >&2
    exit 1
}

# write stamp into the build directory (in Unix format without \r)
writeStamp()
{
    # $1 = server name (eg rmillsmm-w10)
    # $2 = command for ssh (eg msys64 or bash)
    # $3 = destination of stamp
    # $4 = stamp
    echo "echo stamp=$4 > $3" | ssh  -o ConnectTimeout=30 ${user}@$1 $2
}

# if we're asked to clone, we remove the old build directory
prepareToClone()
{
    # $1 = server name (eg rmillsmm-w10)
    # $2 = command to remove directory 'buildserver' ("rmdir/s/q ${cd}buildserver")
    if [ "$clone" == "1" ]; then
        echo "$2" | ssh -o ConnectTimeout=30 ${user}@$1 ${command} 2>/dev/null
    fi
}

# list the current build
reportStatus()
{
    # $1 = server (eg rmillsmm-w10)
    # $2 = program to run (eg msys64 or bash)
    # $3 = string to execute (eg cd .../buildserver/build ; ls -alt *.tar *.zip)
    echo "$3" | ssh -o ConnectTimeout=30 ${user}@$1 $2
}

unixBuild()
{
    # $1 = server                   (eg rmillsmm-w10)
    # $2 = string for announcement  (eg 'MinGW 64' )
    announce  $1 $2
    if [ "$status" == "1" ]; then
        reportStatus $1 $command "cd ${cd}/buildserver/build; ls -alt *.tar.gz | sed -e 's/\+ / /g'"
    else
        # remove the buildserver directory if we are to clone
        prepareToClone $1 "rm -rf ${cd}/buildserver"
        CLANG=""
        if [ $clang == 1 ]; then CLANG='echo "using Clang";export CC=$(which clang);export CXX=$(which clang++)' ; fi
        ! ssh -o ConnectTimeout=30 ${user}@$1 ${command} <<EOF
PATH="/usr/local/bin/:/usr/bin:/mingw64/bin:/usr/pkg/bin/:/usr/pkg/sbin:$PATH"
cd ${cd}
${CLANG}
if [ ! -e buildserver ]; then
    git clone --branch $branch $github buildserver --depth 1
fi
cd  buildserver
git fetch --unshallow
git pull  --rebase
git checkout $tag
git       status
rm    -rf build
mkdir -p  build
cd        build
rm    -rf logs
mkdir -p  logs
export                                             2>&1 | tee -a logs/build.txt
echo cmake .. -G "Unix Makefiles" -DEXIV2_TEAM_PACKAGING=On -DBUILD_SHARED_LIBS=${shared} -DEXIV2_BUILD_UNIT_TESTS=${unit} -DEXIV2_TEAM_USE_SANITIZERS=${asan} -DEXIV2_ENABLE_VIDEO=${video} -DEXIV2_ENABLE_WEBREADY=${webready} -DEXIV2_ENABLE_SSH=0 -DEXIV2_ENABLE_CURL=${webready} -DEXIV2_ENABLE_NLS=$nls -DCMAKE_BUILD_TYPE=${config} 2>&1 | tee -a logs/build.txt
     cmake .. -G "Unix Makefiles" -DEXIV2_TEAM_PACKAGING=On -DBUILD_SHARED_LIBS=${shared} -DEXIV2_BUILD_UNIT_TESTS=${unit} -DEXIV2_TEAM_USE_SANITIZERS=${asan} -DEXIV2_ENABLE_VIDEO=${video} -DEXIV2_ENABLE_WEBREADY=${webready} -DEXIV2_ENABLE_SSH=0 -DEXIV2_ENABLE_CURL=${webready} -DEXIV2_ENABLE_NLS=$nls -DCMAKE_BUILD_TYPE=${config} 2>&1 | tee -a logs/build.txt
if [ "$source" == "0" ]; then
    make                                                                   2>&1 | tee -a logs/build.txt
    ls -alt bin                                                            2>&1 | tee -a logs/build.txt
    make tests                                                             2>&1 | tee -a logs/build.txt
    if [ -e bin/unit_tests -o -e bin/unit_test.exe ]; then bin/unit_tests  2>&1 | tee -a logs/build.txt      ; fi
    make package
else
    make package_source
fi
ls -alt *.tar.gz | sed -e 's/\+ / /g'           2>&1 | tee -a logs/test.txt
EOF
        writeStamp $1 $command ${cd}buildserver/build/stamp $stamp
    fi
}

msvcBuild()
{
    # $1 = server                   (eg rmillsmm-w10)
    cd=c:\\Users\\rmills\\gnu\\github\\exiv2\\

    case "$edition" in
      2019) generator='Visual Studio 16 2019' ; bits='' ;;
      2017) generator='Visual Studio 15 2017' ;;
      2015) generator='Visual Studio 14 2015' ;;
      2013) generator='Visual Studio 12 2013' ;;
      2012) generator='Visual Studio 11 2012' ;;
      2010) generator='Visual Studio 10 2010' ;;
      2008) generator='Visual Studio 9 2008'  ;;
    esac
    if [ "$shared" == "0" ]; then
        profile=msvc${edition}Static${config}${bits}
    else
        profile=msvc${edition}${config}${bits}
    fi

    if [ "$bits" == "64" ]; then
         generator="${generator} Win64"
    fi
    generator="\"${generator}\""
    announce  $1 ${profile}

    if [ "$status" == "1" ]; then
        reportStatus $1 msys64 "cd ${cd}\\buildserver\\build ; ls -alt *.zip | sed -e 's/\+ / /g'"
    else
        prepareToClone $1 "rmdir/s/q ${cd}buildserver"
        ! ssh -o ConnectTimeout=30 ${user}@$1 cmd64 <<EOF
setlocal
cd ${cd}
@echo off
IF NOT EXIST buildserver git clone --branch ${branch} $github buildserver --depth 1
IF NOT EXIST buildserver echo +++++++++++++++ clone failed ++++++++++++++++++++++++++++++++
IF NOT EXIST buildserver exit 1
@echo on
cd buildserver
git fetch --unshallow
git pull  --rebase
git checkout $tag
git status
if     EXIST build rmdir/s/q build
if NOT EXIST build mkdir build
cd           build
if     EXIST logs  rmdir/s/q logs
mkdir logs
echo  test log for $stamp                                                               2>&1 | c:\msys64\usr\bin\tee -a logs\test.txt
set                                                                                   2>&1 | c:\msys64\usr\bin\tee -a logs\build.txt
conan install .. --profile ${profile} --options webready=${webready} --build missing  2>&1 | c:\msys64\usr\bin\tee -a logs\build.txt
cmake         .. -G ${generator} -DCMAKE_BUILD_TYPE=${config} -DEXIV2_ENABLE_DYNAMIC_RUNTIME=${shared} -DBUILD_SHARED_LIBS=${shared} -DEXIV2_ENABLE_WEBREADY=${webready} -DEXIV2_ENABLE_SSH=0 -DEXIV2_ENABLE_CURL=${webready} -DEXIV2_BUILD_UNIT_TESTS=${unit} -DEXIV2_ENABLE_VIDEO=${video} -DEXIV2_TEAM_PACKAGING=On -DCMAKE_INSTALL_PREFIX=..\dist\${profile}  2>&1 | c:\msys64\usr\bin\tee -a  logs\build.txt
cmake --build .  --config ${config}                                                   2>&1 | c:\msys64\usr\bin\tee -a logs\build.txt
cd    bin
set   EXIV2_BINDIR=%CD%
cd    ..\..\test
set   EXIV2_EXT=.exe
set   OLD_PATH=%PATH%
set   PATH=c:\Python37;C:\Python37\Scripts;c:\msys64\usr\bin;%PATH%;
make  test                                                                            2>&1 | c:\msys64\usr\bin\tee -a  ..\build\logs\test.txt
if    NOT %ERRORLEVEL% 1 set RESULT=ignored
if    EXIST %EXIV2_BINDIR%\unit_tests.exe %EXIV2_BINDIR%\unit_tests.exe               2>&1 | c:\msys64\usr\bin\tee -a  ..\build\logs\test.txt
if    NOT %ERRORLEVEL% 1 set RESULT=ignored
set   PATH=%OLD_PATH%
cd    ..\build
cmake --build .  --config ${config} --target package
exit  0
EOF
        writeStamp $1 msys64 ${cd}buildserver\\build\\stamp stamp
    fi
}

##
# assign defaults
all=0
all32=0
asan=0
branch=0.27-maintenance
builds=/Users/Shared/Jenkins/Home/userContent/builds
categorize=0
clang=0
clone=0
config=Release
curl=False
cygwin=0
cygwin32=0
edition=2019
freebsd=0
help=0
linux=0
linux32=0
macos=0
github=github
mingw=0
mingw32=0
msvc=0
msvc32=0
netbsd=0
nls=1
publish=0
server=$(hostname|cut -d. -f 1|cut -d- -f 1)
shared=1
solaris=0
source=0
status=0
unit=False
unix=0
user=$(whoami)
video=0
webready=False
stamp=$(date '+%Y-%m-%d_%H:%M:%S')
if [ "$#" == "0" ]; then help=1; fi

##
# parse command line
while [ "$#" != "0" ]; do
    arg="$1"
    shift
    case "$arg" in
      -h|--help|-\?) help=1       ;;
      all)          all=1;all32=0 ;;
      all32)        all=0;all32=1 ;;
      cygwin)       cygwin=1      ;;
      freebsd)      freebsd=1     ;;
      linux)        linux=1       ;;
      linux32)      linux32=1     ;;
      macos)        macos=1       ;;
      mingw)        mingw=1       ;;
      msvc)         msvc=1        ;;
      msvc32)       msvc32=1      ;;
      netbsd)       netbsd=1      ;;
      solaris)      solaris=1     ;;
      unix)         unix=1        ;;
      --asan)       asan=1        ;;
      --categorize) categorize=1  ;;
      --clang)      clang=1       ;;
      --clone)      clone=1       ;;
      --debug)      config=Debug  ;;
      --full)       full=1        ;;
      --nls)        nls=1         ;;
      --nonls)      nls=0         ;;
      --publish)    publish=1     ;;
      --source)     source=1      ;;
      --static)     shared=0      ;;
      --status)     status=1      ;;
      --unit)       unit=True     ;;
      --video)      video=1       ;;
      --webready)   webready=True ;;
      --2008)       edition=2008  ;;
      --2010)       edition=2010  ;;
      --2012)       edition=2012  ;;
      --2013)       edition=2013  ;;
      --2015)       edition=2015  ;;
      --2017)       edition=2017  ;;
      --2019)       edition=2019  ;;
      --branch)     if [ $# -gt 0 ]; then branch="$1"  ; shift; else bomb $arg ; fi ;;
      --builds)     if [ $# -gt 0 ]; then builds="$1"  ; shift; else bomb $arg ; fi ;;
      --github)     if [ $# -gt 0 ]; then github="$1"  ; shift; else bomb $arg ; fi ;;
      --server)     if [ $# -gt 0 ]; then server="$1"  ; shift; else bomb $arg ; fi ;;
      --stamp)      if [ $# -gt 0 ]; then stamp="$1"   ; shift; else bomb $arg ; fi ;;
      --tag)        if [ $# -gt 0 ]; then tag="$1"     ; shift; else bomb $arg ; fi ;;
      --user)       if [ $# -gt 0 ]; then user="$1"    ; shift; else bomb $arg ; fi ;;
      *)            echo "*** invalid option: $arg ***" 1>&2; help=1; ;;
    esac
done

if [ $source == 1 ]; then
    clone=1
fi

if [ $asan == 1 ]; then
    debug=1
fi

if [ $help == 1 ]; then
    syntax;
    exit 0;
fi

if [ ! -z $tag ]; then
    clone=1
    publish=1
fi

if [ $github == github   ]; then github=git://github.com/exiv2/exiv2                          ; fi
if [ $github == rmillsmm ]; then github=rmills@rmillsmm:/Users/rmills/gnu/github/exiv2/github ; fi

if [ "$all" == "1" ]; then
    cygwin=1; linux=1; macos=1; mingw=1; msvc=1;unit=True;clone=1;publish=1;
fi
if [ "$unix" == "1" ]; then
    solaris=1;freebsd=1;netbsd=1;publish=1;unit=True;clone=1;
fi
if [ "$all32" == "1" ]; then
    linux32=1; msvc32=1;clone=1;publish=1
fi

if [ "$full" == "1" ]; then
	ssh=0
	video=1
	curl=1
	webready=True
	publish=1
fi

publishBundle()
{
    # $1 = server    (eg rmillsmm-w10)
    # $2 = command   (eg msys32)
    # $3 = path      (eg /c/msys32/home/rmills/gnu/github/exiv2/buildserver/build)
    # $4 = extension (eg tar.gz or zip)
    if [ $publish == 1 ]; then
		# find the build stamp left during the build
		stamp_saved=$stamp
		if [ -e stamp ]; then rm -rf stamp ; fi
		scp -q "$user@$1:$3/stamp" . 2>/dev/null                    # silently collect build stamp file
		if [ -e stamp ]; then source stamp; fi                      # and read it!

		# echo ++-- "ls -l $3/*$4"
		files=$(echo ls -1 $3/*$4 | ssh -o ConnectTimeout=30 $user@$1 $2 2>/dev/null)    # find the names of the bundles
		# echo +++ files = $files # after ssh $user@$1 $2 ls -1 $3/\*$4
		for file in $files; do
			# echo ++++++ '>'$file'<'
			if [ ! -z $file ]; then                                 # copy to builds/all and merge the stamp into the filename
				echo scp -pq "$user@$1:$file" $builds/all/$(basename $file $4)-$stamp$4
					 scp -pq "$user@$1:$file" $builds/all/$(basename $file $4)-$stamp$4 2>/dev/null
			fi
		done
		stamp=$stamp_saved
		$(dirname $this)/categorize.py  $builds
    fi
}

libary_type=''
if [ $shared != 1 ]; then library_type=Static; fi

##
# perform builds
if [ $linux == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='bash'
    unixBuild     ${server}-ubuntu Linux64
    publishBundle ${server}-ubuntu           ${command}   /home/$user/gnu/github/exiv2/buildserver/build              '.tar.gz'
    if [ $publish == 1 ]; then
		# recursively build package_source on a clean clone
		if [ -z $tag ]; then
		   "$this" --source --clone --stamp "$stamp" --branch   "$branch"   --github "$github" --publish
		else
		   "$this" --source --clone --stamp "$stamp" --tag      "$tag"      --github "$github" --publish
		fi
	fi
fi
if [ $linux32 == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='bash'
    unixBuild     ${server}-ubuntu32 Linux32
    publishBundle ${server}-ubuntu32         ${command}   /home/$user/gnu/github/exiv2/buildserver/build              '.tar.gz'
fi

if [ $source == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='bash'
    unixBuild     ${server}-ubuntu Source
    publishBundle ${server}-ubuntu           ${command}   /home/$user/gnu/github/exiv2/buildserver/build               '.tar.gz'
fi

if [ $freebsd == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='bash'
    unixBuild     ${server}-freebsd FreeBSD
    publishBundle ${server}-freebsd          ${command}   /home/$user/gnu/github/exiv2/buildserver/build              '.tar.gz'
fi
if [ $netbsd == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='bash'
    unixBuild     ${server}-netbsd NetBSD
    publishBundle ${server}-netbsd           ${command}   /home/$user/gnu/github/exiv2/buildserver/build              '.tar.gz'
fi
if [ $solaris == 1 ]; then
    nls_saved=$nls
    nls=0                  # gettest isn't installed on rmillsmm-solaris
    cd=/export/home/rmills/gnu/github/exiv2/
    command='bash'
    unixBuild     ${server}-solaris Solaris
    publishBundle ${server}-solaris          ${command}   /export/home/$user/gnu/github/exiv2/buildserver/build       '.tar.gz'
    nls=nls_saved
fi

clang=0  # clang is not supported on Cygwin/macOS/MinGW/msvc/FreeBSD/NetBSD/Solaris
if [ $macos == 1 ]; then
    cd=/Users/rmills/gnu/github/exiv2/
    command='bash'
    unixBuild         ${server} macOS
    publishBundle     ${server}                bash        /Users/$user/gnu/github/exiv2/buildserver/build            '.tar.gz'
fi

if [ $msvc == 1 ]; then
    command='cmd64'
    bits=64
    msvcBuild     ${server}-w10
    publishBundle ${server}-w10                msys64      /c/Users/$user/gnu/github/exiv2/buildserver/build          '.zip'
fi
if [ $msvc32 == 1 ]; then
    command='cmd64'
    bits='32'
    msvcBuild     ${server}-w10
    publishBundle ${server}-w10                msys64      /c/Users/$user/gnu/github/exiv2/buildserver/build          '.zip'
fi

if [ $cygwin == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='cygwin64'
    unixBuild ${server}-w10 Cygwin/64
    publishBundle ${server}-w10                msys64      /c/cygwin64/home/$user/gnu/github/exiv2/buildserver/build  '.tar.gz'
fi

if [ $mingw == 1 ]; then
    cd=/home/rmills/gnu/github/exiv2/
    command='msys64'
    unixBuild         ${server}-w10 MinGW/64
    publishBundle     ${server}-w10            msys64      /c/msys64/home/$user/gnu/github/exiv2/buildserver/build     '.tar.gz'
fi

# That's all Folks
##
