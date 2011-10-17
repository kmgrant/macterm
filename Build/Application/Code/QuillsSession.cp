/*!	\file QuillsSession.cp
	\brief Session APIs exposed to scripting languages.
*/
/*###############################################################

	MacTerm
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
#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// library includes
#include <CFRetainRelease.h>
#include <Console.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// application includes
#include "AppResources.h"
#include "SessionFactory.h"
#include "QuillsSession.h"
#include "URL.h"



#pragma mark Types
namespace {

struct create_cfstr:
public std::unary_function< std::string, CFStringRef >
{
	CFStringRef
	operator ()	(std::string const&		inString)
	{
		return CFStringCreateWithCString(kCFAllocatorDefault, inString.c_str(), kCFStringEncodingUTF8);
	}
};

typedef std::pair< Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, void* >	MyFileHandlerPythonObjectPair;
typedef std::map< std::string, MyFileHandlerPythonObjectPair >					MyFileHandlerPythonObjectPairByExtension;

typedef std::pair< Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, void* >	MyURLHandlerPythonObjectPair;
typedef std::map< std::string, MyURLHandlerPythonObjectPair >					MyURLHandlerPythonObjectPairBySchema;


} // anonymous namespace

#pragma mark Variables
namespace {

Quills::FunctionReturnStringByLongArg1VoidPtrArg2LongVector		gProcessWorkingDirCallbackInvoker = nullptr;
void*															gProcessWorkingDirPythonCallback = nullptr;
Quills::FunctionReturnVoidArg1VoidPtr							gSessionOpenedCallbackInvoker = nullptr;
void*															gSessionOpenedPythonCallback = nullptr;
MyFileHandlerPythonObjectPairByExtension&						gFileOpenCallbackInvokerPythonObjectPairsByExtension ()
																{
																	static MyFileHandlerPythonObjectPairByExtension x; return x;
																}
MyURLHandlerPythonObjectPairBySchema&							gURLOpenCallbackInvokerPythonObjectPairsBySchema ()
																{
																	static MyURLHandlerPythonObjectPairBySchema x; return x;
																}
std::string														gKeepAliveText(" "); // note; to override default, use Python

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
	try
	{
		// create a nullptr-terminated array of strings
		std::vector< std::string >::size_type const		argc = inArgV.size();
		void const**									args = new void const* [argc];
		CFRetainRelease									argsObject;
		CFRetainRelease									dirObject(CFStringCreateWithCString(kCFAllocatorDefault,
																							inWorkingDirectory.c_str(),
																							kCFStringEncodingUTF8),
																	true/* is retained */);
		TerminalWindowRef								terminalWindow = SessionFactory_NewTerminalWindowUserFavorite();
		Preferences_ContextRef							workspaceContext = nullptr;
		
		
		std::transform(inArgV.begin(), inArgV.end(), args, create_cfstr());
		argsObject.setCFTypeRef(CFArrayCreate(kCFAllocatorDefault, args, argc, &kCFTypeArrayCallBacks), true/* is retained */);
		_session = SessionFactory_NewSessionArbitraryCommand(terminalWindow, argsObject.returnCFArrayRef()/* command */,
																nullptr/* preferences context */, false/* reconfigure terminal */,
																workspaceContext, 0/* window index */,
																dirObject.returnCFStringRef());
		
		// WARNING: this cleanup is not exception-safe and should be fixed
		delete [] args;
		
		if (nullptr != gSessionOpenedCallbackInvoker) (*gSessionOpenedCallbackInvoker)(gSessionOpenedPythonCallback);
	}
	catch (...)
	{
		// WARNING: The constructors for objects that are exposed to scripts
		// cannot throw exceptions, because ownership rules imply an object
		// is always created and released.  Instead, the object “exists” in
		// a state that simply triggers exceptions when certain methods are
		// later called on the object.  This also helps in cases where an
		// object may asynchronously become invalid (e.g. a connection dying
		// when a script still holds a reference).
		Console_Warning(Console_WriteLine, "unexpected exception in Session constructor");
		_session = nullptr;
	}
}// default constructor


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
std::string
Session::pseudo_terminal_device_name ()
{
	std::string		result;
	
	
	if (nullptr == _session)
	{
		throw std::runtime_error("specified session does not have this information");
	}
	else
	{
		CFStringRef		deviceNameCFString = Session_ReturnPseudoTerminalDeviceNameCFString(_session);
		
		
		StringUtilities_CFToUTF8(deviceNameCFString, result);
	}
	return result;
}// pseudo_terminal_device_name


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
std::string
Session::resource_location_string ()
{
	std::string		result;
	
	
	if (nullptr == _session)
	{
		throw std::runtime_error("specified session does not have this information");
	}
	else
	{
		CFStringRef		resourceLocationCFString = Session_ReturnResourceLocationCFString(_session);
		
		
		StringUtilities_CFToUTF8(resourceLocationCFString, result);
	}
	return result;
}// resource_location_string


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
std::string
Session::state_string ()
{
	std::string		result;
	
	
	if (nullptr == _session)
	{
		throw std::runtime_error("specified session does not have this information");
	}
	else
	{
		CFStringRef		stateCFString = nullptr;
		Session_Result	sessionResult = Session_GetStateString(_session, stateCFString);
		
		
		if (sessionResult.ok())
		{
			StringUtilities_CFToUTF8(stateCFString, result);
		}
	}
	return result;
}// state_string


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
			OSStatus			error = noErr;
			
			
			error = FSPathMakeRef(REINTERPRET_CAST(inPathname.c_str(), UInt8 const*), &fileRef, nullptr/* is a directory */);
			if (noErr == error)
			{
				error = LSCopyItemInfoForRef(&fileRef, kLSRequestTypeCreator, &fileInfo);
				if (noErr == error)
				{
					HFSUniStr255	nameBuffer;
					
					
					error = FSGetCatalogInfo(&fileRef, kFSCatInfoNone, nullptr/* catalog info */,
												&nameBuffer, nullptr/* spec record */, nullptr/* parent directory */);
				}
			}
			
			if ((fileInfo.creator == AppResources_ReturnCreatorCode() &&
					fileInfo.filetype == AppResources_ReturnFileTypeForSessionDescriptions()) ||
					(extensionName == "session"))
			{
				// read a configuration set
				SessionDescription_ReadFromFile(fileRef);
			}
			else if ((//fileInfo.creator == AppResources_ReturnCreatorCode() &&
							fileInfo.filetype == AppResources_ReturnFileTypeForMacroSets()) ||
						(extensionName == "macros"))
			{
				// UNIMPLEMENTED - import macros from text
			}
			else if (extensionName == "term")
			{
				// it appears to be a Terminal XML property list file; parse it
				TerminalWindowRef		terminalWindow = SessionFactory_NewTerminalWindowUserFavorite();
				Preferences_ContextRef	workspaceContext = nullptr;
				
				
				SessionFactory_NewSessionFromTerminalFile(terminalWindow, inPathname.c_str(), workspaceContext,
															0/* window index */);
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

(4.0)
*/
std::map< long, std::string >
Session::pids_cwds		(const std::vector< long >&		inProcessIDs)
{
	std::map< long, std::string >	result;
	
	
	if (nullptr != gProcessWorkingDirCallbackInvoker)
	{
		result = gProcessWorkingDirCallbackInvoker(gProcessWorkingDirPythonCallback, inProcessIDs);
	}
	return result;
}// pids_cwds


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

(4.0)
*/
void
Session::_on_seekpidscwds_call_py	(FunctionReturnStringByLongArg1VoidPtrArg2LongVector	inRoutine,
									 void*													inPythonFunctionObject)
{
	gProcessWorkingDirCallbackInvoker = inRoutine;
	gProcessWorkingDirPythonCallback = inPythonFunctionObject;
}// _on_seekpidscwds_call_py


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
