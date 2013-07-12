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

// library includes
#import <BoundName.objc++.h>



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

The "propertiesByKey" may be used to associate any data you want
with a preference value.  (This is not allocated unless the
method "propertiesByKey" is called.)  A common use of this data
is an associated binding; for example, a text label near the
control that displays a preference value could be bound to a
string value in its dictionary that holds a human-readable and
localized description of the setting.
*/
@interface PreferenceValue_Inherited : NSObject //{
{
@private
	PrefsContextManager_Object*		prefsMgr;
	NSMutableDictionary*			propertiesByKey;
}

// initializers
	- (id)
	initWithContextManager:(PrefsContextManager_Object*)_;

// new methods
	- (void)
	didSetPreferenceValue;
	- (void)
	willSetPreferenceValue;

// accessors
	- (void)
	setInherited:(BOOL)_; // binding (to subclasses, typically)
	- (BOOL)
	isInheritEnabled; // binding (to subclasses, typically)
	- (PrefsContextManager_Object*)
	prefsMgr;
	- (NSMutableDictionary*)
	propertiesByKey;

// overrides for subclasses (none of these is implemented in the base!)
	- (BOOL)
	isInherited; // subclasses MUST implement; binding (to subclasses, typically)
	- (void)
	setNilPreferenceValue; // subclasses MUST implement

@end //}


/*!
Since the vast majority of bindings are to a single underlying
preference tag, this variant of the object is available to make
it easy to store and retrieve one tag value.
*/
@interface PreferenceValue_InheritedSingleTag : PreferenceValue_Inherited //{
{
@private
	Preferences_Tag		preferencesTag;
}

// initializers
	- (id)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_;

// accessors
	- (Preferences_Tag)
	preferencesTag;

@end //}


/*!
Manages bindings for a single color preference.
*/
@interface PreferenceValue_Color : PreferenceValue_InheritedSingleTag //{

// initializers
	- (id)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_;

// accessors
	- (NSColor*)
	colorValue;
	- (void)
	setColorValue:(NSColor*)_; // binding

@end //}


/*!
Manages bindings for any preference whose value is
defined to be Boolean.
*/
@interface PreferenceValue_Flag : PreferenceValue_InheritedSingleTag //{
{
	BOOL	inverted;
}

// initializers
	- (id)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_;
	- (id)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	inverted:(BOOL)_; // designated initializer

// new methods
	- (BOOL)
	readValueSeeIfDefault:(BOOL*)_;

// accessors
	- (NSNumber*)
	numberValue;
	- (void)
	setNumberValue:(NSNumber*)_; // binding

@end //}


/*!
Manages bindings for any preference whose value is
defined to be a number (optionally integer-only and/or
unsigned-only).
*/
@interface PreferenceValue_Number : PreferenceValue_InheritedSingleTag //{
{
	PreferenceValue_CType	valueCType;
}

// initializers
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

@end //}


/*!
Manages bindings for any preference whose value is
defined to be a pointer to a CFStringRef.
*/
@interface PreferenceValue_String : PreferenceValue_InheritedSingleTag //{

// initializers
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

@end //}


/*!
For use with PreferenceValue_Array.  Stores a description
for a constant value, and the value itself.  When an
array is bound to a user interface element such as a
pop-up menu or a matrix, the (localized) description for
the specified value is used to represent the value.
*/
@interface PreferenceValue_IntegerDescriptor : BoundName_Object //{
{
@private
	UInt32		describedValue;
}

// initializers
	- (id)
	initWithIntegerValue:(UInt32)_
	description:(NSString*)_;

// accessors
	- (UInt32)
	describedIntegerValue;
	- (void)
	setDescribedIntegerValue:(UInt32)_;

@end //}


/*!
Manages bindings for a single preference that has a
fixed array of possible values (with descriptions).
This is very commonly bound to a pop-up menu or a
matrix element.

The descriptor array should contain objects of a
type such as PreferenceValue_IntegerDescriptor, to
specify which values are stored and how they are
displayed to the user.
*/
@interface PreferenceValue_Array : PreferenceValue_Inherited //{
{
@private
	NSArray*					valueDescriptorArray;
	PreferenceValue_Number*		preferenceAccessObject;
}

// initializers
	- (id)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	preferenceCType:(PreferenceValue_CType)_
	valueDescriptorArray:(NSArray*)_;

// accessors
	- (NSArray*)
	valueDescriptorArray; // binding
	- (id)
	currentValueDescriptor;
	- (void)
	setCurrentValueDescriptor:(id)_; // binding

@end //}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
