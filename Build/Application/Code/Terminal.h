/*!	\file Terminal.h
	\brief Terminal screen buffer and emulators.
	
	MacTelnet splits terminals into two main concepts.  The
	first is the Screen, which this file implements, consisting
	of a screen buffer and underlying emulator that parses all
	data inserted into the terminal.  The second is the View
	(see TerminalView.h), which is essentially the one or more
	Mac OS window controls that render a terminal screen.
	
	Simply put, a Screen drives the back-end, and a View drives
	the front-end.  MacTelnet 3.0 significantly advances the
	NCSA Telnet code base by allowing Screens and Views to have
	unlimited association.  What this means is that it will soon
	be trivial to add any of the following features: viewing the
	same terminal in more than one window, viewing the same
	terminal in multiple (split-pane) views in the same window,
	or using the same view to overlay data from any number of
	source screen buffers!
	
	This was originally the NCSA virtual screen kernel, written
	in large part by Gaige B. Paulsen.  It has been revamped in
	MacTelnet 3.0.
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

#ifndef __TERMINAL__
#define __TERMINAL__

// library includes
#include "ListenerModel.h"

// MacTelnet includes
#include "SessionRef.typedef.h"
#include "TerminalSpeaker.h"
#include "TerminalTextAttributes.typedef.h"
#include "TerminalViewRef.typedef.h"



#pragma mark Constants

/*!
Possible return values from certain APIs in this module.
*/
enum Terminal_Result
{
	kTerminal_ResultOK = 0,						//!< no error
	kTerminal_ResultInvalidID = -1,				//!< a given "TerminalScreenRef" does not correspond to any known screen
	kTerminal_ResultInvalidIterator = -2,		//!< a given "Terminal_LineRef" does not correspond to any known row
	kTerminal_ResultParameterError = -3,		//!< invalid input (e.g. a null pointer)
	kTerminal_ResultNotEnoughMemory = -4,		//!< there is not enough memory to allocate required data structures
	kTerminal_ResultIteratorCannotAdvance = -5  //!< attempt to advance iterator past the end of its list
};

/*!
Setting changes that MacTelnet allows other modules to ÒlistenÓ for, via
Terminal_StartMonitoring().
*/
typedef FourCharCode Terminal_Change;
enum
{
	kTerminal_ChangeAudioEvent			= FOUR_CHAR_CODE('Bell'),	//!< terminal bell triggered (context: TerminalScreenRef)
	kTerminal_ChangeAudioState			= FOUR_CHAR_CODE('BEnD'),	//!< terminal bell enabled or disabled (context: TerminalScreenRef);
																	//!  use Terminal_BellIsEnabled() to determine the new state
	kTerminal_ChangeCursorLocation		= FOUR_CHAR_CODE('Curs'),	//!< cursor has moved; new position can be found with
																	//!  Terminal_CursorGetLocation() (context:
																	//!  TerminalScreenRef)
	kTerminal_ChangeCursorState			= FOUR_CHAR_CODE('CurV'),	//!< cursor has been shown or hidden; new state can be
																	//!  found with Terminal_CursorIsVisible() (context:
																	//!  TerminalScreenRef)
	kTerminal_ChangeFileCaptureBegun	= FOUR_CHAR_CODE('CapB'),	//!< file capture started (context: TerminalScreenRef)
	kTerminal_ChangeFileCaptureEnding	= FOUR_CHAR_CODE('CapE'),	//!< capture about to stop (context: TerminalScreenRef)
	kTerminal_ChangeLineFeedNewLineMode	= FOUR_CHAR_CODE('LFNL'),	//!< terminal has changed the expected behavior of the
																	//!  Return key; use Terminal_LineFeedNewLineMode()
																	//!  to determine the new mode (context: TerminalScreenRef)
	kTerminal_ChangeNewLEDState			= FOUR_CHAR_CODE('LEDS'),	//!< the state of at least one LED in a monitored
																	//!  Terminal has changed (context: TerminalScreenRef)
	kTerminal_ChangeReset				= FOUR_CHAR_CODE('Rset'),	//!< terminal was explicitly reset (context:
																	//!  TerminalScreenRef)
	kTerminal_ChangeScrollActivity		= FOUR_CHAR_CODE('^v<>'),	//!< screen size or visible area has changed
																	//!  (context: TerminalScreenRef)
	kTerminal_ChangeText				= FOUR_CHAR_CODE('UpdT'),	//!< text has changed, requiring an update (context:
																	//!  Terminal_RangeDescriptionConstPtr)
	kTerminal_ChangeVideoMode			= FOUR_CHAR_CODE('RevV'),	//!< terminal has toggled between normal and reverse
																	//!  video modes; use Terminal_ReverseVideoIsEnabled()
																	//!  to determine the new mode (context: TerminalScreenRef)
	kTerminal_ChangeWindowFrameTitle	= FOUR_CHAR_CODE('WinT'),	//!< terminal received a new title meant for its window;
																	//!  use Terminal_CopyTitleForWindow() to determine title
																	//!  (context: TerminalScreenRef)
	kTerminal_ChangeWindowIconTitle		= FOUR_CHAR_CODE('IcnT'),	//!< terminal received a new title meant for its icon;
																	//!  use Terminal_CopyTitleForIcon() to determine title
																	//!  (context: TerminalScreenRef)
	kTerminal_ChangeWindowMinimization	= FOUR_CHAR_CODE('MnmR')	//!< terminal received a request to minimize or restore;
																	//!  use Terminal_WindowIsToBeMinimized() for more info
																	//!  (context: TerminalScreenRef)
};

