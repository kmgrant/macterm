/*###############################################################

	Session.cp
	
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
#include <sstream>
#include <string>

// standard-C++ includes
#include <algorithm>
#include <map>
#include <vector>

// Unix includes
#include <strings.h>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <AppleEventUtilities.h>
#include <CFRetainRelease.h>
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <Cursors.h>
#include <FileSelectionDialogs.h>
#include <ListenerModel.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// resource includes
#include "GeneralResources.h"
#include "SpacingConstants.r"
#include "StringResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "BasicTypesAE.h"
#include "Clipboard.h"
#include "Commands.h"
#include "ConnectionData.h"
#include "DialogUtilities.h"
#include "DragAndDrop.h"
#include "EventInfoControlScope.h"
#include "EventLoop.h"
#include "FileUtilities.h"
#include "MacroManager.h"
#include "NetEvents.h"
#include "Preferences.h"
#include "QuillsSession.h"
#include "RasterGraphicsKernel.h"
#include "RasterGraphicsScreen.h"
#include "RecordAE.h"
#include "Session.h"
#include "SessionDescription.h"
#include "SessionFactory.h"
#include "SpecialKeySequencesDialog.h"
#include "TektronixVirtualGraphics.h"
#include "TektronixRealGraphics.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "TerminalWindow.h"
#include "Terminology.h"
#include "UIStrings.h"
#include "VTKeys.h"



#pragma mark Constants

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	enum Session_Type
	{
		kSession_TypeLocalNonLoginShell		= 0,	// local Unix process running the user’s preferred shell
		kSession_TypeLocalLoginShell		= 1		// local Unix process running /usr/bin/login
	};
}

#pragma mark Types

typedef std::map< HIViewRef, EventHandlerRef >		ControlDragDropHandlerMap;

typedef std::map< HIViewRef, EventHandlerRef >		ControlTextInputHandlerMap;

typedef std::map< Session_PropertyKey, void* >		SessionAuxiliaryDataMap;

typedef std::vector< TerminalScreenRef >	MyCaptureFileList;

/*!
A “safe” wrapper around the help tag structure.
Useful for constructing it in one shot while
initializing everything.  Pass nullptr for a tag
string if you do not intend to use it.

The input location is for flexibility...currently
it is converted to an integer.

None of the input strings is retained, so do
that beforehand.  Note that help tags must
generally be global because it is rarely safe
to delete them (one never knows when the system
will be finished using a tag).
*/
struct My_HMHelpContentRecWrap
{
public:
	My_HMHelpContentRecWrap		(HMTagDisplaySide	inDisplaySide = kHMOutsideBottomCenterAligned)
	{
		_rec.version = kMacHelpVersion;
		_rec.tagSide = inDisplaySide;
		setFrame(CGRectMake(0, 0, 0, 0));
		rename(nullptr, nullptr);
	}
	
	inline CFStringRef
	altName () const
	{
		return _altName;
	}
	
	inline CFStringRef
	mainName () const
	{
		return _mainName;
	}
	
	inline HMHelpContentRec const*
	ptr () const { return &_rec; }
	
	inline void
	rename	(CFStringRef	inMain,
			 CFStringRef	inAlt)
	{
		// the system does not allow a name to be undefined,
		// so an internal reference is kept to make it easier
		// to detect never-defined tags
		_mainName = inMain;
		_rec.content[0].contentType = kHMCFStringContent;
		_rec.content[0].u.tagCFString = _mainName;
		if (nullptr == _mainName)
		{
			_rec.content[0].u.tagCFString = CFSTR("");
		}
		
		// the system does not allow a name to be undefined,
		// so an internal reference is kept to make it easier
		// to detect never-defined tags
		_altName = inAlt;
		_rec.content[1].contentType = kHMCFStringContent;
		_rec.content[1].u.tagCFString = _altName;
		if (nullptr == inAlt)
		{
			_rec.content[1].u.tagCFString = CFSTR("");
		}
	}
	
	inline void
	setFrame	(HIRect const&		inNewFrame)
	{
		// the tag is forced to render at a point, so the rectangle is squished
		SetRect(&_rec.absHotRect, STATIC_CAST(inNewFrame.origin.x, short), STATIC_CAST(inNewFrame.origin.y, short),
				0, 0);
		SetRect(&_rec.absHotRect, _rec.absHotRect.left, _rec.absHotRect.top,
				_rec.absHotRect.left + STATIC_CAST(inNewFrame.size.width, short),
				_rec.absHotRect.top + STATIC_CAST(inNewFrame.size.height, short));
	}

private:
	HMHelpContentRec	_rec;
	CFStringRef			_mainName;	// unlike the record name, this can be nullptr
	CFStringRef			_altName;	// unlike the record name, this can be nullptr
};

typedef std::vector< TerminalScreenRef >	MyPrintJobList;

typedef std::vector< SInt16 >				MyRasterGraphicsScreenList;

typedef std::vector< SInt16 >				MyTEKGraphicList;

typedef std::vector< TerminalScreenRef >	MyTerminalScreenList;

struct Session
{
	Boolean						readOnly;					// whether or not user input is allowed
	Session_Type				kind;						// type of session; affects use of the union below
	Session_State				status;						// indicates whether session is initialized, etc.
	Session_StateAttributes		statusAttributes;			// “tags” for the status, above
	CFRetainRelease				statusString;				// one word (usually) describing the state succinctly
	CFRetainRelease				resourceLocationString;		// one-liner for remote URL or local Unix command line
	CFRetainRelease				deviceNameString;			// pathname of slave pseudo-terminal device attached to the session
	UInt32						connectionDateTime;			// result of GetDateTime() call at connection time
	CFAbsoluteTime				terminationAbsoluteTime;	// result of CFAbsoluteTimeGetCurrent() call at disconnection time
	EventHandlerUPP				windowClosingUPP;			// wrapper for window closing callback
	EventHandlerRef				windowClosingHandler;		// invoked whenever a session terminal window should close
	EventHandlerUPP				windowFocusChangeUPP;		// wrapper for window focus-change callback
	EventHandlerRef				windowFocusChangeHandler;	// invoked whenever a session terminal window is chosen for keyboard input
	EventHandlerUPP				terminalViewDragDropUPP;	// wrapper for drag-and-drop callback
	ControlDragDropHandlerMap	terminalViewDragDropHandlers;// invoked whenever a terminal view is the target of drag-and-drop
	EventHandlerUPP				terminalViewEnteredUPP;		// wrapper for mouse tracking (focus-follows-mouse) callback
	ControlDragDropHandlerMap	terminalViewEnteredHandlers;// invoked whenever the mouse moves into a terminal view
	EventHandlerUPP				terminalViewTextInputUPP;   // wrapper for keystroke callback
	ControlTextInputHandlerMap	terminalViewTextInputHandlers;// invoked whenever a terminal view is focused during a key press
	ListenerModel_Ref			changeListenerModel;		// who to notify for various kinds of changes to this session data
	ListenerModel_ListenerRef	changeListener;				// listener that updates the status string
	ListenerModel_ListenerRef	dataArrivalListener;		// listener that processes incoming data
	ListenerModel_ListenerRef	windowValidationListener;	// responds after a window is created, and just before it dies
	ListenerModel_ListenerRef	terminalWindowListener;		// responds when terminal window states change
	ListenerModel_ListenerRef	preferencesListener;		// responds when certain preference values are initialized or changed
	EventLoopTimerUPP			longLifeTimerUPP;			// procedure that is called when a session has been open 15 seconds
	EventLoopTimerRef			longLifeTimer;				// 15-second timer
	ConnectionDataPtr			dataPtr;					// data for this connection
	InterfaceLibAlertRef		currentTerminationAlert;	// retained while a sheet is still open so a 2nd sheet is not displayed
	TerminalWindowRef			terminalWindow;				// terminal window housing this session
	Local_ProcessRef			mainProcess;				// the command whose output is directly attached to the terminal
	MyTEKGraphicList			targetVectorGraphics;		// list of TEK graphics attached to this session
	MyRasterGraphicsScreenList	targetRasterGraphicsScreens;	// list of open ICR graphics screens, if any
	MyTerminalScreenList		targetDumbTerminals;		// list of DUMB terminals to which incoming data is being copied
	MyTerminalScreenList		targetTerminals;			// list of screen buffers to which incoming data is being copied
	MyCaptureFileList			targetFiles;				// list of open files, if any, to which incoming data is being copied
	MyPrintJobList				targetPrintJobs;			// list of open printer spool files, if any, to which incoming data is being copied
	UInt8*						readBufferPtr;				// buffer space for processing data
	size_t						readBufferSizeMaximum;		// maximum number of bytes that can be processed at once
	size_t						readBufferSizeInUse;		// number of bytes of data currently in the read buffer
	CFStringEncoding			writeEncoding;				// the character set that text (data) sent to a session should be using
	Session_Watch				activeWatch;				// if any, what notification is currently set up for internal data events
	EventLoopTimerUPP			inactivityWatchTimerUPP;	// procedure that is called if data has not arrived after awhile
	EventLoopTimerRef			inactivityWatchTimer;		// short timer
	AlertMessages_BoxRef		watchBox;					// if defined, the global alert used to show notifications for this session
	SessionAuxiliaryDataMap		auxiliaryDataMap;			// all tagged data associated with this session
	SessionRef					selfRef;					// convenient reference to this structure
	
	struct
	{
		Boolean		cursorFlashes;				//!< preferences callback should update this value
		Boolean		remapBackquoteToEscape;		//!< preferences callback should update this value
	} preferencesCache;
};
typedef Session*		SessionPtr;
typedef SessionPtr*		SessionHandle;

typedef MemoryBlockPtrLocker< SessionRef, Session >		SessionPtrLocker;
typedef LockAcquireRelease< SessionRef, Session >		SessionAutoLocker;

struct My_PasteAlertInfo
{
	// sent to pasteWarningCloseNotifyProc()
	SessionRef			sessionForPaste;
	CFRetainRelease		clipboardData;
};
typedef My_PasteAlertInfo*		My_PasteAlertInfoPtr;

struct TerminateAlertInfo
{
	// sent to terminationWarningCloseNotifyProc()
	SessionRef		sessionBeingTerminated;
};
typedef TerminateAlertInfo*		TerminateAlertInfoPtr;

struct My_WatchAlertInfo
{
	// sent to watchCloseNotifyProc()
	SessionRef		sessionBeingWatched;
};
typedef My_WatchAlertInfo*		My_WatchAlertInfoPtr;

#pragma mark Internal Method Prototypes

static void					changeNotifyForSession				(SessionPtr, Session_Change, void*);
static void					changeStateAttributes				(SessionPtr, Session_StateAttributes,
																	Session_StateAttributes);
static void					configureSaveDialog					(SessionRef, NavDialogCreationOptions&);
static void					connectionStateChanged				(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
static My_HMHelpContentRecWrap&	createHelpTagForInterrupt		();
static My_HMHelpContentRecWrap&	createHelpTagForResume			();
static My_HMHelpContentRecWrap&	createHelpTagForSuspend			();
static PMPageFormat			createSessionPageFormat				();
static IconRef				createSessionStateActiveIcon		();
static IconRef				createSessionStateDeadIcon			();
static void					dataArrived							(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
static pascal void			detectLongLife						(EventLoopTimerRef, void*);
static void					getKeyEventCharacters				(SInt16, SInt16, EventModifiers, SInt16*, SInt16*);
static Boolean				handleSessionKeyDown				(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
static Boolean				isReadOnly							(SessionPtr);
static void					killConnection						(SessionPtr);
static pascal void			navigationFileCaptureDialogEvent	(NavEventCallbackMessage, NavCBRecPtr, void*);
static pascal void			navigationSaveDialogEvent			(NavEventCallbackMessage, NavCBRecPtr, void*);
static pascal void			pageSetupCloseNotifyProc			(PMPrintSession, WindowRef, Boolean);
static void					pasteWarningCloseNotifyProc			(InterfaceLibAlertRef, SInt16, void*);
static void					preferenceChanged					(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
static Boolean				queueCharacterInKeyboardBuffer		(SessionPtr, char);
static pascal OSStatus		receiveTerminalViewDragDrop			(EventHandlerCallRef, EventRef, void*);
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
static pascal OSStatus		receiveTerminalViewEntered			(EventHandlerCallRef, EventRef, void*);
#endif
static pascal OSStatus		receiveTerminalViewTextInput		(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveWindowClosing				(EventHandlerCallRef, EventRef, void*);
static pascal OSStatus		receiveWindowFocusChange			(EventHandlerCallRef, EventRef, void*);
static void					sendRecordableDataTransmitEvent		(CFStringRef, Boolean);
static void					sendRecordableDataTransmitEvent		(char const*, SInt16, Boolean);
static void					sendRecordableSpecialTransmitEvent	(FourCharCode, Boolean);
static OSStatus				sessionDragDrop						(EventHandlerCallRef, EventRef, SessionRef,
																 HIViewRef, DragRef);
static void					terminalWindowChanged				(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
static void					terminationWarningCloseNotifyProc	(InterfaceLibAlertRef, SInt16, void*);
static void					watchClearForSession				(SessionPtr);
static void					watchCloseNotifyProc				(AlertMessages_BoxRef, SInt16, void*);
static void					watchNotifyForSession				(SessionPtr, Session_Watch);
static pascal void			watchNotifyFromTimer				(EventLoopTimerRef, void*);
static void					watchTimerResetForSession			(SessionPtr, Session_Watch);
static void					windowValidationStateChanged		(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	SessionPtrLocker&	gSessionPtrLocks ()	{ static SessionPtrLocker x; return x; }
	PMPageFormat&		gSessionPageFormat () { static PMPageFormat x = createSessionPageFormat(); return x; }
	IconRef				gSessionActiveIcon () { static IconRef x = createSessionStateActiveIcon(); return x; }
	IconRef				gSessionDeadIcon () { static IconRef x = createSessionStateDeadIcon(); return x; }
}

#pragma mark Functors

/*!
Appends the specified data to a file currently
receiving captures from the given screen.

Model of STL Unary Function.

(1.0)
*/
#pragma mark captureFileDataWriter
class captureFileDataWriter:
public std::unary_function< TerminalScreenRef/* argument */, void/* return */ >
{
public:
	captureFileDataWriter	(UInt8 const*	inBuffer,
							 size_t			inBufferSize)
	: _buffer(inBuffer), _bufferSize(inBufferSize)
	{
	}
	
	void
	operator()	(TerminalScreenRef	inScreen)
	{
		Terminal_FileCaptureWriteData(inScreen, _buffer, _bufferSize);
	}

protected:

private:
	UInt8 const*	_buffer;
	size_t			_bufferSize;
};

/*!
Writes the specified data to a print job that
is currently in progress for the given screen.

Model of STL Unary Function.

(1.0)
*/
#pragma mark printJobDataWriter
class printJobDataWriter:
public std::unary_function< TerminalScreenRef/* argument */, void/* return */ >
{
public:
	printJobDataWriter	(UInt8 const*	inBuffer,
						 size_t			inBufferSize)
	: _buffer(inBuffer), _bufferSize(inBufferSize)
	{
	}
	
	void
	operator()	(TerminalScreenRef		inScreen)
	{
		//Terminal_PrintingSpool(inScreen, _buffer, &_bufferSize);
	}

protected:

private:
	UInt8 const*	_buffer;
	size_t			_bufferSize;
};

/*!
Writes the specified data to a given Interactive
Color Raster Graphics screen.

Model of STL Unary Function.

(1.0)
*/
#pragma mark rasterGraphicsDataWriter
class rasterGraphicsDataWriter:
public std::unary_function< SInt16/* argument */, void/* return */ >
{
public:
	rasterGraphicsDataWriter	(UInt8 const*	inBuffer,
								 size_t			inBufferSize)
	: _buffer(inBuffer), _bufferSize(inBufferSize)
	{
	}
	
	void
	operator()	(SInt16		UNUSED_ARGUMENT(inICRWindowID))
	{
		char*	mutableCopy = REINTERPRET_CAST(Memory_NewPtr(_bufferSize), char*);
		
		
		if (mutableCopy != nullptr)
		{
			BlockMoveData(_buffer, mutableCopy, INTEGER_MINIMUM(GetPtrSize(mutableCopy), _bufferSize));
			VRwrite(mutableCopy, GetPtrSize(mutableCopy));
			Memory_DisposePtr(&mutableCopy);
		}
	}

protected:

private:
	UInt8 const*	_buffer;
	size_t			_bufferSize;
};

/*!
Writes the specified data to a given terminal,
allowing the terminal to interpret the data in
some special way if appropriate (for example,
VT100 escape sequences).

Model of STL Unary Function.

(1.0)
*/
#pragma mark terminalDataWriter
class terminalDataWriter:
public std::unary_function< TerminalScreenRef/* argument */, void/* return */ >
{
public:
	terminalDataWriter	(UInt8 const*	inBuffer,
						 size_t			inBufferSize)
	: _buffer(inBuffer), _bufferSize(inBufferSize)
	{
	}
	
	void
	operator()	(TerminalScreenRef	inScreen)
	{
		Terminal_EmulatorProcessData(inScreen, _buffer, _bufferSize);
	}

protected:

private:
	UInt8 const*	_buffer;
	size_t			_bufferSize;
};

/*!
Writes the specified data to a given terminal,
only all non-printable characters are changed
to be printable.  In other words, it assumes
the terminal is “dumb” and will not know what
to do with such characters.

Model of STL Unary Function.

(1.0)
*/
#pragma mark terminalDumbDataWriter
class terminalDumbDataWriter:
public std::unary_function< TerminalScreenRef/* argument */, void/* return */ >
{
public:
	terminalDumbDataWriter	(UInt8 const*	inBuffer,
							 size_t			inBufferSize)
	: _buffer(inBuffer), _bufferSize(inBufferSize)
	{
	}
	
	void
	operator()	(TerminalScreenRef	inScreen)
	{
		// dumb terminal - raw mode for debugging, pass through escape sequences
		// and other special characters as <27> symbols
		register size_t		i = 0;
		UInt8 const*		currentCharPtr = _buffer;
		
		
		for (i = 0; i < _bufferSize; ++i, ++currentCharPtr)
		{
			if ((*currentCharPtr < ' '/* codes below a plain space are control characters */) ||
				(*currentCharPtr >= 127))
			{
				std::ostringstream		tempBuffer;
				
				
				tempBuffer
				<< "<"
				<< STATIC_CAST(*currentCharPtr, unsigned int)
				<< ">"
				;
				_tempBufferString = tempBuffer.str();
				Terminal_EmulatorProcessCString(inScreen, _tempBufferString.c_str());
			}
			else
			{
				Terminal_EmulatorProcessData(inScreen, currentCharPtr, 1);
			}
		}
	}

protected:

private:
	UInt8 const*	_buffer;
	size_t			_bufferSize;
	std::string		_tempBufferString;
};

/*!
Writes the specified data to a given TEK window.

Model of STL Unary Function.

(1.0)
*/
#pragma mark vectorGraphicsDataWriter
class vectorGraphicsDataWriter:
public std::unary_function< SInt16/* argument */, void/* return */ >
{
public:
	vectorGraphicsDataWriter	(UInt8 const*	inBuffer,
								 size_t			inBufferSize)
	: _buffer(inBuffer), _bufferSize(inBufferSize)
	{
	}
	
	void
	operator()	(SInt16		inTEKWindowID)
	{
		_result = VGwrite(inTEKWindowID, REINTERPRET_CAST(_buffer, char const*), STATIC_CAST(_bufferSize, SInt16));
	}

protected:

private:
	UInt8 const*	_buffer;
	size_t			_bufferSize;
	SInt16			_result;
};


#pragma mark Public Methods

/*!
Creates a Session and returns a reference to it.  If
any problems occur, nullptr is returned.

In general, you should NOT create sessions this way;
use the Session Factory module.  In particular, note
that Session_New() performs MINIMAL INITIALIZATION,
and you need to call various Session_...() APIs to
set up the new session appropriately.  When finished
initializing, change the session state by calling
Session_SetState(session, kSession_StateInitialized),
so that any modules listening for session state
changes will realize that a new, initialized session
has arrived.

(3.0)
*/
SessionRef
Session_New		(Boolean	inIsReadOnly)
{
	SessionRef		result = REINTERPRET_CAST(new Session, SessionRef);
	
	
	// add an entry for this reference to the map
	if (result != nullptr)
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), result);
		
		
		ptr->mainProcess = nullptr;
		ptr->readOnly = inIsReadOnly;
		ptr->kind = kSession_TypeLocalNonLoginShell;
		ptr->changeListenerModel = ListenerModel_New(kListenerModel_StyleStandard,
														kConstantsRegistry_ListenerModelDescriptorSessionChanges);
		ptr->changeListener = ListenerModel_NewStandardListener(connectionStateChanged);
		ptr->dataArrivalListener = ListenerModel_NewStandardListener(dataArrived);
		ptr->windowValidationListener = ListenerModel_NewStandardListener(windowValidationStateChanged);
		//ptr->terminalWindowListener set in response to window-open event
		//drag-and-drop handlers set in response to window-open event
		ptr->statusString.clear();
		ptr->resourceLocationString.clear();
		ptr->deviceNameString.clear();
		try
		{
			ptr->dataPtr = REINTERPRET_CAST(new ConnectionData(), ConnectionDataPtr);
		}
		catch (std::bad_alloc)
		{
			ptr->dataPtr = nullptr;
		}
		ptr->currentTerminationAlert = nullptr;
		ptr->terminalWindow = nullptr;
		ptr->readBufferSizeMaximum = 512; // arbitrary, for initialization; see Session_SetDataProcessingCapacity()
		ptr->readBufferSizeInUse = 0;
		// WARNING: Session_SetDataProcessingCapacity() also allocates/deallocates this buffer
		ptr->readBufferPtr = new UInt8[ptr->readBufferSizeMaximum];
		assert(nullptr != ptr->readBufferPtr);
		ptr->writeEncoding = kCFStringEncodingUTF8; // initially...
		ptr->activeWatch = kSession_WatchNothing;
		ptr->inactivityWatchTimerUPP = nullptr;
		ptr->inactivityWatchTimer = nullptr;
		ptr->watchBox = nullptr;
		//ptr->auxiliaryDataMap is self-initializing
		ptr->selfRef = result;
		Session_StartMonitoring(result, kSession_ChangeState, ptr->changeListener);
		Session_StartMonitoring(result, kSession_ChangeDataArrived, ptr->dataArrivalListener);
		Session_StartMonitoring(result, kSession_ChangeWindowValid, ptr->windowValidationListener);
		Session_StartMonitoring(result, kSession_ChangeWindowInvalid, ptr->windowValidationListener);
		
		ptr->status = kSession_StateBrandNew;
		changeNotifyForSession(ptr, kSession_ChangeState, result/* context */);
		ptr->statusAttributes = 0;
		changeNotifyForSession(ptr, kSession_ChangeStateAttributes, result/* context */);
		
		// 3.0 - record the time when the connection began
		GetDateTime(&ptr->connectionDateTime);
		ptr->terminationAbsoluteTime = 0;
		
		// install a one-shot timer to tell interested parties when this session
		// has been opened for 15 seconds
		assert(ptr->selfRef != nullptr);
		ptr->longLifeTimerUPP = NewEventLoopTimerUPP(detectLongLife);
		(OSStatus)InstallEventLoopTimer(GetCurrentEventLoop(),
										kEventDurationSecond * kSession_LifetimeMinimumForNoWarningClose,
										kEventDurationForever/* time between fires - this timer does not repeat */,
										ptr->longLifeTimerUPP, ptr->selfRef/* user data */, &ptr->longLifeTimer);
		
		// create a callback for preferences, then listen for certain preferences
		// (this will also initialize the preferences cache values)
		ptr->preferencesListener = ListenerModel_NewStandardListener(preferenceChanged, result/* context */);
		Preferences_StartMonitoring(ptr->preferencesListener, kPreferences_TagCursorBlinks,
									true/* call immediately to initialize */);
		Preferences_StartMonitoring(ptr->preferencesListener, kPreferences_TagMapBackquote,
									true/* call immediately to initialize */);
	}
	
	return result;
}// New


/*!
Destroys a session created with Session_New().

(3.0)
*/
void
Session_Dispose		(SessionRef*	inoutRefPtr)
{
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), *inoutRefPtr);
		
		
		ptr->status = kSession_StateImminentDisposal;
		changeNotifyForSession(ptr, kSession_ChangeState, *inoutRefPtr/* context */);
	}
	
	if (gSessionPtrLocks().returnLockCount(*inoutRefPtr) > 0)
	{
		Console_WriteLine("warning, attempt to dispose of locked session");
	}
	else
	{
		{
			SessionAutoLocker	ptr(gSessionPtrLocks(), *inoutRefPtr);
			
			
			// clean up
			Preferences_StopMonitoring(ptr->preferencesListener, kPreferences_TagCursorBlinks);
			Preferences_StopMonitoring(ptr->preferencesListener, kPreferences_TagMapBackquote);
			ListenerModel_ReleaseListener(&ptr->preferencesListener);
			
			Session_StopMonitoring(*inoutRefPtr, kSession_ChangeState, ptr->changeListener);
			Session_StopMonitoring(*inoutRefPtr, kSession_ChangeDataArrived, ptr->dataArrivalListener);
			Session_StopMonitoring(*inoutRefPtr, kSession_ChangeWindowValid, ptr->windowValidationListener);
			Session_StopMonitoring(*inoutRefPtr, kSession_ChangeWindowInvalid, ptr->windowValidationListener);
			
			delete ptr->dataPtr, ptr->dataPtr = nullptr;
			
			// dispose contents
			// WARNING: Session_SetDataProcessingCapacity() also allocates/deallocates this buffer
			if (ptr->readBufferPtr != nullptr) delete [] ptr->readBufferPtr, ptr->readBufferPtr = nullptr;
			ListenerModel_ReleaseListener(&ptr->windowValidationListener);
			ListenerModel_ReleaseListener(&ptr->dataArrivalListener);
			ListenerModel_ReleaseListener(&ptr->changeListener);
			ListenerModel_Dispose(&ptr->changeListenerModel);
		}
		
		// delete outside the lock block
		delete *(REINTERPRET_CAST(inoutRefPtr, SessionPtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Arranges for the specified target to be considered for all
subsequent data written to this session.  For example, if
you add an open file, arriving text will be appended to
the file; if you add a terminal screen, text will be sent
to the terminal; if you add a graphics device, data will
be interpreted as graphics commands, etc.

“Incompatible” types are detected automatically, and all
existing targets that are not compatible with the new target
will be disabled if necessary.  This means you do not have
to worry about targets receiving data they do not know how
to handle.

See documentation on Session_DataTarget for details.

\retval kSession_ResultOK
if the target was added successfully

\retval kSession_ResultInvalidReference
if "inRef" is invalid

\retval kSession_ResultParameterError
if "inTarget" or "inTargetData" are invalid

(3.0)
*/
Session_Result
Session_AddDataTarget	(SessionRef				inRef,
						 Session_DataTarget		inTarget,
						 void*					inTargetData)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_Result		result = kSession_ResultOK;
	
	
	switch (inTarget)
	{
	case kSession_DataTargetStandardTerminal:
		{
			MyTerminalScreenList::size_type		listSize = ptr->targetTerminals.size();
			
			
			ptr->targetTerminals.push_back(REINTERPRET_CAST(inTargetData, TerminalScreenRef));
			assert(ptr->targetTerminals.size() == (1 + listSize));
		}
		break;
	
	case kSession_DataTargetDumbTerminal:
		{
			MyTerminalScreenList::size_type		listSize = ptr->targetDumbTerminals.size();
			
			
			ptr->targetDumbTerminals.push_back(REINTERPRET_CAST(inTargetData, TerminalScreenRef));
			assert(ptr->targetDumbTerminals.size() == (1 + listSize));
		}
		break;
	
	case kSession_DataTargetTektronixGraphicsCanvas:
		{
			MyTEKGraphicList::size_type		listSize = ptr->targetVectorGraphics.size();
			
			
			ptr->targetVectorGraphics.push_back(*(REINTERPRET_CAST(inTargetData, SInt16 const*/* TEK graphics ID */)));
			assert(ptr->targetVectorGraphics.size() == (1 + listSize));
		}
		break;
	
	case kSession_DataTargetInteractiveColorRasterGraphicsScreen:
		{
			MyRasterGraphicsScreenList::size_type	listSize = ptr->targetRasterGraphicsScreens.size();
			
			
			ptr->targetRasterGraphicsScreens.push_back(*(REINTERPRET_CAST(inTargetData, SInt16 const*/* ICR window ID */)));
			assert(ptr->targetRasterGraphicsScreens.size() == (1 + listSize));
		}
		break;
	
	case kSession_DataTargetOpenCaptureFile:
		{
			MyCaptureFileList::size_type	listSize = ptr->targetFiles.size();
			
			
			ptr->targetFiles.push_back(REINTERPRET_CAST(inTargetData, TerminalScreenRef));
			assert(ptr->targetFiles.size() == (1 + listSize));
		}
		break;
	
	default:
		// ???
		result = kSession_ResultParameterError;
		break;
	}
	return result;
}// AddDataTarget


/*!
Copies the specified data into the “read buffer” of
the specified session, starting one byte beyond the
last byte currently in the buffer, and inserts an
event in the internal queue (if there isn’t one already)
informing the session that there is data to be handled.

Returns the number of bytes NOT appended; will be 0
if there was enough room for all the given data.

(3.0)
*/
size_t
Session_AppendDataForProcessing		(SessionRef		inRef,
									 UInt8 const*	inDataPtr,
									 size_t			inSize)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	size_t				numberOfBytesToCopy = 0;
	size_t const		kMaximumBytesLeft = ptr->readBufferSizeMaximum - ptr->readBufferSizeInUse;
	size_t				result = inSize;
	
	
	// figure out how much to copy
	numberOfBytesToCopy = INTEGER_MINIMUM(kMaximumBytesLeft, inSize);
	if (numberOfBytesToCopy > 0)
	{
		result = inSize - numberOfBytesToCopy;
		BlockMoveData(inDataPtr, ptr->readBufferPtr + ptr->readBufferSizeInUse, numberOfBytesToCopy);
		ptr->readBufferSizeInUse += numberOfBytesToCopy;
		changeNotifyForSession(ptr, kSession_ChangeDataArrived, inRef/* context */);
	}
	return result;
}// AppendDataForProcessing


/*!
Returns a pointer to the "ConnectionDataPtr" data in the
internal structure.

TEMPORARY, TRANSITIONAL FUNCTION THAT *WILL* DISAPPEAR!!!

(3.0)
*/
ConnectionDataPtr
Session_ConnectionDataPtr	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	ConnectionDataPtr	result = ptr->dataPtr;
	
	
	return result;
}// ConnectionDataPtr


/*!
Returns an icon that describes the session status (for
example, running or not running).  This can be displayed
in user interface elements.

If any problems occur, nullptr is returned.  Otherwise,
the icon is acquired, so use ReleaseIconRef() when
finished with it.

Any applied attributes (see Session_StateAttribute) *may*
influence the icon that is returned.  For example, a
badge may be added.

(3.1)
*/
Session_Result
Session_CopyStateIconRef	(SessionRef		inRef,
							 IconRef&		outCopiedIcon)
{
	Session_Result		result = kSession_ResultOK;
	
	
	outCopiedIcon = nullptr;
	if (inRef == nullptr) result = kSession_ResultInvalidReference;
	else
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		switch (ptr->status)
		{
		// for now, these all use the same icon
		case kSession_StateBrandNew:
		case kSession_StateInitialized:
		case kSession_StateActiveUnstable:
		case kSession_StateActiveStable:
			outCopiedIcon = gSessionActiveIcon();
			assert_noerr(AcquireIconRef(outCopiedIcon));
			break;
		
		case kSession_StateDead:
		case kSession_StateImminentDisposal:
			outCopiedIcon = gSessionDeadIcon();
			assert_noerr(AcquireIconRef(outCopiedIcon));
			break;
		
		default:
			// ???
			break;
		}
		
		switch (ptr->statusAttributes)
		{
		case kSession_StateAttributeNotification:
			// TEMPORARY: a notification-specific icon (a bell?) may be better here
			(OSStatus)GetIconRef(kOnSystemDisk, kSystemIconsCreator,
									kAlertCautionIcon, &outCopiedIcon);
			break;
		
		case kSession_StateAttributeOpenDialog:
			(OSStatus)GetIconRef(kOnSystemDisk, kSystemIconsCreator,
									kAlertCautionIcon, &outCopiedIcon);
			break;
		
		default:
			break;
		}
		
		if (nullptr == outCopiedIcon)
		{
			// return some non-success value...
			result = kSession_ResultProtocolMismatch;
		}
	}
	
	return result;
}// CopyStateIconRef


