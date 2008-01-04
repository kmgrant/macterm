/*!	\file FormatDialog.h
	\brief Implements a dialog for editing font, size and color
	information for a terminal window.
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

#ifndef __FORMATDIALOG__
#define __FORMATDIALOG__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef UInt32 FormatDialog_FormatOptions;
enum
{
	// flags that affect one particular format, not all formats
	kFormatDialog_FormatOptionDisableFontItems		= (1 << 0),
	kFormatDialog_FormatOptionDisableFontSizeItems	= (1 << 1),
	kFormatDialog_FormatOptionDisableForeColorItems	= (1 << 2),
	kFormatDialog_FormatOptionDisableBackColorItems	= (1 << 3),
	kFormatDialog_FormatOptionMaskDisableAllItems	= 0xFFFFFFFF
};
enum
{
	kFormatDialog_NumberOfFormats	= 3,	// the number of format indices, below
	kFormatDialog_IndexNormalText	= 0,
	kFormatDialog_IndexBoldText		= 1,
	kFormatDialog_IndexBlinkingText	= 2
};

#pragma mark Types

typedef struct FormatDialog_OpaqueStruct*	FormatDialog_Ref;

struct FormatDialog_SetupData
{
	struct
	{
		FormatDialog_FormatOptions		options;	// see the valid enumerated flags above
		
		struct
		{
			Str255		familyName;
			UInt16		size;			// provide an acceptable size value (4 or more)
		} font;
		
		struct
		{
			RGBColor	foreground;
			RGBColor	background;
		} colors;
	} format[kFormatDialog_NumberOfFormats];
};
typedef FormatDialog_SetupData const*	FormatDialog_SetupDataConstPtr;
typedef FormatDialog_SetupData*			FormatDialog_SetupDataPtr;

#pragma mark Callbacks

/*!
Format Dialog Close Notification Method

When a Format dialog is closed, this method is
invoked.  Use this to know exactly when it is
safe to call FormatDialog_Dispose().
*/
typedef void	(*FormatDialog_CloseNotifyProcPtr)		(FormatDialog_Ref	inDialogThatClosed,
														 Boolean			inOKButtonPressed);
inline void
FormatDialog_InvokeCloseNotifyProc	(FormatDialog_CloseNotifyProcPtr	inUserRoutine,
									 FormatDialog_Ref					inDialogThatClosed,
									 Boolean							inOKButtonPressed)
{
	(*inUserRoutine)(inDialogThatClosed, inOKButtonPressed);
}



#pragma mark Public Methods

FormatDialog_Ref
	FormatDialog_New						(FormatDialog_SetupDataConstPtr		inSetupDataPtr,
											 FormatDialog_CloseNotifyProcPtr	inCloseNotifyProcPtr);

void
	FormatDialog_Dispose					(FormatDialog_Ref*					inoutDialogPtr);

void
	FormatDialog_Display					(FormatDialog_Ref					inDialog);

Boolean
	FormatDialog_GetContents				(FormatDialog_Ref					inDialog,
											 FormatDialog_SetupDataPtr			outSetupDataPtr);

HIWindowRef
	FormatDialog_ReturnParentWindow			(FormatDialog_Ref					inDialog);

void
	FormatDialog_StandardCloseNotifyProc	(FormatDialog_Ref					inDialogThatClosed,
											 Boolean							inOKButtonPressed);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
