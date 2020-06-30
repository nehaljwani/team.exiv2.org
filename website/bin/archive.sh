#!/bin/sh
# Create download tables

basedir=.
table=__archive_table__
rm -rf $basedir/var/$table

subString()
{
	# $1 = string $2 = sub
	# $3 = if sub  $4 otherwise
    case "$1" in
        *$2*) echo $3;;
        *)    echo $4;;
    esac
}

#  <tr>
#   <td>Exiv2 v0.3 Windows executable</td>
#   <td><a title="Download the v0.3 Windows executable" href="releases/exiv2-0.3-win.zip" >exiv2-0.3-win.zip</a></td>
#   <td>188242</td>
#   <td class="text-nowrap">10-May-2004</td>
#   <td>eae2eeaffb328e8023ac346877e7860f59e0d82a4df4cf080a6ef9e3026e9fb0</td>
#  </tr>

# get the releases in reversed 'version' order 0.27 > 0.10 > 0.6 etc
releases=$(ls -1 $basedir/html/releases/* | sort -t- -k 2 -V | perl -e 'print reverse <>')
# echo +++++++++++++++++++++++++++++
# echo +++ releases = $releases
# echo +++++++++++++++++++++++++++++

for p in $releases; do
	size=$(ls -la        $p | sed -e 's#  # #g' | cut -d' ' -f 5)
	# date=unknown
	checkSum=$(sha256sum $p | cut -d' ' -f 1)

	# labels for 0.26 and earlier releases (Source = .tar.gz, Windows = -win.zip, Doc = -doc.tar.gz)
	file=$(   echo $p    | cut -d/ -f 4-)
	stub=$(echo $file | cut -d- -f 2-)
	source=$(echo $file | cut -d- -f 2)
	version=$(subString "$stub" ".tar.gz"    $(echo $stub |sed -E -e 's/.tar.gz//') $(echo $stub |sed -E -e 's/-win.zip//'))
	kind=$(   subString "$p"    ".tar.gz"    "Source"           "Windows Executable" )
	kind=$(   subString "$p"    "doc"        "Documentation"    "$kind"              )
    version=$(subString "$file" "Source"     "$source"          "$version"           )

    # labels for v0.27 and later (exiv2-0.27.0-msvc64.zip, exiv2-0.27.1-Source.tar.gz exiv2-0.27.0-Linux.tar.gz)
    case "$file" in
        *msvc*   ) kind='Windows Build'   ;;
        *Linux*  ) kind='Linux Build'     ;;
        *CYGWIN* ) kind='Cygwin Build'    ;;
        *MinGW*  ) kind='MinGW/msys2 Build'     ;;
        *Darwin* ) kind='macOS Build'    ;;
    esac
    case "$kind" in
         *Build* ) version=$(echo $version | cut -d- -f 1) ;;
    esac

	echo "<tr>  \
            <td>Exiv2 v$version $kind</td> \
            <td><a href=\"/releases/$file\">$file</a></td> \
            <td align=\"right\">$size</td> \
            <!-- <td class=\"text-nowrap\">$date</td> --> \
            <td>$checkSum</td> \
          </tr>"  >> $basedir/var/$table

done

# That's all Folks!
##

