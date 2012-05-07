/*!	\file VectorCanvas.cp
	\brief Renders vector graphics, onscreen or offscreen.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

// library includes
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaFuture.objc++.h>
#import <ColorUtilities.h>
#import <CommonEventHandlers.h>
#import <HIViewWrap.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlockReferenceLocker.template.h>
#import <MemoryBlocks.h>
#import <NIBLoader.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>
#import <StringUtilities.h>

// Mac includes
#import <Carbon/Carbon.h>

// application includes
#import "AppResources.h"
#import "Clipboard.h"
#import "Console.h"
#import "ConstantsRegistry.h"
#import "DebugInterface.h"
#import "DialogUtilities.h"
#import "Preferences.h"
#import "Session.h"
#import "VectorInterpreter.h"
#import "WindowTitleDialog.h"



#pragma mark Constants
namespace {

/*!
IMPORTANT

The following values MUST agree with the view IDs in
the NIB "TEKWindow.nib".
*/
HIViewID const	idMyCanvas		= { 'Cnvs', 0/* ID */ };

UInt16 const	kMy_MaximumX = 4095;
UInt16 const	kMy_MaximumY = 3139;	// TEMPORARY - figure out where the hell this value comes from
Float32 const	kMy_DefaultStrokeWidth = 0.5;

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
@interface VectorCanvas_Path : NSObject
{
@public
	NSBezierPath*		bezierPath;
	CGPathDrawingMode	drawingMode;
	SInt16				fillColorIndex;
	SInt16				strokeColorIndex;
}

@end


namespace {

typedef std::vector< CGDeviceColor >	My_CGColorList;

/*!
Internal representation of a VectorCanvas_Ref.
*/
struct My_VectorCanvas
{
	OSType					id;
	VectorCanvas_Ref		selfRef;
	VectorCanvas_Target		target;
	VectorInterpreter_ID	interpreter;
	SessionRef				session;
	WindowTitleDialog_Ref	renameDialog;
	EventHandlerUPP			closeUPP;
	EventHandlerRef			closeHandler;
	My_CGColorList			deviceColors;
	SInt16					xorigin;
	SInt16					yorigin;
	SInt16					xscale;
	SInt16					yscale;
	SInt16					ingin;
	SInt16					width;
	SInt16					height;
	VectorCanvas_WindowController*	windowController;
	NSMutableArray*			drawingPathElements;
	NSBezierPath*			scrapPath;
	// the following are deprecated (the Carbon interface will eventually go away)
	HIWindowRef				owningWindow;
	HIViewRef				canvas;
	CarbonEventHandlerWrap	canvasDrawHandler;
	RgnHandle				currentPathRgn;
};
typedef My_VectorCanvas*		My_VectorCanvasPtr;
typedef My_VectorCanvas const*	My_VectorCanvasConstPtr;

typedef MemoryBlockPtrLocker< VectorCanvas_Ref, My_VectorCanvas >			My_VectorCanvasPtrLocker;
typedef LockAcquireRelease< VectorCanvas_Ref, My_VectorCanvas >				My_VectorCanvasAutoLocker;
typedef MemoryBlockReferenceLocker< VectorCanvas_Ref, My_VectorCanvas >		My_VectorCanvasReferenceLocker;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

UInt16		copyColorPreferences	(My_VectorCanvasPtr, Preferences_ContextRef, Boolean = true);
void		getPaletteColor			(My_VectorCanvasPtr, SInt16, CGDeviceColor&);
void		getPaletteColor			(My_VectorCanvasPtr, SInt16, RGBColor&);
void		handleMouseDown			(My_VectorCanvasPtr, Point);
Boolean		inSplash				(Point, Point);
OSStatus	receiveCanvasDraw		(EventHandlerCallRef, EventRef, void*);
OSStatus	receiveWindowClosing	(EventHandlerCallRef, EventRef, void*);
void		setPaletteColor			(My_VectorCanvasPtr, SInt16, RGBColor const&);
SInt16		setPortCanvasPort		(My_VectorCanvasPtr);

} // anonymous namespace

@interface VectorCanvas_View (VectorCanvas_ViewInternal)

// accessors

- (void*)
internalPtr;
- (My_VectorCanvasPtr)
internalPtrAsVectorCanvasPtr;
- (void)
setInternalPtr:(void*)_;

// other methods

- (void)
renderDrawingInCurrentFocusWithRect:(NSRect)_;

@end // VectorCanvas_View (VectorCanvas_ViewInternal)

@interface VectorCanvas_WindowController (VectorCanvas_WindowControllerInternal)

// accessors

- (void*)
internalPtr;
- (My_VectorCanvasPtr)
internalPtrAsVectorCanvasPtr;
- (void)
setInternalPtr:(void*)_;

@end // VectorCanvas_WindowController (VectorCanvas_WindowControllerInternal)

#pragma mark Variables
namespace {

My_VectorCanvasPtrLocker&			gVectorCanvasPtrLocks ()	{ static My_VectorCanvasPtrLocker x; return x; }
My_VectorCanvasReferenceLocker&		gVectorCanvasRefLocks ()	{ static My_VectorCanvasReferenceLocker x; return x; }

long						RGMlastclick = 0L;

// the following variables are used in bitmap mode
Boolean						gRGMPbusy = false;	// is device already in use?
SInt16						gRGMPwidth = 0;
SInt16						gRGMPheight = 0;
SInt16						gRGMPxoffset = 0;
SInt16						gRGMPyoffset = 0;

} // anonymous namespace


#pragma mark Public Methods

