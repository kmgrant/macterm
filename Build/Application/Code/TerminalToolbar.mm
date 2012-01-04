/*!	\file TerminalToolbar.mm
	\brief Items used in the toolbars of terminal windows.
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

#import "TerminalToolbar.objc++.h"
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <CocoaFuture.objc++.h>
#import <Console.h>

// application includes
#import "AppResources.h"
#import "Commands.h"
#import "ConstantsRegistry.h"



#pragma mark Constants
namespace {

// NOTE: do not ever change these, that would only break user preferences
NSString*	kMy_ToolbarItemIDLED1		= @"net.macterm.MacTerm.toolbaritem.led1";
NSString*	kMy_ToolbarItemIDLED2		= @"net.macterm.MacTerm.toolbaritem.led2";
NSString*	kMy_ToolbarItemIDLED3		= @"net.macterm.MacTerm.toolbaritem.led3";
NSString*	kMy_ToolbarItemIDLED4		= @"net.macterm.MacTerm.toolbaritem.led4";
NSString*	kMy_ToolbarItemIDPrint		= @"net.macterm.MacTerm.toolbaritem.print";
NSString*	kMy_ToolbarItemIDTabs		= @"net.macterm.MacTerm.toolbaritem.tabs";

} // anonymous namespace

// some IDs are shared (NOTE: do not ever change these, that would only break user preferences)
NSString*	kTerminalToolbar_ItemIDCustomize	= @"net.macterm.MacTerm.toolbaritem.customize";



#pragma mark Internal Methods

@implementation TerminalToolbar_Delegate


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super init];
	return self;
}// init


#pragma mark NSToolbarDelegate


/*!
Responds when the specified kind of toolbar item should be
constructed for the given toolbar.

(4.0)
*/
- (NSToolbarItem*)
toolbar:(NSToolbar*)				toolbar
itemForItemIdentifier:(NSString*)	itemIdentifier
willBeInsertedIntoToolbar:(BOOL)	flag
{
#pragma unused(toolbar, flag)
	NSToolbarItem*		result = nil;
	
	
	// NOTE: no need to handle standard items here
	// TEMPORARY - need to create all custom items
	if ([itemIdentifier isEqualToString:kTerminalToolbar_ItemIDCustomize])
	{
		result = [[[TerminalToolbar_ItemCustomize alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED1])
	{
		result = [[[TerminalToolbar_ItemLED1 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED2])
	{
		result = [[[TerminalToolbar_ItemLED2 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED3])
	{
		result = [[[TerminalToolbar_ItemLED3 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDLED4])
	{
		result = [[[TerminalToolbar_ItemLED4 alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDPrint])
	{
		result = [[[TerminalToolbar_ItemPrint alloc] init] autorelease];
	}
	else if ([itemIdentifier isEqualToString:kMy_ToolbarItemIDTabs])
	{
		result = [[[TerminalToolbar_ItemTabs alloc] init] autorelease];
	}
	return result;
}// toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:


/*!
Returns the identifiers for the kinds of items that can appear
in the given toolbar.

(4.0)
*/
- (NSArray*)
toolbarAllowedItemIdentifiers:(NSToolbar*)	toolbar
{
#pragma unused(toolbar)
	return [NSArray arrayWithObjects:
						kMy_ToolbarItemIDTabs,
						NSToolbarSpaceItemIdentifier,
						NSToolbarFlexibleSpaceItemIdentifier,
						kTerminalToolbar_ItemIDCustomize,
						kMy_ToolbarItemIDLED1,
						kMy_ToolbarItemIDLED2,
						kMy_ToolbarItemIDLED3,
						kMy_ToolbarItemIDLED4,
						kMy_ToolbarItemIDPrint,
						nil];
}// toolbarAllowedItemIdentifiers


/*!
Returns the identifiers for the items that will appear in the
given toolbar whenever the user has not customized it.

(4.0)
*/
- (NSArray*)
toolbarDefaultItemIdentifiers:(NSToolbar*)	toolbar
{
#pragma unused(toolbar)
	return [NSArray arrayWithObjects:
						kMy_ToolbarItemIDTabs,
						NSToolbarFlexibleSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						kMy_ToolbarItemIDLED1,
						kMy_ToolbarItemIDLED2,
						kMy_ToolbarItemIDLED3,
						kMy_ToolbarItemIDLED4,
						NSToolbarSpaceItemIdentifier,
						kMy_ToolbarItemIDPrint,
						NSToolbarSpaceItemIdentifier,
						NSToolbarSpaceItemIdentifier,
						nil];
}// toolbarDefaultItemIdentifiers


@end // TerminalToolbar_Delegate


@implementation TerminalToolbar_ItemCustomize


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	// WARNING: The Customize item is currently redundantly implemented in the Terminal Window module;
	// this is TEMPORARY, but both implementations should agree.
	self = [super initWithItemIdentifier:kTerminalToolbar_ItemIDCustomize];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnCustomizeToolbarIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Customize", @"toolbar item name; for customizing the toolbar")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	// TEMPORARY; only doing it this way during Carbon/Cocoa transition
	[[Commands_Executor sharedExecutor] runToolbarCustomizationPaletteSetup:NSApp];
}// performToolbarItemAction:


@end // TerminalToolbar_ItemCustomize


@implementation TerminalToolbar_ItemLED1


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED1];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOffIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"L1", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED1);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemLED1


@implementation TerminalToolbar_ItemLED2


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED2];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOffIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"L2", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED2);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemLED2


@implementation TerminalToolbar_ItemLED3


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED3];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOffIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"L3", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED3);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemLED3


@implementation TerminalToolbar_ItemLED4


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDLED4];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnLEDOffIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"L4", @"toolbar item name; for terminal LED #1")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandToggleTerminalLED4);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemLED4


