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
		willSet(newType) {
			ifWritebackEnabled {
				// auto-set "Identity" to a reasonable default based on this
				switch newType {
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
		didSet(newType) {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultBaseEmulator = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var terminalIdentity = "xterm-256color" {
		didSet(isOn) {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultIdentity = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var tweak24BitColorEnabled = false {
		didSet(isOn) {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakITermGraphicsEnabled = false {
		didSet(isOn) {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakVT100FixLineWrappingBugEnabled = false {
		didSet(isOn) {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakSixelGraphicsEnabled = false {
		didSet(isOn) {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakXTerm256ColorsEnabled = false {
		didSet(isOn) {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakXTermBackgroundColorEraseEnabled = false {
		didSet(isOn) {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakXTermColorEnabled = false {
		didSet(isOn) {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakXTermGraphicsEnabled = false {
		didSet(isOn) {
			updateTweakValue()
		}
	}
	@Published @objc public var tweakXTermWindowAlterationEnabled = false {
		didSet(isOn) {
			updateTweakValue()
		}
	}
	public var runner: UIPrefsTerminalEmulation_ActionHandling

	@objc public init(runner: UIPrefsTerminalEmulation_ActionHandling) {
		self.runner = runner
	}

	private func updateTweakValue() {
		// call this from a "didSet(isOn)" above...this is a shared method
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
			UICommon_Default1OptionLineView("Base Emulator", bindIsDefaultTo: $viewModel.isDefaultBaseEmulator, isEditingDefault: viewModel.isEditingDefaultContext) {
				Picker("", selection: $viewModel.selectedBaseEmulatorType) {
					localizedLabelView(.none)
					localizedLabelView(.vt100)
					localizedLabelView(.vt102)
					localizedLabelView(.vt220)
					localizedLabelView(.xterm)
				}.pickerStyle(PopUpButtonPickerStyle())
					.offset(x: -8, y: 0) // TEMPORARY; to eliminate left-padding created by Picker for empty label
					.frame(minWidth: 160, maxWidth: 160)
					//.help("How to manage text that has scrolled off the top of the terminal screen.") // (add when SDK is updated)
			}
			Spacer().asMacTermSectionSpacingV()
			UICommon_Default1OptionLineView("Identity", bindIsDefaultTo: $viewModel.isDefaultIdentity, isEditingDefault: viewModel.isEditingDefaultContext) {
				TextField("", text: $viewModel.terminalIdentity)
					.frame(minWidth: 150, maxWidth: 150)
					//.help("Number of rows visible on the terminal screen.") // (add when SDK is updated)
			}
			Spacer().asMacTermSectionSpacingV()
			UICommon_Default1OptionLineView("Emulation Tweaks", bindIsDefaultTo: $viewModel.isDefaultEmulationTweaks, isEditingDefault: viewModel.isEditingDefaultContext,
											disableDefaultAlignmentGuide: true) {
				List {
					Toggle("24-Bit Color (Millions)", isOn: $viewModel.tweak24BitColorEnabled)
						//.help("...") // (add when SDK is updated)
					Toggle("iTerm Graphics", isOn: $viewModel.tweakITermGraphicsEnabled)
						//.help("...") // (add when SDK is updated)
					Toggle("VT100 Fix Line Wrapping Bug", isOn: $viewModel.tweakVT100FixLineWrappingBugEnabled)
						//.help("...") // (add when SDK is updated)
					Toggle("VT340 Sixel Graphics", isOn: $viewModel.tweakSixelGraphicsEnabled)
						//.help("...") // (add when SDK is updated)
					Toggle("XTerm 256 Colors", isOn: $viewModel.tweakXTerm256ColorsEnabled)
						//.help("...") // (add when SDK is updated)
					Toggle("XTerm Background Color Erase", isOn: $viewModel.tweakXTermBackgroundColorEraseEnabled)
						//.help("...") // (add when SDK is updated)
					Toggle("XTerm Color", isOn: $viewModel.tweakXTermColorEnabled)
						//.help("...") // (add when SDK is updated)
					Toggle("XTerm Graphics Characters", isOn: $viewModel.tweakXTermGraphicsEnabled)
						//.help("...") // (add when SDK is updated)
					Toggle("XTerm Window Alteration", isOn: $viewModel.tweakXTermWindowAlterationEnabled)
						//.help("...") // (add when SDK is updated)
				}.frame(minWidth: 250, minHeight: 48, idealHeight: 150)
					.fixedSize()
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.top] + 13 }) // TEMPORARY; try to find a nicer way to do this (top-align both)
			}
			UICommon_OptionLineView("") {
				Text("The Identity should accurately describe the set of tweaks (for example, “xterm-256color”) so that applications will know what to use.")
					.controlSize(.small)
					.fixedSize(horizontal: false, vertical: true)
					.lineLimit(3)
					.multilineTextAlignment(.leading)
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
					.frame(maxWidth: 250)
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
