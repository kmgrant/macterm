/*!	\file VectorCanvas.mm
	\brief Renders vector graphics, onscreen or offscreen.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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
#import <Carbon/Carbon.h> // for kVK... virtual key codes (TEMPORARY; deprecated)
#import <Cocoa/Cocoa.h>

// library includes
#import <CocoaAnimation.h>
#import <CocoaExtensions.objc++.h>
#import <ContextSensitiveMenu.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlockReferenceLocker.template.h>
#import <MemoryBlocks.h>
#import <SoundSystem.h>

// application includes
#import "Console.h"
#import "ConstantsRegistry.h"
#import "EventLoop.h"
#import "Preferences.h"
#import "Session.h"
#import "TerminalView.h"
#import "UIStrings.h"
#import "VectorInterpreter.h"



#pragma mark Constants
namespace {

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
	- (instancetype)
	init;
	- (instancetype)
	initWithPurpose:(VectorCanvas_PathPurpose)_ NS_DESIGNATED_INITIALIZER;

// accessors
	- (void)
	setPurpose:(VectorCanvas_PathPurpose)_;

// new methods
	- (VectorCanvas_Path*)
	copyWithEmptyPathForPurpose:(VectorCanvas_PathPurpose)_;

@end //}


/*!
Private properties.
*/
@interface VectorCanvas_View () //{

// accessors
	@property (assign) NSRect
	dragRectangle;
	@property (assign) NSPoint
	dragStart;

@end //}


/*!
The private class interface.
*/
@interface VectorCanvas_View (VectorCanvas_ViewInternal) //{

// new methods
	- (void)
	renderDrawingInCurrentFocusWithRect:(NSRect)_;

@end //}


namespace {

typedef std::vector< CGFloatRGBColor >	My_CGColorList;

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
	CGFloatRGBColor			outsideColor;
	TerminalView_MousePointerColor	mousePointerColor;
	SInt16					ingin;
	SInt16					canvasWidth;
	SInt16					canvasHeight;
	CGFloat					viewScaleX;
	CGFloat					viewScaleY;
	CGFloat					unscaledZoomOriginX;
	CGFloat					unscaledZoomOriginY;
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
void				getPaletteColor			(My_VectorCanvasPtr, SInt16, CGFloatRGBColor&);
VectorCanvas_Path*	pathElementWithPurpose	(My_VectorCanvasPtr, VectorCanvas_PathPurpose, Boolean = false);
void				setPaletteColor			(My_VectorCanvasPtr, SInt16, CGFloatRGBColor const&);

} // anonymous namespace

#pragma mark Variables
namespace {

My_VectorCanvasPtrLocker&			gVectorCanvasPtrLocks ()	{ static My_VectorCanvasPtrLocker x; return x; }
My_VectorCanvasReferenceLocker&		gVectorCanvasRefLocks ()	{ static My_VectorCanvasReferenceLocker x; return x; }

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
	ptr->canvasWidth = 400;
	ptr->canvasHeight = 300;
	ptr->viewScaleX = 1.0;
	ptr->viewScaleY = 1.0;
	ptr->unscaledZoomOriginX = 0;
	ptr->unscaledZoomOriginY = 0;
	ptr->ingin = 0;
	
	// create a color palette for this window (colors are set when a view is attached)
	ptr->deviceColors.resize(kMy_MaxColors);
	ptr->outsideColor.red = 0.8; // arbitrary (changed by preferences)
	ptr->outsideColor.green = 0.8; // arbitrary (changed by preferences)
	ptr->outsideColor.blue = 0.8; // arbitrary (changed by preferences)
	ptr->mousePointerColor = kTerminalView_MousePointerColorBlack;
	
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
		Float32		x0 = (STATIC_CAST(inStartX, SInt32) * ptr->canvasWidth) / kVectorInterpreter_MaxX;
		Float32		y0 = STATIC_CAST(ptr->canvasHeight, SInt32) -
							((STATIC_CAST(inStartY, SInt32) * ptr->canvasHeight) / kVectorInterpreter_MaxY);
		Float32		x1 = (STATIC_CAST(inEndX, SInt32) * ptr->canvasWidth) / kVectorInterpreter_MaxX;
		Float32		y1 = STATIC_CAST(ptr->canvasHeight, SInt32) -
							((STATIC_CAST(inEndY, SInt32) * ptr->canvasHeight) / kVectorInterpreter_MaxY);
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
		
