/*!	\file WindowTitleDialog.mm
	\brief Implements a dialog box for changing the title
	of a terminal window.
*/
/*###############################################################

	MacTerm
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

#import "WindowTitleDialog.h"
#import <UniversalDefines.h>

// standard-C includes
#import <climits>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>

// library includes
#import <AlertMessages.h>
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <FlagManager.h>
#import <MAAttachedWindow.h>
#import <PopoverManager.objc++.h>

// application includes
#import "Commands.h"
#import "ConstantsRegistry.h"
#import "HelpSystem.h"
#import "Session.h"
#import "VectorCanvas.h"



#pragma mark Types

@interface WindowTitleDialog_Handler : NSObject< PopoverManager_Delegate, WindowTitleDialog_ViewManagerChannel >
{
	WindowTitleDialog_Ref					selfRef;			// identical to address of structure, but typed as ref
	WindowTitleDialog_ViewManager*			viewMgr;			// loads the Rename interface
	MAAttachedWindow*						containerWindow;	// holds the Rename dialog view
	NSView*									managedView;		// the view that implements the majority of the interface
	SessionRef								session;			// the session, if any, to which this applies
	VectorCanvas_Ref						canvas;				// the canvas, if any, to which this applies
	HIWindowRef								renamedWindow;		// the window to be renamed
	PopoverManager_Ref						popoverMgr;			// manages common aspects of popover window behavior
	WindowTitleDialog_CloseNotifyProcPtr	closeNotifyProc;	// routine to call when the dialog is dismissed
}

+ (WindowTitleDialog_Handler*)
viewHandlerFromRef:(WindowTitleDialog_Ref)_;

// designated initializer
- (id)
initForCarbonWindow:(HIWindowRef)_
notificationProc:(WindowTitleDialog_CloseNotifyProcPtr)_;

- (id)
initForCarbonWindow:(HIWindowRef)_
notificationProc:(WindowTitleDialog_CloseNotifyProcPtr)_
session:(SessionRef)_;

- (id)
initForCarbonWindow:(HIWindowRef)_
notificationProc:(WindowTitleDialog_CloseNotifyProcPtr)_
vectorGraphicsCanvas:(VectorCanvas_Ref)_;

- (void)
display;

- (void)
remove;

- (NSWindow*)
renamedCocoaWindow;

// PopoverManager_Delegate

- (NSPoint)
idealAnchorPointForParentWindowFrame:(NSRect)_;

- (MAWindowPosition)
idealArrowPositionForParentWindowFrame:(NSRect)_;

// WindowTitleDialog_ViewManagerChannel

- (void)
titleDialog:(WindowTitleDialog_ViewManager*)_
didLoadManagedView:(NSView*)_;

- (void)
titleDialog:(WindowTitleDialog_ViewManager*)_
didFinishUsingManagedView:(NSView*)_
acceptingRename:(BOOL)_
finalTitle:(NSString*)_;

- (NSString*)
titleDialog:(WindowTitleDialog_ViewManager*)_
returnInitialTitleTextForManagedView:(NSView*)_;

@end // WindowTitleDialog_Handler



#pragma mark Public Methods

/*!
This method is used to initialize the generic window title
dialog box.  It creates the dialog box invisibly, and uses
the specified window’s title as the initial field value.

(3.0)
*/
WindowTitleDialog_Ref
WindowTitleDialog_New	(HIWindowRef							inParentWindow,
						 WindowTitleDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	WindowTitleDialog_Ref	result = nullptr;
	
	
	result = (WindowTitleDialog_Ref)[[WindowTitleDialog_Handler alloc]
										initForCarbonWindow:inParentWindow notificationProc:inCloseNotifyProcPtr];
	return result;
}// New


/*!
This method is used to initialize a session-specific window
title dialog box.  It creates the dialog box invisibly, and
uses the specified session’s user-defined title as the
initial field value.

When the user changes the title, the session’s user-defined
title is updated (which may affect the title of one or more
windows, but this is up to the Session implementation).

(3.1)
*/
WindowTitleDialog_Ref
WindowTitleDialog_NewForSession		(SessionRef								inSession,
									 WindowTitleDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	WindowTitleDialog_Ref	result = nullptr;
	
	
	result = (WindowTitleDialog_Ref)[[WindowTitleDialog_Handler alloc]
										initForCarbonWindow:Session_ReturnActiveWindow(inSession)
															notificationProc:inCloseNotifyProcPtr
															session:inSession];
	return result;
}// NewForSession


