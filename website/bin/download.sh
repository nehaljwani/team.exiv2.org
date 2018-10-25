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
cp        $basedir/builds/*                   $basedir/html/builds

for P in Darwin Linux CYGWIN MinGW-64 MinGW-32 MSVC Source ; do
    ext=tar.gz
	if [ $P == MSVC ]; then ext=zip ; fi
	p=exiv2-$version-$P.$ext

	size=$(ls -la        html/builds/$p | cut -d' ' -f 5)
	date=$(stat -c "%y"  html/builds/$p | cut -d' ' -f 1)
	checkSum=$(sha256sum html/builds/$p | cut -d' ' -f 1)

	echo "<tr>  \
            <td>Exiv2 v$version $P</td> \
            <td><a href=\"builds/${p}\">${p}</a></td> \
            <td>$size</td> \
            <td class=\"text-nowrap\">$date</td> \
            <td>$checkSum</td> \
          </tr>"  >> $basedir/var/$table

	platform=$P
	config="64 bit shared libraries"
	if [ "$platform" == Darwin   ]; then platform="MacOSX"                 ; fi
	if [ "$platform" == MinGW-32 ]; then config="32 bit shared libraries"  ; fi
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

# That's all Folks!
##
