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

@objc public protocol UIArrangeWindow_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func doneArranging(viewModel: UIArrangeWindow_Model)
}

class UIArrangeWindow_RunnerDummy : NSObject, UIArrangeWindow_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	public func doneArranging(viewModel: UIArrangeWindow_Model) {
		print(#function)
	}
}

public class UIArrangeWindow_Model : NSObject, ObservableObject {

	public var runner: UIArrangeWindow_ActionHandling

	@objc public init(runner: UIArrangeWindow_ActionHandling) {
		self.runner = runner
	}

}

public struct UIArrangeWindow_View : View {

	@EnvironmentObject private var viewModel: UIArrangeWindow_Model

	public var body: some View {
		VStack (alignment: .leading) {
			Text("Move this window to the desired location.")
				.font(Font.system(.headline))
			Text("The top-left corner will be used as the guide for new windows.")
				.font(Font.system(.subheadline))
			HStack {
				Spacer()
				Button("Done", action: { viewModel.runner.doneArranging(viewModel: viewModel) })
			}
		}.frame(minWidth: 500)
			.padding(20)
	}

}

public class UIArrangeWindow_ObjC : NSObject {

	@objc public static func makeViewController(_ data: UIArrangeWindow_Model) -> NSViewController {
		return NSHostingController<AnyView>(rootView: AnyView(UIArrangeWindow_View().environmentObject(data)))
	}

}

public struct UIArrangeWindow_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIArrangeWindow_Model(runner: UIArrangeWindow_RunnerDummy())
		return VStack {
			UIArrangeWindow_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIArrangeWindow_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
