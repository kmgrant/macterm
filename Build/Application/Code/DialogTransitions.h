/*!	\file DialogTransitions.h
	\brief Routines to simplify and standardize how dialogs
	are opened and closed.
	
	A user interface should be obvious; that is one of the
	founding principles of this module.  Dialog boxes, whenever
	possible, appear to spawn from specific onscreen objects,
	to make it clear what they are for.  Similarly, they close
	in a way that describes what is happening to the contents
	of the dialog (a Cancel sends a dialog to the Trash, but
	an OK sends it back where it came from).
	
	These effects are achieved largely with zoom rectangles,
	but on Mac OS X this module may one day be enhanced to do
	much more interesting things!
*/
/*###############################################################

	MacTerm
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

#ifndef __DIALOGTRANSITIONS__
#define __DIALOGTRANSITIONS__

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

void
	DialogTransitions_Close						(WindowRef				inWindow);

void
	DialogTransitions_CloseImmediately			(WindowRef				inWindow);

/*!
Convenience function for choosing an effect appropriate
for cancellation of a dialog.
*/
#define	DialogTransitions_CloseCancel(d)		DialogTransitions_CloseLowerRight(d)

void
	DialogTransitions_CloseLowerRight			(WindowRef				inWindow);

void
	DialogTransitions_CloseSheet				(WindowRef				inWindow,
												 WindowRef				inParentWindow);

void
	DialogTransitions_CloseSheetImmediately		(WindowRef				inWindow,
												 WindowRef				inParentWindow);
void
	DialogTransitions_CloseSheetToRectangle		(WindowRef				inWindow,
												 WindowRef				inParentWindow,
												 Rect const*			inToWhere);

void
	DialogTransitions_CloseToControl			(WindowRef				inWindow,
												 ControlRef				inToWhat);

void
	DialogTransitions_CloseToRectangle			(WindowRef				inWindow,
												 WindowRef				inParentWindow,
												 Rect const*			inToWhere);

void
	DialogTransitions_CloseToWindowRegion		(WindowRef				inWindow,
												 WindowRef				inParentWindow,
												 WindowRegionCode		inToWhere);

void
	DialogTransitions_CloseUpperLeft			(WindowRef				inWindow);

void
	DialogTransitions_Display					(WindowRef				inWindow);

void
	DialogTransitions_DisplayFromControl		(WindowRef				inWindow,
												 ControlRef				inFromWhat);

void
	DialogTransitions_DisplayFromRectangle		(WindowRef				inWindow,
												 WindowRef				inParentWindow,
												 Rect const*			inFromWhere);

void
	DialogTransitions_DisplayFromWindowRegion	(WindowRef				inWindow,
												 WindowRef				inParentWindow,
												 WindowRegionCode		inFromWhere);

void
	DialogTransitions_DisplaySheet				(WindowRef				inWindow,
												 WindowRef				inParentWindow,
												 Boolean				inIsOpaque);

void
	DialogTransitions_DisplaySheetFromRectangle	(WindowRef				inWindow,
												 WindowRef				inParentWindow,
												 Rect const*			inFromWhere);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
