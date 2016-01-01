/*!	\file DragAndDrop.cp
	\brief Drag-and-drop management.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#include "DragAndDrop.h"
#include <UniversalDefines.h>

// standard-C++ includes
#include <map>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CGContextSaveRestore.h>
#include <Console.h>
#include <MemoryBlocks.h>
#include <WindowInfo.h>

// application includes
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "RegionUtilities.h"



#pragma mark Constants
namespace {

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

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

Boolean		isGraphiteTheme		();

} // anonymous namespace



#pragma mark Public Methods

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


#pragma mark Internal Methods
namespace {

/*!
Determines if the user wants a monochrome display.

(3.1)
*/
Boolean
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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
