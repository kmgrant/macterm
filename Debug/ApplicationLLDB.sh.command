#!/bin/sh

PATH=/Applications/Xcode.app/Contents/Developer/usr/bin:$PATH
export PATH

cd `dirname $0`

# !!! IMPORTANT !!!
# Since MacTerm runs from a shared library referenced by a Python
# interpreter, "lldb" can only run on the Python binary and the
# environment must be set correctly to find dependent libraries.
# Relying on the "MacTerm" script's own debug mode is prudent.

cd '../Build/MacTerm.app/Contents/MacOS'
echo "MacTerm: launching MacTerm in the debugger (use 'r' at the prompt to start the program)"
env MACTERM_DEBUG=1 ./MacTerm
