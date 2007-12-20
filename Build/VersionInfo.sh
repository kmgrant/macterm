#!/bin/sh

# VersionInfo.sh
#
# This script should be updated with each new release.  (Build
# numbers, however, are based on dates and automatically set.)
#
# If invoked without arguments, prints the 5 major version
# components space-delimited on a single line, in the following
# order:
#     major-version
#     minor-version
#     subminor-version
#     alpha-beta
#     build-number
#
# If invoked with arguments, executes the given command line but
# first injects the 5 version components into the following
# environment variables:
#     MY_MAJOR_NUMBER
#     MY_MINOR_NUMBER
#     MY_SUBMINOR_NUMBER
#     MY_ALPHA_BETA
#     MY_BUILD_NUMBER
#
# Kevin Grant (kevin@ieee.org)
# June 17, 2004

MY_MAJOR_NUMBER=3
export MY_MAJOR_NUMBER

MY_MINOR_NUMBER=1
export MY_MINOR_NUMBER

MY_SUBMINOR_NUMBER=0
export MY_SUBMINOR_NUMBER

MY_ALPHA_BETA=a
export MY_ALPHA_BETA

MY_BUILD_NUMBER=`/bin/date +%Y%m%d`
export MY_BUILD_NUMBER

if [ "x$1" = "x" ] ; then
    echo "$MY_MAJOR_NUMBER $MY_MINOR_NUMBER $MY_SUBMINOR_NUMBER $MY_ALPHA_BETA $MY_BUILD_NUMBER"
else
    exec ${1+"$@"}
fi
