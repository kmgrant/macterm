/*###############################################################

	TektronixRealGraphics.cp
	
	MacTelnet
		© 1998-2006 by Kevin Grant.
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
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "Session.h"
#include "tekdefs.h"		/* NCSA: sb - all the TEK defines are now here */
#include "TektronixRealGraphics.h"
#include "TektronixVirtualGraphics.h"



//
// internal methods
//

static void					handleNewSize			(WindowRef				inWindow,
													 Float32				inDeltaX,
													 Float32				inDeltaY,
													 void*					inContext);

static pascal OSStatus		receiveWindowClosing	(EventHandlerCallRef	inHandlerCallRef,
													 EventRef				inEvent,
													 void*					inContext);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
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
	struct RGMwindows*									RGMwind[MAXWIND];
	EventHandlerUPP										gTEKWindowClosingUPPs[MAXWIND];
	EventHandlerRef										gTEKWindowClosingHandlers[MAXWIND];
	std::vector< CommonEventHandlers_WindowResizer >	gWindowResizeHandlers(MAXWIND);
}


//
// public methods
//

/*!
To determine if the specified window is a
Tektronix Real Graphics window, use this
method.  If true is returned, the output
"outRG" will be the number of the graphics
window.  If false is returned, the output
is negative in "outRG".

(3.0)
*/
Boolean
TektronixRealGraphics_IsRealGraphicsWindow	(WindowRef		inWindow,
											 SInt16*		outRG)
{
	Boolean		result = false;
	
	
	*outRG = RGfindbywind(inWindow);
	result = (*outRG > -1);
	
	return result;
}// IsRealGraphicsWindow


char const*
RGMdevname ()
{
	return("Macintosh Windows");
}// RGMwindows


short
RGMnewwin ()
{
	short	i = 0;

	while ((i < MAXWIND) && (RGMwind[i] != nullptr)) i++;

	if (i >= MAXWIND) return(-1);

	RGMwind[i] = (struct RGMwindows *) Memory_NewPtr(sizeof(struct RGMwindows));
	if (RGMwind[i] == nullptr) {
		return -1;
		}

	RGMwind[i]->id = 'RGMW';
	
	// load the NIB containing this dialog (automatically finds the right localization)
	RGMwind[i]->wind = NIBWindow(AppResources_ReturnBundleForNIBs(),
									CFSTR("TEKWindow"), CFSTR("Window")) << NIBLoader_AssertWindowExists;
	
	if (RGMwind[i]->wind == 0L) {
		Memory_DisposePtr((Ptr*)&RGMwind[i]);
		RGMwind[i] = nullptr;
		return -1;
		}
	
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
		
		
		gTEKWindowClosingUPPs[i] = NewEventHandlerUPP(receiveWindowClosing);
		error = InstallWindowEventHandler(RGMwind[i]->wind, gTEKWindowClosingUPPs[i],
											GetEventTypeCount(whenWindowClosing), whenWindowClosing,
											nullptr/* user data */,
											&gTEKWindowClosingHandlers[i]/* event handler reference */);
		assert(error == noErr);
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
	
	return(i);
}// RGMnewwin


void
RGMinit ()
{
	register short	i = 0;
	
	
	for (i = 0; i < MAXWIND; i++) RGMwind[i] = nullptr;
}// RGMinit


short
RGMgin	(short		w)
{
	short	result = 0;
	
	
	if (RGsetwind(w)) result = -1;
	else
	{
		setgraphcurs();
		RGMwind[w]->ingin = 1;
	}
	return result;
}// RGMgin


short
RGMpencolor		(short		w,
				 short		color)
{
	short	result = 0;
	
	
	if (RGsetwind(w)) result = -1;
	else ForeColor((long)RGMcolor[color]);
	
	return result;
}// RGMpencolor


short
RGMclrscr	(short		w)
{
	short	result = 0;
	
	
	if (RGsetwind(w)) result = -1;
	else
	{
		Rect		bounds;
		
		
		GetPortBounds(GetWindowPort(RGMwind[w]->wind), &bounds);
		PaintRect(&bounds);
	}
	
	return result;
}// RGMclrscr


short
RGMclose	(short		w)
{
	short	result = 0;
	
	
	if (RGsetwind(w)) result = -1;
	else
	{
		detachGraphics(w);
		RemoveEventHandler(gTEKWindowClosingHandlers[w]), gTEKWindowClosingHandlers[w] = nullptr;
		DisposeEventHandlerUPP(gTEKWindowClosingUPPs[w]), gTEKWindowClosingUPPs[w] = nullptr;
		gWindowResizeHandlers[w] = CommonEventHandlers_WindowResizer();
		DisposeWindow(RGMwind[w]->wind);
		Memory_DisposePtr((Ptr*)&RGMwind[w]->name);
		Memory_DisposePtr((Ptr*)&RGMwind[w]);
		RGMwind[w] = nullptr;
	}
	return result;
}// RGMclose


