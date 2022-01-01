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

@objc public protocol UIPrefsTerminalScreen_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func resetToDefaultGetWidth() -> Int
	func resetToDefaultGetHeight() -> Int
	func resetToDefaultGetScrollbackRowCount() -> Int
	func resetToDefaultGetScrollbackType() -> Terminal_ScrollbackType
}

class UIPrefsTerminalScreen_RunnerDummy : NSObject, UIPrefsTerminalScreen_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func resetToDefaultGetWidth() -> Int { print(#function); return 80 }
	func resetToDefaultGetHeight() -> Int { print(#function); return 24 }
	func resetToDefaultGetScrollbackRowCount() -> Int { print(#function); return 200 }
	func resetToDefaultGetScrollbackType() -> Terminal_ScrollbackType { print(#function); return .disabled }
}

public class UIPrefsTerminalScreen_Model : UICommon_DefaultingModel, ObservableObject {

	@Published @objc public var isDefaultWidth = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { widthValue = Double(runner.resetToDefaultGetWidth()) } }
		}
	}
	@Published @objc public var isDefaultHeight = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { heightValue = Double(runner.resetToDefaultGetHeight()) } }
		}
	}
	@Published @objc public var isDefaultScrollback = true {
		willSet(isOn) {
			if isOn {
				ifUserRequestedDefault {
					scrollbackValue = Double(runner.resetToDefaultGetScrollbackRowCount())
					selectedScrollbackType = runner.resetToDefaultGetScrollbackType()
				}
			}
		}
	}
	// NOTE: SwiftUI currently does not handle unexpected numerical conversions
	// (e.g. if user enters a floating-point value but binding is just an Int,
	// this is an untrappable runtime error!); for user-friendliness, the binding
	// is using a more-flexible Double type, despite expecting Int, so that it is
	// not possible for weird inputs to crash the app!!!
	@Published @objc public var widthValue = 80.0 {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultWidth = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var heightValue = 24.0 {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultHeight = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var scrollbackValue = 200.0 {
		didSet {
			ifWritebackEnabled {
				if scrollbackValue <= 0.5 {
					// auto-off for zero value (causes same update triggers; see below)
					selectedScrollbackType = .disabled
				} else {
					inNonDefaultContext { isDefaultScrollback = false }
					runner.dataUpdated()
				}
			}
		}
	}
	@Published @objc public var selectedScrollbackType: Terminal_ScrollbackType = .disabled {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultScrollback = false }
				runner.dataUpdated()
			}
		}
	}
	public var runner: UIPrefsTerminalScreen_ActionHandling

	@objc public init(runner: UIPrefsTerminalScreen_ActionHandling) {
		self.runner = runner
	}

	// MARK: UICommon_DefaultingModel

	override func setDefaultFlagsToTrue() {
		// unconditional; used by base when swapping to "isEditingDefaultContext"
		isDefaultWidth = true
		isDefaultHeight = true
		isDefaultScrollback = true
	}

}

public struct UIPrefsTerminalScreen_View : View {

	@EnvironmentObject private var viewModel: UIPrefsTerminalScreen_Model
	private var widthFormatter = NumberFormatter()
	private var heightFormatter = NumberFormatter()
	private var scrollbackRowsFormatter = NumberFormatter()

	func isFixedRowCountDisabled() -> Bool {
		return (.fixed != viewModel.selectedScrollbackType)
	}

	func localizedLabelView(_ forType: Terminal_ScrollbackType) -> some View {
		var aTitle: String = ""
		switch forType {
		case .disabled:
			aTitle = "Off"
		case .fixed:
			aTitle = "Fixed Size"
		case .unlimited:
			aTitle = "Unlimited"
		case .distributed:
			aTitle = "Distributed"
		}
		return Text(aTitle).tag(forType)
	}

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			UICommon_DefaultOptionHeaderView()
			UICommon_Default1OptionLineView("Width",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Width",
																							bindIsDefaultTo: $viewModel.isDefaultWidth),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				TextField("", value: $viewModel.widthValue, formatter: widthFormatter)
					.frame(minWidth: 50, maxWidth: 50)
					.macTermToolTipText("Number of columns visible on the terminal screen.")
				Spacer()
			}
			Spacer().asMacTermMinorSectionSpacingV()
			UICommon_Default1OptionLineView("Height",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Height",
																							bindIsDefaultTo: $viewModel.isDefaultHeight),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				TextField("", value: $viewModel.heightValue, formatter: heightFormatter)
					.frame(minWidth: 50, maxWidth: 50)
					.macTermToolTipText("Number of rows visible on the terminal screen.")
			}
			Spacer().asMacTermMinorSectionSpacingV()
			UICommon_Default1OptionLineView("Scrollback",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Scrollback",
																							bindIsDefaultTo: $viewModel.isDefaultScrollback),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Picker("", selection: $viewModel.selectedScrollbackType) {
					// TBD: how to insert dividing-line in this type of menu?
					localizedLabelView(.disabled)
					localizedLabelView(.fixed)
					localizedLabelView(.unlimited)
					localizedLabelView(.distributed)
				}.pickerStyle(PopUpButtonPickerStyle())
					.macTermOffsetForEmptyPickerTitle()
					.frame(minWidth: 160, maxWidth: 160)
					.macTermToolTipText("How to manage text that has scrolled off the top of the terminal screen.")
			}
			UICommon_OptionLineView() {
				HStack(
					alignment: .firstTextBaseline
				) {
					TextField("", value: $viewModel.scrollbackValue, formatter: scrollbackRowsFormatter)
						.disabled(isFixedRowCountDisabled())
						.foregroundColor(isFixedRowCountDisabled() ? .gray : .primary) // default behavior of disabled() does not dim the text so do that manually
						.frame(maxWidth: 80)
						.padding([.leading], 16)
						.macTermToolTipText("Number of lines of text to keep after they scroll off the top of the terminal screen (any more, and the oldest lines are discarded).")
					Text("rows")
						.disabled(isFixedRowCountDisabled())
						.foregroundColor(isFixedRowCountDisabled() ? .gray : .primary) // default behavior of disabled() does not dim the text so do that manually
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsTerminalScreen_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsTerminalScreen_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsTerminalScreen_View().environmentObject(data)))
	}

}

public struct UIPrefsTerminalScreen_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsTerminalScreen_Model(runner: UIPrefsTerminalScreen_RunnerDummy())
		return VStack {
			UIPrefsTerminalScreen_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsTerminalScreen_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
