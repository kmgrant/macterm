/*###############################################################

	SessionDescription.cp
	
	MacTelnet
		© 1998-2011 by Kevin Grant.
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
#include <algorithm>
#include <vector>

// pseudo-standard-C++ includes
#if __MWERKS__
#   include <hash_fun>
#   include <hash_map>
#   define hash_map_namespace Metrowerks
#elif (__GNUC__ > 3)
#   include <ext/hash_map>
#   define hash_map_namespace __gnu_cxx
#elif (__GNUC__ == 3)
#   include <ext/stl_hash_fun.h>
#   include <ext/hash_map>
#   define hash_map_namespace __gnu_cxx
#elif (__GNUC__ < 3)
#   include <stl_hash_fun.h>
#   include <hash_map>
#   define hash_map_namespace
#else
#   include <hash_fun>
#   include <hash_map>
#   define hash_map_namespace
#endif

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <AlertMessages.h>
#include <CFRetainRelease.h>
#include <CocoaBasic.h>
#include <FileSelectionDialogs.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlockReferenceLocker.template.h>
#include <MemoryBlocks.h>
#include <TextDataFile.h>

// application includes
#include "AppResources.h"
#include "DialogUtilities.h"
#include "Preferences.h"
#include "SessionDescription.h"
#include "SessionFactory.h"
#include "TerminalView.h"
#include "Terminology.h"
#include "UIStrings.h"



#pragma mark Types

struct My_SessionFile
{
	SessionDescription_ContentType		fileType;			//!< determines valid part of union below
	
	// data common to all session files
	CFStringRef					windowName;			//!< name of terminal window
	CFStringRef					currentMacrosName;	//!< name of macro set preference collection that should be selected
	CFStringRef					terminalFont;		//!< name of font used for rendering regular terminal text
	CFStringRef					answerBack;			//!< string identifying terminal emulation type
	CFStringRef					toolbarInfo;		//!< string describing toolbar state
	
	CGDeviceColor*				colorTextNormalPtr;
	CGDeviceColor*				colorBackgroundNormalPtr;
	CGDeviceColor*				colorTextBoldPtr;
	CGDeviceColor*				colorBackgroundBoldPtr;
	CGDeviceColor*				colorTextBlinkingPtr;
	CGDeviceColor*				colorBackgroundBlinkingPtr;
	
	SInt32*						invisibleLineCountPtr;
	SInt32*						visibleColumnCountPtr;
	SInt32*						visibleLineCountPtr;
	SInt32*						windowPositionHPtr;
	SInt32*						windowPositionVPtr;
	SInt32*						fontSizePtr;
	
	Boolean*					isFTPPtr;
	Boolean*					isTEKPageInSameWindowPtr;
	Boolean*					isBerkeleyCRPtr;
	Boolean*					isAuthenticationEnabledPtr;
	Boolean*					isEncryptionEnabledPtr;
	Boolean*					isPageUpPageDownRemappingPtr;
	Boolean*					isKeypadMappingToPFKeysPtr;
	
	// data specific to certain types of session files
	union
	{
		struct
		{
			CFStringRef		command;		//!< command line
		} local;
	} variable;
	
	SessionDescription_Ref		selfRef;	//!< convenient opaque reference back to this structure
};
typedef My_SessionFile const*	My_SessionFileConstPtr;
typedef My_SessionFile*			My_SessionFilePtr;

typedef MemoryBlockPtrLocker< SessionDescription_Ref, My_SessionFile >			My_SessionFilePtrLocker;
typedef LockAcquireRelease< SessionDescription_Ref, My_SessionFile >			My_SessionFileAutoLocker;
typedef MemoryBlockReferenceLocker< SessionDescription_Ref, My_SessionFile >	My_SessionFileReferenceLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_SessionFilePtrLocker&		gSessionFilePtrLocks ()	{ static My_SessionFilePtrLocker x; return x; }
	My_SessionFileReferenceLocker&	gSessionFileRefLocks ()	{ static My_SessionFileReferenceLocker x; return x; }
}

#pragma mark Internal Method Prototypes

static OSStatus		overwriteFile		(SInt16, My_SessionFileConstPtr);
static Boolean		parseFile			(SInt16, My_SessionFilePtr);

#pragma mark Functors

/*!
This is a functor that determines if a pair of
C (null-terminated) strings is identical.

Model of STL Binary Predicate.

(1.0)
*/
#pragma mark isStringPairEqual
class isStringPairEqual:
public std::binary_function< char const*/* argument 1 */, char const*/* argument 2 */, bool/* return */ >
{
public:
	bool
	operator()	(char const*	inString1,
				 char const*	inString2)
	const
	{
		return (CPP_STD::strcmp(inString1, inString2) == 0);
	}

protected:

private:
};


/*!
This is a functor that calls Memory_DisposePtr() on a
pointer - so, on output, the pointer is set to nullptr.
(Presumably, the pointer was allocated with
Memory_NewPtr().)

Model of STL Unary Function.

(1.0)
*/
#pragma mark ptrDisposer
class ptrDisposer:
public std::unary_function< Ptr&/* argument */, void/* return */ >
{
public:
	void
	operator()	(Ptr&	inoutPtr)
	const
	{
		Memory_DisposePtr(&inoutPtr);
	}

protected:

private:
};



#pragma mark Public Methods

/*!
Creates a new session file data model with default
values, returning a reference that you can use to
change or retrieve values and save the file.  The
new reference is locked automatically (so, an
implicit SessionDescription_Retain() call is made).

You must specify the type for the file, which
determines restrictions on the information that
may be stored.

See also SessionDescription_NewFromFile(), which
is appropriate for constructing an in-memory model
from an existing file on disk.

(3.0)
*/
SessionDescription_Ref
SessionDescription_New	(SessionDescription_ContentType		inIntendedContents)
{
	SessionDescription_Ref	result = REINTERPRET_CAST(Memory_NewPtr(sizeof(My_SessionFile)),
														SessionDescription_Ref);
	
	
	if (result != nullptr)
	{
		My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), result);
		
		
		SessionDescription_Retain(result);
		ptr->selfRef = result;
		ptr->fileType = inIntendedContents;
	}
	return result;
}// New


/*!
Scans the specified file, which should be open with
read permission, and parses out all session data
into an in-memory structure.  Returns a reference
to the data, after which it is safe to close the file.

Since session files can represent different kinds of
sessions, a file type is provided to you so you know
what was found.  This indicates what data you can expect
to retrieve, and what will likely be undefined (and
return errors if requested).	

(3.0)
*/
SessionDescription_Ref
SessionDescription_NewFromFile	(SInt16								inFileReferenceNumber,
								 SessionDescription_ContentType*	outFileTypePtr)
{
	SessionDescription_Ref		result = SessionDescription_New(kSessionDescription_ContentTypeUnknown);
	
	
	if (result != nullptr)
	{
		My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), result);
		
		
		parseFile(inFileReferenceNumber, ptr);
		*outFileTypePtr = ptr->fileType;
	}
	return result;
}// NewFromFile


/*!
Adds an additional lock on the specified reference.
This indicates that you are using the file for some
reason, so attempts by anyone else to delete the
file object with SessionDescription_Release() will
fail until you release your lock (and everyone else
releases locks they may have).

(3.0)
*/
void
SessionDescription_Retain	(SessionDescription_Ref		inRef)
{
	gSessionFileRefLocks().acquireLock(inRef);
}// Retain


