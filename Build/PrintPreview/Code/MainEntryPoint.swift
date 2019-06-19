/*!	\file MainEntryPoint.swift
	\brief Front end to the Print Preview application.
*/
/*###############################################################

	Print Preview
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
import Cocoa



// MARK: Types

/*!
The delegate for NSApplication.
*/
@NSApplicationMain
class PrintPreviewAppDelegate : NSObject, NSApplicationDelegate
{

@objc var parentRunningApp: NSRunningApplication?
var panelWC: MainEntryPoint_TerminalPrintDialogWC?
private var registeredObservers = [NSKeyValueObservation]()


// MARK: Initializers

/*!
Designated initializer.

(2019.06)
*/
override init()
{
	super.init()
	
	// register value-transformer classes used by bindings
	let valueTransformer = MainEntryPoint_FontSizeValueTransformer()
	ValueTransformer.setValueTransformer(valueTransformer, forName:NSValueTransformerName("MainEntryPoint_FontSizeValueTransformer")/* used in XIB */)
}// init()


// MARK: Actions

/*!
Rotates forward through windows; since this is an
internal application that creates the illusion of
a seamless interface with the parent, the “next
window” could actually be in a different application.

(2016.09)
*/
@IBAction func
orderFrontNextWindow(_ sender: AnyObject)
{
	if let definedParentApp = self.parentRunningApp
	{
		definedParentApp.activate(options:[])
	}
}// orderFrontNextWindow(sender:)


/*!
Rotates backward through windows; since this is an
internal application that creates the illusion of
a seamless interface with the parent, the “previous
window” could actually be in a different application.

(2016.09)
*/
@IBAction func
orderFrontPreviousWindow(_ sender: AnyObject)
{
	if let definedParentApp = self.parentRunningApp
	{
		definedParentApp.activate(options:[])
	}
}// orderFrontPreviousWindow(sender:)


// MARK: NSApplicationDelegate

/*!
Responds to application startup by keeping track of
the parent process, gathering configuration data from
the environment and then displaying a Print Preview
window with the appropriate terminal text (in the
specified font, etc.).

(2016.09)
*/
func
applicationDidFinishLaunching(_/* <- critical underscore; without it, no app launch (different method!) */ aNotification: Notification)
{
	NSLog("Launched.")
	
	// detect when parent process is terminated
	self.parentRunningApp = nil
	let selfFileSysRep = (Bundle.main.bundleURL.absoluteURL as NSURL).fileSystemRepresentation
	let pathSelf = CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, selfFileSysRep) as String
	