@implementation TerminalToolbar_ItemPrint


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDPrint];
	if (nil != self)
	{
		[self setAction:@selector(performToolbarItemAction:)];
		[self setTarget:self];
		[self setEnabled:YES];
		[self setImage:[NSImage imageNamed:(NSString*)AppResources_ReturnPrintIconFilenameNoExtension()]];
		[self setLabel:NSLocalizedString(@"Print", @"toolbar item name; for printing")];
		[self setPaletteLabel:[self label]];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Responds when the toolbar item is used.

(4.0)
*/
- (void)
performToolbarItemAction:(id)	sender
{
#pragma unused(sender)
	Commands_ExecuteByIDUsingEvent(kCommandPrint);
}// performToolbarItemAction:


@end // TerminalToolbar_ItemPrint


@implementation TerminalToolbar_TabSource


/*!
Designated initializer.

(4.0)
*/
- (id)
initWithDescription:(NSAttributedString*)	aDescription
{
	self = [super init];
	if (nil != self)
	{
		self->description = [aDescription copy];
	}
	return self;
}// initWithDescription:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[description release];
	[super dealloc];
}// dealloc


/*!
The attributed string describing the title of the tab; this
is used in the segmented control as well as any overflow menu.

(4.0)
*/
- (NSAttributedString*)
attributedDescription
{
	return description;
}// attributedDescription


/*!
The tooltip that is displayed when the mouse points to the
tab’s segment in the toolbar.

(4.0)
*/
- (NSString*)
toolTip
{
	return @"";
}// toolTip


/*!
Performs the action for this tab (namely, to bring something
to the front).  The sender will be an instance of the
"TerminalToolbar_ItemTabs" class.

(4.0)
*/
- (void)
performAction:(id) sender
{
#pragma unused(sender)
}// performAction:


@end // TerminalToolbar_TabSource


@implementation TerminalToolbar_ItemTabs


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithItemIdentifier:kMy_ToolbarItemIDTabs];
	if (nil != self)
	{
		self->segmentedControl = [[NSSegmentedControl alloc] initWithFrame:NSZeroRect];
		[self->segmentedControl setTarget:self];
		[self->segmentedControl setAction:@selector(performSegmentedControlAction:)];
		if (FlagManager_Test(kFlagOS10_5API))
		{
			if ([self->segmentedControl respondsToSelector:@selector(setSegmentStyle:)])
			{
				[self->segmentedControl setSegmentStyle:FUTURE_SYMBOL(4, NSSegmentStyleTexturedSquare)];
			}
		}
		
		//[self setAction:@selector(performToolbarItemAction:)];
		//[self setTarget:self];
		[self setEnabled:YES];
		[self setView:self->segmentedControl];
		[self setMinSize:NSMakeSize(120, 25)]; // arbitrary
		[self setMaxSize:NSMakeSize(1024, 25)]; // arbitrary
		[self setLabel:@""];
		[self setPaletteLabel:NSLocalizedString(@"Tabs", @"toolbar item name; for tabs")];
		
		[self setTabTargets:[NSArray arrayWithObjects:
										[[[TerminalToolbar_TabSource alloc]
											initWithDescription:[[[NSAttributedString alloc]
																	initWithString:NSLocalizedString
																					(@"Tab 1", @"toolbar item tabs; default segment 0 name")]
																	autorelease]] autorelease],
										[[[TerminalToolbar_TabSource alloc]
											initWithDescription:[[[NSAttributedString alloc]
																	initWithString:NSLocalizedString
																					(@"Tab 2", @"toolbar item tabs; default segment 1 name")]
																	autorelease]] autorelease],
										nil]
				andAction:nil];
	}
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[self->segmentedControl release];
	[self->targets release];
	[super dealloc];
}// dealloc


