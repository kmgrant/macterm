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

@objc public enum UIPrefsTerminalEmulation_BaseEmulatorType : Int {
	case none // “dumb” terminal
	case vt100 // VT100 strict terminal
	case vt102 // VT102 strict terminal
	case vt220 // VT220 strict terminal
	case xterm // XTerm (partial) terminal
}

@objc public protocol UIPrefsTerminalEmulation_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func resetToDefaultGetSelectedBaseEmulatorType() -> UIPrefsTerminalEmulation_BaseEmulatorType
	func resetToDefaultGetIdentity() -> String
	func resetToDefaultGetTweak24BitColorEnabled() -> Bool
	func resetToDefaultGetTweakITermGraphicsEnabled() -> Bool
	func resetToDefaultGetTweakVT100FixLineWrappingBugEnabled() -> Bool
	func resetToDefaultGetTweakSixelGraphicsEnabled() -> Bool
	func resetToDefaultGetTweakXTerm256ColorsEnabled() -> Bool
	func resetToDefaultGetTweakXTermBackgroundColorEraseEnabled() -> Bool
	func resetToDefaultGetTweakXTermColorEnabled() -> Bool
	func resetToDefaultGetTweakXTermGraphicsEnabled() -> Bool
	func resetToDefaultGetTweakXTermWindowAlterationEnabled() -> Bool
}

