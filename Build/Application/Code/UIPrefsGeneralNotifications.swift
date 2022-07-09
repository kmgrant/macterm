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

@objc public protocol UIPrefsGeneralNotifications_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdated()
	func playSelectedBellSound()
}

class UIPrefsGeneralNotifications_RunnerDummy : NSObject, UIPrefsGeneralNotifications_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdated() { print(#function) }
	func playSelectedBellSound() { print(#function) }
}

public class UIPrefsGeneralNotifications_BellSoundItemModel : NSObject, Identifiable, ObservableObject {

	@objc public var uniqueID = UUID()
	@Published @objc public var soundName: String // sound name or other label
	@Published public var helpText: String // for help tag in pop-up menu
	@objc public static let offItemModel = UIPrefsGeneralNotifications_BellSoundItemModel(soundName: "Off", helpText: "When terminal bell occurs, no sound will be played.")
	@objc public static let defaultItemModel = UIPrefsGeneralNotifications_BellSoundItemModel(soundName: "Default", helpText: "When terminal bell occurs, alert sound will be played (based on system-wide sound settings).")

	@objc public init(soundName: String, helpText: String? = nil) {
		self.soundName = soundName
		if let givenHelpText = helpText {
			self.helpText = givenHelpText
		} else {
			self.helpText = "When terminal bell occurs, sound with this name will be played."
		}
	}

}

public class UIPrefsGeneralNotifications_Model : UICommon_BaseModel, ObservableObject {

	@Published @objc public var bellSoundItems: [UIPrefsGeneralNotifications_BellSoundItemModel] = []
	@Published @objc public var selectedBellSoundID = UIPrefsGeneralNotifications_BellSoundItemModel.offItemModel.uniqueID {
		didSet { ifWritebackEnabled { runner.playSelectedBellSound(); runner.dataUpdated() } }
	}
	@Published @objc public var visualBell = false {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var bellNotificationInBackground = false {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	@Published @objc public var backgroundNotificationAction: AlertMessages_NotificationType = .doNothing {
		didSet { ifWritebackEnabled { runner.dataUpdated() } }
	}
	public var runner: UIPrefsGeneralNotifications_ActionHandling

	@objc public init(runner: UIPrefsGeneralNotifications_ActionHandling) {
		self.runner = runner
	}

}

public struct UIPrefsGeneralNotifications_BellSoundItemView : View {

	@EnvironmentObject private var itemModel: UIPrefsGeneralNotifications_BellSoundItemModel

	public var body: some View {
		Text(itemModel.soundName).tag(itemModel.uniqueID)
			.macTermToolTipText(itemModel.helpText)
	}

}

public struct UIPrefsGeneralNotifications_View : View {

	@EnvironmentObject private var viewModel: UIPrefsGeneralNotifications_Model

	func localizedLabelView(_ forType: AlertMessages_NotificationType) -> some View {
		var aTitle: String = ""
		switch forType {
		case .doNothing:
			aTitle = "No alert"
		case .markDockIcon:
			aTitle = "Modify the Dock icon"
		case .markDockIconAndBounceOnce:
			aTitle = "…and bounce the Dock icon"
		case .markDockIconAndBounceRepeatedly:
			aTitle = "…and bounce repeatedly"
		}
		return Text(aTitle).tag(forType)
	}

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_OptionLineView("Terminal Bell", noDefaultSpacing: true) {
					Picker("", selection: $viewModel.selectedBellSoundID) {
						UIPrefsGeneralNotifications_BellSoundItemView().environmentObject(UIPrefsGeneralNotifications_BellSoundItemModel.offItemModel)
						UIPrefsGeneralNotifications_BellSoundItemView().environmentObject(UIPrefsGeneralNotifications_BellSoundItemModel.defaultItemModel)
						// TBD: how to insert dividing-line in this type of menu?
						//VStack { Divider().padding(.leading).disabled(true) } // this looks like a separator but is still selectable (SwiftUI bug?)
						ForEach(viewModel.bellSoundItems) { item in
							UIPrefsGeneralNotifications_BellSoundItemView().environmentObject(item)
						}
					}.pickerStyle(PopUpButtonPickerStyle())
						.macTermOffsetForEmptyPickerTitle()
						.frame(minWidth: 160, maxWidth: 160)
						.macTermToolTipText("The sound to play when a terminal bell occurs.")
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Always use visual bell", isOn: $viewModel.visualBell)
						.macTermToolTipText("Set if “bell” events should use a visual flash even if a sound will play too.")
				}
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Text("When a bell sounds in an inactive window, a visual appears automatically.")
						.controlSize(.small)
						.fixedSize(horizontal: false, vertical: true)
						.lineLimit(2)
						.multilineTextAlignment(.leading)
						.frame(maxWidth: 220)
						.offset(x: 18, y: 0) // try to align with checkbox label
				}
				Spacer().asMacTermMinorSectionSpacingV()
				UICommon_OptionLineView("", noDefaultSpacing: true) {
					Toggle("Background notification on bell", isOn: $viewModel.bellNotificationInBackground)
						.macTermToolTipText("Set if a notification is sent to the system for “bell” events when MacTerm is not the active application.")
				}
			}
			Spacer().asMacTermSectionSpacingV()
			Group {
				UICommon_OptionLineView("When In Background", noDefaultSpacing: true) {
					Picker("", selection: $viewModel.backgroundNotificationAction) {
						// TBD: how to insert dividing-line in this type of menu?
						localizedLabelView(.doNothing)
						localizedLabelView(.markDockIcon)
						localizedLabelView(.markDockIconAndBounceOnce)
						localizedLabelView(.markDockIconAndBounceRepeatedly)
					}.pickerStyle(RadioGroupPickerStyle())
						.macTermOffsetForEmptyPickerTitle()
						.macTermToolTipText("How to respond to notifications when MacTerm is not the active application.")
				}
			}
			Spacer().asMacTermSectionSpacingV()
			HStack {
				Text("Use system-wide settings to further customize notification behavior.")
					.controlSize(.small)
					.fixedSize(horizontal: false, vertical: true)
					.lineLimit(3)
					.multilineTextAlignment(.leading)
					.frame(maxWidth: 400)
			}.withMacTermSectionLayout()
			Spacer().asMacTermSectionSpacingV()
			Spacer().layoutPriority(1)
		}
	}

}

public class UIPrefsGeneralNotifications_ObjC : NSObject {

	@objc public static func makeView(_ data: UIPrefsGeneralNotifications_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIPrefsGeneralNotifications_View().environmentObject(data)))
	}

}

public struct UIPrefsGeneralNotifications_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIPrefsGeneralNotifications_Model(runner: UIPrefsGeneralNotifications_RunnerDummy())
		data.bellSoundItems.append(UIPrefsGeneralNotifications_BellSoundItemModel(soundName: "Basso"))
		data.bellSoundItems.append(UIPrefsGeneralNotifications_BellSoundItemModel(soundName: "Glass"))
		data.bellSoundItems.append(UIPrefsGeneralNotifications_BellSoundItemModel(soundName: "Hero"))
		return VStack {
			UIPrefsGeneralNotifications_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIPrefsGeneralNotifications_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
