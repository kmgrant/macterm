#!/bin/sh

cd `dirname $0`
exec gdb '../Build/_Generated/ForDebugging/BugReporter.app/Contents/MacOS/BugReporter'