/*!
Call this routine before anything else in this module.

(3.0)
*/
void
VectorCanvas_Init ()
{
	// set up bitmap mode
	gRGMPbusy = false;
	//gRGMPwidth = kVectorInterpreter_MaxX;
	//gRGMPheight = kVectorInterpreter_MaxY;
	gRGMPxoffset = 0;
	gRGMPyoffset = 0;
}// Init


/*!
Creates a new vector graphics canvas that targets a Mac window.
Returns nullptr on failure.

The canvas is implicitly retained; VectorCanvas_Retain() is
only needed to place additional locks on the reference.

(3.0)
*/
VectorCanvas_Ref
VectorCanvas_New	(VectorInterpreter_ID	inID,
					 VectorCanvas_Target	inTarget)
{
	My_VectorCanvasPtr		ptr = new My_VectorCanvas;
	VectorCanvas_Ref		result = REINTERPRET_CAST(ptr, VectorCanvas_Ref);
	Boolean					isCocoaView = DebugInterface_CocoaBasedVectorGraphics();
	
	
	VectorCanvas_Retain(result);
	
	ptr->id = 'RGMW';
	ptr->selfRef = result;
	ptr->target = inTarget;
	
	// for now, choose between Cocoa/CG and Carbon/QuickDraw implementations
	if (isCocoaView)
	{
		// load the NIB containing the canvas
		ptr->windowController = [[VectorCanvas_WindowController alloc] initWithInternalPtr:ptr];
		
		// create regions for paths; the “complete path” represents the
		// contents of the entire drawing to be rendered and the “scrap
		// path“ represents something used temporarily by drawing commands
		ptr->drawingPathElements = [[NSMutableArray alloc] initWithCapacity:10/* arbitrary; expands as needed */];
		ptr->scrapPath = [[NSBezierPath bezierPath] retain];
		
		// TEMPORARY; for now, initialize but ignore Carbon equivalents
		ptr->owningWindow = nullptr;
		ptr->currentPathRgn = nullptr;
	}
	else
	{
		// initialize but do not use the Cocoa version
		ptr->windowController = nil;
		ptr->drawingPathElements = nil;
		ptr->scrapPath = nil;
		
		// create a region for temporary paths
		ptr->currentPathRgn = Memory_NewRegion();
		
		// load the NIB containing this dialog (automatically finds the right localization)
		ptr->owningWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
										CFSTR("TEKWindow"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
		
		// associate this canvas with the window so that it can be found by other things;
		// NOTE that this is just the simplest thing for now, but a better implementation
		// would be to use *control* properties and a custom HIView class, associating
		// the canvas directly with the view that renders it
		{
			OSStatus	error = SetWindowProperty(ptr->owningWindow, AppResources_ReturnCreatorCode(),
													kConstantsRegistry_WindowPropertyTypeVectorCanvas,
													sizeof(ptr->selfRef), &ptr->selfRef);
			assert_noerr(error);
		}
		
		// install a close handler so TEK windows are detached properly
		{
			EventTypeSpec const		whenWindowClosing[] =
									{
										{ kEventClassWindow, kEventWindowClose }
									};
			OSStatus				error = noErr;
			
			
			ptr->closeUPP = NewEventHandlerUPP(receiveWindowClosing);
			error = InstallWindowEventHandler(ptr->owningWindow, ptr->closeUPP,
												GetEventTypeCount(whenWindowClosing), whenWindowClosing,
												ptr->selfRef/* user data */,
												&ptr->closeHandler/* event handler reference */);
			assert_noerr(error);
		}
	}
	ptr->renameDialog = nullptr;
	
	ptr->interpreter = inID;
	ptr->session = nullptr;
	ptr->xorigin = 0;
	ptr->yorigin = 0;
	ptr->xscale = kMy_MaximumX;
	ptr->yscale = kMy_MaximumY;
	ptr->width = 400;
	ptr->height = 300;
	ptr->ingin = 0;
	
	// set up bitmap mode
	if (kVectorCanvas_TargetQuickDrawPicture == ptr->target)
	{
		gRGMPbusy = true;
		//gRGMPwidth = kVectorInterpreter_MaxX;
		//gRGMPheight = kVectorInterpreter_MaxY;
		gRGMPxoffset = 0;
		gRGMPyoffset = 0;
	}
	
	// create a color palette for this window
	ptr->deviceColors.resize(kMy_MaxColors);
	{
		Preferences_ContextRef		defaultFormat = nullptr;
		Preferences_Result			prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_GetDefaultContext(&defaultFormat, Quills::Prefs::FORMAT);
		assert(kPreferences_ResultOK == prefsResult);
		copyColorPreferences(ptr, defaultFormat);
	}
	
	if (isCocoaView)
	{
		[[ptr->windowController window] makeKeyAndOrderFront:nil];
	}
	else
	{
		HIViewWrap		canvasView(idMyCanvas, ptr->owningWindow);
		
		
		// install this handler last so drawing cannot occur prematurely
		ptr->canvas = canvasView;
		ptr->canvasDrawHandler.install(GetControlEventTarget(canvasView), receiveCanvasDraw,
										CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlDraw),
										result/* context */);
		assert(ptr->canvasDrawHandler.isInstalled());
		
		// set the window kind, for the benefit of code that varies by window type
		SetWindowKind(ptr->owningWindow, WIN_TEK);
		
		{
			HIWindowRef		frontWindow = GetFrontWindowOfClass(kDocumentWindowClass, true/* visible */);
			
			
			// unless the front window is another graphic, do not force it in front
			if (WIN_TEK != GetWindowKind(frontWindow)) SendBehind(ptr->owningWindow, frontWindow);
			ShowWindow(ptr->owningWindow);
		}
	}
	
	return result;
}// New


