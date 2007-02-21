#!/bin/sh

# when double-clicked in the Finder, this will run a
# terminal (preferably, MacTelnet!) to run "make" for you

cd `dirname $0`/PythonWrapper && make

