/*!	\file MainEntryPoint.mm
	\brief Front end to the Bug Reporter application.
*/
/*###############################################################

	Bug Reporter
		© 2005-2016 by Kevin Grant.
	
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

#import "UniversalDefines.h"

// standard-C++ includes
#import <iostream>

// Mac includes
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CoreFoundation.h>

// library includes
#import <CFUtilities.h>



#pragma mark Internal Method Prototypes

/*!
The delegate for NSApplication.
*/
@interface BugReporterAppDelegate : NSObject //{

// NSApplicationDelegate
	- (void)
	applicationDidFinishLaunching:(NSNotification*)_;

@end //}



#pragma mark Public Methods

/*!
Main entry point.
*/
int
main	(int			argc,
		 const char*	argv[])
{
	return NSApplicationMain(argc, argv);
}


#pragma mark Internal Methods

@implementation BugReporterAppDelegate


#pragma mark NSApplicationDelegate

- (void)
applicationDidFinishLaunching:(NSNotification*)		aNotification
{
#pragma unused(aNotification)

	NSAlert*	alertBox = [[[NSAlert alloc] init] autorelease];
	NSString*	button1 = NSLocalizedString(@"Compose E-Mail", @"button label");
	NSString*	button2 = NSLocalizedString(@"Quit Without Reporting", @"button label");
	NSString*	messageText = NSLocalizedString
								(@"MacTerm has quit because of a software defect.  Please notify the authors so this can be fixed.",
									@"main message");
	NSString*	helpText = NSLocalizedString
							(@"A starting point for your mail message will be automatically created for you.",
								@"main help text");
	int			clickedButton = NSAlertFirstButtonReturn;
	BOOL		sendMail = NO;
	
	
	// bring this process to the front
	[NSApp activateIgnoringOtherApps:YES];
	
	// display an error to the user, with options
	alertBox.messageText = messageText;
	alertBox.informativeText = helpText;
	UNUSED_RETURN(NSButton*)[alertBox addButtonWithTitle:button1];
	UNUSED_RETURN(NSButton*)[alertBox addButtonWithTitle:button2];
	clickedButton = [alertBox runModal];
	switch (clickedButton)
	{
	case NSAlertSecondButtonReturn:
		// quit without doing anything
		break;
	
	case NSAlertFirstButtonReturn:
	default:
		// compose an E-mail with details on the error
		sendMail = YES;
		break;
	}
	
	// if appropriate, ask a mail program to compose a new E-mail
	if (sendMail)
	{
		CFStringRef		theURLCFString = CFUtilities_StringCast
											(CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
												CFSTR("BugReporterMailURLToOpen")));
		CFBundleRef		buggyAppBundle = nullptr;
		unsigned int	failureMode = 0;
		BOOL			failedToSubmitReport = NO;
		
		
		// “load” the buggy application bundle, which assumes that this app
		// is physically located within the Resources folder of that bundle
		// (so in particular, THIS WILL NOT WORK IN TESTING unless you
		// PHYSICALLY MOVE the built executable into the Resources directory
		// of a built application bundle!!!)
		{
			// create “...Foo.app/Contents/Resources/BugReporter.app”
			CFURLRef	bugReporterBundleURL = CFBundleCopyBundleURL(CFBundleGetMainBundle());
			
			
			if (nullptr != bugReporterBundleURL)
			{
				// create “...Foo.app/Contents/Resources”
				CFURLRef	buggyAppBundleResourcesURL = CFURLCreateCopyDeletingLastPathComponent
															(kCFAllocatorDefault, bugReporterBundleURL);
				
				
				if (nullptr != buggyAppBundleResourcesURL)
				{
					// create “...Foo.app/Contents”
					CFURLRef	buggyAppBundleContentsURL = CFURLCreateCopyDeletingLastPathComponent
															(kCFAllocatorDefault, buggyAppBundleResourcesURL);
					
					
					if (nullptr != buggyAppBundleContentsURL)
					{
						CFURLRef	buggyAppBundleURL = CFURLCreateCopyDeletingLastPathComponent
														(kCFAllocatorDefault, buggyAppBundleContentsURL);
						
						
						if (nullptr != buggyAppBundleURL)
						{
							buggyAppBundle = CFBundleCreate(kCFAllocatorDefault, buggyAppBundleURL);
							CFRelease(buggyAppBundleURL), buggyAppBundleURL = nullptr;
						}
						CFRelease(buggyAppBundleContentsURL), buggyAppBundleContentsURL = nullptr;
					}
					CFRelease(buggyAppBundleResourcesURL), buggyAppBundleResourcesURL = nullptr;
				}
				CFRelease(bugReporterBundleURL), bugReporterBundleURL = nullptr;
			}
		}
		
		// launch the bug report URL given in this application’s Info.plist
		// (most likely a mail URL that creates a new E-mail message)
		if (nullptr == theURLCFString)
		{
			failedToSubmitReport = YES;
			failureMode = 1;
		}
		else
		{
			// The string is ASSUMED to be a mailto: URL that ends with a
			// partial “body=” specification.  Here, additional information
			// is generated and appended (in URL encoding format) to the
			// string, with the expectation that it will end up in the body.
			// Note that this extra information is NOT localized because I
			// want to be able to read it!
			CFMutableStringRef		modifiedURLCFString = CFStringCreateMutableCopy
															(kCFAllocatorDefault, CFStringGetLength(theURLCFString),
																theURLCFString);
			
			
			if (nullptr == modifiedURLCFString)
			{
				failedToSubmitReport = YES;
				failureMode = 2;
			}
			else
			{
				// write an initial header to the console that describes the user’s runtime environment
				// (again, this must be in URL-encoded format!)
			#define BUG_REPORT_INCLUDES_PREAMBLE		1
			#define BUG_REPORT_INCLUDES_OS_VERSION		1
			#define BUG_REPORT_INCLUDES_APP_VERSION		1
				
			#if BUG_REPORT_INCLUDES_PREAMBLE
				CFStringAppendCString
					(modifiedURLCFString,
						"%0D---%0D%0DThe%20following%20information%20will%20help%20the%20authors%20with%20debugging.%20%20"
						"Please%20keep%20it%20in%20this%20message.%0D", kCFStringEncodingUTF8);
			#endif
				
				// generate OS version information
				// (again, this must be in URL-encoded format!)
			#if BUG_REPORT_INCLUDES_OS_VERSION
				{
					NSCharacterSet*		allowedChars = [NSCharacterSet URLQueryAllowedCharacterSet];
					NSString*			versionString = [[[NSProcessInfo processInfo] operatingSystemVersionString]
															stringByAddingPercentEncodingWithAllowedCharacters:allowedChars];
					
					
					CFStringAppend(modifiedURLCFString, CFSTR("%20%20Mac%20OS%20X%20"));
					CFStringAppend(modifiedURLCFString, BRIDGE_CAST(versionString, CFStringRef));
					CFStringAppend(modifiedURLCFString, CFSTR("%0D"));
				}
			#endif
				
				// generate application version information
				// (again, this must be in URL-encoded format!)
			#if BUG_REPORT_INCLUDES_APP_VERSION
				// NOTE: again, as mentioned above, this will only be defined if the
				// BugReporter is physically located in the bundle's "Contents/Resources"
				if (nullptr != buggyAppBundle)
				{
					CFStringRef		appVersionCFString = CFUtilities_StringCast
															(CFBundleGetValueForInfoDictionaryKey
																(buggyAppBundle, CFSTR("CFBundleVersion")));
					
					
					if (nullptr == appVersionCFString)
					{
						CFStringAppend(modifiedURLCFString, CFSTR("%20%20Could%20not%20find%20application%20version!%0D"));
					}
					else
					{
						CFStringAppend(modifiedURLCFString, CFSTR("%20%20Application%20Version%20"));
						CFStringAppend(modifiedURLCFString, appVersionCFString);
						CFStringAppend(modifiedURLCFString, CFSTR("%0D"));
					}
				}
			#endif
				
				// finally, create a URL out of the modified URL string, and open it!
				{
					CFURLRef	bugReporterURL = CFURLCreateWithString(kCFAllocatorDefault, modifiedURLCFString, nullptr/* base */);
					
					
					if (nullptr == bugReporterURL)
					{
						failedToSubmitReport = YES;
						failureMode = 3;
					}
					else
					{
						// launch mail program with a new message pointed to the bug address
						// and containing all of the special information above (that is, if
						// the user’s mail program respects all mailto: URL specifications!)
						OSStatus	error = LSOpenCFURLRef(bugReporterURL, nullptr/* launched URL */);
						
						
						if (noErr != error)
						{
							failedToSubmitReport = YES;
							failureMode = 4;
						}
						CFRelease(bugReporterURL), bugReporterURL = nullptr;
					}
				}
				CFRelease(modifiedURLCFString), modifiedURLCFString = nullptr;
			}
			//CFRelease(theURLCFString), theURLCFString = nullptr; // not released because CFBundleGetValueForInfoDictionaryKey() does not make a copy
		}
		
		if (nullptr != buggyAppBundle)
		{
			CFRelease(buggyAppBundle), buggyAppBundle = nullptr;
		}
		
		// if even this fails, the user can’t report bugs with this program!
		if (failedToSubmitReport)
		{
			// display an error
			NSAlert*	errorBox = [[[NSAlert alloc] init] autorelease];
			
			
			button1 = NSLocalizedString(@"Quit Without Reporting", @"button label");
			messageText = NSLocalizedString
							(@"Unfortunately there has been a problem reporting this bug automatically.",
								@"submission failure message");
			helpText = NSLocalizedString
						(@"Please visit the main web site to report this issue, instead.",
							@"submission failure help message");
			NSLog(@"Failed to submit bug report in the normal fashion (internal failure mode: %d).", failureMode);
			
			errorBox.messageText = messageText;
			errorBox.informativeText = helpText;
			UNUSED_RETURN(NSButton*)[errorBox addButtonWithTitle:button1];
			
			UNUSED_RETURN(int)[errorBox runModal];
		}
	}
	
	// quit the bug reporter application
	[NSApp terminate:nil];
}// applicationDidFinishLaunching:


@end // BugReporterAppDelegate

// BELOW IS REQUIRED NEWLINE TO END FILE
