/*!	\file QuillsTerminal.h
	\brief Terminal window APIs exposed to scripting languages.
	
	Information on these APIs is available through "pydoc".
*/
/*###############################################################

	MacTelnet
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

#ifndef __QUILLSTERMINAL__
#define __QUILLSTERMINAL__

// standard-C++ includes
#include <stdexcept>
#include <string>
#include <vector>

// MacTelnet includes
#include "QuillsCallbacks.typedef.h"
#include "Terminal.h"



#pragma mark Public Methods
namespace Quills {

class Terminal
{
public:	
#if SWIG
%feature("docstring",
"Specifies an appropriate dumb-terminal rendering for the given\n\
character code, which is UTF-16 in the range 0-0xFFFF.  This is\n\
global to all terminal views that are using the DUMB emulator,\n\
it cannot be set on a per-screen basis.\n\
\n\
The dumb terminal has a default rendering for codes that have\n\
not specified a different rendering, and the default usually\n\
just prints the numerical value in angle brackets.\n\
") set_dumb_string_for_char;
#endif
	static void set_dumb_string_for_char	(unsigned short		unicode,
											 std::string		rendering_utf8);
	
#if SWIG
%feature("docstring",
"Return a pair of integers as a tuple, that locates a word in\n\
the given string.  The first integer is a character position,\n\
and the second integer is a character count.  Note that since\n\
the given string is an encoded sequence of bytes, it may well\n\
contain fewer characters than bytes, and offsets refer only to\n\
the positions of characters!  Do not use byte offsets.\n\
\n\
The starting character will be in the middle of any word, so\n\
scan both forwards and backwards from that point.\n\
\n\
If there is no word, the pair should hold the original offset in\n\
the first element, and the second element should be 1.\n\
\n\
The character encoding of the given string is UTF-8.\n\
") word_of_char_in_string;

// raise Python exception if C++ throws anything
%exception word_of_char_in_string
{
	try
	{
		$action
	}
    SWIG_CATCH_STDEXCEPT // catch various std::exception derivatives
	SWIG_CATCH_UNKNOWN
}
#endif
	static std::pair<long, long> word_of_char_in_string		(std::string	text_utf8,
															 long			offset);
	
	// only intended for direct use by the SWIG wrapper
	static void _on_seekword_call_py 	(Quills::FunctionReturnLongPairArg1VoidPtrArg2CharPtrArg3Long, void*);
};

#if SWIG
%extend Terminal {
%feature("docstring",
"Register a Python function to be called (with string and integer\n\
offset arguments) every time a word must be found in a string of\n\
text.  The string uses UTF-8 encoding, and may include new-lines.\n\
\n\
Typically, this is used in response to double-clicks.  The value\n\
returned should be a tuple of two integers: the first, an offset\n\
(zero-based) into the original string that identifies the start\n\
of the word to use; the second, the COUNT of characters to read.\n\
The returned range should surround the original offset location.\n\
") on_seekword_call;
	// NOTE: "PyObject* inPythonFunction" is typemapped in Quills.i;
	// "CallPythonStringLongReturnLongPair" is defined in Quills.i
	static void
	on_seekword_call	(PyObject*	inPythonFunction)
	{
		Quills::Terminal::_on_seekword_call_py(CallPythonStringLongReturnLongPair, reinterpret_cast< void* >(inPythonFunction));
		Py_INCREF(inPythonFunction);
	}
}
#endif

} // namespace Quills

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