/*!
Releases one lock on the specified Session File and
deletes the file object *if* no other locks remain.
Your copy of the reference is set to nullptr.

This does NOT close the file, because this module
did not open the file in the first place.  (It is
actually safe to close a file at any time after
creating a Session File reference out of it.)

(3.0)
*/
void
SessionDescription_Release	(SessionDescription_Ref*	inoutRefPtr)
{
	gSessionFileRefLocks().releaseLock(*inoutRefPtr);
	unless (gSessionFileRefLocks().isLocked(*inoutRefPtr))
	{
		{
			My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), *inoutRefPtr);
			
			
			// release data used by all file types
			if (ptr->windowName != nullptr)
			{
				CFRelease(ptr->windowName), ptr->windowName = nullptr;
			}
			if (ptr->currentMacrosName != nullptr)
			{
				CFRelease(ptr->currentMacrosName), ptr->currentMacrosName = nullptr;
			}
			if (ptr->colorTextNormalPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->colorTextNormalPtr, Ptr*));
			}
			if (ptr->colorBackgroundNormalPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->colorBackgroundNormalPtr, Ptr*));
			}
			if (ptr->colorTextBoldPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->colorTextBoldPtr, Ptr*));
			}
			if (ptr->colorBackgroundBoldPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->colorBackgroundBoldPtr, Ptr*));
			}
			if (ptr->colorTextBlinkingPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->colorTextBlinkingPtr, Ptr*));
			}
			if (ptr->colorBackgroundBlinkingPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->colorBackgroundBlinkingPtr, Ptr*));
			}
			if (ptr->invisibleLineCountPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->invisibleLineCountPtr, Ptr*));
			}
			if (ptr->visibleColumnCountPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->visibleColumnCountPtr, Ptr*));
			}
			if (ptr->visibleLineCountPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->visibleLineCountPtr, Ptr*));
			}
			if (ptr->windowPositionHPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->windowPositionHPtr, Ptr*));
			}
			if (ptr->windowPositionVPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->windowPositionVPtr, Ptr*));
			}
			if (ptr->fontSizePtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->fontSizePtr, Ptr*));
			}
			if (ptr->isFTPPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->isFTPPtr, Ptr*));
			}
			if (ptr->isTEKPageInSameWindowPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->isTEKPageInSameWindowPtr, Ptr*));
			}
			if (ptr->isBerkeleyCRPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->isBerkeleyCRPtr, Ptr*));
			}
			if (ptr->isAuthenticationEnabledPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->isAuthenticationEnabledPtr, Ptr*));
			}
			if (ptr->isEncryptionEnabledPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->isEncryptionEnabledPtr, Ptr*));
			}
			if (ptr->isPageUpPageDownRemappingPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->isPageUpPageDownRemappingPtr, Ptr*));
			}
			if (ptr->isKeypadMappingToPFKeysPtr != nullptr)
			{
				Memory_DisposePtr(REINTERPRET_CAST(&ptr->isKeypadMappingToPFKeysPtr, Ptr*));
			}
			
			// check the file type to see which type-specific data to free
			if (ptr->fileType == kSessionDescription_ContentTypeCommand)
			{
				if (nullptr != ptr->variable.local.command)
				{
					CFRelease(ptr->variable.local.command), ptr->variable.local.command = nullptr;
				}
			}
			else
			{
				// ???
			}
		}
		Memory_DisposePtr(REINTERPRET_CAST(inoutRefPtr, Ptr*));
	}
}// Release


