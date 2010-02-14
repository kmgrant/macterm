/*!	\file Keypads.mm
	\brief Cocoa implementation of floating keypads.
*/
/*###############################################################

	MacTelnet
		© 1998-2008 by Kevin Grant.
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

Preferences_Tag		gArrangeWindowBinding = 0;
Point				gArrangeWindowStackingOrigin = { 0, 0 };
EventTargetRef		gControlKeysEventTarget = nullptr;	//!< temporary, for Carbon interaction

}// anonymous namespace



#pragma mark Public Methods

/*!
Binds the “Arrange Window” panel to a preferences tag,
which determines both its new window position and the
preference that is auto-updated as the window is moved.

Set to 0 to have no binding effect.

(4.0)
*/
void
Keypads_SetArrangeWindowPanelBinding	(Preferences_Tag	inBinding)
{
	size_t		actualSize = 0;
	
	
	unless (Preferences_GetData(inBinding, sizeof(gArrangeWindowStackingOrigin), &gArrangeWindowStackingOrigin, &actualSize) ==
			kPreferences_ResultOK)
	{
		SetPt(&gArrangeWindowStackingOrigin, 40, 40); // assume a default, if preference can’t be found
	}
	gArrangeWindowBinding = inBinding;
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
			[[Keypads_ArrangeWindowPanelController sharedArrangeWindowPanelController] showWindow:NSApp];
			[[Keypads_ArrangeWindowPanelController sharedArrangeWindowPanelController] setOriginToX:gArrangeWindowStackingOrigin.h
																						andY:gArrangeWindowStackingOrigin.v];
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
This routine consistently determines the current
keyboard focus session for all keypad menus.

(3.0)
*/
SessionRef
getCurrentSession ()
{
	return SessionFactory_ReturnUserFocusSession();
}// getCurrentSession

}// anonymous namespace


@implementation Keypads_ArrangeWindowPanelController

static Keypads_ArrangeWindowPanelController*	gKeypads_ArrangeWindowPanelController = nil;
+ (id)
sharedArrangeWindowPanelController
{
	if (nil == gKeypads_ArrangeWindowPanelController)
	{
		gKeypads_ArrangeWindowPanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gKeypads_ArrangeWindowPanelController;
}

- (id)
init
{
	self = [super initWithWindowNibName:@"KeypadArrangeWindow"];
	return self;
}

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
		Point				prefValue;
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		
		
		SetPt(&prefValue, x, y);
		prefsResult = Preferences_SetData(kPreferences_TagWindowStackingOrigin, sizeof(prefValue), &prefValue);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteLine, "failed to set terminal window origin");
		}
		TerminalWindow_StackWindows(); // re-stack windows so the user can see the effect of the change
	}
	Keypads_SetVisible(kKeypads_WindowTypeArrangeWindow, false);
	// INCOMPLETE
}

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
}

@end // Keypads_ArrangeWindowPanelController


@implementation Keypads_PanelController

- (void)
sendCharacter:(UInt8)	inCharacter
{
	SessionRef		currentSession = getCurrentSession();
	char			ck[1] = { inCharacter };
	
	
	if (nullptr != currentSession) Session_UserInputString(currentSession, ck, sizeof(ck), false/* record */);
}

- (void)
sendKey:(UInt8)		inKey
{
	SessionRef		currentSession = getCurrentSession();
	
	
	if (nullptr != currentSession) Session_UserInputKey(currentSession, inKey);
}

@end // Keypads_PanelController


@implementation Keypads_ControlKeysPanelController

static Keypads_ControlKeysPanelController*		gKeypads_ControlKeysPanelController = nil;
+ (id)
sharedControlKeysPanelController
{
	if (nil == gKeypads_ControlKeysPanelController)
	{
		gKeypads_ControlKeysPanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gKeypads_ControlKeysPanelController;
}

- (id)
init
{
	self = [super initWithWindowNibName:@"KeypadControlKeys"];
	return self;
}

- (IBAction)
typeNull:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x00];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlAtSign, gControlKeysEventTarget);
}

- (IBAction)
typeControlA:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x01];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlA, gControlKeysEventTarget);
}

- (IBAction)
typeControlB:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x02];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlB, gControlKeysEventTarget);
}

- (IBAction)
typeControlC:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x03];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlC, gControlKeysEventTarget);
}

- (IBAction)
typeControlD:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x04];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlD, gControlKeysEventTarget);
}

- (IBAction)
typeControlE:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x05];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlE, gControlKeysEventTarget);
}

- (IBAction)
typeControlF:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x06];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlF, gControlKeysEventTarget);
}

- (IBAction)
typeControlG:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x07];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlG, gControlKeysEventTarget);
}

- (IBAction)
typeControlH:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x08];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlH, gControlKeysEventTarget);
}

- (IBAction)
typeControlI:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x09];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlI, gControlKeysEventTarget);
}

- (IBAction)
typeControlJ:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0A];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlJ, gControlKeysEventTarget);
}

- (IBAction)
typeControlK:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0B];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlK, gControlKeysEventTarget);
}

- (IBAction)
typeControlL:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0C];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlL, gControlKeysEventTarget);
}

- (IBAction)
typeControlM:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0D];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlM, gControlKeysEventTarget);
}

- (IBAction)
typeControlN:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0E];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlN, gControlKeysEventTarget);
}

- (IBAction)
typeControlO:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x0F];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlO, gControlKeysEventTarget);
}

- (IBAction)
typeControlP:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x10];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlP, gControlKeysEventTarget);
}

- (IBAction)
typeControlQ:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x11];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlQ, gControlKeysEventTarget);
}

- (IBAction)
typeControlR:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x12];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlR, gControlKeysEventTarget);
}

