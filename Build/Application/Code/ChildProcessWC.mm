/*!	\file ChildProcessWC.mm
	\brief A window that is a local proxy providing access to
	a window in another process.
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

#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <CocoaExtensions.objc++.h>

// application includes
#import "ChildProcessWC.objc++.h"
#import "Commands.h"



#pragma mark Internal Method Prototypes

/*!
The private class interface.
*/
@interface ChildProcessWC_Object (ChildProcessWC_ObjectInternal) //{

// new methods
	- (void)
	handleChildExit;
	- (NSWindow*)
	newWindow;

@end //}

@interface ChildProcessWC_Object () //{

// accessors
	@property (copy) void
	(^atExitBlock)();
	@property (retain) NSRunningApplication*
	childApplication;
	@property (strong) NSMutableArray*
	registeredObservers;

@end //}

#pragma mark Variables
namespace {

NSMutableArray*&	gChildProcessWindowProxies()		{ static NSMutableArray* _ = [[NSMutableArray alloc] init]; return _; } // array of ChildProcessWC_Object*

} // anonymous namespace



#pragma mark Public Methods

#pragma mark -
@implementation ChildProcessWC_Object //{


@synthesize atExitBlock = _atExitBlock;
@synthesize childApplication = _childApplication;
@synthesize registeredObservers = _registeredObservers;


#pragma mark Class Methods


/*!
Convenience initializer.

(2016.09)
*/
+ (instancetype)
childProcessWCWithRunningApp:(NSRunningApplication*)		aRunningApp
{
	return [[[self.class alloc] initWithRunningApp:aRunningApp] autorelease];
}// childProcessWCWithRunningApp:


/*!
Convenience initializer.

(2016.09)
*/
+ (instancetype)
childProcessWCWithRunningApp:(NSRunningApplication*)		aRunningApp
atExit:(ChildProcessWC_AtExitBlockType)					anExitBlock
{
	return [[[self.class alloc] initWithRunningApp:aRunningApp atExit:anExitBlock] autorelease];
}// childProcessWCWithRunningApp:modalToWindow:atExit:


#pragma mark Initializers


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
	NSWindow*	window = [self newWindow];
	
	
	self = [super initWithWindow:window];
	if (nil != self)
	{
		_childApplication = [aRunningApp retain];
		_registeredObservers = [[NSMutableArray alloc] init];
		[self.registeredObservers addObject:[[self newObserverFromKeyPath:@"terminated" ofObject:aRunningApp
																			options:(NSKeyValueChangeSetting)]
											autorelease]];
		if (nullptr != anExitBlock)
		{
			_atExitBlock = Block_copy(anExitBlock);
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


/*!
Destructor.

(2016.09)
*/
- (void)
dealloc
{
	//NSLog(@"ChildProcessWC_Object dealloc"); // debug
	[self handleChildExit];
	if (nullptr != self->_atExitBlock)
	{
		Block_release(self->_atExitBlock);
	}
	[self ignoreWhenObjectsPostNotes];
	[self removeObserversSpecifiedInArray:self.registeredObservers];
	[_childApplication release];
	[_registeredObservers release];
	[super dealloc];
}// dealloc


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
				if (_terminationRequestInProgress)
				{
					// termination has completed; this should exactly match the
					// response in "windowWillClose:" for the non-request case
					[self handleChildExit];
					_terminationRequestInProgress = NO;
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
	//BOOL	canNext = [[[NSApp keyWindow] firstResponder] tryToPerform:@selector(orderFrontNextWindow:) with:nil];
	[[Commands_Executor sharedExecutor] orderFrontNextWindow:nil];
	[self.childApplication activateWithOptions:(0)];
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
	_terminationRequestInProgress = YES;
	
	// although typically the close is a side effect of
	// the child already having exited, an explicit close
	// should cause the child to exit if necessary (not
	// in a forced way — for now, at least)
	if (NO == [self.childApplication terminate])
	{
		// process has already exited; this should exactly match the
		// response in "observeValueForKeyPath:ofObject:change:context:"
		// for the request case
		_terminationRequestInProgress = NO;
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
	if (NO == _exitHandled)
	{
		if (nil != self.atExitBlock)
		{
			self.atExitBlock();
		}
		
		// NOTE: if "dealloc" is the trigger then obviously
		// this part will have no effect
		[gChildProcessWindowProxies() removeObject:self];
		
		_exitHandled = YES;
	}
}// handleChildExit


/*!
Creates a local window in the application that acts as a proxy
for a window in a different process.  See the initializer.

(2017.06)
*/
- (NSWindow*)
newWindow
{
	NSWindow*		result = [[NSWindow alloc]
								initWithContentRect:NSMakeRect(0, 0, 1, 1)
												#if MAC_OS_X_VERSION_MIN_REQUIRED < 101200 /* MAC_OS_X_VERSION_10_12 */
													styleMask:NSTitledWindowMask
												#else
													styleMask:NSWindowStyleMaskTitled
												#endif
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

// BELOW IS REQUIRED NEWLINE TO END FILE
