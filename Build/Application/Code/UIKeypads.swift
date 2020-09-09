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

extension Button {
	// set specific dimensions for square keypad keys
	public func asMacTermKeypadKeySquare() -> some View {
		let styleObject = UIKeypads_KeyButtonStyle()
		return frame(minWidth: styleObject.width, maxWidth: styleObject.width, minHeight: styleObject.height, maxHeight: styleObject.height)
						.buttonStyle(styleObject)
	}
	// set specific dimensions for square keypad keys but enlarge text
	public func asMacTermKeypadKeySquareLargeFont() -> some View {
		let styleObject = UIKeypads_KeyButtonStyle(fontSize: 20)
		return frame(minWidth: styleObject.width, maxWidth: styleObject.width, minHeight: styleObject.height, maxHeight: styleObject.height)
						.buttonStyle(styleObject)
	}
	// horizontal key button (e.g. keypad “0”)
	public func asMacTermKeypadKeyRect2HLargeFont() -> some View {
		let styleObject = UIKeypads_KeyButtonStyle(fontSize: 20, narrowWidth: false, narrowHeight: true)
		return frame(minWidth: styleObject.width, maxWidth: styleObject.width, minHeight: styleObject.height, maxHeight: styleObject.height)
			.buttonStyle(styleObject)
	}
	// vertical key button (e.g. keypad “Enter”)
	public func asMacTermKeypadKeyRect2VLargeFont() -> some View {
		let styleObject = UIKeypads_KeyButtonStyle(fontSize: 20, narrowWidth: true, narrowHeight: false)
		return frame(minWidth: styleObject.width, maxWidth: styleObject.width, minHeight: styleObject.height, maxHeight: styleObject.height)
			.buttonStyle(styleObject)
	}
}

extension Spacer {
	// occupy the same space as a normal square key
	public func asMacTermKeypadKeySquareSpace() -> some View {
		let styleObject = UIKeypads_KeyButtonStyle()
		return frame(minWidth: styleObject.width, maxWidth: styleObject.width, minHeight: styleObject.height, maxHeight: styleObject.height)
	}
	// separate each key from the one above it
	public func asMacTermKeypadKeySpacingV() -> some View {
		padding([.top], 1)
	}
	// separate each key from the one next to it
	public func asMacTermKeypadKeySpacingH() -> some View {
		padding([.trailing], 1)
	}
}

extension Text {
	// for keypad keys with more than one label, e.g. control keys
	public func asMacTermKeySecondaryLabel() -> some View {
		font(Font.system(size: 10))
	}
}

@objc public protocol UIKeypads_ActionHandling : NSObjectProtocol {
	// implement these functions to bind to button actions
	func respondToAction(viewModel: UIKeypads_Model, keyID: UIKeypads_KeyID)
	func saveChanges(viewModel: UIKeypads_Model)
}

class UIKeypads_RunnerDummy : NSObject, UIKeypads_ActionHandling {
	// dummy used for debugging in playground (just prints function that is called)
	public func respondToAction(viewModel: UIKeypads_Model, keyID: UIKeypads_KeyID) {
		let functionName = #function
		let keyNumber = keyID.rawValue
		print("\(functionName): pressed keyID #\(keyNumber)'")
	}
	public func saveChanges(viewModel: UIKeypads_Model) {
		print(#function)
	}
}

public enum UIKeypads_ControlKeyLabels : Int {
	case plain // just the control key
	case keyNames // control key with subtitle showing purpose, e.g. "BEL"
	case keyCodesHex // control key with subtitle showing hex ASCII code, e.g. "0x07"
}

@objc public enum UIKeypads_FunctionKeyLayout : Int {
	case vt220 // VT220 and multi-gnome-terminal: F1-F4 = PF1-PF4, F15 = help ("?"), F16 = "do"
	case xtermX11 // XTerm on X11: 48 keys, all normal names
	case xtermXFree86 // XTerm on XFree86 / gnome-terminal / GNU screen: 48 keys, F1-F4 = PF1-PF4
	case rxvt // rxvt: 48 keys, F15 = help ("?"), F16 = "do"
}

@objc public enum UIKeypads_KeyID : Int {
	case arrowDown
	case arrowLeft
	case arrowRight
	case arrowUp
	case controlNull // ⌃@
	case controlA
	case controlB
	case controlC
	case controlD
	case controlE
	case controlF
	case controlG
	case controlH
	case controlI
	case controlJ
	case controlK
	case controlL
	case controlM
	case controlN
	case controlO
	case controlP
	case controlQ
	case controlR
	case controlS
	case controlT
	case controlU
	case controlV
	case controlW
	case controlX
	case controlY
	case controlZ
	case controlLeftSquareBracket // ⌃[
	case controlBackslash // ⌃\
	case controlRightSquareBracket // ⌃]
	case controlCaret // ⌃^
	case controlUnderscore // ⌃_
	case f1
	case f2
	case f3
	case f4
	case f5
	case f6
	case f7
	case f8
	case f9
	case f10
	case f11
	case f12
	case f13
	case f14
	case f15
	case f16
	case f17
	case f18
	case f19
	case f20
	case f21
	case f22
	case f23
	case f24
	case f25
	case f26
	case f27
	case f28
	case f29
	case f30
	case f31
	case f32
	case f33
	case f34
	case f35
	case f36
	case f37
	case f38
	case f39
	case f40
	case f41
	case f42
	case f43
	case f44
	case f45
	case f46
	case f47
	case f48
	case keypad0
	case keypad1
	case keypad2
	case keypad3
	case keypad4
	case keypad5
	case keypad6
	case keypad7
	case keypad8
	case keypad9
	case keypadComma
	case keypadDecimalPoint
	case keypadDelete
	case keypadEnter
	case keypadFind
	case keypadHyphen
	case keypadInsert
	case keypadPageDown
	case keypadPageUp
	case keypadPF1
	case keypadPF2
	case keypadPF3
	case keypadPF4
	case keypadSelect
}

public class UIKeypads_Model : NSObject, ObservableObject {

