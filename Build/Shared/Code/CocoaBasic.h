/*!	\file CocoaBasic.h
	\brief Fundamental Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.1
	© 2008-2009 by Kevin Grant
	
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



#pragma mark Public Methods

void
	CocoaBasic_AboutPanelDisplay					();

Boolean
	CocoaBasic_ApplicationLoad						();

void
	CocoaBasic_ColorPanelDisplay					();

void
	CocoaBasic_ColorPanelSetTargetView				(HIViewRef);

Boolean
	CocoaBasic_FileOpenPanelDisplay					(CFStringRef = nullptr,
													 CFStringRef = nullptr,
													 CFArrayRef = nullptr);

Boolean
	CocoaBasic_GrowlIsAvailable						();

void
	CocoaBasic_GrowlNotify							(CFStringRef,
													 CFStringRef = nullptr,
													 CFStringRef = nullptr);

void
	CocoaBasic_MakeFrontWindowCarbonUserFocusWindow	();

void
	CocoaBasic_MakeKeyWindowCarbonUserFocusWindow	();

void
	CocoaBasic_PlaySoundByName						(CFStringRef);

void
	CocoaBasic_PlaySoundFile						(CFURLRef);

CFStringRef
	CocoaBasic_ReturnStringEncodingLocalizedName	(CFStringEncoding);

CFArrayRef
	CocoaBasic_ReturnUserSoundNames					();

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
