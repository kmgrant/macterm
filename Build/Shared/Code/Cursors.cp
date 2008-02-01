/*###############################################################

	Cursors.cp
	
	Interface Library 1.1
	© 1998-2006 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "UniversalDefines.h"

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <ConstantsRegistry.h>
#include <Cursors.h>
#include <FlagManager.h>
#include <MemoryBlocks.h>
#include <Releases.h>



#pragma mark Types

struct CursorLink
{
	CursHandle				bwCursorHandle;
	CCrsrHandle				colorCursorHandle;
	SInt16					cursorID;
	struct CursorLink*		next;
};
typedef struct CursorLink CursorLink;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	CursorLink*		gDynamicCursorListPtr = nullptr;
	SInt16			gCurrentCursor = kCursor_Arrow;
	UInt32			gCursorStageCount = 0L;
	UInt32			gCountAtLastUpdate = 0L;
	UInt32			gCountAtDeferral = 0L;
	UInt32			gDeferralTicks = 0L;
	Boolean			gHaveAppearanceCursors = false;
	Boolean			gAppearanceCursorInEffect = false;
	Boolean			gAnimatedCursorInEffect = false;
}

#pragma mark Internal Method Prototypes

static void		setCurrentCursorID		(SInt16);



#pragma mark Public Methods

/*!
Use this method to set up this module.
You must call this routine before using
any other routines in this module, to
be sure that the module will work.

(1.0)
*/
void
Cursors_Init ()
{
	//UInt32	appearanceFlags = 0L;
	//UInt32	appearanceVersion = 0L;
	SInt32		sysv = 0L;
	OSStatus	error = noErr;
	
	
	//error = Gestalt(gestaltAppearanceAttr, &appearanceFlags);
	//error = Gestalt(gestaltAppearanceVersion, &appearanceVersion);
	//majorRev = Releases_ReturnMajorRevisionForVersion(appearanceVersion);
	//minorRev = Releases_ReturnMinorRevisionForVersion(appearanceVersion);
	
	// Appearance 1.1 or later supports theme cursors
	//gHaveAppearanceCursors = ((appearanceFlags & (1 << gestaltAppearanceExists)) &&
	//							(((majorRev == 0x01) && (minorRev >= 0x01)) || (majorRev > 0x01)));
	
	// problems with Appearance code - to hell with it, just look for OS 8.5 or later
	error = Gestalt(gestaltSystemVersion, &sysv);
	if (error == noErr)
	{
		UInt8	majorRev = Releases_ReturnMajorRevisionForVersion(sysv);
		UInt8	minorRev = Releases_ReturnMinorRevisionForVersion(sysv);
		
		
		// Mac OS 8.5?
		gHaveAppearanceCursors = (((majorRev == 0x08) && (minorRev >= 0x05)) || (majorRev > 0x08));
	}
}// Init


/*!
Use this method when you are done using
cursors to dispose of any memory that
was allocated.

(1.0)
*/
void
Cursors_Done ()
{
	// make sure none of the to-be-killed cursors is in use!
	InitCursor();
	
	// destroy the linked list and all cursors
	if (gDynamicCursorListPtr != nullptr)
	{
		CursorLink*		ptr = gDynamicCursorListPtr;
		Ptr				memoryManagerPtr = nullptr;
		
		
		while (ptr != nullptr)
		{
			memoryManagerPtr = (Ptr)ptr;
			DisposeCCursor(ptr->colorCursorHandle);
			ptr = ptr->next;
			DisposePtr(memoryManagerPtr), memoryManagerPtr = nullptr;
		}
	}
}// Done


/*!
Use this method to determine if the installed
version of the Appearance Manager supports
theme cursors.

(1.0)
*/
Boolean
Cursors_AppearanceCursorsAvail ()
{
	return gHaveAppearanceCursors;
}// AppearanceCursorsAvail


/*!
To conditionally change the cursor to a
watch, call this method.  In order for
the deferral to work, your application
must be calling Cursors_Idle() quite
regularly (ideally in its own thread).

This method is useful if you don't want
the cursor to flicker on fast machines,
but you are performing an operation
that is potentially slow on certain
computers.  On the faster machines, it
is likely the deferral time will not
elapse so the cursor will not change;
on slower machines, the operation may
take enough time that it is prudent to
display the watch cursor.

As soon as you force the cursor to a
particular value, any cursor deferral
is automatically cancelled.  Thus, you
generally call Cursors_DeferredWatch(),
perform a lengthy operation (with
sufficiently periodic calls to the
Cursors_Idle() method), and finish by
calling another routine, such as
Cursors_UseArrow(), that forces the
cursor to a particular shape and
cancels the deferred watch cursor.

(1.0)
*/
SInt16
Cursors_DeferredUseWatch	(UInt32		inAfterHowManyTicks)
{
	gCountAtDeferral = TickCount();
	gDeferralTicks = inAfterHowManyTicks;
	return Cursors_ReturnCurrent();
}// DeferredUseWatch


