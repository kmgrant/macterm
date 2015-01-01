/*!	\file VectorCanvas.mm
	\brief Renders vector graphics, onscreen or offscreen.
*/
/*###############################################################

	MacTerm
		© 1998-2015 by Kevin Grant.
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

#import "VectorCanvas.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cstdio>
#import <cstring>

// standard-C++ includes
#import <vector>

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <ColorUtilities.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlockReferenceLocker.template.h>
#import <MemoryBlocks.h>
#import <SoundSystem.h>

// application includes
#import "Console.h"
#import "ConstantsRegistry.h"
#import "Preferences.h"
#import "Session.h"
#import "VectorInterpreter.h"



#pragma mark Constants
namespace {

UInt16 const	kMy_MaximumX = 4095;
UInt16 const	kMy_MaximumY = 3139;	// TEMPORARY - figure out where the hell this value comes from
Float32 const	kMy_DefaultStrokeWidth = 0.5;
Float32 const	kMy_DefaultTextStrokeWidth = 0.25;

enum
{
	// WARNING: Currently, despite the constants, these values are
	// very important, they match definitions in TEK.  The order
	// (zero-based) should be: white, black, red, green, blue, cyan,
	// magenta, yellow.
	kMy_ColorIndexBackground	= 0,	// technically, white
	kMy_ColorIndexForeground	= 1,	// technically, black
	kMy_ColorIndexRed			= 2,
	kMy_ColorIndexGreen			= 3,
	kMy_ColorIndexBlue			= 4,
	kMy_ColorIndexCyan			= 5,
	kMy_ColorIndexMagenta		= 6,
	kMy_ColorIndexYellow		= 7,
	kMy_MaxColors				= 8
};

} // anonymous namespace

#pragma mark Types

/*!
Holds information on a uniquely-drawn element of a
vector graphic.  A path itself only holds geometric
data (points), so if a drawing is multi-colored or
has lines of different widths it must consist of an
array of elements of this type.
*/
@interface VectorCanvas_Path : NSObject //{
{
@public
	VectorCanvas_PathPurpose	purpose;
	NSBezierPath*				bezierPath;
	Float32						normalLineWidth;
	CGPathDrawingMode			drawingMode;
	SInt16						fillColorIndex;
	SInt16						strokeColorIndex;
}

// initializers
	- (id)
	init;
	- (id)
	initWithPurpose:(VectorCanvas_PathPurpose)_; // designated initializer

// accessors
	- (void)
	setPurpose:(VectorCanvas_PathPurpose)_;

// new methods
	- (VectorCanvas_Path*)
	copyWithEmptyPathAndPurpose:(VectorCanvas_PathPurpose)_;

@end //}


namespace {

typedef std::vector< CGDeviceColor >	My_CGColorList;

/*!
Internal representation of a VectorCanvas_Ref.
*/
struct My_VectorCanvas
{
	OSType					structureID;
	VectorCanvas_Ref		selfRef;
	VectorInterpreter_Ref	interpreter;
	SessionRef				session;
	My_CGColorList			deviceColors;
	SInt16					xorigin;
	SInt16					yorigin;
	SInt16					xscale;
	SInt16					yscale;
	SInt16					ingin;
	SInt16					width;
	SInt16					height;
	VectorCanvas_View*		canvasView;
	NSMutableArray*			drawingPathElements;
	NSBezierPath*			scrapPath;
};
typedef My_VectorCanvas*		My_VectorCanvasPtr;
typedef My_VectorCanvas const*	My_VectorCanvasConstPtr;

typedef MemoryBlockPtrLocker< VectorCanvas_Ref, My_VectorCanvas >			My_VectorCanvasPtrLocker;
typedef LockAcquireRelease< VectorCanvas_Ref, My_VectorCanvas >				My_VectorCanvasAutoLocker;
typedef MemoryBlockReferenceLocker< VectorCanvas_Ref, My_VectorCanvas >		My_VectorCanvasReferenceLocker;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

UInt16				copyColorPreferences	(My_VectorCanvasPtr, Preferences_ContextRef, Boolean = true);
void				getPaletteColor			(My_VectorCanvasPtr, SInt16, CGDeviceColor&);
void				handleMouseDown			(My_VectorCanvasPtr, Point);
Boolean				inSplash				(Point, Point);
VectorCanvas_Path*	pathElementWithPurpose	(My_VectorCanvasPtr, VectorCanvas_PathPurpose, Boolean = false);
void				setPaletteColor			(My_VectorCanvasPtr, SInt16, RGBColor const&);

} // anonymous namespace

/*!
The private class interface.
*/
@interface VectorCanvas_View (VectorCanvas_ViewInternal) //{

	- (void)
	renderDrawingInCurrentFocusWithRect:(NSRect)_;

@end //}

#pragma mark Variables
namespace {

My_VectorCanvasPtrLocker&			gVectorCanvasPtrLocks ()	{ static My_VectorCanvasPtrLocker x; return x; }
My_VectorCanvasReferenceLocker&		gVectorCanvasRefLocks ()	{ static My_VectorCanvasReferenceLocker x; return x; }

UInt32		gVectorCanvasLastClickTime = 0L;

} // anonymous namespace


#pragma mark Public Methods

