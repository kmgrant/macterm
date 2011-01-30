/*!	\file GrowlSupport.mm
	\brief Growl Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.5
	© 2008-2011 by Kevin Grant
	
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

// Mac includes
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>

// library includes
#import <AutoPool.objc++.h>
#import <Console.h>
#import <Growl/Growl.h>



#pragma mark Types

@interface GrowlSupport_Delegate : NSObject<GrowlApplicationBridgeDelegate>
{
	BOOL	isReady;
}

+ (id)
sharedGrowlDelegate;

- (BOOL)
isReady;

@end

#pragma mark Variables
namespace {

BOOL		gGrowlFrameworkIsLinked = NO;
NSString*	gGrowlPrefsPaneGlobalPath = @"/Library/PreferencePanes/Growl.prefPane";

} // anonymous namespace



#pragma mark Public Methods

/*!
Initializes the Growl delegate.

(1.2)
*/
void
GrowlSupport_Init ()
{
	AutoPool	_;
	
	
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
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
#endif
}// Init


/*!
Returns true only if the Growl daemon is installed and
running, indicating that it is okay to send Growl
notifications.

(1.0)
*/
Boolean
GrowlSupport_IsAvailable ()
{
	AutoPool	_;
	Boolean		result = false;
	
	
	result = ((YES == gGrowlFrameworkIsLinked) && (YES == [[GrowlSupport_Delegate sharedGrowlDelegate] isReady]));
	return result;
}// IsAvailable


/*!
Posts the specified notification to Growl.  The name should match
one of the strings in "Growl Registration Ticket.growlRegDict".
The title and description can be anything.  If the title is set
to nullptr, it copies the notification name; if the description
is set to nullptr, it is set to an empty string.

Has no effect if Growl is not installed and available.

(1.0)
*/
void
GrowlSupport_Notify		(CFStringRef	inNotificationName,
						 CFStringRef	inTitle,
						 CFStringRef	inDescription)
{
	AutoPool	_;
	
	
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	if (GrowlSupport_IsAvailable())
	{
		if (nullptr == inTitle) inTitle = inNotificationName;
		if (nullptr == inDescription) inDescription = CFSTR("");
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
				notify with name \"%@\" title \"%@\" description \"%@\" application name \"MacTelnet\"\n\
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
#endif
}// Notify


/*!
Returns true only if the Growl preferences pane exists in
its expected location.

(1.5)
*/
Boolean
GrowlSupport_PreferencesPaneCanDisplay ()
{
	AutoPool	_;
	Boolean		result = ([[NSFileManager defaultManager] isReadableFileAtPath:gGrowlPrefsPaneGlobalPath]) ? true : false;
	
	
	return result;
}// PreferencesPaneCanDisplay


/*!
Launches “System Preferences” to open the Growl preferences
pane, if it is available.

(1.5)
*/
void
GrowlSupport_PreferencesPaneDisplay ()
{
	AutoPool	_;
	
	
	if (GrowlSupport_PreferencesPaneCanDisplay())
	{
		(BOOL)[[NSWorkspace sharedWorkspace] openFile:gGrowlPrefsPaneGlobalPath];
	}
}// PreferencesPaneDisplay


#pragma mark Internal Methods

@implementation GrowlSupport_Delegate

static GrowlSupport_Delegate*	gGrowlSupport_Delegate = nil;
+ (id)
sharedGrowlDelegate
{
	if (nil == gGrowlSupport_Delegate)
	{
		gGrowlSupport_Delegate = [[[self class] allocWithZone:NULL] init];
	}
	return gGrowlSupport_Delegate;
}


- (id)
init
{
	self = [super init];
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	self->isReady = ([GrowlApplicationBridge isGrowlInstalled] && [GrowlApplicationBridge isGrowlRunning]);
#else
	self->isReady = NO;
#endif
	return self;
}
- (void)
dealloc
{
}// dealloc


- (BOOL)
isReady
{
	return self->isReady;
}// isReady


#pragma mark GrowlApplicationBridgeDelegate

- (void)
growlIsReady
{
	// this might only be received upon restart of Growl, not at startup;
	// but it is handled in case Growl is started after MacTelnet starts
	self->isReady = true;
}// growlIsReady

@end

// BELOW IS REQUIRED NEWLINE TO END FILE
