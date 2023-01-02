/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
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

import Cocoa
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

	// for first line above views of a panel, e.g. UICommon_DefaultOptionHeaderView;
	// preserve left and right space all the way down a vertical stack of views
	public func withMacTermTopHeaderLayout() -> some View {
		if #available(macOS 11.0, *) {
			// macOS 11 seems to shift SwiftUI view up more, clipping the top;
			// shift “header” views downward to make them fully visible
			return AnyView(padding([.horizontal], 16).padding([.top], 16))
		}
		return AnyView(padding([.horizontal], 16))
	}
}

extension Spacer {
	// separate each new section from the one above it
	public func asMacTermSectionSpacingV() -> some View {
		padding([.top], 16)
	}

	// slightly separate each new section from the one above it
	public func asMacTermMinorSectionSpacingV() -> some View {
		padding([.top], 4)
	}
}

extension Text {
	// for sections, e.g. in Preferences, a right-aligned title like "Options:"
	public func asMacTermSectionHeading(isSmall: Bool = false) -> some View {
		bold()
		.frame(width: isSmall ? CGFloat(80) : CGFloat(160), alignment: .trailing)
		.multilineTextAlignment(.trailing)
		.controlSize(isSmall ? .small : .regular)
	}
}

extension VerticalAlignment {
	// for sections, when the goal is to align a custom view with the section title text;
	// example usage:
	// - given VStack, first child at top could use...
	//   .alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
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

extension View {
	// return true only if macOS 11 or greater
	public func isOS11() -> Bool {
		if #available(macOS 11.0, *) {
			return true
		}
		return false
	}

	// eliminate left-padding created by Picker for empty label
	public func macTermOffsetForEmptyPickerTitle() -> some View {
		offset(x: -8, y: 0)
	}

	// for scrollable List controls that appear in sections;
	// this calls alignmentGuide() to shift the text to line up
	// well with its section title (note: this may require the
	// option view to use "disableDefaultAlignmentGuide")
	public func macTermSectionAlignmentGuideList() -> some View {
		alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.top] + (isOS11() ? 27 : 18) })
	}

	// simplifies call to help() API (only available in macOS 11),
	// allowing builds for older OSes to simply do nothing; note
	// that this erases the type of the view sequence so it should
	// typically be one of the last things done in a view chain
	public func macTermToolTipText(_ text: String) -> some View {
		// note: offset(0, 0) is a hack to be able to return a
		// view that does not materially affect anything
		if #available(macOS 11.0, *) {
			return AnyView(offset(x: 0, y: 0).help(text))
		}
		return AnyView(offset(x: 0, y: 0))
	}

	// for custom views; locates any SwiftUI Environment setting
	// so that "controlSize" can be set consistently
	static func getNSControlSize(from environment: EnvironmentValues) -> NSControl.ControlSize {
		var result: NSControl.ControlSize = .regular
		switch environment.controlSize {
		case .small:
			result = .small
		case .mini:
			result = .mini
		case .regular:
			fallthrough
		default:
			result = .regular
		}
		if #available(macOS 11.0, *) {
			if environment.controlSize == .large {
				result = .large
			}
		}
		return result
	}
}

// MARK: -
public class UICommon_BaseModel : NSObject {

	@objc public var disableWriteback = false // to prevent changes from causing looping updates; see below, and Objective-C code that initializes views or changes data sources

	func ifWritebackEnabled(_ block: () -> ()) {
		// automatically do nothing if "disableWriteback" is set; see "ifUserRequestedDefault"
		if false == disableWriteback {
			block()
		}
	}

}

// MARK: -
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

// MARK: -
struct UICommon_DividerSectionView : View {

	var body: some View {
		// simplify the process of creating a horizontal line that
		// separates sections of a standard container, with margins;
		// use this in stacks that contain other section views, e.g.
		// UICommon_OptionLineView or UICommon_Default1OptionLineView
		HStack(
			alignment: .sectionAlignmentMacTerm
		) {
			VStack {
				Divider().disabled(true)
			}
		}.withMacTermSectionLayout()
	}

}

