/*!	\file OtherApps.h
	\brief Lists methods that interact with other programs.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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

#pragma once

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <ResultCode.template.h>

// application includes
#include "Preferences.h"



#pragma mark Constants

/*!
Possible return values from OtherApps module routines.
*/
typedef ResultCode< UInt16 >		OtherApps_Result;
OtherApps_Result const	kOtherApps_ResultOK(0);					//!< no error
OtherApps_Result const	kOtherApps_ResultGenericFailure(1);		//!< unspecified error occurred



#pragma mark Public Methods

//!\name Creating Property Lists
//@{

CFPropertyListRef
	OtherApps_NewPropertyListFromFile		(CFURLRef		inFile,
											 CFErrorRef*		outErrorOrNull = nullptr);

//@}

//!\name Importing External Color Schemes
//@{

OtherApps_Result
	OtherApps_ImportITermColors		(CFDictionaryRef				inXMLBasedDictionary,
									 Preferences_ContextRef		inoutFormatToModifyOrNull,
									 CFStringRef					inNameHintOrNull);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
