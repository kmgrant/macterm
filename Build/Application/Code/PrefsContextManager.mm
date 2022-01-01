/*!	\file PrefsContextManager.mm
	\brief An Objective-C object for common preferences tasks.
	
	User interface objects that manage preferenes (such
	as panels) should allocate one of these objects to
	greatly simplify reading and changing values.
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

#import "PrefsContextManager.objc++.h"
#import <UniversalDefines.h>

// Mac includes
@import Cocoa;

// library includes
#import <CocoaExtensions.objc++.h>



#pragma mark Constants

NSString*	kPrefsContextManager_ContextWillChangeNotification =
				@"kPrefsContextManager_ContextWillChangeNotification";
NSString*	kPrefsContextManager_ContextDidChangeNotification =
				@"kPrefsContextManager_ContextDidChangeNotification";

#pragma mark Public Methods

#pragma mark -
@implementation PrefsContextManager_Object


/*!
Initializes the object with no current context.

(4.1)
*/
- (instancetype)
init
{
	self = [super init];
	if (nil != self)
	{
		self->currentContext = nullptr;
	}
	return self;
}// init


/*!
Designated initializer.

Locates the default context for the specified preferences class
and makes that the current context.  In other words, the new
object manages global defaults.

Although "init" can be called to create the object with no valid
context, "initWithDefaultContextInClass:" fails to initialize if
there is any problem finding the default context of the given
class.

(4.1)
*/
- (instancetype)
initWithDefaultContextInClass:(Quills::Prefs::Class)	aClass
{
	self = [super init];
	if (nil != self)
	{
		Preferences_Result	prefsResult = Preferences_GetDefaultContext(&self->currentContext, aClass);
		
		
		if ((kPreferences_ResultOK != prefsResult) ||
			(false == Preferences_ContextIsValid(self->currentContext)))
		{
			Console_Warning(Console_WriteLine, "unable to find the default context for the given preferences class!");
			self = nil;
		}
		else
		{
			Preferences_RetainContext(self->currentContext);
		}
	}
	return self;
}// initWithDefaultContextInClass:


/*!
Destructor.

(4.1)
*/
- (void)
dealloc
{
	if (Preferences_ContextIsValid(self->currentContext))
	{
		Preferences_ReleaseContext(&self->currentContext);
	}
}// dealloc


#pragma mark Accessors


/*!
Accessor.

(4.1)
*/
- (Preferences_ContextRef)
currentContext
{
	return self->currentContext;
}
- (void)
setCurrentContext:(Preferences_ContextRef)	aContext
{
	if (aContext != self->currentContext)
	{
		[self postNote:kPrefsContextManager_ContextWillChangeNotification];
		
		if (Preferences_ContextIsValid(self->currentContext))
		{
			Preferences_ReleaseContext(&self->currentContext);
		}
		
		self->currentContext = aContext;
		
		if (Preferences_ContextIsValid(self->currentContext))
		{
			Preferences_RetainContext(self->currentContext);
		}
		
		[self postNote:kPrefsContextManager_ContextDidChangeNotification];
	}
}// setCurrentContext:


#pragma mark New Methods


/*!
Removes any value for the specified tag from the current context by
calling Preferences_ContextDeleteData() and returns YES only if the
deletion succeeds.  This change becomes permanent upon the next save.

(4.1)
*/
- (BOOL)
deleteDataForPreferenceTag:(Preferences_Tag)	aTag
{
	BOOL	result = NO;
	
	
	if (Preferences_ContextIsValid(self->currentContext))
	{
		Preferences_Result	prefsResult = Preferences_ContextDeleteData(self->currentContext, aTag);
		
		
		result = (kPreferences_ResultOK == prefsResult);
	}
	return result;
}// deleteDataForPreferenceTag:


/*!
Reads the specified color data from the current preferences context.
If the value is not found, the Default color value is returned instead.
When the return value was read from the Default context (NOT simply if
the value happens to match the Default value), "outIsDefault" is filled
in with YES; otherwise it is NO.

IMPORTANT:	Only tags with values of type RGBColor should be given!
			The given NSColor is automatically converted internally.

(4.1)
*/
- (NSColor*)
readColorForPreferenceTag:(Preferences_Tag)		aTag
isDefault:(BOOL*)								outIsDefault
{
	NSColor*	result = [NSColor blackColor];
	
	
	if (Preferences_ContextIsValid(self->currentContext))
	{
		CGFloatRGBColor		colorDataFloat;
		Boolean				isDefault = false;
		Preferences_Result	prefsResult = Preferences_ContextGetData(self->currentContext, aTag,
																		sizeof(colorDataFloat), &colorDataFloat,
																		true/* search defaults */, &isDefault);
		
		
		if (nullptr != outIsDefault)
		{
			*outIsDefault = (true == isDefault);
		}
		
		if (kPreferences_ResultOK == prefsResult)
		{
			result = [NSColor colorWithCalibratedRed:colorDataFloat.red green:colorDataFloat.green
														blue:colorDataFloat.blue alpha:1.0];
		}
	}
	
	return result;
}// readColorForPreferenceTag:isDefault:


/*!
Reads a true/false value from the current preferences context.
If the value is not found, the specified default is returned
instead.

IMPORTANT:	Only tags with values of type Boolean should be given!

(4.1)
*/
- (BOOL)
readFlagForPreferenceTag:(Preferences_Tag)	aTag
defaultValue:(BOOL)							aDefault
{
	BOOL	result = aDefault;
	
	
	if (Preferences_ContextIsValid(self->currentContext))
	{
		Boolean		flagValue = false;
		Boolean		isDefault = false;
		
		
		if (kPreferences_ResultOK ==
			Preferences_ContextGetData(self->currentContext, aTag, sizeof(flagValue), &flagValue,
										false/* search defaults */, &isDefault))
		{
			result = (flagValue) ? YES : NO;
		}
		else
		{
			result = aDefault; // assume a default, if preference can’t be found
		}
	}
	return result;
}// readFlagForPreferenceTag:defaultValue:


/*!
Writes a new user preference for the specified color to the current
preferences context and returns true only if this succeeds.

IMPORTANT:	Only tags with values of type CGFloatRGBColor should be given!
			The given NSColor is automatically converted internally.

(4.1)
*/
- (BOOL)
writeColor:(NSColor*)				aValue
forPreferenceTag:(Preferences_Tag)	aTag
{
	NSColor*	newColor = [aValue colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
	BOOL		result = NO;
	
	
	if (nil == newColor)
	{
		// when given nothing and the context is non-Default, delete the setting;
		// this will revert to either the Default value (in non-Default contexts)
		// or the “factory default” value (in Default contexts)
		result = [self deleteDataForPreferenceTag:aTag];
	}
	else
	{
		CGFloatRGBColor		newColorFloat;
		CGFloat				ignoredAlpha = 0;
		
		
		[newColor getRed:&newColorFloat.red green:&newColorFloat.green blue:&newColorFloat.blue alpha:&ignoredAlpha];
		
		if (Preferences_ContextIsValid(self->currentContext))
		{
			Preferences_Result	prefsResult = Preferences_ContextSetData(self->currentContext, aTag,
																			sizeof(newColorFloat), &newColorFloat);
			
			
			result = (kPreferences_ResultOK == prefsResult);
		}
	}
	
	return result;
}// writeColor:forPreferenceTag:


/*!
Writes a true/false value to the current preferences context and
returns true only if this succeeds.

IMPORTANT:	Only tags with values of type Boolean should be given!

(4.1)
*/
- (BOOL)
writeFlag:(BOOL)					aFlag
forPreferenceTag:(Preferences_Tag)	aTag
{
	BOOL	result = NO;
	
	
	if (Preferences_ContextIsValid(self->currentContext))
	{
		Boolean				flagValue = (YES == aFlag) ? true : false;
		Preferences_Result	prefsResult = Preferences_ContextSetData(self->currentContext, aTag,
																		sizeof(flagValue), &flagValue);
		
		
		if (kPreferences_ResultOK == prefsResult)
		{
			result = YES;
		}
	}
	return result;
}// writeFlag:forPreferenceTag:


@end // PrefsContextManager_Object

// BELOW IS REQUIRED NEWLINE TO END FILE