// MARK: -
struct UICommon_OptionLineView <Content: View> : View {

	@State private var unusedDefault = true
	private var disableDefaultAlignmentGuide = false
	private var isSmallHeading = false
	private var isSmallMultilineTitle = false
	private var noDefaultSpacing = false
	let optionView: Content
	var title = ""

	// variant with title and content
	init(_ aTitle: String, isSmallHeading: Bool = false, isSmallMultilineTitle: Bool = false, disableDefaultAlignmentGuide: Bool = false, noDefaultSpacing: Bool = false, @ViewBuilder content: () -> Content) {
		self.isSmallHeading = isSmallHeading
		self.isSmallMultilineTitle = isSmallMultilineTitle
		if aTitle == "" {
			self.title = " " // blank space ensures consistent vertical spacing
		} else {
			if isSmallMultilineTitle {
				self.title = aTitle
			} else {
				self.title = aTitle + ":"
			}
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
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
				Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
					.controlSize(.mini)
					.labelsHidden()
					.hidden()
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
				Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
					.controlSize(.mini)
					.labelsHidden()
					.hidden()
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			}
			if isSmallMultilineTitle {
				Text(title)
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
					.controlSize(.small)
					.fixedSize(horizontal: false, vertical: true)
					.lineLimit(10)
					.multilineTextAlignment(.leading)
					.padding(0)
					.frame(width: 160, alignment: .leading)
			} else {
				Text(title).asMacTermSectionHeading(isSmall: isSmallHeading)
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			}
			if disableDefaultAlignmentGuide {
				optionView
			} else {
				optionView
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			}
		}.withMacTermSectionLayout()
	}

}

// MARK: -
// (this is used several times in the views below)
var UICommon_DefaultToggleAccessibilityDesc = "Restore to Default Value"
var UICommon_DefaultToggleAccessibilityDescTemplate = NSLocalizedString("%1$@: Restore to Default Value",
																		comment: "accessibility description of preference setting; 1$ = description of setting") 
var UICommon_DefaultToggleToolTipText = NSLocalizedString("When the current collection is not the Default collection, this may be set to tie the preference next to this checkbox to the Default collection.  Edit the preference next to this checkbox to remove the tie and make the setting unique to this collection.",
															comment: "tooltip for checkbox that returns a preference setting to its default value") 
var UICommon_DefaultToggleToolTipTextTemplate = NSLocalizedString("When the current collection is not the Default collection, this may be set to tie “%1$@” to the Default collection.  Edit “%1$@” to remove the tie and make the setting unique to this collection.",
																	comment: "tooltip for checkbox that returns named preference setting to its default value (multi-checkbox case); 1$ = description of setting")

struct UICommon_DefaultToggleProperties {

	// this collects all the unique attributes of “Use Default” checkboxes
	// that frequently appear next to settings in Preferences panels
	var accessibilityPrefName: String? // should be Title Case and short
	var bindIsDefaultTo: Binding<Bool> // binding
	var customToolTip = false // optional; most checkboxes appear alone so their tooltips can be generic without identifying preferences

	func accessibilityDescription() -> String {
		if let definedPrefName = self.accessibilityPrefName {
			return String(format: UICommon_DefaultToggleAccessibilityDescTemplate, definedPrefName)
		}
		return UICommon_DefaultToggleAccessibilityDesc
	}

	func toolTipText() -> String {
		if self.customToolTip {
			if let definedPrefName = self.accessibilityPrefName {
				return String(format: UICommon_DefaultToggleToolTipTextTemplate, definedPrefName)
			}
		}
		return UICommon_DefaultToggleToolTipText
	}

}

// MARK: -
struct UICommon_Default1OptionLineView <Content: View> : View {

