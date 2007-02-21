# AutoDatePropertyList.sh
#
# Performs substitution on special macros %LIKE_THIS% in
# various template .plist files.  Version numbers are
# inserted everywhere they should be, along with a dated
# build number.
#
# The Application Xcode project uses this to set up all
# property lists prior to including them in bundles.
#
# Kevin Grant (kevin@ieee.org)
# August 29, 2006

# most variables depended upon below should be set by Xcode
editable_plist="${BUILT_PRODUCTS_DIR}/${INFOPLIST_PATH}"

# set build number in the .plist automatically, based on current date;
# Xcode automatically substitutes $(VAR) references (see Target Inspector)
# but %MY_BUILD_NUMBER% is a special token that Xcode won't recognize
# and leave as-is so this script can substitute its own value accordingly

build=`date +%Y%m%d`

target="${editable_plist}"

if [ ! -r "${target}" ] ; then
	echo "unable to read ${target}" > /dev/stderr
	exit 1
fi

# substitute all version numbers
perl -pi -e "s|\%MY_MAJOR_NUMBER\%|${MY_MAJOR_NUMBER}|g" "${target}"
perl -pi -e "s|\%MY_MINOR_NUMBER\%|${MY_MINOR_NUMBER}|g" "${target}"
perl -pi -e "s|\%MY_SUBMINOR_NUMBER\%|${MY_SUBMINOR_NUMBER}|g" "${target}"
perl -pi -e "s|\%MY_ALPHA_BETA\%|${MY_ALPHA_BETA}|g" "${target}"

# ideal case: replace variable reference
perl -pi -e "s|\%MY_BUILD_NUMBER\%|${build}|g" "${target}"

# hack case: if Xcode doesn't realize it should run this script,
# a search for instances of 8 continuous digits will do the trick!
perl -pi -e "s|\d\d\d\d\d\d\d\d|${build}|g" "${target}"

echo "Automatically set build number to ${build} in ${target}, with results of:"
cat "${target}" | grep "${build}"
exit 0
