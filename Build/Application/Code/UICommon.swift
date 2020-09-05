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
// The following are various common extensions or utilities
// that more than one SwiftUI file might use.
//

extension HStack {
	// preserve left and right space all the way down a vertical stack of views
	public func withMacTermSectionLayout() -> some View {
		padding([.horizontal], 16)
	}
}

extension Spacer {
	// separate each new section from the one above it
	public func asMacTermSectionSpacingV() -> some View {
		padding([.top], 16)
	}
}

extension Text {
	// for sections, e.g. in Preferences, a right-aligned title like "Options:"
	public func asMacTermSectionHeading() -> some View {
		bold()
		.frame(width: 160, alignment: .trailing)
		.multilineTextAlignment(.trailing)
	}
}

struct UICommon_OptionLineView <Content: View> : View {

	let optionView: Content
	var title = ""

	// variant with title and content
	init(_ aTitle: String, @ViewBuilder content: () -> Content) {
		if aTitle == "" {
			self.title = " " // blank space ensures consistent vertical spacing
		} else {
			self.title = aTitle + ":"
		}
		self.optionView = content()
	}

	// variant for spacing additional options in the same section (blank title)
	init(@ViewBuilder content: () -> Content) {
		self.init("", content: content)
	}

	var body: some View {
		HStack {
			Text(title).asMacTermSectionHeading()
			optionView
		}.withMacTermSectionLayout()
	}

}

struct UICommon_ProgressCircleView : NSViewRepresentable {

	typealias NSViewType = NSProgressIndicator

	// MARK: NSViewRepresentable

	func makeNSView(context: Self.Context) -> Self.NSViewType {
		let result = NSViewType()
		result.controlSize = .small
		result.isIndeterminate = true
		result.style = .spinning
		result.startAnimation(nil)
		return result
	}

	func updateNSView(_ nsView: Self.NSViewType, context: Self.Context) {
		// (nothing needed)
	}

}
