# AppBundle.sh
#
# Not meant to be executed, read it with ".".
# Optionally consults the environment (defined
# by Xcode) and figures out where the "relevant"
# built application bundle is.
#
# On output, defines several useful variables:
#	$project_dir	sub-project root
#	$build_dir	debug/opt dependent on build
#	$dot_app_dir	path to <whatever>.app
#
# IMPORTANT: Define "top_dir" to be the root of
# the MacTelnet project.  Your source script may
# be in a different location than this file, so
# this directory is not derived.
#
# You may also defined "target_project" and
# "target_bundle" to locate another bundle (like
# "PrefsConverter" and "PrefsConverter").  The
# default is "Application" and "MacTelnet" (i.e.
# MacTelnet.app).
#
# Kevin Grant (kevin@ieee.org)
# August 29, 2006

if [ "x${top_dir}" = "x" ] ; then
    echo "$0: script included without top_dir variable being defined first" > /dev/stderr
    exit 1
fi

if [ "x${target_project}" = "x" ] ; then
    target_project=Application
fi

if [ "x${target_bundle}" = "x" ] ; then
    target_bundle=MacTelnet
fi

project_dir=${top_dir}/${target_project}
if [ "x${TARGET_BUILD_DIR}" = "x" ] ; then
    build_dir=${project_dir}/ForTigerDebug
    echo "$0: no build in progress, using default build area: $build_dir" > /dev/stderr
else
    build_dir=${TARGET_BUILD_DIR}
fi
dot_app_dir=${build_dir}/${target_bundle}.app

