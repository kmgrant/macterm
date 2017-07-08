/*!	\file RecordAE.h
	\brief Detects events and records them into scripts.
	
	This module is new for Mac OS X and basically streamlines
	recording of events into scripts!  Now, all recording is
	done in one place: as events take place this module finds
	out about them, arranges for them to be recorded and then
	passes them on to other Carbon Event handlers.
	
	When you initialize this module, it installs event handlers
	to tell when recording starts and stops.  While recording,
	it installs various application-level event handlers for
	(at least) basic events like windows being closed, etc.
*/
/*###############################################################
	
	MacTerm
		© 1998-2017 by Kevin Grant.
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
#include <ApplicationServices/ApplicationServices.h>



#pragma mark Constants

enum RecordAE_Result
{
	kRecordAE_ResultOK					= 0,	//!< initialization succeeded
	kRecordAE_ResultCannotInitialize	= 1		//!< there is a problem registering Apple Events
};



#pragma mark Public Methods

//!\name Initialization
//@{

RecordAE_Result
	RecordAE_Init							();

void
	RecordAE_Done							();

//@}

//!\name Utilities
//@{

OSStatus
	RecordAE_CreateRecordableAppleEvent		(DescType			inEventClass,
											 DescType			inEventID,
											 AppleEvent*		outAppleEventPtr);

AEAddressDesc const*
	RecordAE_ReturnSelfAddress				();

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