#ifndef REZ
typedef UInt32 Terminal_Emulator;
typedef UInt32 Terminal_EmulatorType; // part of Terminal_Emulator
typedef UInt32 Terminal_EmulatorVariant; // part of Terminal_Emulator
#endif

enum
{
	// These masks chop up the 16-bit emulator type into two parts,
	// the terminal type and the variant of it; this allows up to 256
	// terminal types, and 256 variants (for example, VT is a type,
	// and VT100 and VT220 are variants of the VT terminal type).
	//
	// Standardizing on this approach will make it *much* easier to
	// implement future terminal types - for example, many variants
	// of terminals share identical features, so you can check if
	// ANY variant of a particular terminal is in use just by
	// isolating the upper byte.  For convenience, two macros below
	// are included to isolate the upper or lower byte for you.
	// Use them!!!
	kTerminal_EmulatorTypeByteShift		= 8,
	kTerminal_EmulatorTypeMask			= (0x000000FF << kTerminal_EmulatorTypeByteShift),
	kTerminal_EmulatorVariantByteShift	= 0,
	kTerminal_EmulatorVariantMask		= (0x000000FF << kTerminal_EmulatorVariantByteShift)
};
enum
{
	// use these constants only when you need to determine the terminal emulator family
	// (and if you add support for new terminal types, add constants to this list in
	// the same way as shown below)
	kTerminal_EmulatorTypeVT = ((0 << kTerminal_EmulatorTypeByteShift) & kTerminal_EmulatorTypeMask),
		kTerminal_EmulatorVariantVT100 = ((0x00 << kTerminal_EmulatorVariantByteShift) & kTerminal_EmulatorVariantMask),
		kTerminal_EmulatorVariantVT102 = ((0x01 << kTerminal_EmulatorVariantByteShift) & kTerminal_EmulatorVariantMask),
		kTerminal_EmulatorVariantVT220 = ((0x02 << kTerminal_EmulatorVariantByteShift) & kTerminal_EmulatorVariantMask),
		kTerminal_EmulatorVariantVT320 = ((0x03 << kTerminal_EmulatorVariantByteShift) & kTerminal_EmulatorVariantMask),
		kTerminal_EmulatorVariantVT420 = ((0x04 << kTerminal_EmulatorVariantByteShift) & kTerminal_EmulatorVariantMask),
	kTerminal_EmulatorTypeXTerm = ((1 << kTerminal_EmulatorTypeByteShift) & kTerminal_EmulatorTypeMask),
		kTerminal_EmulatorVariantXTermOriginal = ((0x00 << kTerminal_EmulatorVariantByteShift) & kTerminal_EmulatorVariantMask),
		kTerminal_EmulatorVariantXTermColor = ((0x01 << kTerminal_EmulatorVariantByteShift) & kTerminal_EmulatorVariantMask),
	kTerminal_EmulatorTypeDumb = ((2 << kTerminal_EmulatorTypeByteShift) & kTerminal_EmulatorTypeMask),
		kTerminal_EmulatorVariantDumb1 = ((0x00 << kTerminal_EmulatorVariantByteShift) & kTerminal_EmulatorVariantMask),
	kTerminal_EmulatorTypeANSI = ((3 << kTerminal_EmulatorTypeByteShift) & kTerminal_EmulatorTypeMask),
		kTerminal_EmulatorVariantANSIBBS = ((0x00 << kTerminal_EmulatorVariantByteShift) & kTerminal_EmulatorVariantMask),
		kTerminal_EmulatorVariantANSISCO = ((0x01 << kTerminal_EmulatorVariantByteShift) & kTerminal_EmulatorVariantMask)
};
enum
{
	// refer to a terminal type using these simpler constants
	kTerminal_EmulatorANSIBBS = kTerminal_EmulatorTypeANSI | kTerminal_EmulatorVariantANSIBBS,				// PC (ÒANSIÓ) terminals
	kTerminal_EmulatorANSISCO = kTerminal_EmulatorTypeANSI | kTerminal_EmulatorVariantANSISCO,
	kTerminal_EmulatorVT100 = kTerminal_EmulatorTypeVT | kTerminal_EmulatorVariantVT100,					// VT terminals
	kTerminal_EmulatorVT102 = kTerminal_EmulatorTypeVT | kTerminal_EmulatorVariantVT102,
	kTerminal_EmulatorVT220 = kTerminal_EmulatorTypeVT | kTerminal_EmulatorVariantVT220,
	kTerminal_EmulatorVT320	= kTerminal_EmulatorTypeVT | kTerminal_EmulatorVariantVT320,
	kTerminal_EmulatorVT420 = kTerminal_EmulatorTypeVT | kTerminal_EmulatorVariantVT420,
	kTerminal_EmulatorXTermOriginal = kTerminal_EmulatorTypeXTerm | kTerminal_EmulatorVariantXTermOriginal,	// xterm terminals
	kTerminal_EmulatorXTermColor = kTerminal_EmulatorTypeXTerm | kTerminal_EmulatorVariantXTermColor,
	kTerminal_EmulatorDumb = kTerminal_EmulatorTypeDumb | kTerminal_EmulatorVariantDumb1					// ÒdumbÓ terminals
};

