/*!	\file QuillsSession.h
	\brief Session APIs exposed to scripting languages.
	
	Information on these APIs is available through "pydoc".
*/
/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This program is free software; you can redistribute it or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version
	2 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more
	details.
	
	You should have received a copy of the GNU General Public
	License along with this program; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include <UniversalDefines.h>

#pragma once

// standard-C++ includes
#include <map>
#include <string>
#include <vector>

// application includes
#include "QuillsCallbacks.typedef.h"
#include "QuillsSWIG.h"
#include "SessionRef.typedef.h"



#pragma mark Public Methods
namespace Quills {

#if SWIG
%feature("docstring",
"Run commands in terminal windows, and other Session-related actions.\n\
") Session;
#endif
class Session
{
public:
#if SWIG
%feature("docstring",
"Create a new session with a terminal window, change to a\n\
specific directory (if 'cwd' is not empty) and run a Unix\n\
command line.  The session remains active until it is\n\
terminated by the user or the command finishes.\n\
") Session;
%feature("kwargs") Session;
#endif
	Session	(std::vector< std::string >		argv,
			 std::string					cwd = "");
	
#if SWIG
%feature("docstring",
"Return the path of the pseudo-terminal device connected to the\n\
session; for example, '/dev/ttyp0'.  Writes to this device are\n\
immediately interpreted by the local terminal of the session!\n\
\n\
The character encoding is UTF-8.\n\
") pseudo_terminal_device_name;

// raise Python exception if C++ throws anything
%exception pseudo_terminal_device_name
{
	try
	{
		$action
	}
	SWIG_CATCH_STDEXCEPT // catch various std::exception derivatives
	QUILLS_CATCH_ALL
}
#endif
	std::string pseudo_terminal_device_name ();
	
#if SWIG
%feature("docstring",
"Return a string describing the resource for the session, which\n\
will usually be its Unix command line.\n\
\n\
The character encoding is UTF-8.\n\
") resource_location_string;

// raise Python exception if C++ throws anything
%exception resource_location_string
{
	try
	{
		$action
	}
	SWIG_CATCH_STDEXCEPT // catch various std::exception derivatives
	QUILLS_CATCH_ALL
}
#endif
	std::string resource_location_string ();
	
#if SWIG
%feature("docstring",
"Return a simple string description of the current state of the\n\
session.  For example, a session might be 'Running'.\n\
\n\
The character encoding is UTF-8.\n\
") state_string;

// raise Python exception if C++ throws anything
%exception state_string
{
	try
	{
		$action
	}
	SWIG_CATCH_STDEXCEPT // catch various std::exception derivatives
	QUILLS_CATCH_ALL
}
#endif
	std::string state_string ();
	
#if SWIG
%feature("docstring",
"Either invoke a Python callback to handle the specified file,\n\
or trigger the default MacTerm handler if no Python callback\n\
is available.  Callbacks registered via on_fileopen_call() are\n\
considered.\n\
\n\
Currently, file type is determined only using the extension of\n\
the pathname.\n\
\n\
This function returns nothing and is asynchronous; you can,\n\
however, use a routine like on_new_call() to be notified of\n\
new sessions when they appear.\n\
") handle_file;

// do not lose exceptions that may be raised by callbacks
%exception handle_file
{
	try
	{
		$action
	}
	SWIG_CATCH_STDEXCEPT // catch various std::exception derivatives
	QUILLS_CATCH_ALL
}
#endif
	static void handle_file (std::string	pathname);
	
#if SWIG
%feature("docstring",
"Either invoke a Python callback to handle the specified URL,\n\
or trigger the default MacTerm handler if no Python callback\n\
is available.  Callbacks registered via on_urlopen_call() are\n\
considered.\n\
\n\
This function returns nothing and is asynchronous; you can,\n\
however, use a routine like on_new_call() to be notified of\n\
new sessions when they appear.\n\
") handle_url;

// do not lose exceptions that may be raised by callbacks
%exception handle_url
{
	try
	{
		$action
	}
	SWIG_CATCH_STDEXCEPT // catch various std::exception derivatives
	QUILLS_CATCH_ALL
}
#endif
	static void handle_url (std::string		url);
	
#if SWIG
%feature("docstring",
"Return what set_keep_alive_transmission() sets.\n\
") keep_alive_transmission;
#endif
	static std::string keep_alive_transmission ();
	
#if SWIG
%feature("docstring",
"Determine the current working directories of the specified\n\
processes, among user-owned processes.  The result is a map by\n\
process ID, where nonexistent entries could not be determined\n\
(because a process no longer exists or you lack permission,\n\
etc.).  Each process ID maps to a directory path string.\n\
\n\
The character encoding of directory path strings is UTF-8.\n\
") pids_cwds;

// do not lose exceptions that may be raised by callbacks
%exception pids_cwds
{
	try
	{
		$action
	}
	SWIG_CATCH_STDEXCEPT // catch various std::exception derivatives
	QUILLS_CATCH_ALL
}
#endif
	static std::map< long, std::string > pids_cwds (const std::vector< long >&	pids);
	
#if SWIG
%feature("docstring",
"Specify the text to send to the server when a long idle timer\n\
expires on a session that is watching for inactivity.\n\
\n\
Probably, this should be a very short string that will not\n\
interface with whatever happens to be running on the server; a\n\
single space is typical.\n\
") set_keep_alive_transmission;
#endif
	static void set_keep_alive_transmission	(std::string	text);
	
