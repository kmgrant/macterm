/*###############################################################

	ContextSensitiveMenu.cp
	
	Contexts Library 2.0
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
#include "ContextSensitiveMenu.h"



#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	UInt16		gItemCountAddedGroup = 0;
	UInt16		gItemCountAddedTotal = 0;
}

#pragma mark Internal Method Prototypes

static UInt16		getGroupAddedItemCount		();
static UInt16		getTotalAddedItemCount		();
static void			setGroupAddedItemCount		(UInt16);
static void			setTotalAddedItemCount		(UInt16);



#pragma mark Public Methods

/*!
To create a new, unique menu to use as a contextual menu,
call this routine.  The new menu, if successfully created,
will already have been added to the hierarchical menu list
(the required location for special menus like contextual
menus).  If no valid ID can be found, or there is no
memory, or some other problem occurs, nullptr is returned.

The menu ID (not a resource ID) of the new menu is returned
in "outMenuIDPtrOrNull", if not nullptr.

IMPORTANT:	Before displaying the new menu, be sure to
			invoke the ContextSensitiveMenu_DoneAddingItems()
			method.

(1.0)
*/
MenuRef
ContextSensitiveMenu_New	(SInt16*	outMenuIDPtrOrNull)
{
	MenuRef		result = nullptr;
	
	
	// reinitialize globals
	setGroupAddedItemCount(0);
	setTotalAddedItemCount(0);
	
	if (nullptr != outMenuIDPtrOrNull) *outMenuIDPtrOrNull = 0;
	
	if (noErr == CreateNewMenu(kContextSensitiveMenu_DefaultMenuID, 0/* attributes */, &result))
	{
		InsertMenu(result, kInsertHierarchicalMenu);
		if (nullptr != outMenuIDPtrOrNull) *outMenuIDPtrOrNull = kContextSensitiveMenu_DefaultMenuID;
	}
	return result;
}// New


/*!
To destroy a menu created with ContextSensitiveMenu_New(),
call this method.

(1.0)
*/
void
ContextSensitiveMenu_Dispose	(MenuRef*	inoutDeadMenu)
{
	if (nullptr != inoutDeadMenu)
	{
		DeleteMenu(kContextSensitiveMenu_DefaultMenuID);
		DisposeMenu(*inoutDeadMenu), *inoutDeadMenu = nullptr;
	}
}// Dispose


/*!
Unconditionally adds an item to a contextual menu.

The information in the specified structure is copied,
so you can free or reuse your original copy for
multiple items (for instance).

If "inItemInfo" is nullptr, a dividing line is inserted.
Note, however, that ContextSensitiveMenu_NewItemGroup()
is often preferable because it guards against multiple
dividers and other aesthetic problems.

Returns anything that may be returned by
ContextSensitiveMenu_AddItemProvisionally().

(1.0)
*/
OSStatus
ContextSensitiveMenu_AddItem	(MenuRef							inToWhichMenu,
								 ContextSensitiveMenu_ItemConstPtr	inItemInfo)
{
	return ContextSensitiveMenu_AddItemProvisionally(inToWhichMenu, inItemInfo, nullptr/* arbiter */,
														nullptr/* event */, nullptr/* context */, 0L/* command */);
}// AddItem


