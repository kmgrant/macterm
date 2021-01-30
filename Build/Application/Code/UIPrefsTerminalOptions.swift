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

@objc public protocol UIPrefsTerminalOptions_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func resetToDefaultGetWrapLines() -> Bool
	func resetToDefaultGetEightBit() -> Bool
	func resetToDefaultGetSaveLinesOnClear() -> Bool
	func resetToDefaultGetNormalKeypadTopRow() -> Bool
	func resetToDefaultGetLocalPageKeys() -> Bool
}

class UIPrefsTerminalOptions_RunnerDummy : NSObject, UIPrefsTerminalOptions_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func resetToDefaultGetWrapLines() -> Bool { print(#function); return false }
	func resetToDefaultGetEightBit() -> Bool { print(#function); return false }
	func resetToDefaultGetSaveLinesOnClear() -> Bool { print(#function); return false }
	func resetToDefaultGetNormalKeypadTopRow() -> Bool { print(#function); return false }
	func resetToDefaultGetLocalPageKeys() -> Bool { print(#function); return false }
}

public class UIPrefsTerminalOptions_Model : UICommon_DefaultingModel, ObservableObject {

	@Published @objc public var isDefaultWrapLines = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { lineWrapEnabled = runner.resetToDefaultGetWrapLines() } }
		}
	}
	@Published @objc public var isDefaultEightBit = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { eightBitEnabled = runner.resetToDefaultGetEightBit() } }
		}
	}
	@Published @objc public var isDefaultSaveLinesOnClear = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { saveLinesOnClearEnabled = runner.resetToDefaultGetSaveLinesOnClear() } }
		}
	}
	@Published @objc public var isDefaultNormalKeypadTopRow = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { normalKeypadTopRowEnabled = runner.resetToDefaultGetNormalKeypadTopRow() } }
		}
	}
	@Published @objc public var isDefaultLocalPageKeys = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { localPageKeysEnabled = runner.resetToDefaultGetLocalPageKeys() } }
		}
	}
	@Published @objc public var lineWrapEnabled = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultWrapLines = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var eightBitEnabled = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultEightBit = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var saveLinesOnClearEnabled = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultSaveLinesOnClear = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var normalKeypadTopRowEnabled = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultNormalKeypadTopRow = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var localPageKeysEnabled = false {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultLocalPageKeys = false }
				runner.dataUpdated()
			}
		}
	}
	public var runner: UIPrefsTerminalOptions_ActionHandling

	@objc public init(runner: UIPrefsTerminalOptions_ActionHandling) {
		self.runner = runner
	}

	// MARK: UICommon_DefaultingModel

	override func setDefaultFlagsToTrue() {
		// unconditional; used by base when swapping to "isEditingDefaultContext"
		isDefaultWrapLines = true
		isDefaultEightBit = true
		isDefaultSaveLinesOnClear = true
		isDefaultNormalKeypadTopRow = true
		isDefaultLocalPageKeys = true
	}

}

public struct UIPrefsTerminalOptions_View : View {

	@EnvironmentObject private var viewModel: UIPrefsTerminalOptions_Model

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			UICommon_DefaultOptionHeaderView()
			UICommon_Default1OptionLineView("General",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Line Wrap",
																							bindIsDefaultTo: $viewModel.isDefaultWrapLines),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Toggle("Wrap lines (no truncation)", isOn: $viewModel.lineWrapEnabled)
					.macTermToolTipText("Set if new text should appear on the next line when the terminal cursor is at the right end of the screen, instead of overwriting the last column.")
			}
			UICommon_Default1OptionLineView("",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "8-Bit Bytes",
																							bindIsDefaultTo: $viewModel.isDefaultEightBit),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Toggle("Do not strip high bit of bytes", isOn: $viewModel.eightBitEnabled)
					.macTermToolTipText("Set if bytes should be treated as 8-bit values instead of 7-bit values (some applications require this).")
			}
			UICommon_Default1OptionLineView("",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Save Lines on Clear",
																							bindIsDefaultTo: $viewModel.isDefaultSaveLinesOnClear),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Toggle("Save lines when screen clears", isOn: $viewModel.saveLinesOnClearEnabled)
					.macTermToolTipText("Set if all the rows of text on the main terminal screen should be inserted into the scrollback buffer instead of going away when the screen clears.")
			}
			Spacer().asMacTermSectionSpacingV()
			UICommon_Default1OptionLineView("Keyboard",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Normal Keypad Top Row",
																							bindIsDefaultTo: $viewModel.isDefaultNormalKeypadTopRow),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Toggle("Normal keypad top row", isOn: $viewModel.normalKeypadTopRowEnabled)
					.macTermToolTipText("Set if keypad buttons on extended keyboards have their normal actions instead of being remapped to the VT PF1, PF2, PF3 and PF4 keys.")
			}
			UICommon_Default1OptionLineView("",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Local Page Keys",
																							bindIsDefaultTo: $viewModel.isDefaultLocalPageKeys),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Toggle("Local page keys (↖︎↘︎⇞⇟)", isOn: $viewModel.localPageKeysEnabled)
					.macTermToolTipText("Set if the Home, End, Page Up and Page Down keys affect local scrolling in MacTerm instead of being sent to the active session as remote paging commands.")
			}
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsTerminalOptions_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsTerminalOptions_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsTerminalOptions_View().environmentObject(data)))
	}

}

public struct UIPrefsTerminalOptions_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsTerminalOptions_Model(runner: UIPrefsTerminalOptions_RunnerDummy())
		return VStack {
			UIPrefsTerminalOptions_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsTerminalOptions_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
