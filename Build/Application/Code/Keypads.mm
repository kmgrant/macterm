/*!	\file Keypads.mm
	\brief Cocoa implementation of floating keypads.
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

#import "Keypads.h"

// Mac includes
#import <Cocoa/Cocoa.h>

// standard-C++ includes
#import <map>

// library includes
#import <AutoPool.objc++.h>
#import <Console.h>
#import <SoundSystem.h>

// application includes
#import "Commands.h"
#import "Session.h"
#import "SessionFactory.h"
#import "TranslucentMenuArrow.h"
#import "VTKeys.h"



#pragma mark Types
namespace {

typedef std::map< NSButton*, unsigned int >		My_IntsByButton;

} // anonymous namespace

#pragma mark Variables
namespace {

Preferences_ContextRef		gArrangeWindowBindingContext = nullptr;
Preferences_Tag				gArrangeWindowBinding = 0;
Preferences_Tag				gArrangeWindowScreenBinding = 0;
FourCharCode				gArrangeWindowDataTypeForWindowBinding = typeQDPoint;
FourCharCode				gArrangeWindowDataTypeForScreenBinding = typeHIRect;
Point						gArrangeWindowStackingOrigin = { 0, 0 };
EventTargetRef				gControlKeysEventTarget = nullptr;	//!< temporary, for Carbon interaction
id							gControlKeysResponder = nil;
Boolean						gControlKeysMayAutoHide = false;
Session_FunctionKeyLayout	gFunctionKeysLayout = kSession_FunctionKeyLayoutVT220;
My_IntsByButton&			gAnimationStagesByButton ()		{ static My_IntsByButton x; return x; }
Float32						gDeltasFontBlowUpAnimation[] =
							{
								// font size deltas during each animation frame;
								// WARNING: must be the same size as "gDelaysFontBlowUpAnimation"
								+1.0,
								+1.0,
								+1.0,
								+1.0,
								+1.0,
								-1.0,
								-1.0,
								-1.0,
								-1.0,
								-1.0
							};
NSTimeInterval				gDelaysFontBlowUpAnimation[] =
							{
								// delays between animation frames of the font size animation;
								// WARNING: must be the same size as "gDeltasFontBlowUpAnimation"
								0.01,
								0.01,
								0.02,
								0.04,
								0.06,
								0.05,
								0.04,
								0.03,
								0.01,
								0.01
							};

}// anonymous namespace



#pragma mark Public Methods

/*!
Binds the “Arrange Window” panel to a preferences tag, which
determines both the source of its initial window position and
the destination that is auto-updated as the window is moved.

Set the screen binding information to nonzero values to also
keep track of the boundaries of the display that contains most
of the window.  This is usually important to save somewhere as
well, because it allows you to intelligently restore the window
if the user has resized the screen after the window was saved.

The window frame binding data type is currently allowed to be
one of the following: "typeQDPoint" (Point), "typeHIRect"
(HIRect).  The "inWindowBindingOrZero" tag must be documented as
expecting the corresponding type!  If a type is rectangular, the
width and height are set to 0, but the origin is still set
according to the window position.

The screen binding must currently always be "typeHIRect", and
the origin and size are both defined.

Set "inBinding" to 0 to have no binding effect.

(4.0)
*/
void
Keypads_SetArrangeWindowPanelBinding	(Preferences_Tag			inWindowBindingOrZero,
										 FourCharCode				inDataTypeForWindowBinding,
										 Preferences_Tag			inScreenBindingOrZero,
										 FourCharCode				inDataTypeForScreenBinding,
										 Preferences_ContextRef		inContextOrNull)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Preferences_ContextRef	newContext = inContextOrNull;
	size_t					actualSize = 0;
	UInt16 const			kDefaultX = 128; // arbitrary
	UInt16 const			kDefaultY = 128; // arbitrary
	
	
	assert((typeQDPoint == inDataTypeForWindowBinding) || (typeHIRect == inDataTypeForWindowBinding));
	assert((0 == inDataTypeForScreenBinding) || (typeHIRect == inDataTypeForScreenBinding));
	
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
	
	if (0 != gArrangeWindowBinding)
	{
		if (typeQDPoint == gArrangeWindowDataTypeForWindowBinding)
		{
			prefsResult = Preferences_ContextGetData(gArrangeWindowBindingContext, gArrangeWindowBinding,
														sizeof(gArrangeWindowStackingOrigin), &gArrangeWindowStackingOrigin,
														false/* search defaults */, &actualSize);
			if (kPreferences_ResultOK != prefsResult)
			{
				SetPt(&gArrangeWindowStackingOrigin, kDefaultX, kDefaultY); // assume a default, if preference can’t be found
			}
		}
		else if (typeHIRect == gArrangeWindowDataTypeForWindowBinding)
		{
			HIRect		prefValue;
			
			
			prefsResult = Preferences_ContextGetData(gArrangeWindowBindingContext, gArrangeWindowBinding,
														sizeof(prefValue), &prefValue, false/* search defaults */, &actualSize);
			if (kPreferences_ResultOK == prefsResult)
			{
				gArrangeWindowStackingOrigin.h = prefValue.origin.x;
				gArrangeWindowStackingOrigin.v = prefValue.origin.y;
			}
			else
			{
				SetPt(&gArrangeWindowStackingOrigin, kDefaultX, kDefaultY); // assume a default, if preference can’t be found
			}
		}
	}
}// SetArrangeWindowPanelBinding


/*!
Returns true only if the specified panel is showing.
Windows that have been minimized into the Dock are still
considered visible.

(3.1)
*/
Boolean
Keypads_IsVisible	(Keypads_WindowType		inKeypad)
{
	AutoPool	_;
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
	
	case kKeypads_WindowTypeFullScreen:
		result = (YES == [[[Keypads_FullScreenPanelController sharedFullScreenPanelController] window] isVisible]);
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
}// IsVisible


