/*!	\file Keypads.mm
	\brief Cocoa implementation of floating keypads.
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

#import "Keypads.h"

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <SoundSystem.h>

// application includes
#import "Commands.h"
#import "Session.h"
#import "SessionFactory.h"
#import "VTKeys.h"

// Swift imports
#import <MacTermQuills/MacTermQuills-Swift.h>



#pragma mark Types

/*!
Implements the Arrange Window panel.
*/
@interface Keypads_ArrangeWindowPanelController : NSWindowController <UIArrangeWindow_ActionHandling> //{
{
	UIArrangeWindow_Model*		_viewModel;
}

// class methods
	+ (instancetype)
	sharedArrangeWindowPanelController;

// accessors
	- (UIArrangeWindow_Model*)
	viewModel;

// new methods
	- (void)
	setOriginToX:(int)_
	andY:(int)_;

@end //}


/*!
Implements the Control Keys palette.
*/
@interface Keypads_ControlKeysPanelController : NSWindowController <UIKeypads_ActionHandling> //{
{
	UIKeypads_Model*	_viewModel;
}

// class methods
	+ (instancetype)
	sharedControlKeysPanelController;

// accessors
	- (UIKeypads_Model*)
	viewModel;

@end //}


/*!
Implements the multi-layout Function Keys palette.
*/
@interface Keypads_FunctionKeysPanelController : NSWindowController <UIKeypads_ActionHandling> //{
{
	UIKeypads_Model*	_viewModel;
}

// class methods
	+ (instancetype)
	sharedFunctionKeysPanelController;

// accessors
	- (UIKeypads_Model*)
	viewModel;

@end //}


/*!
Implements the VT220 Keypad palette.
*/
@interface Keypads_VT220KeysPanelController : NSWindowController <UIKeypads_ActionHandling> //{
{
	UIKeypads_Model*	_viewModel;
}

// class methods
	+ (instancetype)
	sharedVT220KeysPanelController;

// accessors
	- (UIKeypads_Model*)
	viewModel;

@end //}

#pragma mark Internal Method Prototypes
namespace {

SessionRef	getCurrentSession	();
void		sendCharacter		(UInt8);
void		sendKey				(UInt8);

}// anonymous namespace

#pragma mark Variables
namespace {

Preferences_ContextRef		gArrangeWindowBindingContext = nullptr;
Preferences_Tag				gArrangeWindowBinding = 0;
Preferences_Tag				gArrangeWindowScreenBinding = 0;
FourCharCode				gArrangeWindowDataTypeForWindowBinding = kPreferences_DataTypeCGPoint;
FourCharCode				gArrangeWindowDataTypeForScreenBinding = kPreferences_DataTypeHIRect;
CGPoint						gArrangeWindowStackingOrigin = CGPointZero;
void						(^gArrangeWindowDidEndBlock)(void) = nil;
NSObject< Keypads_ControlKeyResponder >*		gControlKeysResponder = nil;
Boolean						gControlKeysMayAutoHide = false;
Session_FunctionKeyLayout	gFunctionKeysLayout = kSession_FunctionKeyLayoutVT220;

}// anonymous namespace



#pragma mark Public Methods

/*!
Binds the “Arrange Window” panel to a preferences tag, which
determines both the source of its initial window position and
the destination that is auto-updated as the window is moved.

This variant accepts a block to invoke when the user has
finished setting the arrangement value.

Set the screen binding information to nonzero values to also
keep track of the boundaries of the display that contains most
of the window.  This is usually important to save somewhere as
well, because it allows you to intelligently restore the window
if the user has resized the screen after the window was saved.

The window frame binding data type is currently allowed to be
"kPreferences_DataTypeCGPoint" or "kPreferences_DataTypeHIRect".
The "inWindowBindingOrZero" tag must be documented as expecting
the corresponding type!  If a type is rectangular, the width
and height are set to 0, but the origin is still set according
to the window position.

The screen binding must be "kPreferences_DataTypeHIRect" and
the origin and size are both defined.

Set "inBinding" to 0 to have no binding effect.

(2021.01)
*/
void
Keypads_SetArrangeWindowPanelBinding	(void						(^inDidEndBlock)(void),
										 Preferences_Tag			inWindowBindingOrZero,
										 FourCharCode				inDataTypeForWindowBinding,
										 Preferences_Tag			inScreenBindingOrZero,
										 FourCharCode				inDataTypeForScreenBinding,
										 Preferences_ContextRef		inContextOrNull)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Preferences_ContextRef	newContext = inContextOrNull;
	UInt16 const			kDefaultX = 128; // arbitrary
	UInt16 const			kDefaultY = 128; // arbitrary
	
	
	assert((kPreferences_DataTypeCGPoint == inDataTypeForWindowBinding) || (kPreferences_DataTypeHIRect == inDataTypeForWindowBinding));
	assert((0 == inDataTypeForScreenBinding) || (kPreferences_DataTypeHIRect == inDataTypeForScreenBinding));
	
	if (nullptr == newContext)
	{
		prefsResult = Preferences_GetDefaultContext(&newContext);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	assert(nullptr != newContext);
	if (newContext != gArrangeWindowBindingContext)
	{
		Preferences_RetainContext(newContext);
		if (nullptr != gArrangeWindowBindingContext)
		{
			Preferences_ReleaseContext(&gArrangeWindowBindingContext);
		}
		gArrangeWindowBindingContext = newContext;
	}
	
	gArrangeWindowBinding = inWindowBindingOrZero;
	gArrangeWindowDataTypeForWindowBinding = inDataTypeForWindowBinding;
	gArrangeWindowScreenBinding = inScreenBindingOrZero;
	gArrangeWindowDataTypeForScreenBinding = inDataTypeForScreenBinding;
	
	gArrangeWindowDidEndBlock = inDidEndBlock;
	
	if (0 != gArrangeWindowBinding)
	{
		if (kPreferences_DataTypeCGPoint == gArrangeWindowDataTypeForWindowBinding)
		{
			prefsResult = Preferences_ContextGetData(gArrangeWindowBindingContext, gArrangeWindowBinding,
														sizeof(gArrangeWindowStackingOrigin), &gArrangeWindowStackingOrigin,
														false/* search defaults */);
			if (kPreferences_ResultOK != prefsResult)
			{
				gArrangeWindowStackingOrigin = CGPointMake(kDefaultX, kDefaultY); // assume a default, if preference can’t be found
			}
		}
		else if (kPreferences_DataTypeHIRect == gArrangeWindowDataTypeForWindowBinding)
		{
			Preferences_TopLeftCGRect	prefValue;
			
			
			prefsResult = Preferences_ContextGetData(gArrangeWindowBindingContext, gArrangeWindowBinding,
														sizeof(prefValue), &prefValue, false/* search defaults */);
			if (kPreferences_ResultOK == prefsResult)
			{
				gArrangeWindowStackingOrigin.x = prefValue.origin.x;
				gArrangeWindowStackingOrigin.y = prefValue.origin.y;
			}
			else
			{
				gArrangeWindowStackingOrigin = CGPointMake(kDefaultX, kDefaultY); // assume a default, if preference can’t be found
			}
		}
	}
}// SetArrangeWindowPanelBinding (7 arguments)