/*!
Retrieves the specified yes-no value from the in-memory
model and returns it.

\retval kSessionDescription_ResultOK
if the flag is retrieved successfully

\retval kSessionDescription_ResultDataNotAllowed
if you pass in a file whose type is inappropriate for the
kind of data you request (for example, you ask for a
flag from a macro set containing only strings)

\retval kSessionDescription_ResultDataUnavailable
if the data you asked for could not be delivered

(3.0)
*/
SessionDescription_Result
SessionDescription_GetBooleanData	(SessionDescription_Ref				inRef,
									 SessionDescription_BooleanType		inType,
									 Boolean&							outFlag)
{
	SessionDescription_Result	result = kSessionDescription_ResultOK;
	My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), inRef);
	
	
	switch (inType)
	{
	case kSessionDescription_BooleanTypeTEKPageClears:
		if (ptr->isTEKPageInSameWindowPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outFlag = *(ptr->isTEKPageInSameWindowPtr);
		break;
	
	case kSessionDescription_BooleanTypeRemapCR:
		if (ptr->isBerkeleyCRPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outFlag = *(ptr->isBerkeleyCRPtr);
		break;
	
	case kSessionDescription_BooleanTypePageKeysDoNotControlTerminal:
		if (ptr->isPageUpPageDownRemappingPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outFlag = *(ptr->isPageUpPageDownRemappingPtr);
		break;
	
	case kSessionDescription_BooleanTypeRemapKeypadTopRow:
		if (ptr->isKeypadMappingToPFKeysPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outFlag = *(ptr->isKeypadMappingToPFKeysPtr);
		break;
	
	default:
		// ???
		result = kSessionDescription_ResultDataUnavailable;
		break;
	}
	return result;
}// GetBooleanData


/*!
Retrieves the specified information from the in-memory
model and returns its integer value.

\retval kSessionDescription_ResultOK
if the numerical value is retrieved successfully

\retval kSessionDescription_ResultDataNotAllowed
if you pass in a file whose type is inappropriate for the
kind of data you request (for example, you ask for a
numerical value from a macro set containing only strings)

\retval kSessionDescription_ResultDataUnavailable
if the data you asked for could not be delivered

(3.0)
*/
SessionDescription_Result
SessionDescription_GetIntegerData	(SessionDescription_Ref				inRef,
									 SessionDescription_IntegerType		inType,
									 SInt32&							outNumber)
{
	SessionDescription_Result	result = kSessionDescription_ResultOK;
	My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), inRef);
	
	
	switch (inType)
	{
	case kSessionDescription_IntegerTypeScrollbackBufferLineCount:
		if (ptr->invisibleLineCountPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outNumber = *(ptr->invisibleLineCountPtr);
		break;
	
	case kSessionDescription_IntegerTypeTerminalVisibleColumnCount:
		if (ptr->visibleColumnCountPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outNumber = *(ptr->visibleColumnCountPtr);
		break;
	
	case kSessionDescription_IntegerTypeTerminalVisibleLineCount:
		if (ptr->visibleLineCountPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outNumber = *(ptr->visibleLineCountPtr);
		break;
	
	case kSessionDescription_IntegerTypeWindowContentLeftEdge:
		if (ptr->windowPositionHPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outNumber = *(ptr->windowPositionHPtr);
		break;
	
	case kSessionDescription_IntegerTypeWindowContentTopEdge:
		if (ptr->windowPositionVPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outNumber = *(ptr->windowPositionVPtr);
		break;
	
	case kSessionDescription_IntegerTypeTerminalFontSize:
		if (ptr->fontSizePtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outNumber = *(ptr->fontSizePtr);
		break;
	
	default:
		// ???
		result = kSessionDescription_ResultDataUnavailable;
		break;
	}
	return result;
}// GetIntegerData


/*!
Retrieves the specified information from the in-memory
model and fills in a CGDeviceColor structure using it.

\retval kSessionDescription_ResultOK
if the color is retrieved successfully

\retval kSessionDescription_ResultDataNotAllowed
if you pass in a file whose type is inappropriate for the
kind of data you request (for example, you ask for a color
from a macro set)

\retval kSessionDescription_ResultDataUnavailable
if the data you asked for could not be delivered

(3.0)
*/
SessionDescription_Result
SessionDescription_GetRGBColorData	(SessionDescription_Ref				inRef,
									 SessionDescription_RGBColorType	inType,
									 CGDeviceColor&						outColor)
{
	SessionDescription_Result	result = kSessionDescription_ResultOK;
	My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), inRef);
	
	
	switch (inType)
	{
	case kSessionDescription_RGBColorTypeTextNormal:
		if (ptr->colorTextNormalPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outColor = *(ptr->colorTextNormalPtr);
		break;
	
	case kSessionDescription_RGBColorTypeBackgroundNormal:
		if (ptr->colorBackgroundNormalPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outColor = *(ptr->colorBackgroundNormalPtr);
		break;
	
	case kSessionDescription_RGBColorTypeTextBold:
		if (ptr->colorTextBoldPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outColor = *(ptr->colorTextBoldPtr);
		break;
	
	case kSessionDescription_RGBColorTypeBackgroundBold:
		if (ptr->colorBackgroundBoldPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outColor = *(ptr->colorBackgroundBoldPtr);
		break;
	
	case kSessionDescription_RGBColorTypeTextBlinking:
		if (ptr->colorTextBlinkingPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outColor = *(ptr->colorTextBlinkingPtr);
		break;
	
	case kSessionDescription_RGBColorTypeBackgroundBlinking:
		if (ptr->colorBackgroundBlinkingPtr == nullptr) result = kSessionDescription_ResultDataUnavailable;
		else outColor = *(ptr->colorBackgroundBlinkingPtr);
		break;
	
	default:
		// ???
		result = kSessionDescription_ResultDataUnavailable;
		break;
	}
	return result;
}// GetRGBColorData


/*!
Retrieves the specified information from the in-memory
model and returns a CFString reference to it.

The string is NOT copied; if you want to use the string
for any length of time, you must call CFRetain() on it
(and CFRelease() when you are done).

\retval kSessionDescription_ResultOK
if the string is retrieved successfully

\retval kSessionDescription_ResultDataNotAllowed
if you pass in a file whose type is inappropriate for the
kind of data you request (for example, you ask for a host
name from a local shell session file)

\retval kSessionDescription_ResultDataUnavailable
if the data you asked for could not be delivered

(3.0)
*/
SessionDescription_Result
SessionDescription_GetStringData	(SessionDescription_Ref			inRef,
									 SessionDescription_StringType	inType,
									 CFStringRef&					outString)
{
	SessionDescription_Result	result = kSessionDescription_ResultOK;
	My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), inRef);
	
	
	switch (inType)
	{
	case kSessionDescription_StringTypeCommandLine:
		if (ptr->fileType != kSessionDescription_ContentTypeCommand)
		{
			result = kSessionDescription_ResultDataNotAllowed;
		}
		else
		{
			outString = ptr->variable.local.command;
		}
		break;
	
	case kSessionDescription_StringTypeWindowName:
		outString = ptr->windowName;
		break;
	
	case kSessionDescription_StringTypeMacroSet:
		outString = ptr->currentMacrosName;
		break;
	
	case kSessionDescription_StringTypeTerminalFont:
		outString = ptr->terminalFont;
		break;
	
	case kSessionDescription_StringTypeAnswerBack:
		outString = ptr->answerBack;
		break;
	
	case kSessionDescription_StringTypeToolbarInfo:
		outString = ptr->toolbarInfo;
		break;
	
	default:
		// ???
		result = kSessionDescription_ResultDataUnavailable;
		break;
	}
	return result;
}// GetStringData


/*!
Displays a file selection dialog asking the user to
open a session description file.  In MacTelnet 3.0,
this method responds by sending an “open documents”
Apple Event back to MacTelnet (making it recordable),
instead of directly parsing the file specification
of the chosen file.

(2.6)
*/
void
SessionDescription_Load ()
{
	// IMPORTANT: These should be consistent with declared types in the application "Info.plist".
	void const*			kTypeList[] = { CFSTR("net.macterm.session"),
										CFSTR("com.mactelnet.session"),/* legacy type */
										CFSTR("session"),/* redundant, needed for older systems */
										CFSTR("CONF")/* redundant, needed for older systems */ };
	CFRetainRelease		fileTypes(CFArrayCreate(kCFAllocatorDefault, kTypeList,
									sizeof(kTypeList) / sizeof(CFStringRef), &kCFTypeArrayCallBacks),
									true/* is retained */);
	CFStringRef			promptCFString = nullptr;
	CFStringRef			titleCFString = nullptr;
	
	
	(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptOpenSession, promptCFString);
	(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogTitleOpenSession, titleCFString);
	(Boolean)CocoaBasic_FileOpenPanelDisplay(promptCFString, titleCFString, fileTypes.returnCFArrayRef());
}// Load


/*!
Reads the specified session description file into memory and
then creates a session based on it.

Returns "true" only if the FILE is opened successfully.  Note
that since sessions are created asynchronously, you must use
other means to determine if a new session has been created
successfully (for example, register to receive notification
from the Session Factory module).

(4.0)
*/
Boolean
SessionDescription_ReadFromFile		(FSRef const&	inFile)
{
	OSStatus	error = noErr;
	SInt16		fileRefNum = -1;
	Boolean		result = false;
	
	
	error = FSOpenFork(&inFile, 0/* name length */, nullptr/* name */, fsRdPerm, &fileRefNum);
	if (noErr == error)
	{
		SessionDescription_Ref			sessionFile = nullptr;
		SessionDescription_ContentType	sessionFileType = kSessionDescription_ContentTypeUnknown;
		
		
		sessionFile = SessionDescription_NewFromFile(fileRefNum, &sessionFileType);
		if (sessionFileType == kSessionDescription_ContentTypeUnknown)
		{
			// error
		}
		else
		{
			// create a session using the file data; the following call is asynchronous
			TerminalWindowRef		terminalWindow = SessionFactory_NewTerminalWindowUserFavorite();
			Preferences_ContextRef	workspaceContext = nullptr;
			
			
			(SessionRef)SessionFactory_NewSessionFromDescription(terminalWindow, sessionFile, workspaceContext,
																	0/* window index */);
		}
		SessionDescription_Release(&sessionFile);
		FSCloseFork(fileRefNum), fileRefNum = -1;
	}
	
	return result;
}// ReadFromFile


/*!
Writes the entire in-memory model for the given Session
File to disk, effectively saving any changes made.  The
specified open file obviously needs to be open with
write permissions.

Returns "kSessionDescription_ResultOK" only if
the entire file was written successfully.

(3.0)
*/
SessionDescription_Result
SessionDescription_Save		(SessionDescription_Ref		inRef,
							 SInt16						inFileReferenceNumber)
{
	SessionDescription_Result	result = kSessionDescription_ResultOK;
	OSStatus					error = noErr;
	My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), inRef);
	
	
	error = overwriteFile(inFileReferenceNumber, ptr);
	if (error != noErr) result = kSessionDescription_ResultFileError;
	return result;
}// Save


/*!
Changes the specified yes-no value in the in-memory
model, allocating memory for it if necessary.

\retval kSessionDescription_ResultOK
if the flag is stored successfully

\retval kSessionDescription_ResultInsufficientBufferSpace
if there is not enough memory to store the given flag

\retval kSessionDescription_ResultDataNotAllowed
if you pass in a file whose type is inappropriate for the
kind of data you try to store (for example, you try to
store a flag in a macro set)

\retval kSessionDescription_ResultUnknownType
if the specified data type is unknown

(3.0)
*/
SessionDescription_Result
SessionDescription_SetBooleanData	(SessionDescription_Ref				inRef,
									 SessionDescription_BooleanType		inType,
									 Boolean							inFlag)
{
	SessionDescription_Result	result = kSessionDescription_ResultOK;
	My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), inRef);
	Boolean**					flagPtrPtr = nullptr;
	
	
	switch (inType)
	{
	case kSessionDescription_BooleanTypeTEKPageClears:
		flagPtrPtr = &ptr->isTEKPageInSameWindowPtr;
		break;
	
	case kSessionDescription_BooleanTypeRemapCR:
		flagPtrPtr = &ptr->isBerkeleyCRPtr;
		break;
	
	case kSessionDescription_BooleanTypePageKeysDoNotControlTerminal:
		flagPtrPtr = &ptr->isPageUpPageDownRemappingPtr;
		break;
	
	case kSessionDescription_BooleanTypeRemapKeypadTopRow:
		flagPtrPtr = &ptr->isKeypadMappingToPFKeysPtr;
		break;
	
	default:
		// ???
		result = kSessionDescription_ResultUnknownType;
		break;
	}
	
	// if a valid type was given, copy the numerical value;
	// allocate space for the number if it has not been used yet
	if (flagPtrPtr != nullptr)
	{
		if (*flagPtrPtr == nullptr)
		{
			*flagPtrPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(Boolean)), Boolean*);
		}
		if (*flagPtrPtr == nullptr) result = kSessionDescription_ResultInsufficientBufferSpace;
		else **flagPtrPtr = inFlag;
	}
	
	return result;
}// SetBooleanData


/*!
Changes the specified number in the in-memory model,
allocating memory for it if necessary.

\retval kSessionDescription_ResultOK
if the number is stored successfully

\retval kSessionDescription_ResultInsufficientBufferSpace
if there is not enough memory to store the given value

\retval kSessionDescription_ResultDataNotAllowed
if you pass in a file whose type is inappropriate for the
kind of data you try to store (for example, you try to
store a numerical value in a macro set)

\retval kSessionDescription_ResultUnknownType
if the specified data type is unknown

\retval kSessionDescription_ResultInvalidValue
if "inValidateBeforeStoring" is true and the data you
provide is out of range

(3.0)
*/
SessionDescription_Result
SessionDescription_SetIntegerData	(SessionDescription_Ref				inRef,
									 SessionDescription_IntegerType		inType,
									 SInt32								inNumber,
									 Boolean							inValidateBeforeStoring)
{
	SessionDescription_Result	result = kSessionDescription_ResultOK;
	My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), inRef);
	SInt32**					numberPtrPtr = nullptr;
	
	
	switch (inType)
	{
	case kSessionDescription_IntegerTypeScrollbackBufferLineCount:
		numberPtrPtr = &ptr->invisibleLineCountPtr;
		break;
	
	case kSessionDescription_IntegerTypeTerminalVisibleColumnCount:
		numberPtrPtr = &ptr->visibleColumnCountPtr;
		break;
	
	case kSessionDescription_IntegerTypeTerminalVisibleLineCount:
		numberPtrPtr = &ptr->visibleLineCountPtr;
		break;
	
	case kSessionDescription_IntegerTypeWindowContentLeftEdge:
		numberPtrPtr = &ptr->windowPositionHPtr;
		break;
	
	case kSessionDescription_IntegerTypeWindowContentTopEdge:
		numberPtrPtr = &ptr->windowPositionVPtr;
		break;
	
	case kSessionDescription_IntegerTypeTerminalFontSize:
		numberPtrPtr = &ptr->fontSizePtr;
		break;
	
	default:
		// ???
		result = kSessionDescription_ResultUnknownType;
		break;
	}
	
	// if a valid type was given, copy the numerical value;
	// allocate space for the number if it has not been used yet
	if (numberPtrPtr != nullptr)
	{
		if (*numberPtrPtr == nullptr)
		{
			*numberPtrPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(SInt32)), SInt32*);
		}
		if (*numberPtrPtr == nullptr) result = kSessionDescription_ResultInsufficientBufferSpace;
		else **numberPtrPtr = inNumber;
	}
	
	return result;
}// SetIntegerData


