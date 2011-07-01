/*!	\file QuillsEvents.h
	\brief User interaction event APIs exposed to scripting
	languages.
	
	Use this class to interact with the user via the
	MacTelnet graphical user interface.
	
	Information on these APIs is available through "pydoc".
*/
/*###############################################################

	MacTerm
		© 1998-2010 by Kevin Grant.
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

#ifndef __QUILLSEVENTS__
#define __QUILLSEVENTS__

// application includes
#include "QuillsCallbacks.typedef.h"



#pragma mark Public Methods
namespace Quills {

#if SWIG
%feature("docstring",
"Manage setup and teardown of the main application event loop.\n\
") Events;
#endif
class Events
{
public:
#if SWIG
%feature("docstring",
"Begin graphical user interface interaction.  Complete control\n\
is given to the user, any future callbacks to the code are made\n\
in response to actions such as clicks on menus and buttons.\n\
\n\
IMPORTANT: This call blocks until the user asks to quit.\n\
") run_loop;
#endif
	static void run_loop ();
	
#if SWIG
// do not lose exceptions that may be raised by callbacks
%exception handle_url
{
	try
	{
		$action
	}
    SWIG_CATCH_STDEXCEPT // catch various std::exception derivatives
	SWIG_CATCH_UNKNOWN
}
#endif
	// only intended for direct use by MacTelnet’s application delegate in Cocoa
	static void _handle_endloop ();
	
	// only intended for direct use by the SWIG wrapper
	static void _on_endloop_call_py (Quills::FunctionReturnVoidArg1VoidPtr, void*);

private:
	Events (); // class is not instantiated
};

// callback support
#if SWIG
%extend Events {
%feature("docstring",
"Register a Python function to be called (with no arguments)\n\
immediately after the main event loop terminates.\n\
\n\
This is the only way for Python code to continue running after\n\
you call Events.run_loop().  At some point in this callback,\n\
you MUST call Base.all_done() to clean up MacTelnet modules.\n\
") on_endloop_call;
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonVoidReturnVoid" is defined in Quills.i
	static void
	on_endloop_call	(PyObject*	inPythonFunction)
	{
		Quills::Events::_on_endloop_call_py(CallPythonVoidReturnVoid, reinterpret_cast< void* >(inPythonFunction));
		Py_INCREF(inPythonFunction);
	}
}
#endif

} // namespace Quills

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
