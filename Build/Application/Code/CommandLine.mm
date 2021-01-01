/*!	\file CommandLine.mm
	\brief Cocoa implementation of the floating command line.
*/
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

#import "CommandLine.h"
#include <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <CFRetainRelease.h>
#import <CocoaExtensions.objc++.h>
#import <Console.h>
#import <SoundSystem.h>

// application includes
#import "HelpSystem.h"
#import "Preferences.h"
#import "Session.h"
#import "SessionFactory.h"
#import "UIStrings.h"



#pragma mark Types

/*!
The private class interface.
*/
@interface CommandLine_PanelController (CommandLine_PanelControllerInternal) //{

// new methods
	- (void)
	sendText:(NSString*)_
	newLine:(BOOL)_
	addingToHistory:(BOOL)_;
	- (void)
	sendTextSoFar:(NSString*)_;

@end //} CommandLine_PanelController (CommandLine_PanelControllerInternal)

/*!
The private class interface.
*/
@interface CommandLine_TerminalLikeComboBox (CommandLine_TerminalLikeComboBoxInternal) //{

// new methods
	- (void)
	synchronizeWithFormatPreferencesForFieldEditor:(NSTextView*)_;

@end //} CommandLine_TerminalLikeComboBox

#pragma mark Variables
namespace {

CommandLine_PanelController*	gCommandLine_PanelController = nil;

} // anonymous namespace



#pragma mark Public Methods

/*!
If the command line window was visible the last time the
application quit, then this will initialize and display it;
otherwise, nothing is done (deferring initialization until
the window is actually requested).

(4.0)
*/
void
CommandLine_Init ()
{
	Boolean		windowIsVisible = false;
	
	
	unless (kPreferences_ResultOK ==
			Preferences_GetData(kPreferences_TagWasCommandLineShowing,
								sizeof(windowIsVisible), &windowIsVisible))
	{
		windowIsVisible = false; // assume invisible if the preference can’t be found
	}
	
	if (windowIsVisible)
	{
		CommandLine_Display();
	}
}// Init


/*!
Saves the visibility of the command line window.  Nothing is
saved if the window was never used.

(4.0)
*/
void
CommandLine_Done ()
{
	// do not initialize the window here if it was never constructed!
	if (nil != gCommandLine_PanelController)
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		Boolean				windowIsVisible = false;
		
		
		// save current visibility
		windowIsVisible = [[[CommandLine_PanelController sharedCommandLinePanelController] window] isVisible];
		prefsResult = Preferences_SetData(kPreferences_TagWasCommandLineShowing,
											sizeof(windowIsVisible), &windowIsVisible);
		if (kPreferences_ResultOK != prefsResult)
		{
			Console_Warning(Console_WriteValue, "failed to store visibility of command line, error", prefsResult);
		}
		
		if (NO == [[NSUserDefaults standardUserDefaults] synchronize])
		{
			Console_Warning(Console_WriteLine, "failed to save command line visibility setting");
		}
	}
}// Done


/*!
Shows the command line floating window, and focuses it.

(3.1)
*/
void
CommandLine_Display ()
{
@autoreleasepool {
	[[CommandLine_PanelController sharedCommandLinePanelController] showWindow:NSApp];
}// @autoreleasepool
}// Display


#pragma mark -
@implementation CommandLine_HistoryDataSource //{


#pragma mark Initializers


/*!
Designated initializer.

(3.1)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
		// arrays grow automatically, this is just an initial size
		_commandHistoryArray = [[NSMutableArray alloc] initWithCapacity:15/* initial capacity; arbitrary */];
	}
	return self;
}// init


#pragma mark Accessors


/*!
Returns the array of strings for recent command lines.

(3.1)
*/
- (NSMutableArray*)
historyArray
{
	return _commandHistoryArray;
}// historyArray


#pragma mark NSComboBoxDataSource


/*!
For auto-completion support; has no effect.

(3.1)
*/
- (NSString*)
comboBox:(NSComboBox*)			aComboBox
completedString:(NSString*)		string
{
#pragma unused(aComboBox, string)
	// this combo box does not support completion
	return nil;
}// comboBox:completedString:


/*!
Returns the index into the underlying array for the first
item that matches the given string value.

(3.1)
*/
- (NSUInteger)
comboBox:(NSComboBox*)					aComboBox
indexOfItemWithStringValue:(NSString*)	string
{
#pragma unused(aComboBox)
	return [_commandHistoryArray indexOfObject:string];
}// comboBox:indexOfItemWithStringValue:


/*!
Returns the exact object (string) in the underlying array
at the given index.

(3.1)
*/
- (id)
comboBox:(NSComboBox*)					aComboBox
objectValueForItemAtIndex:(NSInteger)	index
{
#pragma unused(aComboBox)
	return [_commandHistoryArray objectAtIndex:index];
}// comboBox:objectValueForItemAtIndex:


/*!
Returns the size of the underlying array.

(3.1)
*/
- (NSInteger)
numberOfItemsInComboBox:(NSComboBox*)	aComboBox
{
#pragma unused(aComboBox)
	return [_commandHistoryArray count];
}// numberOfItemsInComboBox:


@end //} CommandLine_HistoryDataSource


#pragma mark -
@implementation CommandLine_TerminalLikeComboBox //{


@synthesize terminalLikeDelegate = _terminalLikeDelegate;


#pragma mark Accessors


/*!
Accessor.

(2016.11)
*/
- (id < CommandLine_TerminalLikeComboBoxDelegate >)
terminalLikeDelegate
{
	return _terminalLikeDelegate;
}
- (void)
setTerminalLikeDelegate:(id< CommandLine_TerminalLikeComboBoxDelegate >)		aDelegate
{
	// this property exists only to allow the type of "delegate" to be specified more precisely
	_terminalLikeDelegate = aDelegate;
	self.delegate = aDelegate;
}// setTerminalLikeDelegate:


#pragma mark NSResponder


/*!
Embellishes this become-first-responder notification by
ensuring that colors for the cursor and text selection
match the color scheme.  This is useful when the user’s
default terminal background is dark, for instance, and
perhaps clashing with typical system highlighting.

This can be triggered in various ways, such as tabbing to
the field or clicking on it with the mouse.  Note in
particular that "becomeFirstResponder" is more reliably
called for various focus situations than something like
"textDidBeginEditing:", which appears to only be called
once the user actually changes the text in some way.

NOTE:	This has to be done as part of a notification
		because a normal text field does not expose enough
		customization properties to affect the appearance
		of selected text.  Once the superclass version of
		this method has been called, the field editor
		exists and can be used to perform the required
		customizations via the NSTextView class.

(2016.11)
*/
- (BOOL)
becomeFirstResponder
{
	// parent implementation will trigger the creation of the
	// field editor so it is important to do that first (if
	// not, "currentEditor" would be nil)
	BOOL		result = [super becomeFirstResponder];
	NSText*		fieldEditor = [self currentEditor];
	
	
	if ([fieldEditor isKindOfClass:NSTextView.class])
	{
		// the field editor is actually an NSTextView*; use the
		// additional properties exposed on NSTextView to make
		// text selections match the color scheme
		NSTextView*		fieldEditorAsView = STATIC_CAST(fieldEditor, NSTextView*);
		
		
		[self synchronizeWithFormatPreferencesForFieldEditor:fieldEditorAsView];
	}
	
	return result;
}// becomeFirstResponder


/*!
Detects certain key presses and responds in a manner
that is consistent with most Unix shells.

All supported key sequences here are translated into
descriptive commands for the delegate implementation
of "control:textView:doCommandBySelector:".  Also,
certain key sequences can ONLY be detected directly
in that delegate method, such as Tab keys.

(2016.11)
*/
- (BOOL)
performKeyEquivalent:(NSEvent*)		aKeyEvent
{
	id		editorObject = self.currentEditor; // might be nil
	BOOL	result = NO;
	
	
	if ([editorObject isKindOfClass:NSTextView.class])
	{
		NSTextView*		asTextView = STATIC_CAST(editorObject, NSTextView*);
		
		
		if (aKeyEvent.modifierFlags & NSEventModifierFlagControl)
		{
			if (NSOrderedSame == [aKeyEvent.charactersIgnoringModifiers compare:@"D" options:NSCaseInsensitiveSearch])
			{
				// control-D; interpret immediately in the terminal (shell complete, or output end-of-file)
				result = [self.terminalLikeDelegate control:self textView:asTextView doCommandBySelector:@selector(commandLineSendTextThenEndOfFile:)];
			}
			else if (NSOrderedSame == [aKeyEvent.charactersIgnoringModifiers compare:@"L" options:NSCaseInsensitiveSearch])
			{
				// control-L; interpret as a screen-clear immediately in the terminal
				result = [self.terminalLikeDelegate control:self textView:asTextView doCommandBySelector:@selector(commandLineTerminalClear:)];
			}
		}
		else if (aKeyEvent.modifierFlags & NSEventModifierFlagCommand)
		{
			if (NSOrderedSame == [aKeyEvent.charactersIgnoringModifiers compare:[NSString stringWithUTF8String:"\015"] options:NSCaseInsensitiveSearch])
			{
				// command-return; interpret as a request to send text immediately WITHOUT a new-line
				result = [self.terminalLikeDelegate control:self textView:asTextView doCommandBySelector:@selector(commandLineSendTextThenNothing:)];
			}
		}
	}
	
	return result;
}// performKeyEquivalent:


@end //} CommandLine_TerminalLikeComboBox


#pragma mark -
@implementation CommandLine_PanelController //{


/*!
Specifies the string in the main command line field.
*/
@synthesize commandLineText = _commandLineText;

/*!
Specifies whether or not the input from the Command Line
is sent to all terminal windows.
*/
@synthesize multiTerminalInput = _multiTerminalInput;

/*!
Specifies the color to use for the insertion point.
*/
@synthesize textCursorNSColor = _textCursorNSColor;


#pragma mark Class Methods


/*!
Returns the singleton.

(3.1)
*/
+ (id)
sharedCommandLinePanelController
{
	static dispatch_once_t		onceToken;
	
	
	dispatch_once(&onceToken,
	^{
		gCommandLine_PanelController = [[self.class allocWithZone:NULL] init];
	});
	return gCommandLine_PanelController;
}// sharedCommandLinePanelController


#pragma mark Initializers


/*!
Designated initializer.

(3.1)
*/
- (instancetype)
init
{
	self = [super initWithWindowNibName:@"CommandLineCocoa"];
	if (nil != self)
	{
		_commandLineText = [[NSMutableString alloc] init];
		_incompleteCommandFragments = [[NSMutableArray alloc] init];
		_textCursorNSColor = nil; // set later
		_multiTerminalInput = NO; // set later
	}
	return self;
}// init


/*!
Destructor.

(3.1)
*/
- (void)
dealloc
{
	[_commandLineText release];
	[_incompleteCommandFragments release];
	[_textCursorNSColor release];
	[super dealloc];
}// dealloc


#pragma mark Actions


/*!
Responds to a click in the help button.

(2018.10)
*/
- (IBAction)
orderFrontContextualHelp:(id)	sender
{
#pragma unused(sender)
	CFRetainRelease		keyPhrase(UIStrings_ReturnCopy(kUIStrings_HelpSystemTopicHelpUsingTheCommandLine),
									CFRetainRelease::kAlreadyRetained);
	
	
	UNUSED_RETURN(HelpSystem_Result)HelpSystem_DisplayHelpWithSearch(keyPhrase.returnCFStringRef());
}// orderFrontContextualHelp:


#pragma mark Accessors: Array Values


/*!
Accessor.

Specifies the text fragments that have been sent without
a new-line since the last new-line was sent (e.g. during
tab-completion).  This can help to compose a complete
picture, as the "commandLineText" would only contain
characters that have not been sent.

This is represented as an ordered array because the real
command line may not be apparent (for example, the shell
may tab-complete based on a fragment).  The read-only
property "incompleteCommandLineText" produces a joined
string from these fragments, showing ellipses in between
to indicate the uncertainty.

(2016.11)
*/
- (void)
insertObject:(NSString*)								aString
inIncompleteCommandFragmentsAtIndex:(unsigned long)	anIndex
{
	// notify about the complete-string version so that any
	// bindings will regenerate the complete string
	[self willChangeValueForKey:NSStringFromSelector(@selector(incompleteCommandLineText))];
	[_incompleteCommandFragments insertObject:[aString copy] atIndex:anIndex];
	[self didChangeValueForKey:NSStringFromSelector(@selector(incompleteCommandLineText))];
}
- (void)
removeObjectFromIncompleteCommandFragmentsAtIndex:(unsigned long)	anIndex
{
	// notify about the complete-string version so that any
	// bindings will regenerate the complete string
	[self willChangeValueForKey:NSStringFromSelector(@selector(incompleteCommandLineText))];
	[_incompleteCommandFragments removeObjectAtIndex:anIndex];
	[self didChangeValueForKey:NSStringFromSelector(@selector(incompleteCommandLineText))];
}// removeObjectFromIncompleteCommandFragmentsAtIndex:


#pragma mark Accessors: General


/*!
Returns the cumulative text that has been sent without
a new-line since the last new-line was sent (e.g. during
tab-completion).  This can help to compose a complete
picture, as the "commandLineText" would only contain
characters that have not been sent.

Ellipses (…) are inserted in between fragments.

(2016.11)
*/
- (NSString*)
incompleteCommandLineText
{
	NSString*	result = nil;
	
	
	if (_incompleteCommandFragments.count > 0)
	{
		result = [_incompleteCommandFragments componentsJoinedByString:@"…"/* LOCALIZE THIS? */];
		result = [result stringByAppendingString:@"…"/* LOCALIZE THIS? */];
		[result retain];
	}
	
	return result;
}// incompleteCommandLineText


/*!
Returns the user’s default terminal background color.

(2016.11)
*/
- (NSColor*)
textBackgroundNSColor
{
	return [[commandLineField backgroundColor] retain];
}// textBackgroundNSColor


/*!
Returns the user’s default terminal text color.

(2016.11)
*/
- (NSColor*)
textForegroundNSColor
{
	return [[commandLineField textColor] retain];
}// textForegroundNSColor


#pragma mark CommandLine_TerminalLikeComboBoxDelegate


/*!
Responds to certain editing actions by ensuring that the
selection colors are set correctly.  Ignores other events.

Due to the bizarre way that Cocoa maps key events, it is
simply not feasible to override some sequences without
first posing as another command.  For instance, a Tab is
only detected by hacking "".

(2016.11)
*/
- (BOOL)
control:(NSControl*)		aControl
textView:(NSTextView*)		aTextView
doCommandBySelector:(SEL)	aSelector
{
	NSString*	stringSoFar = aTextView.string;
	BOOL		clearTextInput = NO; // whether or not to erase the field after this action
	BOOL		result = NO;
	
	
	// this delegate is not designed to manage other fields...
	assert(self->commandLineField == aControl);
	
	// IMPORTANT: some of these are determined by intercepting
	// control-key combinations in the "performKeyEquivalent:"
	// of the field subclass itself; others can only be detected
	// here, by detecting standard system commands and pretending
	// they are different commands
	if (@selector(commandLineSendDeleteCharacter:) == aSelector)
	{
		// send session’s designated backspace or delete character;
		// this should not be performed unless the field is empty!
		Session_SendDeleteBackward(SessionFactory_ReturnUserRecentSession(), kSession_EchoCurrentSessionValue);
		result = YES;
	}
	else if (@selector(commandLineSendEscapeCharacter:) == aSelector)
	{
		// send escape character without clearing local command line
		char	escapeChar[] = { '\033', '\000' };
		
		
		[self sendText:[NSString stringWithCString:escapeChar encoding:NSASCIIStringEncoding]
						newLine:NO addingToHistory:NO];
		clearTextInput = NO;
		result = YES;
	}
	else if (@selector(commandLineSendTextThenEndOfFile:) == aSelector)
	{
		// send local command line text (no new-line), then control-D;
		// this causes most shells to page-complete or log out, and
		// causes most other programs to end multi-line input/output
		char	controlDChar[] = { '\004', '\000' };
		
		
		[self sendTextSoFar:stringSoFar];
		[self sendText:[NSString stringWithCString:controlDChar encoding:NSASCIIStringEncoding]
						newLine:NO addingToHistory:NO];
		clearTextInput = YES;
		result = YES;
	}
	else if (@selector(commandLineSendTextThenNewLine:) == aSelector)
	{
		// send local command line text to session and then send the
		// session’s designated new-line character or sequence
		NSMutableArray*		mutableArrayProxy = nil;
		
		
		[self sendText:stringSoFar newLine:YES addingToHistory:((nil == self.incompleteCommandLineText) &&
																(stringSoFar.length > 0))];
		clearTextInput = YES;
		
		// when a command has finally been sent with a new-line,
		// erase any incomplete version of that command-line text
		// (due to bindings, this will update the interface too)
		mutableArrayProxy = [self mutableArrayValueForKey:@"incompleteCommandFragments"];
		assert(nil != mutableArrayProxy);
		[mutableArrayProxy removeAllObjects];
		
		result = YES;
	}
	else if (@selector(commandLineSendTextThenNothing:) == aSelector)
	{
		// send local command line text to session WITHOUT any new-line
		[self sendTextSoFar:stringSoFar];
		clearTextInput = YES;
		result = YES;
	}
	else if (@selector(commandLineSendTextThenTab:) == aSelector)
	{
		// send local command line text (no new-line), then a Tab; this
		// causes most shells to “complete” a command or file name, and
		// causes most editors to insert a tab character (note that the
		// user must use Shift-Tab to change local keyboard focus)
		char	tabChar[] = { '\011', '\000' };
		
		
		[self sendTextSoFar:stringSoFar];
		[self sendText:[NSString stringWithCString:tabChar encoding:NSASCIIStringEncoding]
						newLine:NO addingToHistory:NO];
		clearTextInput = YES;
		result = YES;
	}
	else if (@selector(commandLineTerminalClear:) == aSelector)
	{
		// send control-L without clearing local command line (this
		// typically causes the terminal to erase all lines and move
		// the cursor to home)
		char	controlLChar[] = { '\014', '\000' };
		
		
		[self sendText:[NSString stringWithCString:controlLChar encoding:NSASCIIStringEncoding]
						newLine:NO addingToHistory:NO];
		clearTextInput = NO; // clear terminal only; keep the command-line input so far
		result = YES;
	}
	else if (@selector(cancelOperation:) == aSelector)
	{
		// defined by Cocoa; remap to the escape sequence
		result = [self control:aControl textView:aTextView doCommandBySelector:@selector(commandLineSendEscapeCharacter:)]; // recursive
	}
	else if (@selector(deleteBackward:) == aSelector)
	{
		// defined by Cocoa; remap so that when the field is empty, this
		// immediately sends a delete-character
		if (0 == aTextView.string.length)
		{
			// remap to a more descriptive command
			if (_incompleteCommandFragments.count > 0)
			{
				// since the local “incomplete” string represents text that was
				// previously sent, assume that it should be shortened by one
				// composed character sequence to approximate the new prefix
				NSString*	lastComponent = [_incompleteCommandFragments lastObject];
				
				
				if (lastComponent.length > 0)
				{
					NSRange		lastCharacterRange = NSMakeRange(lastComponent.length - 1, 1);
					
					
					lastCharacterRange = [lastComponent rangeOfComposedCharacterSequencesForRange:lastCharacterRange];
					lastComponent = [lastComponent stringByReplacingCharactersInRange:lastCharacterRange withString:@""];
					[self removeObjectFromIncompleteCommandFragmentsAtIndex:(_incompleteCommandFragments.count - 1)];
					if (lastComponent.length > 0)
					{
						[self insertObject:lastComponent inIncompleteCommandFragmentsAtIndex:(_incompleteCommandFragments.count)];
					}
				}
			}
			result = [self control:aControl textView:aTextView doCommandBySelector:@selector(commandLineSendDeleteCharacter:)]; // recursive
		}
	}
	else if (@selector(insertNewline:) == aSelector)
	{
		// defined by Cocoa; remap to immediately send text WITH a new-line
		// (NOTE: this is not accomplished using an “action” binding because
		// the OS doesn’t send an action for an empty-field, return-only case;
		// also, it is nicer if several key handlers are nearby in the code)
		result = [self control:aControl textView:aTextView doCommandBySelector:@selector(commandLineSendTextThenNewLine:)]; // recursive
	}
	else if (@selector(insertTab:) == aSelector)
	{
		// defined by Cocoa; remap to immediately send all the text
		// and then send a Tab character (while this technically
		// breaks focus rings in the Command Line window, the user
		// can still use Shift-Tab to change focus backwards)
		result = [self control:aControl textView:aTextView doCommandBySelector:@selector(commandLineSendTextThenTab:)]; // recursive
	}
	else
	{
		//NSLog(@"command line asked to perform selector '%@'", NSStringFromSelector(aSelector)); // debug
	}
	
	if (clearTextInput)
	{
		self->commandLineField.stringValue = @""; // clear the field
		aTextView.string = @""; // clear the temporary-editing string too
	}
	
	return result;
}// control:textView:doCommandBySelector:


#pragma mark NSWindowController


/*!
Handles initialization that depends on user interface
elements being properly set up.  (Everything else is just
done in "init".)

(3.1)
*/
- (void)
windowDidLoad
{
	[super windowDidLoad];
	
	assert(nil != commandLineField);
	assert(nil != incompleteTextField);
	assert(self == commandLineField.terminalLikeDelegate);
	
	Preferences_Result	prefsResult = kPreferences_ResultOK;
	
	
	// INCOMPLETE: the following finds the initial values but it
	// does not detect later changes to Preferences; it may be
	// nice to keep the settings in sync immediately (instead of
	// only on the next launch)
	
	// set the font but keep the size (it is too difficult to fix
	// the size and layout of the “combo box” right now)
	{
		CFStringRef		fontName = nullptr;
		
		
		prefsResult = Preferences_GetData(kPreferences_TagFontName, sizeof(fontName), &fontName);
		if (kPreferences_ResultOK == prefsResult)
		{
			[commandLineField setFont:[NSFont fontWithName:BRIDGE_CAST(fontName, NSString*)
															size:[[commandLineField font] pointSize]]];
		}
	}
	
	// set colors
	{
		CGFloatRGBColor		colorInfo;
		
		
		prefsResult = Preferences_GetData(kPreferences_TagTerminalColorNormalBackground, sizeof(colorInfo), &colorInfo);
		if (kPreferences_ResultOK == prefsResult)
		{
			NSColor*	colorObject = [NSColor colorWithDeviceRed:colorInfo.red green:colorInfo.green blue:colorInfo.blue
										alpha:1.0];
			
			
			[commandLineField setDrawsBackground:YES];
			[commandLineField setBackgroundColor:colorObject];
			
			// refuse to set the text color if the background cannot be found, just in case the
			// text color is white or some other color that would suck with the default background
			prefsResult = Preferences_GetData(kPreferences_TagTerminalColorNormalForeground, sizeof(colorInfo), &colorInfo);
			if (kPreferences_ResultOK == prefsResult)
			{
				colorObject = [NSColor colorWithDeviceRed:colorInfo.red green:colorInfo.green blue:colorInfo.blue
								alpha:1.0];
				[commandLineField setTextColor:colorObject];
			}
			
			// see if a custom cursor color has been defined; if not, use the foreground
			// INCOMPLETE: there is also an “automatic” setting for the cursor…
			prefsResult = Preferences_GetData(kPreferences_TagTerminalColorCursorBackground, sizeof(colorInfo), &colorInfo);
			if (kPreferences_ResultOK == prefsResult)
			{
				[self->_textCursorNSColor release];
				self->_textCursorNSColor = [[NSColor colorWithDeviceRed:colorInfo.red green:colorInfo.green blue:colorInfo.blue
																		alpha:1.0] retain];
			}
			if (nil == self->_textCursorNSColor)
			{
				// assume a default if preference can’t be found
				self->_textCursorNSColor = [self.textForegroundNSColor retain];
			}
		}
	}
}// windowDidLoad


