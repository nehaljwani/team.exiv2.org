#!/bin/sh
# Create download tables

basedir=.
version=$(cat $basedir/var/__version__)

table=__download_table__
buttons=__download_buttons__

rm -rf $basedir/var/$table
rm -rf $basedir/var/$buttons

##
# clean up from last time and copy builds html/builds
if [ -e   $basedir/html/builds ]; then rm -rf $basedir/html/builds ; fi
mkdir -p  $basedir/html/builds

for P in $basedir/builds/*
do
    # P = ./builds/exiv2-0.27.0.2-CYGWIN-2018:10:31_11:57:09.tar.gz
    platform=$(echo $P | cut -d- -f 3   ) # CYGWIN
    S=$(       echo $P | cut -d- -f 1-3 ) # ./builds/exiv2-0.27.0.2-CYGWIN
    stub=$(    echo $S | cut -d/ -f 3-  ) # exiv2-0.27.0.2-CYGWIN
    ext=$(     echo $P | cut -d. -f 6-  ) # tar.gz
    p=$stub.$ext                          # exiv2-0.27.0.2-CYGWIN.tar.gz
    if [ 1 -eq 2 ]; then
		echo P = $P
		echo platform = $platform
		echo S = $S
		echo stub = $stub
		echo ext  = $ext
		echo p = $p
    fi

    cp    -p  $P  $basedir/html/builds/$p
	size=$(ls -la        html/builds/$p | cut -d' ' -f 5)
	date=$(stat -c "%y"  html/builds/$p | cut -d' ' -f 1-2 | cut -d: -f 1-2)
	checkSum=$(sha256sum html/builds/$p | cut -d' ' -f 1)

	echo "<tr>  \
            <td>Exiv2 v$version $platform</td> \
            <td><a href=\"builds/$p\">$p</a></td> \
            <td>$size</td> \
            <td class=\"text-nowrap\">$date</td> \
            <td>$checkSum</td> \
          </tr>"  >> $basedir/var/$table

	config="64 bit shared libraries"
	if [ "$platform" == Darwin   ]; then platform="MacOSX"                 ; fi
	if [ "$platform" == MinGW32  ]; then config="32 bit shared libraries"  ; fi
	if [ "$platform" == MSVC     ]; then platform="Visual Studio"; config="64 bit DLLs for<br>Visual Studio 2017"; fi
	if [ "$platform" != Source   ]; then
  	  echo "<tr><td>$platform<h3></td><td>$config</td> \
	        <td> \
	          <p3 class=\"text-center\"> \
                <a href=\"builds/$p\" class=\"btn btn-sm btn-success\"> \
	            <span class=\"glyphicon glyphicon-download-alt\" aria-hidden=\"true\"></span>&nbsp;$p \
	            </a> \
	         </p3> \
            </td></tr>" >> $basedir/var/$buttons
    fi
done
ls -l $basedir/html/builds/*

# That's all Folks!
##
