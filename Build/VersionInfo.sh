#!/bin/sh

# VersionInfo.sh
#
# This script should be updated with each new release.  (Build
# numbers, however, are based on dates and automatically set.)
#
# If invoked without arguments, prints the 5 major version
# components space-delimited on a single line in the following
# order, with the preferences version as the 6th entry:
#     major-version
#     minor-version
#     subminor-version
#     alpha-beta
#     build-number
#     prefs-version
#
# If invoked with arguments, executes the given command line but
# first injects the 5 version components and the preferences
# version into the following environment variables:
#     MY_MAJOR_NUMBER
#     MY_MINOR_NUMBER
#     MY_SUBMINOR_NUMBER
#     MY_ALPHA_BETA
#     MY_BUILD_NUMBER
#     MY_PREFS_VERSION
#
# Kevin Grant (kmg@mac.com)
# June 17, 2004

MY_MAJOR_NUMBER=4
export MY_MAJOR_NUMBER

MY_MINOR_NUMBER=0
export MY_MINOR_NUMBER

MY_SUBMINOR_NUMBER=0
export MY_SUBMINOR_NUMBER

MY_ALPHA_BETA=b
export MY_ALPHA_BETA

MY_BUILD_NUMBER=`/bin/date +%Y%m%d`
export MY_BUILD_NUMBER

# IMPORTANT: The preferences version number is arbitrary, but it MUST match the
#            Preferences Converter application (PrefsConverter.xcode project);
#            the converter applies upgrades to user preferences one "version" at
#            a time, and it will only apply changes that it thinks are needed.
#            So for instance, if you bump the version by one, you MUST add some
#            code to the converter that handles the transition from the previous
#            version to your new version.  Note that the preferences system is
#            capable of incorporating new settings automatically, so conversion
#            (and a new version) ONLY applies if you are doing something that
#            is incompatible; for example, if you need to delete old settings,
#            or you need to change the meaning of an existing key name.
MY_PREFS_VERSION=6
export MY_PREFS_VERSION

if [ "x$1" = "x" ] ; then
    echo "$MY_MAJOR_NUMBER $MY_MINOR_NUMBER $MY_SUBMINOR_NUMBER $MY_ALPHA_BETA $MY_BUILD_NUMBER $MY_PREFS_VERSION"
else
    exec ${1+"$@"}
fi
