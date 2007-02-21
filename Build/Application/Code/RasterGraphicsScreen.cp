/*###############################################################

	RasterGraphicsScreen.c
	
	MacTelnet
		© 1998-2003 by Kevin Grant.
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
 *      by Gaige B. Paulsen
 *
 *	Following are the Per Device calls:
 *
 *   MacRGraster( p,x1,y1,x2,y2,wid)- Plot raster in rect @(x1,y1,x2,y2) with wid @ p
 *   MacRGcopy( x1,y1,x2,y2,x3,y3,x4,y4)- 
 *   MacRGmap( offset,count,data)   - 
 *
 *
 *  WARNING, WARNING!
 *  Gaige has this cute idea about how to do "subwindows" of real windows by shifting
 *  the window number by 4 bits (MAC_WINDOW_SHIFT).  Then, the remainder is the
 *  sub-window number.  It will probably work, but you MUST keep the shifted and
 *  non-shifted numbers straight.  For example, MacRGdestroy() and MacRGremove() take
 *  different uses of the window number right now.  
 *
 *
 *  Macintosh only Routines:
 *
 *      Version Date    Notes
 *      ------- ------  ---------------------------------------------------
 *		0.5 	880912	Initial Coding -GBP
 *      1.0     890216  Minor fixes for 1.0 - TKK
 */

#include "UniversalDefines.h"

// standard-C includes
#include <cstring>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include "MemoryBlocks.h"
#include "RegionUtilities.h"
#include "StringUtilities.h"

// resource includes
#include "StringResources.h"

// MacTelnet includes
#define __ALLNU__

#include "ErrorAlerts.h"
#include "EventLoop.h"
#include "VirtualDevice.h"
#include "RasterGraphicsKernel.h"

#define MAX_MAC_RGS			8
#define MAC_WINDOW_SHIFT	4		/* Bits shifted */

#include "RasterGraphicsScreen.h"



#pragma mark Types

struct RealGraphicsWindow
{
	VirtualDevice_Ref	virtualDevice;		//!< virtual device to draw in - it has its own colors
	WindowRef 			window;				//!< window (nullptr if not in use)
	PaletteHandle 		palette;			//!< the colors
	char				title[256];			//!< the window title
	Point				size;				//!< the window dimensions
};
typedef struct RealGraphicsWindow	RealGraphicsWindow;
typedef RealGraphicsWindow*			RealGraphicsWindowPtr;
typedef RealGraphicsWindowPtr		RealGraphicsWindowArray;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	RealGraphicsWindowArray		MacRGs;
	short						RGwn = 0;   // window number in use; WARNING: set/retrieve only via get/setCurrentICRWindowID()
}

#pragma mark Internal Method Prototypes

static short	getCurrentICRWindowID		();
static void		setCurrentICRWindowID		(short);



#pragma mark Public Methods

/*!
This method creates the global array of real
graphics windows, "MacRGs".

(2.6)
*/
void
MacRGinit ()
{
	short	i = 0;
	
	
	MacRGs = (RealGraphicsWindowArray)Memory_NewPtr(MAX_MAC_RGS * sizeof(RealGraphicsWindow));
	for (i = 0; i < MAX_MAC_RGS; i++) MacRGs[i].window = nullptr;
}// MacRGinit


/************************************************************
 * MacRGnewwindow( name, x1, y1, x2, y2)		-			*
 *		make a new subwindow to wn							*
 ************************************************************/
