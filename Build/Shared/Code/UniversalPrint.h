/*!	\file UniversalPrint.h
	\brief Use this library to allow your application to print
	using either the Mac OS 8 or Mac OS X printing managers,
	depending upon which version of this library is installed.
	
	By using this library’s interface for your printing needs,
	you only have to write one set of source code (one that
	interfaces with this library), and yet your application can
	use either of the two printing architectures transparently!
	
	This library can be built targeted at Carbon, or not.  Thus,
	two different versions of this library exist, each having
	identical application programming interfaces.  Identical
	interfaces in both means that your application generally
	never needs to worry about which of the two editions is
	actually in place at run time.  Nevertheless, the method
	UniversalPrint_ReturnArchitecture() is provided so you can
	find out which runtime version is being used, when necessary.
*/
/*###############################################################

	Universal Printing Library 1.1
	© 1998-2007 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "UniversalDefines.h"

#ifndef __UNIVERSALPRINT__
#define __UNIVERSALPRINT__

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Releases.h>



#pragma mark Types

typedef struct UniversalPrint_OpaqueContext*	UniversalPrint_ContextRef;

#if TARGET_API_MAC_OS8
#	define SIZE_OF_TPRINT		sizeof(TPrint)
#else
	// Carbon doesn’t define this type, but its size is defined in <Printing.h>
#	define SIZE_OF_TPRINT		(2 + 14 + 8 + 8 + 14 + 16 + 20 + 120)
#endif

typedef SInt16 UniversalPrint_Architecture;
enum
{
	// one of these constants is returned from UniversalPrint_ReturnArchitecture()
	kUniversalPrint_ArchitectureTraditional = 0,
	kUniversalPrint_ArchitectureMacOSX = 1	// Carbon printing
};

typedef UInt16 UniversalPrint_Mode;
enum
{
	kUniversalPrint_ModeNormal = 0,		// “show job dialog”
	kUniversalPrint_ModeOneCopy = 1		// “don’t show job dialog”
};

struct UniversalPrint_SavedContext
{
	UniversalPrint_Architecture		architecture;
	union
	{
		// if the value of field "architecture" is kUniversalPrint_ArchitectureTraditional...
		struct
		{
			Handle		storageTHPrint;
		} traditional;
		
		// if the value of field "architecture" is kUniversalPrint_ArchitectureMacOSX...
		struct
		{
			Handle		storageFlattenedPrintSettings;
			Handle		storageFlattenedPageFormat;
		} osX;
	} data;
};
typedef UniversalPrint_SavedContext*	UniversalPrint_SavedContextPtr;



/*!
Printing Idle Method

This routine should, at a minimum, check for -.
keypresses and cancel printing.
*/
typedef pascal void (*UniversalPrint_IdleProcPtr)();
inline void
UniversalPrint_InvokeIdleProc	(UniversalPrint_IdleProcPtr		inUserRoutine)
{
	(*inUserRoutine)();
}

#if TARGET_API_MAC_CARBON
/*!
Printing Dialog Complete Method

Routines of this type are invoked whenever a
window-modal printing dialog box closes.

A dialog box may modify print settings; use
accessor routines in this module to read the
information you need from the given print
record reference.
*/
typedef pascal void (*UniversalPrint_SheetDoneProcPtr)	(UniversalPrint_ContextRef	inRef,
														 WindowRef					inParentWindow,
														 Boolean					inDialogAccepted);
inline void
UniversalPrint_InvokeSheetDoneProc	(UniversalPrint_SheetDoneProcPtr	inUserRoutine,
									 UniversalPrint_ContextRef			inRef,
									 WindowRef							inParentWindow,
									 Boolean							inDialogAccepted)
{
	(*inUserRoutine)(inRef, inParentWindow, inDialogAccepted);
}
#endif



/*###############################################################
	INITIALIZING AND FINISHING WITH PRINTING
###############################################################*/

// CALL THIS ROUTINE ONCE, BEFORE ANY OTHER PRINTING ROUTINE; TO GET THE PRINTING DRIVER’S
// RESOURCE FILE (MAC OS 8.x) INVOKE CurResFile() IMMEDIATELY AFTER A CALL TO THIS ROUTINE
void
	UniversalPrint_Init					();

// CALL THIS ROUTINE AFTER YOU ARE DONE WITH PRINTING
void
	UniversalPrint_Done					();

/*###############################################################
	DETERMINING SPECIFIC PRINTING LIBRARY INFORMATION
###############################################################*/

ApplicationSharedLibraryVersion
	UniversalPrint_Version					();

inline Boolean
	UniversalPrint_Available				()
	{
		return true;
	}

UniversalPrint_Architecture
	UniversalPrint_ReturnArchitecture		();

/*###############################################################
	CREATING AND DESTROYING OPAQUE PRINT RECORDS
###############################################################*/

UniversalPrint_ContextRef
	UniversalPrint_NewContext				();

void
	UniversalPrint_DisposeContext			(UniversalPrint_ContextRef*			inoutRefPtr);

/*###############################################################
	STATUS INFORMATION
###############################################################*/

OSStatus
	UniversalPrint_ReturnLastResult			(UniversalPrint_ContextRef			inRef);

/*###############################################################
	ACCESSING DATA IN OPAQUE PRINT RECORDS
###############################################################*/

void
	UniversalPrint_GetDeviceResolution		(UniversalPrint_ContextRef			inRef,
											 SInt16*							outVerticalResolution,
											 SInt16*							outHorizontalResolution);

void
	UniversalPrint_GetPageBounds			(UniversalPrint_ContextRef			inRef,
											 Rect*								outBounds);