	public var runner: UIKeypads_ActionHandling
	@Published @objc public var buttonsDisabled = false
	@Published public var controlKeyLabels = UIKeypads_ControlKeyLabels.plain
	@Published @objc public var functionKeyLayout = UIKeypads_FunctionKeyLayout.vt220 {
		didSet {
			runner.saveChanges(viewModel: self)
		}
	}

	@objc public init(runner: UIKeypads_ActionHandling) {
		self.runner = runner
	}

}

struct UIKeypads_KeyButtonStyle : ButtonStyle {

	public var fontSize: CGFloat
	public var width: CGFloat
	public var height: CGFloat

	init() {
		self.init(fontSize: 14, narrowWidth: true, narrowHeight: true)
	}

	init(fontSize: CGFloat) {
		self.init(fontSize: fontSize, narrowWidth: true, narrowHeight: true)
	}

	init(narrowWidth: Bool, narrowHeight: Bool) {
		self.init(fontSize: 12, narrowWidth: narrowWidth, narrowHeight: narrowHeight)
	}

	init(fontSize: CGFloat, narrowWidth: Bool, narrowHeight: Bool) {
		self.fontSize = fontSize
		self.width = narrowWidth ? 36 : 82
		self.height = narrowHeight ? 36 : 82
	}

	func makeBody(configuration: Self.Configuration) -> some View {
		UIKeypads_KeyButton(configuration: configuration, fontSize: fontSize, width: width, height: height)
	}

	private struct UIKeypads_KeyButton : View {

		@Environment(\.colorScheme) var colorScheme
		@Environment(\.isEnabled) var viewEnabled

		let configuration: UIKeypads_KeyButtonStyle.Configuration
		let fontSize: CGFloat
		let width: CGFloat
		let height: CGFloat