/*!
Controls Terminal_Reset().
*/
typedef UInt32 Terminal_ResetFlags;
enum
{
	kTerminal_ResetFlagsGraphicsCharacters  = (1 << 0),		//!< pass this value to reset only the active
															//!  character set; this is primarily used when
															//!  something screws up (either in MacTelnet
															//!  or in the program using the terminal)
															//!  that leaves the screen rendered entirely
															//!  in the graphics character set
	kTerminal_ResetFlagsAll					= 0xFFFFFFFF	//!< pass this value to do a full reset
};

/*!
Controls over text-finding behavior.
*/
typedef UInt32 Terminal_SearchFlags;
enum
{
	kTerminal_SearchFlagsCaseSensitive		= (1 << 0),		//!< lowercase and uppercase letters not considered the same?
	kTerminal_SearchFlagsSearchBackwards	= (1 << 1),		//!< search oldest (topmost, offscreen) rows first?
	kTerminal_SearchFlagsWrapAround			= (1 << 2),		//!< continue search from top after bottom is hit?
	kTerminal_SearchFlagsReverseBuffer		= (1 << 3)		//!< search bottommost lines first?
};

/*!
Controls over the computerÕs voice when it is speaking text.
*/
enum Terminal_SpeechMode
{
	kTerminal_SpeechModeSpeakAlways = 0,		//!< no restrictions on speech
	kTerminal_SpeechModeSpeakWhenActive = 1,	//!< mute speech if the terminal window is not frontmost
	kTerminal_SpeechModeSpeakWhenInactive = 2	//!< mute speech if the terminal window is frontmost
};