		// set the color palette; this is done only when a view is set so that
		// dependent views (the matte) will also exist
		{
			Preferences_ContextRef		defaultFormat = nullptr;
			Preferences_Result			prefsResult = kPreferences_ResultOK;
			
			
			prefsResult = Preferences_GetDefaultContext(&defaultFormat, Quills::Prefs::FORMAT);
			assert(kPreferences_ResultOK == prefsResult);
			copyColorPreferences(ptr, defaultFormat);
		}
		
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

In addition, if a matte color is found, it is used to
change the color of any superview “background view”.

Returns the number of colors that were changed.

(3.1)
*/
UInt16
copyColorPreferences	(My_VectorCanvasPtr			inPtr,
						 Preferences_ContextRef		inSource,
						 Boolean					inSearchForDefaults)
{
	SInt16							currentIndex = 0;
	Preferences_Tag					currentPrefsTag = '----';
	CGFloatRGBColor					colorValue;
	TerminalView_BackgroundView*	backgroundViewOrNil = STATIC_CAST([inPtr->canvasView superviewWithClass:TerminalView_BackgroundView.class],
																		TerminalView_BackgroundView*);
	UInt16							result = 0;
	
	
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
	
	// the matte is displayed by a different view but for all practical
	// purposes it is updated whenever the canvas color changes
	if (nil == backgroundViewOrNil)
	{
		// there will be no view when this is called at initialization time
		//Console_Warning(Console_WriteLine, "unable to find matte view");
	}
	else
	{
		// the "outsideColor" is only used by the canvas itself in special
		// circumstances, such as when the user has zoomed the image out so
		// far that the image no longer fills the view; this color is also
		// used by an entirely different view (the matte) to render the
		// border at all times 
		currentPrefsTag = kPreferences_TagTerminalColorMatteBackground;
		if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, currentPrefsTag,
																sizeof(inPtr->outsideColor), &inPtr->outsideColor,
																inSearchForDefaults))
		{
			// the same color is used for the matte region, which is actually
			// rendered by an entirely different control that is always visible
			backgroundViewOrNil.exactColor = [NSColor colorWithCalibratedRed:inPtr->outsideColor.red
																				green:inPtr->outsideColor.green
																				blue:inPtr->outsideColor.blue
																				alpha:1.0];
			[backgroundViewOrNil setNeedsDisplay];
		}
	}
	
	// also update preferred mouse pointer color
	if (kPreferences_ResultOK == Preferences_ContextGetData(inSource, kPreferences_TagTerminalMousePointerColor,
															sizeof(inPtr->mousePointerColor),
															&inPtr->mousePointerColor,
															inSearchForDefaults))
	{
		++result;
		
		// immediately invalidate so the cursor is updated by the OS
		[[inPtr->canvasView window] invalidateCursorRectsForView:inPtr->canvasView];
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
					 CGFloatRGBColor&		outColor)
{
	outColor = inPtr->deviceColors[inZeroBasedIndex];
}// getPaletteColor


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
			[inPtr->drawingPathElements addObject:[[activePath copyWithEmptyPathForPurpose:inPurpose] autorelease]];
		}
	}
	result = [inPtr->drawingPathElements lastObject];
	assert(nil != result);
	
	return result;
}// pathElementWithPurpose


/*!
Changes the RGB color for the specified TEK color index.

(2016.09)
*/
void
setPaletteColor		(My_VectorCanvasPtr			inPtr,
					 SInt16						inZeroBasedIndex,
					 CGFloatRGBColor const&		inColor)
{
	inPtr->deviceColors[inZeroBasedIndex] = inColor;
}// setPaletteColor

} // anonymous namespace


#pragma mark -
@implementation VectorCanvas_Path


/*!
No-argument initializer; invokes "initWithPurpose:" using
"kVectorCanvas_PathPurposeGraphics".

(4.1)
*/
- (instancetype)
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
- (instancetype)
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
copyWithEmptyPathForPurpose:(VectorCanvas_PathPurpose)	aPurpose
{
	VectorCanvas_Path*	result = [[self.class alloc] init];
	
	
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
}// copyWithEmptyPathForPurpose:


@end // VectorCanvas_Path


#pragma mark -
@implementation VectorCanvas_View


#pragma mark Internally-Declared Properties


/*!
If a non-empty rectangle, the rendering of the view
displays this as a drag rectangle.  Used for mouse
events.
*/
@synthesize dragRectangle = _dragRectangle;

/*!
The location of the initial mouse-down when handling
drag rectangles.  Not meaningful otherwise.
*/
@synthesize dragStart = _dragStart;


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithFrame:(NSRect)		aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		_interpreterRef = nullptr;
		_dragRectangle = NSZeroRect;
		_dragStart = NSZeroPoint;
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
	VectorInterpreter_Release(&_interpreterRef);
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
	return _interpreterRef;
}
- (void)
setInterpreterRef:(VectorInterpreter_Ref)	anInterpreter
{
	if (_interpreterRef != anInterpreter)
	{
		if (nullptr != _interpreterRef)
		{
			VectorInterpreter_Release(&_interpreterRef);
		}
		if (nullptr != anInterpreter)
		{
			VectorCanvas_Ref	canvas = VectorInterpreter_ReturnCanvas(anInterpreter);
			
			
			UNUSED_RETURN(VectorCanvas_Result)VectorCanvas_SetCanvasNSView(canvas, self);
			VectorInterpreter_Retain(anInterpreter);
		}
		_interpreterRef = anInterpreter;
	}
}// setInterpreterRef


