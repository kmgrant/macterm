/*!	\file PrefPanelTranslations.h
	\brief Implements the Translations panel of Preferences.
	
	This panel lets the user configure text translation in
	a session (to support special language glyphs, for
	instance).
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#ifndef __PREFPANELTRANSLATIONS__
#define __PREFPANELTRANSLATIONS__

// application includes
#include "Panel.h"
#include "Preferences.h"
#include "PrefsWindow.h"



#pragma mark Types

#ifdef __OBJC__

/*!
Loads a NIB file that defines this panel.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface PrefPanelTranslations_ViewManager : Panel_ViewManager< Panel_Delegate,
																	PrefsWindow_PanelInterface > //{
{
	IBOutlet NSTableView*	translationTableView;
@private
	NSMutableArray*				translationTables;
	Preferences_ContextRef		activeContext;
}

// accessors
	- (BOOL)
	backupFontEnabled;
	- (void)
	setBackupFontEnabled:(BOOL)_; // binding
	- (NSString*)
	backupFontFamilyName;
	- (void)
	setBackupFontFamilyName:(NSString*)_; // binding
	- (NSIndexSet*)
	translationTableIndexes;
	- (void)
	setTranslationTableIndexes:(NSIndexSet*)_; // binding
	- (NSArray*)
	translationTables; // binding

// actions
	- (IBAction)
	performBackupFontSelection:(id)_;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

Preferences_TagSetRef
	PrefPanelTranslations_NewTagSet		();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
