/*###############################################################

	DragAndDrop.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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

// standard-C++ includes
#include <map>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CGContextSaveRestore.h>
#include <Console.h>
#include <Cursors.h>
#include <MemoryBlocks.h>
#include <WindowInfo.h>

// MacTelnet includes
#include "CommandLine.h"
#include "ConnectionData.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "DragAndDrop.h"
#include "EventLoop.h"
#include "MacroManager.h"
#include "RegionUtilities.h"



#pragma mark Constants

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	// these colors are based on the drag-and-drop rounded rectangles used in Mail on Tiger
	// UNIMPLEMENTED: put these into an external file such as DefaultPreferences.plist
	RGBColor const		kMy_DragHighlightFrameInColor	 = { 5140, 21074, 56026 };
	RGBColor const		kMy_DragHighlightFrameInGraphite = { 5140, 5140, 5140 };
	RGBColor const		kMy_DragHighlightBoxInColor		 = { 48316, 52685, 61680 };
	RGBColor const		kMy_DragHighlightBoxInGraphite	 = { 52685, 52685, 52685 };
	float const			kMy_DragHighlightFrameInColorRedFraction
							= (float)kMy_DragHighlightFrameInColor.red /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightFrameInColorGreenFraction
							= (float)kMy_DragHighlightFrameInColor.green /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightFrameInColorBlueFraction
							= (float)kMy_DragHighlightFrameInColor.blue /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightFrameInGraphiteRedFraction
							= (float)kMy_DragHighlightFrameInGraphite.red /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightFrameInGraphiteGreenFraction
							= (float)kMy_DragHighlightFrameInGraphite.green /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightFrameInGraphiteBlueFraction
							= (float)kMy_DragHighlightFrameInGraphite.blue /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightBoxInColorRedFraction
							= (float)kMy_DragHighlightBoxInColor.red /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightBoxInColorGreenFraction
							= (float)kMy_DragHighlightBoxInColor.green /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightBoxInColorBlueFraction
							= (float)kMy_DragHighlightBoxInColor.blue /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightBoxInGraphiteRedFraction
							= (float)kMy_DragHighlightBoxInGraphite.red /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightBoxInGraphiteGreenFraction
							= (float)kMy_DragHighlightBoxInGraphite.green /
								(float)RGBCOLOR_INTENSITY_MAX;
	float const			kMy_DragHighlightBoxInGraphiteBlueFraction
							= (float)kMy_DragHighlightBoxInGraphite.blue /
								(float)RGBCOLOR_INTENSITY_MAX;
	
	float const			kMy_DragHighlightFrameCurveSize = 7.0; // width in pixels of circular corners
}

#pragma mark Types

struct DragInfo
{
	UInt16		numberOfDragItems;	// number of things being dragged
	Boolean		hasHFS;				// does this drag contain a single file?
	Boolean		hasText;			// does this drag contain any text?
};
typedef DragInfo*	DragInfoPtr;

typedef std::map< DragRef, DragInfoPtr >	DragRefToDragInfoPtrMap;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	DragRefToDragInfoPtrMap&	gDragRefsToDragInfoPtrs ()	{ static DragRefToDragInfoPtrMap x; return x; }
	Boolean						gIsDragManagerAvailable = false;
	Boolean						gDropCursorInContent = false;
	Boolean						gDropDestinationCanAcceptItems = false;
}

#pragma mark Internal Method Prototypes

static Boolean			isGraphiteTheme		();
static pascal OSErr		trackingHandler		(DragTrackingMessage, WindowRef, void*, DragRef);



#pragma mark Public Methods

/*!
Call this method once, at program startup time,
to initialize this module.

(3.0)
*/
void
DragAndDrop_Init ()
{
	OSStatus	error = noErr;
	long		dragMgrAttr = 0L;
	
	
	error = Gestalt(gestaltDragMgrAttr, &dragMgrAttr);
	gIsDragManagerAvailable = ((error == noErr) && ((dragMgrAttr & (1L << gestaltDragMgrPresent)) != 0));
	gIsDragManagerAvailable = gIsDragManagerAvailable && 
								((dragMgrAttr & (1L << gestaltPPCDragLibPresent)) != 0) &&
								API_AVAILABLE(InstallTrackingHandler);
	
	// if the Drag Manager is available, install the tracking handlers
	//if (DragAndDrop_Available())
	if (0)
	{
		DragTrackingHandlerUPP		trackingProc = NewDragTrackingHandlerUPP(trackingHandler);
		
		
		error = InstallTrackingHandler(trackingProc, nullptr, nullptr);
		
		// if any error occurred, turn off Drag Manager support (avoids potential problems)
		if (error != noErr)
		{
			Console_WriteLine("Warning - Unexpected error initializing drag-and-drop services.");
			Console_WriteValue("          Error code", error);
			gIsDragManagerAvailable = false;
		}
	}
}// Init


