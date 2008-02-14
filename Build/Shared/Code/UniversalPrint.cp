/*###############################################################

	UniversalPrint.cp
	
	Universal Printing Library 1.1
	© 1998-2008 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
    This program is distributed in the hope that it will be
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

// Mac includes
#if TARGET_API_MAC_OS8
#	include <Printing.h>
#else
#	include <ApplicationServices/ApplicationServices.h>
#endif
#include <Carbon/Carbon.h>

// library includes
#include <MemoryBlockHandleLocker.template.h>
#include <MemoryBlocks.h>
#include <UniversalPrint.h>



#pragma mark Constants

#define KEY_UNIVERSALPRINTCONTEXTREF	"UniversalPrint_ContextRef"	//!< used with PMSessionSetDataInSession

#pragma mark Types

struct My_UniversalPrintRecord
{
	OSStatus			lastResult;			// error code from last Print Manager / Carbon Print Manager call
#if TARGET_API_MAC_OS8
	// data required by the Mac OS 8 printing architecture
	THPrint				recordHandle;
	TPPrPort			printingPortPtr;
#else
	// data required by the Carbon Printing Manager
	PMPrintSession		session;
	PMPrintSettings		settings;
	PMPageFormat		pageFormat;
	PMPrintContext		documentContext;
	PMIdleUPP			idleUPP;
	PMSheetDoneUPP		sheetDoneUPP;		// possible memory optimization (future)
	PMDialog			pageSetupDialog,	// possible memory optimization (future)
						printDialog;		// possible memory optimization (future)
	UniversalPrint_SheetDoneProcPtr	sheetDoneProc;	// possible memory optimization (future)
#endif
};
typedef My_UniversalPrintRecord*		My_UniversalPrintRecordPtr;
typedef My_UniversalPrintRecordPtr*		My_UniversalPrintRecordHandle;

typedef MemoryBlockHandleLocker< UniversalPrint_ContextRef, My_UniversalPrintRecord >	My_UniversalPrintRecordHandleLocker;
typedef LockAcquireRelease< UniversalPrint_ContextRef, My_UniversalPrintRecord >		My_UniversalPrintRecordAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	UniversalPrint_Mode		gPrintingMode = kUniversalPrint_ModeNormal;
#if TARGET_API_MAC_CARBON
	PMSheetDoneUPP			gSheetDoneUPP = nullptr;
#endif
	My_UniversalPrintRecordHandleLocker&	gUniversalPrintRecordHandleLocks()		{ static My_UniversalPrintRecordHandleLocker x; return x; }
}

#pragma mark Internal Method Prototypes

#if !TARGET_API_MAC_OS8
static OSStatus						flattenAndCopy		(PMPrintSettings, PMPageFormat, Handle, Handle);
#endif

#if TARGET_API_MAC_CARBON
static pascal void					sheetDone			(PMPrintSession, WindowRef, Boolean);
#endif



#pragma mark Public Methods

/*!
This method returns the version of this
library.  The high 8 bits represent the
major version number.  The next 11 bits
represent the second decimal version
number.  The following 11 bits represent
the third decimal version number.  The
bottom 2 bits combine to form a number
that is equal to one of the kReleaseKind...
constants, indicating the kind of version
(final, development, alpha or beta).

(1.0)
*/
ApplicationSharedLibraryVersion
UniversalPrint_Version ()
{
	return Releases_Version(1, 0, 0, kReleaseKindAlpha);
}// Version


/*!
Call this routine to set up the printing
library.  You should call this in the same
place in your code where you would normally
call PrOpen().

(1.0)
*/
void
UniversalPrint_Init ()
{
#if TARGET_API_MAC_OS8
	PrOpen();
#else
	gSheetDoneUPP = NewPMSheetDoneUPP(sheetDone);
#endif
}// Init


/*!
Call this routine to shut down the printing
library.  You should call this in the same
place in your code where you would normally
call PrClose().

(1.0)
*/
void
UniversalPrint_Done ()
{
#if TARGET_API_MAC_OS8
	PrClose();
#else
	DisposePMSheetDoneUPP(gSheetDoneUPP), gSheetDoneUPP = nullptr;
#endif
}// Done


/*!
To allocate storage for an architecture-dependent
data structure and acquire an opaque reference to
it, use this method.  By using this routine to
manipulate printing data, you effectively write
the same code whether this library is built for
Mac OS 8 or Mac OS X!

(1.0)
*/
UniversalPrint_ContextRef
UniversalPrint_NewContext ()
{
	UniversalPrint_ContextRef	result = (UniversalPrint_ContextRef)NewHandle(sizeof(My_UniversalPrintRecord));
	
	
	if (result != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), result);
		
		
		ptr->lastResult = noErr;
	#if TARGET_API_MAC_OS8
		ptr->recordHandle = (THPrint)NewHandle(SIZE_OF_TPRINT);
		ptr->lastResult = MemError();
		if (ptr->lastResult == noErr)
		{
			PrintDefault(ptr->recordHandle);
			ptr->lastResult = PrError();
		}
		ptr->printingPortPtr = nullptr; // this is defined by PrOpenDoc()
	#else
		ptr->lastResult = PMCreateSession(&ptr->session);
		if (ptr->lastResult == noErr) ptr->lastResult = PMCreatePrintSettings(&ptr->settings);
		if (ptr->lastResult == noErr) ptr->lastResult = PMSessionDefaultPrintSettings(ptr->session, ptr->settings);
		if (ptr->lastResult == noErr) ptr->lastResult = PMCreatePageFormat(&ptr->pageFormat);
		if (ptr->lastResult == noErr) ptr->lastResult = PMSessionDefaultPageFormat(ptr->session, ptr->pageFormat);
		ptr->idleUPP = nullptr;
		ptr->sheetDoneProc = nullptr;
	#endif
	}
	return result;
}// NewContext


