/*!	\file QuillsSession.h
	\brief Session APIs exposed to scripting languages.
	
	Information on these APIs is available through "pydoc".
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#include "UniversalDefines.h"

#ifndef __QUILLSSESSION__
#define __QUILLSSESSION__

// standard-C++ includes
#include <string>
#include <vector>

// MacTelnet includes
#include <QuillsCallbacks.typedef.h>
#include <SessionRef.typedef.h>



#pragma mark Public Methods
namespace Quills {

class Session
{
public:
#if SWIG
%feature("docstring",
"Create a new session with a terminal window, and run a Unix\n\
command line in it.  The session remains active until it is\n\
terminated by the user or the command finishes.\n\
") Session;
#endif
	Session	(std::vector< std::string >		inArgV);
	
#if SWIG
%feature("docstring",
"Either invoke a Python callback to handle the specified URL,\n\
or trigger the default MacTelnet handler if no Python callback\n\
is available for the schema (e.g. 'http') of the given URL.\n\
This function returns nothing and is asynchronous; you can,\n\
however, use a routine like on_new_call() to be notified of\n\
new sessions when they appear.\n\
") handle_url;
#endif
	static void handle_url (std::string		inURL);
	
	// only intended for direct use by the SWIG wrapper
	static void _on_new_call_py (Quills::FunctionReturnVoidArg1VoidPtr, void*);
	static void _on_urlopen_call_py (Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, void*, std::string);
	static void _stop_new_call_py (Quills::FunctionReturnVoidArg1VoidPtr);
	static void _stop_urlopen_call_py (Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, std::string);

private:
	SessionRef		_session;
};

// callback support
#if SWIG
%feature("docstring",
"Register a Python function to be called (with no arguments)\n\
every single time a session is created.\n\
") on_new_call;

%feature("docstring",
"Register a Python function to be called (with a single string\n\
argument) every single time an open is requested for a URL\n\
whose schema (e.g. 'http') matches the schema given as the\n\
argument to on_urlopen_call().  You cannot register more than\n\
one Python function for a particular URL schema.  Registering\n\
a Python function for a schema that MacTelnet natively handles\n\
will override the default MacTelnet implementation.\n\
\n\
Your handler is given a single argument, the URL string, which\n\
you must decompose yourself (but note that Python has built-in\n\
libraries such as the 'urlparse' module, to help).  Generally\n\
your handler constructs a Session object with a command that\n\
is appropriate for the URL, although you could do something\n\
else: for instance, using Python's built-in 'webbrowser' or\n\
'urllib' modules.\n\
") on_urlopen_call;

%feature("docstring",
"Prevent a Python function from being called when sessions are\n\
created.  This would be to undo the effects of a previous call\n\
to on_new_call().\n\
") stop_new_call;

%feature("docstring",
"Prevent a Python function from being called when URL opens are\n\
requested.  This would be to undo the effects of a previous\n\
call to on_urlopen_call().\n\
") stop_urlopen_call;

%extend Session {
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonVoidReturnVoid" is defined in Quills.i
	static void
	on_new_call	(PyObject*	inPythonFunction)
	{
		Quills::Session::_on_new_call_py(CallPythonVoidReturnVoid, reinterpret_cast< void* >(inPythonFunction));
		Py_INCREF(inPythonFunction);
	}
	
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonStringReturnVoid" is defined in Quills.i
	static void
	on_urlopen_call	(PyObject*		inPythonFunction,
					 std::string	inSchema)
	{
		Quills::Session::_on_urlopen_call_py(CallPythonStringReturnVoid, reinterpret_cast< void* >(inPythonFunction),
												inSchema);
		Py_INCREF(inPythonFunction);
	}
	
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonVoidReturnVoid" is defined in Quills.i
	static void
	stop_new_call	(PyObject*	inPythonFunction)
	{
		Quills::Session::_stop_new_call_py(CallPythonVoidReturnVoid);
		Py_DECREF(inPythonFunction);
	}
	
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonStringReturnVoid" is defined in Quills.i
	static void
	stop_urlopen_call	(PyObject*		inPythonFunction,
						 std::string	inSchema)
	{
		Quills::Session::_stop_urlopen_call_py(CallPythonStringReturnVoid, inSchema);
		Py_DECREF(inPythonFunction);
	}
}
#endif

} // namespace Quills

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