/*!
Adds a lock on the specified reference.  This indicates you
are using the canvas, so attempts by anyone else to delete
the canvas with VectorCanvas_Release() will fail until you
release your lock (and everyone else releases locks they
may have).

(4.0)
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

(4.0)
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
			
			
			if (nil != ptr->windowController)
			{
				[ptr->windowController release], ptr->windowController = nil;
			}
			
			if (nullptr != ptr->renameDialog)
			{
				WindowTitleDialog_Dispose(&ptr->renameDialog);
			}
			
			setPortCanvasPort(ptr);
			if (nullptr != ptr->session)
			{
				Session_TEKDetachTargetGraphic(ptr->session);
			}
			RemoveEventHandler(ptr->closeHandler), ptr->closeHandler = nullptr;
			DisposeEventHandlerUPP(ptr->closeUPP), ptr->closeUPP = nullptr;
			DisposeWindow(ptr->owningWindow);
			
			[ptr->drawingPathElements release], ptr->drawingPathElements = nil;
			[ptr->scrapPath release], ptr->scrapPath = nil;
			
			if (nullptr != ptr->currentPathRgn)
			{
				Memory_DisposeRegion(&ptr->currentPathRgn);
			}
			
			if (kVectorCanvas_TargetQuickDrawPicture == ptr->target)
			{
				gRGMPbusy = false;
			}
		}
		delete *(REINTERPRET_CAST(inoutRefPtr, My_VectorCanvasPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_AudioEvent		(VectorCanvas_Ref		UNUSED_ARGUMENT(inRef))
{
}// AudioEvent


/*!
Returns a copy of the window title string, or nullptr on error.

(3.1)
*/
void
VectorCanvas_CopyTitle	(VectorCanvas_Ref	inRef,
						 CFStringRef&		outTitle)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	NSWindow*					owningWindow = [ptr->windowController window];
	
	
	outTitle = (CFStringRef)[[owningWindow title] copy];
}// CopyTitle


/*!
Renders a straight line between two points expressed in
canvas coordinates.

You can choose to target the scrap path via the "inTarget"
parameter, effectively drawing a line that is cached instead
of being added immediately to the main drawing.

\retval kVectorCanvas_ResultOK
if there are no errors

\retval kVectorCanvas_ResultInvalidReference
if the specified canvas is unrecognized

\retval kVectorCanvas_ResultUnsupportedAction
if the current canvas target does not support this operation

(3.1)
*/
VectorCanvas_Result
VectorCanvas_DrawLine	(VectorCanvas_Ref			inRef,
						 SInt16						inStartX,
						 SInt16						inStartY,
						 SInt16						inEndX,
						 SInt16						inEndY,
						 VectorCanvas_PathTarget	inTarget)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	VectorCanvas_Result			result = kVectorCanvas_ResultInvalidReference;
	
	
	if (nullptr != ptr)
	{
		SInt32		xl0 = (STATIC_CAST(inStartX, SInt32) * ptr->width) / kVectorInterpreter_MaxX;
		SInt32		yl0 = STATIC_CAST(ptr->height, SInt32) -
							((STATIC_CAST(inStartY, SInt32) * ptr->height) / kVectorInterpreter_MaxY);
		SInt32		xl1 = (STATIC_CAST(inEndX, SInt32) * ptr->width) / kVectorInterpreter_MaxX;
		SInt32		yl1 = STATIC_CAST(ptr->height, SInt32) -
							((STATIC_CAST(inEndY, SInt32) * ptr->height) / kVectorInterpreter_MaxY);
		
		
		if (nil != ptr->windowController)
		{
			// Cocoa window
			switch (ptr->target)
			{
			case kVectorCanvas_TargetScreenPixels:
				if (kVectorCanvas_PathTargetScrap == inTarget)
				{
					// line is being added to a temporary scrap path, not the main drawing
					assert(nil != ptr->scrapPath);
					[ptr->scrapPath moveToPoint:NSMakePoint(xl0, yl0)];
					[ptr->scrapPath lineToPoint:NSMakePoint(xl1, yl1)];
				}
				else
				{
					// line is being added to the main drawing
					VectorCanvas_Path*	currentElement = nil;
					
					
					assert(nil != ptr->drawingPathElements);
					if ([ptr->drawingPathElements count] == 0)
					{
						[ptr->drawingPathElements addObject:[[VectorCanvas_Path alloc] init]];
					}
					currentElement = [ptr->drawingPathElements lastObject];
					assert(nil != currentElement);
					[currentElement->bezierPath moveToPoint:NSMakePoint(xl0, yl0)];
					[currentElement->bezierPath lineToPoint:NSMakePoint(xl1, yl1)];
				}
				break;
			
			case kVectorCanvas_TargetQuickDrawPicture:
			default:
				// not supported
				result = kVectorCanvas_ResultUnsupportedAction;
				break;
			}
		}
		else
		{
			// legacy Carbon and QuickDraw view
			switch (ptr->target)
			{
			case kVectorCanvas_TargetScreenPixels:
				setPortCanvasPort(ptr);
				MoveTo(STATIC_CAST(xl0, short), STATIC_CAST(yl0, short));
				LineTo(STATIC_CAST(xl1, short), STATIC_CAST(yl1, short));
				break;
			
			case kVectorCanvas_TargetQuickDrawPicture:
				MoveTo(gRGMPxoffset + (SInt16) (STATIC_CAST(inStartX, SInt32) * gRGMPwidth / kVectorInterpreter_MaxX),
						gRGMPyoffset + gRGMPheight - (SInt16) (STATIC_CAST(inStartY, SInt32) * gRGMPheight / kVectorInterpreter_MaxY));
				LineTo(gRGMPxoffset + (SInt16) (STATIC_CAST(inEndX, SInt32) * gRGMPwidth/kVectorInterpreter_MaxX),
						gRGMPyoffset + gRGMPheight - (SInt16) (STATIC_CAST(inEndY, SInt32) * gRGMPheight / kVectorInterpreter_MaxY));
				break;
			
			default:
				// not supported
				result = kVectorCanvas_ResultUnsupportedAction;
				break;
			}
		}
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
	
	
	(OSStatus)HIViewSetNeedsDisplay(ptr->canvas, true);
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
	
	
	setPortCanvasPort(ptr);
	ptr->ingin = 1;
	
	return 0;
}// MonitorMouse