/*!
Creates a new vector graphics canvas that targets a Mac window.
Returns nullptr on failure.

The canvas is implicitly retained; VectorCanvas_Retain() is
only needed to place additional locks on the reference.

(3.0)
*/
VectorCanvas_Ref
VectorCanvas_New	(VectorInterpreter_Ref	inData)
{
	My_VectorCanvasPtr	ptr = new My_VectorCanvas;
	VectorCanvas_Ref	result = REINTERPRET_CAST(ptr, VectorCanvas_Ref);
	
	
	VectorCanvas_Retain(result);
	
	ptr->structureID = 'RGMW';
	ptr->selfRef = result;
	
	// create regions for paths; the “complete path” represents the
	// contents of the entire drawing to be rendered and the “scrap
	// path“ represents something used temporarily by drawing commands
	ptr->drawingPathElements = [[NSMutableArray alloc] initWithCapacity:10/* arbitrary; expands as needed */];
	ptr->scrapPath = [[NSBezierPath bezierPath] retain];
	
	ptr->canvasView = nil;
	
	ptr->interpreter = inData;
	ptr->session = nullptr;
	ptr->xorigin = 0;
	ptr->yorigin = 0;
	ptr->xscale = kMy_MaximumX;
	ptr->yscale = kMy_MaximumY;
	ptr->width = 400;
	ptr->height = 300;
	ptr->ingin = 0;
	
	// create a color palette for this window
	ptr->deviceColors.resize(kMy_MaxColors);
	{
		Preferences_ContextRef		defaultFormat = nullptr;
		Preferences_Result			prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_GetDefaultContext(&defaultFormat, Quills::Prefs::FORMAT);
		assert(kPreferences_ResultOK == prefsResult);
		copyColorPreferences(ptr, defaultFormat);
	}
	
	return result;
}// New


/*!
Adds a lock on the specified reference.  This indicates you
are using the canvas, so attempts by anyone else to delete
the canvas with VectorCanvas_Release() will fail until you
release your lock (and everyone else releases locks they
may have).

(4.1)
*/
void
VectorCanvas_Retain		(VectorCanvas_Ref	inRef)
{
	gVectorCanvasRefLocks().acquireLock(inRef);
}// Retain


/*!
Releases one lock on the specified canvas created with
VectorCanvas_New() and deletes the canvas *if* no other
locks remain.  Your copy of the reference is set to
nullptr.

(4.1)
*/
void
VectorCanvas_Release	(VectorCanvas_Ref*		inoutRefPtr)
{
	gVectorCanvasRefLocks().releaseLock(*inoutRefPtr);
	unless (gVectorCanvasRefLocks().isLocked(*inoutRefPtr))
	{
		// clean up structure
		{
			My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), *inoutRefPtr);
			
			
			[ptr->canvasView release], ptr->canvasView = nil;
			[ptr->drawingPathElements release], ptr->drawingPathElements = nil;
			[ptr->scrapPath release], ptr->scrapPath = nil;
		}
		delete *(REINTERPRET_CAST(inoutRefPtr, My_VectorCanvasPtr*)), *inoutRefPtr = nullptr;
	}
}// Release


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_AudioEvent		(VectorCanvas_Ref		UNUSED_ARGUMENT(inRef))
{
}// AudioEvent


/*!
Removes all cached data that might have been used to speed up
drawing.  This should be invoked by the data store when it makes
any substantial change that is outside the normal process of
composing a drawing (e.g. clearing the screen).

(4.1)
*/
void
VectorCanvas_ClearCaches	(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	
	
	[ptr->drawingPathElements removeAllObjects];
	[ptr->scrapPath removeAllPoints];
	[ptr->canvasView setNeedsDisplay:YES];
}// ClearCaches


/*!
Renders a straight line between two points expressed in
canvas coordinates.

If the specified line is being used to draw text, the exact
appearance of the line may change in order to make the text
look better (e.g. the line width may be narrower or the
end cap style may change compared to normal graphics lines).
If you do not know whether or not the purpose is to render
text, use kVectorCanvas_PathPurposeGraphics for "inPurpose".

You can choose to target the scrap path via the "inTarget"
parameter, effectively drawing a line that is cached instead
of being added immediately to the main drawing.  Note that
currently a purpose of "kVectorCanvas_PathPurposeText" is
not supported when targeting the scrap path.

\retval kVectorCanvas_ResultOK
if there are no errors

\retval kVectorCanvas_ResultInvalidReference
if the specified canvas is unrecognized

(3.1)
*/
VectorCanvas_Result
VectorCanvas_DrawLine	(VectorCanvas_Ref			inRef,
						 SInt16						inStartX,
						 SInt16						inStartY,
						 SInt16						inEndX,
						 SInt16						inEndY,
						 VectorCanvas_PathPurpose	inPurpose,
						 VectorCanvas_PathTarget	inTarget)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	VectorCanvas_Result			result = kVectorCanvas_ResultInvalidReference;
	
	
	if (nullptr != ptr)
	{
		Float32		x0 = (STATIC_CAST(inStartX, SInt32) * ptr->width) / kVectorInterpreter_MaxX;
		Float32		y0 = STATIC_CAST(ptr->height, SInt32) -
							((STATIC_CAST(inStartY, SInt32) * ptr->height) / kVectorInterpreter_MaxY);
		Float32		x1 = (STATIC_CAST(inEndX, SInt32) * ptr->width) / kVectorInterpreter_MaxX;
		Float32		y1 = STATIC_CAST(ptr->height, SInt32) -
							((STATIC_CAST(inEndY, SInt32) * ptr->height) / kVectorInterpreter_MaxY);
		Boolean		isSinglePoint = ((x0 == x1) && (y0 == y1));
		
		
		if (kVectorCanvas_PathTargetScrap == inTarget)
		{
			// line is being added to a temporary scrap path, not the main drawing
			assert(nil != ptr->scrapPath);
			
			// normally lone points are drawn with special line caps, but with a
			// single path there is currently no real solution except to force the
			// point to actually form a tiny line in an arbitrary direction
			if (isSinglePoint)
			{
				x1 += 0.1;
				y1 += 0.1;
			}
			
			[ptr->scrapPath moveToPoint:NSMakePoint(x0, y0)];
			[ptr->scrapPath lineToPoint:NSMakePoint(x1, y1)];
		}
		else
		{
			// line is being added to the main drawing
			VectorCanvas_Path*	currentElement = pathElementWithPurpose(ptr, inPurpose, isSinglePoint/* force create */);
			
			
			// lone points are invisible unless the line cap is specifically set
			// to make them visible
			if (isSinglePoint)
			{
				[currentElement->bezierPath setLineCapStyle:NSRoundLineCapStyle];
				
				// force the next drawing element to have a separate path
				// (cannot afford to have the single point made invisible
				// by future changes to the line width)
				UNUSED_RETURN(VectorCanvas_Path*)pathElementWithPurpose(ptr, inPurpose, true/* force create */);
			}
			
			[currentElement->bezierPath moveToPoint:NSMakePoint(x0, y0)];
			[currentElement->bezierPath lineToPoint:NSMakePoint(x1, y1)];
		}
		result = kVectorCanvas_ResultOK;
	}
	
	return result;
}// DrawLine


