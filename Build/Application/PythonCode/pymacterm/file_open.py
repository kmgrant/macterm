# vim: set fileencoding=UTF-8 :

"""Routines to open various types of files.

macros -- set current macro set according to a ".macros" key-value-pair file
prefs -- import preferences stored in a standard XML property list
script -- run any executable file as a Session
session -- start a Session according to a ".session" key-value-pair file

"""

__author__ = 'Kevin Grant <kmg@mac.com>'
__date__ = '1 January 2008'
__version__ = '4.0.0'

from . import file_kvp
# note: Quills is a compiled module, library path must be set properly
import quills

def macros(pathname):
    """macros(pathname) -> None

    Load macros from the given ".macros" file, by parsing the
    file and assuming that the first up to 12 keys have values
    representing strings to be sent.  (This is a limitation of
    the original file format, that might be avoidable if a
    transition to XML is made in the future.)
    Raise SyntaxError if the file is not formatted properly.
    Raise KeyError if the file is successfully parsed but it has
    no keys that apparently represent macros (macro key names
    should look like "f1", "f2", ... or "m0", "m1", ...).

    WARNING: This routine is incomplete, as the Quills API has
    not advanced far enough to make use of all of the settings
    that could exist in a ".macros" file.

    TEMPORARY: This should probably be separated into a
    routine that converts macros files into Quills.Prefs
    objects, and a routine that is a file-open handler that
    actually enables those macro preferences.  Right now, this
    is doing both.

    """
    with open(pathname, 'rU') as mfile:
        parser = file_kvp.Parser(file_object=mfile)
        defs = parser.results()
        macro_set = quills.Prefs(quills.Prefs.MACRO_SET)
        for key in defs:
            data = defs[key]
            first_part = key.rstrip('0123456789')
            second_part = key.replace(first_part, "")
            number = int(second_part)
            # need a one-based index, files normally start with 0 (except F1...)
            if first_part != "f" and first_part != "F":
                number += 1
            macro_set.define_macro(number, name="Macro %i" % number,
                                   contents=data)
        quills.Prefs.set_current_macros(macro_set)

def prefs(pathname):
    """prefs(pathname) -> None

    Synchronously import preferences from the given XML file
    and automatically generate a unique name if necessary.
    Raise an exception on failure.

    """
    quills.Prefs.import_from_file(pathname, allow_rename=True)

def script(pathname):
    """script(pathname) -> None

    Asynchronously open a session from the given script file, by
    running the script!  Raise an exception on failure.

    """
    args = [pathname]
    ignored_session = quills.Session(args)

def session(pathname):
    """session(pathname) -> None

    Asynchronously open a session from the given ".session" file,
    by parsing the file for command information, and trying to
    use other information from the file to configure the session
    and its terminal window.
    Raise SyntaxError if the file is not formatted properly.
    Raise KeyError if the file is successfully parsed but it has
    no command information.

    WARNING: This routine is incomplete, as the Quills API has
    not advanced far enough to make use of all of the settings
    that could exist in a ".session" file.

    """
    with open(pathname, 'rU') as ifh:
        parser = file_kvp.Parser(file_object=ifh)
        defs = parser.results()
        if 'command' in defs:
            args = defs['command'].split()
            ignored_session = quills.Session(args)
        else:
            raise KeyError('no "command" was found in the file')