/*!
This method is used to initialize a canvas-specific window
title dialog box.  It creates the dialog box invisibly, and
uses the specified vector canvas’ user-defined title as the
initial field value.

When the user changes the title, the canvas’ user-defined
title is updated (which may affect the title of one or more
windows, but this is up to the canvas implementation).

(3.1)
*/
WindowTitleDialog_Ref
WindowTitleDialog_NewForVectorCanvas	(VectorCanvas_Ref						inCanvas,
										 WindowTitleDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr)
{
	WindowTitleDialog_Ref	result = nullptr;
	
	
	result = (WindowTitleDialog_Ref)[[WindowTitleDialog_Handler alloc]
										initForCarbonWindow:VectorCanvas_ReturnWindow(inCanvas)
															notificationProc:inCloseNotifyProcPtr
															vectorGraphicsCanvas:inCanvas];
	return result;
}// NewForVectorCanvas


/*!
Call this method to destroy a window title dialog
box and its associated data structures.  On return,
your copy of the dialog reference is set to nullptr.

(3.0)
*/
void
WindowTitleDialog_Dispose	(WindowTitleDialog_Ref*		inoutDialogPtr)
{
	WindowTitleDialog_Handler*	ptr = [WindowTitleDialog_Handler viewHandlerFromRef:*inoutDialogPtr];
	
	
	[ptr release];
}// Dispose


/*!
This method displays and handles events in the
generic window title dialog box.  When the user
clicks a button in the dialog, its disposal
callback is invoked.

(3.0)
*/
void
WindowTitleDialog_Display	(WindowTitleDialog_Ref		inDialog)
{
	WindowTitleDialog_Handler*	ptr = [WindowTitleDialog_Handler viewHandlerFromRef:inDialog];
	
	
	if (nullptr == ptr)
	{
		Alert_ReportOSStatus(paramErr);
	}
	else
	{
		// load the view asynchronously and eventually display it in a window
		[ptr display];
	}
}// Display


/*!
Hides the Rename dialog.  It can be redisplayed at any
time by calling WindowTitleDialog_Display() again.

(4.0)
*/
void
WindowTitleDialog_Remove	(WindowTitleDialog_Ref		inDialog)
{
	WindowTitleDialog_Handler*	ptr = [WindowTitleDialog_Handler viewHandlerFromRef:inDialog];
	
	
	[ptr remove];
}// Remove


/*!
The default handler for closing a window title dialog.

(3.0)
*/
void
WindowTitleDialog_StandardCloseNotifyProc	(WindowTitleDialog_Ref	UNUSED_ARGUMENT(inDialogThatClosed),
											 Boolean				UNUSED_ARGUMENT(inOKButtonPressed))
{
	// do nothing
}// StandardCloseNotifyProc


#pragma mark Internal Methods


@implementation WindowTitleDialog_Handler


/*!
Converts from the opaque reference type to the internal type.

(4.0)
*/
+ (WindowTitleDialog_Handler*)
viewHandlerFromRef:(WindowTitleDialog_Ref)		aRef
{
	return (WindowTitleDialog_Handler*)aRef;
}// viewHandlerFromRef


/*!
Designated initializer.

(4.0)
*/
- (id)
initForCarbonWindow:(HIWindowRef)							aWindow
notificationProc:(WindowTitleDialog_CloseNotifyProcPtr)		aProc
{
	self = [super init];
	if (nil != self)
	{
		self->selfRef = (WindowTitleDialog_Ref)self;
		self->viewMgr = nil;
		self->containerWindow = nil;
		self->managedView = nil;
		self->session = nullptr;
		self->canvas = nullptr;
		self->renamedWindow = aWindow;
		self->popoverMgr = nullptr;
		self->closeNotifyProc = aProc;
	}
	return self;
}// initForCarbonWindow:notificationProc:


/*!
Initializer for windows that belong to Sessions.
This is an important distinction because a Session
may customize the raw title slightly, and the user
interface should only allow the user-defined portion
of the title to be editable.

(4.0)
*/
- (id)
initForCarbonWindow:(HIWindowRef)							aWindow
notificationProc:(WindowTitleDialog_CloseNotifyProcPtr)		aProc
session:(SessionRef)										aSession
{
	self = [self initForCarbonWindow:aWindow notificationProc:aProc];
	if (nil != self)
	{
		self->session = aSession;
	}
	return self;
}// initForCarbonWindow:notificationProc:session:


/*!
Initializer for windows that belong to vector graphics
windows.

(4.0)
*/
- (id)
initForCarbonWindow:(HIWindowRef)							aWindow
notificationProc:(WindowTitleDialog_CloseNotifyProcPtr)		aProc
vectorGraphicsCanvas:(VectorCanvas_Ref)						aCanvas
{
	self = [self initForCarbonWindow:aWindow notificationProc:aProc];
	if (nil != self)
	{
		self->canvas = aCanvas;
	}
	return self;
}// initForCarbonWindow:notificationProc:vectorGraphicsCanvas:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[containerWindow release];
	[viewMgr release];
	if (nullptr != popoverMgr)
	{
		PopoverManager_Dispose(&popoverMgr);
	}
	[super dealloc];
}// dealloc


/*!
Creates the Rename view asynchronously; when the view is ready,
it calls "titleDialog:didLoadManagedView:".

(4.0)
*/
- (void)
display
{
	if (nil == self->viewMgr)
	{
		// no focus is done the first time because this is
		// eventually done in "titleDialog:didLoadManagedView:"
		self->viewMgr = [[WindowTitleDialog_ViewManager alloc]
							initForCarbonWindow:self->renamedWindow responder:self];
	}
	else
	{
		// window is already loaded, just activate it
		PopoverManager_DisplayPopover(self->popoverMgr);
	}
}// display


/*!
Hides the popover.  It can be shown again at any time
using the "display" method.

(4.0)
*/
- (void)
remove
{
	if (nil != self->popoverMgr)
	{
		PopoverManager_RemovePopover(self->popoverMgr);
	}
}// remove


/*!
Returns the Cocoa window that represents the renamed
window, even if that is a Carbon window.

(4.0)
*/
- (NSWindow*)
renamedCocoaWindow
{
	NSWindow*	result = CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(self->renamedWindow);
	
	
	return result;
}// renamedCocoaWindow


#pragma mark PopoverManager_Delegate


/*!
Returns the location (relative to the window) where the
popover’s arrow tip should appear.  The location of the
popover itself depends on the arrow placement chosen by
"idealArrowPositionForParentWindowFrame:".

(4.0)
*/
- (NSPoint)
idealAnchorPointForParentWindowFrame:(NSRect)	parentFrame
{
	NSWindow*	parentWindow = [self renamedCocoaWindow];
	NSRect		screenFrame = [[parentWindow screen] visibleFrame];
	NSRect		managedViewFrame = [self->managedView frame];
	NSPoint		result = NSMakePoint(parentFrame.size.width / 2.0, parentFrame.size.height - 16/* arbitrary */);
	
	
	// if the window position would make the popover fall off the top of the
	// screen (e.g. in Full Screen mode), force the popover to move a bit
	if ((NO == NSContainsRect(screenFrame, parentFrame)) &&
		NSIntersectsRect(parentFrame,
							NSMakeRect(screenFrame.origin.x, screenFrame.origin.y + screenFrame.size.height,
										screenFrame.size.width, 1/* height */)))
	{
		result.y -= managedViewFrame.size.height; // arbitrary; force the popover to be onscreen
	}
	return result;
}// idealAnchorPointForParentWindowFrame:


/*!
Returns arrow placement information for the popover.

(4.0)
*/
- (MAWindowPosition)
idealArrowPositionForParentWindowFrame:(NSRect)		parentFrame
{
#pragma unused(parentFrame)
	MAWindowPosition	result = MAPositionBottom;
	
	
	return result;
}// idealArrowPositionForParentWindowFrame:


#pragma mark WindowTitleDialog_ViewManagerChannel


