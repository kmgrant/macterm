#!/bin/sh

# when double-clicked in the Finder, this will run a
# terminal (preferably, MacTerm!) to run "make" for you

PATH=/Developer/usr/bin:$PATH
export PATH

cd `dirname $0`/ReleaseNotes && make

