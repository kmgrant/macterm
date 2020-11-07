import AppKit
import PlaygroundSupport

import MacTermQuills

// (comment-in whichever UI you want to test)
//let view = UIAddressList_Previews.previews
//let view = UIArrangeWindow_Previews.previews
//let view = UIDebugInterface_Previews.previews
//let view = UIKeypads_Previews.previews
//let view = UIPrefsTerminalOptions_Previews.previews
//let view = UIPrefsTerminalEmulation_Previews.previews
//let view = UIPrefsTerminalScreen_Previews.previews
let view = UIPrefsWorkspaceOptions_Previews.previews

NSSetUncaughtExceptionHandler { exception in
	print("EXCEPTION: \(exception)")
}

PlaygroundPage.current.setLiveView(view)