	@State private var unusedDefault = true
	private var disableDefaultAlignmentGuide = false
	private var isEditingDefault = false
	var toggle1: UICommon_DefaultToggleProperties
	let optionView: Content
	var title = ""

	init(_ aTitle: String, toggleConfig: UICommon_DefaultToggleProperties, isEditingDefault: Bool,
			disableDefaultAlignmentGuide: Bool = false, @ViewBuilder content: () -> Content) {
		if aTitle == "" {
			self.title = " " // blank space ensures consistent vertical spacing
		} else {
			self.title = aTitle + ":"
		}
		self.disableDefaultAlignmentGuide = disableDefaultAlignmentGuide
		self.isEditingDefault = isEditingDefault
		self.optionView = content()
		self.toggle1 = toggleConfig
	}

	var body: some View {
		HStack(
			alignment: .sectionAlignmentMacTerm
		) {
			Toggle(" ", isOn: toggle1.bindIsDefaultTo)
				.controlSize(.mini)
				.disabled(isEditingDefault || toggle1.bindIsDefaultTo.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
				.accessibility(label: Text(self.toggle1.accessibilityDescription()))
				.macTermToolTipText(toggle1.toolTipText())
			// create unused checkboxes to ensure consistent alignment if
			// any “default” options appear in the same stack
			Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
				.controlSize(.mini)
				.disabled(true)
				.labelsHidden()
				.hidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
				.controlSize(.mini)
				.disabled(true)
				.labelsHidden()
				.hidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			Text(title).asMacTermSectionHeading()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			if disableDefaultAlignmentGuide {
				optionView
			} else {
				optionView
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			}
		}.withMacTermSectionLayout()
	}

}

// MARK: -
struct UICommon_Default2OptionLineView <Content: View> : View {

	@State private var unusedDefault = true
	private var disableDefaultAlignmentGuide = false
	private var isEditingDefault = false
	var toggle1: UICommon_DefaultToggleProperties
	var toggle2: UICommon_DefaultToggleProperties
	let optionView: Content
	var title = ""

	init(_ aTitle: String, toggle1Config: UICommon_DefaultToggleProperties, toggle2Config: UICommon_DefaultToggleProperties,
			isEditingDefault: Bool, disableDefaultAlignmentGuide: Bool = false, @ViewBuilder content: () -> Content) {
		if aTitle == "" {
			self.title = " " // blank space ensures consistent vertical spacing
		} else {
			self.title = aTitle + ":"
		}
		self.disableDefaultAlignmentGuide = disableDefaultAlignmentGuide
		self.isEditingDefault = isEditingDefault
		self.optionView = content()
		self.toggle1 = toggle1Config
		self.toggle2 = toggle2Config
		// when there are multiple checkboxes, include preference names in tooltips
		self.toggle1.customToolTip = true
		self.toggle2.customToolTip = true
	}

