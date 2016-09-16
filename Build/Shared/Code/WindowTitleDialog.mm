/*!	\file WindowTitleDialog.mm
	\brief Implements a dialog box for changing the title
	of a terminal window.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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
#import <Popover.objc++.h>
#import <PopoverManager.objc++.h>



#pragma mark Types

/*!
Manages the rename-window user interface.
*/
@interface WindowTitleDialog_Handler : NSObject< PopoverManager_Delegate, WindowTitleDialog_VCDelegate > //{
{
	WindowTitleDialog_Ref						_selfRef;				// identical to address of structure, but typed as ref
	WindowTitleDialog_VC*						_viewMgr;				// loads the Rename interface
	Popover_Window*								_containerWindow;		// holds the Rename dialog view
	NSView*										_managedView;			// the view that implements the majority of the interface
	NSWindow*									_targetCocoaWindow;		// the window to be renamed, if Cocoa
	HIWindowRef									_targetCarbonWindow;		// the window to be renamed, if Carbon
	BOOL										_animated;				// YES if there should be animation when opening/closing
	PopoverManager_Ref							_popoverMgr;				// manages common aspects of popover window behavior
	WindowTitleDialog_ReturnTitleCopyBlock		_initBlock;				// block to invoke when initializing dialog title
	WindowTitleDialog_CloseNotifyBlock			_closeNotifyBlock;		// block to invoke when the dialog is dismissed
}

// class methods
	+ (WindowTitleDialog_Handler*)
	viewHandlerFromRef:(WindowTitleDialog_Ref)_;

// initializers
	- (instancetype)
	initForCocoaWindow:(NSWindow*)_
	orCarbonWindow:(HIWindowRef)_
	animated:(BOOL)_
	whenInitializing:(WindowTitleDialog_ReturnTitleCopyBlock)_
	whenClosing:(WindowTitleDialog_CloseNotifyBlock)_ NS_DESIGNATED_INITIALIZER;
	- (instancetype)
	initForCocoaWindow:(NSWindow*)_
	animated:(BOOL)_
	whenInitializing:(WindowTitleDialog_ReturnTitleCopyBlock)_
	whenClosing:(WindowTitleDialog_CloseNotifyBlock)_;
	- (instancetype)
	initForCarbonWindow:(HIWindowRef)_
	animated:(BOOL)_
	whenInitializing:(WindowTitleDialog_ReturnTitleCopyBlock)_
	whenClosing:(WindowTitleDialog_CloseNotifyBlock)_;

// new methods
	- (void)
	display;
	- (void)
	removeWithAcceptance:(BOOL)_;

// accessors
	- (NSWindow*)
	renamedCocoaWindow;

// PopoverManager_Delegate
	- (NSPoint)
	idealAnchorPointForFrame:(NSRect)_
	parentWindow:(NSWindow*)_;
	- (Popover_Properties)
	idealArrowPositionForFrame:(NSRect)_
	parentWindow:(NSWindow*)_;
	- (NSSize)
	idealSize;

// WindowTitleDialog_VCDelegate
	- (void)
	titleDialog:(WindowTitleDialog_VC*)_
	didLoadManagedView:(NSView*)_;
	- (void)
	titleDialog:(WindowTitleDialog_VC*)_
	didFinishUsingManagedView:(NSView*)_
	acceptingRename:(BOOL)_
	finalTitle:(NSString*)_;
	- (NSString*)
	titleDialog:(WindowTitleDialog_VC*)_
	returnInitialTitleTextForManagedView:(NSView*)_;

@end //}



#pragma mark Public Methods

/*!
Returns a new window-modal version of the rename dialog
that initializes its string field using the given
initialization block.  The close-notify block is called
when the dialog closes; if the user accepts, a new
string is provided (otherwise, the string is nullptr).

(2016.09)
*/
WindowTitleDialog_Ref
WindowTitleDialog_NewWindowModal		(NSWindow*								inCocoaParentWindow,
									 Boolean									inIsAnimated,
									 WindowTitleDialog_ReturnTitleCopyBlock	inInitBlock,
									 WindowTitleDialog_CloseNotifyBlock		inFinalBlock)
{
	WindowTitleDialog_Ref	result = nullptr;
	
	
	result = (WindowTitleDialog_Ref)[[WindowTitleDialog_Handler alloc]
										initForCocoaWindow:inCocoaParentWindow animated:inIsAnimated
															whenInitializing:inInitBlock
															whenClosing:inFinalBlock];
	return result;
}// NewWindowModal