/*!
Call this method once in your main event loop
to make sure that animated cursors animate.

(1.0)
*/
void
Cursors_Idle ()
{
	UInt32		currentTicks = TickCount();
	
	
	// any deferred cursor?
	if ((gDeferralTicks > 0L) && ((currentTicks - gCountAtDeferral) > gDeferralTicks))
	{
		Cursors_UseWatch();
	}
	
	// animate any theme cursor that can be animated, but no more
	// than every so many ticks (to keep animation smooth)
	if (gAnimatedCursorInEffect)
	{
		if (((currentTicks - gCountAtLastUpdate) > 8) && (gAppearanceCursorInEffect))
		{
			// then it's time to update the cursor
			(OSStatus)SetAnimatedThemeCursor(Cursors_ReturnCurrent(), ++gCursorStageCount);
			gCountAtLastUpdate = TickCount();
		}
	}
}// Idle


/*!
To determine the ID of the cursor that was
most recently set using this module, use this
method.  If you consistently use this module
to change the cursor *everywhere* in your
program, then you can use this method to
effectively get the “current” cursor, which
has many advantages.

(1.0)
*/
SInt16
Cursors_ReturnCurrent ()
{
	return gCurrentCursor;
}// ReturnCurrent


/*!
Use this method to use a cursor with a particular
resource ID.  This routine will first check for a
theme cursor (Mac OS 8.5 or later).  If it cannot
find a theme cursor appropriate for the request or
if Appearance 1.1 or later is not available, this
method then looks for a color cursor ('crsr'
resource) with the specified ID.  If no color
version is available, it looks for a monochrome
('CURS' resource) cursor.  If no appropriate cursor
can be found, the cursor becomes an arrow.

The ID of the cursor most recently specified using
this library is returned.  It is highly recommended
that you use this library for *every* cursor change
you make, so you can keep track of cursors easily.
Regardless of whether a cursor can be found or not,
this method always returns the cursor that was
*supposed* to be in place prior to the change -
that way, your code can reliably determine what
cursor is to be in effect.

To support animated theme cursors, you should call
the method Cursors_Idle() once per event loop, in
modal dialog box event filters, and continuously
throughout a series of procedures that might take
a non-trivial amount of time on a computer that
your application supports.

(1.0)
*/
SInt16
Cursors_Use		(SInt16		inCursorID)
{
	Boolean		tryToGetResource = true;
	SInt16		result = Cursors_ReturnCurrent();
	
	
	// cancel any deferred cursor
	gCountAtDeferral = gDeferralTicks = 0L;
	
	//if (result != inCursorID)
	{
		// initially assume no Appearance cursors are available
		gAppearanceCursorInEffect = gAnimatedCursorInEffect = false;
		setCurrentCursorID(inCursorID);
		
		if (Cursors_AppearanceCursorsAvail())
		{
			ThemeCursor		cursor = kThemeArrowCursor;
			
			
			// determine the appropriate Appearance Manager
			// analog cursor for the specified resource ID,
			// and use it
			tryToGetResource = false;
			
			switch (inCursorID)
			{
			case kCursor_Arrow:
				cursor = kThemeArrowCursor;
				gAppearanceCursorInEffect = true;
				break;
			
			case kCursor_ArrowCopy:
				cursor = kThemeCopyArrowCursor;
				gAppearanceCursorInEffect = true;
				break;
			
			case kCursor_ArrowContextualMenu:
				cursor = kThemeContextualMenuArrowCursor;
				gAppearanceCursorInEffect = true;
				break;
			
			case kCursor_Crosshairs:
				cursor = kThemeCrossCursor;
				gAppearanceCursorInEffect = true;
				break;
			
			case kCursor_Disallow:
				cursor = kThemeNotAllowedCursor;
				gAppearanceCursorInEffect = true;
				break;
				
			case kCursor_HandGrab:
				cursor = kThemeClosedHandCursor;
				gAppearanceCursorInEffect = true;
				break;
			
			case kCursor_HandOpen:
				cursor = kThemeOpenHandCursor;
				gAppearanceCursorInEffect = true;
				break;
			
			case kCursor_HandPoint:
				cursor = kThemePointingHandCursor;
				gAppearanceCursorInEffect = true;
				break;
			
			case kCursor_IBeam:
				cursor = kThemeIBeamCursor;
				gAppearanceCursorInEffect = true;
				break;
			
			case kCursor_Plus:
				cursor = kThemePlusCursor;
				gAppearanceCursorInEffect = true;
				break;
			
			case kCursor_Poof:
				if (FlagManager_Test(kFlagOS10_3API))
				{
					// Mac OS X 10.3 has a special theme cursor for this effect
					//cursor = kThemePoofCursor; // TEMPORARY
					//gAppearanceCursorInEffect = true;
				}
				break;
				
			case kCursor_Watch:
			#if TARGET_API_MAC_OS8
				cursor = kThemeWatchCursor;
				gAppearanceCursorInEffect = gAnimatedCursorInEffect = true;
			#else
				// under Mac OS X, the spinning cursor appears automatically, when appropriate
			#endif
				break;
			
			case kCursor_ArrowHelp:
			case kCursor_ArrowWatch:
				cursor = kThemeArrowCursor;
				gAppearanceCursorInEffect = true;
				break;
			
			default:
				tryToGetResource = true;
				break;
			}
			
			if (gAppearanceCursorInEffect) SetThemeCursor(cursor);
			if (gAnimatedCursorInEffect) gCursorStageCount = 0L;
		}
		
		if (tryToGetResource)
		{
			CursorLink*		ptr = gDynamicCursorListPtr;
			Boolean			createNew = true;
			
			
			if (gDynamicCursorListPtr != nullptr)
			{
				ptr = gDynamicCursorListPtr;
				if (ptr != nullptr) do
				{
					if (ptr->cursorID == inCursorID) createNew = false;
					else if (ptr->next != nullptr) ptr = ptr->next;
				} while ((ptr->next != nullptr) && (createNew));
				if (createNew)
				{
					ptr->next = (CursorLink*)Memory_NewPtr(sizeof(CursorLink));
					ptr = ptr->next;
				}
			}
			else
			{
				ptr = gDynamicCursorListPtr = (CursorLink*)Memory_NewPtr(sizeof(CursorLink));
			}
			
			if (ptr != nullptr)
			{
				ptr->next->colorCursorHandle = GetCCursor(inCursorID);
				if (ptr->next->colorCursorHandle == nullptr)
				{
					ptr->next->bwCursorHandle = GetCursor(inCursorID);
					if (ptr->next->bwCursorHandle == nullptr)
					{
						InitCursor(); // set to an arrow
					}
					else
					{
						SInt8		state = HGetState((Handle)ptr->next->bwCursorHandle);
						
						
						HLockHi((Handle)ptr->next->bwCursorHandle);
						SetCursor(*(ptr->next->bwCursorHandle)); // use monochrome cursor
						HSetState((Handle)ptr->next->bwCursorHandle, state);
					}
				}
				else SetCCursor(ptr->next->colorCursorHandle); // use color cursor
			}
			else
			{
				//Cursor	arrow;
				
				
				//GetQDGlobalsArrow(&arrow);
				//SetCursor(&arrow); // set to an arrow
				InitCursor();
			}
		}
	}
	
	return result;
}// Use


