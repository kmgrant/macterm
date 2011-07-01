/*###############################################################

	DialogUtilities.cp
	
	MacTerm
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

#include "UniversalDefines.h"

// standard-C includes
#include <cctype>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Cursors.h>
#include <Embedding.h>
#include <IconManager.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <WindowInfo.h>

// application includes
#include "DialogTransitions.h"
#include "DialogUtilities.h"
#include "EventLoop.h"



#pragma mark Internal Method Prototypes

static void		dialogClose							(HIWindowRef, Boolean, Boolean);
static void		dialogDisplayEnsureVisibility		(HIWindowRef);
static void		dialogDisplayPreparation			(HIWindowRef, Boolean);



#pragma mark Public Methods

/*!
Removes a dialog window from view.  For users of
Mac OS 8.5 and beyond, this routine will display
a window transition and play the appropriate
theme sound.

This effect is only appropriate for closing a
dialog with the intent to save its contents (that
is, the user clicked OK).  If the user Cancels an
operation, use DialogTransitions_CloseCancel()
instead.

Also note that sometimes you may wish to transition
a dialog to a specific object to emphasize what is
being affected by the user’s changes in the dialog
(zooming into a list item or control, for example).
Look for APIs similar to this one that may be more
appropriate for your dialog.

(3.0)
*/
void
DialogTransitions_Close		(WindowRef	inWindow)
{
	dialogClose(inWindow, false/* is sheet */, false/* no animation */);
	(OSStatus)TransitionWindow(inWindow, kWindowZoomTransitionEffect, kWindowHideTransitionAction, nullptr);
}// Close


/*!
Removes a dialog window immediately from view without
any animation effect whatsoever.  This is ONLY used
in special cases - for example, a text entry dialog
box where the user types something and presses Return
to dismiss the dialog immediately to reveal something
in the window beneath.

(3.0)
*/
void
DialogTransitions_CloseImmediately	(WindowRef	inWindow)
{
	dialogClose(inWindow, false/* is sheet */, true/* no animation */);
}// CloseImmediately


/*!
Removes a dialog window from view while displaying a
zoom transition from the window’s area to the lower-
right-hand corner of the screen.  For users of
Mac OS 8.5 and beyond, this routine will display a
window transition and play the appropriate theme
sound; otherwise, the window simply disappears.

This kind of transition is used to signify that a
dialog’s contents are being completely discarded (it
looks like the dialog is being thrown into the Trash).

However, you normally do not call this routine
directly; instead, use DialogTransitions_CloseCancel(),
which is a macro that calls this routine.  This way,
the Cancel effect can be changed in the future should
the need arise.

(3.0)
*/
void
DialogTransitions_CloseLowerRight	(WindowRef	inWindow)
{
	dialogClose(inWindow, false/* is sheet */, false/* no animation */);
	{
		Rect	lowerRight;
		Rect	screenRect;
		
		
		RegionUtilities_GetWindowDeviceGrayRect(inWindow, &screenRect);
		SetRect(&lowerRight, screenRect.right, screenRect.bottom, screenRect.right, screenRect.bottom);
		(OSStatus)TransitionWindow(inWindow, kWindowZoomTransitionEffect, kWindowHideTransitionAction, &lowerRight);
	}
}// CloseLowerRight


#if TARGET_API_MAC_CARBON
/*!
Removes a sheet window from view, with the
appropriate window sliding effect.

Mac OS X only.

(3.0)
*/
void
DialogTransitions_CloseSheet	(WindowRef	inWindow,
								 WindowRef	UNUSED_ARGUMENT(inParentWindow))
{
	dialogClose(inWindow, true/* is sheet */, false/* no animation */);
}// CloseSheet
#endif


#if TARGET_API_MAC_CARBON
/*!
Removes a sheet window from view with no closing
animation.  This is used for standard “close warning”
alerts (in which case you would immediately hide the
parent window along with the sheet).

Mac OS X only.

(3.0)
*/
void
DialogTransitions_CloseSheetImmediately		(WindowRef		inWindow,
											 WindowRef		UNUSED_ARGUMENT(inParentWindow))
{
	dialogClose(inWindow, true/* is sheet */, true/* no animation */);
}// CloseSheetImmediately
#endif


