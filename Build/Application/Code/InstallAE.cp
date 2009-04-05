/*###############################################################

	InstallAE.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <MemoryBlocks.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "CoercionsAE.h"
#include "ConnectionAE.h"
#include "ConstantsRegistry.h"
#include "CoreSuiteAE.h"
#include "DataAccessAE.h"
#include "GenesisAE.h"
#include "GetElementAE.h"
#include "GetPropertyAE.h"
#include "HiddenAE.h"
#include "InstallAE.h"
#include "ObjectClassesAE.h"
#include "RequiredAE.h"
#include "Terminology.h"
#include "URLAccessAE.h"



#pragma mark Internal Method Prototypes

static OSStatus			installApplicationEventClassHandlers	();
static OSStatus			installCoercionHandlers					();
static OSStatus			installDataHandlers						();
static OSStatus			installHiddenEventHandlers				();
static OSStatus			installMiscellaneousCoreHandlers		();
static OSStatus			installObjectAccessors					();
static Boolean			installRequiredHandlers					();
static OSStatus			installURLAccessEventHandlers			();
static pascal OSErr		tokenComparer							(DescType, AEDesc const*, AEDesc const*, Boolean*);
static pascal OSErr		tokenDestroyer							(AEDesc*);



#pragma mark Public Methods

/*!
This method initializes the Apple Event handlers
for the Required Suite, and creates an address
descriptor so MacTelnet can send events to itself
(for recordability).

If any problems occur, false is returned and the
application should be considered impossible to
start up!

You must call AppleEvents_Done() to clean up
the memory allocations, etc. performed by a call
to AppleEvents_Init() when you are done with
Apple Events (that is, when the program ends).

(3.0)
*/
Boolean
InstallAE_Init ()
{
	Boolean			result = true;
	OSStatus		error = noErr;
	long			gestaltValue = 0L;
	
	
	error = Gestalt(gestaltAppleEventsAttr, &gestaltValue); // see if AppleEvents are available
	{
		Boolean		haveAppleEvents = ((noErr == error) &&
										((gestaltValue >> gestaltAppleEventsPresent) & 0x0001));
		
		
		if (haveAppleEvents) result = installRequiredHandlers();
		else result = false; // no AppleScript, big problem!
	}
	
	if (result) // Got milk?  Go for cookies.
	{
		error = AEObjectInit();
		result = (noErr == error);
	}
	
	if (result) // still OK?
	{
		// install data access handlers
		if (result) error = installDataHandlers();
		result = (noErr == error);
		
		// install other types of core event handlers
		if (result) error = installMiscellaneousCoreHandlers();
		result = (noErr == error);
	}
	
	if (result) // still OK?
	{
		// install event handlers for every MacTelnet event class
		if (result) error = installApplicationEventClassHandlers();
		result = (noErr == error);
		if (result) error = installHiddenEventHandlers();
		result = (noErr == error);
		if (result) error = installURLAccessEventHandlers();
		result = (noErr == error);
	}
	
	// object accessors
	error = installObjectAccessors();
	result = (noErr == error);
	
	// coercion handlers
	error = installCoercionHandlers();
	result = (noErr == error);
	
	return result;
}// Init


/*!
Cleans up this module.

(3.0)
*/
void
InstallAE_Done ()
{
	// could remove and dispose handlers; UNIMPLEMENTED
}// Done


#pragma mark Internal Methods

