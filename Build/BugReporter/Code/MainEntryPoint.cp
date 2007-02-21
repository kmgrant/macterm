/*###############################################################

	MainEntryPoint.cp
	
	Bug Reporter
		© 2005-2006 by Kevin Grant.
	
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

// standard-C++ includes
#include <iostream>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFUtilities.h>
#include <Releases.h>
#include <UniversalDefines.h>



#pragma mark Constants

enum
{
	kASRequiredSuite = kCoreEventClass
};

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	AEAddressDesc			gSelfAddress;		//!< used to target send-to-self Apple Events
	ProcessSerialNumber		gSelfProcessID;		//!< used to target send-to-self Apple Events
}

#pragma mark Internal Method Prototypes

static OSStatus				addErrorToReply				(ConstStringPtr, OSStatus, AppleEventPtr);
static Boolean				installRequiredHandlers		();
static pascal OSErr			receiveApplicationOpen		(AppleEvent const*, AppleEvent*, SInt32);
static pascal OSErr			receiveApplicationPrefs		(AppleEvent const*, AppleEvent*, SInt32);
static pascal OSErr			receiveApplicationReopen	(AppleEvent const*, AppleEvent*, SInt32);
static pascal OSErr			receiveApplicationQuit		(AppleEvent const*, AppleEvent*, SInt32);
static pascal OSErr			receiveOpenDocuments		(AppleEvent const*, AppleEvent*, SInt32);
static pascal OSErr			receivePrintDocuments		(AppleEvent const*, AppleEvent*, SInt32);

#pragma mark Public Methods

int
main	(int	argc,
		 char*	argv[])
{
	int		result = 0;
	
	
	// install event handlers that can receive callbacks
	// from the OS within the (blocked) main event loop;
	// this is where all the “work” is done
	{
		Boolean		installedHandlersOK = false;
		
		
		installedHandlersOK = installRequiredHandlers();
		assert(installedHandlersOK);
	}
	
	// configure GUI
	{
		IBNibRef	interfaceBuilderObject = nullptr;
		OSStatus	error = noErr;
		
		
		result = 1; // assume failure unless found otherwise
		error = CreateNibReference(CFSTR("BugReporter"), &interfaceBuilderObject);
		if (error == noErr)
		{
			error = SetMenuBarFromNib(interfaceBuilderObject, CFSTR("MenuBar"));
			if (error == noErr)
			{
				result = 0; // success!
			}
			DisposeNibReference(interfaceBuilderObject), interfaceBuilderObject = nullptr;
		}
	}
	
	// display GUI and wait for events (including Quit!)
	if (0 == result)
	{
		RunApplicationEventLoop();
	}
	
	// quit, returning appropriate status to the shell
	return result;
}


#pragma mark Internal Methods

/*!
Adds an error to the reply record; at least the
error code is provided, and optionally you can
accompany it with an equivalent message.

You should use this to return error codes from
ALL Apple Events; if you do, return "noErr" from
the handler but potentially define "inError" as
a specific error code.

(3.1)
*/
static OSStatus
addErrorToReply		(ConstStringPtr		inErrorMessageOrNull,
					 OSStatus			inError,
					 AppleEventPtr		inoutReplyAppleEventPtr)
{
	OSStatus	result = noErr;
	
	
	// the reply must exist
	if ((inoutReplyAppleEventPtr->descriptorType != typeNull) ||
		(inoutReplyAppleEventPtr->dataHandle != nullptr))
	{
		// put error code
		if (inError != noErr)
		{
			result = AEPutParamPtr(inoutReplyAppleEventPtr, keyErrorNumber, typeSInt16,
									&inError, sizeof(inError));
		}
		
		// if provided, also put the error message
		if (inErrorMessageOrNull != nullptr)
		{
			result = AEPutParamPtr(inoutReplyAppleEventPtr, keyErrorString, typeChar,
									inErrorMessageOrNull + 1/* skip length byte */,
									STATIC_CAST(PLstrlen(inErrorMessageOrNull) * sizeof(UInt8), Size));
		}
	}
	else
	{
		// no place to put the data - do nothing
		result = errAEReplyNotValid;
	}
	
	return result;
}// addErrorToReply


/*!
This routine installs Apple Event handlers for
the Required Suite, including the six standard
Apple Events (open and print documents, open,
re-open and quit application, and display
preferences).

If ALL handlers install successfully, "true" is
returned; otherwise, "false" is returned.

(3.1)
*/
static Boolean
installRequiredHandlers ()
{
	Boolean			result = false;
	OSStatus		error = noErr;
	AEEventClass	eventClass = '----';
	
	
	eventClass = kASRequiredSuite;
	if (error == noErr) error = AEInstallEventHandler(eventClass, kAEOpenApplication,
														NewAEEventHandlerUPP(receiveApplicationOpen),
														0L, false);
	if (error == noErr) error = AEInstallEventHandler(eventClass, kAEReopenApplication,
														NewAEEventHandlerUPP(receiveApplicationReopen),
														0L, false);
	if (error == noErr) error = AEInstallEventHandler(eventClass, kAEOpenDocuments,
														NewAEEventHandlerUPP(receiveOpenDocuments),
														0L, false);
	if (error == noErr) error = AEInstallEventHandler(eventClass, kAEPrintDocuments,
														NewAEEventHandlerUPP(receivePrintDocuments),
														0L, false);
	if (error == noErr) error = AEInstallEventHandler(eventClass, kAEQuitApplication,
														NewAEEventHandlerUPP(receiveApplicationQuit),
														0L, false);
	if (error == noErr) error = AEInstallEventHandler(eventClass, kAEShowPreferences,
														NewAEEventHandlerUPP(receiveApplicationPrefs),
														0L, false);
	result = (error == noErr);
	
	return result;
}// installRequiredHandlers


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEOpenApplication".