/*!
Marks the canvas view as invalid, which will trigger a redraw
at the next opportunity.

(3.1)
*/
void
VectorCanvas_InvalidateView		(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	
	
	[ptr->canvasView setNeedsDisplay:YES];
}// InvalidateView


/*!
Sets what is known as "GIN" mode; the mouse cursor changes
to a cross, keypresses will send the mouse location, and
mouse button clicks (with or without a “shift”) will send
characters indicating the event type.  Returns 0 if successful.

(3.0)
*/
SInt16
VectorCanvas_MonitorMouse	(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	
	
	ptr->ingin = 1;
	
	return 0;
}// MonitorMouse


/*!
Returns the intepreter whose commands affect this canvas.

(3.1)
*/
VectorInterpreter_Ref
VectorCanvas_ReturnInterpreter		(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	VectorInterpreter_Ref		result = ptr->interpreter;
	
	
	return result;
}// ReturnInterpreter


/*!
Returns the session that responds to events from this canvas,
or nullptr if none has been set.

(3.1)
*/
SessionRef
VectorCanvas_ReturnListeningSession		(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	SessionRef					result = ptr->session;
	
	
	return result;
}// ReturnListeningSession


/*!
Issues drawing commands to permanently include the contents of
the scrap path in the main picture, as a filled-in region that
optionally has a border of the specified size.

\retval kVectorCanvas_ResultOK
if there are no errors

\retval kVectorCanvas_ResultInvalidReference
if the specified canvas is unrecognized

\retval kVectorCanvas_ResultParameterError
if the specified color index is out of range (drawing occurs anyway)

(4.1)
*/
VectorCanvas_Result
VectorCanvas_ScrapPathFill	(VectorCanvas_Ref	inRef,
							 SInt16				inFillColor,
							 Float32			inFrameWidthOrZero)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	VectorCanvas_Result			result = kVectorCanvas_ResultInvalidReference;
	
	
	if (nullptr != ptr)
	{
		// flag bad color specifications, but draw the shape anyway
		if ((inFillColor < 0) || (inFillColor >= kMy_MaxColors))
		{
			Console_Warning(Console_WriteValue, "vector canvas temporary path fill was given invalid color index", inFillColor);
			result = kVectorCanvas_ResultParameterError;
			inFillColor = kMy_ColorIndexBackground; // attempt to recover by using a default color
		}
		else
		{
			result = kVectorCanvas_ResultOK;
		}
		
		// draw the path that has accumulated so far in the temporary space
		{
			assert(nil != ptr->drawingPathElements);
			assert(nil != ptr->scrapPath);
			VectorCanvas_Path*	elementData = [[VectorCanvas_Path alloc] init];
			
			
			[elementData->bezierPath appendBezierPath:ptr->scrapPath];
			if (inFrameWidthOrZero > 0.001/* arbitrary */)
			{
				[elementData->bezierPath setLineWidth:(inFrameWidthOrZero * elementData->normalLineWidth)];
			}
			elementData->drawingMode = kCGPathFill;
			elementData->fillColorIndex = inFillColor;
			[ptr->drawingPathElements addObject:elementData];
			[elementData release];
		}
	}
	
	return result;
}// ScrapPathFill


/*!
Clears the internal region used for scrap/temporary drawing;
subsequent drawing commands targeting the region will therefore
build an independent picture.  This call cannot nest; any
previous path is thrown out.

In legacy code that uses Carbon this MUST occur at drawing time.
In the new Cocoa implementation this SHOULD NOT and NEED NOT
occur at drawing time; simply pass "kVectorCanvas_PathTargetScrap"
as the target when using VectorCanvas_DrawLine() to have the scrap
path updated as drawing commands occur.  In either implementation
VectorCanvas_ScrapPathFill() is used to ensure that the temporary
path is added to the final rendering; in Carbon this is drawn
immediately, and in Cocoa the internal path is merely updated
(to be drawn at the next opportunity).

(4.1)
*/
VectorCanvas_Result
VectorCanvas_ScrapPathReset		(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	VectorCanvas_Result			result = kVectorCanvas_ResultInvalidReference;
	
	
	if (nullptr != ptr)
	{
		// Cocoa view; in the new implementation drawing is NOT
		// assumed to be taking place at this time, so simply
		// ensure that the scrap path target is empty and do
		// not begin a path in any particular drawing context
		assert(nil != ptr->scrapPath);
		[ptr->scrapPath removeAllPoints];
		result = kVectorCanvas_ResultOK;
	}
	return result;
}// ScrapPathReset


