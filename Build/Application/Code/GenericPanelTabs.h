/*!	\file GenericPanelTabs.h
	\brief Implements a panel that can contain others.
	
	Currently, the only available interface is tabs.
	Each panel is automatically placed in its own tab
	and the panel name is used as the tab title.  The
	unified tab set itself supports the Panel interface,
	allowing the tabs to be dropped into any container
	that support panels (such as the Preferences window
	and the Generic Dialog).
*/
/*###############################################################

	MacTerm
		© 1998-2011 by Kevin Grant.
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

#ifndef __GENERICPANELTABS__
#define __GENERICPANELTABS__

// standard-C++ includes
#include <vector>

// Mac includes
#include <CoreFoundation/CoreFoundation.h>

// application includes
#include "Panel.h"



#pragma mark Types

typedef std::vector< Panel_Ref >		GenericPanelTabs_List;



#pragma mark Public Methods

Panel_Ref
	GenericPanelTabs_New	(CFStringRef					inName,
							 Panel_Kind						inKind,
							 GenericPanelTabs_List const&	inTabs);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