	var body: some View {
		HStack(
			alignment: .sectionAlignmentMacTerm
		) {
			Toggle(" ", isOn: toggle1.bindIsDefaultTo)
				.controlSize(.mini)
				.disabled(isEditingDefault || toggle1.bindIsDefaultTo.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
				.accessibility(label: Text(self.toggle1.accessibilityDescription()))
				.macTermToolTipText(toggle1.toolTipText())
			Toggle(" ", isOn: toggle2.bindIsDefaultTo)
				.controlSize(.mini)
				.disabled(isEditingDefault || toggle2.bindIsDefaultTo.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
				.accessibility(label: Text(self.toggle2.accessibilityDescription()))
				.macTermToolTipText(toggle2.toolTipText())
			// create unused checkboxes to ensure consistent alignment if
			// any “default” options appear in the same stack
			Toggle(" ", isOn: $unusedDefault) // blank space helps to set vertical alignment
				.controlSize(.mini)
				.disabled(true)
				.labelsHidden()
				.hidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			Text(title).asMacTermSectionHeading()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			if disableDefaultAlignmentGuide {
				optionView
			} else {
				optionView
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			}
		}.withMacTermSectionLayout()
	}

}

// MARK: -
struct UICommon_Default3OptionLineView <Content: View> : View {

	@State private var unusedDefault = true
	private var disableDefaultAlignmentGuide = false
	private var isEditingDefault = false
	var toggle1: UICommon_DefaultToggleProperties
	var toggle2: UICommon_DefaultToggleProperties
	var toggle3: UICommon_DefaultToggleProperties
	let optionView: Content
	var title = ""

	init(_ aTitle: String, toggle1Config: UICommon_DefaultToggleProperties, toggle2Config: UICommon_DefaultToggleProperties,
			toggle3Config: UICommon_DefaultToggleProperties, isEditingDefault: Bool,
			disableDefaultAlignmentGuide: Bool = false, @ViewBuilder content: () -> Content) {
		if aTitle == "" {
			self.title = " " // blank space ensures consistent vertical spacing
		} else {
			self.title = aTitle + ":"
		}
		self.disableDefaultAlignmentGuide = disableDefaultAlignmentGuide
		self.isEditingDefault = isEditingDefault
		self.optionView = content()
		self.toggle1 = toggle1Config
		self.toggle2 = toggle2Config
		self.toggle3 = toggle3Config
		// when there are multiple checkboxes, include preference names in tooltips
		self.toggle1.customToolTip = true
		self.toggle2.customToolTip = true
		self.toggle3.customToolTip = true
	}

	var body: some View {
		HStack(
			alignment: .sectionAlignmentMacTerm
		) {
			Toggle(" ", isOn: toggle1.bindIsDefaultTo)
				.controlSize(.mini)
				.disabled(isEditingDefault || toggle1.bindIsDefaultTo.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
				.accessibility(label: Text(self.toggle1.accessibilityDescription()))
				.macTermToolTipText(toggle1.toolTipText())
			Toggle(" ", isOn: toggle2.bindIsDefaultTo)
				.controlSize(.mini)
				.disabled(isEditingDefault || toggle2.bindIsDefaultTo.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
				.accessibility(label: Text(self.toggle2.accessibilityDescription()))
				.macTermToolTipText(toggle2.toolTipText())
			Toggle(" ", isOn: toggle3.bindIsDefaultTo)
				.controlSize(.mini)
				.disabled(isEditingDefault || toggle3.bindIsDefaultTo.wrappedValue)
				.labelsHidden()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
				.accessibility(label: Text(self.toggle3.accessibilityDescription()))
				.macTermToolTipText(toggle3.toolTipText())
			// (all variants have space for up to 3 checkboxes so no extra padding here)
			Text(title).asMacTermSectionHeading()
				.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			if disableDefaultAlignmentGuide {
				optionView
			} else {
				optionView
					.alignmentGuide(.sectionAlignmentMacTerm, computeValue: { d in d[.firstTextBaseline] })
			}
		}.withMacTermSectionLayout()
	}

}

// MARK: -
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
		}.withMacTermTopHeaderLayout()
	}

}

// MARK: -
struct UICommon_FieldErrorView : View {

	private var isError: Binding<Bool>
	private var toolTipText: String

	init(_ isError: Binding<Bool>, toolTipText: String) {
		self.isError = isError
		self.toolTipText = toolTipText
	}

	var body: some View {
		// this is hidden or displayed based on the binding; it should
		// appear after an affected TextField because the arrow will
		// point backward to the field with the error (since it is
		// expected to appear next to text, its default alignment
		// guide assumes a text baseline)
		let result = HStack {
			Image(nsImage: isError.wrappedValue ? (NSImage(named: NSImage.invalidDataFreestandingTemplateName) ?? NSImage()) : NSImage())
				.resizable()
				.alignmentGuide(.firstTextBaseline, computeValue: { d in d[.middle] + 4 })
				.frame(maxWidth: 16, maxHeight: 16)
			Image(nsImage: isError.wrappedValue ? (NSImage(named: NSImage.cautionName) ?? NSImage()) : NSImage())
				.resizable()
				.alignmentGuide(.firstTextBaseline, computeValue: { d in d[.middle] + 4 })
				.frame(maxWidth: 16, maxHeight: 16)
		}.frame(maxHeight: 16)
		if isError.wrappedValue {
			result.macTermToolTipText(toolTipText)
		} else {
			result
		}
	}

}

// MARK: -
struct UICommon_FileSystemPathView : NSViewRepresentable {

