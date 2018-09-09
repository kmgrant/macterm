/*!	\file RecordAE.cp
	\brief Detects events and records them into scripts.
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

#include "RecordAE.h"
#include <UniversalDefines.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>

// library includes
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <MemoryBlocks.h>

// application includes
#include "AppleEventUtilities.h"
#include "ConstantsRegistry.h"



#pragma mark Variables
namespace {

AEAddressDesc			gSelfAddress;						//!< allows application to be recordable
ProcessSerialNumber		gSelfProcessID;						//!< identifies application in terms of an OS process

} // anonymous namespace



#pragma mark Public Methods

/*!
This method initializes the Apple Event handlers
for the Required Suite, and creates an address
descriptor so MacTerm can send events to itself
(for recordability).

\retval kRecordAE_ResultOK
if all handlers were installed successfully

\retval kRecordAE_ResultCannotInitialize
if any handler failed to install

(3.0)
*/
RecordAE_Result
RecordAE_Init ()
{
	RecordAE_Result		result = kRecordAE_ResultCannotInitialize;
	OSStatus			error = noErr;
	
	
	// Set up a self-addressed, stamped envelope so MacTerm can perform all
	// of its operations by making an end run through the system.  This odd
	// but powerful approach allows recording applications, such as the
	// Script Editor, to automatically write scripts based on what users do
	// in MacTerm!
	gSelfProcessID.highLongOfPSN = 0;
	gSelfProcessID.lowLongOfPSN = kCurrentProcess; // don’t use GetCurrentProcess()!
	error = AECreateDesc(typeProcessSerialNumber, &gSelfProcessID, sizeof(gSelfProcessID), &gSelfAddress);
	
	return result;
}// Init


/*!
Cleans up this module by removing global event
handlers, etc.

(3.0)
*/
void
RecordAE_Done ()
{
	AEDisposeDesc(&gSelfAddress);
}// Done


/*!
Creates an Apple Event using AECreateAppleEvent(), with
the most likely parameters for “send to self” events:
RecordAE_GetSelfAddress(), kAutoGenerateReturnID, and
kAnyTransactionID.

(3.0)
*/
OSStatus
RecordAE_CreateRecordableAppleEvent		(DescType		inEventClass,
										 DescType		inEventID,
										 AppleEvent*	outAppleEventPtr)
{
	OSStatus	result = noErr;
	
	
	result = AECreateAppleEvent(inEventClass, inEventID, RecordAE_ReturnSelfAddress(),
								kAutoGenerateReturnID, kAnyTransactionID, outAppleEventPtr);
	return result;
}// CreateRecordableAppleEvent


/*!
Returns the address of the current application, used
for event routing.  The result will not be valid if
you have not used RecordAE_Init().

(3.0)
*/
AEAddressDesc const*
RecordAE_ReturnSelfAddress ()
{
	return &gSelfAddress;
}// ReturnSelfAddress

// BELOW IS REQUIRED NEWLINE TO END FILE
