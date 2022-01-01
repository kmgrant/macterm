/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

import SwiftUI

//
// IMPORTANT: Many "public" entities below are required
// in order to interact with Swift playgrounds.
//

@objc public protocol UIPrefsSessionDataFlow_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func resetToDefaultGetLocalEchoEnabled() -> Bool
	func resetToDefaultGetLineInsertionDelay() -> Int
	func resetToDefaultGetScrollingDelay() -> Int
	func resetToDefaultGetAutoCaptureToDirectoryEnabled() -> Bool
	func resetToDefaultGetCaptureFileDirectory() -> URL
	func resetToDefaultGetCaptureFileName() -> String
	func resetToDefaultGetCaptureFileNameIsGenerated() -> Bool
}

class UIPrefsSessionDataFlow_RunnerDummy : NSObject, UIPrefsSessionDataFlow_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func resetToDefaultGetLocalEchoEnabled() -> Bool { print(#function); return false }
	func resetToDefaultGetLineInsertionDelay() -> Int { print(#function); return 10 }
	func resetToDefaultGetScrollingDelay() -> Int { print(#function); return 0 }
	func resetToDefaultGetAutoCaptureToDirectoryEnabled() -> Bool { print(#function); return false }
	func resetToDefaultGetCaptureFileDirectory() -> URL { print(#function); return URL(fileURLWithPath: "/var/tmp") }
	func resetToDefaultGetCaptureFileName() -> String { print(#function); return "tmp.txt" }
	func resetToDefaultGetCaptureFileNameIsGenerated() -> Bool { print(#function); return false }
}

public class UIPrefsSessionDataFlow_Model : UICommon_DefaultingModel, ObservableObject {

	@Published @objc public var isDefaultLocalEchoEnabled = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { isLocalEchoEnabled = runner.resetToDefaultGetLocalEchoEnabled() } }
		}
	}
	@Published @objc public var isDefaultLineInsertionDelay = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { lineInsertionDelayValue = Double(runner.resetToDefaultGetLineInsertionDelay()) } }
		}
	}
	@Published @objc public var isDefaultScrollingDelay = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { scrollingDelayValue = Double(runner.resetToDefaultGetScrollingDelay()) } }
		}
	}
	@Published @objc public var isDefaultAutoCaptureToDirectoryEnabled = true {
		willSet(isOn) {
			if isOn {
				ifUserRequestedDefault {
					captureFileDirectory = runner.resetToDefaultGetCaptureFileDirectory()
					captureFileName = runner.resetToDefaultGetCaptureFileName()
					isCaptureFileNameGenerated = runner.resetToDefaultGetCaptureFileNameIsGenerated()
					isAutoCaptureToDirectoryEnabled = runner.resetToDefaultGetAutoCaptureToDirectoryEnabled()
				}
			}
		}
	}
	@Published @objc public var isLocalEchoEnabled = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultLocalEchoEnabled = false }
				runner.dataUpdated()
			}
		}
	}
	// NOTE: SwiftUI currently does not handle unexpected numerical conversions
	// (e.g. if user enters a floating-point value but binding is just an Int,
	// this is an untrappable runtime error!); for user-friendliness, the binding
	// is using a more-flexible Double type, despite expecting Int, so that it is
	// not possible for weird inputs to crash the app!!!
	@Published @objc public var lineInsertionDelayValue = 10.0 {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultLineInsertionDelay = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var scrollingDelayValue = 0.0 {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultScrollingDelay = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var isAutoCaptureToDirectoryEnabled = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultAutoCaptureToDirectoryEnabled = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var captureFileDirectory = URL(fileURLWithPath: "/var/tmp") {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultAutoCaptureToDirectoryEnabled = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var captureFileName = "tmp.txt" {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultAutoCaptureToDirectoryEnabled = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var isCaptureFileNameGenerated = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultAutoCaptureToDirectoryEnabled = false }
				runner.dataUpdated()
			}
		}
	}
	public var runner: UIPrefsSessionDataFlow_ActionHandling

	@objc public init(runner: UIPrefsSessionDataFlow_ActionHandling) {
		self.runner = runner
	}

	// MARK: UICommon_DefaultingModel

	override func setDefaultFlagsToTrue() {
		// unconditional; used by base when swapping to "isEditingDefaultContext"
		isDefaultLocalEchoEnabled = true
		isDefaultLineInsertionDelay = true
		isDefaultScrollingDelay = true
		isDefaultAutoCaptureToDirectoryEnabled = true
	}

}

public struct UIPrefsSessionDataFlow_View : View {

	@EnvironmentObject private var viewModel: UIPrefsSessionDataFlow_Model
	private var lineInsertionDelayFormatter = NumberFormatter()
	private var scrollingDelayFormatter = NumberFormatter()

