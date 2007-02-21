/*###############################################################

	ProgressDialog.cp
	
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

#include "UniversalDefines.h"

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <Console.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <NIBLoader.h>
#include <RegionUtilities.h>

// resource includes
#include "DialogResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "DialogUtilities.h"
#include "ProgressDialog.h"



#pragma mark Constants

/*!
IMPORTANT

The following values MUST agree with the control IDs in
the "Dialog" NIB from the package "ProgressDialog.nib".
*/
enum
{
	kSignatureMyTextProgressMessage		= FOUR_CHAR_CODE('PMsg'),
	kSignatureMyProgressBar				= FOUR_CHAR_CODE('Prog')
};
static HIViewID const	idMyTextProgressMessage		= { kSignatureMyTextProgressMessage,	0/* ID */ };
static HIViewID const	idMyProgressBar				= { kSignatureMyProgressBar,			0/* ID */ };

#pragma mark Types

struct ProgressDialog
{
	WindowRef		dialogWindow;			//!< the progress window
	ControlRef		textProgressMessage;	//!< the prompt text
	ControlRef		progressBar;			//!< the thermometer
};
typedef struct ProgressDialog	ProgressDialog;
typedef ProgressDialog*			ProgressDialogPtr;

typedef MemoryBlockPtrLocker< ProgressDialogRef, ProgressDialog >	ProgressDialogPtrLocker;

#pragma mark Internal Method Prototypes

static void		arrangeWindow			(ProgressDialogPtr);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Point						gNewDialogOrigin = { 0, 0 };
	ProgressDialogPtrLocker&	gProgressDialogPtrLocks()	{ static ProgressDialogPtrLocker x; return x; }
}



#pragma mark Public Methods

/*!
Creates a progress dialog displaying the given
message text, and blocking input to other windows
if "inIsModal" is true.  Returns a reference to
the new window.

(3.0)
*/
ProgressDialogRef
ProgressDialog_New		(ConstStringPtr		inStatusText,
						 Boolean			inIsModal)
{
	ProgressDialogRef	result = REINTERPRET_CAST(Memory_NewPtr(sizeof(ProgressDialog)), ProgressDialogRef);
	
	
	if (result != nullptr)
	{
		ProgressDialogPtr	ptr = gProgressDialogPtrLocks().acquireLock(result);
		
		
		// load the NIB containing this dialog (automatically finds the right localization)
		ptr->dialogWindow = NIBWindow(AppResources_ReturnBundleForNIBs(),
										CFSTR("ProgressDialog"), CFSTR("Dialog")) << NIBLoader_AssertWindowExists;
		
		// find references to all controls that are needed for any operation
		{
			OSStatus	error = noErr;
			
			
			error = GetControlByID(ptr->dialogWindow, &idMyTextProgressMessage, &ptr->textProgressMessage);
			assert(error == noErr);
			error = GetControlByID(ptr->dialogWindow, &idMyProgressBar, &ptr->progressBar);
			assert(error == noErr);
		}
		
		// modeless windows should be offset out of the way (not in the middle),
		// whereas modal windows are best left in the center of the screen
		unless (inIsModal) arrangeWindow(ptr);
		
		ProgressDialog_SetStatus(result, inStatusText);
		ProgressDialog_SetTitle(result, "\p");
		
		gProgressDialogPtrLocks().releaseLock(result, &ptr);
	}
	return result;
}// New


/*!
Destroys a progress dialog created with
ProgressDialog_New().

(3.0)
*/
void
ProgressDialog_Dispose		(ProgressDialogRef*		inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		ProgressDialogPtr	ptr = gProgressDialogPtrLocks().acquireLock(*inoutRefPtr);
		
		
		DisposeWindow(ptr->dialogWindow);
		gProgressDialogPtrLocks().releaseLock(*inoutRefPtr, &ptr);
		Memory_DisposePtr(REINTERPRET_CAST(inoutRefPtr, Ptr*));
	}
}// Dispose


/*!
Shows a progress window.  Do not use ProgressDialog_GetWindow()
to help you display a particular progress dialog, because
future implementations may not place each progress dialog in
its own window.

(3.0)
*/
void
ProgressDialog_Display	(ProgressDialogRef	inRef)
{
	ProgressDialogPtr	ptr = gProgressDialogPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		ShowWindow(ptr->dialogWindow);
	}
	gProgressDialogPtrLocks().releaseLock(inRef, &ptr);
}// Display


/*!
To determine which window contains a progress
dialog, use this method.  If any problems
occur, nullptr is returned.

(3.0)
*/
WindowRef
ProgressDialog_ReturnWindow		(ProgressDialogRef	inRef)
{
	ProgressDialogPtr	ptr = gProgressDialogPtrLocks().acquireLock(inRef);
	WindowRef			result = nullptr;
	
	
	if (ptr != nullptr)
	{
		result = ptr->dialogWindow;
	}
	gProgressDialogPtrLocks().releaseLock(inRef, &ptr);
	return result;
}// ReturnWindow


/*!
Sets the value of the progress bar of a progress
dialog.

(3.0)
*/
void
ProgressDialog_SetProgressPercentFull	(ProgressDialogRef	inRef,
										 SInt8				inPercentage)
{
	ProgressDialogPtr	ptr = gProgressDialogPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		SetControl32BitValue(ptr->progressBar, inPercentage);
	}
	gProgressDialogPtrLocks().releaseLock(inRef, &ptr);
}// SetProgressPercentFull


/*!
Changes the style of the progress bar of a
progress dialog; you might do this occasionally
if your process reaches a point where its
completion time cannot be determined.

(3.0)
*/
void
ProgressDialog_SetProgressIndicator		(ProgressDialogRef			inRef,
										 TelnetProgressIndicator	inIndicatorType)
{
	ProgressDialogPtr	ptr = gProgressDialogPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		Boolean		isIndeterminate = false;
		
		
		switch (inIndicatorType)
		{
			case kTelnetProgressIndicatorIndeterminate:
				isIndeterminate = true;
				break;
			
			default:
				isIndeterminate = false;
				break;
		}
		(OSStatus)SetControlData(ptr->progressBar, kControlIndicatorPart, kControlProgressBarIndeterminateTag,
									sizeof(isIndeterminate), &isIndeterminate);
	}
	gProgressDialogPtrLocks().releaseLock(inRef, &ptr);
}// SetProgressIndicator


/*!
Sets the text displayed in a progress dialog.

(3.0)
*/
void
ProgressDialog_SetStatus	(ProgressDialogRef	inRef,
							 ConstStringPtr		inStatusText)
{
	ProgressDialogPtr	ptr = gProgressDialogPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		SetControlText(ptr->textProgressMessage, inStatusText);
	}
	gProgressDialogPtrLocks().releaseLock(inRef, &ptr);
}// SetStatus


/*!
Sets the title of a progress dialog’s window.

(3.0)
*/
void
ProgressDialog_SetTitle		(ProgressDialogRef	inRef,
							 ConstStringPtr		inTitleText)
{
	ProgressDialogPtr	ptr = gProgressDialogPtrLocks().acquireLock(inRef);
	
	
	if (ptr != nullptr)
	{
		SetWTitle(ptr->dialogWindow, inTitleText);
	}
	gProgressDialogPtrLocks().releaseLock(inRef, &ptr);
}// SetTitle


#pragma mark Internal Methods

/*!
Arranges the specified progress dialog in an
appropriate stagger position.

(3.0)
*/
static void
arrangeWindow	(ProgressDialogPtr	inPtr)
{
	if (inPtr != nullptr)
	{
		WindowRef	window = inPtr->dialogWindow;
		
		
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
