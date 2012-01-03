/*!	\file TerminalFile.h
	\brief Defines an API to obtain information from Apple 
	Terminal (.term) files.
	
	Currently there is no support to create or write to
	".term" files.
	
	Developed by Ian Anderson.
*/
/*###############################################################

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#ifndef __TERMINALFILE__
#define __TERMINALFILE__

// Mac includes
#include <CoreFoundation/CoreFoundation.h>

#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif



#pragma mark Constants

enum TerminalFile_Result
{
	kTerminalFile_ResultOK					= 0,	//!< no error occurred
	kTerminalFile_ResultGenericFailure		= 1,	//!< a nonspecific error has occurred
	kTerminalFile_ResultInvalidFileType		= 2,	//!< the file passed in was not a valid Terminal 
													//   File (not of XML property list format)
	kTerminalFile_ResultNumberConversionErr	= 3,	//!< unable to convert a number from 
													//   the Terminal File to a variable
	kTerminalFile_ResultTagNotFound			= 4,	//!< one of the requested tags was not 
													//   found in the given Terminal File
	kTerminalFile_ResultMemAllocErr			= 5		//!< an error occurred during 
													//   internal memory allocation
};

/*!
This is the version of Terminal used to create the Terminal file.
*/
enum TerminalFile_Version
{
	kTerminalFile_VersionUnknown	= 0,	//!< unknown file format
	kTerminalFile_Version1_1		= 1,	//!< Terminal file created in Mac OS X 10.1
	kTerminalFile_Version1_3		= 2,	//!< Terminal file created in Mac OS X 10.2
	kTerminalFile_Version1_4		= 3		//!< Terminal file created in Mac OS X 10.3
};

/*!
When you retrieve data, you can specify the class of data being 
retrieved.  Currently, Apple's ".term" files describe terminal 
window data only, but in the future the files may contain other 
information - this allows future API calls to retrieve new data 
when it is available and supported by this module.

More than one collection of data of the same class may exist in 
a Terminal File; the index provided to 
TerminalFile_GetAttributes() indicates which one you want, and 
TerminalFile_GetSettingsCount() provides the total number of 
collections of any one class. 
*/
enum TerminalFile_SettingsType
{
	kTerminalFile_SettingsTypeWindow	= 'Wset'	//!< information for terminal windows
};

