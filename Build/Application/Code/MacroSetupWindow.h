/*!	\file MacroSetupWindow.h
	\brief Implements the macro editor UI.
	
	As of Mac OS X, MacTelnet’s macro editor is a large document
	window, that allows the user to edit macros even more easily
	than before.  All changes take effect immediately, and the
	currently displayed set is automatically made active.
*/
/*###############################################################

	MacTelnet
		© 1998-2007 by Kevin Grant.
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

#ifndef __MACROSETUPWINDOW__
#define __MACROSETUPWINDOW__



#pragma mark Public Methods

void
	MacroSetupWindow_Init				();

void
	MacroSetupWindow_Done				();

Boolean
	MacroSetupWindow_Display			();

void
	MacroSetupWindow_ReceiveDrop		(DragReference		inDragRef,
										 UInt8*				inData,
										 Size				inDataSize);

void
	MacroSetupWindow_Remove				();

WindowRef
	MacroSetupWindow_ReturnWindow		();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