/*!
To destroy storage for an architecture-dependent
data structure to which you previously acquired
a reference using UniversalPrint_NewRecord(), use
this routine.  On output, your copy of the
reference is set to nullptr.

(1.0)
*/
void
UniversalPrint_DisposeContext	(UniversalPrint_ContextRef*		inoutRefPtr)
{
	if (inoutRefPtr != nullptr)
	{
		{
			My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), *inoutRefPtr);
			
			
		#if TARGET_API_MAC_OS8
			DisposeHandle((Handle)ptr->recordHandle), ptr->recordHandle = nullptr;
		#else
			(OSStatus)PMRelease(ptr->settings), ptr->settings = nullptr;
			(OSStatus)PMRelease(ptr->pageFormat), ptr->pageFormat = nullptr;
			if (ptr->idleUPP != nullptr) DisposePMIdleUPP(ptr->idleUPP), ptr->idleUPP = nullptr;
		#endif
		}
		DisposeHandle((Handle)*inoutRefPtr), *inoutRefPtr = nullptr;
	}
}// DisposeContext


/*!
To start a new printing document, use this method.
All context information, port information, etc. is
retained via the opaque reference, so you need only
call the corresponding UniversalPrint_EndDocument()
method with the same opaque reference to end a
document.  The UniversalPrint_BeginPage() and
UniversalPrint_EndPage() routines automatically ÒknowÓ
the document as well, so you can invoke those routines
between document opening and closing and get the
expected results.

(1.0)
*/
void
UniversalPrint_BeginDocument	(UniversalPrint_ContextRef		inRef)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		ptr->printingPortPtr = PrOpenDoc(ptr->recordHandle, nullptr/* port */, nullptr/* I/O buffer */);
		ptr->lastResult = PrError();
	#else
		ptr->lastResult = PMSessionBeginDocument(ptr->session, ptr->settings, ptr->pageFormat);
	#endif
	}
}// BeginDocument


/*!
To start a new printing page, use this method.  All
context information, port information, etc. is
retained via the opaque reference, so you need only
call the corresponding UniversalPrint_EndPage() method
with the same opaque reference to end a page.  The
UniversalPrint_BeginDocument() and
UniversalPrint_EndDocument() routines should bracket
calls to page routines (and the page automatically
refers to such a document).

You can specify nullptr for the page boundaries, in
which case default boundaries are used.

(1.0)
*/
void
UniversalPrint_BeginPage	(UniversalPrint_ContextRef		inRef,
							 Rect const*					inPageBoundsPtr)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		{
			Rect		rect = *inPageBoundsPtr;
			
			
			PrOpenPage(ptr->printingPortPtr, &rect);
			ptr->lastResult = PrError();
		}
	#else
		{
			PMRect		preciseBounds;
			PMRect*		boundsPtr = nullptr;
			
			
			if (inPageBoundsPtr != nullptr)
			{
				preciseBounds.top = inPageBoundsPtr->top;
				preciseBounds.left = inPageBoundsPtr->left;
				preciseBounds.bottom = inPageBoundsPtr->bottom;
				preciseBounds.right = inPageBoundsPtr->right;
				boundsPtr = &preciseBounds;
			}
			ptr->lastResult = PMSessionBeginPage(ptr->session, ptr->pageFormat, boundsPtr);
		}
	#endif
	}
}// BeginPage


/*!
Cancels printing in progress.  Under Carbon, this
is not possible, so calling this routine has no
effect.  On the other hand, you generally call this
routine from an idle procedure, and Carbon handles
manual print spooling, etc. anyway.  Since a print
job is likely to be spooled to disk and printed
under CarbonÕs control, and not your applicationÕs
explicit loop, you are never responsible for the
cancellation of a print job anyway.  The user can
always drag documents out of a desktop printer to
cancel them (and this behavior is essentially the
case for most drivers under Mac OS 8, as well).

(1.0)
*/
void
UniversalPrint_Cancel		(UniversalPrint_ContextRef		inRef)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
		ptr->lastResult = noErr;
	#if TARGET_API_MAC_OS8
		(*(ptr->recordHandle))->prJob.fFromUsr = true;
	#endif
	}
}// Cancel


