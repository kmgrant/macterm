/*!	\file DragAndDrop.h
	\brief Drag-and-drop management.
	
	This module is now largely for rendering new-style drag
	highlighting, as other drag activities can be automatically
	handled by various types of views.
*/
/*###############################################################

	MacTelnet
		© 1998-2009 by Kevin Grant.
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

#ifndef __DRAGANDDROP__
#define __DRAGANDDROP__



#pragma mark Constants

FourCharCode const			kDragFlavorTypeMacRomanText = 'TEXT';



#pragma mark Public Methods

//!\name Drag Highlighting
//@{

void
	DragAndDrop_HideHighlightBackground		(CGContextRef	inPort,
											 CGRect const&	inArea);

void
	DragAndDrop_HideHighlightFrame			(CGContextRef	inPort,
											 CGRect const&	inArea);

void
	DragAndDrop_ShowHighlightBackground		(CGContextRef	inPort,
											 CGRect const&	inArea);

void
	DragAndDrop_ShowHighlightFrame			(CGContextRef	inPort,
											 CGRect const&	inArea);

//@}

#endif


// BELOW IS REQUIRED NEWLINE TO END FILE
