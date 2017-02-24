#!/usr/bin/python
# vim: set fileencoding=UTF-8 :

"""Utility routines that are highly generic.

command_data -- run a program and return its standard output
mac_os_name -- return a name like Panther, Tiger, Leopard, etc.
slash_free_path -- remove all leading and trailing slashes
sort_dict -- for testing, return deterministic "dict" description

"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

__author__ = 'Kevin Grant <kmg@mac.com>'
__date__ = '30 December 2006'
__version__ = '4.0.0'

def command_data(cmdline_tuple, allow_nonzero_exit=False):
    """command_data(tuple) -> string

    Run a program and return its standard output as a
    string without any editing.  If there is ANY problem
    running the program, the result will be None unless
    the keyword argument "allow_nonzero_exit" is True.

    >>> command_data(('/this/does/not/exist'))

    >>> command_data(('/bin/echo', 'hello, world!'))
    'hello, world!\\n'

    >>> command_data(('/usr/bin/perl', '-e', 'print "hello\\n"; exit 1'),
    ...              allow_nonzero_exit=False)

    >>> command_data(('/usr/bin/perl', '-e', 'print "hello\\n"; exit 1'),
    ...              allow_nonzero_exit=True)
    'hello\\n'

    """
    result = None
    import subprocess
    try:
        cmd_run = subprocess.Popen(cmdline_tuple, stdin=None,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, close_fds=True)
        (cmd_stdout, cmd_stderr) = cmd_run.communicate()
        if (cmd_run.returncode == 0) or allow_nonzero_exit:
            result = str(cmd_stdout)
    except Exception:
        #print("exception in subprocess.Popen of %r:" % cmdline_tuple, _)
        pass
    return result

def mac_os_name():
    """mac_os_name() -> string

    Return a common name like "Leopard", "Tiger" or "Panther" for
    the current system, using heuristics to figure out what the
    system is.  Return "UNKNOWN" for anything else.

    Unfortunately it is quite difficult to determine this in a
    standard way, because (for instance) platform.mac_version()
    fails with the Panther version of Python.

    >>> mac_os_name() in ['Sierra', 'El Capitán', 'Yosemite',
    ...                   'Mavericks', 'Mountain Lion', 'Lion',
    ...                   'Snow Leopard', 'Leopard', 'Tiger', 'Panther']
    True

    """
    result = "UNKNOWN"
    import os
    uname_tuple = os.uname()
    darwin_version = str(uname_tuple[2])
    if darwin_version.startswith('7.'):
        result = "Panther"
    elif darwin_version.startswith('8.'):
        result = "Tiger"
    elif darwin_version.startswith('9.'):
        result = "Leopard"
    elif darwin_version.startswith('10.'):
        result = "Snow Leopard"
    elif darwin_version.startswith('11.'):
        result = "Lion"
    elif darwin_version.startswith('12.'):
        result = "Mountain Lion"
    elif darwin_version.startswith('13.'):
        result = "Mavericks"
    elif darwin_version.startswith('14.'):
        result = "Yosemite"
    elif darwin_version.startswith('15.'):
        result = "El Capitán"
    elif darwin_version.startswith('16.'):
        result = "Sierra"
    return result

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
    """Runs all of this module's "doctest" test cases.
    """
    import doctest
    from . import utilities
    return doctest.testmod(utilities)

if __name__ == '__main__':
    _test()
