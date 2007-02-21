/*!	\file SplashScreenDialog.h
	\brief Creates the title dialog that appears when MacTelnet
	first runs.
	
	On Mac OS X, this dialog is drawn in an overlay window so
	that cool effects can be applied to it.
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

#ifndef __SPLASHSCREENDIALOG__
#define __SPLASHSCREENDIALOG__



#pragma mark Public Methods

void
	SplashScreenDialog_Init				();

void
	SplashScreenDialog_Done				();

void
	SplashScreenDialog_Display			();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