/*!
The historical variant, which does not support Objective-C.

(4.0)
*/
void
Keypads_SetArrangeWindowPanelBinding	(Preferences_Tag			inWindowBindingOrZero,
										 FourCharCode				inDataTypeForWindowBinding,
										 Preferences_Tag			inScreenBindingOrZero,
										 FourCharCode				inDataTypeForScreenBinding,
										 Preferences_ContextRef		inContextOrNull)
{
	Keypads_SetArrangeWindowPanelBinding(^{}, inWindowBindingOrZero, inDataTypeForWindowBinding,
											inScreenBindingOrZero, inDataTypeForScreenBinding,
											inContextOrNull);
}// SetArrangeWindowPanelBinding (5 arguments)


/*!
Returns true only if the “Function Keys” keypad is using
the specified layout.

(2020.09)
*/
Boolean
Keypads_IsFunctionKeyLayoutEqualTo	(Session_FunctionKeyLayout	inLayout)
{
	Boolean				result = false;
	UIKeypads_Model*	viewModel = [Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController].viewModel;
	
	
	result = (viewModel.functionKeyLayout == inLayout);
	
	return result;
}// IsFunctionKeyLayoutEqualTo


/*!
Returns true only if the specified panel is showing.
Windows that have been minimized into the Dock are still
considered visible.

(3.1)
*/
Boolean
Keypads_IsVisible	(Keypads_WindowType		inKeypad)
{
@autoreleasepool {
	Boolean		result = false;
	
	
	switch (inKeypad)
	{
	case kKeypads_WindowTypeArrangeWindow:
		result = (YES == [[[Keypads_ArrangeWindowPanelController sharedArrangeWindowPanelController] window] isVisible]);
		break;
	
	case kKeypads_WindowTypeControlKeys:
		result = ((YES == [[[Keypads_ControlKeysPanelController sharedControlKeysPanelController] window] isVisible]) ||
					(YES == [[[Keypads_ControlKeysPanelController sharedControlKeysPanelController] window] isMiniaturized]));
		break;
	
	case kKeypads_WindowTypeFunctionKeys:
		result = ((YES == [[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] window] isVisible]) ||
					(YES == [[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] window] isMiniaturized]));
		break;
	
	case kKeypads_WindowTypeVT220Keys:
		result = ((YES == [[[Keypads_VT220KeysPanelController sharedVT220KeysPanelController] window] isVisible]) ||
					(YES == [[[Keypads_VT220KeysPanelController sharedVT220KeysPanelController] window] isMiniaturized]));
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// @autoreleasepool
}// IsVisible


