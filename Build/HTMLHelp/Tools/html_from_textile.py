#!/usr/bin/env python

# html_from_textile.py
#
# Converts any given Textile input file into HTML.
#
# Kevin Grant (kevin@ieee.org)
# March 7, 2005

import os.path
import sys

dir = '.'
try: raise RuntimeError
except:
    x = sys.exc_info()[2]
    prog = x.tb_frame.f_code.co_filename
    dir = os.path.dirname(prog)

sys.path.append(os.path.join(dir, 'textile-2.0.10'))
import textile

if len(sys.argv) < 2: raise 'not enough arguments'

file = open(sys.argv[1], 'r')
if not file: raise 'unable to open', file

contents = file.read()
if not contents: raise 'unable to read', file

html = textile.textile(contents)
print html

