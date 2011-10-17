/*!	\file CommandLine.h
	\brief The floating one-line input window.
	
	Input is sent to the frontmost terminal window.
*/
/*###############################################################

	MacTerm
		© 1998-2011 by Kevin Grant.
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

#ifndef __COMMANDLINE__
#define __COMMANDLINE__

// Mac includes
#include <Carbon/Carbon.h>
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
@interface CommandLine_HistoryDataSource : NSObject
{
	NSMutableArray*		_commandHistoryArray;
}

- (NSMutableArray*)
historyArray;

@end


/*!
A special customization of a combo box that makes it look more
like a terminal window.  See "CommandLineCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CommandLine_TerminalLikeComboBox : NSComboBox

- (void)
textDidBeginEditing:(NSNotification*)_;

@end


/*!
Implements the floating command line window.  See
"CommandLineCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CommandLine_PanelController : NSWindowController
{
	NSMutableString*							commandLineText; // binding
	IBOutlet CommandLine_TerminalLikeComboBox*	commandLineField;
}

+ (id)
sharedCommandLinePanelController;

- (IBAction)
displayHelp:(id)_;

- (IBAction)
sendText:(id)_;

- (NSColor*)
textColor;

@end

#endif // __OBJC__



#pragma mark Public Methods

void
	CommandLine_Init			();

void
	CommandLine_Done			();

void
	CommandLine_Display			();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
