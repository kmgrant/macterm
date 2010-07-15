/*!	\file Session.cp
	\brief The front end to a particular running command
	or server connection.  This remains valid as long as
	the session is defined, even if the command has
	terminated or all its windows are temporarily hidden.
*/
/*###############################################################

	MacTelnet
		© 1998-2010 by Kevin Grant.
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
#include <CocoaBasic.h>
#include <Console.h>
#include <Cursors.h>
#include <FileSelectionDialogs.h>
#include <ListenerModel.h>
#include <Localization.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <Panel.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>
#include <StringUtilities.h>

// resource includes
#include "GeneralResources.h"
#include "SpacingConstants.r"

// MacTelnet includes
#include "AppResources.h"
#include "Clipboard.h"
#include "Commands.h"
#include "DialogUtilities.h"
#include "EventLoop.h"
#include "NetEvents.h"
#include "Preferences.h"
#include "PrefPanelSessions.h"
#include "PrefsContextDialog.h"
#include "QuillsSession.h"
#include "RasterGraphicsKernel.h"
#include "RasterGraphicsScreen.h"
#include "Session.h"
#include "SessionDescription.h"
#include "SessionFactory.h"
#include "Terminal.h"
#include "TerminalView.h"
#include "TerminalWindow.h"
#include "Terminology.h"
#include "UIStrings.h"
#include "VectorCanvas.h"
#include "VectorInterpreter.h"
#include "VTKeys.h"



#pragma mark Constants
namespace {

enum Session_Type
{
	kSession_TypeLocalNonLoginShell		= 0,	// local Unix process running the user’s preferred shell
	kSession_TypeLocalLoginShell		= 1		// local Unix process running /usr/bin/login
};

} // anonymous namespace

#pragma mark Types
namespace {

/*!
Obsolete data structure.
*/
struct My_KeyPress
{
	ControlRef		control;			//!< which control is focused
	SInt16			characterCode;		//!< code uniquifying the character corresponding to the key pressed
	SInt16			characterCode2;		//!< if nonzero, the key press represents a sequence of two characters to send
	SInt16			virtualKeyCode;		//!< code uniquifying the key pressed
	Boolean			commandDown;		//!< the state of the Command modifier key
	Boolean			controlDown;		//!< the state of the Control modifier key
	Boolean			optionDown;			//!< the state of the Option modifier key
	Boolean			shiftDown;			//!< the state of the Shift modifier key
};
typedef My_KeyPress*	My_KeyPressPtr;

typedef std::map< Session_PropertyKey, void* >		My_AuxiliaryDataByKey;

typedef std::vector< TerminalScreenRef >	My_CaptureFileList;

typedef std::map< HIViewRef, EventHandlerRef >		My_DragDropHandlerByView;

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

typedef std::vector< TerminalScreenRef >	My_PrintJobList;

typedef std::vector< SInt16 >				My_RasterGraphicsScreenList;

typedef std::vector< SInt16 >				My_TEKGraphicList;

typedef std::vector< TerminalScreenRef >	My_TerminalScreenList;

typedef std::map< HIViewRef, EventHandlerRef >	My_TextInputHandlerByView;

/*!
The data structure that is known as a "Session_Ref" to
any code outside this module.  See Session_New().
*/
struct My_Session
{
	My_Session		(Preferences_ContextRef, Boolean);
	~My_Session		();
	
	Preferences_ContextRef		configuration;				// not always in sync; see Session_ReturnConfiguration()
	Boolean						readOnly;					// whether or not user input is allowed
	Session_Type				kind;						// type of session; affects use of the union below
	Session_State				status;						// indicates whether session is initialized, etc.
	Session_StateAttributes		statusAttributes;			// “tags” for the status, above
	CFRetainRelease				statusString;				// one word (usually) describing the state succinctly
	CFRetainRelease				alternateTitle;				// user-defined window title
	CFRetainRelease				resourceLocationString;		// one-liner for remote URL or local Unix command line
	CFRetainRelease				commandLineArguments;		// CFArrayRef of CFStringRef; typically agrees with "resourceLocationString"
	CFRetainRelease				originalDirectoryString;	// pathname of the directory that was current when the session was executed
	CFRetainRelease				deviceNameString;			// pathname of slave pseudo-terminal device attached to the session
	UInt32						connectionDateTime;			// result of GetDateTime() call at connection time
	CFAbsoluteTime				terminationAbsoluteTime;	// result of CFAbsoluteTimeGetCurrent() call at disconnection time
	EventHandlerUPP				windowClosingUPP;			// wrapper for window closing callback
	EventHandlerRef				windowClosingHandler;		// invoked whenever a session terminal window should close
	EventHandlerUPP				windowFocusChangeUPP;		// wrapper for window focus-change callback
	EventHandlerRef				windowFocusChangeHandler;	// invoked whenever a session terminal window is chosen for keyboard input
	EventHandlerUPP				terminalViewDragDropUPP;	// wrapper for drag-and-drop callback
	My_DragDropHandlerByView	terminalViewDragDropHandlers;// invoked whenever a terminal view is the target of drag-and-drop
	EventHandlerUPP				terminalViewEnteredUPP;		// wrapper for mouse tracking (focus-follows-mouse) callback
	My_DragDropHandlerByView	terminalViewEnteredHandlers;// invoked whenever the mouse moves into a terminal view
	EventHandlerUPP				terminalViewTextInputUPP;   // wrapper for keystroke callback
	My_TextInputHandlerByView	terminalViewTextInputHandlers;// invoked whenever a terminal view is focused during a key press
	ListenerModel_Ref			changeListenerModel;		// who to notify for various kinds of changes to this session data
	ListenerModel_ListenerRef	windowValidationListener;	// responds after a window is created, and just before it dies
	ListenerModel_ListenerRef	terminalWindowListener;		// responds when terminal window states change
	ListenerModel_ListenerRef	preferencesListener;		// responds when certain preference values are initialized or changed
	EventLoopTimerUPP			autoActivateDragTimerUPP;	// procedure that is called when a drag hovers over an inactive window
	EventLoopTimerRef			autoActivateDragTimer;		// short timer
	EventLoopTimerUPP			longLifeTimerUPP;			// procedure that is called when a session has been open 15 seconds
	EventLoopTimerRef			longLifeTimer;				// 15-second timer
	InterfaceLibAlertRef		currentTerminationAlert;	// retained while a sheet is still open so a 2nd sheet is not displayed
	TerminalWindowRef			terminalWindow;				// terminal window housing this session
	HIWindowRef					xwindow;						// redundant copy of TerminalWindow_ReturnWindow(terminalWindow)
	Local_ProcessRef			mainProcess;				// the command whose output is directly attached to the terminal
	Session_EventKeys			eventKeys;					// information on keyboard short-cuts for major events
	My_TEKGraphicList			targetVectorGraphics;		// list of TEK graphics attached to this session
	My_RasterGraphicsScreenList	targetRasterGraphicsScreens;	// list of open ICR graphics screens, if any
	My_TerminalScreenList		targetDumbTerminals;		// list of DUMB terminals to which incoming data is being copied
	My_TerminalScreenList		targetTerminals;			// list of screen buffers to which incoming data is being copied
	Boolean						vectorGraphicsPageOpensNewWindow;	// true if a TEK PAGE opens a new window instead of clearing the current one
	VectorInterpreter_Mode		vectorGraphicsCommandSet;	// e.g. TEK 4014 or 4105
	VectorInterpreter_ID		vectorGraphicsID;			// the ID of the current graphic, if any; see "VectorInterpreter.h"
	size_t						readBufferSizeMaximum;		// maximum number of bytes that can be processed at once
	size_t						readBufferSizeInUse;		// number of bytes of data currently in the read buffer
	UInt8*						readBufferPtr;				// buffer space for processing data
	CFStringEncoding			writeEncoding;				// the character set that text (data) sent to a session should be using
	Session_Watch				activeWatch;				// if any, what notification is currently set up for internal data events
	EventLoopTimerUPP			inactivityWatchTimerUPP;	// procedure that is called if data has not arrived after awhile
	EventLoopTimerRef			inactivityWatchTimer;		// short timer
	AlertMessages_BoxRef		watchBox;					// if defined, the global alert used to show notifications for this session
	My_AuxiliaryDataByKey		auxiliaryDataMap;			// all tagged data associated with this session
	SessionRef					selfRef;					// convenient reference to this structure
	
	struct
	{
		Boolean		enabled;		//!< is local echo enabled?
		Boolean		halfDuplex;		//!< data is echoed and sent immediately, instead of waiting for
									//!  a control key (as in full duplex)
	} echo;
	
	struct
	{
		Boolean		cursorFlashes;				//!< preferences callback should update this value
		Boolean		remapBackquoteToEscape;		//!< preferences callback should update this value
	} preferencesCache;
};
typedef My_Session*		My_SessionPtr;
typedef My_SessionPtr*	My_SessionHandle;

typedef MemoryBlockPtrLocker< SessionRef, My_Session >	My_SessionPtrLocker;
typedef LockAcquireRelease< SessionRef, My_Session >	My_SessionAutoLocker;
typedef MemoryBlockReferenceTracker<SessionRef>			My_SessionRefTracker;

struct My_PasteAlertInfo
{
	// sent to pasteWarningCloseNotifyProc()
	SessionRef			sessionForPaste;
	CFRetainRelease		sourcePasteboard;
};
typedef My_PasteAlertInfo*		My_PasteAlertInfoPtr;

struct My_TerminateAlertInfo
{
	My_TerminateAlertInfo	(SessionRef		inSession): sessionBeingTerminated(inSession) {}
	
	// sent to terminationWarningCloseNotifyProc()
	SessionRef		sessionBeingTerminated;
};
typedef My_TerminateAlertInfo*	My_TerminateAlertInfoPtr;

struct My_WatchAlertInfo
{
	// sent to watchCloseNotifyProc()
	SessionRef		sessionBeingWatched;
};
typedef My_WatchAlertInfo*		My_WatchAlertInfoPtr;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

pascal void					autoActivateWindow					(EventLoopTimerRef, void*);
void						changeNotifyForSession				(My_SessionPtr, Session_Change, void*);
void						changeStateAttributes				(My_SessionPtr, Session_StateAttributes,
																 Session_StateAttributes);
