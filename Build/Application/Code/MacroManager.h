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
		© 1998-2011 by Kevin Grant.
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

#ifndef __MACROMANAGER__
#define __MACROMANAGER__

// Mac includes
#include <CoreServices/CoreServices.h>
#ifdef __OBJC__
@class NSMenuItem;
#else
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

enum MacroManager_Action
{
	// IMPORTANT: For simplicity, these are made the same as command IDs used to set them in the UI.
	// But, you should use the utility routines below to convert between them anyway.
	kMacroManager_ActionSendTextVerbatim			= kCommandSetMacroActionEnterTextVerbatim,	//!< macro content is a string to send as-is (no
																								//!  metacharacters allowed)
	kMacroManager_ActionSendTextProcessingEscapes	= kCommandSetMacroActionEnterText,			//!< macro content is a string to send (perhaps
																								//!  with metacharacters to be substituted)
	kMacroManager_ActionHandleURL					= kCommandSetMacroActionOpenURL,			//!< macro content is a URL to be opened
	kMacroManager_ActionNewWindowWithCommand		= kCommandSetMacroActionNewWindowCommand	//!< macro content is a Unix command line to be
																								//!  executed in a new terminal window
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

void
	MacroManager_UpdateMenuItem			(NSMenuItem*				inMenuItem,
										 UInt16						inOneBasedMacroIndex,
										 Preferences_ContextRef		inMacroSetOrNullForActiveSet = nullptr);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
