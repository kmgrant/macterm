#!/bin/sh

cd `dirname $0`

# !!! IMPORTANT !!!
# Since MacTelnet runs from a shared library referenced by a
# Python interpreter, gdb can only run on the Python binary
# and it will auto-breakpoint at the library load point.
#
# When you enter gdb, you need to say "run MacOS/MacTelnet".
# (The current working directory is important due to the way
# bundles magically look for Python script components.)
#
# A trap will automatically trigger when MacTelnet's framework
# is loaded, which is normal.  Simply continue ("c") past it.
#
# Below is an example scenario within the debugger.
#     (gdb) r MacOS/MacTelnet
#     Starting program: Build/Application/ForDebugging/MacTelnet.app/Contents/MacOS/Python MacOS/MacTelnet
#     Reading symbols for shared libraries .................. done
#     
#     Program received signal SIGTRAP, Trace/breakpoint trap.
#     0x8fe0100c in __dyld__dyld_start ()
#     (gdb) c
#     Continuing.
#     ...

cd '../Build/Application/ForDebugging/MacTelnet.app/Contents'
echo
echo '______________________________________________________________________________'
echo
echo 'IMPORTANT: When in gdb, say "run MacOS/MacTelnet".  This will auto-breakpoint'
echo '           at __dyld__dyld_start().  Start MacTelnet by using the "c" command.'
echo '______________________________________________________________________________'
echo
exec gdb 'MacOS/python'
