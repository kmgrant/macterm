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

@objc public protocol UIAddressList_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func refresh(addressList: UIAddressList_Model)
}

public class UIAddressList_RunnerDummy : NSObject, UIAddressList_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	public func refresh(addressList: UIAddressList_Model) {
		print(#function)
		addressList.refreshInProgress = false
	}
}

public class UIAddressList_ItemModel : NSObject, Identifiable, ObservableObject {

	public var id = UUID()
	@Published public var addressString: String // also the IP address or other address

	@objc public init(string: String) {
		self.addressString = string
	}

}

public class UIAddressList_Model : NSObject, ObservableObject {

	@Published @objc public var addressArray: [UIAddressList_ItemModel] = []
	@Published @objc public var refreshInProgress = false
	public var runner: UIAddressList_ActionHandling

	@objc public init(runner: UIAddressList_ActionHandling) {
		self.runner = runner
	}

}

public struct UIAddressList_ItemView : View {

	@EnvironmentObject private var itemModel: UIAddressList_ItemModel

	public var body: some View {
		Text(itemModel.addressString)
			//.fixedSize()
			.font(Font.system(.body, design: .monospaced))
			.onDrag({ return NSItemProvider(object: itemModel.addressString as NSString) })
	}

}

public struct UIAddressList_View : View {

	@EnvironmentObject private var viewModel: UIAddressList_Model

	public var body: some View {
		VStack {
			List {
				ForEach(viewModel.addressArray) { item in
					UIAddressList_ItemView().environmentObject(item)
				}
			}.frame(minWidth: 200, minHeight: 48, idealHeight: 150)
			HStack (
				alignment: .firstTextBaseline
			) {
				Text("You can drag addresses from this list.")
					.font(Font.system(.caption))
					.padding([.leading], 8)
				Spacer()
				if viewModel.refreshInProgress {
					UICommon_ProgressCircleView()
						.alignmentGuide(.firstTextBaseline, computeValue: { $0[.bottom] - 3 })
				} else {
					UICommon_ProgressCircleView()
						.alignmentGuide(.firstTextBaseline, computeValue: { $0[.bottom] - 3 })
						.hidden()
				}
				Button(
					action: {
						viewModel.refreshInProgress = true
						// implementation of refresh() must reset "refreshInProgress" to false!
						viewModel.runner.refresh(addressList: viewModel)
					}
				) {
					if #available(macOS 11.0, *) {
						Image(systemName: "arrow.clockwise")
					} else {
						Text("Refresh")
						//Text("􀅈")
					}
				}.buttonStyle(BorderlessButtonStyle())
					.accessibility(label: Text("Refresh"))
					.macTermToolTipText("Refreshes the list by querying the system in the background for all current addresses.")
			}.padding([.trailing], 20)
				.padding([.bottom], 4)
		}.frame(maxWidth: 500, maxHeight: .infinity)
	}

}

public class UIAddressList_ObjC : NSObject {

	@objc public static func makeViewController(_ data: UIAddressList_Model) -> NSViewController {
		return NSHostingController<AnyView>(rootView: AnyView(UIAddressList_View().environmentObject(data)))
	}

}

public struct UIAddressList_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIAddressList_Model(runner: UIAddressList_RunnerDummy())
		data.addressArray.append(UIAddressList_ItemModel(string: "123.45.67.89"))
		data.addressArray.append(UIAddressList_ItemModel(string: "1.1.1.1"))
		data.addressArray.append(UIAddressList_ItemModel(string: "abcd:cdef:abcd:cdef:abcd:cdef:abcd:cdef"))
		return VStack {
			// show Light and Dark variants simultaneously
			UIAddressList_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIAddressList_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
