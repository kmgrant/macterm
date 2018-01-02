/*!	\file SessionDescription.h
	\brief All code related to opening and saving session
	description files.
	
	Note that this will eventually be replaced by pure Python
	code.  A basic parser is already implemented in Python,
	but the Quills API must be extended to allow all of the
	data from this file format to be passed in.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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



#pragma mark Constants

enum SessionDescription_Result
{
	kSessionDescription_ResultOK						= 0,	//!< no error occurred
	kSessionDescription_ResultDataUnavailable			= 1,	//!< file does not contain specified type of data
	kSessionDescription_ResultDataNotAllowed			= 2,	//!< file cannot contain specified type of data
	kSessionDescription_ResultGenericFailure			= 3,	//!< unknown kind of error occurred
	kSessionDescription_ResultParameterError			= 4,	//!< some problem with given input
	kSessionDescription_ResultInsufficientBufferSpace	= 5,	//!< not enough room in a given memory block
	kSessionDescription_ResultFileError					= 6,	//!< some file-related error (e.g. EOF)
	kSessionDescription_ResultInvalidValue				= 7,	//!< if you asked that data be validated before storage,
																//!  this result indicates there is something wrong with
																//!  the data you provided
	kSessionDescription_ResultUnknownType				= 8		//!< parameter error; type identifier not among the
																//!  expected set of values
};

enum SessionDescription_ContentType
{
	kSessionDescription_ContentTypeCommand				= 'Shll',	//!< represents local shell
	kSessionDescription_ContentTypeUnknown				= '----'	//!< unknown contents, either because the
																	//!  data model has just been created or
																	//!  perhaps has been read from a file
																	//!  created by a future MacTerm version
};

/*!
The following constants represent information in Session Files
that can be returned as true or false (yes or no) values.  See
comments for each one to determine the types of Session Files
it applies to.
*/
enum SessionDescription_BooleanType
{
	/*!
	True if a TEK PAGE command clears the screen instead of opening a new
	window.  Available for "kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_BooleanTypeTEKPageClears					= 'TEKC',
	
	/*!
	True if a carriage return maps to CR-null instead of CR-LF.  Available
	for "kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_BooleanTypeRemapCR							= 'BkCR',
	
	/*!
	True if page keys are sent to the remote server or running process instead
	of controlling the terminal view directly.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_BooleanTypePageKeysDoNotControlTerminal		= 'PgUp',
	
	/*!
	True if the 4 top keypad keys are remapped to the VT220 keypad instead of
	being interpreted as their usual values.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_BooleanTypeRemapKeypadTopRow				= 'PFKy'
};

/*!
The following constants represent information in Session Files
that can be returned as signed integers.  See comments for each
one to determine the types of Session Files it applies to.
*/
enum SessionDescription_IntegerType
{
	/*!
	Number of lines in the scrollback buffer.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_IntegerTypeScrollbackBufferLineCount		= 'Sclb',
	
	/*!
	Number of columns of text allowed in the terminal screen.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_IntegerTypeTerminalVisibleColumnCount		= 'Cols',
	
	/*!
	Number of rows of text in the main terminal screen area.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_IntegerTypeTerminalVisibleLineCount			= 'Rows',
	
	/*!
	Screen position of window as a pixel offset from the left display edge.
	Available for "kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_IntegerTypeWindowContentLeftEdge			= 'Left',
	
	/*!
	Screen position of window as a pixel offset from the top display edge.
	Available for "kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_IntegerTypeWindowContentTopEdge				= 'TopE',
	
	/*!
	Size in points of the terminal font.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_IntegerTypeTerminalFontSize					= 'FSiz'
};

/*!
The following constants represent information in Session Files
that can be returned as an RGBColor.  See comments for each
one to determine the types of Session Files it applies to.
*/
enum SessionDescription_RGBColorType
{
	/*!
	Color of foreground (text) when no style is applied.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_RGBColorTypeTextNormal				= 'Text',
	
	/*!
	Color of background (cell) when no style is applied.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_RGBColorTypeBackgroundNormal		= 'Back',
	
	/*!
	Color of foreground (text) when bold style is applied.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_RGBColorTypeTextBold				= 'Bold',
	
	/*!
	Color of background (text) when bold style is applied.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_RGBColorTypeBackgroundBold			= 'BBck',
	
	/*!
	Color of foreground (text) when blinking style is applied.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_RGBColorTypeTextBlinking			= 'Blnk',
	
	/*!
	Color of background (cell) when blinking style is applied.  Available for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_RGBColorTypeBackgroundBlinking		= 'BlBk'
};

/*!
The following constants represent information in Session Files
that can be returned as a CFStringRef.  See comments for each
one to determine the types of Session Files it applies to.
*/
enum SessionDescription_StringType
{
	/*!
	The entire command line defining a spawned process.  Available only for
	"kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_StringTypeCommandLine		= 'CmdL',
	
	/*!
	The name of a window.  Available for "kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_StringTypeWindowName		= 'WinN',
	
	/*!
	The name of a window.  Available for "kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_StringTypeTerminalFont		= 'Font',
	
	/*!
	The perceived emulation type, also known as the answer-back message.
	Normally something like "vt100" or "vt220", but could be any value to
	trick a remote application into thinking the terminal matches another
	type.  Available for "kSessionDescription_ContentTypeCommand".
	*/
	kSessionDescription_StringTypeAnswerBack		= 'Term',
	
