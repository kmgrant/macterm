/*!	\file CommandLine.h
	\brief The floating one-line input window.
	
	Input is sent to the frontmost terminal window.
*/
/*###############################################################

	MacTerm
		© 1998-2019 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif
#include <CoreServices/CoreServices.h>



#pragma mark Types

#ifdef __OBJC__

/*!
Manages the history menu of the command line.  Attempting
to use bindings and controllers for this purpose failed...
*/
@interface CommandLine_HistoryDataSource : NSObject //{
{
	NSMutableArray*		_commandHistoryArray;
}

// accessors
	- (NSMutableArray*)
	historyArray;

@end //}


/*!
This protocol is not meant to be implemented; it exists only
to ensure that the required selectors are known to Objective-C
(eliminating warnings for checking them).  The selectors are
passed to "control:textView:doCommandBySelector:"; see
"CommandLine_TerminalLikeComboBoxDelegate".
*/
@protocol __CommandLine_TerminalLikeComboBoxDelegateMethods //{

	// send session’s designated backspace or delete character;
	// this should not be performed unless the field is empty!
	- (void)
	commandLineSendDeleteCharacter:(id)_;

	// send escape character without clearing local command line
	- (void)
	commandLineSendEscapeCharacter:(id)_;

	// send local command line text (no new-line), then control-D;
	// this causes most shells to page-complete or log out, and
	// causes most other programs to end multi-line input/output
	- (void)
	commandLineSendTextThenEndOfFile:(id)_;

	// send local command line text to session and then send the
	// session’s designated new-line character or sequence
	- (void)
	commandLineSendTextThenNewLine:(id)_;

	// send local command line text to session WITHOUT any new-line
	- (void)
	commandLineSendTextThenNothing:(id)_;

	// send local command line text (no new-line), then a Tab; this
	// causes most shells to “complete” a command or file name, and
	// causes most editors to insert a tab character (note that the
	// user must use Shift-Tab to change local keyboard focus)
	- (void)
	commandLineSendTextThenTab:(id)_;

	// send control-L without clearing local command line (this
	// typically causes the terminal to erase all lines and move
	// the cursor to home)
	- (void)
	commandLineTerminalClear:(id)_;

@end // }


/*!
The "control:textView:doCommandBySelector:" from the parent
protocol has been extended to also receive the selectors
from "__CommandLine_TerminalLikeComboBoxDelegateMethods".
*/
@protocol CommandLine_TerminalLikeComboBoxDelegate < NSComboBoxDelegate > //{

@end // }


/*!
A special customization of a combo box that makes it look
more like a terminal window.  See "CommandLineCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CommandLine_TerminalLikeComboBox : NSComboBox //{

// accessors
	@property (assign) IBOutlet id < CommandLine_TerminalLikeComboBoxDelegate >
	terminalLikeDelegate;

// NSResponder
	- (BOOL)
	becomeFirstResponder;
	- (BOOL)
	performKeyEquivalent:(NSEvent*)_;

@end //}


/*!
Implements the floating command line window.  See
"CommandLineCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CommandLine_PanelController : NSWindowController < CommandLine_TerminalLikeComboBoxDelegate > //{
{
	IBOutlet CommandLine_TerminalLikeComboBox*	commandLineField;
	IBOutlet NSTextField*						incompleteTextField;
@private
	NSMutableString*							_commandLineText;
	NSMutableArray*								_incompleteCommandFragments;
	NSColor*									_textCursorNSColor;
	BOOL										_multiTerminalInput;
}

// class methods
	+ (id)
	sharedCommandLinePanelController;

// actions
	- (IBAction)
	orderFrontContextualHelp:(id)_;

// accessors: array values
	- (void)
	insertObject:(NSString*)_
	inIncompleteCommandFragmentsAtIndex:(unsigned long)_;
	- (void)
	removeObjectFromIncompleteCommandFragmentsAtIndex:(unsigned long)_;

// accessors: general
	@property (strong) NSString*
	commandLineText; // binding
	@property (assign, readonly) NSString*
	incompleteCommandLineText; // binding; depends on "incompleteCommandFragments"
	@property (assign) BOOL
	multiTerminalInput;
	- (NSColor*)
	textBackgroundNSColor;
	@property (strong, readonly) NSColor*
	textCursorNSColor;
	- (NSColor*)
	textForegroundNSColor;

@end //}

#endif // __OBJC__



#pragma mark Public Methods

void
	CommandLine_Init			();

void
	CommandLine_Done			();

void
	CommandLine_Display			();

// BELOW IS REQUIRED NEWLINE TO END FILE
