#!/bin/sh

cd `dirname $0`
exec gdb '../Build/BugReporter/ForDebugging/BugReporter.app/Contents/MacOS/BugReporter'
