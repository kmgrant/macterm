#!/usr/bin/python
# vim: set fileencoding=UTF-8 :
"""Routines to open MacTelnet sessions using file pathnames.
"""
__author__ = 'Kevin Grant <kevin@ieee.org>'
__date__ = '1 January 2008'
__version__ = '4.0.0'

# note: Quills is a compiled module, library path must be set properly
import Quills

def script(pathname):
    """script(pathname) -> None
    
    Asynchronously open a session from the given script file, by
    running the script!  Raise an exception on failure.
    
    """
    args = [pathname]
    session = Quills.Session(args)

def _test():
    import doctest, pymactelnet.file.open
    return doctest.testmod(pymactelnet.file.open)
