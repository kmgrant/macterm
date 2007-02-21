/*!	\file HelpSystem.h
	\brief A simple mechanism for managing access to MacTelnet
	Help, especially context-sensitive help for use with things
	like dialog help buttons.
	
	To reduce dependencies on the actual string value of a
	contextual help search, this module usually operates in
	terms of symbolic constants; however, in rare cases where
	the string value is needed (to set the name of a menu item,
	usually), an API is provided.
*/
/*###############################################################

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

#ifndef __HELPSYSTEM__
#define __HELPSYSTEM__

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Constants

enum HelpSystem_ResultCode
{
	kHelpSystem_ResultCodeSuccess				= 0,
	kHelpSystem_ResultCodeNoSuchString			= 1,	//!< tag is invalid
	kHelpSystem_ResultCodeCannotDisplayHelp		= 2,	//!< some problem trying to display help system
	kHelpSystem_ResultCodeCannotRetrieveString	= 3		//!< some error while reading a valid key phrase
};

typedef FourCharCode HelpSystem_KeyPhrase;
enum
{
	// identifiers that trigger actions in MacTelnet Help (usually, opening specific pages)
	kHelpSystem_KeyPhraseDefault		= FOUR_CHAR_CODE('----'), // used to open Ònothing in particularÓ
	kHelpSystem_KeyPhraseCommandLine	= FOUR_CHAR_CODE('CmdL'),
	kHelpSystem_KeyPhraseConnections	= FOUR_CHAR_CODE('Cnxn'),
	kHelpSystem_KeyPhraseDuplicate		= FOUR_CHAR_CODE('Dupl'),
	kHelpSystem_KeyPhraseFavorites		= FOUR_CHAR_CODE('Favr'),
	kHelpSystem_KeyPhraseFind			= FOUR_CHAR_CODE('Find'),
	kHelpSystem_KeyPhraseFormatting		= FOUR_CHAR_CODE('Frmt'),
	kHelpSystem_KeyPhraseKioskSetup		= FOUR_CHAR_CODE('Kios'),
	kHelpSystem_KeyPhraseMacros			= FOUR_CHAR_CODE('Mcro'),
	kHelpSystem_KeyPhrasePreferences	= FOUR_CHAR_CODE('Pref'),
	kHelpSystem_KeyPhraseScreenSize		= FOUR_CHAR_CODE('ScSz'),
	kHelpSystem_KeyPhraseSpecialKeys	= FOUR_CHAR_CODE('Keys'),
	kHelpSystem_KeyPhraseTerminals		= FOUR_CHAR_CODE('Term')
};

#pragma mark Types

/*!
Convenient object that automatically sets the current
key phrase as specified at construction, and restores
to the default at destruction.  Useful in objects.
*/
class HelpSystem_WindowKeyPhraseSetter
{
public:
	inline HelpSystem_WindowKeyPhraseSetter		(WindowRef				inWindow,
												 HelpSystem_KeyPhrase	inKeyPhrase);
	
	inline ~HelpSystem_WindowKeyPhraseSetter	();
	
	//! returns true only if the phrase was set successfully
	inline bool
	isInstalled		() const;

protected:
private:
	HelpSystem_ResultCode	_resultCode;
	WindowRef				_window;
};



#pragma mark Public Methods

//!\name Initialization
//@{

HelpSystem_ResultCode
	HelpSystem_Init							();

void
	HelpSystem_Done							();

//@}

//!\name Displaying Help
//@{

// NOT NORMALLY USED; YOU MIGHT NEED THIS FOR A CONTEXTUAL MENU ITEM
HelpSystem_ResultCode
	HelpSystem_CopyKeyPhraseCFString		(HelpSystem_KeyPhrase		inKeyPhrase,
											 CFStringRef&				outString);

// DEPRECATED - USE HelpSystem_SetCurrentContextKeyPhrase() AND HelpSystem_DisplayHelpInCurrentContext() INSTEAD
HelpSystem_ResultCode
	HelpSystem_DisplayHelpFromKeyPhrase		(HelpSystem_KeyPhrase		inKeyPhrase);

HelpSystem_ResultCode
	HelpSystem_DisplayHelpInCurrentContext	();

HelpSystem_ResultCode
	HelpSystem_DisplayHelpWithoutContext	();

// IMPLICITLY FIGURES OUT THE KEY PHRASE, IF ANY, FOR THE FRONTMOST NON-FLOATING WINDOW
HelpSystem_KeyPhrase
	HelpSystem_GetCurrentContextKeyPhrase	();

// AFFECTS BEHAVIOR OF HelpSystem_GetCurrentContextKeyPhrase() AND HelpSystem_DisplayHelpInCurrentContext()
HelpSystem_ResultCode
	HelpSystem_SetWindowKeyPhrase			(WindowRef					inWindow,
											 HelpSystem_KeyPhrase		inKeyPhrase);

//@}



#pragma mark Inline Methods

HelpSystem_WindowKeyPhraseSetter::
HelpSystem_WindowKeyPhraseSetter	(WindowRef				inWindow,
									 HelpSystem_KeyPhrase	inKeyPhrase)
:
_resultCode(HelpSystem_SetWindowKeyPhrase(inWindow, inKeyPhrase)),
_window(inWindow)
{
}


HelpSystem_WindowKeyPhraseSetter::
~HelpSystem_WindowKeyPhraseSetter ()
{
	(HelpSystem_ResultCode)HelpSystem_SetWindowKeyPhrase(_window, kHelpSystem_KeyPhraseDefault);
}


bool
HelpSystem_WindowKeyPhraseSetter::
isInstalled ()
const
{
	return (kHelpSystem_ResultCodeSuccess == _resultCode);
}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
