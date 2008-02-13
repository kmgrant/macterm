/*###############################################################

	GetPropertyAE.cp
	
	MacTelnet
		© 1998-2007 by Kevin Grant.
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

#include "UniversalDefines.h"

// standard-C includes
#include <cctype>
#include <cstring>

// standard-C++ includes
#include <string>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <RegionUtilities.h>
#include <StringUtilities.h>
#include <WindowInfo.h>

// resource includes
#include "StringResources.h"

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "BasicTypesAE.h"
#include "Clipboard.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "Folder.h"
#include "GenesisAE.h"
#include "GetElementAE.h"
#include "GetPropertyAE.h"
#include "MacroManager.h"
#include "Network.h"
#include "ObjectClassesAE.h"
#include "Preferences.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "Terminology.h"



#pragma mark Constants

typedef SInt16 PropertyAccessMechanism;
enum
{
	kPropertyAccessModify = -1,		//!< change the data for the property
	kPropertyAccessObtain = 0,		//!< acquire the data from the property
	kPropertyAccessSize = 1			//!< determine the size of the data from the property
};

#pragma mark Internal Method Prototypes

static OSErr	accessPropertyOfClassApplication			(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassApplicationPreferences	(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassClipboardWindow		(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassConnection				(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassDialogReply			(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassFormat					(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassMacro					(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassMacroSet				(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassProxyServer			(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassSessionWindow			(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassTerminalWindow			(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSErr	accessPropertyOfClassWindow					(ObjectClassesAE_TokenPtr, AEDesc*,
															 PropertyAccessMechanism);
static OSStatus getPropertyByID								(DescType, AEDesc const*, AEDesc*);
static OSErr	tokenAccess									(AEDesc const*, AEDesc*, PropertyAccessMechanism);



#pragma mark Public Methods

/*!
To acquire an Apple Event descriptor containing
data corresponding to the specified property of
the specified class, use this method.  The token
encodes all necessary information about the
desired property and the class having the
property.  You generally use this routine to
help handle a Òget dataÓ Apple Event.

The value of "inDesiredDataType" is used to
coerce the resultant data into a form that the
user wants.  For example, it might be possible
to convert the actual property into some other
kind of object - the ÒotherÓ type would be
"inDesiredDataType".

(3.0)
*/
OSErr
GetPropertyAE_DataFromClassProperty		(AEDesc const*		inTokenDescriptor,
										 DescType const		inDesiredDataType,
										 AEDesc*			outDataDescriptor,
										 Boolean*			outUsedDesiredDataType)
{
	OSErr	result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("Obtaining data...");
	
	result = tokenAccess(inTokenDescriptor, outDataDescriptor, kPropertyAccessObtain);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	
	return result;
}// DataFromClassProperty


/*!
To acquire an Apple Event descriptor containing
the size in bytes of data corresponding to the
specified property of the specified class, use
this method.  The token encodes all necessary
information about the desired property and the
class having the property.  You generally use
this routine to help handle a Òget data sizeÓ
Apple Event.

(3.0)
*/
OSErr
GetPropertyAE_DataSizeFromClassProperty		(AEDesc const*		inTokenDescriptor,
											 AEDesc*			outDataDescriptor,
											 Boolean*			outUsedDesiredDataType)
{
	OSErr   result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("Obtaining data size...");
	
	result = tokenAccess(inTokenDescriptor, outDataDescriptor, kPropertyAccessSize);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	
	return result;
}// DataSizeFromClassProperty


/*!
To use an Apple Event descriptor containing
data to modify the specified property of the
specified class, use this method.  The token
encodes all necessary information about the
desired property and the class having the
property.  You generally use this routine to
help handle a Òset dataÓ Apple Event.

(3.0)
*/
OSErr
GetPropertyAE_DataToClassProperty		(AEDesc const*		inTokenDescriptor,
										 AEDesc*			inDataDescriptor)
{
	OSErr   result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("Modifying data...");
	
	result = tokenAccess(inTokenDescriptor, inDataDescriptor, kPropertyAccessModify);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	
	return result;
}// DataToClassProperty


