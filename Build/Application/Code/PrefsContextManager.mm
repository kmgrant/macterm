/*!	\file PrefsContextManager.mm
	\brief An Objective-C object for common preferences tasks.
	
	User interface objects that manage preferenes (such
	as panels) should allocate one of these objects to
	greatly simplify reading and changing values.
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

#import "PrefsContextManager.objc++.h"
#import <UniversalDefines.h>

// Mac includes
#import <Cocoa/Cocoa.h>



#pragma mark Public Methods

@implementation PrefsContextManager_Object


/*!
Initializes the object with no current context.

(4.1)
*/
- (id)
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
- (id)
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
	[super dealloc];
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
		if (Preferences_ContextIsValid(self->currentContext))
		{
			Preferences_ReleaseContext(&self->currentContext);
		}
		
		self->currentContext = aContext;
		
		if (Preferences_ContextIsValid(self->currentContext))
		{
			Preferences_RetainContext(self->currentContext);
		}
	}
}// setCurrentContext:


#pragma mark New Methods


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
		size_t		actualSize = 0;
		
		
		if (kPreferences_ResultOK ==
			Preferences_ContextGetData(self->currentContext, aTag, sizeof(flagValue), &flagValue, false/* search defaults */,
										&actualSize, &isDefault))
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
