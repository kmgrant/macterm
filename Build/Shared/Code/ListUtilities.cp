/*###############################################################

	ListUtilities.cp
	
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
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <ListUtilities.h>



#pragma mark Public Methods

/*!
To get a standard ListHandle representation
of an Appearance list box control, use this
method.  If any problems occur, the output
list will be nullptr.

(1.0)
*/
void
ListUtilities_GetControlListHandle		(ControlRef			inListBoxControl,
										 ListHandle*		outListHandle)
{
	Size	actualSize = 0;
	
	
	if (GetControlData(inListBoxControl, kControlListBoxPart, kControlListBoxListHandleTag, sizeof(ListHandle),
							(Ptr)outListHandle, &actualSize) != noErr) *outListHandle = nullptr;
}// GetControlListHandle


/*!
To get a standard ListHandle representation
of an Appearance list box control that is a
dialog box item, use this method.  If any
problems occur, the output list will be
nullptr.

(1.0)
*/
void
ListUtilities_GetDialogItemListHandle		(DialogRef			inDialog,
											 DialogItemIndex	inItemIndex,
											 ListHandle*		outListHandle)
{
	ControlRef		listControl = nullptr;
	
	
	if (GetDialogItemAsControl(inDialog, inItemIndex, &listControl) == noErr)
	{
		ListUtilities_GetControlListHandle(listControl, outListHandle);
	}
	else *outListHandle = nullptr;
}// GetDialogItemListHandle


/*!
If a list contains Pascal string items (as
standard lists do), you can use this method
to obtain the list data as a string.  The
first column is assumed.

(3.0)
*/
void
ListUtilities_GetListItemText		(ListHandle		inListHandle,
									 UInt16			inZeroBasedItemIndex,
									 Str255			outItemText)
{
	Cell		cell;
	SInt16		length = 254; // on input, maximum possible length
	
	
	SetPt(&cell, 0, inZeroBasedItemIndex);
	LGetCell(outItemText + 1, &length, cell, inListHandle);
	outItemText[0] = (char)length;
}// GetListItemText


/*!
To obtain a unique variation of the specified
item text, guaranteeing that it will not be
the identical text of any item in the specified
list, use this method.

WARNING:	Do not use this method unless your
			list contains Pascal string data
			in its items (standard lists do).

(3.0)
*/
void
ListUtilities_GetUniqueItemText		(ListHandle		inListHandle,
									 Str255			inoutItemText)
{
	// only change the specified item text if itÕs already ÒtakenÓ by an item in the menu
	while (!ListUtilities_IsItemUnique(inListHandle, inoutItemText))
	{
		if ((inoutItemText[inoutItemText[0]] > '9') ||
			(inoutItemText[inoutItemText[0]] < '0')) // add a number
		{
			inoutItemText[++inoutItemText[0]] = ' ';
			inoutItemText[++inoutItemText[0]] = '1';
		}
		else if (inoutItemText[inoutItemText[0]] == '9') // add another digit
		{
			inoutItemText[inoutItemText[0]] = '-';
			inoutItemText[++inoutItemText[0]] = '1';
		}
		else inoutItemText[inoutItemText[0]]++; // increment the number
	}
}// GetUniqueItemText


/*!
To determine if a list contains no item with
text identical to the specified item text,
use this method.

WARNING:	Do not use this method unless your
			list contains Pascal string data
			in its items (standard lists do).

(3.0)
*/
Boolean
ListUtilities_IsItemUnique		(ListHandle		inListHandle,
								 Str255			inItemText)
{
	Boolean		result = true;
	UInt16		itemCount = ListUtilities_ReturnItemCount(inListHandle),
				i = 0;
	Str255		itemString;
	
	
	// look for an identical item
	for (i = 0; ((result) && (i < itemCount)); i++)
	{
		ListUtilities_GetListItemText(inListHandle, i, itemString);
		if (!PLstrcmp(itemString, inItemText)) result = false;
	}
	
	return result;
}// IsItemUnique


/*!
To determine the total number of rows in a
list, use this method.

(1.0)
*/
UInt16
ListUtilities_ReturnItemCount	(ListHandle		inListHandle)
{
	UInt16		result = 0;
	
	
	if (inListHandle != nullptr)
	{
		Cell		junk;
		Boolean		searchRows = true,
					searchColumns = false;
		
		
		SetPt(&junk, 0, -1);
		while (LNextCell(searchColumns, searchRows, &junk, inListHandle)) result++;
	}
	
	return result;
}// ReturnItemCount


/*!
To determine the total number of selected
cells in a list, use this method.

(1.0)
*/
UInt16
ListUtilities_ReturnSelectedItemCount	(ListHandle		inListHandle)
{
	UInt16		result = 0;
	
	
	if (inListHandle != nullptr)
	{
		Cell		junk;
		UInt16		total = ListUtilities_ReturnItemCount(inListHandle);
		
		
		SetPt(&junk, 0, 0);
		while (LGetSelect(true/* next */, &junk, inListHandle) && (result <= total)) result++, junk.v++;
	}
	
	return result;
}// ReturnSelectedItemCount


/*!
If a list contains Pascal string items (as
standard lists do), you can use this method
to specify the list data as a string.

See also ListUtilities_SetListItemText().

(3.0)
*/
void
ListUtilities_SetListCellText   (ListHandle			inListHandle,
								 UInt16				inZeroBasedRowIndex,
								 UInt16				inZeroBasedColumnIndex,
								 ConstStringPtr		inItemText)
{
	Cell	cell;
	
	
	SetPt(&cell, inZeroBasedColumnIndex, inZeroBasedRowIndex);
	LSetCell((Ptr)(inItemText + 1), PLstrlen(inItemText), cell, inListHandle);
}// SetListCellText


/*!
If a list contains Pascal string items (as
standard lists do), you can use this method
to specify the list data as a string.  The
first column is assumed.

(3.0)
*/
void
ListUtilities_SetListItemText   (ListHandle			inListHandle,
								 UInt16				inZeroBasedItemIndex,
								 ConstStringPtr		inItemText)
{
	Cell	cell;
	
	
	SetPt(&cell, 0, inZeroBasedItemIndex);
	LSetCell((Ptr)(inItemText + 1), PLstrlen(inItemText), cell, inListHandle);
}// SetListItemText


/*!
Ensures the size of the list for the specified
list box control matches the control size,
accounting for any scroll bars the list has.

(3.0)
*/
void
ListUtilities_SynchronizeListBoundsWithControlBounds	(ControlRef		inListBoxControl)
{
	ListHandle		list = nullptr;
	SInt32			scrollBarWidth = 16;
	Rect			bounds;
	
	
	ListUtilities_GetControlListHandle(inListBoxControl, &list);
	GetControlBounds(inListBoxControl, &bounds);
#if TARGET_API_MAC_CARBON
	if (noErr != GetThemeMetric(kThemeMetricScrollBarWidth, &scrollBarWidth))
	{
		scrollBarWidth = 16;
	}
#endif
	if (GetListVerticalScrollBar(list) != nullptr) bounds.right -= scrollBarWidth;
	if (GetListHorizontalScrollBar(list) != nullptr) bounds.bottom -= scrollBarWidth;
	LSize(bounds.right - bounds.left, bounds.bottom - bounds.top, list);
}// SynchronizeListBoundsWithControlBounds

// BELOW IS REQUIRED NEWLINE TO END FILE
