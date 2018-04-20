/*!	\file EventLoop.mm
	\brief The main loop, and event-related utilities.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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

// standard-C++ includes
#import <algorithm>
#import <map>
#import <vector>

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>
#import <QuickTime/QuickTime.h>

// library includes
#import <AlertMessages.h>
#import <CarbonEventHandlerWrap.template.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <GrowlSupport.h>
#import <MacHelpUtilities.h>
#import <TouchBar.objc++.h>

// application includes
#import "Commands.h"
#import "DialogUtilities.h"
#import "TerminalWindow.h"



#pragma mark Internal Method Prototypes
namespace {

OSStatus				receiveHICommand				(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveSheetOpening				(EventHandlerCallRef, EventRef, void*);
OSStatus				receiveWindowActivated			(EventHandlerCallRef, EventRef, void*);
OSStatus				updateModifiers					(EventHandlerCallRef, EventRef, void*);

} // anonymous namespace

#pragma mark Variables
namespace {

NSAutoreleasePool*					gGlobalPool = nil;
CarbonEventHandlerWrap				gCarbonEventHICommandHandler(GetApplicationEventTarget(),
																	receiveHICommand,
																	CarbonEventSetInClass
																		(CarbonEventClass(kEventClassCommand),
																			kEventCommandProcess,
																			kEventCommandUpdateStatus),
																	nullptr/* user data */);
Console_Assertion					_1(gCarbonEventHICommandHandler.isInstalled(), __FILE__, __LINE__);
UInt32								gCarbonEventModifiers = 0L; // current modifier key states; updated by the callback
CarbonEventHandlerWrap				gCarbonEventModifiersHandler(GetApplicationEventTarget(),
																	updateModifiers,
																	CarbonEventSetInClass
																		(CarbonEventClass(kEventClassKeyboard),
																			kEventRawKeyModifiersChanged,
																			kEventRawKeyUp),
																	nullptr/* user data */);
Console_Assertion					_2(gCarbonEventModifiersHandler.isInstalled(), __FILE__, __LINE__);
CarbonEventHandlerWrap				gCarbonEventWindowActivateHandler(GetApplicationEventTarget(),
																		receiveWindowActivated,
																		CarbonEventSetInClass
																			(CarbonEventClass(kEventClassWindow),
																				kEventWindowActivated, kEventWindowDeactivated),
																		nullptr/* user data */);
Console_Assertion					_4(gCarbonEventWindowActivateHandler.isInstalled(), __FILE__, __LINE__);
EventHandlerUPP						gCarbonEventSheetOpeningUPP = nullptr;
EventHandlerRef						gCarbonEventSheetOpeningHandler = nullptr;

} // anonymous namespace



#pragma mark Public Methods

