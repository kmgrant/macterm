/*###############################################################

	TelnetPrinting.cp
	
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
#include <cstdio>
#include <cstring>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <Console.h>
#include <Cursors.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>
#include <StringUtilities.h>
#include <UniversalPrint.h>

// resource includes
#include "DialogResources.h"
#include "StringResources.h"

// MacTelnet includes
#include "ConnectionData.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "Folder.h"
#include "ProgressDialog.h"
#include "TelnetPrinting.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "UIStrings.h"
#include "VectorCanvas.h"
#include "VectorInterpreter.h"
#include "VectorToBitmap.h"



#pragma mark Constants

enum
{
	kTHPrintResourceType = 'THPr',
	kTHPrintResourceID = 128
};
enum
{
	ascLF = 10,
	ascFF = 12,
	ascCR = 13
};

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	UniversalPrint_ContextRef	gPrintRecordInUse = nullptr;
}

#pragma mark Internal Method Prototypes

static pascal void		printIdle		();
static void				printText		(TerminalScreenRef, TerminalViewRef, ConstStringPtr);



#pragma mark Public Methods

/*!
Begins printer redirection, as requested by a
directive from a remote terminal application.

This routine has been rewritten in MacTelnet 3.0
to solve many issues.  One, the new Folder Manager
is now used to find the Temporary Folder, which
eliminates a couple of useless global variables.
Two, the FSpxxxx() routines from the modern Mac
File Manager are now used, instead of outdated
routines from earlier OS versions.  Three, the
function is now single-entry, single-exit.  And
lastly, it is now internal to this module, which
makes the code faster.

(3.0)
*/
void
TelnetPrinting_Begin	(TerminalPrintingInfoPtr	inPrintingInfoPtr)
{
	static UInt16	gPrintingTempFileNumber = 0;
	OSStatus		error = noErr;
	FSSpec			temporaryItemsFolder,
					temporaryFile;
	Boolean			fileCreated = false;
	
	
	inPrintingInfoPtr->enabled = true;
	inPrintingInfoPtr->bufferTail = 0x00000000;
	CPP_STD::snprintf(inPrintingInfoPtr->temporaryFileName, sizeof(inPrintingInfoPtr->temporaryFileName),
						"MacTelnet tempfile #%d", ++gPrintingTempFileNumber);
	StringUtilities_CToPInPlace(inPrintingInfoPtr->temporaryFileName);
	
	error = Folder_GetFSSpec(kFolder_RefMacTemporaryItems, &temporaryItemsFolder);
	if (error == noErr) error = FSMakeFSSpec(temporaryItemsFolder.vRefNum, temporaryItemsFolder.parID,
												(StringPtr)inPrintingInfoPtr->temporaryFileName, &temporaryFile);
	
	if ((error == noErr) || (error == fnfErr))
	{
		if ((error = FSpCreate(&temporaryFile, '----', 'TEXT', GetScriptManagerVariable(smKeyScript))) != noErr)
		{
			if (error == dupFNErr) error = noErr, fileCreated = true;
		}
		
		if (error == noErr)
		{
			if ((error = FSpOpenDF(&temporaryFile, fsRdWrPerm, &(inPrintingInfoPtr->temporaryFileRefNum))) == noErr)
			{
				if (SetEOF(inPrintingInfoPtr->temporaryFileRefNum, 0L)) error = fileBoundsErr;
			}
		}
	}
	
	if (error != noErr)
	{
		// if any problems occur, cancel the printer redirection and delete the temporary file
		Sound_StandardAlert();
		Alert_ReportOSStatus(error);
		inPrintingInfoPtr->enabled = false;
		if (fileCreated) FSpDelete(&temporaryFile);
	}
}// Begin


