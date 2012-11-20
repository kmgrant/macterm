/*!	\file GrowlSupport.h
	\brief Growl Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.9
	© 2008-2012 by Kevin Grant
	
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



#pragma mark Constants

/*!
Since there are differences between Growl and Mac OS X in
terms of flexibility, this setting allows the sender of a
notification to specify how it is intended to be seen by
the user.  In certain situations a notification may
intentionally NOT be sent to Mac OS X simply because it
does not offer the user the same options as Growl to
display the notification in a sensible way.
*/
enum GrowlSupport_NoteDisplay
{
	kGrowlSupport_NoteDisplayAlways = 0,		//!< should be sent to all systems; always appears
	kGrowlSupport_NoteDisplayConfigurable = 1	//!< sent to Growl only, since only Growl offers the
												//!  flexibility to display events in many different
												//!  ways; use this for events that should not trigger
												//!  an immediate display in all cases (as Mac OS X
												//!  will always do)
};



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
	GrowlSupport_Notify							(GrowlSupport_NoteDisplay,
												 CFStringRef,
												 CFStringRef = nullptr,
												 CFStringRef = nullptr,
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
