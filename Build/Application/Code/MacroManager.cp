/*###############################################################

	MacroManager.cp
	
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
#include <cstdio>
#include <cstring>

// standard-C++ includes
#include <sstream>
#include <string>

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <AlertMessages.h>
#include <CocoaBasic.h>
#include <FileSelectionDialogs.h>
#include <MemoryBlocks.h>
#include <StringUtilities.h>

// resource includes
#include "ApplicationVersion.h"
#include "DialogResources.h"
#include "GeneralResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "Folder.h"
#include "MacroManager.h"
#include "Network.h"
#include "Preferences.h"
#include "Session.h"
#include "Terminal.h"
#include "UIStrings.h"



#pragma mark Constants

#define MACRO_IP		0xFF	// metacharacter (for "\i" in source string); substituted with IP address
#define MACRO_LINES		0xFE	// metacharacter (for "\#" in source string); substituted with screen row count

enum
{
	MACRO_MAX_LEN = 256		// maximum number of characters allowed in any macro
};

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	Handle							gMacros[MACRO_SET_COUNT][MACRO_COUNT] =
									{
										// IMPORTANT - initialize all macros to empty!
										{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
										{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
										{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
										{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
										{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
									};
	MacroSetNumber					gActiveSet = 1;
	Boolean							gInited = false;
	MacroManager_InvocationMethod	gMacroMode = kMacroManager_InvocationMethodCommandDigit;
	ListenerModel_Ref				gMacroChangeListenerModel = nullptr;
}

#pragma mark Internal Method Prototypes

static Boolean	allEmpty					(MacroSet);
static void		changeNotifyForMacros		(Macros_Change, void*);
static OSStatus	cStringToFile				(SInt16, char const*);
static SInt16	getMacro					(MacroSet, MacroIndex, char*, SInt16);
static void		parseFile					(MacroSet, SInt16, MacroManager_InvocationMethod*);
static void		setTempMacrosToNull			(MacroSet);
static void		setMacro					(MacroSet, MacroIndex, char const*);



#pragma mark Public Methods

/*!
Loads the special macro files in the Preferences
folder, and initializes the five macro sets from
those files.  If any or all of the files are
missing, the respective macro sets are simply
left blank.

(3.0)
*/
void
Macros_Init ()
{
	FSSpec							folder;
	FSSpec							file;
	Str255							fileName;
	register SInt16					i = 0;
	MacroManager_InvocationMethod   mode = kMacroManager_InvocationMethodCommandDigit;
	
	
	// set up a listener model to handle callbacks
	gMacroChangeListenerModel = ListenerModel_New(kListenerModel_StyleStandard,
													kConstantsRegistry_ListenerModelDescriptorMacroChanges);
	assert(nullptr != gMacroChangeListenerModel);
	
	// there are five files, one per set; read each file, importing only if no errors occur
	for (i = MACRO_SET_COUNT; i >= 1; --i)
	{
		Macros_SetActiveSetNumber(i); // set numbers are one-based
		// TEMPORARY: This needs to migrate to the new Preferences API.
		//GetIndString(fileName, rStringsImportantFileNames, siImportantFileNameMacroSet1 + i - 1);
		if (PLstrlen(fileName) > 0)
		{
			if (noErr == Folder_GetFSSpec(kFolder_RefUserMacroFavorites, &folder))
			{
				if (noErr == FSMakeFSSpec(folder.vRefNum, folder.parID, fileName, &file))
				{
					// if this point is reached, then the requested file exists; import it!
					Macros_ImportFromText(Macros_ReturnActiveSet(), &file, &mode);
				}
			}
		}
	}
	Macros_SetMode(mode); // the effect of this is that Macro Set 1 determines the “persistent” macro keys
	gInited = true;
}// Init


/*!
Maintains implicit persistence of the five macro
sets by saving them into special macro files in
the Preferences folder.

Note that errors are basically ignored; a nicer
implementation might advise the user when some
kind of problem occurs that could jeopardize
the saving of macros.  In fact, this should be a
definite priority for some future release!!!

(3.0)
*/
void
Macros_Done ()
{
	if (gInited)
	{
		FSSpec				folder,
							file;
		Str255				fileName;
		register SInt16		i = 0;
		OSStatus			error = noErr;
		
		
		// there are five files, one per set; find each file, exporting only if no errors occur
		for (i = MACRO_SET_COUNT; i >= 1; --i)
		{
			Macros_SetActiveSetNumber(i); // set numbers are one-based
			// TEMPORARY: This needs to migrate to the new Preferences API.
			//GetIndString(fileName, rStringsImportantFileNames, siImportantFileNameMacroSet1 + i - 1);
			if (PLstrlen(fileName) > 0)
			{
				if (Folder_GetFSSpec(kFolder_RefUserMacroFavorites, &folder) == noErr)
				{
					error = FSMakeFSSpec(folder.vRefNum, folder.parID, fileName, &file);
					
					if ((error == noErr) || (error == fnfErr))
					{
						// If this point is reached, then it is possible to specify where on
						// disk the macros go; export them!  Note that due to the nature of
						// the import procedure, the “persistent” macro mode will really be
						// Macro Set 1’s mode.
						Macros_ExportToText(Macros_ReturnActiveSet(), &file, Macros_ReturnMode());
						
						// since MacTelnet depends on the file having a specific name, make
						// sure the user can’t change it
						{
							FileInfo		info;
							
							
							FSpGetFInfo(&file, (FInfo*)&info);
							info.finderFlags |= kNameLocked;
							FSpSetFInfo(&file, (FInfo*)&info);
						}
					}
				}
			}
		}
		
		// dispose of callback information; do this last because other
		// clean-up actions might still trigger callback invocations
		ListenerModel_Dispose(&gMacroChangeListenerModel);
		
		gInited = false;
	}
}// Done