/* Window Settings data access tags

For tags that use Core Foundation types, pass in the address of 
an uninitialized or NULL CFTypeRef variable.  For retain/release 
purposes, consider the CFTypeRef returned as if it had been 
returned from a CreateXXX or CopyXXX function.  You own it and 
need to release it when you're done.
*/
enum TerminalFile_AttributeTag
{
		//!< the following tags are available in version 1.2 format files and earlier
	kTerminalFile_AttributeTagWindowKeypad						= 'KPad',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowSourceDotLogin				= '.Lin',	//!< data: Boolean
		//!< the following tags are available in version 1.2 format files and later
	kTerminalFile_AttributeTagWindowUserShell					= 'Shel',	//!< data: CFStringRef
	kTerminalFile_AttributeTagWindowShellExitAction				= 'SEAc',	//!< data: TerminalFile_WindowShellExitAction
	kTerminalFile_AttributeTagWindowColumnCount					= 'Colm',	//!< data: UInt32 (cannot be 0)
	kTerminalFile_AttributeTagWindowRowCount					= 'Rows',	//!< data: UInt32 (cannot be 0)
	kTerminalFile_AttributeTagWindowTitleBits					= 'TBit',	//!< data: TerminalFile_WindowTitleBits
	kTerminalFile_AttributeTagWindowCustomTitle					= 'CuTi',	//!< data: CFStringRef
	kTerminalFile_AttributeTagWindowTextColors					= 'TxCl',	//!< data: CFArrayRef (of RGBColor*)
	kTerminalFile_AttributeTagWindowDoubleBold					= 'DBld',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowDisableColors				= 'DisC',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowBlinkCursor					= 'BlCr',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowCursorShape					= 'CrSh',	//!< data: UInt16 (kTerminalCursorType...)
	kTerminalFile_AttributeTagWindowFontName					= 'FxFn',	//!< data: CFStringRef
	kTerminalFile_AttributeTagWindowFontSize					= 'FxFS',	//!< data: Float32
	kTerminalFile_AttributeTagWindowSaveLines					= 'SvLn',	//!< data: SInt32 (-1 for unlimited scrollback)
	kTerminalFile_AttributeTagWindowScrollbackEnabled			= 'ScBk',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowAutowrap					= 'AWrp',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowScrollOnInput				= 'AFoc',	//!< data: Boolean (scroll to bottom on input)
	kTerminalFile_AttributeTagWindowTranslateOnPaste			= 'Tran',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowStrictEmulation				= 'StEm',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowMetaKeyMapping				= 'Meta',	//!< data: TerminalFile_WindowMetaKeyMapping
	kTerminalFile_AttributeTagWindowAudibleBell					= 'ABel',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowLocation					= 'lULY',	//!< data: Point
	kTerminalFile_AttributeTagWindowIsMinimized					= 'Mini',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowExecutionString				= 'ExSt',	//!< data: CFStringRef
	kTerminalFile_AttributeTagWindowOpaqueness					= 'TOpq',	//!< data: Float32 (between 0.0 and 1.0)
	kTerminalFile_AttributeTagWindowFontSpacingV				= 'FHtS',	//!< data: Float32
	kTerminalFile_AttributeTagWindowFontSpacingH				= 'FWdS',	//!< data: Float32
	kTerminalFile_AttributeTagWindowLocationY					= 'WiLY',	//!< data: SInt16
		//!< the following tags are available in version 1.3 format files only
	kTerminalFile_AttributeTagWindowMacTermFunctionKeys			= 'MTFk',	//!< data: Boolean
		//!< the following tags are available in version 1.3 format files and later
	kTerminalFile_AttributeTagWindowCloseAction					= 'ClAc',	//!< data: TerminalFile_WindowCloseAction
	kTerminalFile_AttributeTagWindowCleanCommandList			= 'ClCm',	//!< data: CFArrayRef (of CFStringRef)
	kTerminalFile_AttributeTagWindowDeleteSendsBS				= 'DeBS',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowEscape8BitCharsWithCtrlV	= 'CVEs',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowBackwrap					= 'BWrp',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowVisualBell					= 'VBel',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowFontAntialiased				= 'FnAA',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowDoubleWideChars				= 'DWde',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowDColumnsDWide				= 'DcDw',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowTextEncoding				= 'SEnc',	//!< data: CFStringEncoding
	kTerminalFile_AttributeTagWindowScrollRgnCompatible			= 'ScRC',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowScrollbackRows				= 'ScRw',	//!< data: UInt32
		//!< the following tags are available in version 1.4 format files and later
	kTerminalFile_AttributeTagWindowTerminalType				= 'Term',	//!< data: CFStringRef
	kTerminalFile_AttributeTagWindowOptClickMoveCursor			= 'OpCr',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowRewrapOnResize				= 'RWRs',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowBlinkingText				= 'BTxt',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowDragCopy					= 'DgCp',	//!< data: Boolean
	kTerminalFile_AttributeTagWindowBackgroundImage				= 'BImg',	//!< data: CFStringRef
	kTerminalFile_AttributeTagWindowKeyMappings					= 'KMap',	//!< data: CFDictionaryRef (with )
	kTerminalFile_AttributeTagWindowPadding						= 'WPad',	//!< data: Rect
	kTerminalFile_AttributeTagWindowScrollbar					= 'Sbar'	//!< data: Boolean
};

/*!
Returned from "kTerminalFile_AttributeTagWindowCloseAction" of 
"kTerminalFile_SettingsTypeWindow", indicating what should 
happen when a terminal window is closed.
*/
enum TerminalFile_WindowCloseAction
{
	kTerminalFile_WindowCloseActionPromptNever				= 0,	//!< never prompt before closing
	kTerminalFile_WindowCloseActionPromptIfUncleanCommand	= 1,	//!< prompt before closing unless the only running 
																	//!  processes belong to the list returned using 
																	//!  "kTerminalFile_AttributeTagWindowCleanCommandList" 
																	//!  and "kTerminalFile_SettingsTypeWindow"
	kTerminalFile_WindowCloseActionPromptAlways				= 2		//!< always prompt before closing
};

/*!
Returned from "kTerminalFile_AttributeTagWindowMetaKeyMapping" 
of "kTerminalFile_SettingsTypeWindow", indicating what kind of 
key mapping occurs to handle the meta key on UNIX workstation 
keyboards.
*/
enum TerminalFile_WindowMetaKeyMapping
{
	kTerminalFile_WindowMetaKeyMappingNone		= -1,	//!< no meta key mapping
	kTerminalFile_WindowMetaKeyMappingOption	= 27	//!< option key maps to the meta key
};