/*!
Returns true only if the operating system supports
the drag and drop APIs.

(3.0)
*/
Boolean
DragAndDrop_Available ()
{
	return gIsDragManagerAvailable;
}// Available


/*!
Returns true only if the specified drag began via
this module’s internal tracking handler and contains
only basic text items.

(3.0)
*/
Boolean
DragAndDrop_DragContainsOnlyTextItems	(DragRef	inDragRef)
{
	Boolean		result = false;
	UInt16		numberOfDragItems = 0;
	
	
	if ((noErr == CountDragItems(inDragRef, &numberOfDragItems)) &&
		(numberOfDragItems > 0))
	{
		// check to see if all of the drag items contain TEXT (a drag
		// is only accepted if all of the items in the drag can be
		// accepted)
		UInt16		oneBasedIndexIntoDragItemList = 0;
		
		
		result = true; // initially...
		for (oneBasedIndexIntoDragItemList = 1;
				oneBasedIndexIntoDragItemList <= numberOfDragItems;
				++oneBasedIndexIntoDragItemList)
		{
			FlavorFlags		flavorFlags = 0L;
			ItemReference	dragItem = nullptr;
			
			
			GetDragItemReferenceNumber(inDragRef, oneBasedIndexIntoDragItemList, &dragItem);
			
			// determine if this is a text item
			if (noErr != GetFlavorFlags(inDragRef, dragItem, kDragFlavorTypeMacRomanText,
										&flavorFlags))
			{
				result = false;
				break;
			}
		}
	}
	
	return result;
}// DragContainsOnlyTextItems


/*!
Returns true only if the specified drag began via
this module’s internal tracking handler and consists
of a single HFS file item.

(3.0)
*/
Boolean
DragAndDrop_DragIsExactlyOneFile	(DragRef	inDragRef)
{
	Boolean								result = false;
	DragRefToDragInfoPtrMap::iterator	pairIterator = gDragRefsToDragInfoPtrs().find(inDragRef);
	
	
	// find the drag data corresponding to this drag
	if (pairIterator != gDragRefsToDragInfoPtrs().end())
	{
		DragInfoPtr		dragInfoPtr = pairIterator->second;
		
		
		result = dragInfoPtr->hasHFS;
	}
	return result;
}// DragIsExactlyOneFile


/*!
Iterates over all basic text items in the specified
drag and concatenates together all text into one
buffer.  If this succeeds, "noErr" is returned and
the specified Handle will be set to a valid buffer
that you must dispose yourself; otherwise, the
accumulation failed and you should not do anything
with the Handle, as it will not be valid.

(3.0)
*/
OSStatus
DragAndDrop_GetDraggedTextAsNewHandle	(DragRef	inDragRef,
										 Handle*	outTextHandlePtr)
{
	OSStatus	result = noErr;
	
	
	if (outTextHandlePtr == nullptr) result = memPCErr;
	else
	{
		ItemReference		draggedItem = 0;
		UInt16				numberOfItems = 0;
		register UInt16		i = 0;
		
		
		*outTextHandlePtr = nullptr;
		
		// loop through all of the drag items contained in this drag
		// and collect the text into the accumulation handle
		CountDragItems(inDragRef, &numberOfItems);
		for (i = 1; i <= numberOfItems; ++i)
		{
			Size		textSize = 0;
			
			
			// get the item’s reference number, so it can be referred to later
			GetDragItemReferenceNumber(inDragRef, i, &draggedItem);
			
			// Try to get the flags for a 'TEXT' flavor.  If this returns
			// without an error, then a 'TEXT' flavor exists in the item.
			result = GetFlavorDataSize(inDragRef, draggedItem, kDragFlavorTypeMacRomanText, &textSize);
			if (result == noErr)
			{
				if (*outTextHandlePtr == nullptr)
				{
					// no data yet, create a new handle for accumulation
					*outTextHandlePtr = Memory_NewHandle(textSize);
					if (*outTextHandlePtr == nullptr)
					{
						// not enough memory!
						result = memFullErr;
						break;
					}
				}
				else
				{
					// append to existing TEXT data
					result = Memory_SetHandleSize(*outTextHandlePtr,
													GetHandleSize(*outTextHandlePtr) + textSize);
					if (result != noErr)
					{
						// not enough memory!
						result = memFullErr;
						break;
					}
				}
				
				// temporarily lock down the accumulation handle and get the drag data
				HLock(*outTextHandlePtr);
				GetFlavorData(inDragRef, draggedItem, kDragFlavorTypeMacRomanText, *(*outTextHandlePtr),
								&textSize, 0L/* offset */);
				HUnlock(*outTextHandlePtr);
			}
		}
	}
	return result;
}// GetDraggedTextAsNewHandle


