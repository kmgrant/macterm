#!/bin/sh

# CreateASDictionaryRezInput.sh
#
# Runs sdp on the MacTelnet .sdef file that actually defines the
# AppleScript dictionary, and writes output to a Rez input file.
#
# The input and output are set from Xcode to allow certain
# variables to be used, but basically will resolve to be
# ../Build/Application/Resources/English.lproj/Dictionary.sdef
# and ../Build/Resources/English.lproj/\
# AutomaticallyGeneratedDictionary.r.
#
# Kevin Grant (kevin@ieee.org)
# March 26, 2004

inpf=$1
outf=$2

date "+Started generating AppleScript dictionary Rez input at %T."
date "+Input will come from ${inpf}."
date "+Output will go to ${outf}."

# start with a warning comment, and use "sdp" to generate Rez input
# for an 'aete' resource
echo "// Automatically generated, DO NOT EDIT!!!" >| "${outf}" && \
echo "// Automatically generated, DO NOT EDIT!!!" >> "${outf}" && \
echo "// Automatically generated, DO NOT EDIT!!!" >> "${outf}" && \
echo "// (Source file: ${inpf})" >> "${outf}" && \
echo >> "${outf}" && \
# (use /Developer/Tools/sdp on Mac OS X 10.3 or earlier)
/usr/bin/sdp -f a -o - < "${inpf}" >> "${outf}" && \
date "+Finished generating AppleScript dictionary Rez input at %T."

