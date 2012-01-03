/*!	\file PrefPanelMacros.h
	\brief Implements the Macros panel of Preferences.
	
	This panel provides several advantages over the traditional
	macro editor: for one, it allows many sets of macros to be
	in memory at once.  Also, by re-classifying macros as a
	preferences category, persistence of macros is maintained
	(when before exporting and importing was necessary).  The
	modeless nature of the new Preferences window also means
	that macros can now be modified more easily than before,
	with instant effects.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#ifndef __PREFPANELMACROS__
#define __PREFPANELMACROS__

// application includes
#include "Panel.h"



#pragma mark Public Methods

Panel_Ref
	PrefPanelMacros_New					();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
