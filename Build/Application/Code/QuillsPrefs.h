/*!	\file QuillsPrefs.h
	\brief User preferences APIs exposed to scripting languages.
	
	Use this class to access and modify user preferences from
	within a script.
	
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

#ifndef __QUILLSPREFS__
#define __QUILLSPREFS__

// standard-C++ includes
#include <string>
#include <vector>

// MacTelnet includes
#include <PreferencesContextRef.typedef.h>



#pragma mark Public Methods
namespace Quills {

#if SWIG
%feature("docstring",
"GENERAL -- Preferences not typically found in collections.\n\
FORMAT -- Font and color settings.\n\
MACRO_SET -- Actions mapped to keyboard short-cuts.\n\
SESSION -- How to reach, and interact with, a resource.\n\
TERMINAL -- Characteristics of the emulator and its data storage.\n\
TRANSLATION -- Text encoding.\n\
WORKSPACE -- Windows that are spawned at the same time.\n\
_FACTORY_DEFAULTS -- Represents the DefaultPreferences.plist,\n\
for internal use only.\n\
") Prefs;
#endif
class Prefs
{
public:
	enum Class
	{
		GENERAL = 0,
		FORMAT = 1,
		MACRO_SET = 2,
		SESSION = 3,
		TERMINAL = 4,
		TRANSLATION = 5,
		WORKSPACE = 6,
		_FACTORY_DEFAULTS = 100,
	};
	
#if SWIG
%feature("docstring",
"Create a new collection of the given type.\n\
\n\
Currently, only in-memory (temporary) collections are supported\n\
through this interface.\n\
") Prefs;
#endif
	Prefs	(Class	inClass);
	~Prefs	();
	
#if SWIG
%feature("kwargs") define_macro;
%feature("docstring",
"Add or modify a macro in this collection.\n\
\n\
The index is at least 1, and specifies which macro in the set\n\
to change.\n\
\n\
The keyword arguments are optional; any that are given will be\n\
assigned as attributes of the macro.  Currently, not all\n\
possible attributes can be set from scripts.\n\
\n\
Any given strings must use UTF-8 encoding.\n\
") define_macro;
#endif
	void define_macro	(unsigned int		index_in_set,
						 std::string		name = "",
						 std::string		contents = "");
	
#if SWIG
%feature("docstring",
"Return a list of collection names for preferences saved in the\n\
given category, if any.\n\
\n\
Each string is in UTF-8 encoding.\n\
") list_collections;
#endif
	static std::vector< std::string > list_collections (Class	of_class);
	
#if SWIG
%feature("docstring",
"Change the active macro set to the specified collection,\n\
which should be a Prefs instance of type MACRO_SET.\n\
\n\
Since changes to collections are detected, you may continue to\n\
modify the specified macros, and anything that depends on them\n\
(such as a Macros menu) will update automatically.\n\
") set_current_macros;
#endif
	
	// only intended for direct use by the SWIG wrapper
	static void _set_current_macros	(Prefs&);

private:
	Preferences_ContextRef		_context;	//!< manages access to settings
};

#if SWIG
%extend Prefs {
	// Any routines that accept a Python-generated object
	// for unspecified future use must be able to retain
	// that reference.  These method wrappers help to keep
	// the Python stuff contained here, relying on internal
	// methods for most of the implementation.
	
	static void set_current_macros	(PyObject*		new_set)
	{
		void*				objectPtr = nullptr;
		Quills::Prefs*		prefsPtr = nullptr;
		int					conversionResult = 0;
		
		
		// inform SWIG that it should no longer allow Python to free this object
		conversionResult = SWIG_ConvertPtr(new_set, &objectPtr, SWIGTYPE_p_Quills__Prefs, SWIG_POINTER_DISOWN |  0 );
		if (false == SWIG_IsOK(conversionResult))
		{
			throw std::invalid_argument("specified set does not appear to be a Prefs object");
		}
		prefsPtr = REINTERPRET_CAST(objectPtr, Quills::Prefs*);
		Quills::Prefs::_set_current_macros(*prefsPtr);
	}
} // %extend
#endif

} // namespace Quills

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