/*!
Disconnects an active connection, freeing allocated memory
and (if the user has such a preference set) destroying
Kerberos tickets.  The window remains open unless the user
has that preference disabled, in which case this routine
invokes killConnection(), as well.

If a connection fails or is aborted “early”, it is usually
destroyed with killConnection().  If the connection’s
window is already on the screen and the connection has not
been previously disconnected, it is usually terminated with
Connections_Disconnect() and then ultimately killed with
killConnection() later.  If you add clean-up code to this
routine, make sure you consider when your code for
initialization is done - it may be more correct to do your
clean-up in killConnection().  When in doubt, do clean-up
in killConnection().

(3.0)
*/
void
Session_Disconnect		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	ConnectionDataPtr	connectionDataPtr = ptr->dataPtr;
	
	
	if (Session_StateIsActive(inRef))
	{
		// active session; terminate associated tasks
		if (Terminal_FileCaptureInProgress(connectionDataPtr->vs)) Terminal_FileCaptureEnd(connectionDataPtr->vs);
		if (Terminal_PrintingInProgress(connectionDataPtr->vs)) Terminal_PrintingEnd(connectionDataPtr->vs);
	}
	
	killConnection(ptr);
}// Disconnect


/*!
Displays a Save dialog for a capture file based on the
contents of the given terminal window, handling the user’s
response automatically.  The dialog could be a sheet, so
this routine may return immediately without the user
having responded to the save; the file capture initiation
(or lack thereof) is handled automatically, asynchronously,
in that case.

(3.0)
*/
void
Session_DisplayFileCaptureSaveDialog	(SessionRef		inRef)
{
	NavDialogCreationOptions	dialogOptions;
	NavDialogRef				navigationServicesDialog = nullptr;
	OSStatus					error = noErr;
	
	
	error = NavGetDefaultDialogCreationOptions(&dialogOptions);
	if (error == noErr)
	{
		// this call sets most of the options up front (parent window, etc.)
		configureSaveDialog(inRef, dialogOptions);
		
		// now set things specific to this instance
		(UIStrings_Result)UIStrings_Copy(kUIStrings_FileDefaultCaptureFile, dialogOptions.saveFileName);
		(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptCaptureToFile, dialogOptions.message);
		dialogOptions.modality = kWindowModalityWindowModal;
	}
	error = NavCreatePutFileDialog(&dialogOptions, 'TEXT'/* type */, 'ttxt'/* creator (TextEdit) */,
									NewNavEventUPP(navigationFileCaptureDialogEvent), inRef/* client data */,
									&navigationServicesDialog);
	
	// display the dialog; it is a sheet, so this will return immediately
	// and the dialog will close whenever the user is actually done with it
	error = NavDialogRun(navigationServicesDialog);
}// DisplayFileCaptureSaveDialog


/*!
Displays a Print dialog for the given terminal window,
handling the user’s response automatically.  The dialog
could be a sheet, so this routine may return immediately
without the user having confirmed the print.

(3.1)
*/
void
Session_DisplayPrintJobDialog	(SessionRef		inRef)
{
	// UNIMPLEMENTED
	Console_WriteLine("warning, unimplemented");
}// DisplayPrintJobDialog


/*!
Displays a Print dialog for the given terminal window,
handling the user’s response automatically.  The dialog
could be a sheet, so this routine may return immediately
without the user having confirmed the print.

(3.1)
*/
void
Session_DisplayPrintPageSetupDialog		(SessionRef		inRef)
{
	OSStatus			error = noErr;
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	PMPrintSession		printSession = nullptr;
	
	
	error = PMCreateSession(&printSession);
	if (noErr == error)
	{
		static PMSheetDoneUPP	gMyPageSetupDoneUPP = NewPMSheetDoneUPP(pageSetupCloseNotifyProc);
		PMPageFormat			pageFormat = gSessionPageFormat();
		
		
		assert(nullptr != gMyPageSetupDoneUPP);
		error = PMSessionDefaultPageFormat(printSession, pageFormat);
		if (noErr == error)
		{
			// ??? necessary to validate a defaulted format ???
			error = PMSessionValidatePageFormat(printSession, pageFormat, kPMDontWantBoolean/* result */);
			if (noErr == error)
			{
				HIWindowRef		window = Session_ReturnActiveWindow(inRef);
				
				
				assert(nullptr != window);
				error = PMSessionUseSheets(printSession, window, gMyPageSetupDoneUPP);
				if (noErr == error)
				{
					Boolean		meaninglessAcceptedFlag = false;
					
					
					// since sheets are used, this is not a modal dialog and
					// the "meaninglessAcceptedFlag" is, well, meaningless!
					error = PMSessionPageSetupDialog(printSession, pageFormat, &meaninglessAcceptedFlag);
				}
			}
		}
	}
	
	if (noErr != error)
	{
		// ONLY release for errors; otherwise, the sheet callback does this
		PMRelease(printSession), printSession = nullptr;
	}
	
	Alert_ReportOSStatus(error);	
}// DisplayPrintPageSetupDialog


/*!
Displays the Save dialog for the given terminal window,
handling the user’s response automatically.  The dialog
could be a sheet, so this routine may return immediately
without the user having responded to the save; the save
(or lack thereof) is handled automatically, asynchronously,
in that case.

(3.0)
*/
void
Session_DisplaySaveDialog	(SessionRef		inRef)
{
	NavDialogCreationOptions	dialogOptions;
	NavDialogRef				navigationServicesDialog = nullptr;
	OSStatus					error = noErr;
	
	
	error = NavGetDefaultDialogCreationOptions(&dialogOptions);
	if (noErr == error)
	{
		// this call sets most of the options up front (parent window, etc.)
		configureSaveDialog(inRef, dialogOptions);
		
		(UIStrings_Result)UIStrings_Copy(kUIStrings_FileDefaultSession, dialogOptions.saveFileName);
		(UIStrings_Result)UIStrings_Copy(kUIStrings_SystemDialogPromptSaveSession, dialogOptions.message);
		dialogOptions.modality = kWindowModalityWindowModal;
	}
	error = NavCreatePutFileDialog(&dialogOptions, kApplicationFileTypeSessionDescription,
									AppResources_ReturnCreatorCode(),
									NewNavEventUPP(navigationSaveDialogEvent),
									inRef/* client data */, &navigationServicesDialog);
	
	// display the dialog; it is a sheet, so this will return immediately
	// and the dialog will close whenever the user is actually done with it
	error = NavDialogRun(navigationServicesDialog);
}// DisplaySaveDialog


/*!
Displays the Special Key Sequences dialog for the specified
session, handling the user’s response automatically.  The
dialog could be a sheet, so this routine may return immediately
without the user having finished; any updates will therefore
be handled automatically, asynchronously, in that case.

(3.1)
*/
void
Session_DisplaySpecialKeySequencesDialog	(SessionRef		inRef)
{
	SpecialKeySequencesDialog_Ref	dialog = nullptr;
	
	
	// display the sheet
	dialog = SpecialKeySequencesDialog_New(inRef, SpecialKeySequencesDialog_StandardCloseNotifyProc);
	SpecialKeySequencesDialog_Display(dialog); // automatically disposed when the user clicks a button
}// DisplaySpecialKeySequencesDialog


/*!
Displays the Close warning alert for the given terminal
window, handling the user’s response automatically.  On
Mac OS X, the dialog is a sheet (unless "inForceModalDialog"
is true), so this routine may return immediately without
the user having responded to the warning; the session
termination (or lack thereof) is handled automatically,
asynchronously, in that case.

Although the warning dialog is handled completely (e.g.
session is automatically terminated if the user says so),
you may still wish to find out when the user has responded
to the warning - to handle a Close All or Quit command, for
example.  To receive notification when a button is clicked
in the warning alert or sheet, listen for the
"kSession_ChangeCloseWarningAnswered" event, with the
Session_StartMonitoring() routine.

If you set "inForceModalDialog" to true, which is only
relevant on Mac OS X, you can force the warning to be a
modal dialog as opposed to a sheet.  In that case, this
routine is guaranteed to only return when the dialog has
been dismissed.

(3.0)
*/
void
Session_DisplayTerminationWarning	(SessionRef		inRef,
									 Boolean		UNUSED_ARGUMENT_CLASSIC(inForceModalDialog))
{
	SessionAutoLocker		ptr(gSessionPtrLocks(), inRef);
	HIWindowRef				window = Session_ReturnActiveWindow(inRef);
	TerminateAlertInfoPtr	terminateAlertInfoPtr = REINTERPRET_CAST(Memory_NewPtr(sizeof(TerminateAlertInfo)),
																		TerminateAlertInfoPtr);
	Rect					originalStructureBounds;
	Rect					centeredStructureBounds;
	OSStatus				error = noErr;
	Boolean					isModal = inForceModalDialog;
	
	
	// TEMPORARY - this should really take into account whether the quit event is interactive
	error = GetWindowBounds(window, kWindowStructureRgn, &originalStructureBounds);
	if (noErr == error)
	{
		// for modal dialogs, first move the window to the center of the
		// screen (so the user can see which window is being referred to);
		// also remember its original location, in case the user cancels;
		// but quell the animation if this window should be closed without
		// warning (so-called active unstable) or if only one window is
		// open that would cause an alert to be displayed
		if ((inForceModalDialog) &&
			(false == Session_StateIsActiveUnstable(inRef)) &&
			((SessionFactory_ReturnCount() - SessionFactory_ReturnStateCount(kSession_StateActiveUnstable)) > 1))
		{
			SInt16 const	kOffsetFromCenterV = -130; // in pixels; arbitrary
			SInt16 const	kAbsoluteMinimumV = 30; // in pixels; arbitrary
			Rect			availablePositioningBounds;
			
			
			// center the window on the screen, but slightly offset toward the top half;
			// do not allow the window to go off of the screen, however
			RegionUtilities_GetPositioningBounds(window, &availablePositioningBounds);
			centeredStructureBounds = originalStructureBounds;
			CenterRectIn(&centeredStructureBounds, &availablePositioningBounds);
			centeredStructureBounds.top += kOffsetFromCenterV;
			centeredStructureBounds.bottom += kOffsetFromCenterV;
			if (centeredStructureBounds.top < kAbsoluteMinimumV)
			{
				SInt16 const	kWindowHeight = centeredStructureBounds.bottom - centeredStructureBounds.top;
				
				
				centeredStructureBounds.top = kAbsoluteMinimumV;
				centeredStructureBounds.bottom = centeredStructureBounds.top + kWindowHeight;
			}
			error = TransitionWindow(window, kWindowSlideTransitionEffect, kWindowMoveTransitionAction,
										&centeredStructureBounds);
		}
	}
	
	// if a warning alert is already showing (presumably because
	// a sheet is open and now a modal dialog is being displayed),
	// first Cancel the open sheet before displaying another one
	if (ptr->currentTerminationAlert != nullptr)
	{
		Alert_HitItem(ptr->currentTerminationAlert, kAlertStdAlertCancelButton);
	}
	
	terminateAlertInfoPtr->sessionBeingTerminated = inRef;
	
	ptr->currentTerminationAlert = Alert_New();
	Alert_SetParamsFor(ptr->currentTerminationAlert, kAlert_StyleOKCancel);
	Alert_SetType(ptr->currentTerminationAlert, kAlertCautionAlert);
	
	// set message
	{
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		CFStringRef			primaryTextCFString = nullptr;
		
		
		stringResult = UIStrings_Copy(kUIStrings_AlertWindowClosePrimaryText, primaryTextCFString);
		if (stringResult.ok())
		{
			CFStringRef		helpTextCFString = nullptr;
			
			
			stringResult = UIStrings_Copy(kUIStrings_AlertWindowCloseHelpText, helpTextCFString);
			if (stringResult.ok())
			{
				Alert_SetTextCFStrings(ptr->currentTerminationAlert, primaryTextCFString, helpTextCFString);
				CFRelease(helpTextCFString);
			}
			CFRelease(primaryTextCFString);
		}
	}
	// set title
	{
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		CFStringRef			titleCFString = nullptr;
		
		
		stringResult = UIStrings_Copy(kUIStrings_AlertWindowCloseName, titleCFString);
		if (stringResult == kUIStrings_ResultOK)
		{
			Alert_SetTitleCFString(ptr->currentTerminationAlert, titleCFString);
			CFRelease(titleCFString);
		}
	}
	// set buttons
	{
		UIStrings_Result	stringResult = kUIStrings_ResultOK;
		CFStringRef			buttonTitleCFString = nullptr;
		
		
		stringResult = UIStrings_Copy(kUIStrings_ButtonClose, buttonTitleCFString);
		if (stringResult == kUIStrings_ResultOK)
		{
			Alert_SetButtonText(ptr->currentTerminationAlert, kAlertStdAlertOKButton, buttonTitleCFString);
			CFRelease(buttonTitleCFString);
		}
	}
	
	//if (Session_TimeSinceConnected(inRef) <= kSession_LifetimeMinimumForNoWarningClose)
	if (Session_StateIsActiveUnstable(inRef))
	{
		// the connection JUST opened, so kill it without confirmation
		terminationWarningCloseNotifyProc(ptr->currentTerminationAlert, kAlertStdAlertOKButton,
											terminateAlertInfoPtr/* user data */);
	}
	else
	{
		Boolean		willLeaveTerminalWindowOpen = false;
		size_t		actualSize = 0;
		
		
		unless (isModal)
		{
			// under Mac OS X, the sheet should close immediately if the
			// user clicks the Close button and the terminal window will
			// disappear immediately; however, if the window is going to
			// remain on the screen, the sheet must close normally; so,
			// here the preference value is checked so that the sheet
			// can behave properly
			unless (Preferences_GetData(kPreferences_TagDontAutoClose, sizeof(willLeaveTerminalWindowOpen),
										&willLeaveTerminalWindowOpen, &actualSize) ==
					kPreferences_ResultOK)
			{
				willLeaveTerminalWindowOpen = false; // assume windows automatically close, if preference can’t be found
			}
		}
		
		// apply the alert status attribute to the session; this may,
		// for instance, affect session status icon displays
		changeStateAttributes(ptr, kSession_StateAttributeOpenDialog/* attributes to set */,
								0/* attributes to clear */);
		
		// display the confirmation alert; depending on the kind of
		// alert (sheet or modal dialog), the following code may
		// return immediately without session termination (yet);
		// ensure that the relevant window is visible and frontmost
		// when the message appears
		ShowWindow(ptr->dataPtr->window);
		EventLoop_SelectBehindDialogWindows(ptr->dataPtr->window);
		if (isModal)
		{
			Alert_Display(ptr->currentTerminationAlert);
			if (Alert_ItemHit(ptr->currentTerminationAlert) == kAlertStdAlertCancelButton)
			{
				// in the event that the user cancelled and the window
				// was transitioned to the screen center, “un-transition”
				// the most recent window back to its original location -
				// unless of course the user has since moved the window
				Rect	currentStructureBounds;
				
				
				(OSStatus)GetWindowBounds(window, kWindowStructureRgn, &currentStructureBounds);
				if (EqualRect(&currentStructureBounds, &centeredStructureBounds))
				{
					if (FlagManager_Test(kFlagOS10_3API))
					{
						// on 10.3 and beyond, an asynchronous transition can occur
						HIRect						floatBounds = CGRectMake(originalStructureBounds.left, originalStructureBounds.top,
																				originalStructureBounds.right - originalStructureBounds.left,
																				originalStructureBounds.bottom - originalStructureBounds.top);
						TransitionWindowOptions		transitionOptions;
						
						
						bzero(&transitionOptions, sizeof(transitionOptions));
						transitionOptions.version = 0;
						(OSStatus)TransitionWindowWithOptions(window, kWindowSlideTransitionEffect, kWindowMoveTransitionAction,
																&floatBounds, true/* asynchronous */, &transitionOptions);
					}
					else
					{
						(OSStatus)TransitionWindow(window, kWindowSlideTransitionEffect, kWindowMoveTransitionAction,
													&originalStructureBounds);
					}
				}
			}
			terminationWarningCloseNotifyProc(ptr->currentTerminationAlert, Alert_ItemHit(ptr->currentTerminationAlert),
												terminateAlertInfoPtr/* user data */);
		}
		else
		{
			Alert_MakeWindowModal(ptr->currentTerminationAlert, ptr->dataPtr->window/* parent */, !willLeaveTerminalWindowOpen/* is window close alert */,
									terminationWarningCloseNotifyProc, terminateAlertInfoPtr/* user data */);
			Alert_Display(ptr->currentTerminationAlert); // notifier disposes the alert when the sheet eventually closes
		}
	}
}// DisplaySessionTerminationWarning


/*!
Determines the port number within a string
that may contain a host name and optionally a
port number afterwards (à la New Sessions
dialog box).  The port number can be tagged
on either with white space or a colon.

If the parse succeeds (that is, if the output
port number is valid), "true" is returned;
otherwise, "false" is returned.  On output,
the string given is modified to include only
the host name (minus any port number).

(2.6)
*/
Boolean
Session_ExtractPortNumberFromHostString		(StringPtr		inoutString,
											 SInt16*		outPortNumberPtr)
{
	Str255				string;
	register SInt16		i = 0;
	StringPtr			xxxxptr = nullptr;
	StringPtr			yyyyptr = nullptr;
	Boolean				result = false;
	
	
	// copy the string
	BlockMoveData(inoutString, string, 255 * sizeof(UInt8));
	
	// remove leading spaces
	for (i = 1; ((i <= PLstrlen(string)) && (string[i] == ' ')); i++) {}
	
	if (i > PLstrlen(string))
	{
		inoutString[0] = 0;
		result = false;
	}
	else
	{
		SInt32		portRequested = 0L;
		
		
		xxxxptr = &string[i - 1];
		
		//	Now look for a port number...
		while ((i <= PLstrlen(string)) && (string[i] != ' ') && (string[i] != ':')) i++;
		
		yyyyptr = &string[i];
		
		if (i < PLstrlen(string))
		{
			string[i] = PLstrlen(string) - i;
			StringToNum(&string[i], &portRequested);
			if ((portRequested > 0) && (portRequested < 65535))	result = true;
		}
		
		xxxxptr[0] = yyyyptr - xxxxptr - 1;
		
		// copy parsed host name string back
		BlockMoveData(xxxxptr, inoutString, (PLstrlen(xxxxptr) + 1) * sizeof(UInt8));
		
		*outPortNumberPtr = (SInt16)portRequested;
	}
	
	return result;
}// ExtractPortNumberFromHostString


