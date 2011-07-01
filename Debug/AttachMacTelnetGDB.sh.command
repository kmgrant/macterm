#!/bin/sh

# This is like "DebugMacTelnet.sh.command", except it tries to find
# a currently-running Python 2.5 process (MacTelnet on Leopard) and
# attach to it.

PATH=/Developer/usr/bin:$PATH
export PATH

attached_pid=`ps -A -o 'pid command' | grep -i MacTelnet_python | grep RunApplication | awk '{print $1}'`
if [ "x$attached_pid" = "x" ] ; then
    echo "$0: could not find process ID of MacTelnet" >&2
    exit 1
fi
warn=n
if [ "x${warn}" = "xy" ] ; then
    echo
    echo 'WARNING: This command cannot run in MacTelnet, it should run in some'
    echo 'other terminal.  Otherwise, MacTelnet and gdb will become unkillable.'
    echo
    echo 'Are you running this command outside of MacTelnet? (y/n)'
    read yn
    if [ "x${yn:0}" != "xy" ] ; then
        echo "$0: quitting" >&2
        exit 0
    fi
fi
echo
echo 'Use "c" to continue.'
exec gdb --pid=${attached_pid}
