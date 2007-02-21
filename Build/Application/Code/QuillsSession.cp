/*###############################################################

	QuillsSession.cp
	
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
#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

// library includes
#include <Console.h>

// MacTelnet includes
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

typedef std::pair< Quills::FunctionReturnVoidArg1VoidPtrArg2CharPtr, void* >	MyURLHandlerPythonObjectPair;
typedef std::map< std::string, MyURLHandlerPythonObjectPair >					MyURLHandlerPythonObjectPairBySchema;


} // anonymous namespace

#pragma mark Variables
namespace {

Quills::FunctionReturnVoidArg1VoidPtr	gSessionOpenedCallbackInvoker = nullptr;
void*									gSessionOpenedPythonCallback = nullptr;
MyURLHandlerPythonObjectPairBySchema&	gURLOpenCallbackInvokerPythonObjectPairsBySchema ()
										{
											static MyURLHandlerPythonObjectPairBySchema x; return x;
										}


} // anonymous namespace



#pragma mark Public Methods
namespace Quills {


/*!
See header or "pydoc" for Python docstrings.

(3.1)
*/
Session::
Session		(std::vector< std::string >		inArgV)
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
															nullptr/* preferences context */);
	delete [] args;
	
	if (nullptr != gSessionOpenedCallbackInvoker) (*gSessionOpenedCallbackInvoker)(gSessionOpenedPythonCallback);
}// default constructor


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
			// may do not distinguish mutable and immutable strings
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


#pragma mark Internal Methods

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