/*!
Changes the specified color in the in-memory model,
allocating memory for it if necessary.

\retval kSessionDescription_ResultOK
if the color is stored successfully

\retval kSessionDescription_ResultInsufficientBufferSpace
if there is not enough memory to store the given color

\retval kSessionDescription_ResultDataNotAllowed
if you pass in a file whose type is inappropriate for the
kind of data you try to store (for example, you try to
store a color in a macro set)

\retval kSessionDescription_ResultUnknownType
if the specified data type is unknown

(3.0)
*/
SessionDescription_Result
SessionDescription_SetRGBColorData	(SessionDescription_Ref				inRef,
									 SessionDescription_RGBColorType	inType,
									 CGDeviceColor const&				inColor)
{
	SessionDescription_Result	result = kSessionDescription_ResultOK;
	My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), inRef);
	CGDeviceColor**				colorPtrPtr = nullptr;
	
	
	switch (inType)
	{
	case kSessionDescription_RGBColorTypeTextNormal:
		colorPtrPtr = &ptr->colorTextNormalPtr;
		break;
	
	case kSessionDescription_RGBColorTypeBackgroundNormal:
		colorPtrPtr = &ptr->colorBackgroundNormalPtr;
		break;
	
	case kSessionDescription_RGBColorTypeTextBold:
		colorPtrPtr = &ptr->colorTextBoldPtr;
		break;
	
	case kSessionDescription_RGBColorTypeBackgroundBold:
		colorPtrPtr = &ptr->colorBackgroundBoldPtr;
		break;
	
	case kSessionDescription_RGBColorTypeTextBlinking:
		colorPtrPtr = &ptr->colorTextBlinkingPtr;
		break;
	
	case kSessionDescription_RGBColorTypeBackgroundBlinking:
		colorPtrPtr = &ptr->colorBackgroundBlinkingPtr;
		break;
	
	default:
		// ???
		result = kSessionDescription_ResultUnknownType;
		break;
	}
	
	// if a valid type was given, copy the color information;
	// allocate space for the color if it has not been used yet
	if (colorPtrPtr != nullptr)
	{
		if (*colorPtrPtr == nullptr)
		{
			*colorPtrPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(CGDeviceColor)), CGDeviceColor*);
		}
		if (*colorPtrPtr == nullptr) result = kSessionDescription_ResultInsufficientBufferSpace;
		else **colorPtrPtr = inColor;
	}
	
	return result;
}// SetRGBColorData


/*!
Updates the specified information in the in-memory
model.  The given string is retained if necessary.

\retval kSessionDescription_ResultOK
if the string is stored successfully

\retval kSessionDescription_ResultDataNotAllowed
if you pass in a file type inappropriate for the type
of data you store (for example, you try to set a host
name for a local shell session file)

\retval kSessionDescription_ResultUnknownType
if the specified data type is unknown

\retval kSessionDescription_ResultInvalidValue
if "inValidateBeforeStoring" is true and the string
you provide has the wrong format; this is very much
data-type dependent (for example, if you are storing
a host name, there are certain patterns the string
must match)

(3.0)
*/
SessionDescription_Result
SessionDescription_SetStringData	(SessionDescription_Ref			inRef,
									 SessionDescription_StringType	inType,
									 CFStringRef					inString,
									 Boolean						inValidateBeforeStoring)
{
	SessionDescription_Result	result = kSessionDescription_ResultOK;
	My_SessionFileAutoLocker	ptr(gSessionFilePtrLocks(), inRef);
	
	
	switch (inType)
	{
	case kSessionDescription_StringTypeCommandLine:
		if (ptr->fileType != kSessionDescription_ContentTypeCommand)
		{
			result = kSessionDescription_ResultDataNotAllowed;
		}
		else
		{
			if (nullptr != ptr->variable.local.command) CFRelease(ptr->variable.local.command);
			CFRetain(inString);
			ptr->variable.local.command = inString;
		}
		break;
	
	case kSessionDescription_StringTypeWindowName:
		CFRetain(inString);
		if (ptr->windowName != nullptr) CFRelease(ptr->windowName);
		ptr->windowName = inString;
		break;
	
	case kSessionDescription_StringTypeMacroSet:
		CFRetain(inString);
		if (ptr->currentMacrosName != nullptr) CFRelease(ptr->currentMacrosName);
		ptr->currentMacrosName = inString;
		break;
	
	case kSessionDescription_StringTypeTerminalFont:
		CFRetain(inString);
		if (ptr->terminalFont != nullptr) CFRelease(ptr->terminalFont);
		ptr->terminalFont = inString;
		break;
	
	case kSessionDescription_StringTypeAnswerBack:
		CFRetain(inString);
		if (ptr->answerBack != nullptr) CFRelease(ptr->answerBack);
		ptr->answerBack = inString;
		break;
	
	case kSessionDescription_StringTypeToolbarInfo:
		CFRetain(inString);
		if (ptr->toolbarInfo != nullptr) CFRelease(ptr->toolbarInfo);
		ptr->toolbarInfo = inString;
		break;
	
	default:
		// ???
		result = kSessionDescription_ResultUnknownType;
		break;
	}
	return result;
}// SetStringData