/*!
Ends printer redirection, as requested by a
directive from a remote terminal application.
The contents of the temporary file (which was
capturing “data to print”) will be read and sent
to the printer.

This routine has been rewritten in MacTelnet 3.0
to solve many issues.  One, the new Folder Manager
is now used to find the Temporary Folder, which
eliminates a couple of useless global variables.
Two, the FSpxxxx() routines from the modern Mac
File Manager are now used, instead of outdated
routines from earlier OS versions.  Three, the
function is now single-entry, single-exit.  And
lastly, it is now internal to this module, which
makes the code faster.

(3.0)
*/
void
TelnetPrinting_End		(TerminalPrintingInfoPtr	inPrintingInfoPtr)
{
	Boolean		failed = false;
	
	
	if (inPrintingInfoPtr->enabled)
	{
		FSSpec		temporaryItemsFolder,
					temporaryFile;
		OSStatus	error = noErr;
		
		
		error = Folder_GetFSSpec(kFolder_RefMacTemporaryItems, &temporaryItemsFolder);
		if (error == noErr) error = FSMakeFSSpec(temporaryItemsFolder.vRefNum, temporaryItemsFolder.parID,
													(StringPtr)inPrintingInfoPtr->temporaryFileName, &temporaryFile);
		
		inPrintingInfoPtr->enabled = false;
		
		// PRINTING BEGIN (ideally, externalize this in TelnetPrinting.cp)
		{
			GrafPtr						oldPort = nullptr;
			UniversalPrint_ContextRef	printRecordRef = nullptr;
			
			
			GetPort(&oldPort);
			
			UniversalPrint_Init();
			printRecordRef = UniversalPrint_NewContext();
			
			Cursors_UseArrow();
			
			if (UniversalPrint_JobDialogDisplay(printRecordRef)) // if false, printing was cancelled
			{
				if ((error = UniversalPrint_ReturnLastResult(printRecordRef)) != noErr) {/* error */}
				
				UniversalPrint_BeginDocument(printRecordRef);
				if ((error = UniversalPrint_ReturnLastResult(printRecordRef)) != noErr) failed = true;
				else
				{
					printPages(printRecordRef, (StringPtr)nullptr/* don’t print title */,
								inPrintingInfoPtr->wrapColumnCount, nullptr, inPrintingInfoPtr->temporaryFileRefNum);
					UniversalPrint_EndDocument(printRecordRef);
					if ((error = UniversalPrint_ReturnLastResult(printRecordRef)) != noErr) {/* error */}
				}
			}
			
			UniversalPrint_DisposeContext(&printRecordRef);
			UniversalPrint_Done();
			
			SetPort(oldPort);
		}
		// PRINTING END (externalize in TelnetPrinting.c)
		
		if ((error = FSClose(inPrintingInfoPtr->temporaryFileRefNum)) != noErr) failed = true;
		inPrintingInfoPtr->temporaryFileRefNum = -1;
		
		if ((error = FSpDelete(&temporaryFile)) != noErr) failed = true;
	}
	
	if (failed) Sound_StandardAlert();
}// End


