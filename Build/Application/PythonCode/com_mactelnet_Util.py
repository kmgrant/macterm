#!/usr/bin/python
# vim: set fileencoding=UTF-8 :
"""Utility routines that are highly generic.

slash_free_path -- remove all leading and trailing slashes
sort_dict -- for testing, return deterministic "dict" description
"""
__author__ = 'Kevin Grant <kevin@ieee.org>'
__date__ = '30 December 2006'
__version__ = '3.1.0'

def slash_free_path(path):
	"""slash_free_path(path) -> string
	
	Return a string without any leading and trailing slashes.
	Other slashes are preserved.
	
	(Below are REAL testcases run by doctest!)
	
	>>> slash_free_path('foo')
	'foo'
	
	>>> slash_free_path('/this/')
	'this'
	
	>>> slash_free_path('////wow///')
	'wow'
	
	>>> slash_free_path('/lead')
	'lead'
	
	>>> slash_free_path('follow/')
	'follow'
	
	>>> slash_free_path('/one/two/three/')
	'one/two/three'
	
	"""
	loop_guard = 0
	while path.startswith('/') and loop_guard < 100:
		path = path.lstrip('/')
		loop_guard += 1
	loop_guard = 0
	while path.endswith('/') and loop_guard < 100:
		path = path.rstrip('/')
		loop_guard += 1
	return path

def sort_dict(keys_values):
	"""sort_dict(dict) -> string
	
	Return a string showing the contents of a dictionary in a
	deterministic order.  Useful for testing when the result
	is a known dictionary.
	
	>>> x = dict()
	>>> x['foo'] = 1
	>>> x['bar'] = 'junk'
	>>> x['baz'] = 4.5
	>>> sort_dict(x)
	'bar:junk baz:4.5 foo:1'
	
	>>> y = dict()
	>>> y[1] = 2
	>>> y[2] = 3
	>>> y[3] = 4
	>>> sort_dict(y)
	'1:2 2:3 3:4'
	
	"""
	result = ""
	parts = ["%s:%s" % (k, keys_values[k]) for k in keys_values]
	parts.sort()
	result = " ".join(parts)
	return result

def _test():
	import doctest, com_mactelnet_Util
	return doctest.testmod(com_mactelnet_Util)

if __name__ == '__main__':
	_test()
