/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
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

@objc public protocol UIPrefsGeneralFullScreen_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
}

class UIPrefsGeneralFullScreen_RunnerDummy : NSObject, UIPrefsGeneralFullScreen_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
}

public class UIPrefsGeneralFullScreen_Model : UICommon_BaseModel, ObservableObject {

	@Published @objc public var showScrollBar = false {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var showMenuBar = false {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var showWindowFrame = false {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var allowForceQuit = false {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	public var runner: UIPrefsGeneralFullScreen_ActionHandling

	@objc public init(runner: UIPrefsGeneralFullScreen_ActionHandling) {
		self.runner = runner
	}

}

public struct UIPrefsGeneralFullScreen_View : View {

	@EnvironmentObject private var viewModel: UIPrefsGeneralFullScreen_Model

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_OptionLineView("Access Control", noDefaultSpacing: true) {
					Toggle("Show scroll bar", isOn: $viewModel.showScrollBar)
						.macTermToolTipText("Set if there should be a vertical scroll bar while in Full Screen mode.")
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Show menu bar on demand", isOn: $viewModel.showMenuBar)
						.macTermToolTipText("Set if the menu bar should appear when the mouse is moved to the top of the screen in Full Screen mode.")
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Show window frame", isOn: $viewModel.showWindowFrame)
						.macTermToolTipText("Set if the window title and toolbar area should always be visible in Full Screen mode.")
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Allow “Force Quit” command", isOn: $viewModel.allowForceQuit)
						.macTermToolTipText("Set if “Force Quit” and similar Apple menu actions should be accessible while in Full Screen mode.")
				}
			}
			Spacer().asMacTermSectionSpacingV()
			HStack {
				Text("You can turn off Full Screen by pressing ⌃⌘F, using the window zoom button or the toolbar, or by selecting “Exit Full Screen” from the View menu.")
					.controlSize(.small)
					.fixedSize(horizontal: false, vertical: true)
					.lineLimit(3)
					.multilineTextAlignment(.leading)
					.frame(maxWidth: 400)
			}.withMacTermSectionLayout()
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsGeneralFullScreen_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsGeneralFullScreen_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsGeneralFullScreen_View().environmentObject(data)))
	}

}

public struct UIPrefsGeneralFullScreen_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsGeneralFullScreen_Model(runner: UIPrefsGeneralFullScreen_RunnerDummy())
		return VStack {
			UIPrefsGeneralFullScreen_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsGeneralFullScreen_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
