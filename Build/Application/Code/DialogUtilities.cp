/*!	\file DialogUtilities.cp
	\brief Lists methods that simplify window management,
	dialog box management, and window content control management.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#include "DialogUtilities.h"
#include <UniversalDefines.h>

// standard-C includes
#include <cctype>
#include <cstring>

// Unix includes
#include <strings.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <HIViewWrap.h>
#include <MemoryBlocks.h>

// application includes
#include "UIStrings.h"



#pragma mark Public Methods

/*!
To get a copy of the text in a text field control,
use this method.

(3.0)
*/
void
GetControlText	(ControlRef			inControl,
				 Str255				outText)
{
	if (outText != nullptr)
	{
		Size	actualSize = 0;
		
		
		// obtain the data in the generic buffer
		UNUSED_RETURN(OSStatus)GetControlData(inControl, kControlEditTextPart, kControlEditTextTextTag,
												255 * sizeof(UInt8), (Ptr)(1 + outText), &actualSize);
		((Ptr)(outText))[0] = (char)actualSize; // mush in the length byte
	}
}// GetControlText


/*!
Returns the specified control’s text as a CFString.
You must release the string yourself.

(3.0)
*/
void
GetControlTextAsCFString	(ControlRef		inControl,
							 CFStringRef&	outCFString)
{
	Size	actualSize = 0;
	
	
	outCFString = nullptr; // in case of error
	UNUSED_RETURN(OSStatus)GetControlData(inControl, kControlEditTextPart, kControlEditTextCFStringTag,
											sizeof(outCFString), &outCFString, &actualSize);
}// GetControlTextAsCFString


/*!
Removes the interior of a region so that
it ends up describing only its outline.

(2.6)
*/
void
OutlineRegion	(RgnHandle		inoutRegion)
{
	RgnHandle	tempRgn = NewRgn();
	
	
	if (tempRgn != nullptr)
	{
		CopyRgn(inoutRegion, tempRgn);
		InsetRgn(tempRgn, 1, 1);
		DiffRgn(inoutRegion, tempRgn, inoutRegion);
		DisposeRgn(tempRgn), tempRgn = nullptr;
	}
}// OutlineRegion


/*!
This method uses the SetControlData() API to
change the text of an editable text control.

Use DrawOneControl() to update the control
to reflect the change.

(3.0)
*/
void
SetControlText		(ControlRef			inControl,
					 ConstStringPtr		inText)
{
	Ptr		stringBuffer = nullptr;
	
	
	// create a disposable copy of the given text
	stringBuffer = Memory_NewPtr(PLstrlen(inText) * sizeof(UInt8)); // we skip the length byte!
	BlockMoveData(inText + 1, stringBuffer, GetPtrSize(stringBuffer)); // buffer has no length byte
	
	// set the control text to be the copy that was just created
	UNUSED_RETURN(OSStatus)SetControlData(inControl, kControlEditTextPart, kControlEditTextTextTag,
											GetPtrSize(stringBuffer), stringBuffer);
	
	// destroy the copy
	Memory_DisposePtr(&stringBuffer);
}// SetControlText


/*!
This method uses the SetControlData() API to change
the text of an editable text control using the
contents of a Core Foundation string.

If you pass a nullptr string, the Toolbox will not
accept this, so this routine converts it to an
empty string first.

Use DrawOneControl() to update the control to reflect
the change.

(3.0)
*/
void
SetControlTextWithCFString		(ControlRef		inControl,
								 CFStringRef	inText)
{
	if (inText != nullptr)
	{
		// set the control text
		UNUSED_RETURN(OSStatus)SetControlData(inControl, kControlEditTextPart, kControlEditTextCFStringTag,
												sizeof(inText), &inText);
	}
	else
	{
		CFStringRef		emptyString = CFSTR("");
		
		
		// set the control text to an empty string
		UNUSED_RETURN(OSStatus)SetControlData(inControl, kControlEditTextPart, kControlEditTextCFStringTag,
												sizeof(emptyString), &emptyString);
	}
}// SetControlTextWithCFString


/*!
Sets the current keyboard focus to a specific view.

(3.1)
*/
OSStatus
DialogUtilities_SetKeyboardFocus	(HIViewRef		inView)
{
	OSStatus	result = noErr;
	
	
	result = SetKeyboardFocus(HIViewGetWindow(inView), inView, kControlFocusNoPart);
	if (noErr == result)
	{
		result = SetKeyboardFocus(HIViewGetWindow(inView), inView, kControlFocusNextPart);
	}
	return result;
}// SetKeyboardFocus


/*!
To change the font of the current graphics port
to be the font with the specified name, use this
method.

This routine is present for assisting Carbon
compatibility: Apple does not recommend using
GetFNum(), so the fewer places font IDs are used,
the better.

(3.0)
*/
void
TextFontByName		(ConstStringPtr		inFontName)
{
	SInt16	fontID = FMGetFontFamilyFromName(inFontName);
	
	
	if (fontID != kInvalidFontFamily)
	{
		TextFont(fontID);
	}
}// TextFontByName

// BELOW IS REQUIRED NEWLINE TO END FILE
