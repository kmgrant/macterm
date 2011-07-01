/*!	\file ProgressDialog.h
	\brief Implements all progress dialog boxes (modal or
	modeless) in MacTelnet.
	
	Collecting all of these dialogs under one standard interface
	allows the user interface to work better, since each new
	dialog can be staggered over any existing one automatically
	(or for that matter, they could automatically be merged into
	a single dialog in the future, such as in the progress
	window of Outlook Express 5).
*/
/*###############################################################

	MacTerm
		© 1998-2008 by Kevin Grant.
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

#ifndef __PROGRESSDIALOG__
#define __PROGRESSDIALOG__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Types

typedef struct ProgressDialog_OpaqueStruct*		ProgressDialog_Ref;

typedef SInt16 ProgressDialog_Indicator;
enum
{
	kProgressDialog_IndicatorThermometer	= 0,	// a standard progress bar
	kProgressDialog_IndicatorIndeterminate	= 1		// a barber-pole indeterminate progress bar
};



#pragma mark Public Methods

//!\name Creating and Destroying Progress Dialogs
//@{

ProgressDialog_Ref
	ProgressDialog_New						(CFStringRef				inStatusText,
											 Boolean					inIsModal);

void
	ProgressDialog_Dispose					(ProgressDialog_Ref*		inoutRefPtr);

//@}

//!\name Manipulating Progress Dialogs
//@{

void
	ProgressDialog_Display					(ProgressDialog_Ref			inRef);

HIWindowRef
	ProgressDialog_ReturnWindow				(ProgressDialog_Ref			inRef);

void
	ProgressDialog_SetProgressIndicator		(ProgressDialog_Ref			inRef,
											 ProgressDialog_Indicator	inIndicatorType);

void
	ProgressDialog_SetProgressPercentFull	(ProgressDialog_Ref			inRef,
											 SInt8						inPercentage);

void
	ProgressDialog_SetStatus				(ProgressDialog_Ref			inRef,
											 CFStringRef				inStatusText);

void
	ProgressDialog_SetTitle					(ProgressDialog_Ref			inRef,
											 CFStringRef				inTitleText);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