/*!
If the current responder is the given target, it is removed;
otherwise, this call has no effect.  (Requiring you to
specify the previous responder prevents you from accidentally
removing a responder attachment that you are not aware of.)

Once a responder is removed, the default behavior (using a
session window) is restored and the palette is hidden if it
was only made visible originally by the responder’s
attachment.  If the user had the palette open when the
original attachment was made, the palette remains visible.

Note that the current responder is also removed automatically
if Keypads_SetResponder() is called to set a new one.

(4.1)
*/
void
Keypads_RemoveResponder		(Keypads_WindowType		inFromKeypad,
							 NSObject*				inCurrentTarget)
{
	switch (inFromKeypad)
	{
	case kKeypads_WindowTypeControlKeys:
		if (gControlKeysResponder == inCurrentTarget)
		{
			gControlKeysResponder = nil;
			if (gControlKeysMayAutoHide)
			{
				Keypads_SetVisible(inFromKeypad, false);
				gControlKeysMayAutoHide = false;
			}
		}
		break;
	
	case kKeypads_WindowTypeArrangeWindow:
	case kKeypads_WindowTypeFunctionKeys:
	case kKeypads_WindowTypeVT220Keys:
	default:
		break;
	}
}// RemoveResponder


/*!
Update the button names and layout of the “Function Keys”
keypad, based on the specified layout.

(2020.09)
*/
void
Keypads_SetFunctionKeyLayout	(Session_FunctionKeyLayout	inLayout)
{
	UIKeypads_Model*	viewModel = [Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController].viewModel;
	
	
	// update UI and save to preferences (triggers "saveChangesWithViewModel:")
	viewModel.functionKeyLayout = inLayout;
}// SetFunctionKeyLayout


/*!
Arranges for button presses to send a message to the given
object instead of causing characters to be typed into the
active terminal session.  The given object must respond to
a selector of this form:
	controlKeypadSentCharacterCode:(NSNumber*)	asciiCode
(Ordinarily this might be defined as a protocol but the
header is currently included by regular C++ code.)

Optionally, the responder may also implement the method
"controlKeypadHidden" to detect when the keypad is hidden by
the user.

If an object is provided, the keypad window is automatically
displayed.  Currently, only the control keys palette type
(kKeypads_WindowTypeControlKeys) is recognized.

Passing a responder of "nil" has no effect.  To remove a
responder, call Keypads_RemoveResponder().

You can pass "nil" as the target to set no target, in which
case the default behavior (using a session window) is
restored and the palette is hidden if there is no user
preference to keep it visible.

(4.1)
*/
void
Keypads_SetResponder	(Keypads_WindowType		inFromKeypad,
						 NSObject*				inCurrentTarget)
{
	if (nil != inCurrentTarget)
	{
		switch (inFromKeypad)
		{
		case kKeypads_WindowTypeControlKeys:
			if ([inCurrentTarget conformsToProtocol:@protocol(Keypads_ControlKeyResponder)])
			{
				auto		asResponder = STATIC_CAST(inCurrentTarget, NSObject< Keypads_ControlKeyResponder >*);
				
				
				gControlKeysResponder = asResponder;
			}
			else
			{
				NSLog(@"keypad responder does not conform to protocol correctly: %@", inCurrentTarget);
			}
			if (false == Keypads_IsVisible(kKeypads_WindowTypeControlKeys))
			{
				Keypads_SetVisible(inFromKeypad, true);
				gControlKeysMayAutoHide = true;
			}
			break;
		
		case kKeypads_WindowTypeArrangeWindow:
		case kKeypads_WindowTypeFunctionKeys:
		case kKeypads_WindowTypeVT220Keys:
		default:
			break;
		}
	}
}// SetResponder


/*!
Shows or hides the specified keypad panel.

This call will clear the "gControlKeysMayAutoHide" flag.
If the intent is to programmatically display a keypad
in order to temporarily control another panel (such as
a preferences pane), gControlKeysMayAutoHide should be
set to true afterwards.

(3.1)
*/
void
Keypads_SetVisible	(Keypads_WindowType		inKeypad,
					 Boolean				inIsVisible)
{
@autoreleasepool {
	switch (inKeypad)
	{
	case kKeypads_WindowTypeArrangeWindow:
		if (inIsVisible)
		{
			[[Keypads_ArrangeWindowPanelController sharedArrangeWindowPanelController]
				setOriginToX:STATIC_CAST(gArrangeWindowStackingOrigin.x, SInt16)
								andY:STATIC_CAST(gArrangeWindowStackingOrigin.y, SInt16)];
			[[Keypads_ArrangeWindowPanelController sharedArrangeWindowPanelController] showWindow:NSApp];
		}
		else
		{
			[[Keypads_ArrangeWindowPanelController sharedArrangeWindowPanelController] close];
		}
		break;
	
	case kKeypads_WindowTypeControlKeys:
		if (inIsVisible)
		{
			[[Keypads_ControlKeysPanelController sharedControlKeysPanelController] showWindow:NSApp];
		}
		else
		{
			[[Keypads_ControlKeysPanelController sharedControlKeysPanelController] close];
		}
		gControlKeysMayAutoHide = false;
		break;
	
	case kKeypads_WindowTypeFunctionKeys:
		if (inIsVisible)
		{
			[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] showWindow:NSApp];
		}
		else
		{
			[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] close];
		}
		break;
	
	case kKeypads_WindowTypeVT220Keys:
		if (inIsVisible)
		{
			[[Keypads_VT220KeysPanelController sharedVT220KeysPanelController] showWindow:NSApp];
		}
		else
		{
			[[Keypads_VT220KeysPanelController sharedVT220KeysPanelController] close];
		}
		break;
	
	default:
		// ???
		break;
	}
}// @autoreleasepool
}// SetVisible