short
MacRGnewwindow		(char const*	inTitle,
					 short			inX1,
					 short			inY1,
					 short			inX2,
					 short			inY2)
{
	short		result = 0;
	short		x1 = inX1,
				x2 = inX2;
	short		w = 0;
	Rect		wDims;
	
	
	for (w = 0; w < MAX_MAC_RGS && MacRGs[w].window; w++) {}
	
	if (w >= MAX_MAC_RGS) result = -1;
	else
	{
		register SInt16		i = 0;
		
		
		// make sure the width is even
		if ((x2 - x1) & 1) ++x2;
		
		SetRect(&wDims, x1 + 80, inY1 + 80, x2 + 80, inY2 + 80); // arbitrary location
		CPP_STD::strcpy(MacRGs[w].title, inTitle);
		
		// create a Mac OS window
		{
			Str255		title;
			
			
			CPP_STD::strcpy((char*)title, inTitle);
			StringUtilities_CToPInPlace((char*)title);
			{
				OSStatus		error = 0L;
				
				
				error = CreateNewWindow(kDocumentWindowClass,
										kWindowCloseBoxAttribute | kWindowCollapseBoxAttribute,
										&wDims, &MacRGs[w].window);
				if (error == noErr)
				{
					SetWTitle(MacRGs[w].window, title);
					(OSStatus)SetWindowBounds(MacRGs[w].window, kWindowContentRgn, &wDims);
					ShowWindow(MacRGs[w].window);
					EventLoop_SelectBehindDialogWindows(MacRGs[w].window);
				}
				else MacRGs[w].window = nullptr;
			}
			
			if (MacRGs[w].window != nullptr)
			{
				SetWindowKind(MacRGs[w].window, WIN_ICRG);
			}
			else result = -1;
		}
		
		// create a grayscale palette
		if (result >= 0)
		{
			RGBColor	gray;
			
			
			MacRGs[w].palette = NewPalette(256/* number of entries */,
											(CTabHandle)nullptr/* source colors */,
											pmTolerant, 0/* tolerance */);
			if (MacRGs[w].palette != nullptr)
			{
				// NOTE:	QuickDraw always assumes that white is at location 0 and
				//			black is at location 255, even though this is the inverse
				//			of the actual intensity values.
				for (i = 0; i < 256; i++)
				{
					gray.red = i << 8;
					gray.green = i << 8;
					gray.blue = i << 8;
					SetEntryColor(MacRGs[w].palette, i, &gray);
				}
			}
			else result = -1;
		}
		
		// now create the virtual device
		if (result >= 0)
		{
			VirtualDevice_ResultCode	error = kVirtualDevice_ResultCodeSuccess;
			
			
			error = VirtualDevice_New(&MacRGs[w].virtualDevice, &wDims, MacRGs[w].palette);
			
			if ((error != kVirtualDevice_ResultCodeSuccess) &&
				(error != kVirtualDevice_ResultCodeEmptyBoundaries))
			{
				result = -1;
			}
		}
		
		if (result >= 0)
		{
			MacRGs[w].size.h = x2 - x1;
			MacRGs[w].size.v = inY2 - inY1;
			setCurrentICRWindowID(w);
			result = (w << MAC_WINDOW_SHIFT);
		}
	}
	
	// in the event of an error, clean up and tell the user
	if (result < 0)
	{
		if (MacRGs[w].palette != nullptr)
		{
			DisposePalette(MacRGs[w].palette), MacRGs[w].palette = nullptr;
		}
		if (MacRGs[w].window != nullptr)
		{
			DisposeWindow(MacRGs[w].window), MacRGs[w].window = nullptr;
		}
		ErrorAlerts_DisplayStopMessageNoTitle(rStringsMemoryErrors,
												siMemErrorCannotCreateRasterGraphicsWindow);
	}
	
	return result;
}// MacRGnewwindow


/****************************************
 * MacRGsetwindow(wn)		-			*
 *		set the drawing window to wn	*
 ****************************************/
void
MacRGsetwindow	(short		wn)
{
	short		w = wn >> MAC_WINDOW_SHIFT;
	
	
	if (MacRGs[w].window != nullptr)
	{
		SetPortWindowPort(MacRGs[w].window);
		setCurrentICRWindowID(w);
		
		// optionally set the clip region
	}
}// MacRGsetwindow


