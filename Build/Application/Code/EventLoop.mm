/*!	\file EventLoop.mm
	\brief The main loop, and event-related utilities.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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

#import "EventLoop.h"
#import <UniversalDefines.h>

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <AlertMessages.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>

// application includes
#import "Commands.h"
#import "TerminalWindow.h"



#pragma mark Types

/*!
Private properties.
*/
@interface EventLoop_AppObject () //{

// accessors
	@property (strong) NSTouchBar*
	dynamicTouchBar;

@end //}



#pragma mark Public Methods

/*!
Call this method at the start of the program,
before it is necessary to track events.  This
method creates the global mouse region, among
other things.

\retval kEventLoop_ResultOK
always; no errors are currently defined

(3.0)
*/
EventLoop_Result
EventLoop_Init ()
{
	EventLoop_Result	result = kEventLoop_ResultOK;
	
	
	// ensure that Cocoa globals are defined (NSApp); this has to be
	// done separately, and not with NSApplicationMain(), because
	// some modules can only initialize after Cocoa is ready (and
	// not all modules are using Cocoa, yet)
	{
	@autoreleasepool {
		NSBundle*	mainBundle = nil;
		NSString*	mainFileName = nil;
		BOOL		loadOK = NO;
		
		
		[EventLoop_AppObject sharedApplication];
		assert(nil != NSApp);
		mainBundle = [NSBundle mainBundle];
		assert(nil != mainBundle);
		mainFileName = (NSString*)[mainBundle objectForInfoDictionaryKey:@"NSMainNibFile"]; // e.g. "MainMenuCocoa"
		assert(nil != mainFileName);
		loadOK = [NSBundle loadNibNamed:mainFileName owner:NSApp];
		assert(loadOK);
	}// @autoreleasepool
	}
	
	return result;
}// Init


/*!
Call this method at the end of the program,
when other clean-up is being done and it is
no longer necessary to track events.

(3.0)
*/
void
EventLoop_Done ()
{
}// Done


/*!
Returns true if the main window is in Full Screen mode, by
calling EventLoop_IsWindowFullScreen() on the main window.

(4.1)
*/
Boolean
EventLoop_IsMainWindowFullScreen ()
{
	Boolean		result = EventLoop_IsWindowFullScreen([NSApp mainWindow]);
	
	
	return result;
}// IsMainWindowFullScreen


/*!
Returns true if the specified window is in Full Screen mode.

This is slightly more convenient than checking the mask of an
arbitrary NSWindow but it is also necessary since there are
multiple ways for a terminal window to become full-screen
(user preference to bypass system mode).

(4.1)
*/
Boolean
EventLoop_IsWindowFullScreen	(NSWindow*		inWindow)
{
	Boolean		result = (0 != ([inWindow styleMask] & NSWindowStyleMaskFullScreen));
	
	
	if (NO == result)
	{
		TerminalWindowRef	terminalWindow = [inWindow terminalWindowRef];
		
		
		if (nullptr != terminalWindow)
		{
			result = TerminalWindow_IsFullScreen(terminalWindow);
		}
	}
	
	return result;
}// IsWindowFullScreen


/*!
Runs the main event loop, AND DOES NOT RETURN.  This routine
can ONLY be invoked from the main application thread.

To effectively run code “after” this point, modify the
application delegate’s "terminate:" method.

(3.0)
*/
void
EventLoop_Run ()
{
@autoreleasepool {
	[NSApp run];
}// @autoreleasepool
}// Run


#pragma mark -
@implementation EventLoop_AppObject //{


#pragma mark Initializers


/*!
Designated initializer.

(2016.11)
*/
- (id)
init
{
	self = [super init];
	if (nil != self)
	{
		_dynamicTouchBar = nil; // created on demand
	}
	return self;
}// init


#pragma mark NSApplication


/*!
Customizes the appearance of errors so that they use the
same kind of application-modal dialogs as other messages.
Returns YES only if recovery was attempted.

(2016.09)
*/
- (BOOL)
presentError:(NSError*)		anError
{
	// this flag should agree with behavior in
	// "presentError:modalForWindow:delegate:didPresentSelector:contextInfo:"
	BOOL	result = ((nil != anError.recoveryAttempter) && ([anError localizedRecoveryOptions].count > 0));
	
	
	[self presentError:anError modalForWindow:nil delegate:nil didPresentSelector:nil contextInfo:nil];
	return result;
}// presentError:


/*!
Customizes the appearance of errors so that they use the
same kind of sheets as other messages.

(2016.05)
*/
- (void)
presentError:(NSError*)			anError
modalForWindow:(NSWindow*)		aParentWindow
delegate:(id)					aDelegate
didPresentSelector:(SEL)		aDidPresentSelector
contextInfo:(void*)				aContext
{
	NSArray*	buttonArray = [anError localizedRecoveryOptions];
	BOOL		callSuper = YES;
	
	
	// debug
	//NSLog(@"presenting error [domain: %@, code: %ld, info: %@]: %@", [anError domain], (long)[anError code], [anError userInfo], anError);
	
	if ([anError.domain isEqualToString:NSCocoaErrorDomain] &&
		(NSUserCancelledError == anError.code))
	{
		// no reason to display a message in this case
		callSuper = NO;
	}
	else if (buttonArray.count > 3)
	{
		// ???
		// do not know how to handle this; defer to superclass
		callSuper = YES;
	}
	else
	{
		AlertMessages_BoxWrap	box(Alert_NewWindowModal(nullptr/*aParentWindow*/),
									AlertMessages_BoxWrap::kAlreadyRetained);
		NSString*				dialogText = anError.localizedDescription;
		NSString*				helpText = anError.localizedRecoverySuggestion;
		NSString*				helpSearch = anError.helpAnchor;
		id						recoveryAttempter = anError.recoveryAttempter;
		
		
		Alert_SetParamsFor(box.returnRef(), kAlert_StyleOK);
		
		if (nil == dialogText)
		{
			dialogText = @"";
		}
		if (nil == helpText)
		{
			helpText = @"";
		}
		Alert_SetTextCFStrings(box.returnRef(), BRIDGE_CAST(dialogText, CFStringRef), BRIDGE_CAST(helpText, CFStringRef));
		
		if ([buttonArray count] > 0)
		{
			Alert_SetButtonText(box.returnRef(), kAlert_ItemButton1, BRIDGE_CAST([buttonArray objectAtIndex:0], CFStringRef));
			Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemButton1,
			^{
				[recoveryAttempter attemptRecoveryFromError:anError optionIndex:0 delegate:aDelegate
															didRecoverSelector:aDidPresentSelector contextInfo:aContext];
			});
		}
		if ([buttonArray count] > 1)
		{
			Alert_SetButtonText(box.returnRef(), kAlert_ItemButton2, BRIDGE_CAST([buttonArray objectAtIndex:1], CFStringRef));
			Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemButton2,
			^{
				[recoveryAttempter attemptRecoveryFromError:anError optionIndex:1 delegate:aDelegate
															didRecoverSelector:aDidPresentSelector contextInfo:aContext];
			});
		}
		if ([buttonArray count] > 2)
		{
			Alert_SetButtonText(box.returnRef(), kAlert_ItemButton3, BRIDGE_CAST([buttonArray objectAtIndex:2], CFStringRef));
			Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemButton3,
			^{
				[recoveryAttempter attemptRecoveryFromError:anError optionIndex:2 delegate:aDelegate
															didRecoverSelector:aDidPresentSelector contextInfo:aContext];
			});
		}
		if (nil != helpSearch)
		{
			Alert_SetHelpButton(box.returnRef(), true);
			Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemHelpButton,
			^{
				NSString*		bookID = [[NSBundle mainBundle] objectForInfoDictionaryKey: @"CFBundleHelpBookName"];
				
				
				[[NSHelpManager sharedHelpManager] openHelpAnchor:helpSearch inBook:bookID];
			});
		}
		
		Alert_Display(box.returnRef()); // retains alert until it is dismissed
		
		// alert is handled above; should not display default alert
		callSuper = NO;
	}
	
	if (callSuper)
	{
		[super presentError:anError modalForWindow:aParentWindow delegate:aDelegate
							didPresentSelector:aDidPresentSelector contextInfo:aContext];
	}
}// presentError:modalForWindow:delegate:didPresentSelector:contextInfo:


#pragma mark NSResponder


/*!
On OS 10.12.1 and beyond, returns a Touch Bar to display
at the top of the hardware keyboard (when available) or
in any Touch Bar simulator window.

This method should not be called except by the OS.

(2016.11)
*/
- (NSTouchBar*)
makeTouchBar
{
	NSTouchBar*		result = self.dynamicTouchBar;
	
	
	if (nil == result)
	{
		self.dynamicTouchBar = result = [[NSTouchBar alloc] init];
		result.customizationIdentifier = kConstantsRegistry_TouchBarIDApplicationMain;
		result.customizationAllowedItemIdentifiers =
		@[
			NSTouchBarItemIdentifierFlexibleSpace,
			NSTouchBarItemIdentifierFixedSpaceSmall,
			NSTouchBarItemIdentifierFixedSpaceLarge
		];
		result.defaultItemIdentifiers =
		@[
			NSTouchBarItemIdentifierOtherItemsProxy,
		];
	}
	
	return result;
}// makeTouchBar


@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
