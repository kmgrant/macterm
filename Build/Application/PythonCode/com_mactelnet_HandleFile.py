#!/usr/bin/python
"""Routines to open MacTelnet sessions using file pathnames.
"""
__author__ = 'Kevin Grant <kevin@ieee.org>'
__date__ = '1 January 2008'
__version__ = '3.1.0'

try:
	import Quills
except ImportError, err:
	import sys, os
	print >>sys.stderr, "Unable to import Quills."
	if "DYLD_LIBRARY_PATH" in os.environ:
		print >>sys.stderr, "Shared library path:", os.environ["DYLD_LIBRARY_PATH"]
	print >>sys.stderr, "Python path:", sys.path
	raise err

def script(pathname):
	"""script(pathname) -> None
	
	Asynchronously open a session from the given script file, by
	running the script!
	
	"""
	try:
		args = [pathname];
		session = Quills.Session(args)
	except:
		# exceptions are not currently handled at higher levels, so they
		# must be blocked here to prevent an application crash
		# TEMPORARY
		pass

def _test():
	import doctest, com_mactelnet_HandleFile
	return doctest.testmod(com_mactelnet_HandleFile)
