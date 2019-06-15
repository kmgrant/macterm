/*!	\file MainEntryPoint.swift
	\brief Front end to the Bug Reporter application.
*/
/*###############################################################

	Bug Reporter
		© 2005-2019 by Kevin Grant.
	
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

// Mac includes
import AppKit
import Foundation



// MARK: Types

/*!
The delegate for NSApplication.
*/
@NSApplicationMain
class BugReporterAppDelegate : NSObject, NSApplicationDelegate
{

// MARK: NSApplicationDelegate

/*!
Effectively the main entry point; called by the OS when
the launch is complete.

(4.0)
*/
func
applicationDidFinishLaunching(_/* <- critical underscore; without it, no app launch (different method!) */ aNotification: Notification)
{
	var appVersionString: String? = nil
	var isCrash = false
	var showBugAlert = false
	var sendMail = false
	
	
	print("Launched.")
	
	// check environment variables for special action requests
	// and information (otherwise take no action); some actions
	// below may be able to adapt when information is missing
	if let varValue = ProcessInfo.processInfo.environment["BUG_REPORTER_MACTERM_COMMENT_EMAIL_ONLY"]
	{
		if varValue == "1"
		{
			// launching process wants to proceed directly to E-mail
			// without displaying any local user interface
			print("Received request to send E-mail.")
			showBugAlert = false
			sendMail = true
			isCrash = false
		}
	}
	if let varValue = ProcessInfo.processInfo.environment["BUG_REPORTER_MACTERM_BUG"]
	{
		if varValue == "1"
		{
			// launching process wants to report some kind of problem
			print("Received defect notification from parent.")
			showBugAlert = true
			if let varValue = ProcessInfo.processInfo.environment["BUG_REPORTER_MACTERM_CRASH"]
			{
				if varValue == "1"
				{
					// launching process has definitely detected a crash
					print("Received crash notification from parent.")
					isCrash = true
				}
			}
		}
	}
	if let varValue = ProcessInfo.processInfo.environment["BUG_REPORTER_MACTERM_VERSION"]
	{
		print("Received version information.")
		appVersionString = varValue
	}
	
	if (!showBugAlert && !sendMail)
	{
		print("Nothing to do; set environment variables to provide instructions.")
	}
	
	if showBugAlert
	{
		let alertBox: NSAlert = NSAlert()
		let button1: String = NSLocalizedString("Compose E-Mail", comment:"button label")
		let button2: String = NSLocalizedString("Quit Without Reporting", comment:"button label")
		let helpText: String = NSLocalizedString("A starting point for your mail message will be automatically created for you.",
													comment:"main help text")
		var messageText: String? = nil
		var clickedButton: NSApplication.ModalResponse = .alertFirstButtonReturn
		
		
		if isCrash
		{
			messageText = NSLocalizedString("MacTerm has quit because of a software defect.  Please report this so the problem can be fixed.",
											comment:"crash message")
		}
		else
		{
			messageText = NSLocalizedString("MacTerm has detected a software defect.  Please report this so the problem can be fixed.",
											comment:"regular bug message")
		}
		
		// bring this process to the front
		NSApp.activate(ignoringOtherApps:true)
		
		// display an error to the user, with options
		alertBox.alertStyle = NSAlert.Style.critical
		alertBox.messageText = messageText!
		alertBox.informativeText = helpText
		/*var unused: NSButton =*/alertBox.addButton(withTitle:button1)
		/*var unused: NSButton =*/alertBox.addButton(withTitle:button2)
		clickedButton = alertBox.runModal()
		switch (clickedButton)
		{
		case .alertSecondButtonReturn:
			// quit without doing anything
			break
		case .alertFirstButtonReturn:
			// compose an E-mail with details on the error
			sendMail = true
		default:
			// compose an E-mail with details on the error
			sendMail = true
		}
	}
	
	// if appropriate, ask a mail program to compose a new E-mail;
	// note that this can be a direct result of multiple scenarios
	// above so it should be handled carefully (as the results can
	// vary slightly, e.g. showing different messages)
	if sendMail
	{
		var mailKey: String? = nil
		var dictValueMailURLString: Any? = nil
		var failureMode = 0
		var failedToSubmitReport = false
		
		
		if isCrash
		{
			mailKey = "BugReporterMailURLForCrash"
		}
		else if showBugAlert
		{
			mailKey = "BugReporterMailURLForDefect"
		}
		else
		{
			mailKey = "BugReporterMailURLForComment"
		}
		
		// launch the bug report URL given in this application’s Info.plist
		// (most likely a mail URL that creates a new E-mail message)
		dictValueMailURLString = Bundle.main.object(forInfoDictionaryKey:mailKey!)
		if let mailURLString = dictValueMailURLString as? String
		{
			// The string is ASSUMED to be a mailto: URL that ends with a
			// partial “body=” specification.  Here, additional information
			// is generated and appended (in URL encoding format) to the
			// string, with the expectation that it will end up in the body.
			// Note that this extra information is NOT localized because I
			// want to be able to read it!
			if var modifiedURLString = mailURLString.mutableCopy() as? String
			{
				// write an initial header to the console that describes the user’s runtime environment
				// (again, this must be in URL-encoded format!)
				let bugReportIncludesPreamble = true
				let bugReportIncludesOSVersion = true
				let bugReportIncludesAppVersion = true
				
				if bugReportIncludesPreamble
				{
					modifiedURLString.append("%0D---%0D%0DThe%20following%20information%20helps%20to%20solve%20problems%20and%20answer%20questions.%20%20Please%20keep%20it%20in%20this%20message.%0D")
				}
				
				// generate OS version information
				// (again, this must be in URL-encoded format!)
				if bugReportIncludesOSVersion
				{
					let allowedChars = CharacterSet.urlQueryAllowed
					if let versionString = ProcessInfo.processInfo.operatingSystemVersionString.addingPercentEncoding(withAllowedCharacters:allowedChars)
					{
						modifiedURLString.append("%20%20OS%20")
						modifiedURLString.append(versionString)
						modifiedURLString.append("%0D")
					}
				}
				
				// generate application version information
				// (again, this must be in URL-encoded format!)
				if bugReportIncludesAppVersion
				{
					// depends on environment given above
					if (nil != appVersionString)
					{
						modifiedURLString.append("%20%20Application%20Version%20")
						modifiedURLString.append(appVersionString!)
						modifiedURLString.append("%0D")
					}
					else
					{
						modifiedURLString.append("%20%20Could%20not%20find%20application%20version!%0D")
					}
				}
				modifiedURLString.append("%0D---%0D%0D")
				
				// finally, create a URL out of the modified URL string, and open it!
				if let mailURL = URL(string:modifiedURLString)
				{
					// launch mail program with a new message pointed to the bug address
					// and containing all of the special information above (that is, if
					// the user’s mail program respects all mailto: URL specifications!)
					let launchConfigDict = Dictionary< NSWorkspace.LaunchConfigurationKey, Any >()
					let workspace = NSWorkspace.shared
					let options: NSWorkspace.LaunchOptions = [.withErrorPresentation, .inhibitingBackgroundOnly, .async]
					
					do
					{
						try /*var unused: NSRunningApplication? = */workspace.open(mailURL, options:options, configuration:launchConfigDict)
					}
					catch
					{
						failedToSubmitReport = true
						failureMode = 4
						
						// NOTE: since "NSWorkspaceLaunchWithErrorPresentation" is
						// also given in the options above, it is possible for both
						// the basic error message and a system-provided one to
						// appear; as a contrived example, if the URL type were
						// something bogus and unrecognized instead of "mailto:",
						// the error presented below would be something like “unable
						// to open file” but the system would pop up an elaborate
						// alert allowing the user to search the App Store instead
						NSApp.presentError(error)
					}
				}
				else
				{
					failedToSubmitReport = true
					failureMode = 3
				}
			}
			else
			{
				failedToSubmitReport = true
				failureMode = 2
			}
		}
		else
		{
			failedToSubmitReport = true
			failureMode = 1
		}
		
		// there is a problem with the report and/or E-mail steps;
		// depending on the issue, a more descriptive error alert
		// may also have been presented above (or more logging)
		if failedToSubmitReport
		{
			print("Failed to submit bug report in the normal fashion; internal failure mode = \(failureMode)")
		}
	}
	
	// quit the bug reporter application
	NSApp.terminate(nil)
}// applicationDidFinishLaunching

}// BugReporterAppDelegate

// BELOW IS REQUIRED NEWLINE TO END FILE
