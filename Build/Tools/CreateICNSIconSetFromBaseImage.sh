#!/bin/bash

# Converts an input image into copies at several different sizes,
# creates a temporary ".iconset" and then uses that to generate a
# final ".icns" file with all resolutions supported.  The input
# image should be very highly detailed, like 1024+ pixel sides.
#
# Note that application icon artwork is maintained in the file
# "Build/Application/Artwork/AppIcon.pxm", and Pixelmator can be
# used to export that artwork to a source PNG.  Since Pixelmator
# supports AppleScript, the process can be further automated by
# inserting an appropriate "osascript" command to do the export.
#
# Kevin Grant (kmg@mac.com)
# October 8, 2017

set -e
set -o errexit

filename=$1
if [ "x$filename" = "x" ] ; then
  filename="AppLogo2048×2048.png"
  #echo "$0: expected base image filename" >&2
  #exit 1
fi
outicns=$2
if [ "x$outicns" = "x" ] ; then
  outicns=./IconForBundle.icns
  #echo "$0: expected output icon filename" >&2
  #exit 1
fi

outdir=/tmp/AppIcon.iconset

set -u

rm -Rf ${outdir}
mkdir -p ${outdir}

# copy the file to produce all required ".iconset" variants of the image
echo "$0: copy images" >&2
for newname in \
icon_16x16.png \
icon_16x16@2x.png \
icon_32x32.png \
icon_32x32@2x.png \
icon_128x128.png \
icon_128x128@2x.png \
icon_256x256.png \
icon_256x256@2x.png \
icon_512x512.png \
icon_512x512@2x.png \
; do
  cp ${filename} ${outdir}/${newname}
  ls -l ${outdir}/${newname}
done

echo "$0: scale images" >&2
sips --resampleHeightWidth 16 16 ${outdir}/icon_16x16.png >/dev/null
sips --resampleHeightWidth 32 32 ${outdir}/icon_16x16@2x.png >/dev/null
sips --resampleHeightWidth 32 32 ${outdir}/icon_32x32.png >/dev/null
sips --resampleHeightWidth 64 64 ${outdir}/icon_32x32@2x.png >/dev/null
sips --resampleHeightWidth 128 128 ${outdir}/icon_128x128.png >/dev/null
sips --resampleHeightWidth 256 256 ${outdir}/icon_128x128@2x.png >/dev/null
sips --resampleHeightWidth 256 256 ${outdir}/icon_256x256.png >/dev/null
sips --resampleHeightWidth 512 512 ${outdir}/icon_256x256@2x.png >/dev/null
sips --resampleHeightWidth 512 512 ${outdir}/icon_512x512.png >/dev/null
sips --resampleHeightWidth 1024 1024 ${outdir}/icon_512x512@2x.png >/dev/null

rm -f ${outicns}
echo "$0: create '.icns'" >&2
iconset=${outdir}
iconutil --convert icns --output ${outicns} ${iconset}
ls -l ${outicns}
