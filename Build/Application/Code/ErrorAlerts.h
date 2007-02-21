/*!	\file ErrorAlerts.h
	\brief Handles display of user interfaces to notify the user
	of errors in typical formats.
	
	More specialized needs can use the parameterized call
	ErrorAlerts_DisplayParameterizedAlert(), or the Alert module
	directly.
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

#ifndef __ERRORALERTS__
#define __ERRORALERTS__

// Mac includes
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "ConstantsRegistry.h"
#include "UIStrings.h"



#pragma mark Constants

typedef SInt32 InternalTelnetMemoryError;
enum
{
	// these are used to help identify where memory errors take place
	kInternalTelnetMemoryErrorClipboardTextToScrap = 1,
	kInternalTelnetMemoryErrorConnectionOpenFromMenuItem = 2,
	kInternalTelnetMemoryErrorDNRLookup = 3,
	kInternalTelnetMemoryErrorRealScreenCreation = 4,
	kInternalTelnetMemoryErrorSafeArrayCreation = 5,
	kInternalTelnetMemoryErrorVirtualScreenResize = 6
};

#pragma mark Types

struct TelnetAlertParameterBlock
{
	struct
	{
		ConstStringPtr	title1,
						title2,
						title3;	// OK, Cancel and Other buttons; invalid strings invoke defaults
		ConstStringPtr	key1,
						key2,
						key3;	// only the first character of these strings is considered
	} buttons;
	
	struct
	{
		UInt32		timeout;	// number of SECONDS before the alert disappears automatically
		AlertType	type;
	} dialog;
	
	struct
	{
		Boolean		alertSound,
					helpButton,
					networkDisruption,
					notTitled,
					speech;
	} flags;
	
	struct
	{
		ConstStringPtr	large,		// REQUIRED parameter
						small;
		CFStringRef		titleCFString;
	} text;
};
typedef struct TelnetAlertParameterBlock*			TelnetAlertParameterBlockPtr;
typedef struct TelnetAlertParameterBlock const*		TelnetAlertParameterBlockConstPtr;



#pragma mark Public Methods

void
	ErrorAlerts_Init						();

void
	ErrorAlerts_Done						();

void
	ErrorAlerts_DisplayNoteMessage			(SInt16									inStringListResourceID,
											 UInt16									inStringIndex,
											 UIStrings_AlertWindowCFString			inTitleStringID);

void
	ErrorAlerts_DisplayNoteMessageNoTitle	(SInt16									inStringListResourceID,
											 UInt16									inStringIndex);

SInt16
	ErrorAlerts_DisplayParameterizedAlert	(TelnetAlertParameterBlockConstPtr		inParameters);

void
	ErrorAlerts_DisplayStopMessage			(SInt16									inStringListResourceID,
											 UInt16									inStringIndex,
											 UIStrings_AlertWindowCFString			inTitleStringID);

void
	ErrorAlerts_DisplayStopMessageNoTitle	(SInt16									inStringListResourceID,
											 UInt16									inStringIndex);

void
	ErrorAlerts_DisplayStopQuitMessage		(SInt16									inStringListResourceID,
											 UInt16									inStringIndex,
											 UIStrings_AlertWindowCFString			inTitleStringID);

void
	ErrorAlerts_OperationFailed				(short									inMessageID,
											 short									inInternalID,
											 OSStatus								inOSID);

void
	ErrorAlerts_OutOfMemory					(InternalTelnetMemoryError				inInternalID);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