/*!
Controls over text-copying behavior, given the ambiguity of
two end points.
*/
typedef UInt32 Terminal_TextCopyFlags;
enum
{
	kTerminal_TextCopyFlagsRectangular					= (1 << 0),		//!< only considers text within a rectangular area
	kTerminal_TextCopyFlagsAlwaysNewLineAtRightMargin	= (1 << 1)		//!< normally, the new-line sequence is skipped for
																		//!  any line where the copy area includes the right
																		//!  margin and the right margin character is not a
																		//!  whitespace character; set this flag to force
																		//!  new-line appendages in these cases
};

/*!
Controls over word-finding behavior.  Currently, none are
defined, so the value should be set to 0.
*/
typedef UInt32 Terminal_WordFlags;

#pragma mark Types

#include "TerminalScreenRef.typedef.h"

typedef struct Terminal_OpaqueLineIterator*		Terminal_LineRef;	//!< efficient access to an arbitrary screen line

struct Terminal_RangeDescription
{
	TerminalScreenRef	screen;				//!< the screen for which this text range applies
	SInt16				firstRow;			//!< zero-based row number where range occurs; 0 is topmost main screen line,
											//!  a negative line number is in the scrollback buffer
	UInt16				firstColumn;		//!< zero-based column number where range begins
	UInt16				columnCount;		//!< number of columns wide the range is; if 0, the range is empty
	UInt16				rowCount;			//!< number of rows the range covers (it is rectangular, not flush to the edges)
};
typedef Terminal_RangeDescription const*	Terminal_RangeDescriptionConstPtr;

#pragma mark Callbacks

/*!
Screen Run Routine

This defines a function that can be used as an iterator
over all contiguous blocks of text in a virtual screen
that share *exactly* the same attributes.  The specified
text buffer (which is read-only) includes the contents of
the current chunk of text, whose starting column is also
given - assuming a renderer needs to know this.  The
specified text attributes apply to every character in the
chunk, and *include* any attributes that are actually
applied to the entire line (double-sized text, for
instance).

This callback acts on text chunks that are not necessarily
entire lines, and is guaranteed to be called with a series
of characters whose attributes all match.  The expectation
is that you are using this for rendering purposes.

IMPORTANT:  The line text buffer may be nullptr, and if it
			is, you should still pay attention to the
			length value; it implies a blank area of that
			many characters in length.
*/
typedef void (*Terminal_ScreenRunProcPtr)	(TerminalScreenRef			inScreen,
											 char const*				inLineTextBufferOrNull,
											 UInt16						inLineTextBufferLength,
											 Terminal_LineRef			inRow,
											 UInt16						inZeroBasedStartColumnNumber,
											 TerminalTextAttributes		inAttributes,
											 void*						inContextPtr);
inline void
Terminal_InvokeScreenRunProc	(Terminal_ScreenRunProcPtr		inUserRoutine,
								 TerminalScreenRef				inScreen,
								 char const*					inLineTextBufferOrNull,
								 UInt16							inLineTextBufferLength,
								 Terminal_LineRef				inRow,
								 UInt16							inZeroBasedStartColumnNumber,
								 TerminalTextAttributes			inAttributes,
								 void*							inContextPtr)
{
	(*inUserRoutine)(inScreen, inLineTextBufferOrNull, inLineTextBufferLength,
						inRow, inZeroBasedStartColumnNumber, inAttributes, inContextPtr);
}



#pragma mark Public Methods

//!\name Creating and Destroying Terminal Screen Buffers
//@{