/*!
Returns the canvas attached to the given window, or nullptr.

Note that this is a potentially limiting API; a future user
interface may benefit from showing more than one graphic in
the same window.  At that point, it will be necessary to use
properties of HIViews (and not windows) to attach canvases.

(3.1)
*/
VectorCanvas_Ref
VectorCanvas_ReturnFromWindow	(HIWindowRef	inWindow)
{
	VectorCanvas_Ref	result = nullptr;
	OSStatus			error = GetWindowProperty(inWindow, AppResources_ReturnCreatorCode(),
													kConstantsRegistry_WindowPropertyTypeVectorCanvas,
													sizeof(result), nullptr/* actual size */, &result);
	
	
	if (noErr != error) result = nullptr;
	return result;
}// ReturnFromWindow


/*!
Returns the intepreter whose commands affect this canvas.

(3.1)
*/
VectorInterpreter_ID
VectorCanvas_ReturnInterpreterID	(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	VectorInterpreter_ID		result = ptr->interpreter;
	
	
	return result;
}// ReturnInterpreterID


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
Converts the specified TEK window drawing into a picture in
QuickDraw format (PICT), and returns the handle.  Call
KillPicture() when finished with the result.

TEMPORARY.  This will eventually be unnecessary when the
rendering has transitioned completely to Core Graphics and
HIView.

DEPRECATED.  Use VectorCanvas_ReturnNewNSImage() instead.

(2.6)
*/
PicHandle
VectorCanvas_ReturnNewQuickDrawPicture		(VectorInterpreter_ID	inDrawingNumber)
{
	VectorInterpreter_ID	graphicID = 0;
	PicHandle				result = nullptr;
	
	
	graphicID = VectorInterpreter_New(kVectorInterpreter_TargetQuickDrawPicture,
										VectorInterpreter_ReturnMode(inDrawingNumber));
	if (kVectorInterpreter_InvalidID != graphicID)
	{
		VectorCanvas_Ref	sourceCanvas = VectorInterpreter_ReturnCanvas(inDrawingNumber);
		Rect				pictureBounds;
		
		
		SetRect(&pictureBounds, 0, 0, 384, 384); // arbitrary?
		VectorCanvas_SetBounds(&pictureBounds);
		VectorInterpreter_CopyZoom(graphicID, inDrawingNumber);
		
		result = OpenPicture(&pictureBounds);
		ClipRect(&pictureBounds);
		{
			My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), sourceCanvas);
			SInt16						backgroundColorIndex = VectorInterpreter_ReturnBackgroundColor(graphicID);
			RGBColor					backgroundColor;
			
			
			assert((backgroundColorIndex >= 0) && (backgroundColorIndex < kMy_MaxColors));
			getPaletteColor(ptr, backgroundColorIndex, backgroundColor);
			RGBBackColor(&backgroundColor);
			EraseRect(&pictureBounds);
		}
		VectorInterpreter_Redraw(inDrawingNumber, graphicID);
		ClosePicture();
		VectorInterpreter_Dispose(&graphicID);
	}
	return result;
}// ReturnNewQuickDrawPicture


/*!
Returns the window that a canvas belongs to.

(4.0)
*/
NSWindow*
VectorCanvas_ReturnNSWindow		(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	NSWindow*					result = [ptr->windowController window];
	
	
	return result;
}// ReturnNSWindow


/*!
Returns the window that a canvas belongs to.  Note that some
canvases target bitmaps and may never be placed in a window.

(3.1)
*/
HIWindowRef
VectorCanvas_ReturnWindow	(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	HIWindowRef					result = ptr->owningWindow;
	
	
	return result;
}// ReturnWindow