		var body: some View {
			// make a button that looks like a keyboard key, and handle all modes (Dark/Light)
			// and all states (normal, pressed, disabled)
			let drawAsDark = (colorScheme == .dark)
			let drawAsEnabled = (viewEnabled == true)
			let drawAsPressed = (configuration.isPressed == true)
			let borderColor = drawAsDark
								? (drawAsEnabled
									? (drawAsPressed
										? Color(red: 0.8, green: 0.8, blue: 0.8)
										: Color(red: 0.5, green: 0.5, blue: 0.5))
									: Color(red: 0.4, green: 0.4, blue: 0.4))
								: (drawAsEnabled
									? (drawAsPressed
										? Color(red: 0.5, green: 0.5, blue: 0.5)
										: Color(red: 0.6, green: 0.6, blue: 0.6))
									: Color(red: 0.7, green: 0.7, blue: 0.7))
			let textColor = drawAsDark
							? (drawAsEnabled
								? (drawAsPressed
									? Color(red: 0.9, green: 0.9, blue: 0.9)
									: Color(red: 0.8, green: 0.8, blue: 0.8))
								: Color(red: 0.4, green: 0.4, blue: 0.4))
							: (drawAsEnabled
								? (drawAsPressed
									? Color(red: 0.0, green: 0.0, blue: 0.0)
									: Color(red: 0.2, green: 0.2, blue: 0.2))
								: Color(red: 0.7, green: 0.7, blue: 0.7))
			let buttonGradientLightNormal = Gradient(stops: [
					Gradient.Stop(color: Color(red: 0.97, green: 0.97, blue: 0.97), location: 0.0),
					Gradient.Stop(color: Color(red: 0.99, green: 0.99, blue: 0.99), location: 0.2),
					Gradient.Stop(color: Color(red: 0.85, green: 0.85, blue: 0.85), location: 1.0)
				])
			let buttonGradientDarkNormal = Gradient(stops: [
					Gradient.Stop(color: Color(red: 0.37, green: 0.37, blue: 0.37), location: 0.0),
					Gradient.Stop(color: Color(red: 0.39, green: 0.39, blue: 0.39), location: 0.2),
					Gradient.Stop(color: Color(red: 0.3, green: 0.3, blue: 0.3), location: 0.95)
				])
			let buttonGradientLightPressed = Gradient(stops: [
					Gradient.Stop(color: Color(red: 0.8, green: 0.8, blue: 0.8), location: 0.2),
					Gradient.Stop(color: Color(red: 0.6, green: 0.6, blue: 0.6), location: 0.95)
				])
			let buttonGradientDarkPressed = Gradient(stops: [
					Gradient.Stop(color: Color(red: 0.6, green: 0.6, blue: 0.6), location: 0.2),
					Gradient.Stop(color: Color(red: 0.5, green: 0.5, blue: 0.5), location: 0.95)
				])
			let buttonGradientLightDisabled = Gradient(stops: [
					Gradient.Stop(color: Color(red: 0.8, green: 0.8, blue: 0.8), location: 0.2)
				])
			let buttonGradientDarkDisabled = Gradient(stops: [
					Gradient.Stop(color: Color(red: 0.2, green: 0.2, blue: 0.2), location: 0.2)
				])
			let buttonGradientNormal = drawAsDark ? buttonGradientDarkNormal : buttonGradientLightNormal
			let buttonGradientPressed = drawAsDark ? buttonGradientDarkPressed : buttonGradientLightPressed
			let buttonGradientDisabled = drawAsDark ? buttonGradientDarkDisabled : buttonGradientLightDisabled
			let buttonEnabledBackground = LinearGradient(gradient: drawAsPressed ? buttonGradientPressed : buttonGradientNormal,
															startPoint: UnitPoint(x: width > height ? 0.2 : 0.5, y: 0),
															endPoint: UnitPoint(x: height > width ? 0.9 : 0.8, y: 1.0))
			let buttonDisabledBackground = LinearGradient(gradient: buttonGradientDisabled,
															startPoint: UnitPoint(x: width > height ? 0.2 : 0.5, y: 0),
															endPoint: UnitPoint(x: height > width ? 0.9 : 0.8, y: 1.0))
			return configuration.label
				.frame(minWidth: self.width, maxWidth: self.width,
						minHeight: self.height, maxHeight: self.height)
				.animation(.spring())
				.background(
					ZStack {
						RoundedRectangle(cornerRadius: 3.5, style: .continuous)
							.inset(by: 0)
							.fill(drawAsEnabled ? buttonEnabledBackground : buttonDisabledBackground)
						RoundedRectangle(cornerRadius: 3, style: .continuous)
							.stroke(borderColor, lineWidth: 1)
							.blendMode(.normal)
					}
				)
				.focusable(drawAsEnabled) // TEMPORARY; note, this appears to create correct focus ring but space-bar activation does nothing (SwiftUI 1.0 bug?); revisit in next SDK 
				.font(.system(size: fontSize, weight: .bold, design: .default))
				.foregroundColor(textColor)
				.scaleEffect(drawAsPressed ? 0.95 : 1)
		}
	}

}

public struct UIKeypads_ControlKeysView : View {

	@EnvironmentObject private var viewModel: UIKeypads_Model
	// some display options for future use; disabling for now (TEMPORARY)
	private var showKeyNames = false // if set, keys have abbreviated control key names below main label
	private var showKeyCodes = false // (ignored if "showKeyNames" is true) keys have ASCII code values in small text below main label

