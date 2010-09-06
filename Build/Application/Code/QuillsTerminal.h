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
};

} // namespace Quills

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
