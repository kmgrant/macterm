/*!	\file CoreSuiteAE.h
	\brief Event handlers for the Core Suite, including
	specifically the following events: "count", "make",
	"duplicate", "close", "delete", "move" and "save".
*/
/*###############################################################

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

#ifndef __CORESUITEAE__
#define __CORESUITEAE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

pascal OSErr
	CoreSuiteAE_Close						(AppleEvent const*		inAppleEventPtr,
											 AppleEvent*			outReplyAppleEventPtr, 
											 SInt32					inData);

pascal OSErr
	CoreSuiteAE_CreateElement				(AppleEvent const*		inAppleEventPtr,
											 AppleEvent*			outReplyAppleEventPtr, 
											 SInt32					inData);

pascal OSErr
	CoreSuiteAE_CountElements				(AppleEvent const*		inAppleEventPtr,
											 AppleEvent*			outReplyAppleEventPtr, 
											 SInt32					inData);

// THE INSTALLED COUNTING PROCEDURE, PART OF THE "whose" IMPLEMENTATION REQUIREMENTS
pascal OSErr
	CoreSuiteAE_CountElementsOfContainer	(DescType				inTypeOfCountedElements,
											 DescType				inContainerClass,
											 AEDesc const*			inContainerTokenDescriptor,
											 SInt32*				outCount);

pascal OSErr
	CoreSuiteAE_Delete						(AppleEvent const*		inAppleEventPtr,
											 AppleEvent*			outReplyAppleEventPtr, 
											 SInt32					inData);

pascal OSErr
	CoreSuiteAE_Duplicate					(AppleEvent const*		inAppleEventPtr,
											 AppleEvent*			outReplyAppleEventPtr, 
											 SInt32					inData);

pascal OSErr
	CoreSuiteAE_Move						(AppleEvent const*		inAppleEventPtr,
											 AppleEvent*			outReplyAppleEventPtr, 
											 SInt32					inData);

pascal OSErr
	CoreSuiteAE_Save						(AppleEvent const*		inAppleEventPtr,
											 AppleEvent*			outReplyAppleEventPtr, 
											 SInt32					inData);

pascal OSErr
	CoreSuiteAE_SelectObject				(AppleEvent const*		inAppleEventPtr,
											 AppleEvent*			outReplyAppleEventPtr, 
											 SInt32					inData);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
