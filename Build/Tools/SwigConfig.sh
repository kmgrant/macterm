#!/bin/sh

# SwigConfig.sh
#
# Allows you to build SWIG for MacTerm's use.
# (Download SWIG from "http://www.swig.org/".)
#
# Run this instead of "./configure" from your untarred
# "swig-<x.x>" build directory.  Then run "make" and "make check"
# (with the same $PATH set during configuration); if successful,
# install with "sudo make install" as an administrator.  [You may
# need to use "sudo" for all steps on some OS versions.]
#
# Note the choice of /opt below as the location of dependencies.
#
# Kevin Grant (kmg@mac.com)
# July 29, 2006

# IMPORTANT: MacTerm's build system assumes /opt/swig
# regardless of what you may customize here.  See the
# Xcode configuration files for a SWIG_PREFIX variable.
swigversion=3.0.5
prefix_with_ver=/opt/swig-${swigversion}
prefix_no_ver=/opt/swig

# set up a version-free alias for the specific version directory;
# once SWIG is built (post-configuration) this link will be valid
ln -snf ${prefix_with_ver} ${prefix_no_ver}

# enable only Python
./configure \
--prefix=${prefix_with_ver} \
--with-boost=no \
--without-pcre \
--disable-ccache \
--with-python=/System/Library/Frameworks/Python.framework/Versions/2.6/bin/python \
--without-allegrocl \
--without-android \
--without-cffi \
--without-chicken \
--without-clisp \
--without-csharp \
--without-d \
--without-gcj \
--without-go \
--without-guile \
--without-java \
--without-javascript \
--without-lua \
--without-mzscheme \
--without-ocaml \
--without-octave \
--without-perl5 \
--without-php \
--without-pike \
--without-r \
--without-ruby \
--without-scilab \
--without-tcl \
--without-uffi \
;

