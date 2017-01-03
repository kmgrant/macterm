/*!	\file CocoaBasic.h
	\brief Fundamental Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2017 by Kevin Grant
	
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

#ifndef __COCOABASIC__
#define __COCOABASIC__

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#ifdef __OBJC__
@class NSResponder;
@class NSWindow;
#else
class NSResponder;
class NSWindow;
#endif

// compile-time options
#ifndef COCOA_BASIC_SUPPORTS_CARBON
#define COCOA_BASIC_SUPPORTS_CARBON 0
#endif



#pragma mark Public Methods

//!\name General
//@{

Boolean
	CocoaBasic_CreateFileAndDirectoriesWithData		(CFURLRef,
													 CFStringRef,
													 CFDataRef = nullptr);

CGDeviceColor
	CocoaBasic_GetGray								(CGDeviceColor const&,
													 Float32 = 0.5);

CGDeviceColor
	CocoaBasic_GetGray								(CGDeviceColor const&,
													 CGDeviceColor const&,
													 Float32 = 0.5);

void
	CocoaBasic_PlaySoundByName						(CFStringRef);

void
	CocoaBasic_PlaySoundFile						(CFURLRef);

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
	CocoaBasic_FileOpenPanelDisplay					(void (^inOpenURLHandler)(CFURLRef),
													 CFStringRef = nullptr,
													 CFStringRef = nullptr,
													 CFArrayRef = nullptr);

//@}

//!\name Window Management
//@{

void
	CocoaBasic_InvalidateAppRestorableState			();

// WORKS WITH NSApp AND ANY NSWindow, FOR EXAMPLE
void
	CocoaBasic_InvalidateRestorableState			(NSResponder*);

#if COCOA_BASIC_SUPPORTS_CARBON
void
	CocoaBasic_MakeFrontWindowCarbonUserFocusWindow	();
#endif

#if COCOA_BASIC_SUPPORTS_CARBON
void
	CocoaBasic_MakeKeyWindowCarbonUserFocusWindow	();
#endif

#if COCOA_BASIC_SUPPORTS_CARBON
Boolean
	CocoaBasic_RegisterCocoaCarbonWindow			(NSWindow*);
#endif

#if COCOA_BASIC_SUPPORTS_CARBON
NSWindow*
	CocoaBasic_ReturnNewOrExistingCocoaCarbonWindow	(HIWindowRef);
#endif

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