/*!
Copies as much configuration information as possible
from the specified session, and returns a new
Session Description containing all the data.  You
can use this to save a session to a file.

\retval kSession_ResultOK
if there are no errors

\retval kSession_ResultInvalidReference
if the specified session is unrecognized

\retval kSession_ResultParameterError
if "outNewSaveFileMemoryModelPtr" is nullptr

(3.0)
*/
Session_Result
Session_FillInSessionDescription	(SessionRef					inRef,
									 SessionDescription_Ref*	outNewSaveFileMemoryModelPtr)
{
	Session_Result		result = kSession_ResultOK;
	
	
	if (outNewSaveFileMemoryModelPtr == nullptr) result = kSession_ResultParameterError;
	else
	{
		// now write to the file, using an in-memory model first
		SessionDescription_Result	saveError = kSessionDescription_ResultOK;
		SessionDescription_Ref		saveFileMemoryModel = SessionDescription_New
															(kSessionDescription_ContentTypeCommand);
		
		
		*outNewSaveFileMemoryModelPtr = saveFileMemoryModel;
		if (saveFileMemoryModel != nullptr)
		{
			SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
			TerminalWindowRef	terminalWindow = Session_ReturnActiveTerminalWindow(inRef);
			TerminalViewRef		view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
			
			
			// write all information into memory
			{
				CFStringRef		stringValue = nullptr;
				RGBColor		colorValue;
				OSStatus		error = noErr;
				
				
				// window title
				error = CopyWindowTitleAsCFString(Session_ReturnActiveWindow(inRef), &stringValue);
				if (error == noErr)
				{
					saveError = SessionDescription_SetStringData
								(saveFileMemoryModel, kSessionDescription_StringTypeWindowName, stringValue);
					CFRelease(stringValue), stringValue = nullptr;
				}
				
				// command info
				saveError = SessionDescription_SetStringData
								(saveFileMemoryModel, kSessionDescription_StringTypeCommandLine,
									ptr->resourceLocationString.returnCFStringRef());
				
				// font info
				{
					Str255		fontName;
					UInt16		fontSize = 0;
					
					
					TerminalView_GetFontAndSize(view, fontName, &fontSize);
					stringValue = CFStringCreateWithPascalString(kCFAllocatorDefault, fontName,
																	GetApplicationTextEncoding());
					if (stringValue != nullptr)
					{
						saveError = SessionDescription_SetStringData
									(saveFileMemoryModel, kSessionDescription_StringTypeTerminalFont, stringValue);
						CFRelease(stringValue), stringValue = nullptr;
					}
					saveError = SessionDescription_SetIntegerData
								(saveFileMemoryModel, kSessionDescription_IntegerTypeTerminalFontSize, fontSize);
				}
				
				// color info
				{
					TerminalView_GetColor(view, kTerminalView_ColorIndexNormalText, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeTextNormal, colorValue);
					
					TerminalView_GetColor(view, kTerminalView_ColorIndexNormalBackground, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeBackgroundNormal, colorValue);
					
					TerminalView_GetColor(view, kTerminalView_ColorIndexBoldText, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeTextBold, colorValue);
					
					TerminalView_GetColor(view, kTerminalView_ColorIndexBoldBackground, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeBackgroundBold, colorValue);
					
					TerminalView_GetColor(view, kTerminalView_ColorIndexBlinkingText, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeTextBlinking, colorValue);
					
					TerminalView_GetColor(view, kTerminalView_ColorIndexBlinkingBackground, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeBackgroundBlinking, colorValue);
				}
				
				// terminal screen metrics
				{
					TerminalScreenRef	screen = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
					UInt16				columns = Terminal_ReturnColumnCount(screen);
					UInt16				rows = Terminal_ReturnRowCount(screen);
					UInt16				scrollback = Terminal_ReturnInvisibleRowCount(screen);
					
					
					saveError = SessionDescription_SetIntegerData
								(saveFileMemoryModel, kSessionDescription_IntegerTypeTerminalVisibleColumnCount, columns);
					saveError = SessionDescription_SetIntegerData
								(saveFileMemoryModel, kSessionDescription_IntegerTypeTerminalVisibleLineCount, rows);
					saveError = SessionDescription_SetIntegerData
								(saveFileMemoryModel, kSessionDescription_IntegerTypeScrollbackBufferLineCount, scrollback);
				}
				
				// terminal type
				{
					Session_TerminalCopyAnswerBackMessage(inRef, stringValue);
					saveError = SessionDescription_SetStringData
								(saveFileMemoryModel, kSessionDescription_StringTypeAnswerBack, stringValue);
					CFRelease(stringValue), stringValue = nullptr;
				}
				
				// keyboard mapping info
				{
					Boolean		flag = false;
					
					
					flag = Session_ConnectionDataPtr(inRef)->crmap;
					saveError = SessionDescription_SetBooleanData
								(saveFileMemoryModel, kSessionDescription_BooleanTypeRemapCR, flag);
					flag = !Session_PageKeysControlTerminalView(inRef);
					saveError = SessionDescription_SetBooleanData
								(saveFileMemoryModel, kSessionDescription_BooleanTypePageKeysDoNotControlTerminal, flag);
					flag = Session_ConnectionDataPtr(inRef)->keypadmap;
					saveError = SessionDescription_SetBooleanData
								(saveFileMemoryModel, kSessionDescription_BooleanTypeRemapKeypadTopRow, flag);
				}
				
				// TEK info
				{
					Boolean		flag = false;
					
					
					flag = !Session_TEKPageCommandOpensNewWindow(inRef);
					saveError = SessionDescription_SetBooleanData
								(saveFileMemoryModel, kSessionDescription_BooleanTypeTEKPageClears, flag);
				}
				
				// Kerberos info: no longer supported
				
				// toolbar info
				{
					// TEMPORARY; should respect all possible toolbar view options (see "SessionDescription.h")
					//stringValue = TerminalWindow_ToolbarIsVisible(ptr->terminalWindow) ? CFSTR("icon+text") : CFSTR("hidden");
					stringValue = CFSTR("icon+text");
					saveError = SessionDescription_SetStringData
								(saveFileMemoryModel, kSessionDescription_StringTypeToolbarInfo, stringValue);
				}
			}
		}
	}
	
	return result;
}// FillInSessionDescription


/*!
Jump-scrolls the specified session’s terminal screen,
showing as much data as there is available.

(3.0)
*/
void
Session_FlushNetwork	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	register SInt16		remainingBytesCount = 0;
	
	
	TerminalView_SetDrawingEnabled(TerminalWindow_ReturnViewWithFocus(Session_ReturnActiveTerminalWindow(inRef)),
									false); // no output
	remainingBytesCount = 1; // just needs to be positive to start with
	while (remainingBytesCount > 0) remainingBytesCount = Session_ProcessMoreData(inRef);
	TerminalView_SetDrawingEnabled(TerminalWindow_ReturnViewWithFocus(Session_ReturnActiveTerminalWindow(inRef)),
									true); // output now
	RegionUtilities_RedrawWindowOnNextUpdate(ptr->dataPtr->window);
}// FlushNetwork


/*!
Clears out any buffered keyboard input from the user,
sending it to the session.

(3.0)
*/
void
Session_FlushUserInputBuffer	(SessionRef		inRef)
{
	SInt16*		kblenPtr = nullptr;
	
	
	kblenPtr = Session_kblen(inRef);
	if (*kblenPtr > 0)
	{
		// flush buffer
		Session_SendData(inRef, Session_kbbuf(inRef), *kblenPtr);
		*kblenPtr = 0;
	}
}// FlushUserInputBuffer


/*!
Returns a succinct string representation of the
specified session’s state.  This is localized so
it can be displayed in user interface elements.

(3.1)
*/
Session_Result
Session_GetStateString		(SessionRef		inRef,
							 CFStringRef&	outUncopiedString)
{
	Session_Result		result = kSession_ResultOK;
	
	
	if (inRef == nullptr) result = kSession_ResultInvalidReference;
	else
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		outUncopiedString = ptr->statusString.returnCFStringRef();
	}
	
	return result;
}// GetStateString


/*!
Returns the most recent user-specified window title;
defaults to the one given in the New Sessions dialog,
but can be updated whenever the user invokes the
“Rename…” menu command.

If the title has never been set, nullptr may be returned.

A copy of the internal string is NOT made; retain it if
you need to.

\retval kSession_ResultOK
if there are no errors

\retval kSession_ResultInvalidReference
if the specified session is unrecognized

(3.0)
*/
Session_Result
Session_GetWindowUserDefinedTitle	(SessionRef		inRef,
									 CFStringRef&	outUncopiedString)
{
	Session_Result		result = kSession_ResultOK;
	
	
	if (inRef == nullptr) result = kSession_ResultInvalidReference;
	else
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		outUncopiedString = ptr->dataPtr->alternateTitle.returnCFStringRef();
	}
	
	return result;
}// GetWindowUserDefinedTitle


/*!
Returns "true" only if the specified session is not
allowing user input.

IMPORTANT:	This is ONLY a flag.  It is not enforced,
			for efficiency reasons.

(3.0)
*/
Boolean
Session_IsReadOnly	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = isReadOnly(ptr);
	
	
	return result;
}// IsReadOnly


/*!
Returns a pointer to the "kbbuf" data in the internal
structure.

TEMPORARY, TRANSITIONAL FUNCTION THAT *WILL* DISAPPEAR!!!

(3.0)
*/
char*
Session_kbbuf		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	char*				result = ptr->dataPtr->kbbuf;
	
	
	return result;
}// kbbuf


/*!
Returns a pointer to the "kblen" data in the internal
structure.

TEMPORARY, TRANSITIONAL FUNCTION THAT *WILL* DISAPPEAR!!!

(3.0)
*/
SInt16*
Session_kblen		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	SInt16*				result = &ptr->dataPtr->kblen;
	
	
	return result;
}// kblen


/*!
Returns "true" only if local echoing is enabled.  Local
echoing means that characters are parsed locally prior
to being sent to the server.  For slow network connections,
it may be too sluggish to type and wait for the server to
echo back characters.

(3.0)
*/
Boolean
Session_LocalEchoIsEnabled		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (ptr->dataPtr->echo != 0);
	
	
	return result;
}// LocalEchoIsEnabled


/*!
Returns "true" only if local echo is set to full-duplex
mode for the given session.  Full duplex means that data
is locally echoed but not sent to the server until a
return key or control key is pressed.

(3.0)
*/
Boolean
Session_LocalEchoIsFullDuplex	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (ptr->dataPtr->halfdup == 0);
	
	
	return result;
}// LocalEchoIsFullDuplex


/*!
Returns "true" only if local echo is set to half-duplex
mode for the given session.  Half duplex means that data
is locally echoed and immediately sent to the server.

(3.0)
*/
Boolean
Session_LocalEchoIsHalfDuplex	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (ptr->dataPtr->halfdup != 0);
	
	
	return result;
}// LocalEchoIsHalfDuplex


/*!
Returns "true" only if the specified connection
is disabled - that is, its network activity has
not been suspended by a Scroll Lock.

(3.0)
*/
Boolean
Session_NetworkIsSuspended		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = !ptr->dataPtr->enabled;
	
	
	return result;
}// NetworkIsSuspended


/*!
Returns "true" only if the Page Up, Page Down,
and similar keys control the local terminal
instead of causing commands to be sent to the
session’s process (local or remote).

(3.0)
*/
Boolean
Session_PageKeysControlTerminalView		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = !ptr->dataPtr->pgupdwn;
	
	
	return result;
}// PageKeysControlTerminalView


/*!
Returns a pointer to the "parsedat" data in the internal
structure.

TEMPORARY, TRANSITIONAL FUNCTION THAT *WILL* DISAPPEAR!!!

(3.0)
*/
UInt8*
Session_parsedat		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	UInt8*				result = ptr->dataPtr->parsedat;
	
	
	return result;
}// parsedat


/*!
Returns the size of the internal "parsedat" array in the
internal structure.

TEMPORARY, TRANSITIONAL FUNCTION THAT *WILL* DISAPPEAR!!!

(3.0)
*/
size_t
Session_parsedat_size	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	size_t				result = sizeof(ptr->dataPtr->parsedat);
	
	
	return result;
}// parsedat_size


/*!
Returns a pointer to the "parseIndex" data in the internal
structure.

TEMPORARY, TRANSITIONAL FUNCTION THAT *WILL* DISAPPEAR!!!

(3.0)
*/
SInt16*
Session_parseIndex		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	SInt16*				result = &ptr->dataPtr->parseIndex;
	
	
	return result;
}// parseIndex


/*!
Creates a "kMyCarbonEventKindSessionDataArrived" event
from class "kMyCarbonEventClassSession" and sends it
to the main queue.  Use this to report input from an
external source (such as a running process or the user’s
keyboard) that belongs to the specified session.  The
session, in turn, will process the data appropriately
and in the proper sequence if multiple events exist.

(3.0)
*/
Session_Result
Session_PostDataArrivedEventToMainQueue		(SessionRef		inRef,
											 void*			inBuffer,
											 UInt32			inNumberOfBytesToProcess,
											 EventPriority	inPriority,
											 EventQueueRef	inDispatcherQueue)
{
	Session_Result		result = kSession_ResultParameterError;
	
	
	if (inRef == nullptr) result = kSession_ResultInvalidReference;
	else
	{
		EventRef	dataArrivedEvent = nullptr;
		OSStatus	error = noErr;
		
		
		// create a Carbon Event
		error = CreateEvent(nullptr/* allocator */, kEventClassNetEvents_Session,
							kEventNetEvents_SessionDataArrived,
							GetCurrentEventTime(), kEventAttributeNone, &dataArrivedEvent);
		if (error == noErr)
		{
			// specify the event queue that should receive a reply; set this to
			// the current thread’s event queue, otherwise it is not possible
			// to block this thread until the data processing is finished
			// (see below for more)
			error = SetEventParameter(dataArrivedEvent, kEventParamNetEvents_DispatcherQueue,
										typeNetEvents_EventQueueRef, sizeof(inDispatcherQueue),
										&inDispatcherQueue);
			if (error == noErr)
			{
				// specify the session that has new data to process
				error = SetEventParameter(dataArrivedEvent, kEventParamNetEvents_DirectSession,
											typeNetEvents_SessionRef, sizeof(inRef), &inRef);
				if (error == noErr)
				{
					// specify the data to process; for efficiency, the pointer
					// is passed instead of copying the data, etc. which means
					// the buffer must remain valid until the handler returns
					error = SetEventParameter(dataArrivedEvent, kEventParamNetEvents_SessionData,
												typeVoidPtr, sizeof(inBuffer), &inBuffer);
					if (error == noErr)
					{
						// specify the size of the data to process
						error = SetEventParameter(dataArrivedEvent, kEventParamNetEvents_SessionDataSize,
													typeUInt32, sizeof(inNumberOfBytesToProcess),
													&inNumberOfBytesToProcess);
						if (error == noErr)
						{
							// send the message to the main thread
							error = PostEventToQueue(GetMainEventQueue(), dataArrivedEvent, inPriority);
							if (error == noErr)
							{
								// “data arrived” event successfully queued
								result = kSession_ResultOK;
							}
						}
					}
				}
			}
		}
		if (dataArrivedEvent != nullptr) ReleaseEvent(dataArrivedEvent), dataArrivedEvent = nullptr;
	}
	return result;
}// PostDataArrivedEventToQueue


/*!
Returns whether the specified session processes all
eight bits of the bytes it receives, instead of the
low seven bits.

(3.0)
*/
Boolean
Session_ProcessesAll8Bits		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = ptr->dataPtr->eightbit;
	
	
	return result;
}// ProcessesAll8Bits


/*!
Reads as much available data as possible (based on
the size of the processing buffer) and processes it,
which will result in either displaying it as text or
interpreting it as one or more commands.  The number
of bytes left to be processed is returned, so if
the result is greater than zero you ought to call
this routine again.

You can change the buffer size using the method
Session_SetDataProcessingCapacity().

(3.0)
*/
size_t
Session_ProcessMoreData		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	size_t				result = 0;
	
	
	// local session data on Mac OS X is handled in a separate
	// thread using the blocking read() system call, so in that
	// case data is expected to have been placed in the read buffer
	// already by that thread
	Session_ReceiveData(ptr->selfRef, ptr->readBufferPtr, ptr->readBufferSizeInUse);
	//Session_TerminalWrite(ptr->selfRef, ptr->readBufferPtr, ptr->readBufferSizeInUse);
	ptr->readBufferSizeInUse = 0;
	return result;
}// ProcessMoreData


/*!
Attaches the specified data to the given session, such
that the given tag can be used to retrieve the data
later.  If any data was previously attached to the
given session using the same tag, that data will be
OVERWRITTEN with the data you specify now; the caller
is responsible for ensuring the uniqueness of the
tags used.

(3.0)
*/
void
Session_PropertyAdd		(SessionRef				inRef,
						 Session_PropertyKey	inAuxiliaryDataTag,
						 void*					inAuxiliaryData)
{
	if (inRef != nullptr)
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		ptr->auxiliaryDataMap[inAuxiliaryDataTag] = inAuxiliaryData;
	}
}// PropertyAdd


/*!
Returns the data associated with the specified
session and tag.  The data will be nullptr if the
tag is invalid.

The type of the data, as well as the size of
the buffer referenced by the returned pointer,
is defined when Session_PropertyAdd() is
invoked.

(3.0)
*/
void
Session_PropertyLookUp	(SessionRef				inRef,
						 Session_PropertyKey	inAuxiliaryDataTag,
						 void**					outAuxiliaryDataPtr)
{
	if (inRef != nullptr)
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		if (ptr->auxiliaryDataMap.find(inAuxiliaryDataTag) != ptr->auxiliaryDataMap.end())
		{
			// tag was found - return the data
			*outAuxiliaryDataPtr = ptr->auxiliaryDataMap[inAuxiliaryDataTag];
		}
	}
}// PropertyLookUp


/*!
Detaches the specified data from the given session, such
that the given tag can no longer be used to retrieve the
data later.  Whatever data is associated with the given
tag is returned; this will be 0 (nullptr) if the tag is
invalid.

(3.0)
*/
void*
Session_PropertyRemove	(SessionRef				inRef,
						 Session_PropertyKey	inAuxiliaryDataTag)
{
	void*	result = nullptr;
	
	
	if (inRef != nullptr)
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		// erase the specified data from the internal map, and return it;
		// note that the [] operator of a standard map will actually create
		// an item if it doesn’t exist; that’s okay, this just means the
		// item will be deleted immediately and the return value will be 0
		result = ptr->auxiliaryDataMap[inAuxiliaryDataTag];
		ptr->auxiliaryDataMap.erase(ptr->auxiliaryDataMap.find(inAuxiliaryDataTag));
	}
	
	return result;
}// PropertyRemove


/*!
Provides the specified data to all targets currently
active in the given session; the targets react in
whatever way is appropriate.  Currently, a target
can be a terminal (VT or DUMB), a capture file, a
TEK graphics window, or an Interactive Color Raster
graphics device.

See the documentation on Session_DataTarget for
more information on session data targets.

\retval kSession_ResultOK
always; currently, no other errors are defined

(3.0)
*/
Session_Result
Session_ReceiveData		(SessionRef		inRef,
						 void const*	inBufferPtr,
						 size_t			inByteCount)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_Result		result = kSession_ResultOK;
	
	
	// “carbon copy” the data to all active attached targets
	{
		UInt8 const* const	kBuffer = REINTERPRET_CAST(inBufferPtr, UInt8 const*);
		
		
		// dumb terminals are considered compatible with any kind of data and always receive data
		std::for_each(ptr->targetDumbTerminals.begin(), ptr->targetDumbTerminals.end(),
						terminalDumbDataWriter(kBuffer, inByteCount));
		
		// if any TEK canvases are installed, they take precedence
		if (ptr->targetVectorGraphics.empty())
		{
			// ICR windows are lower precedence than TEK, but still take
			// precedence over any installed standard terminals or files
			if (ptr->targetRasterGraphicsScreens.empty())
			{
				// terminals, capture files and print jobs are considered
				// compatible; write data to all types of targets at the
				// same time, if any are attached
				std::for_each(ptr->targetTerminals.begin(), ptr->targetTerminals.end(), terminalDataWriter(kBuffer, inByteCount));
				std::for_each(ptr->targetFiles.begin(), ptr->targetFiles.end(), captureFileDataWriter(kBuffer, inByteCount));
				std::for_each(ptr->targetPrintJobs.begin(), ptr->targetPrintJobs.end(), printJobDataWriter(kBuffer, inByteCount));
			}
			else
			{
				// write to all attached ICR windows
				std::for_each(ptr->targetRasterGraphicsScreens.begin(), ptr->targetRasterGraphicsScreens.end(),
								rasterGraphicsDataWriter(kBuffer, inByteCount));
			}
		}
		else
		{
			// write to all attached TEK windows
			std::for_each(ptr->targetVectorGraphics.begin(), ptr->targetVectorGraphics.end(),
							vectorGraphicsDataWriter(kBuffer, inByteCount));
		}
	}
	return result;
}// ReceiveData


/*!
Causes the specified target to no longer be considered for
writes to the given session.

If any “incompatible” types were disabled when the given
target was added previously, they will be re-enabled now
if possible.  Targets are re-enabled only if they are
higher in precedence than other installed targets - for
example, no terminals will be re-enabled if there are still
TEK graphics devices attached.

\retval kSession_ResultOK
if the target was added successfully

\retval kSession_ResultInvalidReference
if "inRef" is invalid

\retval kSession_ResultParameterError
if "inTarget" or "inTargetData" are invalid

(3.0)
*/
Session_Result
Session_RemoveDataTarget	(SessionRef				inRef,
							 Session_DataTarget		inTarget,
							 void*					inTargetData)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_Result		result = kSession_ResultOK;
	
	
	switch (inTarget)
	{
	case kSession_DataTargetStandardTerminal:
		ptr->targetTerminals.erase(std::remove(ptr->targetTerminals.begin(), ptr->targetTerminals.end(),
												REINTERPRET_CAST(inTargetData, TerminalScreenRef)),
									ptr->targetTerminals.end());
		break;
	
	case kSession_DataTargetDumbTerminal:
		ptr->targetDumbTerminals.erase(std::remove(ptr->targetDumbTerminals.begin(), ptr->targetDumbTerminals.end(),
													REINTERPRET_CAST(inTargetData, TerminalScreenRef)),
										ptr->targetDumbTerminals.end());
		break;
	
	case kSession_DataTargetTektronixGraphicsCanvas:
		ptr->targetVectorGraphics.erase(std::remove(ptr->targetVectorGraphics.begin(), ptr->targetVectorGraphics.end(),
													*(REINTERPRET_CAST(inTargetData, SInt16*/* TEK graphic ID */))),
										ptr->targetVectorGraphics.end());
		break;
	
	case kSession_DataTargetInteractiveColorRasterGraphicsScreen:
		ptr->targetRasterGraphicsScreens.erase(std::remove(ptr->targetRasterGraphicsScreens.begin(),
															ptr->targetRasterGraphicsScreens.end(),
															*(REINTERPRET_CAST(inTargetData, SInt16*/* ICR window ID */))),
												ptr->targetRasterGraphicsScreens.end());
		break;
	
	case kSession_DataTargetOpenCaptureFile:
		ptr->targetFiles.erase(std::remove(ptr->targetFiles.begin(), ptr->targetFiles.end(),
											REINTERPRET_CAST(inTargetData, TerminalScreenRef)),
								ptr->targetFiles.end());
		break;
	
	case kSession_DataTargetOpenPrinterSpool:
		ptr->targetFiles.erase(std::remove(ptr->targetPrintJobs.begin(), ptr->targetPrintJobs.end(),
											REINTERPRET_CAST(inTargetData, TerminalScreenRef)),
								ptr->targetPrintJobs.end());
		break;
	
	default:
		// ???
		result = kSession_ResultParameterError;
		break;
	}
	return result;
}// RemoveDataTarget


/*!
Returns the most recently active Terminal Window
associated with the specified session, or nullptr
if it cannot be found.  A window will be returned
as long as any window from the session is open; it
does not have to be the current user focus.

It is entirely possible for a valid session to have
no terminal window.  For example, the terminal
window may have been hidden by the user, or the
session may be new.  The "kSession_ChangeWindowValid"
and "kSession_ChangeWindowInvalid" change notifications
can be useful for determining when a session has no
valid terminal window.

Currently, a session can have at most one terminal
window, but in the future it may be possible for
more than one window to belong to a session.

(3.0)
*/
TerminalWindowRef
Session_ReturnActiveTerminalWindow	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	TerminalWindowRef	result = ptr->terminalWindow;
	
	
	return result;
}// ReturnActiveTerminalWindow


/*!
Returns the most recently active window associated
with the specified session, or nullptr if it cannot
be found.

This follows the same logic, and therefore has the
same caveats, as Session_ReturnActiveTerminalWindow().
See documentation on that function for more.

(3.0)
*/
HIWindowRef
Session_ReturnActiveWindow	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	WindowRef			result = ptr->dataPtr->window;
	
	
	return result;
}// ReturnActiveWindow


/*!
Returns a pathname for the slave pseudo-terminal device
attached to the given session.  This can be displayed in
user interface elements.

(3.1)
*/
CFStringRef
Session_ReturnPseudoTerminalDeviceNameCFString		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFStringRef			result = ptr->deviceNameString.returnCFStringRef();
	
	
	return result;
}// ReturnPseudoTerminalDeviceNameCFString


/*!
Returns a succinct string representation of the
specified session’s resource; for a remote session
this is always a URL, for local sessions it is the
Unix command line.  This can be displayed in user
interface elements.

(3.0)
*/
CFStringRef
Session_ReturnResourceLocationCFString		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFStringRef			result = ptr->resourceLocationString.returnCFStringRef();
	
	
	return result;
}// ReturnResourceLocationCFString


/*!
Returns the state of the specified session.  This
is critical for knowing when it is safe to perform
certain tasks.

(3.0)
*/
Session_State
Session_ReturnState	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_State		result = ptr->status;
	
	
	return result;
}// ReturnState


/*!
Returns attributes of the current session state
(for example, whether or not an alert user interface
element is currently modal to the session).

(3.1)
*/
Session_StateAttributes
Session_ReturnStateAttributes	(SessionRef		inRef)
{
	SessionAutoLocker			ptr(gSessionPtrLocks(), inRef);
	Session_StateAttributes		result = ptr->statusAttributes;
	
	
	return result;
}// ReturnStateAttributes


/*!
Activates the specified session, from the user’s
perspective.  For example, brings all of its
windows to the front, showing them if they are
obscured, etc.

\retval kSession_ResultOK
if there are no errors

\retval kSession_ResultInvalidReference
if the specified session is unrecognized

(3.1)
*/
Session_Result
Session_Select	(SessionRef		inRef)
{
	Session_Result		result = kSession_ResultOK;
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	if (nullptr == ptr) result = kSession_ResultInvalidReference;
	else
	{
		// notify listeners; this should trigger, for instance, the associated
		// window(s) becoming frontmost
		changeNotifyForSession(ptr, kSession_ChangeSelected, inRef);
	}
	return result;
}// Select


/*!
Adds the specified data to a buffer, which will be sent to
the local or remote process for the given session when the
receiver is ready.

Since this version of the function has encoding information
available (in the CFString), it converts the data into the
target encoding and sends the new format as a raw stream of
bytes.  The assumption is that the application running in
the terminal will know how to decode the data in the format
that the user has selected for the session.

Returns the number of bytes actually written, or a negative
number on error.  If conversion is required, conversion
problems are represented as incomplete writes instead of
errors; so you should check the returned count against the
actual size of the string, to see if all data was sent.

(3.0)
*/
SInt16
Session_SendData	(SessionRef		inRef,
					 CFStringRef	inString)
{
	// first determine how much space will be needed
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFIndex				numberOfBytesRequired = 0;
	CFIndex				numberOfCharactersConverted = CFStringGetBytes(inString,
																		CFRangeMake(0, CFStringGetLength(inString)),
																		ptr->writeEncoding,
																		0/* loss byte, or 0 for no lossy conversion */,
																		false/* is external representation */,
																		nullptr/* no buffer means “return size only” */,
																		0/* size - ignored for null buffer */,
																		&numberOfBytesRequired);
	SInt16		result = -1;
	
	
	if (numberOfBytesRequired > 0)
	{
		CFIndex const	kBufferSize = numberOfBytesRequired;
		UInt8*			buffer = new UInt8[kBufferSize];
		
		
		if (nullptr != buffer)
		{
			// now actually translate/copy the data
			numberOfCharactersConverted = CFStringGetBytes(inString,
															CFRangeMake(0, CFStringGetLength(inString)),
															ptr->writeEncoding,
															0/* loss byte, or 0 for no lossy conversion */,
															false/* is external representation */,
															buffer, kBufferSize, &numberOfBytesRequired);
			
			if ((numberOfBytesRequired > 0) && (numberOfBytesRequired <= kBufferSize))
			{
				result = Session_SendData(inRef, buffer, numberOfBytesRequired);
			}
			delete [] buffer;
		}
	}
	return result;
}// SendData


/*!
Adds the specified data to a buffer, which will be
sent to the local or remote process for the given
session when the receiver is ready.

Returns the number of bytes actually written, or a
negative number on error.

(3.0)
*/
SInt16
Session_SendData	(SessionRef		inRef,
					 void const*	inBufferPtr,
					 size_t			inByteCount)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	SInt16				result = 0;
	
	
	if (nullptr != ptr->mainProcess)
	{
		result = Local_WriteBytes(Local_ProcessReturnMasterTerminal(ptr->mainProcess), inBufferPtr, inByteCount);
	}
	
	return result;
}// SendData


/*!
Sends any data waiting to be sent (i.e. in the
buffer) to the local or remote process for the
given session immediately, clearing out the
buffer if possible.  Returns the number of bytes
left in the buffer.

(3.0)
*/
SInt16
Session_SendFlush	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	SInt16				result = 0;
	
	
	// not currently defined
	
	return result;
}// SendFlush