/*!
Creates a temporary storage area for macros.  Use
Macros_DisposeSet() to destroy this set when you
are finished with it.  If any problems occur,
nullptr is returned.

(3.0)
*/
MacroSet
Macros_NewSet ()
{
	return (MacroSet)Memory_NewPtr(MACRO_COUNT * sizeof(Handle));
}// NewSet


/*!
Destroys a temporary storage area that you previously
created with Macros_NewSet().  On output, your pointer
to the macro set is automatically set to nullptr.

(3.0)
*/
void
Macros_DisposeSet	(MacroSet*		inoutSetToDestroy)
{
	if (inoutSetToDestroy != nullptr)
	{
		register SInt16		i = 0;
		
		
		for (i = 0; i < MACRO_COUNT; ++i)
		{
			if ((*inoutSetToDestroy)[i] != nullptr)
			{
				HUnlock((*inoutSetToDestroy)[i]);
				Memory_DisposeHandle(&(*inoutSetToDestroy)[i]);
			}
		}
		Memory_DisposePtr((Ptr*)inoutSetToDestroy);
	}
}// DisposeSet


/*!
Determines if there is any text in any of the
macros in the specified macro set.

(3.0)
*/
Boolean
Macros_AllEmpty		(MacroSet	inSet)
{
	return allEmpty(inSet);
}// AllEmpty


/*!
Duplicates all of the macros in one set into a
second set.

(3.0)
*/
void
Macros_Copy		(MacroSet	inSource,
				 MacroSet	inDestination)
{
	register SInt16		i = 0;
	
	
	for (i = 0; i < MACRO_COUNT; ++i)
	{
		if (inDestination[i] != nullptr) Memory_DisposeHandle(&inDestination[i]);
		if (inSource[i] != nullptr)
		{
			SInt8		handleState = 0;
			Size		moveSize = GetHandleSize(inSource[i]);
			
			
			inDestination[i] = Memory_NewHandle(moveSize);
			handleState = HGetState(inSource[i]);
			HLockHi(inSource[i]);
			HLockHi(inDestination[i]);
			BlockMoveData(*(inSource[i]), *(inDestination[i]), moveSize);
			HUnlock(inDestination[i]);
			HSetState(inSource[i], handleState);
		}
		
		// now notify all interested listeners that a macro has changed
		{
			MacroDescriptor		descriptor;
			
			
			descriptor.set = inDestination;
			descriptor.index = i;
			changeNotifyForMacros(kMacros_ChangeContents, &descriptor);
		}
	}
}// Copy