/*!
Issues drawing commands to permanently include the contents of
the scrap path in the main picture, as a filled-in region that
optionally has a border of the specified size.

\retval kVectorCanvas_ResultOK
if there are no errors

\retval kVectorCanvas_ResultInvalidReference
if the specified canvas is unrecognized

\retval kVectorCanvas_ResultParameterError
if the specified color index is out of range

\retval kVectorCanvas_ResultUnsupportedAction
if the current canvas target does not support this operation

(4.0)
*/
VectorCanvas_Result
VectorCanvas_ScrapPathFill		(VectorCanvas_Ref	inRef,
								 SInt16				inFillColor,
								 Float32			inFrameWidthOrZero)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	VectorCanvas_Result			result = kVectorCanvas_ResultInvalidReference;
	
	
	if ((inFillColor < 0) || (inFillColor >= kMy_MaxColors))
	{
		Console_Warning(Console_WriteValue, "vector canvas temporary path fill was given invalid color index", inFillColor);
		result = kVectorCanvas_ResultParameterError;
	}
	else if (nullptr != ptr)
	{
		if (nil != ptr->windowController)
		{
			// Cocoa view
			switch (ptr->target)
			{
			case kVectorCanvas_TargetScreenPixels:
				{
					assert(nil != ptr->drawingPathElements);
					assert(nil != ptr->scrapPath);
					VectorCanvas_Path*	elementData = [[VectorCanvas_Path alloc] init];
					
					
					[elementData->bezierPath appendBezierPath:ptr->scrapPath];
					if (inFrameWidthOrZero > 0.001/* arbitrary */)
					{
						[elementData->bezierPath setLineWidth:(inFrameWidthOrZero * kMy_DefaultStrokeWidth)];
					}
					elementData->drawingMode = kCGPathFill;
					elementData->fillColorIndex = inFillColor;
					[ptr->drawingPathElements addObject:elementData];
					[elementData release];
				}
				result = kVectorCanvas_ResultOK;
				break;
			
			case kVectorCanvas_TargetQuickDrawPicture:
			default:
				result = kVectorCanvas_ResultUnsupportedAction;
				break;
			}
		}
		else
		{
			// legacy Carbon view
			switch (ptr->target)
			{
			case kVectorCanvas_TargetQuickDrawPicture:
			case kVectorCanvas_TargetScreenPixels:
				assert(nullptr != ptr->currentPathRgn);
				PaintRgn(ptr->currentPathRgn);
				result = kVectorCanvas_ResultOK;
				break;
			
			default:
				result = kVectorCanvas_ResultUnsupportedAction;
				break;
			}
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

(4.0)
*/
VectorCanvas_Result
VectorCanvas_ScrapPathReset		(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	VectorCanvas_Result			result = kVectorCanvas_ResultOK;
	
	
	if (nullptr == ptr)
	{
		result = kVectorCanvas_ResultInvalidReference;
	}
	else
	{
		if (nil != ptr->windowController)
		{
			// Cocoa view; in the new implementation drawing is NOT
			// assumed to be taking place at this time, so simply
			// ensure that the scrap path target is empty and do
			// not begin a path in any particular drawing context
			assert(nil != ptr->scrapPath);
			[ptr->scrapPath removeAllPoints];
		}
		else
		{
			// legacy Carbon view; current usage just happens to
			// assume that drawing is already occurring so simply
			// open a region (as before) and let subsequent drawing
			// go into it
			assert(nullptr != ptr->currentPathRgn);
			SetEmptyRgn(ptr->currentPathRgn);
			OpenRgn();
		}
	}
	return result;
}// ScrapPathReset


/*!
Sets the boundaries for the given QuickDraw picture.  The
top-left corner can be set to non-zero values to offset
graphics drawings.

TEMPORARY.  This routine was moved from an older bitmap
rendering module, and currently uses globals.  This will
be changed as the renderer moves to Core Graphics.

(2.6)
*/
SInt16
VectorCanvas_SetBounds		(Rect const*	inBoundsPtr)
{
	SInt16		result = 0;
	
	
	gRGMPheight= inBoundsPtr->bottom - inBoundsPtr->top;
	gRGMPwidth = inBoundsPtr->right - inBoundsPtr->left;
	gRGMPyoffset = inBoundsPtr->top;
	gRGMPxoffset = inBoundsPtr->left;
	
	return result;
}// SetBounds


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

\retval kVectorCanvas_ResultOK
if there are no errors

\retval kVectorCanvas_ResultInvalidReference
if the specified canvas is unrecognized

\retval kVectorCanvas_ResultParameterError
if the specified color index is out of range

\retval kVectorCanvas_ResultUnsupportedAction
if the current canvas target does not support this operation

(3.0)
*/
VectorCanvas_Result
VectorCanvas_SetPenColor	(VectorCanvas_Ref	inRef,
							 SInt16				inColor)
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
		if (nil != ptr->windowController)
		{
			// Cocoa window
			switch (ptr->target)
			{
			case kVectorCanvas_TargetScreenPixels:
				{
					// currently it is assumed that the color is being set in the main drawing
					VectorCanvas_Path*	currentElement = nil;
					
					
					assert(nil != ptr->drawingPathElements);
					if ([ptr->drawingPathElements count] == 0)
					{
						[ptr->drawingPathElements addObject:[[VectorCanvas_Path alloc] init]];
					}
					currentElement = [ptr->drawingPathElements lastObject];
					assert(nil != currentElement);
					if (currentElement->strokeColorIndex != inColor)
					{
						// to change the color, create a new entry (subsequent path
						// changes will use this color until something changes again)
						[ptr->drawingPathElements addObject:[[VectorCanvas_Path alloc] init]];
						currentElement = [ptr->drawingPathElements lastObject];
						assert(nil != currentElement);
					}
					currentElement->strokeColorIndex = inColor;
				}
				break;
			
			case kVectorCanvas_TargetQuickDrawPicture:
			default:
				// not supported
				result = kVectorCanvas_ResultUnsupportedAction;
				break;
			}
		}
		else
		{
			// legacy Carbon and QuickDraw view
			RGBColor	newColor;
			
			
			switch (ptr->target)
			{
			case kVectorCanvas_TargetScreenPixels:
				setPortCanvasPort(ptr);
				getPaletteColor(ptr, inColor, newColor);
				RGBForeColor(&newColor);
				break;
			
			case kVectorCanvas_TargetQuickDrawPicture:
				// legacy Carbon and QuickDraw view
				getPaletteColor(ptr, inColor, newColor);
				RGBForeColor(&newColor);
				break;
			
			default:
				// not supported
				result = kVectorCanvas_ResultUnsupportedAction;
				break;
			}
		}
	}
	
	return result;
}// SetPenColor


/*!
Sets the name of the graphics window.  Returns 0 if successful.

(3.1)
*/
SInt16
VectorCanvas_SetTitle	(VectorCanvas_Ref	inRef,
						 CFStringRef		inTitle)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	NSWindow*					owningWindow = [ptr->windowController window];
	
	
	[owningWindow setTitle:(NSString*)inTitle];
	
	return 0;
}// SetTitle


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

