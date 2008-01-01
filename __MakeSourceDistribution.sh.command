#!/bin/sh

# __MakeSourceDistribution.sh.command
#
# Creates a tarball with all necessary files to build MacTelnet.
#
# You may need to set your PATH appropriately to find the
# Subversion (svn) program.
#
# Kevin Grant (kevin@ieee.org)
# June 17, 2004

# config
cat=/bin/cat
date=/bin/date
dirname=/usr/bin/dirname
find=/usr/bin/find
gnu_tar=/usr/bin/tar
root=~/Desktop # where the tarball goes
svn=svn

$date "+Started creating source tarball at %T."

# this must run from the top level of the trunk
BIN=`$dirname $0`
cd ${BIN}

# find the current version
version_parts=`Build/VersionInfo.sh`
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

# use Subversion to dump all checked-in files to the target;
# this is awesome because it simply omits any temporary files
# or other work-area gunk that doesn't belong in the release
$svn export . ${target}/

# create archive
$gnu_tar zcvf "$root/$directory.tar.gz" -C "$root" "$directory"

$date "+Finished creating source tarball at %T."

