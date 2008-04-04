/*###############################################################

	VectorCanvas.cp
	
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
/*
by Gaige B. Paulsen
spawned from rgp.c by Aaron Contorer for NCSA
Copyright 1987, board of trustees, University of Illinois

Routines for Macintosh Window output.  
*/

#include "UniversalDefines.h"

// standard-C includes
#include <cstdio>
#include <cstring>

// standard-C++ includes
#include <vector>

// library includes
#include <CarbonEventHandlerWrap.template.h>
#include <CarbonEventUtilities.template.h>
#include <ColorUtilities.h>
#include <CommonEventHandlers.h>
#include <HIViewWrap.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// Mac includes
#include <Carbon/Carbon.h>

// MacTelnet includes
#include "AppResources.h"
#include "Console.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "Session.h"
#include "VectorCanvas.h"
#include "VectorInterpreter.h"



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

} // anonymous namespace

#pragma mark Types
namespace {

/*!
Internal representation of a VectorCanvas_Ref.
*/
struct My_VectorCanvas
{
	OSType					id;
	VectorCanvas_Ref		selfRef;
	VectorCanvas_Target		target;
	SessionRef				vs;
	HIWindowRef				wind;
	EventHandlerUPP			closeUPP;
	EventHandlerRef			closeHandler;
	HIViewRef				canvas;
	CarbonEventHandlerWrap	canvasDrawHandler;
	SInt16					xorigin;
	SInt16					yorigin;
	SInt16					xscale;
	SInt16					yscale;
	SInt16					vg;
	SInt16					ingin;
	SInt16					width;
	SInt16					height;
};
typedef My_VectorCanvas*		My_VectorCanvasPtr;
typedef My_VectorCanvas const*	My_VectorCanvasConstPtr;

typedef MemoryBlockPtrLocker< VectorCanvas_Ref, My_VectorCanvas >	My_VectorCanvasPtrLocker;
typedef LockAcquireRelease< VectorCanvas_Ref, My_VectorCanvas >		My_VectorCanvasAutoLocker;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void				handleMouseDown				(My_VectorCanvasPtr, Point);
Boolean				inSplash					(Point, Point);
pascal OSStatus		receiveCanvasDraw			(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveWindowClosing		(EventHandlerCallRef, EventRef, void*);
SInt16				setPortCanvasPort			(My_VectorCanvasPtr);

} // anonymous namespace

#pragma mark Variables
namespace {

My_VectorCanvasPtrLocker&	gVectorCanvasPtrLocks ()	{ static My_VectorCanvasPtrLocker x; return x; }

long						RGMlastclick = 0L;
short						RGMcolor[] =
							{
								30,			// black
								33,			// white
								205,		// red
								341,		// green
								409,		// blue
								273,		// cyan
								137,		// magenta
								69			// yellow
							};

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

(3.0)
*/
VectorCanvas_Ref
VectorCanvas_New	(VectorInterpreter_ID	inID,
					 VectorCanvas_Target	inTarget)
{
	My_VectorCanvasPtr		ptr = new My_VectorCanvas;
	VectorCanvas_Ref		result = REINTERPRET_CAST(ptr, VectorCanvas_Ref);
	
	
	ptr->id = 'RGMW';
	ptr->selfRef = result;
	ptr->target = inTarget;
	
	// load the NIB containing this dialog (automatically finds the right localization)
	ptr->wind = NIBWindow(AppResources_ReturnBundleForNIBs(),
							CFSTR("TEKWindow"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
	
	{
		HIViewWrap		canvasView(idMyCanvas, ptr->wind);
		
		
		ptr->canvas = canvasView;
		ptr->canvasDrawHandler.install(GetControlEventTarget(canvasView), receiveCanvasDraw,
										CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlDraw),
										result/* context */);
		assert(ptr->canvasDrawHandler.isInstalled());
	}
	
	// install a close handler so TEK windows are detached properly
	{
		EventTypeSpec const		whenWindowClosing[] =
								{
									{ kEventClassWindow, kEventWindowClose }
								};
		OSStatus				error = noErr;
		
		
		ptr->closeUPP = NewEventHandlerUPP(receiveWindowClosing);
		error = InstallWindowEventHandler(ptr->wind, ptr->closeUPP,
											GetEventTypeCount(whenWindowClosing), whenWindowClosing,
											ptr->selfRef/* user data */,
											&ptr->closeHandler/* event handler reference */);
		assert_noerr(error);
	}
	
	ptr->vg = inID;
	ptr->vs = nullptr;
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
	
	SetWindowKind(ptr->wind, WIN_TEK);
	{
		HIWindowRef		frontWindow = GetFrontWindowOfClass(kDocumentWindowClass, true/* visible */);
		
		
		SendBehind(ptr->wind, frontWindow);
		ShowWindow(ptr->wind);
	}
	
	return result;
}// New


/*!
Disposes of a vector graphics canvas.  The specified ID is
invalid after this routine returns.  Returns 0 if successful.

(3.0)
*/
void
VectorCanvas_Dispose	(VectorCanvas_Ref*		inoutRefPtr)
{
	// clean up structure
	{
		My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), *inoutRefPtr);
		
		
		setPortCanvasPort(ptr);
		if (nullptr != ptr->vs)
		{
			Session_TEKDetachTargetGraphic(ptr->vs);
		}
		RemoveEventHandler(ptr->closeHandler), ptr->closeHandler = nullptr;
		DisposeEventHandlerUPP(ptr->closeUPP), ptr->closeUPP = nullptr;
		DisposeWindow(ptr->wind);
		
		if (kVectorCanvas_TargetQuickDrawPicture == ptr->target)
		{
			gRGMPbusy = false;
		}
	}
	delete *(REINTERPRET_CAST(inoutRefPtr, My_VectorCanvasPtr*)), *inoutRefPtr = nullptr;
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
Erases the entire background of the vector graphics canvas.
Returns 0 if successful.
  
(3.0)
*/
SInt16
VectorCanvas_ClearScreen	(VectorCanvas_Ref	inRef)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	SInt16						result = 0;
	
	
	if (setPortCanvasPort(ptr)) result = -1;
	else
	{
		Rect	bounds;
		
		
		GetPortBounds(GetWindowPort(ptr->wind), &bounds);
		PaintRect(&bounds);
	}
	return result;
}// ClearScreen


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_CursorHide ()
{
}// CursorHide


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_CursorLock ()
{
}// CursorLock


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_CursorShow ()
{
}// CursorShow


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_DataLine	(VectorCanvas_Ref	UNUSED_ARGUMENT(inRef),
						 SInt16				UNUSED_ARGUMENT(inData),
						 SInt16				UNUSED_ARGUMENT(inCount))
{
}// DataLine


/*!
Renders a single point in canvas coordinates.  Returns 0 if
successful.

(3.1)
*/
SInt16
VectorCanvas_DrawDot	(VectorCanvas_Ref	inRef,
						 SInt16				inX,
						 SInt16				inY)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	
	