/*!
Assigns an NSView subclass to render this canvas.  This should
part of the response to loading a NIB file.

\retval kVectorCanvas_ResultOK
if there are no errors

\retval kVectorCanvas_ResultInvalidReference
if the specified canvas is unrecognized

\retval kVectorCanvas_ResultParameterError
if the specified view is invalid

(4.1)
*/
VectorCanvas_Result
VectorCanvas_SetCanvasNSView	(VectorCanvas_Ref		inRef,
								 VectorCanvas_View*		inNSViewBasedRenderer)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	VectorCanvas_Result			result = kVectorCanvas_ResultInvalidReference;
	
	
	if (nil == inNSViewBasedRenderer)
	{
		result = kVectorCanvas_ResultParameterError;
	}
	else if (nullptr != ptr)
	{
		ptr->canvasView = inNSViewBasedRenderer;
		result = kVectorCanvas_ResultOK;
	}
	
	return result;
}// SetCanvasNSView


/*!
Specifies the session that receives input events, such as
the mouse location.  Returns 0 if successful.

(3.1)
*/
SInt16
VectorCanvas_SetListeningSession	(VectorCanvas_Ref	inRef,
									 SessionRef			inSession)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	
	
	ptr->session = inSession;
	return 0;
}// SetListeningSession


/*!
Chooses a color for drawing dots and lines, from among
a set palette.

TEK defines colors as the following: 0 is white, 1 is
black, 2 is red, 3 is green, 4 is blue, 5 is cyan, 6 is
magenta, and 7 is yellow.

If the specified purpose does not match the most recent
one used for drawing then the color is applied to a new
sub-path.

\retval kVectorCanvas_ResultOK
if there are no errors

\retval kVectorCanvas_ResultInvalidReference
if the specified canvas is unrecognized

\retval kVectorCanvas_ResultParameterError
if the specified color index is out of range

(3.0)
*/
VectorCanvas_Result
VectorCanvas_SetPenColor	(VectorCanvas_Ref			inRef,
							 SInt16						inColor,
							 VectorCanvas_PathPurpose	inPurpose)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	VectorCanvas_Result			result = kVectorCanvas_ResultInvalidReference;
	
	
	if ((inColor < 0) || (inColor >= kMy_MaxColors))
	{
		Console_Warning(Console_WriteValue, "vector canvas was given invalid pen color index", inColor);
		result = kVectorCanvas_ResultParameterError;
	}
	else if (nullptr != ptr)
	{
		// currently it is assumed that the color is being set in the main drawing
		VectorCanvas_Path*	currentElement = pathElementWithPurpose(ptr, inPurpose);
		
		
		if (currentElement->strokeColorIndex != inColor)
		{
			// to change the color, create a new entry (subsequent path
			// changes will use this color until something changes again)
			[ptr->drawingPathElements addObject:[[[VectorCanvas_Path alloc] initWithPurpose:inPurpose] autorelease]];
			currentElement = [ptr->drawingPathElements lastObject];
			assert(nil != currentElement);
		}
		currentElement->strokeColorIndex = inColor;
	}
	
	return result;
}// SetPenColor


#pragma mark Internal Methods
namespace {

/*!
Attempts to read all supported color tags from the given
preference context, and any colors that exist will be
used to update the specified canvas.

Returns the number of colors that were changed.

(3.1)
*/
UInt16
copyColorPreferences	(My_VectorCanvasPtr			inPtr,
						 Preferences_ContextRef		inSource,
						 Boolean					inSearchForDefaults)
{
	SInt16				currentIndex = 0;
	Preferences_Tag		currentPrefsTag = '----';
	RGBColor			colorValue;
	UInt16				result = 0;
	
	
	currentIndex = kMy_ColorIndexBackground;
	currentPrefsTag = kPreferences_TagTerminalColorNormalBackground;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setPaletteColor(inPtr, currentIndex, colorValue);
		++result;
	}
	
	currentIndex = kMy_ColorIndexForeground;
	currentPrefsTag = kPreferences_TagTerminalColorNormalForeground;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setPaletteColor(inPtr, currentIndex, colorValue);
		++result;
	}
	
	currentIndex = kMy_ColorIndexRed;
	currentPrefsTag = kPreferences_TagTerminalColorANSIRed;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setPaletteColor(inPtr, currentIndex, colorValue);
		++result;
	}
	
	currentIndex = kMy_ColorIndexGreen;
	currentPrefsTag = kPreferences_TagTerminalColorANSIGreen;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setPaletteColor(inPtr, currentIndex, colorValue);
		++result;
	}
	
	currentIndex = kMy_ColorIndexBlue;
	currentPrefsTag = kPreferences_TagTerminalColorANSIBlue;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setPaletteColor(inPtr, currentIndex, colorValue);
		++result;
	}
	
	currentIndex = kMy_ColorIndexCyan;
	currentPrefsTag = kPreferences_TagTerminalColorANSICyan;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setPaletteColor(inPtr, currentIndex, colorValue);
		++result;
	}
	
	currentIndex = kMy_ColorIndexMagenta;
	currentPrefsTag = kPreferences_TagTerminalColorANSIMagenta;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setPaletteColor(inPtr, currentIndex, colorValue);
		++result;
	}
	
	currentIndex = kMy_ColorIndexYellow;
	currentPrefsTag = kPreferences_TagTerminalColorANSIYellow;
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
															sizeof(colorValue), &colorValue, inSearchForDefaults))
	{
		setPaletteColor(inPtr, currentIndex, colorValue);
		++result;
	}
	
	return result;
}// copyColorPreferences


/*!
Retrieves the floating-point RGB color for the specified
TEK color index.

(3.1)
*/
void
getPaletteColor		(My_VectorCanvasPtr		inPtr,
					 SInt16					inZeroBasedIndex,
					 CGDeviceColor&			outColor)
{
	outColor = inPtr->deviceColors[inZeroBasedIndex];
}// getPaletteColor


