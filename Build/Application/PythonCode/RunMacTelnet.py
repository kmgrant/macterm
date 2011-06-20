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
    # Define default symbols for ALL possible user customizations.  This
    # may also be a useful reference for customizers, because it shows a
    # valid (if uninteresting) implementation of each function, including
    # any parameters or return values.
    def app_will_finish():
        Base.all_done()
    def initial_workspace():
        return ""
    # User-Defined Customizations Module "customize_mactelnet"
    #
    # If your default Python path includes this module name, then the module
    # will be imported automatically!  And, any functions it defines that match
    # a known protocol will be called.  This allows "cleaner" customization,
    # because you can affect the behavior of this startup code without actually
    # editing the file (MacTelnet upgrades will also be smoother).
    #
    # Although the path usually contains many directories, the following choices
    # will probably work best:
    #     Mac OS X 10.5.x and 10.6.x
    #         /Library/Python/2.5/site-packages/customize_mactelnet.py
    #     Mac OS X 10.3.9 and 10.4.x
    #         /Library/Python/2.3/site-packages/customize_mactelnet.py
    # NOTE: The path includes the interpreter version, which changes across OS
    #       upgrades.  It is also possible that the new Python version won't
    #       like your code.  Expect to relocate and possibly update your code
    #       after each major OS upgrade.
    #
    # USER CUSTOMIZATIONS PROTOCOL REFERENCE
    #
    # Your customizations may assume that "quills" is properly imported.
    #
    # The following user customizations are defined.  If you do not want to use
    # something, just leave it out; but what you do define must EXACTLY match
    # what is expected in this file, or MacTelnet may have problems starting up!
    # (If you see a failure, run the Console application to help debug.)
    #
    # --------------------------------------------------------------------------
    # app_will_finish()
    #
    # Since running the main event loop prevents Python from ever returning to
    # this script, anything that should happen "after" must be in a callback
    # that is automatically executed.
    #
    # One way to think of it is as if the program has two "halves"; the first
    # half is up to the end of the event loop; the second half is everything
    # afterward, and the second half must be in this function.
    #
    # The Base.all_done() function is automatically called after your function
    # returns.
    #
    # EXAMPLE
    #     def app_will_finish():
    #         print "this is the 2nd half of my script"
    #
    # --------------------------------------------------------------------------
    # initial_workspace() -> string
    #
    # Return the UTF-8-encoded name to pass as the "initial_workspace" keyword
    # parameter for Base.all_init().  Any Prefs.WORKSPACE collection name will
    # override the Default and cause a different set of windows to be spawned.
    # If not defined or not found, the Default is used anyway.
    #
    # If you just want to set this to a static value, you're advised to use the
    # Preferences window, and just edit the Default to do what you want.  The
    # only benefit to setting it here is when you want to do something dynamic;
    # for instance, you could check to see if the computer's host name indicates
    # a VPN connection, and spawn different startup sessions in that case.
    #
    # EXAMPLE
    #     def initial_workspace():
    #         ws = ""
    #         from socket import gethostname
    #         host = str(gethostname())
    #         print "MacTelnet: Current machine hostname is '%s'." % host
    #         if host.endswith("mycompany.com"):
    #             ws = "Company Servers" # or whatever you used
    #         return ws
    #
    # --------------------------------------------------------------------------
    if "MACTELNET_SKIP_CUSTOM_LIBS" in os.environ:
        print "MacTelnet: ignoring any 'customize_mactelnet' module (environment setting)"
    else:
        try:
            import customize_mactelnet
            user_syms = dir(customize_mactelnet)
            if '__file__' in user_syms:
                print "MacTelnet: imported 'customize_mactelnet' module from", customize_mactelnet.__file__ # could be a list
            else:
                print "MacTelnet: imported 'customize_mactelnet' module"
            # look for valid user overrides (EVERYTHING used here should be
            # documented above and initialized at the beginning)
            print "MacTelnet: module contains:", user_syms
            if 'app_will_finish' in user_syms:
                print "MacTelnet: employing user customization 'app_will_finish'"
                app_will_finish = customize_mactelnet.app_will_finish
            if 'initial_workspace' in user_syms:
                print "MacTelnet: employing user customization 'initial_workspace'"
                initial_workspace = customize_mactelnet.initial_workspace
        except ImportError, err:
            # assume the module was never created, and ignore (nothing is
            # printed as an error in this case because the overwhelming
            # majority of users are not expected to ever create this module)
            pass
    
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
    Base.all_init(initial_workspace=initial_workspace())
    
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
    
    # if the current version is very old, warn the user
    try:
        parts = str(Base.version()).split(".")
        if len(parts) > 4:
            build = parts[4] # formatted as 8 digits and YYYYMMDD, e.g. 20110610
            latest_yyyymmdd = now.strftime("%Y%m%d") # also 8 digits and YYYYMMDD
            if (len(latest_yyyymmdd) == 8) and (len(build) == 8):
                year_now = int(latest_yyyymmdd[0:4])
                year_build = int(build[0:4])
                old = False
                # arbitrary; releases more than this many months old
                # are considered out-of-date
                months_for_old = 2
                if 1 == (year_now - year_build):
                    # when the years are different but it's just the
                    # previous year, the releases might still be close
                    # (e.g. December versus January); so check months,
                    # subtracting 12 from the older one so that it is
                    # always considered less
                    month_now = int(latest_yyyymmdd[4:6])
                    month_build = int(build[4:6]) - 12
                    diff = int(month_now) - int(month_build)
                    old = (diff > months_for_old)
                elif (year_now - year_build) > 1:
                    # over a year, definitely old
                    old = True
                else:
                    # builds are same year, do a diff of months (and
                    # because of the way the numbers are, it's enough
                    # to subtract the whole thing without breaking it
                    # up into parts)
                    diff = int(latest_yyyymmdd) - int(build)
                    old = (diff > months_for_old)
                if old:
                    Base._version_warning()
    except Exception, e:
        print "exception in version check", e
    
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
        app_will_finish()
        Base.all_done()
    
    Events.on_endloop_call(terminate)
    
    # start user interaction (WARNING: will not return, but calls the endloop callback when finished)
    Events.run_loop()
    # IMPORTANT: anything beyond this point will NEVER RUN; use callbacks instead
