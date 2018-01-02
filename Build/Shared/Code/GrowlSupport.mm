/*!	\file GrowlSupport.mm
	\brief Growl Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2018 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#import <GrowlSupport.h>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>

// library includes
#import <CocoaFuture.objc++.h>
#import <Console.h>
#import <Growl/Growl.h>



#pragma mark Types

/*!
This protocol exists in order to suppress warnings about
undeclared selectors used in "@selector()" calls below.
There is code to dynamically check for selectors that are
only available in the 10.8 SDK for system notifications.
*/
@protocol GrowlSupport_LikeMountainLionNotifications //{

	// 10.8: selector to request delivery of a notification
	- (void)
	deliverNotification:(id)_;

	// 10.8: selector to set sound name in a user notification object
	- (void)
	setSoundName:(NSString*)_;

	// 10.8: selector to set subtitle in a user notification object
	- (void)
	setSubtitle:(NSString*)_;

@end //}


/*!
Interfaces with Growl for notifications, if it is installed.
*/
@interface GrowlSupport_Delegate : NSObject<GrowlApplicationBridgeDelegate> //{
{
	BOOL	isReady;
}

// class methods
	+ (id)
	sharedGrowlDelegate;

// accessors
	- (BOOL)
	isReady;

@end //}


/*!
On Mac OS X 10.8 and beyond, interfaces with the Mac OS X
user notification system.
*/
@interface GrowlSupport_MacUserNotificationDelegate : NSObject<NSUserNotificationCenterDelegate> //{

// NSUserNotificationCenterDelegate
	- (void)
	userNotificationCenter:(NSUserNotificationCenter*)_
	didDeliverNotification:(NSUserNotification*)_;
	- (void)
	userNotificationCenter:(NSUserNotificationCenter*)_
	didActivateNotification:(NSUserNotification*)_;
	- (BOOL)
	userNotificationCenter:(NSUserNotificationCenter*)_
	shouldPresentNotification:(NSUserNotification*)_;

@end //}

#pragma mark Variables
namespace {

BOOL		gGrowlFrameworkIsLinked = NO;
NSString*	gGrowlPrefsPaneGlobalPath = @"/Library/PreferencePanes/Growl.prefPane";
id			gMountainLionUserNotificationCenter = nil;

} // anonymous namespace



#pragma mark Public Methods

/*!
Initializes the Growl delegate.

(1.2)
*/
void
GrowlSupport_Init ()
{
@autoreleasepool {
	// If supported by the current OS, forward notifications to the system’s
	// Notification Center.  This requires a bit of trickery because the
	// current code base is compiled to support older OS versions (and yet
	// is able to detect and use newer OS features).
	gMountainLionUserNotificationCenter = CocoaFuture_DefaultUserNotificationCenter();
	if (nil != gMountainLionUserNotificationCenter)
	{
		if ([gMountainLionUserNotificationCenter respondsToSelector:@selector(setDelegate:)])
		{
			GrowlSupport_MacUserNotificationDelegate*	delegate = [[GrowlSupport_MacUserNotificationDelegate alloc] init];
			
			
			[gMountainLionUserNotificationCenter performSelector:@selector(setDelegate:) withObject:delegate];
		}
	}
	
	// IMPORTANT: The framework is weak-linked so that the application will
	// always launch on older Mac OS X versions; check for a defined symbol
	// before attempting to call ANYTHING in Growl.  Other code in this file
	// should check that "gGrowlFrameworkIsLinked" is YES before using Growl.
	// And ideally, NO OTHER SOURCE FILE should directly depend on Growl!
	if (NULL != objc_getClass("GrowlApplicationBridge"))
	{
		gGrowlFrameworkIsLinked = YES;
	}
	else
	{
		gGrowlFrameworkIsLinked = NO;
	}
	
	if (gGrowlFrameworkIsLinked)
	{
		[GrowlApplicationBridge setGrowlDelegate:[GrowlSupport_Delegate sharedGrowlDelegate]];
	}
}// @autoreleasepool
}// Init


