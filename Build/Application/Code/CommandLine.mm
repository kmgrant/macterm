/*!	\file CommandLine.mm
	\brief Cocoa implementation of the floating command line.
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

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <AutoPool.objc++.h>
#import <Console.h>
#import <SoundSystem.h>

// MacTelnet includes
#import "CommandLine.h"
#import "HelpSystem.h"
#import "Preferences.h"
#import "Session.h"
#import "SessionFactory.h"



#pragma mark Public Methods

/*!
Shows the command line floating window, and focuses it.

(3.1)
*/
void
CommandLine_Display ()
{
	AutoPool	_;
	
	
	[[CommandLine_PanelController sharedCommandLinePanelController] showWindow:NSApp];
}// Display


#pragma mark Internal Methods

@implementation CommandLine_HistoryDataSource

- (id)init
{
	self = [super init];
	// arrays grow automatically, this is just an initial size
	_commandHistoryArray = [[NSMutableArray alloc] initWithCapacity:15/* initial capacity; arbitrary */];
	return self;
}

- (NSMutableArray*)historyArray
{
	return _commandHistoryArray;
}

- (int)numberOfItemsInComboBox:(NSComboBox*)aComboBox
{
#pragma unused(aComboBox)
	return [_commandHistoryArray count];
}

- (id)comboBox:(NSComboBox*)aComboBox objectValueForItemAtIndex:(int)index
{
#pragma unused(aComboBox)
	return [_commandHistoryArray objectAtIndex:index];
}

- (unsigned int)comboBox:(NSComboBox*)aComboBox indexOfItemWithStringValue:(NSString*)string
{
#pragma unused(aComboBox)
	return [_commandHistoryArray indexOfObject:string];
}

- (NSString*)comboBox:(NSComboBox*)aComboBox completedString:(NSString*)string
{
#pragma unused(aComboBox, string)
	// this combo box does not support completion
	return nil;
}

@end // CommandLine_HistoryDataSource

@implementation CommandLine_PanelController

static CommandLine_PanelController*		gCommandLine_PanelController = nil;
+ (id)sharedCommandLinePanelController
{
	if (nil == gCommandLine_PanelController)
	{
		gCommandLine_PanelController = [[CommandLine_PanelController allocWithZone:NULL] init];
	}
	return gCommandLine_PanelController;
}

- (id)init
{
	self = [super initWithWindowNibName:@"CommandLineCocoa"];
	commandLineText = [[NSString alloc] init];
	return self;
}

- (IBAction)displayHelp:(id)sender
{
#pragma unused(sender)
	(HelpSystem_Result)HelpSystem_DisplayHelpFromKeyPhrase(kHelpSystem_KeyPhraseCommandLine);
}

- (IBAction)sendText:(id)sender
{
#pragma unused(sender)
	SessionRef		session = SessionFactory_ReturnUserFocusSession();
	
	
	if ((nullptr == session) || (nil == commandLineText))
	{
		// apparently nothing to send or nowhere to send it
		Sound_StandardAlert();
	}
	else
	{
		Session_UserInputCFString(session, (CFStringRef)commandLineText, true/* record */);
		Session_SendNewline(session, kSession_EchoCurrentSessionValue);
		[[[commandLineField dataSource] historyArray] insertObject:[[NSString alloc] initWithString:commandLineText] atIndex:0];
	}
}

- (void)windowDidLoad
{
	[super windowDidLoad];
	Preferences_Result	prefsResult = kPreferences_ResultOK;
	
	
#if 0
	// set the font; though this can look kind of odd
	{
		Str255		fontName;
		
		
		prefsResult = Preferences_GetData(kPreferences_TagFontName, sizeof(fontName), fontName);
		if (kPreferences_ResultOK == prefsResult)
		{
			NSString*	fontNameString = [[NSString alloc] initWithBytes:(fontName + 1/* skip length byte */)
											length:PLstrlen(fontName) encoding:NSMacOSRomanStringEncoding];
			
			
			[commandLineField setFont:[NSFont fontWithName:fontNameString [[commandLineField font] pointSize]]];
		}
	}
#endif
	// set colors
	{
		RGBColor	colorInfo;
		
		
		prefsResult = Preferences_GetData(kPreferences_TagTerminalColorNormalBackground, sizeof(colorInfo), &colorInfo);
		if (kPreferences_ResultOK == prefsResult)
		{
			NSColor*	colorObject = [NSColor colorWithDeviceRed:((Float32)colorInfo.red / (Float32)RGBCOLOR_INTENSITY_MAX)
										green:((Float32)colorInfo.green / (Float32)RGBCOLOR_INTENSITY_MAX)
										blue:((Float32)colorInfo.blue / (Float32)RGBCOLOR_INTENSITY_MAX)
										alpha:1.0];
			
			
			[commandLineField setDrawsBackground:YES];
			[commandLineField setBackgroundColor:colorObject];
			
			// refuse to set the text color if the background cannot be found, just in case the
			// text color is white or some other color that would suck with the default background
			prefsResult = Preferences_GetData(kPreferences_TagTerminalColorNormalForeground, sizeof(colorInfo), &colorInfo);
			if (kPreferences_ResultOK == prefsResult)
			{
				colorObject = [NSColor colorWithDeviceRed:((Float32)colorInfo.red / (Float32)RGBCOLOR_INTENSITY_MAX)
								green:((Float32)colorInfo.green / (Float32)RGBCOLOR_INTENSITY_MAX)
								blue:((Float32)colorInfo.blue / (Float32)RGBCOLOR_INTENSITY_MAX)
								alpha:1.0];
				[commandLineField setTextColor:colorObject];
			}
		}
	}
}

@end // CommandLine_PanelController

// BELOW IS REQUIRED NEWLINE TO END FILE
