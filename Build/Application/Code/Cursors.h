/*!	\file Cursors.h
	\brief Changes the mouse cursor in a way that allows you to
	revert to the previous cursor at any time.
	
	This module transparently takes advantage of the Mac OS 8.5
	Appearance Manager cursors when available, and uses its own
	cursors otherwise (both color and black-and-white).  So
	regardless of the OS you are running, you can use this
	module to get the best cursors available.
	
	If you consistently use this module to manage the mouse
	cursor, you can reliably determine the current shape of the
	cursor and revert back to it as needed, significantly
	reducing the complexity of cursor-handling code.  That’s
	because each cursor-setting routine returns the ID of the
	cursor that was last set using this module.
	
	Cursors are only loaded as needed, and the same cursor is
	never loaded more than once.
*/
/*###############################################################

	Interface Library 1.3
	© 1998-2006 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "UniversalDefines.h"

#ifndef __CURSORS__
#define __CURSORS__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

enum
{
	kCursor_Arrow = 0, // dummy ID for reference purposes
	kCursor_IBeam = 1,
	kCursor_Crosshairs = 2,
	kCursor_Plus = 3,
	kCursor_Watch = 4,
	kCursor_ArrowHelp = 150,
	kCursor_ArrowCopy = 151,
	kCursor_ArrowWatch = 152,
	kCursor_ArrowContextualMenu = 153,
	kCursor_HandOpen = 154,
	kCursor_HandGrab = 155,
	kCursor_HandPoint = 156,
	kCursor_Poof = 200,
	kCursor_Disallow = 201
};



#pragma mark Public Methods

//!\name Initialization
//@{

void
	Cursors_Init						();

void
	Cursors_Done						();

//@}

//!\name Determining If Appearance 1.1 Cursors Are Available
//@{

Boolean
	Cursors_AppearanceCursorsAvail		();

//@}

//!\name Changing the Cursor, Determining the Previous Cursor
//@{

SInt16
	Cursors_DeferredUseWatch			(UInt32				inForHowManyTicks);

SInt16
	Cursors_GetCurrent					();

void
	Cursors_Idle						();

SInt16
	Cursors_Use							(SInt16				inCursorID);

SInt16
	Cursors_UseArrow					();

SInt16
	Cursors_UseArrowContextualMenu		();

SInt16
	Cursors_UseArrowCopy				();

SInt16
	Cursors_UseArrowHelp				();

SInt16
	Cursors_UseArrowWatch				();

SInt16
	Cursors_UseCrosshairs				();

SInt16
	Cursors_UseDisallow					();

SInt16
	Cursors_UseHandGrab					();

SInt16
	Cursors_UseHandOpen					();

SInt16
	Cursors_UseHandPoint				();

SInt16
	Cursors_UseMagnifyingGlass			();

SInt16
	Cursors_UseIBeam					();

SInt16
	Cursors_UsePlus						();

SInt16
	Cursors_UsePoof						();

SInt16
	Cursors_UseWatch					();

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