/*!
This routine installs Apple Event handlers for
the MacTelnet Suite.

If any handler cannot be installed, an error
is returned and some handlers may not be
installed properly.

(3.0)
*/
static OSStatus
installApplicationEventClassHandlers ()
{
	OSStatus			result = noErr;
	AEEventClass		eventClass = kMySuite;
	AEEventHandlerUPP	upp = nullptr;
	
	
	// install event handlers for every MacTelnet event
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(AppleEvents_HandleAlert);
		result = AEInstallEventHandler(eventClass, kTelnetEventIDAlert, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(ConnectionAE_HandleConnectionOpen);
		result = AEInstallEventHandler(eventClass, kTelnetEventIDConnectionOpen, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(AppleEvents_HandleCopySelectedText);
		result = AEInstallEventHandler(eventClass, kTelnetEventIDCopyToClipboard, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(AppleEvents_HandleLaunchFind);
		result = AEInstallEventHandler(eventClass, kTelnetEventIDLaunchFind, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(ConnectionAE_HandleSend);
		result = AEInstallEventHandler(eventClass, kTelnetEventIDDataSend, upp, 0L, false);
	}
	
	return result;
}// installApplicationEventClassHandlers


/*!
This routine installs Apple Event type coercion
handlers.  MacTelnet uses coercion handlers to
convert complex data types into simpler ones,
and to resolve objects into internal tokens.

Incomplete.

(3.0)
*/
static OSStatus
installCoercionHandlers ()
{
	OSStatus				result = noErr;
	DescType				sourceType = typeNull,
							destinationType = typeNull;
	AECoercionHandlerUPP	upp = nullptr;
	
	
	sourceType = typeObjectSpecifier;
	destinationType = cMyInternalToken;
	upp = REINTERPRET_CAST(NewAECoerceDescUPP(CoercionsAE_CoerceObjectToAnotherType), AECoercionHandlerUPP);
	result = AEInstallCoercionHandler(sourceType, destinationType, upp, 0L/* reference constant */,
										true/* source is a descriptor? */, false/* system type handler? */);
	
	return result;
}// installCoercionHandlers


/*!
This routine installs Apple Event handlers for
getting and setting data, such as properties
of classes.

If any handler cannot be installed, an error
is returned and some handlers may not be
installed properly.

(3.0)
*/
static OSStatus
installDataHandlers ()
{
	OSStatus			result = noErr;
	AEEventClass		eventClass = kAECoreSuite;
	AEEventHandlerUPP	upp = nullptr;
	
	
	// install event handlers for every data event
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(DataAccessAE_HandleSetData);
		result = AEInstallEventHandler(eventClass, kAESetData, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(DataAccessAE_HandleGetData);
		result = AEInstallEventHandler(eventClass, kAEGetData, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(DataAccessAE_HandleGetDataSize);
		result = AEInstallEventHandler(eventClass, kAEGetDataSize, upp, 0L, false);
	}
	
	return result;
}// installDataHandlers


/*!
This routine installs Apple Event handlers for
the MacTelnet Suite, but in particular for those
events which are not in MacTelnet’s Dictionary.
These events are used to do things that are
useful for MacTelnet Help, but which are not in
general useful for scripts (such as displaying
particular dialog boxes).

If any handler cannot be installed, an error is
returned and some handlers may not be installed
properly.

(3.0)
*/
static OSStatus
installHiddenEventHandlers ()
{
	OSStatus			result = noErr;
	AEEventClass		eventClass = kMySuite;
	AEEventHandlerUPP	upp = nullptr;
	
	
	// install event handlers for every MacTelnet event
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(HiddenAE_HandleConsoleWriteLine);
		result = AEInstallEventHandler(eventClass, kTelnetHiddenEventIDConsoleWriteLine, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(HiddenAE_HandleDisplayDialog);
		result = AEInstallEventHandler(eventClass, kTelnetHiddenEventIDDisplayDialog, upp, 0L, false);
	}
	
	return result;
}// installHiddenEventHandlers


/*!
This routine installs Apple Event handlers for
core events not related to data access.  See
installDataHandlers() for details on what is
installed for data-access-related core events.

If any handler cannot be installed, an error
is returned and some handlers may not be
installed properly.

(3.0)
*/
static OSStatus
installMiscellaneousCoreHandlers ()
{
	OSStatus			result = noErr;
	AEEventClass		eventClass = kAECoreSuite;
	AEEventHandlerUPP	upp = nullptr;
	
	
	// install event handlers for non-data core events (see installDataHandlers() for the others)
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(AppleEventUtilities_ObjectExists);
		result = AEInstallEventHandler(eventClass, kAEDoObjectsExist, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(CoreSuiteAE_CountElements);
		result = AEInstallEventHandler(eventClass, kAECountElements, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(CoreSuiteAE_Close);
		result = AEInstallEventHandler(eventClass, kAEClose, upp, 0L, false);
	}
	#if 0
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(CoreSuiteAE_CreateElement);
		result = AEInstallEventHandler(eventClass, kAECreateElement, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(CoreSuiteAE_Duplicate);
		result = AEInstallEventHandler(eventClass, kAEClone, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(CoreSuiteAE_Delete);
		result = AEInstallEventHandler(eventClass, kAEDelete, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(CoreSuiteAE_Move);
		result = AEInstallEventHandler(eventClass, kAEMove, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(CoreSuiteAE_Save);
		result = AEInstallEventHandler(eventClass, kAESave, upp, 0L, false);
	}
	
	eventClass = kAEMiscStandards;
	
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(?);
		result = AEInstallEventHandler(eventClass, kAECut, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(?);
		result = AEInstallEventHandler(eventClass, kAECopy, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(?);
		result = AEInstallEventHandler(eventClass, kAEPaste, upp, 0L, false);
	}
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(?);
		result = AEInstallEventHandler(eventClass, kAERevert, upp, 0L, false);
	}
	#endif
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(CoreSuiteAE_SelectObject);
		result = AEInstallEventHandler(eventClass, kAESelect, upp, 0L, false);
	}
	
	return result;
}// installMiscellaneousCoreHandlers


/*!
This routine installs OSL accessors for obtaining
tokens from element references.

If any accessor cannot be installed, an error is
returned and some accessors may not be installed
properly.

(3.0)
*/
static OSStatus
installObjectAccessors ()
{
	OSStatus		result = noErr;
	DescType		obtainedClass = '----';
	DescType		referenceClass = '----';
	OSLAccessorUPP  upp = nullptr;
	
	
	// set up callbacks
	result = AESetObjectCallbacks(NewOSLCompareUPP(tokenComparer),
									NewOSLCountUPP(CoreSuiteAE_CountElementsOfContainer),
									NewOSLDisposeTokenUPP(tokenDestroyer),	// disposes of dynamic data in tokens
									(OSLGetMarkTokenUPP)nullptr,
									(OSLMarkUPP)nullptr,
									(OSLAdjustMarksUPP)nullptr,
									(OSLGetErrDescUPP)nullptr);
	
	// install accessors for elements of objects - incomplete
	if (noErr == result)
	{
		upp = NewOSLAccessorUPP(GetElementAE_ApplicationFromNull);
		obtainedClass = cApplication;
		referenceClass = typeNull;
		result = AEInstallObjectAccessor(obtainedClass, referenceClass, upp, 0L, false/* is system handler */);
	}
	if (noErr == result)
	{
		// install accessors for all window classes, and classes that can be derived from windows
		upp = NewOSLAccessorUPP(GetElementAE_WindowFromNull);
		obtainedClass = cWindow;
		referenceClass = typeNull;
		result = AEInstallObjectAccessor(obtainedClass, referenceClass, upp, 0L, false/* is system handler */);
		if (noErr == result)
		{
			obtainedClass = cMyConnection;
			result = AEInstallObjectAccessor(obtainedClass, referenceClass, upp, 0L, false/* is system handler */);
		}
		if (noErr == result)
		{
			obtainedClass = cMySessionWindow;
			result = AEInstallObjectAccessor(obtainedClass, referenceClass, upp, 0L, false/* is system handler */);
		}
		if (noErr == result)
		{
			obtainedClass = cMyTerminalWindow;
			result = AEInstallObjectAccessor(obtainedClass, referenceClass, upp, 0L, false/* is system handler */);
		}
	}
	
	// install accessors for properties
	if (noErr == result)
	{
		upp = NewOSLAccessorUPP(GetPropertyAE_FromNull);
		obtainedClass = cProperty;
		referenceClass = typeNull;
		result = AEInstallObjectAccessor(obtainedClass, referenceClass, upp, 0L, false/* is system handler */);
		
		upp = NewOSLAccessorUPP(GetPropertyAE_FromObject);
		obtainedClass = cProperty;
		referenceClass = typeAEList;
		(OSStatus)AEInstallObjectAccessor(obtainedClass, referenceClass, upp, 0L, false/* is system handler */);
		referenceClass = cMyInternalToken;
		(OSStatus)AEInstallObjectAccessor(obtainedClass, referenceClass, upp, 0L, false/* is system handler */);
	}
	
	return result;
}// installObjectAccessors


/*!
This routine installs Apple Event handlers for
the Required Suite, including Appearance events
and the six standard Apple Events (open and
print documents, open, re-open and quit
application, and display preferences).

If ALL handlers install successfully, "true" is
returned; otherwise, "false" is returned.

(3.0)
*/
static Boolean
installRequiredHandlers ()
{
	Boolean			result = false;
	OSStatus		error = noErr;
	AEEventClass	eventClass = '----';
	
	
	eventClass = kASRequiredSuite;
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEOpenApplication,
														NewAEEventHandlerUPP(RequiredAE_HandleApplicationOpen),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEReopenApplication,
														NewAEEventHandlerUPP(RequiredAE_HandleApplicationReopen),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEOpenDocuments,
														NewAEEventHandlerUPP(RequiredAE_HandleOpenDocuments),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEPrintDocuments,
														NewAEEventHandlerUPP(RequiredAE_HandlePrintDocuments),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEQuitApplication,
														NewAEEventHandlerUPP(RequiredAE_HandleApplicationQuit),
														0L, false);
	if (noErr == error) error = AEInstallEventHandler(eventClass, kAEShowPreferences,
														NewAEEventHandlerUPP(RequiredAE_HandleApplicationPreferences),
														0L, false);
	if (noErr == error)
	{
		// ignore any errors for these
		eventClass = kAppearanceEventClass;
		(OSStatus)AEInstallEventHandler(eventClass, kAEAppearanceChanged,
										NewAEEventHandlerUPP(RequiredAE_HandleAppearanceSwitch),
										0L, false);
		(OSStatus)AEInstallEventHandler(eventClass, kAESystemFontChanged,
										NewAEEventHandlerUPP(RequiredAE_HandleAppearanceSwitch),
										0L, false);
		(OSStatus)AEInstallEventHandler(eventClass, kAESmallSystemFontChanged,
										NewAEEventHandlerUPP(RequiredAE_HandleAppearanceSwitch),
										0L, false);
		(OSStatus)AEInstallEventHandler(eventClass, kAEViewsFontChanged,
										NewAEEventHandlerUPP(RequiredAE_HandleAppearanceSwitch),
										0L, false);
	}
	
	result = (noErr == error);
	
	return result;
}// installRequiredHandlers


/*!
This routine installs Apple Event handlers for
the URL Suite (URL Access).

If any handler cannot be installed, an error
is returned and some handlers may not be
installed properly.

(3.0)
*/
static OSStatus
installURLAccessEventHandlers ()
{
	OSStatus			result = noErr;
	AEEventClass		eventClass = kStandardURLSuite;
	AEEventHandlerUPP	upp = nullptr;
	
	
	// install event handlers for every supported URL event
	if (noErr == result)
	{
		upp = NewAEEventHandlerUPP(URLAccessAE_HandleUniformResourceLocator);
		result = AEInstallEventHandler(eventClass, kStandardURLEventIDGetURL, upp, 0L, false);
	}
	
	return result;
}// installURLAccessEventHandlers


/*!
This callback routine examines the specified
objects, and determines if the operator holds.
The method for doing this will obviously vary
based on the kind of objects given, and it is
possible that the comparison will fail, giving
the result "errAEEventNotHandled".

The specified operator type can be one of the
following values:

	kAEGreaterThan
	kAEGreaterThanEquals
	kAEEquals
	kAELessThan
	kAELessThanEquals
	kAEBeginsWith
	kAEEndsWith
	kAEContains

Unimplemented.

(3.0)
*/
static pascal OSErr
tokenComparer		(DescType			inOperator,
					 AEDesc const*		inObject1,
					 AEDesc const*		inObject2,
					 Boolean*			outAreEqual)
{
	OSErr		result = noErr;
	
	
	return result;
}// tokenComparer


/*!
This callback routine examines the specified
token (an Apple Event descriptor whose type is
"cMyInternalToken"), assuming that its data is
a "ObjectClassesAE_Token" class (see the file
"ObjectClassesAE.h" for details).  If the class
flags indicate that the dynamic data pointer of
the token is a "Handle" or "Ptr", then the data
is disposed (releasing the memory).

(3.0)
*/
static pascal OSErr
tokenDestroyer		(AEDesc*		inoutTokenDescriptor)
{
	OSErr		result = noErr;
	
	
	if (nullptr != inoutTokenDescriptor)
	{
		ObjectClassesAE_Token   token;
		
		
		if (noErr == GenesisAE_CreateTokenFromObjectSpecifier(inoutTokenDescriptor, &token))
		{
			if (token.flags & kObjectClassesAE_TokenFlagsDisposeDataPointer)
			{
				Memory_DisposePtr((Ptr*)&token.genericPointer);
			}
			else if (token.flags & kObjectClassesAE_TokenFlagsDisposeDataHandle)
			{
				Memory_DisposeHandle((Handle*)&token.genericPointer);
			}
		}
	}
	
	return result;
}// tokenDestroyer

// BELOW IS REQUIRED NEWLINE TO END FILE
