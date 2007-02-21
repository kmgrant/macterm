/*!	\file QuillsPrefs.h
	\brief User preferences APIs exposed to scripting
	languages.
	
	Use this class to access and modify user preferences
	from within a script.
	
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

#ifndef __QUILLSPREFS__
#define __QUILLSPREFS__

// standard-C++ includes
#include <string>
#include <vector>



#pragma mark Public Methods
namespace Quills {

class Prefs
{
public:
	enum Class
	{
		GENERAL = 0,
		FORMAT = 1,
		MACRO_SET = 2,
		SESSION = 3,
		TERMINAL = 4
	};
#if SWIG
%feature("docstring",
"Returns a list of collection names for preferences saved in the\n\
given category, if any.\n\
") list_collections;
#endif
	static std::vector< std::string > list_collections (Class	inClass);

private:
	Prefs (); // class is not instantiated
};

} // namespace Quills

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
