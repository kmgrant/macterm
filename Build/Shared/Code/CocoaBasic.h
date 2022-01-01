/*!	\file CocoaBasic.h
	\brief Fundamental Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2022 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#ifdef __OBJC__
@class NSResponder;
@class NSWindow;
#else
class NSResponder;
class NSWindow;
#endif



#pragma mark Public Methods

//!\name General
//@{

Boolean
	CocoaBasic_CreateFileAndDirectoriesWithData		(CFURLRef,
													 CFStringRef,
													 CFDataRef = nullptr);

CGFloatRGBColor
	CocoaBasic_GetGray								(CGFloatRGBColor const&,
													 Float32 = 0.5);

CGFloatRGBColor
	CocoaBasic_GetGray								(CGFloatRGBColor const&,
													 CGFloatRGBColor const&,
													 Float32 = 0.5);

void
	CocoaBasic_PlaySoundByName						(CFStringRef);

void
	CocoaBasic_PlaySoundFile						(CFURLRef);

void
	CocoaBasic_PostUserNotification					(CFStringRef,
													 CFStringRef,
													 CFStringRef = nullptr,
													 CFStringRef = nullptr);

void
	CocoaBasic_PromptUserToAllowNotifications		();

CFStringRef
	CocoaBasic_ReturnStringEncodingLocalizedName	(CFStringEncoding);

CFArrayRef
	CocoaBasic_ReturnUserSoundNames					();

Boolean
	CocoaBasic_SetFileTypeCreator					(CFStringRef,
													 OSType,
													 OSType);

Boolean
	CocoaBasic_SpeakingInProgress					();

Boolean
	CocoaBasic_StartSpeakingString					(CFStringRef);

void
	CocoaBasic_StopSpeaking							();

//@}

//!\name Panels
//@{

void
	CocoaBasic_AboutPanelDisplay					();

Boolean
	CocoaBasic_FileOpenPanelDisplay					(CFStringRef,
													 CFArrayRef,
													 void (^inOpenURLHandler)(CFURLRef));

//@}

//!\name Window Management
//@{

void
	CocoaBasic_InvalidateAppRestorableState			();

// WORKS WITH NSApp AND ANY NSWindow, FOR EXAMPLE
void
	CocoaBasic_InvalidateRestorableState			(NSResponder*);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
