#!/bin/sh

cd `dirname $0`
exec gdb '../Build/_Generated/ForDebugging/PrefsConverter.app/Contents/MacOS/PrefsConverter'
