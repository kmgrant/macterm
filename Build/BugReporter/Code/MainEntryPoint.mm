/*!	\file MainEntryPoint.mm
	\brief Front end to the Bug Reporter application.
*/
/*###############################################################

	Bug Reporter
		© 2005-2018 by Kevin Grant.
	
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
#import <AppKit/AppKit.h>

// library includes
#import <Console.h>



#pragma mark Types

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
		 char const*	argv[])
{
	return NSApplicationMain(argc, argv);
}


#pragma mark -
@implementation BugReporterAppDelegate //{


#pragma mark NSApplicationDelegate


/*!
Effectively the main entry point; called by the OS when
the launch is complete.

(4.0)
*/
- (void)
applicationDidFinishLaunching:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	NSString*	appVersionString = nil;
	BOOL		isCrash = NO;
	BOOL		showBugAlert = NO;
	BOOL		sendMail = NO;
	
	
	// check environment variables for special action requests
	// and information (otherwise take no action); some actions
	// below may be able to adapt when information is missing
	{
		char const*		varValue; // set and reused below
		
		
		varValue = getenv("BUG_REPORTER_MACTERM_COMMENT_EMAIL_ONLY");
		if ((nullptr != varValue) && (0 == strcmp(varValue, "1")))
		{
			// launching process wants to proceed directly to E-mail
			// without displaying any local user interface
			Console_WriteLine("Received request to send E-mail.");
			showBugAlert = NO;
			sendMail = YES;
			isCrash = NO;
		}
		
		varValue = getenv("BUG_REPORTER_MACTERM_BUG");
		if ((nullptr != varValue) && (0 == strcmp(varValue, "1")))
		{
			// launching process wants to report some kind of problem
			Console_WriteLine("Received defect notification from parent.");
			showBugAlert = YES;
			varValue = getenv("BUG_REPORTER_MACTERM_CRASH");
			if ((nullptr != varValue) && (0 == strcmp(varValue, "1")))
			{
				// launching process has definitely detected a crash
				Console_WriteLine("Received crash notification from parent.");
				isCrash = YES;
			}
		}
		
		varValue = getenv("BUG_REPORTER_MACTERM_VERSION");
		if (nullptr != varValue)
		{
			Console_WriteLine("Received version information.");
			appVersionString = [NSString stringWithUTF8String:varValue];
		}
	}
	
	if (showBugAlert)
	{
		NSAlert*			alertBox = [[[NSAlert alloc] init] autorelease];
		NSString*			button1 = NSLocalizedString(@"Compose E-Mail", @"button label");
		NSString*			button2 = NSLocalizedString(@"Quit Without Reporting", @"button label");
		NSString*			messageText = ((isCrash)
											? NSLocalizedString
												(@"MacTerm has quit because of a software defect.  Please report this so the problem can be fixed.",
													@"crash message")
											: NSLocalizedString
												(@"MacTerm has detected a software defect.  Please report this so the problem can be fixed.",
													@"regular bug message"));
		NSString*			helpText = NSLocalizedString
										(@"A starting point for your mail message will be automatically created for you.",
											@"main help text");
		NSModalResponse		clickedButton = NSAlertFirstButtonReturn;
		
		
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
	}
	
	// if appropriate, ask a mail program to compose a new E-mail;
	// note that this can be a direct result of multiple scenarios
	// above so it should be handled carefully (as the results can
	// vary slightly, e.g. showing different messages)
	if (sendMail)
	{
		NSString*		mailKey = ((isCrash)
									? @"BugReporterMailURLForCrash"
									: ((showBugAlert)
										? @"BugReporterMailURLForDefect"
										: @"BugReporterMailURLForComment"));
		id				dictValueMailURLString = [[NSBundle mainBundle] objectForInfoDictionaryKey:mailKey];
		NSString*		mailURLString = nil; // set below
		unsigned int	failureMode = 0;
		BOOL			failedToSubmitReport = NO;
		
		
		assert([dictValueMailURLString isKindOfClass:NSString.class]);
		mailURLString = STATIC_CAST(dictValueMailURLString, NSString*);
		
		// launch the bug report URL given in this application’s Info.plist
		// (most likely a mail URL that creates a new E-mail message)
		if (nil == mailURLString)
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
			NSMutableString*	modifiedURLString = [mailURLString mutableCopy];
			
			
			if (nil == modifiedURLString)
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
				[modifiedURLString appendString:@"%0D---%0D%0DThe%20following%20information%20helps%20to%20solve%20problems%20and%20answer%20questions.%20%20"
													"Please%20keep%20it%20in%20this%20message.%0D"];
			#endif
				
				// generate OS version information
				// (again, this must be in URL-encoded format!)
			#if BUG_REPORT_INCLUDES_OS_VERSION
				{
					NSCharacterSet*		allowedChars = [NSCharacterSet URLQueryAllowedCharacterSet];
					NSString*			versionString = [[[NSProcessInfo processInfo] operatingSystemVersionString]
															stringByAddingPercentEncodingWithAllowedCharacters:allowedChars];
					
					
					[modifiedURLString appendString:@"%20%20OS%20"];
					[modifiedURLString appendString:versionString];
					[modifiedURLString appendString:@"%0D"];
				}
			#endif
				
				// generate application version information
				// (again, this must be in URL-encoded format!)
			#if BUG_REPORT_INCLUDES_APP_VERSION
				// depends on environment given above
				if (nil == appVersionString)
				{
					[modifiedURLString appendString:@"%20%20Could%20not%20find%20application%20version!%0D"];
				}
				else
				{
					[modifiedURLString appendString:@"%20%20Application%20Version%20"];
					[modifiedURLString appendString:appVersionString];
					[modifiedURLString appendString:@"%0D"];
				}
			#endif
				[modifiedURLString appendString:@"%0D---%0D%0D"];
				
				// finally, create a URL out of the modified URL string, and open it!
				{
					NSURL*	mailURL = [NSURL URLWithString:modifiedURLString];
					
					
					if (nil == mailURL)
					{
						failedToSubmitReport = YES;
						failureMode = 3;
					}
					else
					{
						// launch mail program with a new message pointed to the bug address
						// and containing all of the special information above (that is, if
						// the user’s mail program respects all mailto: URL specifications!)
						NSRunningApplication*			launchedApp = nil;
						NSDictionary< NSString*, id >*	launchConfigDict = @{};
						NSError*						error = nil;
						
						
						launchedApp = [[NSWorkspace sharedWorkspace]
										openURL:mailURL
												options:(NSWorkspaceLaunchWithErrorPresentation |
															NSWorkspaceLaunchInhibitingBackgroundOnly |
															NSWorkspaceLaunchAsync)
												configuration:launchConfigDict
												error:&error];
						if ((nil == launchedApp) || (nil != error))
						{
							failedToSubmitReport = YES;
							failureMode = 4;
							
							if (nil != error)
							{
								// NOTE: since "NSWorkspaceLaunchWithErrorPresentation" is
								// also given in the options above, it is possible for both
								// the basic error message and a system-provided one to
								// appear; as a contrived example, if the URL type were
								// something bogus and unrecognized instead of "mailto:",
								// the error presented below would be something like “unable
								// to open file” but the system would pop up an elaborate
								// alert allowing the user to search the App Store instead
								[NSApp presentError:error];
							}
						}
					}
				}
				
				[modifiedURLString release]; modifiedURLString = nil;
			}
		}
		
		// there is a problem with the report and/or E-mail steps;
		// depending on the issue, a more descriptive error alert
		// may also have been presented above (or more logging)
		if (failedToSubmitReport)
		{
			Console_Warning(Console_WriteValue, "Failed to submit bug report in the normal fashion; internal failure mode", failureMode);
		}
	}
	
	// quit the bug reporter application
	[NSApp terminate:nil];
}// applicationDidFinishLaunching:


@end //} BugReporterAppDelegate

// BELOW IS REQUIRED NEWLINE TO END FILE
