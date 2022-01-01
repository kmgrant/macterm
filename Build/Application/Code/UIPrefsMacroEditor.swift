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

@objc public protocol UIPrefsMacroEditor_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func getSelectedMacroOrdinaryKey() -> String
	func resetToDefaultGetSelectedMacroKeyBinding() -> MacroManager_KeyBinding
	func resetToDefaultGetSelectedModifiers() -> MacroManager_ModifierKeyMask
	func resetToDefaultGetSelectedAction() -> MacroManager_Action
	func resetToDefaultGetSelectedMacroContent() -> String
	func selectControlKeyToInsertBacklashSequence(viewModel: UIPrefsMacroEditor_Model) // caller should set "viewModel.isKeypadBindingToMacroContentInsertion" while bound
}

class UIPrefsMacroEditor_RunnerDummy : NSObject, UIPrefsMacroEditor_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func getSelectedMacroOrdinaryKey() -> String { print(#function); return "" }
	func resetToDefaultGetSelectedMacroKeyBinding() -> MacroManager_KeyBinding { print(#function); return .ordinaryCharacter }
	func resetToDefaultGetSelectedModifiers() -> MacroManager_ModifierKeyMask { print(#function); return [] }
	func resetToDefaultGetSelectedAction() -> MacroManager_Action { print(#function); return .sendTextProcessingEscapes }
	func resetToDefaultGetSelectedMacroContent() -> String { print(#function); return "" }
	func selectControlKeyToInsertBacklashSequence(viewModel: UIPrefsMacroEditor_Model) { print(#function); print(#function); viewModel.isKeypadBindingToMacroContentInsertion = true }
}

public class UIPrefsMacroEditor_Model : UICommon_DefaultingModel, ObservableObject {

	@Published @objc public var isKeypadBindingToMacroContentInsertion = false
	@Published @objc public var isDefaultMacroKey = true {
		willSet(isOn) {
			if isOn {
				ifUserRequestedDefault {
					selectedKeyBinding = runner.resetToDefaultGetSelectedMacroKeyBinding()
					macroOrdinaryKey = runner.getSelectedMacroOrdinaryKey() // note: shares underlying preference with key binding (reset by call above)
					macroModifiers = runner.resetToDefaultGetSelectedModifiers()
				}
			}
		}
	}
	@Published @objc public var isDefaultMacroAction = true {
		willSet(isOn) {
			if isOn {
				ifUserRequestedDefault {
					selectedAction = runner.resetToDefaultGetSelectedAction()
					macroContent = runner.resetToDefaultGetSelectedMacroContent()
				}
			}
		}
	}
	@Published @objc public var macroContent = "" {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultMacroAction = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var macroOrdinaryKey = "" {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultMacroKey = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var macroModifiers: MacroManager_ModifierKeyMask = [] {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultMacroKey = false }
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var selectedAction : MacroManager_Action = .sendTextProcessingEscapes {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultMacroAction = false }
				if selectedActionIsVerbatim() {
					isKeypadBindingToMacroContentInsertion = false
				}
				runner.dataUpdated()
			}
		}
	}
	@Published @objc public var selectedKeyBinding: MacroManager_KeyBinding = .ordinaryCharacter {
		didSet {
			ifWritebackEnabled {
				inNonDefaultContext { isDefaultMacroKey = false }
				runner.dataUpdated()
			}
		}
	}
	public var runner: UIPrefsMacroEditor_ActionHandling

	@objc public init(runner: UIPrefsMacroEditor_ActionHandling) {
		self.runner = runner
	}

	func selectedActionIsVerbatim() -> Bool {
		// only valid for actions that can process backslash escape sequences
		return ((self.selectedAction != .findTextProcessingEscapes) &&
				(self.selectedAction != .sendTextProcessingEscapes))
	}

	// MARK: UICommon_DefaultingModel

	override func setDefaultFlagsToTrue() {
		// unconditional; used by base when swapping to "isEditingDefaultContext"
		isDefaultMacroKey = true
		isDefaultMacroAction = true
	}

}

public struct UIPrefsMacroEditor_View : View {

	@EnvironmentObject private var viewModel: UIPrefsMacroEditor_Model

	func localizedHelpString(_ forType: MacroManager_Action) -> String {
		// see also localizedLabelView(...)
		var result: String = ""
		switch forType {
		case .findTextProcessingEscapes:
			result = "Initiates a search in the current terminal window using the text in the field but any backslash (\\) sequences are expanded first."
		case .findTextVerbatim:
			result = "Initiates a search in the current terminal window using the text in the field.  Backslash (\\) sequences are NOT replaced."
		case .handleURL:
			result = "Opens the URL entered in the field (like a web site or E-mail link)."
		case .newWindowWithCommand:
			result = "Opens a new terminal window, running the command line entered in the field."
		case .selectMatchingWindow:
			result = "Activates the next terminal window whose title partially matches the text in the field.  If used repeatedly, cycles through matching windows."
		case .sendTextProcessingEscapes:
			result = "Sends the text in the field to the active session but any backslash (\\) sequences are expanded first."
		case .sendTextVerbatim:
			result = "Sends the text in the field to the active session.  Backslash (\\) sequences are NOT replaced."
		}
		return result
	}

	func localizedLabelView(_ forType: UIKeypads_KeyID) -> some View {
		return UIKeypads_ControlKeysView.localizedLabelView(forType)
	}

	func localizedLabelView(_ forType: MacroManager_KeyBinding) -> some View {
		var anIconSystemName: String? = nil
		var aTitle: String = ""
		switch forType {
		case .ordinaryCharacter:
			aTitle = "Ordinary Character"
		case .backwardDelete:
			anIconSystemName = "delete.left"
			aTitle = "Backward Delete"
		case .forwardDelete:
			anIconSystemName = "delete.right"
			aTitle = "Forward Delete"
		case .home:
			anIconSystemName = "arrow.up.to.line.alt"
			aTitle = "Home"
		case .end:
			anIconSystemName = "arrow.down.to.line.alt"
			aTitle = "End"
		case .pageUp:
			aTitle = " ⇞   Page Up"
		case .pageDown:
			aTitle = " ⇟   Page Down"
		case .upArrow:
			anIconSystemName = "arrow.up"
			aTitle = "Up Arrow"
		case .downArrow:
			anIconSystemName = "arrow.down"
			aTitle = "Down Arrow"
		case .leftArrow:
			anIconSystemName = "arrow.left"
			aTitle = "Left Arrow"
		case .rightArrow:
			anIconSystemName = "arrow.right"
			aTitle = "Right Arrow"
		case .clear:
			anIconSystemName = "clear"
			aTitle = "Clear"
		case .escape:
			anIconSystemName = "escape"
			aTitle = "Escape"
		case .return:
			anIconSystemName = "return"
			aTitle = "Return"
		case .enter:
			anIconSystemName = "projective"
			aTitle = "Enter"
		case .functionKeyF1:
			aTitle = " F1  Function Key"
		case .functionKeyF2:
			aTitle = " F2  Function Key"
		case .functionKeyF3:
			aTitle = " F3  Function Key"
		case .functionKeyF4:
			aTitle = " F4  Function Key"
		case .functionKeyF5:
			aTitle = " F5  Function Key"
		case .functionKeyF6:
			aTitle = " F6  Function Key"
		case .functionKeyF7:
			aTitle = " F7  Function Key"
		case .functionKeyF8:
			aTitle = " F8  Function Key"
		case .functionKeyF9:
			aTitle = " F9  Function Key"
		case .functionKeyF10:
			aTitle = " F10  Function Key"
		case .functionKeyF11:
			aTitle = " F11  Function Key"
		case .functionKeyF12:
			aTitle = " F12  Function Key"
		case .functionKeyF13:
			aTitle = " F13  Function Key"
		case .functionKeyF14:
			aTitle = " F14  Function Key"
		case .functionKeyF15:
			aTitle = " F15  Function Key"
		case .functionKeyF16:
			aTitle = " F16  Function Key"
		}
		return HStack {
			if #available(macOS 11.0, *) {
				if let definedSymbolName = anIconSystemName {
					Image(systemName: definedSymbolName)
				}
			}
			Text(aTitle)
		}.tag(forType)
	}

	func localizedLabelView(_ forType: MacroManager_ModifierKeyMask) -> some View {
		var anIconSystemName: String? = nil
		var aTitle: String = ""
		if forType.contains(.command) {
			anIconSystemName = "command"
			aTitle = "⌘"
		} else if forType.contains(.control) {
			anIconSystemName = "control"
			aTitle = "⌃"
		} else if forType.contains(.option) {
			anIconSystemName = "option"
			aTitle = "⌥"
		} else if forType.contains(.shift) {
			anIconSystemName = "shift"
			aTitle = "⇧"
		}
		return HStack {
			if #available(macOS 11.0, *) {
				if let definedSymbolName = anIconSystemName {
					Image(systemName: definedSymbolName)
				} else {
					Text(aTitle)
				}
			} else {
				Text(aTitle)
			}
		}.tag(Int(forType.rawValue))
	}

	func localizedLabelView(_ forType: MacroManager_Action) -> some View {
		// see also localizedHelpString(...)
		var aTitle: String = ""
		switch forType {
		case .findTextProcessingEscapes:
			aTitle = "Find in Local Terminal with Substitutions"
		case .findTextVerbatim:
			aTitle = "Find in Local Terminal Verbatim"
		case .handleURL:
			aTitle = "Open URL"
		case .newWindowWithCommand:
			aTitle = "New Window with Command"
		case .selectMatchingWindow:
			aTitle = "Select Window by Title"
		case .sendTextProcessingEscapes:
			aTitle = "Enter Text with Substitutions"
		case .sendTextVerbatim:
			aTitle = "Enter Text Verbatim"
		}
		return Text(aTitle).tag(forType)
	}

	func isInsertControlKeyCharacterDisabled() -> Bool {
		// only valid for actions that can process backslash escape sequences
		return viewModel.selectedActionIsVerbatim()
	}

	func isOrdinaryKeyDisabled() -> Bool {
		return (viewModel.selectedKeyBinding != .ordinaryCharacter)
	}

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			UICommon_DefaultOptionHeaderView()
			Group {
				UICommon_Default1OptionLineView("Invoke With",
												toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Macro Key Sequence",
																								bindIsDefaultTo: $viewModel.isDefaultMacroKey),
												isEditingDefault: viewModel.isEditingDefaultContext) {
					HStack {
						Picker("", selection: $viewModel.selectedKeyBinding) {
							localizedLabelView(.ordinaryCharacter)
							Group {
								localizedLabelView(.backwardDelete)
								localizedLabelView(.forwardDelete)
								localizedLabelView(.home)
								localizedLabelView(.end)
								localizedLabelView(.pageUp)
								localizedLabelView(.pageDown)
							}
							Group {
								localizedLabelView(.upArrow)
								localizedLabelView(.downArrow)
								localizedLabelView(.leftArrow)
								localizedLabelView(.rightArrow)
							}
							Group {
								localizedLabelView(.clear)
								localizedLabelView(.escape)
								localizedLabelView(.return)
								localizedLabelView(.enter)
							}
							Group {
								localizedLabelView(.functionKeyF1)
								localizedLabelView(.functionKeyF2)
								localizedLabelView(.functionKeyF3)
								localizedLabelView(.functionKeyF4)
							}
							Group {
								localizedLabelView(.functionKeyF5)
								localizedLabelView(.functionKeyF6)
								localizedLabelView(.functionKeyF7)
								localizedLabelView(.functionKeyF8)
							}
							Group {
								localizedLabelView(.functionKeyF9)
								localizedLabelView(.functionKeyF10)
								localizedLabelView(.functionKeyF11)
								localizedLabelView(.functionKeyF12)
							}
							Group {
								localizedLabelView(.functionKeyF13)
								localizedLabelView(.functionKeyF14)
								localizedLabelView(.functionKeyF15)
								localizedLabelView(.functionKeyF16)
							}
						}.pickerStyle(PopUpButtonPickerStyle())
							.macTermOffsetForEmptyPickerTitle()
							.frame(minWidth: 216, maxWidth: 216, idealHeight: 32)
							.macTermToolTipText("The type of key binding to use for this macro, along with any Modifiers enabled below.  If “Ordinary Key” is selected, use the text field next to this menu to indicate the key to use.  Note that capital letters imply a Shift modifier so usually lowercase letters should be entered.")
						TextField("", text: $viewModel.macroOrdinaryKey)
							.disabled(isOrdinaryKeyDisabled())
							.foregroundColor(isOrdinaryKeyDisabled() ? .gray : .primary) // default behavior of disabled() does not dim the text so do that manually
							.frame(minWidth: 48, maxWidth: 48)
							.macTermToolTipText("A regular key character to add to the macro key sequence, such as a letter; alternately, select a special key from the menu.  Note that capital letters imply a Shift modifier so usually lowercase letters should be entered.")
					}
				}
				UICommon_OptionLineView("Modifiers") {
					HStack {
						Button(action: {
							if viewModel.macroModifiers.contains(.command) {
								viewModel.macroModifiers.remove(.command)
							} else {
								viewModel.macroModifiers.insert(.command)
							}
						}) {
							localizedLabelView(.command)
						}.asMacTermKeypadKeyRectCompact(selected: viewModel.macroModifiers.contains(.command))
							.macTermToolTipText("Click this to specify whether or not the Command key (⌘) is part of the key sequence for this macro.")
						Button(action: {
							if viewModel.macroModifiers.contains(.option) {
								viewModel.macroModifiers.remove(.option)
							} else {
								viewModel.macroModifiers.insert(.option)
							}
						}) {
							localizedLabelView(.option)
						}.asMacTermKeypadKeyRectCompact(selected: viewModel.macroModifiers.contains(.option))
							.macTermToolTipText("Click this to specify whether or not the Option key (⌥) is part of the key sequence for this macro.")
						Button(action: {
							if viewModel.macroModifiers.contains(.control) {
								viewModel.macroModifiers.remove(.control)
							} else {
								viewModel.macroModifiers.insert(.control)
							}
						}) {
							localizedLabelView(.control)
						}.asMacTermKeypadKeyRectCompact(selected: viewModel.macroModifiers.contains(.control))
							.macTermToolTipText("Click this to specify whether or not the Control key (⌃) is part of the key sequence for this macro.")
						Button(action: {
							if viewModel.macroModifiers.contains(.shift) {
								viewModel.macroModifiers.remove(.shift)
							} else {
								viewModel.macroModifiers.insert(.shift)
							}
						}) {
							localizedLabelView(.shift)
						}.asMacTermKeypadKeyRectCompact(selected: viewModel.macroModifiers.contains(.shift))
							.macTermToolTipText("Click this to specify whether or not the Shift key (⇧) is part of the key sequence for this macro.  Note that a Shift key may be implied if you enter a capital letter in the field above.")
						Spacer().layoutPriority(1)
					}
				}
				UICommon_OptionLineView() {
					Text("Not all key combinations will be available to macros (for example, if you have short-cuts defined in System Preferences).")
						.controlSize(.small)
						.fixedSize(horizontal: false, vertical: true)
						.lineLimit(3)
						.multilineTextAlignment(.leading)
						.frame(minWidth: 400, maxWidth: 800, alignment: .leading)
				}
				UICommon_Default1OptionLineView("Action",
												toggleConfig: UICommon_DefaultToggleProperties(accessibilityPrefName: "Macro Action",
																								bindIsDefaultTo: $viewModel.isDefaultMacroAction),
												isEditingDefault: viewModel.isEditingDefaultContext) {
					Picker("", selection: $viewModel.selectedAction) {
						localizedLabelView(.sendTextVerbatim)
						localizedLabelView(.sendTextProcessingEscapes)
						localizedLabelView(.handleURL)
						localizedLabelView(.newWindowWithCommand)
						localizedLabelView(.selectMatchingWindow)
						localizedLabelView(.findTextVerbatim)
						localizedLabelView(.findTextProcessingEscapes)
					}.pickerStyle(PopUpButtonPickerStyle())
						.macTermOffsetForEmptyPickerTitle()
						.frame(minWidth: 288, maxWidth: 288)
						.macTermToolTipText("The operation to perform when the macro is invoked (via keyboard, menu or toolbar); the content below is the main input to the action.")
				}
				UICommon_OptionLineView(localizedHelpString(viewModel.selectedAction), isSmallMultilineTitle: true) {
					VStack(
						alignment: .leading
					) {
						ZStack {
							ZStack(
								alignment: .trailing
							) {
								TextField("", text: $viewModel.macroContent)
									.fixedSize(horizontal: false, vertical: true)
									.multilineTextAlignment(.leading)
									.frame(minWidth: 400, maxWidth: .infinity)
									.macTermToolTipText("The content of the macro; the exact interpretation of the content depends on the Action that is selected.  If multiple lines are necessary in a “with Substitutions” style of macro, add “\\n”; in a verbatim macro, hold down the Option key and press the Return key to insert a new line.")
								if viewModel.isKeypadBindingToMacroContentInsertion {
									if #available(macOS 11.0, *) {
										Image(systemName: "keyboard")
									} else {
										Text("!! ")
									}
								}
							}
							if viewModel.isKeypadBindingToMacroContentInsertion {
								RoundedRectangle(cornerRadius: 3).stroke()
							}
						}.padding([.vertical], 10)
						ZStack(
							alignment: .trailing
						) {
							Button(action: {
								if viewModel.isKeypadBindingToMacroContentInsertion {
									viewModel.isKeypadBindingToMacroContentInsertion = false
								} else {
									viewModel.isKeypadBindingToMacroContentInsertion = false // initially...
									viewModel.runner.selectControlKeyToInsertBacklashSequence(viewModel: viewModel)
								}
							}) {
								Text("Insert Control Key Character…")
							}.asMacTermKeypadKeyCompactTitleExactWidth(220, selected: viewModel.isKeypadBindingToMacroContentInsertion)
								.disabled(isInsertControlKeyCharacterDisabled())
								.macTermToolTipText("Binds the “Control Keys” palette to the macro field above.  While bound, you can click a control key button to easily insert the equivalent backslash sequence; for example, clicking “⌃D” in the palette would enter the code “\\004”.")
							if viewModel.isKeypadBindingToMacroContentInsertion {
								if #available(macOS 11.0, *) {
									Image(systemName: "keyboard")
								} else {
									Text("!! ")
								}
							}
						}
					}
				}
			}
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsMacroEditor_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsMacroEditor_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsMacroEditor_View().environmentObject(data)))
	}

}

public struct UIPrefsMacroEditor_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsMacroEditor_Model(runner: UIPrefsMacroEditor_RunnerDummy())
		return VStack {
			UIPrefsMacroEditor_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsMacroEditor_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
