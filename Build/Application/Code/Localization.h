/*!	\file Localization.h
	\brief Helps to make this program easier to translate into
	other languages and cultures.
	
	All Localization module utility functions work automatically
	based on your specified text reading directions.  There are
	all kinds of APIs for conveniently arranging sets of controls
	according to certain constraints.
*/
/*###############################################################

	Interface Library 2.6
	© 1998-2020 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be
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

#pragma once

// Mac includes
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#ifdef __OBJC__
@class NSButton;
#else
class NSButton;
#endif


#pragma mark Constants

/*!
The following constants are used as a rough approximation
of the interface guidelines for cases where controls are
arranged programmatically.
*/
enum
{
	// standard heights in pixels for buttons of various kinds
	BUTTON_HT						= 20,
	BUTTON_HT_SMALL					= 17,
	
	// amount of space required between a button’s title and its sides
	MINIMUM_BUTTON_TITLE_CUSHION	= 24,
	
	// standard distances in pixels between buttons and their surroundings
	HSP_BUTTONS						= 12,
	HSP_BUTTON_AND_DIALOG			= 24,
	VSP_BUTTON_AND_DIALOG			= 20,
	VSP_BUTTON_SMALL_AND_DIALOG		= 14,
};


#pragma mark Public Methods

//!\name Utilities for View Manipulation
//@{

void
	Localization_AdjustHelpNSButton				(NSButton*					inHelpButton);

void
	Localization_ArrangeNSButtonArray			(CFArrayRef					inNSButtonArray);

CGFloat
	Localization_AutoSizeNSButton				(NSButton*					inButton,
												 CGFloat					inMinimumWidth = 92.0/* includes spacing */,
												 Boolean					inResize = true);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