See also the RGBColor version.

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
Retrieves the RGB color for the specified TEK color index.

See also the CGDeviceColor version.

(3.1)
*/
void
getPaletteColor	(My_VectorCanvasPtr		inPtr,
				 SInt16					inZeroBasedIndex,
				 RGBColor&				outColor)
{
	CGDeviceColor	deviceColor;
	
	
	getPaletteColor(inPtr, inZeroBasedIndex, deviceColor);
	outColor = ColorUtilities_QuickDrawColorMake(deviceColor);
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
		RGMlastclick = TickCount();
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
		
		
		SetPortWindowPort(inPtr->owningWindow);
		
		last = inViewLocalMouse;
		current = inViewLocalMouse;
		anchor = inViewLocalMouse;
		
		ColorUtilities_SetGrayPenPattern();
		PenMode(patXor);
		
		SetRect(&rect, 0, 0, 0, 0);
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
			if (RGMlastclick && ((RGMlastclick + GetDblTime()) > TickCount()))
			{
				inPtr->xscale = kMy_MaximumX;
				inPtr->yscale = kMy_MaximumY;
				inPtr->xorigin = 0;
				inPtr->yorigin = 0;
				
				VectorInterpreter_Zoom(inPtr->interpreter, 0, 0, kMy_MaximumX - 1, kMy_MaximumY - 1);
				//VectorInterpreter_PageCommand(inPtr->interpreter);
				RGMlastclick = 0L;
			}
			else
			{
				RGMlastclick = TickCount();
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
			
			RGMlastclick = 0L;
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
Handles "kEventControlDraw" of "kEventClassControl".

Invoked by Mac OS X whenever the TEK canvas should be
redrawn.

(3.1)
*/
OSStatus
receiveCanvasDraw	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					inVectorCanvasPtr)
{
	OSStatus			result = eventNotHandledErr;
	My_VectorCanvasPtr	dataPtr = REINTERPRET_CAST(inVectorCanvasPtr, My_VectorCanvasPtr);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlDraw);
	{
		HIViewRef		view = nullptr;
		
		
		// get the target view
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		
		// if the view was found, continue
		if (noErr == result)
		{
			//ControlPartCode		partCode = 0;
			CGrafPtr			drawingPort = nullptr;
			CGContextRef		drawingContext = nullptr;
			CGrafPtr			oldPort = nullptr;
			GDHandle			oldDevice = nullptr;
			
			
			// find out the current port
			GetGWorld(&oldPort, &oldDevice);
			
			// could determine which part (if any) to draw; if none, draw everything
			// (ignored, not needed)
			//result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, partCode);
			//result = noErr; // ignore part code parameter if absent
			
			// determine the port to draw in; if none, the current port
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, drawingPort);
			if (noErr != result)
			{
				// use current port
				drawingPort = oldPort;
				result = noErr;
			}
			
			// determine the port to draw in; if none, the current port
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamCGContextRef,
															typeCGContextRef, drawingContext);
			assert_noerr(result);
			
			// if all information can be found, proceed with drawing
			if (noErr == result)
			{
				HIRect		floatBounds;
				HIRect		floatClipBounds;
				RgnHandle	optionalTargetRegion = nullptr;
				
				
				SetPort(drawingPort);
				
				// determine boundaries of the content view being drawn
				HIViewGetBounds(view, &floatBounds);
				
				// update internal dimensions to match current view size
				// (NOTE: could be done in a bounds-changed handler)
				{
					Rect	bounds;
					
					
					GetControlBounds(view, &bounds);
					OffsetRect(&bounds, -bounds.left, -bounds.top);
					
					dataPtr->width = bounds.right - bounds.left;
					dataPtr->height = bounds.bottom - bounds.top;
				}
				
				// maybe a focus region has been provided
				if (noErr == CarbonEventUtilities_GetEventParameter(inEvent, kEventParamRgnHandle, typeQDRgnHandle,
																	optionalTargetRegion))
				{
					Rect	clipBounds;
					
					
					GetRegionBounds(optionalTargetRegion, &clipBounds);
					floatClipBounds = CGRectMake(clipBounds.left, clipBounds.top, clipBounds.right - clipBounds.left,
													clipBounds.bottom - clipBounds.top);
				}
				else
				{
					floatClipBounds = floatBounds;
				}
				
				// finally, draw the graphic!
				// INCOMPLETE - these callbacks need to be updated to support Core Graphics
				{
					SInt16			backgroundColorIndex = VectorInterpreter_ReturnBackgroundColor(dataPtr->interpreter);
					CGDeviceColor	backgroundColor;
					
					
					assert((backgroundColorIndex >= 0) && (backgroundColorIndex < kMy_MaxColors));
					getPaletteColor(dataPtr, backgroundColorIndex, backgroundColor);
					CGContextSetRGBFillColor(drawingContext, backgroundColor.red, backgroundColor.green,
												backgroundColor.blue, 1.0/* alpha */);
					CGContextFillRect(drawingContext, floatClipBounds);
				}
				VectorInterpreter_Redraw(dataPtr->interpreter, dataPtr->interpreter);
				
				result = noErr;
			}
			
			// restore port
			SetGWorld(oldPort, oldDevice);
		}
	}
	return result;
}// receiveCanvasDraw