/*!
Removes from view a dialog window that is a child
of another window.  For users of Mac OS 8.5 and
beyond, this routine will display a window transition
that “comes to” the center of the control rectangle
of the specified control (within its owning window),
and will play the appropriate theme sound.

If you pass nullptr for "inToWhat", this method just
invokes DialogTransitions_Close().

(3.0)
*/
void
DialogTransitions_CloseToControl	(WindowRef		inWindow,
									 ControlRef		inToWhat)
{
	Rect		bounds;
	
	
	GetControlBounds(inToWhat, &bounds);
	DialogTransitions_CloseToRectangle(inWindow, GetControlOwner(inToWhat), &bounds);
}// CloseToControl


/*!
Removes from view a dialog window that is a child
of another window.  For users of Mac OS 8.5 and
beyond, this routine will display a window transition
that “comes to” the given rectangle (in the local
coordinates of the designated parent window), and
will play the appropriate theme sound.

This is an unusual effect, but you might use it to
zoom to a specific part of a control, such as the
boundary of a list item (to show that the dialog is
modifying that list item).

(3.0)
*/
void
DialogTransitions_CloseToRectangle	(WindowRef		inWindow,
									 WindowRef		inParentWindow,
									 Rect const*	inToWhere)
{
	if (inToWhere == nullptr) DialogTransitions_Close(inWindow);
	else
	{
		dialogClose(inWindow, false/* is sheet */, false/* no animation */);
		{
			Rect		bounds;
			
			
			bounds = *inToWhere;
			{
				GrafPtr		oldPort = nullptr;
				
				
				GetPort(&oldPort);
				SetPortWindowPort(inParentWindow);
				LocalToGlobal((Point*)&bounds.top);
				LocalToGlobal((Point*)&bounds.bottom);
				SetPort(oldPort);
			}
			
			(OSStatus)TransitionWindow(inWindow, kWindowZoomTransitionEffect, kWindowHideTransitionAction, &bounds);
		}
	}
}// CloseToRectangle


/*!
Removes from view a dialog window that is a child of
another window.  For users of Mac OS 8.5 and beyond,
this routine will display a window transition that
“comes to” the bounding rectangle of the specified
window region, and will play the appropriate theme
sound.

This is an unusual effect, but you might use it to
zoom to a specific part of a window - for example, a
dialog that sets the title of a window might zoom
closed into the title region of the window frame.

(3.0)
*/
void
DialogTransitions_CloseToWindowRegion	(WindowRef			inWindow,
										 WindowRef			inParentWindow,
										 WindowRegionCode	inToWhere)
{
	dialogClose(inWindow, false/* is sheet */, false/* no animation */);
	{
		Rect	bounds;
		
		
		(OSStatus)GetWindowBounds(inParentWindow, inToWhere, &bounds);
		(OSStatus)TransitionWindow(inWindow, kWindowZoomTransitionEffect, kWindowHideTransitionAction, &bounds);
	}
}// CloseToWindowRegion


/*!
Removes a dialog window from view while displaying a
zoom transition from the window’s area to the upper-
left-hand corner of the screen.  For users of Mac OS
8.5 and beyond, this routine will display a window
transition and play the appropriate theme sound;
otherwise, the window simply disappears.

This kind of transition is used to signify that a
dialog’s contents are system-wide (it looks like the
dialog is being pulled into the Apple menu).

(3.0)
*/
void
DialogTransitions_CloseUpperLeft	(WindowRef		inWindow)
{
	dialogClose(inWindow, false/* is sheet */, false/* no animation */);
	{
		Rect		upperLeft;
		
		
		SetRect(&upperLeft, 0, 0, 0, 0);
		(OSStatus)TransitionWindow(inWindow, kWindowZoomTransitionEffect, kWindowHideTransitionAction, &upperLeft);
	}
}// CloseUpperLeft


/*!
Displays a dialog window and makes it active.
For users of Mac OS 8.5 and beyond, this
routine will display a window transition and
play the appropriate theme sound.

This effect is appropriate for most dialogs.
However, dialogs that are clearly pertinent
to specific things, such as list items or
controls, would be more intuitively displayed
using a more refined transition API.  Look
for APIs similar to this one that may be more
appropriate for your needs.

(3.0)
*/
void
DialogTransitions_Display		(WindowRef		inWindow)
{
	dialogDisplayPreparation(inWindow, false/* is sheet */);
	(OSStatus)TransitionWindow(inWindow, kWindowZoomTransitionEffect, kWindowShowTransitionAction, nullptr);
	dialogDisplayEnsureVisibility(inWindow);
}// Display


