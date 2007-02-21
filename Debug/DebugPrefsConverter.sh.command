#!/bin/sh

cd `dirname $0`
exec gdb '../Build/PrefsConverter/ForDebugging/PrefsConverter.app/Contents/MacOS/PrefsConverter'