Terminal_Result
	Terminal_NewScreen						(SInt16						inLineCountScrollBackBuffer,
											 SInt16						inLineCountVisibleRows,
											 SInt16						inMaximumColumnCount,
											 Boolean					inForceLineSaving,
											 TerminalScreenRef*			outScreenPtr);

SInt16
	Terminal_DisposeScreen					(TerminalScreenRef			inScreen);

//@}

//!\name Enabling Session Talkback (Such As VT100 Device Attributes)
//@{

Terminal_Result
	Terminal_SetListeningSession			(TerminalScreenRef			inScreen,
											 SessionRef					inSession);

//@}

//!\name Creating and Destroying Terminal Screen Buffer Iterators
//@{

Terminal_LineRef
	Terminal_NewMainScreenLineIterator		(TerminalScreenRef			inScreen,
											 UInt16						inLineNumberZeroForTop);

Terminal_LineRef
	Terminal_NewScrollbackLineIterator		(TerminalScreenRef			inScreen,
											 UInt16						inLineNumberZeroForNewest);

void
	Terminal_DisposeLineIterator			(Terminal_LineRef*			inoutIteratorPtr);

//@}

//!\name Buffer Size
//@{

void
	Terminal_GetMaximumBounds				(TerminalScreenRef			inRef,
											 SInt16*					outZeroBasedTopScrollbackRowPtrOrNull,
											 SInt16*					outZeroBasedLeftColumnPtrOrNull,
											 SInt16*					outZeroBasedBottomRowPtrOrNull,
											 SInt16*					outZeroBasedRightColumnPtrOrNull);

UInt16
	Terminal_ReturnAllocatedColumnCount		();

UInt16
	Terminal_ReturnColumnCount				(TerminalScreenRef			inScreen);

UInt16
	Terminal_ReturnInvisibleRowCount		(TerminalScreenRef			inScreen);

UInt16
	Terminal_ReturnRowCount					(TerminalScreenRef			inScreen);

Terminal_Result
	Terminal_SetVisibleColumnCount			(TerminalScreenRef			inScreen,
											 UInt16						inNewNumberOfCharactersWide);

Terminal_Result
	Terminal_SetVisibleRowCount				(TerminalScreenRef			inScreen,
											 UInt16						inNewNumberOfLinesHigh);

//@}

//!\name Buffer Iteration
//@{

Terminal_Result
	Terminal_ForEachLikeAttributeRunDo		(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inRow,
											 Terminal_ScreenRunProcPtr	inDoWhat,
											 void*						inContextPtr);

Terminal_Result
	Terminal_LineIteratorAdvance			(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inRow,
											 SInt16						inHowManyRowsForwardOrNegativeForBackward);

//@}

//!\name Buffer Search
//@{

Boolean
	Terminal_SearchForPhrase				(TerminalScreenRef			inScreen,
											 char const*				inPhraseBuffer,
											 UInt32						inPhraseBufferLength,
											 Terminal_SearchFlags		inFlags);

Boolean
	Terminal_SearchForWord					(TerminalScreenRef			inScreen,
											 SInt16						inZeroBasedStartColumn,
											 Terminal_LineRef			inRow,
											 SInt16*					outWordStartColumnPtr,
											 Terminal_LineRef*			outWordStartRowPtr,
											 SInt16*					outWordEndColumnPtr,
											 Terminal_LineRef*			outWordEndRowPtr,
											 Terminal_WordFlags			inFlags);

//@}

//!\name Accessing Screen Data
//@{

Terminal_Result
	Terminal_ChangeLineAttributes			(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inRow,
											 TerminalTextAttributes		inAttributesToSet,
											 TerminalTextAttributes		inAttributesToClear);

Terminal_Result
	Terminal_ChangeLineRangeAttributes		(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inRow,
											 UInt16						inZeroBasedStartColumn,
											 SInt16						inZeroBasedPastTheEndColumnOrNegativeForLastColumn,
											 TerminalTextAttributes		inAttributesToSet,
											 TerminalTextAttributes		inAttributesToClear);

