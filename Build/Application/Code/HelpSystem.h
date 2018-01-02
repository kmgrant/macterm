/*!	\file HelpSystem.h
	\brief A simple mechanism for managing access to the
	application’s user documentation and context-sensitive
	help.
	
	To reduce dependencies on the actual string value of a
	contextual help search, this module usually operates in
	terms of symbolic constants; however, in rare cases where
	the string value is needed (to set the name of a menu item,
	usually), an API is provided.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Constants

enum HelpSystem_Result
{
	kHelpSystem_ResultOK					= 0,
	kHelpSystem_ResultNoSuchString			= 1,	//!< tag is invalid
	kHelpSystem_ResultCannotDisplayHelp		= 2,	//!< some problem trying to display help system
	kHelpSystem_ResultCannotRetrieveString	= 3		//!< some error while reading a valid key phrase
};

typedef FourCharCode HelpSystem_KeyPhrase;
enum
{
	// identifiers that trigger actions in MacTerm Help (usually, opening specific pages)
	kHelpSystem_KeyPhraseDefault		= '----', // used to open “nothing in particular”
	kHelpSystem_KeyPhraseCommandLine	= 'CmdL',
	kHelpSystem_KeyPhraseConnections	= 'Cnxn',
	kHelpSystem_KeyPhraseDuplicate		= 'Dupl',
	kHelpSystem_KeyPhraseFavorites		= 'Favr',
	kHelpSystem_KeyPhraseFind			= 'Find',
	kHelpSystem_KeyPhraseFormatting		= 'Frmt',
	kHelpSystem_KeyPhraseIPAddresses	= 'IPAd',
	kHelpSystem_KeyPhraseKioskSetup		= 'Kios',
	kHelpSystem_KeyPhraseMacros			= 'Mcro',
	kHelpSystem_KeyPhrasePreferences	= 'Pref',
	kHelpSystem_KeyPhraseScreenSize		= 'ScSz',
	kHelpSystem_KeyPhraseSpecialKeys	= 'Keys',
	kHelpSystem_KeyPhraseTerminals		= 'Term'
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
	HelpSystem_Result	_resultCode;
	WindowRef			_window;
};



#pragma mark Public Methods

//!\name Initialization
//@{

HelpSystem_Result
	HelpSystem_Init								();

void
	HelpSystem_Done								();

//@}

//!\name Displaying Help
//@{

// NOT NORMALLY USED; YOU MIGHT NEED THIS FOR A CONTEXTUAL MENU ITEM
HelpSystem_Result
	HelpSystem_CopyKeyPhraseCFString			(HelpSystem_KeyPhrase		inKeyPhrase,
												 CFStringRef&				outString);

// DEPRECATED - USE HelpSystem_SetCurrentContextKeyPhrase() AND HelpSystem_DisplayHelpInCurrentContext() INSTEAD
HelpSystem_Result
	HelpSystem_DisplayHelpFromKeyPhrase			(HelpSystem_KeyPhrase		inKeyPhrase);

HelpSystem_Result
	HelpSystem_DisplayHelpInCurrentContext		();

HelpSystem_Result
	HelpSystem_DisplayHelpWithoutContext		();

// IMPLICITLY FIGURES OUT THE KEY PHRASE, IF ANY, FOR THE FRONTMOST NON-FLOATING WINDOW
HelpSystem_KeyPhrase
	HelpSystem_ReturnCurrentContextKeyPhrase	();

// AFFECTS BEHAVIOR OF HelpSystem_ReturnCurrentContextKeyPhrase() AND HelpSystem_DisplayHelpInCurrentContext()
HelpSystem_Result
	HelpSystem_SetWindowKeyPhrase				(WindowRef					inWindow,
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
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_SetWindowKeyPhrase(_window, kHelpSystem_KeyPhraseDefault);
}


bool
HelpSystem_WindowKeyPhraseSetter::
isInstalled ()
const
{
	return (kHelpSystem_ResultOK == _resultCode);
}

// BELOW IS REQUIRED NEWLINE TO END FILE
