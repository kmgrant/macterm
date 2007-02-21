/*###############################################################

	QuillsPrefs.cp
	
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

// standard-C++ includes
#include <string>
#include <vector>

// Mac includes
#include <Carbon/Carbon.h>

// MacTelnet includes
#include "Preferences.h"
#include "QuillsPrefs.h"



#pragma mark Internal Method Prototypes
namespace {

Preferences_Class		returnPreferencesClassForPrefsClass		(Quills::Prefs::Class);

} // anonymous namespace


#pragma mark Public Methods
namespace Quills {

/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
std::vector< std::string >
Prefs::list_collections		(Prefs::Class	inForWhichClass)
{
	std::vector< std::string >	result;
	CFArrayRef					nameCFArray = nullptr;
	Preferences_ResultCode		prefsError = kPreferences_ResultCodeSuccess;
	
	
	prefsError = Preferences_CreateContextNameArray
					(returnPreferencesClassForPrefsClass(inForWhichClass), nameCFArray);
	if (kPreferences_ResultCodeSuccess == prefsError)
	{
		CFIndex const	kCount = CFArrayGetCount(nameCFArray);
		
		
		for (CFIndex i = 0; i < kCount; ++i)
		{
			CFStringRef		nameCFString = REINTERPRET_CAST
											(CFArrayGetValueAtIndex(nameCFArray, i), CFStringRef);
			
			
			if (nullptr != nameCFString)
			{
				assert(CFStringGetTypeID() == CFGetTypeID(nameCFString));
				char const*		nameCString = CFStringGetCStringPtr
												(nameCFString, kCFStringEncodingMacRoman);
				
				
				if (nullptr != nameCString)
				{
					std::string		nameStdString(nameCString);
					
					
					result.push_back(nameStdString);
				}
			}
		}
		CFRelease(nameCFArray), nameCFArray = nullptr;
	}
	
	return result;
}// list_collections


} // namespace Quills


#pragma mark Internal Methods
namespace {

/*!
The Python API exposes different typenames for each
class of preference, so this API maps those typenames
to internal typenames.

(3.1)
*/
Preferences_Class
returnPreferencesClassForPrefsClass		(Quills::Prefs::Class	inType)
{
	Preferences_Class	result = kPreferences_ClassGeneral;
	
	
	switch (inType)
	{
	case Quills::Prefs::GENERAL:
		result = kPreferences_ClassGeneral;
		break;
	
	case Quills::Prefs::FORMAT:
		result = kPreferences_ClassFormat;
		break;
	
	case Quills::Prefs::MACRO_SET:
		result = kPreferences_ClassMacroSet;
		break;
	
	case Quills::Prefs::SESSION:
		result = kPreferences_ClassSession;
		break;
	
	case Quills::Prefs::TERMINAL:
		result = kPreferences_ClassTerminal;
		break;
	
	default:
		// ???
		break;
	}
	
	return result;
}// returnPreferencesClassForCollectionType


} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