	// only intended for direct use by the SWIG wrapper
	static void _on_fileopen_ext_call_py (Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, void*, std::string);
	static void _on_new_call_py (Quills::FunctionReturnVoidArg1VoidPtr, void*);
	static void _on_seekpidscwds_call_py (Quills::FunctionReturnStringByLongArg1VoidPtrArg2LongVector, void*);
	static void _on_urlopen_call_py (Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, void*, std::string);
	static void _stop_fileopen_ext_call_py (Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, std::string);
	static void _stop_new_call_py (Quills::FunctionReturnVoidArg1VoidPtr);
	static void _stop_urlopen_call_py (Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, std::string);

private:
	SessionRef		_session;
};

// callback support
#if SWIG
%extend Session {
%feature("docstring",
"Register a Python function to be called, with a single string\n\
argument, every time an open is requested for a file with the\n\
given attribute.\n\
\n\
Specify only one attribute (keyword parameter) at a time.  You\n\
can reuse the same callback function, just register it more than\n\
once and provide a different attribute for each call.\n\
\n\
Currently, the only supported attribute is 'extension', which\n\
refers to the end of the filename without a dot (.).  Examples\n\
include 'txt' for text, and 'sh' for Bourne shell.  Note that the\n\
Finder obeys extension mappings in the 'Info.plist' file of the\n\
application bundle, so you may wish to update that file when\n\
adding new handlers.  If the 'Info.plist' does not include the\n\
extension you choose, your handler will only be used for files\n\
that are forced to open with MacTerm (say, when they are dragged\n\
onto the Dock icon).\n\
\n\
You cannot register more than one Python function for the same\n\
attribute.  Registering a Python function for an attribute that\n\
MacTerm natively handles will override the default MacTerm\n\
implementation.\n\
\n\
Your handler is given a single argument, the pathname string,\n\
which you must decompose yourself (but note that Python has\n\
built-in libraries such as the 'os.path' module to help parse).\n\
Generally your handler constructs a Session object with a command\n\
that is appropriate for the file, although you could do something\n\
else.\n\
") on_fileopen_call;
%feature("kwargs") on_fileopen_call;
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonStringReturnVoid" is defined in Quills.i
	static void
	on_fileopen_call	(PyObject*		inPythonFunction,
						 std::string	extension = "")
	{
		if (extension != "")
		{
			Quills::Session::_on_fileopen_ext_call_py(CallPythonStringReturnVoid, reinterpret_cast< void* >(inPythonFunction),
														extension);
			Py_INCREF(inPythonFunction);
		}
	}
	
%feature("docstring",
"Register a Python function to be called (with no arguments)\n\
every single time a session is created.\n\
") on_new_call;
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonVoidReturnVoid" is defined in Quills.i
	static void
	on_new_call	(PyObject*	inPythonFunction)
	{
		Quills::Session::_on_new_call_py(CallPythonVoidReturnVoid, reinterpret_cast< void* >(inPythonFunction));
		Py_INCREF(inPythonFunction);
	}
	
%feature("docstring",
"Register a Python function to be called (with a list argument)\n\
every time the current working directory of one or more processes\n\
is needed.  Each argument is an integer, the process ID to check.\n\
\n\
Return a dictionary that maps integers to strings.  Each integer\n\
is a process ID for which a directory could be found, and the\n\
corresponding string in UTF-8 encoding should be a POSIX path for\n\
a directory (the string may be empty if nothing was found, but it\n\
is also OK to simply omit process IDs that had errors).\n\
\n\
This function takes multiple arguments and returns a batch of\n\
results because it is very likely to require a fairly expensive\n\
lookup (currently, spawning a separate process).  Therefore, it\n\
is advantageous to request directories for as many processes as\n\
possible in a single call.\n\
\n\
This is currently for MacTerm internal use only.\n\
") _on_seekpidscwds_call;
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonLongVectorReturnStringByLong" is defined in Quills.i
	static void
	_on_seekpidscwds_call	(PyObject*	inPythonFunction)
	{
		Quills::Session::_on_seekpidscwds_call_py(CallPythonLongVectorReturnStringByLong, reinterpret_cast< void* >(inPythonFunction));
		Py_INCREF(inPythonFunction);
	}
	
%feature("docstring",
"Register a Python function to be called, with a single string\n\
argument, every time an open is requested for a URL whose schema\n\
(e.g. 'http') matches the schema given as the argument to\n\
on_urlopen_call().  You cannot register more than one Python\n\
function for a particular URL schema.  Registering a Python\n\
function for a schema that MacTerm natively handles will\n\
override the default MacTerm implementation.\n\
\n\
Your handler is given a single argument, the URL string, which\n\
you must decompose yourself (but note that Python has built-in\n\
libraries such as the 'urlparse' module to help, and the default\n\
MacTerm parsers are also available in a Python module).\n\
Generally your handler constructs a Session object with a command\n\
that is appropriate for the URL, although you could do something\n\
else: for instance, using Python's built-in 'webbrowser' or\n\
'urllib' modules.\n\
") on_urlopen_call;
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonStringReturnVoid" is defined in Quills.i
	static void
	on_urlopen_call	(PyObject*		inPythonFunction,
					 std::string	schema)
	{
		Quills::Session::_on_urlopen_call_py(CallPythonStringReturnVoid, reinterpret_cast< void* >(inPythonFunction),
												schema);
		Py_INCREF(inPythonFunction);
	}
	
%feature("docstring",
"Prevent a Python function from being called when opens are\n\
requested for files with the given attribute.  Only one of the\n\
attributes (keyword parameters) should be given.  This would be\n\
to undo the effects of a previous call to on_fileopen_call().\n\
") stop_fileopen_call;
%feature("kwargs") stop_fileopen_call;
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonStringReturnVoid" is defined in Quills.i
	static void
	stop_fileopen_call	(PyObject*		inPythonFunction,
						 std::string	extension = "")
	{
		if (extension != "")
		{
			Quills::Session::_stop_fileopen_ext_call_py(CallPythonStringReturnVoid, extension);
			Py_DECREF(inPythonFunction);
		}
	}
	
%feature("docstring",
"Prevent a Python function from being called when sessions are\n\
created.  This would be to undo the effects of a previous call\n\
to on_new_call().\n\
") stop_new_call;
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonVoidReturnVoid" is defined in Quills.i
	static void
	stop_new_call	(PyObject*	inPythonFunction)
	{
		Quills::Session::_stop_new_call_py(CallPythonVoidReturnVoid);
		Py_DECREF(inPythonFunction);
	}
	
%feature("docstring",
"Prevent a Python function from being called when URL opens are\n\
requested.  This would be to undo the effects of a previous\n\
call to on_urlopen_call().\n\
") stop_urlopen_call;
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonStringReturnVoid" is defined in Quills.i
	static void
	stop_urlopen_call	(PyObject*		inPythonFunction,
						 std::string	schema)
	{
		Quills::Session::_stop_urlopen_call_py(CallPythonStringReturnVoid, schema);
		Py_DECREF(inPythonFunction);
	}
}
#endif

} // namespace Quills

// BELOW IS REQUIRED NEWLINE TO END FILE
