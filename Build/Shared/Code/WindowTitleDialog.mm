/*!	\file WindowTitleDialog.mm
	\brief Implements a dialog box for changing the title
	of a terminal window.
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

#import "WindowTitleDialog.h"
#import <UniversalDefines.h>

// standard-C includes
#import <climits>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>

// library includes
#import <CocoaBasic.h>
#import <CocoaExtensions.objc++.h>
#import <MemoryBlocks.h>
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
	BOOL										_animated;				// YES if there should be animation when opening/closing
	NSTextAlignment								_arrowAlignment;		// how to place the arrow
	NSSize										_idealSize;				// minimum size of the managed view
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
	animated:(BOOL)_
	whenInitializing:(WindowTitleDialog_ReturnTitleCopyBlock)_
	whenClosing:(WindowTitleDialog_CloseNotifyBlock)_ NS_DESIGNATED_INITIALIZER;

// new methods
	- (void)
	display;
	- (void)
	removeWithAcceptance:(BOOL)_;

// accessors
	@property (assign) NSTextAlignment
	arrowAlignment;
	- (NSWindow*)
	renamedCocoaWindow;

// PopoverManager_Delegate
	// (undeclared)

// WindowTitleDialog_VCDelegate
	// (undeclared)

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
WindowTitleDialog_NewWindowModal	(NSWindow*								inCocoaParentWindow,
									 Boolean								inIsAnimated,
									 WindowTitleDialog_ReturnTitleCopyBlock	inInitBlock,
									 WindowTitleDialog_CloseNotifyBlock		inFinalBlock)
{
	WindowTitleDialog_Ref	result = nullptr;
	
	
	result = (WindowTitleDialog_Ref)[[WindowTitleDialog_Handler alloc]
										initForCocoaWindow:inCocoaParentWindow
															animated:inIsAnimated
															whenInitializing:inInitBlock
															whenClosing:inFinalBlock];
	return result;
}// NewWindowModal


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
		NSBeep();
	}
	else
	{
		// load the view asynchronously and eventually display it in a window
		[ptr display];
	}
}// Display


/*!
Specifies the location of the arrow on the dialog.
(Use this if the window title moves.)

(2020.04)
*/
void
WindowTitleDialog_SetAlignment	(WindowTitleDialog_Ref	inDialog,
								 NSTextAlignment		inPointerAlignment)
{
	WindowTitleDialog_Handler*	ptr = [WindowTitleDialog_Handler viewHandlerFromRef:inDialog];
	
	
	if (nullptr != ptr)
	{
		ptr.arrowAlignment = inPointerAlignment;
	}
}// Display


#pragma mark Internal Methods


#pragma mark -
@implementation WindowTitleDialog_Handler //{


@synthesize arrowAlignment = _arrowAlignment;


#pragma mark Class Methods


/*!
Converts from the opaque reference type to the internal type.

(4.0)
*/
+ (WindowTitleDialog_Handler*)
viewHandlerFromRef:(WindowTitleDialog_Ref)		aRef
{
	return REINTERPRET_CAST(aRef, WindowTitleDialog_Handler*);
}// viewHandlerFromRef


