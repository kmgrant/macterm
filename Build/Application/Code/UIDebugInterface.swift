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

import SwiftUI

//
// IMPORTANT: Many "public" entities below are required
// in order to interact with Swift playgrounds.
//

@objc public protocol UIDebugInterface_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dumpStateOfActiveTerminal()
	func launchNewCallPythonClient()
	func showTestTerminalToolbar()
	func updateSettingCache()
}

class UIDebugInterface_RunnerDummy : NSObject, UIDebugInterface_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dumpStateOfActiveTerminal() { print(#function) }
	func launchNewCallPythonClient() { print(#function) }
	func showTestTerminalToolbar() { print(#function) }
	func updateSettingCache() { print(#function) }
}

public class UIDebugInterface_Model : NSObject, ObservableObject {

	@Published @objc public var logTerminalState = false {
		willSet(isOn) {
			if isOn {
				print("started logging of terminal state (except echo)")
			} else {
				print("no logging of terminal state (except echo)")
			}
		}
		didSet {
			runner.updateSettingCache()
		}
	}
	@Published @objc public var logTerminalEchoState = false {
		willSet(isOn) {
			if isOn {
				print("started logging of terminal state (echo)")
			} else {
				print("no logging of terminal state (echo)")
			}
		}
		didSet {
			runner.updateSettingCache()
		}
	}
	@Published @objc public var logTerminalInputCharacters = false {
		willSet(isOn) {
			if isOn {
				print("started logging of terminal input characters")
			} else {
				print("no logging of terminal input characters")
			}
		}
		didSet {
			runner.updateSettingCache()
		}
	}
	@Published @objc public var logPseudoTerminalDeviceSettings = false {
		willSet(isOn) {
			if isOn {
				print("started logging of pseudo-terminal device configurations")
			} else {
				print("no logging of pseudo-terminal device configurations")
			}
		}
		didSet {
			runner.updateSettingCache()
		}
	}
	@Published @objc public var logSixelGraphicsDecoderErrors = false {
		willSet(isOn) {
			if isOn {
				print("started logging of SIXEL decoder errors")
			} else {
				print("no logging of SIXEL decoder errors")
			}
		}
		didSet {
			runner.updateSettingCache()
		}
	}
	@Published @objc public var logSixelGraphicsDecoderState = false {
		willSet(isOn) {
			if isOn {
				print("started logging of SIXEL decoder state")
			} else {
				print("no logging of SIXEL decoder state")
			}
		}
		didSet {
			runner.updateSettingCache()
		}
	}
	@Published @objc public var logSixelGraphicsSummary = false {
		willSet(isOn) {
			if isOn {
				print("started logging of SIXEL graphics summary")
			} else {
				print("no logging of SIXEL graphics summary")
			}
		}
		didSet {
			runner.updateSettingCache()
		}
	}
	public var runner: UIDebugInterface_ActionHandling

	@objc public init(runner: UIDebugInterface_ActionHandling) {
		self.runner = runner
	}

}

public struct UIDebugInterface_View : View {

	@EnvironmentObject private var viewModel: UIDebugInterface_Model

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			// maximum 10 items per stack; use Group to build more
			Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_OptionLineView("Global", noDefaultSpacing: true) {
					Toggle("Log Terminal State (Except Echo)", isOn: $viewModel.logTerminalState)
						.fixedSize()
						.macTermToolTipText("Print high-volume log with detailed terminal emulator input and state transition information, except for characters that will be displayed on the terminal screen.")
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Log Terminal Echo State", isOn: $viewModel.logTerminalEchoState)
						.fixedSize()
						.macTermToolTipText("Print high-volume log with information on characters that will be displayed on the terminal screen.")
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Log Terminal Input Characters", isOn: $viewModel.logTerminalInputCharacters)
						.fixedSize()
						.macTermToolTipText("Print high-volume log showing every character as it is processed.")
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Log Pseudoterminal Device Settings", isOn: $viewModel.logPseudoTerminalDeviceSettings)
						.fixedSize()
						.macTermToolTipText("Print information on low-level terminal device, such as enabled control flags.")
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_OptionLineView("Sixels", noDefaultSpacing: true) {
					Toggle("Log Sixel Graphics Decoder Errors", isOn: $viewModel.logSixelGraphicsDecoderErrors)
						.fixedSize()
						.macTermToolTipText("Print serious problems (such as being unable to process Sixel data correctly).")
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Log Sixel Graphics Decoder State", isOn: $viewModel.logSixelGraphicsDecoderState)
						.fixedSize()
						.macTermToolTipText("Print high-volume log with detailed Sixel decoder input and state transition information.")
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Log Sixel Graphics Summary", isOn: $viewModel.logSixelGraphicsSummary)
						.fixedSize()
						.macTermToolTipText("Print overview details when Sixel graphic is created, such as image dimensions and pixel size.")
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_OptionLineView("Active Terminal", noDefaultSpacing: true) {
					Button(action: { viewModel.runner.dumpStateOfActiveTerminal() }) {
						Text("Log Detailed Snapshot")
							.frame(minWidth: 160)
							.macTermToolTipText("Print debugging summary of frontmost terminal window.")
					}.padding([.bottom], -6) // not debugging alignment guides; for now, just do this
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_OptionLineView("Incomplete Work", noDefaultSpacing: true) {
					Button(action: { viewModel.runner.showTestTerminalToolbar() }) {
						Text("Show Floating Toolbar")
							.frame(minWidth: 160)
							.macTermToolTipText("Display window with toolbar items that is shared and updated as terminal windows are activated.")
					}.padding([.bottom], -6) // not debugging alignment guides; for now, just do this
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Button(action: { viewModel.runner.launchNewCallPythonClient() }) {
						Text("Launch XPC Service")
							.frame(minWidth: 160)
							.macTermToolTipText("Spawn XPC Service to experiment with communication between processes.")
					}.padding([.bottom], -6) // not debugging alignment guides; for now, just do this
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIDebugInterface_ObjC : NSObject {

	@objc public static func makeViewController(_ data: UIDebugInterface_Model) -> NSViewController {
		return NSHostingController<AnyView>(rootView: AnyView(UIDebugInterface_View().environmentObject(data)))
	}

}

public struct UIDebugInterface_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIDebugInterface_Model(runner: UIDebugInterface_RunnerDummy())
		return VStack {
			UIDebugInterface_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIDebugInterface_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