/*!
To display the standard “Page Setup” dialog box
and store any changes in a resource file, use
this method.  The dialog’s settings are initially
based on the contents of MacTelnet’s printing
resource from the specified source file.  If no
resource can be found, then the user’s changes
(if any) are stored in the given resource file as
a new resource; otherwise, the original resource
is merely updated.

Could portions of this go in "Preferences.c"?

(3.0)
*/
void
TelnetPrinting_PageSetup		(SInt16		inResourceFileRefNum)
{
	SInt16		oldResourceFile = CurResFile();
	OSStatus	error = noErr;
	
	
	DeactivateFrontmostWindow();
	Cursors_UseArrow();
	
	{
		UniversalPrint_SavedContextPtr	printDataPtr = (UniversalPrint_SavedContextPtr)Memory_NewPtr
																						(sizeof(UniversalPrint_SavedContext));
		
		
		if (printDataPtr != nullptr)
		{
			UniversalPrint_ContextRef	printRecordRef = nullptr;
			SInt16						printingResourceFile = 0;
			Boolean						resourceExists = false;
			
			
			// MacTelnet currently saves and restores data in the traditional (THPrint) format
			printDataPtr->architecture = kUniversalPrint_ArchitectureTraditional;
			
			UniversalPrint_Init();
			printingResourceFile = CurResFile();
			printRecordRef = UniversalPrint_NewContext();
			
			UseResFile(inResourceFileRefNum);
			resourceExists = (Count1Resources(kTHPrintResourceType) > 0);
			
			if (resourceExists)
			{
				// retrieve the resource and update the opaque data to match it
				Console_WriteLine("print data exists on disk");
				printDataPtr->data.traditional.storageTHPrint = Get1Resource(kTHPrintResourceType, kTHPrintResourceID);
				error = ResError();
			}
			else
			{
				// create a new default print record and update the opaque data to match it
				Console_WriteLine("print data does not exist on disk - creating new print data");
				printDataPtr->data.traditional.storageTHPrint = Memory_NewHandle(SIZE_OF_TPRINT);
				error = MemError();
				if (error == noErr)
				{
					UseResFile(printingResourceFile);
					UniversalPrint_DefaultSaved(printRecordRef, printDataPtr); // defaults the print record created above
					error = UniversalPrint_ReturnLastResult(printRecordRef);
				}
			}
			
			// fill in an opaque printing record using whatever data is on disk (this can
			// even create a Carbonized printing record from a legacy version, thanks to
			// the power of UniversalPrintingLib!)
			if (error == noErr)
			{
				Console_WriteLine("reading data (either new or existing) into memory");
				UseResFile(printingResourceFile);
				UniversalPrint_CopyFromSaved(printRecordRef, printDataPtr);
				error = UniversalPrint_ReturnLastResult(printRecordRef);
			}
			
			if (error == noErr)
			{
				// display a Page Setup dialog that contains the settings (new or saved)
				UseResFile(printingResourceFile);
				Console_WriteLine("Page Setup dialog display...");
				if (UniversalPrint_PageSetupDialogDisplay(printRecordRef))
				{
					// if the user made changes, copy them back
					Console_WriteLine("user made changes: writing data to disk");
					UniversalPrint_CopyToSaved(printRecordRef, printDataPtr);
					error = UniversalPrint_ReturnLastResult(printRecordRef);
					if (error == noErr)
					{
						UseResFile(inResourceFileRefNum);
						if (resourceExists)
						{
							Console_WriteLine("resource exists; updating...");
							ChangedResource(printDataPtr->data.traditional.storageTHPrint);
							error = ResError();
							if (error == noErr) UpdateResFile(inResourceFileRefNum);
							DetachResource(printDataPtr->data.traditional.storageTHPrint);
						}
						else
						{
							Console_WriteLine("resource does not exist; creating...");
							AddResource(printDataPtr->data.traditional.storageTHPrint,
										kTHPrintResourceType, kTHPrintResourceID, EMPTY_PSTRING);
							error = ResError();
							if (error == noErr)
							{
								UpdateResFile(inResourceFileRefNum);
								
								// since AddResource() converts the handle to a resource, detach it (so it becomes a handle again)
								DetachResource(printDataPtr->data.traditional.storageTHPrint);
							}
						}
					}
				}
				else error = userCanceledErr;
				
				if (printDataPtr->data.traditional.storageTHPrint != nullptr)
				{
					Memory_DisposeHandle(&printDataPtr->data.traditional.storageTHPrint);
				}
			}
			
			Memory_DisposePtr((Ptr*)&printDataPtr);
			UniversalPrint_DisposeContext(&printRecordRef);
			UniversalPrint_Done();
		}
		else error = memFullErr;
	}
	
	RestoreFrontmostWindow();
	
	Console_WriteValue("result", error);
	
	Alert_ReportOSStatus(error); // display a generic alert to the user if anything went wrong
	
	UseResFile(oldResourceFile);
}// PageSetup


