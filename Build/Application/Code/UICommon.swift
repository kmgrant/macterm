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

extension VerticalAlignment {
	// for sections, when the goal is to align a custom view with the section title text;
	// example usage:
	// - given VStack, first child at top could use...
	//   .alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
	// note that view helper classes below have a "disableDefaultAlignmentGuide" option
	// to allow customizations such as the above; otherwise, by default any option view
	// will have an alignment guide applied to encourage a nice layout by default
	private enum MacTermSectionAlignment : AlignmentID {
		static func defaultValue(in d: ViewDimensions) ->  CGFloat {
			return d[.top]
		}
	}
	static let sectionAlignmentMacTerm = VerticalAlignment(MacTermSectionAlignment.self)

	// give a CSS-style name to vertical centering so it is easier to use (i.e. that way,
	// one can say [.middle] instead of the ridiculous [VerticalAlignment.center])
	static let middle = VerticalAlignment.center
}

public class UICommon_BaseModel : NSObject {

	@objc public var disableWriteback = false // to prevent changes from causing looping updates; see below, and Objective-C code that initializes views or changes data sources

	func ifWritebackEnabled(_ block: () -> ()) {
		// automatically do nothing if "disableWriteback" is set; see "ifUserRequestedDefault"
		if false == disableWriteback {
			block()
		}
	}

}

public class UICommon_DefaultingModel : UICommon_BaseModel {

	@objc public var defaultOverrideInProgress = false // to prevent changes to default flags from causing looping updates; see below, and Objective-C models that change data sources
	@objc public var isEditingDefaultContext = false {
		willSet(isOn) {
			if isOn {
				// when setting this, force all Default checkboxes to be disabled and checked
				defaultOverrideInProgress = true
				setDefaultFlagsToTrue()
				defaultOverrideInProgress = false
			}
		}
	}

	func ifUserRequestedDefault(_ block: () -> ()) {
		// automatically do nothing if "defaultOverrideInProgress" is set; otherwise, run the
		// given block with write-back mode disabled (see "isEditingDefaultContext/willSet")
		if false == defaultOverrideInProgress {
			disableWriteback = true
			block()
			disableWriteback = false
		}
	}

	func inNonDefaultContext(_ block: () -> ()) {
		// automatically do nothing if "isEditingDefaultContext" is set
		if false == isEditingDefaultContext {
			block()
		}
	}

	// MARK: Overrides for Subclasses

	func setDefaultFlagsToTrue() {
		// (subclasses should override; arrange to set all Default checkboxes to be disabled, in checked state)
	}

}

struct UICommon_OptionLineView <Content: View> : View {

	@State private var unusedDefault = true
	private var disableDefaultAlignmentGuide = false
	private var noDefaultSpacing = false
	let optionView: Content
	var title = ""

	// variant with title and content
	init(_ aTitle: String, disableDefaultAlignmentGuide: Bool = false, noDefaultSpacing: Bool = false, @ViewBuilder content: () -> Content) {
		if aTitle == "" {
			self.title = " " // blank space ensures consistent vertical spacing
		} else {
			self.title = aTitle + ":"
		}
		self.disableDefaultAlignmentGuide = disableDefaultAlignmentGuide
		self.noDefaultSpacing = noDefaultSpacing
		self.optionView = content()
	}

	// variant for spacing additional options in the same section (blank title)
	init(@ViewBuilder content: () -> Content) {
		self.init("", content: content)
	}

	var body: some View {
		HStack(
			alignment: .sectionAlignmentMacTerm
		) {
			if false == noDefaultSpacing {
				// create unused checkboxes to ensure consistent alignment if
				// any “default” options appear in the same stack
				Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
					.controlSize(.mini)
					.labelsHidden()
					.hidden()
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
				Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
					.controlSize(.mini)
					.labelsHidden()
					.hidden()
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
				Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
					.controlSize(.mini)
					.labelsHidden()
					.hidden()
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			}
			Text(title).asMacTermSectionHeading()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			if disableDefaultAlignmentGuide {
				optionView
			} else {
				optionView
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			}
		}.withMacTermSectionLayout()
	}

}

struct UICommon_Default1OptionLineView <Content: View> : View {

	@State private var unusedDefault = true
	private var disableDefaultAlignmentGuide = false
	private var isEditingDefault = false
	var isDefaultBinding: Binding<Bool>
	let optionView: Content
	var title = ""

	// variant with “default” checkbox, title, and content
	init(_ aTitle: String, bindIsDefaultTo: Binding<Bool>, isEditingDefault: Bool,
			disableDefaultAlignmentGuide: Bool = false, @ViewBuilder content: () -> Content) {
		if aTitle == "" {
			self.title = " " // blank space ensures consistent vertical spacing
		} else {
			self.title = aTitle + ":"
		}
		self.disableDefaultAlignmentGuide = disableDefaultAlignmentGuide
		self.isDefaultBinding = bindIsDefaultTo
		self.isEditingDefault = isEditingDefault
		self.optionView = content()
	}

