/*!	\file PreferenceValue.h
	\brief Presentation of preference values in user interfaces.
*/
/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
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
@import Cocoa;

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

// initializers
	- (instancetype _Nullable)
	initWithContextManager:(PrefsContextManager_Object* _Nonnull)_;

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
	//! A helper to simplify access to the low-level Preferences API.
	//! It is particularly useful for reassigning contexts as the
	//! user selects from lists in the Preferences window.
	@property (strong, readonly, nonnull) PrefsContextManager_Object*
	prefsMgr;
	//! For generic use; see the class description.
	@property (strong, readonly, nonnull) NSMutableDictionary*
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

// initializers
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_;

// accessors
	//! The low-level preference tag whose value is represented
	//! by the subclass.
	@property (assign) Preferences_Tag
	preferencesTag;

@end //}


/*!
Manages bindings for a single color preference.
*/
@interface PreferenceValue_Color : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_;

// accessors
	- (NSColor* _Nullable)
	colorValue;
	//! The value to store under the associated preferences tag.
	//!
	//! As with other bindings, if nil is given then the value
	//! underneath is deleted.
	- (void)
	setColorValue:(NSColor* _Nullable)_; // binding

@end //}


/*!
Manages bindings for any preference whose value is
defined to be a pointer to a CFURLRef.  The value is
exposed to user interfaces only as a string.
*/
@interface PreferenceValue_FileSystemObject : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype _Nullable)
	initWithURLPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_
	isDirectory:(BOOL)_;
	- (instancetype _Nullable)
	initWithURLInfoPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_
	isDirectory:(BOOL)_;

// new methods
	- (NSString* _Nullable)
	readValueSeeIfDefault:(BOOL* _Nonnull)_;

// accessors
	- (NSString* _Nullable)
	stringValue;
	//! A string to convert into a URL and then store (as if
	//! "setURLValue:" had been used).
	//!
	//! As with other bindings, if nil is given then the value
	//! underneath is deleted.
	- (void)
	setStringValue:(NSString* _Nullable)_; // binding
	- (NSURL* _Nullable)
	URLValue;
	//! The value to store under the associated preferences tag.
	//!
	//! As with other bindings, if nil is given then the value
	//! underneath is deleted.
	- (void)
	setURLValue:(NSURL* _Nullable)_; // binding

@end //}


/*!
Manages bindings for any preference whose value is
defined to be Boolean.
*/
@interface PreferenceValue_Flag : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_;
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_
	inverted:(BOOL)_ NS_DESIGNATED_INITIALIZER;

// new methods
	- (BOOL)
	readValueSeeIfDefault:(BOOL* _Nonnull)_;

// accessors
	- (NSNumber* _Nullable)
	numberValue;
	//! The value to store under the associated preferences tag.
	//! Despite being a generic number, any nonzero value is
	//! considered a YES value and 0 is considered NO.
	//!
	//! As with other bindings, if nil is given then the value
	//! underneath is deleted.
	- (void)
	setNumberValue:(NSNumber* _Nullable)_; // binding

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

// initializers
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_
	preferenceCType:(PreferenceValue_CType)_;

// new methods
	- (NSNumber* _Nullable)
	readValueSeeIfDefault:(BOOL* _Nonnull)_;

// accessors: configuration
	- (NSInteger)
	scaleExponent;
	- (void)
	setScaleExponent:(NSInteger)_
	rounded:(BOOL)_;

// accessors: bindings
	- (NSNumber* _Nullable)
	numberValue;
	//! The value to store under the associated preferences tag.
	//!
	//! As with other bindings, if nil is given then the value
	//! underneath is deleted.
	- (void)
	setNumberValue:(NSNumber* _Nullable)_; // binding
	- (NSString* _Nullable)
	numberStringValue;
	//! A string to convert into a number and then store (as if
	//! "setNumberValue:" had been used).
	//!
	//! As with other bindings, if nil is given then the value
	//! underneath is deleted.
	- (void)
	setNumberStringValue:(NSString* _Nullable)_; // binding

// validators
	- (BOOL)
	validateNumberStringValue:(id _Nonnull* _Nonnull)_
	error:(NSError* _Nonnull* _Nonnull)_;

@end //}


/*!
Manages bindings for any preference whose value is
defined to be a floating-point rectangle.
*/
@interface PreferenceValue_Rect : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_
	preferenceRectType:(PreferenceValue_RectType)_;

// new methods
	- (NSArray* _Nullable)
	readValueSeeIfDefault:(BOOL* _Nonnull)_;

// accessors: bindings
	- (NSArray* _Nullable)
	numberArrayValue;
	//! The value to store under the associated preferences tag.
	//!
	//! As with other bindings, if nil is given then the value
	//! underneath is deleted.
	- (void)
	setNumberArrayValue:(NSArray* _Nullable)_; // binding

// validators
	- (BOOL)
	validateNumberArrayValue:(id _Nonnull* _Nonnull)_
	error:(NSError* _Nonnull* _Nonnull)_;

@end //}


/*!
Manages bindings for any preference whose value is
defined to be a pointer to a CFStringRef.
*/
@interface PreferenceValue_String : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_;