#pragma mark Initializers


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2016.09)
*/
- (instancetype)
init
{
	assert(false && "invalid way to initialize derived class");
	return [self initForCocoaWindow:nil animated:YES whenInitializing:nil whenClosing:nil];
}// init


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
initForCocoaWindow:(NSWindow*)								aCocoaWindow
animated:(BOOL)												anAnimationFlag
whenInitializing:(WindowTitleDialog_ReturnTitleCopyBlock)	anInitBlock
whenClosing:(WindowTitleDialog_CloseNotifyBlock)			aFinalBlock
{
	self = [super init];
	if (nil != self)
	{
		_selfRef = REINTERPRET_CAST(self, WindowTitleDialog_Ref);
		_viewMgr = nil;
		_containerWindow = nil;
		_managedView = nil;
		_targetCocoaWindow = aCocoaWindow;
		_popoverMgr = nullptr;
		_idealSize = NSZeroSize; // set later
		_animated = anAnimationFlag;
		_arrowAlignment = NSTextAlignmentCenter;
		_initBlock = Block_copy(anInitBlock);
		_closeNotifyBlock = Block_copy(aFinalBlock);
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
	Memory_EraseWeakReferences(self);
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


#pragma mark New Methods


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
		_viewMgr = [[WindowTitleDialog_VC alloc]
					initForCocoaWindow:_targetCocoaWindow responder:self];
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
Returns the Cocoa window that represents the renamed window.

(4.0)
*/
- (NSWindow*)
renamedCocoaWindow
{
	NSWindow*	result = _targetCocoaWindow;
	
	
	return result;
}// renamedCocoaWindow


#pragma mark PopoverManager_Delegate


/*!
Assists the dynamic resize of a popover window by indicating
whether or not there are per-axis constraints on resizing.

(2017.05)
*/
- (void)
popoverManager:(PopoverManager_Ref)		aPopoverManager
getHorizontalResizeAllowed:(BOOL*)		outHorizontalFlagPtr
getVerticalResizeAllowed:(BOOL*)		outVerticalFlagPtr
{
#pragma unused(aPopoverManager)
	*outHorizontalFlagPtr = YES;
	*outVerticalFlagPtr = NO;
}// popoverManager:getHorizontalResizeAllowed:getVerticalResizeAllowed:


/*!
Returns the initial view size for the popover.

(2017.05)
*/
- (void)
popoverManager:(PopoverManager_Ref)		aPopoverManager
getIdealSize:(NSSize*)					outSizePtr
{
#pragma unused(aPopoverManager)
	*outSizePtr = _idealSize;
}// popoverManager:getIdealSize:


/*!
Returns the location (relative to the window) where the
popover’s arrow tip should appear.  The location of the
popover itself depends on the arrow placement chosen by
"idealArrowPositionForParentWindowFrame:".

(4.0)
*/
- (NSPoint)
popoverManager:(PopoverManager_Ref)		aPopoverManager
idealAnchorPointForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(aPopoverManager)
	NSRect		screenFrame = [[parentWindow screen] visibleFrame];
	NSRect		managedViewFrame = [_managedView frame];
	NSPoint		result = CGPointZero; // set below
	
	
	if (NSTextAlignmentLeft == _arrowAlignment)
	{
		result.x = 40; // arbitrary
		result.y = (parentFrame.size.height - 18/* arbitrary */);
	}
	else if (NSTextAlignmentRight == _arrowAlignment)
	{
		result.x = (parentFrame.size.width - 40); // arbitrary
		result.y = (parentFrame.size.height - 18/* arbitrary */);
	}
	else
	{
		result.x = CGFLOAT_DIV_2(parentFrame.size.width);
		result.y = (parentFrame.size.height - 12/* arbitrary */);
	}
	
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
}// popoverManager:idealAnchorPointForFrame:parentWindow:


/*!
Returns arrow placement information for the popover.

(4.0)
*/
- (Popover_Properties)
popoverManager:(PopoverManager_Ref)		aPopoverManager
idealArrowPositionForFrame:(NSRect)		parentFrame
parentWindow:(NSWindow*)				parentWindow
{
#pragma unused(aPopoverManager, parentFrame, parentWindow)
	Popover_Properties	result = kPopover_PropertyPlaceFrameBelowArrow;
	
	
	if (NSTextAlignmentLeft == _arrowAlignment)
	{
		result |= kPopover_PropertyArrowBeginning;
	}
	else if (NSTextAlignmentRight == _arrowAlignment)
	{
		result |= kPopover_PropertyArrowEnd;
	}
	else
	{
		result |= kPopover_PropertyArrowMiddle;
	}
	
	return result;
}// popoverManager:idealArrowPositionForFrame:parentWindow:


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
		
		if (@available(macOS 11.0, *))
		{
			// on macOS 11, sheet animations make presenting an arrow-style popover
			// work poorly; therefore, revert to a more normal sheet appearance
			_containerWindow = [[Popover_Window alloc] initWithView:aManagedView
																	windowStyle:kPopover_WindowStyleDialogSheet
																	arrowStyle:kPopover_ArrowStyleNone
																	attachedToPoint:NSZeroPoint/* see delegate */
																	inWindow:asNSWindow];
		}
		else
		{
			_containerWindow = [[Popover_Window alloc] initWithView:aManagedView
																	windowStyle:kPopover_WindowStyleDialogSheet
																	arrowStyle:kPopover_ArrowStyleDefaultRegularSize
																	attachedToPoint:NSZeroPoint/* see delegate */
																	inWindow:asNSWindow];
		}
		_idealSize = [_managedView frame].size;
		[_containerWindow setReleasedWhenClosed:NO];
		_popoverMgr = PopoverManager_New(_containerWindow, [aViewMgr logicalFirstResponder],
											self/* delegate */, animationType, kPopoverManager_BehaviorTypeDialog,
											_targetCocoaWindow.contentView);
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


@end //} WindowTitleDialog_Handler


#pragma mark -
@implementation WindowTitleDialog_VC //{


@synthesize titleText = _titleText;


#pragma mark Initializers


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2016.09)
*/
- (instancetype)
initWithCoder:(NSCoder*)	aCoder
{
#pragma unused(aCoder)
	assert(false && "invalid way to initialize derived class");
	return [self initForCocoaWindow:nil responder:nil];
}// initWithCoder:


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2016.09)
*/
- (instancetype)
initWithNibName:(NSString*)		aNibName
bundle:(NSBundle*)				aBundle
{
#pragma unused(aNibName, aBundle)
	assert(false && "invalid way to initialize derived class");
	return [self initForCocoaWindow:nil responder:nil];
}// initWithNibName:bundle:


/*!
Designated initializer of derived class.

(4.0)
*/
- (instancetype)
initForCocoaWindow:(NSWindow*)					aCocoaWindow
responder:(id< WindowTitleDialog_VCDelegate >)	aResponder
{
	self = [super initWithNibName:@"WindowTitleDialogCocoa" bundle:nil];
	if (nil != self)
	{
		_responder = aResponder;
		_parentCocoaWindow = aCocoaWindow;
		_titleText = [@"" retain];
		
		// NSViewController implicitly loads the NIB when the "view"
		// property is accessed; force that here
		[self view];
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


@end //} WindowTitleDialog_VC

// BELOW IS REQUIRED NEWLINE TO END FILE
