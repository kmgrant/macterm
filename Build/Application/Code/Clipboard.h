/*!	\file Clipboard.h
	\brief The clipboard window, and Copy and Paste management.
	
	Based on the implementation of Apple’s SimpleText clipboard.
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

#ifndef __CLIPBOARD__
#define __CLIPBOARD__

// Mac includes
#if !REZ
#   include <Carbon/Carbon.h>
#   include <CoreServices/CoreServices.h>
#endif

// MacTelnet includes
#include "SessionRef.typedef.h"
#include "TerminalViewRef.typedef.h"



#pragma mark Constants

#if !REZ
typedef UInt16 Clipboard_CopyMethod; // do not redefine
#endif
enum
{
	kClipboard_CopyMethodStandard		= 0,		// use this value, or...
	kClipboard_CopyMethodTable			= 1 << 0,	// ...a bitwise OR of one or more of these mask values
	kClipboard_CopyMethodInline			= 1 << 1
};

#if !REZ
typedef UInt16 Clipboard_PasteMethod; // do not redefine
#endif
enum
{
	kClipboard_PasteMethodStandard		= 0,
	kClipboard_PasteMethodBlock			= 1
};



#if !REZ

#pragma mark Public Methods

void
	Clipboard_Init						();

void
	Clipboard_Done						();

OSStatus
	Clipboard_AEDescToScrap				(AEDesc const*			inDescPtr);

OSStatus
	Clipboard_CreateContentsAEDesc		(AEDesc*				outDescPtr);

void
	Clipboard_GraphicsToScrap			(short					inDrawingNumber);

PasteboardRef
	Clipboard_ReturnPrimaryPasteboard	();

WindowRef
	Clipboard_ReturnWindow				();

void
	Clipboard_SetWindowVisible			(Boolean				inIsVisible);

void
	Clipboard_TextFromScrap				(SessionRef				inSession);

void
	Clipboard_TextToScrap				(TerminalViewRef		inView,
										 Clipboard_CopyMethod   inHowToCopy);

void
	Clipboard_TextType					(TerminalViewRef		inSource,
										 SessionRef				inTarget);

Boolean
	Clipboard_WindowIsVisible			();

#endif /* REZ */

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
