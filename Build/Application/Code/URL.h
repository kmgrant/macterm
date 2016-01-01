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

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#include <UniversalDefines.h>

#ifndef __URL__
#define __URL__

// application includes
#include "TerminalScreenRef.typedef.h"
#include "TerminalViewRef.typedef.h"



#pragma mark Constants

/*!
Internet locations that are special to MacTerm.
*/
enum URL_InternetLocation
{
	kURL_InternetLocationApplicationHomePage		= '.com',	//!< home page of this program
	kURL_InternetLocationApplicationSupportEMail	= 'Mail',	//!< E-mail address for support
	kURL_InternetLocationApplicationUpdatesPage		= 'Updt',	//!< information on updates for this version
	kURL_InternetLocationSourceCodeLicense			= 'CGPL',	//!< GNU General Public License
	kURL_InternetLocationProjectPage				= 'Proj'	//!< project page, for downloading source code
};

enum URL_Type
{
	// must be consecutive and zero-based; do not change this without
	// looking at the order of internal variables in "URL.cp"
	kURL_TypeInvalid,
	kURL_TypeFile,
	kURL_TypeFinger,
	kURL_TypeFTP,
	kURL_TypeGopher,
	kURL_TypeHTTP,
	kURL_TypeHTTPS,
	kURL_TypeMailTo,
	kURL_TypeNews,
	kURL_TypeNNTP,
	kURL_TypeRLogIn,
	kURL_TypeTelnet,
	kURL_TypeTN3270,
	kURL_TypeQuickTime,
	kURL_TypeSFTP,
	kURL_TypeSSH,
	kURL_TypeWAIS,
	kURL_TypeWhoIs,
	kURL_TypeXManPage
};



#pragma mark Public Methods

void
	URL_HandleForScreenView				(TerminalScreenRef				inScreen,
										 TerminalViewRef				inView);

Boolean
	URL_OpenInternetLocation			(URL_InternetLocation			inSpecialInternetLocationToOpen);

OSStatus
	URL_ParseCFString					(CFStringRef					inURLCFString);

URL_Type
	URL_ReturnTypeFromCFString			(CFStringRef					inURLCFString);

URL_Type
	URL_ReturnTypeFromCharacterRange	(char const*					inBegin,
										 char const*					inPastEnd);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
