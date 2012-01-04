/*!	\file TerminalToolbar.h
	\brief Items used in the toolbars of terminal windows.
	
	This module focuses on the Cocoa implementation even
	though Carbon-based toolbar items are still in use.
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

// Mac includes
#import <Cocoa/Cocoa.h>



#pragma mark Constants

extern NSString*	kTerminalToolbar_ItemIDCustomize;

#pragma mark Types

/*!
An instance of this object should be created in order to
handle delegate requests for an NSToolbar.
*/
@interface TerminalToolbar_Delegate : NSObject
{
}
@end

/*!
Toolbar item “Customize”.
*/
@interface TerminalToolbar_ItemCustomize : NSToolbarItem
{
}
@end

/*!
Toolbar item “L1”.
*/
@interface TerminalToolbar_ItemLED1 : NSToolbarItem
{
}
@end

/*!
Toolbar item “L2”.
*/
@interface TerminalToolbar_ItemLED2 : NSToolbarItem
{
}
@end

/*!
Toolbar item “L3”.
*/
@interface TerminalToolbar_ItemLED3 : NSToolbarItem
{
}
@end

/*!
Toolbar item “L4”.
*/
@interface TerminalToolbar_ItemLED4 : NSToolbarItem
{
}
@end

/*!
Toolbar item “Print”.
*/
@interface TerminalToolbar_ItemPrint : NSToolbarItem
{
}
@end

/*!
Toolbar item “Tabs”.
*/
@interface TerminalToolbar_ItemTabs : NSToolbarItem
{
	NSSegmentedControl*		segmentedControl;
	NSArray*				targets;
	SEL						action;
}

- (void)
setTabTargets:(NSArray*)_
andAction:(SEL)_;

@end

/*!
A sample object type that can be used to represent a tab in
the object array of a TerminalToolbar_ItemTabs instance.
*/
@interface TerminalToolbar_TabSource : NSObject
{
	NSAttributedString*		description;
}

- (id)
initWithDescription:(NSAttributedString*)_;

- (NSAttributedString*)
attributedDescription;

- (void)
performAction:(id)_;

@end

// BELOW IS REQUIRED NEWLINE TO END FILE
