/*!	\file OtherApps.mm
	\brief Lists methods that interact with other programs.
*/
/*###############################################################

	MacTerm
		© 1998-2016 by Kevin Grant.
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

#import "OtherApps.h"
#import <UniversalDefines.h>

// library includes
#import <CFRetainRelease.h>

// application includes
#import "Preferences.h"



#pragma mark Internal Method Prototypes
namespace {

bool	importITermColorProperty		(id, Preferences_ContextRef, Preferences_Tag);

} // anonymous namespace



#pragma mark Public Methods

/*!
Translates all supported key-value pairs from the
given dictionary into equivalent preferences.  The
data should come from an ".itermcolors" file.

If the specified preferences context is nullptr, a
new “Format” collection is automatically created in
the user’s set of saved collections, with a unique
name.  If "inNameHintOrNull" is not nullptr, the
collection with this name will be created or updated.

See OtherApps_NewPropertyListFromFile().

\retval kOtherApps_ResultOK
if at least one key can be used (log may show warnings)

\retval kOtherApps_ResultGenericFailure
if nothing in the data set can be used

(2016.09)
*/
OtherApps_Result
OtherApps_ImportITermColors		(CFDictionaryRef				inXMLBasedDictionary,
								 Preferences_ContextRef		inoutFormatToModifyOrNull,
								 CFStringRef					inNameHintOrNull)
{
	OtherApps_Result	result = kOtherApps_ResultGenericFailure;
	
	
	if ((nullptr != inNameHintOrNull) && Preferences_IsContextNameInUse(Quills::Prefs::FORMAT, inNameHintOrNull))
	{
		// the call to Preferences_NewContextFromFavorites() below may
		// create a new collection OR return an existing one; issue a
		// warning if an overwrite has occurred
		Console_Warning(Console_WriteValueCFString, "import of '.itermcolors' will update existing collection", inNameHintOrNull);
	}
	
	if (nullptr != inXMLBasedDictionary)
	{
		NSDictionary*				asNSDict = BRIDGE_CAST(inXMLBasedDictionary, NSDictionary*);
		Boolean const				kIsNew = (nullptr == inoutFormatToModifyOrNull);
		Preferences_ContextWrap		newContext(((kIsNew)
												? Preferences_NewContextFromFavorites
													(Quills::Prefs::FORMAT, inNameHintOrNull)
												: nullptr),
												Preferences_ContextWrap::kAlreadyRetained);
		Preferences_ContextRef		targetContext = ((kIsNew)
														? newContext.returnRef()
														: inoutFormatToModifyOrNull);
		NSUInteger					countValid = 0;
		NSUInteger					countSkipped = 0;
		
		
		// the following keys match the ".itermcolors" file format
		for (NSString* key in asNSDict)
		{
			id		propertyValue = [asNSDict objectForKey:key];
			
			
			if ([key isEqualToString:@"Ansi 0 Color"] &&
				importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIBlack))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 1 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIRed))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 2 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIGreen))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 3 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIYellow))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 4 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIBlue))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 5 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIMagenta))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 6 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSICyan))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 7 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIWhite))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 8 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIBlackBold))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 9 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIRedBold))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 10 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIGreenBold))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 11 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIYellowBold))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 12 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIBlueBold))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 13 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIMagentaBold))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 14 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSICyanBold))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Ansi 15 Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorANSIWhiteBold))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Background Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorNormalBackground) &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorBoldBackground) &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorMatteBackground))
			{
				Console_Warning(Console_WriteValueCFString, "bold background color implicitly copied from '.itermcolors' key", BRIDGE_CAST(key, CFStringRef));
				Console_Warning(Console_WriteValueCFString, "matte background color implicitly copied from '.itermcolors' key", BRIDGE_CAST(key, CFStringRef));
				++countValid;
			}
			else if ([key isEqualToString:@"Bold Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorBoldForeground))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Cursor Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorCursorBackground))
			{
				// if the cursor color is customized, also disable the “automatic cursor” option
				Boolean const	kFlag = false;
				UNUSED_RETURN(Preferences_Result)Preferences_ContextSetData(targetContext, kPreferences_TagAutoSetCursorColor, sizeof(kFlag), &kFlag);
				
				++countValid;
			}
			else if ([key isEqualToString:@"Foreground Color"] &&
						importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorNormalForeground))
			{
				++countValid;
			}
			else if ([key isEqualToString:@"Selected Text Color"])
			{
				Console_Warning(Console_WriteValueCFString, "blinking foreground color implicitly copied from '.itermcolors' key", BRIDGE_CAST(key, CFStringRef));
				if (importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorBlinkingForeground))
				{
					++countValid;
				}
			}
			else if ([key isEqualToString:@"Selection Color"])
			{
				Console_Warning(Console_WriteValueCFString, "blinking background color implicitly copied from '.itermcolors' key", BRIDGE_CAST(key, CFStringRef));
				if (importITermColorProperty(propertyValue, targetContext, kPreferences_TagTerminalColorBlinkingBackground))
				{
					++countValid;
				}
			}
			else if ([key isEqualToString:@"Cursor Text Color"])
			{
				// setting does not translate to anything that MacTerm supports
				Console_Warning(Console_WriteValueCFString, "ignoring unsupported '.itermcolors' key", BRIDGE_CAST(key, CFStringRef));
				++countSkipped;
			}
			else
			{
				// string does not match any known key
				Console_Warning(Console_WriteValueCFString, "ignoring unrecognized '.itermcolors' key", BRIDGE_CAST(key, CFStringRef));
				++countSkipped;
			}
		}
		
		// if at least one valid entry is found, consider the
		// import to be successful (other entries may have
		// been skipped though)
		if (countValid > 0)
		{
			result = kOtherApps_ResultOK;
		}
		
		if (countSkipped > 0)
		{
			Console_Warning(Console_WriteValue, "total number of keys skipped", countSkipped);
		}
	}
	
	return result;
}// ImportITermColors


