/*!	\file RequiredAE.h
	\brief Implements the required suite of Apple Events: that
	is, the “open application”, “open documents”,
	“print documents”, “quit” and “appearance switch” events.
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

#ifndef __REQUIREDAE__
#define __REQUIREDAE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

pascal OSErr
	RequiredAE_HandleAppearanceSwitch					(AppleEvent const*	inAppleEventPtr,
														 AppleEvent*		outReplyAppleEventPtr,
														 SInt32				inData);

pascal OSErr
	RequiredAE_HandleApplicationOpen					(AppleEvent const*	inAppleEventPtr,
														 AppleEvent*		outReplyAppleEventPtr,
														 SInt32				inData);

pascal OSErr
	RequiredAE_HandleApplicationPreferences				(AppleEvent const*	inAppleEventPtr,
														 AppleEvent*		outReplyAppleEventPtr,
														 SInt32				inData);

pascal OSErr
	RequiredAE_HandleApplicationReopen					(AppleEvent const*	inAppleEventPtr,
														 AppleEvent*		outReplyAppleEventPtr,
														 SInt32				inData);

pascal OSErr
	RequiredAE_HandleApplicationQuit					(AppleEvent const*	inAppleEventPtr,
														 AppleEvent*		outReplyAppleEventPtr,
														 SInt32				inData);

pascal OSErr
	RequiredAE_HandleOpenDocuments						(AppleEvent const*	inAppleEventPtr,
														 AppleEvent*		outReplyAppleEventPtr,
														 SInt32				inData);

pascal OSErr
	RequiredAE_HandlePrintDocuments						(AppleEvent const*	inAppleEventPtr,
														 AppleEvent*		outReplyAppleEventPtr,
														 SInt32				inData);

// temp - these are MacTelnet Suite event handlers that will move elsewhere
pascal OSErr
	AppleEvents_HandleAlert								(AppleEvent const*	inAppleEventPtr,
														 AppleEvent*		outReplyAppleEventPtr,
														 SInt32				inData);

pascal OSErr
	AppleEvents_HandleCopySelectedText					(AppleEvent const*	inAppleEventPtr,
														 AppleEvent*		outReplyAppleEventPtr,
														 SInt32				inData);

pascal OSErr
	AppleEvents_HandleLaunchFind						(AppleEvent const*	inAppleEventPtr,
														 AppleEvent*		outReplyAppleEventPtr,
														 SInt32				inData);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