/*!
Sends the specified data to the local or remote
process for the given session immediately (flushing
the buffer), if possible.

Returns the number of bytes actually written (not
counting any previous cache that was flushed), or
a negative number on error.

(3.0)
*/
SInt16
Session_SendFlushData	(SessionRef		inRef,
						 void const*	inBufferPtr,
						 SInt16			inByteCount)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	SInt16				result = 0;
	
	
	(SInt16)Session_SendFlush(inRef);
	if (result == 0) result = Session_SendData(inRef, inBufferPtr, inByteCount);
	
	return result;
}// SendFlushData


/*!
Sends a carriage return sequence.  Depending upon the
user preference and terminal mode for carriage return
handling, this may send CR, CR-LF or CR-null.

Since Apple Events allow carriage returns to not be
echoed as an option, you must override the session
echo setting explicitly.  For efficiency, you can
provide a constant to use the current echo setting
for the given session, or you can provide an explicit
on/off value.

(3.0)
*/
void
Session_SendNewline		(SessionRef		inRef,
						 Session_Echo	inEcho)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	TerminalScreenRef	screen = TerminalWindow_ReturnScreenWithFocus(ptr->terminalWindow);
	Boolean				lineFeedToo = false;
	Boolean				echo = false;
	
	
	if (nullptr != screen)
	{
		lineFeedToo = Terminal_LineFeedNewLineMode(screen);
	}
	
	if (lineFeedToo)
	{
		Session_SendFlushData(inRef, "\015\012", 2);
	}
	else
	{
		Session_SendFlushData(inRef, "\015", 1);
	}
	
	switch (inEcho)
	{
	case kSession_EchoCurrentSessionValue:
		echo = (0 != ptr->dataPtr->echo);
		break;
	
	case kSession_EchoEnabled:
		echo = true;
		break;
	
	case kSession_EchoDisabled:
	default:
		echo = false;
		break;
	}
	if (echo) Session_TerminalWrite(inRef, (UInt8*)"\012\015", 2);
}// SendNewline


/*!
Specifies the maximum number of bytes that can be
read with each call to Session_ProcessMoreData().
Depending on available memory, the actual buffer size
may end up being less than the amount requested.

Requests of less than 512 bytes are increased to be
a minimum of 512 bytes.

\retval kSession_ResultOK
if there are no errors

\retval kSession_ResultInvalidReference
if the specified session is unrecognized

\retval kSession_ResultInsufficientBufferSpace
if there is not enough memory to meet the request

(3.0)
*/
Session_Result
Session_SetDataProcessingCapacity	(SessionRef		inRef,
									 size_t			inBlockSizeInBytes)
{
	Session_Result		result = kSession_ResultOK;
	
	
	if (inRef == nullptr) result = kSession_ResultInvalidReference;
	else
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		UInt8*				newBuffer = nullptr;
		
		
		// set the size, and allocate a buffer if necessary
		ptr->readBufferSizeMaximum = (inBlockSizeInBytes < 512) ? 512 : inBlockSizeInBytes;
		try
		{
			// use the new buffer in place of the stored one, then free the stored buffer
			// WARNING: this allocation/deallocation scheme should match the constructor/destructor
			newBuffer = new UInt8[ptr->readBufferSizeMaximum];
			std::swap(ptr->readBufferPtr, newBuffer);
			delete [] newBuffer, newBuffer = nullptr;
		}
		catch (std::bad_alloc)
		{
			result = kSession_ResultInsufficientBufferSpace;
		}
	}
	
	return result;
}// SetDataProcessingCapacity


/*!
Enables or disables local echoing.  This determines
whether characters are parsed locally prior to being
sent to the server.  For slow network connections,
it may be too sluggish to type and wait for the
server to echo back characters.

If the specified session uses a protocol that cares
about this option (such as TELNET), the appropriate
sequences are automatically sent to the server based
on the new local echo enabled state.

(3.0)
*/
void
Session_SetLocalEchoEnabled		(SessionRef		inRef,
								 Boolean		inIsEnabled)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->dataPtr->echo = inIsEnabled;
}// SetLocalEchoEnabled


/*!
Sets local echo to full-duplex mode.  Full duplex
means that data is locally echoed but not sent to
the server until a return key or control key is
pressed.

(3.0)
*/
void
Session_SetLocalEchoFullDuplex	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->dataPtr->halfdup = 0;
}// SetLocalEchoFullDuplex


/*!
Sets local echo to half-duplex mode.  Half duplex
means that data is locally echoed and immediately
sent to the server.

(3.0)
*/
void
Session_SetLocalEchoHalfDuplex	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->dataPtr->halfdup = 1;
}// SetLocalEchoHalfDuplex


/*!
Specifies whether the given connection is disabled -
that is, whether its network activity has been
suspended by a Scroll Lock.

(3.0)
*/
void
Session_SetNetworkSuspended		(SessionRef		inRef,
								 Boolean		inScrollLock)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->dataPtr->enabled = !inScrollLock;
	
	// send the appropriate character to the Unix process
	if (inScrollLock)
	{
	#if 0
		{
			CFStringRef		userNotificationCFString = nullptr;
			
			
			// 3.1 - dump a string that says “[Suspend Output]”
			if (UIStrings_Copy(kUIStrings_TerminalSuspendOutput, userNotificationCFString).ok())
			{
				Session_TerminalWriteCString(inRef, "\033[3;5m"); // italic, blinking
				Session_TerminalWriteCFString(inRef, userNotificationCFString);
				Session_TerminalWriteCString(inRef, "\033[23;25m"); // remove italic, blinking
				CFRelease(userNotificationCFString), userNotificationCFString = nullptr;
			}
		}
	#endif
		
		// display a help tag over the cursor in an unobtrusive location
		// that confirms for the user that a suspend has in fact occurred
		{
			My_HMHelpContentRecWrap&	tagData = createHelpTagForSuspend();
			HIRect						globalCursorBounds;
			
			
			TerminalView_GetCursorGlobalBounds(TerminalWindow_ReturnViewWithFocus
												(Session_ReturnActiveTerminalWindow(inRef)),
												globalCursorBounds);
			tagData.setFrame(globalCursorBounds);
			(OSStatus)HMDisplayTag(tagData.ptr());
		#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
			// this call does not immediately hide the tag, but rather after a short delay
			if (FlagManager_Test(kFlagOS10_4API))
			{
				(OSStatus)HMHideTagWithOptions(kHMHideTagFade);
			}
		#endif
		}
		
		// suspend
		if (nullptr != ptr->mainProcess)
		{
			queueCharacterInKeyboardBuffer(ptr, STATIC_CAST
											(Local_ReturnTerminalFlowStopCharacter
												(Local_ProcessReturnMasterTerminal(ptr->mainProcess)), char));
		}
		// set the scroll lock attribute of the session
		changeStateAttributes(ptr, kSession_StateAttributeSuspendNetwork/* attributes to set */,
								0/* attributes to clear */);
	}
	else
	{
	#if 0
		{
			CFStringRef		userNotificationCFString = nullptr;
			
			
			// 3.1 - dump a string that says “[Resume Output]”
			if (UIStrings_Copy(kUIStrings_TerminalResumeOutput, userNotificationCFString).ok())
			{
				Session_TerminalWriteCString(inRef, "\033[3m"); // italic
				Session_TerminalWriteCFString(inRef, userNotificationCFString);
				Session_TerminalWriteCString(inRef, "\033[23m"); // remove italic
				CFRelease(userNotificationCFString), userNotificationCFString = nullptr;
			}
		}
	#endif
		
		// hide the help tag displayed by the Suspend
		(OSStatus)HMHideTag();
		
		// resume
		if (nullptr != ptr->mainProcess)
		{
			queueCharacterInKeyboardBuffer(ptr, STATIC_CAST
											(Local_ReturnTerminalFlowStartCharacter
												(Local_ProcessReturnMasterTerminal(ptr->mainProcess)), char));
		}
		// clear the scroll lock attribute of the session
		changeStateAttributes(ptr, 0/* attributes to set */,
								kSession_StateAttributeSuspendNetwork/* attributes to clear */);
	}
	Session_FlushUserInputBuffer(inRef);
}// SetNetworkSuspended


/*!
Defines what Session_SendNewline() does.

(3.0)
*/
void
Session_SetNewlineMode	(SessionRef				inRef,
						 Session_NewlineMode	inNewlineMode)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->dataPtr->crmap = inNewlineMode;
}// SetNewlineMode


/*!
Specifies the process that this session is running.  This
automatically sets the device name, window title, and resource
location (Session_ReturnPseudoTerminalDeviceNameCFString(),
Session_GetWindowUserDefinedTitle(), and
Session_ReturnResourceLocationCFString() can be used to return
these values later).

(3.1)
*/
void
Session_SetProcess	(SessionRef			inRef,
					 Local_ProcessRef	inRunningProcess)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	assert(nullptr != inRunningProcess);
	ptr->mainProcess = inRunningProcess;
	{
		// set device name string
		char const* const	kDeviceName = Local_ProcessReturnSlaveDeviceName(ptr->mainProcess);
		
		
		ptr->deviceNameString.setCFTypeRef(CFStringCreateWithCString
											(kCFAllocatorDefault, kDeviceName, kCFStringEncodingASCII),
											true/* is retained */);
	}
	{
		// set resource location string (and initial window title to match)
		char const* const	kCommandLine = Local_ProcessReturnCommandLineString(ptr->mainProcess);
		
		
		ptr->resourceLocationString.setCFTypeRef(CFStringCreateWithCString
													(kCFAllocatorDefault, kCommandLine, kCFStringEncodingASCII),
													true/* is retained */);
		Session_SetWindowUserDefinedTitle(inRef, ptr->resourceLocationString.returnCFStringRef());
		changeNotifyForSession(ptr, kSession_ChangeResourceLocation, inRef/* context */);
	}
}// SetProcess


/*!
Specifies the encoding to use for data that is sent to the
session via encoding-aware APIs such as Session_SendData()
(with a CFStringRef parameter).

WARNING:	This should generally match what the currently
			running application in a terminal is capable of
			handling.  It is probably user-specified if it
			cannot be determined automatically.

\retval kSession_ResultOK
if there are no errors

\retval kSession_ResultInvalidReference
if the specified session is unrecognized

(3.1)
*/
Session_Result
Session_SetSendDataEncoding		(SessionRef			inRef,
								 CFStringEncoding	inEncoding)
{
	Session_Result		result = kSession_ResultOK;
	
	
	if (nullptr == inRef) result = kSession_ResultInvalidReference;
	else
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		ptr->writeEncoding = inEncoding;
	}
	return result;
}// SetSendDataEncoding


/*!
Specifies whether the given session should speak
incoming text for the user.

(3.0)
*/
void
Session_SetSpeechEnabled	(SessionRef		inRef,
							 Boolean		inIsEnabled)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	Terminal_SetSpeechEnabled(ptr->dataPtr->vs, inIsEnabled);
}// SetSpeechEnabled


/*!
Changes the current state of the specified session,
which will trigger listener notification for all
modules interested in finding out about new states
(such as sessions that die, etc.).

IMPORTANT:	Currently, this routine does not check
			that the specified state is a valid
			“next state” for the current state.
			This may have bizarre consequences;
			for example, transitioning a session
			from a dead state back to an active one.
			In the future, this routine will try to
			implicitly transition a session through
			a series of valid state transitions to
			end up at the requested state, telling
			all interested listeners about each
			intermediate state.

(3.0)
*/
void
Session_SetState	(SessionRef		inRef,
					 Session_State	inNewState)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->status = inNewState;
	if ((kSession_StateDead == ptr->status) || (kSession_StateImminentDisposal == ptr->status))
	{
		Local_KillProcess(&ptr->mainProcess);
	}
	changeNotifyForSession(ptr, kSession_ChangeState, inRef/* context */);
}// SetState


/*!
Used to force the terminal window associated with a
session to have a certain value.  Normally, this is
set at connection time, but for Local sessions this
hack is currently necessary (otherwise, the Terminal
Window cannot be obtained from a Session, which breaks
a lot of things needlessly for Local shells).

Events are fired notifying listeners of the terminal
window becoming either valid or invalid (invalid if
nullptr is provided).

TEMPORARY

NOTE:	This API may not make perfect sense.  In the
		future, a session might have more than one
		logical terminal window associated with it.

(3.0)
*/
void
Session_SetTerminalWindow	(SessionRef			inRef,
							 TerminalWindowRef	inTerminalWindow)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->terminalWindow = inTerminalWindow;
	ptr->dataPtr->window = TerminalWindow_ReturnWindow(inTerminalWindow);
	ptr->dataPtr->vs = TerminalWindow_ReturnScreenWithFocus(inTerminalWindow);
	if (nullptr == inTerminalWindow)
	{
		changeNotifyForSession(ptr, kSession_ChangeWindowInvalid, inRef/* context */);
	}
	else
	{
		changeNotifyForSession(ptr, kSession_ChangeWindowValid, inRef/* context */);
	}
}// SetTerminalWindow


/*!
Enables or disables notification of network activity and
inactivity.

When a notifier fires, this flag is cleared.

(3.1)
*/
void
Session_SetWatch	(SessionRef		inRef,
					 Session_Watch	inNewWatch)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->activeWatch = inNewWatch;
	
	// for watches requiring timers, create a timer if none exists;
	// but wait until data arrives to actually set the clock
	if ((kSession_WatchForInactivity == inNewWatch) ||
		(kSession_WatchForKeepAlive == inNewWatch))
	{
		if (nullptr == ptr->inactivityWatchTimer)
		{
			// first time; create the timer
			OSStatus	error = noErr;
			
			
			ptr->inactivityWatchTimerUPP = NewEventLoopTimerUPP(watchNotifyFromTimer);
			error = InstallEventLoopTimer(GetCurrentEventLoop(),
											kEventDurationForever/* start time - do not start yet */,
											kEventDurationForever/* time between fires - this timer does not repeat */,
											ptr->inactivityWatchTimerUPP, ptr->selfRef/* user data */,
											&ptr->inactivityWatchTimer);
			assert_noerr(error);
		}
		
		// for inactivity timers, start the clock right now (also,
		// the timer automatically resets as new data arrives)
		watchTimerResetForSession(ptr, inNewWatch);
	}
}// SetWatch


/*!
Specifies the most recent user-specified window title;
defaults to the one given in the New Sessions dialog,
but should be updated whenever the user invokes the
“Rename…” menu command.

NOTE:	This API is under evaluation.  If sessions are
		allowed to send their data to multiple windows
		someday, this will not be adequate.  But perhaps
		it could still be used to set a common name for
		all session windows, etc.

(3.0)
*/
void
Session_SetWindowUserDefinedTitle	(SessionRef		inRef,
									 CFStringRef	inWindowName)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->dataPtr->alternateTitle.setCFTypeRef(CFStringCreateCopy(kCFAllocatorDefault, inWindowName));
	changeNotifyForSession(ptr, kSession_ChangeWindowTitle, inRef);
}// SetWindowUserDefinedTitle


/*!
Returns "true" only if incoming text for the specified
session is spoken by a speech synthesizer.  (Note that
this is NOT an indicator of whether the computer is
currently speaking anything; for that, use a system
call such as SpeechBusy().)

(3.0)
*/
Boolean
Session_SpeechIsEnabled		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = Terminal_SpeechIsEnabled(ptr->dataPtr->vs);
	
	
	return result;
}// SpeechIsEnabled


/*!
Interrupts any speech currently in progress for the
specified session.  This does NOT affect the enabled
state of speech, although speech will not be heard
again until resumed.

You might use this routine to stop speaking immediately
(as a menu command, for example).  It may also be
useful to invoke this routine immediately prior to
setting the enabled state to false, if you do not want
the computer to speak the remainder of its buffer.

(3.0)
*/
void
Session_SpeechPause		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	Terminal_SpeechPause(ptr->dataPtr->vs);
}// SpeechPause


/*!
Resumes speech paused with Session_SpeechPause().

(3.0)
*/
void
Session_SpeechResume	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	Terminal_SpeechResume(ptr->dataPtr->vs);
}// SpeechResume


/*!
Arranges for a callback to be invoked whenever a setting
changes for a session (such as its status or character
set).

IMPORTANT:	The context passed to the listener callback
			is reserved for passing information relevant
			to a change.  See "Session.h" for comments
			on what the context means for each type of
			change.

(3.0)
*/
void
Session_StartMonitoring		(SessionRef					inRef,
							 Session_Change				inForWhatChange,
							 ListenerModel_ListenerRef	inListener)
{
	if (inForWhatChange == kSession_AllChanges)
	{
		// recursively invoke for ALL session change types listed in "Session.h"
		Session_StartMonitoring(inRef, kSession_ChangeCloseWarningAnswered, inListener);
		Session_StartMonitoring(inRef, kSession_ChangeDataArrived, inListener);
		Session_StartMonitoring(inRef, kSession_ChangeResourceLocation, inListener);
		Session_StartMonitoring(inRef, kSession_ChangeSelected, inListener);
		Session_StartMonitoring(inRef, kSession_ChangeState, inListener);
		Session_StartMonitoring(inRef, kSession_ChangeStateAttributes, inListener);
		Session_StartMonitoring(inRef, kSession_ChangeWindowInvalid, inListener);
		Session_StartMonitoring(inRef, kSession_ChangeWindowObscured, inListener);
		Session_StartMonitoring(inRef, kSession_ChangeWindowTitle, inListener);
		Session_StartMonitoring(inRef, kSession_ChangeWindowValid, inListener);
	}
	else
	{
		OSStatus			error = noErr;
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		// add a listener to the specified target’s listener model for the given setting change
		error = ListenerModel_AddListenerForEvent(ptr->changeListenerModel, inForWhatChange, inListener);
	}
}// StartMonitoring


/*!
Returns "true" only if the specified connection’s
status says it is active (that is, its DNS lookup
and opening operations are complete, and it is
still connected to a remote server).

There are two active states: unstable and stable.
This routine returns "true" if either state is
current.

(3.0)
*/
Boolean
Session_StateIsActive	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = false;
	
	
	result = ((kSession_StateActiveUnstable == ptr->status) ||
				(kSession_StateActiveStable == ptr->status));
	return result;
}// StateIsActive


/*!
Returns "true" only if the specified connection’s
status says it is active and stable.  This means
the connection has been open for several seconds
and has not died.

There are two active states: unstable and stable.
This routine returns "true" only if the session
is in the stable active state.

(3.0)
*/
Boolean
Session_StateIsActiveStable		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_StateActiveStable == ptr->status);
	
	
	return result;
}// StateIsActiveStable


/*!
Returns "true" only if the specified connection’s
status says it is active but unstable.  This means
the connection has opened very recently, so it is
not clear if it will survive.

There are two active states: unstable and stable.
This routine returns "true" only if the session
is in the unstable active state.

(3.0)
*/
Boolean
Session_StateIsActiveUnstable	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_StateActiveUnstable == ptr->status);
	
	
	return result;
}// StateIsActiveUnstable


/*!
Returns "true" only if the specified session has
JUST been created.  This is guaranteed to be the
initial state of a Session.  HOWEVER, the session
MIGHT NOT be initialized; using it could cause a
disaster.  See Session_StateIsInitialized(), which
tells you when the session has been properly set up.

(3.0)
*/
Boolean
Session_StateIsBrandNew		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_StateBrandNew == ptr->status);
	
	
	return result;
}// StateIsBrandNew


/*!
Returns "true" only if the specified session’s status
says it is dead (that is, its process has been killed
but perhaps its window is still open).

(3.0)
*/
Boolean
Session_StateIsDead		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_StateDead == ptr->status);
	
	
	return result;
}// StateIsDead


/*!
Returns "true" only if the specified session is in the
process of being destroyed.  This is guaranteed to be
the final state of a Session, so when a Session enters
this state it is appropriate to perform cleanup tasks
(for example, removing a Session’s name from a list).

(3.0)
*/
Boolean
Session_StateIsImminentDisposal		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_StateImminentDisposal == ptr->status);
	
	
	return result;
}// StateIsImminentDisposal


/*!
Returns "true" only if the specified session has JUST
been properly initialized.  Unlike the “Brand New”
state, this state asserts that the session is now
valid and can be generally used.  This is therefore
an appropriate time to add the session to a menu for
the user, etc.

(3.1)
*/
Boolean
Session_StateIsInitialized		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_StateInitialized == ptr->status);
	
	
	return result;
}// StateIsInitialized


/*!
Arranges for a callback to no longer be invoked whenever
a setting changes for a session (such as its status or
character set).  Pass nullptr as the session to cancel
notifications when changes occur to any session.

IMPORTANT:	This routine cancels the effects of a previous
			call to Session_StartMonitoring() - your
			parameters must match the previous start-call,
			or the stop will fail.

(3.0)
*/
void
Session_StopMonitoring	(SessionRef					inRef,
						 Session_Change				inForWhatChange,
						 ListenerModel_ListenerRef	inListener)
{
	if (inForWhatChange == kSession_AllChanges)
	{
		// recursively invoke for ALL session change types listed in "Session.h"
		Session_StopMonitoring(inRef, kSession_ChangeCloseWarningAnswered, inListener);
		Session_StopMonitoring(inRef, kSession_ChangeDataArrived, inListener);
		Session_StopMonitoring(inRef, kSession_ChangeResourceLocation, inListener);
		Session_StopMonitoring(inRef, kSession_ChangeSelected, inListener);
		Session_StopMonitoring(inRef, kSession_ChangeState, inListener);
		Session_StopMonitoring(inRef, kSession_ChangeStateAttributes, inListener);
		Session_StopMonitoring(inRef, kSession_ChangeWindowInvalid, inListener);
		Session_StopMonitoring(inRef, kSession_ChangeWindowObscured, inListener);
		Session_StopMonitoring(inRef, kSession_ChangeWindowTitle, inListener);
		Session_StopMonitoring(inRef, kSession_ChangeWindowValid, inListener);
	}
	else
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		// remove a listener from the specified target’s listener model for the given setting change
		ListenerModel_RemoveListenerForEvent(ptr->changeListenerModel, inForWhatChange, inListener);
	}
}// StopMonitoring


/*!
Creates a new TEK graphics window and associates it with
the specified session.  TEK writes will subsequently
affect the new window.  Returns "true" only if the new
window was successfully created (if not, any existing
window remains the target).

(3.0)
*/
Boolean
Session_TEKCreateTargetGraphic		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	SInt16				id = -1;
	Boolean				result = false;
	
	
	id = VGnewwin(TEK_DEVICE_WINDOW, inRef);
	if (id > -1)
	{
		Str255		string;
		
		
		ptr->dataPtr->TEK.graphicsID = id;
		VGgiveinfo(id);
		GetWTitle(ptr->dataPtr->window, string);
		StringUtilities_PToCInPlace(string);
		RGattach(id, inRef, REINTERPRET_CAST(string, char*), ptr->dataPtr->TEK.mode);
		Session_AddDataTarget(inRef, kSession_DataTargetTektronixGraphicsCanvas, &id);
		result = true;
	}
	return result;
}// TEKCreateTargetGraphic


/*!
Dissociates the current target graphic of the given
session from the session.  TEK writes will therefore
no longer affect this graphic.  If there is no
target graphic, this routine does nothing.

(3.0)
*/
void
Session_TEKDetachTargetGraphic		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	if (ptr->dataPtr->TEK.graphicsID > -1) detachGraphics(ptr->dataPtr->TEK.graphicsID);
}// TEKDetachTargetGraphic


/*!
Returns the command set, if any, supported by the
graphic attached to the given session.

(3.1)
*/
TektronixMode
Session_TEKGetMode	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	TektronixMode		result = ptr->dataPtr->TEK.mode;
	
	
	return result;
}// TEKGetMode


/*!
Returns "true" only if there is currently a target
TEK vector graphic for the specified session.

(3.0)
*/
Boolean
Session_TEKHasTargetGraphic		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (ptr->dataPtr->TEK.graphicsID > -1);
	
	
	return result;
}// TEKHasTargetGraphic


/*!
Returns "true" only if the specified session is
set up to handle Tektronix vector graphics.

(3.0)
*/
Boolean
Session_TEKIsEnabled	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kTektronixModeNotAllowed != ptr->dataPtr->TEK.mode);
	
	
	return result;
}// TEKIsEnabled


/*!
Returns "true" only if the user has specified
that the given session should open a new TEK
window whenever a TEK PAGE command is received.
(The alternative is that the current TEK window
is cleared and replaced with the new graphics.)

(3.0)
*/
Boolean
Session_TEKPageCommandOpensNewWindow	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kTektronixPageLocationNewWindowClear == ptr->dataPtr->TEK.pageLocation);
	
	
	return result;
}// TEKPageCommandOpensNewWindow


/*!
Writes the specified string to the TEK graphics
screen, without transmitting it to the network.
Returns the number of characters parsed (equal
to "inLength" if everything was parsed), or -1
if an internal error occurs.

(3.0)
*/
SInt16
Session_TEKWrite	(SessionRef		inRef,
					 char const*	inBuffer,
					 SInt16			inLength)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	SInt16				result = 0;
	
	
	result = VGwrite(ptr->dataPtr->TEK.graphicsID, inBuffer, inLength);
	return result;
}// TEKWrite


/*!
Returns the answer-back message that is sent to the
server in response to a terminal identification
request.  For example, a VT100 terminal might return
the message "vt100", or the name of some other
compatible terminal.

Call CFRelease() on the returned string when you are
finished with it.

\retval kSession_ResultOK
if there are no errors

\retval kSession_ResultInvalidReference
if the specified session is unrecognized or if the
answerback message could not be found

(3.1)
*/
Session_Result
Session_TerminalCopyAnswerBackMessage	(SessionRef		inRef,
										 CFStringRef&	outAnswerBack)
{
	Session_Result		result = kSession_ResultOK;
	
	
	if (nullptr == inRef) result = kSession_ResultInvalidReference;
	else
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		outAnswerBack = Terminal_EmulatorReturnName(ptr->dataPtr->vs);
		if (nullptr != outAnswerBack) CFRetain(outAnswerBack);
	}
	
	return result;
}// TerminalCopyAnswerBackMessage


/*!
Writes the specified buffer to the local terminal,
without transmitting it to the network.  The terminal
emulator will interpret the string and adjust the
display appropriately if any commands are embedded
(otherwise, text is inserted).

(3.0)
*/
void
Session_TerminalWrite	(SessionRef		inRef,
						 UInt8 const*	inBuffer,
						 UInt32			inLength)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	Terminal_EmulatorProcessData(ptr->dataPtr->vs, inBuffer, inLength);
}// TerminalWrite


/*!
Writes the specified string to the local terminal,
without transmitting it to the network.  The terminal
emulator will interpret the string and adjust the
display appropriately if any commands are embedded
(otherwise, text is inserted).

(3.0)
*/
void
Session_TerminalWriteCString	(SessionRef		inRef,
								 char const*	inCString)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	Terminal_EmulatorProcessCString(ptr->dataPtr->vs, inCString);
}// TerminalWriteCString