/*!
If the current target is the given target, it is removed;
otherwise, this call has no effect.  (Requiring you to
specify the previous target prevents you from accidentally
removing a target attachment that you are not aware of.)

Once a target is removed, the default behavior (using a
session window) is restored and the palette is hidden if
it was only made visible originally by the target’s
attachment.  If the user had the palette open when the
original attachment was made, the palette remains visible.

Note that the current target is also removed automatically
if Keypads_SetEventTarget() is called to set a new one.

(4.1)
*/
void
Keypads_RemoveEventTarget	(Keypads_WindowType		inFromKeypad,
							 EventTargetRef			inCurrentTarget)
{
	switch (inFromKeypad)
	{
	case kKeypads_WindowTypeControlKeys:
		if (gControlKeysEventTarget == inCurrentTarget)
		{
			gControlKeysEventTarget = nullptr;
			if (gControlKeysMayAutoHide)
			{
				Keypads_SetVisible(inFromKeypad, false);
				gControlKeysMayAutoHide = false;
			}
		}
		break;
	
	case kKeypads_WindowTypeArrangeWindow:
	case kKeypads_WindowTypeFullScreen:
	case kKeypads_WindowTypeFunctionKeys:
	case kKeypads_WindowTypeVT220Keys:
	default:
		break;
	}
}// RemoveEventTarget


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
	case kKeypads_WindowTypeFullScreen:
	case kKeypads_WindowTypeFunctionKeys:
	case kKeypads_WindowTypeVT220Keys:
	default:
		break;
	}
}// RemoveResponder


/*!
Sets the current event target for button commands issued
by the specified keypad, and automatically shows or hides
the keypad window.  Currently, only the control keys palette
(kKeypads_WindowTypeControlKeys) is recognized.

Passing a responder of "nullptr" has no effect.  To remove
a responder, call Keypads_RemoveResponder().

Any Cocoa responder previously set by Keypads_SetResponder()
is automatically cleared.

IMPORTANT:	This is for Carbon compatibility and is not a
			long-term solution.

(3.1)
*/
void
Keypads_SetEventTarget	(Keypads_WindowType		inFromKeypad,
						 EventTargetRef			inCurrentTarget)
{
	if (nullptr != inCurrentTarget)
	{
		switch (inFromKeypad)
		{
		case kKeypads_WindowTypeControlKeys:
			gControlKeysResponder = nil; // clear this because it will be ignored now anyway
			gControlKeysEventTarget = inCurrentTarget;
			if (false == Keypads_IsVisible(kKeypads_WindowTypeControlKeys))
			{
				Keypads_SetVisible(inFromKeypad, true);
				gControlKeysMayAutoHide = true;
			}
			break;
		
		case kKeypads_WindowTypeArrangeWindow:
		case kKeypads_WindowTypeFullScreen:
		case kKeypads_WindowTypeFunctionKeys:
		case kKeypads_WindowTypeVT220Keys:
		default:
			break;
		}
	}
}// SetEventTarget


