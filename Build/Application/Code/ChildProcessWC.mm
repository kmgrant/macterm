/*!	\file ChildProcessWC.mm
	\brief A window that is a local proxy providing access to
	a window in another process.
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

#import <UniversalDefines.h>

// Mac includes
@import Cocoa;

// library includes
#import <CocoaExtensions.objc++.h>
#import <Console.h>

// application includes
#import "ChildProcessWC.objc++.h"
#import "Commands.h"



#pragma mark Internal Method Prototypes

/*!
Window class for subprocess proxies.
*/
@interface ChildProcessWC_Window : NSWindow //{

@end //}

/*!
The private class interface.
*/
@interface ChildProcessWC_Object (ChildProcessWC_ObjectInternal) //{

// new methods
	- (void)
	handleChildExit;
	- (ChildProcessWC_Window*)
	newWindow;

@end //}

@interface ChildProcessWC_Object () //{

// accessors
	//! The block to invoke when the subprocess exits for any reason. 
	@property (strong) ChildProcessWC_AtExitBlockType
	atExitBlock;
	//! A representation of the child process that displays the window.
	@property (strong) NSRunningApplication*
	childApplication;
	//! Ensures that the exit block is invoked only once.
	@property (assign) BOOL
	exitHandled;
	//! For key-value observing of key properties of the subprocess.
	@property (strong) NSMutableArray*
	registeredObservers;
	//! Helps to keep track of user-initiated requests to close the window
	//! so that it does not happen twice (e.g. if process happens to exit
	//! while already handling a user-requested window close).
	@property (assign) BOOL
	terminationRequestInProgress;

@end //}

#pragma mark Variables
namespace {

NSMutableArray*		gChildProcessWindowProxies()	{ static NSMutableArray* _ = [[NSMutableArray alloc] init]; return _; } // array of ChildProcessWC_Object*

} // anonymous namespace



#pragma mark Public Methods

#pragma mark -
@implementation ChildProcessWC_Object //{


#pragma mark Class Methods


/*!
Convenience initializer.

(2016.09)
*/
+ (instancetype)
childProcessWCWithRunningApp:(NSRunningApplication*)	aRunningApp
{
	return [[self.class alloc] initWithRunningApp:aRunningApp];
}// childProcessWCWithRunningApp:


/*!
Convenience initializer.

(2016.09)
*/
+ (instancetype)
childProcessWCWithRunningApp:(NSRunningApplication*)	aRunningApp
atExit:(ChildProcessWC_AtExitBlockType)					anExitBlock
{
	return [[self.class alloc] initWithRunningApp:aRunningApp atExit:anExitBlock];
}// childProcessWCWithRunningApp:modalToWindow:atExit:


#pragma mark Initializers


/*!
Convenience initializer.

(2016.09)
*/
- (instancetype)
initWithRunningApp:(NSRunningApplication*)	aRunningApp
{
	return [self initWithRunningApp:aRunningApp atExit:nil];
}// initWithRunningApp:


/*!
Designated initializer.

(2016.09)
*/
- (instancetype)
initWithRunningApp:(NSRunningApplication*)	aRunningApp
atExit:(ChildProcessWC_AtExitBlockType)		anExitBlock
{
	ChildProcessWC_Window*		window = [self newWindow];
	
	
	self = [super initWithWindow:window];
	if (nil != self)
	{
		_childApplication = aRunningApp;
		_registeredObservers = [[NSMutableArray alloc] init];
		[self.registeredObservers addObject:[self newObserverFromKeyPath:@"terminated" ofObject:aRunningApp
																			options:(NSKeyValueChangeSetting)]];
		if (nullptr != anExitBlock)
		{
			_atExitBlock = anExitBlock;
		}
		else
		{
			_atExitBlock = nullptr;
		}
		_exitHandled = NO;
		_terminationRequestInProgress = NO;
		
		[self whenObject:NSApp postsNote:NSApplicationWillBecomeActiveNotification
							performSelector:@selector(applicationDidBecomeActive:)];
		
		// retain the window controller until the child exits or the
		// window is otherwise forced to close (this is much more
		// convenient for callers)
		[gChildProcessWindowProxies() addObject:self];
	}
	return self;
}// initWithRunningApp:atExit:


