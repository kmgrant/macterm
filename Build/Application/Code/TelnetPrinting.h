/*!	\file TelnetPrinting.h
	\brief MacTelnet-specific printing functionality.
	
	Implemented with Universal Print, which aided in making the
	transition from Mac OS 8 and 9 to Mac OS X.
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#ifndef __TELNETPRINTING__
#define __TELNETPRINTING__

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <UniversalPrint.h>



#pragma mark Constants

/*!
Meta-characters allow special information to be encoded
into an otherwise-plain-text stream of data.  Spooling
a meta-character is equivalent to adding an instruction
for rendering whatever follows.

(3.1)
*/
typedef UInt8 TerminalPrintingMetaChar;
enum
{
	kTerminalPrintingMetaCharBeginBold		= 0x01,
	kTerminalPrintingMetaCharEndFormat		= 0x02
};

#pragma mark Types

/*!
All the information required to stream data from a
terminal to a printer.
*/
struct TerminalPrintingInfo
{
public:
	TerminalPrintingInfo	(SInt16		inMaximumColumnCount)
	: enabled(false), bufferTail(0x00000000), temporaryFileRefNum(-1), wrapColumnCount(inMaximumColumnCount)
	{
	}
	
	Boolean		enabled;					//!< is terminal text currently being redirected to a printer?
	long		bufferTail;					//!< trailing 4 characters of most recently scanned print buffer;
											//!  used to look for the terminate-printing signal
	char		temporaryFileName[256];		//!< used to store data to be printed
	short		temporaryFileRefNum;		//!< when the temporary file is open, this is its reference number
	SInt16		wrapColumnCount;			//!< maximum number of characters per line
};
typedef TerminalPrintingInfo*				TerminalPrintingInfoPtr;
typedef TerminalPrintingInfo const*			TerminalPrintingInfoConstPtr;



#pragma mark Public Methods

void
	TelnetPrinting_Begin					(TerminalPrintingInfoPtr	inPrintingInfoPtr);

void
	TelnetPrinting_End						(TerminalPrintingInfoPtr	inPrintingInfoPtr);

UniversalPrint_ContextRef
	TelnetPrinting_ReturnNewPrintRecord		();

void
	TelnetPrinting_PrintSelection			();

void
	TelnetPrinting_PageSetup				(SInt16						inResourceFileRefNum);

void
	printPages								(UniversalPrint_ContextRef	inPrintRecordRef,
											 ConstStringPtr				inTitle,
											 SInt16						inNumberOfColumns,
											 char**						inDataOrNull,
											 SInt16						inDataFileRefNumIfNullData);

void
	TelnetPrinting_Spool					(TerminalPrintingInfoPtr	inPrintingInfoPtr,
											 UInt8 const*				inBuffer,
											 SInt32*					pctr);

void
	TelnetPrinting_SpoolMetaCharacter		(TerminalPrintingInfoPtr	inPrintingInfoPtr,
											 TerminalPrintingMetaChar	inSpecialInstruction);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