/*!
Responds to a click/drag in a TEK window.  The current QuickDraw
port will be drawn into.

NOTE: This old routine will be needed to add mouse support to a
new HIView-based canvas that is planned.

(2.6)
*/
void
handleMouseDown		(My_VectorCanvasPtr		inPtr,
					 Point					inViewLocalMouse)
{
	if (inPtr->ingin)
	{
		// report the location of the cursor
		{
			SInt32		lx = 0L;
			SInt32		ly = 0L;
			char		cursorReport[6];
			
			
			lx = ((SInt32)inPtr->xscale * (SInt32)inViewLocalMouse.h) / (SInt32)inPtr->width;
			ly = (SInt32)inPtr->yscale -
					((SInt32)inPtr->yscale * (SInt32)inViewLocalMouse.v) / (SInt32)inPtr->height;
			
			// the report is exactly 5 characters long
			if (0 == VectorInterpreter_FillInPositionReport(inPtr->interpreter, STATIC_CAST(lx, UInt16), STATIC_CAST(ly, UInt16),
															' ', cursorReport))
			{
				Session_SendData(inPtr->session, cursorReport, 5);
				Session_SendData(inPtr->session, " \r\n", 3);
			}
		}
		
		//inPtr->ingin = 0;
		gVectorCanvasLastClickTime = TickCount();
	}
	else
	{
		Point					anchor = inViewLocalMouse;
		Point					current = inViewLocalMouse;
		Point					last = inViewLocalMouse;
		SInt16					x0 = 0;
		SInt16					y0 = 0;
		SInt16					x1 = 0;
		SInt16					y1 = 0;
		Rect					rect;
		MouseTrackingResult		trackingResult = kMouseTrackingMouseDown;
		
		
		last = inViewLocalMouse;
		current = inViewLocalMouse;
		anchor = inViewLocalMouse;
		
		ColorUtilities_SetGrayPenPattern();
		PenMode(patXor);
		
		bzero(&rect, sizeof(rect));
		do
		{
			unless (inSplash(current, anchor))
			{
				FrameRect(&rect);
		
				if (anchor.v < current.v)
				{
					rect.top = anchor.v;
					rect.bottom = current.v;
				}
				else
				{
					rect.top = current.v;
					rect.bottom = anchor.v;
				}
		
				if (anchor.h < current.h)
				{
					rect.left = anchor.h;
					rect.right = current.h;
				}
				else
				{
					rect.right = anchor.h;
					rect.left = current.h;
				}
		
				FrameRect(&rect);
				last = current;
			}
			
			// find next mouse location
			{
				OSStatus	error = noErr;
				
				
				error = TrackMouseLocation(nullptr/* port, or nullptr for current port */, &current,
											&trackingResult);
				if (error != noErr) break;
			}
		}
		while (kMouseTrackingMouseUp != trackingResult);
		
		FrameRect(&rect);
		
		ColorUtilities_SetBlackPenPattern();
		PenMode(patCopy);
		
		if (inSplash(anchor, current))
		{
			if (gVectorCanvasLastClickTime && ((gVectorCanvasLastClickTime + GetDblTime()) > TickCount()))
			{
				inPtr->xscale = kMy_MaximumX;
				inPtr->yscale = kMy_MaximumY;
				inPtr->xorigin = 0;
				inPtr->yorigin = 0;
				
				VectorInterpreter_Zoom(inPtr->interpreter, 0, 0, kMy_MaximumX - 1, kMy_MaximumY - 1);
				//VectorInterpreter_PageCommand(inPtr->interpreter);
				gVectorCanvasLastClickTime = 0L;
			}
			else
			{
				gVectorCanvasLastClickTime = TickCount();
			}
		}
		else
		{
			x0 = (short)((long)rect.left * inPtr->xscale / inPtr->width);
			y0 = (short)(inPtr->yscale - (long)rect.top * inPtr->yscale / inPtr->height);
			x1 = (short)((long)rect.right * inPtr->xscale / inPtr->width);
			y1 = (short)(inPtr->yscale - (long)rect.bottom * inPtr->yscale / inPtr->height);
			x1 = (x1 < (x0 + 2)) ? x0 + 4 : x1;
			y0 = (y0 < (y1 + 2)) ? y1 + 4 : y0;
			
			VectorInterpreter_Zoom(inPtr->interpreter, x0 + inPtr->xorigin, y1 + inPtr->yorigin,
									x1 + inPtr->xorigin, y0 + inPtr->yorigin);
			//VectorInterpreter_PageCommand(inPtr->interpreter);
			
			inPtr->xscale = x1 - x0;
			inPtr->yscale = y0 - y1;
			inPtr->xorigin = x0 + inPtr->xorigin;
			inPtr->yorigin = y1 + inPtr->yorigin;
			
			gVectorCanvasLastClickTime = 0L;
		}
	}
}// handleMouseDown


/*!
Determines if two points are fairly close.

(2.6)
*/
Boolean
inSplash	(Point		inPoint1,
			 Point		inPoint2)
{
	Boolean		result = true;
	
	
	if (((inPoint1.h - inPoint2.h) > 3/* arbitrary */) ||
		((inPoint2.h - inPoint1.h) > 3/* arbitrary */))
	{
		result = false;
	}
	else if (((inPoint1.v - inPoint2.v) > 3/* arbitrary */) ||
				((inPoint2.v - inPoint1.v) > 3/* arbitrary */))
	{
		result = false;
	}
	
	return result;
}// inSplash