/*!
Returned from "kTerminalFile_AttributeTagWindowShellExitBehavior" 
of "kTerminalFile_SettingsTypeWindow", indicating what should 
happen when the shell process running in a terminal window exits.
*/
enum TerminalFile_WindowShellExitAction
{
	kTerminalFile_WindowShellExitActionCloseWindow			= 0,	//!< close window
	kTerminalFile_WindowShellExitActionCloseWindowIfClean	= 1,	//!< close if clean exit
	kTerminalFile_WindowShellExitActionDontCloseWindow		= 2		//!< don't close window
};

/*!
Use these constants to index into the text color array returned 
from "kTerminalFile_AttributeTagWindowTextColors" of 
"kTerminalFile_SettingsTypeWindow", indicating what color to 
draw the various user interface elements in.
*/
enum
{
	kTerminalFile_WindowTextColorIndexNormalText	= 0,	//!< normal text color - index 5 
															//   should have the same value
	kTerminalFile_WindowTextColorIndexBoldText		= 2,	//!< bold text color - index 3 
															//!  should have the same value
	kTerminalFile_WindowTextColorIndexBackground	= 4,	//!< background color - index 1 
															//   should have the same value
	kTerminalFile_WindowTextColorIndexSelection		= 6,	//!< highlight color
	kTerminalFile_WindowTextColorIndexCursor		= 7		//!< cursor color
};

/*!
Use these masks to test against the value returned from 
"kTerminalFile_AttributeTagWindowTitleBits" of 
"kTerminalFile_SettingsTypeWindow", indicating what information 
should be used to construct the window title.
*/
enum
{
	kTerminalFile_WindowTitleMaskShellCommandName		= 1 << 0,	//!< user's shell program
	kTerminalFile_WindowTitleMaskTerminalDeviceName		= 1 << 1,	//!< name of TTY (teletypewriter/pseudo-terminal 
																	//!  device, e.g. "ttyp1" for "/dev/ttyp1")
	kTerminalFile_WindowTitleMaskTerminalDimensions		= 1 << 2,	//!< "WWxHH" where WW is the width  
																	//!  in columns and HH is the 
																	//!  height in rows
	kTerminalFile_WindowTitleMaskHasCustomTitle			= 1 << 3,	//!< window has a custom title, use the string from 
																	//!  "kTerminalFile_AttributeTagWindowCustomTitle" 
																	//!  and "kTerminalFile_SettingsTypeWindow"
	kTerminalFile_WindowTitleMaskDotTermFilename		= 1 << 4,	//!< name of ".term" file for the session
	kTerminalFile_WindowTitleMaskCommandKeyToActivate	= 1 << 5,	//!< key combination to activate the window
	kTerminalFile_WindowTitleMaskActiveProcessName		= 1 << 6	//!< name of foreground process
};

/*!
Use these constants to compare against the value returned from 
"kTerminalFile_AttributeTagWindowTerminalType", indicating what 
kind of terminal emulator to use.
*/
#define kTerminalFile_WindowTermTypeANSI		CFSTR("ansi")			//!< ANSI
#define kTerminalFile_WindowTermTypeRXVT		CFSTR("rxvt")			//!< RXVT
#define kTerminalFile_WindowTermTypeVT52		CFSTR("vt52")			//!< VT52
#define kTerminalFile_WindowTermTypeVT100		CFSTR("vt100")			//!< VT100
#define kTerminalFile_WindowTermTypeVT102		CFSTR("vt102")			//!< VT102
#define kTerminalFile_WindowTermTypeXTerm		CFSTR("xterm")			//!< XTerm
#define kTerminalFile_WindowTermTypeXTermColor	CFSTR("xterm-color")	//!< color XTerm

