/*!	\file Terminology.h
	\brief Four-character codes for the AppleScript Dictionary.
	
	This file contains Apple Events and corresponding classes
	that MacTerm recognizes (for scriptability).
	
	This file is obsolete, as AppleScript support has been
	largely removed.
	
	\attention
	Changes to this file should be reflected in the scripting
	definition (currently "Dictionary.sdef", which is compiled
	into Rez input for an 'aete' resource), and vice-versa.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#ifndef __TERMINOLOGY__
#define __TERMINOLOGY__

// application includes
#include "ConstantsRegistry.h"



#pragma mark Constants

// suite types
#define kASRequiredSuite											'aevt'	// a.k.a. kCoreEventClass

/*###########################################
	URL Suite
###########################################*/

#define kStandardURLSuite											'GURL'

// implemented / extended events

// event "handle URL"
#define kStandardURLEventIDGetURL									'GURL'

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