void						closeTerminalWindow					(My_SessionPtr);
void						configureSaveDialog					(SessionRef, NavDialogCreationOptions&);
UInt16						copyEventKeyPreferences				(My_SessionPtr, Preferences_ContextRef, Boolean);
My_HMHelpContentRecWrap&	createHelpTagForInterrupt			();
My_HMHelpContentRecWrap&	createHelpTagForResume				();
My_HMHelpContentRecWrap&	createHelpTagForSuspend				();
My_HMHelpContentRecWrap&	createHelpTagForVectorGraphics		();
PMPageFormat				createSessionPageFormat				();
IconRef						createSessionStateActiveIcon		();
IconRef						createSessionStateDeadIcon			();
pascal void					detectLongLife						(EventLoopTimerRef, void*);
Boolean						handleSessionKeyDown				(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
Boolean						isReadOnly							(My_SessionPtr);
pascal void					navigationFileCaptureDialogEvent	(NavEventCallbackMessage, NavCBRecPtr, void*);
pascal void					navigationSaveDialogEvent			(NavEventCallbackMessage, NavCBRecPtr, void*);
pascal void					pageSetupCloseNotifyProc			(PMPrintSession, WindowRef, Boolean);
void						pasteWarningCloseNotifyProc			(InterfaceLibAlertRef, SInt16, void*);
void						preferenceChanged					(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
size_t						processMoreData						(My_SessionPtr);
pascal OSStatus				receiveTerminalViewDragDrop			(EventHandlerCallRef, EventRef, void*);
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
pascal OSStatus				receiveTerminalViewEntered			(EventHandlerCallRef, EventRef, void*);
#endif
pascal OSStatus				receiveTerminalViewTextInput		(EventHandlerCallRef, EventRef, void*);
pascal OSStatus				receiveWindowClosing				(EventHandlerCallRef, EventRef, void*);
pascal OSStatus				receiveWindowFocusChange			(EventHandlerCallRef, EventRef, void*);
HIWindowRef					returnActiveWindow					(My_SessionPtr);
OSStatus					sessionDragDrop						(EventHandlerCallRef, EventRef, SessionRef,
																 HIViewRef, DragRef);
void						terminalWindowChanged				(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
void						terminationWarningCloseNotifyProc	(InterfaceLibAlertRef, SInt16, void*);
void						watchClearForSession				(My_SessionPtr);
void						watchCloseNotifyProc				(AlertMessages_BoxRef, SInt16, void*);
void						watchNotifyForSession				(My_SessionPtr, Session_Watch);
pascal void					watchNotifyFromTimer				(EventLoopTimerRef, void*);
void						watchTimerResetForSession			(My_SessionPtr, Session_Watch);
void						windowValidationStateChanged		(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);

} // anonymous namespace

#pragma mark Variables
namespace {

My_SessionPtrLocker&	gSessionPtrLocks ()	{ static My_SessionPtrLocker x; return x; }
PMPageFormat&			gSessionPageFormat () { static PMPageFormat x = createSessionPageFormat(); return x; }
IconRef					gSessionActiveIcon () { static IconRef x = createSessionStateActiveIcon(); return x; }
IconRef					gSessionDeadIcon () { static IconRef x = createSessionStateDeadIcon(); return x; }
My_SessionRefTracker&	gInvalidSessions () { static My_SessionRefTracker x; return x; }

} // anonymous namespace

#pragma mark Functors
namespace {

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
		_result = VectorInterpreter_ProcessData(inTEKWindowID, _buffer, _bufferSize);
	}

protected:

private:
	UInt8 const*	_buffer;
	size_t			_bufferSize;
	size_t			_result;
};

} // anonymous namespace


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
Session_New		(Preferences_ContextRef		inConfigurationOrNull,
				 Boolean					inIsReadOnly)
{
	SessionRef		result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_Session(inConfigurationOrNull, inIsReadOnly), SessionRef);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
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
	if (gSessionPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteLine, "attempt to dispose of locked session");
	}
	else
	{
		// clean up
		{
			My_SessionAutoLocker	ptr(gSessionPtrLocks(), *inoutRefPtr);
			
			
			delete ptr;
		}
		*inoutRefPtr = nullptr;
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_Result			result = kSession_ResultOK;
	
	
	switch (inTarget)
	{
	case kSession_DataTargetStandardTerminal:
		{
			My_TerminalScreenList::size_type	listSize = ptr->targetTerminals.size();
			
			
			ptr->targetTerminals.push_back(REINTERPRET_CAST(inTargetData, TerminalScreenRef));
			assert(ptr->targetTerminals.size() == (1 + listSize));
		}
		break;
	
	case kSession_DataTargetDumbTerminal:
		{
			My_TerminalScreenList::size_type	listSize = ptr->targetDumbTerminals.size();
			
			
			ptr->targetDumbTerminals.push_back(REINTERPRET_CAST(inTargetData, TerminalScreenRef));
			assert(ptr->targetDumbTerminals.size() == (1 + listSize));
		}
		break;
	
	case kSession_DataTargetTektronixGraphicsCanvas:
		{
			My_TEKGraphicList::size_type	listSize = ptr->targetVectorGraphics.size();
			
			
			ptr->targetVectorGraphics.push_back(*(REINTERPRET_CAST(inTargetData, SInt16 const*/* TEK graphics ID */)));
			assert(ptr->targetVectorGraphics.size() == (1 + listSize));
		}
		break;
	
	case kSession_DataTargetInteractiveColorRasterGraphicsScreen:
		{
			My_RasterGraphicsScreenList::size_type	listSize = ptr->targetRasterGraphicsScreens.size();
			
			
			ptr->targetRasterGraphicsScreens.push_back(*(REINTERPRET_CAST(inTargetData, SInt16 const*/* ICR window ID */)));
			assert(ptr->targetRasterGraphicsScreens.size() == (1 + listSize));
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
Copies the specified data into the “read buffer” of the
specified session, starting one byte beyond the
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	size_t					numberOfBytesToCopy = 0;
	size_t const			kMaximumBytesLeft = ptr->readBufferSizeMaximum - ptr->readBufferSizeInUse;
	size_t					result = inSize;
	
	
	// figure out how much to copy
	numberOfBytesToCopy = INTEGER_MINIMUM(kMaximumBytesLeft, inSize);
	if (numberOfBytesToCopy > 0)
	{
		result = inSize - numberOfBytesToCopy;
		CPP_STD::memcpy(ptr->readBufferPtr + ptr->readBufferSizeInUse, inDataPtr, numberOfBytesToCopy);
		ptr->readBufferSizeInUse += numberOfBytesToCopy;
		
		processMoreData(ptr);
		
		// also trigger a watch, if one exists
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
	return result;
}// AppendDataForProcessing


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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
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
	Console_Warning(Console_WriteLine, "unimplemented");
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
	OSStatus				error = noErr;
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	PMPrintSession			printSession = nullptr;
	
	
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
	// display a key mapping customization dialog
	PrefsContextDialog_Ref	dialog = nullptr;
	Panel_Ref				prefsPanel = PrefPanelSessions_NewKeyboardPane();
	
	
	// display the sheet
	dialog = PrefsContextDialog_New(GetUserFocusWindow(), prefsPanel,
									Session_ReturnConfiguration(inRef),
									kPrefsContextDialog_DisplayOptionNoAddToPrefsButton);
	PrefsContextDialog_Display(dialog); // automatically disposed when the user clicks a button
}// DisplaySpecialKeySequencesDialog


/*!
Displays the Close warning alert for the given terminal
window, handling the user’s response automatically.  If it
has been determined that the window was only recently
opened, it may close immediately without displaying any
warning to the user (as if the user had clicked Close in
the alert).

On Mac OS X, a sheet is used (unless "inForceModalDialog"
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
									 Boolean		inForceModalDialog)
{
	My_TerminateAlertInfoPtr	terminateAlertInfoPtr = new My_TerminateAlertInfo(inRef);
	
	
	if (Session_StateIsActiveUnstable(inRef))
	{
		// the connection JUST opened, so kill it without confirmation
		terminationWarningCloseNotifyProc(nullptr/* alert box */, kAlertStdAlertOKButton,
											terminateAlertInfoPtr/* user data */);
	}
	else
	{
		AlertMessages_BoxRef	alertBox = Alert_New();
		TerminalWindowRef		terminalWindow = nullptr;
		HIWindowRef				window = Session_ReturnActiveWindow(inRef);
		Rect					originalStructureBounds;
		Rect					centeredStructureBounds;
		OSStatus				error = noErr;
		
		
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
		
		// access the session as needed, but then unlock it so that
		// it can be killed if necessary
		{
			My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
			
			
			terminalWindow = ptr->terminalWindow;
			
			// if a warning alert is already showing (presumably because
			// a sheet is open and now a modal dialog is being displayed),
			// first Cancel the open sheet before displaying another one
			if (nullptr != ptr->currentTerminationAlert)
			{
				Alert_HitItem(ptr->currentTerminationAlert, kAlertStdAlertCancelButton);
			}
			
			ptr->currentTerminationAlert = alertBox;
		}
		
		// basic alert box setup
		Alert_SetParamsFor(alertBox, kAlert_StyleOKCancel);
		Alert_SetType(alertBox, kAlertCautionAlert);
		
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
					Alert_SetTextCFStrings(alertBox, primaryTextCFString, helpTextCFString);
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
				Alert_SetTitleCFString(alertBox, titleCFString);
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
				Alert_SetButtonText(alertBox, kAlertStdAlertOKButton, buttonTitleCFString);
				CFRelease(buttonTitleCFString);
			}
		}
		
		// finally, display the message
		{
			Boolean		willLeaveTerminalWindowOpen = false;
			size_t		actualSize = 0;
			
			
			unless (inForceModalDialog)
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
			{
				My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
				
				
				changeStateAttributes(ptr, kSession_StateAttributeOpenDialog/* attributes to set */,
										0/* attributes to clear */);
			}
			
			// display the confirmation alert; depending on the kind of
			// alert (sheet or modal dialog), the following code may
			// return immediately without session termination (yet);
			// ensure that the relevant window is visible and frontmost
			// when the message appears
			TerminalWindow_SetVisible(terminalWindow, true);
			TerminalWindow_Select(terminalWindow);
			if (inForceModalDialog)
			{
				Alert_Display(alertBox);
				if (Alert_ItemHit(alertBox) == kAlertStdAlertCancelButton)
				{
					// in the event that the user cancelled and the window
					// was transitioned to the screen center, “un-transition”
					// the most recent window back to its original location -
					// unless of course the user has since moved the window
					Rect	currentStructureBounds;
					
					
					(OSStatus)GetWindowBounds(window, kWindowStructureRgn, &currentStructureBounds);
					if (EqualRect(&currentStructureBounds, &centeredStructureBounds))
					{
						HIRect						floatBounds = CGRectMake(originalStructureBounds.left, originalStructureBounds.top,
																				originalStructureBounds.right - originalStructureBounds.left,
																				originalStructureBounds.bottom - originalStructureBounds.top);
						TransitionWindowOptions		transitionOptions;
						
						
						bzero(&transitionOptions, sizeof(transitionOptions));
						transitionOptions.version = 0;
						(OSStatus)TransitionWindowWithOptions(window, kWindowSlideTransitionEffect, kWindowMoveTransitionAction,
																&floatBounds, true/* asynchronous */, &transitionOptions);
					}
				}
				terminationWarningCloseNotifyProc(alertBox, Alert_ItemHit(alertBox),
													terminateAlertInfoPtr/* user data */);
			}
			else
			{
				Alert_MakeWindowModal(alertBox, window/* parent */, !willLeaveTerminalWindowOpen/* is window close alert */,
										terminationWarningCloseNotifyProc, terminateAlertInfoPtr/* user data */);
				Alert_Display(alertBox); // notifier disposes the alert when the sheet eventually closes
			}
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
			My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
			TerminalWindowRef		terminalWindow = Session_ReturnActiveTerminalWindow(inRef);
			TerminalViewRef			view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
			
			
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
				stringValue = CFStringCreateByCombiningStrings(kCFAllocatorDefault,
																ptr->commandLineArguments.returnCFArrayRef(), CFSTR(" "));
				if (nullptr != stringValue)
				{
					saveError = SessionDescription_SetStringData
									(saveFileMemoryModel, kSessionDescription_StringTypeCommandLine, stringValue);
					CFRelease(stringValue), stringValue = nullptr;
				}
				
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
				if (false == ptr->targetTerminals.empty())
				{
					// TEMPORARY - limitations of this format do not really allow for the
					// concept of multiple screens per window!
					TerminalScreenRef	screen = ptr->targetTerminals.front();
					UInt16				columns = Terminal_ReturnColumnCount(screen);
					UInt16				rows = Terminal_ReturnRowCount(screen);
					UInt32				scrollback = Terminal_ReturnInvisibleRowCount(screen);
					
					
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
					
					
					flag = ptr->eventKeys.newline;
					saveError = SessionDescription_SetBooleanData
								(saveFileMemoryModel, kSessionDescription_BooleanTypeRemapCR, flag);
					flag = !ptr->eventKeys.pageKeysLocalControl;
					saveError = SessionDescription_SetBooleanData
								(saveFileMemoryModel, kSessionDescription_BooleanTypePageKeysDoNotControlTerminal, flag);
					flag = false; // TEMPORARY
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	register SInt16			remainingBytesCount = 0;
	
	
	TerminalView_SetDrawingEnabled(TerminalWindow_ReturnViewWithFocus(Session_ReturnActiveTerminalWindow(inRef)),
									false); // no output
	remainingBytesCount = 1; // just needs to be positive to start with
	while (remainingBytesCount > 0) remainingBytesCount = processMoreData(ptr);
	TerminalView_SetDrawingEnabled(TerminalWindow_ReturnViewWithFocus(Session_ReturnActiveTerminalWindow(inRef)),
									true); // output now
	RegionUtilities_RedrawWindowOnNextUpdate(returnActiveWindow(ptr));
}// FlushNetwork


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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		outUncopiedString = ptr->alternateTitle.returnCFStringRef();
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = isReadOnly(ptr);
	
	
	return result;
}// IsReadOnly


