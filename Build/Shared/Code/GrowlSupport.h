/*!	\file GrowlSupport.h
	\brief Growl Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.9
	© 2008-2011 by Kevin Grant
	
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

#ifndef __GROWLSUPPORT__
#define __GROWLSUPPORT__

// Mac includes
#include <CoreFoundation/CoreFoundation.h>



#pragma mark Public Methods

//!\name Growl Setup
//@{

void
	GrowlSupport_Init							();

Boolean
	GrowlSupport_IsAvailable					();

//@}

//!\name Notifications
//@{

void
	GrowlSupport_Notify							(CFStringRef,
												 CFStringRef = nullptr,
												 CFStringRef = nullptr);

//@}

//!\name User Interface Utilities
//@{

Boolean
	GrowlSupport_PreferencesPaneCanDisplay		();

void
	GrowlSupport_PreferencesPaneDisplay			();

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