/*!
Call this method at the start of the program,
before it is necessary to track events.  This
method creates the global mouse region, among
other things.

\retval noErr
always; no errors are currently defined

(3.0)
*/
EventLoop_Result
EventLoop_Init ()
{
	EventLoop_Result	result = noErr;
	
	
	// a pool must be constructed here, because NSApplicationMain()
	// is never called
	gGlobalPool = [[NSAutoreleasePool alloc] init];
	
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
	
	// support Growl notifications if possible
	GrowlSupport_Init();
	
	// install a callback that detects toolbar sheets
	{
		EventTypeSpec const		whenToolbarSheetOpens[] =
								{
									{ kEventClassWindow, kEventWindowSheetOpening }
								};
		OSStatus				error = noErr;
		
		
		gCarbonEventSheetOpeningUPP = NewEventHandlerUPP(receiveSheetOpening);
		error = InstallApplicationEventHandler(gCarbonEventSheetOpeningUPP,
												GetEventTypeCount(whenToolbarSheetOpens),
												whenToolbarSheetOpens, nullptr/* user data */,
												&gCarbonEventSheetOpeningHandler/* event handler reference */);
		if (noErr != error)
		{
			// it’s not critical if this handler is installed
			Console_Warning(Console_WriteValue, "failed to install event loop sheet-opening handler, error", error);
		}
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
	RemoveEventHandler(gCarbonEventSheetOpeningHandler), gCarbonEventSheetOpeningHandler = nullptr;
	DisposeEventHandlerUPP(gCarbonEventSheetOpeningUPP), gCarbonEventSheetOpeningUPP = nullptr;
	
	//[gGlobalPool release];
}// Done


/*!
Determines the state of the Caps Lock key when you
do not have access to an event record.  Returns
"true" only if the key is down.

(1.0)
*/
Boolean
EventLoop_IsCapsLockKeyDown ()
{
	// under Carbon, a callback updates a variable whenever
	// modifier keys change; therefore, just check that value!
	return ((gCarbonEventModifiers & alphaLock) != 0);
}// IsCapsLockKeyDown


/*!
Determines the state of the  key when you do not
have access to an event record.  Returns "true"
only if the key is down.

(1.0)
*/
Boolean
EventLoop_IsCommandKeyDown ()
{
	// under Carbon, a callback updates a variable whenever
	// modifier keys change; therefore, just check that value!
	return ((gCarbonEventModifiers & cmdKey) != 0);
}// IsCommandKeyDown


/*!
Determines the state of the Control key when you
do not have access to an event record.  Returns
"true" only if the key is down.

(4.0)
*/
Boolean
EventLoop_IsControlKeyDown ()
{
	// under Carbon, a callback updates a variable whenever
	// modifier keys change; therefore, just check that value!
	return ((gCarbonEventModifiers & controlKey) != 0);
}// IsControlKeyDown


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
	Boolean		result = (0 != ([inWindow styleMask] & FUTURE_SYMBOL(1 << 14, NSFullScreenWindowMask)));
	
	
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
This routine blocks until a mouse-down event occurs
or the “double time” has elapsed.  Returns "true"
only if it is a “double-click” event.

Only if "true" is returned, the resulting mouse
location in global coordinates is returned.

You normally invoke this immediately after you have
received a *mouse-up* event to see if the user is
actually intending to click twice.

Despite its name, this routine can detect triple-
clicks, etc. simply by being used repeatedly.  It
can also detect “one and a half clicks” because it
does not wait for a subsequent mouse-up.

DEPRECATED.  This may be necessary to detect certain
mouse behavior in raw event environments during
Carbon porting but it would generally NOT be this
difficult in a pure Cocoa event scheme.

(3.0)
*/
Boolean
EventLoop_IsNextDoubleClick		(HIWindowRef	inWindow,
								 Point&			outGlobalMouseLocation)
{
	Boolean		result = false;
	NSEvent*	nextClick = [NSApp nextEventMatchingMask:NSLeftMouseDownMask
															untilDate:[NSDate dateWithTimeIntervalSinceNow:[NSEvent doubleClickInterval]]
															inMode:NSEventTrackingRunLoopMode dequeue:YES];
	
	
	// NOTE: this used to be accomplished with Carbon event-handling
	// code but as of Mac OS X 10.9 the event loop seems unable to
	// reliably return the next click event; thus, Cocoa is used
	// (and this is a horrible coordinate-translation hack; it
	// reproduces the required behavior for now but it also just
	// underscores how critical it is that everything finally be
	// moved natively to Cocoa)
	if (nil != nextClick)
	{
		NSWindow*	eventWindow = [nextClick window];
		
		
		if (eventWindow == CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(inWindow))
		{
			NSPoint		clickLocation = [nextClick locationInWindow];
			
			
			clickLocation = [eventWindow convertBaseToScreen:clickLocation];
			clickLocation.y = [[eventWindow screen] frame].size.height;
			outGlobalMouseLocation.h = STATIC_CAST(clickLocation.x, SInt16);
			outGlobalMouseLocation.v = STATIC_CAST(clickLocation.y, SInt16);
			clickLocation = [eventWindow localToGlobalRelativeToTopForPoint:[nextClick locationInWindow]];
			outGlobalMouseLocation.h = STATIC_CAST(clickLocation.x, SInt16);
			outGlobalMouseLocation.v = STATIC_CAST(clickLocation.y, SInt16);
			result = ([nextClick clickCount] > 1);
		}
	}
	
	return result;
}// IsNextDoubleClick


/*!
Determines the state of the Option key when you
do not have access to an event record.  Returns
"true" only if the key is down.

(3.0)
*/
Boolean
EventLoop_IsOptionKeyDown ()
{
	// under Carbon, a callback updates a variable whenever
	// modifier keys change; therefore, just check that value!
	return ((gCarbonEventModifiers & optionKey) != 0);
}// IsOptionKeyDown


/*!
Determines the state of the Shift key when
you do not have access to an event record.
Returns "true" only if the key is down.

(3.0)
*/
Boolean
EventLoop_IsShiftKeyDown ()
{
	// under Carbon, a callback updates a variable whenever
	// modifier keys change; therefore, just check that value!
	return ((gCarbonEventModifiers & shiftKey) != 0);
}// IsShiftKeyDown


/*!
Determines the absolutely current state of the
modifier keys and the mouse button.  The
modifiers are somewhat incomplete (for example,
no checking is done for the “right shift key”,
etc.), but all the common modifiers (command,
control, shift, option, caps lock, and the
mouse button) are returned.

(3.0)
*/
EventModifiers
EventLoop_ReturnCurrentModifiers ()
{
	// under Carbon, a callback updates a global variable whenever
	// the state of a modifier key changes; so, just return that value
	return STATIC_CAST(gCarbonEventModifiers, EventModifiers);
}// ReturnCurrentModifiers


/*!
Use instead of FrontWindow() to return the active
non-floating window.

(3.0)
*/
HIWindowRef
EventLoop_ReturnRealFrontWindow ()
{
	return ActiveNonFloatingWindow();
}// ReturnRealFrontWindow


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


#pragma mark Internal Methods
namespace {

/*!
Handles "kEventCommandProcess" of "kEventClassCommand".

(3.0)
*/
OSStatus
receiveHICommand	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassCommand);
	{
		HICommand	received;
		
		
		// determine the command in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, received);
		
		// if the command information was found, proceed
		if (noErr == result)
		{
			switch (kEventKind)
			{
			case kEventCommandProcess:
				// Execute a command selected from a menu.
				//
				// IMPORTANT: This could imply ANY form of menu selection, whether from
				//            the menu bar, from a contextual menu, or from a pop-up menu!
				{
					if (received.commandID != 0)
					{
						Boolean		commandOK = false;
						
						
						// execute the command
						commandOK = Commands_ExecuteByID(received.commandID);
						if (commandOK)
						{
							result = noErr;
						}
						else
						{
							result = eventNotHandledErr;
						}
					}
					else
					{
						UInt32		commandID = 0L;
						OSStatus	error = noErr;
						
						
						result = eventNotHandledErr; // initially...
						
						error = GetMenuItemCommandID(received.menu.menuRef, received.menu.menuItemIndex, &commandID);
						if (noErr == error)
						{
							// if a menu item wasn’t handled, make this an obvious bug by leaving the menu title highlighted
							if (Commands_ExecuteByID(commandID))
							{
								HiliteMenu(0);
								result = noErr;
							}
						}
					}
				}
				break;
			
			default:
				// ???
				result = eventNotHandledErr;
				break;
			}
		}
	}
	return result;
}// receiveHICommand


