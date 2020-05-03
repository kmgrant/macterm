/*!	\file MacroManager.h
	\brief Lists methods to access the strings associated with
	keyboard equivalents.
	
	The MacTerm implementation of macros is very sophisticated.
	There are no practical limits on the number of possible
	macro sets.  Macros can now have actions other than sending
	text; for instance, they can open URLs.  And, they now
	support many more key combinations!
	
	The Preferences module now handles low-level access to
	basic macro information.  Therefore, you can modify, read,
	or monitor macro setting using the now-familiar Preferences
	APIs.
	
	Similarly, the preferences window is the front-end for
	macro editing, so there is no longer a special window
	(however, there is "PrefPanelMacros.cp").
	
	This module provides access to the current macro set,
	automatically triggering all necessary side effects such
	as updating menu key equivalents.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <CoreServices/CoreServices.h>
#ifdef __OBJC__
@class NSMenu;
@class NSMenuItem;
#else
class NSMenu;
class NSMenuItem;
#endif

// library includes
#include <ListenerModel.h>
#include <ResultCode.template.h>

// application includes
#include "Preferences.h"
#include "SessionRef.typedef.h"



#pragma mark Constants

/*!
Possible return values from MacroManager module routines.
*/
typedef ResultCode< UInt16 >	MacroManager_Result;
MacroManager_Result const	kMacroManager_ResultOK(0);					//!< no error
MacroManager_Result const	kMacroManager_ResultGenericFailure(1);		//!< unspecified error occurred

/*!
Possible ways for macros to interpret their content and act on it. 
*/
enum MacroManager_Action
{
	kMacroManager_ActionSendTextVerbatim			= 'MAEV',	//!< macro content is a string to send as-is (no
																//!  metacharacters allowed)
	kMacroManager_ActionSendTextProcessingEscapes	= 'MAET',	//!< macro content is a string to send (perhaps
																//!  with metacharacters to be substituted)
	kMacroManager_ActionHandleURL					= 'MAOU',	//!< macro content is a URL to be opened
	kMacroManager_ActionNewWindowWithCommand		= 'MANW',	//!< macro content is a Unix command line to be
																//!  executed in a new terminal window
	kMacroManager_ActionSelectMatchingWindow		= 'MASW',	//!< macro content is a string to use as a search
																//!  key against the titles of open windows; the
																//!  next window with a matching title is activated
	kMacroManager_ActionFindTextVerbatim			= 'MAFV',	//!< macro content is a string to send as-is (no
																//!  metacharacters allowed)
	kMacroManager_ActionFindTextProcessingEscapes	= 'MAFS'	//!< macro content is a string to send (perhaps
																//!  with metacharacters to be substituted)
};

/*!
Used with MacroManager_StartMonitoring() and MacroManager_StopMonitoring()
to be notified of important changes.
*/
enum MacroManager_Change
{
	kMacroManager_ChangeMacroSetFrom				= 'MMSF',	//!< macro set is about to change (context: old MacroManager_ReturnCurrentMacros())
	kMacroManager_ChangeMacroSetTo					= 'MMST',	//!< macro set has now changed (context: new MacroManager_ReturnCurrentMacros())
};

/*!
Modifier keys that are supported by macros.
*/
typedef UInt32 MacroManager_ModifierKeyMask;
enum
{
	kMacroManager_ModifierKeyMaskCommand	= (1 << 0),		//!< command key (⌘)
	kMacroManager_ModifierKeyMaskControl	= (1 << 1),		//!< control key (⌃)
	kMacroManager_ModifierKeyMaskOption		= (1 << 2),		//!< option key (⌥)
	kMacroManager_ModifierKeyMaskShift		= (1 << 3)		//!< shift key (⇧)
};

UInt16 const kMacroManager_MaximumMacroSetSize = 12;	//!< TEMPORARY: arbitrary upper limit on macro set length, for simplicity in other code

#pragma mark Types

/*!
This is used to identify key equivalents.  A code can be either
a character or a virtual key code, so a flag is attached (in bit
17) to indicate which it is.  The upper 15 bits are currently
not used.

MacroManager_KeyIDIsVirtualKey() and MacroManager_KeyIDKeyCode()
can be used to scan the information encoded in this value, and
MacroManager_MakeKeyID() is convenient for construction.
*/
typedef UInt32				MacroManager_KeyID;
MacroManager_KeyID const	kMacroManager_KeyIDIsVirtualKeyMask = 0x00010000;
MacroManager_KeyID const	kMacroManager_KeyIDKeyCodeMask = 0x0000ffff;



#pragma mark Public Methods

//!\name Managing the Active Macro Set
//@{

Preferences_ContextRef
	MacroManager_ReturnCurrentMacros	();

Preferences_ContextRef
	MacroManager_ReturnDefaultMacros	();

MacroManager_Result
	MacroManager_SetCurrentMacros		(Preferences_ContextRef		inMacroSetOrNullForNone = nullptr);

//@}

//!\name Using Macros
//@{

MacroManager_Result
	MacroManager_UserInputMacro			(UInt16						inZeroBasedMacroIndex,
										 SessionRef					inTargetSessionOrNullForActiveSession = nullptr,
										 Preferences_ContextRef		inMacroSetOrNullForActiveSet = nullptr);

//@}

//!\name Receiving Notification of Changes
//@{

MacroManager_Result
	MacroManager_StartMonitoring		(MacroManager_Change		inForWhatChange,
										 ListenerModel_ListenerRef	inListener);

MacroManager_Result
	MacroManager_StopMonitoring			(MacroManager_Change		inForWhatChange,
										 ListenerModel_ListenerRef	inListener);

//@}

//!\name Updating Menus
//@{

void
	MacroManager_AddContextualMenuGroup	(NSMenu*					inoutContextualMenu,
										 Preferences_ContextRef		inMacroSetOrNullForActiveSet,
										 Boolean					inCheckDefaults);

Boolean
	MacroManager_UpdateMenuItem			(NSMenuItem*				inMenuItem,
										 UInt16						inOneBasedMacroIndex,
										 Boolean					inIsTerminalWindowActive,
										 Preferences_ContextRef		inMacroSetOrNullForActiveSet,
										 Boolean					inCheckDefaults);

//@}

//!\name Utilities
//@{

inline MacroManager_Action
	MacroManager_ActionForCommand		(UInt32						inSetMacroActionCommandID)
	{
		return STATIC_CAST(inSetMacroActionCommandID, MacroManager_Action);
	}

inline UInt32
	MacroManager_CommandForAction		(MacroManager_Action		inAction)
	{
		return STATIC_CAST(inAction, UInt32);
	}

inline Boolean
	MacroManager_KeyIDIsVirtualKey		(MacroManager_KeyID			inKeyID)
	{
		return (0L != (inKeyID & kMacroManager_KeyIDIsVirtualKeyMask));
	}

inline UInt16
	MacroManager_KeyIDKeyCode			(MacroManager_KeyID			inKeyID)
	{
		return (inKeyID & kMacroManager_KeyIDKeyCodeMask);
	}

inline MacroManager_KeyID
	MacroManager_MakeKeyID				(Boolean					inIsVirtualKey,
										 UInt16						inKeyCode)
	{
		MacroManager_KeyID		result = inKeyCode;
		MacroManager_KeyID		flagBit = inIsVirtualKey;
		
		
		flagBit <<= 16;
		result |= flagBit;
		return result;
	}

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