/*!
Returns "true" only if the specified session has not
been destroyed with Session_Dispose(), and is not in
the process of being destroyed.

Most of the time, checking for a null reference is
enough, and efficient; this check may be slower, but
is important if you are handling something indirectly
or asynchronously (where a session could have been
destroyed at any time).

(4.0)
*/
Boolean
Session_IsValid		(SessionRef		inRef)
{
	Boolean		result = (gInvalidSessions().find(inRef) == gInvalidSessions().end());
	
	
	return result;
}// IsValid


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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = ptr->echo.enabled;
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (false == ptr->echo.halfDuplex);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = ptr->echo.halfDuplex;
	
	
	return result;
}// LocalEchoIsHalfDuplex


/*!
Returns "true" only if the specified connection is disabled;
that is, its network activity has not been suspended by a
Scroll Lock.

(3.0)
*/
Boolean
Session_NetworkIsSuspended		(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (0 != (ptr->statusAttributes & kSession_StateAttributeSuspendNetwork));
	
	
	if ((false == result) && (nullptr != ptr->mainProcess))
	{
		result = Local_ProcessIsStopped(ptr->mainProcess);
	}
	return result;
}// NetworkIsSuspended


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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_Result			result = kSession_ResultOK;
	
	
	// “carbon copy” the data to all active attached targets; take care
	// not to do this once a session is flagged for destruction, since
	// at that point it may not be able to handle data anymore
	if ((kSession_StateImminentDisposal != ptr->status) &&
		(kSession_StateDead != ptr->status))
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
				// this is the typical case; send data to a sophisticated terminal emulator
				std::for_each(ptr->targetTerminals.begin(), ptr->targetTerminals.end(), terminalDataWriter(kBuffer, inByteCount));
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_Result			result = kSession_ResultOK;
	
	
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
	TerminalWindowRef	result = nullptr;
	
	
	if (Session_IsValid(inRef))
	{
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		result = ptr->terminalWindow;
	}
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	HIWindowRef				result = returnActiveWindow(ptr);
	
	
	return result;
}// ReturnActiveWindow


/*!
Returns the program name and command line arguments used
to start the session originally.  Each array element is
a CFStringRef, but it can be converted back into a C string
using CFString APIs.

(4.0)
*/
CFArrayRef
Session_ReturnCommandLine	(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFArrayRef				result = ptr->commandLineArguments.returnCFArrayRef();
	
	
	return result;
}// ReturnCommandLine


/*!
Returns a variety of preferences unique to this session.

You can make changes to this context ONLY if you do it in “batch
mode” with Preferences_ContextCopy().  In other words, even to
make a single change, you must first add the change to a new
temporary context, then use Preferences_ContextCopy() to read
the temporary settings into the context returned by this routine.
Batch mode changes are detected by the Session and used to
automatically update internal data structures.

Note that you cannot expect all possible tags to be present;
be prepared to not find what you look for.  In addition, tags
that are present in one session may be absent in another.

(4.0)
*/
Preferences_ContextRef
Session_ReturnConfiguration		(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Preferences_ContextRef	result = ptr->configuration;
	
	
	// since many settings are represented internally, this context
	// will not contain the latest information; update the context
	// based on current settings
	
	{
		char const	kKeyChar = ptr->eventKeys.interrupt;
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagKeyInterruptProcess,
													sizeof(kKeyChar), &kKeyChar);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		char const	kKeyChar = ptr->eventKeys.suspend;
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagKeySuspendOutput,
													sizeof(kKeyChar), &kKeyChar);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		char const	kKeyChar = ptr->eventKeys.resume;
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagKeyResumeOutput,
													sizeof(kKeyChar), &kKeyChar);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		Boolean const	kMapCRToCRNull = (kSession_NewlineModeMapCRNull == ptr->eventKeys.newline);
		
		
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagMapCarriageReturnToCRNull,
													sizeof(kMapCRToCRNull), &kMapCRToCRNull);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagEmacsMetaKey,
													sizeof(ptr->eventKeys.meta), &ptr->eventKeys.meta);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagMapDeleteToBackspace,
													sizeof(ptr->eventKeys.deleteSendsBackspace),
													&ptr->eventKeys.deleteSendsBackspace);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagMapArrowsForEmacs,
													sizeof(ptr->eventKeys.arrowsRemappedForEmacs),
													&ptr->eventKeys.arrowsRemappedForEmacs);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagPageKeysControlLocalTerminal,
													sizeof(ptr->eventKeys.pageKeysLocalControl),
													&ptr->eventKeys.pageKeysLocalControl);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	{
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagMapKeypadTopRowForVT220,
													sizeof(ptr->eventKeys.keypadRemappedForVT220),
													&ptr->eventKeys.keypadRemappedForVT220);
		assert(kPreferences_ResultOK == prefsResult);
	}
	
	return result;
}// ReturnConfiguration


/*!
Returns the keys used as short-cuts for various events.
See the documentation on Session_EventKeys for more
information.

(4.0)
*/
Session_EventKeys
Session_ReturnEventKeys		(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_EventKeys		result = ptr->eventKeys;
	
	
	return result;
}// ReturnEventKeys


/*!
Returns the POSIX path of the directory that was current
when the session was started.  If this is empty, it means
that no particular directory was chosen (so it will be
the current directory from its spawning shell, typically
the Finder).

(4.0)
*/
CFStringRef
Session_ReturnOriginalWorkingDirectory		(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFStringRef				result = ptr->originalDirectoryString.returnCFStringRef();
	
	
	return result;
}// ReturnOriginalWorkingDirectory


/*!
Returns a pathname for the slave pseudo-terminal device
attached to the given session.  This can be displayed in
user interface elements.

(3.1)
*/
CFStringRef
Session_ReturnPseudoTerminalDeviceNameCFString		(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFStringRef				result = ptr->deviceNameString.returnCFStringRef();
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFStringRef				result = ptr->resourceLocationString.returnCFStringRef();
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_State			result = ptr->status;
	
	
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
	My_SessionAutoLocker		ptr(gSessionPtrLocks(), inRef);
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
	Session_Result			result = kSession_ResultOK;
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFIndex					numberOfBytesRequired = 0;
	CFIndex					numberOfCharactersConverted = CFStringGetBytes
															(inString,
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	SInt16					result = 0;
	
	
	if (nullptr != ptr->mainProcess)
	{
		result = Local_TerminalWriteBytes(Local_ProcessReturnMasterTerminal(ptr->mainProcess),
											inBufferPtr, inByteCount);
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	SInt16					result = 0;
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	SInt16					result = 0;
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					lineFeedToo = false;
	Boolean					echo = false;
	
	
	if (false == ptr->targetTerminals.empty())
	{
		lineFeedToo = Terminal_LineFeedNewLineMode(ptr->targetTerminals.front());
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
		echo = ptr->echo.enabled;
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
Specifies the maximum number of bytes that can be read with
each call to processMoreData().  Depending on available memory,
the actual buffer size may end up being less than the amount
requested.

Requests of less than 512 bytes are increased to be a minimum
of 512 bytes.

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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		UInt8*					newBuffer = nullptr;
		
		
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
Changes the keys used as short-cuts for various events.
See the documentation on Session_EventKeys for more
information.

(4.0)
*/
void
Session_SetEventKeys	(SessionRef					inRef,
						 Session_EventKeys const&	inKeys)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->eventKeys = inKeys;
}// SetEventKeys


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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->echo.enabled = inIsEnabled;
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->echo.halfDuplex = false;
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->echo.halfDuplex = true;
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	// send the appropriate character to the Unix process
	if (inScrollLock)
	{
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
			UInt8		charToSend = STATIC_CAST
										(Local_TerminalReturnFlowStopCharacter
											(Local_ProcessReturnMasterTerminal(ptr->mainProcess)), UInt8);
			SInt16		bytesSent = Session_SendData(inRef, &charToSend, 1/* number of bytes */);
			
			
			if (bytesSent < 1)
			{
				Console_Warning(Console_WriteLine, "failed to send stop character to session");
			}
		}
		// set the scroll lock attribute of the session
		changeStateAttributes(ptr, kSession_StateAttributeSuspendNetwork/* attributes to set */,
								0/* attributes to clear */);
	}
	else
	{
		// hide the help tag displayed by the Suspend
		(OSStatus)HMHideTag();
		
		// resume
		if (nullptr != ptr->mainProcess)
		{
			UInt8		charToSend = STATIC_CAST
										(Local_TerminalReturnFlowStartCharacter
											(Local_ProcessReturnMasterTerminal(ptr->mainProcess)), UInt8);
			SInt16		bytesSent = Session_SendData(inRef, &charToSend, 1/* number of bytes */);
			
			
			if (bytesSent < 1)
			{
				Console_Warning(Console_WriteLine, "failed to send start character to session");
			}
		}
		// clear the scroll lock attribute of the session
		changeStateAttributes(ptr, 0/* attributes to set */,
								kSession_StateAttributeSuspendNetwork/* attributes to clear */);
	}
}// SetNetworkSuspended


/*!
Specifies the process that this session is running.  This
automatically sets the device name and resource location
(Session_ReturnPseudoTerminalDeviceNameCFString(),
Session_ReturnResourceLocationCFString() and
Session_ReturnCommandLine() and can be used to return these
values later).

(3.1)
*/
void
Session_SetProcess	(SessionRef			inRef,
					 Local_ProcessRef	inRunningProcess)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	assert(nullptr != inRunningProcess);
	ptr->mainProcess = inRunningProcess;
	
	// store important information about the process that spawned
	{
		// set device name string
		char const* const	kDeviceName = Local_ProcessReturnSlaveDeviceName(ptr->mainProcess);
		
		
		ptr->deviceNameString.setCFTypeRef(CFStringCreateWithCString
											(kCFAllocatorDefault, kDeviceName, kCFStringEncodingASCII),
											true/* is retained */);
	}
	ptr->commandLineArguments = Local_ProcessReturnCommandLine(ptr->mainProcess);
	ptr->originalDirectoryString = Local_ProcessReturnOriginalDirectory(ptr->mainProcess);
	ptr->resourceLocationString.setCFTypeRef(CFStringCreateByCombiningStrings
												(kCFAllocatorDefault, ptr->commandLineArguments.returnCFArrayRef(), CFSTR(" ")),
												true/* is retained */);
	
	changeNotifyForSession(ptr, kSession_ChangeResourceLocation, inRef/* context */);
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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	for (My_TerminalScreenList::iterator toScreen = ptr->targetTerminals.begin();
			toScreen != ptr->targetTerminals.end(); ++toScreen)
	{
		Terminal_SetSpeechEnabled(*toScreen, inIsEnabled);
	}
}// SetSpeechEnabled