- (IBAction)
typeControlS:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x13];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlS, gControlKeysEventTarget);
}

- (IBAction)
typeControlT:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x14];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlT, gControlKeysEventTarget);
}

- (IBAction)
typeControlU:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x15];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlU, gControlKeysEventTarget);
}

- (IBAction)
typeControlV:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x16];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlV, gControlKeysEventTarget);
}

- (IBAction)
typeControlW:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x17];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlW, gControlKeysEventTarget);
}

- (IBAction)
typeControlX:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x18];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlX, gControlKeysEventTarget);
}

- (IBAction)
typeControlY:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x19];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlY, gControlKeysEventTarget);
}

- (IBAction)
typeControlZ:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1A];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlZ, gControlKeysEventTarget);
}

- (IBAction)
typeControlLeftSquareBracket:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1B];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlLeftSquareBracket, gControlKeysEventTarget);
}

- (IBAction)
typeControlBackslash:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1C];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlBackslash, gControlKeysEventTarget);
}

- (IBAction)
typeControlRightSquareBracket:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1D];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlRightSquareBracket, gControlKeysEventTarget);
}

- (IBAction)
typeControlTilde:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1E];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlTilde, gControlKeysEventTarget);
}

- (IBAction)
typeControlQuestionMark:(id)	sender
{
#pragma unused(sender)
	[self sendCharacter:0x1F];
	Commands_ExecuteByIDUsingEvent(kCommandKeypadControlQuestionMark, gControlKeysEventTarget);
}

@end // Keypads_ControlKeysPanelController


@implementation Keypads_FullScreenPanelController

static Keypads_FullScreenPanelController*		gKeypads_FullScreenPanelController = nil;
+ (id)
sharedFullScreenPanelController
{
	if (nil == gKeypads_FullScreenPanelController)
	{
		gKeypads_FullScreenPanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gKeypads_FullScreenPanelController;
}

- (id)
init
{
	self = [super initWithWindowNibName:@"KeypadFullScreen"];
	return self;
}

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
}

@end // Keypads_FullScreenPanelController


@implementation Keypads_FunctionKeysPanelController

static Keypads_FunctionKeysPanelController*		gKeypads_FunctionKeysPanelController = nil;
+ (id)
sharedFunctionKeysPanelController
{
	if (nil == gKeypads_FunctionKeysPanelController)
	{
		gKeypads_FunctionKeysPanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gKeypads_FunctionKeysPanelController;
}

- (id)
init
{
	self = [super initWithWindowNibName:@"KeypadFunctionKeys"];
	return self;
}

- (IBAction)
typeF6:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSF6];
}

- (IBAction)
typeF7:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSF7];
}

- (IBAction)
typeF8:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSF8];
}

- (IBAction)
typeF9:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSF9];
}

- (IBAction)
typeF10:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF10];
}

- (IBAction)
typeF11:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF11];
}

- (IBAction)
typeF12:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF12];
}

- (IBAction)
typeF13:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF13];
}

- (IBAction)
typeF14:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF14];
}

- (IBAction)
typeF15:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF15];
}

- (IBAction)
typeF16:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF16];
}

- (IBAction)
typeF17:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF17];
}

- (IBAction)
typeF18:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF18];
}

- (IBAction)
typeF19:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF19];
}

- (IBAction)
typeF20:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF20];
}

@end // Keypads_FunctionKeysPanelController


@implementation Keypads_VT220KeysPanelController

static Keypads_VT220KeysPanelController*	gKeypads_VT220KeysPanelController = nil;
+ (id)
sharedVT220KeysPanelController
{
	if (nil == gKeypads_VT220KeysPanelController)
	{
		gKeypads_VT220KeysPanelController = [[[self class] allocWithZone:NULL] init];
	}
	return gKeypads_VT220KeysPanelController;
}

- (id)
init
{
	self = [super initWithWindowNibName:@"KeypadVT220Keys"];
	return self;
}

- (IBAction)
type0:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK0];
}

- (IBAction)
type1:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK1];
}

- (IBAction)
type2:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK2];
}

- (IBAction)
type3:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK3];
}

- (IBAction)
type4:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK4];
}

- (IBAction)
type5:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK5];
}

- (IBAction)
type6:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK6];
}

- (IBAction)
type7:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK7];
}

- (IBAction)
type8:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK8];
}

- (IBAction)
type9:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSK9];
}

- (IBAction)
typeArrowDown:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSDN];
}

- (IBAction)
typeArrowLeft:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSLT];
}

- (IBAction)
typeArrowRight:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSRT];
}

- (IBAction)
typeArrowUp:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSUP];
}

- (IBAction)
typeComma:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSKC];
}

- (IBAction)
typeDecimalPoint:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSKP];
}

- (IBAction)
typeDelete:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSPGUP/* yes this is correct, based on key position */];
}

- (IBAction)
typeEnter:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSKE];
}

- (IBAction)
typeFind:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSHELP/* yes this is correct, based on key position */];
}

- (IBAction)
typeHyphen:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSKM];
}

- (IBAction)
typeInsert:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSHOME/* yes this is correct, based on key position */];
}

- (IBAction)
typePageDown:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSPGDN];
}

- (IBAction)
typePageUp:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSEND/* yes this is correct, based on key position */];
}

- (IBAction)
typePF1:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF1];
}

- (IBAction)
typePF2:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF2];
}

- (IBAction)
typePF3:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF3];
}

- (IBAction)
typePF4:(id)	sender
{
#pragma unused(sender)
	[self sendKey:VSF4];
}

- (IBAction)
typeSelect:(id)		sender
{
#pragma unused(sender)
	[self sendKey:VSDEL/* yes this is correct, based on key position */];
}

@end // Keypads_VT220KeysPanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