	typealias NSViewType = NSPathControl

	// the Coordinator is constructed by makeCoordinator() (once) and
	// since it is a class it can do Objective-C things like implement
	// action methods (UICommon_FileSystemPathView is a Swift struct)
	class Coordinator : NSObject {

		@Binding var urlRef: URL

		init(_ urlRef: Binding<URL>) {
			_urlRef = urlRef // use "_" to initialize a binding
		}

		// MARK: Actions

		@objc func pathChangedAction(sender: Any?) {
			// this is specified in UICommon_FileSystemPathView (makeNSView()) and
			// is the "action" selector; according to NSPathControl documentation,
			// this will be called when the URL changes via open panel or drags;
			// the "sender" is the NSPathControl object
			guard let definedSender = sender else { print("\(#function) failed: sender is nil"); return }
			guard let pathControl = definedSender as? NSPathControl else { print("\(#function) failed: sender is not an NSPathControl"); return }
			if let definedURL = pathControl.url {
				urlRef = definedURL // SwiftUI binding (triggers any "didSet" actions)
			} else {
				print("failed to process path URL, currently set to nil")
			}
		}

	}

	private var allowedTypes: [String] // UTIs to constrain drags/open-select of files/folders
	@Binding var urlRef: URL // SwiftUI binding, e.g. part of view model; also visible to Coordinator

	init(_ urlRef: Binding<URL>, allowedTypes: [String] = ["public.folder"]) {
		// note: perhaps surprisingly, this can be called very frequently by SwiftUI
		// updates, and it will NOT call makeCoordinator() and makeNSView() each time;
		// rather, the first time it will call:
		//     init() -> makeCoordinator() -> makeNSView() -> updateNSView()
		// ...and every other time it will call:
		//     init() -> updateNSView()
		_urlRef = urlRef // use "_" to initialize a binding
		self.allowedTypes = allowedTypes
	}

	// MARK: NSViewRepresentable

	func makeCoordinator() -> Coordinator {
		// note: this is called after init() but only the first time this type of view
		// is used (and makeNSView() will be called after this)
		return Coordinator($urlRef)
	}

	func makeNSView(context: Self.Context) -> Self.NSViewType {
		// IMPORTANT: this is called after init() and makeCoordinator() but only the
		// first time this type of view is used; in all other cases, init() is
		// called but then only updateNSView() will be called...
		let result = NSViewType()
		result.controlSize = Self.getNSControlSize(from: context.environment)
		result.allowedTypes = self.allowedTypes
		result.isEditable = true
		result.pathStyle = .popUp
		// IMPORTANT: according to NSPathControl documentation, the control’s "action"
		// is invoked when the URL changes for any reason (e.g. drag-and-drop or file
		// selection dialog); this is critical for updating the SwiftUI binding, and
		// in particular nothing else will work (e.g. KVO observing does not)
		result.target = context.coordinator
		result.action = #selector(Coordinator.pathChangedAction)
		// note: NSPathControlDelegate is not currently needed but if it is someday
		// required, it can be implemented by the Coordinator class and set below
		//result.delegate = context.coordinator
		return result
	}

	func updateNSView(_ nsView: Self.NSViewType, context: Self.Context) {
		// IMPORTANT: on every major state change, init() will be called first
		// and updateNSView() is called last but in between the make...() methods
		// above are only called the *first* time (see comments above)
		nsView.url = self.urlRef
	}

}

// MARK: -
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