/*!
Use this convenient routine for filling in the
information in an opaque printing record using
architecture-specific data you retrieved (most
likely) from a resource file.  You must specify
a printing architecture ahead of time (by
setting the appropriate field in the specified
structure), and you must create valid handles
that already contain your data.  In the case of
a Mac OS 8 architecture, the structure dictates
that a handle to a filled-in printing record
must be provided.  For the Mac OS X architecture
the structure requires two handles, respectively
to ÒflattenedÓ print settings and ÒflattenedÓ
page format data.  If the structure contains an
unknown architecture, "cfragArchitectureErr" is
returned by UniversalPrint_ReturnLastResult().

IMPORTANT:	A Pre-Carbon version of this library
			will fail to retrieve data stored in
			a Carbon Printing Manager format,
			whereas the reverse is not true.  If
			the data canÕt be read, the error
			"cfragArchitectureErr" is returned by
			UniversalPrint_ReturnLastResult().

(1.0)
*/
void
UniversalPrint_CopyFromSaved	(UniversalPrint_ContextRef			inRef,
								 UniversalPrint_SavedContextPtr		inArchitectureSpecificDataPtr)
{
	if ((inRef != nullptr) && (inArchitectureSpecificDataPtr != nullptr))
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
		switch (inArchitectureSpecificDataPtr->architecture)
		{
		case kUniversalPrint_ArchitectureTraditional:
			{
				Handle		printRecHandle = inArchitectureSpecificDataPtr->data.traditional.storageTHPrint;
				
				
				UniversalPrint_TakeTraditionalPrintData(inRef, printRecHandle);
			}
			break;
		
		case kUniversalPrint_ArchitectureMacOSX:
		#if TARGET_API_MAC_OS8
			// a non-Carbon version of the library canÕt translate a Mac OS X printing record
			ptr->lastResult = cfragArchitectureErr;
		#else
			{
				Handle			printSettingsHandle =
									inArchitectureSpecificDataPtr->data.osX.storageFlattenedPrintSettings,
								pageFormatHandle =
									inArchitectureSpecificDataPtr->data.osX.storageFlattenedPageFormat;
				PMPrintSettings	unflattenedPrintSettings;
				PMPageFormat	unflattenedPageFormat;
				
				
				// unflatten the given handles, and then fire them off to the sister routines for ÒinstallationÓ
				ptr->lastResult = PMUnflattenPrintSettings(printSettingsHandle, &unflattenedPrintSettings);
				if (ptr->lastResult == noErr)
				{
					ptr->lastResult = PMUnflattenPageFormat(pageFormatHandle, &unflattenedPageFormat);
					if (ptr->lastResult == noErr)
					{
						UniversalPrint_TakeCarbonPrintData(inRef, unflattenedPrintSettings, unflattenedPageFormat);
					}
				}
			}
		#endif
			break;
		
		default:
			// ???
			ptr->lastResult = cfragArchitectureErr;
			break;
		}
	}
}// CopyFromSaved


/*!
Use this convenient routine for filling in the
standard architecture-specific data types using
information in an opaque printing record.  This
routine is great for saving information to disk.
You must specify a printing architecture ahead
of time (by setting the appropriate field in the
specified structure), and you must create valid
handles that this method will resize as needed.
If the structure contains an unknown
architecture, "cfragArchitectureErr" is returned
by UniversalPrint_ReturnLastResult().

IMPORTANT:	A Pre-Carbon version of this library
			will fail to store data in a Carbon
			Printing Manager format, whereas the
			reverse is not true.  If the data
			canÕt be written, the error
			"cfragArchitectureErr" is returned
			by UniversalPrint_ReturnLastResult().

(1.0)
*/
void
UniversalPrint_CopyToSaved	(UniversalPrint_ContextRef			inRef,
							 UniversalPrint_SavedContextPtr		inoutArchitectureSpecificDataPtr)
{
	if ((inRef != nullptr) && (inoutArchitectureSpecificDataPtr != nullptr))
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
		switch (inoutArchitectureSpecificDataPtr->architecture)
		{
		case kUniversalPrint_ArchitectureTraditional:
			{
				Handle		givenHandle = inoutArchitectureSpecificDataPtr->data.traditional.storageTHPrint;
				Handle		tempHandle = nullptr;
				
				
				// COPY the data to the given handle, resizing it if necessary; this allows the use
				// of a resource handle, if desired, in the given structure
				UniversalPrint_MakeTraditionalPrintData(inRef, &tempHandle);
				if (UniversalPrint_ReturnLastResult(inRef) == noErr)
				{
					if (IsHandleValid(givenHandle))
					{
						ptr->lastResult = Memory_SetHandleSize(givenHandle, GetHandleSize(tempHandle));
						if (ptr->lastResult == noErr)
						{
							// note that a simple block-move doesnÕt require a locked handle
							BlockMoveData(*tempHandle, *givenHandle, SIZE_OF_TPRINT);
						}
					}
					else ptr->lastResult = nilHandleErr;
					DisposeHandle(tempHandle), tempHandle = nullptr;
				}
			}
			break;
		
		case kUniversalPrint_ArchitectureMacOSX:
		#if TARGET_API_MAC_OS8
			ptr->lastResult = cfragArchitectureErr;
		#else
			{
				Handle				printSettingsHandle =
										inoutArchitectureSpecificDataPtr->data.osX.storageFlattenedPrintSettings,
									pageFormatHandle =
										inoutArchitectureSpecificDataPtr->data.osX.storageFlattenedPageFormat;
				PMPrintSettings		unflattenedPrintSettings;
				PMPageFormat		unflattenedPageFormat;
				
				
				// grab the Carbon data types first
				UniversalPrint_MakeCarbonPrintData(inRef, &unflattenedPrintSettings, &unflattenedPageFormat);
				
				// flatten and COPY data from the opaque types to the handles; that way, the caller
				// can specify a resource handle that will remain a resource handle when this returns
				ptr->lastResult = flattenAndCopy(unflattenedPrintSettings, unflattenedPageFormat,
													printSettingsHandle, pageFormatHandle);
			}
		#endif
			break;
		
		default:
			// ???
			ptr->lastResult = cfragArchitectureErr;
			break;
		}
	}
}// CopyToSaved


