#!/bin/sh

# find the application bundle; this defines "project_dir",
# "build_dir" and "dot_app_dir", and relies on "top_dir"
tools_dir=`dirname $0`
top_dir=${tools_dir}/..
. ${tools_dir}/AppBundle.sh

exec ${top_dir}/Tools/WarnAboutHeaders.pl ${project_dir}/Code/*.{cp,mm}