/*!
Changes the current state of the specified session,
which will trigger listener notification for all
modules interested in finding out about new states
(such as sessions that die, etc.).

If the proposed state is "kSession_StateDead" or
"kSession_StateImminentDisposal", a (most likely
redundant) kill attempt is made on the process to
be sure that it ends up dead.

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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFStringRef				statusString = nullptr;
	Boolean					keepWindowOpen = true;
	
	
	// update status text to reflect new state
	ptr->status = inNewState;
	
	if (nullptr != ptr->mainProcess)
	{
		if ((kSession_StateDead == ptr->status) || (kSession_StateImminentDisposal == ptr->status))
		{
			Local_KillProcess(&ptr->mainProcess);
		}
	}
	
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
		size_t		actualSize = 0L;
		
		
		// get the user’s process service preference, if possible
		unless (Preferences_GetData(kPreferences_TagDontAutoClose,
									sizeof(keepWindowOpen), &keepWindowOpen,
									&actualSize) == kPreferences_ResultOK)
		{
			keepWindowOpen = false; // assume window should be closed, if preference can’t be found
		}
	}
	
	// now that the state of the Session object is appropriate
	// for the state, notify any other listeners of the change
	changeNotifyForSession(ptr, kSession_ChangeState, inRef/* context */);
	
	unless (keepWindowOpen)
	{
		assert(Session_StateIsDead(inRef));
		closeTerminalWindow(ptr);
	}
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->terminalWindow = inTerminalWindow;
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->alternateTitle.setCFTypeRef(CFStringCreateCopy(kCFAllocatorDefault, inWindowName));
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = false;
	
	
	for (My_TerminalScreenList::iterator toScreen = ptr->targetTerminals.begin();
			toScreen != ptr->targetTerminals.end(); ++toScreen)
	{
		if (Terminal_SpeechIsEnabled(*toScreen)) result = true;
	}
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	for (My_TerminalScreenList::iterator toScreen = ptr->targetTerminals.begin();
			toScreen != ptr->targetTerminals.end(); ++toScreen)
	{
		Terminal_SpeechPause(*toScreen);
	}
}// SpeechPause


/*!
Resumes speech paused with Session_SpeechPause().

(3.0)
*/
void
Session_SpeechResume	(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	for (My_TerminalScreenList::iterator toScreen = ptr->targetTerminals.begin();
			toScreen != ptr->targetTerminals.end(); ++toScreen)
	{
		Terminal_SpeechResume(*toScreen);
	}
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
		OSStatus				error = noErr;
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = false;
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_StateActiveStable == ptr->status);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_StateActiveUnstable == ptr->status);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_StateBrandNew == ptr->status);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_StateDead == ptr->status);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_StateImminentDisposal == ptr->status);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_StateInitialized == ptr->status);
	
	
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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	VectorInterpreter_ID	id = kVectorInterpreter_InvalidID;
	Boolean					result = false;
	
	
	id = VectorInterpreter_New(kVectorInterpreter_TargetScreenPixels, ptr->vectorGraphicsCommandSet);
	if (kVectorInterpreter_InvalidID != id)
	{
		VectorInterpreter_SetPageClears(id, false == ptr->vectorGraphicsPageOpensNewWindow);
		ptr->vectorGraphicsID = id;
		VectorCanvas_SetListeningSession(VectorInterpreter_ReturnCanvas(id), inRef);
		{
			CFStringRef		windowTitleCFString = nullptr;
			
			
			if (noErr == CopyWindowTitleAsCFString(returnActiveWindow(ptr), &windowTitleCFString))
			{
				VectorCanvas_SetTitle(VectorInterpreter_ReturnCanvas(id), windowTitleCFString);
				CFRelease(windowTitleCFString), windowTitleCFString = nullptr;
			}
		}
		Session_AddDataTarget(inRef, kSession_DataTargetTektronixGraphicsCanvas, &id);
		TerminalWindow_Select(ptr->terminalWindow);
		
		// display a help tag over the cursor in an unobtrusive location
		// that confirms for the user that a suspend has in fact occurred
		{
			My_HMHelpContentRecWrap&	tagData = createHelpTagForVectorGraphics();
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
		
		result = true;
	}
	return result;
}// TEKCreateTargetGraphic


/*!
Dissociates the current target graphic of the given session
from the session.  TEK writes will therefore no longer affect
this graphic.  If there is no target graphic, this routine
does nothing.

(3.0)
*/
void
Session_TEKDetachTargetGraphic		(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	if (kVectorInterpreter_InvalidID != ptr->vectorGraphicsID)
	{
		Session_RemoveDataTarget(inRef, kSession_DataTargetTektronixGraphicsCanvas, &ptr->vectorGraphicsID);
		ptr->vectorGraphicsID = kVectorInterpreter_InvalidID;
	}
}// TEKDetachTargetGraphic


/*!
Returns "true" only if there is currently a target
TEK vector graphic for the specified session.

(3.0)
*/
Boolean
Session_TEKHasTargetGraphic		(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kVectorInterpreter_InvalidID != ptr->vectorGraphicsID);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kVectorInterpreter_ModeDisabled != ptr->vectorGraphicsCommandSet);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = ptr->vectorGraphicsPageOpensNewWindow;
	
	
	return result;
}// TEKPageCommandOpensNewWindow


/*!
Set to "true" only if the given session should open a
new TEK window whenever a TEK PAGE command is received.
(The alternative is that the current TEK window is
cleared and replaced with the new graphics.)

See also Session_TEKPageCommandOpensNewWindow().

(3.1)
*/
void
Session_TEKSetPageCommandOpensNewWindow		(SessionRef		inRef,
											 Boolean		inNewWindow)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	ptr->vectorGraphicsPageOpensNewWindow = inNewWindow;
	if (kVectorInterpreter_InvalidID != ptr->vectorGraphicsID)
	{
		VectorInterpreter_SetPageClears(ptr->vectorGraphicsID, false == inNewWindow);
	}
}// TEKSetPageCommandOpensNewWindow


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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		outAnswerBack = nullptr;
		// TEMPORARY - this API is not flexible enough to handle multiple screens per window
		if (false == ptr->targetTerminals.empty())
		{
			outAnswerBack = Terminal_EmulatorReturnName(ptr->targetTerminals.front());
		}
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	std::for_each(ptr->targetTerminals.begin(), ptr->targetTerminals.end(), terminalDataWriter(inBuffer, inLength));	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	for (My_TerminalScreenList::iterator toScreen = ptr->targetTerminals.begin();
			toScreen != ptr->targetTerminals.end(); ++toScreen)
	{
		Terminal_EmulatorProcessCString(*toScreen, inCString);
	}
}// TerminalWriteCString


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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	UInt32					result = 0L;
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFAbsoluteTime			result = 0L;
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_TypeLocalLoginShell == ptr->kind);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_TypeLocalNonLoginShell == ptr->kind);
	
	
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
	Session_SendData(inRef, inStringBuffer);
	
	if (Session_LocalEchoIsEnabled(inRef))
	{
		try
		{
			My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
			char*					stringBuffer = new char
													[1 + CFStringGetMaximumSizeForEncoding
															(CFStringGetLength(inStringBuffer), ptr->writeEncoding)];
			
			
			if (false == CFStringGetCString(inStringBuffer, stringBuffer, sizeof(stringBuffer), ptr->writeEncoding))
			{
				Console_Warning(Console_WriteLine, "text cannot be converted into the encoding used by this terminal");
			}
			else
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
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		if (nullptr != ptr->mainProcess)
		{
			UInt8		charToSend = STATIC_CAST
										(Local_TerminalReturnInterruptCharacter
											(Local_ProcessReturnMasterTerminal(ptr->mainProcess)), UInt8);
			SInt16		bytesSent = Session_SendData(inRef, &charToSend, 1/* number of bytes */);
			
			
			if (bytesSent < 1)
			{
				Console_Warning(Console_WriteLine, "failed to send interrupt character to session");
			}
		}
	}
}// UserInputInterruptProcess