/*!
Destructor.

(2016.09)
*/
- (void)
dealloc
{
	//NSLog(@"ChildProcessWC_Object dealloc"); // debug
	[self handleChildExit];
	[self ignoreWhenObjectsPostNotes];
	[self removeObserversSpecifiedInArray:self.registeredObservers];
}// dealloc


#pragma mark Initializers Disabled From Superclass


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithCoder:(NSCoder*)	aCoder
{
#pragma unused(aCoder)
	assert(false && "invalid way to initialize derived class");
	return [self initWithRunningApp:nil];
}// initWithCoder:


/*!
Designated initializer from base class.  Do not use;
it is defined only to satisfy the compiler.

(2017.06)
*/
- (instancetype)
initWithWindow:(NSWindow*)		aWindow
{
#pragma unused(aWindow)
	assert(false && "invalid way to initialize derived class");
	return [self initWithRunningApp:nil];
}// initWithWindow:


#pragma mark Notifications


/*!
Handles a return to the current process.

(2016.09)
*/
- (void)
applicationDidBecomeActive:(NSNotification*)	aNote
{
#pragma unused(aNote)
	// nothing yet
}// applicationDidBecomeActive:


#pragma mark NSKeyValueObserving


/*!
Intercepts changes to observed key values.

(2016.09)
*/
- (void)
observeValueForKeyPath:(NSString*)	aKeyPath
ofObject:(id)						anObject
change:(NSDictionary*)				aChangeDictionary
context:(void*)						aContext
{
	BOOL	handled = NO;
	
	
	if ([self observerArray:self.registeredObservers containsContext:aContext])
	{
		handled = YES;
		
		if (NSKeyValueChangeSetting == [[aChangeDictionary objectForKey:NSKeyValueChangeKindKey] intValue])
		{
			if ([aKeyPath isEqualToString:@"terminated"])
			{
				if (self.terminationRequestInProgress)
				{
					// termination has completed; this should exactly match the
					// response in "windowWillClose:" for the non-request case
					[self handleChildExit];
					self.terminationRequestInProgress = NO;
				}
				else
				{
					// child process exited without explicit window close; now
					// the local proxy window should be destroyed
					[self close];
				}
			}
			else
			{
				Console_Warning(Console_WriteValueCFString, "valid observer context is not handling key path", BRIDGE_CAST(aKeyPath, CFStringRef));
			}
		}
	}
	
	if (NO == handled)
	{
		[super observeValueForKeyPath:aKeyPath ofObject:anObject change:aChangeDictionary context:aContext];
	}
}// observeValueForKeyPath:ofObject:change:context:


#pragma mark NSWindowDelegate


/*!
Creates the illusion of “activating” the window by switching
to the other running process.

(2016.09)
*/
- (void)
windowDidBecomeMain:(NSNotification*)	aNote
{
#pragma unused(aNote)
	// since the window “becomes main” again when the
	// user activates this application later, it is
	// necessary to force a move to the next window
	if (NO == self.childApplication.isTerminated)
	{
		BOOL	isSomeOnscreenWindow = NO;
		
		
		// ensure that some open window is visible to the user (e.g.
		// it is possible the user has used the Hide feature to
		// obscure them all); otherwise there is no need to switch
		for (NSWindow* aWindow in NSApp.windows)
		{
			if ((aWindow.isVisible) &&
				(NO == [aWindow isKindOfClass:ChildProcessWC_Window.class]) &&
				(nil != aWindow.screen) && ([NSScreen mainScreen] == aWindow.screen))
			{
				isSomeOnscreenWindow = YES;
				break;
			}
		}
		
		if (isSomeOnscreenWindow)
		{
			[[Commands_Executor sharedExecutor] orderFrontNextWindow:nil];
			[self.childApplication activateWithOptions:(0)];
		}
	}
}// windowDidBecomeMain:


/*!
Runs the at-exit block, since a closing window indicates that
the child process has exited.

(2016.09)
*/
- (void)
windowWillClose:(NSNotification*)	aNote
{
#pragma unused(aNote)
	//NSLog(@"ChildProcessWC_Object will close"); // debug
	
	// WARNING: termination may happen asynchronously so
	// monitor the "terminated" property and continue the
	// exit processing only when the change is received;
	// see "observeValueForKeyPath:ofObject:change:context:"
	self.terminationRequestInProgress = YES;
	
	// although typically the close is a side effect of
	// the child already having exited, an explicit close
	// should cause the child to exit if necessary (not
	// in a forced way — for now, at least)
	if (NO == [self.childApplication terminate])
	{
		// process has already exited; this should exactly match the
		// response in "observeValueForKeyPath:ofObject:change:context:"
		// for the request case
		self.terminationRequestInProgress = NO;
		[self handleChildExit];
	}
}// windowWillClose:


@end //} ChildProcessWC_Object


#pragma mark Internal Methods

#pragma mark -
@implementation ChildProcessWC_Object (ChildProcessWC_ObjectInternal) //{


#pragma mark New Methods


/*!
Calls any exit block and removes a retain-lock from
this window controller.  Has no effect if the exit
status has already been handled.

(2016.09)
*/
- (void)
handleChildExit
{
	//NSLog(@"ChildProcessWC_Object handle exit"); // debug
	if (NO == self.exitHandled)
	{
		if (nil != self.atExitBlock)
		{
			self.atExitBlock();
		}
		
		// NOTE: if "dealloc" is the trigger then obviously
		// this part will have no effect
		[gChildProcessWindowProxies() removeObject:self];
		
		self.exitHandled = YES;
	}
}// handleChildExit


/*!
Creates a local window in the application that acts as a proxy
for a window in a different process.  See the initializer.

(2017.06)
*/
- (ChildProcessWC_Window*)
newWindow
{
	ChildProcessWC_Window*	result = [[ChildProcessWC_Window alloc]
										initWithContentRect:NSMakeRect(0, 0, 1, 1)
															styleMask:NSWindowStyleMaskTitled
															backing:NSBackingStoreBuffered
															defer:NO];
	
	
	// enable "windowDidBecomeMain:", etc.
	result.delegate = self;
	
	// by default, put this window offscreen (presumably it is not
	// meant to display anything locally, as the child process
	// version of the window will suffice)
	[result setFrameOrigin:NSMakePoint(-16000, -16000)/* arbitrary */];
	
	// the proxy must be displayed so that its close operation
	// can be used to detect the end of the process, and so
	// that the user can select it as part of the window cycle;
	// although, "orderBack:" is preferred because that creates
	// a more natural cycling (the child process window will
	// already be active, and rotating back to the parent
	// should cause every other window to be visited before
	// seeing the child again)
	[result orderBack:nil];
	
	return result;
}// newWindow


@end //} ChildProcessWC_Object (ChildProcessWC_ObjectInternal)


#pragma mark -
@implementation ChildProcessWC_Window //{


#pragma mark Initializers


/*!
Designated initializer.

(2021.01)
*/
- (instancetype)
initWithContentRect:(NSRect)	contentRect
styleMask:(NSWindowStyleMask)	aStyle
backing:(NSBackingStoreType)	bufferingType
defer:(BOOL)					flag
{
	self = [super initWithContentRect:contentRect styleMask:aStyle backing:bufferingType defer:flag];
	if (nil != self)
	{
	}
	return self;
}// initWithContentRect:styleMask:backing:defer:


@end //} ChildProcessWC_Window

// BELOW IS REQUIRED NEWLINE TO END FILE