/*!
To fill in a saved record with default values, as
if you called PrintDefault() or the combined
PMSessionDefaultPrintSettings() and
PMSessionDefaultPageFormat() routines on the
internal data, use this method.

IMPORTANT:	This routine may fail if the installed
			version of this library is pre-Carbon
			and the requested data is in Carbon
			format, or vice-versa.

(1.0)
*/
void
UniversalPrint_DefaultSaved		(UniversalPrint_ContextRef			inRef,
								 UniversalPrint_SavedContextPtr		inoutArchitectureSpecificDataPtr)
{
	if ((inRef != nullptr) && (inoutArchitectureSpecificDataPtr != nullptr))
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
		switch (inoutArchitectureSpecificDataPtr->architecture)
		{
		case kUniversalPrint_ArchitectureTraditional:
		#if TARGET_API_MAC_OS8
			{
				// (sigh) life was so much simpler with the traditional printing architecture...
				Handle		printRecHandle = inoutArchitectureSpecificDataPtr->data.traditional.storageTHPrint;
				
				
				PrSetError(noErr);
				PrintDefault((THPrint)printRecHandle);
				ptr->lastResult = PrError();
			}
		#else
			{
				// if the data is in Classic format (for example, read from disk), it could still
				// be defaulted on Mac OS X; just take the extra steps of converting everything
				PMPrintSettings		printSettings = nullptr;
				PMPageFormat		pageFormat = nullptr;
				
				
				ptr->lastResult = PMSessionConvertOldPrintRecord
									(ptr->session, inoutArchitectureSpecificDataPtr->data.traditional.storageTHPrint,
										&printSettings, &pageFormat);
				if (ptr->lastResult == noErr)
				{
					ptr->lastResult = PMSessionDefaultPrintSettings(ptr->session, printSettings);
					if (ptr->lastResult == noErr) ptr->lastResult = PMSessionDefaultPageFormat(ptr->session, pageFormat);
					if (ptr->lastResult == noErr)
					{
						ptr->lastResult = PMSessionMakeOldPrintRecord
											(ptr->session, printSettings, pageFormat,
												&inoutArchitectureSpecificDataPtr->data.traditional.storageTHPrint);
					}
				}
			}
		#endif
			break;
		
		case kUniversalPrint_ArchitectureMacOSX:
			// ...enter Carbon hell, where many steps are needed to get the job done
		#if TARGET_API_MAC_OS8
			ptr->lastResult = cfragArchitectureErr;
		#else
			{
				Handle			printSettingsHandle =
									inoutArchitectureSpecificDataPtr->data.osX.storageFlattenedPrintSettings,
								pageFormatHandle =
									inoutArchitectureSpecificDataPtr->data.osX.storageFlattenedPageFormat;
				PMPrintSettings	unflattenedPrintSettings;
				PMPageFormat	unflattenedPageFormat;
				
				
				// unflatten the given handles, default them, and then re-flatten them
				ptr->lastResult = PMUnflattenPrintSettings(printSettingsHandle, &unflattenedPrintSettings);
				if (ptr->lastResult == noErr)
				{
					ptr->lastResult = PMUnflattenPageFormat(pageFormatHandle, &unflattenedPageFormat);
					if (ptr->lastResult == noErr) ptr->lastResult = PMSessionDefaultPrintSettings
																	(ptr->session, unflattenedPrintSettings);
					if (ptr->lastResult == noErr) ptr->lastResult = PMSessionDefaultPageFormat
																	(ptr->session, unflattenedPageFormat);
					if (ptr->lastResult == noErr) ptr->lastResult = flattenAndCopy(unflattenedPrintSettings,
																					unflattenedPageFormat,
																					printSettingsHandle,
																					pageFormatHandle);
				}
			}
		#endif
			break;
		
		default:
			// ???
			ptr->lastResult = cfragArchitectureErr;
			break;
		}
	}
}// DefaultSaved


