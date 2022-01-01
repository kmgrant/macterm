/*!	\file PreferenceValue.mm
	\brief Presentation of preference values in user interfaces.
	
	This greatly simplifies bindings for multiple views
	that handle the same setting: namely, the one or more
	views that are used for the value of a preference and
	the “inherit” checkbox that reflects whether or not the
	preference comes from a parent context.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

#import "PreferenceValue.objc++.h"
#import <UniversalDefines.h>

// Mac includes
#import <objc/objc-runtime.h>
@import Cocoa;

// library includes
#import <CFRetainRelease.h>
#import <CocoaExtensions.objc++.h>
#import <ListenerModel.h>

// application includes
#import "ConstantsRegistry.h"
#import "UIStrings.h"



#pragma mark Types

/*!
Private properties.
*/
@interface PreferenceValue_Array () //{

// accessors
	//! Helper for storing the currently-selected index in the array.
	@property (strong) PreferenceValue_Number*
	preferenceAccessObject;

@end //}


/*!
Private properties.
*/
@interface PreferenceValue_CollectionBinding () //{

// accessors
	//! If set, the "valueDescriptorArray" starts with a Default
	//! item representing the context from which others inherit
	//! undefined values.
	@property (assign) BOOL
	includeDefaultFlag;
	//! Helper for storing the currently-selected collection’s name.
	@property (strong) PreferenceValue_String*
	preferenceAccessObject;
	@property (strong) ListenerModel_StandardListener*
	preferenceChangeListener;
	@property (assign) Quills::Prefs::Class
	preferencesClass;
	//! The message to send to "targetForDidRebuildArray" whenever
	//! the contents of "valueDescriptorArray" have been refreshed
	//! to match underlying preferences.  For example, this would
	//! happen if a user adds or deletes or renames a collection
	//! in the specified class.
	@property (assign) SEL
	selectorForDidRebuildArray;
	//! An object that can receive the message indicated by
	//! "selectorForDidRebuildArray".
	@property (assign) id
	targetForDidRebuildArray;
	// (See header file description.  Internally writable.)
	@property (strong) NSArray< PreferenceValue_StringDescriptor* >*
	valueDescriptorArray;

@end //}


/*!
The private class interface.
*/
@interface PreferenceValue_CollectionBinding (PreferenceValue_CollectionBindingInternal) //{

// new methods
	- (void)
	rebuildDescriptorArray;

@end //}


/*!
Private properties.
*/
@interface PreferenceValue_FileSystemObject () //{

// accessors
	//! The directory flag given to the initializer.
	@property (assign) BOOL
	isDirectory;
	//! Set if the "initWithURLInfo..." initializer was used.
	@property (assign) BOOL
	isURLInfoObject;

@end //}


/*!
Private properties.
*/
@interface PreferenceValue_Flag () //{

// accessors
	//! Set to YES if the value represented in the UI is the opposite
	//! of the underlying preference value (i.e. if the preference is
	//! set to YES then something like a checkbox is UNCHECKED, and
	//! if set to NO then that checkbox would be CHECKED).
	@property (assign) BOOL
	inverted;

@end //}


/*!
Private properties.
*/
@interface PreferenceValue_Number () //{

// accessors
	//! For implementation use; see method "scaleExponent".
	//! For example, set to "-3" to scale by 10^-3 (or 1/1000).
	@property (assign) NSInteger
	scaleExponentValue;
	//! If set, scaled value is displayed as nearest integer.
	@property (assign) BOOL
	scaleWithRounding;
	//! The precise type of the underlying integer storage.
	@property (assign) PreferenceValue_CType
	valueCType;

@end //}


/*!
Private properties.
*/
@interface PreferenceValue_Rect () //{

// accessors
	//! Describes the way a rectangle is stored in preferences
	//! (e.g. whether 4 different values represent an origin and a
	//! size versus edges, and the numerical precision).
	@property (assign) PreferenceValue_RectType
	valueRectType;

@end //}


#pragma mark Variables
namespace {

Float32				gScaleFactorsByExponentOffset[] =
					{
						0.001f,	// -3
						0.01f,	// -2
						0.1f,	// -1
						1,		// 0
						10,		// 1
						100,		// 2
						1000		// 3
					};
NSInteger const		kMinExponent = -3; // arbitrary; should be in sync with array above!
STATIC_ASSERT(((sizeof(gScaleFactorsByExponentOffset) / sizeof(Float32)) == (-kMinExponent * 2 + 1)),
				assert_correct_array_size_for_exp_range);

} // anonymous namespace


#pragma mark Internal Methods

#pragma mark -
@implementation PreferenceValue_Inherited


@synthesize propertiesByKey = _propertiesByKey;


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super init];
	if (nil != self)
	{
		_prefsMgr = aContextMgr;
		_propertiesByKey = nil;
	}
	return self;
}// initWithContextManager:


#pragma mark New Methods


/*!
Call within a subclass method that sets a new preference value
so that related settings can synchronize.

(4.1)
*/
- (void)
didSetPreferenceValue
{
	[self didChangeValueForKey:@"inheritEnabled"];
	[self didChangeValueForKey:@"inherited"];
}
- (void)
willSetPreferenceValue
{
	[self willChangeValueForKey:@"inherited"];
	[self willChangeValueForKey:@"inheritEnabled"];
}// willSetPreferenceValue


#pragma mark Overrides for Subclasses


/*!
Returns YES only if the current preference has been read from
a parent context (in other words, it is NOT DEFINED in the
current context).

See also "setInherited:", which is implemented for you.
*/
- (BOOL)
isInherited
{
	NSAssert(false, @"isInherited method must be implemented by PreferenceValue_Inherited subclasses");
	return NO;
}// isInherited


/*!
Delete the current value of the preference, allowing it to
inherit from any parent context (such as the Default context
or the factory defaults).  This is a side effect of invoking
"setInherited:YES".

(4.1)
*/
- (void)
setNilPreferenceValue
{
	NSAssert(false, @"setNilPreferenceValue method must be implemented by PreferenceValue_Inherited subclasses");
}// setNilPreferenceValue


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInheritEnabled
{
	return (NO == [self isInherited]);
}// isInheritEnabled


/*!
Accessor.

(4.1)
*/
- (void)
setInherited:(BOOL)		aFlag
{
	[self willChangeValueForKey:@"inheritEnabled"];
	if (aFlag)
	{
		// the “inherited” flag can be removed by deleting the value
		[self setNilPreferenceValue];
	}
	else
	{
		// this particular request doesn’t make sense; it is implied by
		// setting any new value
		Console_Warning(Console_WriteLine, "request to change “inherited” state to false, which is ignored");
	}
	[self didChangeValueForKey:@"inheritEnabled"];
}// setInherited:


/*!
Accessor.

(4.1)
*/
- (NSMutableDictionary*)
propertiesByKey
{
	if (nil == _propertiesByKey)
	{
		_propertiesByKey = [[NSMutableDictionary alloc] initWithCapacity:1]; // arbitrary initial size
	}
	return _propertiesByKey;
}// propertiesByKey


