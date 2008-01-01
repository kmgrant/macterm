/*!	\file WindowInfo.h
	\brief Attaches auxiliary data to Mac OS windows to make
	them easier to deal with generically.
	
	NOTE:   This module is deprecated on Mac OS X.  It was made
			originally to address serious deficiencies in the
			Window Manager and other toolbox APIs, but any
			Mac OS X application can use other methods to do
			the things Window Info does.
	
	This new, extremely powerful means of associating
	information with windows makes it possible to tag windows
	with a wide array of feature information and access it
	using a simple, Mac OS window pointer (WindowRef) or dialog
	pointer (DialogRef).  Use of attached procedure pointers
	allows standardization of the means of performing operations
	on all your windows, while each individual window can still
	be handled in a special way.  This class has very little
	deduction capability: that is, you generally can’t call a
	"get" function until you call the corresponding "set"
	function at least once.  It is primarily a storage facility
	that allows you to associate a tremendous wealth of
	information with every window in your application, and,
	through this, allows you to greatly simplify regular tasks
	such as resizing windows.
*/
/*###############################################################

	Interface Library 1.1
	© 1998-2006 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "UniversalDefines.h"

#ifndef __WINDOWINFO__
#define __WINDOWINFO__

#ifndef REZ

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Constants

typedef FourCharCode WindowInfoDescriptor;
enum
{
	kInvalidWindowInfoDescriptor = FOUR_CHAR_CODE('----')
};

typedef SInt32 WindowInfoMessage;
enum
{
	// messages applicable to a WindowContextualMenuProcPtr
	kWindowInfoMessageContextualMenuSetup = 1,		// request to set up a contextual menu’s items, and perform other
													//   appropriate actions (e.g. highlight subject of contextual menu event)
	kWindowInfoMessageContextualMenuCleanup = 2		// contextual menu is now gone; perform any necessary cleanup actions
													//   (e.g. remove highlighting from object of contextual menu click)
};

#pragma mark Types

typedef struct OpaqueWindowInfo**		WindowInfoRef;

#pragma mark Callbacks

/*!
Window Contextual Menu Click Notification Method

When a contextual menu event is detected for a window, first
create a “bare bones” menu (containing any standard items
you need), then invoke this notifier on the window affected
(so appropriate items can be added if necessary), and finally
display the menu to the user OUTSIDE of this callback routine.

Typically, this notifier examines the mouse location and uses
the Contextual Menu module to add items or groups of items to
the specified menu, as appropriate (for example, the user
might have right-clicked on an icon or a control).  It is also
appropriate for this notifier to highlight the most specific
item in the window that the contextual menu will apply to,
prior to returning the menu that should be displayed.
*/
typedef pascal void (*WindowContextualMenuProcPtr)		(WindowRef			inWindow,
														 Point				inGlobalMouse,
														 MenuRef			inoutMenu,
														 WindowInfoMessage	inCallbackMessage);
#define InvokeWindowContextualMenuProc(userRoutine, inWindow, inGlobalMouse, inoutMenu, inCallbackMessage) \
	(*userRoutine)((inWindow), (inGlobalMouse), (inoutMenu), (inCallbackMessage))

/*!
Window Resize Notification Method

Whenever SetWindowBounds() is used to change the size of a window,
WindowInfo_InvokeWindowResizeResponder() should be invoked on the
general Window Info reference for that window.  The function is
notified of the change in size of the window, and should respond
accordingly (usually by moving and/or resizing controls).
*/
typedef pascal void (*WindowResizeResponderProcPtr)		(WindowRef			inWindow,
														 SInt32				inDeltaSizeX,
														 SInt32				inDeltaSizeY,
														 SInt32				inData);
#define InvokeWindowResizeResponderProc(userRoutine, inWindow, inDeltaSizeX, inDeltaSizeY, inData) \
	(*userRoutine)((inWindow), (inDeltaSizeX), (inDeltaSizeY), (inData))



#pragma mark Public Methods

/*###############################################################
	CREATING, INITIALIZING AND DESTROYING WINDOW INFO DATA
###############################################################*/

WindowInfoRef
	WindowInfo_New								();

