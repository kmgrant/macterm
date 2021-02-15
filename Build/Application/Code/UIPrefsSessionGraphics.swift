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

@objc public protocol UIPrefsSessionGraphics_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func resetToDefaultGetSelectedTEKMode() -> VectorInterpreter_Mode
	func resetToDefaultGetPageCommandClearsScreen() -> Bool
}

class UIPrefsSessionGraphics_RunnerDummy : NSObject, UIPrefsSessionGraphics_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func resetToDefaultGetSelectedTEKMode() -> VectorInterpreter_Mode { print(#function); return .TEK4105 }
	func resetToDefaultGetPageCommandClearsScreen() -> Bool { print(#function); return false }
}

public class UIPrefsSessionGraphics_Model : UICommon_DefaultingModel, ObservableObject {

	@Published @objc public var isDefaultTEKMode = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { selectedTEKMode = runner.resetToDefaultGetSelectedTEKMode() } }
		}
	}
	@Published @objc public var isDefaultPageClears = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { pageCommandClearsScreen = runner.resetToDefaultGetPageCommandClearsScreen() } }
		}
	}
	@Published @objc public var selectedTEKMode: VectorInterpreter_Mode = .disabled {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultTEKMode = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var pageCommandClearsScreen = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultPageClears = false }
				runner.dataUpdated()
			}
		}
	}
	public var runner: UIPrefsSessionGraphics_ActionHandling

	@objc public init(runner: UIPrefsSessionGraphics_ActionHandling) {
		self.runner = runner
	}

	// MARK: UICommon_DefaultingModel

	override func setDefaultFlagsToTrue() {
		// unconditional; used by base when swapping to "isEditingDefaultContext"
		isDefaultTEKMode = true
		isDefaultPageClears = true
	}

}

public struct UIPrefsSessionGraphics_View : View {

	@EnvironmentObject private var viewModel: UIPrefsSessionGraphics_Model

	func localizedLabelView(_ forType: VectorInterpreter_Mode) -> some View {
		var aTitle: String = ""
		switch forType {
		case .disabled:
			aTitle = "Disabled"
		case .TEK4014:
			aTitle = "TEK 4014 (black and white)"
		case .TEK4105:
			aTitle = "TEK 4105 (color)"
		}
		return Text(aTitle).tag(forType)
	}

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			UICommon_DefaultOptionHeaderView()
			UICommon_Default1OptionLineView("TEK Graphics",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "TEK Graphics",
																							bindIsDefaultTo: $viewModel.isDefaultTEKMode),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Picker("", selection: $viewModel.selectedTEKMode) {
					localizedLabelView(.disabled)
					localizedLabelView(.TEK4014)
					localizedLabelView(.TEK4105)
				}.pickerStyle(RadioGroupPickerStyle())
					.macTermOffsetForEmptyPickerTitle()
					.macTermToolTipText("Type of vector graphics sequences to recognize in this session.")
			}
			Spacer().asMacTermSectionSpacingV()
			UICommon_Default1OptionLineView("Options",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "PAGE Clears Screen",
																							bindIsDefaultTo: $viewModel.isDefaultPageClears),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Toggle("PAGE clears screen", isOn: $viewModel.pageCommandClearsScreen)
					.macTermToolTipText("Set if a TEK graphics “PAGE” command clears the screen of the current vector graphics window instead of opening a new window.")
			}
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsSessionGraphics_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsSessionGraphics_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsSessionGraphics_View().environmentObject(data)))
	}

}

public struct UIPrefsSessionGraphics_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsSessionGraphics_Model(runner: UIPrefsSessionGraphics_RunnerDummy())
		return VStack {
			UIPrefsSessionGraphics_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsSessionGraphics_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