/*!
Erases a standard drag highlight background.  Call this as
part of your handling for the mouse leaving a valid drop
region.

This is the reverse of DragAndDrop_ShowHighlightBackground().

The drawing state of the given port is preserved.

IMPORTANT:	This could completely erase the area.  You must
			do your own drawing on top after this call (e.g.
			to completely restore the normal view after the
			drop highlight disappears).

(3.1)
*/
void
DragAndDrop_HideHighlightBackground	(CGContextRef	inTargetContext,
									 CGRect const&	inArea)
{
	float const				kThickness = 2.0; // based on observation of Apple appearance
	CGContextSaveRestore	contextLock(inTargetContext);
	CGRect					mutableArea = CGRectInset(inArea, kThickness * 0.5, kThickness * 0.5);
	
	
	CGContextSetRGBFillColor(inTargetContext, 1.0/* red */, 1.0/* green */, 1.0/* blue */, 1.0/* alpha */);
	CGContextSetLineWidth(inTargetContext, kThickness);
	CGContextBeginPath(inTargetContext);
	RegionUtilities_AddRoundedRectangleToPath(inTargetContext, mutableArea,
												kMy_DragHighlightFrameCurveSize,
												kMy_DragHighlightFrameCurveSize);
	CGContextFillPath(inTargetContext);
}// HideHighlightBackground


/*!
Erases a standard drag highlight background.  Call this as
part of your handling for the mouse leaving a valid drop
region.

This is the reverse of DragAndDrop_ShowHighlightBackground().

The current port is preserved.  The drawing state of the
given port is preserved.

IMPORTANT:	This could completely erase the area.  You must
			do your own drawing on top after this call (e.g.
			to completely restore the normal view after the
			drop highlight disappears).

(3.1)
*/
void
DragAndDrop_HideHighlightBackground	(CGrafPtr		inPort,
									 Rect const*	inAreaPtr)
{
	GDHandle	oldDevice = nullptr;
	CGrafPtr	oldPort = nullptr;
	RGBColor	whiteColor =
				{
					RGBCOLOR_INTENSITY_MAX, RGBCOLOR_INTENSITY_MAX, RGBCOLOR_INTENSITY_MAX
				};
	RGBColor	oldBackColor;
	Rect		drawingBounds = *inAreaPtr;
	
	
	GetGWorld(&oldPort, &oldDevice);
	SetPort(inPort);
	GetBackColor(&oldBackColor);
	RGBBackColor(&whiteColor);
	EraseRect(&drawingBounds);
	RGBBackColor(&oldBackColor);
	SetGWorld(oldPort, oldDevice);
}// HideHighlightBackground


