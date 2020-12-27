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

@objc public protocol UIPrefsSessionKeyboard_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func selectNewInterruptKey(viewModel: UIPrefsSessionKeyboard_Model) // caller should set "viewModel.isKeypadBindingToInterruptKey" while bound
	func selectNewSuspendKey(viewModel: UIPrefsSessionKeyboard_Model) // caller should set "viewModel.isKeypadBindingToSuspendKey" while bound
	func selectNewResumeKey(viewModel: UIPrefsSessionKeyboard_Model) // caller should set "viewModel.isKeypadBindingToResumeKey" while bound
	func resetToDefaultGetInterruptKeyMapping() -> UIKeypads_KeyID
	func resetToDefaultGetSuspendKeyMapping() -> UIKeypads_KeyID
	func resetToDefaultGetResumeKeyMapping() -> UIKeypads_KeyID
	func resetToDefaultGetArrowKeysMapToEmacs() -> Bool
	func resetToDefaultGetSelectedMetaMapping() -> Session_EmacsMetaKey
	func resetToDefaultGetDeleteSendsBackspace() -> Bool
	func resetToDefaultGetSelectedNewlineMapping() -> Session_NewlineMode
}

class UIPrefsSessionKeyboard_RunnerDummy : NSObject, UIPrefsSessionKeyboard_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func selectNewInterruptKey(viewModel: UIPrefsSessionKeyboard_Model) { print(#function); viewModel.isKeypadBindingToInterruptKey = true }
	func selectNewSuspendKey(viewModel: UIPrefsSessionKeyboard_Model) { print(#function); viewModel.isKeypadBindingToSuspendKey = true }
	func selectNewResumeKey(viewModel: UIPrefsSessionKeyboard_Model) { print(#function); viewModel.isKeypadBindingToResumeKey = true }
	func resetToDefaultGetInterruptKeyMapping() -> UIKeypads_KeyID { print(#function); return .controlC }
	func resetToDefaultGetSuspendKeyMapping() -> UIKeypads_KeyID { print(#function); return .controlS }
	func resetToDefaultGetResumeKeyMapping() -> UIKeypads_KeyID { print(#function); return .controlQ }
	func resetToDefaultGetArrowKeysMapToEmacs() -> Bool { print(#function); return false }
	func resetToDefaultGetSelectedMetaMapping() -> Session_EmacsMetaKey { print(#function); return .off }
	func resetToDefaultGetDeleteSendsBackspace() -> Bool { print(#function); return false }
	func resetToDefaultGetSelectedNewlineMapping() -> Session_NewlineMode { print(#function); return .mapLF }
}

public class UIPrefsSessionKeyboard_Model : UICommon_DefaultingModel, ObservableObject {

	func clearKeys() {
		// helper for key-binding button actions
		isKeypadBindingToInterruptKey = false
		isKeypadBindingToSuspendKey = false
		isKeypadBindingToResumeKey = false
	}

	@Published @objc public var isKeypadBindingToInterruptKey = false
	@Published @objc public var isKeypadBindingToSuspendKey = false
	@Published @objc public var isKeypadBindingToResumeKey = false
	@Published @objc public var isDefaultInterruptKeyMapping = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { interruptKeyMapping = runner.resetToDefaultGetInterruptKeyMapping() } }
		}
	}
	@Published @objc public var isDefaultSuspendKeyMapping = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { suspendKeyMapping = runner.resetToDefaultGetSuspendKeyMapping() } }
		}
	}
	@Published @objc public var isDefaultResumeKeyMapping = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { resumeKeyMapping = runner.resetToDefaultGetResumeKeyMapping() } }
		}
	}
	@Published @objc public var isDefaultEmacsCursorArrows = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { arrowKeysMapToEmacs = runner.resetToDefaultGetArrowKeysMapToEmacs() } }
		}
	}
	@Published @objc public var isDefaultEmacsMetaMapping = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { selectedMetaMapping = runner.resetToDefaultGetSelectedMetaMapping() } }
		}
	}
	@Published @objc public var isDefaultDeleteMapping = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { deleteSendsBackspace = runner.resetToDefaultGetDeleteSendsBackspace() } }
		}
	}
	@Published @objc public var isDefaultNewlineMapping = true {
		willSet(isOn) {
			if isOn { ifUserRequestedDefault { selectedNewlineMapping = runner.resetToDefaultGetSelectedNewlineMapping() } }
		}
	}
	@Published @objc public var interruptKeyMapping: UIKeypads_KeyID = .controlC {
		didSet(isOn) {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultInterruptKeyMapping = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var suspendKeyMapping: UIKeypads_KeyID = .controlS {
		didSet(isOn) {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultSuspendKeyMapping = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var resumeKeyMapping: UIKeypads_KeyID = .controlQ {
		didSet(isOn) {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultResumeKeyMapping = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var arrowKeysMapToEmacs = false {
		didSet(isOn) {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultEmacsCursorArrows = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var selectedMetaMapping: Session_EmacsMetaKey = .off {
		didSet(newType) {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultEmacsMetaMapping = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var deleteSendsBackspace = false {
		didSet(isOn) {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultDeleteMapping = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var selectedNewlineMapping: Session_NewlineMode = .mapLF {
		didSet(newType) {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultNewlineMapping = false }
				runner.dataUpdated()
			}
		}
	}
	public var runner: UIPrefsSessionKeyboard_ActionHandling

	@objc public init(runner: UIPrefsSessionKeyboard_ActionHandling) {
		self.runner = runner
	}

	// MARK: UICommon_DefaultingModel

	override func setDefaultFlagsToTrue() {
		// unconditional; used by base when swapping to "isEditingDefaultContext"
		isDefaultInterruptKeyMapping = true
		isDefaultSuspendKeyMapping = true
		isDefaultResumeKeyMapping = true
		isDefaultEmacsCursorArrows = true
		isDefaultEmacsMetaMapping = true
		isDefaultDeleteMapping = true
		isDefaultNewlineMapping = true
	}

}

public struct UIPrefsSessionKeyboard_View : View {

	@EnvironmentObject private var viewModel: UIPrefsSessionKeyboard_Model

	func localizedLabelView(_ forType: UIKeypads_KeyID) -> some View {
		return UIKeypads_ControlKeysView.localizedLabelView(forType)
	}

	func localizedLabelView(_ forType: Session_EmacsMetaKey) -> some View {
		var aTitle: String = ""
		switch forType {
		case .off:
			aTitle = "Off"
		case .option:
			aTitle = "⌥"
		case .shiftOption:
			aTitle = "⇧ ⌥"
		}
		return Text(aTitle).tag(forType)
	}

	func localizedLabelView(_ forType: Session_NewlineMode) -> some View {
		var aTitle: String = ""
		switch forType {
		case .mapLF:
			aTitle = "LF"
		case .mapCR:
			aTitle = "CR"
		case .mapCRLF:
			aTitle = "CR LF"
		case .mapCRNull:
			aTitle = "CR NULL"
		}
		return Text(aTitle).tag(forType)
	}

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			UICommon_DefaultOptionHeaderView()
			Group {
				UICommon_Default1OptionLineView("Process Control", bindIsDefaultTo: $viewModel.isDefaultInterruptKeyMapping, isEditingDefault: viewModel.isEditingDefaultContext) {
					ZStack(
						alignment: .trailing
					) {
						Button(action: {
							if viewModel.isKeypadBindingToInterruptKey {
								viewModel.isKeypadBindingToInterruptKey = false
							} else {
								viewModel.clearKeys()
								viewModel.runner.selectNewInterruptKey(viewModel: viewModel)
							}
						}) {
							localizedLabelView(viewModel.interruptKeyMapping)
						}.asMacTermKeypadKeyRectCompact()
							.macTermToolTipText("Click this to show the “Control Keys” palette, then choose a new key from the palette to bind the setting.")
						if viewModel.isKeypadBindingToInterruptKey {
							if #available(macOS 11.0, *) {
								Image(systemName: "keyboard")
							} else {
								Text("!! ")
							}
						}
					}
					Text("Interrupt Process")
				}
				UICommon_Default1OptionLineView("Flow Control", bindIsDefaultTo: $viewModel.isDefaultSuspendKeyMapping, isEditingDefault: viewModel.isEditingDefaultContext) {
					ZStack(
						alignment: .trailing
					) {
						Button(action: {
							if viewModel.isKeypadBindingToSuspendKey {
								viewModel.isKeypadBindingToSuspendKey = false
							} else {
								viewModel.clearKeys()
								viewModel.runner.selectNewSuspendKey(viewModel: viewModel)
							}
						}) {
							localizedLabelView(viewModel.suspendKeyMapping)
						}.asMacTermKeypadKeyRectCompact()
							.macTermToolTipText("Click this to show the “Control Keys” palette, then choose a new key from the palette to bind the setting.")
						if viewModel.isKeypadBindingToSuspendKey {
							if #available(macOS 11.0, *) {
								Image(systemName: "keyboard")
							} else {
								Text("!! ")
							}
						}
					}
					Text("Suspend (Scroll Lock / XOFF)")
				}
				UICommon_Default1OptionLineView("", bindIsDefaultTo: $viewModel.isDefaultResumeKeyMapping, isEditingDefault: viewModel.isEditingDefaultContext) {
					ZStack(
						alignment: .trailing
					) {
						Button(action: {
							if viewModel.isKeypadBindingToResumeKey {
								viewModel.isKeypadBindingToResumeKey = false
							} else {
								viewModel.clearKeys()
								viewModel.runner.selectNewResumeKey(viewModel: viewModel)
							}
						}) {
							localizedLabelView(viewModel.resumeKeyMapping)
						}.asMacTermKeypadKeyRectCompact()
							.macTermToolTipText("Click this to show the “Control Keys” palette, then choose a new key from the palette to bind the setting.")
						if viewModel.isKeypadBindingToResumeKey {
							if #available(macOS 11.0, *) {
								Image(systemName: "keyboard")
							} else {
								Text("!! ")
							}
						}
					}
					Text("Resume (XON)")
				}
				UICommon_OptionLineView("") {
					if viewModel.isKeypadBindingToInterruptKey {
						Text("Use “Control Keys” to select a key binding for “Interrupt Process”.")
							.controlSize(.small)
							.fixedSize(horizontal: false, vertical: true)
							.lineLimit(2)
							.multilineTextAlignment(.leading)
							.frame(maxWidth: 220)
							.offset(x: 18, y: 0) // try to align with checkbox label
					} else if viewModel.isKeypadBindingToSuspendKey {
						Text("Use “Control Keys” to select a key binding for “Suspend”.")
							.controlSize(.small)
							.fixedSize(horizontal: false, vertical: true)
							.lineLimit(2)
							.multilineTextAlignment(.leading)
							.frame(maxWidth: 220)
							.offset(x: 18, y: 0) // try to align with checkbox label
					} else if viewModel.isKeypadBindingToResumeKey {
						Text("Use “Control Keys” to select a key binding for “Resume”.")
							.controlSize(.small)
							.fixedSize(horizontal: false, vertical: true)
							.lineLimit(2)
							.multilineTextAlignment(.leading)
							.frame(maxWidth: 220)
							.offset(x: 18, y: 0) // try to align with checkbox label
					} else {
						Text("First click a button above, then choose a replacement key from the palette.")
							.controlSize(.small)
							.fixedSize(horizontal: false, vertical: true)
							.lineLimit(2)
							.multilineTextAlignment(.leading)
							.frame(maxWidth: 220)
							.offset(x: 18, y: 0) // try to align with checkbox label
					}
				}
			}
			UICommon_DividerSectionView()
			//Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_Default1OptionLineView("Emacs Cursor", bindIsDefaultTo: $viewModel.isDefaultEmacsCursorArrows, isEditingDefault: viewModel.isEditingDefaultContext) {
					Toggle("Map to arrow keys", isOn: $viewModel.arrowKeysMapToEmacs)
						.macTermToolTipText("Set if arrow keys send equivalent cursor-movement commands as defined by the Emacs text editor.")
				}
				UICommon_Default1OptionLineView("Emacs Meta", bindIsDefaultTo: $viewModel.isDefaultEmacsMetaMapping, isEditingDefault: viewModel.isEditingDefaultContext) {
					Picker("", selection: $viewModel.selectedMetaMapping) {
						localizedLabelView(.off)
						localizedLabelView(.option)
						localizedLabelView(.shiftOption)
					}.pickerStyle(SegmentedPickerStyle())
						.frame(maxWidth: 200)
						.offset(x: -8, y: 0) // TEMPORARY; to eliminate left-padding created by Picker for empty label
						.macTermToolTipText("Whether or not to send a “meta” sequence when the specified key(s) are pressed.  Used for commands in the Emacs text editor.")
				}
				UICommon_Default1OptionLineView("Delete", bindIsDefaultTo: $viewModel.isDefaultDeleteMapping, isEditingDefault: viewModel.isEditingDefaultContext) {
					Toggle("Map to backspace", isOn: $viewModel.deleteSendsBackspace)
						.macTermToolTipText("Set if the keyboard Delete key sends a “backspace” character instead of a “delete” character.")
				}
				UICommon_Default1OptionLineView("New Line", bindIsDefaultTo: $viewModel.isDefaultNewlineMapping, isEditingDefault: viewModel.isEditingDefaultContext,
												disableDefaultAlignmentGuide: true) {
					Picker("", selection: $viewModel.selectedNewlineMapping) {
						localizedLabelView(.mapLF)
						localizedLabelView(.mapCR)
						localizedLabelView(.mapCRLF)
						localizedLabelView(.mapCRNull)
					}.pickerStyle(PopUpButtonPickerStyle())
						.offset(x: -8, y: 0) // TEMPORARY; to eliminate left-padding created by Picker for empty label
						.frame(minWidth: 160, maxWidth: 160)
						.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.top] + 8 }) // TEMPORARY; try to find a nicer way to do this (top-align both)
						.macTermToolTipText("How to terminate lines in text (for example, Unix-like systems typically expect a LF mapping).")
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsSessionKeyboard_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsSessionKeyboard_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsSessionKeyboard_View().environmentObject(data)))
	}

}

public struct UIPrefsSessionKeyboard_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsSessionKeyboard_Model(runner: UIPrefsSessionKeyboard_RunnerDummy())
		return VStack {
			UIPrefsSessionKeyboard_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsSessionKeyboard_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