/*!
Tries to read the specified file as a property list format.
Returns nullptr if the import fails for any reason.

The root data type is likely to be a dictionary, though this
is technically not guaranteed; use CFGetTypeID() on the result
and compare with CFDictionaryGetTypeID() to verify.

If "outErrorOrNull" is not nullptr, any error produced from
the construction attempt is returned and the caller is
responsible for calling CFRelease() on it.  Otherwise, any
error object is parsed by this function, sent to the console
and then released.

(2016.09)
*/
CFPropertyListRef
OtherApps_NewPropertyListFromFile		(CFURLRef		inFile,
										 CFErrorRef*		outErrorOrNull)
{
	CFPropertyListRef   	result = nullptr;
	CFRetainRelease		streamObject(CFReadStreamCreateWithFile(kCFAllocatorDefault, inFile),
										CFRetainRelease::kAlreadyRetained);
	
	
	if (streamObject.exists() && CFReadStreamOpen(streamObject.returnCFReadStreamRef()))
	{
		CFPropertyListFormat		actualFormat = kCFPropertyListXMLFormat_v1_0;
		CFErrorRef				errorCFObject = nullptr;
		
		
		result = CFPropertyListCreateWithStream
					(kCFAllocatorDefault, streamObject.returnCFReadStreamRef(),
						0/* length or 0 for all */, kCFPropertyListImmutable,
						&actualFormat, &errorCFObject);
		if (nullptr != errorCFObject)
		{
			CFRetainRelease		errorDescription(CFErrorCopyDescription(errorCFObject),
													CFRetainRelease::kAlreadyRetained);
			
			
			if (nullptr != outErrorOrNull)
			{
				// error object ownership transfers to caller
				*outErrorOrNull = errorCFObject;
			}
			else
			{
				// error object is still owned by this function
				Console_Warning(Console_WriteValueCFString,
								"failed to open property list", CFURLGetString(inFile));
				Console_Warning(Console_WriteValueCFString,
								"error", errorDescription.returnCFStringRef());
				CFRelease(errorCFObject), errorCFObject = nullptr;
			}
		}
		
		CFReadStreamClose(streamObject.returnCFReadStreamRef());
	}
	
	return result;
}// NewPropertyListFromFile


#pragma mark Internal Methods
namespace {

/*!
Attempts to translate the specified property value
into a preference setting of the specified type.
Returns true only if successful.

Currently, "aPropertyListType" should always be
a dictionary with 3 keys: "Red Component", "Green
Component" and "Blue Component".

(2016.09)
*/
bool
importITermColorProperty		(id							aPropertyListType,
							 Preferences_ContextRef		aTargetContext,
							 Preferences_Tag				aTargetTag)
{
	bool	result = false;
	
	
	if ([aPropertyListType isKindOfClass:NSDictionary.class])
	{
		CGDeviceColor	colorValue;
		id				redNumber = [aPropertyListType objectForKey:@"Red Component"];
		id				greenNumber = [aPropertyListType objectForKey:@"Green Component"];
		id				blueNumber = [aPropertyListType objectForKey:@"Blue Component"];
		
		
		if ((nil != redNumber) && (nil != greenNumber) && (nil != blueNumber))
		{
			colorValue.red = [redNumber floatValue];
			colorValue.green = [greenNumber floatValue];
			colorValue.blue = [blueNumber floatValue];
			if ((colorValue.red >= 0) && (colorValue.green >= 0) && (colorValue.blue >= 0) &&
				(colorValue.red <= 1.0) && (colorValue.green <= 1.0) && (colorValue.blue <= 1.0))
			{
				Preferences_Result		prefsResult = Preferences_ContextSetData
														(aTargetContext, aTargetTag, sizeof(colorValue), &colorValue);
				
				
				if (kPreferences_ResultOK == prefsResult)
				{
					result = true;
				}
			}
		}
	}
	
	return result;
}// importITermColorProperty

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
