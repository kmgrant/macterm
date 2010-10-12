#!/bin/sh

# MacTelnetSwigConfig.sh
#
# Allows you to build SWIG for MacTelnet's use.
# (Download SWIG from "http://www.swig.org/".)
#
# Run this instead of "./configure" from your untarred
# "swig-<x.x>" build directory.
#
# Kevin Grant (kmg@mac.com)
# July 29, 2006

# IMPORTANT: MacTelnet's build system assumes /opt/swig
# regardless of what you may customize here.  See the
# Xcode configuration files for a SWIG_PREFIX variable.
#
# SWIG 1.3.29 or later works with Tiger (10.4).
# SWIG 1.3.30 or later is required for Leopard (10.5).
swigversion=1.3.39
prefix_with_ver=/opt/swig-${swigversion}
prefix_no_ver=/opt/swig

# set up a version-free alias for the specific version directory;
# once SWIG is built (post-configuration) this link will be valid
ln -snf ${prefix_with_ver} ${prefix_no_ver}

# enable only Python
./configure \
--prefix=${prefix_with_ver} \
--disable-ccache \
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
--without-octave \
--without-perl5 \
--without-pike \
--without-php \
--without-r \
--without-ruby \
--without-tcl \
;

