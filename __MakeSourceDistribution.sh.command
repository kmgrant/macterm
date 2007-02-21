#!/bin/sh

# __MakeSourceDistribution.sh.command
#
# Creates a tarball with all necessary files to build MacTelnet.
#
# IMPORTANT: Build MacTelnet first; that's the only way the version
#            number will be properly defined.
#
# Kevin Grant (kevin@ieee.org)
# June 17, 2004

# config
cat=/bin/cat
date=/bin/date
dirname=/usr/bin/dirname
grep=/usr/bin/grep
gnu_tar=/usr/bin/tar
ls=/bin/ls
perl=/usr/bin/perl
root=~/Desktop # where the tarball goes

$date "+Started creating source tarball at %T."

BIN=`$dirname $0`

# this must run from the top level directory
cd ${BIN}

# extract version from Info.plist's XML
pushd ${BIN}/Build > /dev/null
version_parts=`./VersionInfo.sh`
popd > /dev/null
if [ "x${version_parts}" = "x" ] ; then
	echo "$0: version lookup failed" > /dev/stderr
	exit 1
fi
i=0
for PART in $version_parts ; do
	version_array[$i]=${PART}
	i=1+$i
done
maj=${version_array[0]}
min=${version_array[1]}
sub=${version_array[2]}
alb=${version_array[3]}
bld=${version_array[4]}
if [ "x$sub" = "x0" ] ; then
	sub=
elif [ "x$sub" != "x" ] ; then
	sub=.${sub}
fi
version=${maj}.${min}${sub}${alb}build${bld}
directory=sourceMacTelnet${version}
target=$root/$directory

mkdir "$target"

# find the files to archive; printed filenames will
# contain a double-underscore ('__') in place of
# actual spaces, so those should be restored
${BIN}/__LocateAllNecessaryFiles.sh > $target/MANIFEST

# attempt to determine if the user may not have cleaned
# the project before creating an archive; this is unwise
# because it will make the archive bigger and full of
# junk files (plus, an attempt should be made to build
# the release independent of any regular work area)
$grep "MacTelnet.build/DerivedSources/English.lproj/AutomaticallyGeneratedDictionary.r" $target/MANIFEST > /dev/null
if [ "$?" = "0" ] ; then
    echo "$0: project appears to be unclean" > /dev/stderr
    exit 1
fi

# NOTE: on Tiger, tar sucks in all kinds of extra files
#       (presumably from Spotlight), DESPITE the explicit
#       list given; this is irritating as hell and even
#       results in permissions warnings being spewed for
#       a few of them, but I don't know how to prevent it
$gnu_tar -cf - `$cat $target/MANIFEST` | $gnu_tar xpf - -C "$target"

# create archive
$gnu_tar zcvf "$root/$directory.tar.gz" -C "$root" "$directory"

$date "+Finished creating source tarball at %T."

