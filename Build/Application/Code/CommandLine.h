/*!	\file CommandLine.h
	\brief The floating one-line input window in MacTelnet 3.0.
	
	Input is sent to the frontmost terminal window when entered
	or tabbed by the user.
*/
/*###############################################################

	MacTelnet
		© 1998-2008 by Kevin Grant.
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

#include "UniversalDefines.h"

#ifndef __COMMANDLINE__
#define __COMMANDLINE__

// Mac includes
#include <Carbon/Carbon.h>
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
- (NSMutableArray*)historyArray;
// NSComboBoxDataSource
- (int)numberOfItemsInComboBox:(NSComboBox*)aComboBox;
- (id)comboBox:(NSComboBox*)aComboBox objectValueForItemAtIndex:(int)index;
- (unsigned int)comboBox:(NSComboBox*)aComboBox indexOfItemWithStringValue:(NSString*)string;
- (NSString*)comboBox:(NSComboBox*)aComboBox completedString:(NSString*)string;
@end

/*!
Implements the floating command line window.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface CommandLine_PanelController : NSWindowController
{
	NSMutableString*		commandLineText; // binding
	
	IBOutlet NSComboBox*	commandLineField;
}
+ (id)sharedCommandLinePanelController;
// the following MUST match what is in "CommandLineCocoa.nib"
- (IBAction)displayHelp:(id)sender;
- (IBAction)sendText:(id)sender;
- (void)windowDidLoad;
@end

#endif // __OBJC__



#pragma mark Public Methods

void
	CommandLine_Display			();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