/*!
Handles "kEventWindowClose" of "kEventClassWindow" for TEK
windows.  The default handler destroys windows, which is fine;
but it is important to first clean up some stuff, especially to
ensure the terminal is re-attached to the data stream!

(3.0)
*/
OSStatus
receiveWindowClosing	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inContext)
{
	OSStatus			result = eventNotHandledErr;
	VectorCanvas_Ref	ref = REINTERPRET_CAST(inContext, VectorCanvas_Ref);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowClose);
	{
		HIWindowRef		window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (noErr == result)
		{
			VectorCanvas_Release(&ref);
			result = noErr;
		}
	}
	
	return result;
}// receiveWindowClosing


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


/*!
Sets the current QuickDraw port to that of the given canvas.
Returns 0 if successful.

(3.0)
*/
SInt16
setPortCanvasPort	(My_VectorCanvasPtr		inPtr)
{
	SetPortWindowPort(inPtr->owningWindow);
	return 0;
}// setPortCanvasPort

} // anonymous namespace


@implementation VectorCanvas_Path


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super init];
	if (nil != self)
	{
		self->bezierPath = [[NSBezierPath bezierPath] retain];
		[self->bezierPath setLineWidth:kMy_DefaultStrokeWidth];
		[self->bezierPath setLineCapStyle:NSRoundLineCapStyle];
		[self->bezierPath setLineJoinStyle:NSBevelLineJoinStyle];
		self->drawingMode = kCGPathStroke;
		self->fillColorIndex = 0;
		self->strokeColorIndex = 0;
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[bezierPath release];
	[super dealloc];
}// dealloc


@end // VectorCanvas_Path


@implementation VectorCanvas_View


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithFrame:(NSRect)		aFrame
{
	self = [super initWithFrame:aFrame];
	if (nil != self)
	{
		self->internalPtr = nullptr;
	}
	return self;
}// initWithFrame:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	My_VectorCanvasPtr	asCanvasPtr = [self internalPtrAsVectorCanvasPtr];
	
	
	if (nullptr != asCanvasPtr)
	{
		VectorCanvas_Release(&asCanvasPtr->selfRef);
	}
	[super dealloc];
}// dealloc


/*!
Copies the drawing in the canvas to the primary pasteboard.
(This should be triggered by a standard Copy command from the
Edit menu.)

(4.0)
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
	return [NSNumber numberWithBool:YES];
}


/*!
Displays the standard printing interface for printing the
specified drawing.  (This should be triggered by an appropriate
printing menu command.)

(4.0)
*/
- (IBAction)
performPrintScreen:(id)		sender
{
#pragma unused(sender)
	NSPrintInfo*	printInfo = [[NSPrintInfo sharedPrintInfo] copy];
	
	
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
	return [NSNumber numberWithBool:YES];
}


/*!
Displays the standard printing interface for printing the
currently-selected part of the specified drawing.  (This
should be triggered by an appropriate printing menu command.)

(4.0)
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
	return [NSNumber numberWithBool:YES];
}


/*!
Displays an interface for setting the title of the canvas window.

(4.0)
*/
- (IBAction)
performRename:(id)	sender
{
#pragma unused(sender)
	My_VectorCanvasPtr		canvasPtr = [self internalPtrAsVectorCanvasPtr];
	
	
	if (nullptr == canvasPtr->renameDialog)
	{
		canvasPtr->renameDialog = WindowTitleDialog_NewForVectorCanvas(canvasPtr->selfRef);
	}
	WindowTitleDialog_Display(canvasPtr->renameDialog);
}
- (id)
canPerformRename:(id <NSValidatedUserInterfaceItem>)	anItem
{
#pragma unused(anItem)
	return [NSNumber numberWithBool:YES];
}


#pragma mark NSView


/*!
It is necessary for canvas views to accept “first responder”
in order to ever receive actions such as menu commands!

(4.0)
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

(4.0)
*/
- (void)
drawRect:(NSRect)	rect
{
	[super drawRect:rect];
	
	My_VectorCanvasPtr		canvasPtr = [self internalPtrAsVectorCanvasPtr];
	
	
	if (nullptr != canvasPtr)
	{
		[self renderDrawingInCurrentFocusWithRect:[self bounds]];
	}
}// drawRect:


/*!
Returns YES only if the view’s coordinate system uses
a top-left origin.

(4.0)
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

(4.0)
*/
- (BOOL)
isOpaque
{
	return NO;
}// isOpaque


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