	public var body: some View {
		VStack (alignment: .leading) {
			Spacer().asMacTermKeypadKeySpacingV()
			HStack {
				// maximum 10 items per stack; use Group to build more
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlNull) }) {
						VStack {
							Text("⌃@")
							if showKeyNames {
								Text("NUL").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x00").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("NUL (0x00) — Filler") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlA) }) {
						VStack {
							Text("⌃A")
							if showKeyNames {
								Text("SOH").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x01").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("SOH (0x01) — Start of Heading") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlB) }) {
						VStack {
							Text("⌃B")
							if showKeyNames {
								Text("STX").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x02").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("STX (0x02) — Start of Text") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlC) }) {
						VStack {
							Text("⌃C")
							if showKeyNames {
								Text("ETX").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x03").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("ETX (0x03) — End of Text") // (add when SDK is updated)
				}
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlD) }) {
						VStack {
							Text("⌃D")
							if showKeyNames {
								Text("EOT").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x04").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("EOT (0x04) — End of Transmission") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlE) }) {
						VStack {
							Text("⌃E")
							if showKeyNames {
								Text("ENQ").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x05").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("ENQ (0x05) — Enquiry") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlF) }) {
						VStack {
							Text("⌃F")
							if showKeyNames {
								Text("ACK").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x06").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("ACK (0x06) — Acknowledge") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlG) }) {
						VStack {
							Text("⌃G")
							if showKeyNames {
								Text("BEL").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x07").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("BEL (0x07) — Bell Sound") // (add when SDK is updated)
				}
				Spacer().asMacTermKeypadKeySpacingH()
			}
			Spacer().asMacTermKeypadKeySpacingV()
			HStack {
				// maximum 10 items per stack; use Group to build more
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlH) }) {
						VStack {
							Text("⌃H")
							if showKeyNames {
								Text("BS").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x08").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("BS (0x08) — Backspace") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlI) }) {
						VStack {
							Text("⌃I")
							if showKeyNames {
								Text("HT").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x09").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("HT (0x09) — Horizontal Tabulation") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlJ) }) {
						VStack {
							Text("⌃J")
							if showKeyNames {
								Text("LF").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x0A").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("NL / LF (0x0A) — New Line / Line Feed") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlK) }) {
						VStack {
							Text("⌃K")
							if showKeyNames {
								Text("VT").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x0B").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("VT (0x0B) — Vertical Tabulation") // (add when SDK is updated)
				}
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlL) }) {
						VStack {
							Text("⌃L")
							if showKeyNames {
								Text("FF").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x0C").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("NP / FF (0x0C) — New Page / Form Feed") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlM) }) {
						VStack {
							Text("⌃M")
							if showKeyNames {
								Text("CR").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x0D").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("CR (0x0D) — Carriage Return") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlN) }) {
						VStack {
							Text("⌃N")
							if showKeyNames {
								Text("SO").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x0E").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("SO (0x0E) — Shift Out (Character Set)") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlO) }) {
						VStack {
							Text("⌃O")
							if showKeyNames {
								Text("SI").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x0F").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("SI (0x0F) — Shift In (Character Set)") // (add when SDK is updated)
				}
				Spacer().asMacTermKeypadKeySpacingH()
			}
			Spacer().asMacTermKeypadKeySpacingV()
			HStack {
				// maximum 10 items per stack; use Group to build more
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlP) }) {
						VStack {
							Text("⌃P")
							if showKeyNames {
								Text("DLE").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x10").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("DLE (0x10) — Data Link Escape") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlQ) }) {
						VStack {
							Text("⌃Q")
							if showKeyNames {
								//Text("DC1").asMacTermKeySecondaryLabel()
								Text("XON").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x11").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("DC1 (0x11) — XON / Device Control 1") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlR) }) {
						VStack {
							Text("⌃R")
							if showKeyNames {
								Text("DC2").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x12").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("DC2 (0x12) — Device Control 2") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlS) }) {
						VStack {
							Text("⌃S")
							if showKeyNames {
								//Text("DC3").asMacTermKeySecondaryLabel()
								Text("XOFF").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x13").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("DC3 (0x13) — XOFF / Device Control 3") // (add when SDK is updated)
				}
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlT) }) {
						VStack {
							Text("⌃T")
							if showKeyNames {
								Text("DC4").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x14").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("DC4 (0x14) — Device Control 4") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlU) }) {
						VStack {
							Text("⌃U")
							if showKeyNames {
								Text("NAK").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x15").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("NAK (0x15) — Negative Acknowledge") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlV) }) {
						VStack {
							Text("⌃V")
							if showKeyNames {
								Text("SYN").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x16").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("SYN (0x16) — Synchronous Idle") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlW) }) {
						VStack {
							Text("⌃W")
							if showKeyNames {
								Text("ETB").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x17").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("ETB (0x17) — End of Transmission Block") // (add when SDK is updated)
				}
				Spacer().asMacTermKeypadKeySpacingH()
			}
			Spacer().asMacTermKeypadKeySpacingV()
			HStack {
				// maximum 10 items per stack; use Group to build more
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlX) }) {
						VStack {
							Text("⌃X")
							if showKeyNames {
								Text("CAN").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x18").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("CAN (0x18) — Cancel") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlY) }) {
						VStack {
							Text("⌃Y")
							if showKeyNames {
								Text("EM").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x19").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("EM (0x19) — End of Medium") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlZ) }) {
						VStack {
							Text("⌃Z")
							if showKeyNames {
								Text("SUB").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x1A").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("SUB (0x1A) — Substitute") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlLeftSquareBracket) }) {
						VStack {
							Text("⌃[")
							if showKeyNames {
								Text("ESC").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x1B").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("ESC (0x1B) — Escape") // (add when SDK is updated)
				}
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlBackslash) }) {
						VStack {
							Text("⌃\\")
							if showKeyNames {
								Text("FS").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x1C").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("FS (0x1C) — Field Separator") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlRightSquareBracket) }) {
						VStack {
							Text("⌃]")
							if showKeyNames {
								Text("GS").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x1D").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("GS (0x1D) — Group Separator") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlCaret) }) {
						VStack {
							Text("⌃^")
							if showKeyNames {
								Text("RS").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x1E").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("RS (0x1E) — Record Separator") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .controlUnderscore) }) {
						VStack {
							Text("⌃_")
							if showKeyNames {
								Text("US").asMacTermKeySecondaryLabel()
							} else if showKeyCodes {
								Text("0x1F").asMacTermKeySecondaryLabel()
							}
						}
					}.asMacTermKeypadKeySquare()
						//.help("US (0x1F) — Unit Separator") // (add when SDK is updated)
				}
				Spacer().asMacTermKeypadKeySpacingH()
			}
			Spacer().asMacTermKeypadKeySpacingV()
		}.padding(0)
		.disabled(self.viewModel.buttonsDisabled)
	}

}

public struct UIKeypads_FunctionKeyLayoutMenu : View {

	@EnvironmentObject private var viewModel: UIKeypads_Model
	private var isOnLayoutVT220: Binding<Bool> {
		Binding<Bool>(get: { self.viewModel.functionKeyLayout == .vt220 }, set: { (Bool) -> Void in self.viewModel.functionKeyLayout = .vt220 })
	}
	private var isOnLayoutXTermX11: Binding<Bool> {
		Binding<Bool>(get: { self.viewModel.functionKeyLayout == .xtermX11 }, set: { (Bool) -> Void in self.viewModel.functionKeyLayout = .xtermX11 })
	}
	private var isOnLayoutXTermXFree86: Binding<Bool> {
		Binding<Bool>(get: { self.viewModel.functionKeyLayout == .xtermXFree86 }, set: { (Bool) -> Void in self.viewModel.functionKeyLayout = .xtermXFree86 })
	}
	private var isOnLayoutRxvt: Binding<Bool> {
		Binding<Bool>(get: { self.viewModel.functionKeyLayout == .rxvt }, set: { (Bool) -> Void in self.viewModel.functionKeyLayout = .rxvt })
	}

	public var body: some View {
		MenuButton("") {
			//Text("Function Key Layout:").bold().disabled(true)
			HStack { Spacer(minLength: 8); Toggle("VT220 / “multi-gnome-terminal”", isOn: isOnLayoutVT220) }
			HStack { Spacer(minLength: 8); Toggle("XTerm (X11)", isOn: isOnLayoutXTermX11) }
			HStack { Spacer(minLength: 8); Toggle("XTerm (XFree86) / “gnome-terminal” / GNU “screen”", isOn: isOnLayoutXTermXFree86) }
			HStack { Spacer(minLength: 8); Toggle("“rxvt”", isOn: isOnLayoutRxvt) }
		}.menuButtonStyle(BorderlessPullDownMenuButtonStyle())
			.controlSize(.small)
			.fixedSize()
	}

}

public struct UIKeypads_FunctionKeysView : View {

	@EnvironmentObject private var viewModel: UIKeypads_Model

	public var body: some View {
		var nameF1 = "F1"
		var nameF2 = "F2"
		var nameF3 = "F3"
		var nameF4 = "F4"
		var nameF15 = "F15"
		var nameF16 = "F16"
		var is48Keys = true
		if self.viewModel.functionKeyLayout == .vt220 ||
			self.viewModel.functionKeyLayout == .xtermXFree86 {
			nameF1 = "PF1"
			nameF2 = "PF2"
			nameF3 = "PF3"
			nameF4 = "PF4"
		}
		if self.viewModel.functionKeyLayout == .vt220 ||
			self.viewModel.functionKeyLayout == .rxvt {
			nameF15 = "?"
			nameF16 = "do"
		}
		if self.viewModel.functionKeyLayout == .vt220 {
			is48Keys = false
		}
		return VStack {
			/*if true {
				// redundant UI to toggle view type (there are also
				// commands in the menu bar to do this)
				HStack {
					Spacer()
					UIKeypads_FunctionKeyLayoutMenu().environmentObject(viewModel)
					Spacer()
				}
			}*/
			Spacer().asMacTermKeypadKeySpacingV()
			HStack {
				// maximum 10 items per stack; use Group to build more
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Group {
						Button(nameF1, action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f1) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button(nameF2, action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f2) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button(nameF3, action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f3) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button(nameF4, action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f4) })
							.asMacTermKeypadKeySquare()
					}
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySpacingH()
					Group {
						Button("F5", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f5) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F6", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f6) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F7", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f7) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F8", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f8) })
							.asMacTermKeypadKeySquare()
					}
				}
				Spacer().asMacTermKeypadKeySpacingH()
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Group {
						Button("F9", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f9) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F10", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f10) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F11", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f11) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F12", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f12) })
							.asMacTermKeypadKeySquare()
					}
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySpacingH()
					Group {
						Button("F13", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f13) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F14", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f14) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button(nameF15, action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f15) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button(nameF16, action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f16) })
							.asMacTermKeypadKeySquare()
					}
				}
				Spacer().asMacTermKeypadKeySpacingH()
				Spacer().asMacTermKeypadKeySpacingH()
				Group {
					Group {
						Button("F17", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f17) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F18", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f18) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F19", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f19) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F20", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f20) })
							.asMacTermKeypadKeySquare()
					}
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySpacingH()
					Group {
						Button("F21", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f21) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F22", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f22) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F23", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f23) })
							.asMacTermKeypadKeySquare()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("F24", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f24) })
							.asMacTermKeypadKeySquare()
					}
				}
				Spacer().asMacTermKeypadKeySpacingH()
			}
			//if is48Keys {
			if true {
				Spacer().asMacTermKeypadKeySpacingV()
				HStack {
					// maximum 10 items per stack; use Group to build more
					Spacer().asMacTermKeypadKeySpacingH()
					Group {
						Group {
							Button("F25", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f25) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F26", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f26) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F27", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f27) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F28", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f28) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
						}
						Spacer().asMacTermKeypadKeySpacingH()
						Spacer().asMacTermKeypadKeySpacingH()
						Group {
							Button("F29", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f29) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F30", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f30) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F31", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f31) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F32", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f32) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
						}
					}
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySpacingH()
					Group {
						Group {
							Button("F33", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f33) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F34", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f34) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F35", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f35) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F36", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f36) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
						}
						Spacer().asMacTermKeypadKeySpacingH()
						Spacer().asMacTermKeypadKeySpacingH()
						Group {
							Button("F37", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f37) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F38", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f38) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F39", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f39) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F40", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f40) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
						}
					}
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySpacingH()
					Group {
						Group {
							Button("F41", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f41) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F42", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f42) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F43", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f43) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F44", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f44) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
						}
						Spacer().asMacTermKeypadKeySpacingH()
						Spacer().asMacTermKeypadKeySpacingH()
						Group {
							Button("F45", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f45) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F46", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f46) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F47", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f47) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
							Spacer().asMacTermKeypadKeySpacingH()
							Button("F48", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .f48) })
								.asMacTermKeypadKeySquare()
								.disabled(false == is48Keys)
						}
					}
					Spacer().asMacTermKeypadKeySpacingH()
				}
			}
			Spacer().asMacTermKeypadKeySpacingV()
		}.padding(0)
		.disabled(self.viewModel.buttonsDisabled)
	}

}

public struct UIKeypads_VT220KeysView : View {

	@EnvironmentObject private var viewModel: UIKeypads_Model

	public var body: some View {
		VStack (alignment: .leading) {
			Spacer().asMacTermKeypadKeySpacingV()
			HStack {
				// maximum 10 items per stack; use Group to build more
				Group {
					// TEMPORARY; move to SF Symbols when SDK is updated
					Spacer().asMacTermKeypadKeySpacingH()
					Button(action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadFind) }) {
						// note: this image is not visible in the Playground due to bundle source
						// but it can be tested in MacTerm itself
						Image("IconForSearch")
					}.asMacTermKeypadKeySquare()
					Spacer().asMacTermKeypadKeySpacingH()
					Button("⤵", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadInsert) })
						.asMacTermKeypadKeySquareLargeFont()
					Spacer().asMacTermKeypadKeySpacingH()
					Button("⌫", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadDelete) })
						.asMacTermKeypadKeySquareLargeFont()
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySquareSpace()
				}
				Spacer().asMacTermKeypadKeySpacingH()
				Button("PF1", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadPF1) })
					.asMacTermKeypadKeySquare()
				Spacer().asMacTermKeypadKeySpacingH()
				Button("PF2", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadPF2) })
					.asMacTermKeypadKeySquare()
				Spacer().asMacTermKeypadKeySpacingH()
				Button("PF3", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadPF3) })
					.asMacTermKeypadKeySquare()
				Spacer().asMacTermKeypadKeySpacingH()
				Button("PF4", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadPF4) })
					.asMacTermKeypadKeySquare()
				Spacer().asMacTermKeypadKeySpacingH()
			}
			Spacer().asMacTermKeypadKeySpacingV()
			HStack {
				// maximum 10 items per stack; use Group to build more
				Group {
					// TEMPORARY; move to SF Symbols when SDK is updated
					Spacer().asMacTermKeypadKeySpacingH()
					Button("☞", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadSelect) })
						.asMacTermKeypadKeySquareLargeFont()
						.accessibility(label: Text("Select"))
						//.help("Select") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button("⇞", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadPageUp) })
						.asMacTermKeypadKeySquareLargeFont()
						.accessibility(label: Text("Page Up"))
						//.help("Page Up") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Button("⇟", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadPageDown) })
						.asMacTermKeypadKeySquareLargeFont()
						.accessibility(label: Text("Page Down"))
						//.help("Page Down") // (add when SDK is updated)
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySquareSpace()
				}
				Spacer().asMacTermKeypadKeySpacingH()
				Button("7", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypad7) })
					.asMacTermKeypadKeySquareLargeFont()
				Spacer().asMacTermKeypadKeySpacingH()
				Button("8", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypad8) })
					.asMacTermKeypadKeySquareLargeFont()
				Spacer().asMacTermKeypadKeySpacingH()
				Button("9", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypad9) })
					.asMacTermKeypadKeySquareLargeFont()
				Spacer().asMacTermKeypadKeySpacingH()
				Button(",", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadComma) })
					.asMacTermKeypadKeySquareLargeFont()
					.accessibility(label: Text("Comma"))
					//.help("Comma") // (add when SDK is updated)
				Spacer().asMacTermKeypadKeySpacingH()
			}
			Spacer().asMacTermKeypadKeySpacingV()
			HStack {
				// maximum 10 items per stack; use Group to build more
				Group {
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySquareSpace()
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySquareSpace()
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySquareSpace()
					Spacer().asMacTermKeypadKeySpacingH()
					Spacer().asMacTermKeypadKeySquareSpace()
				}
				Spacer().asMacTermKeypadKeySpacingH()
				Button("4", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypad4) })
					.asMacTermKeypadKeySquareLargeFont()
				Spacer().asMacTermKeypadKeySpacingH()
				Button("5", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypad5) })
					.asMacTermKeypadKeySquareLargeFont()
				Spacer().asMacTermKeypadKeySpacingH()
				Button("6", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypad6) })
					.asMacTermKeypadKeySquareLargeFont()
				Spacer().asMacTermKeypadKeySpacingH()
				Button("-", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadHyphen) })
					.asMacTermKeypadKeySquareLargeFont()
					.accessibility(label: Text("Hyphen"))
					//.help("Hyphen") // (add when SDK is updated)
				Spacer().asMacTermKeypadKeySpacingH()
			}
			Spacer().asMacTermKeypadKeySpacingV()
			HStack {
				// maximum 10 items per stack; use Group to build more
				Group {
					Spacer().asMacTermKeypadKeySpacingH()
					VStack {
						Spacer().asMacTermKeypadKeySquareSpace()
						Spacer().asMacTermKeypadKeySpacingV()
						Button("⇠", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .arrowLeft) })
							.asMacTermKeypadKeySquareLargeFont()
							.accessibility(label: Text("Left Arrow"))
					}
					Spacer().asMacTermKeypadKeySpacingH()
					VStack {
						Button("⇡", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .arrowUp) })
							.asMacTermKeypadKeySquareLargeFont()
							.accessibility(label: Text("Up Arrow"))
						Spacer().asMacTermKeypadKeySpacingV()
						Button("⇣", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .arrowDown) })
							.asMacTermKeypadKeySquareLargeFont()
							.accessibility(label: Text("Down Arrow"))
					}
					Spacer().asMacTermKeypadKeySpacingH()
					VStack {
						Spacer().asMacTermKeypadKeySquareSpace()
						Spacer().asMacTermKeypadKeySpacingV()
						Button("⇢", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .arrowRight) })
							.asMacTermKeypadKeySquareLargeFont()
							.accessibility(label: Text("Right Arrow"))
					}
					Spacer().asMacTermKeypadKeySpacingH()
					VStack {
						Spacer().asMacTermKeypadKeySquareSpace()
						Spacer().asMacTermKeypadKeySpacingV()
						Spacer().asMacTermKeypadKeySquareSpace()
					}
				}
				Spacer().asMacTermKeypadKeySpacingH()
				VStack {
					HStack {
						Button("1", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypad1) })
							.asMacTermKeypadKeySquareLargeFont()
						Spacer().asMacTermKeypadKeySpacingH()
						Button("2", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypad2) })
							.asMacTermKeypadKeySquareLargeFont()
					}
					Spacer().asMacTermKeypadKeySpacingV()
					Button("0", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypad0) })
						.asMacTermKeypadKeyRect2HLargeFont()
				}
				Spacer().asMacTermKeypadKeySpacingH()
				VStack {
					Button("3", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypad3) })
						.asMacTermKeypadKeySquareLargeFont()
					Spacer().asMacTermKeypadKeySpacingV()
					Button(".", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadDecimalPoint) })
						.asMacTermKeypadKeySquareLargeFont()
						.accessibility(label: Text("Decimal Point"))
						//.help("Decimal Point") // (add when SDK is updated)
				}
				Spacer().asMacTermKeypadKeySpacingH()
				Button("⌤", action: { self.viewModel.runner.respondToAction(viewModel: self.viewModel, keyID: .keypadEnter) })
					.asMacTermKeypadKeyRect2VLargeFont()
					.accessibility(label: Text("Enter"))
					//.help("Enter") // (add when SDK is updated)
				Spacer().asMacTermKeypadKeySpacingH()
			}
			Spacer().asMacTermKeypadKeySpacingV()
		}.padding(0)
		.disabled(self.viewModel.buttonsDisabled)
	}

}