/*!
Handles the following OSL property access:

	typeProperty <- typeNull

(3.0)
*/
pascal OSErr
GetPropertyAE_FromNull	(DescType			inDesiredClass,
						 AEDesc const*		inFromWhat,
						 DescType			inFromWhatClass,
						 DescType			inFormOfReference,
						 AEDesc const*		inReference,
						 AEDesc*			outObjectOfDesiredClass,
						 long				UNUSED_ARGUMENT(inData))
{
	OSErr   result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: requested property of implicit container");
	Console_WriteValueFourChars("desired class", inDesiredClass);
	Console_WriteValueFourChars("origin class", inFromWhatClass);
	Console_WriteValueFourChars("form of reference", inFormOfReference);
	if (inDesiredClass != cProperty)
	{
		result = errAEWrongDataType;
	}
	else if (inFormOfReference != formPropertyID)
	{
		result = errAEBadTestKey;
	}
	
	// create the appropriate object
	if (result == noErr)
	{
		DescType	propertyID = typeWildCard;
		
		
		Console_WriteLine("using a property ID");
		result = AppleEventUtilities_CopyDescriptorDataAs(typeType, inReference, &propertyID, sizeof(propertyID),
															nullptr/* actual size storage */);
		if (result != noErr)
		{
			Console_WriteLine("could not obtain that property ID");
		}
		else
		{
			// implicit container property
			AEDesc		applicationTokenDesc;
			
			
			result = GetElementAE_ApplicationImplicit(&applicationTokenDesc);
			if (result == noErr)
			{
				//TMP//result = getPropertyByID(propertyID, &applicationTokenDesc, outObjectOfDesiredClass);
				Console_WriteLine("UNIMPLEMENTED");
			}
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// FromNull


/*!
Handles the following OSL property access:

	typeProperty <- ANYTHING

(3.0)
*/
pascal OSErr
GetPropertyAE_FromObject	(DescType			inDesiredClass,
							 AEDesc const*		inFromWhat,
							 DescType			inFromWhatClass,
							 DescType			inFormOfReference,
							 AEDesc const*		inReference,
							 AEDesc*			outObjectOfDesiredClass,
							 long				UNUSED_ARGUMENT(inData))
{
	OSErr   result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: requested property");
	Console_WriteValueFourChars("desired class", inDesiredClass);
	Console_WriteValueFourChars("origin class", inFromWhatClass);
	Console_WriteValueFourChars("form of reference", inFormOfReference);
	if (inDesiredClass != cProperty)
	{
		result = errAEWrongDataType;
	}
	//else if (inFromWhatClass != cProperty)
	//{
	//	result = errAENoSuchObject;
	//}
	else if (inFormOfReference != formPropertyID)
	{
		result = errAEBadTestKey;
	}
	
	// create the appropriate object
	if (result == noErr)
	{
		DescType	propertyID = typeWildCard;
		
		
		Console_WriteLine("using a property ID");
		result = AppleEventUtilities_CopyDescriptorDataAs(typeType, inReference, &propertyID, sizeof(propertyID),
															nullptr/* actual size storage */);
		if (result != noErr)
		{
			Console_WriteLine("could not obtain that property ID");
		}
		else
		{
			// explicit container property
			result = getPropertyByID(propertyID, inFromWhat, outObjectOfDesiredClass);
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// FromObject


#pragma mark Internal Methods

/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyApplication".

(3.0)
*/
static OSErr
accessPropertyOfClassApplication	(ObjectClassesAE_TokenPtr   inTokenPtr,
									 AEDesc*					inoutDataDescriptor,
									 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr						result = noErr;
	ObjectClassesAE_PropertyPtr	propertyPtr = &inTokenPtr->as.property;
	
	
	Console_BeginFunction();
	Console_WriteValueFourChars("AppleScript: application property; class type via", propertyPtr->container.eventClass);
	
	// verify that itÕs a property, and verify its container class type
	if (result == noErr)
	{
		result = ((inTokenPtr->flags & kObjectClassesAE_TokenFlagsIsProperty) &&
					(GenesisAE_FirstClassIs(propertyPtr->container.eventClass, cMyApplication) ||
					(propertyPtr->container.eventClass == typeNull))) ? noErr
				: errAEEventNotHandled;
	}
	
	// get the data
	if (result == noErr)
	{
		switch (propertyPtr->dataType)
		{
		case pBestType:
		case pDefaultType:
			Console_WriteLine("default or best type");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				AEDesc		nullDesc;
				
				
				(OSErr)AppleEventUtilities_InitAEDesc(&nullDesc);
				result = AEDuplicateDesc(&nullDesc, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pClass:
			Console_WriteLine("the class");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				result = BasicTypesAE_CreateClassIDDesc(cApplication, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyActiveMacroSet:
			Console_WriteLine("active macro set");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				Console_WriteLine("read");
				// UNIMPLEMENTED
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				Console_WriteLine("write");
				// UNIMPLEMENTED
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyClipboard:
			Console_WriteLine("clipboard");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				// UNIMPLEMENTED
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyFolderScriptsMenuItems:
			Console_WriteLine("user Scripts Menu Items folder");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				FSRef	folder;
				
				
				result = Folder_GetFSRef(kFolder_RefScriptsMenuItems, folder);
				if (result == noErr) BasicTypesAE_CreateFileOrFolderDesc(folder, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyFolderStartupItems:
			Console_WriteLine("user Startup Items folder");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				FSRef	folder;
				
				
				result = Folder_GetFSRef(kFolder_RefStartupItems, folder);
				if (result == noErr) BasicTypesAE_CreateFileOrFolderDesc(folder, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyIPAddress:
			Console_WriteLine("current IP address");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				std::string		ipAddressString;
				int				addressType = AF_INET;
				
				
				if (Network_CurrentIPAddressToString(ipAddressString, addressType))
				{
					Str255		pString;
					
					
					StringUtilities_CToP(ipAddressString.c_str(), pString);
					result = BasicTypesAE_CreatePStringDesc(pString, inoutDataDescriptor);
				}
				else result = errAEEventNotHandled;
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyIsFrontmost:
			Console_WriteLine("frontmost");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				result = BasicTypesAE_CreateBooleanDesc(!FlagManager_Test(kFlagSuspended), inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyPreferences:
			Console_WriteLine("preferences");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				AEDesc			containerDesc, // never changes, in this case
								keyDesc;
				
				
				(OSErr)AppleEventUtilities_InitAEDesc(&containerDesc);
				(OSErr)AppleEventUtilities_InitAEDesc(&keyDesc);
				
				// send a request for "preferences", which resides in a null container
				(OSErr)BasicTypesAE_CreatePropertyIDDesc(pMyPreferences, &keyDesc);
				result = CreateObjSpecifier(cProperty, &containerDesc,
											formPropertyID, &keyDesc, true/* dispose inputs */,
											inoutDataDescriptor);
				if (result != noErr) Console_WriteValue("error creating object specifier", result);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyProgramName:
			Console_WriteLine("name");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				Str255					applicationName;
				ProcessInfoRec			processInfo;
				ProcessSerialNumber		currentProcess;
				
				
				applicationName[0] = 0;
				processInfo.processInfoLength = sizeof(processInfo);
				processInfo.processName = applicationName;
				processInfo.processAppSpec = nullptr;
				
				GetCurrentProcess(&currentProcess);
				GetProcessInformation(&currentProcess, &processInfo);
				result = BasicTypesAE_CreatePStringDesc(applicationName, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyProgramVersion:
			Console_WriteLine("version");
			// no longer supported (no 'vers' resource)
			result = errAEEventNotHandled;
			break;
		
		default:
			result = errAECantSupplyType;
			break;
		}
	}
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// accessPropertyOfClassApplication


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyApplicationPreferences".

(3.0)
*/
static OSErr
accessPropertyOfClassApplicationPreferences	(ObjectClassesAE_TokenPtr   inTokenPtr,
											 AEDesc*					inoutDataDescriptor,
											 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr						result = noErr;
	ObjectClassesAE_PropertyPtr	propertyPtr = &inTokenPtr->as.property;
	//Boolean					canModifyProperty = true;	// change this flag when appropriate to restrict access
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: application preferences property");
	
	// verify that itÕs a property, and verify its container class type
	if (result == noErr)
	{
		result = ((inTokenPtr->flags & kObjectClassesAE_TokenFlagsIsProperty) &&
					GenesisAE_FirstClassIs(propertyPtr->container.eventClass, cMyApplicationPreferences)) ? noErr
				: errAEEventNotHandled;
	}
	
	// get the data
	if (result == noErr)
	{
		switch (propertyPtr->dataType)
		{
		case pMyApplicationPreferencesCursorShape:
			Console_WriteLine("cursor shape");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(FourCharCode), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				FourCharCode				shape = kTelnetEnumeratedClassCursorShapeBlock;
				TerminalView_CursorType		terminalCursorType = kTerminalView_CursorTypeBlock;
				size_t						actualSize = 0L;
				
				
				if (Preferences_GetData(kPreferences_TagTerminalCursorType, sizeof(terminalCursorType),
										&terminalCursorType, &actualSize) == kPreferences_ResultOK)
				{
					switch (terminalCursorType)
					{
					case kTerminalView_CursorTypeVerticalLine:
						shape = kTelnetEnumeratedClassCursorShapeVerticalBar;
						break;
					
					case kTerminalView_CursorTypeThickVerticalLine:
						shape = kTelnetEnumeratedClassCursorShapeVerticalBarBold;
						break;
					
					case kTerminalView_CursorTypeUnderscore:
						shape = kTelnetEnumeratedClassCursorShapeUnderscore;
						break;
					
					case kTerminalView_CursorTypeThickUnderscore:
						shape = kTelnetEnumeratedClassCursorShapeUnderscoreBold;
						break;
					
					case kTerminalView_CursorTypeBlock:
					default:
						shape = kTelnetEnumeratedClassCursorShapeBlock;
						break;
					}
				}
				result = BasicTypesAE_CreateEnumerationDesc(shape, inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				FourCharCode	newShape = kTelnetEnumeratedClassCursorShapeBlock;
				
				
				// make the data match the type in the Dictionary
				result = AECoerceDesc(inoutDataDescriptor, typeEnumerated, inoutDataDescriptor);
				if (result == noErr)
				{
					Size		actualSize = 0L;
					
					
					result = AppleEventUtilities_CopyDescriptorData(inoutDataDescriptor, (Ptr)&newShape,
																	sizeof(newShape), &actualSize);
					if (result == noErr)
					{
						// update the cursor preference (translate from enumerated value)
						TerminalView_CursorType		terminalCursorType = kTerminalView_CursorTypeBlock;
						
						
						switch (newShape)
						{
						case kTelnetEnumeratedClassCursorShapeVerticalBar:
							terminalCursorType = kTerminalView_CursorTypeVerticalLine;
							break;
						
						case kTelnetEnumeratedClassCursorShapeVerticalBarBold:
							terminalCursorType = kTerminalView_CursorTypeThickVerticalLine;
							break;
						
						case kTelnetEnumeratedClassCursorShapeUnderscore:
							terminalCursorType = kTerminalView_CursorTypeUnderscore;
							break;
						
						case kTelnetEnumeratedClassCursorShapeUnderscoreBold:
							terminalCursorType = kTerminalView_CursorTypeThickUnderscore;
							break;
						
						case kTelnetEnumeratedClassCursorShapeBlock:
						default:
							terminalCursorType = kTerminalView_CursorTypeBlock;
							break;
						}
						Preferences_SetData(kPreferences_TagTerminalCursorType,
											sizeof(terminalCursorType), &terminalCursorType);
					}
				}
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyApplicationPreferencesSimplifiedUserInterface:
		case pMyApplicationPreferencesVisualBell:
		default:
			result = errAECantSupplyType;
			break;
		}
	}
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// accessPropertyOfClassApplicationPreferences


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyClipboardWindow".

(3.0)
*/
static OSErr
accessPropertyOfClassClipboardWindow	(ObjectClassesAE_TokenPtr   inTokenPtr,
										 AEDesc*					inoutDataDescriptor,
										 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr						result = noErr;
	ObjectClassesAE_PropertyPtr	propertyPtr = &inTokenPtr->as.property;
	//Boolean					canModifyProperty = true;	// change this flag when appropriate to restrict access
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: clipboard window property");
	
	// extract the data for the clipboard window to which this property belongs
	//nothingyet
	
	// make ABSOLUTELY SURE the clipboard window is not screwed up (otherwise, the system could crash)
	//nothingyet
	
	// verify that itÕs a property, and verify its container class type
	if (result == noErr)
	{
		result = ((inTokenPtr->flags & kObjectClassesAE_TokenFlagsIsProperty) &&
					GenesisAE_FirstClassIs(propertyPtr->container.eventClass, cMyClipboardWindow)) ? noErr
				: errAEEventNotHandled;
	}
	
	// get the data
	if (result == noErr)
	{
		switch (propertyPtr->dataType)
		{
		case pMyClipboardWindowContents:
			Console_WriteLine("contents");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				// no longer supported
				// UNIMPLEMENTED
				result = unimpErr;
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				result = Clipboard_CreateContentsAEDesc(inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				result = Clipboard_AEDescToScrap(inoutDataDescriptor);
			}
			break;
		
		default:
			// this class inherits from the window class; let *it* handle any other properties
			result = accessPropertyOfClassWindow(inTokenPtr, inoutDataDescriptor, inPropertyAccessMechanism);
			break;
		}
	}
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// accessPropertyOfClassClipboardWindow


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyConnection".

(3.0)
*/
static OSErr
accessPropertyOfClassConnection	(ObjectClassesAE_TokenPtr   inTokenPtr,
								 AEDesc*					inoutDataDescriptor,
								 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr   result = noErr;
	
	
	// unimplemented!
	return result;
}// accessPropertyOfClassConnection


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyDialogReply".

(3.0)
*/
static OSErr
accessPropertyOfClassDialogReply	(ObjectClassesAE_TokenPtr   inTokenPtr,
									 AEDesc*					inoutDataDescriptor,
									 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr   result = noErr;
	
	
	// unimplemented!
	return result;
}// accessPropertyOfClassDialogReply


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyFormat".

Unimplemented.

(3.0)
*/
static OSErr
accessPropertyOfClassFormat		(ObjectClassesAE_TokenPtr   inTokenPtr,
								 AEDesc*					inoutDataDescriptor,
								 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr							result = noErr;
	//ObjectClassesAE_PropertyPtr	propertyPtr = &inTokenPtr->as.property;
	//WindowRef						window = nullptr;
	//Boolean						canModifyProperty = true;	// change this flag when appropriate to restrict access
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: format property");
	
#if 0
	// extract the data for the terminal window to which this property belongs
	window = propertyPtr->container.data.format.window;
	
	// make ABSOLUTELY SURE the terminal window is not screwed up (otherwise, the system could crash)
	if (window == nullptr)
	{
		Console_WriteLine("WARNING: invalid format for get-format-property");
		result = errAEEventNotHandled;
	}
	
	// verify that itÕs a property, and verify its container class type
	if (result == noErr)
	{
		result = ((inTokenPtr->flags & kObjectClassesAE_TokenFlagsIsProperty) &&
					GenesisAE_FirstClassIs(propertyPtr->container.eventClass, cMyFormat)) ? noErr
				: errAEEventNotHandled;
	}
	
	// get the data
	if (result == noErr)
	{
		switch (propertyPtr->dataType)
		{
		case pMyFormatFontFamilyName:
			Console_WriteLine("font name");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(PLstrlen(realScreenPtr->text.font.familyName) *
														sizeof(UInt8), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				result = BasicTypesAE_CreatePStringDesc(realScreenPtr->text.font.familyName, inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				// unimplemented
				result = errAEEventNotHandled;
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyFormatFontSize:
			Console_WriteLine("font size");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(realScreenPtr->text.font.size), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				result = BasicTypesAE_CreateSInt16Desc(realScreenPtr->text.font.size, inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				// unimplemented
				result = errAEEventNotHandled;
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyFormatColorText:
		case pMyFormatColorBackground:
			Console_WriteLine("color attribute");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(RGBColor), inoutDataDescriptor);
			}
			else if ((inPropertyAccessMechanism == kPropertyAccessObtain) ||
						(inPropertyAccessMechanism == kPropertyAccessModify))
			{
				ConnectionDataPtr			connectionDataPtr = ScreenFactory_GetWindowConnectionData
																(realScreenPtr->window.ref);
				TerminalView_ColorIndex		colorIndex = kTerminalView_ColorIndexNormalText;
				RGBColor					color;
				
				
				switch (propertyPtr->dataType)
				{
				case pMyTerminalWindowFontColorBlinkingBackground:
					colorIndex = kTerminalView_ColorIndexBlinkingBackground;
					break;
				
				case pMyTerminalWindowFontColorBlinkingText:
					colorIndex = kTerminalView_ColorIndexBlinkingText;
					break;
				
				case pMyTerminalWindowFontColorBoldBackground:
					colorIndex = kTerminalView_ColorIndexBoldBackground;
					break;
				
				case pMyTerminalWindowFontColorBoldText:
					colorIndex = kTerminalView_ColorIndexBoldText;
					break;
				
				case pMyTerminalWindowFontColorBackground:
					colorIndex = kTerminalView_ColorIndexNormalBackground;
					break;
				
				case pMyTerminalWindowFontColorText:
				default:
					colorIndex = kTerminalView_ColorIndexNormalText;
					break;
				}
				if (inPropertyAccessMechanism == kPropertyAccessModify)
				{
					// make the data match the type in the Dictionary
					result = AECoerceDesc(inoutDataDescriptor, cRGBColor, inoutDataDescriptor);
					if (result == noErr)
					{
						Size	actualSize = 0L;
						
						
						result = AppleEventUtilities_CopyDescriptorData(inoutDataDescriptor, (Ptr)&color,
																		sizeof(color), &actualSize);
						if (result == noErr)
						{
							// set the color and force the window to update
							(Boolean)TerminalView_SetColor(connectionDataPtr->vs, colorIndex, &color);
							RegionUtilities_RedrawWindowOnNextUpdate(realScreenPtr->window.ref);
						}
					}
				}
				else
				{
					// return the color
					(Boolean)TerminalView_GetColor(connectionDataPtr->vs, colorIndex, &color);
					result = BasicTypesAE_CreateRGBColorDesc(&color, inoutDataDescriptor);
				}
			}
			else result = errAEEventNotHandled;
			break;
		
		default:
			result = errAECantSupplyType;
			break;
		}
	}
#endif
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// accessPropertyOfClassFormat


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyMacro".

(3.0)
*/
static OSErr
accessPropertyOfClassMacro		(ObjectClassesAE_TokenPtr   inTokenPtr,
								 AEDesc*					inoutDataDescriptor,
								 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr   result = noErr;
	
	
	return result;
}// accessPropertyOfClassMacro


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyMacroSet".

(3.0)
*/
static OSErr
accessPropertyOfClassMacroSet		(ObjectClassesAE_TokenPtr   inTokenPtr,
									 AEDesc*					inoutDataDescriptor,
									 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr						result = noErr;
	ObjectClassesAE_PropertyPtr	propertyPtr = &inTokenPtr->as.property;
	UInt16						macroSetNumber = 0;
	//Boolean					canModifyProperty = true;	// change this flag when appropriate to restrict access
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: macro set property");
	
	// access the macro set to which this property belongs
	macroSetNumber = propertyPtr->container.data.macroSet.number;
	
	// make ABSOLUTELY SURE the macro set is not screwed up (otherwise, the system could crash)
	if ((macroSetNumber < 1) || (macroSetNumber > MACRO_SET_COUNT))
	{
		Console_WriteLine("WARNING: invalid macro set for get-macro-set-property");
		result = errAEEventNotHandled;
	}
	
	// verify that itÕs a property, and verify its container class type
	if (result == noErr)
	{
		result = ((inTokenPtr->flags & kObjectClassesAE_TokenFlagsIsProperty) &&
					GenesisAE_FirstClassIs(propertyPtr->container.eventClass, cMyMacroSet)) ? noErr
				: errAEEventNotHandled;
	}
	
	// get the data
	if (result == noErr)
	{
		switch (propertyPtr->dataType)
		{
		case pMyMacroSetKeyEquivalents:
			Console_WriteLine("key equivalents");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(FourCharCode), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				FourCharCode	keys = kTelnetEnumeratedClassMacroSetKeyEquivalentsCommandDigit;
				UInt16			oldActiveSetNumber = Macros_ReturnActiveSetNumber();
				
				
				Macros_SetActiveSetNumber(macroSetNumber);
				if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
				{
					// if the macro set uses function keys instead, change the default
					keys = kTelnetEnumeratedClassMacroSetKeyEquivalentsFunctionKeys;
				}
				Macros_SetActiveSetNumber(oldActiveSetNumber);
				result = BasicTypesAE_CreateEnumerationDesc(keys, inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				FourCharCode	newKeys = kTelnetEnumeratedClassMacroSetKeyEquivalentsCommandDigit;
				
				
				// make the data match the type in the Dictionary
				result = AECoerceDesc(inoutDataDescriptor, typeEnumerated, inoutDataDescriptor);
				if (result == noErr)
				{
					Size		actualSize = 0L;
					
					
					result = AppleEventUtilities_CopyDescriptorData(inoutDataDescriptor, (Ptr)&newKeys,
																	sizeof(newKeys), &actualSize);
					if (result == noErr)
					{
						// update the macro setÕs key equivalents
						UInt16		oldActiveSetNumber = Macros_ReturnActiveSetNumber();
						
						
						Macros_SetActiveSetNumber(macroSetNumber);
						Macros_SetMode((newKeys == kTelnetEnumeratedClassMacroSetKeyEquivalentsFunctionKeys)
										? kMacroManager_InvocationMethodFunctionKeys : kMacroManager_InvocationMethodCommandDigit);
						// rebuild macro display if necessary to show new key equivalents - INCOMPLETE
						Macros_SetActiveSetNumber(oldActiveSetNumber);
					}
				}
			}
			else result = errAEEventNotHandled;
			break;
		
		default:
			result = errAECantSupplyType;
			break;
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// accessPropertyOfClassMacroSet


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyProxyServerConfiguration".

(3.0)
*/
static OSErr
accessPropertyOfClassProxyServer	(ObjectClassesAE_TokenPtr   inTokenPtr,
									 AEDesc*					inoutDataDescriptor,
									 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr   result = noErr;
	
	
	return result;
}// accessPropertyOfClassProxyServer


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMySessionWindow".

(3.0)
*/
static OSErr
accessPropertyOfClassSessionWindow	(ObjectClassesAE_TokenPtr   inTokenPtr,
									 AEDesc*					inoutDataDescriptor,
									 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr						result = noErr;
	ObjectClassesAE_PropertyPtr	propertyPtr = &inTokenPtr->as.property;
	SessionRef					session = nullptr;
	//Boolean					canModifyProperty = true;	// change this flag when appropriate to restrict access
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: session window property");
	
	// extract the data for the session to which this property belongs
	session = propertyPtr->container.data.sessionWindow.session;
	
	// make ABSOLUTELY SURE the session is not screwed up (otherwise, the system could crash)
	if (session == nullptr) // TMP - insufficient!
	{
		Console_WriteLine("WARNING: invalid session for get-session-property");
		result = errAEEventNotHandled;
	}
	
	// verify that itÕs a property, and verify its container class type
	if (result == noErr)
	{
		result = ((inTokenPtr->flags & kObjectClassesAE_TokenFlagsIsProperty) &&
					GenesisAE_FirstClassIs(propertyPtr->container.eventClass, cMySessionWindow)) ? noErr
				: errAEEventNotHandled;
	}
	
	// get the data
	if (result == noErr)
	{
		switch (propertyPtr->dataType)
		{
		case pMySessionWindowRemote:
			Console_WriteLine("remote");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				// UNIMPLEMENTED!!!
				result = BasicTypesAE_CreateBooleanDesc(false/* TEMPORARY */, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		default:
			// this class inherits from the terminal window class; let *it* handle any other properties
			result = accessPropertyOfClassTerminalWindow(inTokenPtr, inoutDataDescriptor, inPropertyAccessMechanism);
			break;
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// accessPropertyOfClassSessionWindow


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyTerminalWindow".

(3.0)
*/
static OSErr
accessPropertyOfClassTerminalWindow		(ObjectClassesAE_TokenPtr   inTokenPtr,
										 AEDesc*					inoutDataDescriptor,
										 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr						result = noErr;
	ObjectClassesAE_PropertyPtr	propertyPtr = &inTokenPtr->as.property;
	TerminalWindowRef			terminalWindow = nullptr;
	//Boolean					canModifyProperty = true;	// change this flag when appropriate to restrict access
	
	
	Console_BeginFunction();
	Console_WriteValueFourChars("AppleScript: terminal window property; class type via", propertyPtr->container.eventClass);
	
	// extract the data for the terminal window to which this property belongs
	terminalWindow = propertyPtr->container.data.terminalWindow.ref;
	
	// make ABSOLUTELY SURE the terminal window is not screwed up (otherwise, the system could crash)
	if (terminalWindow == nullptr)
	{
		Console_WriteLine("WARNING: invalid terminal window for get-terminal-window-property");
		result = errAEEventNotHandled;
	}
	
	// verify that itÕs a property, and verify its container class type
	if (result == noErr)
	{
		result = ((inTokenPtr->flags & kObjectClassesAE_TokenFlagsIsProperty) &&
					GenesisAE_FirstClassIs(propertyPtr->container.eventClass, cMyTerminalWindow)) ? noErr
				: errAEEventNotHandled;
	}
	
	// get the data
	if (result == noErr)
	{
		switch (propertyPtr->dataType)
		{
		case pMyTerminalWindowColumns:
			Console_WriteLine("number of columns");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(UInt32), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				UInt16		columns = 0;
				
				
				TerminalWindow_GetScreenDimensions(terminalWindow, &columns, nullptr/* rows */);
				result = BasicTypesAE_CreateUInt32Desc(columns, inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				// make the data match the type in the Dictionary
				result = AECoerceDesc(inoutDataDescriptor, typeUInt32, inoutDataDescriptor);
				if (result == noErr)
				{
					Size		actualSize = 0L;
					UInt32		desiredColumns = 0L;
					
					
					// obtain the given column count and ensure it is in range
					result = AppleEventUtilities_CopyDescriptorData(inoutDataDescriptor, (Ptr)&desiredColumns,
																	sizeof(desiredColumns), &actualSize);
					if (result == noErr)
					{
						if ((desiredColumns >= 10) &&
							(desiredColumns <= Terminal_ReturnAllocatedColumnCount()))
						{
							// set the size and force the window to update
							UInt16		rows = 0;
							
							
							TerminalWindow_GetScreenDimensions(terminalWindow, nullptr/* columns */, &rows);
							TerminalWindow_SetScreenDimensions(terminalWindow, desiredColumns, rows, false/* record */);
						}
						else result = errAEEventNotHandled; // out of range
					}
				}
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyTerminalWindowRows:
			Console_WriteLine("number of rows");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(UInt32), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				UInt16		rows = 0;
				
				
				TerminalWindow_GetScreenDimensions(terminalWindow, nullptr/* columns */, &rows);
				result = BasicTypesAE_CreateUInt32Desc(rows, inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				// make the data match the type in the Dictionary
				result = AECoerceDesc(inoutDataDescriptor, typeUInt32, inoutDataDescriptor);
				if (result == noErr)
				{
					Size		actualSize = 0L;
					UInt32		desiredRows = 0L;
					
					
					// obtain the given column count and ensure it is in range
					result = AppleEventUtilities_CopyDescriptorData(inoutDataDescriptor, (Ptr)&desiredRows,
																	sizeof(desiredRows), &actualSize);
					if (result == noErr)
					{
						if ((desiredRows >= 10) && (desiredRows <= 200))
						{
							// set the size and force the window to update
							UInt16		columns = 0;
							
							
							TerminalWindow_GetScreenDimensions(terminalWindow, &columns, nullptr/* rows */);
							TerminalWindow_SetScreenDimensions(terminalWindow, columns, desiredRows, false/* record */);
						}
						else result = errAEEventNotHandled; // out of range
					}
				}
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyTerminalWindowScrollBack:
			Console_WriteLine("scrollback");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(UInt32), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				result = BasicTypesAE_CreateUInt32Desc(0, //TEMP!///realScreenPtr->screen.rowCountScrollback,
														inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyTerminalWindowNormalFormat:
		case pMyTerminalWindowBoldFormat:
		case pMyTerminalWindowBlinkingFormat:
			Console_WriteLine("text format");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				AEDesc			containerDesc,
								keyDesc,
								objectDesc;
				Str255			windowTitle;
				
				
				(OSErr)AppleEventUtilities_InitAEDesc(&containerDesc); // remains a null container
				(OSErr)AppleEventUtilities_InitAEDesc(&keyDesc);
				(OSErr)AppleEventUtilities_InitAEDesc(&objectDesc);
				
				// send a request for "window <name>", which resides in a null container;
				// then, request the appropriate format property
				GetWTitle(propertyPtr->container.data.terminalWindow.windowClass.ref, windowTitle);
				(OSErr)BasicTypesAE_CreatePStringDesc(windowTitle, &keyDesc);
				result = CreateObjSpecifier(cMyWindow, &containerDesc,
											formName, &keyDesc, true/* dispose inputs */,
											&objectDesc);
				if (result != noErr) Console_WriteValue("window access error", result);
				(OSErr)BasicTypesAE_CreatePropertyIDDesc(propertyPtr->dataType, &keyDesc);
				result = CreateObjSpecifier(cProperty, &objectDesc,
											formPropertyID, &keyDesc, true/* dispose inputs */,
											inoutDataDescriptor);
				if (result != noErr) Console_WriteValue("property access error", result);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyTerminalWindowContents:
		case pMyTerminalWindowScreenContents:
		case pMyTerminalWindowScrollbackContents:
			Console_WriteLine("entire text, main text, or scrollback text");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				// TEMPORARY - totally broken, will fix later
			#if 0
				TerminalScreenRef	screen = ScreenFactory_GetWindowActiveScreen
												(propertyPtr->container.data.terminalWindow.windowClass.ref);
				Terminal_LineRef	lineIterator = nullptr;
				UInt16				rowCount = 1;
				
				
				if (propertyPtr->dataType == pMyTerminalWindowContents)
				{
					// entire contents!
					lineIterator = nullptr;//TEMP!
					rowCount = 1;//TEMP!///realScreenPtr->screen.rowCountScrollback +
										//TEMP!///realScreenPtr->screen.rowCountVisible;
				}
				else if (propertyPtr->dataType == pMyTerminalWindowScrollbackContents)
				{
					// scrollback contents
					lineIterator = Terminal_NewScrollbackLineIterator(screen, 0/* starting row */);
					rowCount = Terminal_ReturnInvisibleRowCount(screen);
				}
				else
				{
					// screen contents
					lineIterator = Terminal_NewMainScreenLineIterator(screen, 0/* starting row */);
					rowCount = Terminal_ReturnRowCount(screen);
				}
				
				if (lineIterator == nullptr) result = errAEIllegalIndex; // this is the likely problem
				else
				{
					result = Terminal_CreateContentsAEDesc(screen, lineIterator, rowCount, inoutDataDescriptor);
					Terminal_DisposeLineIterator(&lineIterator);
				}
			#endif
			}
			else result = errAEEventNotHandled;
			break;
		
		default:
			// this class inherits from the window class; let *it* handle any other properties
			result = accessPropertyOfClassWindow(inTokenPtr, inoutDataDescriptor, inPropertyAccessMechanism);
			break;
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// accessPropertyOfClassTerminalWindow


/*!
Used to determine the data size, or acquire, or
modify, the property whose token is given.  The
token encodes the container class, as well, which
should be "cMyWindow".

(3.0)
*/
static OSErr
accessPropertyOfClassWindow	(ObjectClassesAE_TokenPtr   inTokenPtr,
							 AEDesc*					inoutDataDescriptor,
							 PropertyAccessMechanism	inPropertyAccessMechanism)
{
	OSErr						result = noErr;
	ObjectClassesAE_PropertyPtr	propertyPtr = &inTokenPtr->as.property;
	WindowRef					window = nullptr;
	WindowInfo_Ref				windowInfo = nullptr;
	Boolean						canModifyProperty = true;	// change this flag when appropriate to restrict access
	
	
	Console_BeginFunction();
	Console_WriteValueFourChars("AppleScript: window property; class type via", propertyPtr->container.eventClass);
	
	// extract the window to which this property belongs
	window = propertyPtr->container.data.window.ref;
	
	// make ABSOLUTELY SURE the window data is not screwed up (otherwise, the system could crash)
	unless (IsValidWindowRef(window))
	{
		Console_WriteLine("WARNING: invalid window pointer provided for get-window-property");
		result = errAEEventNotHandled;
	}
	
	// if the window is valid, get any Window Info associated with it
	if (result == noErr) windowInfo = WindowInfo_ReturnFromWindow(window);
	
	// verify that itÕs a property, and verify its container class type
	if (result == noErr)
	{
		result = ((inTokenPtr->flags & kObjectClassesAE_TokenFlagsIsProperty) &&
					GenesisAE_FirstClassIs(propertyPtr->container.eventClass, cMyWindow)) ? noErr
				: errAEEventNotHandled;
	}
	
	// get the data
	if (result == noErr)
	{
		switch (propertyPtr->dataType)
		{
		case pMyWindowBounds:
		case pMyWindowPosition:
			Console_WriteLine("bounding rectangle or position");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				Size	byteCount = (propertyPtr->dataType == pMyWindowPosition)
										? sizeof(Point) : sizeof(Rect);
				
				
				result = BasicTypesAE_CreateSizeDesc(byteCount, inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				Rect		rect;
				Point		topLeftCorner;
				GrafPtr		oldPort = nullptr;
				
				
				// determine the window location in global coordinates
				GetPortBounds(GetWindowPort(window), &rect);
				SetPt(&topLeftCorner, rect.left, rect.top);
				GetPort(&oldPort);
				SetPortWindowPort(window);
				LocalToGlobal(&topLeftCorner);
				rect.left += topLeftCorner.h;
				rect.top += topLeftCorner.v;
				rect.right += topLeftCorner.h;
				rect.bottom += topLeftCorner.v;
				SetPort(oldPort);
				if (propertyPtr->dataType == pMyWindowPosition)
				{
					result = BasicTypesAE_CreatePointDesc(topLeftCorner, inoutDataDescriptor);
				}
				else result = BasicTypesAE_CreateRectDesc(&rect, inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				if (propertyPtr->dataType == pMyWindowPosition)
				{
					canModifyProperty = true; // all windows can be moved
				}
				else if (windowInfo != nullptr)
				{
					switch (WindowInfo_ReturnWindowDescriptor(windowInfo))
					{
					case kConstantsRegistry_WindowDescriptorClipboard:
					case kConstantsRegistry_WindowDescriptorPreferences:
						// bounding rectangles of these windows can be changed
						canModifyProperty = true;
						break;
					
					default:
						// most other windows cannot be resized
						canModifyProperty = false;
						break;
					}
				}
				else canModifyProperty = false;
				
				if (canModifyProperty)
				{
					if (propertyPtr->dataType == pMyWindowPosition)
					{
						Point		newPosition;
						
						
						// make the data match the type in the Dictionary
						result = AECoerceDesc(inoutDataDescriptor, cpMyWindowPosition, inoutDataDescriptor);
						if (result == noErr)
						{
							Size		actualSize = 0L;
							
							
							result = AppleEventUtilities_CopyDescriptorData(inoutDataDescriptor, (Ptr)&newPosition,
																			sizeof(newPosition), &actualSize);
							if (result == noErr)
							{
								// move the window
								MoveWindow(window, newPosition.h, newPosition.v, false/* activate */);
							}
						}
					}
					else
					{
						Rect		newContentRect;
						
						
						// make the data match the type in the Dictionary
						result = AECoerceDesc(inoutDataDescriptor, cpMyWindowBounds, inoutDataDescriptor);
						if (result == noErr)
						{
							Size		actualSize = 0L;
							
							
							result = AppleEventUtilities_CopyDescriptorData(inoutDataDescriptor, (Ptr)&newContentRect,
																			sizeof(newContentRect), &actualSize);
							if (result == noErr)
							{
								Rect		contentRect;
								SInt32		deltaX = 0;
								SInt32		deltaY = 0;
								
								
								// determine if the new dimensions are acceptable (if possible)
								if (windowInfo != nullptr)
								{
									Point		dimensions;
									
									
									WindowInfo_GetWindowMaximumDimensions(windowInfo, &dimensions);
									if ((newContentRect.right - newContentRect.left) > dimensions.h)
									{
										result = errAEEventNotHandled;
									}
									if ((newContentRect.bottom - newContentRect.top) > dimensions.v)
									{
										result = errAEEventNotHandled;
									}
									
									WindowInfo_GetWindowMinimumDimensions(windowInfo, &dimensions);
									if ((newContentRect.right - newContentRect.left) < dimensions.h)
									{
										result = errAEEventNotHandled;
									}
									if ((newContentRect.bottom - newContentRect.top) < dimensions.v)
									{
										result = errAEEventNotHandled;
									}
								}
								
								// if so, continue to move and/or resize the window
								if (result == noErr)
								{
									// calculate the change in size (used for automatic control resizing)
									result = GetWindowBounds(window, kWindowContentRgn, &contentRect);
									assert_noerr(result);
									deltaX = (newContentRect.right - newContentRect.left) -
												(contentRect.right - contentRect.left);
									deltaY = (newContentRect.bottom - newContentRect.top) -
												(contentRect.bottom - contentRect.top);
									
									// rearrange the window, and resize its controls if possible (and if necessary)
									result = SetWindowBounds(window, kWindowContentRgn, &newContentRect);
									assert_noerr(result);
									if (windowInfo != nullptr)
									{
										WindowInfo_NotifyWindowOfResize(window, deltaX, deltaY);
									}
								}
							}
						}
					}
				}
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowHasCloseBox:
			Console_WriteLine("closeable");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				Boolean		flag = false;
				
				
			#if TARGET_API_MAC_OS8
				flag = GetWindowGoAwayFlag(window);
			#else
				{
					WindowAttributes	attributes;
					
					
					GetWindowAttributes(window, &attributes);
					flag = ((attributes & kWindowCloseBoxAttribute) != 0);
				}
			#endif
				result = BasicTypesAE_CreateBooleanDesc(flag, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowHasCollapseBox:
			Console_WriteLine("collapsible");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				result = BasicTypesAE_CreateBooleanDesc(IsWindowCollapsable(window), inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowHasTitleBar:
			Console_WriteLine("titled");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				// UNIMPLEMENTED - but virtually all MacTelnet windows have title bars!
				result = BasicTypesAE_CreateBooleanDesc(true, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowHasToolbar:
			Console_WriteLine("toolbarred");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				WindowAttributes	attributes = 0L;
				Boolean				flag = false;
				
				
			#if TARGET_API_MAC_CARBON
				// this property could only be true on Mac OS X
				result = GetWindowAttributes(window, &attributes);
				assert_noerr(result);
				flag = ((attributes & (1L << 6)/* Mac OS X toolbar widget attribute */) != 0);
			#endif
				result = BasicTypesAE_CreateBooleanDesc(flag, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowHidden:
			Console_WriteLine("obscured");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				// this trait only applies to terminal windows
				Boolean		flag = TerminalWindow_ExistsFor(window);
				
				
				if (flag)
				{
					TerminalWindowRef		terminalWindow = TerminalWindow_ReturnFromWindow(window);
					
					
					if (terminalWindow != nullptr) flag = TerminalWindow_IsObscured(terminalWindow);
				}
				result = BasicTypesAE_CreateBooleanDesc(flag, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowIndexInWindowList:
			Console_WriteLine("index");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(SInt16), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				SInt16		windowIndex = 1;
				WindowRef	frontWindow = EventLoop_ReturnRealFrontWindow();
				
				
				// find the window
				if (frontWindow != window)
				{
					// if not asking for the front window, search the window list for a match
					WindowRef	windowIterator = frontWindow;
					
					
					do
					{
						windowIterator = GetNextWindow(windowIterator);
						++windowIndex;
						if (windowIndex > 40/* arbitrary */) break; // paranoia, but infinite loops are ALWAYS bad!
					} while ((windowIterator != window) && (windowIterator != frontWindow));
				}
				result = BasicTypesAE_CreateSInt16Desc(windowIndex, inoutDataDescriptor);
				if (result == noErr)
				{
					// make the data match the type in the Dictionary
					result = AECoerceDesc(inoutDataDescriptor, cpMyWindowIndexInWindowList,
											inoutDataDescriptor);
				}
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowIsCollapsed:
			Console_WriteLine("collapsed");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				result = BasicTypesAE_CreateBooleanDesc(IsWindowCollapsed(window), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				result = AECoerceDesc(inoutDataDescriptor, typeBoolean, inoutDataDescriptor);
				if (result == noErr)
				{
					Size		actualSize = 0L;
					Boolean		flag = false;
					
					
					result = AppleEventUtilities_CopyDescriptorData(inoutDataDescriptor, (Ptr)&flag,
																	sizeof(flag), &actualSize);
					if (result == noErr)
					{
						// collapse or un-collapse the window (minimizes on Mac OS X)
						CollapseWindow(window, flag/* collapse / minimize */);
					}
				}
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowIsFloating:
			Console_WriteLine("floating");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				Boolean		flag = false; // UNIMPLEMENTED
				
				
				result = BasicTypesAE_CreateBooleanDesc(flag, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowIsModal:
			Console_WriteLine("modal");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				Boolean		flag = false; // UNIMPLEMENTED
				
				
				result = BasicTypesAE_CreateBooleanDesc(flag, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowIsResizeable:
			Console_WriteLine("resizeable");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				Boolean		flag = false; // UNIMPLEMENTED
				
				
				result = BasicTypesAE_CreateBooleanDesc(flag, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowIsVisible:
			Console_WriteLine("visible");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				result = BasicTypesAE_CreateBooleanDesc(IsWindowVisible(window), inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowIsZoomable:
			Console_WriteLine("zoomable");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				Boolean		flag = false;
				
				
			#if TARGET_API_MAC_OS8
				flag = GetWindowZoomFlag(window);
			#else
				{
					WindowAttributes	attributes;
					
					
					GetWindowAttributes(window, &attributes);
					flag = (((attributes & kWindowHorizontalZoomAttribute) != 0) ||
							((attributes & kWindowVerticalZoomAttribute) != 0));
				}
			#endif
				result = BasicTypesAE_CreateBooleanDesc(flag, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowIsZoomed:
			Console_WriteLine("zoomed");
			if (inPropertyAccessMechanism == kPropertyAccessSize)
			{
				result = BasicTypesAE_CreateSizeDesc(sizeof(Boolean), inoutDataDescriptor);
			}
			else if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				Boolean		flag = IsWindowInStandardState(window, nullptr/* ideal size */, nullptr/* ideal standard state */);
				
				
				result = BasicTypesAE_CreateBooleanDesc(flag, inoutDataDescriptor);
			}
			else result = errAEEventNotHandled;
			break;
		
		case pMyWindowName:
			Console_WriteLine("name");
			if (inPropertyAccessMechanism == kPropertyAccessObtain)
			{
				Str255		windowName;
				
				
				GetWTitle(window, windowName);
				result = BasicTypesAE_CreatePStringDesc(windowName, inoutDataDescriptor);
				if (result == noErr)
				{
					// make the data match the type in the Dictionary
					result = AECoerceDesc(inoutDataDescriptor, cpMyWindowName, inoutDataDescriptor);
				}
			}
			else if (inPropertyAccessMechanism == kPropertyAccessModify)
			{
				if (windowInfo != nullptr)
				{
					switch (WindowInfo_ReturnWindowDescriptor(windowInfo))
					{
					case kConstantsRegistry_WindowDescriptorAnyTerminal:
					case kConstantsRegistry_WindowDescriptorDebuggingConsole:
						// titles of these windows can be changed
						canModifyProperty = true;
						break;
					
					default:
						// most other window titles cannot be changed
						canModifyProperty = false;
						break;
					}
				}
				else canModifyProperty = true;
				
				if (canModifyProperty)
				{
					Str255		windowName;
					
					
					// make the data match the type in the Dictionary
					result = AECoerceDesc(inoutDataDescriptor, cpMyWindowName, inoutDataDescriptor);
					if (result == noErr)
					{
						Size	actualSize = 0L;
						
						
						result = AppleEventUtilities_CopyDescriptorData(inoutDataDescriptor, (Ptr)&windowName[1],
																		sizeof(windowName), &actualSize);
						if (result == noErr)
						{
							// set the length (itÕs a Pascal string) and then set the title of the window
							windowName[0] = ((UInt8)actualSize);
							SetWTitle(window, windowName);
						}
					}
				}
				else result = errAEEventNotHandled;
			}
			else result = errAEEventNotHandled;
			break;
		
		default:
			result = errAECantSupplyType;
			break;
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// accessPropertyOfClassWindow


/*!
To obtain a property class using "formPropertyID"
selection data as a reference (that is, you know
the property ID of a property in a class), use this
method.  The property ID must be a valid property
for some class.

The container type is expected to be a known class.

(3.0)
*/
static OSStatus
getPropertyByID		(DescType		inPropertyID,
					 AEDesc const*  inFromWhat,
					 AEDesc*		outObjectOfDesiredClass)
{
	OSStatus	result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteValueFourChars("AppleScript: ÒpropertyÓ by property ID", inPropertyID);
	
	if (inFromWhat->descriptorType == cMyInternalToken)
	{
		// now decide what property is being specified
		ObjectClassesAE_Token   containerToken;
		Size					actualSize = 0;
		
		
		Console_WriteLine("from internal token");
		result = AppleEventUtilities_CopyDescriptorData(inFromWhat, &containerToken,
														sizeof(containerToken), &actualSize);
		if (result == noErr)
		{
			ObjectClassesAE_Token   propertyToken;
			AEDesc					propertyDescriptor;
			Boolean					usedDesiredDataType = false;
			
			
			propertyToken.flags = kObjectClassesAE_TokenFlagsIsProperty;
			propertyToken.genericPointer = nullptr;
			propertyToken.as.property.dataType = inPropertyID;
			propertyToken.as.property.container = containerToken.as.object;
			Console_WriteValueFourChars("parent object type", propertyToken.as.property.container.eventClass);
			
			result = GenesisAE_CreateTokenFromObjectData(propertyToken.as.property.dataType, &propertyToken,
															&propertyDescriptor, nullptr/* final type */);
			if (result == noErr)
			{
				result = GetPropertyAE_DataFromClassProperty(&propertyDescriptor, typeWildCard,
																outObjectOfDesiredClass, &usedDesiredDataType);
				Console_WriteValueFourChars("final data type", outObjectOfDesiredClass->descriptorType);
			}
		}
	}
	else
	{
		Console_WriteValueFourChars("do not know how to read properties from given container type", inFromWhat->descriptorType);
		result = errAECantSupplyType;
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// getPropertyByID


/*!
This routine helps to reduce code duplication,
and handles construction of a token data type
and property access using that token.

(3.0)
*/
static OSErr
tokenAccess		(AEDesc const*				inTokenDescriptor,
				 AEDesc*					inoutDataDescriptor,
				 PropertyAccessMechanism	inAccessMechanism)
{
	OSErr					result = noErr;
	ObjectClassesAE_Token   token;
	
	
	Console_BeginFunction();
	Console_WriteLine("accessing data");
	result = AppleEventUtilities_CopyDescriptorData(inTokenDescriptor, &token, sizeof(token),
													nullptr/* actual size storage */);
	if (result == noErr)
	{
		if (token.flags & kObjectClassesAE_TokenFlagsIsProperty)
		{
			// request to get, set, or obtain the data size of a property of a class
			Console_WriteValueFourChars("property in container, class type", token.as.property.container.eventClass);
			Console_WriteValueFourChars("property type", token.as.property.dataType);
			switch (token.as.property.container.eventClass)
			{
			case cMyApplication:
			case typeNull:
				result = accessPropertyOfClassApplication(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMyApplicationPreferences:
				result = accessPropertyOfClassApplicationPreferences(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMyClipboardWindow:
				result = accessPropertyOfClassClipboardWindow(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMyConnection:
				result = accessPropertyOfClassConnection(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMyDialogReply:
				result = accessPropertyOfClassDialogReply(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMyFormat:
				result = accessPropertyOfClassFormat(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMyMacro:
				result = accessPropertyOfClassMacro(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMyMacroSet:
				result = accessPropertyOfClassMacroSet(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMyProxyServerConfiguration:
				result = accessPropertyOfClassProxyServer(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMySessionWindow:
				result = accessPropertyOfClassSessionWindow(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMyTerminalWindow:
				result = accessPropertyOfClassTerminalWindow(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			case cMyWindow:
				result = accessPropertyOfClassWindow(&token, inoutDataDescriptor, inAccessMechanism);
				break;
			
			default:
				Console_WriteValueFourChars("unrecognized property container type!", token.as.property.container.eventClass);
				result = errAEWrongDataType; // not a MacTelnet class
				break;
			}
		}
		else if ((token.flags & kObjectClassesAE_TokenFlagsIsObject) && (inAccessMechanism == kPropertyAccessObtain))
		{
			// request to get a class itself (as opposed to one of its properties)
			Console_WriteValueFourChars("container itself, class type", token.as.object.eventClass);
			switch (token.as.object.eventClass)
			{
			case cMyApplication:
				// taken care of automatically, because the application container is standard
				result = errAEEventNotHandled;
				break;
			
			case cMyApplicationPreferences:
				// the only recognized instance of the application preferences class is a
				// property of the null (application) container
				{
					//ObjectClassesAE_ApplicationPreferencesPtr	dataPtr = &token.as.object.data.applicationPreferences;
					AEDesc										containerDesc; // never changes, in this case
					AEDesc										keyDesc;
					
					
					(OSErr)AppleEventUtilities_InitAEDesc(&containerDesc);
					(OSErr)AppleEventUtilities_InitAEDesc(&keyDesc);
					
					// return the specifier "preferences of application"
					(OSErr)BasicTypesAE_CreatePropertyIDDesc(pMyPreferences, &keyDesc);
					result = CreateObjSpecifier(cMyApplicationPreferences, &containerDesc,
												formPropertyID, &keyDesc, true/* dispose inputs */,
												inoutDataDescriptor);
					if (result != noErr) Console_WriteValue("error creating object specifier", result);
				}
				break;
			
			case cMyClipboardWindow:
				{
					//ObjectClassesAE_ClipboardWindowPtr	dataPtr = &token.as.object.data.clipboardWindow;
					AEDesc									containerDesc; // never changes, in this case
					AEDesc									keyDesc;
					
					
					(OSErr)AppleEventUtilities_InitAEDesc(&containerDesc);
					(OSErr)AppleEventUtilities_InitAEDesc(&keyDesc);
					
					// return the specifier "clipboard of application"
					(OSErr)BasicTypesAE_CreatePropertyIDDesc(pMyClipboard, &keyDesc);
					result = CreateObjSpecifier(cMyClipboardWindow, &containerDesc, formPropertyID,
												&keyDesc, true/* dispose inputs */, inoutDataDescriptor);
					if (result != noErr) Console_WriteValue("error creating object specifier", result);
				}
				break;
			
			case cMyConnection:
				{
					//ObjectClassesAE_ConnectionPtr	dataPtr = &token.as.object.data.connection;
					AEDesc							containerDesc; // never changes, in this case
					AEDesc							keyDesc;
					
					
					(OSErr)AppleEventUtilities_InitAEDesc(&containerDesc);
					(OSErr)AppleEventUtilities_InitAEDesc(&keyDesc);
					
					// TEMP - return the specifier "connection 1", which is correct only for the frontmost window!!!
					(OSErr)BasicTypesAE_CreateSInt32Desc(1, &keyDesc);
					result = CreateObjSpecifier(cMyConnection, &containerDesc, formAbsolutePosition,
												&keyDesc, true/* dispose inputs */, inoutDataDescriptor);
					if (result != noErr) Console_WriteValue("error creating object specifier", result);
				}
				break;
			
			case cMyDialogReply:
				// incomplete - there needs to be a global application property like
				// "most recent dialog reply" to handle this properly
				result = errAEEventNotHandled;
				break;
			
			case cMyMacro:
				// incomplete - needs to use token.as.object.data.macro to find macro set
				{
					//ObjectClassesAE_MacroPtr		dataPtr = &token.as.object.data.macro;
					
					
					result = errAEEventNotHandled;
				}
				break;
			
			case cMyMacroSet:
				{
					ObjectClassesAE_MacroSetPtr	dataPtr = &token.as.object.data.macroSet;
					AEDesc						containerDesc; // never changes, in this case
					AEDesc						keyDesc;
					
					
					(OSErr)AppleEventUtilities_InitAEDesc(&containerDesc);
					(OSErr)AppleEventUtilities_InitAEDesc(&keyDesc);
					
					// send a request for "macro set <index>", which resides in a null container
					(OSErr)BasicTypesAE_CreateSInt32Desc(dataPtr->number, &keyDesc);
					result = CreateObjSpecifier(cMyMacroSet, &containerDesc,
												formAbsolutePosition, &keyDesc, true/* dispose inputs */,
												inoutDataDescriptor);
					if (result != noErr) Console_WriteValue("error creating object specifier", result);
				}
				break;
			
			case cMyProxyServerConfiguration:
				result = AECreateDesc(cMyProxyServerConfiguration, (Ptr)&token.as.object.data.proxyServer,
										sizeof(token.as.object.data.proxyServer), inoutDataDescriptor);
				break;
			
			case cMySessionWindow:
			case cMyTerminalWindow:
			case cMyWindow:
				{
					ObjectClassesAE_WindowPtr	dataPtr = &token.as.object.data.window;
					AEDesc						containerDesc; // never changes, in this case
					AEDesc						keyDesc;
					DescType					keyForm = formAbsolutePosition;
					
					
					(OSErr)AppleEventUtilities_InitAEDesc(&containerDesc);
					(OSErr)AppleEventUtilities_InitAEDesc(&keyDesc);
					
					if (dataPtr->ref == EventLoop_ReturnRealFrontWindow())
					{
						// frontmost window; return the specifier "window 1", which is more flexible
						(OSErr)BasicTypesAE_CreateSInt32Desc(1, &keyDesc);
						keyForm = formAbsolutePosition;
					}
					else
					{
						// return the windowÕs name as the specifier; not as good as an index, especially
						// if multiple windows have the same name, but probably good enough in most cases
						Str255		name;
						
						
						GetWTitle(dataPtr->ref, name);
						BasicTypesAE_CreatePStringDesc(name, &keyDesc);
						keyForm = formName;
					}
					result = CreateObjSpecifier(cMyWindow, &containerDesc, keyForm, &keyDesc,
												true/* dispose inputs */, inoutDataDescriptor);
					if (result != noErr) Console_WriteValue("error creating object specifier", result);
				}
				break;
			
			default:
				Console_WriteLine("unrecognized class type!");
				result = errAEWrongDataType; // not a MacTelnet class
				break;
			}
		}
		else
		{
			// request to get WHO KNOWS WHAT!
			Console_WriteLine("unrecognized request!");
			result = errAEWrongDataType;
		}
	}
	Console_WriteValue("result", result);
	Console_EndFunction();
	
	return result;
}// tokenAccess

// BELOW IS REQUIRED NEWLINE TO END FILE