	NSLog("Helper application: \(pathSelf)") // debug
	for runningApp in NSRunningApplication.runningApplications(withBundleIdentifier:"net.macterm.MacTerm")
	{
		// when searching for the parent application, consider not only
		// the bundle ID but the location (in case the user has somehow
		// started more than one, use the one that physically contains
		// this internal application bundle)
		guard let bundleURL = runningApp.bundleURL else { continue } // skip non-bundle apps
		let appFileSysRep = (bundleURL.absoluteURL as NSURL).fileSystemRepresentation
		let pathApp = CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, appFileSysRep) as String
		
		
		//NSLog("Candidate app path: \(pathApp)") // debug
		if pathSelf.starts(with:pathApp)
		{
			// absolute root directories match; this is the right parent
			self.parentRunningApp = runningApp
			NSLog("Found parent application: \(pathApp)") // debug
			break
		}
	}
	
	guard let knownParentRunningApp = self.parentRunningApp
	else
	{
		NSLog("Terminating because parent application cannot be found.")
		NSApp.terminate(nil)
		return // make Swift compiler happy (should not be needed)
	}
	
	self.registeredObservers.append(knownParentRunningApp.observe(\.isTerminated,
	options:[.new]) { [unowned self] object, change in
		// it is not necessary to kill the print preview application
		// simply because the parent exited; in fact, it could be good
		// not to, as an abnormal exit indicates a crash (the user can
		// continue to print!); the exception is if the preview window
		// has already closed, in which case the process is useless
		if true == change.newValue
		{
			var shouldExit = true
			if let definedWC = self.panelWC,
				let definedWindow = definedWC.window
			{
				if definedWindow.isVisible
				{
					NSLog("Detected parent exit; ignoring because preview is still active.")
					shouldExit = false
				}
			}
			if (shouldExit)
			{
				// theoretically this case should not happen because the
				// application is configured to quit as soon as the last
				// window closes; still, being explicit doesn’t hurt
				NSLog("Detected parent exit and Print Preview window now closed; terminating.")
				NSApp.terminate(nil)
			}
		}
	})
	
	// key configuration data MUST be passed through the environment;
	// if any expected variable is not available, it is an error (the
	// main application code that spawns Print Preview must use these
	// same environment variable names)
	guard
	  let jobTitleString = ProcessInfo.processInfo.environment["MACTERM_PRINT_PREVIEW_JOB_TITLE"]
	, let pasteboardNameString = ProcessInfo.processInfo.environment["MACTERM_PRINT_PREVIEW_PASTEBOARD_NAME"]
	, let fontNameString = ProcessInfo.processInfo.environment["MACTERM_PRINT_PREVIEW_FONT_NAME"]
	, let fontSizeString = ProcessInfo.processInfo.environment["MACTERM_PRINT_PREVIEW_FONT_SIZE"]
	else
	{
		NSLog("Terminating because one or more required environment variables is not set.")
		NSApp.terminate(nil)
		return // make Swift compiler happy (should not be needed)
	}
	var isLandscape = false
	if let isLandscapeString = ProcessInfo.processInfo.environment["MACTERM_PRINT_PREVIEW_FONT_SIZE"]
	{
		if isLandscapeString == "1"
		{
			isLandscape = true
		}
	}
	var textToPrintPasteboard: NSPasteboard? = nil // see below
	var previewText = ""
	var floatFontSize: Float = 12.0 // see below
	
	
	// convert string value of font size environment variable into a number
	let fontSizeScanner = Scanner(string:fontSizeString)
	if false == fontSizeScanner.scanFloat(&floatFontSize)
	{
		NSLog("Warning, failed to parse “\(fontSizeString)” into a valid font size; ignoring.")
		floatFontSize = 12.0; // arbitrary work-around
	}
	
	// find the unique pasteboard that was given
	let dataClasses = [NSString.self]
	textToPrintPasteboard = NSPasteboard(name:NSPasteboard.Name(pasteboardNameString))
	if let definedPasteboard = textToPrintPasteboard
	{
		if false == definedPasteboard.canReadObject(forClasses:dataClasses, options:nil)
		{
			NSLog("Launch environment for print preview has not specified any preview text.")
		}
		else
		{
			let dataArray = definedPasteboard.readObjects(forClasses:dataClasses, options:nil)
			
			
			if let definedArray = dataArray
			{
				if (0 == definedArray.count)
				{
					NSLog("No data found for print preview.")
				}
				else
				{
					if let dataStr = definedArray[0] as? NSString
					{
						previewText = dataStr as String
					}
					else
					{
						NSLog("No suitable data found for print preview.")
					}
				}
			}
			else
			{
				NSLog("Failed to retrieve text for print preview.")
			}
		}
	}
	else
	{
		NSLog("No pasteboard named “\(pasteboardNameString)”!")
	}
	
	// enable the following if desired for debugging the input environment
	/*
	if (true)
	{
		NSLog("RECEIVED JOB TITLE: “\(jobTitleString)”") // debug
		NSLog("RECEIVED PASTEBOARD NAME: “\(pasteboardNameString)”") // debug
		if let definedPasteboard = textToPrintPasteboard
		{
			NSLog("RESOLVED TO PASTEBOARD: “\(definedPasteboard)”") // debug
		}
		else
		{
			NSLog("NO PASTEBOARD") // debug
		}
		NSLog("PASTEBOARD TEXT: “\(previewText)”") // debug
		NSLog("RECEIVED FONT: “\(fontNameString)”") // debug
		NSLog("RECEIVED FONT SIZE (converted): “\(floatFontSize)”") // debug
		NSLog("RECEIVED LANDSCAPE MODE: \(isLandscape)") // debug
	}
	*/
	
	// will no longer need this pasteboard anywhere
	if let definedPasteboard = textToPrintPasteboard
	{
		definedPasteboard.releaseGlobally()
	}
	
	// bring this process to the front
	NSApp.activate(ignoringOtherApps:true)
	
	// create a preview sheet; this is retained by the controller
	let previewFont = NSFont(name:fontNameString, size:CGFloat(floatFontSize))
	self.panelWC = MainEntryPoint_TerminalPrintDialogWC(string:previewText, font:previewFont,
														title:jobTitleString, landscape:isLandscape)
	if let definedWC = self.panelWC,
		let definedWindow = definedWC.window
	{
		definedWindow.orderFront(nil)
	}
	else
	{
		NSLog("Failed to load Print Preview window!")
	}
	
	// FIXME: also, can window rotation include the sub-application window, and can sub rotate to parent?
}// applicationDidFinishLaunching:


/*!
Returns true to cause this internal application to
quit as soon as the preview is dismissed for any
reason.

(2016.09)
*/
func
applicationShouldTerminateAfterLastWindowClosed(_ anApp: NSApplication) -> Bool
{
	return true
}// applicationShouldTerminateAfterLastWindowClosed()

}// PrintPreviewAppDelegate


// MARK: -

/*!
Implements a value transformer to clean up font size floating-point values.

This ensures that the default string value of a number, which may have
dozens of arbitrary after-decimal digits, only shows a few digits.
*/
class MainEntryPoint_FontSizeValueTransformer : ValueTransformer
{

override class func
allowsReverseTransformation() -> Bool
{
	return true
}

override class func
transformedValueClass() -> AnyClass
{
	return NSString.self
}

override func
reverseTransformedValue(_ value: Any?) -> Any?
{
	guard let valueString = value as? String else { return nil }
	return Float(valueString)
}

override func
transformedValue(_ value: Any?) -> Any?
{
	// by default a bound number could have a long floating-point
	// representation that is not attractive as a string; return
	// a formatted version that limits the number of decimal places
	guard let valueFloat = value as? Float else { return nil }
	return NSString(format:"%.2f", valueFloat) as String
}

}// MainEntryPoint_FontSizeValueTransformer


// MARK: -