/*!
Adds an item to a contextual menu only if the given
callback allows it.

The information in the specified structure is copied,
so you can free or reuse your original copy for
multiple items (for instance).

If "inItemInfo" is nullptr, a dividing line is inserted.
Note, however, that ContextSensitiveMenu_NewItemGroup()
is often preferable because it guards against multiple
dividers and other aesthetic problems.

The last four arguments indicate respectively the
function that is the context arbiter, and the parameters
to pass to it.  If you pass nullptr values for these, the
item is simply inserted into the menu without regard
for any context.  (The ContextSensitiveMenu_AddItem()
routine is a simpler way to achieve that.)

\retval noErr
if the menu item was added successfully

\retval reqFailed
if the menu item is rejected in the current context

\retval (other)
if an unusual error (like lack of memory) occurs

(1.0)
*/
OSStatus
ContextSensitiveMenu_AddItemProvisionally	(MenuRef								inToWhichMenu,
											 ContextSensitiveMenu_ItemConstPtr		inItemInfo,
											 ContextSensitiveMenu_VerifierProcPtr	inArbiter,
											 EventRecord*							inoutEventPtr,
											 void*									inoutUserDataPtr,
											 UInt32									inCommandInfo)
{
	enum
	{
		kEndOfContextualMenu = 999
	};
	OSStatus	result = noErr;
	Boolean		doInsert = false;
	
	
	if (nullptr == inToWhichMenu) result = memPCErr;
	
	doInsert = (noErr == result);
	if (doInsert)
	{
		if (nullptr != inArbiter)
		{
			doInsert = ContextSensitiveMenu_InvokeContextVerifierProc
						(inArbiter, inoutEventPtr, inoutUserDataPtr, inCommandInfo);
			if (!doInsert) result = reqFailed;
		}
		else doInsert = true;
		
		if (doInsert)
		{
			if (nullptr != inItemInfo)
			{
				UInt16		insertedItemNumber = 0;
				
				
				{
					UInt16 const	kGroupAddedItemCount = getGroupAddedItemCount();
					UInt16 const	kTotalAddedItemCount = getTotalAddedItemCount();
					
					
					// update the count of contextual items added to this menu
					if ((kTotalAddedItemCount) && (!kGroupAddedItemCount))
					{
						// if any items are preceding this one and it’s the first in its group, add a divider automatically
						(OSStatus)InsertMenuItemTextWithCFString(inToWhichMenu, CFSTR(""), kEndOfContextualMenu,
																	kMenuItemAttrSeparator, 0/* command ID */);
					}
					
					// increment counters for next time (group counter resets via a call to ContextSensitiveMenu_NewItemGroup())
					setGroupAddedItemCount(kGroupAddedItemCount + 1);
					setTotalAddedItemCount(kTotalAddedItemCount + 1);
				}
				
				// insert an empty string and then change the text, to avoid Menu Manager interpolation
				// (NOTE: this was true with older APIs; now that modern Menu Manager APIs are used, is this needed?)
				(OSStatus)InsertMenuItemTextWithCFString(inToWhichMenu, CFSTR("<item>"), kEndOfContextualMenu,
															0/* attributes */, 0/* command ID */);
				insertedItemNumber = CountMenuItems(inToWhichMenu);
				SetMenuItemTextWithCFString(inToWhichMenu, insertedItemNumber, inItemInfo->commandText);
				result = SetMenuItemCommandID(inToWhichMenu, insertedItemNumber, inItemInfo->commandID);
			}
			else
			{
				// add a divider
				(OSStatus)InsertMenuItemTextWithCFString(inToWhichMenu, CFSTR(""), kEndOfContextualMenu,
															kMenuItemAttrSeparator, 0/* command ID */);
			}
		}
	}
	return result;
}// AddItemProvisionally


/*!
When you are finished adding items to a menu, you MUST
call this routine to clean up.

(1.0)
*/
void
ContextSensitiveMenu_DoneAddingItems	(MenuRef	UNUSED_ARGUMENT(inWhichMenu))
{
	// okay, this routine doesn’t do anything - but forcing a completion routine increases flexibility for the future
}// DoneAddingItems


/*!
Initializes a contextual menu item structure.  You should
always invoke this routine on any ContextSensitiveMenu_Item
structure you allocate, to ensure no fields are uninitialized.

(1.0)
*/
void
ContextSensitiveMenu_InitItem	(ContextSensitiveMenu_ItemPtr	inoutItem)
{
	inoutItem->commandID = 0L;
	inoutItem->commandText = CFSTR("");
}// InitItem


/*!
To mark subsequent calls to ContextSensitiveMenu_AddItem()
or ContextSensitiveMenu_AddItemProvisionally() as belonging
to a new group of related items, use this method.  The
Context-Sensitive Menu module will automatically add only
as many dividing lines as are necessary.  So, if no items
from a particular group end up getting added, no dividing
line for that group is inserted.  This is the easiest way
to manage dividing lines in your contextual menus.

(1.0)
*/
void
ContextSensitiveMenu_NewItemGroup	(MenuRef	UNUSED_ARGUMENT(inForWhichMenu))
{
	setGroupAddedItemCount(0);
}// NewItemGroup


#pragma mark Internal Methods

/*!
To determine the number of items from the
current group added to a contextual menu,
use this method.

(1.0)
*/
static inline UInt16
getGroupAddedItemCount ()
{
	return gItemCountAddedGroup;
}// getGroupAddedItemCount


/*!
To determine the total number of items
added to a contextual menu, use this
method.

(1.0)
*/
static inline UInt16
getTotalAddedItemCount ()
{
	return gItemCountAddedTotal;
}// getTotalAddedItemCount


/*!
To specify the number of items from the
current group added to a contextual menu,
use this method.

(1.0)
*/
static inline void
setGroupAddedItemCount		(UInt16		inCount)
{
	gItemCountAddedGroup = inCount;
}// setGroupAddedItemCount


/*!
To specify the number of items added to a
contextual menu, use this method.

(1.0)
*/
static inline void
setTotalAddedItemCount		(UInt16		inCount)
{
	gItemCountAddedTotal = inCount;
}// setTotalAddedItemCount

// BELOW IS REQUIRED NEWLINE TO END FILE