/*!
Exports macros stored in the specified macro storage area
to a file.  If you pass nullptr for the file specification,
the user is prompted to create a file in which to store
the macro set.  The exported file also indicates the keys
to be used for the macro keys (such as function keys,
versus command-digit keys).

To place macros in temporary storage space, you can use
the Macros_Set() or Macros_Copy() methods.

(2.6)
*/
void
Macros_ExportToText		(MacroSet						inSet,
						 FSSpec*						inFileSpecPtrOrNull,
						 MacroManager_InvocationMethod	inMacroModeOfExportedSet)
{
	SInt16 		refNum = 0;
	FSSpec		macroFile; // this is ONLY storage, used as needed
	FSSpec*		fileSpecPtr = inFileSpecPtrOrNull; // this refers to the file to write to!
	OSStatus	error = noErr;
	Boolean		good = true;
	
	
	if (inFileSpecPtrOrNull == nullptr)
	{
		Str255		prompt,
					title,
					fileDefaultName;
		
		
		// TEMPORARY: This needs to migrate to the new Navigation Services API.
		PLstrcpy(prompt, "\p");
		PLstrcpy(title, "\p");
		//GetIndString(prompt, rStringsNavigationServices, siNavPromptExportMacrosToFile);
		//GetIndString(title, rStringsNavigationServices, siNavDialogTitleExportMacros);
		{
			// TEMPORARY; the string retrieval has been upgraded to CFStrings, but not
			//            the Navigation Services calls; eventually, the CFString can
			//            be passed directly (localized) instead of being hacked into
			//            a Pascal string first
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			filenameCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_FileDefaultMacroSet, filenameCFString);
			if (kUIStrings_ResultOK == stringResult)
			{
				CFStringGetPascalString(filenameCFString, fileDefaultName, sizeof(fileDefaultName),
										kCFStringEncodingMacRoman/* TEMPORARY */);
				CFRelease(filenameCFString);
			}
			else
			{
				PLstrcpy(fileDefaultName, "\p");
			}
		}
		{
			NavReplyRecord	reply;
			
			
			Alert_ReportOSStatus(error = FileSelectionDialogs_PutFile
											(prompt, title, fileDefaultName,
												AppResources_ReturnCreatorCode(),
												kApplicationFileTypeMacroSet,
												kPreferences_NavPrefKeyMacroStuff,
												kNavDefaultNavDlogOptions | kNavDontAddTranslateItems,
												EventLoop_HandleNavigationUpdate, &reply, &macroFile));
			good = ((error == noErr) && (reply.validRecord));
			if (good) fileSpecPtr = &macroFile;
		}
	}
	
	if (good)
	{
		SInt16 		i = 0;
		char		temp[MACRO_MAX_LEN],
					temp2[MACRO_MAX_LEN];
		Boolean		exists = false;
		
		
		// try to create the file; if this fails with a “duplicate file name” error, then the file already exists
		if ((error = FSpCreate(fileSpecPtr, AppResources_ReturnCreatorCode(),
								kApplicationFileTypeMacroSet, GetScriptManagerVariable(smSysScript))) == dupFNErr)
		{
			exists = true;
		}
		
		error = FSpOpenDF(fileSpecPtr, fsWrPerm, &refNum);
		
		if (exists) SetEOF(refNum, 0L);
		
		for (i = 0; i < MACRO_COUNT; ++i)
		{
			Macros_Get(inSet, i, temp, sizeof(temp));
			
			// IMPORTANT:	The internal method parseFile() determines the “key equivalents” of an imported
			//				set by checking to see if the first character of each line is a capital "F".  If
			//				you see a need to change the code below, be sure to update parseFile()!
			CPP_STD::snprintf(temp2, sizeof(temp2),
								(inMacroModeOfExportedSet == kMacroManager_InvocationMethodFunctionKeys)
								? "F%d = \""
								: "Cmd%d = \"",
								(inMacroModeOfExportedSet == kMacroManager_InvocationMethodFunctionKeys)
								? i + 1
								: i);
			cStringToFile(refNum, temp2);
			if (*temp) 
			{									
				cStringToFile(refNum, temp);
			}
			CPP_STD::strcpy(temp2, "\"\015");
			cStringToFile(refNum, temp2);
		}
		FSClose(refNum);
	}
}// ExportToText


/*!
Explicitly obtains the human-readable value of a
particular macro in a macro space.  You can use
Macros_Copy() to work between two macro spaces.

The value of "inZeroBasedMacroNumber" must be
between 0 and one less than MACRO_COUNT.

(3.0)
*/
void
Macros_Get	(MacroSet		inFromWhichSet,
			 MacroIndex		inZeroBasedMacroNumber,
			 char*			outValue,
			 SInt16			inRoom)
{
	getMacro(inFromWhichSet, inZeroBasedMacroNumber, outValue, inRoom);
}// Get