/*!
Sends the appropriate sequence for the specified key and
optional modifiers, taking into account terminal settings
(such as whether or not arrows are remapped to Emacs commands).

\retval kSession_ResultOK
if the key was apparently input successfully

\retval kSession_ResultInvalidReference
if "inRef" is invalid

(3.1)
*/
Session_Result
Session_UserInputKey	(SessionRef		inRef,
						 UInt8			inKeyOrASCII,
						 UInt32			inEventModifiers)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_Result			result = kSession_ResultOK;
	
	
	if (nullptr == ptr) result = kSession_ResultInvalidReference;
	else
	{
		if (inKeyOrASCII < VSF10)
		{
			// 7-bit ASCII; send as-is
			Session_SendData(ptr->selfRef, &inKeyOrASCII, 1);
		}
		else if (ptr->eventKeys.arrowsRemappedForEmacs && (inKeyOrASCII <= VSLT) && (inKeyOrASCII >= VSUP))
		{
			UInt8		actualKeySeq[2] = { inKeyOrASCII, '\0' };
			size_t		sequenceLength = 1;
			
			
			// map from arrow keys to Emacs control keys; consider all standard
			// Mac modes, i.e. option-arrow for words, command-arrows for lines
			switch (inKeyOrASCII)
			{
			case VSLT:
				if (inEventModifiers & optionKey)
				{
					// move one word backward (meta-b)
					actualKeySeq[0] = 0x1B; // ESC (meta equivalent)
					actualKeySeq[1] = 'b';
					sequenceLength = 2;
				}
				else if (inEventModifiers & cmdKey)
				{
					// move to beginning of line (^A)
					actualKeySeq[0] = 0x01;
				}
				else
				{
					// move one character backward (^B)
					actualKeySeq[0] = 0x02;
				}
				break;
			
			case VSRT:
				if (inEventModifiers & optionKey)
				{
					// move one word forward (meta-f)
					actualKeySeq[0] = 0x1B; // ESC (meta equivalent)
					actualKeySeq[1] = 'f';
					sequenceLength = 2;
				}
				else if (inEventModifiers & cmdKey)
				{
					// move to beginning of line (^E)
					actualKeySeq[0] = 0x05;
				}
				else
				{
					// move one character forward (^F)
					actualKeySeq[0] = 0x06;
				}
				break;
			
			case VSUP:
				if (inEventModifiers & optionKey)
				{
					// move to beginning of line (^A)
					actualKeySeq[0] = 0x01;
				}
				else if (inEventModifiers & cmdKey)
				{
					// move to beginning of buffer (meta-<)
					actualKeySeq[0] = 0x1B; // ESC (meta equivalent)
					actualKeySeq[1] = '<';
					sequenceLength = 2;
				}
				else
				{
					// move one line up (^P)
					actualKeySeq[0] = 0x10; // ^P
				}
				break;
			
			case VSDN:
				if (inEventModifiers & optionKey)
				{
					// move to end of line (^E)
					actualKeySeq[0] = 0x05;
				}
				else if (inEventModifiers & cmdKey)
				{
					// move to end of buffer (meta-<)
					actualKeySeq[0] = 0x1B; // ESC (meta equivalent)
					actualKeySeq[1] = '>';
					sequenceLength = 2;
				}
				else
				{
					// move one line down (^N)
					actualKeySeq[0] = 0x0E;
				}
				break;
			
			default:
				break;
			}
			Session_SendData(ptr->selfRef, &actualKeySeq, sequenceLength);
		}
		else
		{
			if (false == ptr->targetTerminals.empty())
			{
				// allow the terminal to perform the appropriate action for the key,
				// given its current mode (for instance, a VT100 might be in VT52
				// mode, which would send different data than ANSI mode)
				for (My_TerminalScreenList::iterator toScreen = ptr->targetTerminals.begin();
						toScreen != ptr->targetTerminals.end(); ++toScreen)
				{
					(Terminal_Result)Terminal_UserInputVTKey(*toScreen, inKeyOrASCII, ptr->echo.enabled);
				}
			}
		}
	}
	return result;
}// UserInputKey


/*!
Allows the user to “type” whatever text is on the Clipboard
(or, if not nullptr, the specified pasteboard; useful for
handling drags, for instance).

With some terminal applications (like shells), pasting in more
than one line can cause unexpected results.  So, if the given
text is multi-line, the user is warned and given options on how
to perform the Paste.  This also means that this function could
return before the Paste actually occurs.

\retval kSession_ResultOK
always; no other return codes currently defined

(3.1)
*/
Session_Result
Session_UserInputPaste	(SessionRef			inRef,
						 PasteboardRef		inSourceOrNull)
{
	PasteboardRef const		kPasteboard = (nullptr == inSourceOrNull)
											? Clipboard_ReturnPrimaryPasteboard()
											: inSourceOrNull;
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	My_PasteAlertInfoPtr	pasteAlertInfoPtr = new My_PasteAlertInfo;
	CFStringRef				pastedCFString = nullptr;
	CFStringRef				pastedDataUTI = nullptr;
	Session_Result			result = kSession_ResultOK;
	
	
	pasteAlertInfoPtr->sessionForPaste = inRef;
	if (Clipboard_CreateCFStringFromPasteboard(pastedCFString, pastedDataUTI, kPasteboard))
	{
		// examine the Clipboard; if the data contains new-lines, warn the user
		Boolean		isOneLine = false;
		Boolean		noWarning = false;
		size_t		actualSize = 0;
		
		
		pasteAlertInfoPtr->sourcePasteboard = kPasteboard;
		
		// determine if the user should be warned
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNoPasteWarning,
									sizeof(noWarning), &noWarning, &actualSize))
		{
			noWarning = false; // assume a value, if preference can’t be found
		}
		
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
			isOneLine = Clipboard_IsOneLineInBuffer(bufferPtr, kBufferLength);
			if (nullptr != allocatedBuffer) delete [] allocatedBuffer, allocatedBuffer = nullptr;
		}
		
		// now, paste (perhaps displaying a warning first)
		{
			AlertMessages_BoxRef	box = Alert_New();
			
			
			if (isOneLine)
			{
				// the Clipboard contains only one line of text; Paste immediately without warning
				pasteWarningCloseNotifyProc(box, kAlertStdAlertOKButton, pasteAlertInfoPtr/* user data */);
			}
			else if (noWarning)
			{
				// the Clipboard contains more than one line, and the user does not want to be warned;
				// proceed with the Paste, but do it without joining (“other button” option)
				pasteWarningCloseNotifyProc(box, kAlertStdAlertOtherButton, pasteAlertInfoPtr/* user data */);
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
				TerminalWindow_SetVisible(ptr->terminalWindow, true);
				TerminalWindow_Select(ptr->terminalWindow);
				Alert_MakeWindowModal(box, returnActiveWindow(ptr)/* parent */, false/* is window close alert */,
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
	Session_SendData(inRef, inStringBuffer, inStringBufferSize);
	
	if (Session_LocalEchoIsEnabled(inRef))
	{
		Session_TerminalWrite(inRef, (UInt8 const*)inStringBuffer,
								inStringBufferSize / sizeof(char));
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_WatchForInactivity == ptr->activeWatch);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_WatchForKeepAlive == ptr->activeWatch);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_WatchForPassiveData == ptr->activeWatch);
	
	
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
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = (kSession_WatchNothing == ptr->activeWatch);
	
	
	return result;
}// WatchIsOff