#pragma mark Internal Methods
namespace {

/*!
This routine consistently determines the session where all
keypad commands will go.  Note that since Cocoa floating windows
can have the user focus, the appropriate terminal window MAY NOT
always be the user focus window!

(3.0)
*/
SessionRef
getCurrentSession ()
{
	return SessionFactory_ReturnUserRecentSession();
}// getCurrentSession


/*!
If Keypads_SetResponder() has been called, the message
"controlKeypadSentCharacterCode:" is sent to that view
with the given value (as an NSNumber*).

Otherwise, this finds the appropriate target session and
sends the specified character, as if the user had typed
that character.

(2020.09)
*/
void
sendCharacter	(UInt8		inCharacter)
{
	if (nil != gControlKeysResponder)
	{
		if ([gControlKeysResponder respondsToSelector:@selector(controlKeypadSentCharacterCode:)])
		{
			[gControlKeysResponder controlKeypadSentCharacterCode:[NSNumber numberWithChar:inCharacter]];
		}
	}
	else
	{
		SessionRef		currentSession = getCurrentSession();
		
		
		if (nullptr != currentSession)
		{
			Session_UserInputKey(currentSession, inCharacter);
		}
	}
}// sendCharacter


/*!
Finds the appropriate target session and sends the character
represented by the specified virtual key, as if the user had
typed that key.  The key code should match those that are
valid for Session_UserInputKey().

(2020.09)
*/
void
sendFunctionKey	(UInt8						inNumber,
				 Session_FunctionKeyLayout	inLayout)
{
	SessionRef		currentSession = getCurrentSession();
	
	
	if (nullptr != currentSession)
	{
		Session_Result		sessionResult = Session_UserInputFunctionKey(currentSession, inNumber, inLayout);
		
		
		if (kSession_ResultOK != sessionResult)
		{
			Sound_StandardAlert();
		}
	}
}// sendFunctionKey


/*!
Finds the appropriate target session and sends the character
represented by the specified virtual key, as if the user had
typed that key.  The key code should match those that are
valid for Session_UserInputKey().

NOTE:	For historical reasons it is possible to transmit
		certain function key codes this way, but it is better
		to use "sendFunctionKeyWithIndex:forLayout:" for
		function keys.

(2020.09)
*/
void
sendKey	(UInt8		inKey)
{
	SessionRef		currentSession = getCurrentSession();
	
	
	if (nullptr != currentSession)
	{
		Session_Result		sessionResult = Session_UserInputKey(currentSession, inKey);
		
		
		if (kSession_ResultOK != sessionResult)
		{
			Sound_StandardAlert();
		}
	}
}// sendKey

}// anonymous namespace


#pragma mark -
@implementation Keypads_ArrangeWindowPanelController //{


static Keypads_ArrangeWindowPanelController*	gKeypads_ArrangeWindowPanelController = nil;


#pragma mark Class Methods


/*!
Returns the singleton.

(2020.09)
*/
+ (id)
sharedArrangeWindowPanelController
{
	static dispatch_once_t		onceToken;
	
	
	dispatch_once(&onceToken,
	^{
		gKeypads_ArrangeWindowPanelController = [[self.class allocWithZone:NULL] init];
	});
	return gKeypads_ArrangeWindowPanelController;
}// sharedArrangeWindowPanelController


#pragma mark Initializers


/*!
Designated initializer.

(2020.09)
*/
- (instancetype)
init
{
	UIArrangeWindow_Model*	viewModel = [[UIArrangeWindow_Model alloc] initWithRunner:self];
	NSViewController*		viewController = [UIArrangeWindow_ObjC makeViewController:viewModel];
	NSPanel*				panelObject = [NSPanel windowWithContentViewController:viewController];
	
	
	panelObject.styleMask = (NSWindowStyleMaskTitled |
								NSWindowStyleMaskUtilityWindow);
	panelObject.title = NSLocalizedString(@"Arrange Window", @"title for floating window that specifies window location preferences");
	
	self = [super initWithWindow:panelObject];
	if (nil != self)
	{
		self->_viewModel = viewModel;
	}
	return self;
}// init


#pragma mark Accessors


/*!
Returns the object that binds to the UI.

(2020.09)
*/
- (UIArrangeWindow_Model*)
viewModel
{
	return _viewModel;
}// viewModel


#pragma mark New Methods


/*!
Moves the window to the specified position (which is actually
expressed relative to the top-left corner).

(4.0)
*/
- (void)
setOriginToX:(int)	x
andY:(int)			y // this is flipped to be measured from the top
{
	NSWindow*	window = [self window];
	NSScreen*	screen = [window screen];
	NSPoint		origin;
	
	
	origin.x = x;
	origin.y = [screen frame].size.height - [window frame].size.height - y;
	[[self window] setFrameOrigin:origin];
}// setOriginToX:andY:


#pragma mark UIArrangeWindow_ActionHandling