(3.1)
*/
static pascal OSErr
receiveApplicationOpen	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	OSStatus		error = noErr;
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	Boolean			doContinue = true;
	Boolean			failedToSubmitReport = false;
	
	
	error = noErr;
	
	// set up self address for Apple Events
	gSelfProcessID.highLongOfPSN = 0;
	gSelfProcessID.lowLongOfPSN = kCurrentProcess; // don’t use GetCurrentProcess()!
	error = AECreateDesc(typeProcessSerialNumber, &gSelfProcessID, sizeof(gSelfProcessID), &gSelfAddress);
	
	// always make this the frontmost application when it runs
	SetFrontProcess(&gSelfProcessID);
	
	{
		// notify the user of the crash
		AlertStdCFStringAlertParamRec	params;
		CFStringRef						messageText = nullptr;
		CFStringRef						helpText = nullptr;
		DialogRef						dialog = nullptr;
		DialogItemIndex					itemHit = kAlertStdAlertCancelButton;
		
		
		error = GetStandardAlertDefaultParams(&params, kStdCFStringAlertVersionOne);
		params.defaultButton = kAlertStdAlertOKButton;
		params.cancelButton = kAlertStdAlertCancelButton;
		params.defaultText = CFCopyLocalizedStringFromTable(CFSTR("Compose E-Mail"), CFSTR("Alerts"), CFSTR("button label"));
		params.cancelText = CFCopyLocalizedStringFromTable(CFSTR("Quit Without Reporting"), CFSTR("Alerts"), CFSTR("button label"));
		messageText = CFCopyLocalizedStringFromTable
						(CFSTR("MacTelnet has quit because of a software defect.  Please notify the authors so this can be fixed."),
							CFSTR("Alerts"), CFSTR("main message"));
		helpText = CFCopyLocalizedStringFromTable
					(CFSTR("A starting point for your mail message will be automatically created for you."),
						CFSTR("Alerts"), CFSTR("main help text"));
		error = CreateStandardAlert(kAlertNoteAlert, messageText, helpText, &params, &dialog);
		error = RunStandardAlert(dialog, nullptr/* filter */, &itemHit);
		doContinue = (kAlertStdAlertOKButton == itemHit);
	}
	
	if (doContinue)
	{
		CFStringRef		theURLCFString = CFUtilities_StringCast
											(CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
												CFSTR("BugReporterMailURLToOpen")));
		CFBundleRef		buggyAppBundle = nullptr;
		
		
		// “load” the buggy application bundle, which assumes that this app
		// is physically located within the Resources folder of that bundle
		// (so in particular, THIS WILL NOT WORK IN TESTING unless you
		// PHYSICALLY MOVE the built executable into the Resources directory
		// of a built MacTelnet.app!!!)
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
		if (nullptr == theURLCFString) failedToSubmitReport = true;
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
			
			
			if (nullptr == modifiedURLCFString) failedToSubmitReport = true;
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
					char	s[255];
					long	gestaltResult = 0L;
					
					
					if (Gestalt(gestaltSystemVersion, &gestaltResult) != noErr)
					{
						CPP_STD::strcpy(s, "%20%20Could%20not%20find%20Mac%20OS%20version!%0D");
					}
					else
					{
						// NOTE: since std::snprintf() actually attributes special meaning to
						// the percent (%) symbol, each "%" destined for a URL-encoding "%"
						// must appear doubled in the format string; each "%" destined for
						// substitution by std::snprintf() must NOT be doubled!!!
						CPP_STD::snprintf(s, sizeof(s), "%%20%%20This%%20computer%%20is%%20running%%20Mac%%20OS%%20X%%20%d.%d.%d.%%0D",
											Releases_GetMajorRevisionForVersion(gestaltResult),
											Releases_GetMinorRevisionForVersion(gestaltResult),
											Releases_GetSuperminorRevisionForVersion(gestaltResult));
					}
					CFStringAppendCString(modifiedURLCFString, s, kCFStringEncodingUTF8);
				}
			#endif
				
				// generate application version information
				// (again, this must be in URL-encoded format!)
			#if BUG_REPORT_INCLUDES_APP_VERSION
				// NOTE: again, as mentioned above, this will only be defined if the
				// BugReporter is physically located in MacTelnet.app/Contents/Resources
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
						CFStringAppend(modifiedURLCFString, CFSTR("%20%20Application%20version:%20"));
						CFStringAppend(modifiedURLCFString, appVersionCFString);
						CFStringAppend(modifiedURLCFString, CFSTR("%0D"));
					}
				}
			#endif
				
				// finally, create a URL out of the modified URL string, and open it!
				{
					CFURLRef	bugReporterURL = CFURLCreateWithString(kCFAllocatorDefault, modifiedURLCFString, nullptr/* base */);
					
					
					if (nullptr == bugReporterURL) failedToSubmitReport = true;
					else
					{
						// launch mail program with a new message pointed to the bug address
						// and containing all of the special information above (that is, if
						// the user’s mail program respects all mailto: URL specifications!)
						error = LSOpenCFURLRef(bugReporterURL, nullptr/* launched URL */);
						if (noErr != error)
						{
							failedToSubmitReport = true;
						}
						CFRelease(bugReporterURL), bugReporterURL = nullptr;
					}
				}
				CFRelease(modifiedURLCFString), modifiedURLCFString = nullptr;
			}
			CFRelease(theURLCFString), theURLCFString = nullptr;
		}
		
		// if even this fails, the user can’t report bugs with this program!
		if (failedToSubmitReport)
		{
			
			AlertStdCFStringAlertParamRec	params;
			CFStringRef						messageText = nullptr;
			DialogRef						dialog = nullptr;
			DialogItemIndex					itemHit = kAlertStdAlertCancelButton;
			
			
			error = GetStandardAlertDefaultParams(&params, kStdCFStringAlertVersionOne);
			params.defaultButton = kAlertStdAlertOKButton;
			params.cancelButton = kAlertStdAlertCancelButton;
			params.defaultText = CFCopyLocalizedStringFromTable(CFSTR("Quit Without Reporting"), CFSTR("Alerts"), CFSTR("button label"));
			messageText = CFCopyLocalizedStringFromTable
							(CFSTR("Unfortunately there has been a problem reporting this bug automatically.  "
									"Please visit the main web site to report this issue, instead."),
								CFSTR("Alerts"), CFSTR("submission failure message"));
			error = CreateStandardAlert(kAlertNoteAlert, messageText, nullptr/* help text */, &params, &dialog);
			error = RunStandardAlert(dialog, nullptr/* filter */, &itemHit);
		}
	}
	
	// send a "quit" Apple Event to terminate the application
	// (and allow recording scripts to see that this happened)
	{
		AppleEvent		quitEvent;
		AppleEvent		reply;
		
		
		AEInitializeDesc(&quitEvent);
		AEInitializeDesc(&reply);
		error = AECreateAppleEvent(kASRequiredSuite, kAEQuitApplication, &gSelfAddress,
									kAutoGenerateReturnID, kAnyTransactionID, &quitEvent);
		if (error == noErr)
		{
			Boolean		confirm = false;
			
			
			if (confirm)
			{
				AEDesc		saveOptionsDesc;
				DescType	saveOptions = kAEYes;
				
				
				// add confirmation message parameter based on whether any connections are open
				AEInitializeDesc(&saveOptionsDesc);
				error = AECreateDesc(typeEnumerated, &saveOptions, sizeof(saveOptions), &saveOptionsDesc);
				error = AEPutParamDesc(&quitEvent, keyAESaveOptions, &saveOptionsDesc);
			}
			
			// send the message to quit
			error = AESend(&quitEvent, &reply, kAENoReply, kAENormalPriority, kAEDefaultTimeout,
							nullptr/* idle routine */, nullptr/* filter routine */);
		}
		AEDisposeDesc(&quitEvent);
	}
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationOpen


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEReopenApplication".