@end // PreferenceValue_Inherited


#pragma mark -
@implementation PreferenceValue_InheritedSingleTag


@synthesize preferencesTag = _preferencesTag;


#pragma mark Initializers


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithContextManager:aContextMgr];
	if (nil != self)
	{
		_preferencesTag = aTag;
	}
	return self;
}// initWithPreferencesTag:contextManager:


#pragma mark Initializers Disabled From Superclass


/*!
This initializer exists only to catch incorrect calls by
subclasses.

(4.1)
*/
- (instancetype)
initWithContextManager:(PrefsContextManager_Object*)	aContextMgr
{
#pragma unused(aContextMgr)
	NSAssert(false, @"initWithContextManager: initializer is not valid for subclasses of PreferenceValue_InheritedSingleTag");
	return nil;
}// initWithContextManager:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (Preferences_Tag)
preferencesTag
{
	return _preferencesTag;
}
- (void)
setPreferencesTag:(Preferences_Tag)		aTag
{
	[self willSetPreferenceValue];
	_preferencesTag = aTag;
	[self didSetPreferenceValue];
}// setPreferencesTag:


@end // PreferenceValue_InheritedSingleTag


#pragma mark -
@implementation PreferenceValue_IntegerDescriptor


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithIntegerValue:(UInt32)	aValue
description:(NSString*)			aString
{
	self = [super initWithBoundName:aString];
	if (nil != self)
	{
		_describedIntegerValue = aValue;
	}
	return self;
}// initWithIntegerValue:description:


@end // PreferenceValue_IntegerDescriptor


#pragma mark -
@implementation PreferenceValue_StringDescriptor


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithStringValue:(NSString*)		aValue
description:(NSString*)				aString
{
	self = [super initWithBoundName:aString];
	if (nil != self)
	{
		_describedStringValue = aValue;
	}
	return self;
}// initWithStringValue:description:


@end // PreferenceValue_StringDescriptor


#pragma mark -
@implementation PreferenceValue_Array


/*!
Designated initializer.

This initializer is for arrays of values that are
stored numerically.  In the future this class may
support other types of enumerations, e.g. lists of
fixed string values.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
preferenceCType:(PreferenceValue_CType)			aCType
valueDescriptorArray:(NSArray*)					aDescriptorArray
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
		_valueDescriptorArray = [aDescriptorArray copy];
		_preferenceAccessObject = [[PreferenceValue_Number alloc]
									initWithPreferencesTag:aTag contextManager:aContextMgr preferenceCType:aCType];
		
		// monitor the preferences context manager so that observers
		// of preferences in sub-objects can be told to expect changes
		[self whenObject:aContextMgr postsNote:kPrefsContextManager_ContextWillChangeNotification
							performSelector:@selector(prefsContextWillChange:)];
		[self whenObject:aContextMgr postsNote:kPrefsContextManager_ContextDidChangeNotification
							performSelector:@selector(prefsContextDidChange:)];
	}
	return self;
}// initWithPreferencesTag:contextManager:preferenceCType:valueDescriptorArray:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
}// dealloc


#pragma mark New Methods


/*!
Responds to a change in preferences context by notifying
observers that key values have changed (so that updates
to the user interface occur).

(4.1)
*/
- (void)
prefsContextDidChange:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	// note: should be opposite order of "prefsContextWillChange:"
	[self didChangeValueForKey:@"currentMultiValueDescriptors"];
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didSetPreferenceValue];
}// prefsContextDidChange:


/*!
Responds to a change in preferences context by notifying
observers that key values have changed (so that updates
to the user interface occur).

(4.1)
*/
- (void)
prefsContextWillChange:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	// note: should be opposite order of "prefsContextDidChange:"
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentValueDescriptor"];
	[self willChangeValueForKey:@"currentMultiValueDescriptors"];
}// prefsContextWillChange:


/*!
Accessor.

(4.1)
*/
- (id)
currentValueDescriptor
{
	UInt32		currentValue = [[self.preferenceAccessObject numberStringValue] intValue];
	id			result = nil;
	
	
	for (id object in [self valueDescriptorArray])
	{
		PreferenceValue_IntegerDescriptor*	asDesc = STATIC_CAST(object, PreferenceValue_IntegerDescriptor*);
		
		
		if (currentValue == [asDesc describedIntegerValue])
		{
			result = asDesc;
			break;
		}
	}
	
	if (nil == result)
	{
		// preferences setting did not match anything in the
		// predefined list of described values
		//Console_Warning(Console_WriteValueFourChars, "search failed for preference tag", self.preferencesTag); // debug
		//Console_Warning(Console_WriteValue, "not found in array: value", currentValue); // debug
		result = self.placeholderDescriptor;
	}
	
	return result;
}
- (void)
setCurrentValueDescriptor:(id)	selectedObject
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentValueDescriptor"];
	
	if (nil == selectedObject)
	{
		[self setNilPreferenceValue];
	}
	else
	{
		PreferenceValue_IntegerDescriptor*	asInfo = (PreferenceValue_IntegerDescriptor*)selectedObject;
		
		
		[self.preferenceAccessObject setNumberStringValue:
										[[NSNumber numberWithInt:[asInfo describedIntegerValue]] stringValue]];
	}
	
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didSetPreferenceValue];
}// setCurrentValueDescriptor:


/*!
Accessor.

(4.1)
*/
- (NSArray*)
currentMultiValueDescriptors
{
	UInt32				currentValue = [[self.preferenceAccessObject numberStringValue] intValue];
	NSMutableArray*		result = [[NSMutableArray alloc] initWithCapacity:0];
	
	
	for (id object in [self valueDescriptorArray])
	{
		PreferenceValue_IntegerDescriptor*	asDesc = STATIC_CAST(object, PreferenceValue_IntegerDescriptor*);
		
		
		// the assumption is that multi-value integers will be bits
		// that can be compared as bit sets
		if (currentValue & [asDesc describedIntegerValue])
		{
			[result addObject:asDesc];
		}
	}
	
	return result;
}
- (void)
setCurrentMultiValueDescriptors:(NSArray*)	selectedObjects
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentMultiValueDescriptors"];
	
	if (nil == selectedObjects)
	{
		[self setNilPreferenceValue];
	}
	else
	{
		UInt32		newValue = 0L;
		
		
		for (id object in selectedObjects)
		{
			PreferenceValue_IntegerDescriptor*	asInfo = (PreferenceValue_IntegerDescriptor*)object;
			
			
			// the assumption is that multi-value integers will be bits
			// that can be combined as bit sets
			newValue |= [asInfo describedIntegerValue];
		}
		
		[self.preferenceAccessObject setNumberStringValue:[[NSNumber numberWithInt:newValue] stringValue]];
	}
	
	[self didChangeValueForKey:@"currentMultiValueDescriptors"];
	[self didSetPreferenceValue];
}// setCurrentMultiValueDescriptors:


#pragma mark PreferenceValue_Inherited


/*!
Embellishes base class implementation to also synchronize
the preferences tag associated with the internal access
object.

(4.1)
*/
- (void)
didSetPreferenceValue
{
	self.preferenceAccessObject.preferencesTag = self.preferencesTag;
	[super didSetPreferenceValue];
}// didSetPreferenceValue


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = self.preferenceAccessObject.isInherited;
	
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentMultiValueDescriptors"];
	[self willChangeValueForKey:@"currentValueDescriptor"];
	[self.preferenceAccessObject setNilPreferenceValue];
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didChangeValueForKey:@"currentMultiValueDescriptors"];
	[self didSetPreferenceValue];
}// setNilPreferenceValue


@end // PreferenceValue_Array


#pragma mark -
@implementation PreferenceValue_CollectionBinding


/*!
Initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
sourceClass:(Quills::Prefs::Class)				aPreferencesClass
includeDefault:(BOOL)							aDefaultFlag
{
	return [self initWithPreferencesTag:aTag contextManager:aContextMgr sourceClass:aPreferencesClass
										includeDefault:aDefaultFlag
										didRebuildTarget:nil didRebuildSelector:nil];
}// initWithPreferencesTag:contextManager:sourceClass:


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
sourceClass:(Quills::Prefs::Class)				aPreferencesClass
includeDefault:(BOOL)							aDefaultFlag
didRebuildTarget:(id)							aTarget
didRebuildSelector:(SEL)						aSelector
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
		_valueDescriptorArray = @[];
		_targetForDidRebuildArray = aTarget;
		_selectorForDidRebuildArray = aSelector;
		_includeDefaultFlag = aDefaultFlag;
		_preferencesClass = aPreferencesClass;
		_preferenceAccessObject = [[PreferenceValue_String alloc] initWithPreferencesTag:aTag contextManager:aContextMgr];
		
		// monitor the preferences context manager so that observers
		// of preferences in sub-objects can be told to expect changes
		[self whenObject:aContextMgr postsNote:kPrefsContextManager_ContextWillChangeNotification
							performSelector:@selector(prefsContextWillChange:)];
		[self whenObject:aContextMgr postsNote:kPrefsContextManager_ContextDidChangeNotification
							performSelector:@selector(prefsContextDidChange:)];
		
		// install a callback that finds out about changes to available preferences collections
		// ("rebuildDescriptorArray" will be implicitly called as a side effect of this setup)
		{
			Preferences_Result		error = kPreferences_ResultOK;
			
			
			_preferenceChangeListener = [[ListenerModel_StandardListener alloc]
											initWithTarget:self eventFiredSelector:@selector(model:preferenceChange:context:)];
			
			error = Preferences_StartMonitoring(self.preferenceChangeListener.listenerRef, kPreferences_ChangeContextName,
												false/* call immediately to get initial value */);
			assert(kPreferences_ResultOK == error);
			error = Preferences_StartMonitoring(self.preferenceChangeListener.listenerRef, kPreferences_ChangeNumberOfContexts,
												true/* call immediately to get initial value */);
			assert(kPreferences_ResultOK == error);
		}
	}
	return self;
}// initWithPreferencesTag:contextManager:sourceClass:includeDefault:didRebuildTarget:didRebuildSelector:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[self ignoreWhenObjectsPostNotes];
	UNUSED_RETURN(Preferences_Result)Preferences_StopMonitoring(self.preferenceChangeListener.listenerRef,
																kPreferences_ChangeContextName);
	UNUSED_RETURN(Preferences_Result)Preferences_StopMonitoring(self.preferenceChangeListener.listenerRef,
																kPreferences_ChangeNumberOfContexts);
}// dealloc


#pragma mark New Methods


/*!
Called when a monitored preference is changed.  See the
initializer for the set of events that is monitored.

(4.1)
*/
- (void)
model:(ListenerModel_Ref)				aModel
preferenceChange:(ListenerModel_Event)	anEvent
context:(void*)							aContext
{
#pragma unused(aModel, aContext)
	switch (anEvent)
	{
	case kPreferences_ChangeContextName:
		// a context has been renamed; refresh the list
		[self rebuildDescriptorArray];
		break;
	
	case kPreferences_ChangeNumberOfContexts:
		// contexts were added or removed; destroy and rebuild the list
		[self rebuildDescriptorArray];
		break;
	
	default:
		// ???
		break;
	}
}// model:preferenceChange:context:


/*!
Responds to a change in preferences context by notifying
observers that key values have changed (so that updates
to the user interface occur).

(4.1)
*/
- (void)
prefsContextDidChange:(NSNotification*)		aNotification
{
#pragma unused(aNotification)
	// note: should be opposite order of "prefsContextWillChange:"
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didSetPreferenceValue];
}// prefsContextDidChange:


/*!
Responds to a change in preferences context by notifying
observers that key values have changed (so that updates
to the user interface occur).

(4.1)
*/
- (void)
prefsContextWillChange:(NSNotification*)	aNotification
{
#pragma unused(aNotification)
	// note: should be opposite order of "prefsContextDidChange:"
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentValueDescriptor"];
}// prefsContextWillChange:


/*!
Returns the preference’s current value, and indicates whether or
not that value was inherited from a parent context.

(4.1)
*/
- (NSString*)
readValueSeeIfDefault:(BOOL*)	outIsDefault
{
	return [self.preferenceAccessObject readValueSeeIfDefault:outIsDefault];
}// readValueSeeIfDefault:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (PreferenceValue_StringDescriptor*)
currentValueDescriptor
{
	// NOTE: the “is default” nature of a value is reflected
	// separately in the user interface (inheritance indicator)
	// and NOT in the value; see "isInherited"
	CFRetainRelease						defaultName(UIStrings_ReturnCopy(kUIStrings_PreferencesWindowDefaultFavoriteName),
													CFRetainRelease::kAlreadyRetained);
	NSString*							asNSStringDefaultName = BRIDGE_CAST(defaultName.returnCFStringRef(), NSString*);
	NSString*							currentValue = [self.preferenceAccessObject stringValue];
	PreferenceValue_StringDescriptor*	result = nil;
	
	
	for (PreferenceValue_StringDescriptor* asDesc in self.valueDescriptorArray)
	{
		NSString*		thisValue = asDesc.describedStringValue;
		
		
		// consider an empty string or a Default string to be equivalent
		if ([currentValue isEqualToString:thisValue] ||
			([currentValue isEqualToString:@""] && [thisValue isEqualToString:asNSStringDefaultName]))
		{
			result = asDesc;
			break;
		}
	}
	
	return result;
}
- (void)
setCurrentValueDescriptor:(PreferenceValue_StringDescriptor*)	selectedObject
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentValueDescriptor"];
	
	if (nil == selectedObject)
	{
		[self setNilPreferenceValue];
	}
	else
	{
		[self.preferenceAccessObject setStringValue:[selectedObject describedStringValue]];
	}
	
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didSetPreferenceValue];
}// setCurrentValueDescriptor:


#pragma mark PreferenceValue_Inherited


/*!
Embellishes base class implementation to also synchronize
the preferences tag associated with the internal access
object.

(4.1)
*/
- (void)
didSetPreferenceValue
{
	self.preferenceAccessObject.preferencesTag = self.preferencesTag;
	[super didSetPreferenceValue];
}// didSetPreferenceValue


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = self.preferenceAccessObject.isInherited;
	
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self willSetPreferenceValue];
	[self willChangeValueForKey:@"currentValueDescriptor"];
	[self.preferenceAccessObject setNilPreferenceValue];
	[self didChangeValueForKey:@"currentValueDescriptor"];
	[self didSetPreferenceValue];
}// setNilPreferenceValue


@end // PreferenceValue_CollectionBinding


#pragma mark -
@implementation PreferenceValue_CollectionBinding (PreferenceValue_CollectionBindingInternal)


#pragma mark New Methods


/*!
Rebuilds the array of value descriptors that represents the
available preferences contexts (for example, in a menu that is
displayed to the user).

This should be called whenever preferences are changed
(contexts added, removed or renamed).

(4.1)
*/
- (void)
rebuildDescriptorArray
{
	CFArrayRef				newNameArray = nullptr;
	Preferences_Result		arrayResult = Preferences_CreateContextNameArray(_preferencesClass, newNameArray, _includeDefaultFlag);
	
	
	if (kPreferences_ResultOK != arrayResult)
	{
		Console_Warning(Console_WriteValue, "unable to create context name array for collection binding, error", arrayResult);
		self.valueDescriptorArray = @[];
	}
	else
	{
		NSArray*			asNSArray = BRIDGE_CAST(newNameArray, NSArray*);
		NSMutableArray*		newValueDescArray = [[NSMutableArray alloc] initWithCapacity:asNSArray.count];
		
		
		for (NSString* collectionName in asNSArray)
		{
			[newValueDescArray addObject:[[PreferenceValue_StringDescriptor alloc]
												initWithStringValue:collectionName description:collectionName]];
		}
		
		// make the field refer to the new array (released later)
		self.valueDescriptorArray = newValueDescArray;
		
		CFRelease(newNameArray), newNameArray = nullptr;
	}
}// rebuildDescriptorArray


@end // PreferenceValue_CollectionBinding (PreferenceValue_CollectionBindingInternal)


#pragma mark -
@implementation PreferenceValue_Color


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
	}
	return self;
}// initWithPreferencesTag:contextManager:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSColor*)
colorValue
{
	BOOL		isDefault = NO;
	NSColor*	result = [self.prefsMgr readColorForPreferenceTag:self.preferencesTag isDefault:&isDefault];
	
	
	return result;
}
- (void)
setColorValue:(NSColor*)	aColor
{
	[self willSetPreferenceValue];
	
	BOOL	saveOK = [self.prefsMgr writeColor:aColor forPreferenceTag:self.preferencesTag];
	
	
	if (NO == saveOK)
	{
		Console_Warning(Console_WriteValueFourChars, "failed to save a color preference",
						self.preferencesTag);
	}
	
	[self didSetPreferenceValue];
}// setColorValue:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = NO;
	
	
	UNUSED_RETURN(NSColor*)[self.prefsMgr readColorForPreferenceTag:self.preferencesTag isDefault:&result];
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self setColorValue:nil];
}// setNilPreferenceValue


@end // PreferenceValue_Color


#pragma mark -
@implementation PreferenceValue_FileSystemObject


/*!
Designated initializer.

(2017.05)
*/
- (instancetype)
initWithURLPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
isDirectory:(BOOL)								aDirectoryFlag
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
		_isDirectory = aDirectoryFlag;
		_isURLInfoObject = NO;
	}
	return self;
}// initWithURLPreferencesTag:contextManager:isDirectory:


/*!
Designated initializer.

(2017.05)
*/
- (instancetype)
initWithURLInfoPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)		aContextMgr
isDirectory:(BOOL)									aDirectoryFlag
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
		_isDirectory = aDirectoryFlag;
		_isURLInfoObject = YES;
	}
	return self;
}// initWithURLInfoPreferencesTag:contextManager:isDirectory:


#pragma mark New Methods


/*!
Returns the preference’s current value, and indicates whether or
not that value was inherited from a parent context.

(4.1)
*/
- (NSString*)
readValueSeeIfDefault:(BOOL*)	outIsDefault
{
	NSString*				result = @"";
	Boolean					isDefault = false;
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	
	
	if (Preferences_ContextIsValid(sourceContext))
	{
		CFURLRef			fileURLValue = nullptr;
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		
		
		if (self.isURLInfoObject)
		{
			// preferences tag uses CFURLRef indirectly
			// through the Preferences_URLInfo structure
			Preferences_URLInfo		urlInfo;
			
			
			prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
														sizeof(urlInfo), &urlInfo,
														true/* search defaults */, &isDefault);
			if (kPreferences_ResultOK == prefsResult)
			{
				fileURLValue = urlInfo.urlRef;
				if (urlInfo.isStale)
				{
					// if the retrieved URL came from a stale bookmark, the
					// new URL should be saved back to preferences
					Preferences_Result		updateResult = Preferences_ContextSetData
															(sourceContext, self.preferencesTag,
																sizeof(urlInfo), &urlInfo);
					
					
					Console_Warning(Console_WriteValueCFString, "stale bookmark was saved, new URL", CFURLGetString(urlInfo.urlRef));
					if (kPreferences_ResultOK != updateResult)
					{
						// failed to update (ignore for now)
						Console_Warning(Console_WriteValue, "unable to save updated bookmark, error", updateResult);
					}
				}
			}
		}
		else
		{
			// preferences tag uses a CFURLRef directly
			prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
														sizeof(fileURLValue), &fileURLValue,
														true/* search defaults */, &isDefault);
		}
		
		if (kPreferences_ResultOK == prefsResult)
		{
			result = BRIDGE_CAST(CFURLCopyFileSystemPath(fileURLValue, kCFURLPOSIXPathStyle), NSString*);
		}
		
		// the Preferences module always creates a new URL so
		// release whatever was returned
		if (nullptr != fileURLValue)
		{
			CFRelease(fileURLValue), fileURLValue = nullptr;
		}
	}
	
	if (nullptr != outIsDefault)
	{
		*outIsDefault = (true == isDefault);
	}
	
	return result;
}// readValueSeeIfDefault:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
stringValue
{
	BOOL		isDefault = NO;
	NSString*	result = [self readValueSeeIfDefault:&isDefault];
	
	
	return result;
}
- (void)
setStringValue:(NSString*)	aString
{
	[self willSetPreferenceValue];
	
	if (nil == aString)
	{
		// when given nothing and the context is non-Default, delete the setting;
		// this will revert to either the Default value (in non-Default contexts)
		// or the “factory default” value (in Default contexts)
		BOOL	deleteOK = [self.prefsMgr deleteDataForPreferenceTag:self.preferencesTag];
		
		
		if (NO == deleteOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to remove file-value preference",
							self.preferencesTag);
		}
	}
	else
	{
		BOOL					saveOK = NO;
		Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
		
		
		if (Preferences_ContextIsValid(targetContext))
		{
			NSURL*		fileURLValue = [NSURL fileURLWithPath:aString isDirectory:self.isDirectory];
			
			
			if (nil != fileURLValue)
			{
				CFURLRef			asCFURL = BRIDGE_CAST(fileURLValue, CFURLRef);
				Preferences_Result	prefsResult = kPreferences_ResultOK;
				
				
				if (self.isURLInfoObject)
				{
					// preferences tag uses CFURLRef indirectly
					// through the Preferences_URLInfo structure
					Preferences_URLInfo		urlInfo;
					
					
					bzero(&urlInfo, sizeof(urlInfo));
					urlInfo.urlRef = asCFURL;
					urlInfo.isStale = false; // (this is ignored when setting the value)
					prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																sizeof(urlInfo), &urlInfo);
				}
				else
				{
					// preferences tag uses a CFURLRef directly
					prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																sizeof(asCFURL), &asCFURL);
				}
				
				if (kPreferences_ResultOK == prefsResult)
				{
					saveOK = YES;
				}
			}
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to save file-value preference",
							self.preferencesTag);
		}
	}
	
	[self didSetPreferenceValue];
}// setStringValue:


/*!
Accessor.

(4.1)
*/
- (NSURL*)
URLValue
{
	BOOL		isDefault = NO;
	NSString*	result = [self readValueSeeIfDefault:&isDefault];
	
	
	return [NSURL fileURLWithPath:result isDirectory:self.isDirectory];
}
- (void)
setURLValue:(NSURL*)	aURL
{
	if (nil == aURL)
	{
		[self setStringValue:nil];
	}
	else if ([aURL isFileURL])
	{
		[self setStringValue:[aURL path]];
	}
	else
	{
		Console_Warning(Console_WriteValueFourChars, "failed to save file-value preference because a non-file URL was given",
						self.preferencesTag);
	}
}// setURLValue:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = NO;
	
	
	UNUSED_RETURN(NSString*)[self readValueSeeIfDefault:&result];
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self setStringValue:nil];
}// setNilPreferenceValue


@end // PreferenceValue_FileSystemObject


#pragma mark -
@implementation PreferenceValue_Flag


/*!
Common initializer; assumes no inversion.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
{
	return [self initWithPreferencesTag:aTag contextManager:aContextMgr inverted:NO];
}// initWithPreferencesTag:contextManager:


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
inverted:(BOOL)									anInversionFlag
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
		_inverted = anInversionFlag;
	}
	return self;
}// initWithPreferencesTag:contextManager:inverted:


#pragma mark New Methods


/*!
Returns the preference’s current value, and indicates whether or
not that value was inherited from a parent context.

(4.1)
*/
- (BOOL)
readValueSeeIfDefault:(BOOL*)	outIsDefault
{
	BOOL					result = NO;
	Boolean					isDefault = false;
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	
	
	if (Preferences_ContextIsValid(sourceContext))
	{
		Boolean				flagValue = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
																		sizeof(flagValue), &flagValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			if (self.inverted)
			{
				flagValue = (! flagValue);
			}
			result = (flagValue) ? YES : NO;
		}
	}
	
	if (nullptr != outIsDefault)
	{
		*outIsDefault = (true == isDefault);
	}
	
	return result;
}// readValueSeeIfDefault:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSNumber*)
numberValue
{
	BOOL		isDefault = NO;
	NSNumber*	result = ([self readValueSeeIfDefault:&isDefault]) ? @(YES) : @(NO);
	
	
	return result;
}
- (void)
setNumberValue:(NSNumber*)		aFlag
{
	[self willSetPreferenceValue];
	
	if (nil == aFlag)
	{
		// when given nothing and the context is non-Default, delete the setting;
		// this will revert to either the Default value (in non-Default contexts)
		// or the “factory default” value (in Default contexts)
		BOOL	deleteOK = [self.prefsMgr deleteDataForPreferenceTag:self.preferencesTag];
		
		
		if (NO == deleteOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to remove flag-value preference",
							self.preferencesTag);
		}
	}
	else
	{
		BOOL					saveOK = NO;
		Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
		
		
		if (Preferences_ContextIsValid(targetContext))
		{
			Boolean				asBoolean = (self.inverted)
											? (NO == [aFlag boolValue])
											: (YES == [aFlag boolValue]);
			Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																			sizeof(asBoolean), &asBoolean);
			
			
			if (kPreferences_ResultOK == prefsResult)
			{
				saveOK = YES;
			}
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to save flag-value preference",
							self.preferencesTag);
		}
	}
	
	[self didSetPreferenceValue];
}// setNumberValue:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = NO;
	
	
	UNUSED_RETURN(BOOL)[self readValueSeeIfDefault:&result];
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self setNumberValue:nil];
}// setNilPreferenceValue


@end // PreferenceValue_Flag


#pragma mark -
@implementation PreferenceValue_Number


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
preferenceCType:(PreferenceValue_CType)			aCType
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
		_scaleExponentValue = 0;
		_scaleWithRounding = YES;
		_valueCType = aCType;
	}
	return self;
}// initWithPreferencesTag:contextManager:preferenceCType:


#pragma mark New Methods