/*!
Erases a standard drag highlight frame.  Call this as part
of your handling for the mouse leaving a valid drop region.

This is the reverse of DragAndDrop_ShowHighlightFrame().

The drawing state of the given port is preserved.

IMPORTANT:	The erased frame will leave a blank region.
			You must do your own drawing on top after this
			call (e.g. to completely restore the normal
			view after the drop highlight disappears).

(3.1)
*/
void
DragAndDrop_HideHighlightFrame	(CGContextRef	inTargetContext,
								 CGRect const&	inArea)
{
	float const				kThickness = 2.0; // based on observation of Apple appearance
	CGContextSaveRestore	contextLock(inTargetContext);
	CGRect					mutableArea = CGRectInset(inArea, kThickness * 0.5, kThickness * 0.5);
	
	
	CGContextSetRGBStrokeColor(inTargetContext, 1.0/* red */, 1.0/* green */, 1.0/* blue */, 1.0/* alpha */);
	CGContextSetLineWidth(inTargetContext, kThickness);
	CGContextBeginPath(inTargetContext);
	RegionUtilities_AddRoundedRectangleToPath(inTargetContext, mutableArea,
												kMy_DragHighlightFrameCurveSize,
												kMy_DragHighlightFrameCurveSize);
	CGContextStrokePath(inTargetContext);
}// HideHighlightFrame


/*!
This version of the routine works with QuickDraw ports.
See also the Core Graphics version.

(3.1)
*/
void
DragAndDrop_HideHighlightFrame	(CGrafPtr		inPort,
								 Rect const*	inAreaPtr)
{
	// old QuickDraw implementation
	GDHandle	oldDevice = nullptr;
	CGrafPtr	oldPort = nullptr;
	RGBColor	oldForeColor;
	RGBColor	whiteColor =
				{
					RGBCOLOR_INTENSITY_MAX, RGBCOLOR_INTENSITY_MAX, RGBCOLOR_INTENSITY_MAX
				};
	PenState	oldPenState;
	Rect		drawingBounds = *inAreaPtr;
	
	
	GetGWorld(&oldPort, &oldDevice);
	SetPort(inPort);
	GetForeColor(&oldForeColor);
	GetPenState(&oldPenState);
	RGBForeColor(&whiteColor);
	PenSize(2/* width */, 2/* height */);
	InsetRect(&drawingBounds, 1, 1);
	EraseRoundRect(&drawingBounds, 12/* oval width */, 12/* oval height */);
	FrameRoundRect(&drawingBounds, 12/* oval width */, 12/* oval height */);
	SetPenState(&oldPenState);
	RGBForeColor(&oldForeColor);
	SetGWorld(oldPort, oldDevice);
}// HideHighlightFrame


/*!
If the specified drag began via this module’s internal
tracking handler, returns the number of items in that
drag; otherwise, returns 0.

(3.0)
*/
UInt16
DragAndDrop_ReturnDragItemCount		(DragRef	inDragRef)
{
	UInt16								result = 0;
	DragRefToDragInfoPtrMap::iterator	pairIterator = gDragRefsToDragInfoPtrs().find(inDragRef);
	
	
	// find the drag data corresponding to this drag
	if (pairIterator != gDragRefsToDragInfoPtrs().end())
	{
		DragInfoPtr		dragInfoPtr = pairIterator->second;
		
		
		result = dragInfoPtr->numberOfDragItems;
	}
	return result;
}// ReturnDragItemCount


/*!
Draws a standard drag highlight background in the given port.
Call this as part of your handling for the mouse entering a
valid drop region.

Typically, you call this routine first, then draw your content,
then call DragAndDrop_ShowHighlightFrame() to complete.  This
gives the frame high precedence (always “on top”) but the
background has least precedence.

The drawing state of the given port is preserved.

IMPORTANT:	This could completely erase the area.  You must
			do your own drawing on top after this call (e.g.
			to draw text but not a background).

(3.1)
*/
void
DragAndDrop_ShowHighlightBackground	(CGContextRef	inTargetContext,
									 CGRect const&	inArea)
{
	float const				kThickness = 2.0; // based on observation of Apple appearance
	CGContextSaveRestore	contextLock(inTargetContext);
	CGRect					mutableArea = CGRectInset(inArea, kThickness * 0.5, kThickness * 0.5);
	
	
	if (isGraphiteTheme())
	{
		CGContextSetRGBFillColor(inTargetContext, kMy_DragHighlightBoxInGraphiteRedFraction,
									kMy_DragHighlightBoxInGraphiteGreenFraction,
									kMy_DragHighlightBoxInGraphiteBlueFraction,
									1.0/* alpha */);
	}
	else
	{
		CGContextSetRGBFillColor(inTargetContext, kMy_DragHighlightBoxInColorRedFraction,
									kMy_DragHighlightBoxInColorGreenFraction,
									kMy_DragHighlightBoxInColorBlueFraction,
									1.0/* alpha */);
	}
	CGContextSetLineWidth(inTargetContext, kThickness);
	CGContextBeginPath(inTargetContext);
	RegionUtilities_AddRoundedRectangleToPath(inTargetContext, mutableArea,
												kMy_DragHighlightFrameCurveSize,
												kMy_DragHighlightFrameCurveSize);
	CGContextFillPath(inTargetContext);
}// ShowHighlightBackground


