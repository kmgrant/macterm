/*!	\file FormatDialog.h
	\brief Implements a dialog for editing font, size and color
	information for a terminal window.
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

#ifndef __FORMATDIALOG__
#define __FORMATDIALOG__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef UInt32 FormatDialogFormatOptions;
enum
{
	// flags that affect one particular format, not all formats
	kFormatDialogFormatOptionDisableFontItems = (1 << 0),
	kFormatDialogFormatOptionDisableFontSizeItems = (1 << 1),
	kFormatDialogFormatOptionDisableForeColorItems = (1 << 2),
	kFormatDialogFormatOptionDisableBackColorItems = (1 << 3),
	kFormatDialogFormatOptionMaskDisableAllItems = 0xFFFFFFFF
};
enum
{
	kFormatDialogNumberOfFormats = 3,	// the number of format indices, below
	kFormatDialogIndexNormalText = 0,
	kFormatDialogIndexBoldText = 1,
	kFormatDialogIndexBlinkingText = 2
};

#pragma mark Types

typedef struct OpaqueFormatDialog**		FormatDialogRef;

struct FormatDialogSetupData
{
	struct
	{
		FormatDialogFormatOptions		options;	// see the valid enumerated flags above
		
		struct
		{
			Str255			familyName;
			UInt16			size;			// provide an acceptable size value (4 or more)
		} font;
		
		struct
		{
			RGBColor		foreground;
			RGBColor		background;
		} colors;
	} format[kFormatDialogNumberOfFormats];
};
typedef FormatDialogSetupData const*		FormatDialogSetupDataConstPtr;
typedef FormatDialogSetupData*				FormatDialogSetupDataPtr;

#pragma mark Callbacks

/*!
Format Dialog Close Notification Method

When a Format dialog is closed, this method is
invoked.  Use this to know exactly when it is
safe to call FormatDialog_Dispose().
*/
typedef void	 (*FormatDialogCloseNotifyProcPtr)	(FormatDialogRef		inDialogThatClosed,
													 Boolean				inOKButtonPressed);
inline void
InvokeFormatDialogCloseNotifyProc	(FormatDialogCloseNotifyProcPtr		inUserRoutine,
									 FormatDialogRef					inDialogThatClosed,
									 Boolean							inOKButtonPressed)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed);
}



#pragma mark Public Methods

FormatDialogRef
	FormatDialog_New						(FormatDialogSetupDataConstPtr	inSetupDataPtr,
											 FormatDialogCloseNotifyProcPtr	inCloseNotifyProcPtr);

void
	FormatDialog_Dispose					(FormatDialogRef*				inoutDialogPtr);

void
	FormatDialog_Display					(FormatDialogRef				inDialog);

Boolean
	FormatDialog_GetContents				(FormatDialogRef				inDialog,
											 FormatDialogSetupDataPtr		outSetupDataPtr);

WindowRef
	FormatDialog_GetParentWindow			(FormatDialogRef				inDialog);

void
	FormatDialog_StandardCloseNotifyProc	(FormatDialogRef				inDialogThatClosed,
											 Boolean						inOKButtonPressed);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