/*!
Handles "kEventWindowSheetOpening" of "kEventClassWindow".

Invoked by Mac OS X whenever a sheet is about to open.
This handler could prevent the opening by returning
"userCanceledErr".

(3.1)
*/
OSStatus
receiveSheetOpening		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowSheetOpening);
	
	{
		HIWindowRef		sheetWindow = nullptr;
		
		
		// determine the window that is opening
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, sheetWindow);
		
		// if the window was found, proceed
		if (noErr == result)
		{
			CFStringRef		sheetTitleCFString = nullptr;
			
			
			// use special knowledge of how the Customize Toolbar sheet is set up
			// (/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/
			//	HIToolbox.framework/Versions/A/Resources/English.lproj/Toolbar.nib)
			// to hack in a more Cocoa-like look-and-feel; this is obviously
			// completely cosmetic, so any errors are ignored, etc.
			if (noErr == CopyWindowTitleAsCFString(sheetWindow, &sheetTitleCFString))
			{
				// attempt to identify the customization sheet using its title
				if (kCFCompareEqualTo == CFStringCompare(sheetTitleCFString, CFSTR("Configure Toolbar")/* from NIB */,
															0/* options */))
				{
					HIViewRef	contentView = nullptr;
					
					
					if (noErr == HIViewFindByID(HIViewGetRoot(sheetWindow), kHIViewWindowContentID,
												&contentView))
					{
						//HIViewID const		kPrompt2ID = { 'tcfg', 7 }; // from NIB
						HIViewID const		kShowLabelID = { 'tcfg', 5 }; // from NIB
						HIViewRef			possibleTextView = HIViewGetFirstSubview(contentView);
						
						
						// attempt to find the static text views that are not bold
						while (nullptr != possibleTextView)
						{
							ControlKind		kindInfo;
							HIViewID		viewID;
							
							
							// ensure this is some kind of text view
							if ((noErr == GetControlKind(possibleTextView, &kindInfo)) &&
								(kControlKindSignatureApple == kindInfo.signature) &&
								(kControlKindStaticText == kindInfo.kind))
							{
								// since the text was sized assuming regular font, it needs to
								// become slightly wider to accommodate bold font
								HIRect		textBounds;
								if (noErr == HIViewGetFrame(possibleTextView, &textBounds))
								{
									// the size problem only applies to the main prompt, not the others;
									// this prompt is arbitrarily identified by having a width bigger
									// than any of the other text areas (because it does not have a
									// proper ControlID like all the other customization sheet views)
									if (textBounds.size.width > 260/* arbitrary; uniquely identify the main prompt! */)
									{
										textBounds.size.width += 20; // arbitrary
										HIViewSetFrame(possibleTextView, &textBounds);
									}
								}
								
								// make the text bold!
								{
									ControlFontStyleRec		styleRec;
									
									
									if (noErr == GetControlData(possibleTextView, kControlEntireControl, kControlFontStyleTag,
																sizeof(styleRec), &styleRec, nullptr/* actual size */))
									{
										styleRec.style = bold;
										styleRec.flags |= kControlUseFaceMask;
										UNUSED_RETURN(OSStatus)SetControlFontStyle(possibleTextView, &styleRec);
									}
								}
								
								if ((noErr == GetControlID(possibleTextView, &viewID)) &&
									(kShowLabelID.signature == viewID.signature) &&
									(kShowLabelID.id == viewID.id))
								{
									// while there is a hack in place anyway, might as well
									// correct the annoying one pixel vertical misalignment
									// of the "Show" label for the pop-up menu in the corner
									HIViewMoveBy(possibleTextView, 0/* delta X */, 1.0/* delta Y */);
									
									// while there is a hack in place anyway, add a colon to
									// the label for consistency with every other dialog in
									// the OS!
									SetControlTextWithCFString(possibleTextView, CFSTR("Show:"));
									
									// resize the label a smidgen to make room for the colon
									if (noErr == HIViewGetFrame(possibleTextView, &textBounds))
									{
										textBounds.size.width += 2; // arbitrary
										HIViewSetFrame(possibleTextView, &textBounds);
									}
								}
							}
							possibleTextView = HIViewGetNextView(possibleTextView);
						}
					}
				}
				CFRelease(sheetTitleCFString), sheetTitleCFString = nullptr;
			}
		}
	}
	
	// IMPORTANT: Do not interfere with this event.
	result = eventNotHandledErr;
	
	return result;
}// receiveSheetOpening