/*!
Imports macros.  If you pass nullptr for the file specification,
the user is prompted to locate a file from which to fill in the
macro set.  If the user does not cancel and no errors occur
(that is, the macros actually got imported), true is returned;
otherwise, false is returned.

Sometimes, the key equivalents used for the macros can be
determined from the text file.  If so, the macro mode “as saved
in the macro set” is imported as well, and returned in
"outMacroModeOfImportedSet".  If not, the value
"kMacroManager_InvocationMethodCommandDigit" (the default) is
returned in "outMacroModeOfImportedSet".  If you do not need
this information, it is safe to pass nullptr for
"outMacroModeOfImportedSet".

(3.0)
*/
Boolean
Macros_ImportFromText	(MacroSet						inSet,
						 FSSpec const*					inFileSpecPtrOrNull,
						 MacroManager_InvocationMethod*	outMacroModeOfImportedSet)
{
	Boolean		result = true;
	
	
	// if no file is given, a dialog is displayed and all selected files
	// are automatically routed through Apple Events to trigger an import
	if (nullptr == inFileSpecPtrOrNull)
	{
		// IMPORTANT: These should be consistent with declared types in the application "Info.plist".
		void const*			kTypeList[] = { CFSTR("com.mactelnet.macros"),
											CFSTR("macros"),/* redundant, needed for older systems */
											CFSTR("TEXT")/* redundant, needed for older systems */ };
		CFRetainRelease		fileTypes(CFArrayCreate(kCFAllocatorDefault, kTypeList,
										sizeof(kTypeList) / sizeof(CFStringRef), &kCFTypeArrayCallBacks),
										true/* is retained */);
		CFStringRef			promptCFString = nullptr;
		CFStringRef			titleCFString = nullptr;
		
		
		(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptOpenMacroSet, promptCFString);
		(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogTitleOpenMacroSet, titleCFString);
		(Boolean)CocoaBasic_FileOpenPanelDisplay(promptCFString, titleCFString, fileTypes.returnCFArrayRef());
	}
	else
	{
		OSStatus 	error = noErr;
		SInt16		fileRef = 0;
		
		
		error = FSpOpenDF(inFileSpecPtrOrNull, fsRdPerm, &fileRef);
		if ((noErr == error) && (result))
		{
			parseFile(inSet, fileRef, outMacroModeOfImportedSet);
			FSClose(fileRef);
		}
	}
	
	return result;
}// ImportFromText


/*!
Explicitly changes all of the macros in the specified
temporary space to correspond to the text contained
in a buffer of the specified size.

IMPORTANT:	The internal parseFile() method does the
			same fundamental thing as this routine,
			but the complexities of file access made
			it easier to simply duplicate most of the
			code.  If you ever change anything here,
			be sure to compare your changes with the
			code of parseFile().

(3.0)
*/
void
Macros_ParseTextBuffer	(MacroSet							inSet,
						 UInt8*								inSourcePtr,
						 Size								inSize,
						 MacroManager_InvocationMethod*		outMacroModeOfImportedSet)
{
	UInt8				buffer[300];
	UInt8				newMacro[MACRO_MAX_LEN];
	UInt8*				bufferPtr = nullptr;
	UInt8*				newMacroPtr = nullptr;
	UInt8*				dynamicSourcePtr = inSourcePtr;
	OSStatus			error = noErr;
	UInt16				numMacrosRead = 0;
	register SInt16		totalLen = 0;
	Size				count = 1; // number of bytes copied in total
	
	
	bufferPtr = buffer;
	
	setTempMacrosToNull(inSet); // sets all handles in the current temporary macro space to nullptr

	while ((error != eofErr) && (numMacrosRead < MACRO_COUNT))
	{
		*bufferPtr = *dynamicSourcePtr++;
		if (count >= inSize) error = eofErr;
		while ((*bufferPtr != 0x0D) && (error != eofErr)) // while not CR or EOF
		{
			++bufferPtr;
			++count;
			if (count <= inSize) *bufferPtr = *dynamicSourcePtr++;
			else error = eofErr;
		}
		
		// 3.0 - determine the key equivalents, if possible (this only needs to be done once)
		if ((!numMacrosRead) && (outMacroModeOfImportedSet != nullptr))
		{
			// See Macros_ExportToText() for code that explicitly specifies that
			// the key for a function key macro starts with a capital "F".  That
			// code is the basis for the following test, which simply looks at
			// the first character of each macro to see what the key equivalents
			// in the set are.
			if (buffer[0] == 'F') *outMacroModeOfImportedSet = kMacroManager_InvocationMethodFunctionKeys;
			else *outMacroModeOfImportedSet = kMacroManager_InvocationMethodCommandDigit;
		}
		
		totalLen = bufferPtr - buffer;
		bufferPtr = buffer;
		newMacroPtr = newMacro;
		while ((*bufferPtr++ != '"') && (totalLen != 0)) --totalLen;
		
		while ((*bufferPtr != '"') && (totalLen != 0))
		{
			*newMacroPtr++ = *bufferPtr++;
			--totalLen;
		}
		*newMacroPtr = '\0'; // make this a C string
		
		Macros_Set(inSet, numMacrosRead, (char*)newMacro);
		++numMacrosRead;
		bufferPtr = buffer;
	}
}// ParseTextBuffer


/*!
Returns the active macro set.

(3.0)
*/
MacroSet
Macros_ReturnActiveSet ()
{
	return gMacros[Macros_ReturnActiveSetNumber() - 1];
}// ReturnActiveSet


/*!
Determines which set (from 1 to MACRO_SET_COUNT) is
active.  The active set is the one which is referenced
by the Macros_Get() and Macros_Set() methods.

(3.0)
*/
MacroSetNumber
Macros_ReturnActiveSetNumber ()
{
	return gActiveSet;
}// ReturnActiveSetNumber


