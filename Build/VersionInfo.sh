#!/bin/sh

# VersionInfo.sh
#
# Parses the built executable's Info.plist file and finds the 5
# major version components, printing them out in order and space-
# delimited.
#
# Kevin Grant (kevin@ieee.org)
# June 17, 2004

if [ ! -r ./VersionInfo.sh ] ; then
    echo "$0: run this from the Build directory" > /dev/stderr
    exit 1
fi

# config
awk=/usr/bin/awk
dirname=/usr/bin/dirname
grep=/usr/bin/grep
perl=/usr/bin/perl

# find the generated property list
top_dir=`$dirname $0`
info_plist="${top_dir}/Application/ForDebugging/MacTelnet.app/Contents/Info.plist"

if [ ! -r "${info_plist}" ] ; then
    echo "$0: generated property list not found; please use Xcode to build the MacTelnet target" > /dev/stderr
    exit 1
fi

# extract version components from Info.plist's XML
$grep '<string>.*\..*\..*\..*\..*' ${info_plist} | $perl -ne 's/\s*<\/?string>\s*//g; print' | $awk 'BEGIN { FS = "." } { print $1" "$2" "$3" "$4" "$5 }'

