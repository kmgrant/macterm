#!/bin/sh

PATH=/Applications/Xcode.app/Contents/Developer/usr/bin:$PATH
export PATH

cd `dirname $0`
echo "MacTerm: launching BugReporter in the debugger (use 'r' at the prompt to start the program)"
exec lldb -- '../Build/_Generated/ForDebugging/BugReporter.app/Contents/MacOS/BugReporter'