/*!
Prints the selection in the frontmost window.

(2.6)
*/
void
TelnetPrinting_PrintSelection ()
{
	WindowRef	frontWindow = EventLoop_ReturnRealFrontWindow();
	
	
	UniversalPrint_Init();
	
	if (TerminalWindow_ExistsFor(frontWindow))
	{
		Str255				title;
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(frontWindow);
		TerminalScreenRef	screen = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
		TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
		
		
		GetWTitle(frontWindow, title);
		printText(screen, view, title);
	}
	
	UniversalPrint_Done();
}// PrintSelection


/*	printPages(ref,title,columns,charh,refNum) -
 *			Prints <charh> on the printer through opaque <ref> with <title>
 *			using <columns>.
 */
void
printPages		(UniversalPrint_ContextRef	inPrintingRef,
				 ConstStringPtr				Title,
				 SInt16						columns,
				 char**						charh,
				 SInt16						refNum)
{
	char*				charp = nullptr;
	char*				start = nullptr;
	long				charlen = 0L,
						count = 0;
	SInt16				v = 0,
						h = 0,
						scount = 0,
						lines = 0,
						pgcount = 0,
						pgtmplen = 0,
						maxlines = 0;
	char				pagetemp[20];
	FMetricRec			info;
	SInt16				pFheight = 0,
						pFwidth = 0;
	UInt8				buf[256];			// to build a line in from the file
	UInt8				nextchar = '\0';	// next unprocessed char in file
	SInt16				rdy = 0;
	SInt16				indent = 0;			// indent to give reasonable left margin
	OSStatus			sts = noErr;
	long				dummyCount = 0L;
	//char				tmp[100];			// only for debugging
	char				stupidarray[160];	// used for finding string widths
	ProgressDialog_Ref	idleDialog = nullptr;
	Rect				pageBounds;
	
	
	// 3.0 - display a cool printing progress dialog
	{
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		CFStringRef			dialogCFString = nullptr;
		
		
		stringResult = UIStrings_Copy(kUIStrings_ProgressWindowPrintingPrimaryText, dialogCFString);
		if (stringResult.ok())
		{
			idleDialog = ProgressDialog_New(dialogCFString, true/* modal */);
			CFRelease(dialogCFString), dialogCFString = nullptr;
		}
	}
	ProgressDialog_Display(idleDialog);
	ProgressDialog_SetProgressIndicator(idleDialog, kProgressDialog_IndicatorIndeterminate);
	
	// Printer drivers automatically “guess” the document name by looking
	// at the title of the frontmost window.  Therefore, setting the title
	// of the progress dialog to be the given title will allow drivers to
	// correctly display the title of the printed data.  Since the idle
	// dialog has no title bar, this title will not appear on the screen.
	SetWTitle(ProgressDialog_ReturnWindow(idleDialog), Title);
	
	for (v=0; v<150; v++) stupidarray[v]='W';	/* Set up the width array */
	
	{
		SInt16		iHRes = 0,
					iVRes = 0;
		
		
		UniversalPrint_GetDeviceResolution(inPrintingRef, &iVRes, &iHRes);
		indent = (iHRes * 180) / 254;	/* 1.8 centimeters left margin */
	}
	{
		Rect		paperRect;
		
		
		UniversalPrint_GetPaperBounds(inPrintingRef, &paperRect);
		if (-paperRect.left > indent) indent = 0;
		else indent += paperRect.left;
	}
	
	TextFontByName("\pMonaco");
	TextFace(normal);
	v = 12;
	
	UniversalPrint_GetPageBounds(inPrintingRef, &pageBounds);
	do {
		TextSize(v);
		FontMetrics( &info );
		pFheight = FixRound( info.ascent + info.descent + info.leading);
		pFwidth  = FixRound( info.widMax);
		//pFwidth  = CharWidth('W');			/* Re-assign to largest char */
		v--;
	} while ( TextWidth(stupidarray,0,columns) > (pageBounds.right - pageBounds.left - indent));
	//snprintf (tmp,sizeof(tmp),"Fheight:%d, Fwidth:%d, TextSize:%d",pFheight,pFwidth,v+1); //putln (tmp);

	if (charh!=nullptr) {
		HLock(charh);
		start=charp=*charh;
		charlen= GetHandleSize(charh);
	} else {
		if ((sts=GetFPos(refNum, &charlen)) != noErr)
			{
			//snprintf(tmp,sizeof(tmp),"GetFPos: ERROR %d",(SInt16)sts);
			//putln(tmp);
			}
		charlen-=3;					/* skip last 3 chars, as they are part of ESC seq */
		if ((sts=SetFPos(refNum,(SInt16) fsFromStart,0)) != noErr)	/* rewind to beginning of file */
			{ /*snprintf(tmp,sizeof(tmp),"SetFPos: ERROR %d",sts); putln(tmp);*/ }
		start = (char *)buf;				/* always (the array doesn't move) */
		dummyCount=1;
		if ((sts=FSRead(refNum,&dummyCount,&nextchar)) != noErr)		/* get first char */
			{ /*snprintf(tmp,sizeof(tmp),"FSRead: ERROR %d",sts); putln(tmp);*/ }
	}
	
	h=pageBounds.right - pageBounds.left - indent;	/* Get the width */
	v=pageBounds.bottom- pageBounds.top;			/* Get the height */

	maxlines = v/pFheight-1;
	pgcount = 0;
	while (count<charlen) {
		pgcount++;
		lines = 1;
		UniversalPrint_BeginPage(inPrintingRef, (Rect*)nullptr);
		if ((sts=UniversalPrint_ReturnLastResult(inPrintingRef)) == noErr)
		{
			UniversalPrint_SetIdleProc(inPrintingRef, printIdle);
			//snprintf (tmp,sizeof(tmp),"printing page:%d",pgcount); //putln(tmp);
	
			if (Title != nullptr) {
				MoveTo( indent, lines*pFheight);
				DrawString( Title);
				CPP_STD::snprintf(pagetemp, sizeof(pagetemp), "Page %d", pgcount); // LOCALIZE THIS
				pgtmplen=CPP_STD::strlen(pagetemp);
				MoveTo( pageBounds.right-(pgtmplen * pFwidth)-1, lines++*pFheight);
				DrawText( pagetemp, 0, pgtmplen);
				lines++;					/* one blank line after title line */
			}
	
			if (charh!=nullptr) {									/* print from handle */
				while (lines <= maxlines && count<charlen) {
					scount=0;
					while ((count<charlen) && (*charp++!='\n')) { count++; scount++; }
					MoveTo(indent,lines++*pFheight);
					if (scount>0)
						DrawText(start, 0, scount);
					count++;
					start=charp;
				}
			} else {											/* print from file */
				while (lines <= maxlines && count<charlen) {
					rdy = 0;
					scount = 0;
					while ((count<charlen) && (!rdy)) {
						if (scount > 250)
							nextchar = ascLF;
						switch (nextchar) {
							case ascCR:
								rdy=1;
								break;
							case ascLF:
								rdy=1;
								break;
							case ascFF:
								rdy=1;
								break;
							default:
								buf[scount++]=nextchar;
								count++;
								dummyCount=1;
								if ((sts=FSRead (refNum,&dummyCount,&nextchar)) != noErr)
									{ /*snprintf(tmp,sizeof(tmp),"FSRead: ERROR %d",sts); putln(tmp);*/ }
								break;
						}
					}
					MoveTo(indent,lines*pFheight);
					if (scount>0)
						DrawText(start, 0, scount);
					if (nextchar==ascLF)
						lines++;						/* LF -> new line */
					if (nextchar==ascFF)
						lines = maxlines+1;				/* FF -> new page */
					dummyCount=1;
					if ((sts=FSRead (refNum,&dummyCount,&nextchar)) != noErr)
						{ /*snprintf(tmp,sizeof(tmp),"FSRead: ERROR %d",sts); putln(tmp);*/ }
					count++;
				}
			}
			UniversalPrint_EndPage(inPrintingRef);
			if ((sts=UniversalPrint_ReturnLastResult(inPrintingRef)) != noErr) { /*snprintf(tmp,sizeof(tmp),"ClosePage: ERROR %d",sts); putln(tmp);*/ }
		}
		else break;
	}
	if (charh != nullptr) HUnlock(charh);
	
	Alert_ReportOSStatus(sts);
	
	ProgressDialog_Dispose(&idleDialog);
	RestoreFrontmostWindow();
	Cursors_UseArrow();
}// printPages


