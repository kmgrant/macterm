/*!	\file QuillsTerminal.cp
	\brief Terminal window APIs exposed to scripting languages.
*/
/*###############################################################

	MacTerm
		© 1998-2014 by Kevin Grant.
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

#include "QuillsTerminal.h"
#include <UniversalDefines.h>

// standard-C++ includes
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// library includes
#include <Console.h>



#pragma mark variables
namespace {

Quills::FunctionReturnLongPairArg1VoidPtrArg2CharPtrArg3Long	gTerminalSeekWordCallbackInvoker = nullptr;
void*															gTerminalSeekWordPythonCallback = nullptr;

} // anonymous namespace



#pragma mark Public Methods
namespace Quills {

/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Terminal::set_dumb_string_for_char	(unsigned short		unicode,
									 std::string		rendering_utf8)
{
	Terminal_SetDumbTerminalRendering(unicode, rendering_utf8.c_str());
}


/*!
See header or "pydoc" for Python docstrings.

(4.0)
*/
std::pair<long, long>
Terminal::word_of_char_in_string	(std::string	text_utf8,
									 long			offset)
{
	std::pair<long, long>	result = std::make_pair(offset, 1);
	
	
	if (nullptr != gTerminalSeekWordCallbackInvoker)
	{
		char*	mutableTextCopy = new char[1 + text_utf8.size()];
		
		
		// make a copy of the argument, since scripting languages
		// do not distinguish mutable and immutable strings
		mutableTextCopy[text_utf8.size()] = '\0';
		std::copy(text_utf8.begin(), text_utf8.end(), mutableTextCopy);
		
		result = (*gTerminalSeekWordCallbackInvoker)(gTerminalSeekWordPythonCallback, mutableTextCopy, offset);
		
		delete [] mutableTextCopy;
	}
	return result;
}// word_of_char_in_string


/*!
See header or "pydoc" for Python docstrings.

(4.0)
*/
void
Terminal::_on_seekword_call_py	(FunctionReturnLongPairArg1VoidPtrArg2CharPtrArg3Long	inRoutine,
								 void*													inPythonFunctionObject)
{
	gTerminalSeekWordCallbackInvoker = inRoutine;
	gTerminalSeekWordPythonCallback = inPythonFunctionObject;
}// _on_seekword_call_py


} // namespace Quills

// BELOW IS REQUIRED NEWLINE TO END FILE
