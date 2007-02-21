/*!	\file TerminalWindow.h
	\brief Second-highest level of abstraction for local or
	remote shells.
	
	A terminal window is the entity which manages the Mac OS
	window, terminal views, scroll bars, toolbars and other
	elements that make up a terminal window.
	
	Where possible, use the APIs in Session.h to indirectly
	affect a terminal window as a result of a session operation.
	Similarly, look here before considering the use of even
	lower-level APIs from TerminalView.h, etc.
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

#ifndef __TERMINALWINDOW__
#define __TERMINALWINDOW__

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include "ListenerModel.h"

// MacTelnet includes
#include "TerminalScreenRef.typedef.h"
#include "TerminalViewRef.typedef.h"



#pragma mark Constants

enum TerminalWindow_ResultCode
{
	kTerminalWindow_ResultCodeSuccess					= 0,	//!< no error occurred
	kTerminalWindow_ResultCodeGenericFailure			= 1,	//!< unspecified problem
	kTerminalWindow_ResultCodeInsufficientBufferSpace	= 2		//!< not enough room in a provided array, for example
};

/*!
Setting changes that MacTelnet allows other modules to ÒlistenÓ for,
via TerminalWindow_StartMonitoring().
*/
enum TerminalWindow_Change
{
	kTerminalWindow_ChangeIconTitle			= FOUR_CHAR_CODE('NIcT'),	//!< the title of a monitored Terminal WindowÕs
																		//!  collapsed Dock tile has changed (context:
																		//!  TerminalWindowRef)
	
	kTerminalWindow_ChangeObscuredState		= FOUR_CHAR_CODE('ShHd'),	//!< a monitored Terminal Window has been
																		//!  hidden or redisplayed (context:
																		//!  TerminalWindowRef)
	
	kTerminalWindow_ChangeScreenDimensions	= FOUR_CHAR_CODE('Size'),	//!< the screen dimensions of a monitored
																		//!  Terminal Window have changed (context:
																		//!  TerminalWindowRef)
	
	kTerminalWindow_ChangeWindowTitle		= FOUR_CHAR_CODE('NWnT')	//!< the title of a monitored Terminal Window
																		//!  has changed (context: TerminalWindowRef)
};

typedef UInt32 TerminalWindow_Flags;
enum
{
	kTerminalWindow_FlagSaveLinesThatScrollOffTop = (1 << 0),	//!< save or trash scrolled screen rows?
	kTerminalWindow_FlagShowToolbar = (1 << 1),					//!< window header initially visible?
	kTerminalWindow_FlagTextWraps = (1 << 2)					//!< wrap mode initially enabled?
};

/*!
Unique descriptors for collections of terminal views.  For example,
these might be used to describe the collection of views for a single
tab, or the collection of all views in the entire window, etc.
*/
enum TerminalWindow_ViewGroup
{
	kTerminalWindow_ViewGroupEverything		= FOUR_CHAR_CODE('****'),	//!< contains EVERY view in the window
	kTerminalWindow_ViewGroupActive			= FOUR_CHAR_CODE('Frnt'),	//!< contains all views in the visible tab
	kTerminalWindow_ViewGroup1				= FOUR_CHAR_CODE('Tab1'),	//!< contains all views in the first tab
	kTerminalWindow_ViewGroup2				= FOUR_CHAR_CODE('Tab2'),	//!< contains all views in the second tab
	kTerminalWindow_ViewGroup3				= FOUR_CHAR_CODE('Tab3'),	//!< contains all views in the third tab
	kTerminalWindow_ViewGroup4				= FOUR_CHAR_CODE('Tab4'),	//!< contains all views in the fourth tab
	kTerminalWindow_ViewGroup5				= FOUR_CHAR_CODE('Tab5')	//!< contains all views in the fifth tab
};

#pragma mark Types

typedef struct OpaqueTerminalWindow*	TerminalWindowRef;



#pragma mark Public Methods

//!\name Creating and Destroying Terminal Windows
//@{

// DO NOT CREATE TERMINAL WINDOWS THIS WAY (USE SessionFactory METHODS, INSTEAD)
TerminalWindowRef
	TerminalWindow_New								(UInt16						inCountMaximumViewableColumns,
													 UInt16						inCountMaximumViewableRows,
													 UInt16						inCountScrollbackBufferRows,
													 TerminalWindow_Flags		inFlags);

void
	TerminalWindow_Dispose							(TerminalWindowRef*			inoutRefPtr);