(3.1)
*/
static pascal OSErr
receiveApplicationReopen	(AppleEvent const*	inAppleEventPtr,
							 AppleEvent*		outReplyAppleEventPtr,
							 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationReopen


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEShowPreferences".

(3.1)
*/
static pascal OSErr
receiveApplicationPrefs		(AppleEvent const*	inAppleEventPtr,
							 AppleEvent*		outReplyAppleEventPtr,
							 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationPrefs


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEQuitApplication".

(3.1)
*/
static pascal OSErr
receiveApplicationQuit	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	Boolean			doQuit = true;
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	if (doQuit)
	{
		// terminate loop, which will return control to main()
		QuitApplicationEventLoop();
	}
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receiveApplicationQuit


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEOpenDocuments".

(3.1)
*/
static pascal OSErr
receiveOpenDocuments	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receiveOpenDocuments


/*!
Handles the "kASRequiredSuite" Apple Event
of type "kAEPrintDocuments".

(3.1)
*/
static pascal OSErr
receivePrintDocuments	(AppleEvent const*	inAppleEventPtr,
						 AppleEvent*		outReplyAppleEventPtr,
						 SInt32				inData)
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	error = noErr;
	
	(OSStatus)addErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// receivePrintDocuments

// BELOW IS REQUIRED NEWLINE TO END FILE