/*!
To create a new Universal Print Record and
either fill it in with default values or
any saved values from the preferences file,
use this method.  You dispose of the record
later with UniversalPrint_DisposeContext().

Always call this routine prior to starting a
print job, from within a UniversalPrint_Init()-
UniversalPrint_Done() loop, so that the user’s
Page Setup and Print settings are used, and so
that the default print idle procedure is able
to let the user cancel the print job.

(3.0)
*/
UniversalPrint_ContextRef
TelnetPrinting_ReturnNewPrintRecord ()
{
	UniversalPrint_ContextRef		result = nullptr;
	UniversalPrint_SavedContextPtr	printDataPtr = (UniversalPrint_SavedContextPtr)Memory_NewPtr
																					(sizeof(UniversalPrint_SavedContext));
	
	
	if (printDataPtr != nullptr)
	{
		OSStatus	error = noErr;
		
		
		// create a new print record, scanning for a resource
		result = UniversalPrint_NewContext();
		
		printDataPtr->architecture = kUniversalPrint_ArchitectureTraditional;
		printDataPtr->data.traditional.storageTHPrint = GetResource(kTHPrintResourceType, kTHPrintResourceID);
		error = ResError();
		if ((printDataPtr->data.traditional.storageTHPrint != nullptr) && (error == noErr))
		{
			// update the opaque data to match the retrieved resource
			UniversalPrint_CopyFromSaved(result, printDataPtr);
			ReleaseResource(printDataPtr->data.traditional.storageTHPrint);
		}
		else
		{
			// create a new default print record and update the opaque data to match it
			printDataPtr->data.traditional.storageTHPrint = Memory_NewHandle(SIZE_OF_TPRINT);
			UniversalPrint_DefaultSaved(result, printDataPtr);
			UniversalPrint_CopyFromSaved(result, printDataPtr);
			Memory_DisposeHandle(&printDataPtr->data.traditional.storageTHPrint);
		}
		Memory_DisposePtr((Ptr*)&printDataPtr);
	}
	
	gPrintRecordInUse = result;
	return result;
}// ReturnNewPrintRecord