/*!
To close a printing document, use this method.
All context information, port information, etc. is
retained via the opaque reference, so if you called
UniversalPrint_BeginDocument(),
UniversalPrint_EndDocument() will automatically
know what document you are referring to.

(1.0)
*/
void
UniversalPrint_EndDocument	(UniversalPrint_ContextRef		inRef)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		PrCloseDoc(ptr->printingPortPtr);
		ptr->lastResult = PrError();
	#else
		ptr->lastResult = PMSessionEndDocument(ptr->session);
	#endif
	}
}// EndDocument


/*!
To close a printing page, use this method.  All
context information, port information, etc. is
retained via the opaque reference, so if you called
UniversalPrint_BeginPage(), UniversalPrint_EndPage()
will automatically know what page you are referring
to.

(1.0)
*/
void
UniversalPrint_EndPage	(UniversalPrint_ContextRef		inRef)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		PrClosePage(ptr->printingPortPtr);
		ptr->lastResult = PrError();
	#else
		ptr->lastResult = PMSessionEndPage(ptr->session);
	#endif
	}
}// EndPage


/*!
To acquire the resolution of a printed page, use
this method.  Technically, Carbon allows greater
precision on this measurement, but for compatibility
with Mac OS 8, this routine returns integer numbers.

(1.0)
*/
void
UniversalPrint_GetDeviceResolution	(UniversalPrint_ContextRef	inRef,
									 SInt16*					outVerticalResolution,
									 SInt16*					outHorizontalResolution)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		ptr->lastResult = noErr;
		*outVerticalResolution = (*(ptr->recordHandle))->prInfo.iVRes;
		*outHorizontalResolution = (*(ptr->recordHandle))->prInfo.iHRes;
	#else
		{
			PMResolution	resolution;
			
			
			ptr->lastResult = PMGetResolution(ptr->pageFormat, &resolution);
			*outVerticalResolution = STATIC_CAST(resolution.vRes, SInt16);
			*outHorizontalResolution = STATIC_CAST(resolution.hRes, SInt16);
		}
	#endif
	}
}// GetDeviceResolution


/*!
To acquire the boundaries of a printed page, use
this method.  Technically, Carbon allows greater
precision on this measurement, but for compatibility
with Mac OS 8, this routine fills in a basic
QuickDraw rectangle.

(1.0)
*/
void
UniversalPrint_GetPageBounds	(UniversalPrint_ContextRef		inRef,
								 Rect*							outBounds)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		ptr->lastResult = noErr;
		*outBounds = (*(ptr->recordHandle))->prInfo.rPage;
	#else
		{
			PMRect		preciseBounds;
			
			
			ptr->lastResult = PMGetPhysicalPageSize(ptr->pageFormat, &preciseBounds);
			SetRect(outBounds, STATIC_CAST(preciseBounds.left, SInt16), STATIC_CAST(preciseBounds.top, SInt16),
					STATIC_CAST(preciseBounds.right, SInt16), STATIC_CAST(preciseBounds.bottom, SInt16));
		}
	#endif
	}
}// GetPageBounds


/*!
To acquire the boundaries of the paper being used for
printing, use this method.  Technically, Carbon allows
greater precision on this measurement, but for
compatibility with Mac OS 8, this routine fills in a
basic QuickDraw rectangle.

(1.0)
*/
void
UniversalPrint_GetPaperBounds	(UniversalPrint_ContextRef		inRef,
								 Rect*							outBounds)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		ptr->lastResult = noErr;
		*outBounds = (*(ptr->recordHandle))->rPaper;
	#else
		{
			PMRect		preciseBounds;
			
			
			ptr->lastResult = PMGetPhysicalPaperSize(ptr->pageFormat, &preciseBounds);
			SetRect(outBounds, STATIC_CAST(preciseBounds.left, SInt16), STATIC_CAST(preciseBounds.top, SInt16),
					STATIC_CAST(preciseBounds.right, SInt16), STATIC_CAST(preciseBounds.bottom, SInt16));
		}
	#endif
	}
}// GetPaperBounds


/*!
Displays the standard Print dialog box.  Most
applications require this routine for their primary
Print command, but special print commands (such as
Print One Copy, etc.) may not use a print dialog.

IMPORTANT:	At least on Mac OS X, it is necessary
			to initialize the page range and number
			of copies so that the dialog can enforce
			this (otherwise, the defaults are 0,
			which are invalid and cause spurious
			alerts).  Use UniversalPrint_SetPageRange()
			and UniversalPrint_SetNumberOfCopies().

The dialog box may modify print settings; use
accessor routines in this module to read the
information you need.

If the user chooses to go ahead with printing,
"true" is returned; otherwise, the printing job
was cancelled, so "false" is returned.

(1.0)
*/
Boolean
UniversalPrint_JobDialogDisplay		(UniversalPrint_ContextRef		inRef)
{
	Boolean		result = false;
	
	
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
		// set initial values for the print dialog box
		UniversalPrint_SetNumberOfCopies(inRef, 1, false/* lock */); // Mac OS X requires an initial value
		UniversalPrint_SetPageRange(inRef, 1/* first */, 1/* last */); // Mac OS X requires an initial value
	#if TARGET_API_MAC_OS8
		result = PrJobDialog(ptr->recordHandle);
		ptr->lastResult = PrError();
	#else
		ptr->lastResult = PMSessionPrintDialog(ptr->session, ptr->settings, ptr->pageFormat, &result);
	#endif
	}
	
	return result;
}// JobDialogDisplay


