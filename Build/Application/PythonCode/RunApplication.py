#!/usr/bin/python
# vim: set fileencoding=UTF-8 :

"""This is where the bulk of MacTerm is initialized, but it's best to run it
indirectly by using the "./MacTerm" script (which sets all the library paths).

This Python code relies on compiled C++ routines from MacTerm, which are in
the module "quills".  Most Python code is in the "pymacterm" module, and any of
it may require "quills" in order to work.

When the MacTerm GUI runs, the event loop prevents normal flow of Python code.
So, "quills" relies heavily on callbacks: functions can be defined ahead of time
in Python, registered according to their purpose, and used only when they're
needed.

This exposure to Python simplifies debugging, and gives you extensibility and
configuration options that most applications lack!  This file shows just a few
examples of what you can do...look for more information at MacTerm.net.
"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

__author__ = 'Kevin Grant <kmg@mac.com>'
__date__ = '24 August 2006'
__version__ = '4.0.0'

def warn(*args, **kwargs):
    """Shorthand for printing to sys.stderr.
    """
    print(*args, file=sys.stderr, **kwargs)

if __name__ == "__main__":
    import os
    import string
    import datetime

    try:
        from quills import Base, Events, Prefs, Session, Terminal
    except ImportError as err:
        import sys
        warn("Unable to import Quills.")
        if "DYLD_LIBRARY_PATH" in os.environ:
            warn("Shared library path:", os.environ["DYLD_LIBRARY_PATH"])
        warn("Python path:", sys.path)
        raise err
    import pymacterm.file_open
    import pymacterm.term_text
    import pymacterm.url_open
    from pymacterm.utilities import bytearray_to_str as bytearray_to_str
    from pymacterm.utilities import command_data as command_data

    # undo environment settings made by the "MacTerm" script, so as not
    # to pollute the user environment too much
    for removed_var in (
        # this list should basically correspond to any uses of
        # "os.environ" in the "MacTerm" front-end script
        'DYLD_FRAMEWORK_PATH',
        'DYLD_LIBRARY_PATH',
        'INITIAL_APP_BUNDLE_DIR',
        'PYTHONEXECUTABLE',
        'PYTHONPATH',
        'VERSIONER_PYTHON_PREFER_32_BIT',
        'VERSIONER_PYTHON_VERSION',
    ):
        if removed_var in os.environ:
            del os.environ[removed_var]

    # Define default symbols for ALL possible user customizations.  This
    # may also be a useful reference for customizers, because it shows a
    # valid (if uninteresting) implementation of each function, including
    # any parameters or return values.
    def app_will_finish():
        """Default symbol for user customization (does nothing).
        """
        pass
    def initial_workspace():
        """Default symbol for user customization (does nothing).
        """
        return ""
    # User-Defined Customizations Module "customize_macterm"
    #
    # If your default Python path includes this module name, then the module
    # will be imported automatically!  And, any functions it defines that match
    # a known protocol will be called.  This allows "cleaner" customization,
    # because you can affect the behavior of this startup code without actually
    # editing the file (MacTerm upgrades will also be smoother).
    #
    # Although the path usually contains many directories, the following choice
    # will probably work best:
    #     /Library/Python/2.x/site-packages/customize_macterm.py
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
    # what is expected in this file, or MacTerm may have problems starting up!
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
    #         print("This is the 2nd half of my script.")
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
    #         print("MacTerm: Current machine hostname is '%s'." % host)
    #         if host.endswith("mycompany.com"):
    #             ws = "Company Servers" # or whatever you used
    #         return ws
    #
    # --------------------------------------------------------------------------
    if "MACTERM_SKIP_CUSTOM_LIBS" in os.environ:
        warn("MacTerm: Ignoring any 'customize_macterm' module",
             "(environment setting).")
    else:
        try:
            # try to import using the obsolete name, and emit a console warning
            # if it exists
            import customize_mactelnet
            warn("MacTerm: Warning, 'customize_mactelnet' legacy module",
                 "will be ignored; rename it to 'customize_macterm'.")
        except ImportError as err:
            pass
        try:
            import customize_macterm
            USER_SYMS = dir(customize_macterm)
            if '__file__' in USER_SYMS:
                print("MacTerm: Imported 'customize_macterm' module from",
                      customize_macterm.__file__) # could be a list
            else:
                print("MacTerm: Imported 'customize_macterm' module.")
            # look for valid user overrides (EVERYTHING used here should be
            # documented above and initialized at the beginning)
            print("MacTerm: Module contains:", USER_SYMS)
            if 'app_will_finish' in USER_SYMS:
                print("MacTerm: Employing user customization 'app_will_finish'.")
                app_will_finish = customize_macterm.app_will_finish
            if 'initial_workspace' in USER_SYMS:
                print("MacTerm: Employing user customization",
                      "'initial_workspace'.")
                initial_workspace = customize_macterm.initial_workspace
        except ImportError as err:
            # assume the module was never created, and ignore (nothing is
            # printed as an error in this case because the overwhelming
            # majority of users are not expected to ever create this module)
            pass

    # if you intend to use your own GUI elements with "wx", you need to
    # import and construct the application object at this point (that is,
    # before Quills is initialized); this allows your callbacks to pop up
    # simple interfaces, such as wx.MessageDialog(), within MacTerm!
    #import wx
    #app = wx.PySimpleApp()

    # below are examples of things you might want to enable for debugging...

    # memory allocation changes - see "man malloc" for more options here
    #os.environ['MallocGuardEdges'] = '1'
    #os.environ['MallocScribble'] = '1'

    # debugging of Carbon Events
    #os.environ['EventDebug'] = '1'

    # preamble
    NOW_DT = datetime.datetime.now()
    print("MacTerm: %s." % NOW_DT.strftime("%A, %B %d, %Y, %I:%M %p"))

    # load all required MacTerm modules
    Base.all_init(initial_workspace=initial_workspace())

    # banner
    print("MacTerm: Base initialization complete; this is version %s." %
          Base.version())

    # optionally invoke some unit tests
    DO_TESTING = ("MACTERM_RUN_TESTS" in os.environ)
    if DO_TESTING:
        import pymacterm
        pymacterm.run_all_tests()

    # register MacTerm features that are actually implemented in Python!
    Session.on_urlopen_call(pymacterm.url_open.file, 'file')
    Session.on_urlopen_call(pymacterm.url_open.sftp, 'sftp')
    Session.on_urlopen_call(pymacterm.url_open.ssh, 'ssh')
    Session.on_urlopen_call(pymacterm.url_open.x_man_page, 'x-man-page')
    Session.on_fileopen_call(pymacterm.file_open.script, 'bash')
    Session.on_fileopen_call(pymacterm.file_open.script, 'command')
    Session.on_fileopen_call(pymacterm.file_open.script, 'csh')
    Session.on_fileopen_call(pymacterm.file_open.script, 'pl')
    Session.on_fileopen_call(pymacterm.file_open.prefs, 'plist')
    Session.on_fileopen_call(pymacterm.file_open.script, 'py')
    Session.on_fileopen_call(pymacterm.file_open.script, 'sh')
    Session.on_fileopen_call(pymacterm.file_open.script, 'tcl')
    Session.on_fileopen_call(pymacterm.file_open.script, 'tcsh')
    Session.on_fileopen_call(pymacterm.file_open.script, 'tool')
    Session.on_fileopen_call(pymacterm.file_open.prefs, 'xml')
    Session.on_fileopen_call(pymacterm.file_open.script, 'zsh')

    def pids_cwds(pids_tuple):
        """A callback, invoked by MacTerm, whenever the current working
        directories for a given list of process IDs is needed.  This responds by
        running "lsof".  Returns a map from process ID integers to directory
        path strings.
        """
        result = dict()
        if len(pids_tuple) > 0:
            # run 'lsof' to find the current working directories of given
            # processes (if you need many, give them all at once so the data can
            # be found in a single invocation; individual lookups will be much
            # slower)
            cmd = ['/usr/sbin/lsof', '-a', '-d', 'cwd', '-Fn']
            for pid in pids_tuple:
                cmd.extend(('-p', str(pid)))
            cmd = tuple(cmd)
            # allow nonzero exits; if a list of process IDs is given and ANY of
            # them doesn't return something, the others might still return
            # valid data
            all_output = command_data(cmd, allow_nonzero_exit=True)
            if all_output is not None:
                # each line of output has a single letter type prefix; the
                # first should be a line with, e.g. "p12345" (process ID);
                # the second should be the directory, e.g. "n/some/path"
                fields = all_output.split(bytearray('\n', encoding='UTF-8'))
                pid = None
                for field in fields:
                    if (len(field) > 1) and (field[0] == ord('p')):
                        pid = int(bytearray_to_str(field[1:]))
                        path = None
                    elif (len(field) > 1) and (field[0] == ord('n')):
                        path = bytearray_to_str(field[1:])
                        if (pid is not None) and (path is not None):
                            print("set", repr(pid), "to", repr(path))
                            result[pid] = path
                            pid = None
        return result
    Session._on_seekpidscwds_call(pids_cwds)

    # if desired, override what string is sent after keep-alive timers expire
    #Session.set_keep_alive_transmission(".")

    try:
        Terminal.on_seekword_call(pymacterm.term_text.find_word)
    except Exception as _:
        warn("Warning, exception while trying to register word finder for",
             "double clicks:", _)

    for i in range(0, 256):
        try:
            rendering = pymacterm.term_text.get_dumb_rendering(i)
            Terminal.set_dumb_string_for_char(i, rendering)
        except Exception as _:
            warn("Warning, exception while setting character code %i:" % i, _)

    # banner
    print("MacTerm: Full initialization complete.")

    # (if desired, insert code at this point to interact with MacTerm)

    # if you want to find out when new sessions are created, you can
    # define a Python function and simply register it...
    #def my_py_func():
    #    print("\n\nI was called by MacTerm!\n\n")
    #Session.on_new_call(my_py_func)

    # the following line would run a command every time you start up...
    #session_1 = Session("progname -arg1 -arg2 -arg3 val3 -arg4 val4".split())

    # the following would define some custom macros immediately...
    #my_set = Prefs(Prefs.MACRO_SET)
    #my_set.define_macro(1, name="my first macro!", contents="some text")
    #my_set.define_macro(2, name="my second macro!", contents="another macro")
    #my_set.define_macro(3, name="my third macro!", contents="different")
    #Prefs.set_current_macros(my_set)

    def terminate():
        """Default on_endloop_call() implementation; calls the app_will_finish()
        hook and then calls Base.all_done() to tear down MacTerm.
        """
        app_will_finish()
        Base.all_done()

    Events.on_endloop_call(terminate)

    # start user interaction (WARNING: will not return, but calls the endloop
    # callback when finished)
    Events.run_loop()
    # IMPORTANT: anything beyond this will NEVER RUN; use callbacks instead
