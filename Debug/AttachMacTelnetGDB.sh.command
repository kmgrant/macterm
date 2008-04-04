#!/bin/sh

# This is like "DebugMacTelnet.sh.command", except it tries to find
# a currently-running python2.5 process (MacTelnet on Leopard) and
# attach to it.

attached_pid=`ps -A -o 'pid command' | grep -i python | grep RunMacTelnet | awk '{print $1}'`
if [ "x$attached_pid" = "x" ] ; then
    echo "$0: could not find process ID of MacTelnet" >&2
    exit 1
fi
warn=y
if [ "x${warn}" = "xy" ] ; then
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
echo '______________________________________________________________________________'
echo
echo 'IMPORTANT: When in gdb, say "run MacTelnet".  This will auto-breakpoint at'
echo '           __dyld__dyld_start().  Start MacTelnet by using the "c" command.'
echo '           (On Leopard, you may have to "c" multiple times.)'
echo '______________________________________________________________________________'
echo
exec gdb --pid=${attached_pid}
