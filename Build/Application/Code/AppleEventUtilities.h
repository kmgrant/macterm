/*!	\file AppleEventUtilities.h
	\brief Useful routines for firing off Apple Events.
	
	Much of this is based on the AppleScript 1.3 SDK, developed
	by people at Apple Computer, Inc.  There are also routines
	that assist event handlers with Carbon compatibility.
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

#ifndef __APPLEEVENTUTILITIES__
#define __APPLEEVENTUTILITIES__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

OSStatus
	AppleEventUtilities_AddErrorToReply				(ConstStringPtr			inErrorMessageOrNull,
													 OSStatus				inError,
													 AppleEventPtr			inoutReplyAppleEventPtr);

OSStatus
	AppleEventUtilities_AddResultToReply			(AppleEventPtr			inResultAppleEventPtr,
													 AppleEventPtr			inoutReplyAppleEventPtr,
													 OSStatus				inError);

// USE THIS INSTEAD OF DEREFERENCING AN AEDesc DATA HANDLE, FOR MAXIMUM CARBON COMPATIBILITY
OSStatus
	AppleEventUtilities_CopyDescriptorData			(AEDesc const*			inDescriptorPtr,
													 void*					outDataPtr,
													 Size 					inDataPtrMaximumSize,
													 Size*					outActualSizePtr);

// USE THIS INSTEAD OF THE ABOVE IF YOU WANT AUTOMATIC COERCION TO A SPECIFIC TYPE
OSStatus
	AppleEventUtilities_CopyDescriptorDataAs		(DescType				inDesiredType,
													 AEDesc const*			inDescriptorPtr,
													 void*					outDataPtr,
													 Size 					inDataPtrMaximumSize,
													 Size*					outActualSizePtr);

OSStatus
	AppleEventUtilities_DuplicateOrResolveObject	(AEDesc const*			inDescriptor,
													 AEDesc*				outDuplicate);

OSStatus
	AppleEventUtilities_ExecuteScript				(Handle					inScriptData,
													 Boolean				inNotifyUser);

OSStatus
	AppleEventUtilities_ExecuteScriptFile			(FSSpec const*			inFile,
													 Boolean				inNotifyUser);

// USE THIS INSTEAD OF OTHER METHODS FOR INITIALIZING AEDesc STRUCTURES TO typeNull WITH nullptr DATA
OSStatus
	AppleEventUtilities_InitAEDesc					(AEDesc*				inoutDescriptorPtr);

// WHEN AN EVENT HAS AN OPTIONAL RESULT, USE THIS ROUTINE TO DETERMINE IF THE USER PROVIDED ONE
Boolean
	AppleEventUtilities_IsValidReply				(AppleEventPtr			inAppleEventPtr);

pascal OSErr
	AppleEventUtilities_ObjectExists				(AppleEvent const*		inAppleEventPtr,
													 AppleEvent*			outReplyAppleEventPtr, 
													 SInt32					inData);

OSErr
	AppleEventUtilities_RequiredParametersError		(AppleEvent const*		inAppleEventPtr);

Boolean
	AppleEventUtilities_RetrieveReplyError			(AppleEventPtr			inReplyAppleEventPtr,
													 OSStatus*				outErrorPtr);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
