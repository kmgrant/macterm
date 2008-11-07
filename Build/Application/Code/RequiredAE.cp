/*###############################################################

	RequiredAE.cp
	
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

// Unix includes
#include <strings.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <Console.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// resource includes
#include "GeneralResources.h"

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "AppResources.h"
#include "BasicTypesAE.h"
#include "Clipboard.h"
#include "Commands.h"
#include "ConnectionData.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "FileUtilities.h"
#include "MacroManager.h"
#include "MainEntryPoint.h"
#include "ObjectClassesAE.h"
#include "Preferences.h"
#include "PrefsWindow.h"
#include "QuillsSession.h"
#include "RecordAE.h"
#include "RequiredAE.h"
#include "Session.h"
#include "SessionDescription.h"
#include "SessionFactory.h"
#include "TerminalFile.h"
#include "TerminalView.h"
#include "Terminology.h"
#include "UIStrings.h"



#pragma mark Internal Method Prototypes

static OSStatus		handleQuit										(AppleEvent const*, AppleEvent*, DescType);
static void			moveWindowAndDisplayTerminationAlertSessionOp	(SessionRef, void*, SInt32, void*);
static void			receiveTerminationWarningAnswer					(ListenerModel_Ref, ListenerModel_Event, void*, void*);
static void			setTerminalWindowTranslucency					(TerminalWindowRef, void*, SInt32, void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	AppleEvent const*			gCurrentQuitAppleEventPtr = nullptr;
	Boolean						gCurrentQuitCancelled = false;
	ListenerModel_ListenerRef	gCurrentQuitWarningAnswerListener = nullptr;
}



#pragma mark Public Methods

/*!
Handles the following event types:

kAppearanceEventClass
---------------------
	kAEAppearanceChanged
	kAESystemFontChanged
	kAESmallSystemFontChanged
	kAEViewsFontChanged

(3.0)
*/
pascal OSErr
RequiredAE_HandleAppearanceSwitch	(AppleEvent const*		inAppleEventPtr,
									 AppleEvent*			outReplyAppleEventPtr,
									 SInt32					UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	DescType		returnedType = '----';
	AEEventID		eventID = '----';
	Size			actualSize = 0;
	
	
	error = AEGetAttributePtr(inAppleEventPtr, keyEventIDAttr, typeType, &returnedType, 
								&eventID, sizeof(eventID), &actualSize);
	if (error == noErr)
	{
		switch (eventID)
		{
		case kAEViewsFontChanged:
			// tell everything that cares to respond to this event;
			// but be careful when sending errors back to the
			// Apple Event manager - it should only be sent errors
			// that it can do something about
			error = noErr;
			break;
		
		case kAEAppearanceChanged:
		case kAESystemFontChanged:
		case kAESmallSystemFontChanged:
		default:
			error = errAEEventNotHandled;
			break;
		}
	}
	
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandleAppearanceSwitch


/*!
Handles the following event types:

kASRequiredSuite
----------------
	kAEOpenApplication

(3.0)
*/
pascal OSErr
RequiredAE_HandleApplicationOpen	(AppleEvent const*	UNUSED_ARGUMENT(inAppleEventPtr),
									 AppleEvent*		outReplyAppleEventPtr,
									 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	Boolean			quellAutoNew = false;
	size_t			actualSize = 0L;
	
	
	error = noErr;
	
	// get the user’s “don’t auto-new” application preference, if possible
	if (Preferences_GetData(kPreferences_TagDontAutoNewOnApplicationReopen, sizeof(quellAutoNew),
							&quellAutoNew, &actualSize) !=
		kPreferences_ResultOK)
	{
		// assume a value if it cannot be found
		quellAutoNew = false;
	}
	
	// start by spawning a new shell most of the time;
	// however, this can be forced to skip if another
	// thread has processed an override action (such
	// as handling a URL from Launch Services that would
	// spawn a unique shell anyway)
	unless ((quellAutoNew) || FlagManager_Test(kFlagUserOverrideAutoNew))
	{
		Commands_ExecuteByIDUsingEvent(kCommandNewSessionShell);
	}
	
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandleApplicationOpen


/*!
Handles the following event types:

kASRequiredSuite
----------------
	kAEShowPreferences

(3.0)
*/
pascal OSErr
RequiredAE_HandleApplicationPreferences		(AppleEvent const*	UNUSED_ARGUMENT(inAppleEventPtr),
											 AppleEvent*		outReplyAppleEventPtr,
											 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	// display the Preferences window
	PrefsWindow_Display();
	error = noErr;
	
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandleApplicationPreferences


/*!
Handles the following event types:

kASRequiredSuite
----------------
	kAEReopenApplication

(3.0)
*/
pascal OSErr
RequiredAE_HandleApplicationReopen	(AppleEvent const*	UNUSED_ARGUMENT(inAppleEventPtr),
									 AppleEvent*		outReplyAppleEventPtr,
									 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	Boolean			quellAutoNew = false;
	size_t			actualSize = 0L;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “application re-opened” event");
	
	// get the user’s “don’t auto-new” application preference, if possible
	if (Preferences_GetData(kPreferences_TagDontAutoNewOnApplicationReopen, sizeof(quellAutoNew),
							&quellAutoNew, &actualSize) !=
		kPreferences_ResultOK)
	{
		// assume a value if it cannot be found
		quellAutoNew = false;
	}
	
	unless (quellAutoNew)
	{
		// handle the case where MacTelnet has no open windows and the user
		// double-clicks its icon in the Finder
		if (EventLoop_ReturnRealFrontWindow() == nullptr)
		{
			// no open windows - respond by spawning a shell
			Commands_ExecuteByID(kCommandNewSessionShell);
		}
	}
	
	Console_WriteValue("result", error);
	Console_EndFunction();
	
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandleApplicationReopen


/*!
Handles the following event types:

kASRequiredSuite
----------------
	kAEQuitApplication

(3.0)
*/
pascal OSErr
RequiredAE_HandleApplicationQuit	(AppleEvent const*	inAppleEventPtr,
									 AppleEvent*		outReplyAppleEventPtr,
									 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	// ignore if a prior Quit event is still being handled
	if (gCurrentQuitAppleEventPtr == nullptr)
	{
		gCurrentQuitAppleEventPtr = inAppleEventPtr;
		gCurrentQuitCancelled = false;
		
		// this callback changes the "gCurrentQuitCancelled" flag accordingly as session windows are visited
		gCurrentQuitWarningAnswerListener = ListenerModel_NewStandardListener(receiveTerminationWarningAnswer);
		
		// temp - somehow, this call produces an error, because this event specifies it has no direct object?
		//error = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
		if (error == noErr)
		{
			Size		actualSize = 0L;
			DescType	returnedType,
						saveOption = kAEAsk; // unless otherwise specified, ask whether to discard open windows
			
			
			// if the optional parameter was given as to whether to close
			// open windows without prompting the user, retrieve it
			error = AEGetParamPtr(inAppleEventPtr, keyAESaveOptions, typeEnumerated, &returnedType,
									&saveOption, sizeof(saveOption), &actualSize);
			if (error == errAEDescNotFound) error = noErr;
			if (error == noErr)
			{
				// kills all open connections (asking the user as appropriate), and if the
				// user never cancels, *flags* the main event loop to terminate cleanly
				// (never directly ExitToShell() in an Apple Event handler!)
				error = handleQuit(inAppleEventPtr, outReplyAppleEventPtr, saveOption);
			}
		}
		
		ListenerModel_ReleaseListener(&gCurrentQuitWarningAnswerListener);
		gCurrentQuitAppleEventPtr = nullptr;
	}
	
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandleApplicationQuit


/*!
Handles the following event types:

kASRequiredSuite
----------------
	kAEOpenDocuments

(3.0)
*/
pascal OSErr
RequiredAE_HandleOpenDocuments	(AppleEvent const*	inAppleEventPtr,
								 AppleEvent*		outReplyAppleEventPtr,
								 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	AEDescList		docList;
	
	
	if ((error = AEGetParamDesc(inAppleEventPtr, keyDirectObject, typeAEList, &docList)) == noErr)
	{
		// check for missing parameters
		error = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
		if (noErr == error)
		{
			long		numberOfItemsInList = 0L;
			
			
			// count the number of descriptor records in the list
			if ((error = AECountItems(&docList, &numberOfItemsInList)) == noErr)
			{
				LSItemInfoRecord	fileInfo;
				FSRef				fileRef;
				UInt16				setNumber = 0; // cumulative track of how many macro sets have been imported
				UInt16				macrosImportedSuccessfully = 0;
				AEKeyword			returnedKeyword = '----';
				DescType			returnedType = '----';
				Size				actualSize = 0;
				long				itemIndex = 0L;
				
				
				for (itemIndex = 1; itemIndex <= numberOfItemsInList; ++itemIndex)
				{
					error = AEGetNthPtr(&docList, itemIndex, typeFSRef, &returnedKeyword, &returnedType,
										&fileRef, sizeof(fileRef), &actualSize);
					if (noErr == error)
					{
						FSSpec			fileSpec;
						HFSUniStr255	nameBuffer;
						
						
						error = FSGetCatalogInfo(&fileRef, kFSCatInfoNone, nullptr/* catalog info */,
													&nameBuffer, &fileSpec, nullptr/* parent directory */);
						if (noErr == error)
						{
							CFRetainRelease		fileNameCFString = CFStringCreateWithCharacters
																	(kCFAllocatorDefault, nameBuffer.unicode,
																		nameBuffer.length);
							
							
							if (fileNameCFString.exists())
							{
								size_t const	kBufferSize = 1024;
								UInt8*			pathBuffer = new UInt8[kBufferSize];
								
								
								error = LSCopyItemInfoForRef(&fileRef, kLSRequestTypeCreator, &fileInfo);
								if (noErr != error)
								{
									bzero(&fileInfo, sizeof(fileInfo));
									error = noErr;
								}
							#if 1
								// pass the file to Python and see if any handlers are installed to open it;
								// in the future this may pass much more information to Quills, so that more
								// sophisticated handling decisions can be made
								{
									// it appears to be a text file; run the user’s preferred editor on it
									error = FSRefMakePath(&fileRef, pathBuffer, kBufferSize);
									if (error != noErr)
									{
										// TEMPORARY; this is an inappropriate response, since
										// it may not be the user that initiated this request;
										// really, an error must be attached/returned in the event
										Sound_StandardAlert();
									}
									else
									{
										Quills::Session::handle_file(REINTERPRET_CAST(pathBuffer, char const*));
									}
								}
							#else
								if (CFStringHasSuffix(fileNameCFString.returnCFStringRef(), CFSTR(".command")) ||
									CFStringHasSuffix(fileNameCFString.returnCFStringRef(), CFSTR(".tool")))
								{
									// it appears to be a shell script; run it
									error = FSRefMakePath(&fileRef, pathBuffer, kBufferSize);
									if (error != noErr)
									{
										// TEMPORARY; this is an inappropriate response, since
										// it may not be the user that initiated this request;
										// really, an error must be attached/returned in the event
										Sound_StandardAlert();
									}
									else
									{
										char const* const	kPathCString = REINTERPRET_CAST(pathBuffer, char const*);
										
										
										SessionFactory_NewSessionFromCommandFile(nullptr/* existing terminal window to use */,
																					kPathCString);
									}
								}
								else if (CFStringHasSuffix(fileNameCFString.returnCFStringRef(), CFSTR(".txt")))
								{
									// it appears to be a text file; run the user’s preferred editor on it
									error = FSRefMakePath(&fileRef, pathBuffer, kBufferSize);
									if (error != noErr)
									{
										// TEMPORARY; this is an inappropriate response, since
										// it may not be the user that initiated this request;
										// really, an error must be attached/returned in the event
										Sound_StandardAlert();
									}
									else
									{
										char const*		args[] =
														{
															getenv("EDITOR"),
															REINTERPRET_CAST(pathBuffer, char const*),
															nullptr/* terminator */
														};
										
										
										if ((args[1] == nullptr) || (0 == CPP_STD::strcmp(args[1], "")))
										{
											// no editor preference found; use the default
											args[0] = "/usr/bin/vi";
										}
										SessionFactory_NewSessionArbitraryCommand(nullptr/* existing terminal window to use */,
																					args/* arguments */);
									}
								}
								else if (CFStringHasSuffix(fileNameCFString.returnCFStringRef(), CFSTR(".term")))
								{
									// it appears to be a Terminal XML property list file; parse it
									error = FSRefMakePath(&fileRef, pathBuffer, kBufferSize);
									if (error != noErr)
									{
										// TEMPORARY; this is an inappropriate response, since
										// it may not be the user that initiated this request;
										// really, an error must be attached/returned in the event
										Sound_StandardAlert();
									}
									else
									{
										char const* const	kPathCString = REINTERPRET_CAST(pathBuffer, char const*);
										
										
										SessionFactory_NewSessionFromTerminalFile(nullptr/* existing terminal window to use */,
																					kPathCString);
									}
								}
								else if ((fileInfo.creator == 'ToyS' || fileInfo.filetype == 'osas') ||
											CFStringHasSuffix(fileNameCFString.returnCFStringRef(), CFSTR(".scpt")))
								{
									// it appears to be a script; run it
									AppleEventUtilities_ExecuteScriptFile(&fileSpec, true/* notify user of errors */);
								}
								else if ((fileInfo.creator == AppResources_ReturnCreatorCode() &&
												fileInfo.filetype == kApplicationFileTypeSessionDescription) ||
											CFStringHasSuffix(fileNameCFString.returnCFStringRef(), CFSTR(".session")))
								{
									// read a configuration set
									SessionDescription_ReadFromFile(&fileSpec);
								}
								else if ((//fileInfo.creator == AppResources_ReturnCreatorCode() &&
												fileInfo.filetype == kApplicationFileTypeMacroSet) ||
											CFStringHasSuffix(fileNameCFString.returnCFStringRef(), CFSTR(".macros")))
								{
									// read a macro set, replacing the next set if there is more than one
									UInt16		preservedActiveSetNumber = Macros_ReturnActiveSetNumber();
									
									
									// no more than the maximum number of macro sets can be simultaneously imported
									if (++setNumber <= MACRO_SET_COUNT)
									{
										MacroManager_InvocationMethod		mode = kMacroManager_InvocationMethodCommandDigit;
										
										
										Macros_SetActiveSetNumber(setNumber);
										macrosImportedSuccessfully += Macros_ImportFromText
																		(Macros_ReturnActiveSet(), &fileSpec, &mode);
										Macros_SetMode(mode);
										Macros_SetActiveSetNumber(preservedActiveSetNumber);
									}
								}
							#endif
								delete [] pathBuffer;
							}
						}
					}
				}
				
				if (error == noErr) error = AEDisposeDesc(&docList);
			}
		}
	}
	
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandleOpenDocuments


/*!
Handles the following event types:

kASRequiredSuite
----------------
	kAEPrintDocuments

(3.0)
*/
pascal OSErr
RequiredAE_HandlePrintDocuments		(AppleEvent const*	UNUSED_ARGUMENT(inAppleEventPtr),
									 AppleEvent*		outReplyAppleEventPtr,
									 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	// this program doesn’t print files, although this behavior should probably be corrected somehow
	error = errAEEventNotHandled;
	
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandlePrintDocuments


//
// MacTelnet Suite Apple Event handlers
//

/*!
Handles the following event types:

kMySuite
--------
	kTelnetEventIDAlert

(3.0)
*/
pascal OSErr
AppleEvents_HandleAlert		(AppleEvent const*	inAppleEventPtr,
							 AppleEvent*		outReplyAppleEventPtr,
							 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	DescType		returnedType = typeWildCard;
	Str255			dialogText;
	Str255			helpText;
	Size			actualSize = 0L;
	
	
	error = AEGetParamPtr(inAppleEventPtr, keyDirectObject, cString, &returnedType,
							dialogText + 1, sizeof(dialogText) - 1 * sizeof(UInt8), &actualSize);
	dialogText[0] = (UInt8)(actualSize / sizeof(UInt8));
	{
		InterfaceLibAlertRef	box = nullptr;
		
		
		box = Alert_New();
		if ((box != nullptr) && (error == noErr))
		{
			Str31		button1Name;
			Str31		button2Name;
			Str31		button3Name;
			
			
			// set up network disruption parameter: defaults to false if the parameter is not specified
			{
				Boolean		isDisruption = true;
				
				
				error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyNetworkDisruption, typeBoolean, &returnedType,
										&isDisruption, sizeof(isDisruption), &actualSize);
				// UNUSED - TEMPORARY
			}
			
			// set up speech parameter: defaults to false if the parameter is not specified
			{
				Boolean		isSpeech = false;
				
				
				error = AEGetParamPtr(inAppleEventPtr, keyAEParamMySpeech, typeBoolean, &returnedType,
										&isSpeech, sizeof(isSpeech), &actualSize);
				// This is currently erroneous in that it permanently sets the speech flag, affecting
				// future alerts.  This will be fixed in a future release.  Fortunately, the default
				// value is always reset for Apple Event alerts, so scripts will not know there is a
				// problem.
				Alert_SetUseSpeech(isSpeech);
			}
			
			// set up help text parameter: defaults to nothing if the parameter is not specified
			{
				error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyHelpText, cString, &returnedType,
										helpText + 1, sizeof(helpText) - 1 * sizeof(UInt8), &actualSize);
				helpText[0] = (UInt8)(actualSize / sizeof(UInt8));
				if (error != noErr) PLstrcpy(helpText, EMPTY_PSTRING);
				Alert_SetText(box, dialogText, helpText); // sets the required, main text also
			}
			
			// set up title parameter: defaults to nothing if the parameter is not specified;
			// there are many different ways the title string could be specified, so try
			// multiple data types to see which one works - last one found takes precedence
			{
				Str255		titleText;
				
				
				error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyName, cString, &returnedType,
										titleText + 1, sizeof(titleText) - 1 * sizeof(UInt8), &actualSize);
				titleText[0] = (UInt8)(actualSize / sizeof(UInt8));
				if (error == noErr)
				{
					CFStringRef		titleString = nullptr;
					
					
					titleString = CFStringCreateWithPascalString(kCFAllocatorDefault, titleText,
																	kCFStringEncodingMacRoman);
					Alert_SetTitleCFString(box, titleString);
					CFRelease(titleString);
				}
			}
			{
				CFStringRef		titleString = nullptr;
				
				
				error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyName, typeCFStringRef, &returnedType,
										&titleString, sizeof(titleString), &actualSize);
				if (error == noErr)
				{
					Alert_SetTitleCFString(box, titleString);
				}
			}
			
			// set up help button parameter: defaults to false if the parameter is not specified
			{
				Boolean		isHelpButton = false;
				
				
				error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyHelpButton, typeBoolean, &returnedType,
										&isHelpButton, sizeof(isHelpButton), &actualSize);
				Alert_SetHelpButton(box, isHelpButton);
			}
			
			Alert_SetParamsFor(box, kAlert_StyleOK);
			
			// set up type parameter: defaults to plain if the parameter is not specified
			{
				FourCharCode	enumeratedType = kTelnetEnumeratedClassAlertIconTypeNote;
				AlertType		realType = kAlertPlainAlert;
				
				
				error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyWithIcon, typeEnumerated, &returnedType,
										&enumeratedType, sizeof(enumeratedType), &actualSize);
				if (error == noErr)
				{
					switch (enumeratedType)
					{
					case kTelnetEnumeratedClassAlertIconTypeNote:
						realType = kAlertNoteAlert;
						break;
					
					case kTelnetEnumeratedClassAlertIconTypeStop:
						realType = kAlertStopAlert;
						break;
					
					case kTelnetEnumeratedClassAlertIconTypeCaution:
						realType = kAlertCautionAlert;
						break;
					
					default:
						realType = kAlertPlainAlert;
						break;
					}
				}
				Alert_SetType(box, realType);
			}
			
			// set up movability parameter: defaults to false if the parameter is not specified
			{
				Boolean		isMovable = true;
				
				
				error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyTitled, typeBoolean, &returnedType,
										&isMovable, sizeof(isMovable), &actualSize);
				Alert_SetMovable(box, isMovable);
			}
			
			// set up beep parameter just prior to displaying the box: defaults to false if not specified
			{
				Boolean		shouldBeep = false;
				
				
				error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyAlertSound, typeBoolean, &returnedType,
										&shouldBeep, sizeof(shouldBeep), &actualSize);
				if (shouldBeep) Sound_StandardAlert();
			}
			
			// set up buttons: defaults to single OK button if not specified
			{
				AEDescList		buttonNames;
				
				
				(OSErr)AppleEventUtilities_InitAEDesc(&buttonNames);
				
				error = AEGetParamDesc(inAppleEventPtr, keyAEParamMyButtons, typeAEList, &buttonNames);
				if (error != noErr)
				{
					// if no buttons are specified, ensure that the name string is set to the
					// proper value, so that it shows up as “OK” in the dialog reply record
					PLstrcpy(button1Name, "\pOK"); // TEMPORARY - LOCALIZE THIS
				}
				else
				{
					SInt32		numberOfButtons = 0L;
					
					
					error = AECountItems(&buttonNames, &numberOfButtons);
					if (error == noErr)
					{
						register SInt32		i = 0L;
						AEKeyword			returnedKeyword;
						SInt16				whichButton = kAlertStdAlertOKButton;
						StringPtr			whichString = nullptr;
						
						
						if (numberOfButtons > 3) numberOfButtons = 3; // this is the limit, what can I say?
						for (i = 1; i <= numberOfButtons; ++i)
						{
							// assign the appropriate button the text from the list,
							// in the order given; all button titles are saved, so
							// that the item hit can be returned, later, as a result
							switch (i)
							{
							case 3:
								whichButton = kAlertStdAlertOtherButton;
								whichString = button3Name;
								break;
							
							case 2:
								whichButton = kAlertStdAlertCancelButton;
								whichString = button2Name;
								break;
							
							case 1:
							default:
								whichButton = kAlertStdAlertOKButton;
								whichString = button1Name;
								break;
							}
							error = AEGetNthPtr(&buttonNames, i, cString, &returnedKeyword,
												&returnedType, (whichString + 1),
												(31) * sizeof(UInt8), // size of button strings
												&actualSize);
							if (error == noErr)
							{
								CFStringRef		buttonNameCFString = nullptr;
								
								
								if (whichString != nullptr)
								{
									whichString[0] = (UInt8)(actualSize / sizeof(UInt8));
									
									// from an AppleScript perspective, an empty string means
									// “skip middle button”; however, the Alert module expects
									// this to be requested by passing nullptr as the string value
									if ((i == 2) && (!PLstrcmp(whichString, EMPTY_PSTRING))) whichString = nullptr;
								}
								
								if (whichString != nullptr)
								{
									// TEMPORARY: string encoding should not be assumed
									buttonNameCFString = CFStringCreateWithPascalString(kCFAllocatorDefault, whichString,
																						kCFStringEncodingMacRoman);
								}
								
								// set button text regardless (if nullptr, the button is hidden)
								Alert_SetButtonText(box, whichButton, buttonNameCFString);
								
								if (buttonNameCFString != nullptr)
								{
									CFRelease(buttonNameCFString);
								}
							}
						}
					}
				}
			}
			
			error = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
			if (error == noErr)
			{
				Alert_Display(box);
				
				// check to see if there is a place to put the result - if so, return the button hit
				if (AppleEventUtilities_IsValidReply(outReplyAppleEventPtr))
				{
					SInt16					itemHit = Alert_ItemHit(box);
					ObjectClassesAE_Token   token;
					AEDesc					recordDesc;
					
					
					// initialize defaults first
					bzero(&token, sizeof(token)); // temp
					token.flags = 0;
					token.genericPointer = nullptr;
					PLstrcpy(token.as.object.data.dialogReply.buttonName, EMPTY_PSTRING);
					token.as.object.data.dialogReply.gaveUp = false;
					
					// return "?" for the Help button, an empty string if the alert times out, or
					// the name of the button hit otherwise
					if (itemHit == kAlertStdAlertHelpButton)
					{
						PLstrcpy(token.as.object.data.dialogReply.buttonName, "\p?");
					}
					else if (itemHit == kAlertStdAlertOKButton)
					{
						PLstrcpy(token.as.object.data.dialogReply.buttonName, button1Name);
					}
					else if (itemHit == kAlertStdAlertCancelButton)
					{
						PLstrcpy(token.as.object.data.dialogReply.buttonName, button2Name);
					}
					else if (itemHit == kAlertStdAlertOtherButton)
					{
						PLstrcpy(token.as.object.data.dialogReply.buttonName, button3Name);
					}
					else
					{
						token.as.object.data.dialogReply.gaveUp = true;
					}
					
					error = AECreateList(nullptr/* factoring */, 0/* size */, true/* is a record */, &recordDesc);
					if (error == noErr)
					{
						error = AEPutKeyPtr(&recordDesc, pMyDialogReplyButtonReturned, typeChar,
											token.as.object.data.dialogReply.buttonName + 1,
											PLstrlen(token.as.object.data.dialogReply.buttonName));
					}
					if (error == noErr)
					{
						error = AEPutKeyPtr(&recordDesc, pMyDialogReplyGaveUp, typeBoolean,
											&token.as.object.data.dialogReply.gaveUp,
											sizeof(token.as.object.data.dialogReply.gaveUp));
					}
					if (error == noErr) error = AEPutParamDesc(outReplyAppleEventPtr, keyDirectObject, &recordDesc);
				}
			}
			
			Alert_Dispose(&box);
		}
	}
	
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandleAlert


/*!
Handles the following event types:

kMySuite
--------
	kTelnetEventIDCopyToClipboard

(3.0)
*/
pascal OSErr
AppleEvents_HandleCopySelectedText	(AppleEvent const*	inAppleEventPtr,
									 AppleEvent*		outReplyAppleEventPtr,
									 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = noErr;
	
	
	Console_BeginFunction();
	Console_WriteLine("AppleScript: “copy selected text” event");
	
	// temp - somehow, this call produces an error, because this event specifies it has no direct object?
	//error = AppleEventUtilities_RequiredParametersError(inAppleEventPtr);
	Console_WriteValue("required parameters error", error);
	if (error == noErr)
	{
		Size		actualSize = 0L;
		DescType	returnedType;
		Boolean		tableOption = false;
		
		
		// if the optional parameter was given as to whether to copy
		// using tabs for spaces, retrieve it
		error = AEGetParamPtr(inAppleEventPtr, keyAEParamMyTabsInPlaceOfMultipleSpaces, typeBoolean, &returnedType,
								&tableOption, sizeof(tableOption), &actualSize);
		Console_WriteValue("get parameter error", error);
		if (error == errAEDescNotFound) error = noErr;
		if (error == noErr)
		{
			// copy the selected text from the frontmost window
			TerminalViewRef		view = TerminalView_ReturnUserFocusTerminalView();
			
			
			if (nullptr != view)
			{
				Clipboard_TextToScrap(view, (tableOption) ? kClipboard_CopyMethodTable : kClipboard_CopyMethodStandard);
			}
			else
			{
				// error
				error = eventNotHandledErr;
			}
		}
	}
	
	Console_EndFunction();
	
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandleCopySelectedText


/*!
Handles the following event types:

kMySuite
--------
	kTelnetEventIDLaunchFind

(3.0)
*/
pascal OSErr
AppleEvents_HandleLaunchFind	(AppleEvent const*	UNUSED_ARGUMENT(inAppleEventPtr),
								 AppleEvent*		outReplyAppleEventPtr,
								 SInt32				UNUSED_ARGUMENT(inData))
{
	OSErr const		result = noErr; // errors are ALWAYS returned via the reply record
	OSStatus		error = resNotFound;
	
	
	// TEMPORARY - placeholder, this whole event will just be removed eventually
	
	(OSStatus)AppleEventUtilities_AddErrorToReply(nullptr/* message */, error, outReplyAppleEventPtr);
	return result;
}// HandleLaunchFind


#pragma mark Internal Methods

/*!
Handles quitting.  If "inSaveOption" is "kAEAsk", the user is
prompted whether to review unsaved sessions before quitting.
Other valid values are "kAEYes" (save all), and "kAENo" (save
nothing).

If the user chooses to review changes, then the given Apple
Event is suspended until the user has answered one alert for
each unsaved window (or, until the user cancels an alert).

If the application shutdown occurs successfully, "noErr" is
returned; otherwise, "userCanceledErr" is returned.

WARNING:	This routine expects to handle a “quit application”
			Apple Event; you should not use ExitToShell()
			within this routine’s implementation.

(3.0)
*/
static OSStatus
handleQuit	(AppleEvent const*	UNUSED_ARGUMENT(inAppleEventPtr),
			 AppleEvent*		outReplyAppleEventPtr,
			 DescType			inSaveOption)
{
	OSStatus	result = noErr;
	SInt16		itemHit = kAlertStdAlertOKButton; // if only 1 session is open, the user “Reviews” it
	SInt32		sessionCount = SessionFactory_ReturnCount() -
								SessionFactory_ReturnStateCount(kSession_StateActiveUnstable); // ignore recently-opened windows
	Boolean		doQuit = false;
	
	
	// if there are any unsaved sessions, confirm quitting (nice if the user accidentally hit command-Q);
	// note that currently, because the Console is considered a session, the following statement tests
	// for MORE than 1 open session before warning the user (there has to be an elegant way to do this...)
	if ((inSaveOption == kAEAsk) && (sessionCount > 1))
	{
		InterfaceLibAlertRef	box = nullptr;
		
		
		box = Alert_New();
		Alert_SetHelpButton(box, false);
		Alert_SetParamsFor(box, kAlert_StyleDontSaveCancelSave);
		Alert_SetType(box, kAlertCautionAlert);
		// set message
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			primaryTextCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_AlertWindowQuitPrimaryText, primaryTextCFString);
			if (stringResult.ok())
			{
				CFStringRef		helpTextCFString = nullptr;
				
				
				stringResult = UIStrings_Copy(kUIStrings_AlertWindowQuitHelpText, helpTextCFString);
				if (stringResult.ok())
				{
					Alert_SetTextCFStrings(box, primaryTextCFString, helpTextCFString);
					CFRelease(helpTextCFString);
				}
				CFRelease(primaryTextCFString);
			}
		}
		// set title
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			titleCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_AlertWindowQuitName, titleCFString);
			if (stringResult.ok())
			{
				Alert_SetTitleCFString(box, titleCFString);
				CFRelease(titleCFString);
			}
		}
		// set buttons
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			buttonCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_ButtonReviewWithEllipsis, buttonCFString);
			if (stringResult.ok())
			{
				Alert_SetButtonText(box, kAlertStdAlertOKButton, buttonCFString);
				CFRelease(buttonCFString);
			}
		}
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			buttonCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_ButtonDiscardAll, buttonCFString);
			if (stringResult.ok())
			{
				Alert_SetButtonText(box, kAlertStdAlertOtherButton, buttonCFString);
				CFRelease(buttonCFString);
			}
		}
		Alert_Display(box);
		itemHit = Alert_ItemHit(box);
		Alert_Dispose(&box);
	}
	
	if (itemHit == kAlertStdAlertOKButton)
	{
		// “Review…”; so, display alerts for each open session, individually, and
		// quit after all alerts are closed unless the user cancels one of them.
	#if 1
		// A fairly simple way to handle this is to activate each window in turn,
		// and then display a modal alert asking about the frontmost window.  This
		// is not quite as elegant as using sheets, but since it is synchronous it
		// is WAY easier to write code for!  To help the user better understand
		// which window is being referred to, each window is moved to the center
		// of the main screen using a slide animation prior to popping open an
		// alert.  In addition, all background windows become translucent.
		Boolean		cancelQuit = false;
		
		
		doQuit = true;
		
		// a cool effect, but probably too CPU intensive for most...
		//SessionFactory_ForEveryTerminalWindowDo(setTerminalWindowTranslucency, nullptr/* data 1 */,
		//										1/* 0 = opaque, 1 = translucent */, nullptr/* result */);
		
		// prevent tabs from shifting during this process
		(SessionFactory_Result)SessionFactory_SetAutoRearrangeTabsEnabled(false);
		
		// iterate over each session in a MODAL fashion, highlighting a window
		// and either displaying an alert or discarding the window if it has only
		// been open a short time
		SessionFactory_ForEachSessionDo(kSessionFactory_SessionFilterFlagAllSessions,
										moveWindowAndDisplayTerminationAlertSessionOp,
										0L/* data 1 */, 0L/* data 2 */, &cancelQuit/* result */,
										true/* is a final iteration: use a copy of the list? */);
		
		// prevent tabs from shifting during this process
		(SessionFactory_Result)SessionFactory_SetAutoRearrangeTabsEnabled(true);
		
		// make sure all windows become opaque again
		//SessionFactory_ForEveryTerminalWindowDo(setTerminalWindowTranslucency, nullptr/* data 1 */,
		//										0/* 0 = opaque, 1 = translucent */, nullptr/* result */);
		if (cancelQuit) doQuit = false;
	#else
		// The easiest way to accomplish this is to send back a “close” event that
		// contains a list of all open windows.  However, this requires some state
		// to be retained that allows modeless sheets to remain open and yet resume
		// the quit operation once all sheets are closed.  A little complicated,
		// TextEdit somehow does it correctly, but for now it’s not worth the trouble!
		OSStatus	error = noErr;
		AEDesc		closeWindowEvent;
		
		
		error = RecordAE_CreateRecordableAppleEvent(kAECoreSuite, kAEClose, &closeWindowEvent);
		if (error == noErr)
		{
			AEDesc		objectDesc;
			
			
			(OSStatus)AppleEventUtilities_InitAEDesc(&objectDesc);
			
			// send a request for "every window", which resides in a null container
			// (note that technically this should probably determine which windows are
			// “unsaved”, and construct a list ONLY of unsaved windows, as opposed to
			// all open windows)
			{
				AEDesc		keyDesc;
				
				
				(OSStatus)AppleEventUtilities_InitAEDesc(&keyDesc);
				error = BasicTypesAE_CreateAbsoluteOrdinalDesc(kAEAll, &keyDesc);
				if (error == noErr)
				{
					AEDesc		containerDesc;
					
					
					(OSStatus)AppleEventUtilities_InitAEDesc(&containerDesc);
					error = CreateObjSpecifier(cMyWindow, &containerDesc,
												formAbsolutePosition, &keyDesc, true/* dispose inputs */,
												&objectDesc);
				}
			}
			
			// now issue an event that will close the list of windows, following the
			// same save options that the Quit event was issued with
			if (error == noErr)
			{
				// reference to the window to be closed - REQUIRED
				error = AEPutParamDesc(&closeWindowEvent, keyDirectObject, &objectDesc);
				if (error == noErr)
				{
					AEDesc		saveOptionDesc;
					
					
					// add in the save option (whether to display an alert, etc.)
					(OSStatus)AppleEventUtilities_InitAEDesc(&saveOptionDesc);
					if (BasicTypesAE_CreateEnumerationDesc(inSaveOption, &saveOptionDesc) == noErr)
					{
						(OSStatus)AEPutParamDesc(&closeWindowEvent, keyAESaveOptions, &saveOptionDesc);
					}
					
					// send the event, but do not record it into any open script because
					// the “quit” event already completely describes what is taking place
					error = AESend(&closeWindowEvent, outReplyAppleEventPtr,
									kAENoReply | kAECanInteract | kAEDontRecord,
									kAENormalPriority, kAEDefaultTimeout, nullptr/* idle routine */,
									nullptr/* filter routine */);
					AEDisposeDesc(&saveOptionDesc);
				}
			}
			AEDisposeDesc(&objectDesc);
		}
		AEDisposeDesc(&closeWindowEvent);
		result = error;
		if (result == noErr) doQuit = true;
	#endif
	}
	else if (itemHit == kAlertStdAlertOtherButton)
	{
		// “Discard All”; so, no alerts are displayed, but the application still quits
		result = noErr;
		doQuit = true;
	}
	else
	{
		// “Cancel” or Help
		result = userCanceledErr;
	}
	
	if (doQuit) MainEntryPoint_SignalQuit(); // only main() causes the application to shut down
	
	return result;
}// handleQuit