/*!
Determines the current macro mode.  The mode does not
affect how macros are stored or manipulated, it only
affects what is required by the user in order to “type”
one.  The default macro mode is "command-digit", which
means that one of the key combinations from -0 through
-9 or -= or -/ is required to type a macro.  Other
modes, such as "function key" (F1-F12), are also
available.

(3.0)
*/
MacroManager_InvocationMethod
Macros_ReturnMode ()
{
	return gMacroMode;
}// ReturnMode


/*!
Explicitlys change the value of a particular macro.
You can use the Macros_Copy() method to work between
two macro spaces.

The value of "inZeroBasedMacroNumber" must be between
0 and one less than MACRO_COUNT.

(3.0)
*/
void
Macros_Set	(MacroSet		inSet,
			 MacroIndex		inZeroBasedMacroNumber,
			 char const*	inValue)
{
	setMacro(inSet, inZeroBasedMacroNumber, inValue);
}// Set


/*!
Specifies which set (from 1 to MACRO_SET_COUNT) is
active.  The active set is the one which is referenced
by the Macros_Get() and Macros_Set() methods.

(3.0)
*/
void
Macros_SetActiveSetNumber	(MacroSetNumber		inMacroSetNumber)
{
	// first notify all interested listeners that the active macro set *will* be changing (but hasn’t yet)
	changeNotifyForMacros(kMacros_ChangeActiveSetPlanned, nullptr/* context */);
	
	gActiveSet = inMacroSetNumber;
	
	// now notify all interested listeners that the active macro set has changed
	changeNotifyForMacros(kMacros_ChangeActiveSet, nullptr/* context */);
}// SetActiveSetNumber


/*!
Specifies the current macro mode.  The mode does not
affect how macros are stored or manipulated, it only
affects what is required by the user in order to “type”
one.  The default macro mode is "command-digit", which
means that one of the key combinations from -0 through
-9 or -= or -/ is required to type a macro.  Other
modes, such as "function key" (F1-F12), are also
available.

(3.0)
*/
void
Macros_SetMode	(MacroManager_InvocationMethod	inNewMode)
{
	gMacroMode = inNewMode;
	
	// now notify all interested listeners that the macro keys have changed
	changeNotifyForMacros(kMacros_ChangeMode, &inNewMode);
}// SetMode


/*!
Arranges for a callback to be invoked whenever a macro
change occurs.  For example, you can use this to find
out when the text of a macro changes, or the macro keys
change.

For certain changes, an event will also be fired right
away, so that your handler sees the “initial value”.
This currently applies to: "kMacros_ChangeActiveSet",
"kMacros_ChangeMode".

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to a change.  See "MacroManager.h" for
			comments on what the context means for each
			type of change.

(3.0)
*/
void
Macros_StartMonitoring	(Macros_Change				inForWhatChange,
						 ListenerModel_ListenerRef	inListener)
{
	OSStatus	error = noErr;
	
	
	// add a listener to the listener model for the given change
	error = ListenerModel_AddListenerForEvent(gMacroChangeListenerModel, inForWhatChange, inListener);
	
	switch (inForWhatChange)
	{
	case kMacros_ChangeActiveSet:
		changeNotifyForMacros(kMacros_ChangeActiveSet, nullptr/* context */);
		break;
	
	case kMacros_ChangeMode:
		changeNotifyForMacros(kMacros_ChangeMode, &gMacroMode/* context */);
		break;
	
	default:
		// initial event not sent
		break;
	}
}// StartMonitoring


/*!
Arranges for a callback to no longer be invoked whenever
a macro change occurs.

IMPORTANT:	This routine cancels the effects of a previous
			call to Macros_StartMonitoring() - your
			parameters must match the previous start-call,
			or the stop will fail.

(3.0)
*/
void
Macros_StopMonitoring	(Macros_Change				inForWhatChange,
						 ListenerModel_ListenerRef	inListener)
{
	// remove a listener from the listener model for the given change
	ListenerModel_RemoveListenerForEvent(gMacroChangeListenerModel, inForWhatChange, inListener);
}// StopMonitoring