void
	WindowInfo_Dispose							(WindowInfoRef					inoutWindowFeaturesRef);

void
	WindowInfo_Init								(WindowInfoRef					inoutWindowFeaturesRef);

/*###############################################################
	POLYMORPHIC OPERATIONS USING GENERIC WINDOW INFO
###############################################################*/

SInt16
	WindowInfo_GrowWindow						(WindowRef						inWindow,
												 EventRecord*					inoutEventPtr);

/*###############################################################
	INVOKING CALLBACKS WITHOUT KNOWING THEIR ADDRESS
###############################################################*/

void
	WindowInfo_NotifyWindowOfContextualMenu		(WindowRef						inWindow,
												 Point							inGlobalMouse);

void
	WindowInfo_NotifyWindowOfResize				(WindowRef						inWindow,
												 SInt32							inDeltaSizeX, 
												 SInt32							inDeltaSizeY);

/*###############################################################
	ASSOCIATING WINDOW INFO WITH WINDOWS
###############################################################*/

WindowInfoRef
	WindowInfo_ReturnFromDialog					(DialogRef						inDialog);

WindowInfoRef
	WindowInfo_ReturnFromWindow					(WindowRef						inWindow);

void
	WindowInfo_SetForDialog						(DialogRef						inDialog,
												 WindowInfoRef					inWindowFeaturesRef);

void
	WindowInfo_SetForWindow						(WindowRef						inWindow,
												 WindowInfoRef					inWindowFeaturesRef);

/*###############################################################
	ACCESSING DATA IN WINDOW INFO STRUCTURES
###############################################################*/

void
	WindowInfo_GetWindowMaximumDimensions		(WindowInfoRef					inWindowFeaturesRef,
												 Point*							outMaximumSizePtr);

void
	WindowInfo_GetWindowMinimumDimensions		(WindowInfoRef					inWindowFeaturesRef,
												 Point*							outMinimumSizePtr);

Boolean
	WindowInfo_IsPotentialDropTarget			(WindowInfoRef					inWindowFeaturesRef);

Boolean
	WindowInfo_IsWindowFloating					(WindowInfoRef					inWindowFeaturesRef);

void*
	WindowInfo_ReturnAuxiliaryDataPtr			(WindowInfoRef					inWindowFeaturesRef);

WindowInfoDescriptor
	WindowInfo_ReturnWindowDescriptor			(WindowInfoRef					inWindowFeaturesRef);

Rect*
	WindowInfo_ReturnWindowResizeLimits			(WindowInfoRef					inWindowFeaturesRef);

void
	WindowInfo_SetAuxiliaryDataPtr				(WindowInfoRef					inWindowFeaturesRef,
												 void*							inAuxiliaryDataPtr);

void
	WindowInfo_SetDynamicResizing				(WindowInfoRef					inWindowFeaturesRefOrNullToSetGlobalFlag,
												 Boolean						inLiveResizeEnabled);

void
	WindowInfo_SetWindowContextualMenuResponder	(WindowInfoRef					inWindowFeaturesRef,
												 WindowContextualMenuProcPtr	inNewProc);

void
	WindowInfo_SetWindowDescriptor				(WindowInfoRef					inWindowFeaturesRef,
												 WindowInfoDescriptor			inNewWindowDescriptor);

void
	WindowInfo_SetWindowFloating				(WindowInfoRef					inWindowFeaturesRef,
												 Boolean						inIsFloating);

void
	WindowInfo_SetWindowPotentialDropTarget		(WindowInfoRef					inWindowFeaturesRef,
												 Boolean						inIsPotentialDropTarget);

void
	WindowInfo_SetWindowResizeLimits			(WindowInfoRef					inWindowFeaturesRef,
												 SInt16							inMinimumHeight,
												 SInt16							inMinimumWidth,
												 SInt16							inMaximumHeight,
												 SInt16							inMaximumWidth);

void
	WindowInfo_SetWindowResizeResponder			(WindowInfoRef					inWindowFeaturesRef,
												 WindowResizeResponderProcPtr	inNewProc,
												 SInt32							inData);

#endif /* ifndef REZ */

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