/*!
Invoked when the user has dismissed the window that represents
a window location preference.

(2020.09)
*/
- (void)
doneArrangingWithViewModel:(UIArrangeWindow_Model*)		viewModel
{
#pragma unused(viewModel)
	if (0 != gArrangeWindowBinding)
	{
		NSWindow*			window = [self window];
		NSScreen*			screen = [window screen];
		NSRect				windowFrame = [window frame];
		NSRect				screenFrame = [screen frame];
		SInt16				x = STATIC_CAST(windowFrame.origin.x, SInt16);
		SInt16				y = STATIC_CAST(screenFrame.size.height - windowFrame.size.height - windowFrame.origin.y, SInt16);
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		
		
		if (kPreferences_DataTypeCGPoint == gArrangeWindowDataTypeForWindowBinding)
		{
			CGPoint		prefValue = CGPointMake(x, y);
			
			
			prefsResult = Preferences_ContextSetData(gArrangeWindowBindingContext, gArrangeWindowBinding, sizeof(prefValue), &prefValue);
		}
		else if (kPreferences_DataTypeHIRect == gArrangeWindowDataTypeForWindowBinding)
		{
			Preferences_TopLeftCGRect	prefValue;
			
			
			prefValue.origin.x = x;
			prefValue.origin.y = y;
			prefValue.size.width = 0;
			prefValue.size.height = 0;
			prefsResult = Preferences_ContextSetData(gArrangeWindowBindingContext, gArrangeWindowBinding, sizeof(prefValue), &prefValue);
		}
		else
		{
			assert(false && "incorrect window location binding type");
		}
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "failed to set window origin preference");
		}
		
		if (0 != gArrangeWindowScreenBinding)
		{
			if (kPreferences_DataTypeHIRect == gArrangeWindowDataTypeForScreenBinding)
			{
				Preferences_TopLeftCGRect	prefValue;
				
				
				// TEMPORARY: the coordinate system should probably be flipped here
				prefValue.origin.x = screenFrame.origin.x;
				prefValue.origin.y = screenFrame.origin.y;
				prefValue.size.width = screenFrame.size.width;
				prefValue.size.height = screenFrame.size.height;
				prefsResult = Preferences_ContextSetData(gArrangeWindowBindingContext, gArrangeWindowScreenBinding,
															sizeof(prefValue), &prefValue);
				if (kPreferences_ResultOK != prefsResult)
				{
					Console_Warning(Console_WriteValue, "failed to set preference associated with arrange-window panel, error", prefsResult);
				}
			}
			else
			{
				assert(false && "incorrect screen rectangle binding type");
			}
		}
		
		if (nil != gArrangeWindowDidEndBlock)
		{
			gArrangeWindowDidEndBlock();
		}
	}
	Keypads_SetVisible(kKeypads_WindowTypeArrangeWindow, false);
	
	// release the binding
	if (nullptr != gArrangeWindowBindingContext)
	{
		Preferences_ReleaseContext(&gArrangeWindowBindingContext);
	}
}// doneArranging


@end //} Keypads_ArrangeWindowPanelController


#pragma mark -
@implementation Keypads_ControlKeysPanelController //{


static Keypads_ControlKeysPanelController*		gKeypads_ControlKeysPanelController = nil;


/*!
Returns the singleton.

(2020.09)
*/
+ (id)
sharedControlKeysPanelController
{
	static dispatch_once_t		onceToken;
	
	
	dispatch_once(&onceToken,
	^{
		gKeypads_ControlKeysPanelController = [[self.class allocWithZone:NULL] init];
	});
	return gKeypads_ControlKeysPanelController;
}// sharedControlKeysPanelController


/*!
Designated initializer.

(2020.09)
*/
- (instancetype)
init
{
	UIKeypads_Model*	viewModel = [[UIKeypads_Model alloc] initWithRunner:self];
	NSViewController*	viewController = [UIKeypads_ObjC makeControlKeysViewController:viewModel];
	NSPanel*			panelObject = [NSPanel windowWithContentViewController:viewController];
	
	
	panelObject.styleMask = (NSWindowStyleMaskTitled |
								NSWindowStyleMaskClosable |
								NSWindowStyleMaskMiniaturizable |
								NSWindowStyleMaskUtilityWindow);
	panelObject.title = NSLocalizedString(@"Control Keys", @"title for floating keypad that shows control keys");
	
	self = [super initWithWindow:panelObject];
	if (nil != self)
	{
		self->_viewModel = viewModel;
		
		self.windowFrameAutosaveName = @"ControlKeys"; // (for backward compatibility, never change this)
		
		[self whenObject:self.window postsNote:NSWindowWillCloseNotification
							performSelector:@selector(windowWillClose:)];
	}
	return self;
}// init


/*!
Destructor.

(2020.09)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
}// dealloc


#pragma mark Accessors


/*!
Returns the object that binds to the UI.

(2020.09)
*/
- (UIKeypads_Model*)
viewModel
{
	return _viewModel;
}// viewModel


#pragma mark UIKeypads_ActionHandling