/*!
Returns a new window-modal version of the rename dialog
that initializes its string field using the given
initialization block.  The close-notify block is called
when the dialog closes; if the user accepts, a new
string is provided (otherwise, the string is nullptr).

DEPRECATED.  Use the NSWindow* version above.

(2016.09)
*/
WindowTitleDialog_Ref
WindowTitleDialog_NewWindowModalParentCarbon		(HIWindowRef								inCarbonParentWindow,
												 Boolean									inIsAnimated,
												 WindowTitleDialog_ReturnTitleCopyBlock	inInitBlock,
												 WindowTitleDialog_CloseNotifyBlock		inFinalBlock)
{
	WindowTitleDialog_Ref	result = nullptr;
	
	
	result = (WindowTitleDialog_Ref)[[WindowTitleDialog_Handler alloc]
										initForCarbonWindow:inCarbonParentWindow animated:inIsAnimated
															whenInitializing:inInitBlock
															whenClosing:inFinalBlock];
	return result;
}// NewWindowModalParentCarbon


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
WindowTitleDialog_Display	(WindowTitleDialog_Ref	inDialog)
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


#pragma mark Internal Methods


#pragma mark -
@implementation WindowTitleDialog_Handler


/*!
Converts from the opaque reference type to the internal type.

(4.0)
*/
+ (WindowTitleDialog_Handler*)
viewHandlerFromRef:(WindowTitleDialog_Ref)		aRef
{
	return REINTERPRET_CAST(aRef, WindowTitleDialog_Handler*);
}// viewHandlerFromRef


/*!
Designated initializer.

TEMPORARY; this accepts both types of window arguments (Carbon
and Cocoa) but only one is required.  Eventually the Cocoa-only
window version will become the designated initializer, when
there is no reason to support Carbon windows anymore.

(4.0)
*/
- (instancetype)
initForCocoaWindow:(NSWindow*)								aCocoaWindow
orCarbonWindow:(HIWindowRef)									aCarbonWindow
animated:(BOOL)												anAnimationFlag
whenInitializing:(WindowTitleDialog_ReturnTitleCopyBlock)	anInitBlock
whenClosing:(WindowTitleDialog_CloseNotifyBlock)				aFinalBlock
{
	self = [super init];
	if (nil != self)
	{
		_selfRef = REINTERPRET_CAST(self, WindowTitleDialog_Ref);
		_viewMgr = nil;
		_containerWindow = nil;
		_managedView = nil;
		_targetCocoaWindow = aCocoaWindow;
		_targetCarbonWindow = aCarbonWindow;
		_popoverMgr = nullptr;
		_animated = anAnimationFlag;
		_initBlock = Block_copy(anInitBlock);
		_closeNotifyBlock = Block_copy(aFinalBlock);
	}
	return self;
}// initForCocoaWindow:orCarbonWindow:animated:whenInitializing:whenClosing:


/*!
Initializer for parent windows that use Carbon.

DEPRECATED.  Use the NSWindow* version.

(2016.09)
*/
- (instancetype)
initForCarbonWindow:(HIWindowRef)							aWindow
animated:(BOOL)												anAnimationFlag
whenInitializing:(WindowTitleDialog_ReturnTitleCopyBlock)	anInitBlock
whenClosing:(WindowTitleDialog_CloseNotifyBlock)				aFinalBlock
{
	self = [self initForCocoaWindow:nil orCarbonWindow:aWindow animated:anAnimationFlag
									whenInitializing:anInitBlock whenClosing:aFinalBlock];
	if (nil != self)
	{
	}
	return self;
}// initForCarbonWindow:animated:whenInitializing:whenClosing:


/*!
Initializer for parent windows that use Cocoa windows.  This will
eventually be the designated initializer, when it is no longer
necessary to support Carbon windows.

(2016.09)
*/
- (instancetype)
initForCocoaWindow:(NSWindow*)								aWindow
animated:(BOOL)												anAnimationFlag
whenInitializing:(WindowTitleDialog_ReturnTitleCopyBlock)	anInitBlock
whenClosing:(WindowTitleDialog_CloseNotifyBlock)				aFinalBlock
{
	self = [self initForCocoaWindow:aWindow orCarbonWindow:nullptr animated:anAnimationFlag
									whenInitializing:anInitBlock whenClosing:aFinalBlock];
	if (nil != self)
	{
	}
	return self;
}// initForCocoaWindow:animated:whenInitializing:whenClosing:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	Block_release(_initBlock);
	Block_release(_closeNotifyBlock);
	[_containerWindow release];
	[_viewMgr release];
	if (nullptr != _popoverMgr)
	{
		PopoverManager_Dispose(&_popoverMgr);
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
	if (nil == _viewMgr)
	{
		// no focus is done the first time because this is
		// eventually done in "titleDialog:didLoadManagedView:"
		if (nullptr != _targetCarbonWindow)
		{
			_viewMgr = [[WindowTitleDialog_VC alloc]
						initForCarbonWindow:_targetCarbonWindow responder:self];
		}
		else
		{
			_viewMgr = [[WindowTitleDialog_VC alloc]
						initForCocoaWindow:_targetCocoaWindow responder:self];
		}
	}
	else
	{
		// window is already loaded, just activate it
		PopoverManager_DisplayPopover(_popoverMgr);
	}
}// display


/*!
Hides the popover.  It can be shown again at any time
using the "display" method.

(4.0)
*/
- (void)
removeWithAcceptance:(BOOL)		isAccepted
{
	if (nil != _popoverMgr)
	{
		PopoverManager_RemovePopover(_popoverMgr, isAccepted);
	}
}// removeWithAcceptance:


/*!
Returns the Cocoa window that represents the renamed
window, even if that is a Carbon window.

(4.0)
*/
- (NSWindow*)
renamedCocoaWindow
{
	NSWindow*	result = _targetCocoaWindow;
	
	
	if ((nil == result) && (nullptr != _targetCarbonWindow))
	{
		result = CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow(_targetCarbonWindow);
	}
	
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
idealAnchorPointForFrame:(NSRect)	parentFrame
parentWindow:(NSWindow*)			parentWindow
{
	NSRect		screenFrame = [[parentWindow screen] visibleFrame];
	NSRect		managedViewFrame = [_managedView frame];
	NSPoint		result = NSMakePoint(CGFLOAT_DIV_2(parentFrame.size.width), parentFrame.size.height - 12/* arbitrary */);
	
	
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
- (Popover_Properties)
idealArrowPositionForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(parentFrame, parentWindow)
	Popover_Properties	result = kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameBelowArrow;
	
	
	return result;
}// idealArrowPositionForParentWindowFrame:


/*!
Returns the initial size for the popover.

(4.0)
*/
- (NSSize)
idealSize
{
	NSRect		frameRect = [_containerWindow frameRectForViewRect:[_managedView frame]];
	NSSize		result = frameRect.size;
	
	
	return result;
}// idealSize


#pragma mark WindowTitleDialog_VCDelegate


/*!
Called when a WindowTitleDialog_VC has finished loading and
initializing its view; responds by displaying the view in a
window and giving it keyboard focus.

Since this may be invoked multiple times, the window is only
created during the first invocation.

(4.0)
*/
- (void)
titleDialog:(WindowTitleDialog_VC*)		aViewMgr
didLoadManagedView:(NSView*)			aManagedView
{
	_managedView = aManagedView;
	if (nil == _containerWindow)
	{
		NSWindow*						asNSWindow = [self renamedCocoaWindow];
		PopoverManager_AnimationType	animationType = kPopoverManager_AnimationTypeStandard;
		Boolean							noAnimations = (NO == self->_animated);
		
		
		if (noAnimations)
		{
			animationType = kPopoverManager_AnimationTypeNone;
		}
		
		_containerWindow = [[Popover_Window alloc] initWithView:aManagedView
																style:kPopover_WindowStyleDialogSheet
																attachedToPoint:NSZeroPoint/* see delegate */
																inWindow:asNSWindow];
		[_containerWindow setReleasedWhenClosed:NO];
		[_containerWindow setStandardArrowProperties:YES];
		if (nullptr != _targetCarbonWindow)
		{
			_popoverMgr = PopoverManager_New(_containerWindow, [aViewMgr logicalFirstResponder],
												self/* delegate */, animationType, kPopoverManager_BehaviorTypeDialog,
												_targetCarbonWindow);
		}
		else
		{
			_popoverMgr = PopoverManager_New(_containerWindow, [aViewMgr logicalFirstResponder],
												self/* delegate */, animationType, kPopoverManager_BehaviorTypeDialog,
												_targetCocoaWindow.contentView);
		}
		PopoverManager_DisplayPopover(_popoverMgr);
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
titleDialog:(WindowTitleDialog_VC*)		aViewMgr
didFinishUsingManagedView:(NSView*)		aManagedView
acceptingRename:(BOOL)					acceptedRename
finalTitle:(NSString*)					newTitle
{
#pragma unused(aViewMgr, aManagedView)
	// hide the popover
	[self removeWithAcceptance:acceptedRename];
	
	// prepare to rename the window
	if (acceptedRename)
	{
		_closeNotifyBlock(BRIDGE_CAST(newTitle, CFStringRef));
	}
	else
	{
		// user cancelled
		_closeNotifyBlock(nullptr/* no title; signals to block that user cancelled */);
	}
}// titleDialog:didFinishUsingManagedView:acceptingRename:finalTitle:


/*!
Called when the view needs an initial value for its
title text parameter.

(4.0)
*/
- (NSString*)
titleDialog:(WindowTitleDialog_VC*)				aViewMgr
returnInitialTitleTextForManagedView:(NSView*)	aManagedView
{
#pragma unused(aViewMgr, aManagedView)
	NSString*	result = [BRIDGE_CAST(_initBlock(), NSString*) autorelease]; // block returns a copy of a string
	
	
	return result;
}// titleDialog:returnInitialTitleTextForManagedView:


@end // WindowTitleDialog_Handler


#pragma mark -
@implementation WindowTitleDialog_VC


@synthesize titleText = _titleText;


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initForCocoaWindow:(NSWindow*)					aCocoaWindow
orCarbonWindow:(HIWindowRef)					aCarbonWindow
responder:(id< WindowTitleDialog_VCDelegate >)	aResponder
{
	self = [super initWithNibName:@"WindowTitleDialogCocoa" bundle:nil];
	if (nil != self)
	{
		_responder = aResponder;
		_parentCarbonWindow = aCarbonWindow;
		_parentCocoaWindow = aCocoaWindow;
		_titleText = [@"" retain];
		
		// NSViewController implicitly loads the NIB when the "view"
		// property is accessed; force that here
		[self view];
	}
	return self;
}// initForCocoaWindow:orCarbonWindow:responder:


/*!
Initializer for Carbon-based windows.

(4.0)
*/
- (instancetype)
initForCarbonWindow:(HIWindowRef)				aWindow
responder:(id< WindowTitleDialog_VCDelegate >)	aResponder
{
	self = [self initForCocoaWindow:nil orCarbonWindow:aWindow responder:aResponder];
	if (nil != self)
	{
	}
	return self;
}// initForCarbonWindow:responder:


/*!
Initializer for Cocoa-based windows; this will eventually
be the designated initializer.

(4.0)
*/
- (instancetype)
initForCocoaWindow:(NSWindow*)					aWindow
responder:(id< WindowTitleDialog_VCDelegate >)	aResponder
{
	self = [self initForCocoaWindow:aWindow orCarbonWindow:nullptr responder:aResponder];
	if (nil != self)
	{
	}
	return self;
}// initForCocoaWindow:responder:


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[_titleText release];
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
	// remove focus from the text field to ensure that the
	// current value has been committed; otherwise it is
	// possible it will be lost (e.g. user has field focused
	// but uses the mouse to click the button to proceed)
	[[self->titleField window] makeFirstResponder:nil];
	
	// now the binding should have the correct value...
	{
		NSString*	newTitleText = (nil == _titleText)
									? @""
									: [NSString stringWithString:_titleText];
		
		
		[_responder titleDialog:self didFinishUsingManagedView:self.view
										acceptingRename:YES finalTitle:newTitleText];
	}
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
	[_responder titleDialog:self didFinishUsingManagedView:self.view
									acceptingRename:NO finalTitle:nil];
}// performCloseAndRevert:


#pragma mark NSViewController


/*!
Invoked by NSViewController once the "self.view" property is set,
after the NIB file is loaded.  This essentially guarantees that
all file-defined user interface elements are now instantiated and
other settings that depend on valid UI objects can now be made.

NOTE:	As future SDKs are adopted, it makes more sense to only
		implement "viewDidLoad" (which was only recently added
		to NSViewController and is not otherwise available).
		This implementation can essentially move to "viewDidLoad".

(4.1)
*/
- (void)
loadView
{
	[super loadView];
	
	assert(nil != titleField);
	
	[_responder titleDialog:self didLoadManagedView:self.view];
	self.titleText = [_responder titleDialog:self returnInitialTitleTextForManagedView:self.view];
}// loadView


@end // WindowTitleDialog_VC

// BELOW IS REQUIRED NEWLINE TO END FILE