	if (kVectorCanvas_TargetScreenPixels == ptr->target)
	{
		setPortCanvasPort(ptr);
	}
	MoveTo(inX, inY);
	LineTo(inX, inY);
	
	return 0;
}// DrawDot


/*!
Renders a straight line between two points expressed in
canvas coordinates.  Returns 0 if successful.

(3.1)
*/
SInt16
VectorCanvas_DrawLine	(VectorCanvas_Ref	inRef,
						 SInt16				inStartX,
						 SInt16				inStartY,
						 SInt16				inEndX,
						 SInt16				inEndY)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	
	
	if (kVectorCanvas_TargetScreenPixels == ptr->target)
	{
		SInt32		xl0 = (STATIC_CAST(inStartX, SInt32) * ptr->width) / kVectorInterpreter_MaxX;
		SInt32		yl0 = STATIC_CAST(ptr->height, SInt32) -
							((STATIC_CAST(inStartY, SInt32) * ptr->height) / kVectorInterpreter_MaxY);
		SInt32		xl1 = (STATIC_CAST(inEndX, SInt32) * ptr->width) / kVectorInterpreter_MaxX;
		SInt32		yl1 = STATIC_CAST(ptr->height, SInt32) -
							((STATIC_CAST(inEndY, SInt32) * ptr->height) / kVectorInterpreter_MaxY);
		
		
		setPortCanvasPort(ptr);
		MoveTo(STATIC_CAST(xl0, short), STATIC_CAST(yl0, short));
		LineTo(STATIC_CAST(xl1, short), STATIC_CAST(yl1, short));
	}
	else
	{
		MoveTo(gRGMPxoffset + (SInt16) (STATIC_CAST(inStartX, SInt32) * gRGMPwidth / kVectorInterpreter_MaxX),
				gRGMPyoffset + gRGMPheight - (SInt16) (STATIC_CAST(inStartY, SInt32) * gRGMPheight / kVectorInterpreter_MaxY));
		LineTo(gRGMPxoffset + (SInt16) (STATIC_CAST(inEndX, SInt32) * gRGMPwidth/kVectorInterpreter_MaxX),
				gRGMPyoffset + gRGMPheight - (SInt16) (STATIC_CAST(inEndY, SInt32) * gRGMPheight / kVectorInterpreter_MaxY));
	}
	
	return 0;
}// DrawLine


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_FinishPage		(VectorCanvas_Ref	UNUSED_ARGUMENT(inRef))
{
}// FinishPage


/*!
Sets what is known as "GIN" mode; the mouse cursor changes
to a cross, keypresses will send the mouse location, and
mouse button clicks (with or without a ÒshiftÓ) will send
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
Specifies text rendering options.

UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_SetCharacterMode	(VectorCanvas_Ref	UNUSED_ARGUMENT(inRef),
								 SInt16				UNUSED_ARGUMENT(inRotation),
								 SInt16				UNUSED_ARGUMENT(inSize))
{
}// SetCharacterMode


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_SetGraphicsMode ()
{
}// SetGraphicsMode


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
	
	
	ptr->vs = inSession;
	return 0;
}// SetListeningSession


/*!
Chooses a color for drawing dots and lines, from among
a set palette.  Returns 0 if successful.

(3.0)
*/
SInt16
VectorCanvas_SetPenColor	(VectorCanvas_Ref	inRef,
							 SInt16				inColor)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	
	
	if (kVectorCanvas_TargetScreenPixels == ptr->target)
	{
		setPortCanvasPort(ptr);
	}
	ForeColor(STATIC_CAST(RGMcolor[inColor], SInt32));
	
	return 0;
}// SetPenColor


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_SetTextMode ()
{
}// SetTextMode