/*!
Arranges for button presses to send a message to the given
object instead of causing characters to be typed into the
active terminal session.  The given object must respond to
a selector of this form:
	controlKeypadSentCharacterCode:(NSNumber*)	asciiCode
(Ordinarily this might be defined as a protocol but the
header is currently included by regular C++ code.)

If an object is provided, the keypad window is automatically
displayed.  Currently, only the control keys palette type
(kKeypads_WindowTypeControlKeys) is recognized.

Passing a responder of "nil" has no effect.  To remove a
responder, call Keypads_RemoveResponder().

You can pass "nil" as the target to set no target, in which
case the default behavior (using a session window) is
restored and the palette is hidden if there is no user
preference to keep it visible.

Any Carbon target previously set by Keypads_SetEventTarget()
is automatically cleared.

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
			gControlKeysEventTarget = nullptr; // clear this because it will be ignored now anyway
			gControlKeysResponder = inCurrentTarget;
			if (false == Keypads_IsVisible(kKeypads_WindowTypeControlKeys))
			{
				Keypads_SetVisible(inFromKeypad, true);
				gControlKeysMayAutoHide = true;
			}
			break;
		
		case kKeypads_WindowTypeArrangeWindow:
		case kKeypads_WindowTypeFullScreen:
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
	AutoPool		_;
	HIWindowRef		oldActiveWindow = GetUserFocusWindow();
	
	
	switch (inKeypad)
	{
	case kKeypads_WindowTypeArrangeWindow:
		if (inIsVisible)
		{
			[[Keypads_ArrangeWindowPanelController sharedArrangeWindowPanelController] setOriginToX:gArrangeWindowStackingOrigin.h
																						andY:gArrangeWindowStackingOrigin.v];
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
	
	case kKeypads_WindowTypeFullScreen:
		if (inIsVisible)
		{
			[[Keypads_FullScreenPanelController sharedFullScreenPanelController] showWindow:NSApp];
		}
		else
		{
			[[Keypads_FullScreenPanelController sharedFullScreenPanelController] close];
		}
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
	
	if (oldActiveWindow != GetUserFocusWindow())
	{
		(OSStatus)SetUserFocusWindow(oldActiveWindow);
	}
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

}// anonymous namespace


@implementation Keypads_ArrangeWindowPanelController


static Keypads_ArrangeWindowPanelController*	gKeypads_ArrangeWindowPanelController = nil;


/*!
Returns the singleton.

(4.0)
*/
+ (id)
sharedArrangeWindowPanelController
{
	if (nil == gKeypads_ArrangeWindowPanelController)
	{
		gKeypads_ArrangeWindowPanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gKeypads_ArrangeWindowPanelController;
}// sharedArrangeWindowPanelController


/*!
Designated initializer.

(4.0)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"KeypadArrangeWindowCocoa"];
	return self;
}// init



/*!
Invoked when the user has dismissed the window that represents
a window location preference.

(4.0)
*/
- (IBAction)
doneArranging:(id)	sender
{
#pragma unused(sender)
	if (0 != gArrangeWindowBinding)
	{
		NSWindow*			window = [self window];
		NSScreen*			screen = [window screen];
		NSRect				windowFrame = [window frame];
		NSRect				screenFrame = [screen frame];
		SInt16				x = STATIC_CAST(windowFrame.origin.x, SInt32);
		SInt16				y = STATIC_CAST(screenFrame.size.height - windowFrame.size.height - windowFrame.origin.y, SInt32);
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		
		
		if (typeQDPoint == gArrangeWindowDataTypeForWindowBinding)
		{
			Point		prefValue;
			
			
			SetPt(&prefValue, x, y);
			prefsResult = Preferences_ContextSetData(gArrangeWindowBindingContext, gArrangeWindowBinding, sizeof(prefValue), &prefValue);
		}
		else if (typeHIRect == gArrangeWindowDataTypeForWindowBinding)
		{
			HIRect		prefValue;
			
			
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
			if (typeHIRect == gArrangeWindowDataTypeForScreenBinding)
			{
				HIRect		prefValue;
				
				
				// TEMPORARY: the coordinate system should probably be flipped here
				prefValue.origin.x = screenFrame.origin.x;
				prefValue.origin.y = screenFrame.origin.y;
				prefValue.size.width = screenFrame.size.width;
				prefValue.size.height = screenFrame.size.height;
				prefsResult = Preferences_ContextSetData(gArrangeWindowBindingContext, gArrangeWindowScreenBinding,
															sizeof(prefValue), &prefValue);
			}
			else
			{
				assert(false && "incorrect screen rectangle binding type");
			}
		}
	}
	Keypads_SetVisible(kKeypads_WindowTypeArrangeWindow, false);
	
	// release the binding
	if (nullptr != gArrangeWindowBindingContext)
	{
		Preferences_ReleaseContext(&gArrangeWindowBindingContext);
	}
}// doneArranging


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


@end // Keypads_ArrangeWindowPanelController


@implementation Keypads_PanelController


/*!
Designated initializer.

Subclasses should set "baseFontSize" in their "windowDidLoad"
implementation, according to the current "pointSize" of the
"font" of one of the window’s buttons.

(4.0)
*/
- (id)
initWithWindowNibName:(NSString*)	aName
{
	self = [super initWithWindowNibName:aName];
	if (nil != self)
	{
		self->baseFontSize = 13.0; // arbitrary, some sane value; should reset later in "windowDidLoad"
	}
	return self;
}// initWithWindowNibName:


/*!
Changes the size of the font of the specified button by the
amount currently set for its size delta, and then reinstalls a
delayed call to this method.

If the font size rises an arbitrary amount beyond the saved
"self->baseFontSize", the delta is reversed so that it will
return to the base size eventually.  Once it reaches that size,
no more delayed calls are installed.

Therefore the goal of the overall animation is to blow up the
font size slightly before bouncing back to the original size.

IMPORTANT:	Subclasses should set "self->baseFontSize" as
			described in "initWithWindowNibName:"; the default
			value may not be adequate.

(4.0)
*/
- (void)
deltaSizeFontOfButton:(NSButton*)	aButton
{
	My_IntsByButton::iterator	toPair = gAnimationStagesByButton().find(aButton);
	
	
	if (gAnimationStagesByButton().end() != toPair)
	{
		unsigned int const	kOriginalIndex = toPair->second;
		float const			kCurrentSize = [[aButton font] pointSize];
		float const			kFontSizeDelta = gDeltasFontBlowUpAnimation[kOriginalIndex];
		NSTimeInterval		kDelay = gDelaysFontBlowUpAnimation[kOriginalIndex];
		float				newSize = kCurrentSize;
		
		
		// alter the font size; once the animation frames are exhausted, stop
		newSize += kFontSizeDelta;
		if ((newSize < self->baseFontSize) || (toPair->second >= (sizeof(gDeltasFontBlowUpAnimation) - 1)) ||
			(newSize > (self->baseFontSize + 10/* arbitrary */)))
		{
			newSize = self->baseFontSize;
			gAnimationStagesByButton().erase(aButton);
		}
		
		// nudge the button’s label to a new font size
		{
			NSFont*		resizedFont = [NSFont fontWithName:[[aButton font] fontName] size:newSize];
			
			
			[aButton setFont:resizedFont];
			
			toPair = gAnimationStagesByButton().find(aButton);
			if (gAnimationStagesByButton().end() != toPair)
			{
				// as long as the button has an animation frame associated with it,
				// arrange to call this method again (delaying each animation frame)
				toPair->second = 1 + kOriginalIndex;
				[self performSelector:@selector(deltaSizeFontOfButton:) withObject:aButton afterDelay:kDelay];
			}
		}
	}
}// deltaSizeFontOfButton:


/*!
Animates the specified button to highlight a change.

IMPORTANT:	Subclasses should set "self->baseFontSize" as
			described in "initWithWindowNibName:"; the default
			value may not be adequate.

(4.0)
*/
- (void)
drawAttentionToButton:(NSButton*)	aButton
{
	gAnimationStagesByButton()[aButton] = 0;
	[self deltaSizeFontOfButton:aButton];
}// drawAttentionToButton:


/*!
If Keypads_SetResponder() has been called, the message
"controlKeypadSentCharacterCode:" is sent to that view
with the given value (as an NSNumber*).

Otherwise, this finds the appropriate target session and
sends the specified character, as if the user had typed
that character.

(3.1)
*/
- (void)
sendCharacter:(UInt8)	inCharacter
{
	if (nil != gControlKeysResponder)
	{
		if ([gControlKeysResponder respondsToSelector:@selector(controlKeypadSentCharacterCode:)])
		{
			[gControlKeysResponder performSelector:@selector(controlKeypadSentCharacterCode:)
													withObject:[NSNumber numberWithChar:inCharacter]];
		}
		else
		{
			NSLog(@"keypad responder does not respond to selector 'controlKeypadSentCharacterCode:': %@",
					gControlKeysResponder);
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
}// sendCharacter:


/*!
Finds the appropriate target session and sends the character
represented by the specified virtual key, as if the user had
typed that key.  The key code should match those that are
valid for Session_UserInputKey().

(4.0)
*/
- (void)
sendFunctionKeyWithIndex:(UInt8)		inNumber
forLayout:(Session_FunctionKeyLayout)	inLayout
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
}// sendFunctionKeyWithIndex:forLayout:


/*!
Finds the appropriate target session and sends the character
represented by the specified virtual key, as if the user had
typed that key.  The key code should match those that are
valid for Session_UserInputKey().

NOTE:	For historical reasons it is possible to transmit
		certain function key codes this way, but it is better
		to use "sendFunctionKeyWithIndex:forLayout:" for
		function keys.

(3.1)
*/
- (void)
sendKey:(UInt8)		inKey
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
}// sendKey:


@end // Keypads_PanelController


@implementation Keypads_ControlKeysPanelController


static Keypads_ControlKeysPanelController*		gKeypads_ControlKeysPanelController = nil;


/*!
Returns the singleton.

(3.1)
*/
+ (id)
sharedControlKeysPanelController
{
	if (nil == gKeypads_ControlKeysPanelController)
	{
		gKeypads_ControlKeysPanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gKeypads_ControlKeysPanelController;
}// sharedControlKeysPanelController


/*!
Designated initializer.

(3.1)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"KeypadControlKeysCocoa"];
	return self;
}// init


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeNull:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x00];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlAtSign, gControlKeysEventTarget);
}// typeNull:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlA:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x01];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlA, gControlKeysEventTarget);
}// typeControlA:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlB:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x02];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlB, gControlKeysEventTarget);
}// typeControlB:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlC:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x03];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlC, gControlKeysEventTarget);
}// typeControlC:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlD:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x04];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlD, gControlKeysEventTarget);
}// typeControlD:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlE:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x05];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlE, gControlKeysEventTarget);
}// typeControlE:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlF:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x06];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlF, gControlKeysEventTarget);
}// typeControlF:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlG:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x07];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlG, gControlKeysEventTarget);
}// typeControlG:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlH:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x08];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlH, gControlKeysEventTarget);
}// typeControlH:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlI:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x09];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlI, gControlKeysEventTarget);
}// typeControlI:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlJ:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0A];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlJ, gControlKeysEventTarget);
}// typeControlJ:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlK:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0B];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlK, gControlKeysEventTarget);
}// typeControlK:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlL:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0C];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlL, gControlKeysEventTarget);
}// typeControlL:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlM:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0D];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlM, gControlKeysEventTarget);
}// typeControlM:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlN:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0E];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlN, gControlKeysEventTarget);
}// typeControlN:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlO:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0F];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlO, gControlKeysEventTarget);
}// typeControlO:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlP:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x10];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlP, gControlKeysEventTarget);
}// typeControlP:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlQ:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x11];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlQ, gControlKeysEventTarget);
}// typeControlQ:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlR:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x12];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlR, gControlKeysEventTarget);
}// typeControlR:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlS:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x13];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlS, gControlKeysEventTarget);
}// typeControlS:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlT:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x14];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlT, gControlKeysEventTarget);
}// typeControlT:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlU:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x15];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlU, gControlKeysEventTarget);
}// typeControlU:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlV:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x16];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlV, gControlKeysEventTarget);
}// typeControlV:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlW:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x17];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlW, gControlKeysEventTarget);
}// typeControlW:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlX:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x18];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlX, gControlKeysEventTarget);
}// typeControlX:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlY:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x19];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlY, gControlKeysEventTarget);
}// typeControlY:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlZ:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1A];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlZ, gControlKeysEventTarget);
}// typeControlZ:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlLeftSquareBracket:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1B];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlLeftSquareBracket, gControlKeysEventTarget);
}// typeControlLeftSquareBracket:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlBackslash:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1C];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlBackslash, gControlKeysEventTarget);
}// typeControlBackslash:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlRightSquareBracket:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1D];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlRightSquareBracket, gControlKeysEventTarget);
}// typeControlRightSquareBracket:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlCaret:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1E];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlCaret, gControlKeysEventTarget);
}// typeControlCaret:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlUnderscore:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1F];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlUnderscore, gControlKeysEventTarget);
}// typeControlUnderscore:


#pragma mark NSWindowController


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"ControlKeys";
}// windowFrameAutosaveName


@end // Keypads_ControlKeysPanelController


@implementation Keypads_FullScreenPanelController


static Keypads_FullScreenPanelController*		gKeypads_FullScreenPanelController = nil;


/*!
Returns the singleton.

(3.1)
*/
+ (id)
sharedFullScreenPanelController
{
	if (nil == gKeypads_FullScreenPanelController)
	{
		gKeypads_FullScreenPanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gKeypads_FullScreenPanelController;
}// sharedFullScreenPanelController


/*!
Designated initializer.

(3.1)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"KeypadFullScreenCocoa"];
	return self;
}// init


/*!
Turns off Full Screen mode.

(3.1)
*/
- (IBAction)
disableFullScreen:(id)	sender
{
#pragma unused(sender)
	// since the floating window may be focused, an attempt to send an
	// event to the user focus could “miss” a terminal window and hit
	// the application target (where no command handler for full-screen
	// is installed); so, first find an appropriate window to focus
	(OSStatus)SetUserFocusWindow(GetFrontWindowOfClass(kDocumentWindowClass, true/* must be visible */));
	
	Commands_ExecuteByIDUsingEvent(kCommandKioskModeDisable, gControlKeysEventTarget);
}// disableFullScreen:


#pragma mark NSWindowController


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"FullScreenControls";
}// windowFrameAutosaveName


@end // Keypads_FullScreenPanelController


@implementation Keypads_FunctionKeysPanelController


static Keypads_FunctionKeysPanelController*		gKeypads_FunctionKeysPanelController = nil;


/*!
Returns the singleton.

(3.1)
*/
+ (id)
sharedFunctionKeysPanelController
{
	if (nil == gKeypads_FunctionKeysPanelController)
	{
		gKeypads_FunctionKeysPanelController = [[[self class] allocWithZone:NULL] init];
		
		// force the window to load because its settings are needed
		// elsewhere in the user interface (e.g. menu items)
		(NSWindow*)[gKeypads_FunctionKeysPanelController window];
	}
	return gKeypads_FunctionKeysPanelController;
}// sharedFunctionKeysPanelController


/*!
Designated initializer.

(3.1)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"KeypadFunctionKeysCocoa"];
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidBecomeKeyNotification object:[self window]];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidResizeNotification object:[self window]];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:[self window]];
	[menuChildWindow release];
	[super dealloc];
}// dealloc


#pragma mark Internal Methods


/*!
Returns the size of the pop-up menu button at a zero origin.

(4.0)
*/
- (NSRect)
popUpMenuButtonBounds
{
	NSRect		result = NSMakeRect(0, 0, 28, 16);
	
	
	return result;
}// popUpMenuButtonBounds


/*!
Returns the screen rectangle that should be occupied by the
invisible window that renders a pop-up menu arrow.

(4.0)
*/
- (NSRect)
popUpMenuButtonFrame
{
	// determine the location of the menu; this cannot be set up in
	// Interface Builder too easily because it must appear to be in
	// the title bar of the window (everything is handled with a
	// frameless and transparent window at the right location)
	NSRect		result = [[self window] frame];
	NSRect		innerFrame = [self popUpMenuButtonBounds];
	Float32		minWidth = [NSWindow minFrameWidthWithTitle:[[self window] title] styleMask:[[self window] styleMask]];
	
	
	result.origin.x += (result.size.width + minWidth) / 2.0;
	result.origin.y += (result.size.height - innerFrame.size.height);
	result.size = innerFrame.size;
	
	// TEMPORARY, basically a hack: the frame width measurement
	// for the title positions the menu a bit too far from the
	// title text, so move the menu slightly closer
	result.origin.x -= 32; // arbitrary
	
	return result;
}// popUpMenuButtonFrame


/*!
If YES, then the key at the F1 location is called “PF1”;
otherwise, it is named “F1”.

Do not call this until the window is loaded, because the
state is maintained entirely within the user interface
element.

This is an adornment only and has no effect on the actual
behavior of the button!  Be sure to keep the function key
layout setting in sync.

(4.0)
*/
- (void)
setF1IsPF1:(BOOL)	isPF1
{
	NSButton*	target = self->functionKeyF1;
	NSString*	oldTitle = [target title];
	
	
	if (isPF1)
	{
		[target setTitle:NSLocalizedString(@"PF1", @"name for function key PF1")];
	}
	else
	{
		[target setTitle:NSLocalizedString(@"F1", @"name for function key F1")];
	}
	
	if (NO == [oldTitle isEqualToString:[target title]])
	{
		// make the change a bit more obvious
		[self drawAttentionToButton:target];
	}
}// setF1IsPF1:


/*!
If YES, then the key at the F2 location is called “PF2”;
otherwise, it is named “F2”.

Do not call this until the window is loaded, because the
state is maintained entirely within the user interface
element.

This is an adornment only and has no effect on the actual
behavior of the button!  Be sure to keep the function key
layout setting in sync.

(4.0)
*/
- (void)
setF2IsPF2:(BOOL)	isPF2
{
	NSButton*	target = self->functionKeyF2;
	NSString*	oldTitle = [target title];
	
	
	if (isPF2)
	{
		[target setTitle:NSLocalizedString(@"PF2", @"name for function key PF2")];
	}
	else
	{
		[target setTitle:NSLocalizedString(@"F2", @"name for function key F2")];
	}
	
	if (NO == [oldTitle isEqualToString:[target title]])
	{
		// make the change a bit more obvious
		[self drawAttentionToButton:target];
	}
}// setF2IsPF2:


/*!
If YES, then the key at the F3 location is called “PF3”;
otherwise, it is named “F3”.

Do not call this until the window is loaded, because the
state is maintained entirely within the user interface
element.

This is an adornment only and has no effect on the actual
behavior of the button!  Be sure to keep the function key
layout setting in sync.

(4.0)
*/
- (void)
setF3IsPF3:(BOOL)	isPF3
{
	NSButton*	target = self->functionKeyF3;
	NSString*	oldTitle = [target title];
	
	
	if (isPF3)
	{
		[target setTitle:NSLocalizedString(@"PF3", @"name for function key PF3")];
	}
	else
	{
		[target setTitle:NSLocalizedString(@"F3", @"name for function key F3")];
	}
	
	if (NO == [oldTitle isEqualToString:[target title]])
	{
		// make the change a bit more obvious
		[self drawAttentionToButton:target];
	}
}// setF3IsPF3:


/*!
If YES, then the key at the F4 location is called “PF4”;
otherwise, it is named “F4”.

Do not call this until the window is loaded, because the
state is maintained entirely within the user interface
element.

This is an adornment only and has no effect on the actual
behavior of the button!  Be sure to keep the function key
layout setting in sync.

(4.0)
*/
- (void)
setF4IsPF4:(BOOL)	isPF4
{
	NSButton*	target = self->functionKeyF4;
	NSString*	oldTitle = [target title];
	
	
	if (isPF4)
	{
		[target setTitle:NSLocalizedString(@"PF4", @"name for function key PF4")];
	}
	else
	{
		[target setTitle:NSLocalizedString(@"F4", @"name for function key F4")];
	}
	
	if (NO == [oldTitle isEqualToString:[target title]])
	{
		// make the change a bit more obvious
		[self drawAttentionToButton:target];
	}
}// setF4IsPF4:


/*!
If YES, then the key at the F15 location becomes a help key;
otherwise, it is named “F15”.

Do not call this until the window is loaded, because the
state is maintained entirely within the user interface
element.

This is an adornment only and has no effect on the actual
behavior of the button!  Be sure to keep the function key
layout setting in sync.

(4.0)
*/
- (void)
setF15IsHelp:(BOOL)		isHelp
{
	NSButton*	target = self->functionKeyF15;
	NSString*	oldTitle = [target title];
	
	
	if (isHelp)
	{
		[target setTitle:NSLocalizedString(@"?", @"name for the help function key")];
		[target setToolTip:NSLocalizedString(@"Help", @"tool tip for the help function key")];
	}
	else
	{
		[target setTitle:NSLocalizedString(@"F15", @"name for function key F15")];
		[target setToolTip:nil];
	}
	
	if (NO == [oldTitle isEqualToString:[target title]])
	{
		// make the change a bit more obvious
		[self drawAttentionToButton:target];
	}
}// setF15IsHelp:


/*!
If YES, then the key at the F16 location becomes the “do” key;
otherwise, it is named “F16”.

Do not call this until the window is loaded, because the
state is maintained entirely within the user interface
element.

This is an adornment only and has no effect on the actual
behavior of the button!  Be sure to keep the function key
layout setting in sync.

(4.0)
*/
- (void)
setF16IsDo:(BOOL)	isDo
{
	NSButton*	target = self->functionKeyF16;
	NSString*	oldTitle = [target title];
	
	
	if (isDo)
	{
		[target setTitle:NSLocalizedString(@"do", @"name for the do function key")];
	}
	else
	{
		[target setTitle:NSLocalizedString(@"F16", @"name for function key F16")];
	}
	
	if (NO == [oldTitle isEqualToString:[target title]])
	{
		// make the change a bit more obvious
		[self drawAttentionToButton:target];
	}
}// setF16IsDo:


/*!
Helper method to update certain button labels appropriately
for the “rxvt” terminal.

(4.0)
*/
- (void)
setLabelsRxvt:(id)	anArgument
{
#pragma unused(anArgument)
	[self setF1IsPF1:NO];
	[self setF2IsPF2:NO];
	[self setF3IsPF3:NO];
	[self setF4IsPF4:NO];
	[self setF15IsHelp:YES];
	[self setF16IsDo:YES];
}// setLabelsRxvt:


/*!
Helper method to update certain button labels appropriately
for the VT220 layout.

(4.0)
*/
- (void)
setLabelsVT220:(id)		anArgument
{
#pragma unused(anArgument)
	[self setF1IsPF1:YES];
	[self setF2IsPF2:YES];
	[self setF3IsPF3:YES];
	[self setF4IsPF4:YES];
	[self setF15IsHelp:YES];
	[self setF16IsDo:YES];
}// setLabelsVT220:


/*!
Helper method to update certain button labels appropriately
for the “xterm” X11 layout.

(4.0)
*/
- (void)
setLabelsXTermX11:(id)	anArgument
{
#pragma unused(anArgument)
	[self setF1IsPF1:NO];
	[self setF2IsPF2:NO];
	[self setF3IsPF3:NO];
	[self setF4IsPF4:NO];
	[self setF15IsHelp:NO];
	[self setF16IsDo:NO];
}// setLabelsXTermX11:


/*!
Helper method to update certain button labels appropriately
for the “xterm” X11 layout.

(4.0)
*/
- (void)
setLabelsXTermXFree86:(id)	anArgument
{
#pragma unused(anArgument)
	[self setF1IsPF1:YES];
	[self setF2IsPF2:YES];
	[self setF3IsPF3:YES];
	[self setF4IsPF4:YES];
	[self setF15IsHelp:NO];
	[self setF16IsDo:NO];
}// setLabelsXTermXFree86:


#pragma mark Public Methods


/*!
Returns the function key layout currently in use.

WARNING:	This returns the same for any controller right now
			because the setting is global.

(4.0)
*/
- (Session_FunctionKeyLayout)
currentFunctionKeyLayout
{
	return gFunctionKeysLayout;
}// currentFunctionKeyLayout


/*!
Changes the keyboard layout to match the “rxvt” terminal.

(4.0)
*/
- (IBAction)
performSetFunctionKeyLayoutRxvt:(id)	sender
{
#pragma unused(sender)
	Session_FunctionKeyLayout const		kLayout = kSession_FunctionKeyLayoutRxvt;
	NSTimeInterval						resizeTime = 0;
	
	
	[layoutMenu selectItemAtIndex:[layoutMenu indexOfItemWithTarget:self andAction:@selector(performSetFunctionKeyLayoutRxvt:)]];
	
	if (kLayout != gFunctionKeysLayout)
	{
		Preferences_Result		prefsResult = Preferences_SetData(kPreferences_TagFunctionKeyLayout,
																	sizeof(kLayout), &kLayout);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "unable to save function key layout");
		}
		
		// resize the window to reveal all 48 keys
		{
			NSRect	frame = [[self window] frame];
			
			
			frame.size = [[self window] maxSize];
			[[self window] setFrame:frame display:YES animate:YES];
			resizeTime = [[self window] animationResizeTime:frame];
		}
		
		// retain this setting
		gFunctionKeysLayout = kLayout;
	}
	
	// update labels; this is done always for the convenience of the initialization code
	[self performSelector:@selector(setLabelsRxvt:) withObject:nil afterDelay:resizeTime];
}// performSetFunctionKeyLayoutRxvt:


/*!
Changes the keyboard layout to match a traditional VT220
terminal, plus mappings for VT100 function keys.

(4.0)
*/
- (IBAction)
performSetFunctionKeyLayoutVT220:(id)	sender
{
#pragma unused(sender)
	Session_FunctionKeyLayout const		kLayout = kSession_FunctionKeyLayoutVT220;
	NSTimeInterval						resizeTime = 0;
	
	
	[layoutMenu selectItemAtIndex:[layoutMenu indexOfItemWithTarget:self andAction:@selector(performSetFunctionKeyLayoutVT220:)]];
	
	if (kLayout != gFunctionKeysLayout)
	{
		Preferences_Result		prefsResult = Preferences_SetData(kPreferences_TagFunctionKeyLayout,
																	sizeof(kLayout), &kLayout);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "unable to save function key layout");
		}
		
		// resize the window to hide XTerm-specific keys
		{
			NSRect	frame = [[self window] frame];
			
			
			frame.size = [[self window] minSize];
			[[self window] setFrame:frame display:YES animate:YES];
			resizeTime = [[self window] animationResizeTime:frame];
		}
		
		// retain this setting
		gFunctionKeysLayout = kLayout;
	}
	
	// update labels; this is done always for the convenience of the initialization code
	[self performSelector:@selector(setLabelsVT220:) withObject:nil afterDelay:resizeTime];
}// performSetFunctionKeyLayoutVT220:


/*!
Changes the keyboard layout to match the “xterm” terminal
in a traditional X11R6 environment.

(4.0)
*/
- (IBAction)
performSetFunctionKeyLayoutXTermX11:(id)	sender
{
#pragma unused(sender)
	Session_FunctionKeyLayout const		kLayout = kSession_FunctionKeyLayoutXTerm;
	NSTimeInterval						resizeTime = 0;
	
	
	[layoutMenu selectItemAtIndex:[layoutMenu indexOfItemWithTarget:self andAction:@selector(performSetFunctionKeyLayoutXTermX11:)]];
	
	if (kLayout != gFunctionKeysLayout)
	{
		Preferences_Result		prefsResult = Preferences_SetData(kPreferences_TagFunctionKeyLayout,
																	sizeof(kLayout), &kLayout);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "unable to save function key layout");
		}
		
		// resize the window to reveal all 48 keys
		{
			NSRect	frame = [[self window] frame];
			
			
			frame.size = [[self window] maxSize];
			[[self window] setFrame:frame display:YES animate:YES];
			resizeTime = [[self window] animationResizeTime:frame];
		}
		
		// retain this setting
		gFunctionKeysLayout = kLayout;
	}
	
	// update labels; this is done always for the convenience of the initialization code
	[self performSelector:@selector(setLabelsXTermX11:) withObject:nil afterDelay:resizeTime];
}// performSetFunctionKeyLayoutXTermX11:


/*!
Changes the keyboard layout to match the “xterm” terminal
in the XFree86 implementation.

(4.0)
*/
- (IBAction)
performSetFunctionKeyLayoutXTermXFree86:(id)	sender
{
#pragma unused(sender)
	Session_FunctionKeyLayout const		kLayout = kSession_FunctionKeyLayoutXTermXFree86;
	NSTimeInterval						resizeTime = 0;
	
	
	[layoutMenu selectItemAtIndex:[layoutMenu indexOfItemWithTarget:self andAction:@selector(performSetFunctionKeyLayoutXTermXFree86:)]];
	
	if (kLayout != gFunctionKeysLayout)
	{
		Preferences_Result		prefsResult = Preferences_SetData(kPreferences_TagFunctionKeyLayout,
																	sizeof(kLayout), &kLayout);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "unable to save function key layout");
		}
		
		// resize the window to reveal all 48 keys
		{
			NSRect	frame = [[self window] frame];
			
			
			frame.size = [[self window] maxSize];
			[[self window] setFrame:frame display:YES animate:YES];
			resizeTime = [[self window] animationResizeTime:frame];
		}
		
		// retain this setting
		gFunctionKeysLayout = kLayout;
	}
	
	// update labels; this is done always for the convenience of the initialization code
	[self performSelector:@selector(setLabelsXTermXFree86:) withObject:nil afterDelay:resizeTime];
}// performSetFunctionKeyLayoutXTermXFree86:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF1:(id)		sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:1 forLayout:gFunctionKeysLayout];
}// typeF1:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF2:(id)		sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:2 forLayout:gFunctionKeysLayout];
}// typeF2:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF3:(id)		sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:3 forLayout:gFunctionKeysLayout];
}// typeF3:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF4:(id)		sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:4 forLayout:gFunctionKeysLayout];
}// typeF4:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF5:(id)		sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:5 forLayout:gFunctionKeysLayout];
}// typeF5:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF6:(id)		sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:6 forLayout:gFunctionKeysLayout];
}// typeF6:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF7:(id)		sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:7 forLayout:gFunctionKeysLayout];
}// typeF7:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF8:(id)		sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:8 forLayout:gFunctionKeysLayout];
}// typeF8:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF9:(id)		sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:9 forLayout:gFunctionKeysLayout];
}// typeF9:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF10:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:10 forLayout:gFunctionKeysLayout];
}// typeF10:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF11:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:11 forLayout:gFunctionKeysLayout];
}// typeF11:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF12:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:12 forLayout:gFunctionKeysLayout];
}// typeF12:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF13:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:13 forLayout:gFunctionKeysLayout];
}// typeF13:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF14:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:14 forLayout:gFunctionKeysLayout];
}// typeF14:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF15:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:15 forLayout:gFunctionKeysLayout];
}// typeF15:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF16:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:16 forLayout:gFunctionKeysLayout];
}// typeF16:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF17:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:17 forLayout:gFunctionKeysLayout];
}// typeF17:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF18:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:18 forLayout:gFunctionKeysLayout];
}// typeF18:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF19:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:19 forLayout:gFunctionKeysLayout];
}// typeF19:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF20:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:20 forLayout:gFunctionKeysLayout];
}// typeF20:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF21:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:21 forLayout:gFunctionKeysLayout];
}// typeF21:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF22:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:22 forLayout:gFunctionKeysLayout];
}// typeF22:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF23:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:23 forLayout:gFunctionKeysLayout];
}// typeF23:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF24:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:24 forLayout:gFunctionKeysLayout];
}// typeF24:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF25:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:25 forLayout:gFunctionKeysLayout];
}// typeF25:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF26:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:26 forLayout:gFunctionKeysLayout];
}// typeF26:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF27:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:27 forLayout:gFunctionKeysLayout];
}// typeF27:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF28:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:28 forLayout:gFunctionKeysLayout];
}// typeF28:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF29:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:29 forLayout:gFunctionKeysLayout];
}// typeF29:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF30:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:30 forLayout:gFunctionKeysLayout];
}// typeF30:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF31:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:31 forLayout:gFunctionKeysLayout];
}// typeF31:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF32:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:32 forLayout:gFunctionKeysLayout];
}// typeF32:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF33:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:33 forLayout:gFunctionKeysLayout];
}// typeF33:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF34:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:34 forLayout:gFunctionKeysLayout];
}// typeF34:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF35:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:35 forLayout:gFunctionKeysLayout];
}// typeF35:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF36:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:36 forLayout:gFunctionKeysLayout];
}// typeF36:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF37:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:37 forLayout:gFunctionKeysLayout];
}// typeF37:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF38:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:38 forLayout:gFunctionKeysLayout];
}// typeF38:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF39:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:39 forLayout:gFunctionKeysLayout];
}// typeF39:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF40:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:40 forLayout:gFunctionKeysLayout];
}// typeF40:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF41:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:41 forLayout:gFunctionKeysLayout];
}// typeF41:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF42:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:42 forLayout:gFunctionKeysLayout];
}// typeF42:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF43:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:43 forLayout:gFunctionKeysLayout];
}// typeF43:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF44:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:44 forLayout:gFunctionKeysLayout];
}// typeF44:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF45:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:45 forLayout:gFunctionKeysLayout];
}// typeF45:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF46:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:46 forLayout:gFunctionKeysLayout];
}// typeF46:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF47:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:47 forLayout:gFunctionKeysLayout];
}// typeF47:


/*!
Sends the specified VT function key to the active session.

(4.0)
*/
- (IBAction)
typeF48:(id)	sender
{
#pragma unused(sender)
	[self sendFunctionKeyWithIndex:48 forLayout:gFunctionKeysLayout];
}// typeF48:


#pragma mark NSWindowNotifications


/*!
Responds to a focus of the keypad by ensuring that the
title bar’s pop-up menu is also visible.

(4.0)
*/
- (void)
windowDidBecomeKey:(NSNotification*)	aNotification
{
	NSWindow*	keyWindow = (NSWindow*)[aNotification object];
	
	
	[self->menuChildWindow orderWindow:NSWindowAbove relativeTo:[keyWindow windowNumber]];
}// windowDidBecomeKey:


/*!
Responds to a change in the frame of the window by ensuring
that the pop-up menu still seems to be “attached” to the
window’s title bar.

(4.0)
*/
- (void)
windowDidResize:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	//NSWindow*	resizedWindow = (NSWindow*)[aNotification object];
	
	
	[self->menuChildWindow setFrame:[self popUpMenuButtonFrame] display:YES];
}// windowDidResize:


/*!
Responds to a close of the keypad by ensuring that the
title bar’s pop-up menu is also removed.

(4.0)
*/
- (void)
windowWillClose:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	//NSWindow*	closingWindow = (NSWindow*)[aNotification object];
	
	
	[self->menuChildWindow orderOut:NSApp];
}// windowWillClose:


#pragma mark NSWindowController


