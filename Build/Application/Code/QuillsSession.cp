/*###############################################################

	QuillsSession.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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
#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

// library includes
#include <Console.h>
#include <SoundSystem.h>

// resource includes
#include "GeneralResources.h"

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "AppResources.h"
#include "MacroManager.h"
#include "SessionFactory.h"
#include "QuillsSession.h"
#include "URL.h"



#pragma mark Types
namespace {

struct find_cstr:
public std::unary_function< std::string, char const* >
{
	char const*
	operator ()	(std::string const&		inString)
	{
		return inString.c_str();
	}
};

typedef std::pair< Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, void* >	MyFileHandlerPythonObjectPair;
typedef std::map< std::string, MyFileHandlerPythonObjectPair >					MyFileHandlerPythonObjectPairByExtension;

typedef std::pair< Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, void* >	MyURLHandlerPythonObjectPair;
typedef std::map< std::string, MyURLHandlerPythonObjectPair >					MyURLHandlerPythonObjectPairBySchema;


} // anonymous namespace

#pragma mark Variables
namespace {

Quills::FunctionReturnVoidArg1VoidPtr		gSessionOpenedCallbackInvoker = nullptr;
void*										gSessionOpenedPythonCallback = nullptr;
MyFileHandlerPythonObjectPairByExtension&	gFileOpenCallbackInvokerPythonObjectPairsByExtension ()
											{
												static MyFileHandlerPythonObjectPairByExtension x; return x;
											}
MyURLHandlerPythonObjectPairBySchema&		gURLOpenCallbackInvokerPythonObjectPairsBySchema ()
											{
												static MyURLHandlerPythonObjectPairBySchema x; return x;
											}
std::string									gKeepAliveText(" "); // note; to override default, use Python

} // anonymous namespace



#pragma mark Public Methods
namespace Quills {


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
Session::
Session		(std::vector< std::string >		inArgV,
			 std::string					inWorkingDirectory)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
_session(nullptr)
{
	// create a nullptr-terminated array of C strings
	std::vector< std::string >::size_type const		argc = inArgV.size();
	char const**									args = new char const* [1 + argc];
	
	
	std::transform(inArgV.begin(), inArgV.end(), args, find_cstr());
	args[argc] = nullptr;
	_session = SessionFactory_NewSessionArbitraryCommand(nullptr/* terminal window */, args/* command */,
															nullptr/* preferences context */,
															inWorkingDirectory.empty()
																? nullptr
																: inWorkingDirectory.c_str());
	delete [] args;
	
	if (nullptr != gSessionOpenedCallbackInvoker) (*gSessionOpenedCallbackInvoker)(gSessionOpenedPythonCallback);
}// default constructor


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Session::handle_file	(std::string	inPathname)
{
	std::string::size_type const	kIndexOfExtension = inPathname.find_last_of('.');
	
	
	if (inPathname.npos != kIndexOfExtension)
	{
		std::string		extensionName(inPathname.substr(kIndexOfExtension + 1/* skip dot */, inPathname.npos));
		
		
		if (gFileOpenCallbackInvokerPythonObjectPairsByExtension().end() !=
			gFileOpenCallbackInvokerPythonObjectPairsByExtension().find(extensionName))
		{
			// a Python handler has been installed for this schema type
			MyFileHandlerPythonObjectPair const&	pairRef = gFileOpenCallbackInvokerPythonObjectPairsByExtension()[extensionName];
			char*									mutablePathCopy = new char[1 + inPathname.size()];
			
			
			// make a copy of the argument, since scripting languages
			// do not distinguish mutable and immutable strings
			mutablePathCopy[inPathname.size()] = '\0';
			std::copy(inPathname.begin(), inPathname.end(), mutablePathCopy);
			
			// call handler
			(*(pairRef.first))(pairRef.second, mutablePathCopy);
			
			delete [] mutablePathCopy;
		}
		else
		{
			LSItemInfoRecord	fileInfo;
			FSRef				fileRef;
			FSSpec				fileSpec;
			OSStatus			error = noErr;
			
			
			error = FSPathMakeRef(REINTERPRET_CAST(inPathname.c_str(), UInt8 const*), &fileRef, nullptr/* is a directory */);
			if (noErr == error)
			{
				error = LSCopyItemInfoForRef(&fileRef, kLSRequestTypeCreator, &fileInfo);
				if (noErr == error)
				{
					HFSUniStr255	nameBuffer;
					
					
					error = FSGetCatalogInfo(&fileRef, kFSCatInfoNone, nullptr/* catalog info */,
												&nameBuffer, &fileSpec, nullptr/* parent directory */);
				}
			}
			
			if ((fileInfo.creator == 'ToyS' || fileInfo.filetype == 'osas') ||
						(extensionName == ".scpt"))
			{
				// it appears to be a script; run it
				AppleEventUtilities_ExecuteScriptFile(&fileSpec, true/* notify user of errors */);
			}
			else if ((fileInfo.creator == AppResources_ReturnCreatorCode() &&
							fileInfo.filetype == kApplicationFileTypeSessionDescription) ||
						(extensionName == ".session"))
			{
				// read a configuration set
				SessionDescription_ReadFromFile(&fileSpec);
			}
			else if ((//fileInfo.creator == AppResources_ReturnCreatorCode() &&
							fileInfo.filetype == kApplicationFileTypeMacroSet) ||
						(extensionName == ".macros"))
			{
				MacroManager_InvocationMethod		mode = kMacroManager_InvocationMethodCommandDigit;
				
				
				(Boolean)Macros_ImportFromText(Macros_ReturnActiveSet(), &fileSpec, &mode);
				Macros_SetMode(mode);
			}
			else if (extensionName == ".term")
			{
				// it appears to be a Terminal XML property list file; parse it
				SessionFactory_NewSessionFromTerminalFile(nullptr/* existing terminal window to use */,
															inPathname.c_str());
			}
			else
			{
				// no Python handler is installed for this file; use the default handler
				inPathname = "file://" + inPathname;
				
				CFStringRef		urlCFString = CFStringCreateWithCString(kCFAllocatorDefault, inPathname.c_str(),
																		kCFStringEncodingUTF8);
				
				
				if (nullptr != urlCFString)
				{
					// TEMPORARY: could (should?) throw exceptions, translated by SWIG
					// into scripting language exceptions, if an error occurs here
					(OSStatus)URL_ParseCFString(urlCFString);
					CFRelease(urlCFString), urlCFString = nullptr;
				}
			}
		}
	}
}// handle_file


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Session::handle_url		(std::string	inURL)
{
	std::string::size_type const	kIndexOfColon = inURL.find_first_of(':');
	
	
	if (inURL.npos != kIndexOfColon)
	{
		std::string		schemaName(inURL.substr(0, kIndexOfColon));
		
		
		if (gURLOpenCallbackInvokerPythonObjectPairsBySchema().end() !=
			gURLOpenCallbackInvokerPythonObjectPairsBySchema().find(schemaName))
		{
			// a Python handler has been installed for this schema type
			MyURLHandlerPythonObjectPair const&		pairRef = gURLOpenCallbackInvokerPythonObjectPairsBySchema()[schemaName];
			char*									mutableURLCopy = new char[1 + inURL.size()];
			
			
			// make a copy of the argument, since scripting languages
			// do not distinguish mutable and immutable strings
			mutableURLCopy[inURL.size()] = '\0';
			std::copy(inURL.begin(), inURL.end(), mutableURLCopy);
			
			// call handler
			(*(pairRef.first))(pairRef.second, mutableURLCopy);
			
			delete [] mutableURLCopy;
		}
		else
		{
			// no Python handler is installed for this schema type; use the default handler
			CFStringRef		urlCFString = CFStringCreateWithCString(kCFAllocatorDefault, inURL.c_str(),
																	kCFStringEncodingUTF8);
			
			
			if (nullptr != urlCFString)
			{
				// TEMPORARY: could (should?) throw exceptions, translated by SWIG
				// into scripting language exceptions, if an error occurs here
				(OSStatus)URL_ParseCFString(urlCFString);
				CFRelease(urlCFString), urlCFString = nullptr;
			}
		}
	}
}// handle_url


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
std::string
Session::keep_alive_transmission ()
{
	return gKeepAliveText;
}// keep_alive_transmission


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Session::set_keep_alive_transmission	(std::string	inText)
{
	gKeepAliveText = inText;
}// set_keep_alive_transmission


