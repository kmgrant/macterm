/*!	\file Clipboard.h
	\brief The clipboard window, and Copy and Paste management.
	
	Based on the implementation of Apple’s SimpleText clipboard.
*/
/*###############################################################

	MacTerm
		© 1998-2014 by Kevin Grant.
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

#ifndef __CLIPBOARD__
#define __CLIPBOARD__

// Mac includes
#include <Carbon/Carbon.h>
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#endif
#include <CoreServices/CoreServices.h>

// application includes
#include "TerminalViewRef.typedef.h"



#pragma mark Constants

typedef UInt16 Clipboard_CopyMethod; // do not redefine
enum
{
	kClipboard_CopyMethodStandard		= 0,		// use this value, or...
	kClipboard_CopyMethodTable			= 1 << 0,	// ...a bitwise OR of one or more of these mask values
	kClipboard_CopyMethodInline			= 1 << 1
};

/*!
A data constraint allows you to take advantage of common
type filtering that the Clipboard module is already
familiar with.  This can avoid an often cumbersome series
of API calls and string comparisons necessary when dealing
with pasteboards.  It can also simplify tasks like iterating
over buffers, if you can ensure the array has elements of a
certain size.
*/
enum Clipboard_DataConstraint
{
	kClipboard_DataConstraintNone				= 0xFFFF,	//!< any type of data
	kClipboard_DataConstraintText8Bit			= 1 << 0,	//!< only text that can be expressed as bytes
	kClipboard_DataConstraintText16BitNative	= 1 << 1	//!< only Unicode text with native byte-ordering
};

#pragma mark Types

#ifdef __OBJC__

/*!
Implements the data-rendering part of the Clipboard.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Clipboard_ContentView : NSControl //{
{
@private
	BOOL	showDragHighlight;
}

// NSView
	- (void)
	drawRect:(NSRect)_;

// accessors
	- (BOOL)
	showDragHighlight;
	- (void)
	setShowDragHighlight:(BOOL)_;

@end //}


/*!
Implements the Clipboard window.  See "ClipboardCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface Clipboard_WindowController : NSWindowController //{
{
@public
	IBOutlet Clipboard_ContentView*		clipboardContent;
	IBOutlet NSView*					clipboardImageContainer;
	IBOutlet NSImageView*				clipboardImageContent;
	IBOutlet NSView*					clipboardTextContainer;
	IBOutlet NSTextView*				clipboardTextContent;
	IBOutlet NSTextField*				valueKind;
	IBOutlet NSTextField*				valueSize;
	IBOutlet NSTextField*				label1;
	IBOutlet NSTextField*				value1;
	IBOutlet NSTextField*				label2;
	IBOutlet NSTextField*				value2;
}

// class methods
	+ (NSFont*)
	returnNSFontForMonospacedTextOfSize:(unsigned int)_;
	+ (id)
	sharedClipboardWindowController;

// new methods
	- (void)
	setNeedsDisplay;

// accessors
	- (void)
	setDataSize:(size_t)_;
	- (void)
	setDataWidth:(size_t)_
	andHeight:(size_t)_;
	- (void)
	setKindField:(NSString*)_;
	- (void)
	setLabel1:(NSString*)_
	andValue:(NSString*)_;
	- (void)
	setLabel2:(NSString*)_
	andValue:(NSString*)_;
	- (void)
	setShowImage:(BOOL)_;
	- (void)
	setShowText:(BOOL)_;
	- (void)
	setSizeField:(NSString*)_;

@end //}

#endif // __OBJC__


#pragma mark Public Methods

//!\name Initialization
//@{

void
	Clipboard_Init			();

void
	Clipboard_Done			();

//@}

//!\name Accessing Clipboard Data
//@{

OSStatus
	Clipboard_AddCFStringToPasteboard		(CFStringRef				inStringToCopy,
											 PasteboardRef				inPasteboardOrNullForMainClipboard = nullptr);

OSStatus
	Clipboard_AEDescToScrap					(AEDesc const*				inDescPtr);

Boolean
	Clipboard_Contains						(CFStringRef				inUTI,
											 UInt16						inDesiredItemOrZeroForAll,
											 CFStringRef&				outConformingItemActualType,
											 PasteboardItemID&			outConformingItemID,
											 PasteboardRef				inDataSourceOrNull = nullptr);

Boolean
	Clipboard_ContainsGraphics				(PasteboardRef				inWhatChangedOrNullForPrimaryPasteboard = nullptr);

Boolean
	Clipboard_ContainsText					(PasteboardRef				inWhatChangedOrNullForPrimaryPasteboard = nullptr);

Boolean
	Clipboard_CreateCFStringFromPasteboard	(CFStringRef&				outCFString,
											 CFStringRef&				outUTI,
											 PasteboardRef				inPasteboardOrNull = nullptr);

Boolean
	Clipboard_CreateCGImageFromPasteboard	(CGImageRef&				outImage,
											 CFStringRef&				outUTI,
											 PasteboardRef				inPasteboardOrNull = nullptr);

OSStatus
	Clipboard_CreateContentsAEDesc			(AEDesc*					outDescPtr);

Boolean
	Clipboard_GetData						(Clipboard_DataConstraint	inConstraint,
											 CFDataRef&					outData,
											 CFStringRef&				outConformingItemActualType,
											 PasteboardItemID&			outConformingItemID,
											 PasteboardRef				inDataSourceOrNull = nullptr);

PasteboardRef
	Clipboard_ReturnPrimaryPasteboard		();

void
	Clipboard_SetPasteboardModified			(PasteboardRef				inWhatChangedOrNullForPrimaryPasteboard = nullptr);

void
	Clipboard_TextToScrap					(TerminalViewRef			inView,
											 Clipboard_CopyMethod		inHowToCopy,
											 PasteboardRef				inDataTargetOrNull = nullptr);

//@}

//!\name Clipboard User Interface
//@{

void
	Clipboard_SetWindowVisible			(Boolean			inIsVisible);

Boolean
	Clipboard_WindowIsVisible			();

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