/*!
To dispose of an ICR graphics window,
use this method.

(2.6)
*/
void
MacRGdestroy	(short		wn)
{
	if (MacRGs[wn].window != nullptr)
	{
		VRdestroybyName((char*)&MacRGs[wn].title);
	}
}// MacRGdestroy

	
/*!
To dispose of an ICR graphics subwindow,
use this method.

(2.6)
*/
void
MacRGremove		(short		wn)
{
	short		w = wn >> MAC_WINDOW_SHIFT;
	
	
	if (MacRGs[w].window != nullptr)
	{
		PixMapHandle	portPixMap = nullptr;
		
		
		VirtualDevice_Dispose(&MacRGs[w].virtualDevice);
		portPixMap = GetPortPixMap(GetWindowPort(MacRGs[w].window));
	#if TARGET_API_MAC_OS8
		(*(*(portPixMap))->pmTable)->ctSeed = GetCTSeed(); // unseed color table
	#else
		// ??? - is the pixmap structure private under Carbon?
		(*(*(portPixMap))->pmTable)->ctSeed = GetCTSeed(); // unseed color table
	#endif
		
		DisposeWindow(MacRGs[w].window), MacRGs[w].window = nullptr;
		if (MacRGs[w].palette != nullptr)
		{
			DisposePalette(MacRGs[w].palette), MacRGs[w].palette = nullptr;
		}
	}
}// MacRGremove


/*!
To find the screen ID for an ICR graphics
window, given its Mac OS window pointer,
use this method.

(2.6)
*/
short
MacRGfindwind	(WindowRef		inWindow)
{
	short	result = 0;
	
	
	if (inWindow == nullptr) result = -2;
	else
	{
		while (result < MAX_MAC_RGS && inWindow != MacRGs[result].window) result++;
		if (result == MAX_MAC_RGS) result = -1;
	}
	return result;
}// MacRGfindwind


/************************************************************************************/
/* MacRGcopy
*  Copybits the image window into the clipboard.
*  
*/
void
MacRGcopy	(WindowRef		inWindow)
{
	Rect copysize,copyfrom;
	long len,wn;
	PicHandle picture;
	PixMapHandle hidep;
		
	if (( wn= MacRGfindwind( inWindow)) <0)
		return;						/* Couldn't do it */
 	
	VirtualDevice_GetPixelMap(MacRGs[wn].virtualDevice, &hidep);
	VirtualDevice_GetBounds(MacRGs[wn].virtualDevice, &copyfrom);
	
	SetPortWindowPort(inWindow);
	
	copysize = copyfrom;						/* boundary of drawing area */

	picture = OpenPicture(&copysize);
	{
		BitMap const*	bitMapPtr = nullptr;
		RGBColor		black,
						white;
		
		
		black.red = black.green = black.blue = 0;
		white.red = white.green = white.blue = RGBCOLOR_INTENSITY_MAX;
		ClipRect(&copysize);
		RGBForeColor(&black);
		RGBBackColor(&white);
		//ForeColor(blackColor);
		//BackColor(whiteColor);
		LockPixels(hidep);
		VirtualDevice_GetBitMapForCopyBits(MacRGs[wn].virtualDevice, &bitMapPtr);
		CopyBits(bitMapPtr, GetPortBitMapForCopyBits(GetWindowPort(inWindow)),
					&copyfrom, &copysize, srcCopy, nullptr);
		UnlockPixels(hidep);
	}
	ClosePicture();
	
	// copy picture to clipboard
	len = GetHandleSize((Handle)picture);
	HLock((Handle)picture);
#if TARGET_API_MAC_OS8
	(OSStatus)ZeroScrap();
	PutScrap(len, kScrapFlavorTypePicture, (Ptr)*picture);
#else
	(OSStatus)ClearCurrentScrap();
	{
		ScrapRef		currentScrap = nullptr;
		
		
		if (GetCurrentScrap(&currentScrap) == noErr)
		{
			(OSStatus)PutScrapFlavor(currentScrap, kScrapFlavorTypePicture,
										kScrapFlavorMaskNone, GetHandleSize((Handle)picture),
										*picture);
		}
	}
#endif
	HUnlock((Handle)picture);
	KillPicture(picture);
}// MacRGcopy


