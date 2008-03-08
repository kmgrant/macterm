/*!	\file FormatDialog.h
	\brief Implements a dialog for editing font, size and color
	information for a terminal window.
*/
/*###############################################################

	MacTelnet
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

#ifndef __FORMATDIALOG__
#define __FORMATDIALOG__

// Mac includes
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "GenericDialog.h"
#include "Preferences.h"



#pragma mark Types

typedef struct FormatDialog_OpaqueStruct*	FormatDialog_Ref;



#pragma mark Public Methods

FormatDialog_Ref
	FormatDialog_New						(HIWindowRef						inParentWindowOrNullForModalDialog,
											 Preferences_ContextRef				inoutData);

void
	FormatDialog_Dispose					(FormatDialog_Ref*					inoutDialogPtr);

void
	FormatDialog_Display					(FormatDialog_Ref					inDialog);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
