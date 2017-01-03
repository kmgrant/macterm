/*!	\file QuillsPrefs.cp
	\brief User preferences APIs exposed to scripting languages.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#include "QuillsPrefs.h"
#include <UniversalDefines.h>

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

// application includes
#include "MacroManager.h"
#include "Preferences.h"



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
										CFRetainRelease::kAlreadyRetained);
		CFStringRef			nameCFString = nameObject.returnCFStringRef();
		
		
		prefsResult = Preferences_ContextSetData(this->_context, Preferences_ReturnTagVariantForIndex
																	(kPreferences_TagIndexedMacroName, STATIC_CAST(index_in_set, Preferences_Index)),
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
											CFRetainRelease::kAlreadyRetained);
		CFStringRef			contentsCFString = contentsObject.returnCFStringRef();
		
		
		prefsResult = Preferences_ContextSetData(this->_context, Preferences_ReturnTagVariantForIndex
																	(kPreferences_TagIndexedMacroContents, STATIC_CAST(index_in_set, Preferences_Index)),
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

(4.0)
*/
void
Prefs::import_from_file		(std::string	inPathname,
							 bool			inAllowRename)
{
	FSRef		fileRef;
	OSStatus	error = noErr;
	
	
	error = FSPathMakeRef(REINTERPRET_CAST(inPathname.c_str(), UInt8 const*), &fileRef, nullptr/* is a directory */);
	if (noErr != error)
	{
		throw std::invalid_argument("failed to import preferences from file");
	}
	else
	{
		// it appears to be an XML property list file; parse it
		Preferences_ContextWrap		temporaryContext(Preferences_NewContext(Quills::Prefs::GENERAL),
														Preferences_ContextWrap::kAlreadyRetained);
		
		
		if (false == temporaryContext.exists())
		{
			throw std::runtime_error("failed to construct temporary context for importing preferences");
		}
		else
		{
			Quills::Prefs::Class	inferredClass = Quills::Prefs::GENERAL;
			CFStringRef				inferredName = nullptr;
			Preferences_Result		prefsResult = Preferences_ContextMergeInXMLFileRef
													(temporaryContext.returnRef(), fileRef,
														&inferredClass, &inferredName);
			
			
			if (kPreferences_ResultOK != prefsResult)
			{
				throw std::invalid_argument("failed to merge XML for preferences into context");
			}
			
			if (nullptr != inferredName)
			{
				// empty strings are converted to nonexistent strings so that
				// a name will be automatically generated later
				if (0 == CFStringGetLength(inferredName))
				{
					CFRelease(inferredName), inferredName = nullptr;
				}
				else if (Preferences_IsContextNameInUse(inferredClass, inferredName))
				{
					if (inAllowRename)
					{
						// throw away the name, have one generated automatically
						CFRelease(inferredName), inferredName = nullptr;
					}
					else
					{
						throw std::invalid_argument("existing collection is already using the same name");
					}
				}
			}
			
			// data was imported successfully to memory; copy to a persistent store
			{
				Preferences_ContextWrap		savedContext(Preferences_NewContextFromFavorites
															(inferredClass, inferredName/* if nullptr, auto-generated */),
															Preferences_ContextWrap::kAlreadyRetained);
				
				
				//Console_WriteValue("import file: inferred new preferences class", inferredClass);
				//Console_WriteValueCFString("import file: inferred new preferences collection name", inferredName);
				if (false == savedContext.exists())
				{
					throw std::runtime_error("failed to create persistent context for imported preferences");
				}
				else
				{
					prefsResult = Preferences_ContextCopy(temporaryContext.returnRef(), savedContext.returnRef());
					if (kPreferences_ResultOK != prefsResult)
					{
						throw std::runtime_error("failed to copy imported preferences into persistent context");
					}
					else
					{
						prefsResult = Preferences_ContextSave(savedContext.returnRef());
						if (kPreferences_ResultOK != prefsResult)
						{
							throw std::runtime_error("failed to save imported preferences");
						}
						else
						{
							UInt32		showCommand = kCommandDisplayPrefPanelGeneral;
							
							
							// successfully created; now show the relevant Preferences window pane
							switch (inferredClass)
							{
							case Quills::Prefs::FORMAT:
								showCommand = kCommandDisplayPrefPanelFormats;
								break;
							
							case Quills::Prefs::MACRO_SET:
								showCommand = kCommandDisplayPrefPanelMacros;
								break;
							
							case Quills::Prefs::SESSION:
								showCommand = kCommandDisplayPrefPanelSessions;
								break;
							
							case Quills::Prefs::TERMINAL:
								showCommand = kCommandDisplayPrefPanelTerminals;
								break;
							
							case Quills::Prefs::TRANSLATION:
								showCommand = kCommandDisplayPrefPanelTranslations;
								break;
							
							case Quills::Prefs::WORKSPACE:
								showCommand = kCommandDisplayPrefPanelWorkspaces;
								break;
							
							default:
								break;
							}
							UNUSED_RETURN(Boolean)Commands_ExecuteByIDUsingEvent(showCommand);
						}
					}
				}
			}
			
			if (nullptr != inferredName)
			{
				CFRelease(inferredName), inferredName = nullptr;
			}
		}
	}
}// import_from_file


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