/*!
Returns the preference’s current value, and indicates whether or
not that value was inherited from a parent context.

(4.1)
*/
- (NSNumber*)
readValueSeeIfDefault:(BOOL*)	outIsDefault
{
	NSNumber*				result = nil;
	Boolean					isDefault = false;
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	
	
	if (Preferences_ContextIsValid(sourceContext))
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		
		
		switch (self.valueCType)
		{
		case kPreferenceValue_CTypeSInt16:
			{
				SInt16		intValue = 0;
				
				
				prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
															sizeof(intValue), &intValue, true/* search defaults */,
															&isDefault);
				if (kPreferences_ResultOK == prefsResult)
				{
					result = [NSNumber numberWithInt:intValue];
				}
			}
			break;
		
		case kPreferenceValue_CTypeUInt16:
			{
				UInt16		intValue = 0;
				
				
				prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
															sizeof(intValue), &intValue, true/* search defaults */,
															&isDefault);
				if (kPreferences_ResultOK == prefsResult)
				{
					result = [NSNumber numberWithUnsignedInt:intValue];
				}
			}
			break;
		
		case kPreferenceValue_CTypeSInt32:
			{
				SInt32		intValue = 0L;
				
				
				prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
															sizeof(intValue), &intValue, true/* search defaults */,
															&isDefault);
				if (kPreferences_ResultOK == prefsResult)
				{
					result = [NSNumber numberWithLong:intValue];
				}
			}
			break;
		
		case kPreferenceValue_CTypeUInt32:
			{
				UInt32		intValue = 0L;
				
				
				prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
															sizeof(intValue), &intValue, true/* search defaults */,
															&isDefault);
				if (kPreferences_ResultOK == prefsResult)
				{
					result = [NSNumber numberWithUnsignedLong:intValue];
				}
			}
			break;
		
		case kPreferenceValue_CTypeFloat32:
			{
				Float32		floatValue = 0.0;
				
				
				prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
															sizeof(floatValue), &floatValue, true/* search defaults */,
															&isDefault);
				if (kPreferences_ResultOK == prefsResult)
				{
					if (0 != self.scaleExponent)
					{
						floatValue /= gScaleFactorsByExponentOffset[self.scaleExponent - kMinExponent];
						if (self.scaleWithRounding)
						{
							floatValue = STATIC_CAST(roundf(floatValue), Float32);
						}
					}
					result = [NSNumber numberWithFloat:floatValue];
				}
			}
			break;
		
		case kPreferenceValue_CTypeFloat64:
			{
				Float64		floatValue = 0.0;
				
				
				prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
															sizeof(floatValue), &floatValue, true/* search defaults */,
															&isDefault);
				if (kPreferences_ResultOK == prefsResult)
				{
					if (0 != self.scaleExponent)
					{
						floatValue /= gScaleFactorsByExponentOffset[self.scaleExponent - kMinExponent];
						if (self.scaleWithRounding)
						{
							floatValue = STATIC_CAST(round(floatValue), Float64);
						}
					}
					result = [NSNumber numberWithDouble:floatValue];
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	
	if (nullptr != outIsDefault)
	{
		*outIsDefault = (true == isDefault);
	}
	
	return result;
}// readValueSeeIfDefault:


#pragma mark Accessors: Configuration


/*!
Accessor.

(4.1)
*/
- (NSInteger)
scaleExponent
{
	return self.scaleExponentValue;
}
- (void)
setScaleExponent:(NSInteger)	anExponent
rounded:(BOOL)					aRoundingFlag
{
	assert((kPreferenceValue_CTypeFloat32 == self.valueCType) ||
			(kPreferenceValue_CTypeFloat64 == self.valueCType));
	assert((anExponent >= kMinExponent) && (anExponent <= -kMinExponent));
	self.scaleExponentValue = anExponent;
	self.scaleWithRounding = aRoundingFlag;
}// setScaleExponent:


#pragma mark Accessors: Bindings


/*!
Accessor.

(4.1)
*/
- (NSNumber*)
numberValue
{
	BOOL		isDefault = NO;
	NSNumber*	result = [self readValueSeeIfDefault:&isDefault];
	
	
	return result;
}
- (void)
setNumberValue:(NSNumber*)	aNumber
{
	if (nil == aNumber)
	{
		[self setNumberStringValue:nil];
	}
	else
	{
		[self setNumberStringValue:[aNumber stringValue]];
	}
}// setNumberValue:


/*!
Accessor.

(4.1)
*/
- (NSString*)
numberStringValue
{
	BOOL		isDefault = NO;
	NSNumber*	asNumber = [self readValueSeeIfDefault:&isDefault];
	NSString*	result = [asNumber stringValue];
	
	
	return result;
}
- (void)
setNumberStringValue:(NSString*)	aNumberString
{
	[self willSetPreferenceValue];
	
	if (nil == aNumberString)
	{
		// when given nothing and the context is non-Default, delete the setting;
		// this will revert to either the Default value (in non-Default contexts)
		// or the “factory default” value (in Default contexts)
		BOOL	deleteOK = [self.prefsMgr deleteDataForPreferenceTag:self.preferencesTag];
		
		
		if (NO == deleteOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to remove numerical-value preference",
							self.preferencesTag);
		}
	}
	else
	{
		BOOL					saveOK = NO;
		Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
		
		
		if (Preferences_ContextIsValid(targetContext))
		{
			Preferences_Result	prefsResult = kPreferences_ResultOK;
			
			
			// NOTE: The validation method will scrub the string beforehand so
			// requests for numerical values should not fail here.
			switch (self.valueCType)
			{
			case kPreferenceValue_CTypeSInt16:
				{
					SInt16		intValue = STATIC_CAST([aNumberString intValue], SInt16);
					
					
					prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																sizeof(intValue), &intValue);
				}
				break;
			
			case kPreferenceValue_CTypeUInt16:
				{
					UInt16		intValue = STATIC_CAST([aNumberString intValue], UInt16);
					
					
					prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																sizeof(intValue), &intValue);
				}
				break;
			
			case kPreferenceValue_CTypeSInt32:
				{
					SInt32		intValue = [aNumberString intValue];
					
					
					prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																sizeof(intValue), &intValue);
				}
				break;
			
			case kPreferenceValue_CTypeUInt32:
				{
					UInt32		intValue = [aNumberString intValue];
					
					
					prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																sizeof(intValue), &intValue);
				}
				break;
			
			case kPreferenceValue_CTypeFloat32:
				{
					Float32		floatValue = [aNumberString floatValue];
					
					
					if (0 != self.scaleExponent)
					{
						// ignore "self.scaleWithRounding" (not enforced for input strings)
						floatValue *= gScaleFactorsByExponentOffset[self.scaleExponent - kMinExponent];
					}
					prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																sizeof(floatValue), &floatValue);
				}
				break;
			
			case kPreferenceValue_CTypeFloat64:
				{
					Float64		floatValue = [aNumberString doubleValue];
					
					
					if (0 != self.scaleExponent)
					{
						// ignore "self.scaleWithRounding" (not enforced for input strings)
						floatValue *= gScaleFactorsByExponentOffset[self.scaleExponent - kMinExponent];
					}
					prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																sizeof(floatValue), &floatValue);
				}
				break;
			
			default:
				// ???
				break;
			}
			
			if (kPreferences_ResultOK == prefsResult)
			{
				saveOK = YES;
			}
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to save numerical-value preference",
							self.preferencesTag);
		}
	}
	
	[self didSetPreferenceValue];
}// setNumberStringValue:


#pragma mark Validators


