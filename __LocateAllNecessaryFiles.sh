#!/bin/sh

# __LocateAllNecessaryFiles.sh
#
# Prints the path (relative to the top level of the project) to all
# important files.  This is determined by finding everything and
# filtering out backup files and stuff used by the Mac OS, etc.
#
# Despite the filters, it is still a very good idea to only run this
# immediately after "cleaning" a build, so no junk files are found.
#
# Kevin Grant (kevin@ieee.org)
# June 17, 2004

# config
egrep=/usr/bin/egrep
find=/usr/bin/find
sed=/usr/bin/sed

# find self
BIN=`dirname $0`

# show all files in the project except "junk" (backups, generated files, etc.);
# also omit filenames with whitespace, because these will only screw up Unix
# commands down the pipe, and are known to match things that are unnecessary
# (e.g. "English.lproj idx", a generated file)
cd ${BIN}
$find _*.sh* _*.txt Archive Build Debug Extras Licenses Test -type f -o -type l | $egrep -v '(Archive\/ByYear|DeveloperDocs|\/a\.out|\.o$|\.so$|\.pyc$|\.build\/?|DS_Store|vttest-2002|\~\.| )'