/*!
Displays and activates a dialog window that is a
child of another window.  For users of Mac OS 8.5
and beyond, this routine will display a window
transition that “comes from” the center of the
control rectangle of the specified control, and
will play the appropriate theme sound.

If you pass nullptr for "inFromWhat", this method
invokes DialogTransitions_Display().

(3.0)
*/
void
DialogTransitions_DisplayFromControl	(WindowRef		inWindow,
										 ControlRef		inFromWhat)
{
	Rect		bounds;
	
	
	GetControlBounds(inFromWhat, &bounds);
	DialogTransitions_DisplayFromRectangle(inWindow, GetControlOwner(inFromWhat), &bounds);
}// DisplayFromControl


/*!
Displays and activates a dialog window that is a
child of another window.  For users of Mac OS 8.5
and beyond, this routine will display a window
transition that “comes from” the specified rectangle
(in the local coordinates of the designated parent
window), and will play the appropriate theme sound.

(3.0)
*/
void
DialogTransitions_DisplayFromRectangle	(WindowRef		inWindow,
										 WindowRef		inParentWindow,
										 Rect const*	inFromWhere)
{
	if (inFromWhere == nullptr) DialogTransitions_Display(inWindow);
	else
	{
		dialogDisplayPreparation(inWindow, false/* is sheet */);
		{
			Rect		bounds;
			
			
			bounds = *inFromWhere;
			{
				GrafPtr		oldPort = nullptr;
				
				
				GetPort(&oldPort);
				SetPortWindowPort(inParentWindow);
				LocalToGlobal((Point*)&bounds.top);
				LocalToGlobal((Point*)&bounds.bottom);
				SetPort(oldPort);
			}
			
			(OSStatus)TransitionWindow(inWindow, kWindowZoomTransitionEffect, kWindowShowTransitionAction, &bounds);
		}
		dialogDisplayEnsureVisibility(inWindow);
	}
}// DisplayFromRectangle


/*!
Displays and activates a dialog window that is a
child of another window.  For users of Mac OS 8.5
and beyond, this routine will display a window
transition that “comes from” the boundaries of the
specified region of the given parent window, and
will play the appropriate theme sound.

You might use this routine to spawn a dialog that
sets a window frame attribute - for example, a
dialog that sets the window title could zoom from
the title region.

(3.0)
*/
void
DialogTransitions_DisplayFromWindowRegion	(WindowRef			inWindow,
											 WindowRef			inParentWindow,
											 WindowRegionCode	inFromWhere)
{
	dialogDisplayPreparation(inWindow, false/* is sheet */);
	{
		Rect	bounds;
		
		
		(OSStatus)GetWindowBounds(inParentWindow, inFromWhere, &bounds);
		(OSStatus)TransitionWindow(inWindow, kWindowZoomTransitionEffect, kWindowShowTransitionAction, &bounds);
	}
	dialogDisplayEnsureVisibility(inWindow);
}// DisplayFromWindowRegion


#if TARGET_API_MAC_CARBON
/*!
Displays and activates a sheet window, with the
appropriate window sliding effect.

Sheets are automatically made translucent unless
you require complete opaqueness with "inIsOpaque".

Mac OS X only.

(3.0)
*/
void
DialogTransitions_DisplaySheet	(WindowRef		inWindow,
								 WindowRef		inParentWindow,
								 Boolean		inIsOpaque)
{
	EventLoop_SelectBehindDialogWindows(inParentWindow); // handles situation of user clicking background close box (for instance)
	dialogDisplayPreparation(inWindow, true/* is sheet */);
#if 0
	(OSStatus)TransitionWindowAndParent(inWindow, inParentWindow,
										kWindowSheetTransitionEffect, kWindowShowTransitionAction,
										nullptr/* boundaries */);
#else
	unless (inIsOpaque)
	{
		unless (FlagManager_Test(kFlagOS10_2API))
		{
			// 10.1 and earlier support an Appearance brush for this
			(OSStatus)SetThemeWindowBackground(inWindow, kThemeBrushSheetBackgroundTransparent, false/* update */);
		}
	}
	(OSStatus)ShowSheetWindow(inWindow, inParentWindow);
#endif
	dialogDisplayEnsureVisibility(inWindow);
}// DisplaySheet
#endif


