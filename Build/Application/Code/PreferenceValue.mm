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

#import "PreferenceValue.objc++.h"
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>



#pragma mark Internal Methods

@implementation PreferenceValue_Inherited


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super init];
	if (nil != self)
	{
		self->prefsMgr = [aContextMgr retain];
		self->preferencesTag = aTag;
	}
	return self;
}// initWithPreferencesTag:contextManager:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[prefsMgr release];
	[super dealloc];
}// dealloc


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
- (Preferences_Tag)
preferencesTag
{
	return preferencesTag;
}// preferencesTag


/*!
Accessor.

(4.1)
*/
- (PrefsContextManager_Object*)
prefsMgr
{
	return prefsMgr;
}// prefsMgr


@end // PreferenceValue_Inherited


@implementation PreferenceValue_Color


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
	}
	return self;
}// initWithPreferencesTag:contextManager:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	[super dealloc];
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (NSColor*)
colorValue
{
	BOOL		isDefault = NO;
	NSColor*	result = [[self prefsMgr] readColorForPreferenceTag:[self preferencesTag] isDefault:&isDefault];
	
	
	return result;
}
- (void)
setColorValue:(NSColor*)	aColor
{
	[self willSetPreferenceValue];
	
	BOOL	saveOK = [[self prefsMgr] writeColor:aColor forPreferenceTag:[self preferencesTag]];
	
	
	if (NO == saveOK)
	{
		Console_Warning(Console_WriteLine, "failed to save a color preference");
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
	
	
	(NSColor*)[[self prefsMgr] readColorForPreferenceTag:[self preferencesTag] isDefault:&result];
	
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


@implementation PreferenceValue_Flag


/*!
Common initializer; assumes no inversion.

(4.1)
*/
- (id)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
{
	return [self initWithPreferencesTag:aTag contextManager:aContextMgr inverted:NO];
}// initWithPreferencesTag:contextManager:


/*!
Designated initializer.

(4.1)
*/
- (id)
initWithPreferencesTag:(Preferences_Tag)		aTag
contextManager:(PrefsContextManager_Object*)	aContextMgr
inverted:(BOOL)									anInversionFlag
{
	self = [super initWithPreferencesTag:aTag contextManager:aContextMgr];
	if (nil != self)
	{
		self->inverted = anInversionFlag;
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
	Preferences_ContextRef	sourceContext = [[self prefsMgr] currentContext];
	
	
	if (Preferences_ContextIsValid(sourceContext))
	{
		Boolean				flagValue = false;
		size_t				actualSize = 0;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, [self preferencesTag],
																		sizeof(flagValue), &flagValue,
																		true/* search defaults */, &actualSize,
																		&isDefault);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			if (self->inverted)
			{
				flagValue = (! flagValue);
			}
			result = (flagValue) ? YES : NO;
		}
	}
	
	if (nullptr != outIsDefault)
	{
		*outIsDefault = (YES == isDefault);
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
	NSNumber*	result = [NSNumber numberWithBool:[self readValueSeeIfDefault:&isDefault]];
	
	
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
		BOOL	deleteOK = [[self prefsMgr] deleteDataForPreferenceTag:[self preferencesTag]];
		
		
		if (NO == deleteOK)
		{
			Console_Warning(Console_WriteLine, "failed to remove flag-value preference");
		}
	}
	else
	{
		BOOL					saveOK = NO;
		Preferences_ContextRef	targetContext = [[self prefsMgr] currentContext];
		
		
		if (Preferences_ContextIsValid(targetContext))
		{
			Boolean				asBoolean = (self->inverted)
											? (NO == [aFlag boolValue])
											: (YES == [aFlag boolValue]);
			Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, [self preferencesTag],
																			sizeof(asBoolean), &asBoolean);
			
			
			if (kPreferences_ResultOK == prefsResult)
			{
				saveOK = YES;
			}
		}
		
		if (NO == saveOK)
		{
			Console_Warning(Console_WriteLine, "failed to save flag-value preference");
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
	
	
	(BOOL)[self readValueSeeIfDefault:&result];
	
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


@implementation PreferenceValue_String


/*!
Designated initializer.

(4.1)
*/
- (id)
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
	Preferences_ContextRef	sourceContext = [[self prefsMgr] currentContext];
	
	
	if (Preferences_ContextIsValid(sourceContext))
	{
		CFStringRef			stringValue = nullptr;
		size_t				actualSize = 0;
		Preferences_Result	prefsResult = Preferences_ContextGetData(sourceContext, [self preferencesTag],
																		sizeof(stringValue), &stringValue,
																		true/* search defaults */, &actualSize,
																		&isDefault);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			result = BRIDGE_CAST(stringValue, NSString*);
			[result autorelease];
		}
	}
	
	if (nullptr != outIsDefault)
	{
		*outIsDefault = (YES == isDefault);
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
	BOOL					saveOK = NO;
	Preferences_ContextRef	targetContext = [[self prefsMgr] currentContext];
	
	
	if (Preferences_ContextIsValid(targetContext))
	{
		[self willSetPreferenceValue];
		
		CFStringRef			asCFString = BRIDGE_CAST(aString, CFStringRef);
		Preferences_Result	prefsResult = Preferences_ContextSetData(targetContext, [self preferencesTag],
																		sizeof(asCFString), &asCFString);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			saveOK = YES;
		}
		
		[self didSetPreferenceValue];
	}
	
	if (NO == saveOK)
	{
		Console_Warning(Console_WriteLine, "failed to save string-value preference");
	}
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
	
	
	(NSString*)[self readValueSeeIfDefault:&result];
	
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

// BELOW IS REQUIRED NEWLINE TO END FILE
