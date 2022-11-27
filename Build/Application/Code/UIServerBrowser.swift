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

fileprivate let UIServerBrowser_ErrorStringBadHost = "The host name is not in a supported format."
fileprivate let UIServerBrowser_ErrorStringBadPort = "The port must be a number from 0 to 65535."
fileprivate let UIServerBrowser_ErrorStringBadUser = "The user ID must only use letters, numbers, dashes, underscores and periods."

@objc public protocol UIServerBrowser_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func dataUpdatedHostName(_ value: String)
	func dataUpdatedPortNumber(_ value: String)
	func dataUpdatedProtocol(_ value: Session_Protocol)
	func dataUpdatedServicesListVisible(_ value: Bool)
	func dataUpdatedUserID(_ value: String)
	func lookUpSelectedHostName(viewModel: UIServerBrowser_Model?) // caller should set "viewModel.isLookupInProgress" to false when done
	func validateHostName(viewModel: UIServerBrowser_Model) // caller should set "viewModel.isErrorInHostName" if validation fails; behavior should match error message
	func validatePortNumber(viewModel: UIServerBrowser_Model) // caller should set "viewModel.isErrorInPortNumber" if validation fails; behavior should match error message
	func validateUserID(viewModel: UIServerBrowser_Model) // caller should set "viewModel.isErrorInUserID" if validation fails; behavior should match error message
}