/*!
To generate Carbon printing data types reflecting the
settings of an opaque print record, use this method.
If this version of the library is pre-Carbon, this
routine will have no effect.

This routine will most likely be widely used in the
future, when Mac OS 8 compatibility is not an issue
and you will be interested in using Carbon data types
exclusively.

(1.0)
*/
#if !TARGET_API_MAC_OS8
void
UniversalPrint_MakeCarbonPrintData	(UniversalPrint_ContextRef	inRef,
									 PMPrintSettings*			outPrintSettingsPtr,
									 PMPageFormat*				outPageFormatPtr)
{
	if ((inRef != nullptr) && (outPrintSettingsPtr != nullptr) && (outPageFormatPtr != nullptr))
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
		ptr->lastResult = PMCopyPrintSettings(ptr->settings, *outPrintSettingsPtr);
		if (ptr->lastResult == noErr) ptr->lastResult = PMCopyPageFormat(ptr->pageFormat, *outPageFormatPtr);
	}
}// MakeCarbonPrintData
#endif


/*!
To create a new handle that is really a traditional
"THPrint" containing settings from an opaque print
record, use this method.

IMPORTANT:	You should generally avoid creating a data
			structure from an obsolete version of the
			Printing Manager; this method is for
			convenience only.

WARNING:	This method allocates memory, and may fail.
			Check UniversalPrint_ReturnLastResult() to
			be sure it succeeds.

(1.0)
*/
void
UniversalPrint_MakeTraditionalPrintData		(UniversalPrint_ContextRef	inRef,
											 Handle*					outTHPrintPtr)
{
	if ((inRef != nullptr) && (outTHPrintPtr != nullptr))
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		*outTHPrintPtr = NewHandle(SIZE_OF_TPRINT);
		ptr->lastResult = MemError();
		if (*outTHPrintPtr == nullptr) ptr->lastResult = memFullErr;
		if (ptr->lastResult == noErr)
		{
			// note that a simple block-move doesnÕt require a locked handle
			BlockMoveData(*(ptr->recordHandle), *((THPrint)*outTHPrintPtr), SIZE_OF_TPRINT);
		}
	#else
		ptr->lastResult = PMSessionMakeOldPrintRecord(ptr->session, ptr->settings, ptr->pageFormat, outTHPrintPtr);
	#endif
	}
}// MakeTraditionalPrintData


/*!
To display the standard Page Setup dialog box,
use this method.  Most applications provide a
Page Setup command, and should therefore use
this routine to implement that command.

The dialog box may modify print settings; use
accessor routines in this module to read the
information you need.

If the user saves changes, "true" is returned;
otherwise, "false" is returned.

(1.0)
*/
Boolean
UniversalPrint_PageSetupDialogDisplay		(UniversalPrint_ContextRef		inRef)
{
	Boolean		result = false;
	
	
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		result = PrStlDialog(ptr->recordHandle);
		ptr->lastResult = PrError();
	#else
		ptr->lastResult = PMSessionPageSetupDialog(ptr->session, ptr->pageFormat, &result);
	#endif
	}
	
	return result;
}// PageSetupDialogDisplay


#if TARGET_API_MAC_CARBON
/*!
Displays the standard Page Setup sheet over
the specified parent window.  Document-centric
applications providing a Page Setup command
should use this routine on Mac OS X to
implement that command.

If the user saves changes, "true" is returned;
otherwise, "false" is returned.

Incomplete.

(1.0)
*/
void
UniversalPrint_PageSetupSheetDisplay	(UniversalPrint_ContextRef			inRef,
										 WindowRef							inParentWindow,
										 UniversalPrint_SheetDoneProcPtr	inProcPtr)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
		ptr->lastResult = PMSessionUseSheets(ptr->session, inParentWindow, gSheetDoneUPP);
		if (ptr->lastResult == noErr)
		{
			ptr->sheetDoneProc = inProcPtr;
			ptr->lastResult = PMSessionPageSetupDialogInit(ptr->session, ptr->pageFormat, &ptr->pageSetupDialog);
		}
	}
}// PageSetupSheetDisplay
#endif


