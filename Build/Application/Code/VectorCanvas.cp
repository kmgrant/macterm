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

UInt16 const	kMy_MaximumGraphics = 20;	//!< limit is for historical reasons; TEMPORARY

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

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

SInt16				findCanvasWithWindow		(HIWindowRef);
void				handleMouseDown				(HIWindowRef, Point);
Boolean				inSplash					(Point, Point);
pascal OSStatus		receiveCanvasDraw			(EventHandlerCallRef, EventRef, void*);
pascal OSStatus		receiveWindowClosing		(EventHandlerCallRef, EventRef, void*);
SInt16				setPortCanvasPort			(SInt16);

} // anonymous namespace

#pragma mark Variables
namespace {

long												RGMlastclick = 0L;
short												RGMcolor[] =
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
My_VectorCanvasPtr									RGMwind[kMy_MaximumGraphics];

} // anonymous namespace


#pragma mark Public Methods

/*!
Call this routine before anything else in this module.

(3.0)
*/
void
VectorCanvas_Init ()
{
	register SInt16		i = 0;
	
	
	for (i = 0; i < kMy_MaximumGraphics; ++i) RGMwind[i] = nullptr;
}// Init


/*!
Creates a new vector graphics canvas that targets a Mac window.
Returns a nonnegative number (the canvas ID) if successful.

(3.0)
*/
SInt16
VectorCanvas_New ()
{
	SInt16		result = -1;
	SInt16		i = 0;
	
	
	while ((i < kMy_MaximumGraphics) && (nullptr != RGMwind[i]))
	{
		++i;
	}
	if (i < kMy_MaximumGraphics)
	{
		RGMwind[i] = new My_VectorCanvas;
		if (nullptr != RGMwind[i])
		{
			RGMwind[i]->id = 'RGMW';
			
			// load the NIB containing this dialog (automatically finds the right localization)
			RGMwind[i]->wind = NIBWindow(AppResources_ReturnBundleForNIBs(),
											CFSTR("TEKWindow"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
			
			{
				HIViewWrap		canvasView(idMyCanvas, RGMwind[i]->wind);
				
				
				RGMwind[i]->canvas = canvasView;
				RGMwind[i]->canvasDrawHandler.install(GetControlEventTarget(canvasView), receiveCanvasDraw,
														CarbonEventSetInClass(CarbonEventClass(kEventClassControl), kEventControlDraw),
														RGMwind[i]/* context */);
				assert(RGMwind[i]->canvasDrawHandler.isInstalled());
			}
			
			// install a close handler so TEK windows are detached properly
			{
				EventTypeSpec const		whenWindowClosing[] =
										{
											{ kEventClassWindow, kEventWindowClose }
										};
				OSStatus				error = noErr;
				
				
				RGMwind[i]->closeUPP = NewEventHandlerUPP(receiveWindowClosing);
				error = InstallWindowEventHandler(RGMwind[i]->wind, RGMwind[i]->closeUPP,
													GetEventTypeCount(whenWindowClosing), whenWindowClosing,
													nullptr/* user data */,
													&RGMwind[i]->closeHandler/* event handler reference */);
				assert_noerr(error);
			}
			
			SetWindowKind(RGMwind[i]->wind, WIN_TEK);
			ShowWindow(RGMwind[i]->wind);
			EventLoop_SelectBehindDialogWindows(RGMwind[i]->wind);
			
			RGMwind[i]->vg = -1;
			RGMwind[i]->vs = nullptr;
			RGMwind[i]->xorigin = 0;
			RGMwind[i]->yorigin = 0;
			RGMwind[i]->xscale = kMy_MaximumX;
			RGMwind[i]->yscale = kMy_MaximumY;
			RGMwind[i]->width = 400;
			RGMwind[i]->height = 300;
			RGMwind[i]->ingin = 0;
			
			RegionUtilities_SetWindowUpToDate(RGMwind[i]->wind);
			
			result = i;
		}
	}
	
	return result;
}// New


/*!
Disposes of a vector graphics canvas.  The specified ID is
invalid after this routine returns.  Returns 0 if successful.

(3.0)
*/
SInt16
VectorCanvas_Dispose	(SInt16		inCanvasID)
{
	SInt16		result = 0;
	
	
	if (setPortCanvasPort(inCanvasID)) result = -1;
	else
	{
		if (nullptr != RGMwind[inCanvasID]->vs)
		{
			Session_TEKDetachTargetGraphic(RGMwind[inCanvasID]->vs);
		}
		RemoveEventHandler(RGMwind[inCanvasID]->closeHandler), RGMwind[inCanvasID]->closeHandler = nullptr;
		DisposeEventHandlerUPP(RGMwind[inCanvasID]->closeUPP), RGMwind[inCanvasID]->closeUPP = nullptr;
		DisposeWindow(RGMwind[inCanvasID]->wind);
		delete RGMwind[inCanvasID], RGMwind[inCanvasID] = nullptr;
	}
	return result;
}// Dispose


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_AudioEvent		(SInt16		UNUSED_ARGUMENT(inCanvasID))
{
}// AudioEvent


/*!
Erases the entire background of the vector graphics canvas.
Returns 0 if successful.
  
(3.0)
*/
SInt16
VectorCanvas_ClearScreen	(SInt16		inCanvasID)
{
	SInt16		result = 0;
	
	
	if (setPortCanvasPort(inCanvasID)) result = -1;
	else
	{
		Rect	bounds;
		
		
		GetPortBounds(GetWindowPort(RGMwind[inCanvasID]->wind), &bounds);
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
VectorCanvas_DataLine	(SInt16		UNUSED_ARGUMENT(inCanvasID),
						 SInt16		UNUSED_ARGUMENT(inData),
						 SInt16		UNUSED_ARGUMENT(inCount))
{
}// DataLine


/*!
Renders a single point in canvas coordinates.  Returns 0 if
successful.

(3.0)
*/
SInt16
VectorCanvas_DrawDot	(SInt16		inCanvasID,
						 SInt16		inX,
						 SInt16		inY)
{
	SInt16		result = 0;
	
	
	if (setPortCanvasPort(inCanvasID)) result = -1;
	else
	{
		MoveTo(inX, inY);
		LineTo(inX, inY);
	}
	return result;
}// DrawDot


/*!
Renders a straight line between two points expressed in
canvas coordinates.  Returns 0 if successful.

(3.0)
*/
SInt16
VectorCanvas_DrawLine	(SInt16		inCanvasID,
						 SInt16		inStartX,
						 SInt16		inStartY,
						 SInt16		inEndX,
						 SInt16		inEndY)
{
	SInt16		result = 0;
	
	
	if (setPortCanvasPort(inCanvasID)) result = -1;
	else
	{
		SInt32		xl0 = (STATIC_CAST(inStartX, SInt32) * RGMwind[inCanvasID]->width) / kVectorInterpreter_MaxX;
		SInt32		yl0 = STATIC_CAST(RGMwind[inCanvasID]->height, SInt32) -
							((STATIC_CAST(inStartY, SInt32) * RGMwind[inCanvasID]->height) / kVectorInterpreter_MaxY);
		SInt32		xl1 = (STATIC_CAST(inEndX, SInt32) * RGMwind[inCanvasID]->width) / kVectorInterpreter_MaxX;
		SInt32		yl1 = STATIC_CAST(RGMwind[inCanvasID]->height, SInt32) -
							((STATIC_CAST(inEndY, SInt32) * RGMwind[inCanvasID]->height) / kVectorInterpreter_MaxY);
		
		
		MoveTo(STATIC_CAST(xl0, short), STATIC_CAST(yl0, short));
		LineTo(STATIC_CAST(xl1, short), STATIC_CAST(yl1, short));
	}
	return result;
}// DrawLine


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_FinishPage		(SInt16		UNUSED_ARGUMENT(inCanvasID))
{
}// FinishPage


/*!
Determines if a window represents a vector graphic, and if
so, which one.  Returns true and a nonnegative device ID
only if successful.

(3.0)
*/
Boolean
VectorCanvas_GetFromWindow	(HIWindowRef	inWindow,
							 SInt16*		outDeviceIDPtr)
{
	Boolean		result = false;
	
	
	*outDeviceIDPtr = findCanvasWithWindow(inWindow);
	result = (*outDeviceIDPtr > -1);
	
	return result;
}// GetFromWindow


/*!
Sets what is known as "GIN" mode; the mouse cursor changes
to a cross, keypresses will send the mouse location, and
mouse button clicks (with or without a ÒshiftÓ) will send
characters indicating the event type.  Returns 0 if successful.

(3.0)
*/
SInt16
VectorCanvas_MonitorMouse	(SInt16		inCanvasID)
{
	SInt16		result = 0;
	
	
	if (setPortCanvasPort(inCanvasID)) result = -1;
	else
	{
		RGMwind[inCanvasID]->ingin = 1;
	}
	return result;
}// MonitorMouse


/*!
Returns a name for this type of canvas.

(3.0)
*/
char const*
VectorCanvas_ReturnDeviceName ()
{
	return "Macintosh Windows";
}// ReturnDeviceName


/*!
Applies miscellaneous settings to a canvas.

(3.0)
*/
void
VectorCanvas_SetCallbackData	(SInt16		inCanvasID,
								 SInt16		inVectorInterpreterRef,
								 SInt16		UNUSED_ARGUMENT(inData2))
{
	RGMwind[inCanvasID]->vg = inVectorInterpreterRef;
}// SetCallbackData


/*!
Specifies text rendering options.

UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_SetCharacterMode	(SInt16		UNUSED_ARGUMENT(inCanvasID),
								 SInt16		UNUSED_ARGUMENT(inRotation),
								 SInt16		UNUSED_ARGUMENT(inSize))
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
VectorCanvas_SetListeningSession	(SInt16			inCanvasID,
									 SessionRef		inSession)
{
	Boolean		result = -1;
	
	
	if ((inCanvasID >= 0) && (inCanvasID < kMy_MaximumGraphics))
	{
		RGMwind[inCanvasID]->vs = inSession;
	}
	return result;
}// SetListeningSession


/*!
Chooses a color for drawing dots and lines, from among
a set palette.  Returns 0 if successful.

(3.0)
*/
SInt16
VectorCanvas_SetPenColor	(SInt16		inCanvasID,
							 SInt16		inColor)
{
	SInt16		result = 0;
	
	
	if (setPortCanvasPort(inCanvasID)) result = -1;
	else
	{
		ForeColor(STATIC_CAST(RGMcolor[inColor], long));
	}
	return result;
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
VectorCanvas_SetTitle	(SInt16			inCanvasID,
						 CFStringRef	inTitle)
{
	Boolean		result = -1;
	
	
	if (inCanvasID >= 0)
	{
		if (noErr == SetWindowTitleWithCFString(RGMwind[inCanvasID]->wind, inTitle))
		{
			result = 0; // success!
		}
	}
	return result;
}// SetTitle


/*!
UNIMPLEMENTED.

(3.0)
*/
void
VectorCanvas_Uncover	(SInt16		UNUSED_ARGUMENT(inCanvasID))
{
}// Uncover


#pragma mark Internal Methods
namespace {

/*!
Returns the canvas ID whose Mac OS window matches the given
window, or a negative value on error.

(3.0)
*/
SInt16
findCanvasWithWindow	(HIWindowRef	inWindow)
{
	SInt16		result = 0;
	
	
	while (result < kMy_MaximumGraphics)
	{
		if (nullptr != RGMwind[result])
		{
			if (inWindow == RGMwind[result]->wind) break;
		}
		++result;
	}
	if (result >= kMy_MaximumGraphics)
	{
		result = -1;
	}
	return result;
}// findCanvasWithWindow


/*!
Responds to a click/drag in a TEK window.  The current QuickDraw
port will be drawn into.

NOTE: This old routine will be needed to add mouse support to a
new HIView-based canvas that is planned.

(2.6)
*/
void
handleMouseDown		(HIWindowRef	inWindow,
					 Point			inViewLocalMouse)
{
	My_VectorCanvasPtr		ptr = RGMwind[findCanvasWithWindow(inWindow)];
	
	
	if (ptr->ingin)
	{
		// report the location of the cursor
		{
			SInt32		lx = 0L;
			SInt32		ly = 0L;
			char		cursorReport[6];
			
			
			lx = ((SInt32)ptr->xscale * (SInt32)inViewLocalMouse.h) / (SInt32)ptr->width;
			ly = (SInt32)ptr->yscale -
					((SInt32)ptr->yscale * (SInt32)inViewLocalMouse.v) / (SInt32)ptr->height;
			
			// the report is exactly 5 characters long
			if (0 == VectorInterpreter_FillInPositionReport(ptr->vg, STATIC_CAST(lx, UInt16), STATIC_CAST(ly, UInt16),
															' ', cursorReport))
			{
				Session_SendData(ptr->vs, cursorReport, 5);
				Session_SendData(ptr->vs, " \r\n", 3);
			}
		}
		
		//ptr->ingin = 0;
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
		
		
		SetPortWindowPort(inWindow);
		
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
				ptr->xscale = kMy_MaximumX;
				ptr->yscale = kMy_MaximumY;
				ptr->xorigin = 0;
				ptr->yorigin = 0;
				
				VectorInterpreter_Zoom(ptr->vg, 0, 0, kMy_MaximumX - 1, kMy_MaximumY - 1);
				VectorInterpreter_PageCommand(ptr->vg);
				RGMlastclick = 0L;
			}
			else
			{
				RGMlastclick = TickCount();
			}
		}
		else
		{
			x0 = (short)((long)rect.left * ptr->xscale / ptr->width);
			y0 = (short)(ptr->yscale - (long)rect.top * ptr->yscale / ptr->height);
			x1 = (short)((long)rect.right * ptr->xscale / ptr->width);
			y1 = (short)(ptr->yscale - (long)rect.bottom * ptr->yscale / ptr->height);
			x1 = (x1 < (x0 + 2)) ? x0 + 4 : x1;
			y0 = (y0 < (y1 + 2)) ? y1 + 4 : y0;
			
			VectorInterpreter_Zoom(ptr->vg, x0 + ptr->xorigin, y1 + ptr->yorigin,
									x1 + ptr->xorigin, y0 + ptr->yorigin);
			VectorInterpreter_PageCommand(ptr->vg);
			
			ptr->xscale = x1 - x0;
			ptr->yscale = y0 - y1;
			ptr->xorigin = x0 + ptr->xorigin;
			ptr->yorigin = y1 + ptr->yorigin;
			
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
						 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowClose);
	{
		HIWindowRef		window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			SInt16		myRGMnum = findCanvasWithWindow(window);
			
			
			if (myRGMnum > -1)
			{
				SInt16		nonZeroOnError = VectorCanvas_Dispose(myRGMnum);
				
				
				if (nonZeroOnError != 0) Sound_StandardAlert();
				result = noErr;
			}
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
setPortCanvasPort	(SInt16		inCanvasID)
{
	SInt16		result = -1;
	
	
	if ((inCanvasID >= 0) && (inCanvasID < kMy_MaximumGraphics))
	{
		if (nullptr != RGMwind[inCanvasID])
		{
			SetPortWindowPort(RGMwind[inCanvasID]->wind);
			result = 0;
		}
	}
	return result;
}// setPortCanvasPort

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
