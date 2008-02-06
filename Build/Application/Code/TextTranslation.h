/*!	\file TextTranslation.h
	\brief Lists methods for character translation (i.e. between
	what the user sees and what is transmitted on the network).
	
	Written by Roland Månsson (Lund University Computing Center,
	Sweden, roland_m@ldc.lu.se).  Modified by Pascal Maes (UCL/
	ELEC, Place du Levant, 3, B-1348 Louvain-la-Neuve).  Updated
	for Text Encoding Converter support, and new API created by
	Kevin Grant.
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

#ifndef __TEXTTRANSLATION__
#define __TEXTTRANSLATION__

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

//!\name Initialization
//@{

void
	TextTranslation_Init						();

void
	TextTranslation_Done						();

//@}

//!\name Translating Characters
//@{

OSStatus
	TextTranslation_ConvertBufferToNewHandle	(UInt8*					inText,
												 ByteCount				inLength,
												 TECObjectRef			inConverter,
												 Handle*				outNewHandleWithTranslatedText);

//@}

//!\name Utilities
//@{

void
	TextTranslation_AppendCharacterSetsToMenu	(MenuRef				inToWhichMenu,
												 UInt16					inIndentationLevel);

// GUARANTEED TO MATCH NUMBER OF ITEMS APPENDED WITH TextTranslation_AppendCharacterSetsToMenu(), ABOVE
UInt16
	TextTranslation_ReturnCharacterSetCount		();

// GUARANTEED TO MATCH ORDER OF ITEM APPENDING FOR TextTranslation_AppendCharacterSetsToMenu(), ABOVE
UInt16
	TextTranslation_ReturnCharacterSetIndex		(CFStringEncoding		inTextEncoding);

// GUARANTEED TO MATCH ORDER OF ITEM APPENDING FOR TextTranslation_AppendCharacterSetsToMenu(), ABOVE
CFStringEncoding
	TextTranslation_ReturnIndexedCharacterSet	(UInt16					inOneBasedIndex);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