/*!
Returns true only if the Growl daemon is installed and
running (indicating that it is okay to send Growl
notifications) or if the system is running Mac OS X 10.8
or later (indicating that notifications could arrive in
the system).

(1.0)
*/
Boolean
GrowlSupport_IsAvailable ()
{
@autoreleasepool {
	Boolean		result = false;
	
	
	result = (((YES == gGrowlFrameworkIsLinked) && (YES == [[GrowlSupport_Delegate sharedGrowlDelegate] isReady])) ||
				(NSAppKitVersionNumber > NSAppKitVersionNumber10_7));
	return result;
}// @autoreleasepool
}// IsAvailable


/*!
Posts the specified notification to Growl and (on Mac OS X 10.8 or
later) the system’s User Notification Center, if "inDisplay"
indicates a type of notification that Mac OS X can handle well.
This call has no effect on Mac OS X 10.3.

The name should come from "Growl Registration Ticket.growlRegDict".
The title and description can be anything.  If the title is set to
nullptr, it copies the notification name; if the description is set
to nullptr, it is set to an empty string.

The "inSubTitle" and "inSoundName" parameters can only be used with
the system’s notification method (Mac OS X 10.8 and later).  Set
the sound to nullptr if no sound should be played.

Growl notifications have no effect if Growl is not installed and
available.

System notifications on Mac OS X 10.8 and later can occur even if
Growl is installed; it is assumed that the user will disable the
notifications for the mechanism that they would prefer not to use.
Currently only "kGrowlSupport_NoteDisplayAlways" notes will be sent
to Mac OS X.

(1.0)
*/
void
GrowlSupport_Notify		(GrowlSupport_NoteDisplay	inDisplay,
						 CFStringRef				inNotificationName,
						 CFStringRef				inTitle,
						 CFStringRef				inDescription,
						 CFStringRef				inSubTitle,
						 CFStringRef				inSoundName)
{
@autoreleasepool {
	if (nullptr == inTitle)
	{
		inTitle = inNotificationName;
	}
	
	if (nullptr == inDescription)
	{
		inDescription = CFSTR("");
	}
	
	// NOTE: The “Cocoa Future” module can be called on any OS version but
	// the methods will only be implemented on Mac OS X 10.8 and beyond.
	// EVERY method call must check for a definition first!
	if ((nil != gMountainLionUserNotificationCenter) &&
		(kGrowlSupport_NoteDisplayAlways == inDisplay))
	{
		id		newNote = CocoaFuture_AllocInitUserNotification();
		
		
		if (nil != newNote)
		{
			// title
			if ([newNote respondsToSelector:@selector(setTitle:)])
			{
				[newNote performSelector:@selector(setTitle:) withObject:BRIDGE_CAST(inTitle, NSString*)];
			}
			
			// informative text
			if ([newNote respondsToSelector:@selector(setInformativeText:)])
			{
				[newNote performSelector:@selector(setInformativeText:) withObject:BRIDGE_CAST(inDescription, NSString*)];
			}
			
			// subtitle (if any)
			if ([newNote respondsToSelector:@selector(setSubtitle:)] && (nullptr != inSubTitle))
			{
				[newNote performSelector:@selector(setSubtitle:) withObject:BRIDGE_CAST(inSubTitle, NSString*)];
			}
			
			// sound (if any; API allows a "nil" value)
			if ([newNote respondsToSelector:@selector(setSoundName:)])
			{
				[newNote performSelector:@selector(setSoundName:) withObject:BRIDGE_CAST(inSoundName, NSString*)];
			}
		}
		
		if ([gMountainLionUserNotificationCenter respondsToSelector:@selector(deliverNotification:)])
		{
			[gMountainLionUserNotificationCenter performSelector:@selector(deliverNotification:) withObject:newNote];
		}
	}
	
	if (GrowlSupport_IsAvailable())
	{
	#if 1
		// normally an Objective-C call is enough, but see below
		[GrowlApplicationBridge
			notifyWithTitle:(NSString*)inTitle
			description:(NSString*)inDescription
			notificationName:(NSString*)inNotificationName
			iconData:nil
			priority:0
			isSticky:NO
			clickContext:nil];
	#else
		// prior to Growl 1.1.3, AppleScript was the only way
		// notifications would work on Leopard if certain 3rd-party
		// software was installed; but this is probably slower so
		// it is avoided unless there is a good reason to use it
		NSDictionary*			errorDict = nil;
		NSAppleEventDescriptor*	returnDescriptor = nil;
		NSString*				scriptText = [[NSString alloc] initWithFormat:@"\
			tell application \"GrowlHelperApp\"\n\
				notify with name \"%@\" title \"%@\" description \"%@\" application name \"MacTerm\"\n\
			end tell",
			(NSString*)inNotificationName,
			(NSString*)inTitle,
			(NSString*)inDescription
		];
		NSAppleScript*			scriptObject = [[NSAppleScript alloc] initWithSource:scriptText];
		
		
		returnDescriptor = [scriptObject executeAndReturnError:&errorDict];
		if (nullptr == returnDescriptor)
		{
			NSLog(@"%@", errorDict);
			Console_Warning(Console_WriteLine, "unable to send notification via AppleScript, error:");
		}
		[scriptObject release];
	#endif
	}
}// @autoreleasepool
}// Notify


