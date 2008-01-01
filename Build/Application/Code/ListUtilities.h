/*!	\file ListUtilities.h
	\brief Convenient APIs for List Manager lists.
	
	Just because Apple made it a pain to deal with lists
	doesn’t mean you have to burden that pain.  Use this
	module to handle a number of common list-handling
	needs, such as extracting lists from list box
	controls, and counting items.
*/
/*###############################################################

	Interface Library 1.3
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

#ifndef __LISTUTILITIES__
#define __LISTUTILITIES__



// Mac includes
#include <Carbon/Carbon.h>



/*###############################################################
	OBTAINING LIST STRUCTURES FROM APPEARANCE CONTROLS
###############################################################*/

void
	ListUtilities_GetControlListHandle		(ControlRef					inListBoxControl,
											 ListHandle*				outListHandle);

void
	ListUtilities_GetDialogItemListHandle	(DialogRef					inDialog,
											 DialogItemIndex			inItemIndex,
											 ListHandle*				outListHandle);

/*###############################################################
	MANAGING ITEMS IN LISTS
###############################################################*/

UInt16
	ListUtilities_ReturnItemCount			(ListHandle					inListHandle);

UInt16
	ListUtilities_ReturnSelectedItemCount	(ListHandle					inListHandle);

/*###############################################################
	MANAGING LISTS WITH PASCAL STRING ITEM DATA
###############################################################*/

// ASSUMES FIRST COLUMN
void
	ListUtilities_GetListItemText			(ListHandle					inListHandle,
											 UInt16						inZeroBasedItemIndex,
											 Str255						outItemText);

void
	ListUtilities_GetUniqueItemText			(ListHandle					inListHandle,
											 Str255						inoutItemText);

Boolean
	ListUtilities_IsItemUnique				(ListHandle					inListHandle,
											 Str255						inItemText);

void
	ListUtilities_SetListCellText			(ListHandle					inListHandle,
											 UInt16						inZeroBasedRowIndex,
											 UInt16						inZeroBasedColumnIndex,
											 ConstStringPtr				inItemText);

// ASSUMES FIRST COLUMN
void
	ListUtilities_SetListItemText			(ListHandle					inListHandle,
											 UInt16						inZeroBasedItemIndex,
											 ConstStringPtr				inItemText);

/*###############################################################
	MISCELLANEOUS
###############################################################*/

void
	ListUtilities_SynchronizeListBoundsWithControlBounds		(ControlRef		inListBoxControl);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