class UIServerBrowser_RunnerDummy : NSObject, UIServerBrowser_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	func dataUpdatedHostName(_ value: String) { print(#function) }
	func dataUpdatedPortNumber(_ value: String) { print(#function) }
	func dataUpdatedProtocol(_ value: Session_Protocol) { print(#function) }
	func dataUpdatedServicesListVisible(_ value: Bool) { print(#function) }
	func dataUpdatedUserID(_ value: String) { print(#function) }
	func lookUpSelectedHostName(viewModel: UIServerBrowser_Model?) { print(#function); if let definedModel = viewModel { definedModel.isLookupInProgress = true } }
	func validateHostName(viewModel: UIServerBrowser_Model) {
		print(#function)
		viewModel.isErrorInHostName = true
	}
	func validatePortNumber(viewModel: UIServerBrowser_Model) {
		print(#function)
		viewModel.isErrorInPortNumber = true
	}
	func validateUserID(viewModel: UIServerBrowser_Model) {
		print(#function)
		viewModel.isErrorInUserID = true
	}
}

public class UIServerBrowser_ServiceItemModel : NSObject, ObservableObject {

	var serviceName: String
	var hostName: String
	var portNumber: String

	@objc public init(_ serviceName: String, hostName: String, portNumber: String) {
		self.serviceName = serviceName
		self.hostName = hostName
		self.portNumber = portNumber
	}

}

public class UIServerBrowser_Model : UICommon_BaseModel, ObservableObject {

	@Published @objc public var connectionProtocol = Session_Protocol.SSH2 {
		didSet { ifWritebackEnabled { runner.dataUpdatedProtocol(connectionProtocol) } }
	}
	@Published @objc public var isNearbyServicesListVisible = false {
		didSet { ifWritebackEnabled { runner.dataUpdatedServicesListVisible(isNearbyServicesListVisible) } }
	}
	@Published @objc public var hostName = "" {
		didSet { ifWritebackEnabled { runner.dataUpdatedHostName(hostName) } }
	}
	@Published @objc public var isErrorInHostName = false
	@Published @objc public var isLookupInProgress = false
	@Published @objc public var portNumber = "" {
		didSet { ifWritebackEnabled { runner.dataUpdatedPortNumber(portNumber) } }
	}
	@Published @objc public var isErrorInPortNumber = false
	@Published @objc public var userID = "" {
		didSet { ifWritebackEnabled { runner.dataUpdatedUserID(userID) } }
	}
	@Published @objc public var isErrorInUserID = false
	@Published @objc public var recentHostsArray: [String] = []
	@Published @objc public var serverArray: [UIServerBrowser_ServiceItemModel] = []
	@Published @objc public var selectedServer: UIServerBrowser_ServiceItemModel?
	public var runner: UIServerBrowser_ActionHandling

	@objc public init(runner: UIServerBrowser_ActionHandling) {
		self.runner = runner
	}

}

public struct UIServerBrowser_ServiceItemView : View {

	@EnvironmentObject private var itemModel: UIServerBrowser_ServiceItemModel

	public var body: some View {
		HStack(
			alignment: .top
		) {
			Text(itemModel.serviceName)
				.frame(minWidth: 200, maxWidth: 200, alignment: .leading)
			Text(itemModel.hostName)
				.frame(maxWidth: .infinity, alignment: .leading)
			Text(itemModel.portNumber)
				.frame(minWidth: 80, maxWidth: 80, alignment: .leading)
		}.background(Color.white.opacity(0.001)) // IMPORTANT: ensures that double-click works anywhere in the cell
	}

}

public struct UIServerBrowser_View : View {

	@EnvironmentObject private var viewModel: UIServerBrowser_Model

	func localizedLabelView(_ forType: Session_Protocol) -> some View {
		var aTitle: String = ""
		switch forType {
		case .SFTP:
			aTitle = "SFTP"
		case .SSH1:
			aTitle = "SSH Version 1"
		case .SSH2:
			aTitle = "SSH Version 2"
		}
		return Text(aTitle).tag(forType)
	}

	public var body: some View {
		VStack(
			alignment: .leading
		) {
			Spacer().asMacTermSectionSpacingV().fixedSize()
			UICommon_OptionLineView("Connection", noDefaultSpacing: true) {
				HStack(
					alignment: .firstTextBaseline
				) {
					Picker("", selection: $viewModel.connectionProtocol) {
						//localizedLabelView(.SSH1) // no longer available on modern macOS versions
						localizedLabelView(.SSH2)
						localizedLabelView(.SFTP)
					}.pickerStyle(PopUpButtonPickerStyle())
						.labelsHidden()
						.frame(minWidth: 288, maxWidth: 288)
						.macTermToolTipText("The protocol to use for the connection (for example, secure shell or secure file transfer).")
					Button(action: {
						withAnimation {
							if viewModel.isNearbyServicesListVisible {
								viewModel.isNearbyServicesListVisible = false
							} else {
								viewModel.isNearbyServicesListVisible = true
							}
						}
					}) {
						Text("Find a Server…")
					}.asMacTermKeypadKeyCompactTitleExactWidth(120, selected: viewModel.isNearbyServicesListVisible)
						.macTermToolTipText("Show list of nearby servers for this connection type.")
				}
			}
			Spacer().asMacTermSectionSpacingV().fixedSize()
			UICommon_OptionLineView("Host", noDefaultSpacing: true) {
				HStack(
					alignment: .firstTextBaseline
				) {
					ZStack {
						TextField("", text: $viewModel.hostName,
							onCommit: {
								viewModel.runner.validateHostName(viewModel: viewModel)
								if !viewModel.isErrorInHostName && !viewModel.hostName.isEmpty && !viewModel.recentHostsArray.contains(viewModel.hostName) {
									viewModel.recentHostsArray.append(viewModel.hostName)
								}
							})
							.onAppear {
								viewModel.runner.validateHostName(viewModel: viewModel)
							}
							.disableAutocorrection(true)
							.fixedSize(horizontal: false, vertical: true)
							.frame(minWidth: 200, maxWidth: .infinity)
							.macTermToolTipText("The domain name or IP address (v4 or v6) of the server.")
						HStack {
							Spacer()
							if #available(macOS 11.0, *) {
								Menu("") {
									ForEach(viewModel.recentHostsArray, id: \.self) { hostName in
										Button(hostName) {
											viewModel.hostName = hostName
										}
									}
								}.menuStyle(BorderlessButtonMenuStyle())
									.disabled(viewModel.recentHostsArray.isEmpty)
									.frame(minWidth: 16, maxWidth: 16)
							} else {
								MenuButton("") {
									ForEach(viewModel.recentHostsArray, id: \.self) { hostName in
										Button(hostName) {
											viewModel.hostName = hostName
										}
									}
								}.labelsHidden()
									.disabled(viewModel.recentHostsArray.isEmpty)
									.frame(maxWidth: 16)
									.offset(x: -12, y: 0) // account for UI difference in Catalina
									.controlSize(.small)
							}
						}.padding([.trailing], 4)
							.macTermToolTipText("Recently selected host names.")
					}
					UICommon_FieldErrorView($viewModel.isErrorInHostName, toolTipText: UIServerBrowser_ErrorStringBadHost)
					Button(action: {
						if viewModel.isLookupInProgress {
							viewModel.isLookupInProgress = false
							viewModel.runner.lookUpSelectedHostName(viewModel: nil)
						} else {
							viewModel.isLookupInProgress = true // initially...
							viewModel.runner.lookUpSelectedHostName(viewModel: viewModel)
						}
					}) {
						Text("Look Up")
					}.asMacTermKeypadKeyCompactTitleExactWidth(80, selected: viewModel.isLookupInProgress)
						.disabled(viewModel.hostName.isEmpty)
						.macTermToolTipText("Resolve domain name to its current IP address, replacing the contents of the Host field and updating the command line appropriately.")
					if viewModel.isLookupInProgress {
						UICommon_ProgressCircleView()
							.alignmentGuide(.firstTextBaseline, computeValue: { $0[.bottom] - 3 })
					} else {
						UICommon_ProgressCircleView()
							.alignmentGuide(.firstTextBaseline, computeValue: { $0[.bottom] - 3 })
							.hidden()
					}
				}
			}
			UICommon_OptionLineView("Port", noDefaultSpacing: true) {
				HStack(
					alignment: .firstTextBaseline
				) {
					TextField("", text: $viewModel.portNumber,
						onCommit: {
							viewModel.runner.validatePortNumber(viewModel: viewModel)
						})
						.onAppear {
							viewModel.runner.validatePortNumber(viewModel: viewModel)
						}
						.disableAutocorrection(true)
						.fixedSize(horizontal: false, vertical: true)
						.frame(minWidth: 80, maxWidth: 80)
						.macTermToolTipText("The TCP port from 0 to 65535 to use for the connection (for many servers, the default port for the protocol is the correct one).")
					UICommon_FieldErrorView($viewModel.isErrorInPortNumber, toolTipText: UIServerBrowser_ErrorStringBadPort)
				}
			}
			UICommon_OptionLineView("User ID", noDefaultSpacing: true) {
				HStack(
					alignment: .firstTextBaseline
				) {
					TextField("", text: $viewModel.userID,
						onCommit: {
							viewModel.runner.validateUserID(viewModel: viewModel)
						})
						.onAppear {
							viewModel.runner.validateUserID(viewModel: viewModel)
						}
						.disableAutocorrection(true)
						.fixedSize(horizontal: false, vertical: true)
						.frame(minWidth: 80, maxWidth: 80)
						.macTermToolTipText("The log-in name for an account on the remote server.  (Use of secure keys is recommended.  If a password is required, the remote server will prompt for it later.)")
					UICommon_FieldErrorView($viewModel.isErrorInUserID, toolTipText: UIServerBrowser_ErrorStringBadUser)
					Text("(optional)")
				}
			}
			UICommon_OptionLineView("", noDefaultSpacing: true) {
				HStack(
					alignment: .firstTextBaseline
				) {
					Text((viewModel.isErrorInHostName ? UIServerBrowser_ErrorStringBadHost
							: (viewModel.isErrorInPortNumber ? UIServerBrowser_ErrorStringBadPort
								: (viewModel.isErrorInUserID ? UIServerBrowser_ErrorStringBadUser
									: ""))))
						.controlSize(.small)
						.fixedSize(horizontal: false, vertical: true)
						.lineLimit(3)
						.multilineTextAlignment(.leading)
						.frame(maxWidth: 500, alignment: .leading)
						.padding([.top], 4)
				}
			}
			if viewModel.isNearbyServicesListVisible {
				Group {
					Spacer().asMacTermSectionSpacingV().fixedSize()
					HStack(
						alignment: .firstTextBaseline
					) {
						Text("Double-click an entry in the list to apply it to the fields above.")
							.controlSize(.small)
							.padding([.leading], 16)
					}.withMacTermSectionLayout()
					HStack(
						alignment: .firstTextBaseline
					) {
						List(selection: $viewModel.selectedServer) {
							HStack {
								// note: should match layout of UIServerBrowser_ItemView
								Text("Service Name")
									.bold()
									.frame(minWidth: 200, maxWidth: 200, alignment: .leading)
								Text("Host Name")
									.bold()
									.frame(maxWidth: .infinity, alignment: .leading)
								Text("Port")
									.bold()
									.frame(minWidth: 80, maxWidth: 80, alignment: .leading)
							}.controlSize(.small)
							Divider()
							ForEach(viewModel.serverArray, id: \.self) { item in
								UIServerBrowser_ServiceItemView().environmentObject(item)
									.onTapGesture(count: 2) {
										// on double-click, apply details of selected server to other fields
										withAnimation {
											viewModel.hostName = item.hostName
											viewModel.portNumber = item.portNumber
											// auto-hide after selection
											viewModel.isNearbyServicesListVisible = false
										}
									}
							}
						}.frame(minWidth: 200, minHeight: 200, idealHeight: 200)
					}.withMacTermSectionLayout()
				}.animation(.interactiveSpring())
			}
			Spacer().asMacTermSectionSpacingV().fixedSize()
			//Spacer().layoutPriority(1)
		}
	}

}

public class UIServerBrowser_ObjC : NSObject {

	@objc public static func makeView(_ data: UIServerBrowser_Model) -> NSView {
		return NSHostingView<AnyView>(rootView: AnyView(UIServerBrowser_View().environmentObject(data)))
	}

}

public struct UIServerBrowser_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIServerBrowser_Model(runner: UIServerBrowser_RunnerDummy())
		//data.recentHostsArray.append("...")
		//data.recentHostsArray.append("...")
		data.serverArray.append(UIServerBrowser_ServiceItemModel("Sample Service Name", hostName: "nowhere.loopback.edu", portNumber: "22"))
		data.serverArray.append(UIServerBrowser_ServiceItemModel("123.45.67.123", hostName: "123.45.67.123", portNumber: "123"))
		return VStack {
			UIServerBrowser_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data)
			UIServerBrowser_View().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data)
		}
	}
}