/*!
Use these constants with one of the four macros as keys to 
access data in the dictionary returned from 
"kTerminalFile_AttributeTagWindowKeyMappings".  These constants 
match the values in <AppKit/NSEvent.h> so be careful to preserve 
that mapping.
*/
#define kTerminalFile_WindowCursorDown		"F701"
#define kTerminalFile_WindowCursorLeft		"F702"
#define kTerminalFile_WindowCursorRight		"F703"
#define kTerminalFile_WindowCursorUp		"F700"
#define kTerminalFile_WindowDel				"F728"
#define kTerminalFile_WindowEnd				"F72B"
#define kTerminalFile_WindowF1				"F704"
#define kTerminalFile_WindowF2				"F705"
#define kTerminalFile_WindowF3				"F706"
#define kTerminalFile_WindowF4				"F707"
#define kTerminalFile_WindowF5				"F708"
#define kTerminalFile_WindowF6				"F709"
#define kTerminalFile_WindowF7				"F70A"
#define kTerminalFile_WindowF8				"F70B"
#define kTerminalFile_WindowF9				"F70C"
#define kTerminalFile_WindowF10				"F70D"
#define kTerminalFile_WindowF11				"F70E"
#define kTerminalFile_WindowF12				"F70F"
#define kTerminalFile_WindowF13				"F710"
#define kTerminalFile_WindowF14				"F711"
#define kTerminalFile_WindowF15				"F712"
#define kTerminalFile_WindowF16				"F713"
#define kTerminalFile_WindowF17				"F714"
#define kTerminalFile_WindowF18				"F715"
#define kTerminalFile_WindowF19				"F716"
#define kTerminalFile_WindowF20				"F717"
#define kTerminalFile_WindowHome			"F729"
#define kTerminalFile_WindowPageDown		"F72D"
#define kTerminalFile_WindowPageUp			"F72C"

//define NoModifiers(keyCode) 	CFSTR(keyCode)
//define ControlKey(keyCode)	CFSTR("^" keyCode)
//define OptionKey(keyCode)	CFSTR("~" keyCode)
//define ShiftKey(keyCode)		CFSTR("$" keyCode)

/*!
Use these constants to compare the value returned from the 
"kTerminalFile_AttributeTagWindowKeyMappings" dictionary using 
one of the keys defined above.  If the value does not match one 
of these constants, then assume that the action is "send string 
to shell" and that the string to send is the value.
*/
#define kTerminalFile_WindowKeyActionScrollPageDown		CFSTR("scrollPageDown:")
#define kTerminalFile_WindowKeyActionScrollBufferEnd	CFSTR("scrollToEndOfDocument:")
#define kTerminalFile_WindowKeyActionScrollBufferStart	CFSTR("scrollToBeginningOfDocument:")
#define kTerminalFile_WindowKeyActionScrollPageUp		CFSTR("scrollPageUp:")



#pragma mark Types

typedef struct TerminalFile_OpaqueStructure*		TerminalFileRef;

typedef enum TerminalFile_Result					TerminalFile_Result;

typedef enum TerminalFile_Version					TerminalFile_Version;

typedef enum TerminalFile_SettingsType				TerminalFile_SettingsType;

typedef enum TerminalFile_AttributeTag				TerminalFile_AttributeTag;
typedef void*										TerminalFile_AttributeValuePtr;
typedef void const*									TerminalFile_AttributeValueConstPtr;

typedef enum TerminalFile_WindowCloseAction			TerminalFile_WindowCloseAction;
typedef enum TerminalFile_WindowMetaKeyMapping		TerminalFile_WindowMetaKeyMapping;
typedef enum TerminalFile_WindowShellExitAction		TerminalFile_WindowShellExitAction;
typedef OptionBits									TerminalFile_WindowTitleBits;



#pragma mark Public Methods

//!\name Creating and Destroying Terminal File Objects
//@{

// NO "TerminalFile_New()" EXISTS YET; IF WRITING TERMINAL FILES IS EVER SUPPORTED, 
// ADD A TerminalFile_New()

OSStatus
	TerminalFile_NewFromFile			(CFURLRef							inFileURL,
										 TerminalFileRef*					outTermFile);

void
	TerminalFile_Dispose				(TerminalFileRef*					inoutTermFilePtr);

//@}

//!\name Working With Terminal Files
//@{

TerminalFile_Result
	TerminalFile_GetAttributes			(TerminalFileRef					inTermFile,
										 TerminalFile_SettingsType			inSettingsType,
										 CFIndex							inSettingsIndex,
										 ItemCount							inAttributeCount,
										 TerminalFile_AttributeTag const	inTagArray[],
										 TerminalFile_AttributeValuePtr		outValueArray[]);

CFIndex
	TerminalFile_ReturnSettingsCount	(TerminalFileRef					inTermFile,
										 TerminalFile_SettingsType			inSettingsType);

TerminalFile_Version
	TerminalFile_ReturnVersion			(TerminalFileRef					inTermFile);

//@}

#ifdef __cplusplus
}
#endif

#endif // __TERMINALFILE__

// BELOW IS REQUIRED NEWLINE TO END FILE