short MacRGupdate( WindowRef wind)
{
	short		result = 0;
	short		wn = 0;
	GWorldPtr	theWorld = nullptr;
	
	
	if ((wn = MacRGfindwind(wind)) < 0) result = -1;
	else
	{
		Rect		worldRect,
					windowRect;
		RGBColor	black,
					white;
		
		
		VirtualDevice_GetGraphicsWorld(MacRGs[wn].virtualDevice, &theWorld);
		SetPortWindowPort(wind);
		black.red = black.green = black.blue = 0;
		white.red = white.green = white.blue = RGBCOLOR_INTENSITY_MAX;
		RGBForeColor(&black);
		RGBBackColor(&white);
		//ForeColor(blackColor);
		//BackColor(whiteColor);
		BeginUpdate(wind);
		LockPixels(GetPortPixMap(theWorld));
		GetPortBounds(theWorld, &worldRect);
		GetPortBounds(GetWindowPort(wind), &windowRect);
		CopyBits(GetPortBitMapForCopyBits(theWorld),
					GetPortBitMapForCopyBits(GetWindowPort(wind)),
					&worldRect, &windowRect, srcCopy, nullptr);
		UnlockPixels(GetPortPixMap(theWorld));
		EndUpdate(wind);
	}
	
	return result;
}


/*!
To render graphics data in a specific region,
use this method.

(2.6)
*/
short
MacRGraster		(char*		data,
				 short		inX1,
				 short		inY1,
				 short		inX2,
				 short		inY2,
				 short		rowbytes)
{
	short		result = 0;	// ÒsuccessÓ is 0
	short		currentWindowID = getCurrentICRWindowID();
	
	
	if (MacRGs[currentWindowID].window != nullptr)
	{
		CGrafPtr		oldPort = nullptr;
		GDHandle		oldDevice = nullptr;
		Rect			tr;
		char*			baseOfThisRun = nullptr;
		short			i = 0;
		PixMapHandle	pixMap = nullptr;
		GWorldPtr		theOffscreenWorld = nullptr;
		long			temp = 0L;
		
		
		VirtualDevice_GetGraphicsWorld(MacRGs[currentWindowID].virtualDevice, &theOffscreenWorld);
		
		GetGWorld(&oldPort, &oldDevice); // save old settings
		SetGWorld(theOffscreenWorld, nullptr);
		
		VirtualDevice_GetPixelMap(MacRGs[currentWindowID].virtualDevice, &pixMap);
		LockPixels(pixMap);
		baseOfThisRun = (char*)GetPixBaseAddr(pixMap);
		temp = (long)((*pixMap)->rowBytes & 0x3FFF);
		baseOfThisRun += temp * inY1 + inX1;
		
		{
			char*		p = baseOfThisRun;
			char*		q = data;
			
			
			for (i = 0; i < rowbytes; i++) *p++ = *q++;
		}
		UnlockPixels(pixMap);
		
		SetRect(&tr, inX1, inY1, inX2 + 1, inY2 + 1);
		(OSStatus)InvalWindowRect(MacRGs[currentWindowID].window, &tr);
		
		SetGWorld(oldPort, oldDevice);
	}
	else result = -1;
	
	return result;
}// MacRGraster


/*!
To set up the color palette for the current
graphics window, use this method.

(2.6)
*/
short
MacRGmap	(short		start,
			 short		length,
			 char*		data)
{
	short		result = 0; // presently undefined; ÒsuccessÓ is 0
	short		i = 0;
	char*		p = data;
	RGBColor	color;
	short		currentWindowID = getCurrentICRWindowID();
	
	
	for (i = start; i < start + length; i++)
	{
		color.red = (*p++) << 8;
		color.green = (*p++) << 8;
		color.blue = (*p++) << 8;
		SetEntryColor(MacRGs[currentWindowID].palette, i, &color);
	}
	NSetPalette(MacRGs[currentWindowID].window, MacRGs[currentWindowID].palette, pmAllUpdates);
	ActivatePalette(MacRGs[currentWindowID].window);
	VirtualDevice_SetColorTable(MacRGs[currentWindowID].virtualDevice, MacRGs[currentWindowID].palette);
	
	return result;
}// MacRGmap


#pragma mark Internal Methods

/*!
To determine the ID of the ICR window
currently in use, use this method.

(3.0)
*/
static short
getCurrentICRWindowID ()
{
	short   result = RGwn;
	
	
	return result;
}// getCurrentICRWindowID


/*!
To specify the ID of the ICR window to
subsequently work with, use this method.

(3.0)
*/
static void
setCurrentICRWindowID	(short		inNewRG)
{
	RGwn = inNewRG;
}// setCurrentICRWindowID

// BELOW IS REQUIRED NEWLINE TO END FILE