class UIPrefsTerminalEmulation_RunnerDummy : NSObject, UIPrefsTerminalEmulation_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func resetToDefaultGetSelectedBaseEmulatorType() -> UIPrefsTerminalEmulation_BaseEmulatorType { print(#function); return .xterm }
	func resetToDefaultGetIdentity() -> String { print(#function); return "xterm-256color" }
	func resetToDefaultGetTweak24BitColorEnabled() -> Bool { print(#function); return false }
	func resetToDefaultGetTweakITermGraphicsEnabled() -> Bool { print(#function); return false }
	func resetToDefaultGetTweakVT100FixLineWrappingBugEnabled() -> Bool { print(#function); return false }
	func resetToDefaultGetTweakSixelGraphicsEnabled() -> Bool { print(#function); return false }
	func resetToDefaultGetTweakXTerm256ColorsEnabled() -> Bool { print(#function); return false }
	func resetToDefaultGetTweakXTermBackgroundColorEraseEnabled() -> Bool { print(#function); return false }
	func resetToDefaultGetTweakXTermColorEnabled() -> Bool { print(#function); return false }
	func resetToDefaultGetTweakXTermGraphicsEnabled() -> Bool { print(#function); return false }
	func resetToDefaultGetTweakXTermWindowAlterationEnabled() -> Bool { print(#function); return false }
}

public class UIPrefsTerminalEmulation_Model : UICommon_DefaultingModel, ObservableObject {

	@Published @objc public var isDefaultBaseEmulator = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { selectedBaseEmulatorType = runner.resetToDefaultGetSelectedBaseEmulatorType() } }
		}
	}
	@Published @objc public var isDefaultIdentity = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { terminalIdentity = runner.resetToDefaultGetIdentity() } }
		}
	}
	@Published @objc public var isDefaultEmulationTweaks = true {
		willSet(isOn) {
			if isOn {
				ifUserRequestedDefault {
					tweak24BitColorEnabled = runner.resetToDefaultGetTweak24BitColorEnabled()
					tweakITermGraphicsEnabled = runner.resetToDefaultGetTweakITermGraphicsEnabled()
					tweakVT100FixLineWrappingBugEnabled = runner.resetToDefaultGetTweakVT100FixLineWrappingBugEnabled()
					tweakSixelGraphicsEnabled = runner.resetToDefaultGetTweakSixelGraphicsEnabled()
					tweakXTerm256ColorsEnabled = runner.resetToDefaultGetTweakXTerm256ColorsEnabled()
					tweakXTermBackgroundColorEraseEnabled = runner.resetToDefaultGetTweakXTermBackgroundColorEraseEnabled()
					tweakXTermColorEnabled = runner.resetToDefaultGetTweakXTermColorEnabled()
					tweakXTermGraphicsEnabled = runner.resetToDefaultGetTweakXTermGraphicsEnabled()
					tweakXTermWindowAlterationEnabled = runner.resetToDefaultGetTweakXTermWindowAlterationEnabled()
				}
			}
		}
	}
	@Published @objc public var selectedBaseEmulatorType: UIPrefsTerminalEmulation_BaseEmulatorType = .xterm {
		willSet {
			ifWritebackEnabled {
				// auto-set "Identity" to a reasonable default based on this
				switch newValue {
				case .none:
					terminalIdentity = "dumb"
				case .vt100:
					terminalIdentity = "vt100"
				case .vt102:
					terminalIdentity = "vt102"
				case .vt220:
					terminalIdentity = "vt220"
				case .xterm:
					terminalIdentity = "xterm"
				}
			}
		}
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultBaseEmulator = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var terminalIdentity = "xterm-256color" {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultIdentity = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var tweak24BitColorEnabled = false {
		didSet {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakITermGraphicsEnabled = false {
		didSet {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakVT100FixLineWrappingBugEnabled = false {
		didSet {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakSixelGraphicsEnabled = false {
		didSet {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakXTerm256ColorsEnabled = false {
		didSet {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakXTermBackgroundColorEraseEnabled = false {
		didSet {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakXTermColorEnabled = false {
		didSet {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakXTermGraphicsEnabled = false {
		didSet {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakXTermWindowAlterationEnabled = false {
		didSet {
			updateTweakValue()
		}
	}
	public var runner: UIPrefsTerminalEmulation_ActionHandling

	@objc public init(runner: UIPrefsTerminalEmulation_ActionHandling) {
		self.runner = runner
	}

	private func updateTweakValue() {
		// call this from a "didSet" above...this is a shared method
		// since they are all bound to the same Default flag
		ifWritebackEnabled {
			inNonDefaultContext { isDefaultEmulationTweaks = false }
			runner.dataUpdated()
		}
	}

	// MARK: UICommon_DefaultingModel

	override func setDefaultFlagsToTrue() {
		// unconditional; used by base when swapping to "isEditingDefaultContext"
		isDefaultBaseEmulator = true
		isDefaultIdentity = true
		isDefaultEmulationTweaks = true
	}

}

public struct UIPrefsTerminalEmulation_View : View {

	@EnvironmentObject private var viewModel: UIPrefsTerminalEmulation_Model

	func localizedLabelView(_ forType: UIPrefsTerminalEmulation_BaseEmulatorType) -> some View {
		var aTitle: String = ""
		switch forType {
		case .none:
			aTitle = "None (“Dumb”)"
		case .vt100:
			aTitle = "VT100"
		case .vt102:
			aTitle = "VT102"
		case .vt220:
			aTitle = "VT220"
		case .xterm:
			aTitle = "XTerm"
		}
		return Text(aTitle).tag(forType)
	}

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			UICommon_DefaultOptionHeaderView()
			UICommon_Default1OptionLineView("Base Emulator",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Base Emulator",
																							bindIsDefaultTo: $viewModel.isDefaultBaseEmulator),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				Picker("", selection: $viewModel.selectedBaseEmulatorType) {
					localizedLabelView(.none)
					localizedLabelView(.vt100)
					localizedLabelView(.vt102)
					localizedLabelView(.vt220)
					localizedLabelView(.xterm)
				}.pickerStyle(PopUpButtonPickerStyle())
					.macTermOffsetForEmptyPickerTitle()
					.frame(minWidth: 160, maxWidth: 160)
					.macTermToolTipText("The hardware that terminal sequences are generally expected to match (see “Emulation Tweaks” below though).")
			}
			Spacer().asMacTermMinorSectionSpacingV()
			UICommon_Default1OptionLineView("Identity",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Identity",
																							bindIsDefaultTo: $viewModel.isDefaultIdentity),
											isEditingDefault: viewModel.isEditingDefaultContext) {
				TextField("", text: $viewModel.terminalIdentity)
					.frame(minWidth: 150, maxWidth: 150)
					.macTermToolTipText("An answer-back message that applications can query to determine MacTerm’s capabilities.  For example, on Unix this might match a $TERM variable.")
			}
			Spacer().asMacTermMinorSectionSpacingV()
			UICommon_Default1OptionLineView("Emulation Tweaks",
											toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Emulation Tweaks",
																							bindIsDefaultTo: $viewModel.isDefaultEmulationTweaks),
											isEditingDefault: viewModel.isEditingDefaultContext,
											disableDefaultAlignmentGuide: true) {
				List {
					Toggle("24-Bit Color (Millions)", isOn: $viewModel.tweak24BitColorEnabled)
						.macTermToolTipText("Set if terminal sequences for “true color” should be allowed.")
					Toggle("iTerm Graphics", isOn: $viewModel.tweakITermGraphicsEnabled)
						.macTermToolTipText("Set if terminal sequences for reading Base64-encoded image data (as defined by the iTerm emulator) should be recognized by MacTerm.")
					Toggle("VT100 Fix Line Wrapping Bug", isOn: $viewModel.tweakVT100FixLineWrappingBugEnabled)
						.macTermToolTipText("Set if VT100 terminals should be slightly less strict when the cursor reaches the end position, wrapping as expected instead of according to VT spec.")
					Toggle("VT340 Sixel Graphics", isOn: $viewModel.tweakSixelGraphicsEnabled)
						.macTermToolTipText("Set if terminal sequences for SIXEL bitmap graphics should be recognized.")
					Toggle("XTerm 256 Colors", isOn: $viewModel.tweakXTerm256ColorsEnabled)
						.macTermToolTipText("Set if XTerm sequences for extra colors should be recognized.")
					Toggle("XTerm Background Color Erase", isOn: $viewModel.tweakXTermBackgroundColorEraseEnabled)
						.macTermToolTipText("Set if a simple XTerm sequence for erasing is recognized (can be more efficient if applications support it).")
					Toggle("XTerm Color", isOn: $viewModel.tweakXTermColorEnabled)
						.macTermToolTipText("Set if XTerm color sequences are recognized generally.")
					Toggle("XTerm Graphics Characters", isOn: $viewModel.tweakXTermGraphicsEnabled)
						.macTermToolTipText("Set if XTerm graphics character sets are recognized (not likely to be needed anymore with UTF-8/Unicode).")
					Toggle("XTerm Window Alteration", isOn: $viewModel.tweakXTermWindowAlterationEnabled)
						.macTermToolTipText("Set if terminal applications can cause MacTerm windows or tabs to change (for example, to update the title text).")
				}.frame(minWidth: 260, minHeight: 48, idealHeight: 150)
					.fixedSize()
					.macTermSectionAlignmentGuideList()
			}
			UICommon_OptionLineView("") {
				Text("The Identity should accurately describe the set of tweaks (for example, “xterm-256color”) so that applications will know what to use.")
					.controlSize(.small)
					.fixedSize(horizontal: false, vertical: true)
					.lineLimit(3)
					.multilineTextAlignment(.leading)
					.frame(maxWidth: 250, alignment: .leading)
			}
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsTerminalEmulation_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsTerminalEmulation_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsTerminalEmulation_View().environmentObject(data)))
	}

}

public struct UIPrefsTerminalEmulation_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsTerminalEmulation_Model(runner: UIPrefsTerminalEmulation_RunnerDummy())
		return VStack {
			UIPrefsTerminalEmulation_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsTerminalEmulation_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