/*!
Sends a specific macro to the given session (from the
currently active macro set) as a string, representing
the text that should be “typed” for the macro.

Returns "true" only if successful.

(2.6)
*/
Boolean
MacroManager_UserInputMacroString	(SessionRef		inSession,
									 MacroIndex		inZeroBasedMacroNumber)
{
	UInt8*				mp = nullptr;
	UInt8*				first = nullptr;
	register UInt16		zeroBasedActiveSetNumber = Macros_ReturnActiveSetNumber() - 1;
	Boolean				result = true;
	
	
	if (inZeroBasedMacroNumber > (MACRO_COUNT - 1))
	{
		// invalid number
		result = false;
	}
	else if (gMacros[zeroBasedActiveSetNumber][inZeroBasedMacroNumber] != nullptr)
	{
		HLock(gMacros[zeroBasedActiveSetNumber][inZeroBasedMacroNumber]);
		mp = (UInt8*)*gMacros[zeroBasedActiveSetNumber][inZeroBasedMacroNumber];
		first = mp;
		
		while (*mp)
		{
			if (*mp == MACRO_IP)
			{
				std::string		ipAddressString;
				int				addressType = 0;
				
				
				Session_UserInputString(inSession, (char*)first, (mp - first) * sizeof(char), true/* record */);
				if (Network_CurrentIPAddressToString(ipAddressString, addressType))
				{
					Session_UserInputString(inSession, ipAddressString.c_str(),
											ipAddressString.size() * sizeof(char), true/* record */);
				}
				first = mp + 1;
			}
			else if (*mp == MACRO_LINES)
			{
				std::ostringstream	numberOfLinesBuffer;
				std::string			numberOfLinesString;
				
				
				Session_UserInputString(inSession, (char*)first, (mp - first) * sizeof(char), true/* record */);
				numberOfLinesBuffer << Terminal_ReturnRowCount(Session_ConnectionDataPtr(inSession)->vs);
				numberOfLinesString = numberOfLinesBuffer.str();
				Session_UserInputString(inSession, numberOfLinesString.c_str(),
										numberOfLinesString.size(), true/* record */);
				first = mp + 1;
			}
			++mp;
		}
		Session_UserInputString(inSession, (char*)first, (mp - first) * sizeof(char), true/* record */);
		
		HUnlock(gMacros[zeroBasedActiveSetNumber][inZeroBasedMacroNumber]);
	}
	
	return result;
}// UserInputMacroString


#pragma mark Internal Methods

/*!
Determines if all macros in the specified set
are empty.  This is the case if all strings
in the set have zero length.

(3.0)
*/
static Boolean
allEmpty	(MacroSet	inSet)
{
	Boolean				result = true;
	register SInt16		i = 0;
	Str255				temp;
	
	
	for (i = 0; (i < MACRO_COUNT) && (result); ++i)
	{
		getMacro(inSet, i, (char*)&temp, sizeof(temp));
		if (CPP_STD::strlen((char*)&temp) > 0) result = false;
	}
	return result;
}// allEmpty


/*!
Notifies all listeners for the specified Macros
state change, passing the given context to the
listener.

IMPORTANT:	The context must make sense for the
			type of change; see "MacroManager.h"
			for the type of context associated
			with each macros change.

(3.0)
*/
static void
changeNotifyForMacros	(Macros_Change	inWhatChanged,
						 void*			inContextPtr)
{
	// invoke listener callback routines appropriately, from the listener model
	ListenerModel_NotifyListenersOfEvent(gMacroChangeListenerModel, inWhatChanged, inContextPtr);
}// changeNotifyForMacros


/*!
Writes the contents of a string to a file that has an
ID number in the Macintosh HFS.

(2.6)
*/
static OSStatus
cStringToFile	(SInt16			inFileID,
				 char const*	inString)
{
	SInt32		byteCount = CPP_STD::strlen(inString);
	OSStatus	result = noErr;
	
	
	result = FSWrite(inFileID, &byteCount, inString);
	return result;
}// cStringToFile


/*!
Copies a macro’s text into an array of characters.  The
macro number is a zero-based index into the given set.

If the given index is valid, 0 is returned; otherwise,
-1 is returned.

(3.0)
*/
static SInt16
getMacro	(MacroSet		inSet,
			 MacroIndex		inMacroNumber,
			 char*			outValue,
			 SInt16			inRoom)
{
	SInt16		result = 0;
	UInt8*		s = nullptr;
	
	
	// invalid number?
	if (inMacroNumber <= (MACRO_COUNT - 1))
	{
		if (inSet[inMacroNumber] != nullptr)
		{
			s = (UInt8*)*inSet[inMacroNumber];
			
			while (*s && (inRoom >= 5)) // 5 = (size of \xxx) + (terminating \0)
			{
				switch (*s)
				{
				case MACRO_IP:
					// 0xFF -> "\i"
					*outValue++ = '\\';
					*outValue++ = 'i';
					--inRoom;
					break;
				
				case MACRO_LINES:
					// 0xFE -> "\#"
					*outValue++ = '\\';
					*outValue++ = '#';
					--inRoom;
					break;
				
				case '\\':
					// \ -> "\\"
					*outValue++ = '\\';
					*outValue++ = '\\';
					--inRoom;
					break;
				
				case '\033':
					// ESC -> "\e"
					*outValue++ = '\\';
					*outValue++ = 'e';
					--inRoom;
					break;
				
				case '\015':
					// CR -> "\r"
					*outValue++ = '\\';
					*outValue++ = 'r';
					--inRoom;
					break;
				
				case '\012':
					// LF -> "\n"
					*outValue++ = '\\';
					*outValue++ = 'n';
					--inRoom;
					break;
				
				case '\t':
					// tab -> "\t"
					*outValue++ = '\\';
					*outValue++ = 't';
					--inRoom;
					break;
				
				default: 
					if (CPP_STD::isprint(*s)) *outValue++ = *s;
					else
					{
						*outValue++ = '\\';
						*outValue++ = (*s / 64) + '0';
						*outValue++ = ((*s % 64) / 8) + '0';
						*outValue++ = (*s % 8) + '0';
						inRoom = inRoom - 3;
					}
					break;
				}
				
				--inRoom;
				++s;
			}
		}
		*outValue = 0;
	}
	else
	{
		result = -1;
	}
	
	return result;
}// getMacro