// new methods
	- (NSString* _Nullable)
	readValueSeeIfDefault:(BOOL* _Nonnull)_;

// accessors
	- (NSString* _Nullable)
	stringValue;
	//! The value to store under the associated preferences tag.
	//!
	//! As with other bindings, if nil is given then the value
	//! underneath is deleted.
	- (void)
	setStringValue:(NSString* _Nullable)_; // binding

@end //}


/*!
Manages bindings for any preference whose value is
defined to be a pointer to a CFArrayRef of strings,
using a single string as the access point.  The
string is split on a character set in order to
convert.
*/
@interface PreferenceValue_StringByJoiningArray : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_
	characterSetForSplitting:(NSCharacterSet* _Nonnull)_
	stringForJoiningElements:(NSString* _Nonnull)_;

// new methods
	- (NSString* _Nullable)
	readValueSeeIfDefault:(BOOL* _Nonnull)_;

// accessors
	//! When constructing the underlying array preference value
	//! from a string bound to the UI, this is the set of
	//! characters that indicate splitting points (e.g. the
	//! whitespace character set).
	@property (strong, nonnull) NSCharacterSet*
	characterSetForSplitting;
	//! When constructing a string out of an array of strings,
	//! this is the substring that should appear in between
	//! each value (e.g. a single space).
	@property (strong, nonnull) NSString*
	stringForJoiningElements;
	//! The UI representation of the joined array.  If this value
	//! is assigned a new string, the internal array is updated
	//! by splitting the string using "characterSetForSplitting".
	//! And when initialized from a preference array value, the
	//! initial string is constructed by joining the array values
	//! with "stringForJoiningElements".
	//!
	//! As with other bindings, if nil is given then the value
	//! underneath is deleted (in this case, the array).
	@property (strong, nullable) NSString*
	stringValue; // binding

@end //}


/*!
For use with PreferenceValue_Array.  Stores a description
for a constant value, and the value itself.  When an
array is bound to a user interface element such as a
pop-up menu or a matrix, the (localized) description for
the specified value is used to represent the value.
*/
@interface PreferenceValue_IntegerDescriptor : BoundName_Object //{

// initializers
	- (instancetype _Nullable)
	initWithIntegerValue:(UInt32)_
	description:(NSString* _Nonnull)_;

// accessors
	//! The preference value represented by the description.
	//! For example, if the localized description appears in
	//! a pop-up menu and is the selected item, this integer
	//! value might be stored as the preference setting.
	@property (assign) UInt32
	describedIntegerValue;

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

// initializers
	- (instancetype _Nullable)
	initWithStringValue:(NSString* _Nonnull)_
	description:(NSString* _Nonnull)_;

// accessors
	//! The preference value represented by the description.
	//! For example, a localized description of “Name” may
	//! correspond to some internal string like "name".
	@property (strong, nonnull) NSString*
	describedStringValue;

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

// initializers
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_
	preferenceCType:(PreferenceValue_CType)_
	valueDescriptorArray:(NSArray* _Nonnull)_;

// accessors
	@property (strong, readonly, nonnull) NSArray*
	valueDescriptorArray; // binding
	- (NSArray* _Nullable)
	currentMultiValueDescriptors;
	//! For selecting multiple values; see the class description.
	- (void)
	setCurrentMultiValueDescriptors:(NSArray* _Nullable)_; // binding
	- (id _Nullable)
	currentValueDescriptor;
	- (void)
	setCurrentValueDescriptor:(id _Nullable)_; // binding
	@property (strong, nullable) id
	placeholderDescriptor; // value of "currentValueDescriptor" when nothing matches

@end //}


/*!
Manages bindings for a single preference that has a
string value that comes from the list of available
collections in a certain preferences class.  This is
typically bound to a pop-up menu.
*/
@interface PreferenceValue_CollectionBinding : PreferenceValue_InheritedSingleTag //{

// initializers
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_
	sourceClass:(Quills::Prefs::Class)_
	includeDefault:(BOOL)_
	didRebuildTarget:(id _Nullable)_
	didRebuildSelector:(SEL _Nullable)_; // designated initializer
	- (instancetype _Nullable)
	initWithPreferencesTag:(Preferences_Tag)_
	contextManager:(PrefsContextManager_Object* _Nonnull)_
	sourceClass:(Quills::Prefs::Class)_
	includeDefault:(BOOL)_;

// new methods
	- (NSString* _Nullable)
	readValueSeeIfDefault:(BOOL* _Nonnull)_;

// accessors
	//! The description and preference name for a collection that
	//! is currently selected.  Unlike a normal string setting,
	//! this must correspond to a valid preferences context name.
	@property (strong, nonnull) PreferenceValue_StringDescriptor*
	currentValueDescriptor; // binding
	//! An automatically-synchronized list of names of current
	//! contexts in the class given to the initializer (e.g. if
	//! Quills::Prefs::FORMAT was given, this list will always
	//! hold the names of all Format collections).
	@property (strong, readonly, nonnull) NSArray< PreferenceValue_StringDescriptor* >*
	valueDescriptorArray; // binding

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
