#!/bin/sh

PATH=/Applications/Xcode.app/Contents/Developer/usr/bin:$PATH
export PATH

cd `dirname $0`
echo "MacTerm: launching PrefsConverter in the debugger (use 'r' at the prompt to start the program)"
exec lldb -- '../Build/_Generated/ForDebugging/PrefsConverter.app/Contents/MacOS/PrefsConverter'