#pragma mark Internal Methods

/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Session::_on_fileopen_ext_call_py	(FunctionReturnVoidArg1VoidPtrArg2CharPtr	inRoutine,
									 void*										inPythonFunctionObject,
									 std::string								inExtension)
{
	// there is one callback for each extension, so add/overwrite only the given extension’s callback
	gFileOpenCallbackInvokerPythonObjectPairsByExtension()[inExtension] = std::make_pair(inRoutine, inPythonFunctionObject);
}// _on_fileopen_ext_call_py


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Session::_on_new_call_py	(FunctionReturnVoidArg1VoidPtr	inRoutine,
							 void*							inPythonFunctionObject)
{
	gSessionOpenedCallbackInvoker = inRoutine;
	gSessionOpenedPythonCallback = inPythonFunctionObject;
}// _on_new_call_py


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Session::_on_urlopen_call_py	(FunctionReturnVoidArg1VoidPtrArg2CharPtr	inRoutine,
								 void*										inPythonFunctionObject,
								 std::string								inSchema)
{
	// there is one callback for each schema type, so add/overwrite only the given schema’s callback
	gURLOpenCallbackInvokerPythonObjectPairsBySchema()[inSchema] = std::make_pair(inRoutine, inPythonFunctionObject);
}// _on_urlopen_call_py


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Session::_stop_fileopen_ext_call_py	(FunctionReturnVoidArg1VoidPtrArg2CharPtr	inRoutine,
									 std::string								inExtension)
{
	// there is one callback for each schema type, so delete only the given schema’s callback
	if (gFileOpenCallbackInvokerPythonObjectPairsByExtension()[inExtension].first == inRoutine)
	{
		gFileOpenCallbackInvokerPythonObjectPairsByExtension().erase(inExtension);
	}
}// _stop_fileopen_ext_call_py


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Session::_stop_new_call_py	(FunctionReturnVoidArg1VoidPtr	inRoutine)
{
	if (gSessionOpenedCallbackInvoker == inRoutine)
	{
		gSessionOpenedCallbackInvoker = nullptr;
		gSessionOpenedPythonCallback = nullptr;
	}
}// _stop_new_call_py


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
void
Session::_stop_urlopen_call_py	(FunctionReturnVoidArg1VoidPtrArg2CharPtr	inRoutine,
								 std::string								inSchema)
{
	// there is one callback for each schema type, so delete only the given schema’s callback
	if (gURLOpenCallbackInvokerPythonObjectPairsBySchema()[inSchema].first == inRoutine)
	{
		gURLOpenCallbackInvokerPythonObjectPairsBySchema().erase(inSchema);
	}
}// _stop_urlopen_call_py


} // namespace Quills

// BELOW IS REQUIRED NEWLINE TO END FILE