short
RGMpoint	(short		w,
			 short		x,
			 short		y)
{
	short	result = 0;
	
	
	if (RGsetwind(w)) result = -1;
	else
	{
		MoveTo(x, y);
		LineTo(x, y);
	}
	return result;
}// RGMpoint


short
RGMdrawline		(short		w,
				 short		x0,
				 short		y0,
				 short		x1,
				 short		y1)
{
	long	xl0 = 0L,
			yl0 = 0L,
			xl1 = 0L,
			yl1 = 0L;
	short	result = 0;
	
	
	if (RGsetwind(w)) result = -1;
	else
	{
		xl0 = ((long)x0 * RGMwind[w]->width) / INXMAX;
		yl0 = (long)RGMwind[w]->height - (((long)y0 * RGMwind[w]->height) / INYMAX);
		xl1 = ((long)x1 * RGMwind[w]->width) / INXMAX;
		yl1 = (long)RGMwind[w]->height - (((long)y1 * RGMwind[w]->height) / INYMAX);
	
		MoveTo((short)xl0, (short)yl0);
		LineTo((short)xl1, (short)yl1);
	}
	return result;
}// RGMdrawline


void
RGMinfo		(short		w,
			 short		v,
			 short		UNUSED_ARGUMENT(a),
			 short		UNUSED_ARGUMENT(b),
			 short		UNUSED_ARGUMENT(c),
			 short		UNUSED_ARGUMENT(d))
{
	RGMwind[w]->vg=v;
}

void
RGMpagedone	(short		UNUSED_ARGUMENT(w))
{
}

void
RGMdataline	(short		UNUSED_ARGUMENT(w),
			 short		UNUSED_ARGUMENT(data),
			 short		UNUSED_ARGUMENT(count))
{
}


void
RGMcharmode	(short		UNUSED_ARGUMENT(w),
			 short		UNUSED_ARGUMENT(rotation),
			 short		UNUSED_ARGUMENT(size))
{
}

void	RGMgmode ()
{}

void	RGMtmode ()
{}

void	RGMshowcur ()
{
}

void	RGMlockcur ()
{
}

void	RGMhidecur ()
{
}

void
RGMbell	(short	UNUSED_ARGUMENT(w))
{
}

void
RGMuncover	(short	UNUSED_ARGUMENT(w))
{
}

short
RGMoutfunc	(short	(*UNUSED_ARGUMENT(f))())
{
	return(0);
}

short	RGsetwind(short dnum)
{
	if (dnum<0 || dnum>=MAXWIND) return(-1);

	if (RGMwind[dnum] == nullptr) return -1;

	SetPortWindowPort(RGMwind[dnum]->wind);
	return(0);
}

short	RGfindbyVG(short vg)
{
	short	i = 0;
	
	while (i < MAXWIND) {
		if (RGMwind[i] != nullptr) {
			if (RGMwind[i]->vg == vg)
				break;
			}
		i++;
		}

	if (i >= MAXWIND) return(-1);
	return(i);
}


short
RGattach	(short				vg,
			 SessionRef			virt,
			 char*				name,
			 TektronixMode		TEKtype)
{
	short			dnum = 0;
	char			myname[256],
					ts[256];
	short			result = -1;
	
	
	if ((dnum = RGfindbyVG(vg)) >= 0)
	{
		RGMwind[dnum]->vs = virt;
		RGMwind[dnum]->name = (UInt8*)Memory_NewPtr((long)256);
		
		if (RGMwind[dnum]->name == 0L)
			result = -2;
		else
		{
			myname[0] = '¥';
			if (TEKtype == kTektronixMode4105)
				CPP_STD::strcpy(&myname[1], "[4105] ");
			else if (TEKtype == kTektronixMode4014)
				CPP_STD::strcpy(&myname[1], "[4014] ");
			else
				CPP_STD::strcpy(&myname[1], "[?] ");
				
			CPP_STD::strncpy(&myname[CPP_STD::strlen(myname)], name, 120);
			
			{
				unsigned long	dateTimeResult = 0L;
				Str255			string;
				
				
				GetDateTime(&dateTimeResult);
				TimeString(dateTimeResult, false, string, nullptr); // put the time in the temp string
				StringUtilities_PToC(string, ts);
			}
			
			CPP_STD::strncat(myname, "  ", 2);
			CPP_STD::strncat(myname, ts, 64);
			CPP_STD::strcpy((char*)RGMwind[dnum]->name, myname);
			
			if (RGMwind[dnum]->wind != nullptr)
			{
				StringUtilities_CToPInPlace(myname);
				SetWTitle(RGMwind[dnum]->wind, (StringPtr)myname);
			}
			result = 0;
		}
	}
	
	return result;
}// RGattach