/*!
Of "SessionFactory_SessionOpProcPtr" form, this routine
slides the specified session’s window(s) into the center
of the screen and then displays a MODAL alert asking the
user if the session should close.  The "inoutResultPtr"
should be a Boolean pointer that, on output, is false
only if the user cancels the close.

(3.0)
*/
static void
moveWindowAndDisplayTerminationAlertSessionOp	(SessionRef		inSession,
												 void*			UNUSED_ARGUMENT(inData1),
												 SInt32			UNUSED_ARGUMENT(inData2),
												 void*			inoutResultPtr)
{
	unless (gCurrentQuitCancelled)
	{
		HIWindowRef		window = Session_ReturnActiveWindow(inSession);
		Boolean*		outFlagPtr = REINTERPRET_CAST(inoutResultPtr, Boolean*);
		
		
		if (nullptr != window)
		{
			// if the window was obscured, show it first
			if (false == IsWindowVisible(window))
			{
				TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(window);
				
				
				TerminalWindow_SetObscured(terminalWindow, false);
			}
			
			// all windows became translucent; make sure the alert one is opaque
			(OSStatus)SetWindowAlpha(window, 1.0);
			
			Session_StartMonitoring(inSession, kSession_ChangeCloseWarningAnswered, gCurrentQuitWarningAnswerListener);
			Session_DisplayTerminationWarning(inSession, true/* force modal window */);
			Session_StopMonitoring(inSession, kSession_ChangeCloseWarningAnswered, gCurrentQuitWarningAnswerListener);
			*outFlagPtr = gCurrentQuitCancelled;
		}
	}
}// moveWindowAndDisplayTerminationAlertSessionOp