/*!
Returns a path element suitable for adding to the current
drawing for the given reason.

If the drawing is empty, an object is allocated.  If the
last object in the drawing’s path element array has the
given purpose already, it is returned; otherwise, an
object with the given purpose is allocated and returned.

You may also force an object to be allocated by setting
the final parameter to true.

(4.1)
*/
VectorCanvas_Path*
pathElementWithPurpose	(My_VectorCanvasPtr			inPtr,
						 VectorCanvas_PathPurpose	inPurpose,
						 Boolean					inForceCreate)
{
	VectorCanvas_Path*	result = nil;
	
	
	assert(nil != inPtr->drawingPathElements);
	if (0 == [inPtr->drawingPathElements count])
	{
		// drawing was empty
		[inPtr->drawingPathElements addObject:[[[VectorCanvas_Path alloc] initWithPurpose:inPurpose] autorelease]];
	}
	else
	{
		VectorCanvas_Path*	activePath = REINTERPRET_CAST([inPtr->drawingPathElements lastObject], VectorCanvas_Path*);
		
		
		if ((inForceCreate) || (inPurpose != activePath->purpose))
		{
			// drawing was empty or it was currently creating something
			// different; make a new path with the requested purpose
			[inPtr->drawingPathElements addObject:[[activePath copyWithEmptyPathAndPurpose:inPurpose] autorelease]];
		}
	}
	result = [inPtr->drawingPathElements lastObject];
	assert(nil != result);
	
	return result;
}// pathElementWithPurpose


/*!
Changes the RGB color for the specified TEK color index.

(3.1)
*/
void
setPaletteColor		(My_VectorCanvasPtr		inPtr,
					 SInt16					inZeroBasedIndex,
					 RGBColor const&		inColor)
{
	inPtr->deviceColors[inZeroBasedIndex] = ColorUtilities_CGDeviceColorMake(inColor);
}// setPaletteColor

} // anonymous namespace


@implementation VectorCanvas_Path


/*!
No-argument initializer; invokes "initWithPurpose:" using
"kVectorCanvas_PathPurposeGraphics".

(4.1)
*/
- (id)
init
{
	self = [self initWithPurpose:kVectorCanvas_PathPurposeGraphics];
	if (nil != self)
	{
	}
	return self;
}// init


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithPurpose:(VectorCanvas_PathPurpose)	aPurpose
{
	self = [super init];
	if (nil != self)
	{
		[self setPurpose:aPurpose]; // sets purpose and line width
		self->bezierPath = [[NSBezierPath bezierPath] copy];
		[self->bezierPath setLineWidth:self->normalLineWidth];
		[self->bezierPath setLineCapStyle:NSRoundLineCapStyle];
		[self->bezierPath setLineJoinStyle:NSBevelLineJoinStyle];
		self->drawingMode = kCGPathStroke;
		self->fillColorIndex = 0;
		self->strokeColorIndex = 0;
	}
	return self;
}// initWithPurpose:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[bezierPath release];
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (void)
setPurpose:(VectorCanvas_PathPurpose)	aPurpose
{
	// the line width is directly dependent on the purpose of the path
	self->purpose = aPurpose;
	self->normalLineWidth = ((kVectorCanvas_PathPurposeText == aPurpose) ? kMy_DefaultTextStrokeWidth : kMy_DefaultStrokeWidth);
}// setPurpose:


#pragma mark New Methods


/*!
Copies this object.

(4.1)
*/
- (VectorCanvas_Path*)
copyWithEmptyPathAndPurpose:(VectorCanvas_PathPurpose)	aPurpose
{
	VectorCanvas_Path*	result = [[[self class] alloc] init];
	
	
	if (nil != result)
	{
		result->purpose = purpose;
		result->normalLineWidth = normalLineWidth;
		result->bezierPath = [[NSBezierPath bezierPath] copy];
		result->drawingMode = drawingMode;
		result->fillColorIndex = fillColorIndex;
		result->strokeColorIndex = strokeColorIndex;
		[result setPurpose:aPurpose];
	}
	return result;
}// copyWithEmptyPathAndPurpose:


@end // VectorCanvas_Path


@implementation VectorCanvas_View


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithFrame:(NSRect)		aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		self->interpreterRef = nullptr;
	}
	return self;
}// initWithFrame:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	VectorInterpreter_Release(&interpreterRef);
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (VectorInterpreter_Ref)
interpreterRef
{
	return interpreterRef;
}
- (void)
setInterpreterRef:(VectorInterpreter_Ref)	anInterpreter
{
	if (interpreterRef != anInterpreter)
	{
		if (nullptr != interpreterRef)
		{
			VectorInterpreter_Release(&interpreterRef);
		}
		if (nullptr != anInterpreter)
		{
			VectorCanvas_Ref	canvas = VectorInterpreter_ReturnCanvas(anInterpreter);
			
			
			(VectorCanvas_Result)VectorCanvas_SetCanvasNSView(canvas, self);
			VectorInterpreter_Retain(anInterpreter);
		}
		interpreterRef = anInterpreter;
	}
}// setInterpreterRef


#pragma mark Actions


/*!
Copies the drawing in the canvas to the primary pasteboard.
(This should be triggered by a standard Copy command from the
Edit menu.)

(4.1)
*/
- (IBAction)
performCopy:(id)	sender
{
#pragma unused(sender)
	NSRect			imageBounds = [self bounds];
	NSImage*		copiedImage = [[NSImage alloc] initWithSize:imageBounds.size];
	NSArray*		dataTypeArray = [NSArray arrayWithObjects:NSPDFPboardType, NSTIFFPboardType, nil];
	NSPasteboard*	targetPasteboard = [NSPasteboard generalPasteboard];
	
	
	[copiedImage setFlipped:YES];
	[copiedImage lockFocus];
	[self renderDrawingInCurrentFocusWithRect:imageBounds];
	[copiedImage unlockFocus];
	
	// copy both a bitmap image and vector graphics (PDF); the former
	// is more likely to be compatible with other applications and
	// the latter preserves all the flexibility of the original drawing
	[targetPasteboard declareTypes:dataTypeArray owner:nil];
	[targetPasteboard setData:[self dataWithPDFInsideRect:imageBounds] forType:NSPDFPboardType];
	[targetPasteboard setData:[copiedImage TIFFRepresentation] forType:NSTIFFPboardType];
	
	[copiedImage release];
}
- (id)
canPerformCopy:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	return @(YES);
}