/*!
Spools the specified data to disk, scanning for
the special “end printer redirection” character.
If the redirection ends, the spooling is stopped
and printingEnd() is invoked.

(3.0)
*/
void
TelnetPrinting_Spool	(TerminalPrintingInfoPtr	inPrintingInfoPtr,
						 UInt8 const*				inBuffer,
						 SInt32*					pctr)
{
	long const		kEndOfPrintingSignal = '\033[4i';	// <ESC>[4i, equal to 0x1B5B3469
	SInt32			characterSpoolCount = 0L;		// the number of characters to spool to disk
	UInt8 const*	pc = inBuffer;
	char const*		startPtr = nullptr;	// original start of buffer
	OSStatus		error = noErr;
	Boolean			ready = false;	// true if <ESC>[4i
	
	
	startPtr = (char const*)pc;
	
	while ((*pctr > 0) && (!ready))
	{
		inPrintingInfoPtr->bufferTail = (inPrintingInfoPtr->bufferTail << 8) + *pc;
		if (inPrintingInfoPtr->bufferTail == kEndOfPrintingSignal)
		{
			// “printer redirection end” signal received
			ready = true;
			--characterSpoolCount; // don’t count the redirection signal as a printed character
		}
		++characterSpoolCount;
		++pc;
		--(*pctr);
	}
	
	{
		//TextTranslation_ConvertBufferToNewHandle( . . . )
		//TextTranslation_ConvertTextToMac((UInt8*)startPtr, characterSpoolCount, screenPtr->national);
	}
	if ((error = FSWrite(inPrintingInfoPtr->temporaryFileRefNum, &characterSpoolCount, startPtr)) != noErr) Sound_StandardAlert();
	if (ready || (error != noErr)) TelnetPrinting_End(inPrintingInfoPtr);
}// Spool


