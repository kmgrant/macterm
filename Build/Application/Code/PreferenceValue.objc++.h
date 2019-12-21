/*!	\file PreferenceValue.h
	\brief Presentation of preference values in user interfaces.
*/
/*###############################################################

	MacTerm
		© 1998-2019 by Kevin Grant.
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

#pragma once

// Mac includes
#import <Cocoa/Cocoa.h>

// application includes
#import "Preferences.h"
#import "PrefsContextManager.objc++.h"
#import "QuillsPrefs.h"

// library includes
#import <BoundName.objc++.h>
@class ListenerModel_StandardListener;



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
	kPreferenceValue_CTypeFloat32 = 4,	//!< preference requires Float32 variable (float)
	kPreferenceValue_CTypeFloat64 = 5	//!< preference requires Float64 variable (double)
};

/*!
Specifies the different interpretations of a series of
coordinates that define a rectangle.
*/
enum PreferenceValue_RectType
{
	kPreferenceValue_RectTypeHIRect = 1	//!< preference requires CGRect variable with top-left origin
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
	- (instancetype)
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
	Preferences_Tag		_preferencesTag;
}

// initializers
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_;

// accessors
	@property (assign) Preferences_Tag
	preferencesTag;

@end //}


/*!
Manages bindings for a single color preference.
*/
@interface PreferenceValue_Color : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype)
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
defined to be a pointer to a CFURLRef.  The value is
exposed to user interfaces only as a string.
*/
@interface PreferenceValue_FileSystemObject : PreferenceValue_InheritedSingleTag //{
{
	BOOL	isDirectory;
	BOOL	isURLInfoObject;
}

// initializers
	- (instancetype)
	initWithURLPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	isDirectory:(BOOL)_;
	- (instancetype)
	initWithURLInfoPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	isDirectory:(BOOL)_;

// new methods
	- (NSString*)
	readValueSeeIfDefault:(BOOL*)_;

// accessors
	- (NSString*)
	stringValue;
	- (void)
	setStringValue:(NSString*)_; // binding
	- (NSURL*)
	URLValue;
	- (void)
	setURLValue:(NSURL*)_; // binding

@end //}


/*!
Manages bindings for any preference whose value is
defined to be Boolean.
*/
@interface PreferenceValue_Flag : PreferenceValue_InheritedSingleTag //{
{
	BOOL		inverted;
}

// initializers
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_;
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	inverted:(BOOL)_ NS_DESIGNATED_INITIALIZER;

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

If a number has a floating-point value, a scale exponent
may be set to use a different scale for the bound value
versus the stored value.  For example, you can use this
to store a value in units of seconds but display it as
milliseconds.
*/
@interface PreferenceValue_Number : PreferenceValue_InheritedSingleTag //{
{
	NSInteger				scaleExponent; // e.g. set to "-3" to scale by 10^-3 (or 1/1000)
	BOOL					scaleWithRounding; // scaled value displayed as nearest integer
	PreferenceValue_CType	valueCType;
}

// initializers
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	preferenceCType:(PreferenceValue_CType)_;

// new methods
	- (NSNumber*)
	readValueSeeIfDefault:(BOOL*)_;

// accessors: configuration
	- (NSInteger)
	scaleExponent;
	- (void)
	setScaleExponent:(NSInteger)_
	rounded:(BOOL)_;

// accessors: bindings
	- (NSNumber*)
	numberValue;
	- (void)
	setNumberValue:(NSNumber*)_; // binding
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
defined to be a floating-point rectangle.
*/
@interface PreferenceValue_Rect : PreferenceValue_InheritedSingleTag //{
{
	PreferenceValue_RectType	valueRectType;
}

// initializers
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	preferenceRectType:(PreferenceValue_RectType)_;

// new methods
	- (NSArray*)
	readValueSeeIfDefault:(BOOL*)_;

// accessors: bindings
	- (NSArray*)
	numberArrayValue;
	- (void)
	setNumberArrayValue:(NSArray*)_; // binding

// validators
	- (BOOL)
	validateNumberArrayValue:(id*)_
	error:(NSError**)_;

@end //}