/*!
Called when a WindowTitleDialog_ViewManager has finished
loading and initializing its view; responds by displaying
the view in a window and giving it keyboard focus.

Since this may be invoked multiple times, the window is
only created during the first invocation.

(4.0)
*/
- (void)
titleDialog:(WindowTitleDialog_ViewManager*)	aViewMgr
didLoadManagedView:(NSView*)					aManagedView
{
	self->managedView = aManagedView;
	if (nil == self->containerWindow)
	{
		NSWindow*	asNSWindow = [self renamedCocoaWindow];
		
		
		self->containerWindow = [[MAAttachedWindow alloc] initWithView:aManagedView
																		attachedToPoint:NSZeroPoint/* see delegate */
																		inWindow:asNSWindow
																		onSide:MAPositionAutomatic/* see delegate */
																		atDistance:0.0];
		CocoaBasic_ApplyStandardStyleToPopover(self->containerWindow, true/* has arrow */);
		self->popoverMgr = PopoverManager_New(self->containerWindow, [aViewMgr logicalFirstResponder],
												self/* delegate */, kPopoverManager_AnimationTypeStandard,
												self->renamedWindow);
		PopoverManager_DisplayPopover(self->popoverMgr);
	}
}// titleDialog:didLoadManagedView:


/*!
Called when the user has taken some action that would
complete his or her interaction with the view; a
sensible response is to close any containing window.
If the rename is accepted, update the title of the
target window.

(4.0)
*/
- (void)
titleDialog:(WindowTitleDialog_ViewManager*)	aViewMgr
didFinishUsingManagedView:(NSView*)				aManagedView
acceptingRename:(BOOL)							acceptedRename
finalTitle:(NSString*)							newTitle
{
#pragma unused(aViewMgr, aManagedView)
	// hide the popover
	[self remove];
	
	// prepare to rename the window
	if (acceptedRename)
	{
		if (nullptr != self->session)
		{
			// set session window’s user-defined title
			Session_SetWindowUserDefinedTitle(self->session, (CFStringRef)newTitle);
		}
		else if (nullptr != self->canvas)
		{
			// set vector graphics window’s user-defined title
			VectorCanvas_SetTitle(self->canvas, (CFStringRef)newTitle);
		}
		else
		{
			// set raw title
			(OSStatus)SetWindowTitleWithCFString(self->renamedWindow, (CFStringRef)newTitle);
		}
	}
	else
	{
		// user cancelled; do nothing
	}
	
	// notify of close
	if (nullptr != self->closeNotifyProc)
	{
		WindowTitleDialog_InvokeCloseNotifyProc(self->closeNotifyProc, self->selfRef,
												(acceptedRename) ? true : false);
	}
}// titleDialog:didFinishUsingManagedView:acceptingRename:finalTitle:


/*!
Called when the view needs an initial value for its
title text parameter.

(4.0)
*/
- (NSString*)
titleDialog:(WindowTitleDialog_ViewManager*)		aViewMgr
returnInitialTitleTextForManagedView:(NSView*)		aManagedView
{
#pragma unused(aViewMgr, aManagedView)
	NSString*	result = nil;
	
	
	if (nullptr != self->session)
	{
		// find session window’s user-defined title
		CFStringRef		titleString = nullptr;
		Session_Result	sessionResult = kSession_ResultOK;
		
		
		// note that the string is not copied here
		sessionResult = Session_GetWindowUserDefinedTitle(self->session, titleString);
		if ((kSession_ResultOK == sessionResult) && (nullptr != titleString))
		{
			result = (NSString*)titleString;
		}
	}
	else if (nullptr != self->canvas)
	{
		// find vector graphics window’s user-defined title
		CFStringRef		titleString = nullptr;
		
		
		VectorCanvas_CopyTitle(self->canvas, titleString);
		if (nullptr != titleString)
		{
			result = (NSString*)titleString;
			[result autorelease];
		}
	}
	else
	{
		// find raw window title
	}
	
	return result;
}// titleDialog:returnInitialTitleTextForManagedView:


@end // WindowTitleDialog_Handler


@implementation WindowTitleDialog_ViewManager