/*!
Returns true only if the Growl preferences pane exists in
its expected location.

(1.5)
*/
Boolean
GrowlSupport_PreferencesPaneCanDisplay ()
{
@autoreleasepool {
	Boolean		result = ([[NSFileManager defaultManager] isReadableFileAtPath:gGrowlPrefsPaneGlobalPath]) ? true : false;
	
	
	return result;
}// @autoreleasepool
}// PreferencesPaneCanDisplay


/*!
Launches “System Preferences” to open the Growl preferences
pane, if it is available.

(1.5)
*/
void
GrowlSupport_PreferencesPaneDisplay ()
{
@autoreleasepool {
	if (GrowlSupport_PreferencesPaneCanDisplay())
	{
		UNUSED_RETURN(BOOL)[[NSWorkspace sharedWorkspace] openFile:gGrowlPrefsPaneGlobalPath];
	}
}// @autoreleasepool
}// PreferencesPaneDisplay


#pragma mark Internal Methods

#pragma mark -
@implementation GrowlSupport_Delegate


static GrowlSupport_Delegate*	gGrowlSupport_Delegate = nil;


/*!
Returns the singleton.

(4.0)
*/
+ (id)
sharedGrowlDelegate
{
	if (nil == gGrowlSupport_Delegate)
	{
		gGrowlSupport_Delegate = [[self.class allocWithZone:NULL] init];
	}
	return gGrowlSupport_Delegate;
}// sharedGrowlDelegate


/*!
Designated initializer.

(4.0)
*/
- (instancetype)
init
{
	self = [super init];
	self->isReady = [GrowlApplicationBridge isGrowlRunning];
	return self;
}// init


/*!
Destructor.

(4.0)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


/*!
Returns true only if the delegate can handle notifications.

(4.0)
*/
- (BOOL)
isReady
{
	return self->isReady;
}// isReady


#pragma mark GrowlApplicationBridgeDelegate


/*!
Invoked by Growl when it is ready to interact with this
application.

(4.0)
*/
- (void)
growlIsReady
{
	// this might only be received upon restart of Growl and not at startup, but
	// it is handled in case Growl is started after the application launches
	self->isReady = true;
}// growlIsReady


@end


#pragma mark -
@implementation GrowlSupport_MacUserNotificationDelegate


#pragma mark NSUserNotificationCenterDelegate


/*!
Not used.

(4.1)
*/
- (void)
userNotificationCenter:(NSUserNotificationCenter*)	aCenter
didDeliverNotification:(NSUserNotification*)		aNotification
{
#pragma unused(aCenter, aNotification)
}// userNotificationCenter:didDeliverNotification:


/*!
Not used.

(4.1)
*/
- (void)
userNotificationCenter:(NSUserNotificationCenter*)	aCenter
didActivateNotification:(NSUserNotification*)		aNotification
{
#pragma unused(aCenter, aNotification)
}// userNotificationCenter:didActivateNotification:


/*!
Returns YES for all notifications, as currently all
notifications could be useful in the foreground and
there is no facility set up yet to allow individual
notifications to receive feedback (this has historically
always been a “send and forget” style of API).

(4.1)
*/
- (BOOL)
userNotificationCenter:(NSUserNotificationCenter*)	aCenter
shouldPresentNotification:(NSUserNotification*)		aNotification
{
#pragma unused(aCenter, aNotification)
	return YES;
}// userNotificationCenter:shouldPresentNotification:


@end // GrowlSupport_MacUserNotificationDelegate

// BELOW IS REQUIRED NEWLINE TO END FILE