	func isAutoCaptureDisabled() -> Bool {
		return (false == viewModel.isAutoCaptureToDirectoryEnabled)
	}

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			UICommon_DefaultOptionHeaderView()
			UICommon_Default1OptionLineView("Duplication",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Local Echo",
																							bindIsDefaultTo: $viewModel.isDefaultLocalEchoEnabled),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Toggle("Locally echo keystrokes", isOn: $viewModel.isLocalEchoEnabled)
					.macTermToolTipText("Set if MacTerm displays which keys have been pressed, as you type.  Useful if the application running in the terminal does not echo keystrokes or it responds more slowly than you type.")
			}
			Spacer().asMacTermMinorSectionSpacingV()
			Group {
				UICommon_Default1OptionLineView("Line Insertion Delay",
												toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Line Insertion Delay",
																								bindIsDefaultTo: $viewModel.isDefaultLineInsertionDelay),
												isEditingDefault: viewModel.isEditingDefaultContext) {
					TextField("", value: $viewModel.lineInsertionDelayValue, formatter: lineInsertionDelayFormatter)
						.frame(minWidth: 70, maxWidth: 70)
						.macTermToolTipText("Delay between inserted lines; for example, when pasting or dragging-and-dropping several lines at once.  Some terminal applications do not work correctly if text is sent too quickly.")
					Text("milliseconds")
				}
				Spacer().asMacTermMinorSectionSpacingV()
			}
			Group {
				UICommon_Default1OptionLineView("Scrolling Delay",
												toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Scrolling Delay",
																								bindIsDefaultTo: $viewModel.isDefaultScrollingDelay),
												isEditingDefault: viewModel.isEditingDefaultContext) {
					TextField("", value: $viewModel.scrollingDelayValue, formatter: scrollingDelayFormatter)
						.frame(minWidth: 70, maxWidth: 70)
						.macTermToolTipText("Delay as lines are scrolled off the screen.  This can make it easier to keep track of rapidly-scrolling command output but setting this to 0 is strongly recommended.")
					Text("milliseconds")
				}
				Spacer().asMacTermMinorSectionSpacingV()
			}
			Group {
				UICommon_Default1OptionLineView("Capture to File",
												toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Capture to File",
																								bindIsDefaultTo: $viewModel.isDefaultAutoCaptureToDirectoryEnabled),
												isEditingDefault: viewModel.isEditingDefaultContext) {
					Toggle("Automatically write to directory:", isOn: $viewModel.isAutoCaptureToDirectoryEnabled)
						.macTermToolTipText("Set if all text displayed in a terminal window should be saved to a file (as configured below).  At any time, you can use the “End Capture to File” command in the File menu.")
				}
				UICommon_OptionLineView("") {
					UICommon_FileSystemPathView($viewModel.captureFileDirectory)
						.disabled(isAutoCaptureDisabled())
						.padding([.leading], 16)
						.macTermToolTipText("The folder in which to save capture files.")
				}
				Spacer().asMacTermMinorSectionSpacingV()
				UICommon_OptionLineView("") {
					Text("Log file name:")
						.disabled(isAutoCaptureDisabled())
						.foregroundColor(isAutoCaptureDisabled() ? .gray : .primary) // default behavior of disabled() does not dim the text so do that manually
						.padding([.leading], 16)
					TextField("", text: $viewModel.captureFileName)
						.disabled(isAutoCaptureDisabled())
						.foregroundColor(isAutoCaptureDisabled() ? .gray : .primary) // default behavior of disabled() does not dim the text so do that manually
						.frame(minWidth: 200, maxWidth: 400)
						.macTermToolTipText("The name of the file to capture terminal text into, or (if “Generated name” is selected below) a template name that contains backslash substitution sequences.")
				}
				UICommon_OptionLineView("") {
					Toggle("Generated name (allow substitutions)", isOn: $viewModel.isCaptureFileNameGenerated)
						.disabled(isAutoCaptureDisabled())
						.padding([.leading], 16)
						.macTermToolTipText("Set if this name is a template that contains backslash substitution sequences (see below).")
				}
				UICommon_OptionLineView("") {
					Text("""
Recognized sequences:
  \\D — date, as YYYY-MM-DD
  \\T — 24-hour time, as HHMMSS
  \\\\ — \\

Files with generated names can coexist with previous logs in the same directory, whereas files with the same name will always be overwritten.
"""
					).controlSize(.small)
						.fixedSize(horizontal: false, vertical: true)
						.lineLimit(8)
						.multilineTextAlignment(.leading)
						.disabled(isAutoCaptureDisabled())
						.foregroundColor(isAutoCaptureDisabled() ? .gray : .primary) // default behavior of disabled() does not dim the text so do that manually
						.frame(maxWidth: 400, alignment: .leading)
						.padding([.leading], 16)
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsSessionDataFlow_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsSessionDataFlow_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsSessionDataFlow_View().environmentObject(data)))
	}

}

public struct UIPrefsSessionDataFlow_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsSessionDataFlow_Model(runner: UIPrefsSessionDataFlow_RunnerDummy())
		return VStack {
			UIPrefsSessionDataFlow_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsSessionDataFlow_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
