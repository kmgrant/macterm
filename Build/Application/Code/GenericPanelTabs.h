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
		© 1998-2022 by Kevin Grant.
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

#pragma once

// Mac includes
#include <CoreFoundation/CoreFoundation.h>
#ifdef __OBJC__
@class NSView;
#else
class NSView;
#endif

// application includes
#include "Panel.h"
#include "PrefsWindow.h"



#pragma mark Types

#ifdef __OBJC__

/*!
Loads a NIB file that defines this panel.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface GenericPanelTabs_ViewManager : Panel_ViewManager< NSTabViewDelegate,
																Panel_Delegate, Panel_Parent,
																PrefsWindow_PanelInterface > //{

// accessors
	@property (strong, nonnull) IBOutlet NSSegmentedControl*
	tabTitles;
	@property (strong, nonnull) IBOutlet NSTabView*
	tabView;

// initializers
	- (instancetype _Nullable)
	initWithIdentifier:(NSString* _Nonnull)_
	localizedName:(NSString* _Nonnull)_
	localizedIcon:(NSImage* _Nonnull)_
	viewManagerArray:(NSArray* _Nonnull)_ NS_DESIGNATED_INITIALIZER;

@end //}

#endif // __OBJC__

// BELOW IS REQUIRED NEWLINE TO END FILE
