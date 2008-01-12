#!/usr/bin/python
"""Main entry point to MacTelnet.

The set of routines available from Python is known as the Quills
API and is declared in the MacTelnet headers of the same name
(like "QuillsBase.h").

Exposure to Python simplifies debugging, and gives you options
for extensibility and configuration that most applications lack!
This file shows just a few examples of what you can do...look for
more information on MacTelnet.com.
"""
__author__ = 'Kevin Grant <kevin@ieee.org>'
__date__ = '24 August 2006'
__version__ = '3.1.0'

import os

import com_mactelnet_HandleFile as HandleFile
import com_mactelnet_HandleURL as HandleURL
try:
	from Quills import Base, Events, Session
except ImportError, err:
	import sys, os
	print >>sys.stderr, "Unable to import Quills."
	if "DYLD_LIBRARY_PATH" in os.environ:
		print >>sys.stderr, "Shared library path:", os.environ["DYLD_LIBRARY_PATH"]
	print >>sys.stderr, "Python path:", sys.path
	raise err

# below are examples of things you might want to enable for debugging...

# memory allocation changes - see "man malloc" for more options here
#os.environ['MallocGuardEdges'] = '1'
#os.environ['MallocScribble'] = '1'

# debugging of Carbon Events
#os.environ['EventDebug'] = '1'

# if you want to read documentation on the API, try one of these (and
# run MacTelnet from a shell so you can see the documentation)...
#os.system("pydoc Quills.Base")
#os.system("pydoc Quills.Events")
#os.system("pydoc Quills.Prefs")
#os.system("pydoc Quills.Session")



# load all required MacTelnet modules
Base.all_init(os.environ['INITIAL_APP_BUNDLE_DIR'])

# banner
print "MacTelnet: Base initialization complete.  This is MacTelnet version %s." % Base.version()

# If necessary, invoke testcases (each has its own block so that subsets
# can be run more easily).  A _test() call will generate output only for
# problems; therefore, this runner prints its own banner for success.
do_testing = False
def run_module_tests( mod ):
	(failures, test_count) = mod._test()
	if not failures: print "MacTelnet: %s module: SUCCESSFUL unit test (total tests: %d)" % (mod.__name__, test_count)
if do_testing:
	import com_mactelnet_Util
	run_module_tests(com_mactelnet_Util)
if do_testing:
	import com_mactelnet_ParseURL
	run_module_tests(com_mactelnet_ParseURL)
if do_testing:
	import com_mactelnet_HandleURL
	run_module_tests(com_mactelnet_HandleURL)

# register MacTelnet features that are actually implemented in Python!
Session.on_urlopen_call(HandleURL.file, 'file')
Session.on_urlopen_call(HandleURL.ftp, 'ftp')
Session.on_urlopen_call(HandleURL.sftp, 'sftp')
Session.on_urlopen_call(HandleURL.ssh, 'ssh')
Session.on_urlopen_call(HandleURL.telnet, 'telnet')
Session.on_urlopen_call(HandleURL.x_man_page, 'x-man-page')
Session.on_fileopen_call(HandleFile.script, 'bash')
Session.on_fileopen_call(HandleFile.script, 'command')
Session.on_fileopen_call(HandleFile.script, 'csh')
Session.on_fileopen_call(HandleFile.script, 'pl')
Session.on_fileopen_call(HandleFile.script, 'py')
Session.on_fileopen_call(HandleFile.script, 'sh')
Session.on_fileopen_call(HandleFile.script, 'tcl')
Session.on_fileopen_call(HandleFile.script, 'tcsh')
Session.on_fileopen_call(HandleFile.script, 'tool')
Session.on_fileopen_call(HandleFile.script, 'zsh')

# banner
print "MacTelnet: Full initialization complete."

# (if desired, insert code at this point to interact with MacTelnet)

# if you want to find out when new sessions are created, you can
# define a Python function and simply register it...
#def my_py_func():
#	print "\n\nI was called by MacTelnet!\n\n"
#Session.on_new_call(my_py_func)

# the following line would run a command every time you start up...
#session_1 = Session("progname -arg1 -arg2 -arg3 val3 -arg4 val4".split())

# start user interaction (WARNING: will not return until quitting time!)
Events.run_loop()

# unload all required MacTelnet modules, performing necessary cleanup
Base.all_done()

