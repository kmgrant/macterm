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

	MacTerm
		© 1998-2012 by Kevin Grant.
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

#ifndef __TERMINALWINDOW__
#define __TERMINALWINDOW__

// Mac includes
#ifdef __OBJC__
@class NSWindow;
#else
class NSWindow;
#endif
#include <CoreServices/CoreServices.h>

// library includes
#include "ListenerModel.h"

// application includes
#include "Preferences.h"
#include "TerminalScreenRef.typedef.h"
#include "TerminalViewRef.typedef.h"



#pragma mark Constants

enum TerminalWindow_Result
{
	kTerminalWindow_ResultOK						= 0,	//!< no error occurred
	kTerminalWindow_ResultGenericFailure			= 1,	//!< unspecified problem
	kTerminalWindow_ResultInsufficientBufferSpace	= 2,	//!< not enough room in a provided array, for example
	kTerminalWindow_ResultInvalidReference			= 3		//!< TerminalWindowRef is not recognized
};

/*!
Setting changes that MacTerm allows other modules to “listen” for,
via TerminalWindow_StartMonitoring().
*/
enum TerminalWindow_Change
{
	kTerminalWindow_ChangeIconTitle			= 'NIcT',	//!< the title of a monitored Terminal Window’s
														//!  collapsed Dock tile has changed (context:
														//!  TerminalWindowRef)
	
	kTerminalWindow_ChangeObscuredState		= 'ShHd',	//!< a monitored Terminal Window has been
														//!  hidden or redisplayed (context:
														//!  TerminalWindowRef)
	
	kTerminalWindow_ChangeScreenDimensions	= 'Size',	//!< the screen dimensions of a monitored
														//!  Terminal Window have changed (context:
														//!  TerminalWindowRef)
	
	kTerminalWindow_ChangeWindowTitle		= 'NWnT'	//!< the title of a monitored Terminal Window
														//!  has changed (context: TerminalWindowRef)
};

/*!
Unique descriptors for collections of terminal views.  For example,
these might be used to describe the collection of all views in the
entire window, or only the currently focused view, etc.
*/
enum TerminalWindow_ViewGroup
{
	kTerminalWindow_ViewGroupEverything		= '****',	//!< contains EVERY view in the window
	kTerminalWindow_ViewGroupActive			= 'Frnt',	//!< contains all views in the visible tab
};

Float32 const	kTerminalWindow_DefaultMetaTabWidth = 0.0;	//!< tells TerminalWindow_SetTabWidth() to restore a standard width

#pragma mark Types

typedef struct OpaqueTerminalWindow*	TerminalWindowRef;

#ifdef __OBJC__

@class TerminalView_BackgroundView;
@class TerminalView_ContentView;

/*!
Implements the temporary Cocoa window that wraps the
Cocoa version of the Terminal View that is under
development.  See "TerminalWindowCocoa.xib".

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface TerminalWindow_Controller : NSWindowController
{
	IBOutlet TerminalView_ContentView*		testTerminalContentView;
	IBOutlet TerminalView_BackgroundView*	testTerminalPaddingView; // should embed the content view
	IBOutlet TerminalView_BackgroundView*	testTerminalBackgroundView; // should embed the padding view
}

+ (id)
sharedTerminalWindowController; // TEMPORARY

@end


@interface NSWindow (TerminalWindow_NSWindowExtensions)

- (TerminalWindowRef)
terminalWindowRef;

@end

#endif // __OBJC__



#pragma mark Public Methods

//!\name Creating and Destroying Terminal Windows
//@{

// DO NOT CREATE TERMINAL WINDOWS THIS WAY (USE SessionFactory METHODS, INSTEAD)
TerminalWindowRef
	TerminalWindow_New								(Preferences_ContextRef		inTerminalInfoOrNull = nullptr,
													 Preferences_ContextRef		inFontInfoOrNull = nullptr,
													 Preferences_ContextRef		inTranslationInfoOrNull = nullptr,
													 Boolean					inNoStagger = false);

void
	TerminalWindow_Dispose							(TerminalWindowRef*			inoutRefPtr);

//@}

//!\name Terminal Window Information
//@{

void
	TerminalWindow_CopyWindowTitle					(TerminalWindowRef			inRef,
													 CFStringRef&				outName);

Boolean
	TerminalWindow_EventInside						(TerminalWindowRef			inRef,
													 EventRef					inMouseEvent);

// TEMPORARY, FOR COCOA TRANSITION
void
	TerminalWindow_Focus							(TerminalWindowRef			inRef);

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

TerminalWindow_Result
	TerminalWindow_GetViewsInGroup					(TerminalWindowRef			inRef,
													 TerminalWindow_ViewGroup	inViewGroup,
													 UInt16						inArrayLength,
													 TerminalViewRef*			outViewArray,
													 UInt16*					outActualCountOrNull);

// TEMPORARY, FOR COCOA TRANSITION
Boolean
	TerminalWindow_IsFocused						(TerminalWindowRef			inRef);

Boolean
	TerminalWindow_IsObscured						(TerminalWindowRef			inRef);

Boolean
	TerminalWindow_ReconfigureViewsInGroup			(TerminalWindowRef			inRef,
													 TerminalWindow_ViewGroup	inViewGroup,
													 Preferences_ContextRef		inContext,
													 Quills::Prefs::Class		inPrefsClass);

NSWindow*
	TerminalWindow_ReturnNSWindow					(TerminalWindowRef			inRef);

UInt16
	TerminalWindow_ReturnScreenCount				(TerminalWindowRef			inRef);

TerminalScreenRef
	TerminalWindow_ReturnScreenWithFocus			(TerminalWindowRef			inRef);

// NOT FOR GENERAL USE; POSSIBLY A TEMPORARY ACCESSOR
HIWindowRef
	TerminalWindow_ReturnTabWindow					(TerminalWindowRef			inRef);

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

// TEMPORARY, FOR COCOA TRANSITION
void
	TerminalWindow_Select							(TerminalWindowRef			inRef,
													 Boolean					inFocus = true);

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

// TEMPORARY, FOR COCOA TRANSITION
void
	TerminalWindow_SetVisible						(TerminalWindowRef			inRef,
													 Boolean					inIsVisible);

//@}

//!\name Terminal Window Operations
//@{

void
	TerminalWindow_DisplayTextSearchDialog			(TerminalWindowRef			inRef);

TerminalWindow_Result
	TerminalWindow_GetTabWidth						(TerminalWindowRef			inRef,
													 Float32&					outWidthHeightInPixels);

TerminalWindow_Result
	TerminalWindow_GetTabWidthAvailable				(TerminalWindowRef			inRef,
													 Float32&					outMaxWidthHeightInPixels);

Boolean
	TerminalWindow_IsTab							(TerminalWindowRef			inRef);

OSStatus
	TerminalWindow_SetTabAppearance					(TerminalWindowRef			inRef,
													 Boolean					inIsTab);

TerminalWindow_Result
	TerminalWindow_SetTabPosition					(TerminalWindowRef			inRef,
													 Float32					inOffsetFromStartingPointInPixels,
													 Float32					inWidthInPixelsOrFltMax = FLT_MAX);

TerminalWindow_Result
	TerminalWindow_SetTabWidth						(TerminalWindowRef			inRef,
													 Float32					inWidthInPixels);

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
	TerminalWindow_ReturnFromMainWindow				();

TerminalWindowRef
	TerminalWindow_ReturnFromKeyWindow				();

TerminalWindowRef
	TerminalWindow_ReturnFromWindow					(WindowRef					inWindow);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
