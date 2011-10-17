/*!	\file QuillsEvents.cp
	\brief User interaction event APIs exposed to scripting
	languages.
*/
/*###############################################################

	MacTerm
		© 1998-2011 by Kevin Grant.
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

#include "QuillsEvents.h"
#include <UniversalDefines.h>

// application includes
#include "EventLoop.h"



#pragma mark Variables
namespace {

Quills::FunctionReturnVoidArg1VoidPtr	gEventsEndLoopCallbackInvoker = nullptr;
void*									gEventsEndLoopPythonCallback = nullptr;

} // anonymous namespace



#pragma mark Public Methods
namespace Quills {


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Events::run_loop ()
{
	EventLoop_Run(); // returns only when the user asks to quit
}// run_loop


/*!
See header or "pydoc" for Python docstrings.

(4.0)
*/
void
Events::_handle_endloop ()
{
	if (nullptr != gEventsEndLoopCallbackInvoker) (*gEventsEndLoopCallbackInvoker)(gEventsEndLoopPythonCallback);
}// _handle_endloop


/*!
See header or "pydoc" for Python docstrings.

(4.0)
*/
void
Events::_on_endloop_call_py		(FunctionReturnVoidArg1VoidPtr	inRoutine,
								 void*							inPythonFunctionObject)
{
	gEventsEndLoopCallbackInvoker = inRoutine;
	gEventsEndLoopPythonCallback = inPythonFunctionObject;
}// _on_endloop_call_py


} // namespace Quills

// BELOW IS REQUIRED NEWLINE TO END FILE
