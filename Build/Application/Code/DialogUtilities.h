/*!	\file DialogUtilities.h
	\brief Lists methods that simplify window management,
	dialog box management, and window content control management.
	
	Actually, this module contains a ton of routines that really
	should be part of the Mac OS, but aren’t.
	
	NOTE:	Over time, this header has increasingly become a
			dumping ground for APIs that do not seem to have a
			better place.  This will eventually be fixed.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#ifndef __DIALOGUTILITIES__
#define __DIALOGUTILITIES__

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <HIViewWrap.fwd.h>



#pragma mark Public Methods

void
	GetControlText							(ControlRef				inControl,
											 Str255					outText);

void
	GetControlTextAsCFString				(ControlRef				inControl,
											 CFStringRef&			outCFString);

void
	OutlineRegion							(RgnHandle				inoutRegion);

void
	SetControlText							(ControlRef				inControl,
											 ConstStringPtr			inText);

void
	SetControlTextWithCFString				(ControlRef				inControl,
											 CFStringRef			inText);

OSStatus
	DialogUtilities_SetKeyboardFocus		(HIViewRef				inView);

void
	TextFontByName							(ConstStringPtr			inFontName);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
