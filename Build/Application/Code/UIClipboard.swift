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

@objc public protocol UIClipboard_ActionHandling : NSObjectProtocol {
	// (nothing yet)
}

class UIClipboard_RunnerDummy : NSObject, UIClipboard_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	// (nothing yet)
}

public class UIClipboard_Model : NSObject, ObservableObject {

	public var runner: UIClipboard_ActionHandling
	@Published @objc public var clipboardUnknown: Bool = false {
		didSet {
			if clipboardUnknown {
				clipboardText = ""
				clipboardImage = nil
			}
		}
	}
	@Published @objc public var clipboardText: String = "" {
		didSet {
			if !clipboardText.isEmpty {
				clipboardUnknown = false
			}
		}
	}
	@Published @objc public var clipboardImage: NSImage? = nil {
		didSet {
			if nil != clipboardImage {
				clipboardUnknown = false
			}
		}
	}
	@Published @objc public var kindValue: String = ""
	@Published @objc public var sizeValue: String = ""
	@Published @objc public var extraInfoLabel1: String? = nil
	@Published @objc public var extraInfoValue1: String? = nil
	@Published @objc public var extraInfoLabel2: String? = nil
	@Published @objc public var extraInfoValue2: String? = nil

	@objc public init(runner: UIClipboard_ActionHandling) {
		self.runner = runner
	}

}

public struct UIClipboard_View : View {

	@EnvironmentObject private var viewModel: UIClipboard_Model

	public var body: some View {
		HSplitView {
			VStack(
				alignment: .leading
			) {
				HStack(
					alignment: .firstTextBaseline
				) {
					Text("Data Info")
						.font(.headline)
						.fixedSize()
					VStack { Divider().padding(.leading).disabled(true) }
				}
				UICommon_OptionLineView("Kind", isSmallHeading: true, noDefaultSpacing: true) {
					Text(viewModel.kindValue)
						.controlSize(.small)
				}
				UICommon_OptionLineView("Size", isSmallHeading: true, noDefaultSpacing: true) {
					Text(viewModel.sizeValue)
						.controlSize(.small) 
				}
				if let extraInfoLabel1 = viewModel.extraInfoLabel1,
					let extraInfoValue1 = viewModel.extraInfoValue1 { 
					UICommon_OptionLineView(extraInfoLabel1, isSmallHeading: true, noDefaultSpacing: true) {
						Text(extraInfoValue1)
							.controlSize(.small)
					}
				}
				if let extraInfoLabel2 = viewModel.extraInfoLabel2,
					let extraInfoValue2 = viewModel.extraInfoValue2 { 
					UICommon_OptionLineView(extraInfoLabel2, isSmallHeading: true, noDefaultSpacing: true) {
						Text(extraInfoValue2)
							.controlSize(.small)
					}
				}
				Spacer().layoutPriority(1)
			}.frame(minWidth: 200, idealWidth: 250, maxWidth: 300)
				.padding(20)
			ZStack {
				if viewModel.clipboardUnknown {
					VStack(
						alignment: .leading
					) {
						Text("(The clipboard contains unknown data.)")
							.padding(20)
						Spacer().layoutPriority(1)
					}
				} else if let definedImage = viewModel.clipboardImage {
					ScrollView(
						[.horizontal, .vertical],
						showsIndicators: true
					) {
						ZStack {
							Rectangle()
								.shadow(color: .secondary, radius: 5, x: 4.0, y: 3.0)
							Image(nsImage: definedImage)
								//.onDrag({ return NSItemProvider(object: definedImage) })
						}
					}
				} else if !viewModel.clipboardText.isEmpty {
					ScrollView(
						[.horizontal, .vertical],
						showsIndicators: true
					) {
						VStack(
							alignment: .leading
						) {
							Text(viewModel.clipboardText)
								.font(Font.system(.body, design: .monospaced))
								.lineLimit(nil)
								.multilineTextAlignment(/*@START_MENU_TOKEN@*/.leading/*@END_MENU_TOKEN@*/)
								.onDrag({ return NSItemProvider(object: viewModel.clipboardText as NSString) })
								/*.onDrop(of: [kUTTypeText], isTargeted: nil) { providers in
									// UNIMPLEMENTED: copy dropped text to the Clipboard
									return false
								}*/
						}.frame(maxWidth: .infinity)
					}
				} else {
					VStack(
						alignment: .leading
					) {
						Text("(The clipboard is empty.)")
							.padding(20)
						Spacer().layoutPriority(1)
					}
				}
			}.frame(minWidth: 350, alignment: .leading)
				.padding(0)
		}.padding(0)
	}

}

public class UIClipboard_ObjC : NSObject {

	@objc public static func makeViewController(_ data: UIClipboard_Model) -> NSViewController {
		return NSHostingController<AnyView>(rootView: AnyView(UIClipboard_View().environmentObject(data)))
	}

}

public struct UIClipboard_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIClipboard_Model(runner: UIClipboard_RunnerDummy())
		return VStack {
			UIClipboard_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIClipboard_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