(4.0)
*/
- (void)
renderDrawingInCurrentFocusWithRect:(NSRect)	aRect
{
	My_VectorCanvasPtr	canvasPtr = [self internalPtrAsVectorCanvasPtr];
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
		NSEnumerator*	eachObject = [canvasPtr->drawingPathElements objectEnumerator];
		id				currentObject = nil;
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
		while (nil != (currentObject = [eachObject nextObject]))
		{
			VectorCanvas_Path*	asElement = (VectorCanvas_Path*)currentObject;
			
			
			assert([currentObject isKindOfClass:[VectorCanvas_Path class]]);
			
			// update graphics context state if it should change
			if (asElement->fillColorIndex != currentFillColorIndex)
			{
				assert((asElement->fillColorIndex >= 0) && (asElement->fillColorIndex < kMy_MaxColors));
				if ((nil != printOp) && (kMy_ColorIndexBackground == asElement->fillColorIndex))
				{
					// when printing, do not allow the background color to print
					// (because it might be reformatted, e.g. white-on-black);
					// instead, force the background to be white
					CGContextSetRGBFillColor(drawingContext, 1.0, 1.0, 1.0, 1.0/* alpha */);
				}
				else
				{
					getPaletteColor(canvasPtr, asElement->fillColorIndex, scratchColor);
					CGContextSetRGBFillColor(drawingContext, scratchColor.red, scratchColor.green,
												scratchColor.blue, 1.0/* alpha */);
				}
				currentFillColorIndex = asElement->fillColorIndex;
			}
			if (asElement->strokeColorIndex != currentStrokeColorIndex)
			{
				assert((asElement->strokeColorIndex >= 0) && (asElement->strokeColorIndex < kMy_MaxColors));
				if ((nil != printOp) && (kMy_ColorIndexForeground == asElement->strokeColorIndex))
				{
					// when printing, do not allow the foreground color to print
					// (because it might be reformatted, e.g. white-on-black);
					// instead, force the foreground to be black
					CGContextSetRGBStrokeColor(drawingContext, 0.0, 0.0, 0.0, 1.0/* alpha */);
				}
				else
				{
					getPaletteColor(canvasPtr, asElement->strokeColorIndex, scratchColor);
					CGContextSetRGBStrokeColor(drawingContext, scratchColor.red, scratchColor.green,
												scratchColor.blue, 1.0/* alpha */);
				}
				currentStrokeColorIndex = asElement->strokeColorIndex;
			}
			
			// make lines thicker when the drawing is bigger, up to a certain maximum thickness
			[asElement->bezierPath setLineWidth:std::max(std::min((contentBounds.size.width / canvasPtr->width) * kMy_DefaultStrokeWidth,
																	kMy_DefaultStrokeWidth * 2/* arbitrary maximum */),
															kMy_DefaultStrokeWidth / 3 * 2/* arbitrary minimum */)];
			
			// add the new sub-path
			switch (asElement->drawingMode)
			{
			case kCGPathFill:
				[asElement->bezierPath fill];
				break;
			
			case kCGPathStroke:
			default:
				[asElement->bezierPath stroke];
				break;
			}
		}
	}
}// renderDrawingInCurrentFocusWithRect:


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (void*)
internalPtr
{
	return internalPtr;
}
- (My_VectorCanvasPtr)
internalPtrAsVectorCanvasPtr
{
	return REINTERPRET_CAST(internalPtr, My_VectorCanvasPtr);
}
- (void)
setInternalPtr:(void*)		aCanvas
{
	if (internalPtr != aCanvas)
	{
		if (nullptr != internalPtr)
		{
			My_VectorCanvasPtr	asCanvasPtr = [self internalPtrAsVectorCanvasPtr];
			
			
			VectorCanvas_Release(&asCanvasPtr->selfRef);
		}
		if (nullptr != aCanvas)
		{
			My_VectorCanvasPtr	asCanvasPtr = REINTERPRET_CAST(aCanvas, My_VectorCanvasPtr);
			
			
			VectorCanvas_Retain(asCanvasPtr->selfRef);
		}
		internalPtr = aCanvas;
	}
}// setInternalPtr:


@end // VectorCanvas_View (VectorCanvas_ViewInternal)


@implementation VectorCanvas_WindowController


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithInternalPtr:(void*)		aCanvas
{
	self = [super initWithWindowNibName:@"VectorCanvasCocoa"];
	if (nil != self)
	{
		My_VectorCanvasPtr	asCanvasPtr = REINTERPRET_CAST(aCanvas, My_VectorCanvasPtr);
		
		
		self->internalPtr = asCanvasPtr;
		VectorCanvas_Retain(asCanvasPtr->selfRef);
	}
	return self;
}// initWithInternalPtr:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[canvasView release];
	{
		My_VectorCanvasPtr	asCanvasPtr = [self internalPtrAsVectorCanvasPtr];
		
		
		if (nullptr != asCanvasPtr)
		{
			VectorCanvas_Release(&asCanvasPtr->selfRef);
		}
	}
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (VectorCanvas_View*)
canvasView
{
	return canvasView;
}
- (void)
setCanvasView:(VectorCanvas_View*)	aView
{
	if (canvasView != aView)
	{
		if (nil != canvasView)
		{
			[canvasView release];
		}
		if (nil != aView)
		{
			[aView retain];
		}
		canvasView = aView;
	}
}// setCanvasView:


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.0)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	assert(nil != canvasView);
	
	[canvasView setInternalPtr:self->internalPtr];
	[self setInternalPtr:nullptr]; // no longer needed by the controller itself
	
	if (FlagManager_Test(kFlagOS10_7API) && [[self window] respondsToSelector:@selector(setAnimationBehavior:)])
	{
		[[self window] setAnimationBehavior:FUTURE_SYMBOL(3, NSWindowAnimationBehaviorDocumentWindow)];
	}
}// windowDidLoad


@end // VectorCanvas_WindowController


@implementation VectorCanvas_WindowController (VectorCanvas_WindowControllerInternal)


/*!
Accessor.

(4.0)
*/
- (void*)
internalPtr
{
	return internalPtr;
}
- (My_VectorCanvasPtr)
internalPtrAsVectorCanvasPtr
{
	return REINTERPRET_CAST(internalPtr, My_VectorCanvasPtr);
}
- (void)
setInternalPtr:(void*)		aCanvas
{
	if (internalPtr != aCanvas)
	{
		if (nullptr != internalPtr)
		{
			My_VectorCanvasPtr	asCanvasPtr = [self internalPtrAsVectorCanvasPtr];
			
			
			VectorCanvas_Release(&asCanvasPtr->selfRef);
		}
		if (nullptr != aCanvas)
		{
			My_VectorCanvasPtr	asCanvasPtr = REINTERPRET_CAST(aCanvas, My_VectorCanvasPtr);
			
			
			VectorCanvas_Retain(asCanvasPtr->selfRef);
		}
		internalPtr = aCanvas;
	}
}// setInternalPtr:


@end // VectorCanvas_WindowController (VectorCanvas_WindowControllerInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