/*!
Writes the specified string to the local terminal,
without transmitting it to the network.  The terminal
emulator will interpret the string and adjust the
display appropriately if any commands are embedded
(otherwise, text is inserted).

(3.1)
*/
void
Session_TerminalWriteCFString	(SessionRef		inRef,
								 CFStringRef	inCFString)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	Terminal_EmulatorProcessCFString(ptr->dataPtr->vs, inCFString);
}// TerminalWriteCFString


/*!
Returns the clock time (in seconds) for the computer
back when the specified session’s command was run.
You can use TimeString() or DateString() on the
result to get a text representation.

(3.0)
*/
UInt32
Session_TimeOfActivation	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	UInt32				result = 0L;
	
	
	result = ptr->connectionDateTime;
	return result;
}// TimeOfActivation


/*!
Returns the clock time (in seconds) for the computer
back when the specified session’s command exited.
You can use TimeString() or DateString() on the
result to get a text representation.

(3.1)
*/
CFAbsoluteTime
Session_TimeOfTermination	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFAbsoluteTime		result = 0L;
	
	
	result = ptr->terminationAbsoluteTime;
	return result;
}// TimeOfTermination


/*!
Determines whether or not the specified session
started by running /usr/bin/login.

Use Session_TypeIsLocalNonLoginShell() to see if
the session is running the user’s preferred shell
directly.

(3.0)
*/
Boolean
Session_TypeIsLocalLoginShell	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_TypeLocalLoginShell == ptr->kind);
	
	
	return result;
}// TypeIsLocalLoginShell


/*!
Determines whether or not the specified session
is running a local, non-login shell process.

Use Session_TypeIsLocalLoginShell() to see if
the user was required to log in when opening the
session.

(3.0)
*/
Boolean
Session_TypeIsLocalNonLoginShell	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_TypeLocalNonLoginShell == ptr->kind);
	
	
	return result;
}// TypeIsLocalNonLoginShell


/*!
Send a string to a session as if it were typed into
the specified session’s window.  If local echoing is
enabled for the session, the string will be written
to the local data target (usually a terminal) after
it is sent to the remote process.

(3.0)
*/
void
Session_UserInputCFString	(SessionRef		inRef,
							 CFStringRef	inStringBuffer,
							 Boolean		inSendToRecordingScripts)
{
	Session_SendFlush(inRef);
	Session_FlushUserInputBuffer(inRef);
	Session_SendData(inRef, inStringBuffer);
	
	if (Session_LocalEchoIsEnabled(inRef))
	{
		try
		{
			SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
			char*				stringBuffer = new char[1 + CFStringGetLength(inStringBuffer)];
			
			
			if (CFStringGetCString(inStringBuffer, stringBuffer, sizeof(stringBuffer), ptr->writeEncoding))
			{
				Session_TerminalWriteCString(inRef, stringBuffer);
			}
			delete [] stringBuffer;
		}
		catch (std::bad_alloc)
		{
			Console_WriteLine("user input failed, out of memory!");
		}
	}
	
	// 3.0 - send to any scripts that may be recording
	if (inSendToRecordingScripts)
	{
		sendRecordableDataTransmitEvent(inStringBuffer, false/* execute */);
	}
}// UserInputCFString


/*!
Send input to the session to interrupt whatever
process is running.  For local sessions, this means
to send control-C (or whatever the interrupt control
key is).  For remote sessions, this means to send a
telnet "IP" sequence.

(3.1)
*/
void
Session_UserInputInterruptProcess	(SessionRef		inRef,
									 Boolean		inSendToRecordingScripts)
{
	// clear the Suspend state from MacTelnet’s point of view,
	// since the process already considers the pipe reopened
	Session_SetNetworkSuspended(inRef, false);
	
#if 0
	{
		CFStringRef		userNotificationCFString = nullptr;
		
		
		// 3.0 - get a string that says “[Interrupt Process]”
		if (UIStrings_Copy(kUIStrings_TerminalInterruptProcess, userNotificationCFString).ok())
		{
			Session_TerminalWriteCString(inRef, "\033[3m"); // italic
			Session_TerminalWriteCFString(inRef, userNotificationCFString);
			Session_TerminalWriteCString(inRef, "\033[23m"); // remove italic
			CFRelease(userNotificationCFString), userNotificationCFString = nullptr;
		}
	}
#endif
	
	// display a help tag over the cursor in an unobtrusive location
	// that confirms for the user that an interrupt has in fact occurred
	{
		My_HMHelpContentRecWrap&	tagData = createHelpTagForInterrupt();
		HIRect						globalCursorBounds;
		
		
		TerminalView_GetCursorGlobalBounds(TerminalWindow_ReturnViewWithFocus
											(Session_ReturnActiveTerminalWindow(inRef)),
											globalCursorBounds);
		tagData.setFrame(globalCursorBounds);
		(OSStatus)HMDisplayTag(tagData.ptr());
	#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
		// this call does not immediately hide the tag, but rather after a short delay
		if (FlagManager_Test(kFlagOS10_4API))
		{
			(OSStatus)HMHideTagWithOptions(kHMHideTagFade);
		}
	#endif
	}
	
	// send character to Unix process
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		if (nullptr != ptr->mainProcess)
		{
			Session_UserInputQueueCharacter(inRef, STATIC_CAST
											(Local_ReturnTerminalInterruptCharacter
												(Local_ProcessReturnMasterTerminal(ptr->mainProcess)),
												char));
			Session_FlushUserInputBuffer(inRef);
		}
	}
	
	// 3.0 - send to any scripts that may be recording
	if (inSendToRecordingScripts)
	{
		sendRecordableSpecialTransmitEvent
			(kTelnetEnumeratedClassSpecialCharacterInterruptProcess, false/* execute */);
	}
}// UserInputInterruptProcess


/*!
Sends the appropriate sequence for the specified key,
taking into account the terminal mode settings.

\retval kSession_ResultOK
if the key was input successfully

\retval kSession_ResultInvalidReference
if "inRef" is invalid

\retval kSession_ResultParameterError
if "inTarget" or "inTargetData" are invalid

(3.1)
*/
Session_Result
Session_UserInputKey	(SessionRef		inRef,
						 UInt8			inKeyOrASCII)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_Result		result = kSession_ResultOK;
	
	
	if (nullptr == ptr) result = kSession_ResultInvalidReference;
	else
	{
		TerminalWindowRef		terminalWindow = Session_ReturnActiveTerminalWindow(inRef);
		TerminalScreenRef		screen = TerminalWindow_ReturnScreenWithFocus(terminalWindow);
		
		
		// perform an initial mapping if the keypad is in cursor keys mode
		// and the specified key is a keypad number that has a cursor key
		if (Terminal_KeypadHasMovementKeys(screen))
		{
			switch (inKeyOrASCII)
			{
			case VSK2:
				inKeyOrASCII = VSDN;
				break;
			
			case VSK4:
				inKeyOrASCII = VSLT;
				break;
			
			case VSK6:
				inKeyOrASCII = VSRT;
				break;
			
			case VSK8:
				inKeyOrASCII = VSUP;
				break;
			
			default:
				// not applicable
				break;
			}
		}
		
		if (ptr->dataPtr->arrowmap && (inKeyOrASCII <= VSLT) && (inKeyOrASCII >= VSUP))
		{
			UInt8		actualKey = inKeyOrASCII;
			
			
			// map from arrow keys to EMACS control keys;
			// note that option-arrow-keys could do things
			// as well, if modifiers were considered here
			switch (inKeyOrASCII)
			{
			case VSLT:
				actualKey = 0x02; // ^B
				break;
			
			case VSRT:
				actualKey = 0x06; // ^F
				break;
			
			case VSUP:
				actualKey = 0x10; // ^P
				break;
			
			case VSDN:
				actualKey = 0x0E; // ^N
				break;
			
			default:
				break;
			}
			Session_SendData(ptr->selfRef, &actualKey, 1);
		}
		else if (inKeyOrASCII < VSF10)
		{
			// 7-bit ASCII; send as-is
			Session_SendData(ptr->selfRef, &inKeyOrASCII, 1);
		}
		else if ((inKeyOrASCII >= VSK0) && (inKeyOrASCII <= VSKE) && (false == Terminal_KeypadHasApplicationKeys(screen)))
		{
			// VT SPECIFIC:
			// keypad key in numeric mode (as opposed to application key mode)
			UInt8 const		kVTNumericModeTranslation[] =
			{
				// numbers, symbols, Enter, PF1-PF4
				"0123456789,-.\15"
			};
			
			
			Session_SendData(ptr->selfRef, &kVTNumericModeTranslation[inKeyOrASCII - VSK0], 1);
			if (0 != ptr->dataPtr->echo)
			{
				Terminal_EmulatorProcessData(ptr->dataPtr->vs, &kVTNumericModeTranslation[inKeyOrASCII - VSUP], 1);
			} 
			if (VSKE/* Enter */ == inKeyOrASCII)
			{
				Session_SendData(ptr->selfRef, "\012", 1);
			}
		}
		else
		{
			// VT SPECIFIC:
			// keypad key in application mode (as opposed to numeric mode),
			// or an arrow key or PF-key in either mode
			char const		kVTApplicationModeTranslation[] =
			{
				// arrows, numbers, symbols, Enter, PF1-PF4
				"ABCDpqrstuvwxylmnMPQRS"
			};
			char*		seqPtr = nullptr;
			size_t		seqLength = 0;
			
			
			if (Terminal_KeypadHasMovementKeys(screen))
			{
				static char		seqAuxiliaryCode[] = "\033O ";
				
				
				// auxiliary keypad mode
				seqAuxiliaryCode[2] = kVTApplicationModeTranslation[inKeyOrASCII - VSUP];
				seqPtr = seqAuxiliaryCode;
				seqLength = sizeof(seqAuxiliaryCode);
			}
			else if (inKeyOrASCII < VSUP)
			{
				// construct key code sequences starting from VSF10 (see VTKeys.h);
				// each sequence is defined starting with the template, below, and
				// then substituting 1 or 2 characters into the sequence template;
				// the order must exactly match what is in VTKeys.h, because of the
				// subtraction used to derive the array index
				static char			seqVT220Keys[] = "\033[  ~";
				static char const	kArrayIndex2Translation[] = "222122?2?3?3?2?3?3123425161";
				static char const	kArrayIndex3Translation[] = "134956?9?2?3?8?1?4~~~~0~8~7";
				
				
				seqVT220Keys[2] = kArrayIndex2Translation[inKeyOrASCII - VSF10];
				seqVT220Keys[3] = kArrayIndex3Translation[inKeyOrASCII - VSF10];
				seqPtr = seqVT220Keys;
				seqLength = sizeof(seqVT220Keys);
				if ('~' == seqPtr[3]) --seqLength; // a few of the sequences are shorter
			}
			else
			{
				if (inKeyOrASCII < VSK0)
				{
					static char		seqArrowsNormal[] = "\033[ ";
					
					
					if (Terminal_KeypadHasMovementKeys(screen))
					{
						seqArrowsNormal[1] = 'O';
					}
					else
					{
						seqArrowsNormal[1] = '[';
					}
					seqArrowsNormal[2] = kVTApplicationModeTranslation[inKeyOrASCII - VSUP];
					seqPtr = seqArrowsNormal;
					seqLength = sizeof(seqArrowsNormal);
				}
				else if (inKeyOrASCII < VSF1)
				{
					static char		seqKeypadNormal[] = "\033O ";
					
					
					seqKeypadNormal[2] = kVTApplicationModeTranslation[inKeyOrASCII - VSUP];
					seqPtr = seqKeypadNormal;
					seqLength = sizeof(seqKeypadNormal);
				}
				else
				{
					static char		seqFunctionKeys[] = "\033O ";
					
					
					seqFunctionKeys[2] = kVTApplicationModeTranslation[inKeyOrASCII - VSUP];
					seqPtr = seqFunctionKeys;
					seqLength = sizeof(seqFunctionKeys);
				}
			}
			
			Session_SendData(ptr->selfRef, seqPtr, seqLength);
			if (0 != ptr->dataPtr->echo)
			{
				Terminal_EmulatorProcessData(ptr->dataPtr->vs, REINTERPRET_CAST(seqPtr, UInt8*), seqLength);
			}
		}
	}
	
	return result;
}// UserInputKey


/*!
Allows the user to “type” whatever text is on the Clipboard.

With some terminal applications (like shells), pasting in more
than one line can cause unexpected results.  So, if the Clipboard
text is multi-line, the user is warned and given options on how
to perform the Paste.  This also means that this function could
return before the Paste actually occurs.

\retval kSession_ResultOK
always; no other return codes currently defined

(3.1)
*/
Session_Result
Session_UserInputPaste	(SessionRef		inRef)
{
	SessionAutoLocker		ptr(gSessionPtrLocks(), inRef);
	My_PasteAlertInfoPtr	pasteAlertInfoPtr = new My_PasteAlertInfo;
	CFStringRef				pastedCFString = nullptr;
	CFStringRef				pastedDataUTI = nullptr;
	Session_Result			result = kSession_ResultOK;
	
	
	pasteAlertInfoPtr->sessionForPaste = inRef;
	if (Clipboard_CreateCFStringFromPasteboard(pastedCFString, pastedDataUTI))
	{
		// examine the Clipboard; if the data contains new-lines, warn the user
		Boolean		displayWarning = false;
		
		
		pasteAlertInfoPtr->clipboardData = pastedCFString;
		
		// determine if this is a multi-line paste
		{
			UniChar const*		bufferPtr = CFStringGetCharactersPtr(pastedCFString);
			CFIndex				kBufferLength = CFStringGetLength(pastedCFString);
			UniChar*			allocatedBuffer = nullptr;
			
			
			if (nullptr == bufferPtr)
			{
				allocatedBuffer = new UniChar[kBufferLength];
				bufferPtr = allocatedBuffer;
				CFStringGetCharacters(pastedCFString, CFRangeMake(0, kBufferLength), allocatedBuffer);
			}
			displayWarning = (false == Clipboard_IsOneLineInBuffer(bufferPtr, kBufferLength));
			if (nullptr != allocatedBuffer) delete [] allocatedBuffer, allocatedBuffer = nullptr;
		}
		
		// if text is available, proceed
		if (pasteAlertInfoPtr->clipboardData.exists())
		{
			AlertMessages_BoxRef	box = Alert_New();
			
			
			if (false == displayWarning)
			{
				// the Clipboard contains only one line of text, so do not warn first
				pasteWarningCloseNotifyProc(box, kAlertStdAlertOKButton, pasteAlertInfoPtr/* user data */);
			}
			else
			{
				// configure and display the confirmation alert
				
				// set basics
				Alert_SetParamsFor(box, kAlert_StyleOKCancel);
				Alert_SetType(box, kAlertCautionAlert);
				
				// set message
				{
					UIStrings_Result	stringResult = kUIStrings_ResultOK;
					CFStringRef			primaryTextCFString = nullptr;
					
					
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowPasteLinesWarningPrimaryText, primaryTextCFString);
					if (stringResult.ok())
					{
						CFStringRef		helpTextCFString = nullptr;
						
						
						stringResult = UIStrings_Copy(kUIStrings_AlertWindowPasteLinesWarningHelpText, helpTextCFString);
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
					
					
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowPasteLinesWarningName, titleCFString);
					if (stringResult == kUIStrings_ResultOK)
					{
						Alert_SetTitleCFString(box, titleCFString);
						CFRelease(titleCFString);
					}
				}
				
				// set buttons
				{
					UIStrings_Result	stringResult = kUIStrings_ResultOK;
					CFStringRef			buttonTitleCFString = nullptr;
					
					
					stringResult = UIStrings_Copy(kUIStrings_ButtonMakeOneLine, buttonTitleCFString);
					if (stringResult == kUIStrings_ResultOK)
					{
						Alert_SetButtonText(box, kAlertStdAlertOKButton, buttonTitleCFString);
						CFRelease(buttonTitleCFString);
					}
					stringResult = UIStrings_Copy(kUIStrings_ButtonPasteNormally, buttonTitleCFString);
					if (stringResult == kUIStrings_ResultOK)
					{
						Alert_SetButtonText(box, kAlertStdAlertOtherButton, buttonTitleCFString);
						CFRelease(buttonTitleCFString);
					}
				}
				
				// ensure that the relevant window is visible and frontmost
				// when the message appears
				ShowWindow(ptr->dataPtr->window);
				EventLoop_SelectBehindDialogWindows(ptr->dataPtr->window);
				Alert_MakeWindowModal(box, ptr->dataPtr->window/* parent */, false/* is window close alert */,
										pasteWarningCloseNotifyProc, pasteAlertInfoPtr/* user data */);
				
				// returns immediately; notifier disposes the alert when the sheet
				// eventually closes
				Alert_Display(box);
			}
		}
		CFRelease(pastedCFString), pastedCFString = nullptr;
		CFRelease(pastedDataUTI), pastedDataUTI = nullptr;
	}
	
	return result;
}// UserInputPaste


/*!
Adds the specified character to the keyboard input
queue, where it will eventually be processed.  This
is normally used only by things that are intercepting
keyboard input from the user.

(3.0)
*/
void
Session_UserInputQueueCharacter		(SessionRef		inRef,
									 char			inCharacter)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	(Boolean)queueCharacterInKeyboardBuffer(ptr, inCharacter);
}// UserInputQueueCharacter


/*!
Send a string to a session as if it were typed into
the specified session’s window.  If local echoing is
enabled for the session, the string will be written
to the local data target (usually a terminal) after
it is sent to the remote process.

(3.0)
*/
void
Session_UserInputString		(SessionRef		inRef,
							 char const*	inStringBuffer,
							 size_t			inStringBufferSize,
							 Boolean		inSendToRecordingScripts)
{
	Session_SendFlush(inRef);
	Session_FlushUserInputBuffer(inRef);
	Session_SendData(inRef, inStringBuffer, inStringBufferSize);
	
	if (Session_LocalEchoIsEnabled(inRef))
	{
		Session_TerminalWrite(inRef, (UInt8 const*)inStringBuffer,
								inStringBufferSize / sizeof(char));
	}
	
	// 3.0 - send to any scripts that may be recording
	if (inSendToRecordingScripts)
	{
		sendRecordableDataTransmitEvent(inStringBuffer, inStringBufferSize / sizeof(char), false/* execute */);
	}
}// UserInputString


/*!
Returns "true" only if the specified session is being
watched for a lack of activity over a short period of
time.

See Session_SetWatch().

(3.1)
*/
Boolean
Session_WatchIsForInactivity	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_WatchForInactivity == ptr->activeWatch);
	
	
	return result;
}// WatchIsForInactivity


/*!
Returns "true" only if the specified session is being
watched for a lack of activity over a longer period of
time, and will transmit a keep-alive string once that
time passes.

See Session_SetWatch().

(3.1)
*/
Boolean
Session_WatchIsForKeepAlive		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_WatchForKeepAlive == ptr->activeWatch);
	
	
	return result;
}// WatchIsForKeepAlive


/*!
Returns "true" only if the specified session is being
watched for activity.

See Session_SetWatch().

(3.1)
*/
Boolean
Session_WatchIsForPassiveData	(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_WatchForPassiveData == ptr->activeWatch);
	
	
	return result;
}// WatchIsForPassiveData


/*!
Returns "true" only if the specified session has no
active monitors for activity (or lack thereof).

See Session_SetWatch().

(3.1)
*/
Boolean
Session_WatchIsOff		(SessionRef		inRef)
{
	SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean				result = (kSession_WatchNothing == ptr->activeWatch);
	
	
	return result;
}// WatchIsOff


#pragma mark Internal Methods

/*!
Constructor.  See Session_New().

DEPRECATED.  This exists for transition purposes only,
as it is based on a very old data structure that is
still referenced throughout the code.

(3.1)
*/
ConnectionData::
ConnectionData ()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
vs				(nullptr),
window			(nullptr),
alternateTitle	(),
enabled			(true),
bsdel			(0),
eightbit		(0),
national		(0),
arrowmap		(0),
showErrors		(0),
keypadmap		(0),
metaKey			(0),
Xterm			(0),
pgupdwn			(false),
crmap			(0),
echo			(0),
halfdup			(0),
kblen			(0),
// kbbuf initialized below
// parsedat initialized below
parseIndex		(0),
controlKey		(),
TEK				()
{
	bzero(&this->kbbuf, sizeof(this->kbbuf));
	bzero(&this->parsedat, sizeof(this->parsedat));
}// ConnectionData default constructor


/*!
Notifies all listeners for the specified Session
state change, passing the given context to the
listener.

IMPORTANT:	The context must make sense for the
			type of change; see "Session.h" for
			the type of context associated with
			each session change.

(3.0)
*/
static void
changeNotifyForSession		(SessionPtr			inPtr,
							 Session_Change		inWhatChanged,
							 void*				inContextPtr)
{
	// invoke listener callback routines appropriately, from the specified session’s listener model
	ListenerModel_NotifyListenersOfEvent(inPtr->changeListenerModel, inWhatChanged, inContextPtr);
}// changeNotifyForSession


/*!
Applies changes to the session state attributes and
then notifies listeners appropriately.

The specified set attributes are set before the
specified clear attributes are cleared.

(3.1)
*/
static void
changeStateAttributes	(SessionPtr					inPtr,
						 Session_StateAttributes	inAttributesToSet,
						 Session_StateAttributes	inAttributesToClear)
{
	inPtr->statusAttributes |= inAttributesToSet;
	inPtr->statusAttributes &= ~inAttributesToClear;
	changeNotifyForSession(inPtr, kSession_ChangeStateAttributes, inPtr->selfRef/* context */);
}// changeStateAttributes


/*!
Initializes a Navigation Services structure with values
appropriate for save dialogs on a session window.

This includes:
- adding a flag to omit translation items
- setting the application name
- setting a generic preferences key
- setting the parent window

(3.1)
*/
static void
configureSaveDialog		(SessionRef					inRef,
						 NavDialogCreationOptions&	inoutOptions)
{
	inoutOptions.optionFlags |= kNavDontAddTranslateItems;
	Localization_GetCurrentApplicationNameAsCFString(&inoutOptions.clientName);
	inoutOptions.preferenceKey = kPreferences_NavPrefKeyGenericSaveFile;
	inoutOptions.parentWindow = Session_ReturnActiveWindow(inRef);
}// configureSaveDialog