/*!
Responds to user invocation of keypad keys.

(This implementation only responds to keys that are
actually part of this palette; the UIKeypads_KeyID
enumeration has values spanning multiple keypads.)

(2020.09)
*/
- (void)
respondToActionWithViewModel:(UIKeypads_Model*)		aViewModel
keyID:(UIKeypads_KeyID)								aKeyID
{
	switch (aKeyID)
	{
	case UIKeypads_KeyIDControlNull:
		sendCharacter(0x00);
		break;
	
	case UIKeypads_KeyIDControlA:
		sendCharacter(0x01);
		break;
	
	case UIKeypads_KeyIDControlB:
		sendCharacter(0x02);
		break;
	
	case UIKeypads_KeyIDControlC:
		sendCharacter(0x03);
		break;
	
	case UIKeypads_KeyIDControlD:
		sendCharacter(0x04);
		break;
	
	case UIKeypads_KeyIDControlE:
		sendCharacter(0x05);
		break;
	
	case UIKeypads_KeyIDControlF:
		sendCharacter(0x06);
		break;
	
	case UIKeypads_KeyIDControlG:
		sendCharacter(0x07);
		break;
	
	case UIKeypads_KeyIDControlH:
		sendCharacter(0x08);
		break;
	
	case UIKeypads_KeyIDControlI:
		sendCharacter(0x09);
		break;
	
	case UIKeypads_KeyIDControlJ:
		sendCharacter(0x0A);
		break;
	
	case UIKeypads_KeyIDControlK:
		sendCharacter(0x0B);
		break;
	
	case UIKeypads_KeyIDControlL:
		sendCharacter(0x0C);
		break;
	
	case UIKeypads_KeyIDControlM:
		sendCharacter(0x0D);
		break;
	
	case UIKeypads_KeyIDControlN:
		sendCharacter(0x0E);
		break;
	
	case UIKeypads_KeyIDControlO:
		sendCharacter(0x0F);
		break;
	
	case UIKeypads_KeyIDControlP:
		sendCharacter(0x10);
		break;
	
	case UIKeypads_KeyIDControlQ:
		sendCharacter(0x11);
		break;
	
	case UIKeypads_KeyIDControlR:
		sendCharacter(0x12);
		break;
	
	case UIKeypads_KeyIDControlS:
		sendCharacter(0x13);
		break;
	
	case UIKeypads_KeyIDControlT:
		sendCharacter(0x14);
		break;
	
	case UIKeypads_KeyIDControlU:
		sendCharacter(0x15);
		break;
	
	case UIKeypads_KeyIDControlV:
		sendCharacter(0x16);
		break;
	
	case UIKeypads_KeyIDControlW:
		sendCharacter(0x17);
		break;
	
	case UIKeypads_KeyIDControlX:
		sendCharacter(0x18);
		break;
	
	case UIKeypads_KeyIDControlY:
		sendCharacter(0x19);
		break;
	
	case UIKeypads_KeyIDControlZ:
		sendCharacter(0x1A);
		break;
	
	case UIKeypads_KeyIDControlLeftSquareBracket:
		sendCharacter(0x1B);
		break;
	
	case UIKeypads_KeyIDControlBackslash:
		sendCharacter(0x1C);
		break;
	
	case UIKeypads_KeyIDControlRightSquareBracket:
		sendCharacter(0x1D);
		break;
	
	case UIKeypads_KeyIDControlCaret:
		sendCharacter(0x1E);
		break;
	
	case UIKeypads_KeyIDControlUnderscore:
		sendCharacter(0x1F);
		break;
	
	default:
		Console_Warning(Console_WriteValue, "“control keys” keypad not handling keyID", (int)aKeyID);
		break;
	}
}// respondToActionWithViewModel:keyID:


/*!
Writes any settings to preferences, if appropriate.

(2020.09)
*/
- (void)
saveChangesWithViewModel:(UIKeypads_Model*)		aViewModel
{
	// (nothing needed here)
}// saveChangesWithViewModel:


#pragma mark NSWindowDelegate


/*!
If a responder has been set for the keypad then that
responder is notified that the keypad is being hidden.

(2020.09)
*/
- (void)
windowWillClose:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	if (nil != gControlKeysResponder)
	{
		if ([gControlKeysResponder respondsToSelector:@selector(controlKeypadHidden)])
		{
			[gControlKeysResponder controlKeypadHidden];
		}
	}
}// windowWillClose:


@end //} Keypads_ControlKeysPanelController


#pragma mark -
@implementation Keypads_FunctionKeysPanelController //{


static Keypads_FunctionKeysPanelController*		gKeypads_FunctionKeysPanelController = nil;


/*!
Returns the singleton.

(2020.09)
*/
+ (id)
sharedFunctionKeysPanelController
{
	static dispatch_once_t		onceToken;
	
	
	dispatch_once(&onceToken,
	^{
		gKeypads_FunctionKeysPanelController = [[self.class allocWithZone:NULL] init];
		
		// force the window to load because its settings are needed
		// elsewhere in the user interface (e.g. menu items)
		UNUSED_RETURN(NSWindow*)[gKeypads_FunctionKeysPanelController window];
	});
	return gKeypads_FunctionKeysPanelController;
}// sharedFunctionKeysPanelController


