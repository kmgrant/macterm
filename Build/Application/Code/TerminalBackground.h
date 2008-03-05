/*!	\file TerminalBackground.h
	\brief Renders the background of a terminal view
	(for example, a solid color or a picture).
	
	Implemented as a regular HIView, so once you create
	one you can do anything you want with it.  The
	background view supports embedding of subviews.
*/
/*###############################################################

	MacTelnet
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

#ifndef __TERMINALBACKGROUND__
#define __TERMINALBACKGROUND__

// Mac includes
#include <Carbon/Carbon.h>



#pragma mark Public Methods

//!\name Initialization
//@{

void
	TerminalBackground_Init				();

void
	TerminalBackground_Done				();

//@}

//!\name Creating Terminal Backgrounds
//@{

OSStatus
	TerminalBackground_CreateHIView		(HIViewRef&		outView);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
