#!/bin/sh

# MacTelnetSwigConfig.sh
#
# Allows you to build SWIG 1.3.29 for MacTelnet's use.
# (Download SWIG from "http://www.swig.org/".)
#
# Run this instead of "./configure" from your untarred
# "swig-1.3.29" build directory.
#
# Kevin Grant (kevin@ieee.org)
# July 29, 2006

# it is not really clear if it matters what version of GCC is
# used to build SWIG (after all, SWIG generates code that is
# ultimately sent to an arbitrary compiler); but, just in case,
# this forces GCC 3.3, which MacTelnet requires for other reasons

# on Mac OS X 10.4 and later, GCC 3.3 is not the default "gcc";
# but the configuration script searches for whatever "gcc", etc.
# is in the PATH; to force GCC 3.3 (which is installed on 10.4
# using a different name), a virtual GCC installation is created
# that uses version-free names for all the GCC components that
# matter, then that directory is put at the front of the PATH
gccdir=`pwd`/GCC-3.3-BIN
rm -Rf $gccdir
install -d $gccdir
ln -s /usr/bin/gcov-3.3 $gccdir/gcov
ln -s /usr/bin/gcc-3.3 $gccdir/gcc
ln -s /usr/bin/g++-3.3 $gccdir/g++
ln -s /usr/bin/cpp-3.3 $gccdir/cpp
ln -s /usr/bin/gcc-3.3 $gccdir/cc
ln -s /usr/bin/g++-3.3 $gccdir/c++

# fix PATH so the desired GCC is found first
PATH=$gccdir:/bin:/sbin:/usr/bin:/usr/sbin
export PATH

# verify version of GCC that is printed; should be 3.3
echo "checking gcc version... `gcc -dumpversion`"

# set up a version-free alias for the specific version directory;
# once SWIG is built (post-configuration) this link will be valid
ln -s /opt/swig-1.3.29 /opt/swig

# enable only Python
./configure \
--prefix=/opt/swig-1.3.29 \
--with-python \
--without-allegrocl \
--without-chicken \
--without-clisp \
--without-csharp \
--without-gcj \
--without-guile \
--without-java \
--without-lua \
--without-mzscheme \
--without-ocaml \
--without-perl5 \
--without-pike \
--without-php4 \
--without-ruby \
--without-tcl \
;

