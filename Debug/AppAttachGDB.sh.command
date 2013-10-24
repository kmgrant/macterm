#!/bin/sh

# This is like "ApplicationGDB.sh.command", except it tries to find
# a currently-running Python process and attach to it.

PATH=/Applications/Xcode.app/Contents/Developer/usr/bin:$PATH
export PATH

cd `dirname $0`

ps_output=`ps -A -o 'pid command' | grep -i MacTerm_python | grep RunApplication`
attached_pid=`echo $ps_output | awk '{print $1}'`
executable_path=`echo $ps_output | awk '{print $2}'`
if [ "x$attached_pid" = "x" ] ; then
    echo "$0: could not find process ID of MacTerm" >&2
    exit 1
fi
if [ "x$executable_path" = "x" ] ; then
    echo "$0: could not find path to running executable of MacTerm" >&2
    exit 1
fi
warn=n
if [ "x${warn}" = "xy" ] ; then
    echo
    echo 'WARNING: This command cannot run in MacTerm, it should run in some'
    echo 'other terminal.  Otherwise, MacTerm and gdb will become unkillable.'
    echo
    echo 'Are you running this command outside of MacTerm? (y/n)'
    read yn
    if [ "x${yn:0}" != "xy" ] ; then
        echo "$0: quitting" >&2
        exit 0
    fi
fi
echo
echo 'Use "c" to continue.'
exec gdb ${executable_path} ${attached_pid}
