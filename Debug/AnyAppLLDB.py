#!/usr/bin/python

# Runs the debugger on an internal application that is packaged
# inside the main bundle.
#
# A single argument is required, the name of the executable
# binary (which should match the bundle without the ".app").
# A full build must have been done previously, to ensure that
# the target application is already built and installed.
#
# Kevin Grant (kmg@mac.com)
# March 24, 2017

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os
import sys

DIR_SELF = os.path.abspath(os.path.dirname(__file__))
SCRIPT_NAME = sys.argv[0]
try:
    APP_EXEC_NAME = sys.argv[1]
except IndexError:
    print('%s: Application executable name argument is required.' % SCRIPT_NAME, file=sys.stderr)
    sys.exit(1)
if APP_EXEC_NAME == "MacTerm":
    APP_RELATIVE_PATH = "MacTerm.app"
else:
    APP_RELATIVE_PATH = "MacTerm.app/Contents/Resources/%s.app" % APP_EXEC_NAME
APP_ABSOLUTE_PATH = os.path.abspath(os.path.join(DIR_SELF, "..", "Build", APP_RELATIVE_PATH))

sys.path.insert(0, os.path.join(DIR_SELF, "..", "Build", "Application", "PythonCode"))
from pymacterm.utilities import bytearray_to_str as bytearray_to_str
from pymacterm.utilities import command_data as cmd_data

def warn(*args, **kwargs):
    """Shorthand for printing to sys.stderr.
    """
    print(*args, file=sys.stderr, **kwargs)

def pid_for_running_app_named(app_exec_name):
    """Run 'ps' to see if the specified internal application
    is running.  This also requires that the SPECIFIC COPY
    of the application match the current work area, since it
    is possible to be running one MacTerm while debugging
    another one; if an exact match to the absolute path is
    found, the specific process ID is returned.
    """
    result = None
    ps_output = cmd_data(('/bin/ps', 'axo', 'pid,command'))
    ENCODING = 'utf-8'
    # filter out lines that may come from different commands;
    # require the target path to be an app-embedded bundle
    for line in ps_output.split(bytearray('\n', encoding=ENCODING)):
        if (bytearray("%s.app" % app_exec_name, encoding=ENCODING) in line):
            if bytearray(APP_ABSOLUTE_PATH, encoding=ENCODING) in line:
                #warn("match:", line) # debug
                elements = line.split(bytearray(' ', encoding=ENCODING))
                pid = elements[0]
                result = int(pid)
                #break # check all, allowing print-outs for similar matches
            else:
                # if multiple instances are running, ignore ones that are
                # installed in a different tree than this script
                warn("%s: Ignoring similar process (not same workspace): %s" % (SCRIPT_NAME, bytearray_to_str(line)))
    return result

if __name__ == "__main__":
    new_environ = dict(os.environ)

    # note: on newer versions of the OS this might not be needed
    # (since /usr/bin/lldb tries to find Xcode)
    if 'DEVELOPER_DIR' in os.environ:
        developer_dir = os.environ['DEVELOPER_DIR']
        warn("%s: Setting Xcode path to '%s'." % (SCRIPT_NAME, developer_dir))
    else:
        developer_dir = '/Applications/Xcode.app/Contents/Developer'

    os.environ['PATH'] = ":".join(["%s/usr/bin" % developer_dir, os.environ['PATH']])

    # for predictability, always run from the script directory
    if APP_EXEC_NAME == "MacTerm":
        os.chdir('%s/../Build/MacTerm.app/Contents/MacOS' % DIR_SELF)
    else:
        os.chdir(DIR_SELF)
    ABS_CWD = os.path.abspath(os.curdir)
    new_environ['PWD'] = ABS_CWD
    warn("%s: Changed to directory '%s'." % (SCRIPT_NAME, ABS_CWD))

    RUNNING_APP_PID = pid_for_running_app_named(APP_EXEC_NAME)
    if RUNNING_APP_PID is not None:
        warn("%s: Attaching to existing '%s' in the debugger (use 'c' at the prompt to resume the program)." % (SCRIPT_NAME, APP_EXEC_NAME))
        new_argv = ['lldb', '--attach-pid', str(RUNNING_APP_PID)]
    else:
        # certain internal applications have environment dependencies
        # that prevent them from working unless they were launched
        # by the main program so it is an error if they are not found
        if APP_EXEC_NAME in ('PrintPreview'):
            warn("%s: No running process for '%s' (and it only makes sense to attach to it); exiting." % (SCRIPT_NAME, APP_EXEC_NAME))
            sys.exit(1)
        warn("%s: Launching new '%s' in the debugger (use 'r' at the prompt to start the program)." % (SCRIPT_NAME, APP_EXEC_NAME))
        if APP_EXEC_NAME == "MacTerm":
            # !!! IMPORTANT !!!
            # Since MacTerm runs from a shared library referenced by a Python
            # interpreter, "lldb" can only run on the Python binary and the
            # environment must be set correctly to find dependent libraries.
            # Relying on the "MacTerm" script's own debug mode is prudent.
            new_argv = ['./MacTerm']
            new_environ['MACTERM_DEBUG'] = '1'
        else:
            new_argv = ['lldb', '--', '%s/Contents/MacOS/%s' % (APP_ABSOLUTE_PATH, APP_EXEC_NAME)]

    try:
        os.execvpe(new_argv[0], new_argv, new_environ)
    except OSError as _:
        warn("Exception raised while attempting to run",
             new_argv[0], new_argv, ":", str(_))
    sys.exit(1)