short	RGdetach( short vg)
{
		short	dnum = 0;
		char	myname[256];

	if ((dnum = RGfindbyVG(vg)) < 0) return(-1);
	if (dnum >= MAXWIND)  return(-1);

	if (RGMwind[dnum]->vs != nullptr) {
		if (RGMwind[dnum]->wind != nullptr) {
#if 1
			CPP_STD::strncpy((char *) RGMwind[dnum]->name,
			   		     (char *) &RGMwind[dnum]->name[1],255);
			CPP_STD::strncpy(myname, (char *) RGMwind[dnum]->name,256);
			StringUtilities_CToPInPlace(myname);
			SetWTitle(RGMwind[dnum]->wind, (StringPtr)myname);
#else
			CPP_STD::strncpy((char *) RGMwind[dnum]->name,
					        (char *) &RGMwind[dnum]->name[1],255);
			SetWTitle(RGMwind[dnum]->wind, RGMwind[dnum]->name);
#endif
		}
	}
	return(0);
}

short	RGfindbywind(WindowRef wind)
{
	short	i = 0;

	while (i < MAXWIND) {
		if (RGMwind[i] != nullptr) {
			if (RGMwind[i]->wind == wind)
				break;
			}
		i++;
		}

	if (i >= MAXWIND) return(-1);
	return(i);
}

short	RGupdate( WindowRef wind)
{
	short	i = 0,
			done;

	if ((i = RGfindbywind(wind)) < 0)
		return(-1);

	SetPortWindowPort(wind);
	BeginUpdate(wind);

	VGstopred(RGMwind[i]->vg);
	VGpage(RGMwind[i]->vg);
	done = VGpred(RGMwind[i]->vg,RGMwind[i]->vg);

	EndUpdate(wind);
	if (!done)
	{
		// TEMPORARY - notify session of vector graphics update
		// UNIMPLEMENTED
	}
	return(done);
}

short	RGsupdate( short i)
{
	short	rg;

	rg = RGfindbyVG(i);

	if (rg < 0) return(0);
	SetPortWindowPort(RGMwind[rg]->wind);
	if (!VGpred(RGMwind[rg]->vg,RGMwind[rg]->vg))
	{
		// TEMPORARY - notify session of vector graphics update
		// UNIMPLEMENTED
	}
	else
		return(1);
	return(0);
}

short	RGgetVG(WindowRef wind)
{
	short	i;

	i = RGfindbywind(wind);

	return(RGMwind[i]->vg);
}

short	inSplash(Point *p1, Point *p2)
{
	if ((p1->h - p2->h > 3) || (p2->h - p1->h > 3))
		return(0);
	if ((p1->v - p2->v > 3) || (p2->v - p1->v > 3))
		return(0);

	return(1);
}

void VidSync( void)
{
	UInt32	a = TickCount();
	
	
	while (a == TickCount()) {}
}

void RGmousedown
  (
	WindowRef wind,
	Point *wherein
  )
{
	unsigned long	lx = 0L,
					ly = 0L;
	char			thispaceforent[6];
	short			i = 0;
	Point			where;

	where = *wherein;
	if ((i = RGfindbywind(wind)) < 0)
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
			unless (inSplash(&current,&anchor))
			{
				tc = TickCount();
				while(TickCount() == tc)
					/* loop */;
				VidSync();
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
		
		if (!inSplash(&anchor,&current))
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
	unsetgraphcurs();
	RGMlastclick = TickCount();
}


//
// internal methods
//
#pragma mark -

/*!
This method resizes TEK graphics windows dynamically.

(3.0)
*/
static void
handleNewSize	(WindowRef	inWindow,
				 Float32	UNUSED_ARGUMENT(inDeltaX),
				 Float32	UNUSED_ARGUMENT(inDeltaY),
				 void*		UNUSED_ARGUMENT(inContext))
{
	short		myRGMnum = RGfindbywind(inWindow);
	
	
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
Handles "kEventWindowClose" of "kEventClassWindow"
for TEK windows.  The default handler destroys windows,
which is fine; but it is important to first clean up
some stuff, especially to ensure the terminal is
re-attached to the data stream!

(3.0)
*/
static pascal OSStatus
receiveWindowClosing	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inContext))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent),
					kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowClose);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			short		myRGMnum = RGfindbywind(window);
			
			
			if (myRGMnum > -1)
			{
				short   nonZeroOnError = RGMclose(myRGMnum);
				
				
				if (nonZeroOnError != 0) Sound_StandardAlert();
				result = noErr;
			}
		}
	}
	
	return result;
}// receiveWindowClosing

// BELOW IS REQUIRED NEWLINE TO END FILE