	var body: some View {
		HStack(
			alignment: .sectionAlignmentMacTerm
		) {
			Toggle("Tie setting to its value in the Default collection", isOn: isDefaultBinding)
				.controlSize(.mini)
				.disabled(isEditingDefault || isDefaultBinding.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			// create unused checkboxes to ensure consistent alignment if
			// any “default” options appear in the same stack
			Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
				.controlSize(.mini)
				.disabled(true)
				.labelsHidden()
				.hidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
				.controlSize(.mini)
				.disabled(true)
				.labelsHidden()
				.hidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			Text(title).asMacTermSectionHeading()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			if disableDefaultAlignmentGuide {
				optionView
			} else {
				optionView
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			}
		}.withMacTermSectionLayout()
	}

}

struct UICommon_Default2OptionLineView <Content: View> : View {

	@State private var unusedDefault = true
	private var disableDefaultAlignmentGuide = false
	private var isEditingDefault = false
	var isDefault1Binding: Binding<Bool>
	var isDefault2Binding: Binding<Bool>
	let optionView: Content
	var title = ""

	// variant with “default” checkbox, title, and content
	init(_ aTitle: String, bindIsDefault1To: Binding<Bool>, bindIsDefault2To: Binding<Bool>,
			isEditingDefault: Bool, disableDefaultAlignmentGuide: Bool = false, @ViewBuilder content: () -> Content) {
		if aTitle == "" {
			self.title = " " // blank space ensures consistent vertical spacing
		} else {
			self.title = aTitle + ":"
		}
		self.disableDefaultAlignmentGuide = disableDefaultAlignmentGuide
		self.isDefault1Binding = bindIsDefault1To
		self.isDefault2Binding = bindIsDefault2To
		self.isEditingDefault = isEditingDefault
		self.optionView = content()
	}

	var body: some View {
		HStack(
			alignment: .sectionAlignmentMacTerm
		) {
			Toggle("Tie setting to its value in the Default collection", isOn: isDefault1Binding)
				.controlSize(.mini)
				.disabled(isEditingDefault || isDefault1Binding.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			Toggle("Tie setting to its value in the Default collection", isOn: isDefault2Binding)
				.controlSize(.mini)
				.disabled(isEditingDefault || isDefault2Binding.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			// create unused checkboxes to ensure consistent alignment if
			// any “default” options appear in the same stack
			Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
				.controlSize(.mini)
				.disabled(true)
				.labelsHidden()
				.hidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			Text(title).asMacTermSectionHeading()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			if disableDefaultAlignmentGuide {
				optionView
			} else {
				optionView
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			}
		}.withMacTermSectionLayout()
	}

}

struct UICommon_Default3OptionLineView <Content: View> : View {

	@State private var unusedDefault = true
	private var disableDefaultAlignmentGuide = false
	private var isEditingDefault = false
	var isDefault1Binding: Binding<Bool>
	var isDefault2Binding: Binding<Bool>
	var isDefault3Binding: Binding<Bool>
	let optionView: Content
	var title = ""

	// variant with “default” checkbox, title, and content
	init(_ aTitle: String, bindIsDefault1To: Binding<Bool>, bindIsDefault2To: Binding<Bool>, bindIsDefault3To: Binding<Bool>,
			isEditingDefault: Bool, disableDefaultAlignmentGuide: Bool = false, @ViewBuilder content: () -> Content) {
		if aTitle == "" {
			self.title = " " // blank space ensures consistent vertical spacing
		} else {
			self.title = aTitle + ":"
		}
		self.disableDefaultAlignmentGuide = disableDefaultAlignmentGuide
		self.isDefault1Binding = bindIsDefault1To
		self.isDefault2Binding = bindIsDefault2To
		self.isDefault3Binding = bindIsDefault3To
		self.isEditingDefault = isEditingDefault
		self.optionView = content()
	}

	var body: some View {
		HStack(
			alignment: .sectionAlignmentMacTerm
		) {
			Toggle("Tie setting to its value in the Default collection", isOn: isDefault1Binding)
				.controlSize(.mini)
				.disabled(isEditingDefault || isDefault1Binding.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			Toggle("Tie setting to its value in the Default collection", isOn: isDefault2Binding)
				.controlSize(.mini)
				.disabled(isEditingDefault || isDefault2Binding.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			Toggle("Tie setting to its value in the Default collection", isOn: isDefault3Binding)
				.controlSize(.mini)
				.disabled(isEditingDefault || isDefault3Binding.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			// (all variants have space for up to 3 checkboxes so no extra padding here)
			Text(title).asMacTermSectionHeading()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			if disableDefaultAlignmentGuide {
				optionView
			} else {
				optionView
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.middle] })
			}
		}.withMacTermSectionLayout()
	}

}

struct UICommon_DefaultOptionHeaderView : View {

	var title = ""

	init(_ aTitle: String = "Use Default") {
		if aTitle == "" {
			self.title = " " // blank space ensures consistent vertical spacing
		} else {
			self.title = aTitle
		}
	}

	var body: some View {
		HStack(
			alignment: .firstTextBaseline
		) {
			// visually, this is meant to appear “above” any checkboxes that
			// indicate a Default; see UICommon_Default1OptionLineView, etc.
			Text(title)
				.font(Font.system(size: 10))
				.padding([.top], 4)
			Spacer()
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
