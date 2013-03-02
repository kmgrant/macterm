/*!	\file PreferenceValue.h
	\brief Presentation of preference values in user interfaces.
*/
/*###############################################################

	MacTerm
		© 1998-2013 by Kevin Grant.
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

#ifndef __PREFERENCEVALUE__
#define __PREFERENCEVALUE__

// Mac includes
#import <Cocoa/Cocoa.h>

// application includes
#import "Preferences.h"
#import "PrefsContextManager.objc++.h"



#pragma mark Constants

/*!
For legacy reasons, certain preferences have a variety of C
storage types instead of using things like Foundation objects.
*/
enum PreferenceValue_CType
{
	kPreferenceValue_CTypeSInt16 = 0,	//!< preference requires SInt16 variable
	kPreferenceValue_CTypeUInt16 = 1,	//!< preference requires UInt16 variable
	kPreferenceValue_CTypeSInt32 = 2,	//!< preference requires SInt32 variable
	kPreferenceValue_CTypeUInt32 = 3,	//!< preference requires UInt32 variable
	kPreferenceValue_CTypeFloat32 = 4	//!< preference requires Float32 variable
};

#pragma mark Types

/*!
Presents methods that greatly simplify bindings to values and the
various states of the user interface elements that control them.
This class must have subclasses in order to be useful.

Bind a checkbox’s value to the "inherited" path and its enabled
state to the "inheritEnabled" path to implement a box that resets
a preference to the parent context value (by deleting data).

Bind an appropriate editor control (e.g. a color box) to an
appropriate subclass-provided path for editing a particular kind
of value (e.g. in this case, "colorValue" is a likely path).
*/
@interface PreferenceValue_Inherited : NSObject
{
@private
	PrefsContextManager_Object*		prefsMgr;
	Preferences_Tag					preferencesTag;
}

// designated initializer
- (id)
initWithPreferencesTag:(Preferences_Tag)_
contextManager:(PrefsContextManager_Object*)_;

// accessors

- (void)
setInherited:(BOOL)_; // binding (to subclasses, typically)

- (BOOL)
isInheritEnabled; // binding (to subclasses, typically)

- (Preferences_Tag)
preferencesTag;

- (PrefsContextManager_Object*)
prefsMgr;

- (void)
didSetPreferenceValue;
- (void)
willSetPreferenceValue;

// overrides for subclasses (none of these is implemented in the base!)

- (BOOL)
isInherited; // subclasses MUST implement; binding (to subclasses, typically)

- (void)
setNilPreferenceValue; // subclasses MUST implement

@end


/*!
Manages bindings for a single color preference.
*/
@interface PreferenceValue_Color : PreferenceValue_Inherited
{
}

// designated initializer
- (id)
initWithPreferencesTag:(Preferences_Tag)_
contextManager:(PrefsContextManager_Object*)_;

// accessors

- (NSColor*)
colorValue;
- (void)
setColorValue:(NSColor*)_; // binding

@end


/*!
Manages bindings for any preference whose value is
defined to be Boolean.
*/
@interface PreferenceValue_Flag : PreferenceValue_Inherited
{
	BOOL	inverted;
}

- (id)
initWithPreferencesTag:(Preferences_Tag)_
contextManager:(PrefsContextManager_Object*)_;

// designated initializer
- (id)
initWithPreferencesTag:(Preferences_Tag)_
contextManager:(PrefsContextManager_Object*)_
inverted:(BOOL)_;

// new methods

- (BOOL)
readValueSeeIfDefault:(BOOL*)_;

// accessors

- (NSNumber*)
numberValue;
- (void)
setNumberValue:(NSNumber*)_; // binding

@end


/*!
Manages bindings for any preference whose value is
defined to be a number (optionally integer-only and/or
unsigned-only).
*/
@interface PreferenceValue_Number : PreferenceValue_Inherited
{
	PreferenceValue_CType	valueCType;
}

// designated initializer
- (id)
initWithPreferencesTag:(Preferences_Tag)_
contextManager:(PrefsContextManager_Object*)_
preferenceCType:(PreferenceValue_CType)_;

// new methods

- (NSNumber*)
readValueSeeIfDefault:(BOOL*)_;

// accessors

- (NSString*)
numberStringValue;
- (void)
setNumberStringValue:(NSString*)_; // binding

// validators

- (BOOL)
validateNumberStringValue:(id*)_
error:(NSError**)_;

@end


/*!
Manages bindings for any preference whose value is
defined to be a pointer to a CFStringRef.
*/
@interface PreferenceValue_String : PreferenceValue_Inherited
{
}

// designated initializer
- (id)
initWithPreferencesTag:(Preferences_Tag)_
contextManager:(PrefsContextManager_Object*)_;

// new methods

- (NSString*)
readValueSeeIfDefault:(BOOL*)_;

// accessors

- (NSString*)
stringValue;
- (void)
setStringValue:(NSString*)_; // binding

@end

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