/*!
Explicitly changes all of the macros in the specified
temporary space to correspond to the text imported
from an open file.

If specified (that is, not nullptr), the parameter
"outMacroModeOfImportedSet" can be used to determine
the type of key equivalents that a file’s macros were
meant to use.  This is not always possible to
determine - in such case, the default value is
"kMacroManager_InvocationMethodCommandDigit".

IMPORTANT:	Macros_ParseTextBuffer() does the same
			fundamental thing as this routine, but the
			complexities of file access made it easier
			to simply duplicate most of the code.  If
			you ever change anything here, be sure to
			compare your changes with the code of
			Macros_ParseTextBuffer().

(2.6)
*/
static void
parseFile	(MacroSet							inSet,
			 SInt16								inFileRef,
			 MacroManager_InvocationMethod*		outMacroModeOfImportedSet)
{
	UInt8				buffer[300];
	UInt8				newMacro[MACRO_MAX_LEN];
	UInt8*				bufferPtr = nullptr;
	UInt8*				newMacroPtr = nullptr;
	OSStatus			fileErr = noErr;
	UInt16				numMacrosRead = 0;
	register SInt16		totalLen = 0;
	long				count = 1; // read one byte per file access
	
	
	bufferPtr = buffer; 
	
	setTempMacrosToNull(inSet); // sets all handles in the current temporary macro space to nullptr
	
	while ((fileErr != eofErr) && (numMacrosRead < MACRO_COUNT))
	{
		fileErr = FSRead(inFileRef, &count, bufferPtr);
		while ((*bufferPtr != 0x0D) && (fileErr != eofErr)) // while not CR or EOF
		{
			++bufferPtr;
			fileErr = FSRead(inFileRef, &count, bufferPtr);
		}
		
		// 3.0 - determine the key equivalents, if possible (this only needs to be done once)
		if ((!numMacrosRead) && (outMacroModeOfImportedSet != nullptr))
		{
			// See Macros_ExportToText() for code that explicitly specifies that
			// the key for a function key macro starts with a capital "F".  That
			// code is the basis for the following test, which simply looks at
			// the first character to find the key equivalents of the set.
			if (buffer[0] == 'F') *outMacroModeOfImportedSet = kMacroManager_InvocationMethodFunctionKeys;
			else *outMacroModeOfImportedSet = kMacroManager_InvocationMethodCommandDigit;
		}
		
		totalLen = bufferPtr - buffer;
		bufferPtr = buffer;
		newMacroPtr = newMacro;
		while ((*bufferPtr++ != '"') && (totalLen != 0)) --totalLen;
		
		while ((*bufferPtr != '"') && (totalLen != 0))
		{
			*newMacroPtr++ = *bufferPtr++;
			--totalLen;
		}
		*newMacroPtr = '\0'; // make this a C string
		
		Macros_Set(inSet, numMacrosRead, (char*)newMacro);
		++numMacrosRead;
		bufferPtr = buffer;
	}
}// parseFile


/*!
Nullifies all macro handles in the space of the active
temporary set, leaving other temporary sets untouched.

(3.0)
*/
static void
setTempMacrosToNull		(MacroSet	inSet)
{
	register SInt16		i = 0;
	
	
	for (i = 0; i < MACRO_COUNT; ++i)
	{
		if (inSet[i] != nullptr) Memory_DisposeHandle(&inSet[i]);
		inSet[i] = nullptr;
	}
}// setTempMacrosToNull