#pragma mark Internal Methods
namespace {

/*!
Constructor.  See Session_New().

(4.0)
*/
My_Session::
My_Session	(Preferences_ContextRef		inConfigurationOrNull,
			 Boolean					inIsReadOnly)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
configuration((nullptr == inConfigurationOrNull)
				? Preferences_NewContext(Quills::Prefs::SESSION)
				: Preferences_NewCloneContext(inConfigurationOrNull, true/* detach */)),
readOnly(inIsReadOnly),
kind(kSession_TypeLocalNonLoginShell),
status(kSession_StateBrandNew),
statusAttributes(0),
statusString(),
alternateTitle(),
resourceLocationString(),
commandLineArguments(),
originalDirectoryString(),
deviceNameString(),
connectionDateTime(0), // set below
terminationAbsoluteTime(0),
windowClosingUPP(nullptr), // set at window validation time
windowClosingHandler(nullptr), // set at window validation time
windowFocusChangeUPP(nullptr), // set at window validation time
windowFocusChangeHandler(nullptr), // set at window validation time
terminalViewDragDropUPP(nullptr), // set at window validation time
terminalViewDragDropHandlers(), // set at window validation time
terminalViewEnteredUPP(nullptr), // set at window validation time
terminalViewEnteredHandlers(), // set at window validation time
terminalViewTextInputUPP(nullptr), // set at window validation time
terminalViewTextInputHandlers(), // set at window validation time
changeListenerModel(ListenerModel_New(kListenerModel_StyleStandard,
										kConstantsRegistry_ListenerModelDescriptorSessionChanges)),
windowValidationListener(ListenerModel_NewStandardListener(windowValidationStateChanged)),
terminalWindowListener(nullptr), // set at window validation time
preferencesListener(ListenerModel_NewStandardListener(preferenceChanged, this/* context */)),
autoActivateDragTimerUPP(NewEventLoopTimerUPP(autoActivateWindow)),
autoActivateDragTimer(nullptr), // installed only as needed
longLifeTimerUPP(NewEventLoopTimerUPP(detectLongLife)),
longLifeTimer(nullptr), // set later
currentTerminationAlert(nullptr),
terminalWindow(nullptr), // set at window validation time
mainProcess(nullptr),
// controlKey is initialized below
targetVectorGraphics(),
targetRasterGraphicsScreens(),
targetDumbTerminals(),
targetTerminals(),
vectorGraphicsPageOpensNewWindow(true),
vectorGraphicsCommandSet(kVectorInterpreter_ModeTEK4014), // arbitrary, reset later
vectorGraphicsID(kVectorInterpreter_InvalidID),
readBufferSizeMaximum(4096), // arbitrary, for initialization; see Session_SetDataProcessingCapacity()
readBufferSizeInUse(0),
readBufferPtr(new UInt8[this->readBufferSizeMaximum]), // Session_SetDataProcessingCapacity() also defines this buffer
writeEncoding(kCFStringEncodingUTF8), // initially...
activeWatch(kSession_WatchNothing),
inactivityWatchTimerUPP(nullptr),
inactivityWatchTimer(nullptr),
watchBox(nullptr),
auxiliaryDataMap(),
selfRef(REINTERPRET_CAST(this, SessionRef))
// echo initialized below
// preferencesCache initialized below
{
	bzero(&this->echo, sizeof(this->echo));
	bzero(&this->preferencesCache, sizeof(this->preferencesCache));
	
	// 3.0 - record the time when the connection began
	GetDateTime(&this->connectionDateTime);
	
	assert(nullptr != this->readBufferPtr);
	
	bzero(&this->eventKeys, sizeof(this->eventKeys));
	{
		UInt16		preferenceCount = copyEventKeyPreferences(this, inConfigurationOrNull, true/* search defaults too */);
		
		
		assert(preferenceCount > 0);
	}
	
	Session_StartMonitoring(this->selfRef, kSession_ChangeWindowValid, this->windowValidationListener);
	Session_StartMonitoring(this->selfRef, kSession_ChangeWindowInvalid, this->windowValidationListener);
	
	changeNotifyForSession(this, kSession_ChangeState, this->selfRef/* context */);
	changeNotifyForSession(this, kSession_ChangeStateAttributes, this->selfRef/* context */);
	
	// install a one-shot timer to tell interested parties when this session
	// has been opened for 15 seconds
	(OSStatus)InstallEventLoopTimer(GetCurrentEventLoop(),
									kEventDurationSecond * kSession_LifetimeMinimumForNoWarningClose,
									kEventDurationForever/* time between fires - this timer does not repeat */,
									this->longLifeTimerUPP, this->selfRef/* user data */, &this->longLifeTimer);
	
	// create a callback for preferences, then listen for certain preferences
	// (this will also initialize the preferences cache values)
	Preferences_StartMonitoring(this->preferencesListener, kPreferences_TagCursorBlinks,
								true/* call immediately to initialize */);
	Preferences_StartMonitoring(this->preferencesListener, kPreferences_TagMapBackquote,
								true/* call immediately to initialize */);
	Preferences_ContextStartMonitoring(this->configuration, this->preferencesListener, kPreferences_ChangeContextBatchMode);
}// My_Session default constructor


/*!
Destructor.  See Session_Dispose().

(4.0)
*/
My_Session::
~My_Session ()
{
	assert(Session_IsValid(this->selfRef));
	assert(this->status != kSession_StateImminentDisposal);
	
	// give listeners one last chance to respond before the
	// session becomes invalid (starting to destruct)
	this->status = kSession_StateImminentDisposal;
	changeNotifyForSession(this, kSession_ChangeState, this->selfRef/* context */);
	gInvalidSessions().insert(this->selfRef);
	
	// 3.1 - record the time when the command exited; the structure
	// will be deallocated shortly, so this is really only usable
	// by callbacks invoked from this destructor
	this->terminationAbsoluteTime = CFAbsoluteTimeGetCurrent();
	
	if (nullptr != this->mainProcess)
	{
		Local_KillProcess(&this->mainProcess);
	}
	
	if (Session_StateIsActive(this->selfRef))
	{
		for (My_TerminalScreenList::iterator toScreen = this->targetTerminals.begin();
				toScreen != this->targetTerminals.end(); ++toScreen)
		{
			// active session; terminate associated tasks
			if (Terminal_FileCaptureInProgress(*toScreen)) Terminal_FileCaptureEnd(*toScreen);
		}
	}
	
	closeTerminalWindow(this);
	
	// clean up
	(Preferences_Result)Preferences_ContextStopMonitoring(this->configuration, this->preferencesListener,
															kPreferences_ChangeContextBatchMode);
	Preferences_StopMonitoring(this->preferencesListener, kPreferences_TagCursorBlinks);
	Preferences_StopMonitoring(this->preferencesListener, kPreferences_TagMapBackquote);
	ListenerModel_ReleaseListener(&this->preferencesListener);
	Preferences_ReleaseContext(&this->configuration);
	
	Session_StopMonitoring(this->selfRef, kSession_ChangeWindowValid, this->windowValidationListener);
	Session_StopMonitoring(this->selfRef, kSession_ChangeWindowInvalid, this->windowValidationListener);
	
	if (nullptr != this->autoActivateDragTimer)
	{
		RemoveEventLoopTimer(this->autoActivateDragTimer), this->autoActivateDragTimer = nullptr;
	}
	DisposeEventLoopTimerUPP(this->autoActivateDragTimerUPP), this->autoActivateDragTimerUPP = nullptr;
	if (nullptr != this->longLifeTimer)
	{
		RemoveEventLoopTimer(this->longLifeTimer), this->longLifeTimer = nullptr;
	}
	DisposeEventLoopTimerUPP(this->longLifeTimerUPP), this->longLifeTimerUPP = nullptr;
	
	if (kVectorInterpreter_InvalidID != this->vectorGraphicsID)
	{
		VectorInterpreter_Dispose(&this->vectorGraphicsID);
	}
	
	// dispose contents
	// WARNING: Session_SetDataProcessingCapacity() also allocates/deallocates this buffer
	if (this->readBufferPtr != nullptr)
	{
		delete [] this->readBufferPtr, this->readBufferPtr = nullptr;
	}
	ListenerModel_ReleaseListener(&this->windowValidationListener);
	ListenerModel_Dispose(&this->changeListenerModel);
}// My_Session destructor


/*!
Brings the session window to the front.  This is installed when
a drag enters a background window, and is cancelled only if the
drag leaves before the delay is up.  So the effect is that the
window comes to the front after a short hover delay.

Timers that draw must save and restore the current graphics port.

(3.1)
*/
pascal void
autoActivateWindow	(EventLoopTimerRef		UNUSED_ARGUMENT(inTimer),
					 void*					inSessionRef)
{
	SessionRef				ref = REINTERPRET_CAST(inSessionRef, SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	
	
	if (nullptr != ptr->terminalWindow) TerminalWindow_Select(ptr->terminalWindow);
}// autoActivateWindow


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
void
changeNotifyForSession		(My_SessionPtr		inPtr,
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
void
changeStateAttributes	(My_SessionPtr				inPtr,
						 Session_StateAttributes	inAttributesToSet,
						 Session_StateAttributes	inAttributesToClear)
{
	inPtr->statusAttributes |= inAttributesToSet;
	inPtr->statusAttributes &= ~inAttributesToClear;
	changeNotifyForSession(inPtr, kSession_ChangeStateAttributes, inPtr->selfRef/* context */);
}// changeStateAttributes


/*!
Closes the terminal window of the session, if it is
still open, and notifies any listeners that the window
is no longer valid.

(4.0)
*/
void
closeTerminalWindow		(My_SessionPtr	inPtr)
{
	if (nullptr != inPtr->terminalWindow)
	{
		changeNotifyForSession(inPtr, kSession_ChangeWindowInvalid, inPtr->selfRef/* context */);
		// WARNING: This is old-style.  A session does not really “own” a terminal window,
		// it is more logical for a responder to the invalidation event to dispose of the
		// terminal window.
		TerminalWindow_Dispose(&inPtr->terminalWindow);
	}
}// closeTerminalWindow


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
void
configureSaveDialog		(SessionRef					inRef,
						 NavDialogCreationOptions&	inoutOptions)
{
	inoutOptions.optionFlags |= kNavDontAddTranslateItems;
	Localization_GetCurrentApplicationNameAsCFString(&inoutOptions.clientName);
	inoutOptions.preferenceKey = kPreferences_NavPrefKeyGenericSaveFile;
	inoutOptions.parentWindow = Session_ReturnActiveWindow(inRef);
}// configureSaveDialog


/*!
Attempts to read all supported key mapping tags from the given
preference context, and any mappings that exist will be used to
update the specified session.

Returns the number of mappings that were changed.

(4.0)
*/
UInt16
copyEventKeyPreferences		(My_SessionPtr				inPtr,
							 Preferences_ContextRef		inSource,
							 Boolean					inSearchDefaults)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	char					eventKey = '\0';
	UInt16					result = 0;
	
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagKeyInterruptProcess,
												sizeof(eventKey), &eventKey, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		inPtr->eventKeys.interrupt = eventKey;
		++result;
	}
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagKeySuspendOutput,
												sizeof(eventKey), &eventKey, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		inPtr->eventKeys.suspend = eventKey;
		++result;
	}
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagKeyResumeOutput,
												sizeof(eventKey), &eventKey, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		inPtr->eventKeys.resume = eventKey;
		++result;
	}
	
	{
		Boolean		mapCRToCRNull = false;
		
		
		prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagMapCarriageReturnToCRNull,
													sizeof(mapCRToCRNull), &mapCRToCRNull, inSearchDefaults);
		if (kPreferences_ResultOK == prefsResult)
		{
			inPtr->eventKeys.newline = (mapCRToCRNull) ? kSession_NewlineModeMapCRNull : kSession_NewlineModeMapCRLF;
			++result;
		}
	}
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagEmacsMetaKey,
												sizeof(inPtr->eventKeys.meta), &inPtr->eventKeys.meta, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagMapDeleteToBackspace,
												sizeof(inPtr->eventKeys.deleteSendsBackspace),
												&inPtr->eventKeys.deleteSendsBackspace, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagMapArrowsForEmacs,
												sizeof(inPtr->eventKeys.arrowsRemappedForEmacs),
												&inPtr->eventKeys.arrowsRemappedForEmacs, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagPageKeysControlLocalTerminal,
												sizeof(inPtr->eventKeys.pageKeysLocalControl),
												&inPtr->eventKeys.pageKeysLocalControl, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagMapKeypadTopRowForVT220,
												sizeof(inPtr->eventKeys.keypadRemappedForVT220),
												&inPtr->eventKeys.keypadRemappedForVT220, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
	}
	
	return result;
}// copyEventKeyPreferences


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
My_HMHelpContentRecWrap&
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
My_HMHelpContentRecWrap&
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
My_HMHelpContentRecWrap&
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
Returns (creating if necessary) the global help tag record
for a help tag that tells the user that input is now being
redirected to a vector graphics window.

If the title of the tag has not been set already, it is
initialized.

Normally, this tag should be displayed at the terminal
cursor location, so prior to using the result you should
call its setFrame() method.

(4.0)
*/
My_HMHelpContentRecWrap&
createHelpTagForVectorGraphics ()
{
	static My_HMHelpContentRecWrap		gTag;
	
	
	if (nullptr == gTag.mainName())
	{
		CFStringRef		tagCFString = nullptr;
		
		
		if (UIStrings_Copy(kUIStrings_TerminalVectorGraphicsRedirect, tagCFString).ok())
		{
			gTag.rename(tagCFString, nullptr/* alternate name */);
			// you CANNOT release this string because you do not know when the system is done with the tag
		}
	}
	
	return gTag;
}// createHelpTagForVectorGraphics