/*!
Draws a standard drag highlight background in the given port.
Call this as part of your handling for the mouse entering a
valid drop region.

Typically, you call this routine first, then draw your content,
then call DragAndDrop_ShowHighlightFrame() to complete.  This
gives the frame high precedence (always “on top”) but the
background has least precedence.

The current port is preserved.  The drawing state of the
given port is preserved.

IMPORTANT:	This could completely erase the area.  You must
			do your own drawing on top after this call (e.g.
			to draw text but not a background).

(3.1)
*/
void
DragAndDrop_ShowHighlightBackground	(CGrafPtr		inPort,
									 Rect const*	inAreaPtr)
{
	GDHandle	oldDevice = nullptr;
	CGrafPtr	oldPort = nullptr;
	RGBColor	oldBackColor;
	Rect		drawingBounds = *inAreaPtr;
	
	
	GetGWorld(&oldPort, &oldDevice);
	SetPort(inPort);
	GetBackColor(&oldBackColor);
	RGBBackColor(isGraphiteTheme() ? &kMy_DragHighlightBoxInGraphite : &kMy_DragHighlightBoxInColor);
	InsetRect(&drawingBounds, 1, 1);
	EraseRoundRect(&drawingBounds, 12/* oval width */, 12/* oval height */);
	RGBBackColor(&oldBackColor);
	SetGWorld(oldPort, oldDevice);
}// ShowHighlightBackground


/*!
Draws a standard drag highlight in the given port.  Call
this as part of your handling for the mouse entering a
valid drop region.

The drawing state of the given port is preserved.

IMPORTANT:	This could completely erase the area.  You
			must do your own drawing on top after this
			call (e.g. to draw text but not a background).

(3.1)
*/
void
DragAndDrop_ShowHighlightFrame	(CGContextRef	inTargetContext,
								 CGRect const&	inArea)
{
	float const				kThickness = 2.0; // based on observation of Apple appearance
	CGContextSaveRestore	contextLock(inTargetContext);
	CGRect					mutableArea = CGRectInset(inArea, kThickness * 0.5, kThickness * 0.5);
	
	
	if (isGraphiteTheme())
	{
		CGContextSetRGBStrokeColor(inTargetContext, kMy_DragHighlightFrameInGraphiteRedFraction,
									kMy_DragHighlightFrameInGraphiteGreenFraction,
									kMy_DragHighlightFrameInGraphiteBlueFraction,
									1.0/* alpha */);
	}
	else
	{
		CGContextSetRGBStrokeColor(inTargetContext, kMy_DragHighlightFrameInColorRedFraction,
									kMy_DragHighlightFrameInColorGreenFraction,
									kMy_DragHighlightFrameInColorBlueFraction,
									1.0/* alpha */);
	}
	CGContextSetLineWidth(inTargetContext, kThickness);
	CGContextBeginPath(inTargetContext);
	RegionUtilities_AddRoundedRectangleToPath(inTargetContext, mutableArea,
												kMy_DragHighlightFrameCurveSize,
												kMy_DragHighlightFrameCurveSize);
	CGContextStrokePath(inTargetContext);
}// ShowHighlightFrame