#if TARGET_API_MAC_CARBON
/*!
Displays and activates a sheet window, with the
appropriate window sliding effect, from a non-
standard location (that is, somewhere other than
the bottom edge of the title bar).

Sheets are automatically made translucent unless
you require complete opaqueness with "inIsOpaque".

(3.0)
*/
void
DialogTransitions_DisplaySheetFromRectangle		(WindowRef		inWindow,
												 WindowRef		inParentWindow,
												 Rect const*	inFromWhere)
{
	if (inFromWhere == nullptr) DialogTransitions_DisplaySheet(inWindow, inParentWindow, false/* opaque */);
	else
	{
		dialogDisplayPreparation(inWindow, false/* is sheet */);
		{
			Rect		bounds;
			
			
			bounds = *inFromWhere;
			{
				GrafPtr		oldPort = nullptr;
				
				
				GetPort(&oldPort);
				SetPortWindowPort(inParentWindow);
				LocalToGlobal((Point*)&bounds.top);
				LocalToGlobal((Point*)&bounds.bottom);
				SetPort(oldPort);
			}
			
			#if 1
			(OSStatus)TransitionWindowAndParent(inWindow, inParentWindow, kWindowSheetTransitionEffect, kWindowShowTransitionAction, &bounds);
			#endif
		}
		dialogDisplayEnsureVisibility(inWindow);
	}
}// DisplaySheetFromRectangle
#endif


#pragma mark Internal Methods

/*!
Closes dialog boxes in a consistent manner.
This routine undoes any “damage” caused by a
call to dialogDisplayPreparation().

(3.0)
*/
static void
dialogClose		(HIWindowRef	inWindow,
				 Boolean		inIsSheet,
				 Boolean		inNoAnimation)
{
	if (inIsSheet)
	{
	#if TARGET_API_MAC_OS8
		// sheets are not allowed prior to Mac OS X!
		HideWindow(inWindow);
	#else
		WindowRef		parentWindow = nullptr;
		
		
		if ((GetSheetWindowParent(inWindow, &parentWindow) == noErr) &&
			(GetWindowKind(parentWindow) != kDialogWindowKind))
		{
			// restore close and zoom boxes - assume ANY window displaying a sheet has both of these boxes
			(OSStatus)ChangeWindowAttributes(parentWindow,
												kWindowCloseBoxAttribute | kWindowFullZoomAttribute/* set attributes */,
												0L/* clear attributes */);
		}
		
		// close the dialog
		if (inNoAnimation) HideWindow(inWindow);
		else (OSStatus)HideSheetWindow(inWindow);
	#endif
	}
	else
	{
		// close the dialog and restore floating windows
		HideWindow(inWindow);
	}
	Embedding_RestoreFrontmostWindow();
}// dialogClose


/*!
Ensures a dialog box is displayed.  The documentation
on what TransitionWindow() does is unclear, so this
makes sure dialog windows are showing.

(3.0)
*/
static void
dialogDisplayEnsureVisibility	(HIWindowRef	inWindow)
{
	if (inWindow != nullptr)
	{
		unless (IsWindowVisible(inWindow)) ShowWindow(inWindow);
		EventLoop_SelectOverRealFrontWindow(inWindow);
	}
	Cursors_UseArrow();
}// dialogDisplayEnsureVisibility


/*!
Prepares for a dialog’s display by dimming the
frontmost window and deactivating all floating windows.
Later, use dialogClose() to close the dialog, and all
effects of invoking this method will be reversed.

(3.0)
*/
static void
dialogDisplayPreparation	(HIWindowRef	UNUSED_ARGUMENT(inWindow),
							 Boolean		inIsSheet)
{
	if (inIsSheet) 
	{
		// remove close and zoom boxes (which disables the boxes, standard parent window behavior for sheets);
		// this is not required for sheets displayed from NIBs, but required for ones created from dialog resources
		(OSStatus)ChangeWindowAttributes(EventLoop_ReturnRealFrontWindow(), 0L/* set attributes */,
											kWindowCloseBoxAttribute | kWindowFullZoomAttribute/* clear attributes */);
	}
	Embedding_DeactivateFrontmostWindow();
}// dialogDisplayPreparation

// BELOW IS REQUIRED NEWLINE TO END FILE
