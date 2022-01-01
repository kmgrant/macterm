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

@objc public protocol UIPrefsWorkspaceOptions_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func resetToDefaultGetAutoFullScreen() -> Bool
	func resetToDefaultGetUseTabs() -> Bool
}

class UIPrefsWorkspaceOptions_RunnerDummy : NSObject, UIPrefsWorkspaceOptions_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func resetToDefaultGetAutoFullScreen() -> Bool { print(#function); return false }
	func resetToDefaultGetUseTabs() -> Bool { print(#function); return false }
}

public class UIPrefsWorkspaceOptions_Model : UICommon_DefaultingModel, ObservableObject {

	@Published @objc public var isDefaultAutoFullScreen = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { autoFullScreenEnabled = runner.resetToDefaultGetAutoFullScreen() } }
		}
	}
	@Published @objc public var isDefaultUseTabs = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { windowTabsEnabled = runner.resetToDefaultGetUseTabs() } }
		}
	}
	@Published @objc public var autoFullScreenEnabled = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultAutoFullScreen = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var windowTabsEnabled = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultUseTabs = false }
				runner.dataUpdated()
			}
		}
	}
	public var runner: UIPrefsWorkspaceOptions_ActionHandling

	@objc public init(runner: UIPrefsWorkspaceOptions_ActionHandling) {
		self.runner = runner
	}

	// MARK: UICommon_DefaultingModel

	override func setDefaultFlagsToTrue() {
		// unconditional; used by base when swapping to "isEditingDefaultContext"
		isDefaultAutoFullScreen = true
		isDefaultUseTabs = true
	}

}

public struct UIPrefsWorkspaceOptions_View : View {

	@EnvironmentObject private var viewModel: UIPrefsWorkspaceOptions_Model

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			UICommon_DefaultOptionHeaderView()
			UICommon_Default1OptionLineView("Options",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Use Tabs",
																							bindIsDefaultTo: $viewModel.isDefaultUseTabs),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Toggle("Use tabs to arrange windows", isOn: $viewModel.windowTabsEnabled)
					.macTermToolTipText("Set if the terminals for this workspace appear as tabs in the same window frame.")
			}
			UICommon_Default1OptionLineView("",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Auto Full Screen",
																							bindIsDefaultTo: $viewModel.isDefaultAutoFullScreen),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Toggle("Automatically enter Full Screen", isOn: $viewModel.autoFullScreenEnabled)
					.macTermToolTipText("Set if new windows from this workspace should immediately go into Full Screen view.")
			}
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsWorkspaceOptions_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsWorkspaceOptions_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsWorkspaceOptions_View().environmentObject(data)))
	}

}

public struct UIPrefsWorkspaceOptions_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsWorkspaceOptions_Model(runner: UIPrefsWorkspaceOptions_RunnerDummy())
		return VStack {
			UIPrefsWorkspaceOptions_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsWorkspaceOptions_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