/*!
Responds to an action in the menu of a toolbar item by finding
the corresponding object in the target array and making it
perform the action set by "setTabTargets:andAction:".

(4.0)
*/
- (void)
performMenuAction:(id)		sender
{
	if (nullptr != self->action)
	{
		NSMenuItem*		asMenuItem = (NSMenuItem*)sender;
		NSMenu*			menu = [asMenuItem menu];
		
		
		if (nil != menu)
		{
			unsigned int const		kIndex = STATIC_CAST([menu indexOfItem:asMenuItem], unsigned int);
			
			
			if (kIndex < [self->targets count])
			{
				(id)[[self->targets objectAtIndex:kIndex] performSelector:self->action withObject:self];
			}
		}
	}
}// performMenuAction:


/*!
Responds to an action in a segmented control by finding the
corresponding object in the target array and making it perform
the action set by "setTabTargets:andAction:".

(4.0)
*/
- (void)
performSegmentedControlAction:(id)		sender
{
	if (nullptr != self->action)
	{
		if (self->segmentedControl == sender)
		{
			unsigned int const		kIndex = STATIC_CAST([self->segmentedControl selectedSegment], unsigned int);
			
			
			if (kIndex < [self->targets count])
			{
				(id)[[self->targets objectAtIndex:kIndex] performSelector:self->action withObject:self];
			}
		}
	}
}// performSegmentedControlAction:


/*!
Specifies the model on which the tab display is based.

When any tab is selected the specified selector is invoked
on one of the given objects, with this toolbar item as the
sender.  This is true no matter how the item is found: via
segmented control or overflow menu.

Each object:
- MUST respond to an "attributedDescription" selector that
  returns an "NSAttributedString*" for that tab’s label.
- MAY respond to a "toolTip" selector to return a string for
  that tab’s tooltip.

See the class "TerminalToolbar_TabSource" which is an example
of a valid object for the array.

In the future, additional selectors may be prescribed for the
object (to set an icon, for instance).

(4.0)
*/
- (void)
setTabTargets:(NSArray*)	anObjectArray
andAction:(SEL)				aSelector
{
	if (self->targets != anObjectArray)
	{
		[self->targets release];
		self->targets = [anObjectArray copy];
	}
	self->action = aSelector;
	
	// update the user interface
	[self->segmentedControl setSegmentCount:[self->targets count]];
	[self->segmentedControl setSelectedSegment:0];
	{
		NSEnumerator*	toObject = [self->targets objectEnumerator];
		NSMenu*			menuRep = [[[NSMenu alloc] init] autorelease];
		NSMenuItem*		menuItem = [[[NSMenuItem alloc] initWithTitle:@"" action:self->action keyEquivalent:@""] autorelease];
		unsigned int	i = 0;
		
		
		// with "performMenuAction:" and "performSegmentedControlAction:",
		// actions can be handled consistently by the caller; those
		// methods reroute invocations by menu item or segmented control,
		// and present the same sender (this NSToolbarItem) instead
		while (id object = [toObject nextObject])
		{
			NSMenuItem*		newItem = [[[NSMenuItem alloc] initWithTitle:[[object attributedDescription] string]
																			action:@selector(performMenuAction:) keyEquivalent:@""]
										autorelease];
			
			
			[newItem setAttributedTitle:[object attributedDescription]];
			[newItem setTarget:self];
			[self->segmentedControl setLabel:[[object attributedDescription] string] forSegment:i];
			if ([object respondsToSelector:@selector(toolTip)])
			{
				[[self->segmentedControl cell] setToolTip:[object toolTip] forSegment:i];
			}
			[menuRep addItem:newItem];
			++i;
		}
		[menuItem setSubmenu:menuRep];
		[self setMenuFormRepresentation:menuItem];
	}
}// setTabTargets:andAction:


@end // TerminalToolbar_ItemTabs

// BELOW IS REQUIRED NEWLINE TO END FILE
