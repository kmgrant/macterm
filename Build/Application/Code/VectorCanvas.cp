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
#include <CarbonEventUtilities.template.h>
#include <ColorUtilities.h>
#include <CommonEventHandlers.h>
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
#include "tekdefs.h"		/* NCSA: sb - all the TEK defines are now here */
#include "VectorCanvas.h"
#include "VectorInterpreter.h"



#pragma mark Types
namespace {

/*!
Internal representation of a VectorCanvas_Ref.
*/
struct My_VectorCanvas
{
	OSType				id;
	SessionRef			vs;
	HIWindowRef			wind;
	EventHandlerUPP		closeUPP;
	EventHandlerRef		closeHandler;
	HIViewRef			zoom;
	HIViewRef			vert;
	HIViewRef			horiz;
	SInt16				xorigin;
	SInt16				yorigin;
	SInt16				xscale;
	SInt16				yscale;
	SInt16				vg;
	SInt16				ingin;
	SInt16				width;
	SInt16				height;
};
typedef My_VectorCanvas*		My_VectorCanvasPtr;
typedef My_VectorCanvas const*	My_VectorCanvasConstPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

SInt16				findCanvasWithVirtualGraphicsID		(SInt16);
SInt16				findCanvasWithWindow				(HIWindowRef);
void				handleNewSize						(HIWindowRef, Float32, Float32, void*);
Boolean				inSplash							(Point, Point);
pascal OSStatus		receiveWindowClosing				(EventHandlerCallRef, EventRef, void*);
SInt16				setPortCanvasPort					(SInt16);

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
My_VectorCanvasPtr									RGMwind[MAXWIND];
std::vector< CommonEventHandlers_WindowResizer >	gWindowResizeHandlers(MAXWIND);

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
	
	
	for (i = 0; i < MAXWIND; i++) RGMwind[i] = nullptr;
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
	
	
	while ((i < MAXWIND) && (nullptr != RGMwind[i]))
	{
		++i;
	}
	if (i < MAXWIND)
	{
		RGMwind[i] = REINTERPRET_CAST(Memory_NewPtr(sizeof(My_VectorCanvas)), My_VectorCanvasPtr);
		if (nullptr != RGMwind[i])
		{
			RGMwind[i]->id = 'RGMW';
			
			// load the NIB containing this dialog (automatically finds the right localization)
			RGMwind[i]->wind = NIBWindow(AppResources_ReturnBundleForNIBs(),
											CFSTR("TEKWindow"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
			
			// install dynamic resize, constrain, zoom, etc. handlers for the window
			gWindowResizeHandlers[i].install
										(RGMwind[i]->wind, handleNewSize, nullptr/* user data */,
											100/* arbitrary minimum width */, 100/* arbitrary minimum height */,
											SHRT_MAX/* arbitrary maximum width */, SHRT_MAX/* maximum height */);
			assert(gWindowResizeHandlers[i].isInstalled());
			
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
			RGMwind[i]->xscale  = WINXMAX;
			RGMwind[i]->yscale  = WINYMAX;
			RGMwind[i]->width   = 400;
			RGMwind[i]->height  = 300;
			RGMwind[i]->ingin   = 0;
			
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
		detachGraphics(inCanvasID);
		RemoveEventHandler(RGMwind[inCanvasID]->closeHandler), RGMwind[inCanvasID]->closeHandler = nullptr;
		DisposeEventHandlerUPP(RGMwind[inCanvasID]->closeUPP), RGMwind[inCanvasID]->closeUPP = nullptr;
		gWindowResizeHandlers[inCanvasID] = CommonEventHandlers_WindowResizer();
		DisposeWindow(RGMwind[inCanvasID]->wind);
		Memory_DisposePtr((Ptr*)&RGMwind[inCanvasID]);
		RGMwind[inCanvasID] = nullptr;
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
		SInt32		xl0 = (STATIC_CAST(inStartX, SInt32) * RGMwind[inCanvasID]->width) / INXMAX;
		SInt32		yl0 = STATIC_CAST(RGMwind[inCanvasID]->height, SInt32) -
							((STATIC_CAST(inStartY, SInt32) * RGMwind[inCanvasID]->height) / INYMAX);
		SInt32		xl1 = (STATIC_CAST(inEndX, SInt32) * RGMwind[inCanvasID]->width) / INXMAX;
		SInt32		yl1 = STATIC_CAST(RGMwind[inCanvasID]->height, SInt32) -
							((STATIC_CAST(inEndY, SInt32) * RGMwind[inCanvasID]->height) / INYMAX);
		
		
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
Changes the current graphics port to that of the given window,
and draws the picture.  Returns 1 if the drawing is complete,
0 if there is more to do, or -1 on error.

TEMPORARY.  This needs to be transitioned to an HIView and
drawn via Carbon Events.

(3.0)
*/
SInt16
VectorCanvas_RenderInWindow		(HIWindowRef	inWindow)
{
	Boolean		result = -1;
	SInt16		i = findCanvasWithWindow(inWindow);
	
	
	if (i >= 0)
	{
		SetPortWindowPort(inWindow);
		
		BeginUpdate(inWindow);
		VGstopred(RGMwind[i]->vg);
		VGpage(RGMwind[i]->vg);
		result = VGpred(RGMwind[i]->vg, RGMwind[i]->vg);
		EndUpdate(inWindow);
		
		if (0 == result)
		{
			// TEMPORARY - notify session of vector graphics update
			// UNIMPLEMENTED
		}
	}
	return result;
}// RenderInWindow


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
								 SInt16		UNUSED_ARGUMENT(inData2),
								 SInt16		UNUSED_ARGUMENT(inData3),
								 SInt16		UNUSED_ARGUMENT(inData4),
								 SInt16		UNUSED_ARGUMENT(inData5))
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
	
	
	if ((inCanvasID >= 0) && (inCanvasID < MAXWIND))
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
Returns the canvas ID whose virtual graphics ID matches
the given ID, or a negative value on error.

(3.1)
*/
SInt16
findCanvasWithVirtualGraphicsID		(SInt16		inVirtualGraphicsRef)
{
	SInt16		result = 0;
	
	
	while (result < MAXWIND)
	{
		if (nullptr != RGMwind[result])
		{
			if (inVirtualGraphicsRef == RGMwind[result]->vg) break;
		}
		++result;
	}
	if (result >= MAXWIND)
	{
		result = -1;
	}
	return result;
}// findCanvasWithVirtualGraphicsID


/*!
Returns the canvas ID whose Mac OS window matches the given
window, or a negative value on error.

(3.0)
*/
SInt16
findCanvasWithWindow	(HIWindowRef	inWindow)
{
	SInt16		result = 0;
	
	
	while (result < MAXWIND)
	{
		if (nullptr != RGMwind[result])
		{
			if (inWindow == RGMwind[result]->wind) break;
		}
		++result;
	}
	if (result >= MAXWIND)
	{
		result = -1;
	}
	return result;
}// findCanvasWithWindow


/*!
This method resizes TEK graphics windows dynamically.

(3.0)
*/
void
handleNewSize	(HIWindowRef	inWindow,
				 Float32		UNUSED_ARGUMENT(inDeltaX),
				 Float32		UNUSED_ARGUMENT(inDeltaY),
				 void*			UNUSED_ARGUMENT(inContext))
{
	SInt16		myRGMnum = findCanvasWithWindow(inWindow);
	
	
	if (myRGMnum > -1)
	{
		CGrafPtr	oldPort = nullptr;
		GDHandle	oldDevice = nullptr;
		Rect		windowPortRect;
		
		
		GetGWorld(&oldPort, &oldDevice);
		SetPortWindowPort(inWindow);
		GetPortBounds(GetWindowPort(inWindow), &windowPortRect);
		RGMwind[myRGMnum]->width = windowPortRect.right - windowPortRect.left;		
		RGMwind[myRGMnum]->height = windowPortRect.bottom - windowPortRect.top;		 
		VGpage(RGMwind[myRGMnum]->vg);
		//RegionUtilities_SetWindowUpToDate(inWindow);
		//RGupdate(inWindow);
		SetGWorld(oldPort, oldDevice);
	}
}// handleNewSize


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


// NOTE: This old routine will be needed to add mouse support to a
// new HIView-based canvas that is planned.
void RGmousedown
  (
	HIWindowRef wind,
	Point *wherein
  )
{
	unsigned long	lx = 0L,
					ly = 0L;
	char			thispaceforent[6];
	short			i = 0;
	Point			where;

	where = *wherein;
	if ((i = findCanvasWithWindow(wind)) < 0)
		return;

	if (!RGMwind[i]->ingin)
	{
	Point	anchor,current,last;
	UInt32	tc = 0;
	short	x0,y0,x1,y1;
	Rect	rect;
	MouseTrackingResult		trackingResult = kMouseTrackingMouseDown;
	
		SetPortWindowPort(wind);
	
		last  = where;
		current = where;
		anchor = where;
		
		ColorUtilities_SetGrayPenPattern();
		PenMode(patXor);
	
		SetRect(&rect,0,0,0,0);
		do
		{
			unless (inSplash(current,anchor))
			{
				tc = TickCount();
				while(TickCount() == tc)
					/* loop */;
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
		while (trackingResult != kMouseTrackingMouseUp);
		
		FrameRect(&rect);
		
		ColorUtilities_SetBlackPenPattern();
		PenMode(patCopy);
		
		if (!inSplash(anchor,current))
		{
			x0 = (short) ((long) rect.left * RGMwind[i]->xscale / RGMwind[i]->width );
			y0 = (short) (RGMwind[i]->yscale -
					(long) rect.top * RGMwind[i]->yscale / RGMwind[i]->height);
			x1 = (short) ((long) rect.right * RGMwind[i]->xscale / RGMwind[i]->width);
			y1 = (short) (RGMwind[i]->yscale -
					(long) rect.bottom * RGMwind[i]->yscale / RGMwind[i]->height);
			x1 = (x1 < x0+2) ? x0 + 4 : x1;
			y0 = (y0 < y1+2) ? y1 + 4 : y0;

			VGzoom( i,
					x0 + RGMwind[i]->xorigin,
					y1 + RGMwind[i]->yorigin,
					x1 + RGMwind[i]->xorigin,
					y0 + RGMwind[i]->yorigin);

			VGpage(RGMwind[i]->vg);

			RGMwind[i]->xscale  = x1 - x0;
			RGMwind[i]->yscale  = y0 - y1;
			RGMwind[i]->xorigin = x0 + RGMwind[i]->xorigin;
			RGMwind[i]->yorigin = y1 + RGMwind[i]->yorigin;
			
			RGMlastclick = 0L;
		}
		else
		{
			if (RGMlastclick && ((RGMlastclick + GetDblTime()) > TickCount()))
			{
				RGMwind[i]->xscale  = WINXMAX;
				RGMwind[i]->yscale  = WINYMAX;
				RGMwind[i]->xorigin = 0;
				RGMwind[i]->yorigin = 0;

				VGzoom(i,0,0,WINXMAX-1,WINYMAX-1);
				VGpage( RGMwind[i]->vg);
				RGMlastclick = 0L;
			}
			else RGMlastclick = TickCount();
		}
		return;
	
	}
	
	/* NCSA: SB */
	/* NCSA: SB - These computations are being truncated and turned into signed ints. */
	/* NCSA: SB - Just make sure everything is cast correctly, and were fine */
	
	lx = ((unsigned long)RGMwind[i]->xscale * (unsigned long)where.h) 	/* NCSA: SB */
			/ (unsigned long)RGMwind[i]->width;						 	/* NCSA: SB */
	ly = (unsigned long)RGMwind[i]->yscale - 							/* NCSA: SB */
		((unsigned long)RGMwind[i]->yscale * (unsigned long)where.v) / (unsigned long)RGMwind[i]->height; /* NCSA: SB */

	VGgindata(RGMwind[i]->vg,(unsigned short) lx,(unsigned short)ly,' ',thispaceforent);	/* NCSA: SB */
	
	Session_SendData(RGMwind[i]->vs,thispaceforent,5);
	Session_SendData(RGMwind[i]->vs, " \r\n", 3);

    /*	RGMwind[i]->ingin = 0; */
	RGMlastclick = TickCount();
}// RGmousedown


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
	
	
	if ((inCanvasID >= 0) && (inCanvasID < MAXWIND))
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
