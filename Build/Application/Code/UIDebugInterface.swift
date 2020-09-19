/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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
}

class UIDebugInterface_RunnerDummy : NSObject, UIDebugInterface_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dumpStateOfActiveTerminal() { print(#function) }
	func launchNewCallPythonClient() { print(#function) }
	func showTestTerminalToolbar() { print(#function) }
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
	}
	@Published @objc public var logTerminalEchoState = false {
		willSet(isOn) {
			if isOn {
				print("started logging of terminal state (echo)")
			} else {
				print("no logging of terminal state (echo)")
			}
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
	}
	@Published @objc public var logPseudoTerminalDeviceSettings = false {
		willSet(isOn) {
			if isOn {
				print("started logging of pseudo-terminal device configurations")
			} else {
				print("no logging of pseudo-terminal device configurations")
			}
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
	}
	public var runner: UIDebugInterface_ActionHandling

	@objc public init(runner: UIDebugInterface_ActionHandling) {
		self.runner = runner
	}

}

public struct UIDebugInterface_View : View {

	@EnvironmentObject private var viewModel: UIDebugInterface_Model

	public var body: some View {
		Form {
			Spacer().asMacTermSectionSpacingV()
			Section {
				VStack(
					alignment: .leading
				) {
					UICommon_OptionLineView("Global") {
						Toggle("Log Terminal State (Except Echo)", isOn: $viewModel.logTerminalState)
							.fixedSize()
					}
					UICommon_OptionLineView {
						Toggle("Log Terminal Echo State", isOn: $viewModel.logTerminalEchoState)
							.fixedSize()
					}
					UICommon_OptionLineView {
						Toggle("Log Terminal Input Characters", isOn: $viewModel.logTerminalInputCharacters)
							.fixedSize()
					}
					UICommon_OptionLineView {
						Toggle("Log Pseudoterminal Device Settings", isOn: $viewModel.logPseudoTerminalDeviceSettings)
							.fixedSize()
					}
					UICommon_OptionLineView {
						Toggle("Log Sixel Graphics Decoder State", isOn: $viewModel.logSixelGraphicsDecoderState)
							.fixedSize()
					}
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Section {
				VStack(
					alignment: .leading
				) {
					UICommon_OptionLineView("Active Terminal") {
						Button("Log Detailed Snapshot", action: { viewModel.runner.dumpStateOfActiveTerminal() })
					}
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Section {
				VStack(
					alignment: .leading
				) {
					UICommon_OptionLineView("Incomplete Work") {
						Button("Show Cocoa Toolbar", action: { viewModel.runner.showTestTerminalToolbar() })
					}
					UICommon_OptionLineView {
						Button("Launch XPC Service", action: { viewModel.runner.launchNewCallPythonClient() })
					}
				}
			}
			Spacer().asMacTermSectionSpacingV()
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