/*!
Uses the name of the given sender (expected to be a menu item) to
find a Format set of Preferences from which to copy new font and
color information.

(4.0)
*/
- (IBAction)
performFormatByFavoriteName:(id)	sender
{
	VectorCanvas_Ref	canvasRef = VectorInterpreter_ReturnCanvas([self interpreterRef]);
	My_VectorCanvasPtr	canvasPtr = gVectorCanvasPtrLocks().acquireLock(canvasRef);
	BOOL				isError = YES;
	
	
	if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
	{
		// use the specified preferences
		NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
		CFStringRef		collectionName = BRIDGE_CAST([asMenuItem title], CFStringRef);
		
		
		if ((nil != collectionName) && Preferences_IsContextNameInUse(Quills::Prefs::FORMAT, collectionName))
		{
			Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
														(Quills::Prefs::FORMAT, collectionName), true/* is retained */);
			
			
			if (namedSettings.exists())
			{
				// change font and/or colors of frontmost window according to the specified preferences
				UInt16		count = copyColorPreferences(canvasPtr, namedSettings.returnRef(), true/* search defaults */);
				
				
				isError = (0 == count);
			}
		}
	}
	
	if (isError)
	{
		// failed...
		Sound_StandardAlert();
	}
	else
	{
		[canvasPtr->canvasView setNeedsDisplay:YES];
	}
}
- (id)
canPerformFormatByFavoriteName:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


/*!
Copies new font and color information from the Default set.

(4.0)
*/
- (IBAction)
performFormatDefault:(id)	sender
{
#pragma unused(sender)
	VectorCanvas_Ref		canvasRef = VectorInterpreter_ReturnCanvas([self interpreterRef]);
	My_VectorCanvasPtr		canvasPtr = gVectorCanvasPtrLocks().acquireLock(canvasRef);
	Preferences_ContextRef	defaultSettings = nullptr;
	BOOL					isError = YES;
	
	
	// reformat frontmost window using the Default preferences
	if (kPreferences_ResultOK == Preferences_GetDefaultContext(&defaultSettings, Quills::Prefs::FORMAT))
	{
		UInt16		count = copyColorPreferences(canvasPtr, defaultSettings, true/* search for defaults */);
		
		
		isError = (0 == count);
	}
	
	if (isError)
	{
		// failed...
		Sound_StandardAlert();
	}
	else
	{
		[canvasPtr->canvasView setNeedsDisplay:YES];
	}
}
- (id)
canPerformFormatDefault:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	return @(YES);
}


/*!
Displays the standard printing interface for printing the
specified drawing.  (This should be triggered by an appropriate
printing menu command.)

(4.1)
*/
- (IBAction)
performPrintScreen:(id)		sender
{
#pragma unused(sender)
	NSPrintInfo*	printInfo = [[[NSPrintInfo sharedPrintInfo] copy] autorelease];
	
	
	// initial settings are basically arbitrary; attempting to make everything look nice
	[printInfo setHorizontalPagination:NSFitPagination];
	[printInfo setHorizontallyCentered:YES];
	[printInfo setVerticalPagination:NSFitPagination];
	[printInfo setVerticallyCentered:NO];
	[printInfo setLeftMargin:72.0];
	[printInfo setRightMargin:72.0];
	[printInfo setTopMargin:72.0];
	[printInfo setBottomMargin:72.0];
	
	if ([self bounds].size.width > [self bounds].size.height)
	{
		[printInfo setOrientation:NSLandscapeOrientation];
	}
	
	[[NSPrintOperation printOperationWithView:self printInfo:printInfo]
		runOperationModalForWindow:[self window] delegate:nil didRunSelector:nil contextInfo:nullptr];
}
- (id)
canPerformPrintScreen:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


/*!
Displays the standard printing interface for printing the
currently-selected part of the specified drawing.  (This
should be triggered by an appropriate printing menu command.)

(4.1)
*/
- (IBAction)
performPrintSelection:(id)	sender
{
#pragma unused(sender)
	// TEMPORARY; need to update this when mouse support, etc.
	// is once again implemented in graphics windows
	[self performPrintScreen:sender];
}
- (id)
canPerformPrintSelection:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return @(YES);
}


#pragma mark NSControl


/*!
Responds to mouse-down events by interacting with the vector
graphics terminal: drags define zoom regions, double-clicks
restore the default zoom level.

(4.1)
*/
- (void)
mouseDown:(NSEvent*)	anEvent
{
	[super mouseDown:anEvent];
	// UNIMPLEMENTED
}// mouseDown:


#pragma mark NSView


/*!
It is necessary for canvas views to accept “first responder”
in order to ever receive actions such as menu commands!

(4.1)
*/
- (BOOL)
acceptsFirstResponder
{
	return YES;
}// acceptsFirstResponder


/*!
Render the specified part of a vector graphics canvas.

INCOMPLETE.  This is going to be the test bed for transitioning
away from Carbon.  And given that the Carbon view is heavily
dependent on older technologies, it will be awhile before the
Cocoa version is exposed to users.

(4.1)
*/
- (void)
drawRect:(NSRect)	rect
{
	[super drawRect:rect];
	[self renderDrawingInCurrentFocusWithRect:[self bounds]];
}// drawRect:


/*!
Returns YES only if the view’s coordinate system uses
a top-left origin.

(4.1)
*/
- (BOOL)
isFlipped
{
	// since drawing code is originally from Carbon, keep the view
	// flipped for the time being
	return YES;
}// isFlipped


/*!
Returns YES only if the view has no transparent parts.

(4.1)
*/
- (BOOL)
isOpaque
{
	return NO;
}// isOpaque


/*!
Invoked by NSView whenever it’s necessary to define the regions
that change the mouse pointer’s shape.

(4.1)
*/
- (void)
resetCursorRects
{
	[self addCursorRect:[self bounds] cursor:[NSCursor crosshairCursor]];
}// resetCursorRects


