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

@objc public protocol UIPrefsGeneralOptions_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
}

class UIPrefsGeneralOptions_RunnerDummy : NSObject, UIPrefsGeneralOptions_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
}

public class UIPrefsGeneralOptions_Model : UICommon_BaseModel, ObservableObject {

	@Published @objc public var noWindowCloseOnExit = false {
		didSet(isOn) { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var noAutoNewWindows = false {
		didSet(isOn) { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var fadeInBackground = false {
		didSet(isOn) { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var invertSelectedText = false {
		didSet(isOn) { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var autoCopySelectedText = false {
		didSet(isOn) { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var moveCursorToTextDropLocation = false {
		didSet(isOn) { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var doNotDimBackgroundTerminalText = false {
		didSet(isOn) { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var doNotWarnAboutMultiLinePaste = false {
		didSet(isOn) { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var mapBackquoteToEscape = false {
		didSet(isOn) { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var focusFollowsMouse = false {
		didSet(isOn) { ifWritebackEnabled { runner.dataUpdated() } }
	}
	public var runner: UIPrefsGeneralOptions_ActionHandling

	@objc public init(runner: UIPrefsGeneralOptions_ActionHandling) {
		self.runner = runner
	}

}

public struct UIPrefsGeneralOptions_View : View {

	@EnvironmentObject private var viewModel: UIPrefsGeneralOptions_Model

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_OptionLineView("Terminal Windows", noDefaultSpacing: true) {
					Toggle("No window close on process exit", isOn: $viewModel.noWindowCloseOnExit)
						//.help("...") // (add when SDK is updated)
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("No automatic new windows", isOn: $viewModel.noAutoNewWindows)
						//.help("...") // (add when SDK is updated)
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Fade in background", isOn: $viewModel.fadeInBackground)
						//.help("...") // (add when SDK is updated)
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_OptionLineView("Text", noDefaultSpacing: true) {
					Toggle("Invert selected text", isOn: $viewModel.invertSelectedText)
						//.help("...") // (add when SDK is updated)
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Automatically Copy selected text", isOn: $viewModel.autoCopySelectedText)
						//.help("...") // (add when SDK is updated)
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Move cursor to text drop location", isOn: $viewModel.moveCursorToTextDropLocation)
						//.help("...") // (add when SDK is updated)
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Don’t dim background terminal text", isOn: $viewModel.doNotDimBackgroundTerminalText)
						//.help("...") // (add when SDK is updated)
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Don’t warn about multi–line Paste", isOn: $viewModel.doNotWarnAboutMultiLinePaste)
						//.help("...") // (add when SDK is updated)
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_OptionLineView("Keyboard", noDefaultSpacing: true) {
					Toggle("Treat backquote key like Escape", isOn: $viewModel.mapBackquoteToEscape)
						//.help("...") // (add when SDK is updated)
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Focus follows mouse", isOn: $viewModel.focusFollowsMouse)
						//.help("...") // (add when SDK is updated)
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsGeneralOptions_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsGeneralOptions_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsGeneralOptions_View().environmentObject(data)))
	}

}

public struct UIPrefsGeneralOptions_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsGeneralOptions_Model(runner: UIPrefsGeneralOptions_RunnerDummy())
		return VStack {
			UIPrefsGeneralOptions_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsGeneralOptions_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
