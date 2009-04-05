/*!	\file PrintTerminal.h
	\brief The new Mac OS X native printing mechanism for
	terminal views in MacTelnet.
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

#ifndef __PRINTTERMINAL__
#define __PRINTTERMINAL__

// Mac includes
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "TerminalViewRef.typedef.h"



#pragma mark Constants

/*!
Meta-characters allow special information to be encoded
into an otherwise-plain-text stream of data.  Spooling
a meta-character is equivalent to adding an instruction
for rendering whatever follows.

(3.1)
*/
typedef UInt8 PrintTerminal_MetaChar;
enum
{
	kPrintTerminal_MetaCharBeginBold		= 0x01,		//!< Make font weight bold from now on.
	kPrintTerminal_MetaCharEndFormat		= 0x02		//!< Make font style plain from now on.
};

#pragma mark Types

typedef struct PrintTerminal_OpaqueJob*		PrintTerminal_CaptureRef;

typedef struct PrintTerminal_OpaqueJob*		PrintTerminal_JobRef;



#pragma mark Public Methods

//!\name Creating and Destroying Objects
//@{

PrintTerminal_CaptureRef
	PrintTerminal_NewCapture				();

PrintTerminal_JobRef
	PrintTerminal_NewJobFromCapture			(PrintTerminal_CaptureRef	inCapture);

PrintTerminal_JobRef
	PrintTerminal_NewJobFromSelectedText	(TerminalViewRef			inView);

PrintTerminal_JobRef
	PrintTerminal_NewJobFromVisibleScreen	(TerminalViewRef			inView);

void
	PrintTerminal_DisposeCapture			(PrintTerminal_CaptureRef*	inoutCapturePtr);

void
	PrintTerminal_DisposeJob				(PrintTerminal_JobRef*		inoutJobPtr);

//@}

//!\name Streaming Data
//@{

void
	PrintTerminal_CaptureData				(PrintTerminal_CaptureRef	inCapture,
											 UInt8 const*				inBuffer,
											 SInt32*					inoutCounterPtr);

void
	PrintTerminal_CaptureMetaCharacter		(PrintTerminal_CaptureRef	inCapture,
											 PrintTerminal_MetaChar		inSpecialInstruction);

//@}

//!\name Printing
//@{

void
	PrintTerminal_JobSendToPrinter			(PrintTerminal_JobRef		inJob,
											 Boolean					inWithUserInterface);

void
	PrintTerminal_JobSetNumberOfColumns		(PrintTerminal_JobRef		inJob,
											 UInt16						inNumberOfColumns);

void
	PrintTerminal_JobSetTitle				(PrintTerminal_JobRef		inJob,
											 CFStringRef				inTitle);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