	/*!
	A description of the toolbar view options or visibility; if "hidden",
	the toolbar is invisible.  Other possible values are "icon+text",
	"icon+text+small", "icon", "icon+small", "text" and "text+small".
	*/
	kSessionDescription_StringTypeToolbarInfo		= 'Tbar',
	
	/*!
	The name of the Macro Set preferences collection that should be enabled
	when this session is active.  If the string is empty or does not match
	any valid set, then no macros are enabled (the None set).
	*/
	kSessionDescription_StringTypeMacroSet			= 'Mcro'
};

#pragma mark Types

typedef struct SessionDescription_OpaqueStructure*		SessionDescription_Ref;



#pragma mark Public Methods

//!\name Creating and Destroying Session File Objects
//@{

SessionDescription_Ref
	SessionDescription_New						(SessionDescription_ContentType		inIntendedContents);

SessionDescription_Ref
	SessionDescription_NewFromFile				(SInt16								inFileReferenceNumber,
												 SessionDescription_ContentType*	outFileTypePtr);

void
	SessionDescription_Retain					(SessionDescription_Ref				inRef);

void
	SessionDescription_Release					(SessionDescription_Ref*			inoutRefPtr);

//@}

//!\name Retrieving Parsed Data
//@{

SessionDescription_Result
	SessionDescription_GetBooleanData			(SessionDescription_Ref				inRef,
												 SessionDescription_BooleanType		inType,
												 Boolean&							outFlag);

SessionDescription_Result
	SessionDescription_GetIntegerData			(SessionDescription_Ref				inRef,
												 SessionDescription_IntegerType		inType,
												 SInt32&							outNumber);

SessionDescription_Result
	SessionDescription_GetRGBColorData			(SessionDescription_Ref				inRef,
												 SessionDescription_RGBColorType	inType,
												 CGDeviceColor&						outColor);

SessionDescription_Result
	SessionDescription_GetStringData			(SessionDescription_Ref				inRef,
												 SessionDescription_StringType		inType,
												 CFStringRef&						outString);

//@}

//!\name Setting New Data
//@{

SessionDescription_Result
	SessionDescription_SetBooleanData			(SessionDescription_Ref				inRef,
												 SessionDescription_BooleanType		inType,
												 Boolean							inFlag);

SessionDescription_Result
	SessionDescription_SetIntegerData			(SessionDescription_Ref				inRef,
												 SessionDescription_IntegerType		inType,
												 SInt32								inNumber,
												 Boolean							inValidateBeforeStoring = false);

SessionDescription_Result
	SessionDescription_SetRGBColorData			(SessionDescription_Ref				inRef,
												 SessionDescription_RGBColorType	inType,
												 CGDeviceColor const&				inColor);

SessionDescription_Result
	SessionDescription_SetStringData			(SessionDescription_Ref				inRef,
												 SessionDescription_StringType		inType,
												 CFStringRef						inString,
												 Boolean							inValidateBeforeStoring = false);

//@}

//!\name Saving Changed Data
//@{

SessionDescription_Result
	SessionDescription_Save						(SessionDescription_Ref				inRef,
												 SInt16								inFileReferenceNumber);

//@}

//!\name Miscellaneous
//@{

// DEPRECATED
void
	SessionDescription_Load						();

Boolean
	SessionDescription_ReadFromFile				(FSRef const&						inFile);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