/*!
Use this convenience method to set the cursor
to the standard arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseArrow ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_Arrow);
	return result;
}// UseArrow


/*!
Use this convenience method to set the cursor
to an arrow with a miniature contextual menu
next to it (to signify that a menu will appear).
If there is a color cursor available, it is
automatically chosen instead of any monochrome
version.  If neither a color nor black-and-white
cursor of the proper ID can be found, the cursor
is set to a plain arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseArrowContextualMenu ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_ArrowContextualMenu);
	return result;
}// UseArrowContextualMenu


/*!
Use this convenience method to set the cursor
to an arrow with a miniature plus sign next to
it (to signify that copying will occur).  If
there is a color cursor available, it is
automatically chosen instead of any monochrome
version.  If neither a color nor black-and-white
cursor of the proper ID can be found, the cursor
is set to a plain arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseArrowCopy ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_ArrowCopy);
	return result;
}// UseArrowCopy


/*!
Use this convenience method to set the cursor
to an arrow with a miniature question-mark
icon next to it (for Balloon Help).  If there
is a color cursor available, it is automatically
chosen instead of any monochrome version.  If
neither a color nor black-and-white cursor of
the proper ID can be found, the cursor is set
to a plain arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseArrowHelp ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_ArrowHelp);
	return result;
}// UseArrowHelp


/*!
Use this convenience method to set the cursor
to an arrow with a miniature watch next to it
(to signify that another thread is in progress).
If there is a color cursor available, it is
automatically chosen instead of any monochrome
version.  If neither a color nor black-and-white
cursor of the proper ID can be found, the cursor
is set to a plain arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseArrowWatch ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_ArrowWatch);
	return result;
}// UseArrowWatch


/*!
Use this convenience method to set the cursor
to crosshairs (used for drawing).  If there is
a color cursor available, it is automatically
chosen instead of any monochrome version.  If
neither a color nor black-and-white cursor of
the proper ID can be found, the cursor is set
to an arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseCrosshairs ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_Crosshairs);
	return result;
}// UseCrosshairs


/*!
Use this convenience method to set the cursor
to a barred circle (used to indicate that no
drop may occur).  If there is a color cursor
available, it is automatically chosen instead
of any monochrome version.  If neither a color
nor black-and-white cursor of the proper ID
can be found, the cursor is set to an arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.1)
*/
SInt16
Cursors_UseDisallow ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_Disallow);
	return result;
}// UseDisallow