/*!
When the window has loaded, this moves the NIB-provided
pop-up menu into the proper place in the title bar region
and updates the menu based on current user preferences.

(4.0)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	
	assert(nil != functionKeyF1);
	assert(nil != functionKeyF2);
	assert(nil != functionKeyF3);
	assert(nil != functionKeyF4);
	assert(nil != functionKeyF15);
	assert(nil != functionKeyF16);
	assert(nil != layoutMenu);
	
	NSRect		innerFrame = [self popUpMenuButtonBounds];
	NSRect		childFrame = [self popUpMenuButtonFrame];
	NSView*		childContent = [[[TranslucentMenuArrow_View alloc] initWithFrame:innerFrame] autorelease];
	
	
	// store font attribute
	self->baseFontSize = [[self->functionKeyF1 font] pointSize];
	if ((self->baseFontSize < 8) || (self->baseFontSize > 20))
	{
		// arbitrary; protect against insane values
		self->baseFontSize = 13.0;
	}
	
	// create a window that will be attached to the main keypad window,
	// providing the illusion of being part of the title bar
	self->menuChildWindow = [[NSWindow alloc] initWithContentRect:childFrame styleMask:NSBorderlessWindowMask
																	backing:NSBackingStoreBuffered
																	defer:YES];
	[self->menuChildWindow setBackgroundColor:[NSColor clearColor]];
	[self->menuChildWindow setReleasedWhenClosed:NO];
	[self->menuChildWindow setOpaque:NO];
	[self->menuChildWindow setHasShadow:NO];
	[self->menuChildWindow setContentView:childContent];
	[layoutMenu setFrame:innerFrame];
	[[self->menuChildWindow contentView] addSubview:layoutMenu];
	
	// attach the menu pop-up’s window to the keypad window; note that
	// all of these ordering steps seem to be required in order for
	// the icon to appear above the title bar of the parent (the level
	// must be set to work on Panther, and "orderWindow:relativeTo:"
	// must be called to work on any OS version)
	[self->menuChildWindow setLevel:[[self window] level]];
	[[self window] addChildWindow:self->menuChildWindow ordered:NSWindowAbove];
	if ([[self window] isVisible])
	{
		[self->menuChildWindow orderWindow:NSWindowAbove relativeTo:[[self window] windowNumber]];
	}
	
	// update the menu and the global variable based on user preferences
	{
		Session_FunctionKeyLayout	layout = kSession_FunctionKeyLayoutVT220;
		Preferences_Result			prefsResult = Preferences_GetData(kPreferences_TagFunctionKeyLayout,
																		sizeof(layout), &layout);
		
		
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "no existing setting for function key layout was found");
			layout = kSession_FunctionKeyLayoutVT220;
		}
		
		[layoutMenu selectItemAtIndex:0]; // initially...
		switch (layout)
		{
		case kSession_FunctionKeyLayoutRxvt:
			[self performSetFunctionKeyLayoutRxvt:NSApp];
			break;
		
		case kSession_FunctionKeyLayoutXTerm:
			[self performSetFunctionKeyLayoutXTermX11:NSApp];
			break;
		
		case kSession_FunctionKeyLayoutXTermXFree86:
			[self performSetFunctionKeyLayoutXTermXFree86:NSApp];
			break;
		
		case kSession_FunctionKeyLayoutVT220:
		default:
			[self performSetFunctionKeyLayoutVT220:NSApp];
			break;
		}
	}
	
	// clear this flag to ensure that the title item remains disabled
	// (cannot be set from the NIB for some reason)
	[layoutMenu setAutoenablesItems:NO];
	
	// keep the window attached to the title bar
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidBecomeKey:)
														name:NSWindowDidBecomeKeyNotification object:[self window]];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidResize:)
														name:NSWindowDidResizeNotification object:[self window]];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowWillClose:)
														name:NSWindowWillCloseNotification object:[self window]];
}// windowDidLoad


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"FunctionKeys";
}// windowFrameAutosaveName


@end // Keypads_FunctionKeysPanelController


@implementation Keypads_VT220KeysPanelController


static Keypads_VT220KeysPanelController*	gKeypads_VT220KeysPanelController = nil;


/*!
Returns the singleton.

(3.1)
*/
+ (id)
sharedVT220KeysPanelController
{
	if (nil == gKeypads_VT220KeysPanelController)
	{
		gKeypads_VT220KeysPanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gKeypads_VT220KeysPanelController;
}// sharedVT220KeysPanelController


/*!
Designated initializer.

(3.1)
*/
- (id)
init
{
	self = [super initWithWindowNibName:@"KeypadVT220KeysCocoa"];
	return self;
}// init


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
type0:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK0];
}// type0:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
type1:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK1];
}// type1:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
type2:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK2];
}// type2:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
type3:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK3];
}// type3:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
type4:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK4];
}// type4:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
type5:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK5];
}// type5:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
type6:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK6];
}// type6:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
type7:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK7];
}// type7:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
type8:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK8];
}// type8:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
type9:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK9];
}// type9:


/*!
Sends the specified arrow key sequence to the active session.

(3.1)
*/
- (IBAction)
typeArrowDown:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSDN];
}// typeArrowDown:


/*!
Sends the specified arrow key sequence to the active session.

(3.1)
*/
- (IBAction)
typeArrowLeft:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSLT];
}// typeArrowLeft:


/*!
Sends the specified arrow key sequence to the active session.

(3.1)
*/
- (IBAction)
typeArrowRight:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSRT];
}// typeArrowRight:


/*!
Sends the specified arrow key sequence to the active session.

(3.1)
*/
- (IBAction)
typeArrowUp:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSUP];
}// typeArrowUp:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
typeComma:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSKC];
}// typeComma:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
typeDecimalPoint:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSKP];
}// typeDecimalPoint:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
typeDelete:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSPGUP_220DEL/* yes this is correct, based on key position */];
}// typeDelete:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
typeEnter:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSKE];
}// typeEnter:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
typeFind:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSHELP_220FIND/* yes this is correct, based on key position */];
}// typeFind:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
typeHyphen:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSKM];
}// typeHyphen:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
typeInsert:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSHOME_220INS/* yes this is correct, based on key position */];
}// typeInsert:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
typePageDown:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSPGDN_220PGDN];
}// typePageDown:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
typePageUp:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSEND_220PGUP/* yes this is correct, based on key position */];
}// typePageUp:


/*!
Sends the specified function key sequence to the active session.

(3.1)
*/
- (IBAction)
typePF1:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF1];
}// typeF1:


/*!
Sends the specified function key sequence to the active session.

(3.1)
*/
- (IBAction)
typePF2:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF2];
}// typePF2:


/*!
Sends the specified function key sequence to the active session.

(3.1)
*/
- (IBAction)
typePF3:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF3];
}// typePF3:


/*!
Sends the specified function key sequence to the active session.

(3.1)
*/
- (IBAction)
typePF4:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF4];
}// typePF4:


/*!
Sends the specified key’s character to the active session.

(3.1)
*/
- (IBAction)
typeSelect:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSDEL_220SEL/* yes this is correct, based on key position */];
}// typeSelect:


#pragma mark NSWindowController


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"VT220Keys";
}// windowFrameAutosaveName


@end // Keypads_VT220KeysPanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