@end // VectorCanvas_View


@implementation VectorCanvas_View (VectorCanvas_ViewInternal)


#pragma mark New Methods


/*!
Adds the entire vector drawing to the current focus (an image
or a graphics context).  The output varies slightly if this
is for a print-out.

IMPORTANT:	This should only be called within the call tree
			of an NSView’s "drawRect:" or while an NSImage
			has focus locked on it.

(4.1)
*/
- (void)
renderDrawingInCurrentFocusWithRect:(NSRect)	aRect
{
	VectorCanvas_Ref	canvasRef = VectorInterpreter_ReturnCanvas([self interpreterRef]);
	My_VectorCanvasPtr	canvasPtr = gVectorCanvasPtrLocks().acquireLock(canvasRef);
	
	
	if (nullptr != canvasPtr)
	{
		NSGraphicsContext*	contextMgr = [NSGraphicsContext currentContext];
		NSPrintOperation*	printOp = [NSPrintOperation currentOperation];
		CGContextRef		drawingContext = REINTERPRET_CAST([contextMgr graphicsPort], CGContextRef);
		CGRect				contentBounds = CGRectMake(aRect.origin.x, aRect.origin.y, aRect.size.width, aRect.size.height);
		BOOL				isPrinting = (nil != printOp);
		
		
		// draw the background (unless this is for printing)
		unless (isPrinting)
		{
			SInt16			backgroundColorIndex = VectorInterpreter_ReturnBackgroundColor(canvasPtr->interpreter);
			CGDeviceColor	backgroundColor;
			
			
			assert((backgroundColorIndex >= 0) && (backgroundColorIndex < kMy_MaxColors));
			getPaletteColor(canvasPtr, backgroundColorIndex, backgroundColor);
			CGContextSetRGBFillColor(drawingContext, backgroundColor.red, backgroundColor.green,
										backgroundColor.blue, 1.0/* alpha */);
			CGContextFillRect(drawingContext, contentBounds);
		}
		
		// draw the vector graphics; this is achieved by iterating over
		// stored drawing commands and replicating them
		if (nil != canvasPtr->drawingPathElements)
		{
			CGDeviceColor	scratchColor;
			SInt16			currentFillColorIndex = 0;
			SInt16			currentStrokeColorIndex = 0;
			
			
			// scale the entire drawing to fill the view
			if ((canvasPtr->width > 0) && (canvasPtr->height > 0))
			{
				CGContextScaleCTM(drawingContext, contentBounds.size.width / canvasPtr->width,
									contentBounds.size.height / canvasPtr->height);
			}
			
			// initialize state
			unless (isPrinting)
			{
				CGContextSetShadow(drawingContext, CGSizeMake(2.2, -2.2)/* offset; arbitrary */, 6.0/* blur; arbitrary */);
			}
			
			// render each piece of the drawing; for a drawing that always uses the
			// same colors and line sizes, etc. this loop will only iterate once
			for (VectorCanvas_Path* pathElement in canvasPtr->drawingPathElements)
			{
				// update graphics context state if it should change
				if (pathElement->fillColorIndex != currentFillColorIndex)
				{
					assert((pathElement->fillColorIndex >= 0) && (pathElement->fillColorIndex < kMy_MaxColors));
					if ((nil != printOp) && (kMy_ColorIndexBackground == pathElement->fillColorIndex))
					{
						// when printing, do not allow the background color to print
						// (because it might be reformatted, e.g. white-on-black);
						// instead, force the background to be white
						CGContextSetRGBFillColor(drawingContext, 1.0, 1.0, 1.0, 1.0/* alpha */);
					}
					else
					{
						getPaletteColor(canvasPtr, pathElement->fillColorIndex, scratchColor);
						CGContextSetRGBFillColor(drawingContext, scratchColor.red, scratchColor.green,
													scratchColor.blue, 1.0/* alpha */);
					}
					currentFillColorIndex = pathElement->fillColorIndex;
				}
				if (pathElement->strokeColorIndex != currentStrokeColorIndex)
				{
					assert((pathElement->strokeColorIndex >= 0) && (pathElement->strokeColorIndex < kMy_MaxColors));
					if ((nil != printOp) && (kMy_ColorIndexForeground == pathElement->strokeColorIndex))
					{
						// when printing, do not allow the foreground color to print
						// (because it might be reformatted, e.g. white-on-black);
						// instead, force the foreground to be black
						CGContextSetRGBStrokeColor(drawingContext, 0.0, 0.0, 0.0, 1.0/* alpha */);
					}
					else
					{
						getPaletteColor(canvasPtr, pathElement->strokeColorIndex, scratchColor);
						CGContextSetRGBStrokeColor(drawingContext, scratchColor.red, scratchColor.green,
													scratchColor.blue, 1.0/* alpha */);
					}
					currentStrokeColorIndex = pathElement->strokeColorIndex;
				}
				
				// make lines thicker when the drawing is bigger, up to a certain maximum thickness
				[pathElement->bezierPath setLineWidth:std::max(std::min((contentBounds.size.width / canvasPtr->width) * pathElement->normalLineWidth,
																		pathElement->normalLineWidth * 2/* arbitrary maximum */),
																pathElement->normalLineWidth / 3 * 2/* arbitrary minimum */)];
				
				// add the new sub-path
				switch (pathElement->drawingMode)
				{
				case kCGPathFill:
					[pathElement->bezierPath fill];
					break;
				
				case kCGPathStroke:
				default:
					[pathElement->bezierPath stroke];
					break;
				}
			}
		}
	}
	gVectorCanvasPtrLocks().releaseLock(canvasRef, &canvasPtr);
}// renderDrawingInCurrentFocusWithRect:


@end // VectorCanvas_View (VectorCanvas_ViewInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