/*!
Use this convenience method to set the cursor
to a closed hand (for document-moving operations
that are in dragging progress).  If there is a
color cursor available, it is automatically chosen
instead of any monochrome version.  If neither a
color nor black-and-white cursor of the proper ID
can be found, the cursor is set to an arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseHandGrab ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_HandGrab);
	return result;
}// UseHandGrab


/*!
Use this convenience method to set the cursor
to an open hand (for document-moving operations).
If there is a color cursor available, it is
automatically chosen instead of any monochrome
version.  If neither a color nor black-and-white
cursor of the proper ID can be found, the cursor
is set to an arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseHandOpen ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_HandOpen);
	return result;
}// UseHandOpen


/*!
Use this convenience method to set the cursor
to a pointing hand (for miscellaneous operations,
but often for mouse-overs of "hot text" areas).
If there is a color cursor available, it is
automatically chosen instead of any monochrome
version.  If neither a color nor black-and-white
cursor of the proper ID can be found, the cursor
is set to an arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseHandPoint ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_HandPoint);
	return result;
}// UseHandPoint


/*!
Use this convenience method to set the cursor
to a magnifying glass (for browsing).  If there
is a color cursor available, it is automatically
chosen instead of any monochrome version.  If
neither a color nor black-and-white cursor of
the proper ID can be found, the cursor is set
to an arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

Unimplemented.

(1.0)
*/
SInt16
Cursors_UseMagnifyingGlass ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_UseArrow();
	return result;
}// UseMagnifyingGlass


/*!
Use this convenience method to set the cursor
to an I-beam (for text editing).  If there is
a color cursor available, it is automatically
chosen instead of any monochrome version.  If
neither a color nor black-and-white cursor of
the proper ID can be found, the cursor is set
to an arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseIBeam ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_IBeam);
	return result;
}// UseIBeam


/*!
Use this convenience method to set the cursor
to a plus (for spreadsheets).  If there is
a color cursor available, it is automatically
chosen instead of any monochrome version.  If
neither a color nor black-and-white cursor of
the proper ID can be found, the cursor is set
to an arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UsePlus ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_Plus);
	return result;
}// UsePlus


/*!
Use this convenience method to set the cursor
to an arrow with a puff of smoke next to it
(used to indicate that an item will go "poof"
if the mouse button is released).  If there is
a color cursor available, it is automatically
chosen instead of any monochrome version.  If
neither a color nor black-and-white cursor of
the proper ID can be found, the cursor is set
to an arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.1)
*/
SInt16
Cursors_UsePoof ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_Poof);
	return result;
}// UsePoof


/*!
Use this convenience method to set the cursor
to a watch (for time-consuming operations).
If there is a color cursor available, it is
automatically chosen instead of any monochrome
version.  If neither a color nor black-and-white
cursor of the proper ID can be found, the cursor
is set to an arrow.

The ID of the cursor most recently specified
using this library is returned.  It is highly
recommended that you use this library for
*every* cursor change you make, so you can
keep track of cursors easily.

(1.0)
*/
SInt16
Cursors_UseWatch ()
{
	SInt16		result = Cursors_ReturnCurrent();
	
	
	result = Cursors_Use(kCursor_Watch);
	return result;
}// UseWatch


#pragma mark Internal Methods

/*!
To specify the ID of the “current”
cursor, use this method.  This helps
to keep track of what the shape of
the mouse pointer most likely is
(unless it was changed by some other
process).  Use Cursors_ReturnCurrent()
to determine this value later.

(1.0)
*/
static void
setCurrentCursorID		(SInt16		inNewCursor)
{
	gCurrentCursor = inNewCursor;
}// setCurrentCursorID

// BELOW IS REQUIRED NEWLINE TO END FILE
