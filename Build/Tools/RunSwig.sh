#!/bin/sh

# RunSwig.sh
#
# Runs the SWIG program to construct source code out of a SWIG
# interface file (ultimately for Python).
#
# This should be invoked from Xcode, or the environment should
# otherwise provide what Xcode does for build rules: in
# particular, variables like ${INPUT_FILE_BASE}.  For more
# information, see "Xcode Build System Guide: Build Rules".
#
# This script exists to act as an interface to Xcode build rules,
# and to correct some stupid things that SWIG does.
#
# Kevin Grant (kevin@ieee.org)
# December 13, 2007

cat=/bin/cat
mv=/bin/mv
printf=/usr/bin/printf
rm=/bin/rm
swig=${SWIG_PREFIX}/bin/swig
if [ "x${SWIG}" != "x" ] ; then
    swig=${SWIG}
fi
touch=/usr/bin/touch

input_swig_i=${INPUT_FILE_NAME}
input_swig_i_path=${INPUT_FILE_DIR}/${INPUT_FILE_NAME}
generated_cxx=${INPUT_FILE_BASE}_wrap.cxx
generated_py=${INPUT_FILE_BASE}.py
cxx_tmp=/tmp/${INPUT_FILE_BASE}_wrap.cxx.NEW.$$

if [ "x${DERIVED_FILES_DIR}" = "x" ] ; then
    echo "$0: run this only from Xcode" >&2
    exit 1
fi

# change to the target directory, because SWIG annoyingly
# generates the *.cxx file in the current directory even if
# options like -outdir are given to redirect other files
cd "${DERIVED_FILES_DIR}"
if [ "x$?" != "x0" ] ; then
    echo "$0: unable to change to ${DERIVED_FILES_DIR}" >&2
    exit 1
fi

# Xcode does not seem to know when NOT to call this script
if [ "${input_swig_i_path}" -ot "${generated_cxx}" ] ; then
    if [ "${input_swig_i_path}" -ot "${generated_py}" ] ; then
        echo "$0: '${input_swig_i}' has not changed since its derived files were last built" >&2
        exit 0
    fi
fi

# throw away any previous files
$rm -f \
${generated_cxx} \
${cxx_tmp} \
;

# construct the Python code and C++ wrapper code
${swig} ${1+"$@"}
if [ "x$?" != "x0" ] ; then
    echo "$0: SWIG failed" >&2
    exit 1
fi

# now patch SWIG output, to fix certain problems
$rm -f ${cxx_tmp}
$touch ${cxx_tmp}
if [ "x$?" != "x0" ] ; then
    echo "$0: unable to create temporary file" >&2
    exit 1
fi
if [ "x${SWIG_VERSION}" = "x1.3.29" ] ; then
    # certain includes are missing in output from this version
    echo "#include <string.h>" >> ${cxx_tmp}
    echo "#include <math.h>" >> ${cxx_tmp}
fi
# Apple's AvailabilityMacros.h file defines a stupidly-named
# macro, check(), that clobbers a method of the same name in
# output generated by SWIG; so, throw this macro away
$printf "#ifdef check\n#undef check\n#endif\n" >> ${cxx_tmp}
$cat ${generated_cxx} >> ${cxx_tmp}
if [ "x$?" != "x0" ] ; then
    echo "$0: no generated C++ file found" >&2
    exit 1
fi
$mv -f ${cxx_tmp} ${generated_cxx}

