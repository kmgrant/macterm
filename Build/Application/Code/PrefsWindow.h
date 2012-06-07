/*!	\file PrefsWindow.h
	\brief Implements the shell of the Preferences window.
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

#ifndef __PREFSWINDOW__
#define __PREFSWINDOW__

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif

// application includes
#ifdef __OBJC__
#	import "Panel.h"
@class PrefPanelFullScreen_ViewManager;
#endif



#pragma mark Types

#ifdef __OBJC__

/*!
Implements the Cocoa window that wraps the Cocoa version of
the Preferences window that is under development.  See
"PrefsWindowCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefsWindow_Controller : NSWindowController
{
	IBOutlet NSView*		containerView;
@private
	NSMutableArray*						panelIDArray; // ordered array of "panelIdentifier" values
	NSMutableDictionary*				panelsByID; // view managers (Panel_ViewManager subclass) from "panelIdentifier" keys
	NSMutableDictionary*				windowSizesByID; // NSArray* values (each with 2 NSNumber*) from "panelIdentifier" keys
	NSSize								extraWindowSize; // stores window frame extra width and height (not belonging to a panel)
	Panel_ViewManager*					activePanel;
	PrefPanelFullScreen_ViewManager*	fullScreenPanel;
}

+ (id)
sharedPrefsWindowController;

// actions

- (IBAction)
performContextSensitiveHelp:(id)_;

@end

#endif // __OBJC__



#pragma mark Public Methods

void
	PrefsWindow_Done				();

void
	PrefsWindow_Display				();

void
	PrefsWindow_Remove				();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