/*!
Sets the name of the graphics window.  Returns 0 if successful.

(3.1)
*/
SInt16
VectorCanvas_SetTitle	(VectorCanvas_Ref	inRef,
						 CFStringRef		inTitle)
{
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), inRef);
	
	
	(OSStatus)SetWindowTitleWithCFString(ptr->wind, inTitle);
	
	return 0;
}// SetTitle


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_Uncover	(VectorCanvas_Ref	UNUSED_ARGUMENT(inRef))
{
}// Uncover


#pragma mark Internal Methods
namespace {

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
			if (0 == VectorInterpreter_FillInPositionReport(inPtr->vg, STATIC_CAST(lx, UInt16), STATIC_CAST(ly, UInt16),
															' ', cursorReport))
			{
				Session_SendData(inPtr->vs, cursorReport, 5);
				Session_SendData(inPtr->vs, " \r\n", 3);
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
		
		
		SetPortWindowPort(inPtr->wind);
		
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
				
				VectorInterpreter_Zoom(inPtr->vg, 0, 0, kMy_MaximumX - 1, kMy_MaximumY - 1);
				VectorInterpreter_PageCommand(inPtr->vg);
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
			
			VectorInterpreter_Zoom(inPtr->vg, x0 + inPtr->xorigin, y1 + inPtr->yorigin,
									x1 + inPtr->xorigin, y0 + inPtr->yorigin);
			VectorInterpreter_PageCommand(inPtr->vg);
			
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
pascal OSStatus
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
				Rect		bounds;
				HIRect		floatBounds;
				RgnHandle	optionalTargetRegion = nullptr;
				
				
				SetPort(drawingPort);
				
				// determine boundaries of the content view being drawn
				HIViewGetBounds(view, &floatBounds);
				GetControlBounds(view, &bounds);
				OffsetRect(&bounds, -bounds.left, -bounds.top);
				
				// update internal dimensions to match current view size
				// (NOTE: could be done in a bounds-changed handler)
				dataPtr->width = bounds.right - bounds.left;
				dataPtr->height = bounds.bottom - bounds.top;
				
				// maybe a focus region has been provided
				if (noErr == CarbonEventUtilities_GetEventParameter(inEvent, kEventParamRgnHandle, typeQDRgnHandle,
																	optionalTargetRegion))
				{
					Rect	clipBounds;
					HIRect	floatClipBounds;
					
					
					SetClip(optionalTargetRegion);
					GetRegionBounds(optionalTargetRegion, &clipBounds);
					floatClipBounds = CGRectMake(clipBounds.left, clipBounds.top, clipBounds.right - clipBounds.left,
													clipBounds.bottom - clipBounds.top);
					CGContextClipToRect(drawingContext, floatClipBounds);
				}
				else
				{
					static RgnHandle	clipRegion = Memory_NewRegion();
					
					
					SetRectRgn(clipRegion, 0, 0, STATIC_CAST(floatBounds.size.width, SInt16),
								STATIC_CAST(floatBounds.size.height, SInt16));
					SetClip(clipRegion);
					CGContextClipToRect(drawingContext, floatBounds);
				}
				
				// finally, draw the graphic!
				// INCOMPLETE - these callbacks need to be updated to support Core Graphics
				VectorInterpreter_StopRedraw(dataPtr->vg);
				VectorInterpreter_PageCommand(dataPtr->vg);
				{
					SInt16		zeroIfMoreRedrawsNeeded = 0;
					SInt16		loopGuard = 0;
					
					
					while ((0 == zeroIfMoreRedrawsNeeded) && (loopGuard++ < 10000/* arbitrary */))
					{
						zeroIfMoreRedrawsNeeded = VectorInterpreter_PiecewiseRedraw(dataPtr->vg, dataPtr->vg);
					}
				}
				
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
pascal OSStatus
receiveWindowClosing	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inContext)
{
	OSStatus					result = eventNotHandledErr;
	VectorCanvas_Ref			ref = REINTERPRET_CAST(inContext, VectorCanvas_Ref);
	My_VectorCanvasAutoLocker	ptr(gVectorCanvasPtrLocks(), ref);
	UInt32 const				kEventClass = GetEventClass(inEvent);
	UInt32 const				kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowClose);
	{
		HIWindowRef		window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (noErr == result)
		{
			VectorCanvas_Dispose(&ref);
			result = noErr;
		}
	}
	
	return result;
}// receiveWindowClosing


/*!
Sets the current QuickDraw port to that of the given canvas.
Returns 0 if successful.

(3.0)
*/
SInt16
setPortCanvasPort	(My_VectorCanvasPtr		inPtr)
{
	SetPortWindowPort(inPtr->wind);
	return 0;
}// setPortCanvasPort

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