/*!
Designated initializer.

(4.0)
*/
- (id)
initForCarbonWindow:(HIWindowRef)						aWindow
responder:(id< WindowTitleDialog_ViewManagerChannel >)	aResponder
{
	self = [super init];
	if (nil != self)
	{
		responder = aResponder;
		parentCarbonWindow = aWindow;
		titleText = [@"" retain];
		
		// it is necessary to capture and release all top-level objects here
		// so that "self" can actually be deallocated; otherwise, the implicit
		// retain-count of 1 on each top-level object prevents deallocation
		{
			NSArray*	objects = nil;
			NSNib*		loader = [[NSNib alloc] initWithNibNamed:@"WindowTitleDialogCocoa" bundle:nil];
			
			
			if (NO == [loader instantiateNibWithOwner:self topLevelObjects:&objects])
			{
				[self dealloc], self = nil;
			}
			[loader release];
			[objects makeObjectsPerformSelector:@selector(release)];
		}
	}
	return self;
}// initForTerminalWindow:responder:initialOptions:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[titleText release];
	[super dealloc];
}// dealloc


#pragma mark New Methods


/*!
Returns the view that a window ought to focus first
using NSWindow’s "makeFirstResponder:".

(4.0)
*/
- (NSView*)
logicalFirstResponder
{
	return titleField;
}// logicalFirstResponder


/*!
Closes the dialog and renames the parent window.

IMPORTANT:	It is appropriate at this time for the
			responder object to release itself (and
			this object).

(4.0)
*/
- (IBAction)
performCloseAndRename:(id)	sender
{
#pragma unused(sender)
	NSString*	newTitleText = (nil == self->titleText)
								? @""
								: [NSString stringWithString:self->titleText];
	
	
	[self->responder titleDialog:self didFinishUsingManagedView:self->managedView
										acceptingRename:YES finalTitle:newTitleText];
}// performCloseAndRename:


/*!
Cancels the dialog and does not rename the parent window.

IMPORTANT:	It is appropriate at this time for the
			responder object to release itself (and
			this object).

(4.0)
*/
- (IBAction)
performCloseAndRevert:(id)	sender
{
#pragma unused(sender)
	[self->responder titleDialog:self didFinishUsingManagedView:self->managedView
										acceptingRename:NO finalTitle:nil];
}// performCloseAndRevert:


/*!
A helper to make string-setters less cumbersome to write.

(4.0)
*/
- (void)
setStringProperty:(NSString**)		propertyPtr
withName:(NSString*)				propertyName
toValue:(NSString*)					aString
{
	if (aString != *propertyPtr)
	{
		[self willChangeValueForKey:propertyName];
		
		if (nil == aString)
		{
			*propertyPtr = [@"" retain];
		}
		else
		{
			[*propertyPtr autorelease];
			*propertyPtr = [aString copy];
		}
		
		[self didChangeValueForKey:propertyName];
	}
}// setStringProperty:withName:toValue:


#pragma mark Accessors


/*!
Accessor.

(4.0)
*/
- (NSString*)
titleText
{
	return [[titleText copy] autorelease];
}
+ (id)
autoNotifyOnChangeTosetTitleText
{
	return [NSNumber numberWithBool:NO];
}
- (void)
setTitleText:(NSString*)	aString
{
	[self setStringProperty:&titleText withName:@"titleText" toValue:aString];
}// setTitleText:


#pragma mark NSKeyValueObservingCustomization


/*!
Returns true for keys that manually notify observers
(through "willChangeValueForKey:", etc.).

(4.0)
*/
+ (BOOL)
automaticallyNotifiesObserversForKey:(NSString*)	theKey
{
	BOOL	result = YES;
	SEL		flagSource = NSSelectorFromString([[self class] selectorNameForKeyChangeAutoNotifyFlag:theKey]);
	
	
	if (NULL != class_getClassMethod([self class], flagSource))
	{
		// See selectorToReturnKeyChangeAutoNotifyFlag: for more information on the form of the selector.
		result = [[self performSelector:flagSource] boolValue];
	}
	else
	{
		result = [super automaticallyNotifiesObserversForKey:theKey];
	}
	return result;
}// automaticallyNotifiesObserversForKey:


#pragma mark NSNibAwaking


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(4.0)
*/
- (void)
awakeFromNib
{
	// NOTE: superclass does not implement "awakeFromNib", otherwise it should be called here
	assert(nil != managedView);
	assert(nil != titleField);
	
	[self->responder titleDialog:self didLoadManagedView:self->managedView];
	[self setTitleText:[self->responder titleDialog:self returnInitialTitleTextForManagedView:self->managedView]];
}// awakeFromNib


@end // WindowTitleDialog_ViewManager

// BELOW IS REQUIRED NEWLINE TO END FILE
