/*###############################################################

	ProgressDialog.cp
	
	MacTelnet
		© 1998-2009 by Kevin Grant.
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

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <Console.h>
#include <HIViewWrap.h>
#include <HIViewWrapManip.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <RegionUtilities.h>

// application includes
#include "AppResources.h"
#include "DialogUtilities.h"
#include "ProgressDialog.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the "Dialog" NIB from the package "ProgressDialog.nib".
*/
static HIViewID const	idMyTextProgressMessage		= { 'PMsg', 0/* ID */ };
static HIViewID const	idMyProgressBar				= { 'Prog', 0/* ID */ };

#pragma mark Types

struct My_ProgressDialog
{
	My_ProgressDialog	(CFStringRef, Boolean);
	~My_ProgressDialog	();
	
	ProgressDialog_Ref	selfRef;				//!< convenient reference to this structure
	NIBWindow			dialogWindow;			//!< the progress window
	HIViewWrap			textProgressMessage;	//!< the prompt text
	HIViewWrap			progressBar;			//!< the thermometer
};
typedef My_ProgressDialog const*	My_ProgressDialogConstPtr;
typedef My_ProgressDialog*			My_ProgressDialogPtr;

typedef MemoryBlockPtrLocker< ProgressDialog_Ref, My_ProgressDialog >	My_ProgressDialogPtrLocker;
typedef LockAcquireRelease< ProgressDialog_Ref, My_ProgressDialog >		My_ProgressDialogAutoLocker;

#pragma mark Internal Method Prototypes

static void		arrangeWindow		(My_ProgressDialogPtr);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Point							gNewDialogOrigin = { 0, 0 };
	My_ProgressDialogPtrLocker&		gProgressDialogPtrLocks()	{ static My_ProgressDialogPtrLocker x; return x; }
}



#pragma mark Public Methods