/*!
Affects the preferences key under which window position
and size information are automatically saved and
restored.

(4.0)
*/
- (NSString*)
windowFrameAutosaveName
{
	// NOTE: do not ever change this, it would only cause existing
	// user settings to be forgotten
	return @"CommandLine";
}// windowFrameAutosaveName


@end //} CommandLine_PanelController


#pragma mark -
@implementation CommandLine_PanelController (CommandLine_PanelControllerInternal) //{


/*!
Lower-level method to send arbitrary text to the
one or more sessions specified in the command-line
panel.

(2016.11)
*/
- (void)
sendText:(NSString*)	aString
newLine:(BOOL)			aNewLineFlag
addingToHistory:(BOOL)	aHistoryFlag
{
	SessionRef		session = SessionFactory_ReturnUserRecentSession();
	
	
	if ((nullptr == session) || (nil == aString))
	{
		// apparently nothing to send or nowhere to send it
		Sound_StandardAlert();
	}
	else
	{
		if (aHistoryFlag)
		{
			NSMutableArray*		historyArray = [STATIC_CAST([commandLineField dataSource], CommandLine_HistoryDataSource*) historyArray];
			
			
			[historyArray insertObject:[NSString stringWithString:aString] atIndex:0];
		}
		
		if (self.multiTerminalInput)
		{
			SessionFactory_ForEachSession
			(^(SessionRef	inSession,
			   Boolean&		UNUSED_ARGUMENT(outStopFlag))
			{
				Session_UserInputCFString(inSession, BRIDGE_CAST(aString, CFStringRef));
				if (aNewLineFlag)
				{
					Session_SendNewline(inSession, kSession_EchoCurrentSessionValue);
				}
			});
		}
		else
		{
			Session_UserInputCFString(session, BRIDGE_CAST(aString, CFStringRef));
			if (aNewLineFlag)
			{
				Session_SendNewline(session, kSession_EchoCurrentSessionValue);
			}
		}
	}
}// sendText:addingToHistory:newLine:


/*!
Lower-level method to immediately send the given string
to the underlying session and also append it to the
“incomplete” line.  This is appropriate for commands
that perform partial completion, such as a Tab.

(2016.11)
*/
- (void)
sendTextSoFar:(NSString*)	aString
{
	[self insertObject:aString inIncompleteCommandFragmentsAtIndex:(_incompleteCommandFragments.count)];
	[self sendText:aString newLine:NO addingToHistory:NO];
}// sendTextSoFar:


@end //} CommandLine_PanelController (CommandLine_PanelControllerInternal)


#pragma mark -
@implementation CommandLine_TerminalLikeComboBox (CommandLine_TerminalLikeComboBoxInternal) //{


#pragma mark New Methods


/*!
Sets the color of the cursor and text selections to match
the terminal theme, which is useful when the user’s default
terminal background is dark.

NOTE:	This has to be done as part of a notification
		because a normal text field does not expose enough
		customization properties to affect the appearance
		of selected text.  Once editing begins, the field
		editor can be used to perform the required
		customizations.

(2016.11)
*/
- (void)
synchronizeWithFormatPreferencesForFieldEditor:(NSTextView*)	aSelectedTextView
{
	CommandLine_PanelController*		cmdLineController = [CommandLine_PanelController sharedCommandLinePanelController];
	NSColor*						backgroundColor = [cmdLineController textBackgroundNSColor];
	NSColor*						textColor = [cmdLineController textForegroundNSColor];
	NSColor*						selectionBackgroundColor = backgroundColor; // see below
	NSColor*						selectionTextColor = textColor; // see below
	Preferences_Result				prefsResult = kPreferences_ResultOK;
	Boolean							isInverted = false;
	
	
	prefsResult = Preferences_GetData(kPreferences_TagPureInverse, sizeof(isInverted), &isInverted);
	if (kPreferences_ResultOK != prefsResult)
	{
		// assume a value, if preference can’t be found
		isInverted = false;
	}
	
	if (isInverted)
	{
		// user preference is to invert text
		selectionBackgroundColor = textColor;
		selectionTextColor = backgroundColor;
	}
	else
	{
		// user preference is to find appropriately-tinted
		// versions of the normal terminal colors
		UNUSED_RETURN(BOOL)[NSColor selectionColorsForForeground:&selectionTextColor
																	background:&selectionBackgroundColor];
	}
	
	// Since the text color and background are modified to match user
	// preferences, the insertion point should also be a color that
	// looks reasonable against the custom background.  Similarly,
	// text selections should try to match user preferences instead
	// of always using system colors (which might be hard to see).
	// These changes can be made by customizing the shared editor.
	[aSelectedTextView setInsertionPointColor:
						[[CommandLine_PanelController sharedCommandLinePanelController] textCursorNSColor]];
	[aSelectedTextView setSelectedTextAttributes:
						@{
							NSForegroundColorAttributeName: selectionTextColor,
							NSBackgroundColorAttributeName: selectionBackgroundColor,
						}];
}// synchronizeWithFormatPreferencesForFieldEditor:


@end //} CommandLine_TerminalLikeComboBox (CommandLine_TerminalLikeComboBoxInternal)

// BELOW IS REQUIRED NEWLINE TO END FILE
