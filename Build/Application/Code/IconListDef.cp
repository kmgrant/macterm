/*###############################################################

	IconListDef.c
	
	A custom list definition procedure displaying icons with 
	text labels.  Works with Icon Services, when available.
	Note - the list definition itself doesn’t increment or 
	decrement the reference count of IconRefs.  This file has 
	been slightly modified for MacTelnet (see subsequent 
	comments).
	
	Icon List Definition 2.5
		© 2001-2002 by Ian Anderson and Kevin Grant.
	
	This module is free software; you can redistribute it or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version
	2 of the License, or (at your option) any later version.
	
	This module is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more
	details.
	
	You should have received a copy of the GNU General Public
	License along with this module; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

//#include "IconListDef.h"

// MacTelnet includes
// SPECIAL WARNING:  Currently this file is being cross-compiled 
// as a Classic PPC 'LDEF' code resource as well as a regular 
// Carbon C source file.  As long as this is the case, don’t use 
// any functions from any non-system, non-shared library, or 
// another source file (or the LDEF will become unreasonably 
// large).
#include "UniversalDefines.h" // defines accessor macros

#pragma mark Constants

enum
{
	kIconWidth = 32,		// pixels
	kIconHeight = 32,		// pixels
	kTextHeight = 14,		// pixels - an estimate, depends on font metrics
	kIconTextSpacingV = 2,	// pixels
	kContentHeight = kIconHeight + kTextHeight + kIconTextSpacingV
};

// internal methods - variable names changed to conform with MacTelnet coding style
static void	calculateDrawingBounds		(Rect const*					inCellBoundsPtr,
										 Rect*							outIconRect,
										 Rect*							outTextRect);
static void	drawIconListCell			(ListHandle						inList,
										 Rect const*					inCellBoundsPtr,
										 Boolean						inUseIconServices,
										 StandardIconListCellDataPtr	inCellDataPtr,
										 Boolean						inIsCellSelected);

// external methods
pascal void
	IconListDef		(short			inMessage,
					 Boolean		inIsCellSelected,
					 Rect*			inCellBoundsPtr,
					 Cell			inAffectedCell,
					 short			inDataOffset,
					 short			inDataSizeInBytes,
					 ListHandle		inList);



/*!
The main entry point to the list definition procedure, of 
standard ListDefProc form.

(2.0)
*/
pascal void
IconListDef		(short			inMessage,
				 Boolean		inIsCellSelected,
				 Rect*			inCellBoundsPtr,
				 Cell			UNUSED_ARGUMENT_CLASSIC(inAffectedCell),
				 short			UNUSED_ARGUMENT_CARBON(inDataOffset),
				 short			inDataSizeInBytes,
				 ListHandle		inList)
{
	static Boolean	useIconServices = false;
	
	
	switch (inMessage)
	{
		case lInitMsg:
			{
				OSStatus	error = noErr;
				long		gestaltResult = 0L;
				
				
				error = Gestalt(gestaltIconUtilitiesAttr, &gestaltResult);
				useIconServices = ((error == noErr) && ((gestaltResult & 
									(1L << gestaltIconUtilitiesHasIconServices)) != 0));
			}
			break;
		
		// Drawing and highlighting are put together because of 
		// text smoothing (anti-aliasing).  Simply using the 
		// QuickDraw hilitetransfermode to paint the cell 
		// doesn’t work because the text will be smoothed 
		// according to the background color which would still 
		// be white.  So instead it is necessary to: set the 
		// background color according to whether or not the cell 
		// is highlighted, erase the cell, then draw its 
		// contents for both highlight and draw messages.  An 
		// alternative would be, for lHilite messages, to set 
		// the background color, erase only the region around 
		// the icon, then draw only the text but messing with 
		// all the Regions would probably end up taking more 
		// time so this just erases the entire cell.
		case lDrawMsg:
		case lHiliteMsg:
			// sanity check on buffer size (should always pass)
			if (inDataSizeInBytes == sizeof(StandardIconListCellDataRec))
			{
				StandardIconListCellDataPtr		cellDataPtr = nullptr;
				
				
			#if TARGET_API_MAC_OS8	// (optimized for MacTelnet)
				// simply point cellDataPtr to the correct byte in the cell’s DataHandle
				SInt8	oldHState = '\0';
				
				
				// save handle state for later restoration; lock in case
				// a system call that moves memory is invoked (which could
				// invalidate the pointer if the handle wasn’t locked)
				oldHState = HGetState((**inList).cells);
				HLock((**inList).cells);
				cellDataPtr = REINTERPRET_CAST(*((**inList).cells) + inDataOffset, 
												StandardIconListCellDataPtr);
			#else
				// under Carbon, cell data must always be copied; allocate
				// a buffer that is then freed after drawing the cell
				cellDataPtr = (StandardIconListCellDataPtr)NewPtr(inDataSizeInBytes);
				if (cellDataPtr != nullptr)
				{
					LGetCell(cellDataPtr, &inDataSizeInBytes, inAffectedCell, inList);
				}
			#endif
				if (cellDataPtr != nullptr)
				{
					drawIconListCell(inList, inCellBoundsPtr, useIconServices, cellDataPtr, 
										inIsCellSelected);
				}
			#if TARGET_API_MAC_OS8
				HSetState((**inList).cells, oldHState);
			#else
				DisposePtr(REINTERPRET_CAST(cellDataPtr, Ptr));
			#endif
			}
			break;
	}
}// IconListDef