//@}

//!\name Terminal Window Information
//@{

void
	TerminalWindow_GetFontAndSize					(TerminalWindowRef			inRef,
													 StringPtr					outFontFamilyNameOrNull,
													 UInt16*					outFontSizeOrNull);

void
	TerminalWindow_GetScreens						(TerminalWindowRef			inRef,
													 UInt16						inArrayLength,
													 TerminalScreenRef*			outScreenArray,
													 UInt16*					outActualCountOrNull);

void
	TerminalWindow_GetScreenDimensions				(TerminalWindowRef			inRef,
													 UInt16*					outColumnCountPtrOrNull,
													 UInt16*					outRowCountPtrOrNull);

// DEPRECATED
void
	TerminalWindow_GetViews							(TerminalWindowRef			inRef,
													 UInt16						inArrayLength,
													 TerminalViewRef*			outViewArray,
													 UInt16*					outActualCountOrNull);

TerminalWindow_ResultCode
	TerminalWindow_GetViewsInGroup					(TerminalWindowRef			inRef,
													 TerminalWindow_ViewGroup	inViewGroup,
													 UInt16						inArrayLength,
													 TerminalViewRef*			outViewArray,
													 UInt16*					outActualCountOrNull);

Boolean
	TerminalWindow_IsObscured						(TerminalWindowRef			inRef);

Boolean
	TerminalWindow_PtInView							(TerminalWindowRef			inRef,
													 Point						inGlobalPoint);

UInt16
	TerminalWindow_ReturnScreenCount				(TerminalWindowRef			inRef);

TerminalScreenRef
	TerminalWindow_ReturnScreenWithFocus			(TerminalWindowRef			inRef);

UInt16
	TerminalWindow_ReturnScrollBarHeight			();

UInt16
	TerminalWindow_ReturnScrollBarWidth				();

// EQUIVALENT TO TerminalWindow_ReturnViewCountInGroup(inRef, kTerminalWindow_ViewGroupEverything)
UInt16
	TerminalWindow_ReturnViewCount					(TerminalWindowRef			inRef);

UInt16
	TerminalWindow_ReturnViewCountInGroup			(TerminalWindowRef			inRef,
													 TerminalWindow_ViewGroup	inViewGroup);

// EQUIVALENT TO TerminalWindow_ReturnViewWithFocusInGroup(inRef, kTerminalWindow_ViewGroupEverything)
TerminalViewRef
	TerminalWindow_ReturnViewWithFocus				(TerminalWindowRef			inRef);

WindowRef
	TerminalWindow_ReturnWindow						(TerminalWindowRef			inRef);

void
	TerminalWindow_SetFontAndSize					(TerminalWindowRef			inRef,
													 ConstStringPtr				inFontFamilyNameOrNull,
													 UInt16						inFontSizeOrZero);

void
	TerminalWindow_SetObscured						(TerminalWindowRef			inRef,
													 Boolean					inIsHidden);

void
	TerminalWindow_SetScreenDimensions				(TerminalWindowRef			inRef,
													 UInt16						inNewColumnCount,
													 UInt16						inNewRowCount,
													 Boolean					inSendToRecordingScripts);

void
	TerminalWindow_SetIconTitle						(TerminalWindowRef			inRef,
													 CFStringRef				inName);

void
	TerminalWindow_SetWindowTitle					(TerminalWindowRef			inRef,
													 CFStringRef				inName);

//@}

//!\name Terminal Window Operations
//@{

void
	TerminalWindow_DisplayTextSearchDialog			(TerminalWindowRef			inRef);

// API UNDER EVALUATION
void
	TerminalWindow_StackWindows						();

void
	TerminalWindow_StartMonitoring					(TerminalWindowRef			inRef,
													 TerminalWindow_Change		inForWhatChange,
													 ListenerModel_ListenerRef	inListener);

void
	TerminalWindow_StopMonitoring					(TerminalWindowRef			inRef,
													 TerminalWindow_Change		inForWhatChange,
													 ListenerModel_ListenerRef	inListener);

//@}

//!\name Getting Information From Mac OS Windows
//@{

// IMPORTANT - USE THIS FUNCTION TO GUARANTEE A WINDOW IS A TERMINAL, BEFORE MUCKING WITH IT
Boolean
	TerminalWindow_ExistsFor						(WindowRef					inWindow);

TerminalWindowRef
	TerminalWindow_ReturnFromWindow					(WindowRef					inWindow);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