/*!
Designated initializer.

(2020.09)
*/
- (instancetype)
init
{
	UIKeypads_Model*	viewModel = [[UIKeypads_Model alloc] initWithRunner:self];
	NSViewController*	viewController = [UIKeypads_ObjC makeFunctionKeysViewController:viewModel];
	NSPanel*			panelObject = [NSPanel windowWithContentViewController:viewController];
	
	
	panelObject.styleMask = (NSWindowStyleMaskTitled |
								NSWindowStyleMaskClosable |
								NSWindowStyleMaskMiniaturizable |
								NSWindowStyleMaskUtilityWindow);
	panelObject.title = NSLocalizedString(@"Function Keys", @"title for floating keypad that shows function keys");
	
	self = [super initWithWindow:panelObject];
	if (nil != self)
	{
		self->_viewModel = viewModel;
		
		// update the menu and the global variable based on user preferences
		{
			Preferences_Result		prefsResult = Preferences_GetData(kPreferences_TagFunctionKeyLayout,
																		sizeof(gFunctionKeysLayout), &gFunctionKeysLayout);
			
			
			if (kPreferences_ResultOK != prefsResult)
			{
				Console_Warning(Console_WriteLine, "no existing setting for function key layout was found");
				gFunctionKeysLayout = kSession_FunctionKeyLayoutVT220;
			}
			
			self.viewModel.functionKeyLayout = gFunctionKeysLayout;
		}
		
		self.windowFrameAutosaveName = @"FunctionKeys"; // (for backward compatibility, never change this)
	}
	return self;
}// init


#pragma mark Accessors


/*!
Returns the object that binds to the UI.

(2020.09)
*/
- (UIKeypads_Model*)
viewModel
{
	return _viewModel;
}// viewModel


#pragma mark UIKeypads_ActionHandling


