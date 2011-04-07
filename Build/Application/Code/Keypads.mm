/*!	\file Keypads.mm
	\brief Cocoa implementation of floating keypads.
*/
/*###############################################################

	MacTelnet
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

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <AutoPool.objc++.h>
#import <Console.h>

// MacTelnet includes
#import "Commands.h"
#import "Keypads.h"
#import "Session.h"
#import "SessionFactory.h"
#import "VTKeys.h"



#pragma mark Variables
namespace {

Preferences_ContextRef	gArrangeWindowBindingContext = nullptr;
Preferences_Tag			gArrangeWindowBinding = 0;
Preferences_Tag			gArrangeWindowScreenBinding = 0;
FourCharCode			gArrangeWindowDataTypeForWindowBinding = typeQDPoint;
FourCharCode			gArrangeWindowDataTypeForScreenBinding = typeHIRect;
Point					gArrangeWindowStackingOrigin = { 0, 0 };
EventTargetRef			gControlKeysEventTarget = nullptr;	//!< temporary, for Carbon interaction

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
		result = (YES == [[[Keypads_ControlKeysPanelController sharedControlKeysPanelController] window] isVisible]);
		break;
	
	case kKeypads_WindowTypeFullScreen:
		result = (YES == [[[Keypads_FullScreenPanelController sharedFullScreenPanelController] window] isVisible]);
		break;
	
	case kKeypads_WindowTypeFunctionKeys:
		result = (YES == [[[Keypads_FunctionKeysPanelController sharedFunctionKeysPanelController] window] isVisible]);
		break;
	
	case kKeypads_WindowTypeVT220Keys:
		result = (YES == [[[Keypads_VT220KeysPanelController sharedVT220KeysPanelController] window] isVisible]);
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// IsVisible


/*!
Sets the current event target for button commands issued
by the specified keypad, and automatically shows or hides
the keypad window.  Currently, only the control keys palette
(kKeypads_WindowTypeControlKeys) is recognized.

You can pass "nullptr" as the target to set no target, in
which case the default behavior (using a session window)
is restored and the palette is hidden if there is no user
preference to keep it visible.

IMPORTANT:	This is for Carbon compatibility and is not a
			long-term solution.

(3.1)
*/
void
Keypads_SetEventTarget	(Keypads_WindowType		inFromKeypad,
						 EventTargetRef			inCurrentTarget)
{
	switch (inFromKeypad)
	{
	case kKeypads_WindowTypeControlKeys:
		gControlKeysEventTarget = inCurrentTarget;
		// TEMPORARY: only hide if user preference is to keep invisible
		Keypads_SetVisible(inFromKeypad, (nullptr != gControlKeysEventTarget));
		break;
	
	case kKeypads_WindowTypeArrangeWindow:
	case kKeypads_WindowTypeFullScreen:
	case kKeypads_WindowTypeFunctionKeys:
	case kKeypads_WindowTypeVT220Keys:
	default:
		break;
	}
}// SetEventTarget


/*!
Shows or hides the specified keypad panel.

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
	
	(OSStatus)SetUserFocusWindow(oldActiveWindow);
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
Finds the appropriate target session and sends the specified
character, as if the user had typed that character.

(3.1)
*/
- (void)
sendCharacter:(UInt8)	inCharacter
{
	SessionRef		currentSession = getCurrentSession();
	
	
	if (nullptr != currentSession)
	{
		Session_UserInputKey(currentSession, inCharacter);
	}
}// sendCharacter:


/*!
Finds the appropriate target session and sends the character
represented by the specified virtual key, as if the user had
typed that key.  The key code should match those that are
valid for Session_UserInputKey().

(3.1)
*/
- (void)
sendKey:(UInt8)		inKey
{
	SessionRef		currentSession = getCurrentSession();
	
	
	if (nullptr != currentSession)
	{
		Session_UserInputKey(currentSession, inKey);
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
typeControlTilde:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1E];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlTilde, gControlKeysEventTarget);
}// typeControlTilde:


/*!
Sends the specified control character to the active session.

(3.1)
*/
- (IBAction)
typeControlQuestionMark:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1F];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlQuestionMark, gControlKeysEventTarget);
}// typeControlQuestionMark:


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
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF6:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSF6];
}// typeF6:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF7:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSF7];
}// typeF7:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF8:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSF8];
}// typeF8:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF9:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSF9];
}// typeF9:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF10:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF10];
}// typeF10:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF11:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF11];
}// typeF11:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF12:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF12];
}// typeF12:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF13:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF13];
}// typeF13:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF14:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF14];
}// typeF14:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF15:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF15_220HELP];
}// typeF15:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF16:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF16_220DO];
}// typeF16:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF17:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF17];
}// typeF17:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF18:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF18];
}// typeF18:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF19:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF19];
}// typeF19:


/*!
Sends the specified VT function key to the active session.

(3.1)
*/
- (IBAction)
typeF20:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF20];
}// typeF20:


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


@end // Keypads_VT220KeysPanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