/*!
Invoked whenever a monitored connection state is changed
(see Session_New() to see which states are monitored).
This routine responds by updating the session status
string appropriately.

(3.0)
*/
static void
connectionStateChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
							 ListenerModel_Event	inConnectionSettingThatChanged,
							 void*					inEventContextPtr,
							 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inConnectionSettingThatChanged)
	{
	case kSession_ChangeState:
		// update status text to reflect new state
		{
			SessionRef			session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			SessionAutoLocker	ptr(gSessionPtrLocks(), session);
			CFStringRef			statusString = nullptr;
			
			
			// once connected, set the connection time
			if (kSession_StateActiveUnstable == ptr->status)
			{
				GetDateTime(&ptr->connectionDateTime); // in seconds
			}
			
			// now update the session status string
			if (kSession_StateDead == ptr->status)
			{
				Boolean		useTimeFreeStatusString = (0 == ptr->terminationAbsoluteTime);
				
				
				if (false == useTimeFreeStatusString)
				{
					// if possible, be specific about the time of disconnect
					if (UIStrings_Copy(kUIStrings_SessionInfoWindowStatusTerminatedAtTime,
										statusString).ok())
					{
						CFLocaleRef		currentLocale = CFLocaleCopyCurrent();
						
						
						if (nullptr != currentLocale)
						{
							CFDateFormatterRef	timeOnlyFormatter = CFDateFormatterCreate
																	(kCFAllocatorDefault, currentLocale,
																		kCFDateFormatterNoStyle/* date style */,
																		kCFDateFormatterShortStyle/* time style */);
							
							
							if (nullptr != timeOnlyFormatter)
							{
								CFStringRef		timeCFString = CFDateFormatterCreateStringWithAbsoluteTime
																(kCFAllocatorDefault, timeOnlyFormatter,
																	ptr->terminationAbsoluteTime);
								
								
								if (nullptr != timeCFString)
								{
									CFStringRef		statusWithTime = CFStringCreateWithFormat
																			(kCFAllocatorDefault, nullptr/* options */,
																				statusString/* format */, timeCFString);
									
									
									if (nullptr != statusWithTime)
									{
										ptr->statusString.setCFTypeRef(statusWithTime);
										CFRelease(statusWithTime), statusWithTime = nullptr;
									}
									else
									{
										// on error, attempt to fall back with the time-free status string
										useTimeFreeStatusString = true;
									}
									CFRelease(timeCFString), timeCFString = nullptr;
								}
								CFRelease(timeOnlyFormatter), timeOnlyFormatter = nullptr;
							}
							CFRelease(currentLocale), currentLocale = nullptr;
						}
					}
				}
				
				if (useTimeFreeStatusString)
				{
					if (UIStrings_Copy(kUIStrings_SessionInfoWindowStatusProcessTerminated,
										statusString).ok())
					{
						ptr->statusString.setCFTypeRef(statusString);
					}
				}
			}
			else if (kSession_StateActiveUnstable == ptr->status)
			{
				(UIStrings_Result)UIStrings_Copy(kUIStrings_SessionInfoWindowStatusProcessNewborn,
													statusString);
				ptr->statusString.setCFTypeRef(statusString);
			}
			else if (kSession_StateActiveStable == ptr->status)
			{
				(UIStrings_Result)UIStrings_Copy(kUIStrings_SessionInfoWindowStatusProcessRunning,
													statusString);
				ptr->statusString.setCFTypeRef(statusString);
			}
			else
			{
				// ???
				ptr->statusString.setCFTypeRef(CFSTR(""));
			}
			
			// if the user preference is set, kill the terminal window
			// when sessions are finished
			if (kSession_StateDead == ptr->status)
			{
				Boolean		keepWindowOpen = true;
				size_t		actualSize = 0L;
				
				
				// get the user’s process service preference, if possible
				unless (Preferences_GetData(kPreferences_TagDontAutoClose,
											sizeof(keepWindowOpen), &keepWindowOpen,
											&actualSize) == kPreferences_ResultOK)
				{
					keepWindowOpen = false; // assume window should be closed, if preference can’t be found
				}
				
				unless (keepWindowOpen) killConnection(ptr);
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// connectionStateChanged


/*!
Returns (creating if necessary) the global help tag record
for a help tag that confirms for the user that the process
running in the active terminal screen has been interrupted.

If the title of the tag has not been set already, it is
initialized.

Normally, this tag should be displayed at the terminal
cursor location, so prior to using the result you should
call its setFrame() method.

(3.1)
*/
static My_HMHelpContentRecWrap&
createHelpTagForInterrupt ()
{
	static My_HMHelpContentRecWrap		gTag;
	
	
	if (nullptr == gTag.mainName())
	{
		CFStringRef		tagCFString = nullptr;
		
		
		if (UIStrings_Copy(kUIStrings_TerminalInterruptProcess, tagCFString).ok())
		{
			gTag.rename(tagCFString, nullptr/* alternate name */);
			// you CANNOT release this string because you do not know when the system is done with the tag
		}
	}
	
	return gTag;
}// createHelpTagForInterrupt


/*!
Returns (creating if necessary) the global help tag record
for a help tag that confirms for the user that output for
the active terminal screen has been restarted.

If the title of the tag has not been set already, it is
initialized.

Normally, this tag should be displayed at the terminal
cursor location, so prior to using the result you should
call its setFrame() method.

(3.1)
*/
static My_HMHelpContentRecWrap&
createHelpTagForResume ()
{
	static My_HMHelpContentRecWrap		gTag;
	
	
	if (nullptr == gTag.mainName())
	{
		CFStringRef		tagCFString = nullptr;
		
		
		if (UIStrings_Copy(kUIStrings_TerminalResumeOutput, tagCFString).ok())
		{
			gTag.rename(tagCFString, nullptr/* alternate name */);
			// you CANNOT release this string because you do not know when the system is done with the tag
		}
	}
	
	return gTag;
}// createHelpTagForResume


/*!
Returns (creating if necessary) the global help tag record
for a help tag that confirms for the user that output for
the active terminal screen has been stopped.

If the title of the tag has not been set already, it is
initialized.

Normally, this tag should be displayed at the terminal
cursor location, so prior to using the result you should
call its setFrame() method.

(3.1)
*/
static My_HMHelpContentRecWrap&
createHelpTagForSuspend ()
{
	static My_HMHelpContentRecWrap		gTag;
	
	
	if (nullptr == gTag.mainName())
	{
		CFStringRef		tagCFString = nullptr;
		
		
		if (UIStrings_Copy(kUIStrings_TerminalSuspendOutput, tagCFString).ok())
		{
			gTag.rename(tagCFString, nullptr/* alternate name */);
			// you CANNOT release this string because you do not know when the system is done with the tag
		}
	}
	
	return gTag;
}// createHelpTagForSuspend


/*!
Creates a Page Format object that can be used for printing
to any session.  Currently, all sessions use the same one.
This affects things like paper size, etc.

(3.1)
*/
static PMPageFormat
createSessionPageFormat ()
{
	PMPageFormat	result = nullptr;
	OSStatus		error = noErr;
	
	
	error = PMCreatePageFormat(&result);
	assert_noerr(error);
	
	return result;
}// createSessionPageFormat


/*!
Registers the “active session” icon reference with the system,
and returns a reference to the new icon.

(3.1)
*/
static IconRef
createSessionStateActiveIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnSessionStatusActiveIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconSessionStatusActive,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createSessionStateActiveIcon


/*!
Registers the “dead session” icon reference with the system,
and returns a reference to the new icon.

(3.1)
*/
static IconRef
createSessionStateDeadIcon ()
{
	IconRef		result = nullptr;
	FSRef		iconFile;
	
	
	if (AppResources_GetArbitraryResourceFileFSRef
		(AppResources_ReturnSessionStatusDeadIconFilenameNoExtension(),
			CFSTR("icns")/* type */, iconFile))
	{
		if (noErr != RegisterIconRefFromFSRef(AppResources_ReturnCreatorCode(),
												kConstantsRegistry_IconServicesIconSessionStatusDead,
												&iconFile, &result))
		{
			// failed!
			result = nullptr;
		}
	}
	
	return result;
}// createSessionStateDeadIcon


/*!
Invoked when more data has appeared in the internal buffer,
and needs processing.

This can also trigger any watches set on the session.

(3.1)
*/
static void
dataArrived		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
				 ListenerModel_Event	inEvent,
				 void*					inEventContextPtr,
				 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inEvent)
	{
	case kSession_ChangeDataArrived:
		{
			SessionRef		session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			
			
			Session_ProcessMoreData(session);
			
			// also trigger a watch, if one exists
			{
				SessionAutoLocker	ptr(gSessionPtrLocks(), session);
				
				
				if (kSession_WatchForPassiveData == ptr->activeWatch)
				{
					// data arrived; notify immediately
					watchNotifyForSession(ptr, ptr->activeWatch);
				}
				else if ((kSession_WatchForKeepAlive == ptr->activeWatch) ||
							(kSession_WatchForInactivity == ptr->activeWatch))
				{
					// reset timer, start waiting again
					watchTimerResetForSession(ptr, ptr->activeWatch);
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// dataArrived


/*!
Detects when "kSession_LifetimeMinimumForNoWarningClose"
seconds have passed, and changes the state of an active
session to reflect its stability.

This special state is useful for user interface events
such as confirmation alerts that can be more annoying
than useful for windows that have been asked to close
very soon after they’ve opened.

Timers that draw must save and restore the current
graphics port.

Mac OS X only.

(3.0)
*/
static pascal void
detectLongLife	(EventLoopTimerRef		UNUSED_ARGUMENT(inTimer),
				 void*					inSessionRef)
{
	SessionRef			ref = REINTERPRET_CAST(inSessionRef, SessionRef);
	SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	
	
	if (Session_StateIsActiveUnstable(ref)) Session_SetState(ref, kSession_StateActiveStable);
}// detectLongLife


/*!
Invokes the KeyTranslate() function using the data
from the specified event record.  The state of the
key is automatically extracted from the type of
event, which must be "keyUp", "keyDown" or "autoKey".
Also extracted are the modifier key states (high
byte of "modifiers" field).

Calls to this routine are chained; state is preserved
so that previous key events might impact the result
(e.g. one key event is option-E, the aigu accent;
if the subsequent key event is E, the pair of
characters are returned together, where the accent
is the prefix character).  Automatically, if the
current key translation ('KCHR') resource is
different than the one used for the last call to
KeyTranslate(), this routine resets the state to
ensure past characters are ignored.

On output, the two character codes are returned
individually.  Pass nullptr for the last parameter
if you are not interested in its value, but keep
in mind that international keyboards will not work
properly if you ignore the prefix value.  IGNORING
THE SECOND CHARACTER CODE IS A VERY BAD IDEA.

(3.0)
*/
static void
getKeyEventCharacters	(SInt16				inVirtualKeyCode,
						 SInt16				UNUSED_ARGUMENT(inCharacterCode),
						 EventModifiers		inModifiers,
						 SInt16*			outPrimaryCharacterCodePtr,
						 SInt16*			outPrefixCharacterCodePtrOrNull)
{
	UInt16				keyTranslateInput = 0;
	UInt32				correspondingCharacterCodes = 0L;
	static UInt32		keyTranslateState = 0L;
	static void*		resourceKCHRCachePtr = nullptr;
	EventModifiers		changedModifiers = inModifiers;
	
	
	keyTranslateInput = (UInt16)inVirtualKeyCode;
	changedModifiers &= 0xFF00; // ignore low byte
	keyTranslateInput |= changedModifiers;
	keyTranslateInput |= btnState; // set key press/release!!!
	if (REINTERPRET_CAST(GetScriptManagerVariable(smKCHRCache), void*) != resourceKCHRCachePtr)
	{
		// the state only resets when the 'KCHR' changes
		resourceKCHRCachePtr = REINTERPRET_CAST(GetScriptManagerVariable(smKCHRCache), void*);
		keyTranslateState = 0L;
	}
	correspondingCharacterCodes = KeyTranslate(resourceKCHRCachePtr, keyTranslateInput, &keyTranslateState);
	*outPrimaryCharacterCodePtr = (correspondingCharacterCodes & 0x0000FFFF);
	if (outPrefixCharacterCodePtrOrNull != nullptr)
	{
		correspondingCharacterCodes >>= 16; // look at upper bytes
		*outPrefixCharacterCodePtrOrNull = (correspondingCharacterCodes & 0x0000FFFF);
	}
}// getKeyEventCharacters


/*!
Invoked whenever a monitored key-down event from the main
event loop occurs (see Session_New() to see how this routine
is registered).

This function only responds to processed character or key
codes; the original event handler should perform key
mappings, etc. prior to invoking this routine.

The result is "true" only if the event is to be absorbed
(preventing anything else from seeing it).

NOTE:	This used to be a direct callback, hence its strange
		function signature.  This will probably be fixed in
		the future.

(3.1)
*/
static Boolean
handleSessionKeyDown	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inEventThatOccurred,
						 void*					inEventContextPtr,
						 void*					inListenerContextPtr)
{
	enum
	{
		/*!
		Important virtual key codes (see page 2-43 of
		"Inside Macintosh: Macintosh Toolbox Essentials").
		*/
		BScode			= 0x33,	//!< backspace virtual key code
		KPlowest		= 0x41	//!< lowest keypad virtual key code
	};
	assert(inEventThatOccurred == kEventLoop_ControlEventKeyPress);
	assert(inEventContextPtr != nullptr);
	assert(inListenerContextPtr != nullptr);
	
	Boolean								result = false;
	EventInfoControlScope_KeyPressPtr	keyPressInfoPtr = REINTERPRET_CAST
															(inEventContextPtr,
																EventInfoControlScope_KeyPressPtr);
	SessionRef							session = REINTERPRET_CAST(inListenerContextPtr, SessionRef);
	SessionAutoLocker					ptr(gSessionPtrLocks(), session);
	ConnectionDataPtr					connectionDataPtr = nullptr; // TEMPORARY, until transition to SessionRef
	static SInt16						characterCode = '\0'; // ASCII
	static SInt16						virtualKeyCode = '\0'; // see p.2-43 of "IM:MTE" for a set of virtual key codes
	static Boolean						commandDown = false;
	static Boolean						controlDown = false;
	static Boolean						optionDown  = false;
	static Boolean						shiftDown = false;
	static Boolean						metaDown = false;
	
	
	connectionDataPtr = ptr->dataPtr;
	
	characterCode = keyPressInfoPtr->characterCode;
	virtualKeyCode = keyPressInfoPtr->virtualKeyCode;
	commandDown = keyPressInfoPtr->commandDown;
	controlDown = keyPressInfoPtr->controlDown;
	optionDown = keyPressInfoPtr->optionDown;
	shiftDown = keyPressInfoPtr->shiftDown;
	
	// technically add-on support for an EMACS concept
	metaDown = ((commandDown) && (controlDown) &&
				(connectionDataPtr->metaKey == kSession_EMACSMetaKeyControlCommand));
	
	// scan for keys that invoke instant commands
	switch (virtualKeyCode)
	{
	case 0x7A: // F1
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro1);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF6);
		}
		break;
	
	case 0x78: // F2
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro2);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF7);
		}
		break;
	
	case 0x63: // F3
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro3);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF8);
		}
		break;
	
	case 0x76: // F4
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro4);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF9);
		}
		break;
	
	case 0x60: // F5
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro5);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF10);
		}
		break;
	
	case 0x61: // F6
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro6);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF11);
		}
		break;
	
	case 0x62: // F7
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro7);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF12);
		}
		break;
	
	case 0x64: // F8
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro8);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF13);
		}
		break;
	
	case 0x65: // F9
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro9);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF14);
		}
		break;
	
	case 0x6D: // F10
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro10);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF15);
		}
		break;
	
	case 0x67: // F11
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro11);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF16);
		}
		break;
	
	case 0x6F: // F12
		if (Macros_ReturnMode() == kMacroManager_InvocationMethodFunctionKeys)
		{
			result = Commands_ExecuteByID(kCommandSendMacro12);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF17);
		}
		break;
	
	case 0x69: // Print Screen (F13)
		if (0/* if not VT220 */)
		{
			result = Commands_ExecuteByID(kCommandPrintScreen);
		}
		else
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF18);
		}
		break;
	
	case 0x6B: // F14
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF19);
		break;
	
	case 0x71: // F15
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF20);
		break;
	
	case VSPGUP:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((connectionDataPtr->pgupdwn) || Terminal_EmulatorIsVT100(connectionDataPtr->vs))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewPageUp);
		}
		break;
	
	case VSPGDN:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((connectionDataPtr->pgupdwn) || Terminal_EmulatorIsVT100(connectionDataPtr->vs))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewPageDown);
		}
		break;
	
	case VSHOME:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((connectionDataPtr->pgupdwn) || Terminal_EmulatorIsVT100(connectionDataPtr->vs))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewHome);
		}
		break;
	
	case VSEND:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((connectionDataPtr->pgupdwn) || Terminal_EmulatorIsVT100(connectionDataPtr->vs))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewEnd);
		}
		break;
	
	case 0x7B: // Left arrow
	case 0x3B: // Left arrow on non-extended keyboards
		Session_UserInputKey(session, VSLT);
		result = true;
		break;
	
	case 0x7C: // Right arrow
	case 0x3C: // Right arrow on non-extended keyboards
		Session_UserInputKey(session, VSRT);
		result = true;
		break;
	
	case 0x7D: // Down arrow
	case 0x3D: // Down arrow on non-extended keyboards
		Session_UserInputKey(session, VSDN);
		result = true;
		break;
	
	case 0x7E: // Up arrow
	case 0x3E: // Up arrow on non-extended keyboards
		Session_UserInputKey(session, VSUP);
		result = true;
		break;
	
	default:
		// no other virtual key codes have significance
		break;
	}
	
	if (false == result)
	{
		// if no key-based action occurred, look for character-based actions
		if (characterCode == connectionDataPtr->controlKey.suspend)
		{
			Session_SetNetworkSuspended(session, true);
			result = true;
		}
		if (characterCode == connectionDataPtr->controlKey.resume) 
		{
			Session_SetNetworkSuspended(session, false);
			result = true;
		}
		if (characterCode == connectionDataPtr->controlKey.interrupt)  
		{
			Session_UserInputInterruptProcess(session);
			result = true;
		}
		
		// now check for constant character matches
		if (false == result)
		{
			switch (characterCode)
			{
			case '0':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro1);
				}
				break;
			
			case '1':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro2);
				}
				break;
			
			case '2':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro3);
				}
				break;
			
			case '3':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro4);
				}
				break;
			
			case '4':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro5);
				}
				break;
			
			case '5':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro6);
				}
				break;
			
			case '6':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro7);
				}
				break;
			
			case '7':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro8);
				}
				break;
			
			case '8':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro9);
				}
				break;
			
			case '9':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro10);
				}
				break;
			
			case '=':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro11);
				}
				break;
			
			case '/':
				if ((commandDown) && (Macros_ReturnMode() == kMacroManager_InvocationMethodCommandDigit))
				{
					result = Commands_ExecuteByID(kCommandSendMacro12);
				}
				break;
			
			case '\015': // CR
				Session_SendNewline(session, kSession_EchoCurrentSessionValue);
				result = true;
				break;
			
			default:
				// no other character codes have significance
				break;
			}
			
			if (false == result)
			{
				// no key-based or character-based actions have occurred;
				// fine, the key should be respected “verbatim” (send the
				// one or two characters it represents to the session)
				UInt8			charactersToSend[2];
				UInt8*			characterPtr = charactersToSend;
				size_t			theSize = sizeof(charactersToSend);
				EventModifiers	modifiers = 0;
				
				
				if (commandDown) modifiers |= cmdKey;
				if (optionDown) modifiers |= optionKey;
				if (controlDown) modifiers |= controlKey;
				
				// perform one final substitution: the meta key in EMACS
				if (metaDown)
				{
					if (characterCode <= 32)
					{
						// control key changed the ASCII value; fix it
						characterCode |= 0x40;
					}
				}
				else if (shiftDown)
				{
					// this key state is ignored in meta key processing
					modifiers |= shiftKey;
				}
				
				// determine the equivalent characters for the given key presses
				{
					SInt16		prefixChar = 0;
					SInt16		primaryChar = 0;
					
					
					getKeyEventCharacters(virtualKeyCode, characterCode, modifiers, &primaryChar, &prefixChar);
					charactersToSend[0] = STATIC_CAST(prefixChar, UInt8);
					charactersToSend[1] = STATIC_CAST(primaryChar, UInt8);
				}
				
				if (metaDown)
				{
					// EMACS respects a prefixing Escape character as being
					// equivalent to meta; so ESC-CtrlA is like MetaCtrlA
					charactersToSend[0] = 0x1B; // ESC
				}
				else if (0 == charactersToSend[0])
				{
					// no prefix was given; skip it
					characterPtr = &charactersToSend[1];
					theSize = sizeof(characterPtr[0]);
				}
				
				if (metaDown)
				{
					Session_FlushUserInputBuffer(session);
				}
				
				// finally, send the characters (echoing if necessary)
				if (Session_LocalEchoIsEnabled(session) && Session_LocalEchoIsHalfDuplex(session))
				{
					Session_TerminalWrite(session, characterPtr, theSize);
				}
				Session_SendData(session, characterPtr, theSize);
				
				result = true;
			}
		}
	}
	
	return result;
}// handleSessionKeyDown


/*!
Returns "true" only if the specified session is
not allowing user input.

(2.6)
*/
static inline Boolean
isReadOnly		(SessionPtr		inPtr)
{
	return inPtr->readOnly;
}// isReadOnly


/*!
Destroys the window for a session, cleans up
flags and memory allocations made for it, and
removes its name from the Window menu.

Typically, in response to a standard connection
close, you use Session_Disconnect(), which will
take into account the user’s preference as to
whether windows remain open, etc. and clean up
auxiliary tasks initiated by that session, such
as printing or capturing to a file.  That routine
invokes this one if it turns out that a session
window should be destroyed.

(2.6)
*/
static void
killConnection		(SessionPtr		inPtr)
{
	if (inPtr->dataPtr != nullptr)
	{
		//size_t		junk = 0;
		Boolean		wasDead = (inPtr->status == kSession_StateDead);
		
		
		Cursors_UseWatch();
		
		// 3.1 - record the time when the command exited
		inPtr->terminationAbsoluteTime = CFAbsoluteTimeGetCurrent();
		
		inPtr->status = kSession_StateImminentDisposal;
		changeNotifyForSession(inPtr, kSession_ChangeState, inPtr->selfRef/* context */);
		
		if (wasDead)
		{
			if (inPtr->dataPtr->TEK.graphicsID > -1) detachGraphics(inPtr->dataPtr->TEK.graphicsID); // detach the TEK screen
		}
		
		if (nullptr != inPtr->terminalWindow)
		{
			changeNotifyForSession(inPtr, kSession_ChangeWindowInvalid, inPtr->selfRef/* context */);
			// WARNING: This is old-style.  A session does not really “own” a terminal window,
			// it is more logical for a responder to the invalidation event to dispose of the
			// terminal window.
			TerminalWindow_Dispose(&inPtr->terminalWindow);
		}
		
		// 3.0 - destroy the connection initialization parameters!
		//Console_WriteLine("destroying initialization parameters");
		//Memory_DisposeHandle(&inPtr->dataPtr->setupParametersHandle);
		
		Session_Dispose(&inPtr->selfRef);
		
		//MaxMem(&junk);
		Cursors_UseArrow();
	}
	else
	{
		Sound_StandardAlert();
		Console_WriteLine("WARNING: Attempt to kill a connection with nonexistent data!");
	}
}// killConnection


/*!
Invoked by the Mac OS whenever something interesting
happens in a Navigation Services file-capture-save-dialog
attached to a session window.

(3.0)
*/
static pascal void
navigationFileCaptureDialogEvent	(NavEventCallbackMessage	inMessage,
								 	 NavCBRecPtr				inParameters,
								 	 void*						inSessionRef)
{
	SessionRef	session = REINTERPRET_CAST(inSessionRef, SessionRef);
	
	
	switch (inMessage)
	{
	case kNavCBUserAction:
		if (inParameters->userAction == kNavUserActionSaveAs)
		{
			NavReplyRecord		reply;
			OSStatus			error = noErr;
			
			
			// save file
			error = NavDialogGetReply(inParameters->context/* dialog */, &reply);
			if ((error == noErr) && (reply.validRecord))
			{
				FSRef	saveFile;
				FSRef	temporaryFile;
				OSType	captureFileCreator = 'ttxt';
				size_t	actualSize = 0L;
				
				
				// get the user’s Capture File Creator preference, if possible
				unless (Preferences_GetData(kPreferences_TagCaptureFileCreator,
											sizeof(captureFileCreator), &captureFileCreator, &actualSize) ==
						kPreferences_ResultOK)
				{
					captureFileCreator = 'ttxt'; // default to SimpleText if a preference can’t be found
				}
				
				error = FileSelectionDialogs_CreateOrFindUserSaveFile
						(reply, captureFileCreator, 'TEXT', saveFile, temporaryFile);
				if (error == noErr)
				{
					SessionAutoLocker	ptr(gSessionPtrLocks(), session);
					
					
					// delete the temporary file; this is ignored for file captures,
					// since capture files themselves are considered highly volatile
					(OSStatus)FSDeleteObject(&temporaryFile);
					
					// now write to the file
					error = FSCreateFork(&saveFile, 0/* name length */, nullptr/* name */);
					if (error == dupFNErr)
					{
						// if a capture file already exists, try to delete it; if the delete fails,
						// try creating files with alternate, similar names until successful
						if (FSDeleteFork(&saveFile, 0/* name length */, nullptr/* name */) == noErr)
						{
							error = FSCreateFork(&saveFile, 0/* name length */, nullptr/* name */);
						}
						else
						{
							// TEMPORARY
							// persistent create - UNIMPLEMENTED
						}
					}
					
					if (error != noErr) Alert_ReportOSStatus(error);
					else
					{
						SInt16		fileRefNum = 0;
						
						
						error = FSOpenFork(&saveFile, 0/* name length */, nullptr/* name */, fsWrPerm, &fileRefNum);
						if (error != noErr) Alert_ReportOSStatus(error);
						else
						{
							// The capture file is opened for writing at this point - however,
							// the file must be closed later whenever the capture ends.  The
							// Terminal_FileCaptureEnd() routine will notify listeners of the
							// "kTerminal_ChangeFileCaptureEnding" event - this module should
							// listen for that event so that FSClose() can be called.
							SetEOF(fileRefNum, (long)0);
							Terminal_FileCaptureBegin(ptr->dataPtr->vs, fileRefNum);
							
						#if 0
							// set the window proxy icon appropriately for the file capture
							{
								AliasHandle		alias = nullptr;
								
								
								// I can’t remember if it’s a memory leak to forget about an alias or not...warning...
								if (NewAliasMinimal(&captureFile, &alias) == noErr)
								{
									// TEMPORARY - one day it might be nice if MacTelnet could display the same screen
									//             in more than one window; in such case, the following would have to
									//             adapt to iterate over a list of windows (or better yet, Terminal Window
									//             module would simply do this automatically)
									WindowRef		window = TerminalView_ReturnWindow(dataPtr->view);
									
									
									if (window != nullptr)
									{
										(OSStatus)SetWindowProxyAlias(window, alias);
										if (API_AVAILABLE(SetWindowProxyCreatorAndType))
										{
											(OSStatus)SetWindowProxyCreatorAndType(window, captureFileCreator, 'TEXT',
																					captureFile.vRefNum);
										}
									}
									Memory_DisposeHandle(REINTERPRET_CAST(&alias, Handle*));
								}
							}
						#endif
						}
					}
				}
			}
			Alert_ReportOSStatus(error);
			error = FileSelectionDialogs_CompleteSave(&reply);
		}
		break;
	
	case kNavCBTerminate:
		// clean up
		NavDialogDispose(inParameters->context);
		break;
	
	default:
		// not handled
		break;
	}
}// navigationFileCaptureDialogEvent


/*!
Invoked by the Mac OS whenever something interesting
happens in a Navigation Services save-dialog attached
to a session window.

(3.0)
*/
static pascal void
navigationSaveDialogEvent	(NavEventCallbackMessage	inMessage,
						 	 NavCBRecPtr				inParameters,
						 	 void*						inSessionRef)
{
	OSStatus	error = noErr;
	SessionRef	session = REINTERPRET_CAST(inSessionRef, SessionRef);
	
	
	switch (inMessage)
	{
	case kNavCBUserAction:
		if (inParameters->userAction == kNavUserActionSaveAs)
		{
			NavReplyRecord		reply;
			
			
			// save file
			error = NavDialogGetReply(inParameters->context/* dialog */, &reply);
			if ((error == noErr) && (reply.validRecord))
			{
				FSRef	saveFile;
				FSRef	temporaryFile;
				
				
				error = FileSelectionDialogs_CreateOrFindUserSaveFile
						(reply, AppResources_ReturnCreatorCode(),
							kApplicationFileTypeSessionDescription,
							saveFile, temporaryFile);
				if (error == noErr)
				{
					// now write to the file, using an in-memory model first
					Session_Result			sessionError = kSession_ResultOK;
					SessionDescription_Ref  saveFileMemoryModel = nullptr;
					
					
					sessionError = Session_FillInSessionDescription(session, &saveFileMemoryModel);
					if (sessionError == kSession_ResultOK)
					{
						// now save to disk
						SInt16		fileRefNum = -1;
						
						
						// open file for overwrite
						error = FSOpenFork(&temporaryFile, 0/* fork name length */, nullptr/* fork name */, fsWrPerm, &fileRefNum);
						if (error == noErr)
						{
							SessionDescription_Result	saveError = kSessionDescription_ResultOK;
							
							
							SetEOF(fileRefNum, 0L);
							saveError = SessionDescription_Save(saveFileMemoryModel, fileRefNum);
							FSClose(fileRefNum), fileRefNum = -1;
							if (saveError == kSessionDescription_ResultOK)
							{
								error = FSExchangeObjects(&temporaryFile, &saveFile);
								if (error != noErr)
								{
									// if a “safe” save fails, the volume may not support object exchange;
									// so, fall back to overwriting the original file in that case
									error = FSOpenFork(&saveFile, 0/* fork name length */, nullptr/* fork name */, fsWrPerm, &fileRefNum);
									if (error == noErr)
									{
										SetEOF(fileRefNum, 0L);
										saveError = SessionDescription_Save(saveFileMemoryModel, fileRefNum);
										FSClose(fileRefNum), fileRefNum = -1;
									}
								}
							}
							else
							{
								error = writErr;
							}
							
							// delete the temporary; it will either contain a partial save file
							// that did not successfully write, or it will contain whatever the
							// user’s original file had (and the new file will be successfully
							// in the user’s selected location, instead of temporary space)
							(OSStatus)FSDeleteObject(&temporaryFile);
						}
						
						// clean up
						SessionDescription_Release(&saveFileMemoryModel);
					}
					else
					{
						error = paramErr;
					}
				}
			}
			Alert_ReportOSStatus(error);
			error = FileSelectionDialogs_CompleteSave(&reply);
		}
		break;
	
	case kNavCBTerminate:
		// clean up
		NavDialogDispose(inParameters->context);
		break;
	
	default:
		// not handled
		break;
	}
}// navigationSaveDialogEvent


/*!
Invoked when the user completes a Page Setup sheet (whether
or not the settings were kept).

(3.1)
*/
static pascal void
pageSetupCloseNotifyProc	(PMPrintSession		inPrintSession,
							 WindowRef			inWindow,
							 Boolean			inWasAccepted)
{
	// INCOMPLETE
	
    (OSStatus)PMRelease(inPrintSession);
}// pageSetupCloseNotifyProc