#pragma mark Internal Methods

/*!
Writes the specified in-memory data to the given
file (opened with write permission), replacing
any existing content.  You must close the file
yourself (because you opened it).

Returns "noErr" only if the file was written
completely.

(3.0)
*/
OSStatus
overwriteFile	(SInt16						inFileReferenceNumber,
				 My_SessionFileConstPtr		inoutDataPtr)
{
	TextDataFile_Ref	writer = TextDataFile_New(inFileReferenceNumber);
	OSStatus			result = noErr;
	
	
	if (writer == nullptr) result = memFullErr;
	else
	{
		Boolean		makeCompatibleWithNCSATelnet = false; // whether to constrain to 23 specific keys
		Boolean		success = false;
		//int			requiredSize = 0;
		
		
		// IMPORTANT:
		//
		// For NCSA Telnet 2.6 compatibility, keys should be written in this
		// exact format, with exactly 23 keys in this order:
		//		commandkeys = yes|no
		//		name= "window name"
		//		host= "hostname"
		//		port= number
		//		scrollback= number
		//		erase = backspace|delete
		//		size = {t,l,b,r}
		//		vtwidth = number
		//		tekclear = yes|no
		//		rgb0 = {r,g,b}
		//		rgb1 = {r,g,b}
		//		rgb2 = {r,g,b}
		//		rgb3 = {r,g,b}
		//		font = "Fontname"
		//		fsize= number
		//		nlines= number
		//		keystop= number
		//		keygo= number
		//		keyip= number
		//		crmap= 0|1
		//		tekem= -1|0|1
		//		answerback= "emtype"
		// If you write any more or fewer keys than this, or you write them
		// out of order, then NCSA Telnet 2.6 will be unable to import them.
		
		{
			Boolean		flag = true;
			
			
			success = TextDataFile_AddNameValueFlag(writer, nullptr/* class */, "commandkeys", flag);
	  	}
		
		if (inoutDataPtr->windowName != nullptr)
		{
			success = TextDataFile_AddNameValueCFString(writer, nullptr/* class */, "name",
														inoutDataPtr->windowName);
	  	}
		
		if (inoutDataPtr->fileType == kSessionDescription_ContentTypeCommand)
		{
			if (nullptr != inoutDataPtr->variable.local.command)
			{
				success = TextDataFile_AddNameValueCFString(writer, nullptr/* class */, "command",
															inoutDataPtr->variable.local.command);
	  		}
		}
		else
		{
			// ???
		}
		
		if (inoutDataPtr->invisibleLineCountPtr != nullptr)
		{
			success = TextDataFile_AddNameValueNumber(writer, nullptr/* class */, "scrollback",
														*(inoutDataPtr->invisibleLineCountPtr));
		}
		
		//nameValuePairToFile("erase", (connectionDataPtr->bsdel) ? "delete" : "backspace", fn);
		
		//(OSStatus)GetWindowBounds(TerminalView_GetWindow(view), kWindowContentRgn, &rect);
		//requiredSize = snprintf(temp2, sizeof(temp2), "{%d,%d,%d,%d}", rect.top, rect.left, rect.bottom, rect.right);
		//nameValuePairToFile("size", temp2, fn);
		
		if ((inoutDataPtr->windowPositionHPtr != nullptr) && (inoutDataPtr->windowPositionVPtr != nullptr))
		{
			Rect	bounds;
			
			
			bounds.left = *(inoutDataPtr->windowPositionHPtr);
			bounds.top = *(inoutDataPtr->windowPositionVPtr);
			bounds.right = bounds.left; // temporary
			bounds.bottom = bounds.top; // temporary
			success = TextDataFile_AddNameValueRectangle(writer, nullptr/* class */, "size", &bounds);
		}
		
		if (inoutDataPtr->visibleColumnCountPtr != nullptr)
		{
			success = TextDataFile_AddNameValueNumber(writer, nullptr/* class */, "vtwidth",
														*(inoutDataPtr->visibleColumnCountPtr));
		}

		if (inoutDataPtr->isTEKPageInSameWindowPtr != nullptr)
		{
			success = TextDataFile_AddNameValueFlag(writer, nullptr/* class */, "tekclear",
													*(inoutDataPtr->isTEKPageInSameWindowPtr));
		}
		
		// save colors; order is important for compatibility with NCSA Telnet sets
		{
			if (inoutDataPtr->colorTextNormalPtr != nullptr)
			{
				success = TextDataFile_AddNameValueRGBColor(writer, nullptr/* class */, "rgb0",
															inoutDataPtr->colorTextNormalPtr);
			}
			
			if (inoutDataPtr->colorBackgroundNormalPtr != nullptr)
			{
				success = TextDataFile_AddNameValueRGBColor(writer, nullptr/* class */, "rgb1",
															inoutDataPtr->colorBackgroundNormalPtr);
			}
			
			if (inoutDataPtr->colorTextBlinkingPtr != nullptr)
			{
				success = TextDataFile_AddNameValueRGBColor(writer, nullptr/* class */, "rgb2",
															inoutDataPtr->colorTextBlinkingPtr);
			}
			
			if (inoutDataPtr->colorBackgroundBlinkingPtr != nullptr)
			{
				success = TextDataFile_AddNameValueRGBColor(writer, nullptr/* class */, "rgb3",
															inoutDataPtr->colorBackgroundBlinkingPtr);
			}
		}
		
		if (inoutDataPtr->terminalFont != nullptr)
		{
			success = TextDataFile_AddNameValueCFString(writer, nullptr/* class */, "font",
														inoutDataPtr->terminalFont);
	  	}
		
		if (inoutDataPtr->fontSizePtr != nullptr)
		{
			success = TextDataFile_AddNameValueNumber(writer, nullptr/* class */, "fsize",
														*(inoutDataPtr->fontSizePtr));
		}
		
		if (inoutDataPtr->visibleLineCountPtr != nullptr)
		{
			success = TextDataFile_AddNameValueNumber(writer, nullptr/* class */, "nlines",
														*(inoutDataPtr->visibleLineCountPtr));
		}
		
		//requiredSize = snprintf(temp2, sizeof(temp2), "%d", connectionDataPtr->controlKey.suspend);
		//nameValuePairToFile("keystop", temp2, fn);
		//requiredSize = snprintf(temp2, sizeof(temp2), "%d", connectionDataPtr->controlKey.resume);
		//nameValuePairToFile("keygo", temp2, fn);
		//requiredSize = snprintf(temp2, sizeof(temp2), "%d", connectionDataPtr->controlKey.interrupt);
		//nameValuePairToFile("keyip", temp2, fn);
		
		if (inoutDataPtr->isBerkeleyCRPtr != nullptr)
		{
			success = TextDataFile_AddNameValueFlag(writer, nullptr/* class */, "crmap",
													*(inoutDataPtr->isBerkeleyCRPtr));
		}
		
		//requiredSize = snprintf(temp2, sizeof(temp2), "%d", connectionDataPtr->TEK.mode);
		//nameValuePairToFile("tekem", temp2, fn);
		
		if (inoutDataPtr->answerBack != nullptr)
		{
			success = TextDataFile_AddNameValueCFString(writer, nullptr/* class */, "answerback",
														inoutDataPtr->answerBack);
	  	}
		
		// these flags are new to MacTelnet 2.7 and beyond
		unless (makeCompatibleWithNCSATelnet)
		{
			if (inoutDataPtr->colorTextBoldPtr != nullptr)
			{
				success = TextDataFile_AddNameValueRGBColor(writer, nullptr/* class */, "rgb4",
															inoutDataPtr->colorTextBoldPtr);
			}
			if (inoutDataPtr->colorBackgroundBoldPtr != nullptr)
			{
				success = TextDataFile_AddNameValueRGBColor(writer, nullptr/* class */, "rgb5",
															inoutDataPtr->colorBackgroundBoldPtr);
			}
			
			if (inoutDataPtr->isAuthenticationEnabledPtr != nullptr)
			{
				success = TextDataFile_AddNameValueFlag(writer, nullptr/* class */, "authenticate",
														*(inoutDataPtr->isAuthenticationEnabledPtr));
			}
			
			if (inoutDataPtr->isEncryptionEnabledPtr != nullptr)
			{
				success = TextDataFile_AddNameValueFlag(writer, nullptr/* class */, "encrypt",
														*(inoutDataPtr->isEncryptionEnabledPtr));
			}
			
			if (inoutDataPtr->isPageUpPageDownRemappingPtr != nullptr)
			{
				success = TextDataFile_AddNameValueFlag(writer, nullptr/* class */, "pageup",
														*(inoutDataPtr->isPageUpPageDownRemappingPtr));
			}
			
			if (inoutDataPtr->isKeypadMappingToPFKeysPtr != nullptr)
			{
				success = TextDataFile_AddNameValueFlag(writer, nullptr/* class */, "keypad",
														*(inoutDataPtr->isKeypadMappingToPFKeysPtr));
			}
			
			if (inoutDataPtr->isFTPPtr != nullptr)
			{
				success = TextDataFile_AddNameValueFlag(writer, nullptr/* class */, "ftp", *(inoutDataPtr->isFTPPtr));
			}
			
			if (inoutDataPtr->toolbarInfo != nullptr)
			{
				success = TextDataFile_AddNameValueCFString(writer, nullptr/* class */, "toolbar",
															inoutDataPtr->toolbarInfo);
			}
			
			if (inoutDataPtr->currentMacrosName != nullptr)
			{
				success = TextDataFile_AddNameValueCFString(writer, nullptr/* class */, "macros",
															inoutDataPtr->currentMacrosName);
			}
		}
		
		TextDataFile_Dispose(&writer);
	}
	return result;
}// overwriteFile