/*!
Creates a Page Format object that can be used for printing
to any session.  Currently, all sessions use the same one.
This affects things like paper size, etc.

(3.1)
*/
PMPageFormat
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
IconRef
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
IconRef
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
pascal void
detectLongLife	(EventLoopTimerRef		UNUSED_ARGUMENT(inTimer),
				 void*					inSessionRef)
{
	SessionRef				ref = REINTERPRET_CAST(inSessionRef, SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	
	
	if (Session_StateIsActiveUnstable(ref)) Session_SetState(ref, kSession_StateActiveStable);
}// detectLongLife


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
Boolean
handleSessionKeyDown	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
						 ListenerModel_Event	UNUSED_ARGUMENT(inEventThatOccurred),
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
	assert(inEventContextPtr != nullptr);
	assert(inListenerContextPtr != nullptr);
	
	Boolean					result = false;
	My_KeyPressPtr			keyPressInfoPtr = REINTERPRET_CAST(inEventContextPtr, My_KeyPressPtr);
	SessionRef				session = REINTERPRET_CAST(inListenerContextPtr, SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), session);
	TerminalScreenRef		someScreen = ptr->targetTerminals.front(); // TEMPORARY
	static SInt16			characterCode = '\0'; // ASCII
	static SInt16			characterCode2 = '\0'; // ASCII
	static SInt16			virtualKeyCode = '\0'; // see p.2-43 of "IM:MTE" for a set of virtual key codes
	static Boolean			commandDown = false;
	static Boolean			controlDown = false;
	static Boolean			optionDown  = false;
	static Boolean			shiftDown = false;
	static Boolean			metaDown = false;
	static UInt32			eventModifiers = 0;
	
	
	characterCode = keyPressInfoPtr->characterCode;
	characterCode2 = keyPressInfoPtr->characterCode2;
	virtualKeyCode = keyPressInfoPtr->virtualKeyCode;
	commandDown = keyPressInfoPtr->commandDown;
	controlDown = keyPressInfoPtr->controlDown;
	optionDown = keyPressInfoPtr->optionDown;
	shiftDown = keyPressInfoPtr->shiftDown;
	
	// create bit-flag short-cuts that are useful in some cases
	// (TEMPORARY - this is a hack, the input should be cleaned up
	// to simply pass an established set of flags in the first place)
	eventModifiers = 0;
	if (commandDown) eventModifiers |= cmdKey;
	if (controlDown) eventModifiers |= controlKey;
	if (commandDown) eventModifiers |= cmdKey;
	if (optionDown) eventModifiers |= optionKey;
	if (shiftDown) eventModifiers |= shiftKey;
	
	// technically add-on support for an Emacs concept
	if (ptr->eventKeys.meta == kSession_EmacsMetaKeyControlCommand)
	{
		metaDown = ((commandDown) && (controlDown));
	}
	else if (ptr->eventKeys.meta == kSession_EmacsMetaKeyOption)
	{
		metaDown = (optionDown);
	}
	
	// scan for keys that invoke instant commands
	switch (virtualKeyCode)
	{
	case 0x7A: // F1
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF6);
		break;
	
	case 0x78: // F2
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF7);
		break;
	
	case 0x63: // F3
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF8);
		break;
	
	case 0x76: // F4
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF9);
		break;
	
	case 0x60: // F5
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF10);
		break;
	
	case 0x61: // F6
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF11);
		break;
	
	case 0x62: // F7
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF12);
		break;
	
	case 0x64: // F8
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF13);
		break;
	
	case 0x65: // F9
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF14);
		break;
	
	case 0x6D: // F10
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF15);
		break;
	
	case 0x67: // F11
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF16);
		break;
	
	case 0x6F: // F12
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF17);
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
		if ((ptr->eventKeys.pageKeysLocalControl) || Terminal_EmulatorIsVT100(someScreen))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewPageUp);
		}
		break;
	
	case VSPGDN:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((ptr->eventKeys.pageKeysLocalControl) || Terminal_EmulatorIsVT100(someScreen))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewPageDown);
		}
		break;
	
	case VSHOME:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((ptr->eventKeys.pageKeysLocalControl) || Terminal_EmulatorIsVT100(someScreen))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewHome);
		}
		break;
	
	case VSEND:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((ptr->eventKeys.pageKeysLocalControl) || Terminal_EmulatorIsVT100(someScreen))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewEnd);
		}
		break;
	
	case 0x7B: // Left arrow
	case 0x3B: // Left arrow on non-extended keyboards
		Session_UserInputKey(session, VSLT, eventModifiers);
		result = true;
		break;
	
	case 0x7C: // Right arrow
	case 0x3C: // Right arrow on non-extended keyboards
		Session_UserInputKey(session, VSRT, eventModifiers);
		result = true;
		break;
	
	case 0x7D: // Down arrow
	case 0x3D: // Down arrow on non-extended keyboards
		Session_UserInputKey(session, VSDN, eventModifiers);
		result = true;
		break;
	
	case 0x7E: // Up arrow
	case 0x3E: // Up arrow on non-extended keyboards
		Session_UserInputKey(session, VSUP, eventModifiers);
		result = true;
		break;
	
	default:
		// no other virtual key codes have significance
		break;
	}
	
	if (false == result)
	{
		// if no key-based action occurred, look for character-based actions
		if (0 == characterCode2)
		{
			if (characterCode == ptr->eventKeys.suspend)
			{
				Session_SetNetworkSuspended(session, true);
				result = true;
			}
			if (characterCode == ptr->eventKeys.resume) 
			{
				Session_SetNetworkSuspended(session, false);
				result = true;
			}
			if (characterCode == ptr->eventKeys.interrupt)  
			{
				Session_UserInputInterruptProcess(session);
				result = true;
			}
		}
		
		// now check for constant character matches
		if (false == result)
		{
			switch (characterCode)
			{
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
				UInt8		charactersToSend[2];
				UInt8*		characterPtr = charactersToSend;
				size_t		theSize = sizeof(charactersToSend);
				
				
				// perform one final substitution: the meta key in Emacs
				if (metaDown)
				{
					if (characterCode <= 32)
					{
						// control key changed the ASCII value; fix it
						characterCode |= 0x40;
					}
				}
				
				// determine the equivalent characters for the given key presses
				charactersToSend[0] = STATIC_CAST(characterCode, UInt8);
				charactersToSend[1] = STATIC_CAST(characterCode2, UInt8);
				
				if (metaDown)
				{
					// Emacs respects a prefixing Escape character as being
					// equivalent to meta; so ESC-CtrlA is like MetaCtrlA
					charactersToSend[1] = charactersToSend[0];
					charactersToSend[0] = 0x1B; // ESC
				}
				else if (0 == charactersToSend[0])
				{
					// no prefix was given; skip it
					characterPtr = &charactersToSend[1];
					theSize = sizeof(characterPtr[0]);
				}
				else if (0 == charactersToSend[1])
				{
					theSize = sizeof(characterPtr[0]);
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
inline Boolean
isReadOnly		(My_SessionPtr		inPtr)
{
	return inPtr->readOnly;
}// isReadOnly


/*!
Invoked by the Mac OS whenever something interesting
happens in a Navigation Services file-capture-save-dialog
attached to a session window.

(3.0)
*/
pascal void
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
					My_SessionAutoLocker	ptr(gSessionPtrLocks(), session);
					
					
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
							// TEMPORARY - this should be able to use "ptr->targetFiles" and "ptr->targetPrintJobs"
							// TEMPORARY - this command is not capable of handling multiple screens per window
							Terminal_FileCaptureBegin(ptr->targetTerminals.front(), fileRefNum);
							
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
pascal void
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
pascal void
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
void
pasteWarningCloseNotifyProc		(InterfaceLibAlertRef	inAlertThatClosed,
								 SInt16					inItemHit,
								 void*					inMy_PasteAlertInfoPtr)
{
	My_PasteAlertInfoPtr	dataPtr = REINTERPRET_CAST(inMy_PasteAlertInfoPtr, My_PasteAlertInfoPtr);
	
	
	if (dataPtr != nullptr)
	{
		My_SessionAutoLocker	sessionPtr(gSessionPtrLocks(), dataPtr->sessionForPaste);
		
		
		if (inItemHit == kAlertStdAlertOKButton)
		{
			// first join the text into one line (replace new-line sequences
			// with single spaces), then Paste
			Clipboard_TextFromScrap(sessionPtr->selfRef, kClipboard_ModifierOneLine,
									dataPtr->sourcePasteboard.returnPasteboardRef());
		}
		else if (inItemHit == kAlertStdAlertOtherButton)
		{
			// regular Paste, use verbatim what is on the Clipboard
			Clipboard_TextFromScrap(sessionPtr->selfRef, kClipboard_ModifierNone,
									dataPtr->sourcePasteboard.returnPasteboardRef());
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
void
preferenceChanged	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
					 ListenerModel_Event	inPreferenceTagThatChanged,
					 void*					inPreferencesContext,
					 void*					inListenerContextPtr)
{
	Preferences_ContextRef	prefsContext = REINTERPRET_CAST(inPreferencesContext, Preferences_ContextRef);
	SessionRef				ref = REINTERPRET_CAST(inListenerContextPtr, SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	size_t					actualSize = 0L;
	
	
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
		if (kPreferences_ChangeContextBatchMode == inPreferenceTagThatChanged)
		{
			// batch mode; multiple things have changed, so check for the new values
			// of everything that is understood by a terminal view
			(UInt16)copyEventKeyPreferences(ptr, prefsContext, false/* search for defaults */);
		}
		else
		{
			// ???
		}
		break;
	}
}// preferenceChanged


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
processMoreData		(My_SessionPtr	inPtr)
{
	size_t		result = 0;
	
	
	// local session data on Mac OS X is handled in a separate
	// thread using the blocking read() system call, so in that
	// case data is expected to have been placed in the read buffer
	// already by that thread
	Session_ReceiveData(inPtr->selfRef, inPtr->readBufferPtr, inPtr->readBufferSizeInUse);
	//Session_TerminalWrite(inPtr->selfRef, inPtr->readBufferPtr, inPtr->readBufferSizeInUse);
	inPtr->readBufferSizeInUse = 0;
	return result;
}// processMoreData


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
OSStatus
receiveTerminalViewDragDrop		(EventHandlerCallRef	inHandlerCallRef,
								 EventRef				inEvent,
								 void*					inSessionRef)
{
	OSStatus				result = eventNotHandledErr;
	SessionRef				session = REINTERPRET_CAST(inSessionRef, SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), session);
	UInt32 const			kEventClass = GetEventClass(inEvent);
	UInt32 const			kEventKind = GetEventKind(inEvent);
	
	
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
						PasteboardRef	dragPasteboard = nullptr;
						Boolean			acceptDrag = false;
						
						
						result = GetDragPasteboard(dragRef, &dragPasteboard);
						if (noErr == result)
						{
							Boolean		haveSingleFile = false;
							Boolean		haveText = false;
							UInt16		numberOfItems = 0;
							
							
							// read and cache information about this pasteboard
							Clipboard_SetPasteboardModified(dragPasteboard);
							
							// figure out if this drag can be accepted
							if (noErr == CountDragItems(dragRef, &numberOfItems))
							{
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
							}
							
							// check to see if all of the drag items contain TEXT (a drag
							// is only accepted if all of the items in the drag can be
							// accepted)
							unless (haveSingleFile)
							{
								haveText = Clipboard_ContainsText(dragPasteboard);
							}
							
							// determine rules for accepting the drag
							acceptDrag = ((haveText) || (haveSingleFile));
						}
						
						// if the window is inactive, start a hover timer to auto-activate the window
						if ((acceptDrag) && (false == TerminalWindow_IsFocused(ptr->terminalWindow)))
						{
							(OSStatus)InstallEventLoopTimer(GetCurrentEventLoop(),
															kEventDurationSecond/* arbitrary */,
															kEventDurationForever/* time between fires - this timer does not repeat */,
															ptr->autoActivateDragTimerUPP, ptr->selfRef/* user data */,
															&ptr->autoActivateDragTimer);
						}
						
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
					break;
				
				case kEventControlDragWithin:
					TerminalView_SetDragHighlight(view, dragRef, true/* is highlighted */);
					result = noErr;
					break;
				
				case kEventControlDragLeave:
					TerminalView_SetDragHighlight(view, dragRef, false/* is highlighted */);
					if (nullptr != ptr->autoActivateDragTimer)
					{
						(OSStatus)RemoveEventLoopTimer(ptr->autoActivateDragTimer), ptr->autoActivateDragTimer = nullptr;
					}
					result = noErr;
					break;
				
				case kEventControlDragReceive:
					// something was actually dropped
					result = sessionDragDrop(inHandlerCallRef, inEvent, session, view, dragRef);
					if (nullptr != ptr->autoActivateDragTimer)
					{
						(OSStatus)RemoveEventLoopTimer(ptr->autoActivateDragTimer), ptr->autoActivateDragTimer = nullptr;
					}
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
pascal OSStatus
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
pascal OSStatus
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
		EventRef		originalKeyPressEvent = nullptr;
		My_KeyPress		controlKeyPressInfo;
		OSStatus		error = noErr;
		
		
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
				controlKeyPressInfo.characterCode2 = characterCodes[1];
				
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
			if ((controlKeyPressInfo.virtualKeyCode == 0x34) ||
				(controlKeyPressInfo.virtualKeyCode == 0x4C))
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
				My_SessionAutoLocker	ptr(gSessionPtrLocks(), session);
				
				
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
				My_SessionAutoLocker	ptr(gSessionPtrLocks(), session);
				Boolean					sendBackspace = ptr->eventKeys.deleteSendsBackspace;
				
				
				if ((controlKeyPressInfo.optionDown) || (controlKeyPressInfo.commandDown))
				{
					sendBackspace = !sendBackspace;
					
					// clear these modifier keys, because they were only used
					// to alter the effective character and should not have
					// any other special significance
					controlKeyPressInfo.optionDown = false;
					controlKeyPressInfo.commandDown = false;
				}
				
				if (sendBackspace)
				{
					controlKeyPressInfo.characterCode = 0x08; // backspace
				}
				else
				{
					controlKeyPressInfo.characterCode = 0x7F; // delete
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
		handleSessionKeyDown(nullptr/* unused listener model */, 0/* unused event code */,
								&controlKeyPressInfo/* event context */, session/* listener context */);
	}
	
	return result;
}// receiveTerminalViewTextInput


/*!
Handles "kEventWindowClose" of "kEventClassWindow"
for a session’s terminal window.

(3.0)
*/
pascal OSStatus
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
				Session_Dispose(&session);
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
pascal OSStatus
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
			My_SessionAutoLocker	ptr(gSessionPtrLocks(), session);
			
			
			watchClearForSession(ptr);
			
			result = eventNotHandledErr; // pass event to parent handler
		}
	}
	
	return result;
}// receiveWindowFocusChange


/*!
Returns the window most recently used by this session.

DEPRECATED.  There needs to be a Cocoa equivalent for this.

(4.0)
*/
HIWindowRef
returnActiveWindow	(My_SessionPtr		inPtr)
{
	HIWindowRef		result = nullptr;
	
	
	if (Session_IsValid(inPtr->selfRef))
	{
		if (nullptr != inPtr->terminalWindow)
		{
			result = TerminalWindow_ReturnWindow(inPtr->terminalWindow);
		}
	}
	return result;
}// returnActiveWindow


/*!
Responds to "kEventControlDragReceive" of "kEventClassControl"
for a terminal view that is associated with a session.

See receiveTerminalViewDragDrop().

(3.1)
*/
OSStatus
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
			PasteboardRef	dragPasteboard = nullptr;
			
			
			result = GetDragPasteboard(inDrag, &dragPasteboard);
			if (noErr == result)
			{
				Session_Result		pasteResult = kSession_ResultOK;
				Boolean				cursorMovesPriorToDrops = false;
				size_t				actualSize = 0L;
				
				
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
				
				// “type” the text; this could trigger the “multi-line paste” alert
				pasteResult = Session_UserInputPaste(inRef, dragPasteboard);
				if (false == pasteResult.ok())
				{
					Console_Warning(Console_WriteLine, "unable to complete paste from drag");
					Sound_StandardAlert();
				}
				
				// success!
				result = noErr;
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
void
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
					My_SessionAutoLocker	ptr(gSessionPtrLocks(), session);
					
					
					changeNotifyForSession(ptr, kSession_ChangeWindowObscured, session);
				}
			}
			
			if (error)
			{
				//Console_Warning(Console_WriteLine, "unable to notify listeners of obscured-state change");
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
					My_SessionAutoLocker	ptr(gSessionPtrLocks(), session);
					UInt16					newColumnCount = 0;
					UInt16					newRowCount = 0;
					
					
					// determine new size
					TerminalWindow_GetScreenDimensions(terminalWindow, &newColumnCount, &newRowCount);
					
					// send an I/O control message to the TTY informing it that the screen size has changed
					if (nullptr != ptr->mainProcess)
					{
						Local_TerminalResize(Local_ProcessReturnMasterTerminal(ptr->mainProcess),
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
				Console_Warning(Console_WriteLine, "unable to transmit screen dimension changes to window processes");
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

This routine is actually used whether or not there is
an alert, as a convenient way to clean up a session,
regardless.  So, "inAlertThatClosed" might not be
defined; but, "inItemHit" always is, in order to
instruct this routine on how to proceed.  The data
structure is always defined.

(3.0)
*/
void
terminationWarningCloseNotifyProc	(InterfaceLibAlertRef	inAlertThatClosed,
									 SInt16					inItemHit,
									 void*					inTerminateAlertInfoPtr)
{
	My_TerminateAlertInfoPtr	dataPtr = REINTERPRET_CAST(inTerminateAlertInfoPtr, My_TerminateAlertInfoPtr);
	
	
	if (Session_IsValid(dataPtr->sessionBeingTerminated))
	{
		Boolean						destroySession = false;
		
		
		if (nullptr != dataPtr)
		{
			My_SessionAutoLocker			sessionPtr(gSessionPtrLocks(), dataPtr->sessionBeingTerminated);
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
				Boolean		toolbarHidden = (false == IsWindowToolbarVisible(returnActiveWindow(sessionPtr)));
				
				
				(Preferences_Result)Preferences_SetData(kPreferences_TagHeadersCollapsed,
														sizeof(toolbarHidden), &toolbarHidden);
				
				// flag to destroy the session; this cannot occur within this block,
				// because the session pointer is still locked
				destroySession = true;
			}
			
			// clean up
			sessionPtr->currentTerminationAlert = nullptr; // now it is legal to display a different warning sheet
		}
		
		if (destroySession)
		{
			// this call will terminate the running process and
			// immediately close the associated window
			Session_Dispose(&dataPtr->sessionBeingTerminated);
		}
	}
	
	// clean up
	if (nullptr != dataPtr)
	{
		// see Session_DisplayTerminationWarning() for the fact that
		// this structure is allocated with operator "new"
		delete dataPtr, dataPtr = nullptr;
	}
	
	// dispose of the alert
	if (nullptr != inAlertThatClosed)
	{
		Alert_StandardCloseNotifyProc(inAlertThatClosed, inItemHit, nullptr/* user data */);
	}
}// terminationWarningCloseNotifyProc


/*!
Closes the global modeless alert for notifications on the
specified session, and clears any other indicators for a
triggered notification on that session.

(3.1)
*/
void
watchClearForSession	(My_SessionPtr		inPtr)
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
void
watchCloseNotifyProc	(AlertMessages_BoxRef	inAlertThatClosed,
						 SInt16					inItemHit,
						 void*					inMy_WatchAlertInfoPtr)
{
	My_WatchAlertInfoPtr	dataPtr = REINTERPRET_CAST(inMy_WatchAlertInfoPtr, My_WatchAlertInfoPtr);
	
	
	if (nullptr != dataPtr)
	{
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), dataPtr->sessionBeingWatched);
		
		
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
void
watchNotifyForSession	(My_SessionPtr	inPtr,
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
				CFStringRef				growlNotificationName = nullptr;
				CFStringRef				growlNotificationTitle = nullptr;
				CFStringRef				dialogTextCFString = nullptr;
				UIStrings_Result		stringResult = kUIStrings_ResultOK;
				Boolean					displayGrowl = CocoaBasic_GrowlIsAvailable();
				Boolean					displayNormal = (false == displayGrowl);
				
				
				// set message based on watch type
				if (kSession_WatchForInactivity == inWhatTriggered)
				{
					growlNotificationName = CFSTR("Session idle"); // MUST match "Growl Registration Ticket.growlRegDict"
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowNotifyInactivityTitle, growlNotificationTitle);
					if (false == stringResult.ok())
					{
						growlNotificationTitle = growlNotificationName;
						CFRetain(growlNotificationTitle);
					}
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowNotifyInactivityPrimaryText, dialogTextCFString);
				}
				else
				{
					growlNotificationName = CFSTR("Session active"); // MUST match "Growl Registration Ticket.growlRegDict"
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowNotifyActivityTitle, growlNotificationTitle);
					if (false == stringResult.ok())
					{
						growlNotificationTitle = growlNotificationName;
						CFRetain(growlNotificationTitle);
					}
					stringResult = UIStrings_Copy(kUIStrings_AlertWindowNotifyActivityPrimaryText, dialogTextCFString);
				}
				
				if (displayGrowl)
				{
					// page Growl and then clear immediately, instead of waiting
					// for the user to respond
					CocoaBasic_GrowlNotify(growlNotificationName, growlNotificationTitle,
											dialogTextCFString/* description */);
					watchClearForSession(inPtr);
				}
				
				if (displayNormal)
				{
					My_WatchAlertInfoPtr	watchAlertInfoPtr = new My_WatchAlertInfo;
					
					
					// basic setup
					watchAlertInfoPtr->sessionBeingWatched = inPtr->selfRef;
					if (nullptr == inPtr->watchBox)
					{
						inPtr->watchBox = Alert_New();
						Alert_SetParamsFor(inPtr->watchBox, kAlert_StyleOK);
						Alert_SetType(inPtr->watchBox, kAlertNoteAlert);
					}
					
					if (stringResult.ok())
					{
						Alert_SetTextCFStrings(inPtr->watchBox, dialogTextCFString, CFSTR(""));
					}
					
					// show the message; it is disposed asynchronously
					Alert_MakeModeless(inPtr->watchBox, watchCloseNotifyProc, watchAlertInfoPtr/* context */);
					Alert_Display(inPtr->watchBox);
				}
				
				if (nullptr != growlNotificationTitle)
				{
					CFRelease(growlNotificationTitle), growlNotificationTitle = nullptr;
				}
				if (nullptr != dialogTextCFString)
				{
					CFRelease(dialogTextCFString), dialogTextCFString = nullptr;
				}
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
pascal void
watchNotifyFromTimer	(EventLoopTimerRef		UNUSED_ARGUMENT(inTimer),
						 void*					inSessionRef)
{
	SessionRef				ref = REINTERPRET_CAST(inSessionRef, SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	
	
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
void
watchTimerResetForSession	(My_SessionPtr	inPtr,
							 Session_Watch	inWatchType)
{
	OSStatus	error = noErr;
	
	
	if (kSession_WatchForKeepAlive == inWatchType)
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		size_t				actualSize = 0L;
		UInt16				intValue = 0;
		
		
		prefsResult = Preferences_GetData(kPreferences_TagKeepAlivePeriodInMinutes, sizeof(intValue),
											&intValue, &actualSize);
		if (kPreferences_ResultOK != prefsResult)
		{
			// set an arbitrary default value
			intValue = 10;
		}
		
		{
			// an arbitrary length of dead time must elapse before a session
			// is considered inactive and triggers a notification
			EventTimerInterval const	kTimeBeforeInactive = kEventDurationMinute * intValue;
			
			
			// install or reset timer to trigger the no-activity notification when appropriate
			assert(nullptr != inPtr->inactivityWatchTimer);
			error = SetEventLoopTimerNextFireTime(inPtr->inactivityWatchTimer, kTimeBeforeInactive);
			assert_noerr(error);
		}
	}
	else if (kSession_WatchForInactivity == inWatchType)
	{
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		size_t				actualSize = 0L;
		UInt16				intValue = 0;
		
		
		prefsResult = Preferences_GetData(kPreferences_TagIdleAfterInactivityInSeconds, sizeof(intValue),
											&intValue, &actualSize);
		if (kPreferences_ResultOK != prefsResult)
		{
			// set an arbitrary default value
			intValue = 30;
		}
		
		{
			// an arbitrary length of dead time must elapse before a session
			// is considered inactive and triggers a notification
			EventTimerInterval const	kTimeBeforeInactive = kEventDurationSecond * intValue;
			
			
			// install or reset timer to trigger the no-activity notification when appropriate
			assert(nullptr != inPtr->inactivityWatchTimer);
			error = SetEventLoopTimerNextFireTime(inPtr->inactivityWatchTimer, kTimeBeforeInactive);
			assert_noerr(error);
		}
	}
}// watchTimerResetForSession


/*!
Invoked whenever a monitored session’s window has just
been created, or is about to be destroyed.  This routine
responds by installing or removing window-dependent event
handlers for the session.

(3.0)
*/
void
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
			SessionRef				session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			My_SessionAutoLocker	ptr(gSessionPtrLocks(), session);
			
			
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
			SessionRef				session = REINTERPRET_CAST(inEventContextPtr, SessionRef);
			My_SessionAutoLocker	ptr(gSessionPtrLocks(), session);
			
			
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

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