public class UIKeypads_ObjC : NSObject {

	@objc public static func makeControlKeysViewController(_ data: UIKeypads_Model) -> NSViewController {
		return NSHostingController<AnyView>(rootView: AnyView(UIKeypads_ControlKeysView().environmentObject(data)))
	}

	@objc public static func makeFunctionKeysViewController(_ data: UIKeypads_Model) -> NSViewController {
		return NSHostingController<AnyView>(rootView: AnyView(UIKeypads_FunctionKeysView().environmentObject(data)))
	}

	@objc public static func makeVT220KeysViewController(_ data: UIKeypads_Model) -> NSViewController {
		return NSHostingController<AnyView>(rootView: AnyView(UIKeypads_VT220KeysView().environmentObject(data)))
	}

}

public struct UIKeypads_Previews : PreviewProvider {
	public static var previews: some View {
		let data = UIKeypads_Model(runner: UIKeypads_RunnerDummy())
		//data.buttonsDisabled = true // comment-in to test disabled state
		return VStack {
			UIKeypads_ControlKeysView().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data).fixedSize()
			UIKeypads_ControlKeysView().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data).fixedSize()
			UIKeypads_FunctionKeysView().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data).fixedSize()
			UIKeypads_FunctionKeysView().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data).fixedSize()
			UIKeypads_VT220KeysView().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .light).environmentObject(data).fixedSize()
			UIKeypads_VT220KeysView().background(Color(NSColor.windowBackgroundColor)).environment(\.colorScheme, .dark).environmentObject(data).fixedSize()
		}
	}
}
