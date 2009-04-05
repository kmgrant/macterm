/*###############################################################

	HiddenAE.cp
	
	MacTelnet
		© 1998-2006 by Kevin Grant.
		© 2001-2002 by Ian Anderson.
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>

// library includes
#include <Console.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// resource includes
#include "MenuResources.h"

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "Commands.h"
#include "ConstantsRegistry.h"
#include "HiddenAE.h"
#include "PrefsWindow.h"
#include "Terminology.h"



#pragma mark Public Methods

/*!
Handles the following event types:

kMySuite
--------
	kTelnetHiddenEventIDConsoleWriteLine

(3.0)
*/
pascal OSErr
HiddenAE_HandleConsoleWriteLine		(AppleEvent const*	inAppleEventPtr,
									 AppleEvent*		UNUSED_ARGUMENT(outReplyAppleEventPtr),
									 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr		result = noErr;
	DescType	returnedType = '----';
	Size		actualSize = 0L;
	char		buffer[256];
	
	
	result = AEGetParamPtr(inAppleEventPtr, keyDirectObject, cStringClass, &returnedType, buffer + 1,
							sizeof(buffer) - 1 * sizeof(char), &actualSize);
	buffer[0] = actualSize / sizeof(UInt8);
	StringUtilities_PToCInPlace((StringPtr)buffer);
	if (result == noErr)
	{
		// check for missing parameters
		result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
		if (result == noErr)
		{
			Console_WriteLine(buffer);
		}
	}
	
	if (result != noErr) Sound_StandardAlert();
	return result;
}// HandleConsoleWriteLine


/*!
Handles the following event types:

kMySuite
--------
	kTelnetHiddenEventIDDisplayDialog

(3.0)
*/
pascal OSErr
HiddenAE_HandleDisplayDialog	(AppleEvent const*	inAppleEventPtr,
								 AppleEvent*		UNUSED_ARGUMENT(outReplyAppleEventPtr),
								 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr			result = noErr;
	DescType		returnedType = '----';
	Size			actualSize = 0L;
	FourCharCode	dialogCode = '----';
	
	
	result = AEGetParamPtr(inAppleEventPtr, keyDirectObject, typeEnumerated, &returnedType,
								&dialogCode, sizeof(dialogCode), &actualSize);
	if (result == noErr)
	{
		// check for missing parameters
		result = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
		if (result == noErr)
		{
			switch (dialogCode)
			{
			case 'news':
				(Boolean)Commands_ExecuteByID(kCommandNewSessionDialog);
				break;
			
			case 'pref':
				PrefsWindow_Display();
				break;
			
			default:
				result = errAEEventNotHandled;
				break;
			}
		}
	}
	
	if (result != noErr) Sound_StandardAlert();
	return result;
}// HandleDisplayDialog

// BELOW IS REQUIRED NEWLINE TO END FILE