/*!
Implements Print Preview.  See "TerminalPrintDialogCocoa.xib".
*/
class MainEntryPoint_TerminalPrintDialogWC : NSWindowController
{

// IMPORTANT: anything used as a XIB binding REQUIRES "@objc" to be KVO-compliant at runtime
@objc var fontSize: Float = 8.0
@objc var maximumSensibleFontSize = Float(48.0) // arbitrary
@objc var minimumSensibleFontSize = Float(8.0) // arbitrary
var previewFont: NSFont?
@IBOutlet var previewPane: NSTextView?
@objc var previewText: String?
@objc var previewTitle: String?
@objc var printInfo: NSPrintInfo?
@IBOutlet var printInfoOC: NSObjectController?
private var registeredObservers = [NSKeyValueObservation]()


// MARK: Initializers

/*!
Convenience initializer.

(2016.09)
*/
convenience init(string aString: String, font aFont: NSFont?, title aTitle: String, landscape landscapeMode: Bool)
{
	NSLog("Initializing in expected way.")
	
	// this NIB-based initializer is only automatically inherited
	// if the other required initializers are implemented (below)
	self.init(windowNibName:NSNib.Name("TerminalPrintDialogCocoa"))
	
	previewText = aString
	if let definedFont = aFont
	{
		previewFont = definedFont
		fontSize = Float(definedFont.pointSize)
	}
	else
	{
		let defaultSize: Float = 12.0
		fontSize = defaultSize
		previewFont = NSFont.systemFont(ofSize:CGFloat(defaultSize))
		NSLog("Warning, no font object specified; using default font/size instead")
	}
	previewTitle = aTitle
	
	// initialize the page setup to some values that are saner
	// for printing terminal text
	printInfo = NSPrintInfo.shared
	if let definedPrintInfo = printInfo
	{
		if landscapeMode
		{
			definedPrintInfo.orientation = NSPrintInfo.PaperOrientation.landscape
		}
		else
		{
			definedPrintInfo.orientation = NSPrintInfo.PaperOrientation.portrait
		}
		definedPrintInfo.horizontalPagination = NSPrintInfo.PaginationMode.fit
		definedPrintInfo.isHorizontallyCentered = false
		definedPrintInfo.isVerticallyCentered = false
	}
	
	shouldCascadeWindows = false
}// init(string:font:title:landscape:)


/*!
Initializer for using an existing window.

This is overridden only to ensure that Swift will
auto-inherit the useful with-NIB-name initializer.

(2019.06)
*/
override init(window: NSWindow?)
{
	// this appears to be called by the inherited "init(windowNibName:)" implementation
	super.init(window:window)
	//NSLog("\(#file):\(#line):\(#function): assertion failure: initialization via NSWindow is not supported.")
}


/*!
Initializer for unarchiving.

This is overridden only to ensure that Swift will
auto-inherit the useful with-NIB-name initializer.

(2019.06)
*/
required init?(coder: NSCoder)
{
	super.init(coder:coder)
	NSLog("\(#file):\(#line):\(#function): assertion failure: initialization via NSCoder is not supported.")
}


// MARK: Actions

/*!
Closes the sheet without doing anything.

(2016.09)
*/
@IBAction func
cancel(_ sender: AnyObject)		
{
	if let definedWindow = self.window
	{
		definedWindow.orderOut(sender)
	}
}// cancel(sender:)


/*!
Displays the Page Setup sheet for this print job.

(2016.09)
*/
@IBAction func
pageSetup(_ sender: AnyObject)
{
	if let definedWindow = self.window,
		let definedPrintInfo = self.printInfo
	{
		let layout = NSPageLayout()
		layout.beginSheet(with:definedPrintInfo, modalFor:definedWindow, delegate:self,
							didEnd:#selector(didEnd(pageLayout:returnCode:contextInfo:)),
							contextInfo:nil)
	}
	else
	{
		NSLog("Unable to open page setup UI; invalid page setup data structure and/or window!")
	}
}// pageSetup(sender:)


/*!
Asks the preview text view to print itself, and closes
the sheet.

(2016.09)
*/
@IBAction func
printView(_ sender: AnyObject)
{
	if let definedPreviewPane = self.previewPane,
		let definedPrintInfo = self.printInfo,
		let definedWindow = self.window
	{
		let op = NSPrintOperation(view:definedPreviewPane, printInfo:definedPrintInfo)
		
		
		op.runModal(for:definedWindow, delegate:self,
					didRun:#selector(didRun(printOperation:success:contextInfo:)),
					contextInfo:nil)
	}
	else
	{
		NSLog("Unable to print; invalid preview pane and/or page setup and/or window!")
	}
}// printView(sender:)


// MARK: NSWindowController

/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init...".)

(2016.09)
*/
override func
windowDidLoad()
{
	super.windowDidLoad()
	
	// check outlet: controller for PrintInfo
	if let _ = self.printInfoOC
	{
		// used only for assertion
	}
	else
	{
		NSLog("\(#file):\(#line):\(#function): assertion failure: PrintInfo object controller is not defined!")
	}
	
	// check outlet: preview pane
	if let definedPreviewPane = self.previewPane
	{
		if let definedWindow = self.window,
			let definedTitle = self.previewTitle
		{
			definedWindow.title = definedTitle
		}
		
		if let definedTextStorage = definedPreviewPane.textStorage,
			let definedPreviewText = self.previewText
		{
			definedTextStorage.mutableString.setString(definedPreviewText)
		}
		
		definedPreviewPane.font = self.previewFont
		
		// auto-update preview when font size changes (either by
		// slider or text field)
		self.registeredObservers.append(self.observe(\.fontSize,
		options:[.new]) { [unowned self] object, change in
			if let definedFontSize = change.newValue
			{
				if let definedFont = self.previewFont
				{
					self.previewFont = NSFont(name:definedFont.fontName, size:CGFloat(definedFontSize))
				}
				if let definedPreviewPane = self.previewPane
				{
					definedPreviewPane.font = self.previewFont
				}
			}
		})
	}
	else
	{
		NSLog("\(#file):\(#line):\(#function): assertion failure: preview pane is not defined!")
	}
}// windowDidLoad()


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(2016.09)
*/
func
windowFrameAutosaveName() -> String
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return "PrintPreview"
}// windowFrameAutosaveName()


// MARK: Internal Methods

/*!
Responds to the closing of a page layout configuration sheet.

(2016.09)
*/
@objc func // selector
didEnd(pageLayout: NSPageLayout, returnCode: Int, contextInfo: UnsafeRawPointer)
{
	if let definedPrintInfo = pageLayout.printInfo
	{
		if let definedPrintInfoOC = printInfoOC
		{
			DispatchQueue.main.async
			{
				// reset object so that bindings will auto-update
				definedPrintInfoOC.content = nil
				definedPrintInfoOC.content = definedPrintInfo
			}
		}
	}
}// didEnd(pageLayout:returnCode:contextInfo:)


/*!
Responds to the closing of a printing configuration sheet
(or other completion of printing) by finally closing the
preview dialog.

(2016.09)
*/
@objc func // selector
didRun(printOperation: NSPrintOperation, success: Bool, contextInfo: UnsafeRawPointer)
{
	if success
	{
		if let definedWindow = self.window
		{
			DispatchQueue.main.async { definedWindow.orderOut(nil) }
		}
	}
}// didRun(printOperation:success:contextInfo:)

}// MainEntryPoint_TerminalPrintDialogWC

// BELOW IS REQUIRED NEWLINE TO END FILE
