/*!	\file ContextSensitiveMenu.h
	\brief Simplifies handling of contextual menus.
	
	This module creates contextual menus by accumulating
	information about all possible items that belong in a
	menu at a given time, and context routines to decide
	which ones actually appear.
	
	Grouping is a convenient way to insert dividers free
	of headaches like tail dividers and double-dividers.
	All groups with at least one visible item are
	automatically delimited by dividers, and no other
	dividers are inserted.
*/
/*###############################################################

	Contexts Library 2.0
	© 1998-2011 by Kevin Grant
	
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

#include <UniversalDefines.h>

#ifndef __CONTEXTSENSITIVEMENU__
#define __CONTEXTSENSITIVEMENU__

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Constants

enum
{
	kContextSensitiveMenu_DefaultMenuID = 230		// reserved menu ID used for contextual menus in this module
};

#pragma mark Types

struct ContextSensitiveMenu_Item
{
	UInt32			commandID;
	CFStringRef		commandText;
};
typedef ContextSensitiveMenu_Item const*	ContextSensitiveMenu_ItemConstPtr;
typedef ContextSensitiveMenu_Item*			ContextSensitiveMenu_ItemPtr;

#pragma mark Callbacks

/*!
Context Verifier Method

This method gets called by ContextualMenus_AddProvisionally() to
see if a particular item should be added based on the given context-
setting parameters.  Return "true" if the item should be added, or
"false" if it should not be.

The event record provided is a snapshot of the most recent event
that set up the context; this information is almost always needed.
The user data is completely custom, and is a mechanism for allowing
the caller of this arbitration method to specify clues as to the
context that the method should consider.  The command information
parameter is useful to implement variation codes, so you can write
a function that discriminates similar contexts and behaves in certain
ways based on the code provided.

Although this callback is currently used for contextual menus, it
is meant to be flexible enough to handle any generic context
specification.  Therefore, if possible, try to write context
discriminators in such a way as to consider menu items as “commands”
which could represent any invocation method.
*/
typedef Boolean (*ContextSensitiveMenu_VerifierProcPtr)	(EventRecord*		inoutEventPtr,
														 void*				inoutUserData,
														 UInt32				inCommandInfo);
inline Boolean
ContextSensitiveMenu_InvokeContextVerifierProc	(ContextSensitiveMenu_VerifierProcPtr	inUserRoutine,
												 EventRecord*							inoutEventPtr,
												 void*									inoutUserData,
												 UInt32									inCommandInfo)
{
	return (*inUserRoutine)(inoutEventPtr, inoutUserData, inCommandInfo);
}



#pragma mark Public Methods

//!\name Creating, Initializing and Destroying Contextual Menus
//@{

MenuRef
	ContextSensitiveMenu_New					(SInt16*								outMenuIDOrNull);

void
	ContextSensitiveMenu_Dispose				(MenuRef*								inoutDeadMenu);

void
	ContextSensitiveMenu_InitItem				(ContextSensitiveMenu_ItemPtr			inoutItem);

//@}

//!\name Adding Items to Contextual Menus
//@{

OSStatus
	ContextSensitiveMenu_AddItem				(MenuRef								inToWhichMenu,
												 ContextSensitiveMenu_ItemConstPtr		inItemInfo);

OSStatus
	ContextSensitiveMenu_AddItemProvisionally	(MenuRef								inToWhichMenu,
												 ContextSensitiveMenu_ItemConstPtr		inItemInfo,
												 ContextSensitiveMenu_VerifierProcPtr	inArbiter,
												 EventRecord*							inoutEventPtr,
												 void*									inoutUserDataPtr);

void
	ContextSensitiveMenu_DoneAddingItems		(MenuRef								inToWhichMenu);

// USE THIS METHOD EVERY TIME A DIVIDING LINE *MAY* BE NECESSARY (AVOIDS ADDING MORE THAN ONE DIVIDER)
void
	ContextSensitiveMenu_NewItemGroup			(MenuRef								inForWhichMenu);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
