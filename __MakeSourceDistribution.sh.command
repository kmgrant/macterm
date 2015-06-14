#!/bin/sh

# __MakeSourceDistribution.sh.command
#
# Creates a tarball with all necessary files to build MacTerm.
#
# Kevin Grant (kmg@mac.com)
# June 17, 2004

die () {
	[ "x$1" != "x" ] && echo "$0: $1" >&2
	exit 1
}

trap die ERR

# config
date=/bin/date
dirname=/usr/bin/dirname
git=git
root=~/Desktop # where the tarball goes

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
version=${maj}.${min}${sub}${alb}${bld}
directory=sourceMacTerm${version}
target=$root/${directory}.tar.gz

# create archive
$git archive --format=tar.gz --prefix="$directory/" master > "$target"

$date "+Finished creating source tarball at %T."