/*!
The responder to a closed “really paste?” alert.  This routine
performs the Paste to the specified session using the verbatim
Clipboard text if the item hit is the OK button.  If the user
hit the Other button, the Clipboard text is slightly modified
to form a single line, before Paste.  Otherwise, the Paste does
not occur.

The given alert is destroyed.

(3.1)
*/
static void
pasteWarningCloseNotifyProc		(InterfaceLibAlertRef	inAlertThatClosed,
								 SInt16					inItemHit,
								 void*					inMy_PasteAlertInfoPtr)
{
	My_PasteAlertInfoPtr	dataPtr = REINTERPRET_CAST(inMy_PasteAlertInfoPtr, My_PasteAlertInfoPtr);
	
	
	if (dataPtr != nullptr)
	{
		SessionAutoLocker	sessionPtr(gSessionPtrLocks(), dataPtr->sessionForPaste);
		
		
		if (inItemHit == kAlertStdAlertOKButton)
		{
			// first join the text into one line (replace new-line sequences
			// with single spaces), then Paste
			Clipboard_TextFromScrap(sessionPtr->selfRef, kClipboard_ModifierOneLine);
		}
		else if (inItemHit == kAlertStdAlertOtherButton)
		{
			// regular Paste, use verbatim what is on the Clipboard
			Clipboard_TextFromScrap(sessionPtr->selfRef, kClipboard_ModifierNone);
		}
		
		// clean up
		delete dataPtr, dataPtr = nullptr;
	}
	
	// dispose of the alert
	Alert_StandardCloseNotifyProc(inAlertThatClosed, inItemHit, nullptr/* user data */);
}// pasteWarningCloseNotifyProc


/*!
Invoked whenever a monitored preference value is changed
(see Session_New() to see which preferences are monitored).
This routine responds by ensuring that internal variables
are up to date for the changed preference.

(3.0)
*/
static void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					UNUSED_ARGUMENT(inEventContextPtr),
					 void*					inListenerContextPtr)
{
	SessionRef			ref = REINTERPRET_CAST(inListenerContextPtr, SessionRef);
	SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	size_t				actualSize = 0L;
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_TagCursorBlinks:
		// update cache with current preference value
		unless (Preferences_GetData(kPreferences_TagCursorBlinks,
										sizeof(ptr->preferencesCache.cursorFlashes),
										&ptr->preferencesCache.cursorFlashes, &actualSize) ==
				kPreferences_ResultOK)
		{
			ptr->preferencesCache.cursorFlashes = true; // assume a value, if preference can’t be found
		}
		break;
	
	case kPreferences_TagMapBackquote:
		// update cache with current preference value
		unless (Preferences_GetData(kPreferences_TagMapBackquote,
										sizeof(ptr->preferencesCache.remapBackquoteToEscape),
										&ptr->preferencesCache.remapBackquoteToEscape, &actualSize) ==
				kPreferences_ResultOK)
		{
			ptr->preferencesCache.remapBackquoteToEscape = false; // assume a value, if preference can’t be found
		}
		break;
	
	default:
		// ???
		break;
	}
}// preferenceChanged


/*!
Inserts the specified character into the keyboard
input buffer of the given session; the text will be
processed at the first opportunity, but maybe not
right away.

Returns "true" only if the character was queued
successfully.

(3.0)
*/
static Boolean
queueCharacterInKeyboardBuffer	(SessionPtr		inPtr,
								 char			inCharacter)
{
	Boolean		result = true; // TEMPORARY: no error checking here
	
	
	inPtr->dataPtr->kbbuf[inPtr->dataPtr->kblen++] = inCharacter;
	return result;
}// queueCharacterInKeyboardBuffer


/*!
Invoked whenever a monitored drag-drop event from the main
event loop occurs (see Session_New() to see how this routine
is registered).

Responds by retrieving the dropped text or file and inserting
it into the appropriate session window as if the user typed
it.  (Files are translated into Unix pathnames first.)

The result is "noErr" only if the event is to be absorbed
(preventing anything else from seeing it).

(3.1)
*/
static OSStatus
receiveTerminalViewDragDrop		(EventHandlerCallRef	inHandlerCallRef,
								 EventRef				inEvent,
								 void*					inSessionRef)
{
	OSStatus		result = eventNotHandledErr;
	SessionRef		session = REINTERPRET_CAST(inSessionRef, SessionRef);
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert((kEventKind == kEventControlDragEnter) ||
			(kEventKind == kEventControlDragWithin) ||
			(kEventKind == kEventControlDragLeave) ||
			(kEventKind == kEventControlDragReceive));
	{
		HIViewRef	view = nullptr;
		
		
		// determine the view where drag activity has occurred
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
		if (noErr == result)
		{
			DragRef		dragRef = nullptr;
			
			
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDragRef, typeDragRef, dragRef);
			if (noErr == result)
			{
				switch (kEventKind)
				{
				case kEventControlDragEnter:
					// entry point for a new drag in this view
					{
						Boolean		acceptDrag = false;
						UInt16		numberOfItems = 0;
						
						
						// figure out if this drag can be accepted
						if (noErr == CountDragItems(dragRef, &numberOfItems))
						{
							Boolean		haveSingleFile = false;
							Boolean		haveText = false;
							
							
							// check to see if exactly one file is being dragged
							if (1 == numberOfItems)
							{
								FlavorFlags		flavorFlags = 0L;
								ItemReference	dragItem = 0;
								
								
								GetDragItemReferenceNumber(dragRef, 1/* index */, &dragItem);
								if (noErr == GetFlavorFlags(dragRef, dragItem, kDragFlavorTypeHFS, &flavorFlags))
								{
									haveSingleFile = true;
								}
							}
							
							// check to see if all of the drag items contain TEXT (a drag
							// is only accepted if all of the items in the drag can be
							// accepted)
							unless (haveSingleFile)
							{
								UInt16		oneBasedIndexIntoDragItemList = 0;
								
								
								haveText = true; // initially...
								for (oneBasedIndexIntoDragItemList = 1;
										oneBasedIndexIntoDragItemList <= numberOfItems;
										++oneBasedIndexIntoDragItemList)
								{
									FlavorFlags		flavorFlags = 0L;
									ItemReference	dragItem = 0;
									
									
									GetDragItemReferenceNumber(dragRef, oneBasedIndexIntoDragItemList, &dragItem);
									
									// determine if this is a text item
									if (GetFlavorFlags(dragRef, dragItem, kDragFlavorTypeMacRomanText, &flavorFlags)
										!= noErr)
									{
										haveText = false;
										break;
									}
								}
							}
							
							// determine rules for accepting the drag
							acceptDrag = ((haveText) || (haveSingleFile));
							
						#if 0
							// if the drag can be accepted, remind the user where the cursor is
							// (as any drop will be sent to that location, not the mouse location)
							if (acceptDrag)
							{
								TerminalView_ZoomToCursor(TerminalWindow_ReturnViewWithFocus
															(Session_ReturnActiveTerminalWindow(session)),
															true/* quick animation */);
							}
						#endif
							
							// finally, update the event!
							result = SetEventParameter(inEvent, kEventParamControlWouldAcceptDrop,
														typeBoolean, sizeof(acceptDrag), &acceptDrag);
						}
					}
					break;
				
				case kEventControlDragWithin:
					TerminalView_SetDragHighlight(view, dragRef, true/* is highlighted */);
					result = noErr;
					break;
				
				case kEventControlDragLeave:
					TerminalView_SetDragHighlight(view, dragRef, false/* is highlighted */);
					result = noErr;
					break;
				
				case kEventControlDragReceive:
					// something was actually dropped
					result = sessionDragDrop(inHandlerCallRef, inEvent, session, view, dragRef);
					break;
				
				default:
					// ???
					break;
				}
			}
		}
	}
	
	return result;
}// receiveTerminalViewDragDrop


#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
/*!
Handles "kEventControlTrackingAreaEntered" of "kEventClassControl"
for a terminal view.

(3.1)
*/
static pascal OSStatus
receiveTerminalViewEntered		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					UNUSED_ARGUMENT(inSessionRef))
{
	OSStatus		result = eventNotHandledErr;
	//SessionRef		session = REINTERPRET_CAST(inSessionRef, SessionRef);
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassControl);
	assert(kEventKind == kEventControlTrackingAreaEntered);
	{
		Boolean		focusFollowsMouse = false;
		size_t		actualSize = 0L;
		
		
		// do not change focus unless the preference is set;
		// TEMPORARY: possibly inefficient to always check this here,
		// the alternative would be to update preferenceChanged() to
		// monitor changes to the preference and add/remove the
		// tracking handler as appropriate
		unless (Preferences_GetData(kPreferences_TagFocusFollowsMouse, sizeof(focusFollowsMouse),
									&focusFollowsMouse, &actualSize) ==
				kPreferences_ResultOK)
		{
			focusFollowsMouse = false; // assume a value, if preference can’t be found
		}
		
		// ignore this event if the application is not frontmost
		if (FlagManager_Test(kFlagSuspended))
		{
			focusFollowsMouse = false;
		}
		
		if (focusFollowsMouse)
		{
			HIViewRef	view = nullptr;
			
			
			// determine the view that is now under the mouse
			result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, view);
			if (noErr == result)
			{
				HIWindowRef		originalWindow = GetFrontWindowOfClass(kDocumentWindowClass, true/* must be showing */);
				HIWindowRef		oldWindow = GetUserFocusWindow();
				HIWindowRef		newWindow = HIViewGetWindow(view);
				
				
				if (newWindow != oldWindow)
				{
					OSStatus		error = noErr;
					WindowModality	oldWindowModality = kWindowModalityNone;
					HIWindowRef		oldParentWindow = nullptr;
					
					
					error = GetWindowModality(oldWindow, &oldWindowModality, &oldParentWindow);
					if (newWindow != oldParentWindow)
					{
						// only allow focus switching if the active window is a terminal
						// or a sheet on top of a terminal
						if (((nullptr != oldWindow) && TerminalWindow_ExistsFor(oldWindow)) ||
							((nullptr != oldParentWindow) && TerminalWindow_ExistsFor(oldParentWindow)))
						{
							HIWindowRef		newBlockingWindow = nullptr;
							
							
							if (HIWindowIsDocumentModalTarget(newWindow, &newBlockingWindow))
							{
								// the target has a sheet; the OS does not allow a background
								// sheet to become focused separately, so avoid switching unless
								// the request came from a window that had no sheet to begin
								// with (indicating a return to an already-frontmost sheet)
								if ((nullptr == oldParentWindow) &&
									(newWindow == originalWindow))
								{
									// the target has a sheet and the source did not, so it should be
									// possible to focus the sheet instead of the target frame
									HiliteWindow(newWindow, true); // but fix the parent frame anyway
									newWindow = newBlockingWindow;
								}
								else
								{
									focusFollowsMouse = false;
								}
							}
						}
						else
						{
							focusFollowsMouse = false;
						}
						
						if (focusFollowsMouse)
						{
							// highlight and focus, but do not bring to front, the view and its window
							if (nullptr != oldWindow) HiliteWindow(oldWindow, false);
							if (nullptr != oldParentWindow) HiliteWindow(oldParentWindow, false);
							HiliteWindow(newWindow, true);
							SetUserFocusWindow(newWindow);
						}
					}
				}
			}
		}
	}
	
	return result;
}// receiveTerminalViewEntered
#endif


/*!
Handles "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for a session’s terminal views.

(3.0)
*/
static pascal OSStatus
receiveTerminalViewTextInput	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
								 EventRef				inEvent,
								 void*					inSessionRef)
{
	OSStatus		result = eventNotHandledErr;
	SessionRef		session = REINTERPRET_CAST(inSessionRef, SessionRef);
	UInt32 const	kEventClass = GetEventClass(inEvent);
	UInt32 const	kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassTextInput);
	assert(kEventKind == kEventTextInputUnicodeForKeyEvent);
	{
		EventRef						originalKeyPressEvent = nullptr;
		EventInfoControlScope_KeyPress  controlKeyPressInfo;
		OSStatus						error = noErr;
		
		
		error = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamTextInputSendKeyboardEvent, typeEventRef,
														originalKeyPressEvent);
		assert(error == noErr);
		
		// TEMPORARY - just fill in the old structure and invoke the original
		// callback, until eventually this handler does all the work itself
		bzero(&controlKeyPressInfo, sizeof(controlKeyPressInfo));
		
		{
			// determine the control in question
			//???error = CarbonEventUtilities_GetEventParameter
			//???		(originalKeyPressEvent, kEventParamDirectObject, typeControlRef, controlKeyPressInfo.control);
			controlKeyPressInfo.control = TerminalView_ReturnUserFocusHIView
											(TerminalWindow_ReturnViewWithFocus
												(Session_ReturnActiveTerminalWindow(session)));
			assert(controlKeyPressInfo.control != nullptr);
		}
		
		{
			UInt32		modifiers = 0;
			
			
			// determine the modifier key states question
			error = CarbonEventUtilities_GetEventParameter(originalKeyPressEvent, kEventParamKeyModifiers, typeUInt32, modifiers);
			if (error == noErr)
			{
				controlKeyPressInfo.commandDown = ((modifiers & cmdKey) != 0);
				controlKeyPressInfo.controlDown = ((modifiers & controlKey) != 0);
				controlKeyPressInfo.optionDown = ((modifiers & optionKey) != 0);
				controlKeyPressInfo.shiftDown = ((modifiers & shiftKey) != 0);
			}
		}
		
		{
			// determine the character codes
			UInt32		numberOfCharacters = 0;
			UInt8		characterCodes[2] = { '\0', '\0' };
			
			
			error = CarbonEventUtilities_GetEventParameterVariableSize
					(originalKeyPressEvent, kEventParamKeyMacCharCodes, typeChar, characterCodes, numberOfCharacters);
			if ((error == noErr) && (numberOfCharacters > 0))
			{
				controlKeyPressInfo.characterCode = characterCodes[0];
				
				// filter out tab key presses, they have significance to terminals
				if (characterCodes[0] == 0x09)
				{
					result = noErr; // event is completely handled
				}
			}
		}
		
		{
			// determine the virtual key code
			UInt32		keyCode = '\0';
			
			
			error = CarbonEventUtilities_GetEventParameter
					(originalKeyPressEvent, kEventParamKeyCode, typeUInt32, keyCode);
			if (error == noErr)
			{
				controlKeyPressInfo.virtualKeyCode = keyCode;
			}
		}
		
		// perform character-based key or character mappings
		switch (controlKeyPressInfo.characterCode)
		{
		case 3:
			// corrects known bug in system key mapping
			// NOTE: This is based on old NCSA Telnet code.  Does this even *matter* anymore?
			if (controlKeyPressInfo.virtualKeyCode == 0x34)
			{
				// fix for PowerBook 540’s bad KCHR; map control-C to Return
				controlKeyPressInfo.characterCode = 13;
			}
			break;
		
		case '2':
			// corrects known bug in system key mapping
			// NOTE: This is based on old NCSA Telnet code.  Does this even *matter* anymore?
			if ((controlKeyPressInfo.controlDown) && (controlKeyPressInfo.shiftDown))
			{
				// fix bad KCHR control-@
				controlKeyPressInfo.characterCode = 0;
			}
			break;
		
		case '6':
			// corrects known bug in system key mapping
			// NOTE: This is based on old NCSA Telnet code.  Does this even *matter* anymore?
			if ((controlKeyPressInfo.controlDown) && (controlKeyPressInfo.shiftDown))
			{
				// fix bad KCHR control-^
				controlKeyPressInfo.characterCode = 0x1E;
			}
			break;
		
		case '`':
			// user-defined key mapping
			{
				SessionAutoLocker	ptr(gSessionPtrLocks(), session);
				
				
				if ((false == controlKeyPressInfo.commandDown) &&
					(ptr->preferencesCache.remapBackquoteToEscape))
				{
					controlKeyPressInfo.characterCode = 0x1B/* escape */;
				}
			}
			break;
		
		case '@':
		case 32:
			// this, along with the fixed KCHR that maps a ^-@ to
			// a @, takes care of Apple not posting null key values
			// NOTE: This is based on old NCSA Telnet code.  Does this even *matter* anymore?
			if (controlKeyPressInfo.controlDown)
			{
				controlKeyPressInfo.characterCode = '\0';
			}
			break;
		
		default:
			// all others are fine
			break;
		}
		
		// perform key-based key or character mappings
		switch (controlKeyPressInfo.virtualKeyCode)
		{
		case 0x33: // backspace
			// handle mapping BS to DEL, flipping on option-delete or command-delete
			{
				enum
				{
					ASCII_BACKSPACE		= 0x08,
					ASCII_DELETE		= 0x7F
				};
				if (Session_ConnectionDataPtr(session)->bsdel)
				{
					if ((controlKeyPressInfo.optionDown) || (controlKeyPressInfo.commandDown))
					{
						controlKeyPressInfo.characterCode = ASCII_BACKSPACE;
						
						// clear these modifier keys, because they were only used
						// to alter the effective character and should not have
						// any other special significance
						controlKeyPressInfo.optionDown = false;
						controlKeyPressInfo.commandDown = false;
					}
					else
					{
						controlKeyPressInfo.characterCode = ASCII_DELETE;
					}
				}
				else
				{
					if ((controlKeyPressInfo.optionDown) || (controlKeyPressInfo.commandDown))
					{
						controlKeyPressInfo.characterCode = ASCII_DELETE;
						
						// clear these modifier keys, because they were only used
						// to alter the effective character and should not have
						// any other special significance
						controlKeyPressInfo.optionDown = false;
						controlKeyPressInfo.commandDown = false;
					}
					else
					{
						controlKeyPressInfo.characterCode = ASCII_BACKSPACE;
					}
				}
			}
			break;
		
		default:
			// all others are fine
			break;
		}
		
		// hide the cursor until the mouse moves
		ObscureCursor();
		
		// now invoke old callback
		handleSessionKeyDown(nullptr/* unused listener model */, kEventLoop_ControlEventKeyPress,
								&controlKeyPressInfo/* event context */, session/* listener context */);
	}
	
	return result;
}// receiveTerminalViewTextInput


/*!
Handles "kEventWindowClose" of "kEventClassWindow"
for a session’s terminal window.

(3.0)
*/
static pascal OSStatus
receiveWindowClosing	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
						 EventRef				inEvent,
						 void*					inSessionRef)
{
	OSStatus			result = eventNotHandledErr;
	SessionRef			session = REINTERPRET_CAST(inSessionRef, SessionRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowClose);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			if (Session_StateIsDead(session))
			{
				// connection is already broken; close the window and clean up
				Session_Disconnect(session);
			}
			else 
			{
				// connection is still open; disconnect first and (if the user
				// has the preference set) close the window automatically;
				// a confirmation message is displayed UNLESS it has been less
				// than a few seconds since the connection opened, in which
				// case MacTelnet assumes the user doesn’t care and bypasses
				// the confirmation message (what can I say, I’m a nice guy)
				Session_DisplayTerminationWarning(session);
			}
			
			result = noErr; // event is completely handled
		}
	}
	
	return result;
}// receiveWindowClosing


/*!
Embellishes "kEventWindowFocusAcquired" of "kEventClassWindow"
for a session’s terminal window, by clearing any notifications
set on the session.

(3.1)
*/
static pascal OSStatus
receiveWindowFocusChange	(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
							 EventRef				inEvent,
							 void*					inSessionRef)
{
	OSStatus			result = eventNotHandledErr;
	SessionRef			session = REINTERPRET_CAST(inSessionRef, SessionRef);
	UInt32 const		kEventClass = GetEventClass(inEvent);
	UInt32 const		kEventKind = GetEventKind(inEvent);
	
	
	assert(kEventClass == kEventClassWindow);
	assert(kEventKind == kEventWindowFocusAcquired);
	{
		WindowRef	window = nullptr;
		
		
		// determine the window in question
		result = CarbonEventUtilities_GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, window);
		
		// if the window was found, proceed
		if (result == noErr)
		{
			SessionAutoLocker	ptr(gSessionPtrLocks(), session);
			
			
			watchClearForSession(ptr);
			
			result = eventNotHandledErr; // pass event to parent handler
		}
	}
	
	return result;
}// receiveWindowFocusChange


/*!
Sends Apple Events back to MacTelnet that instruct
it to send the specified data to the frontmost
connection ("connection 1").  If desired, you can
simultaneously execute the send (although it is
usually faster to execute separately, or perhaps
you may not want to execute at all).

Currently, this is implemented backwards by calling
the non-CFString version, which might result in
incorrect translation and unnecessary copying.  This
will be fixed later.

(3.0)
*/
static void
sendRecordableDataTransmitEvent	(CFStringRef	inStringToType,
								 Boolean		inExecute)
{
	// TEMPORARY - the CFString implementation should be preferred;
	//             translating CF data into a regular byte array is backwards
	Handle		buffer = Memory_NewHandle(CFStringGetLength(inStringToType) * sizeof(UniChar));
	
	
	if (nullptr != buffer)
	{
		CFIndex		actualSize = 0;
		CFIndex		numberOfCharactersCopied = CFStringGetBytes(inStringToType,
																CFRangeMake(0, CFStringGetLength(inStringToType)),
																kCFStringEncodingMacRoman/* TEMPORARY - should be session-dependent */,
																0/* loss byte, or 0 for no lossy conversion */,
																false/* is external representation */,
																REINTERPRET_CAST(*buffer, UInt8*),
																GetHandleSize(buffer), &actualSize);
		
		
		if (numberOfCharactersCopied < CFStringGetLength(inStringToType))
		{
			// error...
		}
		if (actualSize > GetHandleSize(buffer))
		{
			// error...
		}
		sendRecordableDataTransmitEvent(*buffer, GetHandleSize(buffer), inExecute);
		Memory_DisposeHandle(&buffer);
	}
}// sendRecordableDataTransmitEvent


/*!
Sends Apple Events back to MacTelnet that instruct
it to send the specified data to the frontmost
connection ("connection 1").  If desired, you can
simultaneously execute the send (although it is
usually faster to execute separately, or perhaps
you may not want to execute at all).

(3.0)
*/
static void
sendRecordableDataTransmitEvent		(char const*	inStringToType,
									 SInt16			inStringLength,
									 Boolean		inExecute)
{
	OSStatus	error = noErr;
	AEDesc		dataSendEvent;
	AEDesc		replyDescriptor;
	
	
	RecordAE_CreateRecordableAppleEvent(kMySuite, kTelnetEventIDDataSend, &dataSendEvent);
	if (error == noErr)
	{
		Boolean		sendNewLine = false;
		SInt16		stringLength = inStringLength;
		AEDesc		containerDesc;
		AEDesc		keyDesc;
		AEDesc		objectDesc;
		AEDesc		textDesc;
		
		
		(OSStatus)AppleEventUtilities_InitAEDesc(&containerDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&keyDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&objectDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&textDesc);
		
		// clip newlines and turn them into "with newline" parameters (newlines
		// might be represented by \n, \r, "\012", "\015", or "\015\012"; look
		// for any of these and decrement the length to prevent including them
		// in the recorded string)
		if ((stringLength >= 1) && inStringToType[stringLength - 1] == '\n')
		{
			Console_WriteLine("\\n: removing");
			--stringLength;
			sendNewLine = true;
		}
		if ((stringLength >= 1) && inStringToType[stringLength - 1] == '\r')
		{
			Console_WriteLine("\\r: removing");
			--stringLength;
			sendNewLine = true;
		}
		if ((stringLength >= 4) &&
					(inStringToType[stringLength - 1] == '2') &&
					(inStringToType[stringLength - 2] == '1') &&
					(inStringToType[stringLength - 2] == '0') &&
					(inStringToType[stringLength - 3] == '\\'))
		{
			Console_WriteLine("\\012: removing");
			stringLength -= 4;
			sendNewLine = true;
		}
		if ((stringLength >= 4) &&
					(inStringToType[stringLength - 1] == '5') &&
					(inStringToType[stringLength - 2] == '1') &&
					(inStringToType[stringLength - 2] == '0') &&
					(inStringToType[stringLength - 3] == '\\'))
		{
			Console_WriteLine("\\015: removing");
			stringLength -= 4;
			sendNewLine = true;
		}
		
		// create a descriptor containing the given string
		error = AECreateDesc(cString, inStringToType, stringLength * sizeof(char), &textDesc);
		if (error != noErr) Console_WriteValue("text descriptor creation error", error);
		
		// send a request for "session window <index>", which resides in a null container
		(OSStatus)BasicTypesAE_CreateSInt32Desc(1/* frontmost window */, &keyDesc);
		error = CreateObjSpecifier(cMySessionWindow, &containerDesc,
									formAbsolutePosition, &keyDesc, true/* dispose inputs */,
									&objectDesc);
		if (error != noErr) Console_WriteValue("connection access error", error);
		
		if (error == noErr)
		{
			// the data to transmit - REQUIRED
			(OSStatus)AEPutParamDesc(&dataSendEvent, keyDirectObject, &textDesc);
			
			// reference to the connection where the data will be sent - REQUIRED
			(OSStatus)AEPutParamDesc(&dataSendEvent, keyAEParamMyToDestination, &objectDesc);
			
			if (sendNewLine)
			{
				// add a parameter to send a newline
				(OSStatus)AEPutParamPtr(&dataSendEvent, keyAEParamMyNewline,
										typeBoolean, &sendNewLine, sizeof(sendNewLine));
			}
			
			// send the event, which will record it into any open script;
			// this event is only executed if requested to do so by the caller
			{
				UInt32		flags = kAENoReply | kAECanInteract;
				
				
				unless (inExecute) flags |= kAEDontExecute;
				(OSStatus)AESend(&dataSendEvent, &replyDescriptor,
								flags, kAENormalPriority, kAEDefaultTimeout, nullptr/* idle routine */,
								nullptr/* filter routine */);
			}
		}
		AEDisposeDesc(&textDesc);
		AEDisposeDesc(&objectDesc);
	}
	AEDisposeDesc(&dataSendEvent);
}// sendRecordableDataTransmitEvent