/*!
This version of the routine works with QuickDraw ports.
See also the Core Graphics version.

(3.1)
*/
void
DragAndDrop_ShowHighlightFrame	(CGrafPtr		inPort,
								 Rect const*	inAreaPtr)
{
	// old QuickDraw implementation
	GDHandle	oldDevice = nullptr;
	CGrafPtr	oldPort = nullptr;
	RGBColor	oldForeColor;
	PenState	oldPenState;
	Rect		drawingBounds = *inAreaPtr;
	
	
	// NOTE: make sure DragAndDrop_HideHighlightFrame() inverts all of this
	GetGWorld(&oldPort, &oldDevice);
	SetPort(inPort);
	GetForeColor(&oldForeColor);
	GetPenState(&oldPenState);
	RGBForeColor(isGraphiteTheme() ? &kMy_DragHighlightFrameInGraphite : &kMy_DragHighlightFrameInColor);
	PenSize(2/* width */, 2/* height */);
	InsetRect(&drawingBounds, 1, 1);
	FrameRoundRect(&drawingBounds, 12/* oval width */, 12/* oval height */);
	SetPenState(&oldPenState);
	RGBForeColor(&oldForeColor);
	SetGWorld(oldPort, oldDevice);
}// ShowHighlightFrame


#pragma mark Internal Methods

/*!
Determines if the user wants a monochrome display.

(3.1)
*/
static Boolean
isGraphiteTheme ()
{
	CFStringRef		themeCFString = nullptr;
	Boolean			result = false;
	

	// TEMPORARY: This setting should probably be cached somewhere (Preferences.h?).	
	if (noErr == CopyThemeIdentifier(&themeCFString))
	{
		result = (kCFCompareEqualTo == CFStringCompare
										(themeCFString, kThemeAppearanceAquaGraphite,
											kCFCompareBackwards));
		CFRelease(themeCFString), themeCFString = nullptr;
	}
	
	return result;
}// isGraphiteTheme