/*!
Embellishes "kEventWindowActivated" and "kEventWindowActivated"
of "kEventClassWindow" by resetting the mouse cursor and
updating the Touch Bar.

(3.1)
*/
OSStatus
receiveWindowActivated	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = eventNotHandledErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert((kEventKind == kEventWindowActivated) || (kEventKind == kEventWindowDeactivated));
	
	if (kEventKind == kEventWindowActivated)
	{
		[[NSCursor arrowCursor] set];
	}
	
	// invalidate the application-level Touch Bar so that the
	// terminal window Touch Bar can always be activated
	// (TEMPORARY; only needed until Carbon transition is done)
	CocoaExtensions_PerformSelectorOnTargetWithValue(@selector(setTouchBar:), NSApp, nil);
	
	// IMPORTANT: Do not interfere with this event.
	result = eventNotHandledErr;
	
	return result;
}// receiveWindowActivated


/*!
Invoked by Mac OS X whenever a modifier key’s state changes
(e.g. option, control, command, or shift).  This routine updates
an internal variable that is used by other functions (such as
EventLoop_IsCommandKeyDown()).

(3.0)
*/
OSStatus
updateModifiers		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 void*					UNUSED_ARGUMENT(inUserData))
{
	OSStatus		result = noErr;
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassKeyboard);
	assert((kEventKind == kEventRawKeyModifiersChanged) || (kEventKind == kEventRawKeyUp));
	{
		// extract modifier key bits from the given event; it is important to
		// track both original key presses (kEventRawKeyModifiersChanged) and
		// key releases (kEventRawKeyUp) to know for sure, and both events
		// are documented as having a "kEventParamKeyModifiers" parameter
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, gCarbonEventModifiers);
		
		// if the modifier key information was found, proceed
		if (noErr == result)
		{
			if (NO == [NSApp isActive]) result = eventNotHandledErr;
		}
		else
		{
			// no modifier data available; assume no modifiers are in use!
			gCarbonEventModifiers = 0L;
		}
	}
	return result;
}// updateModifiers

} // anonymous namespace


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
		_terminalWindowTouchBarController = nil; // created on demand
	}
	return self;
}// init


