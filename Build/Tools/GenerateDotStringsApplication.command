#!/bin/sh

# GenerateDotStringsApplication.sh.command
#
# Runs genstrings on the MacTelnet source files that
# actually contain localized string calls, and writes
# output to the proper location (application bundle).
#
# Kevin Grant (kmg@mac.com)
# March 24, 2003

date "+Started generating strings at %T."

# find the application bundle; this defines "project_dir",
# "build_dir" and "dot_app_dir", and relies on "top_dir"
tools_dir=`dirname $0`
top_dir=${tools_dir}/..
. ${tools_dir}/AppBundle.sh

# generate ALL strings
/usr/bin/genstrings ${project_dir}/Code/UIStrings.cp -o ${dot_app_dir}/Contents/Resources/English.lproj

if [ "x$?" != "x0" ] ; then
    exit 1
fi

date "+Finished generating strings at %T."