/*!
Manages bindings for any preference whose value is
defined to be a pointer to a CFStringRef.
*/
@interface PreferenceValue_String : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype)
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
Manages bindings for any preference whose value is
defined to be a pointer to a CFArrayRef of strings,
using a single string as the access point.  The
string is split on a character set in order to
convert.
*/
@interface PreferenceValue_StringByJoiningArray : PreferenceValue_InheritedSingleTag //{
{
@private
	NSCharacterSet*		_characterSetForSplitting;
	NSString*			_stringForJoiningElements;
}

// initializers
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	characterSetForSplitting:(NSCharacterSet*)_
	stringForJoiningElements:(NSString*)_;

// new methods
	- (NSString*)
	readValueSeeIfDefault:(BOOL*)_;

// accessors
	@property (strong) NSCharacterSet*
	characterSetForSplitting;
	@property (strong) NSString*
	stringForJoiningElements;
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
	- (instancetype)
	initWithIntegerValue:(UInt32)_
	description:(NSString*)_;

// accessors
	- (UInt32)
	describedIntegerValue;
	- (void)
	setDescribedIntegerValue:(UInt32)_;

@end //}


/*!
For use with PreferenceValue_Array.  Stores a description for
a string, and the string itself.  When an array is bound to a
user interface element such as a pop-up menu or a matrix, the
(localized) description for the specified value is used to
represent the value.

While in many cases a string’s descriptor may be the string
itself, this provides important flexibility for the cases
where it may not (such as, to map a Default value).
*/
@interface PreferenceValue_StringDescriptor : BoundName_Object //{
{
@private
	NSString*	describedValue;
}

// initializers
	- (instancetype)
	initWithStringValue:(NSString*)_
	description:(NSString*)_;

// accessors
	- (NSString*)
	describedStringValue;
	- (void)
	setDescribedStringValue:(NSString*)_;

@end //}


/*!
Manages bindings for a single preference that has a
fixed array of possible values (with descriptions).
This is very commonly bound to a pop-up menu or a
matrix element.

If the current state forms a set of multiple values,
the "currentMultiValueDescriptors" binding can be
used instead of "currentValueDescriptor".  For an
integer-based set of descriptors, the assumption is
that all integer values can be treated as bits and
combined using bitwise-OR or removed by using a
bitwise-AND with a negation.

The descriptor array should contain objects of a
type such as PreferenceValue_IntegerDescriptor, to
specify which values are stored and how they are
displayed to the user.
*/
@interface PreferenceValue_Array : PreferenceValue_InheritedSingleTag //{
{
@private
	NSArray*					_valueDescriptorArray;
	id							_placeholderDescriptor;
	PreferenceValue_Number*		_preferenceAccessObject;
}

// initializers
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	preferenceCType:(PreferenceValue_CType)_
	valueDescriptorArray:(NSArray*)_;

// accessors
	- (NSArray*)
	valueDescriptorArray; // binding
	- (NSArray*)
	currentMultiValueDescriptors;
	- (void)
	setCurrentMultiValueDescriptors:(NSArray*)_; // binding
	- (id)
	currentValueDescriptor;
	- (void)
	setCurrentValueDescriptor:(id)_; // binding
	@property (strong) id
	placeholderDescriptor; // value of "currentValueDescriptor" when nothing matches

@end //}


/*!
Manages bindings for a single preference that has a
string value that comes from the list of available
collections in a certain preferences class.  This is
typically bound to a pop-up menu.
*/
@interface PreferenceValue_CollectionBinding : PreferenceValue_InheritedSingleTag //{
{
@private
	NSArray*							_valueDescriptorArray;
	id									_targetForDidRebuildArray;
	SEL									_selectorForDidRebuildArray;
	BOOL								_includeDefaultFlag;
	PreferenceValue_String*				_preferenceAccessObject;
	ListenerModel_StandardListener*		_preferenceChangeListener;
	Quills::Prefs::Class				_preferencesClass;
}

// initializers
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	sourceClass:(Quills::Prefs::Class)_
	includeDefault:(BOOL)_
	didRebuildTarget:(id)_
	didRebuildSelector:(SEL)_; // designated initializer
	- (instancetype)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object*)_
	sourceClass:(Quills::Prefs::Class)_
	includeDefault:(BOOL)_;

// new methods
	- (NSString*)
	readValueSeeIfDefault:(BOOL*)_;

// accessors
	@property (strong) PreferenceValue_StringDescriptor*
	currentValueDescriptor; // binding
	@property (strong, readonly) NSArray*
	valueDescriptorArray; // binding

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