/*!
Responds to user invocation of keypad keys.

(This implementation only responds to keys that are
actually part of this palette; the UIKeypads_KeyID
enumeration has values spanning multiple keypads.)

(2020.09)
*/
- (void)
respondToActionWithViewModel:(UIKeypads_Model*)		aViewModel
keyID:(UIKeypads_KeyID)								aKeyID
{
	switch (aKeyID)
	{
	case UIKeypads_KeyIDF1:
		sendFunctionKey(1, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF2:
		sendFunctionKey(2, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF3:
		sendFunctionKey(3, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF4:
		sendFunctionKey(4, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF5:
		sendFunctionKey(5, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF6:
		sendFunctionKey(6, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF7:
		sendFunctionKey(7, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF8:
		sendFunctionKey(8, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF9:
		sendFunctionKey(9, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF10:
		sendFunctionKey(10, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF11:
		sendFunctionKey(11, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF12:
		sendFunctionKey(12, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF13:
		sendFunctionKey(13, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF14:
		sendFunctionKey(14, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF15:
		sendFunctionKey(15, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF16:
		sendFunctionKey(16, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF17:
		sendFunctionKey(17, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF18:
		sendFunctionKey(18, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF19:
		sendFunctionKey(19, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF20:
		sendFunctionKey(20, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF21:
		sendFunctionKey(21, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF22:
		sendFunctionKey(22, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF23:
		sendFunctionKey(23, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF24:
		sendFunctionKey(24, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF25:
		sendFunctionKey(25, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF26:
		sendFunctionKey(26, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF27:
		sendFunctionKey(27, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF28:
		sendFunctionKey(28, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF29:
		sendFunctionKey(29, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF30:
		sendFunctionKey(30, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF31:
		sendFunctionKey(31, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF32:
		sendFunctionKey(32, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF33:
		sendFunctionKey(33, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF34:
		sendFunctionKey(34, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF35:
		sendFunctionKey(35, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF36:
		sendFunctionKey(36, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF37:
		sendFunctionKey(37, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF38:
		sendFunctionKey(38, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF39:
		sendFunctionKey(39, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF40:
		sendFunctionKey(40, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF41:
		sendFunctionKey(41, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF42:
		sendFunctionKey(42, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF43:
		sendFunctionKey(43, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF44:
		sendFunctionKey(44, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF45:
		sendFunctionKey(45, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF46:
		sendFunctionKey(46, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF47:
		sendFunctionKey(47, gFunctionKeysLayout);
		break;
	
	case UIKeypads_KeyIDF48:
		sendFunctionKey(48, gFunctionKeysLayout);
		break;
	
	default:
		Console_Warning(Console_WriteValue, "“function keys” keypad not handling keyID", (int)aKeyID);
		break;
	}
}// respondToActionWithViewModel:keyID:


/*!
Writes any settings to preferences, if appropriate.

(2020.09)
*/
- (void)
saveChangesWithViewModel:(UIKeypads_Model*)		aViewModel
{
	Session_FunctionKeyLayout	savedValue = aViewModel.functionKeyLayout;
	
	
	// save function key layout
	if (savedValue != gFunctionKeysLayout)
	{
		Preferences_Result		prefsResult = Preferences_SetData(kPreferences_TagFunctionKeyLayout,
																	sizeof(savedValue), &savedValue);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "unable to save function key layout");
		}
		
		// retain this setting
		gFunctionKeysLayout = savedValue;
	}
}// saveChangesWithViewModel:


@end //} Keypads_FunctionKeysPanelController


#pragma mark -
@implementation Keypads_VT220KeysPanelController


static Keypads_VT220KeysPanelController*	gKeypads_VT220KeysPanelController = nil;


/*!
Returns the singleton.

(2020.09)
*/
+ (id)
sharedVT220KeysPanelController
{
	static dispatch_once_t		onceToken;
	
	
	dispatch_once(&onceToken,
	^{
		gKeypads_VT220KeysPanelController = [[self.class allocWithZone:NULL] init];
	});
	return gKeypads_VT220KeysPanelController;
}// sharedVT220KeysPanelController


/*!
Designated initializer.

(2020.09)
*/
- (instancetype)
init
{
	UIKeypads_Model*	viewModel = [[UIKeypads_Model alloc] initWithRunner:self];
	NSViewController*	viewController = [UIKeypads_ObjC makeVT220KeysViewController:viewModel];
	NSPanel*			panelObject = [NSPanel windowWithContentViewController:viewController];
	
	
	panelObject.styleMask = (NSWindowStyleMaskTitled |
								NSWindowStyleMaskClosable |
								NSWindowStyleMaskMiniaturizable |
								NSWindowStyleMaskUtilityWindow);
	panelObject.title = NSLocalizedString(@"VT220 Keypad", @"title for floating keypad that shows VT220 keypad");
	
	self = [super initWithWindow:panelObject];
	if (nil != self)
	{
		self->_viewModel = viewModel;
		
		self.windowFrameAutosaveName = @"VT220Keys"; // (for backward compatibility, never change this)
	}
	return self;
}// init


#pragma mark Accessors


/*!
Returns the object that binds to the UI.

(2020.09)
*/
- (UIKeypads_Model*)
viewModel
{
	return _viewModel;
}// viewModel


#pragma mark UIKeypads_ActionHandling


/*!
Responds to user invocation of keypad keys.

(This implementation only responds to keys that are
actually part of this palette; the UIKeypads_KeyID
enumeration has values spanning multiple keypads.)

(2020.09)
*/
- (void)
respondToActionWithViewModel:(UIKeypads_Model*)		aViewModel
keyID:(UIKeypads_KeyID)								aKeyID
{
	switch (aKeyID)
	{
	case UIKeypads_KeyIDKeypad0:
		sendKey(VSK0);
		break;
	
	case UIKeypads_KeyIDKeypad1:
		sendKey(VSK1);
		break;
	
	case UIKeypads_KeyIDKeypad2:
		sendKey(VSK2);
		break;
	
	case UIKeypads_KeyIDKeypad3:
		sendKey(VSK3);
		break;
	
	case UIKeypads_KeyIDKeypad4:
		sendKey(VSK4);
		break;
	
	case UIKeypads_KeyIDKeypad5:
		sendKey(VSK5);
		break;
	
	case UIKeypads_KeyIDKeypad6:
		sendKey(VSK6);
		break;
	
	case UIKeypads_KeyIDKeypad7:
		sendKey(VSK7);
		break;
	
	case UIKeypads_KeyIDKeypad8:
		sendKey(VSK8);
		break;
	
	case UIKeypads_KeyIDKeypad9:
		sendKey(VSK9);
		break;
	
	case UIKeypads_KeyIDArrowDown:
		sendKey(VSDN);
		break;
	
	case UIKeypads_KeyIDArrowLeft:
		sendKey(VSLT);
		break;
	
	case UIKeypads_KeyIDArrowRight:
		sendKey(VSRT);
		break;
	
	case UIKeypads_KeyIDArrowUp:
		sendKey(VSUP);
		break;
	
	case UIKeypads_KeyIDKeypadComma:
		sendKey(VSKC);
		break;
	
	case UIKeypads_KeyIDKeypadDecimalPoint:
		sendKey(VSKP);
		break;
	
	case UIKeypads_KeyIDKeypadDelete:
		sendKey(VSPGUP_220DEL);
		break;
	
	case UIKeypads_KeyIDKeypadEnter:
		sendKey(VSKE);
		break;
	
	case UIKeypads_KeyIDKeypadFind:
		sendKey(VSHELP_220FIND);
		break;
	
	case UIKeypads_KeyIDKeypadHyphen:
		sendKey(VSKM);
		break;
	
	case UIKeypads_KeyIDKeypadInsert:
		sendKey(VSHOME_220INS);
		break;
	
	case UIKeypads_KeyIDKeypadPageDown:
		sendKey(VSPGDN_220PGDN);
		break;
	
	case UIKeypads_KeyIDKeypadPageUp:
		sendKey(VSEND_220PGUP);
		break;
	
	case UIKeypads_KeyIDKeypadPF1:
		sendKey(VSF1);
		break;
	
	case UIKeypads_KeyIDKeypadPF2:
		sendKey(VSF2);
		break;
	
	case UIKeypads_KeyIDKeypadPF3:
		sendKey(VSF3);
		break;
	
	case UIKeypads_KeyIDKeypadPF4:
		sendKey(VSF4);
		break;
	
	case UIKeypads_KeyIDKeypadSelect:
		sendKey(VSDEL_220SEL);
		break;
	
	default:
		Console_Warning(Console_WriteValue, "“VT220 keys” keypad not handling keyID", (int)aKeyID);
		break;
	}
}// respondToActionWithViewModel:keyID:


/*!
Writes any settings to preferences, if appropriate.

(2020.09)
*/
- (void)
saveChangesWithViewModel:(UIKeypads_Model*)		aViewModel
{
	// (nothing needed here)
}// saveChangesWithViewModel:


@end // Keypads_VT220KeysPanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
