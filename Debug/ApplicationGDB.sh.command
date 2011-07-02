#!/bin/sh

PATH=/Developer/usr/bin:$PATH
export PATH

cd `dirname $0`

# !!! IMPORTANT !!!
# Since MacTerm runs from a shared library referenced by a
# Python interpreter, gdb can only run on the Python binary
# and it will auto-breakpoint at the library load point.
#
# When you enter gdb, you need to say "run MacTerm".  (This
# is the argument passed to the Python interpreter, and works
# because the current directory is changed to "MacOS" first.)
#
# A trap will automatically trigger when MacTerm's framework
# is loaded, which is normal.  Simply continue ("c") past it.
#
# Below is an example scenario within the debugger.  Usually
# only one "c" is required, but on Leopard there will be more.
#     (gdb) r MacTerm
#     Starting program: Build/MacTerm.app/Contents/MacOS/MacTerm_python2.5 MacTerm
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
#     MacTerm: Starting up.
#     ...

if [ -r "/usr/bin/python2.5" ] ; then
    # apparently Leopard...the normal method will not work, so use run-then-attach method
    '../Build/MacTerm.app/Contents/MacOS/MacTerm' &
    sleep 1
    exec './AppAttachGDB.sh.command'
else
    # prior to Leopard, execute gdb normally
    cd '../Build/MacTerm.app/Contents/MacOS'
    echo
    echo '______________________________________________________________________________'
    echo
    echo 'IMPORTANT: When in gdb, say "run MacTerm".  This will auto-breakpoint at'
    echo '           __dyld__dyld_start().  Start MacTerm by using the "c" command.'
    echo '           (On Leopard, you may have to "c" multiple times.)'
    echo '______________________________________________________________________________'
    echo
    exec gdb './MacTerm_python2.x'
fi