/*!
This is the drag tracking handler, which handles things
such as adding or removing drag highlighting from MacTelnet
windows.  The Mac OS sets the current graphics port to the
specified window’s port prior to invoking this routine.

(2.6)
*/
pascal OSErr
trackingHandler	(DragTrackingMessage	inMessage,
				 WindowRef				inWindow,
				 void*					UNUSED_ARGUMENT(inUserContext),
				 DragRef				inDragRef)
{
	OSErr	result = noErr;
	
	
	if (inMessage == kDragTrackingEnterHandler)
	{
		// then this is the first time the handler has been invoked for a particular drag
		// (this message is NOT specific to a window; a drag could end up in any window);
		// do some work up front to ascertain static information about the drag, instead
		// of constantly polling for this information whenever the data moves around
		DragInfoPtr		dragInfoPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(DragInfo)), DragInfoPtr);
		
		
		if (dragInfoPtr != nullptr) gDragRefsToDragInfoPtrs()[inDragRef] = dragInfoPtr;
		
		// initialize data structure to contain useful information for this drag
		CountDragItems(inDragRef, &dragInfoPtr->numberOfDragItems);
		
		// check to see if exactly one file is being dragged (terminal
		// windows can accept this kind of drag)
		dragInfoPtr->hasHFS = false;
		if (dragInfoPtr->numberOfDragItems == 1)
		{
			FlavorFlags		flavorFlags = 0L;
			ItemReference	dragItem = 0;
			
			
			GetDragItemReferenceNumber(inDragRef, 1/* index */, &dragItem);
			if (GetFlavorFlags(inDragRef, dragItem, kDragFlavorTypeHFS, &flavorFlags) == noErr)
			{
				dragInfoPtr->hasHFS = true;
			}
		}
		
		// check to see if all of the drag items contain TEXT (a drag
		// is only accepted if all of the items in the drag can be
		// accepted)
		dragInfoPtr->hasText = false;
		unless (dragInfoPtr->hasHFS)
		{
			UInt16		oneBasedIndexIntoDragItemList = 0;
			
			
			dragInfoPtr->hasText = true; // initially...
			for (oneBasedIndexIntoDragItemList = 1;
					oneBasedIndexIntoDragItemList <= dragInfoPtr->numberOfDragItems;
					++oneBasedIndexIntoDragItemList)
			{
				FlavorFlags		flavorFlags = 0L;
				ItemReference	dragItem = 0;
				
				
				GetDragItemReferenceNumber(inDragRef, oneBasedIndexIntoDragItemList, &dragItem);
				
				// determine if this is a text item
				if (GetFlavorFlags(inDragRef, dragItem, kDragFlavorTypeMacRomanText, &flavorFlags)
					!= noErr)
				{
					dragInfoPtr->hasText = false;
					break;
				}
			}
		}
	}
	else if (inMessage == kDragTrackingLeaveHandler)
	{
		// then this will be the last time the handler is invoked for a particular drag
		// (this message is NOT specific to a window; a drag could end up in any window)
		DragRefToDragInfoPtrMap::iterator	pairIterator = gDragRefsToDragInfoPtrs().find(inDragRef);
		
		
		if (pairIterator != gDragRefsToDragInfoPtrs().end())
		{
			// throw away auxiliary data used for this drag
			DragInfoPtr		dragInfoPtr = pairIterator->second;
			
			
			Memory_DisposePtr(REINTERPRET_CAST(&dragInfoPtr, Ptr*));
			gDragRefsToDragInfoPtrs().erase(gDragRefsToDragInfoPtrs().find(inDragRef));
		}
	}
	else if (inWindow == nullptr) result = noErr; // do NOTHING unless a window is a target
	else
	{
		// handle window-to-window drag activity
		WindowInfo_Ref			windowInfoRef = nullptr;
		WindowInfo_Descriptor	windowDescriptor = kWindowInfo_InvalidDescriptor;
		Boolean					isFile = false,
								isText = false;
		
		
		// find the drag data corresponding to this drag
		{
			// then this will be the last time the handler is invoked for a particular drag
			// (this message is NOT specific to a window; a drag could end up in any window)
			DragRefToDragInfoPtrMap::iterator	pairIterator = gDragRefsToDragInfoPtrs().find(inDragRef);
			
			
			if (pairIterator != gDragRefsToDragInfoPtrs().end())
			{
				DragInfoPtr		dragInfoPtr = pairIterator->second;
				
				
				isFile = dragInfoPtr->hasHFS;
				isText = dragInfoPtr->hasText;
			}
		}
		
		// determine the MacTelnet window descriptor, if possible
		windowInfoRef = WindowInfo_ReturnFromWindow(inWindow);
		if (windowInfoRef != nullptr) windowDescriptor = WindowInfo_ReturnWindowDescriptor(windowInfoRef);
		
		switch (inMessage)
		{
		// The following message shows up each time a drag enters any
		// MacTelnet window.  Certain variables are used for tracking
		// data in the window, and these are reset for each new window
		// (i.e. for each new receipt of the following message).
		case kDragTrackingEnterWindow:
			// assume initially that the window cannot accept a drop
			gDropDestinationCanAcceptItems = false;
			
			// if it is not a connection window or special dialog, it cannot accept a drop
			if (((isFile) && TerminalWindow_ExistsFor(inWindow)) ||
				((isText) &&
					((windowDescriptor == kConstantsRegistry_WindowDescriptorClipboard) ||
						((windowInfoRef != nullptr) && WindowInfo_IsPotentialDropTarget(windowInfoRef)))))
			{
				gDropDestinationCanAcceptItems = true;
				
				// initially no drag highlight region
				gDropCursorInContent = false;
			}
			
			// if not the frontmost application, fire a timer that will
			// bring the application to the front after a short delay
			// TEMPORARY: doing this immediately without delay
			if (FlagManager_Test(kFlagSuspended))
			{
				ProcessSerialNumber		psn;
				
				
				if (GetCurrentProcess(&psn) == noErr)
				{
					(OSStatus)SetFrontProcess(&psn);
				}
			}
			
			// if not the frontmost window, fire a timer that will
			// bring the window to the front after a short delay
			// TEMPORARY: doing this immediately without delay
			unless (EventLoop_ReturnRealFrontWindow() == inWindow)
			{
				SelectWindow(inWindow);
			}
			break;
		
		// The following message appears as long as the mouse is in
		// one of MacTelnet’s windows during a drag.  Window drag
		// highlighting is drawn when these messages appear.
		case kDragTrackingInWindow:
			// NO LONGER HANDLED - SEE HIVIEW-SPECIFIC CARBON EVENT HANDLERS FOR DRAG
			break;
		
		// The following message shows up each time a drag leaves any
		// MacTelnet window.
		case kDragTrackingLeaveWindow:
			// NO LONGER HANDLED - SEE HIVIEW-SPECIFIC CARBON EVENT HANDLERS FOR DRAG
			break;
		
		default:
			break;
		}
	}
	
	return result;
}// trackingHandler

// BELOW IS REQUIRED NEWLINE TO END FILE
