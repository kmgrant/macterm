#!/bin/sh

# SwigConfig.sh
#
# Allows you to build SWIG for MacTerm's use.
# (Download SWIG from "http://www.swig.org/".)
#
# Run this instead of "./configure" from your untarred
# "swig-<x.x>" build directory.  Then run "make",
# "make check" and "make install" to install SWIG.
# (Mac OS X comes with an older version of SWIG in
# /usr/bin, but that version has not been tested;
# maybe it works, maybe not.)
#
# Note the choice of /opt/pcre below as the location
# of PCRE; you could probably use "--without-pcre" if
# you don't have it installed, because MacTerm does
# not currently rely on PCRE support in SWIG.
#
# Kevin Grant (kmg@mac.com)
# July 29, 2006

# IMPORTANT: MacTerm's build system assumes /opt/swig
# regardless of what you may customize here.  See the
# Xcode configuration files for a SWIG_PREFIX variable.
#
# SWIG 1.3.29 or later works with Tiger (10.4).
# SWIG 1.3.30 or later is required for Leopard (10.5).
swigversion=2.0.4
prefix_with_ver=/opt/swig-${swigversion}
prefix_no_ver=/opt/swig

# set up a version-free alias for the specific version directory;
# once SWIG is built (post-configuration) this link will be valid
ln -snf ${prefix_with_ver} ${prefix_no_ver}

# enable only Python
./configure \
--prefix=${prefix_with_ver} \
--with-pcre-prefix=/opt/pcre \
--disable-ccache \
--with-python \
--without-allegrocl \
--without-chicken \
--without-clisp \
--without-csharp \
--without-gcj \
--without-go \
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

