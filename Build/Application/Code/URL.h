/*!	\file URL.h
	\brief Functions related to Uniform Resource Locators.
	
	Note that the Quills interface contains routines such
	as Quills::Session::handle_url() that should be used
	instead of directly using this module, in most cases.
	That way, the script-based implementations of URL
	handlers will be respected (i.e. taking precedence
	over Launch Services).
*/
/*###############################################################

	MacTelnet
		© 1998-2007 by Kevin Grant.
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

#ifndef __URL__
#define __URL__

// MacTelnet includes
#include "InternetPrefs.h"
#include "TerminalScreenRef.typedef.h"
#include "TerminalView.h"



#pragma mark Constants

/*!
Internet locations that are special to MacTelnet.

*/
enum URL_InternetLocation
{
	kURL_InternetLocationApplicationHomePage		= '.com',	//!< home page of this program
	kURL_InternetLocationApplicationSupportEMail	= 'Mail',	//!< E-mail address for support
	kURL_InternetLocationApplicationUpdatesPage		= 'Updt',	//!< information on updates for this version
	kURL_InternetLocationSourceCodeLicense			= 'CGPL',	//!< GNU General Public License
	kURL_InternetLocationSourceForgeProject			= 'Proj'	//!< SourceForge project page
};



#pragma mark Public Methods

void
	URL_HandleForScreenView				(TerminalScreenRef				inScreen,
										 TerminalViewRef				inView);

OSStatus
	URL_MakeSSHResourceLocator			(CFStringRef&					outNewURLString,
										 char const*					inHostName,
										 char const*					inPortStringOrNull = nullptr,
										 char const*					inUserNameOrNull = nullptr);

OSStatus
	URL_MakeTelnetResourceLocator		(CFStringRef&					outNewURLString,
										 char const*					inHostName,
										 char const*					inPortStringOrNull = nullptr,
										 char const*					inUserNameOrNull = nullptr);

Boolean
	URL_OpenInternetLocation			(URL_InternetLocation			inSpecialInternetLocationToOpen);

OSStatus
	URL_ParseCFString					(CFStringRef					inURLCFString);

UniformResourceLocatorType
	URL_ReturnTypeFromCFString			(CFStringRef					inURLCFString);

UniformResourceLocatorType
	URL_ReturnTypeFromCharacterRange	(char const*					inBegin,
										 char const*					inPastEnd);

UniformResourceLocatorType
	URL_ReturnTypeFromDataHandle		(Handle							inURLDataHandle);

Boolean
	URL_SendGetURLEventToSelf			(void const*					inDataPtr,
										 Size							inDataSize,
										 AEDesc*						outReplyPtrOrNull);

Boolean
	URL_SetSelectionToProximalURL		(TerminalView_Cell const&		inCommandClickColumnRow,
										 TerminalScreenRef				inScreen,
										 TerminalViewRef				inView);

Boolean
	URL_TypeIsHandledByAppItself		(UniformResourceLocatorType		inType);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