/*!
Sends Apple Events back to MacTelnet that instruct
it to send the specified special character to the
frontmost connection ("connection 1").  If desired,
you can simultaneously execute the send (although
it is usually faster to execute separately, or
perhaps you may not want to execute at all).

The value of "inSpecialCharacterEnumeration" is an
enumeration (see "Terminology.h") identifying the
special character to send.  This routine directly
sends its value as a parameter to the event, no
error checking is done.

(3.0)
*/
static void
sendRecordableSpecialTransmitEvent	(FourCharCode	inSpecialCharacterEnumeration,
									 Boolean		inExecute)
{
	OSStatus	error = noErr;
	AEDesc		dataSendEvent;
	AEDesc		replyDescriptor;
	
	
	RecordAE_CreateRecordableAppleEvent(kMySuite, kTelnetEventIDDataSend, &dataSendEvent);
	if (error == noErr)
	{
		AEDesc		containerDesc;
		AEDesc		keyDesc;
		AEDesc		objectDesc;
		AEDesc		specialCharacterDesc;
		
		
		(OSStatus)AppleEventUtilities_InitAEDesc(&containerDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&keyDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&objectDesc);
		(OSStatus)AppleEventUtilities_InitAEDesc(&specialCharacterDesc);
		
		// create a descriptor containing the given enumeration
		error = AECreateDesc(typeEnumerated, &inSpecialCharacterEnumeration,
								sizeof(inSpecialCharacterEnumeration), &specialCharacterDesc);
		if (error != noErr) Console_WriteValue("special character descriptor creation error", error);
		
		// send a request for "session window <index>", which resides in a null container
		(OSStatus)BasicTypesAE_CreateSInt32Desc(1/* frontmost window */, &keyDesc);
		error = CreateObjSpecifier(cMySessionWindow, &containerDesc,
									formAbsolutePosition, &keyDesc, true/* dispose inputs */,
									&objectDesc);
		if (error != noErr) Console_WriteValue("connection access error", error);
		
		if (error == noErr)
		{
			// the data to transmit - REQUIRED
			(OSStatus)AEPutParamDesc(&dataSendEvent, keyDirectObject, &specialCharacterDesc);
			
			// reference to the connection where the data will be sent - REQUIRED
			(OSStatus)AEPutParamDesc(&dataSendEvent, keyAEParamMyToDestination, &objectDesc);
			
			// send the event, which will record it into any open script;
			// this event is only executed if requested to do so by the caller
			{
				UInt32		flags = kAENoReply | kAECanInteract;
				
				
				unless (inExecute) flags |= kAEDontExecute;
				(OSStatus)AESend(&dataSendEvent, &replyDescriptor,
									flags, kAENormalPriority, kAEDefaultTimeout, nullptr/* idle routine */,
									nullptr/* filter routine */);
			}
		}
		AEDisposeDesc(&specialCharacterDesc);
		AEDisposeDesc(&objectDesc);
	}
	AEDisposeDesc(&dataSendEvent);
}// sendRecordableSpecialTransmitEvent


/*!
Responds to "kEventControlDragReceive" of "kEventClassControl"
for a terminal view that is associated with a session.

See receiveTerminalViewDragDrop().

(3.1)
*/
static OSStatus
sessionDragDrop		(EventHandlerCallRef	UNUSED_ARGUMENT(inHandlerCallRef),
					 EventRef				inEvent,
					 SessionRef				inRef,
					 HIViewRef				inHIView,
					 DragRef				inDrag)
{
	OSStatus			result = eventNotHandledErr;
	TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(GetControlOwner(inHIView));
	Point				mouseLocation;
	Point				globalPinnedMouseLocation;
	
	
	// see if the drop occurred within the terminal window area
	// (otherwise the drop may have been destined for, say, the toolbar)
	if (GetDragMouse(inDrag, &mouseLocation, &globalPinnedMouseLocation) == noErr)
	{
		// translate mouse coordinates to be local to the drop window
		{
			CGrafPtr	oldPort = nullptr;
			GDHandle	oldDevice = nullptr;
			
			
			GetGWorld(&oldPort, &oldDevice);
			SetPortWindowPort(GetControlOwner(inHIView));
			GlobalToLocal(&mouseLocation);
			SetGWorld(oldPort, oldDevice);
		}
		
		// is the drop in the terminal area?
		if (TerminalWindow_EventInside(terminalWindow, inEvent))
		{
			Handle		textToInsert = nullptr;
			Boolean		cursorMovesPriorToDrops = false;
			
			
			// retrieve the text (if text), or construct text (if a file)
			if (DragAndDrop_DragIsExactlyOneFile(inDrag))
			{
				// file drag; figure out the Unix pathname and insert that as if it were typed
				HFSFlavor		fileInfo;
				Size			dataSize = 0;
				ItemReference	draggedFile = 0;
				
				
				GetDragItemReferenceNumber(inDrag, 1/* index */, &draggedFile);
				result = GetFlavorDataSize(inDrag, draggedFile, kDragFlavorTypeHFS, &dataSize);
				if (result == noErr)
				{
					Str255		pathname;
					
					
					// get the file information
					dataSize = sizeof(fileInfo);
					GetFlavorData(inDrag, draggedFile, kDragFlavorTypeHFS, &fileInfo,
									&dataSize, 0L/* offset */);
					
					// construct a POSIX path for the HFS file
					result = FileUtilities_GetPOSIXPathnameFromFSSpec
							(&fileInfo.fileSpec, pathname, false/* is directory */);
					
					// copy the data into a handle, since that’s what’s used for other text drags
					if (result != noErr) Console_WriteValue("pathname error", result);
					else
					{
						// assume the user is running a shell, and escape the pathname
						// from the shell by surrounding it with apostrophes (the new
						// buffer must be large enough to hold 2 apostrophes)
						textToInsert = Memory_NewHandle((2 + PLstrlen(pathname)) * sizeof(unsigned char));
						if (textToInsert != nullptr)
						{
							(*textToInsert)[0] = '\'';
							BlockMoveData(1 + pathname, 1 + *textToInsert,
											PLstrlen(pathname) * sizeof(unsigned char));
							(*textToInsert)[1 + PLstrlen(pathname)] = '\'';
						}
						Console_WriteValuePString("dropped path", pathname);
						
						// success!
						result = noErr;
					}
				}
			}
			else if (DragAndDrop_DragContainsOnlyTextItems(inDrag))
			{
				// text drag; combine all text items and insert the block as if it were typed
				if (DragAndDrop_GetDraggedTextAsNewHandle(inDrag, &textToInsert) != noErr)
				{
					textToInsert = nullptr; // invalidate handle
				}
			}
			else
			{
				// ???
				Sound_StandardAlert();
			}
			
			if (textToInsert != nullptr)
			{
				size_t	actualSize = 0L;
				
				
				// if the user preference is set, first move the cursor to the drop location
				unless (Preferences_GetData(kPreferences_TagCursorMovesPriorToDrops,
											sizeof(cursorMovesPriorToDrops), &cursorMovesPriorToDrops,
											&actualSize) == kPreferences_ResultOK)
				{
					cursorMovesPriorToDrops = false; // assume a value, if a preference can’t be found
				}
				if (cursorMovesPriorToDrops)
				{
					// move cursor based on the local point
					TerminalView_MoveCursorWithArrowKeys(TerminalWindow_ReturnViewWithFocus(terminalWindow),
															mouseLocation);
				}
				
				// “type” the text
				HLock(textToInsert);
				{
					Session_UserInputString(inRef, *textToInsert, GetHandleSize(textToInsert), false/* record */);
					
					// success!
					result = noErr;
				}
				HUnlock(textToInsert);
				
				// clean up
				Memory_DisposeHandle(&textToInsert);
			}
		}
	}
	return result;
}// sessionDragDrop


/*!
Invoked whenever a monitored terminal window state is
changed (see where TerminalWindow_New() is called for
a session, to see which states are subsequently
monitored).  This routine responds by notifying the
processes running in a session window that the size
of the terminal screen is now different.

(3.0)
*/
static void
terminalWindowChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	inTerminalWindowSettingThatChanged,
						 void*					inEventContextPtr,
						 void*					inListenerContextPtr)
{
	SessionRef	session = REINTERPRET_CAST(inListenerContextPtr, SessionRef);
	
	
	switch (inTerminalWindowSettingThatChanged)
	{
	case kTerminalWindow_ChangeObscuredState:
		// notify listeners of this session that the obscured state has changed
		{
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inEventContextPtr, TerminalWindowRef);
			Boolean				error = false;
			
			
			if (nullptr == terminalWindow) error = true;
			else
			{
				if (nullptr == session) error = true;
				else
				{
					SessionAutoLocker	ptr(gSessionPtrLocks(), session);
					
					
					changeNotifyForSession(ptr, kSession_ChangeWindowObscured, session);
				}
			}
			
			if (error)
			{
				//Console_WriteLine("warning, unable to notify listeners of obscured-state change");
			}
		}
		break;
	
	case kTerminalWindow_ChangeScreenDimensions:
		// tell processes running in the window about the new screen size
		{
			TerminalWindowRef	terminalWindow = REINTERPRET_CAST(inEventContextPtr, TerminalWindowRef);
			Boolean				error = false;
			
			
			if (terminalWindow == nullptr) error = true;
			else
			{
				if (session == nullptr) error = true;
				else
				{
					SessionAutoLocker	ptr(gSessionPtrLocks(), session);
					UInt16				newColumnCount = 0;
					UInt16				newRowCount = 0;
					
					
					// determine new size
					TerminalWindow_GetScreenDimensions(terminalWindow, &newColumnCount, &newRowCount);
					
					// send an I/O control message to the TTY informing it that the screen size has changed
					if (nullptr != ptr->mainProcess)
					{
						Local_SendTerminalResizeMessage(Local_ProcessReturnMasterTerminal(ptr->mainProcess),
														newColumnCount, newRowCount,
														0/* tmp - not easy to tell pixel width here */,
														0/* tmp - not easy to tell pixel width here */);
					}
				}
			}
			
			if (error)
			{
				// warn the user somehow?
				// unimplemented
				Console_WriteLine("warning, unable to transmit screen dimension changes to window processes");
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// terminalWindowChanged


/*!
The responder to a closed “terminate session?” alert.
This routine disconnects and/or closes a session if
the item hit is the OK button, otherwise it does not
touch the session window in any way.  The given alert
is destroyed.

(3.0)
*/
static void
terminationWarningCloseNotifyProc	(InterfaceLibAlertRef	inAlertThatClosed,
									 SInt16					inItemHit,
									 void*					inTerminateAlertInfoPtr)
{
	TerminateAlertInfoPtr	dataPtr = REINTERPRET_CAST(inTerminateAlertInfoPtr, TerminateAlertInfoPtr);
	
	
	if (dataPtr != nullptr)
	{
		SessionAutoLocker				sessionPtr(gSessionPtrLocks(), dataPtr->sessionBeingTerminated);
		SessionCloseWarningButtonInfo	info;
		
		
		// clear the alert status attribute to the session
		changeStateAttributes(sessionPtr, 0/* attributes to set */,
								kSession_StateAttributeOpenDialog/* attributes to clear */);
		
		// notify listeners
		info.session = dataPtr->sessionBeingTerminated;
		info.buttonHit = inItemHit;
		changeNotifyForSession(sessionPtr, kSession_ChangeCloseWarningAnswered, &info/* context */);
		
		if (inItemHit == kAlertStdAlertOKButton)
		{
			// implicitly update the toolbar visibility preference based
			// on the toolbar visibility of this closing window
			{
				Boolean		toolbarHidden = (false == IsWindowToolbarVisible(sessionPtr->dataPtr->window));
				
				
				(Preferences_Result)Preferences_SetData(kPreferences_TagHeadersCollapsed,
														sizeof(toolbarHidden), &toolbarHidden);
			}
			
			// temporary - eventually a Session API will know how to close properly
			// close local shell
			Session_SetState(dataPtr->sessionBeingTerminated, kSession_StateDead);
			
			// decide whether to dispose of the terminal window immediately
			{
				Boolean		leaveWindowOpen = false;
				size_t		actualSize = 0L;
				
				
				// if preference is set, close window immediately;
				// otherwise, leave the window open
				unless (Preferences_GetData(kPreferences_TagDontAutoClose, sizeof(leaveWindowOpen),
											&leaveWindowOpen, &actualSize) ==
						kPreferences_ResultOK)
				{
					leaveWindowOpen = false; // assume windows automatically close, if preference can’t be found
				}
				
				if (leaveWindowOpen)
				{
					// make window look dead, but leave it open for now
					changeNotifyForSession(sessionPtr, kSession_ChangeState, dataPtr->sessionBeingTerminated/* context */);
				}
				else
				{
					// close window, clean up, etc.
					Session_Disconnect(dataPtr->sessionBeingTerminated);
				}
			}
		}
		
		// clean up
		sessionPtr->currentTerminationAlert = nullptr; // now it is legal to display a different warning sheet
		Memory_DisposePtr(REINTERPRET_CAST(&dataPtr, Ptr*));
	}
	
	// dispose of the alert
	Alert_StandardCloseNotifyProc(inAlertThatClosed, inItemHit, nullptr/* user data */);
}// terminationWarningCloseNotifyProc


/*!
Closes the global modeless alert for notifications on the
specified session, and clears any other indicators for a
triggered notification on that session.

(3.1)
*/
static void
watchClearForSession	(SessionPtr		inPtr)
{
	// note the change (e.g. can cause icon displays to be updated)
	changeStateAttributes(inPtr, 0/* attributes to set */,
							kSession_StateAttributeNotification/* attributes to clear */);
	
	// hide the global alert, if present
	if (nullptr != inPtr->watchBox)
	{
		Alert_Dispose(&inPtr->watchBox);
	}
}// watchClearForSession


/*!
The responder to a closed watch-notification alert.  The watch
trigger for the session (if any) is cleared if it has not been
cleared already.  The given alert is destroyed.

(3.1)
*/
static void
watchCloseNotifyProc	(AlertMessages_BoxRef	inAlertThatClosed,
						 SInt16					inItemHit,
						 void*					inMy_WatchAlertInfoPtr)
{
	My_WatchAlertInfoPtr	dataPtr = REINTERPRET_CAST(inMy_WatchAlertInfoPtr, My_WatchAlertInfoPtr);
	
	
	if (nullptr != dataPtr)
	{
		SessionAutoLocker	ptr(gSessionPtrLocks(), dataPtr->sessionBeingWatched);
		
		
		watchClearForSession(ptr);
		
		// clean up
		delete dataPtr, dataPtr = nullptr;
	}
	
	// dispose of the alert
	Alert_StandardCloseNotifyProc(inAlertThatClosed, inItemHit, nullptr/* user data */);
}// watchCloseNotifyProc


/*!
Displays the global modeless alert for notifications on the
specified session of the given type.

For watches that simply display alerts, this automatically does
nothing if the specified session is the user focus and this
application is frontmost.  It presumes that the user does not
need to see an alert about the window currently being used!

(3.1)
*/
static void
watchNotifyForSession	(SessionPtr		inPtr,
						 Session_Watch	inWhatTriggered)
{
	Boolean		canTrigger = (kSession_WatchForKeepAlive == inWhatTriggered)
								? true
								: ((SessionFactory_ReturnUserFocusSession() != inPtr->selfRef) ||
									FlagManager_Test(kFlagSuspended));
	
	
	if (canTrigger)
	{
		// note the change (e.g. can cause icon displays to be updated)
		changeStateAttributes(inPtr, kSession_StateAttributeNotification/* attributes to set */,
								0/* attributes to clear */);
		
		// create or update the global alert
		switch (inWhatTriggered)
		{
		case kSession_WatchForPassiveData:
		case kSession_WatchForInactivity:
			{
				My_WatchAlertInfoPtr	watchAlertInfoPtr = new My_WatchAlertInfo;
				CFStringRef				dialogTextCFString = nullptr;
				UIStrings_Result		stringResult = kUIStrings_ResultOK;
				
				
				// basic setup
				watchAlertInfoPtr->sessionBeingWatched = inPtr->selfRef;
				if (nullptr == inPtr->watchBox)
				{
					inPtr->watchBox = Alert_New();
					Alert_SetParamsFor(inPtr->watchBox, kAlert_StyleOK);
					Alert_SetType(inPtr->watchBox, kAlertNoteAlert);
				}
				
				// set message based on watch type
				if (kSession_WatchForInactivity == inWhatTriggered)
				{
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowNotifyInactivityPrimaryText, dialogTextCFString);
				}
				else
				{
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowNotifyActivityPrimaryText, dialogTextCFString);
				}
				if (stringResult.ok())
				{
					Alert_SetTextCFStrings(inPtr->watchBox, dialogTextCFString, CFSTR(""));
					CFRelease(dialogTextCFString), dialogTextCFString = nullptr;
				}
				
				// show the message; it is disposed asynchronously
				Alert_MakeModeless(inPtr->watchBox, watchCloseNotifyProc, watchAlertInfoPtr/* context */);
				Alert_Display(inPtr->watchBox);
			}
			break;
		
		case kSession_WatchForKeepAlive:
			{
				std::string		transmission = Quills::Session::keep_alive_transmission();
				
				
				Session_UserInputString(inPtr->selfRef, transmission.c_str(), transmission.size());
			}
			break;
		
		default:
			// ???
			break;
		}
	}
}// watchNotifyForSession


/*!
This is invoked after a period of inactivity, and simply sends
notification that the session is now inactive (from the point of
view of the user).  This is not to be confused with the session
active state.

Timers that draw must save and restore the current graphics port.

(3.1)
*/
static pascal void
watchNotifyFromTimer	(EventLoopTimerRef		UNUSED_ARGUMENT(inTimer),
						 void*					inSessionRef)
{
	SessionRef			ref = REINTERPRET_CAST(inSessionRef, SessionRef);
	SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	
	
	watchNotifyForSession(ptr, ptr->activeWatch);
}// watchNotifyFromTimer


/*!
If a watch is currently active on the specified session and
the watch is of a type that fires periodically (idle timers),
this routine will reset the timer.  In other words, do this
when you want to start waiting for the precise duration of
the watch (usually, immediately after data arrives or some
other event indicates the watch should start over).

Has no effect for other kinds of watches.

(3.1)
*/
static void
watchTimerResetForSession	(SessionPtr		inPtr,
							 Session_Watch	inWatchType)
{
	if ((kSession_WatchForKeepAlive == inWatchType) ||
		(kSession_WatchForInactivity == inWatchType))
	{
		// an arbitrary length of dead time must elapse before a session
		// is considered inactive and triggers a notification
		// TEMPORARY - should this time be user-configurable?
		EventTimerInterval const	kTimeBeforeInactive = (kSession_WatchForKeepAlive == inWatchType)
															? kEventDurationMinute * 10
															: kEventDurationSecond * 30;
		OSStatus					error = noErr;
		
		
		// install or reset timer to trigger the no-activity notification when appropriate
		assert(nullptr != inPtr->inactivityWatchTimer);
		error = SetEventLoopTimerNextFireTime(inPtr->inactivityWatchTimer, kTimeBeforeInactive);
		assert_noerr(error);
	}
}// watchTimerResetForSession


/*!
Invoked whenever a monitored session’s window has just
been created, or is about to be destroyed.  This routine
responds by installing or removing window-dependent event
handlers for the session.

(3.0)
*/
static void
windowValidationStateChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
								 ListenerModel_Event	inSessionChange,
								 void*					inEventContextPtr,
								 void*					UNUSED_ARGUMENT(inListenerContextPtr))
{
	switch (inSessionChange)
	{
	case kSession_ChangeWindowValid:
		// install window-dependent event handlers
		{
			SessionRef			session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			SessionAutoLocker	ptr(gSessionPtrLocks(), session);
			
			
			// install a callback that disposes of the window properly when it should be closed
			{
				EventTypeSpec const		whenWindowClosing[] =
										{
											{ kEventClassWindow, kEventWindowClose }
										};
				OSStatus				error = noErr;
				
				
				ptr->windowClosingUPP = NewEventHandlerUPP(receiveWindowClosing);
				error = InstallWindowEventHandler(TerminalWindow_ReturnWindow(ptr->terminalWindow),
													ptr->windowClosingUPP, GetEventTypeCount(whenWindowClosing),
													whenWindowClosing, session/* user data */,
													&ptr->windowClosingHandler/* event handler reference */);
				assert(error == noErr);
			}
			
			// install a callback that clears notifications when the window is activated
			{
				EventTypeSpec const		whenWindowFocusChanged[] =
										{
											{ kEventClassWindow, kEventWindowFocusAcquired }
										};
				OSStatus				error = noErr;
				
				
				ptr->windowFocusChangeUPP = NewEventHandlerUPP(receiveWindowFocusChange);
				error = InstallWindowEventHandler(TerminalWindow_ReturnWindow(ptr->terminalWindow),
													ptr->windowFocusChangeUPP, GetEventTypeCount(whenWindowFocusChanged),
													whenWindowFocusChanged, session/* user data */,
													&ptr->windowFocusChangeHandler/* event handler reference */);
				assert(error == noErr);
			}
			
			ptr->terminalWindowListener = ListenerModel_NewStandardListener(terminalWindowChanged, session/* context */);
			TerminalWindow_StartMonitoring(ptr->terminalWindow, kTerminalWindow_ChangeScreenDimensions,
											ptr->terminalWindowListener);
			TerminalWindow_StartMonitoring(ptr->terminalWindow, kTerminalWindow_ChangeObscuredState,
											ptr->terminalWindowListener);
			{
				// install a listener for keystrokes on each view’s control;
				// in the future, terminal windows may have multiple views,
				// which can be focused independently
				UInt16				viewCount = TerminalWindow_ReturnViewCount(ptr->terminalWindow);
				TerminalViewRef*	viewArray = REINTERPRET_CAST(Memory_NewPtr(viewCount * sizeof(TerminalViewRef)),
																	TerminalViewRef*);
				
				
				if (viewArray != nullptr)
				{
					HIViewRef			userFocusView = nullptr;
					HIViewRef			dragFocusView = nullptr;
					register SInt16		i = 0;
					OSStatus			error = noErr;
					
					
					ptr->terminalViewDragDropUPP = NewEventHandlerUPP(receiveTerminalViewDragDrop);
					assert(nullptr != ptr->terminalViewDragDropUPP);
				#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
					ptr->terminalViewEnteredUPP = NewEventHandlerUPP(receiveTerminalViewEntered);
					assert(nullptr != ptr->terminalViewEnteredUPP);
				#endif
					ptr->terminalViewTextInputUPP = NewEventHandlerUPP(receiveTerminalViewTextInput);
					assert(nullptr != ptr->terminalViewTextInputUPP);
					TerminalWindow_GetViews(ptr->terminalWindow, viewCount, viewArray, &viewCount/* actual length */);
					for (i = 0; i < viewCount; ++i)
					{
						userFocusView = TerminalView_ReturnUserFocusHIView(viewArray[i]);
						dragFocusView = TerminalView_ReturnDragFocusHIView(viewArray[i]);
						
						// install a callback that responds to drag-and-drop in views,
						// and a callback that responds to key presses in views
						{
							EventTypeSpec const		whenTerminalViewTextInput[] =
													{
														{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent }
													};
						#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
							EventTypeSpec const		whenTerminalViewEntered[] =
													{
														{ kEventClassControl, kEventControlTrackingAreaEntered }
													};
						#endif
							EventTypeSpec const		whenTerminalViewDragDrop[] =
													{
														{ kEventClassControl, kEventControlDragEnter },
														{ kEventClassControl, kEventControlDragWithin },
														{ kEventClassControl, kEventControlDragLeave },
														{ kEventClassControl, kEventControlDragReceive }
													};
							
							
							// ensure keyboard input to this view is seen
							error = HIViewInstallEventHandler(userFocusView, ptr->terminalViewTextInputUPP,
																GetEventTypeCount(whenTerminalViewTextInput),
																whenTerminalViewTextInput, session/* user data */,
																&ptr->terminalViewTextInputHandlers[userFocusView]);
							assert_noerr(error);
							
						#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
							// if possible, allow focus-follows-mouse
							error = HIViewInstallEventHandler(dragFocusView, ptr->terminalViewEnteredUPP,
																GetEventTypeCount(whenTerminalViewEntered),
																whenTerminalViewEntered, session/* user data */,
																&ptr->terminalViewEnteredHandlers[dragFocusView]);
							assert_noerr(error);
							{
								HIViewTrackingAreaRef	ignoredRef = nullptr;
								
								
								error = HIViewNewTrackingArea(dragFocusView, nullptr/* shape */, 0/* ID */,
																&ignoredRef);
								assert_noerr(error);
							}
						#endif
							
							// ensure drags to this view are seen
							error = HIViewInstallEventHandler(dragFocusView, ptr->terminalViewDragDropUPP,
																GetEventTypeCount(whenTerminalViewDragDrop),
																whenTerminalViewDragDrop, session/* user data */,
																&ptr->terminalViewDragDropHandlers[dragFocusView]);
							assert_noerr(error);
							error = SetControlDragTrackingEnabled(dragFocusView, true/* is drag enabled */);
							assert_noerr(error);
						}
						
						// enable drag tracking for the window, if it is not enabled already
						error = SetAutomaticControlDragTrackingEnabledForWindow
								(GetControlOwner(dragFocusView), true/* is drag enabled */);
						assert_noerr(error);
					}
					Memory_DisposePtr(REINTERPRET_CAST(&viewArray, Ptr*));
				}
			}
		}
		break;
	
	case kSession_ChangeWindowInvalid:
		// remove window-dependent event handlers
		{
			SessionRef			session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			SessionAutoLocker	ptr(gSessionPtrLocks(), session);
			
			
			RemoveEventHandler(ptr->windowClosingHandler), ptr->windowClosingHandler = nullptr;
			DisposeEventHandlerUPP(ptr->windowClosingUPP), ptr->windowClosingUPP = nullptr;
			RemoveEventHandler(ptr->windowFocusChangeHandler), ptr->windowFocusChangeHandler = nullptr;
			DisposeEventHandlerUPP(ptr->windowFocusChangeUPP), ptr->windowFocusChangeUPP = nullptr;
			TerminalWindow_StopMonitoring(ptr->terminalWindow, kTerminalWindow_ChangeScreenDimensions,
											ptr->terminalWindowListener);
			TerminalWindow_StopMonitoring(ptr->terminalWindow, kTerminalWindow_ChangeObscuredState,
											ptr->terminalWindowListener);
			{
				// remove listener from each view where one was installed
				UInt16				viewCount = TerminalWindow_ReturnViewCount(ptr->terminalWindow);
				TerminalViewRef*	viewArray = REINTERPRET_CAST(Memory_NewPtr(viewCount * sizeof(TerminalViewRef)),
																	TerminalViewRef*);
				
				
				if (viewArray != nullptr)
				{
					HIViewRef			userFocusView = nullptr;
					HIViewRef			dragFocusView = nullptr;
					register SInt16		i = 0;
					
					
					TerminalWindow_GetViews(ptr->terminalWindow, viewCount, viewArray, &viewCount/* actual length */);
					for (i = 0; i < viewCount; ++i)
					{
						userFocusView = TerminalView_ReturnUserFocusHIView(viewArray[i]);
						dragFocusView = TerminalView_ReturnDragFocusHIView(viewArray[i]);
						RemoveEventHandler(ptr->terminalViewTextInputHandlers[userFocusView]);
						RemoveEventHandler(ptr->terminalViewDragDropHandlers[dragFocusView]);
					#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
						RemoveEventHandler(ptr->terminalViewEnteredHandlers[dragFocusView]);
					#endif
					}
					DisposeEventHandlerUPP(ptr->terminalViewDragDropUPP), ptr->terminalViewDragDropUPP = nullptr;
					DisposeEventHandlerUPP(ptr->terminalViewEnteredUPP), ptr->terminalViewEnteredUPP = nullptr;
					DisposeEventHandlerUPP(ptr->terminalViewTextInputUPP), ptr->terminalViewTextInputUPP = nullptr;
					Memory_DisposePtr(REINTERPRET_CAST(&viewArray, Ptr*));
				}
			}
			ListenerModel_ReleaseListener(&ptr->terminalWindowListener);
		}
		break;
	
	default:
		// ???
		break;
	}
}// windowValidationStateChanged

// BELOW IS REQUIRED NEWLINE TO END FILE
