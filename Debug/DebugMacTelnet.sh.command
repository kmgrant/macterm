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
#     (gdb) r MacTelnet
#     Starting program: Build/MacTelnet.app/Contents/MacOS/python MacTelnet
#     Reading symbols for shared libraries ++. done
#     
#     Program received signal SIGTRAP, Trace/breakpoint trap.
#     0x8fe0100c in __dyld__dyld_start ()
#     (gdb) c
#     Continuing.
#     Reading symbols for shared libraries . done
#     
#     Program received signal SIGTRAP, Trace/breakpoint trap.
#     0x8fe0100c in __dyld__dyld_start ()
#     (gdb) c
#     Continuing.
#     
#     Program received signal SIGTRAP, Trace/breakpoint trap.
#     0x8fe0100c in __dyld__dyld_start ()
#     (gdb) c
#     Continuing.
#     Reading symbols for shared libraries . done
#     ...
#     MacTelnet: Starting up.
#     ...

cd '../Build/MacTelnet.app/Contents/MacOS'
echo
echo '______________________________________________________________________________'
echo
echo 'IMPORTANT: When in gdb, say "run MacTelnet".  This will auto-breakpoint at'
echo '           __dyld__dyld_start().  Start MacTelnet by using the "c" command.'
echo '           (On Leopard, you may have to "c" multiple times.)'
echo '______________________________________________________________________________'
echo
exec gdb './python'