/*!
Parses the specified key-value pair text file
and stores all the data in the given structure.
The structure should be pre-initialized to zeroes
so that any unset references remain nullptr.

Returns "true" only if the file was read without
errors.

(3.0)
*/
static Boolean
parseFile	(SInt16				inFileReferenceNumber,
			 My_SessionFilePtr	inoutDataPtr)
{
	TextDataFile_Ref	file = TextDataFile_New(inFileReferenceNumber);
	Boolean				result = false;
	
	
	if (file != nullptr)
	{
		typedef std::vector< Ptr >	CStringList;
		typedef hash_map_namespace::hash_map
				<
					char const*,								// key type - name string
					CStringList::size_type,						// value type - index into value array
					hash_map_namespace::hash< char const* >,	// hash code generator
					isStringPairEqual							// key comparator
				> KeyNameToKeyValueArrayIndexHashMap;
		char									keyName[32];
		char									keyValue[255];
		KeyNameToKeyValueArrayIndexHashMap		hashTable;
		CStringList								nameList;
		CStringList								valueList;
		
		
		// read all key-value pairs from the file and stick them into
		// a hash table in memory for reasonable efficient access
		while (TextDataFile_GetNextNameValue(file, keyName, sizeof(keyName), keyValue, sizeof(keyValue),
												true/* strip surrounding quotes, etc. */))
		{
			// canonicalize key names to lower-case in hash table
			{
				char*	charPtr = keyName;
				
				
				while (*charPtr)
				{
					*charPtr = CPP_STD::tolower(*charPtr);
					++charPtr;
				}
			}
			
			// create new value in the list, and assign it
			try
			{
			#ifndef NDEBUG // this is unset when assert() is disabled
				CStringList::size_type const	nameListLengthBeforeAdd = nameList.size();
				CStringList::size_type const	valueListLengthBeforeAdd = valueList.size();
			#endif
				char* const		nameString = Memory_NewPtr(1 + CPP_STD::strlen(keyName));
				char* const		valueString = Memory_NewPtr(1 + CPP_STD::strlen(keyValue));
				
				
				if ((nameString == nullptr) || (valueString == nullptr)) throw std::bad_alloc();
				CPP_STD::strcpy(nameString, keyName);
				CPP_STD::strcpy(valueString, keyValue);
				nameList.push_back(nameString); // may also throw std::bad_alloc
				assert(nameList.size() == (1 + nameListLengthBeforeAdd));
				valueList.push_back(valueString); // may also throw std::bad_alloc
				assert(valueList.size() == (1 + valueListLengthBeforeAdd));
				
				// add a key-value pair to the hash; remember, the value
				// of the hash is the index into the value array, NOT
				// the actual string value read from a line of the file
				hashTable[nameString] = valueListLengthBeforeAdd;
				assert(hashTable.find(nameString) != hashTable.end());
			}
			catch (std::bad_alloc)
			{
				// not enough memory; cannot read file properly!
				break;
			}
		}
		
		// now look up all known keys and read the data into a
		// structure in memory; note that unlike NCSA Telnet 2.6,
		// MacTelnet 3.0 is fairly flexible and is willing to
		// proceed even if most expected keys from the file are
		// missing or if unexpected (ignored) extra keys exist;
		// also unlike 2.6, the processing order is unimportant
		{
			KeyNameToKeyValueArrayIndexHashMap::const_iterator	keyNameToKeyValueArrayIndexIterator;
			
			
			// a name is required, and either a host or a command;
			// otherwise, MacTelnet 3.0 doesn’t care, it just uses
			// default values for other session attributes
			if ((hashTable.find("name") != hashTable.end()) &&
				((hashTable.find("host") != hashTable.end()) ||
					(hashTable.find("command") != hashTable.end())))
			{
				SessionDescription_Result	sessionFileError = kSessionDescription_ResultOK;
				
				
				// at this point, the read is considered successful
				result = true;
				
				// set window name (required)
				sessionFileError = SessionDescription_SetStringData
									(inoutDataPtr->selfRef, kSessionDescription_StringTypeWindowName,
										CFStringCreateWithCString
										(kCFAllocatorDefault, valueList.at(hashTable.find("name")->second),
											kCFStringEncodingMacRoman),
										true/* validate before storing */);
				
				// this file should describe a local session
				inoutDataPtr->fileType = kSessionDescription_ContentTypeCommand;
				keyNameToKeyValueArrayIndexIterator = hashTable.find("command");
				// TEMPORARY - consider a function call here that looks for the "host" key
				// (older NCSA Telnet format) and converts it to an equivalent Unix command
				// that looks up the host and port, etc. on a server
				
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					sessionFileError = SessionDescription_SetStringData
										(inoutDataPtr->selfRef, kSessionDescription_StringTypeCommandLine,
											CFStringCreateWithCString
											(kCFAllocatorDefault,
												valueList.at(keyNameToKeyValueArrayIndexIterator->second),
												kCFStringEncodingMacRoman),
											true/* validate before storing */);
				}
				
				// set window size and position, if given
				keyNameToKeyValueArrayIndexIterator = hashTable.find("size");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					Rect	bounds;
					
					
					if (TextDataFile_StringToRectangle(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
														&bounds))
					{
						// UNIMPLEMENTED
					}
				}
				
				// set terminal scrollback buffer size
				keyNameToKeyValueArrayIndexIterator = hashTable.find("scrollback");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					SInt32		lineCount = 0;
					
					
					if (TextDataFile_StringToNumber(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&lineCount))
					{
						sessionFileError = SessionDescription_SetIntegerData
											(inoutDataPtr->selfRef,
												kSessionDescription_IntegerTypeScrollbackBufferLineCount,
												lineCount, true/* validate before storing */);
					}
				}
				
				// set macros
				keyNameToKeyValueArrayIndexIterator = hashTable.find("key0");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//Macros_Set(Macros_ReturnActiveSet(), 0, valueList.at(keyNameToKeyValueArrayIndexIterator->second));
				}
				keyNameToKeyValueArrayIndexIterator = hashTable.find("key1");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//Macros_Set(Macros_ReturnActiveSet(), 1, valueList.at(keyNameToKeyValueArrayIndexIterator->second));
				}
				keyNameToKeyValueArrayIndexIterator = hashTable.find("key2");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//Macros_Set(Macros_ReturnActiveSet(), 2, valueList.at(keyNameToKeyValueArrayIndexIterator->second));
				}
				keyNameToKeyValueArrayIndexIterator = hashTable.find("key3");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//Macros_Set(Macros_ReturnActiveSet(), 3, valueList.at(keyNameToKeyValueArrayIndexIterator->second));
				}
				keyNameToKeyValueArrayIndexIterator = hashTable.find("key4");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//Macros_Set(Macros_ReturnActiveSet(), 4, valueList.at(keyNameToKeyValueArrayIndexIterator->second));
				}
				keyNameToKeyValueArrayIndexIterator = hashTable.find("key5");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//Macros_Set(Macros_ReturnActiveSet(), 5, valueList.at(keyNameToKeyValueArrayIndexIterator->second));
				}
				keyNameToKeyValueArrayIndexIterator = hashTable.find("key6");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//Macros_Set(Macros_ReturnActiveSet(), 6, valueList.at(keyNameToKeyValueArrayIndexIterator->second));
				}
				keyNameToKeyValueArrayIndexIterator = hashTable.find("key7");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//Macros_Set(Macros_ReturnActiveSet(), 7, valueList.at(keyNameToKeyValueArrayIndexIterator->second));
				}
				keyNameToKeyValueArrayIndexIterator = hashTable.find("key8");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//Macros_Set(Macros_ReturnActiveSet(), 8, valueList.at(keyNameToKeyValueArrayIndexIterator->second));
				}
				keyNameToKeyValueArrayIndexIterator = hashTable.find("key9");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//Macros_Set(Macros_ReturnActiveSet(), 9, valueList.at(keyNameToKeyValueArrayIndexIterator->second));
				}
				
				// set “menus have key equivalents” flag
				keyNameToKeyValueArrayIndexIterator = hashTable.find("commandkeys");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					// this is not supported by MacTelnet 3.0 right now
					//Boolean		menusHaveCommandKeys = false;
					
					
					//if (TextDataFile_StringToFlag(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
					//							`	&menusHaveCommandKeys))
					//{
					//}
				}
				
				// set backspace/delete behavior
				keyNameToKeyValueArrayIndexIterator = hashTable.find("erase");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					if (!CPP_STD::strcmp(valueList.at(keyNameToKeyValueArrayIndexIterator->second), "backspace"))
					{
						// delete is backspace
						//setSessionPtr->bksp = 0;
					}
					else
					{
						// delete is delete
						//setSessionPtr->bksp = 1;
					}
				}
				
				// set terminal width in columns; this key
				// has two aliases, "width" and "vtwidth"
				keyNameToKeyValueArrayIndexIterator = hashTable.find("width");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					SInt32		columnCount = 0;
					
					
					if (TextDataFile_StringToNumber(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&columnCount))
					{
						sessionFileError = SessionDescription_SetIntegerData
											(inoutDataPtr->selfRef,
												kSessionDescription_IntegerTypeTerminalVisibleColumnCount,
												columnCount, true/* validate before storing */);
					}
				}
				keyNameToKeyValueArrayIndexIterator = hashTable.find("vtwidth");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					SInt32		columnCount = 0;
					
					
					if (TextDataFile_StringToNumber(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&columnCount))
					{
						sessionFileError = SessionDescription_SetIntegerData
											(inoutDataPtr->selfRef,
												kSessionDescription_IntegerTypeTerminalVisibleColumnCount,
												columnCount, true/* validate before storing */);
					}
				}
				
				// set terminal height in lines
				keyNameToKeyValueArrayIndexIterator = hashTable.find("nlines");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					SInt32		lineCount = 0;
					
					
					if (TextDataFile_StringToNumber(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&lineCount))
					{
						sessionFileError = SessionDescription_SetIntegerData
											(inoutDataPtr->selfRef,
												kSessionDescription_IntegerTypeTerminalVisibleLineCount,
												lineCount, true/* validate before storing */);
					}
				}
				
				// set “TEK PAGE clears screen” setting
				keyNameToKeyValueArrayIndexIterator = hashTable.find("tekclear");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					Boolean		pageClearsScreen = false;
					
					
					if (TextDataFile_StringToFlag(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&pageClearsScreen))
					{
						sessionFileError = SessionDescription_SetBooleanData
											(inoutDataPtr->selfRef,
												kSessionDescription_BooleanTypeTEKPageClears,
												pageClearsScreen);
					}
				}
				
				// set normal text color
				keyNameToKeyValueArrayIndexIterator = hashTable.find("rgb0");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					CGDeviceColor	normalTextColor;
					
					
					if (TextDataFile_StringToRGBColor(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
														&normalTextColor))
					{
						sessionFileError = SessionDescription_SetRGBColorData
											(inoutDataPtr->selfRef,
												kSessionDescription_RGBColorTypeTextNormal,
												normalTextColor);
					}
				}
				
				// set normal background color
				keyNameToKeyValueArrayIndexIterator = hashTable.find("rgb1");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					CGDeviceColor	normalBackgroundColor;
					
					
					if (TextDataFile_StringToRGBColor(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
														&normalBackgroundColor))
					{
						sessionFileError = SessionDescription_SetRGBColorData
											(inoutDataPtr->selfRef,
												kSessionDescription_RGBColorTypeBackgroundNormal,
												normalBackgroundColor);
					}
				}
				
				// set blinking text color
				keyNameToKeyValueArrayIndexIterator = hashTable.find("rgb2");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					CGDeviceColor	blinkingTextColor;
					
					
					if (TextDataFile_StringToRGBColor(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
														&blinkingTextColor))
					{
						sessionFileError = SessionDescription_SetRGBColorData
											(inoutDataPtr->selfRef,
												kSessionDescription_RGBColorTypeTextBlinking,
												blinkingTextColor);
					}
				}
				
				// set blinking background color
				keyNameToKeyValueArrayIndexIterator = hashTable.find("rgb3");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					CGDeviceColor	blinkingBackgroundColor;
					
					
					if (TextDataFile_StringToRGBColor(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
														&blinkingBackgroundColor))
					{
						sessionFileError = SessionDescription_SetRGBColorData
											(inoutDataPtr->selfRef,
												kSessionDescription_RGBColorTypeBackgroundBlinking,
												blinkingBackgroundColor);
					}
				}
				
				// set bold text color
				keyNameToKeyValueArrayIndexIterator = hashTable.find("rgb4");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					CGDeviceColor	boldTextColor;
					
					
					if (TextDataFile_StringToRGBColor(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
														&boldTextColor))
					{
						sessionFileError = SessionDescription_SetRGBColorData
											(inoutDataPtr->selfRef,
												kSessionDescription_RGBColorTypeTextBold,
												boldTextColor);
					}
				}
				
				// set bold background color
				keyNameToKeyValueArrayIndexIterator = hashTable.find("rgb5");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					CGDeviceColor	boldBackgroundColor;
					
					
					if (TextDataFile_StringToRGBColor(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
														&boldBackgroundColor))
					{
						sessionFileError = SessionDescription_SetRGBColorData
											(inoutDataPtr->selfRef,
												kSessionDescription_RGBColorTypeBackgroundBold,
												boldBackgroundColor);
					}
				}
				
				// set font
				keyNameToKeyValueArrayIndexIterator = hashTable.find("font");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					sessionFileError = SessionDescription_SetStringData
										(inoutDataPtr->selfRef, kSessionDescription_StringTypeTerminalFont,
											CFStringCreateWithCString
											(kCFAllocatorDefault,
												valueList.at(keyNameToKeyValueArrayIndexIterator->second),
												kCFStringEncodingMacRoman),
											true/* validate before storing */);
				}
				
				// set font size
				keyNameToKeyValueArrayIndexIterator = hashTable.find("fsize");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					SInt32		fontSize = 0;
					
					
					if (TextDataFile_StringToNumber(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&fontSize))
					{
						sessionFileError = SessionDescription_SetIntegerData
											(inoutDataPtr->selfRef,
												kSessionDescription_IntegerTypeTerminalFontSize,
												fontSize, true/* validate before storing */);
					}
				}
				
				// set control key sequence for suspend
				keyNameToKeyValueArrayIndexIterator = hashTable.find("keystop");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					SInt32		stopCode = 0;
					
					
					if (TextDataFile_StringToNumber(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&stopCode))
					{
						//setSessionPtr->skey = stopCode;
					}
				}
				
				// set control key sequence for resume
				keyNameToKeyValueArrayIndexIterator = hashTable.find("keygo");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					SInt32		goCode = 0;
					
					
					if (TextDataFile_StringToNumber(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&goCode))
					{
						//setSessionPtr->qkey = goCode;
					}
				}
				
				// set control key sequence for interrupt-process
				keyNameToKeyValueArrayIndexIterator = hashTable.find("keyip");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					SInt32		interruptCode = 0;
					
					
					if (TextDataFile_StringToNumber(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&interruptCode))
					{
						//setSessionPtr->ckey = interruptCode;
					}
				}
				
				// set whether carriage returns should be CR-NULL instead of CR-LF
				keyNameToKeyValueArrayIndexIterator = hashTable.find("crmap");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					Boolean		remapCR = false;
					
					
					if (TextDataFile_StringToFlag(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&remapCR))
					{
						sessionFileError = SessionDescription_SetBooleanData
											(inoutDataPtr->selfRef,
												kSessionDescription_BooleanTypeRemapCR,
												remapCR);
					}
				}
				
				// set whether line mode is enabled
				keyNameToKeyValueArrayIndexIterator = hashTable.find("linemode");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					Boolean		lineMode = false;
					
					
					if (TextDataFile_StringToFlag(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&lineMode))
					{
						//setSessionPtr->linemode = lineMode;
					}
				}
				
				// set 8-bit or 7-bit ASCII
				keyNameToKeyValueArrayIndexIterator = hashTable.find("eightbit");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					Boolean		eightBit = false;
					
					
					if (TextDataFile_StringToFlag(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&eightBit))
					{
						//setTerminalPtr->usesEightBits = eightBit;
					}
				}
				
				// set whether this is an FTP session
				// not needed?
				
				// set serial flag
				//keyNameToKeyValueArrayIndexIterator = hashTable.find("serial");
				//if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				//{
				//	// not supported by MacTelnet 3.0
				//}
				
				// set port flag
				//keyNameToKeyValueArrayIndexIterator = hashTable.find("port");
				//if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				//{
				//	// not supported by MacTelnet 3.0
				//}
				
				// set name of translation table to use
				keyNameToKeyValueArrayIndexIterator = hashTable.find("translation");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					//StringUtilities_CToP(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
					//						setSessionPtr->TranslationTable);
				}
				
				// set TEK emulation mode and command set
				keyNameToKeyValueArrayIndexIterator = hashTable.find("tekem");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					SInt32		mode = 0;
					
					
					if (TextDataFile_StringToNumber(valueList.at(keyNameToKeyValueArrayIndexIterator->second), &mode))
					{
						//setSessionPtr->tektype = mode;
					}
				}
				
				// set answer-back message (a.k.a. perceived terminal type)
				keyNameToKeyValueArrayIndexIterator = hashTable.find("answerback");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					sessionFileError = SessionDescription_SetStringData
										(inoutDataPtr->selfRef, kSessionDescription_StringTypeAnswerBack,
											CFStringCreateWithCString
											(kCFAllocatorDefault,
												valueList.at(keyNameToKeyValueArrayIndexIterator->second),
												kCFStringEncodingMacRoman),
											true/* validate before storing */);
				}
				
				// set Kerberos authentication flag
				// not needed?
				
				// set Kerberos encryption flag
				// not needed?
				
				// set page up, page down, etc. key map flag
				keyNameToKeyValueArrayIndexIterator = hashTable.find("pageup");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					Boolean		mapsPageJumpKeys = false;
					
					
					if (TextDataFile_StringToFlag(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&mapsPageJumpKeys))
					{
						sessionFileError = SessionDescription_SetBooleanData
											(inoutDataPtr->selfRef,
												kSessionDescription_BooleanTypePageKeysDoNotControlTerminal,
												mapsPageJumpKeys);
					}
				}
				
				// set keypad-remap-for-VT220 flag
				keyNameToKeyValueArrayIndexIterator = hashTable.find("keypad");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					Boolean		remapKeypad = false;
					
					
					if (TextDataFile_StringToFlag(valueList.at(keyNameToKeyValueArrayIndexIterator->second),
													&remapKeypad))
					{
						sessionFileError = SessionDescription_SetBooleanData
											(inoutDataPtr->selfRef,
												kSessionDescription_BooleanTypeRemapKeypadTopRow,
												remapKeypad);
					}
				}
				
				// set toolbar info
				keyNameToKeyValueArrayIndexIterator = hashTable.find("toolbar");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					sessionFileError = SessionDescription_SetStringData
										(inoutDataPtr->selfRef, kSessionDescription_StringTypeToolbarInfo,
											CFStringCreateWithCString
											(kCFAllocatorDefault,
												valueList.at(keyNameToKeyValueArrayIndexIterator->second),
												kCFStringEncodingMacRoman),
											true/* validate before storing */);
				}
				
				// set macro set
				keyNameToKeyValueArrayIndexIterator = hashTable.find("macros");
				if (keyNameToKeyValueArrayIndexIterator != hashTable.end())
				{
					sessionFileError = SessionDescription_SetStringData
										(inoutDataPtr->selfRef, kSessionDescription_StringTypeMacroSet,
											CFStringCreateWithCString
											(kCFAllocatorDefault,
												valueList.at(keyNameToKeyValueArrayIndexIterator->second),
												kCFStringEncodingMacRoman),
											true/* validate before storing */);
				}
				
				// all other keys are ignored; one possible course of action
				// is to modify the above so keys are deleted from the hash
				// as they are processed, then at this point a quick check
				// for a non-empty hash table could prompt a warning message
				// telling the user one or more keys from the given file were
				// not recognized...
			}
		}
		
		// clean up
		std::for_each(nameList.begin(), nameList.end(), ptrDisposer());
		std::for_each(valueList.begin(), valueList.end(), ptrDisposer());
		TextDataFile_Dispose(&file);
	}
	
	return result;
}// parseFile

// BELOW IS REQUIRED NEWLINE TO END FILE
