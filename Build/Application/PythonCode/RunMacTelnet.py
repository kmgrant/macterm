#!/usr/bin/python
# vim: set fileencoding=UTF-8 :
"""This is where the bulk of MacTelnet is initialized, but it's best to run it
indirectly by using the "./MacTelnet" script (which sets all the library paths).

This Python code relies on compiled C++ routines from MacTelnet, which are in
the module "quills".  Most Python code is in the "pymactelnet" module, and any
of it may require "quills" in order to work.

When the MacTelnet GUI runs, the event loop prevents normal flow of Python code.
So, "quills" relies heavily on callbacks: functions can be defined ahead of time
in Python, registered according to their purpose, and used only when they're
needed.

This exposure to Python simplifies debugging, and gives you extensibility and
configuration options that most applications lack!  This file shows just a few
examples of what you can do...look for more information at MacTelnet.com.
"""
__author__ = 'Kevin Grant <kmg@mac.com>'
__date__ = '24 August 2006'
__version__ = '4.0.0'

import os
import string
import datetime

try:
    from quills import Base, Events, Prefs, Session, Terminal
except ImportError, err:
    import sys
    print >>sys.stderr, "Unable to import Quills."
    if "DYLD_LIBRARY_PATH" in os.environ:
        print >>sys.stderr, "Shared library path:", os.environ["DYLD_LIBRARY_PATH"]
    print >>sys.stderr, "Python path:", sys.path
    raise err
import pymactelnet.file.open
import pymactelnet.term.text
import pymactelnet.url.open

if __name__ == "__main__":
    # if you intend to use your own GUI elements with "wx", you need to
    # import and construct the application object at this point (that is,
    # before Quills is initialized); this allows your callbacks to pop up
    # simple interfaces, such as wx.MessageDialog(), within MacTelnet!
    #import wx
    #app = wx.PySimpleApp()
    
    # below are examples of things you might want to enable for debugging...
    
    # memory allocation changes - see "man malloc" for more options here
    #os.environ['MallocGuardEdges'] = '1'
    #os.environ['MallocScribble'] = '1'
    
    # debugging of Carbon Events
    #os.environ['EventDebug'] = '1'
    
    # preamble
    now = datetime.datetime.now()
    print "MacTelnet: %s" % now.strftime("%A, %B %d, %Y, %I:%M %p")
    
    # load all required MacTelnet modules
    Base.all_init()
    #Base.all_init(os.environ['INITIAL_APP_BUNDLE_DIR']) # this variant is no longer needed
    
    # undo environment settings made by the "MacTelnet" script, so as not
    # to pollute the user environment too much
    for removed_var in (
        # this list should basically correspond to any uses of
        # "os.environ" in the "MacTelnet" front-end script
        'DYLD_FRAMEWORK_PATH',
        'DYLD_LIBRARY_PATH',
        'INITIAL_APP_BUNDLE_DIR',
        'PYTHONEXECUTABLE',
        'PYTHONPATH',
    ):
        if removed_var in os.environ: del os.environ[removed_var]
    
    # banner
    print "MacTelnet: Base initialization complete.  This is MacTelnet version %s." % Base.version()
    
    # optionally invoke some unit tests
    do_testing = ("MACTELNET_RUN_TESTS" in os.environ)
    if do_testing:
        import pymactelnet
        pymactelnet.run_all_tests()
    
    # register MacTelnet features that are actually implemented in Python!
    Session.on_urlopen_call(pymactelnet.url.open.file, 'file')
    Session.on_urlopen_call(pymactelnet.url.open.ftp, 'ftp')
    Session.on_urlopen_call(pymactelnet.url.open.sftp, 'sftp')
    Session.on_urlopen_call(pymactelnet.url.open.ssh, 'ssh')
    Session.on_urlopen_call(pymactelnet.url.open.telnet, 'telnet')
    Session.on_urlopen_call(pymactelnet.url.open.x_man_page, 'x-man-page')
    Session.on_fileopen_call(pymactelnet.file.open.script, 'bash')
    Session.on_fileopen_call(pymactelnet.file.open.script, 'command')
    Session.on_fileopen_call(pymactelnet.file.open.script, 'csh')
    Session.on_fileopen_call(pymactelnet.file.open.script, 'pl')
    Session.on_fileopen_call(pymactelnet.file.open.script, 'py')
    Session.on_fileopen_call(pymactelnet.file.open.script, 'sh')
    Session.on_fileopen_call(pymactelnet.file.open.script, 'tcl')
    Session.on_fileopen_call(pymactelnet.file.open.script, 'tcsh')
    Session.on_fileopen_call(pymactelnet.file.open.script, 'tool')
    Session.on_fileopen_call(pymactelnet.file.open.script, 'zsh')
    Session.on_fileopen_call(pymactelnet.file.open.macros, 'macros')
    
    # if desired, override what string is sent after keep-alive timers expire
    #Session.set_keep_alive_transmission(".")
    
    try:
        Terminal.on_seekword_call(pymactelnet.term.text.find_word)
    except Exception, e:
        print "warning, exception while trying to register word finder for double clicks:", e
    
    for i in range(0, 256):
        try:
            Terminal.set_dumb_string_for_char(i, pymactelnet.term.text.get_dumb_rendering(i))
        except Exception, e:
            print "warning, exception while setting character code %i:" % i, e
            pass
    
    # banner
    print "MacTelnet: Full initialization complete."
    
    # (if desired, insert code at this point to interact with MacTelnet)
    
    # if you want to find out when new sessions are created, you can
    # define a Python function and simply register it...
    #def my_py_func():
    #    print "\n\nI was called by MacTelnet!\n\n"
    #Session.on_new_call(my_py_func)
    
    # the following line would run a command every time you start up...
    #session_1 = Session("progname -arg1 -arg2 -arg3 val3 -arg4 val4".split())
    
    # the following would define some custom macros immediately...
    #my_set = Prefs(Prefs.MACRO_SET)
    #my_set.define_macro(1, name="my first macro!", contents="some text")
    #my_set.define_macro(2, name="my second macro!", contents="yet another macro")
    #my_set.define_macro(3, name="my third macro!", contents="this is different")
    #Prefs.set_current_macros(my_set)
    
    def terminate():
        # unload all required MacTelnet modules, performing necessary cleanup
        Base.all_done()
    
    Events.on_endloop_call(terminate)
    
    # start user interaction (WARNING: will not return, but calls the endloop callback when finished)
    Events.run_loop()
    # IMPORTANT: anything beyond this point will NEVER RUN; use callbacks instead
