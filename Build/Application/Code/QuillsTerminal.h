/*!	\file QuillsTerminal.h
	\brief Terminal window APIs exposed to scripting languages.
	
	Information on these APIs is available through "pydoc".
*/
/*###############################################################

	MacTelnet
		© 1998-2009 by Kevin Grant.
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
character code.\n\
\n\
Normally, you should just call dumb_strings_init() to register\n\
a callback that is automatically consulted for each character\n\
code in turn.  If dumb_strings_init() is called after this\n\
function, your changes will be overridden by the callback; but\n\
you can call this routine after registering a callback to make\n\
minor corrections.\n\
") set_dumb_string_for_char;
#endif
	static void set_dumb_string_for_char	(unsigned short		unicode,
											 std::string		rendering_utf8);
};

class BasicPalette
{
public:
	BasicPalette ();
	
#if SWIG
%feature("docstring",
"Return the red, green and blue components of the specified\n\
color in the palette.  Note that Python already has utilities\n\
for dealing with colors (the 'colorsys' conversion routines,\n\
for instance), so MacTelnet does not provide much here.\n\
\n\
Currently, the color can be one of: 'red', 'green', 'yellow',\n\
'blue', 'magenta', 'cyan', 'black' or 'white'.\n\
") rgb;

%exception rgb {
	try {
		$action
	} catch (std::invalid_argument const& e) {
		PyErr_SetString(PyExc_KeyError, e.what());
		return NULL;
	}
}
#endif
	std::vector< double > rgb (std::string	inColor);

private:
	// currently, RGB color space is assumed internally; in the future
	// it may be necessary to have a variable that holds the color space
	std::vector< double >	_red;
	std::vector< double >	_green;
	std::vector< double >	_yellow;
	std::vector< double >	_blue;
	std::vector< double >	_magenta;
	std::vector< double >	_cyan;
	std::vector< double >	_white;
	std::vector< double >	_black;
};

} // namespace Quills

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