Terminal_Result
	Terminal_ChangeRangeAttributes			(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inStartRow,
											 UInt16						inNumberOfRowsToConsider,
											 UInt16						inZeroBasedStartColumn,
											 UInt16						inZeroBasedPastTheEndColumn,
											 Boolean					inConstrainToRectangle,
											 TerminalTextAttributes		inAttributesToSet,
											 TerminalTextAttributes		inAttributesToClear);

Terminal_Result
	Terminal_CopyLineRange					(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inRow,
											 UInt16						inZeroBasedStartColumn,
											 SInt16						inZeroBasedEndColumnOrNegativeForLastColumn,
											 char*						outBuffer,
											 SInt32						inBufferLength,
											 SInt32*					outActualLengthPtrOrNull,
											 UInt16						inNumberOfSpacesPerTabOrZeroForNoSubstitution);

Terminal_Result
	Terminal_CopyRange						(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inStartRow,
											 UInt16						inNumberOfRowsToConsider,
											 UInt16						inZeroBasedStartColumnOnFirstRow,
											 UInt16						inZeroBasedEndColumnOnLastRow,
											 char*						outBuffer,
											 SInt32						inBufferLength,
											 SInt32*					outActualLengthPtrOrNull,
											 char const*				inEndOfLineSequence,
											 SInt16						inNumberOfSpacesPerTabOrZeroForNoSubstitution,
											 Terminal_TextCopyFlags		inFlags);

OSStatus
	Terminal_CreateContentsAEDesc			(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inStartRow,
											 UInt16						inNumberOfRowsToConsider,
											 AEDesc*					outDescPtr);

void
	Terminal_DeleteAllSavedLines			(TerminalScreenRef			inScreen);

Terminal_Result
	Terminal_GetLineGlobalAttributes		(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inRow,
											 TerminalTextAttributes*	outAttributesPtr);

Terminal_Result
	Terminal_GetLine						(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inRow,
											 char const*&				outReferenceStart,
											 char const*&				outReferencePastEnd);

Terminal_Result
	Terminal_GetLineRange					(TerminalScreenRef			inScreen,
											 Terminal_LineRef			inRow,
											 UInt16						inZeroBasedStartColumn,
											 SInt16						inZeroBasedPastEndColumnOrNegativeForLastColumn,
											 char const*&				outReferenceStart,
											 char const*&				outReferencePastEnd);

//@}

//!\name Terminal State
//@{

Boolean
	Terminal_BellIsEnabled					(TerminalScreenRef			inScreen);

void
	Terminal_CopyTitleForIcon				(TerminalScreenRef			inRef,
											 CFStringRef&				outTitle);

void
	Terminal_CopyTitleForWindow				(TerminalScreenRef			inRef,
											 CFStringRef&				outTitle);

Terminal_Result
	Terminal_CursorGetLocation				(TerminalScreenRef			inScreen,
											 UInt16*					outZeroBasedColumnPtr,
											 UInt16*					outZeroBasedRowPtr);

Boolean
	Terminal_CursorIsVisible				(TerminalScreenRef			inScreen);

Terminal_Result
	Terminal_EmulatorDeriveFromCString		(TerminalScreenRef			inScreen,
											 char const*				inCString,
											 Terminal_Emulator&			outApparentEmulator);

// DEPRECATED
Boolean
	Terminal_EmulatorIsVT100				(TerminalScreenRef			inScreen);

// DEPRECATED
Boolean
	Terminal_EmulatorIsVT220				(TerminalScreenRef			inScreen);

void
	Terminal_EmulatorSet					(TerminalScreenRef			inScreen,
											 Terminal_Emulator			inEmulator);

Boolean
	Terminal_KeypadHasApplicationKeys		(TerminalScreenRef			inScreen);

Boolean
	Terminal_KeypadHasMovementKeys			(TerminalScreenRef			inScreen);