/*!
Invoked when a session’s termination warning is
closed; 

(3.0)
*/
static void
receiveTerminationWarningAnswer		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
									 ListenerModel_Event	inEventThatOccurred,
									 void*					inEventContextPtr,
									 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	assert(inEventThatOccurred == kSession_ChangeCloseWarningAnswered);
	
	{
		SessionCloseWarningButtonInfoPtr	infoPtr = REINTERPRET_CAST(inEventContextPtr,
																		SessionCloseWarningButtonInfoPtr);
		
		
		gCurrentQuitCancelled = (infoPtr->buttonHit == kAlertStdAlertCancelButton);
	}
}// receiveTerminationWarningAnswer


/*!
A "SessionFactory_TerminalWindowOpProcPtr" that makes a
terminal window partially transparent or fully opaque
depending on the value of "inData2".  The other arguments
are unused and reserved.

(3.1)
*/
static void
setTerminalWindowTranslucency	(TerminalWindowRef		inTerminalWindow,
								 void*					UNUSED_ARGUMENT(inData1),
								 SInt32					inZeroIfOpaqueOneForPartialTransparency,
								 void*					UNUSED_ARGUMENT(inoutResultPtr))
{
	if (0 == inZeroIfOpaqueOneForPartialTransparency)
	{
		(OSStatus)SetWindowAlpha(TerminalWindow_ReturnWindow(inTerminalWindow), 1.0);
	}
	else
	{
		(OSStatus)SetWindowAlpha(TerminalWindow_ReturnWindow(inTerminalWindow), 0.65/* arbitrary */);
	}
}// setTerminalWindowTranslucency

// BELOW IS REQUIRED NEWLINE TO END FILE