/*!
Sets the specified macro’s text to a copy of the given text.
The macro number is a zero-based index into the given set.

If the given index is valid, 0 is returned; otherwise, -1 is
returned.

(3.0)
*/
static void
setMacro	(MacroSet		inoutDestination,
			 MacroIndex		inMacroNumber,
			 char const*	inValue)
{
	if (inMacroNumber <= (MACRO_COUNT - 1))
	{
		SInt16 const	kStringLength = CPP_STD::strlen(inValue);
		
		
		if (kStringLength == 0)
		{
			// if this is an empty string, remove whatever storage might have
			// been used previously by this macro
			if (inoutDestination[inMacroNumber] != nullptr)
			{
				Memory_DisposeHandle(&inoutDestination[inMacroNumber]);
			}
		}
		else if (kStringLength < MACRO_MAX_LEN)
		{
			SInt16		totalByteCount = 0; // length of string plus room for a terminating byte
			
			
			// restrict the maximum length of macros to MACRO_MAX_LEN bytes
			totalByteCount = CPP_STD::strlen(inValue) + 1;
			
			// if necessary, create storage for the macro
			if (inoutDestination[inMacroNumber] == nullptr)
			{
				inoutDestination[inMacroNumber] = Memory_NewHandle(totalByteCount * sizeof(char));
			}
			
			if (inoutDestination[inMacroNumber] != nullptr)
			{
				UInt8*			destPtr = nullptr;
				UInt8 const*	srcPtr = REINTERPRET_CAST(inValue, UInt8 const*);
				OSStatus		memError = noErr;
				
				
				// adjust the handle to the proper size (may be making an existing macro longer)
				memError = Memory_SetHandleSize(inoutDestination[inMacroNumber], totalByteCount * sizeof(char));
				if (memError == noErr)
				{
					SInt16		num = 0,
								pos = 0;
					Boolean		escape = false;
					
					
					HLock(inoutDestination[inMacroNumber]);
					destPtr = (UInt8*)*inoutDestination[inMacroNumber];
					
					while (*srcPtr)
					{
						if (escape)
						{
							escape = false;
							switch (*srcPtr)
							{
							case 'i': // current IP address
								if (pos > 0)
								{
									*destPtr++ = num;
									*destPtr++ = *srcPtr;
									pos = 0;
								}
								*destPtr++ = MACRO_IP;
								break;
							
							case '#': // number of rows in terminal screen
								if (pos > 0)
								{
									*destPtr++ = num;
									*destPtr++ = *srcPtr;
									pos = 0;
								}
								*destPtr++ = MACRO_LINES;
								break;
							
							case 'e': // ESC
								if (pos > 0)
								{
									*destPtr++ = num;
									*destPtr++ = *srcPtr;
									pos = 0;
								}
								*destPtr++ = '\033';
								break;
							
							case 'n': // new-line
								if (pos > 0)
								{
									*destPtr++ = num;
									*destPtr++ = *srcPtr;
									pos = 0;
								}
								*destPtr++ = '\012';
								break;
							
							case 'r': // carriage return
								if (pos > 0)
								{
									*destPtr++ = num;
									*destPtr++ = *srcPtr;
									pos = 0;
								}
								*destPtr++ = '\015';
								break;
							
							case 't': // tab
								if (pos > 0)
								{
									*destPtr++ = num;
									*destPtr++ = *srcPtr;
									pos = 0;
								}
								*destPtr++ = '\t';
								break;
							
							case '"': // double-quotes
								if (pos > 0)
								{
									*destPtr++ = num;
									*destPtr++ = *srcPtr;
									pos = 0;
								}
								*destPtr++ = '\"';
								break;
									
							case '\\': // single backslash
								if (pos > 0)
								{
									*destPtr++ = num;
									escape = true;
									pos = 0;
									num = 0;
								}
								else
								{
									*destPtr++ = '\\';
								}
								break;
							
							default:
								if (CPP_STD::isdigit(*srcPtr) && pos < 3)
								{
									num = num * 8 + (*srcPtr - '0');
									++pos;
									escape = true;
								}
								else
								{
									if (pos == 0 && num == 0)
									{
										*destPtr++ = '\\';
										*destPtr++ = *srcPtr;
									}
									else
									{
										*destPtr++ = num;
										pos = 0;
										--srcPtr; // retreat to previous character
									}
								}
								break;
							}
						}
						else
						{
							if (*srcPtr == '\\')
							{
								num = 0;
								pos = 0;
								escape = true;
							}
							else
							{
								*destPtr++ = *srcPtr;
							}
						}
						++srcPtr;
					}
					
					if (pos > 0) *destPtr++ = num;
					
					*destPtr = '\0'; // terminate string
					
					// The resultant macro may be shorter than the input string due to escaped characters.
					// So, recalculate the length of the macro and resize the handle if necessary.
					totalByteCount = CPP_STD::strlen(*inoutDestination[inMacroNumber]) + 1;
					
					HUnlock(inoutDestination[inMacroNumber]);
					memError = Memory_SetHandleSize(inoutDestination[inMacroNumber],
													totalByteCount * sizeof(char));
					
					// now notify all interested listeners that a macro has changed
					{
						MacroDescriptor		descriptor;
						
						
						descriptor.set = inoutDestination;
						descriptor.index = inMacroNumber;
						changeNotifyForMacros(kMacros_ChangeContents, &descriptor);
					}
				}
			}
		}
	}
}// setMacro

// BELOW IS REQUIRED NEWLINE TO END FILE
