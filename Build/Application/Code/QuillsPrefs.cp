/*###############################################################

	QuillsPrefs.cp
	
	MacTelnet
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

#include "UniversalDefines.h"

// standard-C++ includes
#include <stdexcept>
#include <string>
#include <vector>

// Mac includes
#include <Carbon/Carbon.h>
#include <StringUtilities.h>

// library includes
#include <CFRetainRelease.h>
#include <Console.h>

// MacTelnet includes
#include "MacroManager.h"
#include "Preferences.h"
#include "QuillsPrefs.h"



#pragma mark Public Methods
namespace Quills {


/*!
See header or "pydoc" for Python docstrings.

(4.0)
*/
Prefs::
Prefs	(Prefs::Class	of_class)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
_context(nullptr)
{
	try
	{
		_context = Preferences_NewContext(of_class);
	}
	catch (...)
	{
		// WARNING: The constructors for objects that are exposed to scripts
		// cannot throw exceptions, because ownership rules imply an object
		// is always created and released.  Instead, the object “exists” in
		// a state that simply triggers exceptions when certain methods are
		// later called on the object.  This also helps in cases where an
		// object may asynchronously become invalid.
		Console_Warning(Console_WriteLine, "unexpected exception in Prefs constructor");
		_context = nullptr;
	}
}// default constructor


/*!
See header or "pydoc" for Python docstrings.

(4.0)
*/
Prefs::
~Prefs ()
{
	if (nullptr != _context) Preferences_ReleaseContext(&_context);
}// destructor


/*!
See header or "pydoc" for Python docstrings.

(4.0)
*/
void
Prefs::define_macro		(unsigned int		index_in_set,
						 std::string		name,
						 std::string		contents)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	
	
	if ((index_in_set < 1) || (index_in_set > kMacroManager_MaximumMacroSetSize))
	{
		throw std::invalid_argument("macro index is out of range");
	}
	
	if (false == name.empty())
	{
		CFRetainRelease		nameObject(CFStringCreateWithCString(kCFAllocatorDefault, name.c_str(), kCFStringEncodingUTF8),
										true/* is retained */);
		CFStringRef			nameCFString = nameObject.returnCFStringRef();
		
		
		prefsResult = Preferences_ContextSetData(this->_context, Preferences_ReturnTagVariantForIndex
																	(kPreferences_TagIndexedMacroName, index_in_set),
													sizeof(nameCFString), &nameCFString);;
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_WriteValue("failed to set the macro name, Preferences module result", prefsResult);
			throw std::invalid_argument("failed to set the macro name");
		}
	}
	
	if (false == contents.empty())
	{
		CFRetainRelease		contentsObject(CFStringCreateWithCString(kCFAllocatorDefault, contents.c_str(), kCFStringEncodingUTF8),
											true/* is retained */);
		CFStringRef			contentsCFString = contentsObject.returnCFStringRef();
		
		
		prefsResult = Preferences_ContextSetData(this->_context, Preferences_ReturnTagVariantForIndex
																	(kPreferences_TagIndexedMacroContents, index_in_set),
													sizeof(contentsCFString), &contentsCFString);;
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_WriteValue("failed to set the macro contents, Preferences module result", prefsResult);
			throw std::invalid_argument("failed to set the macro contents");
		}
	}
}// define_macro


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
std::vector< std::string >
Prefs::list_collections		(Prefs::Class	of_class)
{
	std::vector< std::string >	result;
	CFArrayRef					nameCFArray = nullptr;
	Preferences_Result			prefsError = kPreferences_ResultOK;
	
	
	prefsError = Preferences_CreateContextNameArray(of_class, nameCFArray);
	if (kPreferences_ResultOK == prefsError)
	{
		CFIndex const	kCount = CFArrayGetCount(nameCFArray);
		
		
		for (CFIndex i = 0; i < kCount; ++i)
		{
			CFStringRef		nameCFString = REINTERPRET_CAST
											(CFArrayGetValueAtIndex(nameCFArray, i), CFStringRef);
			
			
			if (nullptr != nameCFString)
			{
				assert(CFStringGetTypeID() == CFGetTypeID(nameCFString));
				std::string		nameStdString;
				
				
				StringUtilities_CFToUTF8(nameCFString, nameStdString);
				result.push_back(nameStdString);
			}
		}
		CFRelease(nameCFArray), nameCFArray = nullptr;
	}
	
	return result;
}// list_collections


/*!
See header or "pydoc" for Python docstrings.

(4.0)
*/
void
Prefs::_set_current_macros		(Prefs&		in_set)
{
	MacroManager_SetCurrentMacros(in_set._context);
}// _set_current_macros


} // namespace Quills

// BELOW IS REQUIRED NEWLINE TO END FILE
