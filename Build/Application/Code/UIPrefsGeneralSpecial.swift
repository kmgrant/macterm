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

@objc public enum UIPrefsGeneralSpecial_WindowResizeEffect : Int {
	case terminalScreenSize
	case textSize
}

@objc public enum UIPrefsGeneralSpecial_CommandNBindingType : Int {
	case bindDefaultSession
	case bindShell
	case bindLogInShell
	case bindCustomNewSession
}

@objc public protocol UIPrefsGeneralSpecial_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func setWindowStackingOrigin()
}

class UIPrefsGeneralSpecial_RunnerDummy : NSObject, UIPrefsGeneralSpecial_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func setWindowStackingOrigin() { print(#function) }
}

public class UIPrefsGeneralSpecial_Model : UICommon_BaseModel, ObservableObject {

	@Published @objc public var selectedCursorShape: Terminal_CursorType = .verticalLine {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var cursorFlashEnabled = false {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var selectedWindowResizeEffect: UIPrefsGeneralSpecial_WindowResizeEffect = .terminalScreenSize {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var spacesPerTabValue = 4 {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var selectedCommandNBindingType: UIPrefsGeneralSpecial_CommandNBindingType = .bindDefaultSession {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	public var runner: UIPrefsGeneralSpecial_ActionHandling

	@objc public init(runner: UIPrefsGeneralSpecial_ActionHandling) {
		self.runner = runner
	}

}

public struct UIPrefsGeneralSpecial_View : View {

	@EnvironmentObject private var viewModel: UIPrefsGeneralSpecial_Model
	private var tabSizeFormatter = NumberFormatter()

	func localizedLabelView(_ forType: Terminal_CursorType) -> some View {
		var aTitle: String = ""
		switch forType {
		case .block:
			aTitle = "▊"
		case .verticalLine:
			aTitle = "│"
		case .underscore:
			aTitle = "▁"
		case .thickVerticalLine:
			aTitle = "┃"
		case .thickUnderscore:
			aTitle = "▂"
		}
		return Text(aTitle).tag(forType)
	}

	func localizedLabelView(_ forType: UIPrefsGeneralSpecial_WindowResizeEffect) -> some View {
		var aTitle: String = ""
		switch forType {
		case .terminalScreenSize:
			aTitle = "Terminal screen size"
		case .textSize:
			aTitle = "Text size"
		}
		return Text(aTitle).tag(forType)
	}

	func localizedLabelView(_ forType: UIPrefsGeneralSpecial_CommandNBindingType) -> some View {
		var aTitle: String = ""
		switch forType {
		case .bindDefaultSession:
			aTitle = "Default Session Favorite"
		case .bindShell:
			aTitle = "Shell"
		case .bindLogInShell:
			aTitle = "Log-In Shell"
		case .bindCustomNewSession:
			aTitle = "Custom New Session"
		}
		return Text(aTitle).tag(forType)
	}

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			Spacer().asMacTermSectionSpacingV()
			UICommon_OptionLineView("Terminal Cursor", noDefaultSpacing: true) {
				HStack {
					Picker("", selection: $viewModel.selectedCursorShape) {
						localizedLabelView(.block)
						localizedLabelView(.verticalLine)
						localizedLabelView(.underscore)
						localizedLabelView(.thickVerticalLine)
						localizedLabelView(.thickUnderscore)
					}.pickerStyle(SegmentedPickerStyle())
						.frame(maxWidth: 200)
						.offset(x: -8, y: 0) // TEMPORARY; to eliminate left-padding created by Picker for empty label
						.macTermToolTipText("Select terminal cursor shape (exact size depends on terminal screen font).")
					Toggle("Flash", isOn: $viewModel.cursorFlashEnabled)
						.fixedSize()
						.macTermToolTipText("Set if cursor should animate between on and off states.")
				}
			}
			Spacer().asMacTermMinorSectionSpacingV()
			Group {
				UICommon_OptionLineView("Window Stacking Origin", noDefaultSpacing: true) {
					Button(action: { viewModel.runner.setWindowStackingOrigin() }) {
						Text("Set…")
							.frame(minWidth: 50)
							.macTermToolTipText("Specify screen location where top-left corner of first new window should be anchored.")
					}
				}
				Spacer().asMacTermMinorSectionSpacingV()
				Group {
					UICommon_OptionLineView("Window Resize Effect", disableDefaultAlignmentGuide: true, noDefaultSpacing: true) {
						Picker("", selection: $viewModel.selectedWindowResizeEffect) {
							localizedLabelView(.terminalScreenSize)
							localizedLabelView(.textSize)
						}.pickerStyle(RadioGroupPickerStyle())
							.offset(x: -8, y: 0) // TEMPORARY; to eliminate left-padding created by Picker for empty label
							.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.top] + 8 }) // TEMPORARY; try to find a nicer way to do this (top-align both)
							.macTermToolTipText("Select whether resizing a terminal window will adjust the font size (with same columns and rows), or adjust columns and rows (with same font size).")
					}
					UICommon_OptionLineView("", noDefaultSpacing: true) {
						HStack {
							Text("Control key flips this for one window resize (or Option key with “Enter Full Screen”).")
								.controlSize(.small)
								.fixedSize(horizontal: false, vertical: true)
								.lineLimit(3)
								.multilineTextAlignment(.leading)
								.frame(maxWidth: 350)
						}.withMacTermSectionLayout()
					}
				}
			}
			Spacer().asMacTermMinorSectionSpacingV()
			UICommon_OptionLineView("Tab Conversion", noDefaultSpacing: true) {
				HStack(
					alignment: .firstTextBaseline
				) {
					TextField("", value: $viewModel.spacesPerTabValue, formatter: tabSizeFormatter)
						.frame(maxWidth: 40)
						.macTermToolTipText("When text is copied by “Copy with Tab Substitution”, this determines how many consecutive spaces are equal to one Tab character.")
					Text("spaces per tab (for “Copy with Tab Substitution”)")
				}
			}
			Spacer().asMacTermMinorSectionSpacingV()
			UICommon_OptionLineView("⌘N Key Equivalent", disableDefaultAlignmentGuide: true, noDefaultSpacing: true) {
				Picker("", selection: $viewModel.selectedCommandNBindingType) {
					localizedLabelView(.bindDefaultSession)
					localizedLabelView(.bindShell)
					localizedLabelView(.bindLogInShell)
					localizedLabelView(.bindCustomNewSession)
				}.pickerStyle(RadioGroupPickerStyle())
					.offset(x: -8, y: 0) // TEMPORARY; to eliminate left-padding created by Picker for empty label
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.top] + 8 }) // TEMPORARY; try to find a nicer way to do this (top-align both)
					.macTermToolTipText("Action to perform when the ⌘N key equivalent is used (also visible in File menu).")
			}
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsGeneralSpecial_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsGeneralSpecial_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsGeneralSpecial_View().environmentObject(data)))
	}

}

public struct UIPrefsGeneralSpecial_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsGeneralSpecial_Model(runner: UIPrefsGeneralSpecial_RunnerDummy())
		return VStack {
			UIPrefsGeneralSpecial_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsGeneralSpecial_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
