#!/bin/sh

PATH=/Developer/usr/bin:$PATH
export PATH

cd `dirname $0`
exec gdb '../Build/_Generated/ForDebugging/BugReporter.app/Contents/MacOS/BugReporter'