Boolean
	Terminal_LEDIsOn						(TerminalScreenRef			inScreen,
											 SInt16						inOneBasedLEDNumber);

Boolean
	Terminal_LineFeedNewLineMode			(TerminalScreenRef			inScreen);

Boolean
	Terminal_LineWrapIsEnabled				(TerminalScreenRef			inScreen);

void
	Terminal_Reset							(TerminalScreenRef			inScreen,
											 Terminal_ResetFlags		inFlags = kTerminal_ResetFlagsAll);

UInt16
	Terminal_ReturnNextTabDistance			(TerminalScreenRef			inScreen);

Boolean
	Terminal_ReverseVideoIsEnabled			(TerminalScreenRef			inScreen);

Boolean
	Terminal_SaveLinesOnClearIsEnabled		(TerminalScreenRef			inScreen);

void
	Terminal_SetBellEnabled					(TerminalScreenRef			inScreen,
											 Boolean					inIsEnabled);

void
	Terminal_SetLineWrapEnabled				(TerminalScreenRef			inScreen,
											 Boolean					inIsEnabled);

void
	Terminal_SetSaveLinesOnClear			(TerminalScreenRef			inScreen,
											 Boolean					inClearScreenSavesLines);

Boolean
	Terminal_WindowIsToBeMinimized			(TerminalScreenRef			inScreen);

//@}

//!\name Direct Interaction With the Emulator (Deprecated)
//@{

Terminal_Result
	Terminal_EmulatorProcessCFString		(TerminalScreenRef			inScreen,
											 CFStringRef				inCString);

Terminal_Result
	Terminal_EmulatorProcessCString			(TerminalScreenRef			inScreen,
											 char const*				inCString);

Terminal_Result
	Terminal_EmulatorProcessData			(TerminalScreenRef			inScreen,
											 UInt8 const*				inBuffer,
											 UInt32						inLength);

//@}

//!\name File Capture Handling (Note: May Move)
//@{

Boolean
	Terminal_FileCaptureBegin				(TerminalScreenRef			inScreen,
											 SInt16						inOpenWritableFile);

void
	Terminal_FileCaptureEnd					(TerminalScreenRef			inScreen);

SInt16
	Terminal_FileCaptureGetReferenceNumber	(TerminalScreenRef			inScreen);

Boolean
	Terminal_FileCaptureInProgress			(TerminalScreenRef			inScreen);

Boolean
	Terminal_FileCaptureSaveDialog			(FSSpec*					outFSSpecPtr);

void
	Terminal_FileCaptureWriteData			(TerminalScreenRef			inScreen,
											 UInt8 const*				inBuffer,
											 SInt32						inLength);

//@}

//!\name Print Handling (Note: May Move)
//@{

void
	Terminal_PrintingEnd					(TerminalScreenRef			inScreen);

Boolean
	Terminal_PrintingInProgress				(TerminalScreenRef			inScreen);

//@}

//!\name Sound and Speech (Note: May Move)
//@{

TerminalSpeaker_Ref
	Terminal_ReturnSpeaker					(TerminalScreenRef			inScreen);

void
	Terminal_SetSpeechEnabled				(TerminalScreenRef			inScreen,
											 Boolean					inIsEnabled);

void
	Terminal_SpeakBuffer					(TerminalScreenRef			inScreen,
											 void const*				inBuffer,
											 Size						inBufferSize);

Boolean
	Terminal_SpeechIsEnabled				(TerminalScreenRef			inScreen);

void
	Terminal_SpeechPause					(TerminalScreenRef			inScreen);

void
	Terminal_SpeechResume					(TerminalScreenRef			inScreen);

//@}

//!\name Callbacks
//@{

void
	Terminal_StartMonitoring				(TerminalScreenRef			inScreen,
											 Terminal_Change			inForWhatChange,
											 ListenerModel_ListenerRef	inListener);

void
	Terminal_StopMonitoring					(TerminalScreenRef			inScreen,
											 Terminal_Change			inForWhatChange,
											 ListenerModel_ListenerRef	inListener);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