#pragma mark Actions: Commands_Printing


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
	#if MAC_OS_X_VERSION_MIN_REQUIRED < 1090 /* MAC_OS_X_VERSION_10_9 */
		[printInfo setOrientation:NSLandscapeOrientation];
	#else
		[printInfo setOrientation:NSPaperOrientationLandscape];
	#endif
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


#pragma mark Actions: Commands_StandardEditing


/*!
Copies the drawing in the canvas to the primary pasteboard.
(This should be triggered by a standard Copy command from the
Edit menu.)

(4.1)
*/
- (IBAction)
copy:(id)	sender
{
#pragma unused(sender)
	NSRect			imageBounds = self.frame;
	NSImage*		copiedImage = [[NSImage alloc] initWithSize:imageBounds.size];
	NSArray*		dataTypeArray = @[NSPDFPboardType, NSTIFFPboardType];
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
canCopy:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	return @(YES);
}


#pragma mark Actions: Commands_TextFormatting


/*!
Uses the name of the given sender (expected to be a menu item) to
find a Format set of Preferences from which to copy new font and
color information.

(4.0)
*/
- (IBAction)
performFormatByFavoriteName:(id)	sender
{
	VectorCanvas_Ref			canvasRef = VectorInterpreter_ReturnCanvas(self.interpreterRef);
	My_VectorCanvasAutoLocker	canvasPtr(gVectorCanvasPtrLocks(), canvasRef);
	BOOL						isError = YES;
	
	
	if ([[sender class] isSubclassOfClass:[NSMenuItem class]])
	{
		// use the specified preferences
		NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
		CFStringRef		collectionName = BRIDGE_CAST([asMenuItem title], CFStringRef);
		
		
		if ((nil != collectionName) && Preferences_IsContextNameInUse(Quills::Prefs::FORMAT, collectionName))
		{
			Preferences_ContextWrap		namedSettings(Preferences_NewContextFromFavorites
														(Quills::Prefs::FORMAT, collectionName),
														Preferences_ContextWrap::kAlreadyRetained);
			
			
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
	VectorCanvas_Ref			canvasRef = VectorInterpreter_ReturnCanvas(self.interpreterRef);
	My_VectorCanvasAutoLocker	canvasPtr(gVectorCanvasPtrLocks(), canvasRef);
	Preferences_ContextRef		defaultSettings = nullptr;
	BOOL						isError = YES;
	
	
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


#pragma mark Actions: Commands_VectorGraphicsModifying


/*!
Restores the view to normal scaling (when dragging the mouse
the user can zoom in, or double-click to perform this action).

(2020.12)
*/
- (IBAction)
performGraphicsCanvasResizeTo100Percent:(id)	sender
{
#pragma unused(sender)
	VectorCanvas_Ref			canvasRef = VectorInterpreter_ReturnCanvas(self.interpreterRef);
	My_VectorCanvasAutoLocker	canvasPtr(gVectorCanvasPtrLocks(), canvasRef);
	
	
	canvasPtr->viewScaleX = 1.0;
	canvasPtr->viewScaleY = 1.0;
	canvasPtr->unscaledZoomOriginX = 0;
	canvasPtr->unscaledZoomOriginY = 0;
	
	[TerminalWindow_InfoBubble sharedInfoBubble].stringValue = @"100%";
	[[TerminalWindow_InfoBubble sharedInfoBubble] moveToCenterScreen:self.window.screen]; 
	[[TerminalWindow_InfoBubble sharedInfoBubble] display];
	
	[self setNeedsDisplay];
}
- (id)
canPerformGraphicsCanvasResizeTo100Percent:(id <NSValidatedUserInterfaceItem>)		anItem
{
#pragma unused(anItem)
	return @(YES);
}


#pragma mark NSResponder


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
Responds to magnification events (such as pinching on a
touch pad) by scaling the vector graphic.

(2018.06)
*/
- (void)
magnifyWithEvent:(NSEvent*)		anEvent
{
	//NSSet*					touchSet = [anEvent touchesMatchingPhase:(NSTouchPhaseMoved) inView:self];
	VectorCanvas_Ref			canvasRef = VectorInterpreter_ReturnCanvas(self.interpreterRef);
	My_VectorCanvasAutoLocker	canvasPtr(gVectorCanvasPtrLocks(), canvasRef);
	
	
	// the "magnification" property is defined as a (typically tiny) amount
	// to be ADDED to a scaling factor, not multiplied
	if (0 != anEvent.magnification)
	{
		CGFloat const		kOldUnscaledSizeH = (NSWidth(self.frame) / canvasPtr->viewScaleX);
		
		
		// first update the scaling factors (multiple zooms, if appropriate)
		canvasPtr->viewScaleX += anEvent.magnification;
		//canvasPtr->viewScaleY += (anEvent.magnification * (NSWidth(self.frame) / NSHeight(self.frame))); // keep aspect ratio
		canvasPtr->viewScaleY += anEvent.magnification;
		
		// constrain values (IMPORTANT: should match other event handlers...)
		canvasPtr->viewScaleX = std::max< CGFloat >(0.25/* arbitrary */, canvasPtr->viewScaleX);
		canvasPtr->viewScaleY = std::max< CGFloat >(0.25/* arbitrary */, canvasPtr->viewScaleY);
		canvasPtr->viewScaleX = std::min< CGFloat >(8/* arbitrary */, canvasPtr->viewScaleX);
		canvasPtr->viewScaleY = std::min< CGFloat >(8/* arbitrary */, canvasPtr->viewScaleY);
		
		// scale the offset by a centered amount if possible
		{
			CGFloat const		kNewUnscaledSizeH = (NSWidth(self.frame) / canvasPtr->viewScaleX);
			
			
			canvasPtr->unscaledZoomOriginX += ((kOldUnscaledSizeH - kNewUnscaledSizeH) * 0.5);
			canvasPtr->unscaledZoomOriginY += ((kOldUnscaledSizeH - kNewUnscaledSizeH) * 0.5);
		}
		
		// display new zoom level to user (see also "mouseDown:")
		[TerminalWindow_InfoBubble sharedInfoBubble].stringValue = [NSString stringWithFormat:@"%d%%", (int)(canvasPtr->viewScaleX * 100.0)];
		[[TerminalWindow_InfoBubble sharedInfoBubble] moveToCenterScreen:self.window.screen]; 
		[[TerminalWindow_InfoBubble sharedInfoBubble] display];
		
		[self setNeedsDisplay];
	}
}// magnifyWithEvent:


/*!
Responds to mouse-down events by interacting with the vector
graphics terminal: drags define zoom regions, double-clicks
restore the default zoom level.

(2018.06)
*/
- (void)
mouseDown:(NSEvent*)	anEvent
{
	VectorCanvas_Ref			canvasRef = VectorInterpreter_ReturnCanvas(self.interpreterRef);
	My_VectorCanvasAutoLocker	canvasPtr(gVectorCanvasPtrLocks(), canvasRef);
	NSPoint						windowLocation = [anEvent locationInWindow];
	NSPoint						viewLocation = [self convertPoint:windowLocation fromView:nil];
	
	
	// constrain point to view (in case it was initiated from
	// a border view)
	if (viewLocation.x < 0)
	{
		viewLocation.x = 0;
	}
	if (viewLocation.y < 0)
	{
		viewLocation.y = 0;
	}
	if (viewLocation.x >= NSWidth(self.frame))
	{
		viewLocation.x = (NSWidth(self.frame) - 1);
	}
	if (viewLocation.y >= NSHeight(self.frame))
	{
		viewLocation.y = (NSHeight(self.frame) - 1);
	}
	
	if (anEvent.clickCount > 2)
	{
		// no action on triple-click, etc.
	}
	else if (2 == anEvent.clickCount)
	{
		// reset zoom on double-click
		[self performGraphicsCanvasResizeTo100Percent:nil];
	}
	else if (canvasPtr->ingin)
	{
		// report the location of the cursor
	#if 0
		{
			SInt32		lx = 0L;
			SInt32		ly = 0L;
			char		cursorReport[6];
			
			
			lx = ((SInt32)canvasPtr->xscale * (SInt32)viewLocation.x) / (SInt32)canvasPtr->width;
			ly = (SInt32)canvasPtr->yscale -
					((SInt32)canvasPtr->yscale * (SInt32)(self.frame.size.height - viewLocation.y)) / (SInt32)canvasPtr->height;
			
			// the report is exactly 5 characters long
			if (0 == VectorInterpreter_FillInPositionReport(canvasPtr->interpreter, STATIC_CAST(lx, UInt16), STATIC_CAST(ly, UInt16),
															' ', cursorReport))
			{
				NSLog(@"report: %d, %d (%d, %d)", (int)lx, (int)ly, canvasPtr->xscale * (int)canvasPtr->width, canvasPtr->yscale * (int)canvasPtr->height);
				//Session_SendData(canvasPtr->session, cursorReport, 5);
				//Session_SendData(canvasPtr->session, " \r\n", 3);
			}
		}
	#endif
	}
	else
	{
		// zoom to area specified by rectangle
		BOOL	dragEnded = NO;
		BOOL	flagCancellation = NO;
		
		
		self.dragStart = NSMakePoint(viewLocation.x, viewLocation.y);
		self.dragRectangle = NSZeroRect;
		[self setNeedsDisplay];
		
		while (NO == dragEnded)
		{
			// TEMPORARY; in 10.10 SDK can probably switch to the NSWindow
			// method "trackEventsMatchingMask:timeout:mode:handler:"
			NSEvent*	eventObject = [NSApp nextEventMatchingMask:(NSLeftMouseDraggedMask |
																		NSLeftMouseUpMask |
																		NSFlagsChangedMask |
																		NSKeyDownMask |
																		NSKeyUpMask)
																	untilDate:nil
																	inMode:NSEventTrackingRunLoopMode
																	dequeue:YES];
			
			
			if (nil != eventObject)
			{
				switch (eventObject.type)
				{
				case NSLeftMouseDragged:
					// update rectangle
					if (NO == flagCancellation)
					{
						NSPoint		newWindowLocation = [eventObject locationInWindow];
						NSPoint		newViewLocation = [self convertPoint:newWindowLocation fromView:nil];
						CGRect		asCGRect = NSRectToCGRect(self.dragRectangle);
						
						
						asCGRect.origin.x = self.dragStart.x;
						asCGRect.origin.y = self.dragStart.y;
						asCGRect.size.width = (newViewLocation.x - asCGRect.origin.x);
						asCGRect.size.height = (newViewLocation.y - asCGRect.origin.y);
						asCGRect = CGRectStandardize(asCGRect);
						[self setNeedsDisplayInRect:NSUnionRect(self.dragRectangle, NSRectFromCGRect(asCGRect))];
						self.dragRectangle = NSRectFromCGRect(asCGRect);
						
						// INCOMPLETE: report mouse location to session
					}
					break;
				
				case NSFlagsChanged:
					// modifier keys changed
					if (NO == flagCancellation)
					{
						// (currently not used for anything)
					}
					break;
				
				case NSLeftMouseUp:
					dragEnded = YES;
					break;
				
				case NSKeyDown:
					// if the Escape key is pressed, cancel the drag; do not
					// actually break the loop here, as it is crucial to still
					// wait until the matching mouse-up event can be dequeued
					// (though the rectangle is still hidden immediately and
					// previous drags will have no effect)
					if (kVK_Escape == eventObject.keyCode)
					{
						flagCancellation = YES;
						self.dragRectangle = NSZeroRect;
						[self setNeedsDisplay];
					}
					break;
				
				case NSKeyUp:
					// do nothing
					break;
				
				default:
					// ???
					Console_Warning(Console_WriteValue, "unexpected event received; terminating loop", eventObject.type);
					dragEnded = YES;
					break;
				}
			}
		}
		
		// respond to mouse-up by zooming to selected area
		if ((NO == NSIsEmptyRect(self.dragRectangle)) && (NO == flagCancellation))
		{
			// capture previous zoom to allow further zooming
			CGFloat const		kDragPixelWidth = NSWidth(self.dragRectangle);
			CGFloat const		kDragPixelHeight = NSHeight(self.dragRectangle);
			CGFloat const		kBaseDimension = ((kDragPixelWidth > kDragPixelHeight) ? kDragPixelWidth : kDragPixelHeight);
			CGFloat const		kPreviousScaleX = ((canvasPtr->viewScaleX > 0) ? canvasPtr->viewScaleX : 1.0);
			CGFloat const		kPreviousScaleY = ((canvasPtr->viewScaleY > 0) ? canvasPtr->viewScaleY : 1.0);
			
			
			// first update the scaling factors (multiple zooms, if appropriate);
			// use the same dimension for both to avoid further image distortion
			// beyond that caused by the size of the window
			canvasPtr->viewScaleX = kPreviousScaleX * (NSWidth(self.frame) / kBaseDimension);
			canvasPtr->viewScaleY = kPreviousScaleY * (NSHeight(self.frame) / kBaseDimension);
			
			// constrain values (IMPORTANT: should match other event handlers...)
			canvasPtr->viewScaleX = std::min< CGFloat >(8/* arbitrary */, canvasPtr->viewScaleX);
			canvasPtr->viewScaleY = std::min< CGFloat >(8/* arbitrary */, canvasPtr->viewScaleY);
			
			// use the new rectangle position to determine a suitable drawing offset
			canvasPtr->unscaledZoomOriginX += (self.dragRectangle.origin.x / kPreviousScaleX);
			canvasPtr->unscaledZoomOriginY += (self.dragRectangle.origin.y / kPreviousScaleY);
			
			// display new zoom level to user (see also "magnifyWithEvent:")
			[TerminalWindow_InfoBubble sharedInfoBubble].stringValue = [NSString stringWithFormat:@"%d%%", (int)(canvasPtr->viewScaleX * 100.0), nil];
			[[TerminalWindow_InfoBubble sharedInfoBubble] moveToCenterScreen:self.window.screen]; 
			[[TerminalWindow_InfoBubble sharedInfoBubble] display];
			
			[self setNeedsDisplay];
		}
		
		// hide zoom rectangle
		self.dragStart = NSZeroPoint;
		self.dragRectangle = NSZeroRect;
		[self setNeedsDisplay];
	}
}// mouseDown:


/*!
Responds to mouse-up events by discarding them.  This is
important to balance the locally-handled mouse-down
handler and prevent events from being otherwise forwarded
to the next responder.

(2018.06)
*/
- (void)
mouseUp:(NSEvent*)		anEvent
{
#pragma unused(anEvent)
	// do nothing
}// mouseUp:


#pragma mark NSUserInterfaceValidations


/*!
The standard Cocoa interface for validating things like menu
commands.  This method is present here because it must be in
the same class as the methods that perform actions; if the
action methods move, their validation code must move as well.

Returns true only if the specified item should be available
to the user.

(2020.12)
*/
- (BOOL)
validateUserInterfaceItem:(id <NSObject, NSValidatedUserInterfaceItem>)		anItem
{
	// the call below enables the magic that allows validation to be automatic
	// whenever a "canPerform..." method for a "perform..." action also exists
	BOOL	result = [[Commands_Executor sharedExecutor] validateAction:anItem.action sender:self sourceItem:anItem];
	
	
	return result;
}// validateUserInterfaceItem:


#pragma mark NSView


/*!
Render the specified part of a vector graphics canvas.

(4.1)
*/
- (void)
drawRect:(NSRect)	rect
{
	VectorCanvas_Ref			canvasRef = VectorInterpreter_ReturnCanvas(self.interpreterRef);
	My_VectorCanvasAutoLocker	canvasPtr(gVectorCanvasPtrLocks(), canvasRef);
	
	
	[super drawRect:rect];
	
	if (nullptr != canvasPtr)
	{
		CGContextRef	drawingContext = [[NSGraphicsContext currentContext] CGContext];
		
		
		// draw the background (unless this is for printing)
		if (nil == [NSPrintOperation currentOperation])
		{
			CGContextSetRGBFillColor(drawingContext, canvasPtr->outsideColor.red, canvasPtr->outsideColor.green,
										canvasPtr->outsideColor.blue, 1.0/* alpha */);
			CGContextSetAllowsAntialiasing(drawingContext, false); // avoid artifacts
			CGContextFillRect(drawingContext, NSRectToCGRect(rect));
			CGContextSetAllowsAntialiasing(drawingContext, true);
		}
		
		// draw the vector graphics
		[NSGraphicsContext saveGraphicsState];
		[self renderDrawingInCurrentFocusWithRect:self.frame];
		[NSGraphicsContext restoreGraphicsState]; // needed because the rendering would otherwise scale all subsequent drawing
		
		// draw any zoom rectangle (only appears during mouse tracking) 
		if (NO == NSIsEmptyRect(self.dragRectangle))
		{
			CGFloat		dashElements[] = { 4.0, 2.0 };
			
			
			// TEMPORARY; may want the selection color to be user-customizable
			// (or read it from the Format collection)
			CGContextSetAllowsAntialiasing(drawingContext, false); // avoid artifacts
			CGContextSetRGBFillColor(drawingContext, 1.0, 1.0, 1.0, 0.3/* alpha */);
			CGContextFillRect(drawingContext, NSRectToCGRect(NSInsetRect(self.dragRectangle, 1.0, 1.0)));
			CGContextSetRGBStrokeColor(drawingContext, 1.0, 0.0, 0.0, 1.0/* alpha */);
			CGContextSetLineWidth(drawingContext, 1.0);
			CGContextSetLineDash(drawingContext, 0/* phase */, dashElements, sizeof(dashElements) / sizeof(*dashElements));
			// note: cannot use shadow without creating artifacts in partial-view-refresh scenario
			//CGContextSetShadow(drawingContext, CGSizeMake(1.2f, -1.2f)/* offset; arbitrary */, 3.0f/* blur; arbitrary */);
			CGContextStrokeRect(drawingContext, NSRectToCGRect(NSInsetRect(self.dragRectangle, 1.5, 1.5)));
			CGContextSetAllowsAntialiasing(drawingContext, true);
		}
	}
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
Returns a contextual menu for the vector canvas view.

(2020.12)
*/
- (NSMenu*)
menuForEvent:(NSEvent*)		anEvent
{
#pragma unused(anEvent)
	NSMenu*						result = nil;
	VectorCanvas_Ref			canvasRef = VectorInterpreter_ReturnCanvas(self.interpreterRef);
	My_VectorCanvasAutoLocker	canvasPtr(gVectorCanvasPtrLocks(), canvasRef);
	
	
	if (nullptr != canvasPtr)
	{
		// display a contextual menu
		result = [[[NSMenu alloc] initWithTitle:@""] autorelease];
		
		// set up the contextual menu
		//[result setAllowsContextMenuPlugIns:NO];
		NSMenuItem*		newItem = nil;
		CFStringRef		commandText = nullptr;
		
		
		ContextSensitiveMenu_Init();
		
		// determine if view is currently zoomed-in on some region
		if ((canvasPtr->viewScaleX != 1.0) || (canvasPtr->viewScaleY != 1.0))
		{
			// clipboard-related commands
			ContextSensitiveMenu_NewItemGroup();
			
			if (UIStrings_Copy(kUIStrings_ContextualMenuCopyToClipboard, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(copy:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(result, newItem);
					[newItem release], newItem = nil;
				}
				CFRelease(commandText), commandText = nullptr;
			}
			
			// user macros
			// (currently, there is no way for macros to operate on vector graphics)
			//MacroManager_AddContextualMenuGroup(result, nullptr/* current macro set */, true/* search defaults */); // implicitly calls ContextSensitiveMenu_NewItemGroup() again
			
			// other graphics-selection-related commands
			ContextSensitiveMenu_NewItemGroup();
			
			// add a command to “zoom back to 100%”
			if (UIStrings_Copy(kUIStrings_ContextualMenuZoomCanvas100Percent, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(performGraphicsCanvasResizeTo100Percent:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(result, newItem);
					[newItem release], newItem = nil;
				}
				CFRelease(commandText), commandText = nullptr;
			}
			
			if (UIStrings_Copy(kUIStrings_ContextualMenuPrintSelectedCanvasRegion, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(performPrintSelection:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(result, newItem);
					[newItem release], newItem = nil;
				}
				CFRelease(commandText), commandText = nullptr;
			}
		}
		else
		{
			// clipboard-related commands
			ContextSensitiveMenu_NewItemGroup();
			
			if (UIStrings_Copy(kUIStrings_ContextualMenuCopyToClipboard, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(copy:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(result, newItem);
					[newItem release], newItem = nil;
				}
				CFRelease(commandText), commandText = nullptr;
			}
			
			// window arrangement menu items
			ContextSensitiveMenu_NewItemGroup();
			
			if (EventLoop_IsWindowFullScreen(self.window))
			{
				if (UIStrings_Copy(kUIStrings_ContextualMenuFullScreenExit, commandText).ok())
				{
					newItem = Commands_NewMenuItemForAction(@selector(toggleFullScreen:), commandText, true/* must be enabled */);
					if (nil != newItem)
					{
						ContextSensitiveMenu_AddItem(result, newItem);
						[newItem release], newItem = nil;
					}
					CFRelease(commandText), commandText = nullptr;
				}
			}
			
			if (UIStrings_Copy(kUIStrings_ContextualMenuArrangeAllInFront, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(performArrangeInFront:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(result, newItem);
					[newItem release], newItem = nil;
				}
				CFRelease(commandText), commandText = nullptr;
			}
			
			if (UIStrings_Copy(kUIStrings_ContextualMenuRenameThisWindow, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(performRename:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(result, newItem);
					[newItem release], newItem = nil;
				}
				CFRelease(commandText), commandText = nullptr;
			}
			
			// vector graphics menu items
			ContextSensitiveMenu_NewItemGroup();
			
			if (UIStrings_Copy(kUIStrings_ContextualMenuPrintScreen, commandText).ok())
			{
				newItem = Commands_NewMenuItemForAction(@selector(performPrintScreen:), commandText, true/* must be enabled */);
				if (nil != newItem)
				{
					ContextSensitiveMenu_AddItem(result, newItem);
					[newItem release], newItem = nil;
				}
				CFRelease(commandText), commandText = nullptr;
			}
		}
	}
	
	return result;
}// menuForEvent:


/*!
Invoked by NSView whenever it’s necessary to define the regions
that change the mouse pointer’s shape.

(4.1)
*/
- (void)
resetCursorRects
{
	NSCursor*					newCursor = nil;
	VectorCanvas_Ref			canvasRef = VectorInterpreter_ReturnCanvas(self.interpreterRef);
	My_VectorCanvasAutoLocker	canvasPtr(gVectorCanvasPtrLocks(), canvasRef);
	
	
	if (nullptr != canvasPtr)
	{
		// since vector graphics windows support Format settings
		// similar to terminal windows, use any custom mouse pointer
		// for these as well
		switch (canvasPtr->mousePointerColor)
		{
		case kTerminalView_MousePointerColorBlack:
			{
				static NSCursor*	gCachedCursor = nil;
				
				
				if (nil == gCachedCursor)
				{
					// IMPORTANT: specified hot-spot should be synchronized with the image data
					gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorBlackCrosshairs"]
																	hotSpot:NSMakePoint(15, 15)];
				}
				newCursor = gCachedCursor;
			}
			break;
		
		case kTerminalView_MousePointerColorWhite:
			{
				static NSCursor*	gCachedCursor = nil;
				
				
				if (nil == gCachedCursor)
				{
					// IMPORTANT: specified hot-spot should be synchronized with the image data
					gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorWhiteCrosshairs"]
																	hotSpot:NSMakePoint(15, 15)];
				}
				newCursor = gCachedCursor;
			}
			break;
		
		case kTerminalView_MousePointerColorRed:
		default:
			{
				static NSCursor*	gCachedCursor = nil;
				
				
				if (nil == gCachedCursor)
				{
					// IMPORTANT: specified hot-spot should be synchronized with the image data
					gCachedCursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:@"CursorCrosshairs"]
																	hotSpot:NSMakePoint(15, 15)];
				}
				newCursor = gCachedCursor;
			}
			break;
		}
	}
	
	if (nil == newCursor)
	{
		// some problem finding custom cursor and/or canvas structure;
		// use system default cursor for now
		newCursor = [NSCursor crosshairCursor];
	}
	
	[self addCursorRect:[self bounds] cursor:newCursor];
}// resetCursorRects


@end // VectorCanvas_View


#pragma mark -
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
	VectorCanvas_Ref			canvasRef = VectorInterpreter_ReturnCanvas(self.interpreterRef);
	My_VectorCanvasAutoLocker	canvasPtr(gVectorCanvasPtrLocks(), canvasRef);
	
	
	if (nullptr != canvasPtr)
	{
		CGContextRef	drawingContext = [[NSGraphicsContext currentContext] CGContext];
		CGRect			contentBounds = NSRectToCGRect(aRect);
		BOOL			isPrinting = (nil != [NSPrintOperation currentOperation]);
		
		
		// draw the vector graphics; this is achieved by iterating over
		// stored drawing commands and replicating them
		if (nil != canvasPtr->drawingPathElements)
		{
			CGFloat const		viewScaledPixelWidth = (canvasPtr->viewScaleX * contentBounds.size.width);
			CGFloat const		viewScaledPixelHeight = (canvasPtr->viewScaleY * contentBounds.size.height);
			CGFloat const		canvasDisplayWidth = STATIC_CAST(canvasPtr->canvasWidth, CGFloat);
			CGFloat const		canvasDisplayHeight = STATIC_CAST(canvasPtr->canvasHeight, CGFloat);
			CGFloatRGBColor		scratchColor;
			SInt16				currentFillColorIndex = 0;
			SInt16				currentStrokeColorIndex = 0;
			
			
			// scale (a possibly zoomed part of) the drawing to fill the view
			// and then shift if the zoomed region starts from somewhere else
			if ((canvasDisplayWidth > 0) && (canvasDisplayHeight > 0))
			{
				CGContextTranslateCTM(drawingContext, -(canvasPtr->viewScaleX * canvasPtr->unscaledZoomOriginX),
										-(canvasPtr->viewScaleY * canvasPtr->unscaledZoomOriginY));
				CGContextScaleCTM(drawingContext, viewScaledPixelWidth / canvasDisplayWidth,
									viewScaledPixelHeight / canvasDisplayHeight);
			}
			
			// make the shadow apply to the entire drawing instead of
			// risking shadow overlap for nearby lines (also, this
			// ought to render a bit faster)
			CGContextBeginTransparencyLayerWithRect(drawingContext, contentBounds, nullptr/* extra info dictionary */);
			
			// draw background of graphic itself
			unless (isPrinting)
			{
				SInt16				backgroundColorIndex = VectorInterpreter_ReturnBackgroundColor(canvasPtr->interpreter);
				CGFloatRGBColor		backgroundColor;
				
				
				assert((backgroundColorIndex >= 0) && (backgroundColorIndex < kMy_MaxColors));
				getPaletteColor(canvasPtr, backgroundColorIndex, backgroundColor);
				CGContextSetRGBFillColor(drawingContext, backgroundColor.red, backgroundColor.green,
											backgroundColor.blue, 1.0/* alpha */);
				CGContextSetAllowsAntialiasing(drawingContext, false); // avoid artifacts
				CGContextFillRect(drawingContext, CGRectMake(0, 0, canvasDisplayWidth, canvasDisplayHeight)); // see CTM changes above
				CGContextSetAllowsAntialiasing(drawingContext, true);
			}
			
			// initialize context state
			unless (isPrinting)
			{
				CGContextSetShadow(drawingContext, CGSizeMake(2.2f, -2.2f)/* offset; arbitrary */, 6.0f/* blur; arbitrary */);
			}
			
			// render each piece of the drawing; for a drawing that always uses the
			// same colors and line sizes, etc. this loop will only iterate once
			for (VectorCanvas_Path* pathElement in canvasPtr->drawingPathElements)
			{
				// update graphics context state if it should change
				if (pathElement->fillColorIndex != currentFillColorIndex)
				{
					assert((pathElement->fillColorIndex >= 0) && (pathElement->fillColorIndex < kMy_MaxColors));
					if ((isPrinting) && (kMy_ColorIndexBackground == pathElement->fillColorIndex))
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
					if ((isPrinting) && (kMy_ColorIndexForeground == pathElement->strokeColorIndex))
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
				[pathElement->bezierPath setLineWidth:std::max< CGFloat >(std::min< CGFloat >((contentBounds.size.width / canvasDisplayWidth) * pathElement->normalLineWidth,
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
			
			// end call to CGContextBeginTransparencyLayerWithRect() above
			CGContextEndTransparencyLayer(drawingContext);
		}
	}
}// renderDrawingInCurrentFocusWithRect:


@end // VectorCanvas_View (VectorCanvas_ViewInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