/*!
Returns a constant describing the printing
architecture that this library supports.
The ÒtraditionalÓ architecture refers to
the Print Manager in place for Mac OS 8.6
and earlier.  The ÒMac OS XÓ architecture
refers to the Carbon printing architecture
that is in place in Mac OS 9.

Since this library handles printing totally
transparently, you only need to check this
architecture setting to ensure that the
installed printing library will run on the
userÕs system.  A Mac OS 8 version will not
run under Mac OS X, and vice-versa; however,
either version can be used under Mac OS 9.

(1.0)
*/
UniversalPrint_Architecture
UniversalPrint_ReturnArchitecture ()
{
	UniversalPrint_Architecture		result =
				#if TARGET_API_MAC_OS8
					kUniversalPrint_ArchitectureTraditional;
				#else
					kUniversalPrint_ArchitectureMacOSX;
				#endif
	
	
	return result;
}// ReturnArchitecture


/*!
Returns the error code generated from the last
printing function invoked.  The meaning and
range of values will depend on the architecture
(Mac OS 8 or Mac OS X) of this libraryÕs
implementation.  In the rare event that this
library needs to make multiple printing-related
calls to perform a function, the ÒlastÓ result
will actually refer to the error returned from
the first call that fails.

(1.0)
*/
OSStatus
UniversalPrint_ReturnLastResult		(UniversalPrint_ContextRef		inRef)
{
	OSStatus		result = noErr;
	
	
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
		result = ptr->lastResult;
	}
	else result = memPCErr;
	
	return result;
}// ReturnLastResult


/*!
Returns a constant describing the current
printing mode.  You must explicitly set the
printing mode (using UniversalPrint_SetMode())
to be sure the printing functions will behave
as expected.

(1.0)
*/
UniversalPrint_Mode
UniversalPrint_ReturnMode ()
{
	return gPrintingMode;
}// ReturnMode


/*!
To install a print idle procedure, use this method.
Under Mac OS 8, the most common reason for doing
this is to poll for command-period (cancellation)
events.

(1.0)
*/
void
UniversalPrint_SetIdleProc	(UniversalPrint_ContextRef		inRef,
							 UniversalPrint_IdleProcPtr		inProcPtr)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		ptr->lastResult = noErr;
		(*(ptr->recordHandle))->prJob.pIdleProc = NewPrIdleProc((PrIdleProcPtr)inProcPtr);
	#else
		if (ptr->idleUPP != nullptr) DisposePMIdleUPP(ptr->idleUPP);
		ptr->idleUPP = NewPMIdleUPP(inProcPtr);
		ptr->lastResult = PMSessionSetIdleProc(ptr->session, ptr->idleUPP);
	#endif
	}
}// SetIdleProc


/*!
To set the number of copies to print, use this
method.  Under Mac OS 8, this is usually done
to implement a Òprint nowÓ command, bypassing
the print job dialog.  The "inLock" setting is
ignored under Mac OS 8.

(1.0)
*/
void
UniversalPrint_SetNumberOfCopies	(UniversalPrint_ContextRef	inRef,
									 UInt16						inNumberOfCopies,
									 Boolean					UNUSED_ARGUMENT_CLASSIC(inLock))
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		ptr->lastResult = noErr;
		(*(ptr->recordHandle))->prJob.iCopies = inNumberOfCopies;
	#else
		ptr->lastResult = PMSetCopies(ptr->settings, inNumberOfCopies, inLock);
	#endif
	}
}// SetNumberOfCopies


/*!
To set the number of copies to print, use this
method.  Under Mac OS 8, this is usually done
to implement a Òprint nowÓ command, bypassing
the print job dialog.  The "inLock" setting is
ignored under Mac OS 8.

(1.0)
*/
void
UniversalPrint_SetPageRange	(UniversalPrint_ContextRef	inRef,
							 UInt16						inFirstPage,
							 UInt16						inLastPage)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		ptr->lastResult = noErr;
		(*(ptr->recordHandle))->prJob.iFstPage = inFirstPage;
		(*(ptr->recordHandle))->prJob.iLstPage = inLastPage;
	#else
		ptr->lastResult = PMSetPageRange(ptr->settings, inFirstPage, inLastPage);
	#endif
	}
}// SetPageRange


/*!
Specifies the printing mode.  The printing
mode affects how the printing functions
behave.  Each printing mode generally
corresponds to a printing option in the File
menu (so there are very few of them).  You
must explicitly set the printing mode to be
sure the printing functions will behave as
expected.

(1.0)
*/
void
UniversalPrint_SetMode	(UniversalPrint_Mode	inNewMode)
{
	gPrintingMode = inNewMode;
}// SetMode


/*!
To use data from Carbon printing data types to modify
the settings of an opaque print record, use this
method.  If this version of the library is pre-Carbon,
this routine will have no effect.

(1.0)
*/
#if !TARGET_API_MAC_OS8
void
UniversalPrint_TakeCarbonPrintData	(UniversalPrint_ContextRef	inRef,
									 PMPrintSettings			inPrintSettingsToUse,
									 PMPageFormat				inPageFormatToUse)
{
	if (inRef != nullptr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
		ptr->lastResult = PMCopyPrintSettings(inPrintSettingsToUse, ptr->settings);
		if (ptr->lastResult == noErr) ptr->lastResult = PMCopyPageFormat(inPageFormatToUse, ptr->pageFormat);
	}
}// TakeCarbonPrintData
#endif


