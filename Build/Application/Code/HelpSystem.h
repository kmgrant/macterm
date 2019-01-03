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
		© 1998-2019 by Kevin Grant.
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

// library includes
#include <ResultCode.template.h>



#pragma mark Constants

typedef ResultCode< UInt16 >	HelpSystem_Result;
HelpSystem_Result const		kHelpSystem_ResultOK(0);					//!< no error
HelpSystem_Result const		kHelpSystem_ResultParameterError(1);		//!< bad input - for example, nullptr string
HelpSystem_Result const		kHelpSystem_ResultCannotFindHelpBook(2);	//!< unable to determine help book name


#pragma mark Public Methods

//!\name Displaying Help
//@{

HelpSystem_Result
	HelpSystem_DisplayHelpWithoutContext		();

HelpSystem_Result
	HelpSystem_DisplayHelpWithSearch			(CFStringRef);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