//
// internal methods
//
#pragma mark -

/*!
Determines the region that the icon and its text label will 
occupy within a particular cell’s boundaries.  Pass nullptr for 
either of the 2nd and 3rd parameters if you are not interested 
in the value.

(2.0)
*/
static void
calculateDrawingBounds	(Rect const*	inCellBoundsPtr,
						 Rect*			outIconRect,
						 Rect*			outTextRect)
{
	short	iconTop;
	
	
	iconTop = inCellBoundsPtr->top + 
				INTEGER_HALVED(inCellBoundsPtr->bottom - inCellBoundsPtr->top - kContentHeight);
	
	if (outIconRect != nullptr)
	{
		short	cellCenterH = INTEGER_HALVED(inCellBoundsPtr->left + inCellBoundsPtr->right);
		
		
		SetRect(outIconRect, cellCenterH - INTEGER_HALVED(kIconWidth), iconTop, 
				cellCenterH + INTEGER_HALVED(kIconWidth), iconTop + kIconHeight);
	}
	
	if (outTextRect != nullptr)
	{
		SetRect(outTextRect, inCellBoundsPtr->left, 
				iconTop + kIconHeight + kIconTextSpacingV, inCellBoundsPtr->right, 
				iconTop + kIconHeight + kIconTextSpacingV + kTextHeight);
	}
}// calculateDrawingBounds


/*!
First checks to see if the specified List Cell is highlighted, 
then sets the background color appropriately, erases the cell (so 
that the new background color is actually drawn), and draws the 
icon and text label.  Because this function correctly sets the 
highlighting, there’s no need for a separate function to do so.

(2.0)
*/
static void
drawIconListCell	(ListHandle						inList,
					 Rect const*					inCellBoundsPtr,
					 Boolean						inUseIconServices,
					 StandardIconListCellDataPtr	inCellDataPtr,
					 Boolean						inIsCellSelected)
{
	GrafPtr		savedPort = nullptr;
	CGrafPtr	listPort = nullptr;
	RGBColor	savedColor;
	Boolean		active = false;
	Rect		iconRect;
	Rect		textRect;
	
	
	GetPort(&savedPort);
	listPort = GetListPort(inList);
	SetPort((GrafPtr)listPort);
	
	GetPortBackColor(listPort, &savedColor);
	
	if (inIsCellSelected)
	{
		// set the background to the highlight color
		RGBColor	highlightColor;
		
		
		LMGetHiliteRGB(&highlightColor);
		RGBBackColor(&highlightColor);
	}
	
	EraseRect(inCellBoundsPtr);
	
	calculateDrawingBounds(inCellBoundsPtr, &iconRect, &textRect);
	active = GetListActive(inList);
	
	if (inUseIconServices)
	{
		// draw the icon using Icon Services, if available
		(OSErr)PlotIconRef(&iconRect, kAlignNone, active ? kTransformNone : kTransformDisabled, 
							kIconServicesNormalUsageFlag,
							REINTERPRET_CAST(inCellDataPtr->iconHandle, IconRef));
	}
	else
	{	// draw the icon using Icon Utilities if Icon Services isn’t available
		(OSErr)PlotIconSuite(&iconRect, kAlignNone, 
								active ? kTransformNone : kTransformDisabled, 
								REINTERPRET_CAST(inCellDataPtr->iconHandle, IconSuiteRef));
	}
	
	// draw label text
	#if TARGET_API_MAC_OS8
	{
		// use TextEdit in Classic
		short	savedFont = 0,
				savedSize = 0;
		Style	savedFace = normal;
		
		
		savedFont = GetPortTextFont(listPort);
		savedFace = GetPortTextFace(listPort);
		savedSize = GetPortTextSize(listPort);
		
		TextFont(inCellDataPtr->font);
		TextFace((Style)(inCellDataPtr->face));
		TextSize(inCellDataPtr->size);
		
		TETextBox(&inCellDataPtr->name[1], inCellDataPtr->name[0], &textRect, teCenter);
		
		TextFont(savedFont);
		TextFace(savedFace);
		TextSize(savedSize);
	}
	#else
	{
		// use Appearance in Carbon (better anti-aliasing on Mac OS X)
		CFStringRef		stringRef = nullptr;
		
		
		stringRef = CFStringCreateWithPascalString(kCFAllocatorDefault, inCellDataPtr->name, 
													CFStringGetSystemEncoding());
		if (stringRef != nullptr)
		{
			(OSStatus)DrawThemeTextBox(stringRef, kThemeSmallSystemFont/*inCellDataPtr->font*/,
										active ? kThemeStateActive : kThemeStateInactive, 
										true/* wrap to width */, &textRect, teCenter,
										nullptr/* context */);
			CFRelease(stringRef);
		}
	}
	#endif
	
	RGBBackColor(&savedColor);
	SetPort(savedPort);
}// drawIconListCell

// BELOW IS REQUIRED NEWLINE TO END FILE