/*!
To use data from a traditional "THPrint" to modify
the settings of an opaque print record, use this
method.  If this version of the library is pre-Carbon,
this routine will simply copy the handle; otherwise,
it will translate the handle to Carbon data types.
In any case, subsequent calls to accessor functions
using the given opaque printing record reference will
return data that takes into account modifications
from the traditional printing record you provide to
this function.

IMPORTANT:	You should generally avoid using a data
			structure from an obsolete version of the
			Printing Manager; this method is for
			convenience only.  It is generally used
			for updating an opaque record to use data
			stored in a disk file, which might have
			been created by an older version of the
			Mac OS Printing Manager.

(1.0)
*/
void
UniversalPrint_TakeTraditionalPrintData		(UniversalPrint_ContextRef	inRef,
											 Handle						inTHPrintToUse)
{
	if ((inRef != nullptr) && (IsHandleValid(inTHPrintToUse)))
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), inRef);
		
		
	#if TARGET_API_MAC_OS8
		BlockMoveData(*((THPrint)inTHPrintToUse), *(ptr->recordHandle), SIZE_OF_TPRINT);
		
		// lock the handle in case PrValidate() does something undocumented and stupid with it
		HLock((Handle)ptr->recordHandle);
		(Boolean)PrValidate(ptr->recordHandle);
		ptr->lastResult = PrError();
		HUnlock((Handle)ptr->recordHandle);
	#else
		ptr->lastResult = PMSessionConvertOldPrintRecord(ptr->session, inTHPrintToUse, &ptr->settings,
															&ptr->pageFormat);
	#endif
	}
}// TakeTraditionalPrintData


#pragma mark Internal Methods

#if !TARGET_API_MAC_OS8
/*!
To flatten opaque Carbon printing types and
COPY the flattened versions to existing
handles, use this method.

(1.0)
*/
static OSStatus
flattenAndCopy		(PMPrintSettings			inPrintSettingsToFlatten,
					 PMPageFormat				inPageFormatToFlatten,
					 Handle						inoutPrintSettingsFlattenedCopy,
					 Handle						inoutPageFormatFlattenedCopy)
{
	OSStatus	result = noErr;
	Handle		tempHandle = nullptr;
	
	
	// flatten the print settings information and COPY it to the appropriate handle;
	// this allows the use of things like resource handles, etc.
	if (result == noErr)
	{
		result = PMFlattenPrintSettings(inPrintSettingsToFlatten, &tempHandle);
		if (result == noErr)
		{
			if (IsHandleValid(inoutPrintSettingsFlattenedCopy))
			{
				result = Memory_SetHandleSize(inoutPrintSettingsFlattenedCopy, GetHandleSize(tempHandle));
				if (result == noErr)
				{
					// note that a simple block-move doesnÕt require a locked handle
					BlockMoveData(*tempHandle, *inoutPrintSettingsFlattenedCopy, GetHandleSize(tempHandle));
				}
			}
			else result = nilHandleErr;
			DisposeHandle(tempHandle), tempHandle = nullptr;
		}
	}
	
	// flatten the page format information and COPY it to the appropriate handle; this
	// allows the use of things like resource handles, etc.
	if (result == noErr)
	{
		result = PMFlattenPageFormat(inPageFormatToFlatten, &tempHandle);
		if (result == noErr)
		{
			if (IsHandleValid(inoutPageFormatFlattenedCopy))
			{
				result = Memory_SetHandleSize(inoutPageFormatFlattenedCopy, GetHandleSize(tempHandle));
				if (result == noErr)
				{
					// note that a simple block-move doesnÕt require a locked handle
					BlockMoveData(*tempHandle, *inoutPageFormatFlattenedCopy, GetHandleSize(tempHandle));
				}
			}
			else result = nilHandleErr;
			DisposeHandle(tempHandle), tempHandle = nullptr;
		}
	}
	
	return result;
}// flattenAndCopy
#endif


#if TARGET_API_MAC_CARBON
/*!
Called by Mac OS X when a printing-related sheet
attached to the specified window is closed by the
user (either saving settings or throwing them away).

(1.0)
*/
static pascal void
sheetDone	(PMPrintSession		inPrintSession,
			 WindowRef			inParentWindow,
			 Boolean			inDialogAccepted)
{
	UniversalPrint_ContextRef	ref = nullptr;
	
	
	// the UniversalPrint_ContextRef is attached as keyed data in the print session
	if (PMSessionGetDataFromSession(inPrintSession, CFSTR(KEY_UNIVERSALPRINTCONTEXTREF), (CFTypeRef*)&ref) == noErr)
	{
		My_UniversalPrintRecordAutoLocker	ptr(gUniversalPrintRecordHandleLocks(), ref);
		
		
		UniversalPrint_InvokeSheetDoneProc(ptr->sheetDoneProc, ref, inParentWindow, inDialogAccepted);
	}
}// sheetDone
#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
