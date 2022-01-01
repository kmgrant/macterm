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

import CoreFoundation
import SwiftUI

//
// IMPORTANT: Many "public" entities below are required
// in order to interact with Swift playgrounds.
//

@objc public protocol UIPrefsTranslations_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func setBackupFont(viewModel: UIPrefsTranslations_Model?) // caller should set "viewModel.isFontPanelBindingToBackupFont" while bound
}

class UIPrefsTranslations_RunnerDummy : NSObject, UIPrefsTranslations_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func setBackupFont(viewModel: UIPrefsTranslations_Model?) { print(#function); if let definedModel = viewModel { definedModel.isFontPanelBindingToBackupFont = true } }
}

public class UIPrefsTranslations_ItemModel : NSObject, Identifiable, ObservableObject {

	public typealias Identifier = CFStringEncoding

	@objc public var id: Identifier
	var name: String {
		get {
			return CFStringGetNameOfEncoding(id) as String
		}
	}

	@objc public init(_ encoding: Identifier) {
		self.id = encoding
	}

}

public class UIPrefsTranslations_Model : UICommon_BaseModel, ObservableObject {

	@Published @objc public var isFontPanelBindingToBackupFont = false
	@Published @objc public var translationTableArray: [UIPrefsTranslations_ItemModel] = []
	@Published @objc public var selectedEncoding: UIPrefsTranslations_ItemModel? {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var backupFontName: String? {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var useBackupFont = false {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	public var runner: UIPrefsTranslations_ActionHandling

	@objc public init(runner: UIPrefsTranslations_ActionHandling) {
		self.runner = runner
	}

}

public struct UIPrefsTranslations_ItemView : View {

	@EnvironmentObject private var itemModel: UIPrefsTranslations_ItemModel

	public var body: some View {
		Text(itemModel.name)
	}

}

public struct UIPrefsTranslations_View : View {

	@EnvironmentObject private var viewModel: UIPrefsTranslations_Model

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			Spacer().asMacTermSectionSpacingV().fixedSize()
			ZStack(
				alignment: .top
			) {
				UICommon_OptionLineView("Characters", disableDefaultAlignmentGuide: true, noDefaultSpacing: true) {
					List(selection: $viewModel.selectedEncoding) {
						ForEach(viewModel.translationTableArray, id: \.self) { item in
							UIPrefsTranslations_ItemView().environmentObject(item)
						}
					}.macTermSectionAlignmentGuideList()
				}
				HStack {
					let encodingName = viewModel.selectedEncoding?.name
					VStack {
						Text(encodingName ?? "")
							.controlSize(.small)
							.frame(maxWidth: 156, alignment: .trailing)
							.padding([.top], 36)
							.padding([.leading], 20)
						Spacer()
					}
					Spacer()
				}
			}.frame(minWidth: 200, minHeight: 200, idealHeight: 200)
			Spacer().asMacTermSectionSpacingV().fixedSize()
			UICommon_OptionLineView("Options", noDefaultSpacing: true) {
				Toggle("Use a backup font:", isOn: $viewModel.useBackupFont)
					.macTermToolTipText("Set if the font used by a Format may not contain all required glyphs for the target language.")
			}
			UICommon_OptionLineView("", noDefaultSpacing: true) {
				HStack {
					HStack(
						alignment: .firstTextBaseline
					) {
						Button(action: {
							if viewModel.isFontPanelBindingToBackupFont {
								viewModel.isFontPanelBindingToBackupFont = false
								viewModel.runner.setBackupFont(viewModel: nil)
							} else {
								viewModel.isFontPanelBindingToBackupFont = false // initially...
								viewModel.runner.setBackupFont(viewModel: viewModel)
							}
						}) {
							Text("Set…")
						}.asMacTermKeypadKeyCompactTitleExactWidth(66, selected: viewModel.isFontPanelBindingToBackupFont)
							.disabled(!viewModel.useBackupFont)
							.macTermToolTipText("Specify font to use as a fallback for any glyphs not found in the terminal’s ordinary font.")
						Text(viewModel.backupFontName ?? "")
							.font(Font(NSFont(name: viewModel.backupFontName ?? "", size: 12.0) ?? NSFont.monospacedSystemFont(ofSize: 12.0, weight: .regular)))
							.disabled(!viewModel.useBackupFont)
							.foregroundColor(!viewModel.useBackupFont ? .gray : .primary) // default behavior of disabled() does not dim the text so do that manually
					}
				}.withMacTermSectionLayout()
			}
			UICommon_OptionLineView("", noDefaultSpacing: true) {
				HStack {
					Text("A backup font is used for characters that are not available in the window’s Format font.")
						.controlSize(.small)
						.fixedSize(horizontal: false, vertical: true)
						.lineLimit(5)
						.multilineTextAlignment(.leading)
						.frame(maxWidth: 500)
						.padding([.top], 4)
				}.withMacTermSectionLayout()
			}
			Spacer().asMacTermSectionSpacingV().fixedSize()
			//Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsTranslations_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsTranslations_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsTranslations_View().environmentObject(data)))
	}

}

public struct UIPrefsTranslations_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsTranslations_Model(runner: UIPrefsTranslations_RunnerDummy())
		data.translationTableArray.append(UIPrefsTranslations_ItemModel(CFStringBuiltInEncodings.macRoman.rawValue))
		data.translationTableArray.append(UIPrefsTranslations_ItemModel(CFStringBuiltInEncodings.UTF8.rawValue))
		data.translationTableArray.append(UIPrefsTranslations_ItemModel(CFStringBuiltInEncodings.UTF32.rawValue))
		return VStack {
			UIPrefsTranslations_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsTranslations_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