/*!
Destructor.

(2016.11)
*/
- (void)
dealloc
{
	[_terminalWindowTouchBarController release];
	[super dealloc];
}// dealloc


#pragma mark Actions


/*!
TEMPORARY.

This implements an action that needs to be forwarded to
a Carbon window by starting from an object known to
Interface Builder.  In this case, the Commands_Executor
forwarding class knows how to find the active Carbon
window correctly (whereas “first responder” may not).

To be removed when Carbon windows are retired.

(2016.11)
*/
- (IBAction)
toggleFullScreen:(id)	sender
{
	[[Commands_Executor sharedExecutor] toggleFullScreen:sender];
}// toggleFullScreen:


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
				MacHelpUtilities_LaunchHelpSystemWithSearch(BRIDGE_CAST(helpSearch, CFStringRef));
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
	NSTouchBar*		result = nil;
	
	
	// in order to be able to return a Touch Bar for a Carbon window,
	// the application instance acts as the first responder in the
	// chain and returns an appropriate bar (this approach is
	// temporary; when using a pure Cocoa terminal window, it will
	// make more sense to move the Touch Bar to that controller)
	if (nullptr != TerminalWindow_ReturnFromMainWindow())
	{
		if (nil == _terminalWindowTouchBarController)
		{
			_terminalWindowTouchBarController = [[TouchBar_Controller alloc] initWithNibName:@"TerminalWindowTouchBarCocoa"];
			_terminalWindowTouchBarController.customizationIdentifier = kConstantsRegistry_TouchBarIDTerminalWindowMain;
			_terminalWindowTouchBarController.customizationAllowedItemIdentifiers =
			@[
				kConstantsRegistry_TouchBarItemIDFind,
				kConstantsRegistry_TouchBarItemIDFullScreen,
				FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFlexibleSpace",
								NSTouchBarItemIdentifierFlexibleSpace),
				FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFixedSpaceSmall",
								NSTouchBarItemIdentifierFixedSpaceSmall),
				FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFixedSpaceLarge",
								NSTouchBarItemIdentifierFixedSpaceLarge)
			];
			// (NOTE: default item identifiers are set in the NIB)
		}
		
		// the controller should force the NIB to load and define
		// the Touch Bar, using the settings above and in the NIB
		result = _terminalWindowTouchBarController.touchBar;
		assert(nil != result);
	}
	else
	{
		if (nil == _applicationTouchBarController)
		{
			_applicationTouchBarController = [[TouchBar_Controller alloc] initWithNibName:@"ApplicationTouchBarCocoa"];
			_applicationTouchBarController.customizationIdentifier = kConstantsRegistry_TouchBarIDApplicationMain;
			_applicationTouchBarController.customizationAllowedItemIdentifiers =
			@[
				FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFlexibleSpace",
								NSTouchBarItemIdentifierFlexibleSpace),
				FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFixedSpaceSmall",
								NSTouchBarItemIdentifierFixedSpaceSmall),
				FUTURE_SYMBOL(@"NSTouchBarItemIdentifierFixedSpaceLarge",
								NSTouchBarItemIdentifierFixedSpaceLarge)
			];
			// (NOTE: default item identifiers are set in the NIB)
		}
		
		// the controller should force the NIB to load and define
		// the Touch Bar, using the settings above and in the NIB
		result = _applicationTouchBarController.touchBar;
		assert(nil != result);
	}
	
	return result;
}// makeTouchBar


@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
