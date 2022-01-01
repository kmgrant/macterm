/*!	\file HelpSystem.mm
	\brief A simple mechanism for managing access to the
	application’s user documentation and context-sensitive
	help.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

#import "HelpSystem.h"
#import <UniversalDefines.h>

// Mac includes
@import Cocoa;
@import CoreServices;

// application includes
#import "UIStrings.h"


#pragma mark Public Methods

/*!
Displays the main table of contents of the help system;
that is, help content without any contextual focus.

\retval kHelpSystem_ResultOK
always (failure is not detected)

(3.0)
*/
HelpSystem_Result
HelpSystem_DisplayHelpWithoutContext ()
{
	HelpSystem_Result	result = kHelpSystem_ResultCannotFindHelpBook;
	NSString*			bookID = [[NSBundle mainBundle] objectForInfoDictionaryKey: @"CFBundleHelpBookName"];
	
	
	if (nil != bookID)
	{
		[[NSHelpManager sharedHelpManager] openHelpAnchor:@"" inBook:bookID];
		result = kHelpSystem_ResultOK;
	}
	
	return result;
}// DisplayHelpWithoutContext


/*!
Displays help pages that match the given string, if a search
index has been successfully constructed for the content.

\retval kHelpSystem_ResultOK
if request to display help can be made (failure is not detected)

\retval kHelpSystem_ResultParameterError
if "inQueryString" is nullptr

\retval kHelpSystem_ResultCannotFindHelpBook
if default help book name cannot be determined

(2018.10)
*/
HelpSystem_Result
HelpSystem_DisplayHelpWithSearch	(CFStringRef	inQueryString)
{
	HelpSystem_Result	result = kHelpSystem_ResultParameterError;
	
	
	if (nullptr != inQueryString)
	{
		NSString*		bookID = [[NSBundle mainBundle] objectForInfoDictionaryKey: @"CFBundleHelpBookName"];
		
		
		if (nil == bookID)
		{
			result = kHelpSystem_ResultCannotFindHelpBook;
		}
		else
		{
			[[NSHelpManager sharedHelpManager] findString:BRIDGE_CAST(inQueryString, NSString*) inBook:bookID];
			result = kHelpSystem_ResultOK;
		}
	}
	
	return result;
}// DisplayHelpWithSearch

// BELOW IS REQUIRED NEWLINE TO END FILE