void
	UniversalPrint_GetPaperBounds			(UniversalPrint_ContextRef			inRef,
											 Rect*								outBounds);

void
	UniversalPrint_SetIdleProc				(UniversalPrint_ContextRef			inRef,
											 UniversalPrint_IdleProcPtr			inProcPtr);

void
	UniversalPrint_SetNumberOfCopies		(UniversalPrint_ContextRef			inRef,
											 UInt16								inNumberOfCopies,
											 Boolean							inLock);

void
	UniversalPrint_SetPageRange				(UniversalPrint_ContextRef			inRef,
											 UInt16								inFirstPage,
											 UInt16								inLastPage);

/*###############################################################
	PRINTING MODE (DIALOG OR NO DIALOG) - FOR CONVENIENCE ONLY
###############################################################*/

UniversalPrint_Mode
	UniversalPrint_ReturnMode				();

void
	UniversalPrint_SetMode					(UniversalPrint_Mode				inNewMode);

/*###############################################################
	PRINTING DIALOGS
###############################################################*/

// CALL THIS ROUTINE INSTEAD OF PrJobDialog()
Boolean
	UniversalPrint_JobDialogDisplay			(UniversalPrint_ContextRef			inRef);

// CALL THIS ROUTINE INSTEAD OF PrStlDialog()
Boolean
	UniversalPrint_PageSetupDialogDisplay	(UniversalPrint_ContextRef			inRef);

#if TARGET_API_MAC_CARBON
// USES SHEETS ON MAC OS X
void
	UniversalPrint_JobSheetDisplay			(UniversalPrint_ContextRef			inRef,
											 WindowRef							inParentWindow,
											 UniversalPrint_SheetDoneProcPtr	inProcPtr);
#endif

#if TARGET_API_MAC_CARBON
// USES SHEETS ON MAC OS X
void
	UniversalPrint_PageSetupSheetDisplay	(UniversalPrint_ContextRef			inRef,
											 WindowRef							inParentWindow,
											 UniversalPrint_SheetDoneProcPtr	inProcPtr);
#endif

/*###############################################################
	SAVING AND RESTORING OPAQUE RECORDS
###############################################################*/

// INITIALIZE THE DATA STRUCTURE (SET THE ARCHITECTURE, FILL IN YOUR DATA FROM RESOURCES, ETC.) BEFORE USE
void
	UniversalPrint_CopyFromSaved			(UniversalPrint_ContextRef		inRef,
											 UniversalPrint_SavedContextPtr	inArchitectureSpecificDataPtr);

// INITIALIZE THE DATA STRUCTURE (SET THE ARCHITECTURE, CREATE ALL HANDLES) BEFORE CALLING THIS ROUTINE
void
	UniversalPrint_CopyToSaved				(UniversalPrint_ContextRef		inRef,
											 UniversalPrint_SavedContextPtr	inoutArchitectureSpecificDataPtr);

// CALL THIS ROUTINE INSTEAD OF PrintDefault()
void
	UniversalPrint_DefaultSaved				(UniversalPrint_ContextRef		inRef,
											 UniversalPrint_SavedContextPtr	inoutArchitectureSpecificDataPtr);

/*###############################################################
	SEAMLESS UTILIZATION OF ARCHITECTURE-SPECIFIC DATA
###############################################################*/

// THIS HAS NO EFFECT WITHOUT A CARBON VERSION OF THE LIBRARY; IT IS MOST BENEFICIAL FOR FUTURE USE
#if !TARGET_API_MAC_OS8
void
	UniversalPrint_MakeCarbonPrintData		(UniversalPrint_ContextRef		inRef,
											 PMPrintSettings*				outPrintSettingsPtr,
											 PMPageFormat*					outPageFormatPtr);
#endif

// IF A MAC OS 8 VERSION OF THIS LIBRARY IS BEING USED, THIS METHOD WORKS MUCH FASTER
void
	UniversalPrint_MakeTraditionalPrintData	(UniversalPrint_ContextRef		inRef,
											 Handle*						outTHPrintPtr);

// THIS HAS NO EFFECT WITHOUT A CARBON VERSION OF THE LIBRARY
#if !TARGET_API_MAC_OS8
void
	UniversalPrint_TakeCarbonPrintData		(UniversalPrint_ContextRef		inRef,
											 PMPrintSettings				inPrintSettingsToUse,
											 PMPageFormat					inPageFormatToUse);
#endif

// THIS ROUTINE WORKS UNDER ANY PRINTING ARCHITECTURE (USUALLY USED FOR LEGACY PRINT SETTINGS FROM FILES)
void
	UniversalPrint_TakeTraditionalPrintData	(UniversalPrint_ContextRef		inRef,
											 Handle							inTHPrintToUse);

/*###############################################################
	PRINTING DOCUMENTS
###############################################################*/

// CALL THIS ROUTINE INSTEAD OF PrOpenDoc()
void
	UniversalPrint_BeginDocument			(UniversalPrint_ContextRef		inRef);

// CALL THIS ROUTINE INSTEAD OF PrOpenPage()
void
	UniversalPrint_BeginPage				(UniversalPrint_ContextRef		inRef,
											 Rect const*					inPageBounds);	// can be nullptr

void
	UniversalPrint_Cancel					(UniversalPrint_ContextRef		inRef);

// CALL THIS ROUTINE INSTEAD OF PrCloseDoc()
void
	UniversalPrint_EndDocument				(UniversalPrint_ContextRef		inRef);

// CALL THIS ROUTINE INSTEAD OF PrClosePage()
void
	UniversalPrint_EndPage					(UniversalPrint_ContextRef		inRef);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