/*!
Creates a progress dialog displaying the given
message text, and blocking input to other windows
if "inIsModal" is true.  Returns a reference to
the new window.

(3.0)
*/
ProgressDialog_Ref
ProgressDialog_New		(CFStringRef	inStatusText,
						 Boolean		inIsModal)
{
	ProgressDialog_Ref		result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_ProgressDialog(inStatusText, inIsModal), ProgressDialog_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Destroys a progress dialog created with
ProgressDialog_New().

(3.0)
*/
void
ProgressDialog_Dispose		(ProgressDialog_Ref*	inoutRefPtr)
{
	if (gProgressDialogPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteValue, "attempt to dispose of locked progress dialog; outstanding locks",
						gProgressDialogPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_ProgressDialogPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Shows a progress window.  Do not use ProgressDialog_ReturnWindow()
to help you display a particular progress dialog, because
future implementations may not place each progress dialog in
its own window.

(3.0)
*/
void
ProgressDialog_Display	(ProgressDialog_Ref		inRef)
{
	My_ProgressDialogAutoLocker		ptr(gProgressDialogPtrLocks(), inRef);
	
	
	if (ptr != nullptr)
	{
		ShowWindow(ptr->dialogWindow);
	}
}// Display


/*!
To determine which window contains a progress
dialog, use this method.  If any problems
occur, nullptr is returned.

(3.0)
*/
HIWindowRef
ProgressDialog_ReturnWindow		(ProgressDialog_Ref		inRef)
{
	My_ProgressDialogAutoLocker		ptr(gProgressDialogPtrLocks(), inRef);
	HIWindowRef						result = nullptr;
	
	
	if (ptr != nullptr)
	{
		result = ptr->dialogWindow;
	}
	return result;
}// ReturnWindow


/*!
Sets the value of the progress bar of a progress
dialog.

(3.0)
*/
void
ProgressDialog_SetProgressPercentFull	(ProgressDialog_Ref		inRef,
										 SInt8					inPercentage)
{
	My_ProgressDialogAutoLocker		ptr(gProgressDialogPtrLocks(), inRef);
	
	
	if (ptr != nullptr)
	{
		SetControl32BitValue(ptr->progressBar, inPercentage);
	}
}// SetProgressPercentFull


/*!
Changes the style of the progress bar of a
progress dialog; you might do this occasionally
if your process reaches a point where its
completion time cannot be determined.

(3.0)
*/
void
ProgressDialog_SetProgressIndicator		(ProgressDialog_Ref			inRef,
										 ProgressDialog_Indicator	inIndicatorType)
{
	My_ProgressDialogAutoLocker		ptr(gProgressDialogPtrLocks(), inRef);
	
	
	if (ptr != nullptr)
	{
		Boolean		isIndeterminate = false;
		
		
		switch (inIndicatorType)
		{
			case kProgressDialog_IndicatorIndeterminate:
				isIndeterminate = true;
				break;
			
			default:
				isIndeterminate = false;
				break;
		}
		(OSStatus)SetControlData(ptr->progressBar, kControlIndicatorPart, kControlProgressBarIndeterminateTag,
									sizeof(isIndeterminate), &isIndeterminate);
	}
}// SetProgressIndicator


/*!
Sets the text displayed in a progress dialog.

(3.1)
*/
void
ProgressDialog_SetStatus	(ProgressDialog_Ref		inRef,
							 CFStringRef			inStatusText)
{
	My_ProgressDialogAutoLocker		ptr(gProgressDialogPtrLocks(), inRef);
	
	
	if (ptr != nullptr)
	{
		SetControlTextWithCFString(ptr->textProgressMessage, inStatusText);
	}
}// SetStatus


/*!
Sets the title of a progress dialog’s window.

(3.1)
*/
void
ProgressDialog_SetTitle		(ProgressDialog_Ref		inRef,
							 CFStringRef			inTitleText)
{
	My_ProgressDialogAutoLocker		ptr(gProgressDialogPtrLocks(), inRef);
	
	
	if (ptr != nullptr)
	{
		(OSStatus)SetWindowTitleWithCFString(ptr->dialogWindow, inTitleText);
	}
}// SetTitle


#pragma mark Internal Methods

/*!
Constructor.  See ProgressDialog_New().

(3.1)
*/
My_ProgressDialog::
My_ProgressDialog	(CFStringRef	inStatusText,
					 Boolean		inIsModal)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
selfRef				(REINTERPRET_CAST(this, ProgressDialog_Ref)),
dialogWindow		(NIBWindow(AppResources_ReturnBundleForNIBs(),
						CFSTR("ProgressDialog"), CFSTR("Dialog"))
						<< NIBLoader_AssertWindowExists),
textProgressMessage	(dialogWindow.returnHIViewWithID(idMyTextProgressMessage)
						<< HIViewWrap_AssertExists),
progressBar			(dialogWindow.returnHIViewWithID(idMyProgressBar)
						<< HIViewWrap_AssertExists)
{
	// modeless windows should be offset out of the way (not in the middle),
	// whereas modal windows are best left in the center of the screen
	unless (inIsModal) arrangeWindow(this);
	
	ProgressDialog_SetStatus(this->selfRef, inStatusText);
	ProgressDialog_SetTitle(this->selfRef, CFSTR(""));
}// My_ProgressDialog constructor


/*!
Destructor.  See ProgressDialog_Dispose().

(3.1)
*/
My_ProgressDialog::
~My_ProgressDialog ()
{
	DisposeWindow(this->dialogWindow.operator WindowRef());
}// destructor


/*!
Arranges the specified progress dialog in an
appropriate stagger position.

(3.0)
*/
static void
arrangeWindow	(My_ProgressDialogPtr	inPtr)
{
	if (inPtr != nullptr)
	{
		HIWindowRef		window = inPtr->dialogWindow;
		
		
		// is the origin already saved?
		if (gNewDialogOrigin.h == 0)
		{
			enum
			{
				kPaddingH = 10,	// pixels of space between window structure and screen edge
				kPaddingV = 10	// pixels of space between window structure and screen edge
			};
			Rect		screenBounds;
			Rect		structureRect;
			Rect		contentRect;
			OSStatus	error = noErr;
			
			
			RegionUtilities_GetPositioningBounds(window, &screenBounds);
			error = GetWindowBounds(window, kWindowStructureRgn, &structureRect);
			assert_noerr(error);
			error = GetWindowBounds(window, kWindowContentRgn, &contentRect);
			assert_noerr(error);
			
			// position the progress window in the bottom-left corner of its screen (take into
			// account the structure region, but the “location” is that of the content region)
		#if 0
			SetRect(&contentRect,
					screenBounds.left + kPaddingH + (contentRect.left - structureRect.left),
					screenBounds.bottom - kPaddingV - (structureRect.bottom - structureRect.top) +
						(contentRect.top - structureRect.top),
					0, 0);
		#endif
			// position the progress window in the bottom-middle of its screen (take into
			// account the structure region, but the “location” is that of the content region)
			SetRect(&contentRect,
					INTEGER_HALVED((screenBounds.right - screenBounds.left) - (contentRect.right - contentRect.left)),
					screenBounds.bottom - kPaddingV - (structureRect.bottom - structureRect.top) +
						(contentRect.top - structureRect.top),
					0, 0);
			SetPt(&gNewDialogOrigin, contentRect.left, contentRect.top);
		}
		else
		{
			// stagger this progress dialog
			gNewDialogOrigin.h += 10;
			gNewDialogOrigin.v += 10;
		}
		
		MoveWindow(window, gNewDialogOrigin.h, gNewDialogOrigin.v, false/* activate */);
	}
}// arrangeWindow

// BELOW IS REQUIRED NEWLINE TO END FILE