/*!
Adds a special instruction to the print spool
that applies to every character following it
up to the next meta-character (to reformat text
in boldface, for instance).

UNIMPLEMENTED.  Still in the “idea” stage.  But
you can call this routine in useful places in
preparation for it actually doing something. :)

(3.1)
*/
void
TelnetPrinting_SpoolMetaCharacter	(TerminalPrintingInfoPtr	inPrintingInfoPtr,
									 TerminalPrintingMetaChar	inSpecialInstruction)
{
#if 0
	SInt32		length = sizeof(inSpecialInstruction);
	
	
	TelnetPrinting_Spool(inPrintingInfoPtr, &inSpecialInstruction, &length);
#endif
}// SpoolMetaCharacter


#pragma mark Internal Methods

/*!
This routine services network events and watches
for user-initiated printing cancellation.  It is
called by the operating system repeatedly while
printing is in progress.

To avoid potentially serious driver problems,
this idle procedure always saves and restores
critical system state information, such as the
current resource file and graphics port.

(3.0)
*/
static pascal void
printIdle ()
{
	SInt16		oldResFile = CurResFile();
	CGrafPtr	oldPort = nullptr;
	GDHandle	oldDevice = nullptr;
	
	
	GetGWorld(&oldPort, &oldDevice);
	if (EventLoop_IsNextCancel()) UniversalPrint_Cancel(gPrintRecordInUse);
	UseResFile(oldResFile); // restore this in case it somehow got changed
	SetGWorld(oldPort, oldDevice);
}// printIdle


/*	printText -	Print the text selected on the screen. */
static void
printText	(TerminalScreenRef	inScreen,			/* Which screen to print */
			 TerminalViewRef	inView,
			 ConstStringPtr		inTitle)			/* The title of the printout (the session name) */
{
	Handle						charh = nullptr;
	UniversalPrint_ContextRef	printRecordRef = nullptr;
	
	
	UniversalPrint_Init();
	printRecordRef = TelnetPrinting_ReturnNewPrintRecord();
	
	charh = TerminalView_ReturnSelectedTextAsNewHandle(inView, 0, 0/* flags */);		/* Get the characters to print */
	
	if (IsHandleValid(charh))
	{
		Boolean		printOK = false;
		
		
		if (UniversalPrint_ReturnMode() == kUniversalPrint_ModeNormal)
		{
			DeactivateFrontmostWindow();
			Cursors_UseArrow();
			printOK = UniversalPrint_JobDialogDisplay(printRecordRef);
			RestoreFrontmostWindow();
		}
		else
		{
			printOK = true;
			UniversalPrint_SetNumberOfCopies(printRecordRef, 1, false/* lock */);
		}
		
		if (printOK)
		{
			UniversalPrint_BeginDocument(printRecordRef);
			if (UniversalPrint_ReturnLastResult(printRecordRef) == noErr)
			{
				printPages(printRecordRef, inTitle, Terminal_ReturnColumnCount(inScreen), charh, (SInt16)-1);
				UniversalPrint_EndDocument(printRecordRef);
			}
		}
		else
		{
			// print was cancelled...
		}
		HPurge(charh);							/* Kill the printed chars */
		Cursors_UseArrow();
		UniversalPrint_DisposeContext(&printRecordRef);
	}
}// printText

// BELOW IS REQUIRED NEWLINE TO END FILE