/*!
Validates a number entered by the user, returning an appropriate
error (and a NO result) if the number is incorrect.

(4.0)
*/
- (BOOL)
validateNumberStringValue:(id*/* NSString* */)		ioValue
error:(NSError**)								outError
{
	BOOL	result = NO;
	
	
	if (nil == *ioValue)
	{
		result = YES;
	}
	else
	{
		// first strip whitespace
		*ioValue = [*ioValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
		
		// while an NSNumberFormatter is more typical for validation,
		// the requirements for numbers are quite simple
		NSScanner*	scanner = [NSScanner scannerWithString:*ioValue];
		long long	integerValue = 0LL;
		float		floatValue = 0.0;
		double		doubleValue = 0.0;
		BOOL		scanOK = NO;
		
		
		if (kPreferenceValue_CTypeFloat32 == self.valueCType)
		{
			scanOK = ([scanner scanFloat:&floatValue] && [scanner isAtEnd]);
		}
		else if (kPreferenceValue_CTypeFloat64 == self.valueCType)
		{
			scanOK = ([scanner scanDouble:&doubleValue] && [scanner isAtEnd]);
		}
		else
		{
			scanOK = ([scanner scanLongLong:&integerValue] && [scanner isAtEnd]);
			if (scanOK)
			{
				if ((kPreferenceValue_CTypeUInt16 == self.valueCType) ||
					(kPreferenceValue_CTypeUInt32 == self.valueCType))
				{
					scanOK = (integerValue >= 0);
				}
			}
		}
		
		if (scanOK)
		{
			result = YES;
		}
		else
		{
			if (nil != outError) result = NO;
			else result = YES; // cannot return NO when the error instance is undefined
		}
		
		if (NO == result)
		{
			NSString*	errorMessage = nil;
			
			
			switch (self.valueCType)
			{
			case kPreferenceValue_CTypeUInt16:
			case kPreferenceValue_CTypeUInt32:
				errorMessage = NSLocalizedStringFromTable(@"This value must be a nonnegative integer.",
															@"PrefsWindow"/* table */,
															@"message displayed for bad unsigned integer values");
				break;
			
			case kPreferenceValue_CTypeFloat32:
			case kPreferenceValue_CTypeFloat64:
				errorMessage = NSLocalizedStringFromTable(@"This value must be a number (optionally with a fraction after a decimal point).",
															@"PrefsWindow"/* table */,
															@"message displayed for bad floating-point values");
				break;
			
			case kPreferenceValue_CTypeSInt16:
			case kPreferenceValue_CTypeSInt32:
			default:
				errorMessage = NSLocalizedStringFromTable(@"This value must be an integer (it may be negative).",
															@"PrefsWindow"/* table */,
															@"message displayed for bad signed integer values");
				break;
			}
			
			*outError = [NSError errorWithDomain:BRIDGE_CAST(kConstantsRegistry_NSErrorDomainAppDefault, NSString*)
							code:kConstantsRegistry_NSErrorBadNumber
							userInfo:@{ NSLocalizedDescriptionKey: errorMessage }];
		}
	}
	return result;
}// validateNumberStringValue:error:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = NO;
	
	
	UNUSED_RETURN(NSNumber*)[self readValueSeeIfDefault:&result];
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self setNumberStringValue:nil];
}// setNilPreferenceValue


@end // PreferenceValue_Number


#pragma mark -
@implementation PreferenceValue_Rect


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
preferenceRectType:(PreferenceValue_RectType)	aRectType
{
#pragma unused(aRectType)
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
		_valueRectType = aRectType;
	}
	return self;
}// initWithPreferencesTag:contextManager:preferenceRectType:


#pragma mark New Methods


/*!
Returns the preference’s current value, and indicates whether or
not that value was inherited from a parent context.

(4.1)
*/
- (NSArray*)
readValueSeeIfDefault:(BOOL*)	outIsDefault
{
	NSArray*				result = nil;
	Boolean					isDefault = false;
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	
	
	if (Preferences_ContextIsValid(sourceContext))
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		
		
		switch (self.valueRectType)
		{
		case kPreferenceValue_RectTypeHIRect:
			{
				Preferences_TopLeftCGRect	rectValue;
				
				
				prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
															sizeof(rectValue), &rectValue, true/* search defaults */,
															&isDefault);
				if (kPreferences_ResultOK == prefsResult)
				{
					result = @[[NSNumber numberWithFloat:rectValue.origin.x],
								[NSNumber numberWithFloat:rectValue.origin.y],
								[NSNumber numberWithFloat:rectValue.size.width],
								[NSNumber numberWithFloat:rectValue.size.height]];
				}
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	
	if (nullptr != outIsDefault)
	{
		*outIsDefault = (true == isDefault);
	}
	
	return result;
}// readValueSeeIfDefault:


#pragma mark Accessors: Bindings


/*!
Accessor.

(4.1)
*/
- (NSArray*)
numberArrayValue
{
	BOOL		isDefault = NO;
	NSArray*	result = [self readValueSeeIfDefault:&isDefault];
	
	
	return result;
}
- (void)
setNumberArrayValue:(NSArray*)		aNumberArray
{
	[self willSetPreferenceValue];
	
	if (nil == aNumberArray)
	{
		// when given nothing and the context is non-Default, delete the setting;
		// this will revert to either the Default value (in non-Default contexts)
		// or the “factory default” value (in Default contexts)
		BOOL	deleteOK = [self.prefsMgr deleteDataForPreferenceTag:self.preferencesTag];
		
		
		if (NO == deleteOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to remove rectangle-value preference",
							self.preferencesTag);
		}
	}
	else
	{
		BOOL					saveOK = NO;
		Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
		
		
		if (Preferences_ContextIsValid(targetContext))
		{
			Preferences_Result	prefsResult = kPreferences_ResultOK;
			
			
			// NOTE: The validation method will scrub the string beforehand so
			// requests for numerical values should not fail here.
			switch (self.valueRectType)
			{
			case kPreferenceValue_RectTypeHIRect:
				{
					Preferences_TopLeftCGRect	rectValue;
					
					
					assert(4 == aNumberArray.count);
					rectValue.origin.x = [[aNumberArray objectAtIndex:0] floatValue];
					rectValue.origin.y = [[aNumberArray objectAtIndex:1] floatValue];
					rectValue.size.width = [[aNumberArray objectAtIndex:2] floatValue];
					rectValue.size.height = [[aNumberArray objectAtIndex:3] floatValue];
					
					prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																sizeof(rectValue), &rectValue);
				}
				break;
			
			default:
				// ???
				break;
			}
			
			if (kPreferences_ResultOK == prefsResult)
			{
				saveOK = YES;
			}
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to save rectangle-value preference",
							self.preferencesTag);
		}
	}
	
	[self didSetPreferenceValue];
}// setNumberArrayValue:


#pragma mark Validators


