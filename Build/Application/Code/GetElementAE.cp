/*###############################################################

	GetElementAE.cp
	
	MacTelnet
		© 1998-2006 by Kevin Grant.
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

// resource includes
#include "StringResources.h"

// library includes
#include <Console.h>
#include <WindowInfo.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "Clipboard.h"
#include "ConstantsRegistry.h"
#include "EventLoop.h"
#include "GenesisAE.h"
#include "GetElementAE.h"
#include "MacroManager.h"
#include "ObjectClassesAE.h"
#include "Terminology.h"



#pragma mark Internal Method Prototypes

static OSStatus	createObjectFromWindow				(DescType			inDesiredClass,
													 WindowRef			inWindow,
													 AEDesc*			outObjectOfDesiredClass);

static OSStatus	getMacroByAbsolutePosition			(AEDesc const*		inReference,
													 AEDesc const*		inFromWhat,
													 AEDesc*			outObjectOfDesiredClass);

static OSStatus	getMacroSetByAbsolutePosition		(AEDesc const*		inReference,
													 AEDesc*			outObjectOfDesiredClass);

static OSStatus	getWindowByAbsolutePosition			(DescType			inDesiredClass,
													 AEDesc const*		inReference,
													 AEDesc*			outObjectOfDesiredClass);

static OSStatus	getWindowByName						(DescType			inDesiredClass,
													 AEDesc const*		inReference,
													 AEDesc*			outObjectOfDesiredClass);

static OSStatus	getWindowByRelativePosition			(DescType			inDesiredClass,
													 AEDesc const*		inRelativeToWhat,
													 AEDesc const*		inReference,
													 AEDesc*			outObjectOfDesiredClass);

static SInt16	getWindowCount						();

static Boolean	isClassObtainableFromWindow			(DescType			inDesiredClass);



#pragma mark Public Methods

/*!
Handles the following OSL object access:

	cApplication <- typeNull

(3.0)
*/
pascal OSErr
GetElementAE_ApplicationFromNull	(DescType			inDesiredClass,
									 AEDesc const*		UNUSED_ARGUMENT(inFromWhat),
									 DescType			UNUSED_ARGUMENT(inFromWhatClass),
									 DescType			inFormOfReference,
									 AEDesc const*		UNUSED_ARGUMENT(inReference),
									 AEDesc*			outObjectOfDesiredClass,
									 long				UNUSED_ARGUMENT(inData))
{
	OSErr	result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: requested application");
	
	if (inDesiredClass != cApplication)
	{
		Console_WriteLine("failed, desired type is not an application");
		result = errAEWrongDataType;
	}
	
	// create the appropriate object
	if (result == noErr)
	{
		switch (inFormOfReference)
		{
		default:
			result = GetElementAE_ApplicationImplicit(outObjectOfDesiredClass);
			break;
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// ApplicationFromNull


/*!
To obtain the application class for MacTelnet
itself, use this method.

(3.0)
*/
OSStatus
GetElementAE_ApplicationImplicit	(AEDesc*	outObjectOfDesiredClass)
{
	OSStatus				result = noErr;
	ObjectClassesAE_Token   token;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “application” implicitly");
	token.flags = 0;
	token.genericPointer = nullptr;
	token.as.object.data.application.process.highLongOfPSN = 0;
	token.as.object.data.application.process.lowLongOfPSN = kCurrentProcess;
	result = GenesisAE_CreateTokenFromObjectData(cMyApplication, &token, outObjectOfDesiredClass,
													nullptr/* new type */);
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// ApplicationImplicit


/*!
Handles the following OSL object access:

	cMyMacroSet <- typeNull

References may be by name, or absolute
position.

(3.0)
*/
pascal OSErr
GetElementAE_MacroSetFromNull	(DescType			inDesiredClass,
								 AEDesc const*		UNUSED_ARGUMENT(inFromWhat),
								 DescType			inFromWhatClass,
								 DescType			inFormOfReference,
								 AEDesc const*		inReference,
								 AEDesc*			outObjectOfDesiredClass,
								 long				UNUSED_ARGUMENT(inData))
{
	OSErr   result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: requested macro set");
	if (inDesiredClass != cMyMacroSet)
	{
		result = errAEWrongDataType;
	}
	else if ((inFromWhatClass != typeNull) && (inFromWhatClass != cMyApplication))
	{
		result = errAENoSuchObject;
	}
	
	// create the appropriate object
	if (result == noErr)
	{
		switch (inFormOfReference)
		{
		case formAbsolutePosition:
			result = getMacroSetByAbsolutePosition(inReference, outObjectOfDesiredClass);
			break;
		
		case formName:
			//result = getMacroSetByName(inReference, outObjectOfDesiredClass);
			result = errAEBadTestKey;
			break;
		
		case formTest:
			// handled automatically by installed “count” and “compare” object callbacks
			break;
		
		default:
			result = errAEBadTestKey;
			break;
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// MacroSetFromNull


/*!
Handles the following OSL object access:

	cStringClass <- cMyMacroSet

References may only be by absolute position.

(3.0)
*/
pascal OSErr
GetElementAE_StringFromMacroSet	(DescType			inDesiredClass,
								 AEDesc const*		inFromWhat,
								 DescType			inFromWhatClass,
								 DescType			inFormOfReference,
								 AEDesc const*		inReference,
								 AEDesc*			outObjectOfDesiredClass,
								 long				UNUSED_ARGUMENT(inData))
{
	OSErr   result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: requested macro");
	if (inDesiredClass != cStringClass)
	{
		result = errAEWrongDataType;
	}
	else if (inFromWhatClass != cMyMacroSet)
	{
		result = errAENoSuchObject;
	}
	
	// create the appropriate object
	if (result == noErr)
	{
		switch (inFormOfReference)
		{
		case formAbsolutePosition:
			result = getMacroByAbsolutePosition(inReference, inFromWhat, outObjectOfDesiredClass);
			break;
		
		case formTest:
			// handled automatically by installed “count” and “compare” object callbacks
			break;
		
		default:
			result = errAEBadTestKey;
			break;
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// StringFromMacroSet


/*!
Handles the following OSL object accesses:

	cWindow <- typeNull
	cMyClipboardWindow <- typeNull
	cMyConnection <- typeNull
	cMySessionWindow <- typeNull
	cMyTerminalWindow <- typeNull

References may be by name, or absolute or
relative position.

(3.0)
*/
pascal OSErr
GetElementAE_WindowFromNull		(DescType			inDesiredClass,
								 AEDesc const*		inFromWhat,
								 DescType			inFromWhatClass,
								 DescType			inFormOfReference,
								 AEDesc const*		inReference,
								 AEDesc*			outObjectOfDesiredClass,
								 long				UNUSED_ARGUMENT(inData))
{
	OSErr	result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteValueFourChars("AppleScript: requested window-related class", inDesiredClass);
	if (!isClassObtainableFromWindow(inDesiredClass))
	{
		result = errAEWrongDataType;
	}
	else if ((inFromWhatClass != typeNull) && (inFromWhatClass != cMyApplication))
	{
		result = errAENoSuchObject;
	}
	
	// create the appropriate object
	if (result == noErr)
	{
		switch (inFormOfReference)
		{
		case formAbsolutePosition:
			result = getWindowByAbsolutePosition(inDesiredClass, inReference, outObjectOfDesiredClass);
			break;
		
		case formRelativePosition:
			result = getWindowByRelativePosition(inDesiredClass, inFromWhat, inReference,
													outObjectOfDesiredClass);
			break;
		
		case formName:
			result = getWindowByName(inDesiredClass, inReference, outObjectOfDesiredClass);
			break;
		
		case formTest:
			// handled automatically by installed “count” and “compare” object callbacks
			break;
		
		default:
			result = errAEBadTestKey;
			break;
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// WindowFromNull


#pragma mark Internal Methods

/*!
Creates a token for the specified type of object,
using window data.  This will only work if you
provide a desired class that can be derived from
a window (such as "cMyWindow", or "cMyConnection").
Since not all windows represent all objects,
obviously you must also provide a particular window
that can indeed be used to create the desired class
- for example, passing the Preferences window while
requesting a terminal window class won’t work!

(3.0)
*/
static OSStatus
createObjectFromWindow		(DescType		inDesiredClass,
							 WindowRef		inWindow,
							 AEDesc*		outObjectOfDesiredClass)
{
	OSStatus	result = noErr;
	
	
	if (isClassObtainableFromWindow(inDesiredClass))
	{
		ObjectClassesAE_Token   token;
		
		
		token.flags = 0;
		token.genericPointer = nullptr;
		switch (inDesiredClass)
		{
		case cMyClipboardWindow:
			if (Clipboard_ReturnWindow() == inWindow)
			{
				Console_WriteLine("apparent request: the clipboard window");
				token.as.object.data.clipboardWindow.windowClass.ref = inWindow;
			}
			else result = errAENoSuchObject;
			break;
		
		case cMyTerminalWindow:
			if (TerminalWindow_ExistsFor(inWindow))
			{
				Console_WriteLine("apparent request: a terminal window");
				token.as.object.data.terminalWindow.windowClass.ref = inWindow;
				token.as.object.data.terminalWindow.ref =
					TerminalWindow_ReturnFromWindow(inWindow);
			}
			else result = errAENoSuchObject;
			break;
		
		case cMyWindow:
			Console_WriteLine("apparent request: a window");
			token.as.object.data.window.ref = inWindow;
			break;
		
		default:
			result = errAEWrongDataType;
			break;
		}
		if (result == noErr)
		{
			Console_WriteLine("constructing token");
			result = GenesisAE_CreateTokenFromObjectData(inDesiredClass, &token,
															outObjectOfDesiredClass, nullptr/* new type */);
		}
	}
	else result = errAEWrongDataType;
	
	return result;
}// createObjectFromWindow


/*!
To obtain a macro class using "formAbsolutePosition"
selection data as a reference (that is, you know the
index of a macro in a macro set), use this method.
The index can be an integer (positive to go forward,
negative to go in reverse), or one of the following
constants:

	kAEFirst		the first macro
	kAELast			the last macro
	kAEMiddle		the middle macro
	kAEAny			the first macro
	kAEAll			a list of macros

MacTelnet currently identifies macros in the same
order as they appear in the macro editor.

The container type is expected to be a macro set.

(3.0)
*/
static OSStatus
getMacroByAbsolutePosition	(AEDesc const*		inReference,
							 AEDesc const*		inFromWhat,
							 AEDesc*			outObjectOfDesiredClass)
{
	OSStatus				result = noErr;
	ObjectClassesAE_Token   macroSetToken;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “macro” by absolute position");
	
	result = GenesisAE_CreateTokenFromObjectSpecifier(inFromWhat, &macroSetToken);
	
	if (result == noErr)
	{
		Boolean			handled = false;	// recognized request?
		Boolean			isNotAList = true;	// is the resultant object a single set, not a list of sets?
		SInt16			macroSetNumber = 0;	// used for convenience
		MacroIndex		finalMacroIndex = 0;
		
		
		macroSetNumber = macroSetToken.as.object.data.macroSet.number;
		Console_WriteValue("requested macro from macro set number", macroSetNumber);
		if (inReference->descriptorType == typeSInt32)
		{
			SInt32		macroIndex = 0L;
			
			
			Console_WriteLine("using a number");
			result = AppleEventUtilities_CopyDescriptorDataAs(typeSInt32, inReference, &macroIndex, sizeof(macroIndex),
																nullptr/* actual size storage */);
			if (result != noErr)
			{
				Console_WriteLine("could not obtain that number");
			}
			else
			{
				handled = true;
				
				Console_WriteValue("macro index", macroIndex);
				
				// now decide what macro set is being specified; negative indices mean “search in reverse”
				if (macroIndex == 0)
				{
					result = errAEIllegalIndex;
				}
				else if (macroIndex > 0)
				{
					if (macroIndex > MACRO_COUNT)
					{
						Console_WriteLine("WARNING: unexpectedly large macro index, bailing out...");
						result = errAEIllegalIndex;
					}
					else
					{
						finalMacroIndex = macroIndex - 1; // zero-based
					}
				}
				else
				{
					if (macroIndex < -MACRO_COUNT)
					{
						Console_WriteLine("WARNING: unexpectedly small macro index, bailing out...");
						result = errAEIllegalIndex;
					}
					else
					{
						finalMacroIndex = MACRO_COUNT + macroIndex; // zero-based
					}
				}
			}
		}
		else if (inReference->descriptorType == typeAbsoluteOrdinal)
		{
			DescType	ordinal = kAEFirst;
			
			
			Console_WriteLine("using an absolute ordinal");
			result = AppleEventUtilities_CopyDescriptorData(inReference, &ordinal, sizeof(ordinal),
															nullptr/* actual size storage */);
			if (result != noErr)
			{
				Console_WriteLine("could not obtain that ordinal");
			}
			else
			{
				handled = true;
				
				Console_WriteValueFourChars("ordinal", ordinal);
				
				switch (ordinal)
				{
				case kAEFirst:
				case kAEAny:
					Console_WriteLine("(first or any macro in a set)");
					finalMacroIndex = 0;
					break;
				
				case kAELast:
					Console_WriteLine("(last macro in a set)");
					finalMacroIndex = MACRO_COUNT - 1;
					break;
				
				case kAEMiddle:
					Console_WriteLine("(middle macro in a set)");
					finalMacroIndex = INTEGER_HALVED(MACRO_COUNT);
					break;
				
				case kAEAll:
					Console_WriteLine("(all macros in a set)");
					isNotAList = false;
					{
						AEDesc		macroDesc;
						
						
						result = AppleEventUtilities_InitAEDesc(&macroDesc);
						assert_noerr(result);
						
						Console_WriteLine("creating a list");
						result = AECreateList(nullptr/* factoring */, 0/* size */,
												false/* is a record */, outObjectOfDesiredClass);
						if (result == noErr)
						{
							UInt16				oldSetNumber = Macros_GetActiveSetNumber();
							register SInt16		i = 0;
							char				macroData[255]; // TEMPORARY (arbitrary size restriction)
							
							
							Macros_SetActiveSetNumber(macroSetNumber);
							for (i = 0; i < MACRO_COUNT; ++i)
							{
								// add each macro to the list
								Console_WriteLine("new macro descriptor");
								Macros_Get(Macros_GetActiveSet(), i, macroData, sizeof(macroData));
								result = AECreateDesc(cStringClass, macroData, CPP_STD::strlen(macroData),
														&macroDesc);
								Console_WriteValue("adding to list if zero:", result);
								if (result == noErr)
								{
									result = AEPutDesc(outObjectOfDesiredClass, 0/* index */, &macroDesc);
									if (result == noErr) result = AEDisposeDesc(&macroDesc);
								}
							}
							
							Macros_SetActiveSetNumber(oldSetNumber);
						}
					}
					break;
				
				default:
					// ???
					result = errAENoSuchObject;
					break;
				}
			}
		}
		else
		{
			Console_WriteValueFourChars("unknown form of reference", inReference->descriptorType);
		}
		
		unless (handled) result = errAENoSuchObject; // no other types of reference are supported
		
		// if the object was resolved to a macro, create the string
		if ((handled) && (result == noErr) && (isNotAList))
		{
			char	macroData[255]; // TEMPORARY (arbitrary size restriction)
			UInt16  oldSetNumber = Macros_GetActiveSetNumber();
			
			
			Console_WriteLine("constructing macro string");
			Macros_Get(Macros_GetActiveSet(), finalMacroIndex, macroData, sizeof(macroData));
			result = AECreateDesc(cStringClass, macroData, CPP_STD::strlen(macroData) * sizeof(UInt8),
									outObjectOfDesiredClass);
			Macros_SetActiveSetNumber(oldSetNumber);
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// getMacroByAbsolutePosition


/*!
To obtain a macro set class using "formAbsolutePosition"
selection data as a reference (that is, you know the
index of a macro set in the macro set list), use this
method.  The index can be an integer (positive to go
forward, negative to go in reverse), or one of the
following constants:

	kAEFirst		the first macro set
	kAELast			the last macro set
	kAEMiddle		the middle macro set
	kAEAny			the first macro set
	kAEAll			a list of macro sets

MacTelnet currently identifies macro sets in the same
order as they appear in the macro editor.

(3.0)
*/
static OSStatus
getMacroSetByAbsolutePosition	(AEDesc const*		inReference,
								 AEDesc*			outObjectOfDesiredClass)
{
	OSStatus				result = noErr;
	Boolean					handled = false,	// recognized request?
							isNotAList = true;	// is the resultant object a single set, not a list of sets?
	ObjectClassesAE_Token   token;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “macro set” by absolute position");
	
	token.flags = 0;
	token.genericPointer = nullptr;
	if (inReference->descriptorType == typeSInt32)
	{
		SInt32		macroSetIndex = 0L;
		
		
		Console_WriteLine("using a number");
		result = AppleEventUtilities_CopyDescriptorDataAs(typeSInt32, inReference, &macroSetIndex, sizeof(macroSetIndex),
															nullptr/* actual size storage */);
		if (result != noErr)
		{
			Console_WriteLine("could not obtain that number");
		}
		else
		{
			handled = true;
			
			Console_WriteValue("macro set index", macroSetIndex);
			
			// now decide what macro set is being specified; negative indices mean “search in reverse”
			if (macroSetIndex > 0)
			{
				if (macroSetIndex > MACRO_SET_COUNT)
				{
					Console_WriteLine("unexpectedly large macro set index, bailing out...");
					result = errAEIllegalIndex;
				}
				else
				{
					UInt16		oldSetNumber = Macros_GetActiveSetNumber();
					
					
					Macros_SetActiveSetNumber(STATIC_CAST(macroSetIndex, UInt16));
					token.as.object.data.macroSet.number = Macros_GetActiveSetNumber();
					Macros_SetActiveSetNumber(oldSetNumber);
				}
			}
			else
			{
				Console_WriteLine("(negative value - unimplemented)");
				result = errAEIllegalIndex;
			}
		}
	}
	else if (inReference->descriptorType == typeAbsoluteOrdinal)
	{
		DescType	ordinal = kAEFirst;
		
		
		Console_WriteLine("using an absolute ordinal");
		result = AppleEventUtilities_CopyDescriptorData(inReference, &ordinal, sizeof(ordinal),
														nullptr/* actual size storage */);
		if (result != noErr)
		{
			Console_WriteLine("could not obtain that ordinal");
		}
		else
		{
			handled = true;
			
			Console_WriteValueFourChars("ordinal", ordinal);
			
			switch (ordinal)
			{
			case kAEFirst:
			case kAEAny:
				Console_WriteLine("(first or any macro set)");
				{
					UInt16		oldSetNumber = Macros_GetActiveSetNumber();
					
					
					Macros_SetActiveSetNumber(1);
					token.as.object.data.macroSet.number = Macros_GetActiveSetNumber();
					Macros_SetActiveSetNumber(oldSetNumber);
				}
				break;
			
			case kAELast:
				Console_WriteLine("(last macro set)");
				{
					UInt16		oldSetNumber = Macros_GetActiveSetNumber();
					
					
					Macros_SetActiveSetNumber(MACRO_SET_COUNT);
					token.as.object.data.macroSet.number = Macros_GetActiveSetNumber();
					Macros_SetActiveSetNumber(oldSetNumber);
				}
				break;
			
			case kAEMiddle:
				Console_WriteLine("(middle macro set)");
				{
					UInt16		oldSetNumber = Macros_GetActiveSetNumber();
					
					
					Macros_SetActiveSetNumber(1 + INTEGER_HALVED(MACRO_SET_COUNT));
					token.as.object.data.macroSet.number = Macros_GetActiveSetNumber();
					Macros_SetActiveSetNumber(oldSetNumber);
				}
				break;
			
			case kAEAll:
				Console_WriteLine("(all macro sets)");
				isNotAList = false;
				{
					AEDesc		macroSetDesc;
					
					
					result = AppleEventUtilities_InitAEDesc(&macroSetDesc);
					assert_noerr(result);
					
					Console_WriteLine("creating a list");
					result = AECreateList(nullptr/* factoring */, 0/* size */,
											false/* is a record */, outObjectOfDesiredClass);
					if (result == noErr)
					{
						UInt16				oldSetNumber = Macros_GetActiveSetNumber();
						register UInt16		i = 0;
						
						
						for (i = 1; i <= MACRO_SET_COUNT; ++i)
						{
							// set up the token
							Macros_SetActiveSetNumber(i);
							token.as.object.data.macroSet.number = Macros_GetActiveSetNumber();
							
							// add each macro set to the list
							Console_WriteLine("new macro set descriptor");
							result = GenesisAE_CreateTokenFromObjectData(cMyMacroSet, &token,
																			&macroSetDesc,
																			nullptr/* new type */);
							Console_WriteValue("adding to list if zero:", result);
							if (result == noErr)
							{
								result = AEPutDesc(outObjectOfDesiredClass, 0/* index */,
													&macroSetDesc);
								if (result == noErr) result = AEDisposeDesc(&macroSetDesc);
							}
						}
						
						Macros_SetActiveSetNumber(oldSetNumber);
					}
				}
				break;
			
			default:
				// ???
				result = errAENoSuchObject;
				break;
			}
		}
	}
	else
	{
		Console_WriteValueFourChars("unknown form of reference", inReference->descriptorType);
	}
	
	unless (handled) result = errAENoSuchObject; // no other types of reference are supported
	
	// if the object was resolved to a macro set, create the token
	if ((handled) && (result == noErr) && (isNotAList))
	{
		Console_WriteLine("constructing macro set token");
		result = GenesisAE_CreateTokenFromObjectData(cMyMacroSet, &token, outObjectOfDesiredClass,
														nullptr/* new type */);
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// getMacroSetByAbsolutePosition


/*!
Creates a window object (or the specified class) using
"formAbsolutePosition" selection data as a reference.
If the specified class cannot be created from a window,
an error is returned.

The index can be an integer (positive to go forward,
negative to go in reverse), or one of the following
constants:

	kAEFirst		the first window
	kAELast			the last window
	kAEMiddle		the middle window
	kAEAny			the frontmost window
	kAEAll			a list of windows

MacTelnet currently identifies the frontmost window as
"window 1", and other windows in the window list order
relative to the frontmost window.

If a window is identified as having a more particular
type, it is automatically returned as the most specific
possible type (e.g. a "terminal window").

(3.0)
*/
static OSStatus
getWindowByAbsolutePosition		(DescType			inDesiredClass,
								 AEDesc const*		inReference,
								 AEDesc*			outObjectOfDesiredClass)
{
	OSStatus			result = noErr;
	WindowGroupRef		documentWindowGroup = GetWindowGroupOfClass(kDocumentWindowClass);
	WindowGroupRef		floatingWindowGroup = GetWindowGroupOfClass(kFloatingWindowClass);
	ItemCount const		kTotalDocumentWindows = CountWindowGroupContents(documentWindowGroup, kWindowGroupContentsReturnWindows);
	ItemCount const		kTotalFloatingWindows = CountWindowGroupContents(floatingWindowGroup, kWindowGroupContentsReturnWindows);
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “window” by absolute position");
	if ((kTotalDocumentWindows > 0) || (kTotalFloatingWindows > 0))
	{
		Boolean			handled = false;	// recognized request?
		Boolean			isNotAList = true;	// is the resultant object a single window, not a list of windows?
		WindowRef		finalWindow = nullptr;	// window to use for token
		
		
		if (inReference->descriptorType == typeSInt32)
		{
			SInt32		windowIndex = 0L;
			
			
			Console_WriteLine("using a number");
			result = AppleEventUtilities_CopyDescriptorDataAs(typeSInt32, inReference, &windowIndex, sizeof(windowIndex),
																nullptr/* actual size storage */);
			if (result != noErr)
			{
				Console_WriteLine("could not obtain that number");
			}
			else
			{
				// the index is “virtual”, and is defined as the index into the
				// sum of the document and floating window counts, traversing
				// those lists in order; therefore, the index of (1 + number of
				// document windows) is the first floating window, etc.
				handled = true;
				
				Console_WriteValue("window index", windowIndex);
				
				// decide what window is being specified; negative indices mean “search in reverse”
				if (windowIndex > 0)
				{
					SInt32 const	kDocumentIndex = windowIndex;
					SInt32 const	kFloatingIndex = windowIndex - kTotalDocumentWindows;
					
					
					finalWindow = nullptr;
					if (windowIndex > (kTotalDocumentWindows + kTotalFloatingWindows))
					{
						Console_WriteLine("WARNING: unexpectedly large window index, bailing out...");
						result = errAEIllegalIndex;
					}
					else
					{
						if ((kTotalDocumentWindows > 0) && (kFloatingIndex < 1))
						{
							result = GetIndexedWindow(documentWindowGroup, kDocumentIndex, 0/* flags */, &finalWindow);
						}
						else
						{
							result = GetIndexedWindow(floatingWindowGroup, kFloatingIndex, 0/* flags */, &finalWindow);
						}
					}
				}
				else
				{
					Console_WriteLine("negative index; unimplemented");
					result = errAEIllegalIndex;
				}
			}
		}
		else if (inReference->descriptorType == typeAbsoluteOrdinal)
		{
			DescType	ordinal = kAEFirst;
			
			
			Console_WriteLine("using an absolute ordinal");
			result = AppleEventUtilities_CopyDescriptorData(inReference, &ordinal, sizeof(ordinal),
															nullptr/* actual size storage */);
			if (result != noErr)
			{
				Console_WriteLine("could not obtain that ordinal");
			}
			else
			{
				handled = true;
				
				Console_WriteValueFourChars("ordinal", ordinal);
				
				switch (ordinal)
				{
				case kAEFirst:
				case kAEAny:
					Console_WriteLine("(first or any window)");
					finalWindow = nullptr;
					if (kTotalDocumentWindows > 0)
					{
						result = GetIndexedWindow(documentWindowGroup, 1/* index */, 0/* flags */, &finalWindow);
					}
					if ((result != noErr) || (finalWindow == nullptr))
					{
						result = GetIndexedWindow(floatingWindowGroup, 1/* index */, 0/* flags */, &finalWindow);
					}
					break;
				
				case kAELast:
					// return the last window in the list
					Console_WriteLine("(last window)");
					finalWindow = nullptr;
					if (kTotalDocumentWindows > 0)
					{
						result = GetIndexedWindow(documentWindowGroup, kTotalDocumentWindows/* index */, 0/* flags */,
													&finalWindow);
					}
					if ((result != noErr) || (finalWindow == nullptr))
					{
						result = GetIndexedWindow(floatingWindowGroup, kTotalFloatingWindows/* index */, 0/* flags */,
													&finalWindow);
					}
					break;
				
				case kAEMiddle:
					Console_WriteLine("(middle window)");
					{
						SInt32 const	kMiddleIndex = 1 + INTEGER_HALVED(kTotalDocumentWindows + kTotalFloatingWindows);
						
						
						// the lists overlap; if the middle is in the floating window list,
						// use that; otherwise, use the document window list
						if (kMiddleIndex > kTotalDocumentWindows)
						{
							result = GetIndexedWindow(floatingWindowGroup, kMiddleIndex, 0/* flags */, &finalWindow);
						}
						else
						{
							result = GetIndexedWindow(documentWindowGroup, kMiddleIndex, 0/* flags */, &finalWindow);
						}
					}
					break;
				
				case kAEAll:
					Console_WriteLine("(all windows)");
					isNotAList = false;
					{
						WindowRef*		windowArray = REINTERPRET_CAST
														(CFAllocatorAllocate(kCFAllocatorDefault,
																				(kTotalDocumentWindows + kTotalFloatingWindows) *
																					sizeof(WindowRef),
																				0/* hints */),
															WindowRef*);
						
						
						if (windowArray == nullptr) result = memFullErr;
						else
						{
							// the contents of 2 window groups will go into one array;
							// hence, the 2nd call offsets by the length of the first list
							result = GetWindowGroupContents(documentWindowGroup, kWindowGroupContentsReturnWindows,
															kTotalDocumentWindows, nullptr/* actual count */,
															REINTERPRET_CAST(windowArray, void**));
							if (result == noErr)
							{
								result = GetWindowGroupContents(floatingWindowGroup, kWindowGroupContentsReturnWindows,
																kTotalFloatingWindows, nullptr/* actual count */,
																REINTERPRET_CAST(windowArray + kTotalDocumentWindows, void**));
								if (result == noErr)
								{
									AEDesc		windowDesc;
									
									
									result = AppleEventUtilities_InitAEDesc(&windowDesc);
									assert_noerr(result);
									
									Console_WriteLine("creating a list");
									result = AECreateList(nullptr/* factoring */, 0/* size */,
															false/* is a record */, outObjectOfDesiredClass);
									if (result == noErr)
									{
										register SInt16		i = 0;
										
										
										for (i = 0; i < (kTotalDocumentWindows + kTotalFloatingWindows); ++i)
										{
											// add each window to the list
											Console_WriteLine("new object in list");
											result = createObjectFromWindow(inDesiredClass, windowArray[i], &windowDesc);
											Console_WriteValue("adding to list if zero:", result);
											if (result == noErr)
											{
												result = AEPutDesc(outObjectOfDesiredClass, 0/* index */, &windowDesc);
												if (result == noErr) result = AEDisposeDesc(&windowDesc);
											}
										}
									}
								}
							}
							CFAllocatorDeallocate(kCFAllocatorDefault, windowArray), windowArray = nullptr;
						}
					}
					break;
				
				default:
					// ???
					result = errAENoSuchObject;
					break;
				}
			}
		}
		else
		{
			Console_WriteValueFourChars("unknown form of reference", inReference->descriptorType);
		}
		
		unless (handled) result = errAENoSuchObject; // no other types of reference are supported
		
		// if the object was resolved to a window, create the token
		if ((handled) && (result == noErr) && (isNotAList))
		{
			result = createObjectFromWindow(inDesiredClass, finalWindow, outObjectOfDesiredClass);
		}
	}
	else result = errAENoSuchObject;
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// getWindowByAbsolutePosition


/*!
Creates a window object (or the specified class) using
"formName" selection data as a reference.  If the
specified class cannot be created from a window, an
error is returned.

(3.0)
*/
static OSStatus
getWindowByName			(DescType			inDesiredClass,
						 AEDesc const*		inReference,
						 AEDesc*			outObjectOfDesiredClass)
{
	OSStatus	result = noErr;
	Str255		name;
	Size		actualSize = 0L;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “window” by name");
	result = AppleEventUtilities_CopyDescriptorDataAs(cStringClass, inReference, name + 1,
														255 * sizeof(UInt8), &actualSize);
	
	if (result == noErr)
	{
		name[0] = STATIC_CAST(actualSize / sizeof(UInt8), UInt8);
		
		Console_WriteValuePString("looking for window with name", name);
		{
			WindowRef			window = GetFrontWindowOfClass(kDocumentWindowClass, true/* must be visible */);
			Str255				comparedName;
			SInt16				windowCount = getWindowCount();
			register SInt16		i = 0;
			
			
			result = errAENoSuchObject;
			for (i = 1; i <= windowCount; ++i)
			{
				Console_WriteValuePString("comparing with window of name", comparedName);
				GetWTitle(window, comparedName);
				if (EqualString(comparedName, name, false/* case sensitive */, true/* diacritics sensitive */))
				{
					Console_WriteValuePString("found desired window", name);
					result = createObjectFromWindow(inDesiredClass, window, outObjectOfDesiredClass);
					break;
				}
				window = GetNextWindowOfClass(window, kDocumentWindowClass, true/* must be visible */);
			}
		}
	}
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// getWindowByName


/*!
Creates a window object (or the specified class) using
"formRelativePosition" selection data as a reference.
If the specified class cannot be created from a window,
an error is returned.

(3.0)
*/
static OSStatus
getWindowByRelativePosition		(DescType			inDesiredClass,
								 AEDesc const*		inRelativeToWhat,
								 AEDesc const*		inReference,
								 AEDesc*			outObjectOfDesiredClass)
{
	OSStatus	result = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “window” by relative position");
	if (EventLoop_GetRealFrontWindow() != nullptr)
	{
		DescType	windowRelationship = 0;	// kAENext or kAEPrevious
		WindowRef	window = nullptr;
		
		
		result = AppleEventUtilities_CopyDescriptorData(inRelativeToWhat, &window, sizeof(window),
														nullptr/* actual size storage */);
		if (result == noErr) result = AppleEventUtilities_CopyDescriptorData(inReference, &windowRelationship,
																				sizeof(windowRelationship),
																				nullptr/* actual size storage */);
		if ((inReference->descriptorType == typeEnumerated) &&
			(result == noErr)) // was a next or previous relationship given?
		{
			// set up the object based on the desired relationship
			switch (windowRelationship)
			{
			case kAENext:
				Console_WriteLine("next window");
				window = GetNextWindow(window);
				break;
			
			case kAEPrevious:
				Console_WriteLine("previous window (unimplemented)");
				result = errAENoSuchObject;
				break;
			
			default:
				// ???
				result = errAENoSuchObject;
				break;
			}
			
			if (result == noErr)
			{
				result = createObjectFromWindow(inDesiredClass, window, outObjectOfDesiredClass);
			}
		}
		else result = errAENoSuchObject;
	}
	else result = errAENoSuchObject;
	
	Console_WriteValue("result", result);
	Console_EndFunction();
	return result;
}// getWindowByRelativePosition


/*!
To determine how many windows there are
in the application window list, use this
method.

(3.0)
*/
static SInt16
getWindowCount ()
{
	WindowRef	window = nullptr;
	SInt16     	result = 0;
	
	
	window = GetFrontWindowOfClass(kDocumentWindowClass, true/* must be visible */);
	while (window != nullptr)
	{
		++result;
		window = GetNextWindowOfClass(window, kDocumentWindowClass, true/* must be visible */);
	}
	return result;
}// getWindowCount


/*!
Returns "true" only if the specified class can
be completely defined using window data.

(3.0)
*/
static Boolean
isClassObtainableFromWindow		(DescType	inDesiredClass)
{
	Boolean		result = false;
	
	
	switch (inDesiredClass)
	{
	case cMyClipboardWindow:
	case cMyConnection:
	case cMySessionWindow:
	case cMyTerminalWindow:
	case cMyWindow:
		result = true;
		break;
	
	default:
		break;
	}
	return result;
}// isClassObtainableFromWindow

// BELOW IS REQUIRED NEWLINE TO END FILE