/*!
Validates an array of numbers, returning an appropriate
error (and a NO result) if the number array is incorrect.

(4.0)
*/
- (BOOL)
validateNumberArrayValue:(id*/* NSArray** */)	ioValue
error:(NSError**)							outError
{
	BOOL	result = NO;
	
	
	if (nil == *ioValue)
	{
		result = YES;
	}
	else
	{
		BOOL	arrayOK = NO;
		
		
		// TEMPORARY; minimal validation is done right now but
		// it could for instance try to avoid values that are
		// ridiculously out of range (yet, this itself may need
		// to be parameterized or handled by subclasses)
		if ([*ioValue respondsToSelector:@selector(count)])
		{
			arrayOK = (4 == [*ioValue count]);
		}
		
		if (arrayOK)
		{
			result = YES;
		}
		else
		{
			if (nil != outError) result = NO;
			else result = YES; // cannot return NO when the error instance is undefined
		}
		
		if (NO == result)
		{
			NSString*	errorMessage = nil;
			
			
			switch (self.valueRectType)
			{
			case kPreferenceValue_RectTypeHIRect:
			default:
				errorMessage = NSLocalizedStringFromTable(@"This value must be exactly four floating-point numbers.",
															@"PrefsWindow"/* table */,
															@"message displayed for bad rectangle values");
				break;
			}
			
			*outError = [NSError errorWithDomain:BRIDGE_CAST(kConstantsRegistry_NSErrorDomainAppDefault, NSString*)
							code:kConstantsRegistry_NSErrorBadArray
							userInfo:@{ NSLocalizedDescriptionKey: errorMessage }];
		}
	}
	return result;
}// validateNumberArrayValue:error:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = NO;
	
	
	UNUSED_RETURN(NSArray*)[self readValueSeeIfDefault:&result];
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self setNumberArrayValue:nil];
}// setNilPreferenceValue


@end // PreferenceValue_Rect


#pragma mark -
@implementation PreferenceValue_String


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
	}
	return self;
}// initWithPreferencesTag:contextManager:


#pragma mark New Methods


/*!
Returns the preference’s current value, and indicates whether or
not that value was inherited from a parent context.

(4.1)
*/
- (NSString*)
readValueSeeIfDefault:(BOOL*)	outIsDefault
{
	NSString*				result = @"";
	Boolean					isDefault = false;
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	
	
	if (Preferences_ContextIsValid(sourceContext))
	{
		CFStringRef			stringValue = nullptr;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
																		sizeof(stringValue), &stringValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			result = BRIDGE_CAST(stringValue, NSString*);
		}
	}
	
	if (nullptr != outIsDefault)
	{
		*outIsDefault = (true == isDefault);
	}
	
	return result;
}// readValueSeeIfDefault:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
stringValue
{
	BOOL		isDefault = NO;
	NSString*	result = [self readValueSeeIfDefault:&isDefault];
	
	
	return result;
}
- (void)
setStringValue:(NSString*)	aString
{
	[self willSetPreferenceValue];
	
	if (nil == aString)
	{
		// when given nothing and the context is non-Default, delete the setting;
		// this will revert to either the Default value (in non-Default contexts)
		// or the “factory default” value (in Default contexts)
		BOOL	deleteOK = [self.prefsMgr deleteDataForPreferenceTag:self.preferencesTag];
		
		
		if (NO == deleteOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to remove string-value preference",
							self.preferencesTag);
		}
	}
	else
	{
		BOOL					saveOK = NO;
		Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
		
		
		if (Preferences_ContextIsValid(targetContext))
		{
			CFStringRef			asCFString = BRIDGE_CAST(aString, CFStringRef);
			Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																			sizeof(asCFString), &asCFString);
			
			
			if (kPreferences_ResultOK == prefsResult)
			{
				saveOK = YES;
			}
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to save string-value preference",
							self.preferencesTag);
		}
	}
	
	[self didSetPreferenceValue];
}// setStringValue:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = NO;
	
	
	UNUSED_RETURN(NSString*)[self readValueSeeIfDefault:&result];
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self setStringValue:nil];
}// setNilPreferenceValue


@end // PreferenceValue_String


#pragma mark -
@implementation PreferenceValue_StringByJoiningArray


/*!
Designated initializer.

(4.1)
*/
- (instancetype)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
characterSetForSplitting:(NSCharacterSet*)		aCharacterSet
stringForJoiningElements:(NSString*)			aJoiningCharacter
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
		_characterSetForSplitting = aCharacterSet;
		_stringForJoiningElements = aJoiningCharacter;
	}
	return self;
}// initWithPreferencesTag:contextManager:


#pragma mark New Methods


/*!
Returns the preference’s current value, and indicates whether or
not that value was inherited from a parent context.

(4.1)
*/
- (NSString*)
readValueSeeIfDefault:(BOOL*)	outIsDefault
{
	NSString*				result = @"";
	Boolean					isDefault = false;
	Preferences_ContextRef	sourceContext = self.prefsMgr.currentContext;
	
	
	if (Preferences_ContextIsValid(sourceContext))
	{
		CFArrayRef			arrayValue = nullptr;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, self.preferencesTag,
																		sizeof(arrayValue), &arrayValue,
																		true/* search defaults */, &isDefault);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			result = [BRIDGE_CAST(arrayValue, NSArray*) componentsJoinedByString:self.stringForJoiningElements];
		}
	}
	
	if (nullptr != outIsDefault)
	{
		*outIsDefault = (true == isDefault);
	}
	
	return result;
}// readValueSeeIfDefault:


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSString*)
stringValue
{
	BOOL		isDefault = NO;
	NSString*	result = [self readValueSeeIfDefault:&isDefault];
	
	
	return result;
}
- (void)
setStringValue:(NSString*)	aString
{
	[self willSetPreferenceValue];
	
	if (nil == aString)
	{
		// when given nothing and the context is non-Default, delete the setting;
		// this will revert to either the Default value (in non-Default contexts)
		// or the “factory default” value (in Default contexts)
		BOOL	deleteOK = [self.prefsMgr deleteDataForPreferenceTag:self.preferencesTag];
		
		
		if (NO == deleteOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to remove array-from-string-value preference",
							self.preferencesTag);
		}
	}
	else
	{
		BOOL					saveOK = NO;
		Preferences_ContextRef	targetContext = self.prefsMgr.currentContext;
		
		
		if (Preferences_ContextIsValid(targetContext))
		{
			NSArray*			arrayValue = [aString componentsSeparatedByCharactersInSet:self.characterSetForSplitting];
			CFArrayRef			asCFArray = BRIDGE_CAST(arrayValue, CFArrayRef);
			Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, self.preferencesTag,
																			sizeof(asCFArray), &asCFArray);
			
			
			if (kPreferences_ResultOK == prefsResult)
			{
				saveOK = YES;
			}
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to save array-from-string-value preference",
							self.preferencesTag);
		}
	}
	
	[self didSetPreferenceValue];
}// setStringValue:


#pragma mark PreferenceValue_Inherited


/*!
Accessor.

(4.1)
*/
- (BOOL)
isInherited
{
	// if the current value comes from a default then the “inherited” state is YES
	BOOL	result = NO;
	
	
	UNUSED_RETURN(NSString*)[self readValueSeeIfDefault:&result];
	
	return result;
}// isInherited


/*!
Accessor.

(4.1)
*/
- (void)
setNilPreferenceValue
{
	[self setStringValue:nil];
}// setNilPreferenceValue


@end // PreferenceValue_StringByJoiningArray

// BELOW IS REQUIRED NEWLINE TO END FILE
