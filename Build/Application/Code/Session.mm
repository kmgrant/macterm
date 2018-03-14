/*!	\file Session.mm
	\brief The front end to a particular running command
	or server connection.  This remains valid as long as
	the session is defined, even if the command has
	terminated or all its windows are temporarily hidden.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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

#import "Session.h"
#import <UniversalDefines.h>

// standard-C includes
#import <cctype>
#import <cstring>
#import <sstream>
#import <string>

// standard-C++ includes
#import <algorithm>
#import <map>
#import <set>
#import <vector>

// Unix includes
#import <strings.h>

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <AlertMessages.h>
#import <AppleEventUtilities.h>
#import <CFRetainRelease.h>
#import <CFUtilities.h>
#import <CarbonEventUtilities.template.h>
#import <CocoaAnimation.h>
#import <CocoaBasic.h>
#import <Console.h>
#import <GrowlSupport.h>
#import <ListenerModel.h>
#import <Localization.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlocks.h>
#import <Panel.h>
#import <RegionUtilities.h>
#import <SoundSystem.h>
#import <StringUtilities.h>
#import <WindowTitleDialog.h>

// application includes
#import "AppResources.h"
#import "Clipboard.h"
#import "Commands.h"
#import "DialogUtilities.h"
#import "FileUtilities.h"
#import "GenericDialog.h"
#import "Local.h"
#import "MacroManager.h"
#import "NetEvents.h"
#import "Preferences.h"
#import "PrefPanelSessions.h"
#import "QuillsSession.h"
#import "SessionDescription.h"
#import "SessionFactory.h"
#import "Terminal.h"
#import "TerminalView.h"
#import "TerminalWindow.h"
#import "TextTranslation.h"
#import "UIStrings.h"
#import "VectorCanvas.h"
#import "VectorInterpreter.h"
#import "VectorWindow.h"
#import "VTKeys.h"



#pragma mark Constants
namespace {

enum Session_Type
{
	kSession_TypeLocalNonLoginShell		= 0,	// local Unix process running the user’s preferred shell
	kSession_TypeLocalLoginShell		= 1		// local Unix process running /usr/bin/login
};

/*!
Specifies the type of sheet (if any) that is currently
displayed.  This is currently used to disable key recognition
when the Special Key Sequences sheet is open.
*/
enum My_SessionSheetType
{
	kMy_SessionSheetTypeNone					= 0,
	kMy_SessionSheetTypeSpecialKeySequences		= 1
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
	UInt32			virtualKeyCode;		//!< code uniquifying the key pressed
	Boolean			commandDown;		//!< the state of the Command modifier key
	Boolean			controlDown;		//!< the state of the Control modifier key
	Boolean			optionDown;			//!< the state of the Option modifier key
	Boolean			shiftDown;			//!< the state of the Shift modifier key
};
typedef My_KeyPress*	My_KeyPressPtr;

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
		RegionUtilities_SetRect(&_rec.absHotRect, STATIC_CAST(inNewFrame.origin.x, short), STATIC_CAST(inNewFrame.origin.y, short),
								0, 0);
		RegionUtilities_SetRect(&_rec.absHotRect, _rec.absHotRect.left, _rec.absHotRect.top,
								_rec.absHotRect.left + STATIC_CAST(inNewFrame.size.width, short),
								_rec.absHotRect.top + STATIC_CAST(inNewFrame.size.height, short));
	}

private:
	HMHelpContentRec	_rec;
	CFStringRef			_mainName;	// unlike the record name, this can be nullptr
	CFStringRef			_altName;	// unlike the record name, this can be nullptr
};

typedef std::vector< TerminalScreenRef >		My_PrintJobList;

typedef std::vector< VectorInterpreter_Ref >	My_TEKGraphicList;

typedef std::vector< TerminalScreenRef >		My_TerminalScreenList;

typedef std::map< HIViewRef, EventHandlerRef >	My_TextInputHandlerByView;

typedef std::set< VectorWindow_Ref >			My_VectorWindowSet;

/*!
The data structure that is known as a "SessionRef" to
any code outside this module.  See Session_New().
*/
struct My_Session
{
	My_Session		(Preferences_ContextRef, Boolean);
	~My_Session		();
	
	Preferences_ContextWrap		configuration;				// not always in sync; see Session_ReturnConfiguration()
	Preferences_ContextWrap		translationConfiguration;	// not always in sync; see Session_ReturnTranslationConfiguration()
	Boolean						readOnly;					// whether or not user input is allowed
	Boolean						restartInProgress;			// a session restart is underway; prevents the death of the process from destroying the session
	Session_Type				kind;						// type of session; affects use of the union below
	Session_State				status;						// indicates whether session is initialized, etc.
	Session_StateAttributes		statusAttributes;			// “tags” for the status, above
	CFRetainRelease				statusIconName;				// basename of image file for icon that describes the state and/or attributes
	CFRetainRelease				statusString;				// one word (usually) describing the state succinctly
	CFRetainRelease				alternateTitle;				// user-defined window title
	CFRetainRelease				resourceLocationString;		// one-liner for remote URL or local Unix command line
	CFRetainRelease				commandLineArguments;		// CFArrayRef of CFStringRef; typically agrees with "resourceLocationString"
	CFRetainRelease				originalDirectoryString;	// pathname of the directory that was current when the session was executed
	CFRetainRelease				deviceNameString;			// pathname of slave pseudo-terminal device attached to the session
	CFRetainRelease				pendingPasteLines;			// CFArrayRef of CFStringRef; lines that have not yet been pasted
	CFIndex						pendingPasteCurrentLine;	// index into "pendingPasteLines" of current line to be pasted
	CFAbsoluteTime				activationAbsoluteTime;		// result of CFAbsoluteTimeGetCurrent() call when the command starts or restarts
	CFAbsoluteTime				terminationAbsoluteTime;	// result of CFAbsoluteTimeGetCurrent() call when the command ends
	CFAbsoluteTime				watchTriggerAbsoluteTime;	// result of CFAbsoluteTimeGetCurrent() call when the last watch of any kind went off
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
	ListenerModel_ListenerWrap	windowValidationListener;	// responds after a window is created, and just before it dies
	ListenerModel_ListenerWrap	terminalWindowListener;		// responds when terminal window states change
	ListenerModel_ListenerWrap	vectorWindowListener;		// responds when vector graphics window states change
	ListenerModel_ListenerWrap	preferencesListener;		// responds when certain preference values are initialized or changed
	EventLoopTimerUPP			autoActivateDragTimerUPP;	// procedure that is called when a drag hovers over an inactive window
	EventLoopTimerRef			autoActivateDragTimer;		// short timer
	EventLoopTimerUPP			longLifeTimerUPP;			// procedure that is called when a session has been open 15 seconds
	EventLoopTimerRef			longLifeTimer;				// 15-second timer
	EventLoopTimerUPP			respawnSessionTimerUPP;		// procedure that is called when a session should be respawned
	EventLoopTimerRef			respawnSessionTimer;		// short timer
	MemoryBlocks_WeakPairWrap
	< SessionRef,
		GenericDialog_Ref >		currentDialog;				// weak reference while a sheet is still open so a 2nd sheet is not displayed
	TerminalWindowRef			terminalWindow;				// terminal window housing this session
	Local_ProcessRef			mainProcess;				// the command whose output is directly attached to the terminal
	Session_EventKeys			eventKeys;					// information on keyboard short-cuts for major events
	My_TEKGraphicList			targetVectorGraphics;		// list of TEK graphics attached to this session
	My_TerminalScreenList		targetDumbTerminals;		// list of DUMB terminals to which incoming data is being copied
	My_TerminalScreenList		targetTerminals;			// list of screen buffers to which incoming data is being copied
	CFRetainRelease				autoCaptureFileName;		// if defined, the name or template name for an automatically-created capture file
	CFRetainRelease				autoCaptureDirectoryURL;	// if defined, URL to directory in which to automatically create capture file
	Boolean						autoCaptureToFile;			// if set, session automatically starts a file capture
	Boolean						autoCaptureIsTemplateName;	// if set, file name is actually a pattern that can substitute date, time, etc.
	Boolean						vectorGraphicsPageOpensNewWindow;	// true if a TEK PAGE opens a new window instead of clearing the current one
	VectorInterpreter_Mode		vectorGraphicsCommandSet;	// e.g. TEK 4014 or 4105
	My_VectorWindowSet			vectorGraphicsWindows;		// window controllers for open canvas windows, if any
	VectorInterpreter_Ref		vectorGraphicsInterpreter;	// the ID of the current graphic, if any; see "VectorInterpreter.h"
	size_t						readBufferSizeMaximum;		// maximum number of bytes that can be processed at once
	size_t						readBufferSizeInUse;		// number of bytes of data currently in the read buffer
	UInt8*						readBufferPtr;				// buffer space for processing data
	CFStringEncoding			writeEncoding;				// the character set that text (data) sent to a session should be using
	Session_Watch				activeWatch;				// if any, what notification is currently set up for internal data events
	EventLoopTimerUPP			inactivityWatchTimerUPP;	// procedure that is called if data has not arrived after awhile
	EventLoopTimerRef			inactivityWatchTimer;		// short timer
	EventLoopTimerUPP			pendingPasteTimerUPP;		// procedure that is called periodically when pasting lines
	EventLoopTimerRef			pendingPasteTimer;			// paste timer, reset only when a Paste or an equivalent (like a drag) occurs
	Preferences_ContextWrap		recentSheetContext;			// defined temporarily while a Preferences-dependent sheet (such as key sequences) is up
	My_SessionSheetType			sheetType;					// if "kMy_SessionSheetTypeNone", no significant sheet is currently open
	WindowTitleDialog_Ref		renameDialog;				// if defined, the user interface for renaming the terminal window
	Memory_WeakRefEraser		weakRefEraser;				// at destruction time, clears weak references that involve this object
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
typedef MemoryBlockReferenceTracker< SessionRef >		My_SessionRefTracker;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void						autoActivateWindow					(EventLoopTimerRef, void*);
Boolean						autoCaptureSessionToFile			(My_SessionPtr);
Boolean						captureToFile						(My_SessionPtr, CFURLRef, CFStringRef);
void						changeNotifyForSession				(My_SessionPtr, Session_Change, void*);
void						changeStateAttributes				(My_SessionPtr, Session_StateAttributes,
																 Session_StateAttributes);
void						closeTerminalWindow					(My_SessionPtr);
UInt16						copyAutoCapturePreferences			(My_SessionPtr, Preferences_ContextRef, Boolean);
UInt16						copyEventKeyPreferences				(My_SessionPtr, Preferences_ContextRef, Boolean);
UInt16						copyVectorGraphicsPreferences		(My_SessionPtr, Preferences_ContextRef, Boolean);
My_HMHelpContentRecWrap&	createHelpTagForInterrupt			();
My_HMHelpContentRecWrap&	createHelpTagForLocalEcho			();
My_HMHelpContentRecWrap&	createHelpTagForSuspend				();
My_HMHelpContentRecWrap&	createHelpTagForVectorGraphics		();
IconRef						createSessionStateActiveIcon		();
IconRef						createSessionStateDeadIcon			();
void						detectLongLife						(EventLoopTimerRef, void*);
void						handleSaveFromPanel				(My_SessionPtr, NSSavePanel*);
Boolean						handleSessionKeyDown				(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
Boolean						isReadOnly							(My_SessionPtr);
void						localEchoKey						(My_SessionPtr, UInt8);
void						localEchoString						(My_SessionPtr, CFStringRef);
void						pasteLineFromTimer					(EventLoopTimerRef, void*);
void						preferenceChanged					(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
size_t						processMoreData						(My_SessionPtr);
OSStatus					receiveTerminalViewDragDrop			(EventHandlerCallRef, EventRef, void*);
OSStatus					receiveTerminalViewEntered			(EventHandlerCallRef, EventRef, void*);
OSStatus					receiveTerminalViewTextInput		(EventHandlerCallRef, EventRef, void*);
OSStatus					receiveWindowClosing				(EventHandlerCallRef, EventRef, void*);
OSStatus					receiveWindowFocusChange			(EventHandlerCallRef, EventRef, void*);
void						respawnSession						(EventLoopTimerRef, void*);
HIWindowRef					returnActiveLegacyCarbonWindow		(My_SessionPtr);
NSWindow*					returnActiveNSWindow				(My_SessionPtr);
OSStatus					sessionDragDrop						(EventHandlerCallRef, EventRef, SessionRef,
																 HIViewRef, DragRef);
void						setIconFromState					(My_SessionPtr);
void						sheetClosed							(GenericDialog_Ref, Boolean);
Preferences_ContextRef		sheetContextBegin					(My_SessionPtr, Quills::Prefs::Class,
																 My_SessionSheetType);
void						sheetContextEnd						(My_SessionPtr);
void						terminalHoverLocalEchoString		(My_SessionPtr, UInt8 const*, size_t);
void						terminalInsertLocalEchoString		(My_SessionPtr, UInt8 const*, size_t);
void						terminalWindowChanged				(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
void						terminationWarningClose				(SessionRef&, Boolean, Boolean);
Boolean						vectorGraphicsCreateTarget			(My_SessionPtr);
void						vectorGraphicsDetachTarget			(My_SessionPtr);
void						vectorGraphicsWindowChanged			(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);
void						watchClearForSession				(My_SessionPtr);
void						watchNotifyForSession				(My_SessionPtr, Session_Watch);
void						watchNotifyFromTimer				(EventLoopTimerRef, void*);
void						watchTimerResetForSession			(My_SessionPtr, Session_Watch);
void						windowValidationStateChanged		(ListenerModel_Ref, ListenerModel_Event,
																 void*, void*);

} // anonymous namespace

#pragma mark Variables
namespace {

My_SessionPtrLocker&	gSessionPtrLocks ()	{ static My_SessionPtrLocker x; return x; }
IconRef					gSessionActiveIcon () { static IconRef x = createSessionStateActiveIcon(); return x; }
IconRef					gSessionDeadIcon () { static IconRef x = createSessionStateDeadIcon(); return x; }
My_SessionRefTracker&	gInvalidSessions () { static My_SessionRefTracker x; return x; }

} // anonymous namespace

#pragma mark Functors
namespace {

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
		size_t			i = 0;
		UInt8 const*	currentCharPtr = _buffer;
		
		
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
	operator()	(VectorInterpreter_Ref		inTEKWindowID)
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
Destroys a session created with Session_New(), unless the
session is in use by something else.  Either way, your copy of
the reference is set to nullptr.

IMPORTANT:	Since it is possible (and a user preference) for a
			window to stay open after a process has terminated,
			you should only terminate sessions by calling
			Session_SetState() with "kSession_StateDead".  If
			you do destroy a session however it will terminate
			the process correctly.

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
			
			
			// ignore requests to destroy the session while a
			// restart is underway, since the process is not
			// really “dying” in the typical sense
			if (false == ptr->restartInProgress)
			{
				delete ptr;
			}
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
			
			
			ptr->targetVectorGraphics.push_back(REINTERPRET_CAST(inTargetData, VectorInterpreter_Ref));
			assert(ptr->targetVectorGraphics.size() == (1 + listSize));
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

DEPRECATED.  Use Session_GetStateIconName(), instead.

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
			UNUSED_RETURN(OSStatus)GetIconRef(kOnSystemDisk, kSystemIconsCreator,
												kAlertCautionIcon, &outCopiedIcon);
			break;
		
		case kSession_StateAttributeOpenDialog:
			UNUSED_RETURN(OSStatus)GetIconRef(kOnSystemDisk, kSystemIconsCreator,
												kAlertCautionIcon, &outCopiedIcon);
			break;
		
		default:
			break;
		}
		
		if (nullptr == outCopiedIcon)
		{
			// return some non-success value...
			result = kSession_ResultParameterError;
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
	NSSavePanel*		savePanel = [NSSavePanel savePanel];
	CFRetainRelease		saveFileCFString(UIStrings_ReturnCopy(kUIStrings_FileDefaultCaptureFile),
											CFRetainRelease::kAlreadyRetained);
	CFRetainRelease		promptCFString(UIStrings_ReturnCopy(kUIStrings_SystemDialogPromptCaptureToFile),
										CFRetainRelease::kAlreadyRetained);
	
	
	[savePanel setMessage:BRIDGE_CAST(promptCFString.returnCFStringRef(), NSString*)];
	[savePanel setDirectory:nil];
	[savePanel setNameFieldStringValue:BRIDGE_CAST(saveFileCFString.returnCFStringRef(), NSString*)];
	[savePanel beginSheetModalForWindow:Session_ReturnActiveNSWindow(inRef)
				completionHandler:^(NSInteger aReturnCode)
				{
					if (NSFileHandlingPanelOKButton == aReturnCode)
					{
						My_SessionAutoLocker		ptr(gSessionPtrLocks(), inRef);
						
						
						// save file
						if (false == captureToFile(ptr, BRIDGE_CAST(savePanel.directoryURL, CFURLRef),
													BRIDGE_CAST(savePanel.nameFieldStringValue, CFStringRef)))
						{
							Sound_StandardAlert();
							Console_Warning(Console_WriteLine, "failed to initiate capture to file");
						}
						else
						{
							// UNIMPLEMENTED: set window proxy icon to match opened file?
						}
					}
				}];
}// DisplayFileCaptureSaveDialog


/*!
When a file has been downloaded (e.g. the terminal
intercepts a sequence with embedded data), call this to
show the name of the file in a temporary user interface.

Currently this is implemented by briefly showing the
file name in a floating “tool tip” at the cursor location,
which will fade away after a short time. 

(2017.12)
*/
void
Session_DisplayFileDownloadNameUI	(SessionRef		inRef,
									 CFStringRef	inFileName)
{
	My_HMHelpContentRecWrap&	tagData = createHelpTagForLocalEcho(); // reuse the “local echo” tag
	HIRect						globalCursorBounds;
	
	
	tagData.rename(inFileName, nullptr/* alternate text */);
	TerminalView_GetCursorGlobalBounds(TerminalWindow_ReturnViewWithFocus
										(Session_ReturnActiveTerminalWindow(inRef)),
										globalCursorBounds);
	tagData.setFrame(globalCursorBounds);
	UNUSED_RETURN(OSStatus)HMDisplayTag(tagData.ptr());
	// this call does not immediately hide the tag, but rather after a short delay
	UNUSED_RETURN(OSStatus)HMHideTagWithOptions(kHMHideTagFade);
}// DisplayFileDownloadNameUI


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
	NSSavePanel*		savePanel = [NSSavePanel savePanel];
	CFRetainRelease		saveFileCFString(UIStrings_ReturnCopy(kUIStrings_FileDefaultSession),
											CFRetainRelease::kAlreadyRetained);
	CFRetainRelease		promptCFString(UIStrings_ReturnCopy(kUIStrings_SystemDialogPromptSaveSession),
										CFRetainRelease::kAlreadyRetained);
	
	
	[savePanel setMessage:BRIDGE_CAST(promptCFString.returnCFStringRef(), NSString*)];
	[savePanel setDirectory:nil];
	[savePanel setNameFieldStringValue:BRIDGE_CAST(saveFileCFString.returnCFStringRef(), NSString*)];
	[savePanel beginSheetModalForWindow:Session_ReturnActiveNSWindow(inRef)
				completionHandler:^(NSInteger aReturnCode)
				{
					if (NSFileHandlingPanelOKButton == aReturnCode)
					{
						My_SessionAutoLocker		ptr(gSessionPtrLocks(), inRef);
						
						
						handleSaveFromPanel(ptr, savePanel);
					}
				}];
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
	My_SessionAutoLocker		ptr(gSessionPtrLocks(), inRef);
	Preferences_ContextRef		temporaryContext = sheetContextBegin(ptr, Quills::Prefs::SESSION,
																		kMy_SessionSheetTypeSpecialKeySequences);
	
	
	if (nullptr == temporaryContext)
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "failed to construct temporary sheet context");
	}
	else
	{
		GenericDialog_Wrap						dialog;
		PrefPanelSessions_KeyboardViewManager*	embeddedPanel = [[PrefPanelSessions_KeyboardViewManager alloc] init];
		CFRetainRelease							cancelString(UIStrings_ReturnCopy(kUIStrings_ButtonCancel),
																CFRetainRelease::kAlreadyRetained);
		CFRetainRelease							okString(UIStrings_ReturnCopy(kUIStrings_ButtonOK),
															CFRetainRelease::kAlreadyRetained);
		HIWindowRef								carbonWindow = Session_ReturnActiveLegacyCarbonWindow(inRef);
		
		
		if (nullptr != carbonWindow)
		{
			dialog = GenericDialog_Wrap(GenericDialog_NewParentCarbon(carbonWindow, embeddedPanel, temporaryContext),
										GenericDialog_Wrap::kAlreadyRetained);
		}
		else
		{
			dialog = GenericDialog_Wrap(GenericDialog_New(Session_ReturnActiveNSWindow(inRef).contentView, embeddedPanel, temporaryContext),
										GenericDialog_Wrap::kAlreadyRetained);
		}
		[embeddedPanel release], embeddedPanel = nil; // panel is retained by the call above
		GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton1, okString.returnCFStringRef());
		GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton1,
											^{ sheetClosed(dialog.returnRef(), true/* is OK */); });
		GenericDialog_SetItemTitle(dialog.returnRef(), kGenericDialog_ItemIDButton2, cancelString.returnCFStringRef());
		GenericDialog_SetItemResponseBlock(dialog.returnRef(), kGenericDialog_ItemIDButton2,
											^{ sheetClosed(dialog.returnRef(), false/* is OK */); });
		GenericDialog_SetImplementation(dialog.returnRef(), inRef);
		// TEMPORARY; maybe Session_Retain/Release concept needs to be
		// implemented and called here; for now, assume that the session
		// will remain valid as long as the dialog exists (that ought to
		// be the case)
		GenericDialog_Display(dialog.returnRef(), true/* animated */, ^{}); // retains dialog until it is dismissed
	}
}// DisplaySpecialKeySequencesDialog


/*!
Displays a warning alert for terminating the given terminal
window’s session, handling the user’s response automatically.
You may however be notified of a cancellation by using the
parameter "inCancelAction".

Note that if the window was only recently opened, the action
(such as closing the window, restart or force-quit) will take
effect immediately without even displaying a warning.

There are three basic kinds of messages that are possible
(and they automatically have the expected effect on the
session if the user clicks the action button):

- “Force-Quit Processes” is enabled if you specify the:
  "kSession_TerminationDialogOptionKeepWindow" option
  without "kSession_TerminationDialogOptionRestart".

- “Restart” is enabled if you specify both of these options:
  "kSession_TerminationDialogOptionKeepWindow" and
  "kSession_TerminationDialogOptionRestart".  This is
  handled by SessionFactory_RespawnSession().

- “Close” is the default if you do not specify options to
  enable “Restart” or “Force-Quit Processes”.

In addition to the above options for choosing the basic
message type and action, there are additional options:

- "kSession_TerminationDialogOptionModal" forces the message
  to appear in an application-modal window (otherwise, it is
  a window-modal sheet).  When application-modal, this
  function will not return until an action has been taken
  (either to proceed or to Cancel).  When window-modal, the
  function returns immediately but the action could be taken
  at any future time, handled asynchronously.

(3.0)
*/
void
Session_DisplayTerminationWarning	(SessionRef							inRef,
									 Session_TerminationDialogOptions	inOptions,
									 void								(^inCancelAction)())
{
	Boolean const		kForceModal = (0 != (inOptions & kSession_TerminationDialogOptionModal));
	Boolean const		kKeepWindow = (0 != (inOptions & kSession_TerminationDialogOptionKeepWindow));
	Boolean const		kRestart = (0 != (inOptions & kSession_TerminationDialogOptionRestart));
	Boolean const		kNoAlertAnimation = (0 != (inOptions & kSession_TerminationDialogOptionNoAlertAnimation));
	Boolean const		kIsRestartCommand = ((kRestart) && (kKeepWindow));
	Boolean const		kIsKillCommand = ((false == kRestart) && (kKeepWindow));
	Boolean const		kIsCloseCommand = ((false == kIsRestartCommand) && (false == kIsKillCommand));
	Boolean const		kWillRemoveTerminalWindow = kIsCloseCommand;
	
	
	assert((kIsRestartCommand) || (kIsKillCommand) || (kIsCloseCommand));
	
	if (Session_StateIsActiveUnstable(inRef) || Session_StateIsDead(inRef))
	{
		// the process JUST started or is already dead so kill the window without confirmation
		terminationWarningClose(inRef, kKeepWindow, kRestart);
	}
	else
	{
		AlertMessages_BoxWrap	box;
		TerminalWindowRef		terminalWindow = nullptr;
		HIWindowRef				carbonWindow = Session_ReturnActiveLegacyCarbonWindow(inRef);
		NSWindow*				window = ((nullptr == carbonWindow) ? Session_ReturnActiveNSWindow(inRef) : nil);
		Rect					originalStructureBounds;
		Rect					centeredStructureBounds;
		NSRect					originalFrame;
		NSRect					centeredFrame;
		__block Boolean			wasCancelled = false;
		
		
		if (kForceModal)
		{
			box = AlertMessages_BoxWrap(Alert_NewApplicationModal(), AlertMessages_BoxWrap::kAlreadyRetained);
		}
		else if (nullptr != carbonWindow)
		{
			box = AlertMessages_BoxWrap(Alert_NewWindowModalParentCarbon(carbonWindow/* parent */),
										AlertMessages_BoxWrap::kAlreadyRetained);
		}
		else
		{
			box = AlertMessages_BoxWrap(Alert_NewWindowModal(window/* parent */),
										AlertMessages_BoxWrap::kAlreadyRetained);
		}
		
		if (nullptr != carbonWindow)
		{
			OSStatus	error = noErr;
			
			
			error = GetWindowBounds(carbonWindow, kWindowStructureRgn, &originalStructureBounds);
			if (noErr != error)
			{
				RegionUtilities_SetRect(&originalStructureBounds, 0, 0, 0, 0);
			}
		}
		else
		{
			originalFrame = window.frame;
		}
		
		// TEMPORARY - this should really take into account whether the quit event is interactive
		{
			// for modal dialogs, first move the window to the center of the
			// screen (so the user can see which window is being referred to);
			// also remember its original location, in case the user cancels;
			// but quell the animation if this window should be closed without
			// warning (so-called active unstable) or if only one window is
			// open that would cause an alert to be displayed
			if ((kForceModal) && (kWillRemoveTerminalWindow) &&
				(false == Session_StateIsActiveUnstable(inRef)) &&
				((SessionFactory_ReturnCount() - SessionFactory_ReturnStateCount(kSession_StateActiveUnstable)) > 1))
			{
				SInt16 const	kOffsetFromCenterV = -130; // in pixels; arbitrary
				SInt16 const	kAbsoluteMinimumV = 30; // in pixels; arbitrary
				
				
				// center the window on the screen, but slightly offset toward the top half;
				// do not allow the window to go off of the screen, however
				if (nullptr != carbonWindow)
				{
					Rect		availablePositioningBounds;
					OSStatus	error = noErr;
					
					
					RegionUtilities_GetPositioningBounds(carbonWindow, &availablePositioningBounds);
					centeredStructureBounds = originalStructureBounds;
					RegionUtilities_CenterRectIn(&centeredStructureBounds, &availablePositioningBounds);
					centeredStructureBounds.top += kOffsetFromCenterV;
					centeredStructureBounds.bottom += kOffsetFromCenterV;
					if (centeredStructureBounds.top < kAbsoluteMinimumV)
					{
						SInt16 const	kWindowHeight = centeredStructureBounds.bottom - centeredStructureBounds.top;
						
						
						centeredStructureBounds.top = kAbsoluteMinimumV;
						centeredStructureBounds.bottom = centeredStructureBounds.top + kWindowHeight;
					}
					error = TransitionWindow(carbonWindow, kWindowSlideTransitionEffect, kWindowMoveTransitionAction,
												&centeredStructureBounds);
					if (noErr != error)
					{
						Console_Warning(Console_WriteValue, "failed to transition window for termination warning, error", error);
					}
				}
				else
				{
					NSScreen*	windowScreen = ((nil != window.screen)
												? window.screen
												: [NSScreen mainScreen]);
					CGRect		availableCGRect = NSRectToCGRect(windowScreen.frame);
					
					
					{
						CGRect		centeredCGRect = NSRectToCGRect(window.frame);
						
						
						RegionUtilities_CenterHIRectIn(centeredCGRect, availableCGRect);
						centeredFrame = NSRectFromCGRect(centeredCGRect);
						centeredFrame.origin.y -= kOffsetFromCenterV;
					}
					if ((centeredFrame.origin.y + NSHeight(centeredFrame)) > (availableCGRect.origin.y + availableCGRect.size.height - kAbsoluteMinimumV))
					{
						centeredFrame.origin.y = (availableCGRect.origin.y + availableCGRect.size.height - kAbsoluteMinimumV - NSHeight(centeredFrame));
					}
					[window setFrame:centeredFrame display:YES animate:YES];
				}
			}
		}
		
		// access the session as needed, but then unlock it so that
		// it can be killed if necessary
		{
			My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
			GenericDialog_Ref		existingSheet = ptr->currentDialog.returnTargetRef();
			
			
			terminalWindow = ptr->terminalWindow;
			
			if (nullptr != existingSheet)
			{
				GenericDialog_SetItemDialogEffect(existingSheet, kGenericDialog_ItemIDButton2, kGenericDialog_DialogEffectCloseImmediately);
				GenericDialog_Remove(existingSheet);
			}
			
			// keep track of this sheet (weak reference) so that it can be
			// removed when a global termination warning appears
			ptr->currentDialog.assign(Alert_ReturnGenericDialog(box.returnRef()));
		}
		
		// basic alert box setup
		Alert_SetParamsFor(box.returnRef(), kAlert_StyleOKCancel);
		if (kWillRemoveTerminalWindow)
		{
			Alert_DisableCloseAnimation(box.returnRef());
		}
		
		// set message
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			primaryTextCFString = nullptr;
			
			
			stringResult = UIStrings_Copy((kIsRestartCommand)
											? kUIStrings_AlertWindowRestartSessionPrimaryText
											: (kIsKillCommand)
												? kUIStrings_AlertWindowKillSessionPrimaryText
												: kUIStrings_AlertWindowClosePrimaryText, primaryTextCFString);
			if (stringResult.ok())
			{
				CFStringRef		helpTextCFString = nullptr;
				
				
				stringResult = UIStrings_Copy((kIsRestartCommand)
												? kUIStrings_AlertWindowRestartSessionHelpText
												: (kIsKillCommand)
													? kUIStrings_AlertWindowKillSessionHelpText
													: kUIStrings_AlertWindowCloseHelpText, helpTextCFString);
				if (stringResult.ok())
				{
					Alert_SetTextCFStrings(box.returnRef(), primaryTextCFString, helpTextCFString);
					CFRelease(helpTextCFString);
				}
				CFRelease(primaryTextCFString);
			}
		}
		// set title
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			titleCFString = nullptr;
			
			
			stringResult = UIStrings_Copy((kIsRestartCommand)
											? kUIStrings_AlertWindowRestartSessionName
											: (kIsKillCommand)
												? kUIStrings_AlertWindowKillSessionName
												: kUIStrings_AlertWindowCloseName, titleCFString);
			if (stringResult == kUIStrings_ResultOK)
			{
				Alert_SetTitleCFString(box.returnRef(), titleCFString);
				CFRelease(titleCFString);
			}
		}
		// set buttons
		{
			UIStrings_Result	stringResult = kUIStrings_ResultOK;
			CFStringRef			buttonTitleCFString = nullptr;
			
			
			stringResult = UIStrings_Copy((kIsRestartCommand)
											? kUIStrings_ButtonRestart
											: (kIsKillCommand)
												? kUIStrings_ButtonKill
												: kUIStrings_ButtonClose, buttonTitleCFString);
			if (stringResult == kUIStrings_ResultOK)
			{
				Alert_SetButtonText(box.returnRef(), kAlert_ItemButton1, buttonTitleCFString);
				CFRelease(buttonTitleCFString);
			}
			Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemButton1,
											^{
												// a variable is used because it is nullified in the event
												// that the session is destroyed by the user’s actions
												SessionRef		session = inRef;
												
												
												terminationWarningClose(session, kKeepWindow, kRestart);
												if (nullptr == session)
												{
													// user destroyed the session and its window
												}
											});
			Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemButton2,
											^{
												// clear the alert status attribute for the session
												{
													My_SessionAutoLocker	localPtr(gSessionPtrLocks(), inRef);
													
													
													changeStateAttributes(localPtr, 0/* attributes to set */,
																			kSession_StateAttributeOpenDialog/* attributes to clear */);
												}
												
												// perform any action from the caller (e.g. this is
												// used at quitting time to interrupt the remaining
												// sequence of window-closings)
												inCancelAction();
												
												// in the modal case, this action must also be seen
												// locally in order to reverse animation effects
												wasCancelled = true;
											});
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
		if (kForceModal)
		{
			Alert_Display(box.returnRef(), (false == kNoAlertAnimation)); // retains alert until it is dismissed
			
			// NOTE: "wasCancelled" can be set by the action block that
			// is associated with an alert button, earlier
			if ((kWillRemoveTerminalWindow) && (wasCancelled))
			{
				// in the event that the user cancelled and the window
				// was transitioned to the screen center, “un-transition”
				// the most recent window back to its original location -
				// unless of course the user has since moved the window
				if (nullptr != carbonWindow)
				{
					Rect	currentStructureBounds;
					
					
					UNUSED_RETURN(OSStatus)GetWindowBounds(carbonWindow, kWindowStructureRgn, &currentStructureBounds);
					if (RegionUtilities_EqualRects(&currentStructureBounds, &centeredStructureBounds))
					{
						HIRect						floatBounds = CGRectMake(originalStructureBounds.left, originalStructureBounds.top,
																				originalStructureBounds.right - originalStructureBounds.left,
																				originalStructureBounds.bottom - originalStructureBounds.top);
						TransitionWindowOptions		transitionOptions;
						
						
						bzero(&transitionOptions, sizeof(transitionOptions));
						transitionOptions.version = 0;
						UNUSED_RETURN(OSStatus)TransitionWindowWithOptions(carbonWindow, kWindowSlideTransitionEffect, kWindowMoveTransitionAction,
																			&floatBounds, true/* asynchronous */, &transitionOptions);
					}
				}
				else
				{
					Console_Warning(Console_WriteLine, "window back-transition not implemented for Cocoa");
				}
			}
		}
		else
		{
			Alert_Display(box.returnRef()); // retains alert until it is dismissed
		}
	}
}// DisplaySessionTerminationWarning


/*!
Displays a user interface for editing the window title.
The interface is automatically hidden or destroyed at
appropriate times.

(4.0)
*/
void
Session_DisplayWindowRenameUI	(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	if (nullptr == ptr->renameDialog)
	{
		// create the rename interface, specify how to initialize it
		// and specify how to update the title when finished
		auto			initBlock = ^()
									{
										// initialize with existing title
										CFStringRef		result = nullptr; // note: need to return a copy
										
										
										if (kSession_ResultOK != Session_GetWindowUserDefinedTitle(inRef, result))
										{
											// failed; return copy of empty string
											result = BRIDGE_CAST([@"" retain], CFStringRef);
										}
										
										return result; // return-from-block
									};
		auto			finalBlock = ^(CFStringRef	inNewTitle)
										{
											// if non-nullptr, set new title
											if (nullptr == inNewTitle)
											{
												// user cancelled; ignore
											}
											else
											{
												Session_SetWindowUserDefinedTitle(inRef, inNewTitle);
											}
										};
		HIWindowRef		carbonWindow = Session_ReturnActiveLegacyCarbonWindow(inRef);
		Boolean			noAnimations = false;
		
		
		// determine if animation should occur
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNoAnimations,
									sizeof(noAnimations), &noAnimations))
		{
			noAnimations = false; // assume a value, if preference can’t be found
		}
		
		if (nullptr != carbonWindow)
		{
			ptr->renameDialog = WindowTitleDialog_NewWindowModalParentCarbon
								(carbonWindow, (false == noAnimations), initBlock, finalBlock);
		}
		else
		{
			ptr->renameDialog = WindowTitleDialog_NewWindowModal
								(Session_ReturnActiveNSWindow(inRef), (false == noAnimations),
									initBlock, finalBlock);
		}
	}
	WindowTitleDialog_Display(ptr->renameDialog);
}// DisplayWindowRenameUI


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
			Preferences_ContextRef	currentMacros = MacroManager_ReturnCurrentMacros();
			TerminalWindowRef		terminalWindow = Session_ReturnActiveTerminalWindow(inRef);
			TerminalViewRef			view = TerminalWindow_ReturnViewWithFocus(terminalWindow);
			
			
			// write all information into memory
			{
				CFStringRef			stringValue = nullptr;
				CGDeviceColor		colorValue;
				Session_Result		sessionResult = kSession_ResultOK;
				
				
				// window title
				sessionResult = Session_GetWindowUserDefinedTitle(inRef, stringValue);
				if (sessionResult.ok())
				{
					saveError = SessionDescription_SetStringData
								(saveFileMemoryModel, kSessionDescription_StringTypeWindowName, stringValue);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save window title, error", saveError);
					}
					CFRelease(stringValue), stringValue = nullptr;
				}
				
				// command info
				stringValue = CFStringCreateByCombiningStrings(kCFAllocatorDefault,
																ptr->commandLineArguments.returnCFArrayRef(), CFSTR(" "));
				if (nullptr != stringValue)
				{
					saveError = SessionDescription_SetStringData
									(saveFileMemoryModel, kSessionDescription_StringTypeCommandLine, stringValue);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save command line, error", saveError);
					}
					CFRelease(stringValue), stringValue = nullptr;
				}
				
				// macro info
				if (nullptr != currentMacros)
				{
					Preferences_Result		prefsResult = Preferences_ContextGetName(currentMacros, stringValue);
					
					
					// note: string is not allocated so it must not be released
					if ((kPreferences_ResultOK == prefsResult) && (nullptr != stringValue))
					{
						saveError = SessionDescription_SetStringData
										(saveFileMemoryModel, kSessionDescription_StringTypeMacroSet, stringValue);
						if (kSessionDescription_ResultOK != saveError)
						{
							Console_Warning(Console_WriteValue, "failed to save macro set name, error", saveError);
						}
					}
				}
				
				// font info
				{
					UInt16		fontSize = 0;
					
					
					TerminalView_GetFontAndSize(view, &stringValue, &fontSize);
					if (stringValue != nullptr)
					{
						saveError = SessionDescription_SetStringData
									(saveFileMemoryModel, kSessionDescription_StringTypeTerminalFont, stringValue);
						if (kSessionDescription_ResultOK != saveError)
						{
							Console_Warning(Console_WriteValue, "failed to save font name, error", saveError);
						}
					}
					saveError = SessionDescription_SetIntegerData
								(saveFileMemoryModel, kSessionDescription_IntegerTypeTerminalFontSize, fontSize);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save font size, error", saveError);
					}
				}
				
				// color info
				{
					TerminalView_GetColor(view, kTerminalView_ColorIndexNormalText, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeTextNormal, colorValue);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save normal text color, error", saveError);
					}
					
					TerminalView_GetColor(view, kTerminalView_ColorIndexNormalBackground, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeBackgroundNormal, colorValue);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save normal background color, error", saveError);
					}
					
					TerminalView_GetColor(view, kTerminalView_ColorIndexBoldText, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeTextBold, colorValue);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save bold text color, error", saveError);
					}
					
					TerminalView_GetColor(view, kTerminalView_ColorIndexBoldBackground, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeBackgroundBold, colorValue);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save bold background color, error", saveError);
					}
					
					TerminalView_GetColor(view, kTerminalView_ColorIndexBlinkingText, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeTextBlinking, colorValue);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save blinking text color, error", saveError);
					}
					
					TerminalView_GetColor(view, kTerminalView_ColorIndexBlinkingBackground, &colorValue);
					saveError = SessionDescription_SetRGBColorData
								(saveFileMemoryModel, kSessionDescription_RGBColorTypeBackgroundBlinking, colorValue);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save blinking background color, error", saveError);
					}
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
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save column count, error", saveError);
					}
					saveError = SessionDescription_SetIntegerData
								(saveFileMemoryModel, kSessionDescription_IntegerTypeTerminalVisibleLineCount, rows);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save row count, error", saveError);
					}
					saveError = SessionDescription_SetIntegerData
								(saveFileMemoryModel, kSessionDescription_IntegerTypeScrollbackBufferLineCount, scrollback);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save scrollback row count, error", saveError);
					}
				}
				
				// terminal type
				{
					Session_TerminalCopyAnswerBackMessage(inRef, stringValue);
					saveError = SessionDescription_SetStringData
								(saveFileMemoryModel, kSessionDescription_StringTypeAnswerBack, stringValue);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save terminal type, error", saveError);
					}
					if (nullptr != stringValue)
					{
						CFRelease(stringValue), stringValue = nullptr;
					}
				}
				
				// keyboard mapping info
				{
					Boolean		flag = false;
					
					
					flag = (kSession_NewlineModeMapCRNull == ptr->eventKeys.newline);
					saveError = SessionDescription_SetBooleanData
								(saveFileMemoryModel, kSessionDescription_BooleanTypeRemapCR, flag);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save new-line mapping, error", saveError);
					}
					flag = !ptr->eventKeys.pageKeysLocalControl;
					saveError = SessionDescription_SetBooleanData
								(saveFileMemoryModel, kSessionDescription_BooleanTypePageKeysDoNotControlTerminal, flag);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save page key setting, error", saveError);
					}
					flag = false; // TEMPORARY
					saveError = SessionDescription_SetBooleanData
								(saveFileMemoryModel, kSessionDescription_BooleanTypeRemapKeypadTopRow, flag);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save keypad top row setting, error", saveError);
					}
				}
				
				// TEK info
				{
					Boolean const	kFlag = (false == Session_TEKPageCommandOpensNewWindow(inRef));
					
					
					saveError = SessionDescription_SetBooleanData
								(saveFileMemoryModel, kSessionDescription_BooleanTypeTEKPageClears, kFlag);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save TEK page clear setting, error", saveError);
					}
				}
				
				// Kerberos info: no longer supported
				
				// toolbar info
				{
					// TEMPORARY; should respect all possible toolbar view options (see "SessionDescription.h")
					//stringValue = TerminalWindow_ToolbarIsVisible(ptr->terminalWindow) ? CFSTR("icon+text") : CFSTR("hidden");
					stringValue = CFSTR("icon+text");
					saveError = SessionDescription_SetStringData
								(saveFileMemoryModel, kSessionDescription_StringTypeToolbarInfo, stringValue);
					if (kSessionDescription_ResultOK != saveError)
					{
						Console_Warning(Console_WriteValue, "failed to save toolbar state, error", saveError);
					}
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
	size_t					remainingBytesCount = 0;
	
	
	TerminalView_SetDrawingEnabled(TerminalWindow_ReturnViewWithFocus(Session_ReturnActiveTerminalWindow(inRef)),
									false); // no output
	remainingBytesCount = 1; // just needs to be positive to start with
	while (remainingBytesCount > 0)
	{
		remainingBytesCount = processMoreData(ptr);
	}
	TerminalView_SetDrawingEnabled(TerminalWindow_ReturnViewWithFocus(Session_ReturnActiveTerminalWindow(inRef)),
									true); // output now
}// FlushNetwork


/*!
Returns the name of an image file in the bundle (suitable
for use with APIs such as NSImage’s "iconNamed:"), to
represent the current state of the session.  Useful for
user interface elements.

(4.0)
*/
Session_Result
Session_GetStateIconName	(SessionRef		inRef,
							 CFStringRef&	outUncopiedString)
{
	Session_Result		result = kSession_ResultOK;
	
	
	if (nullptr == inRef) result = kSession_ResultInvalidReference;
	else
	{
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		outUncopiedString = ptr->statusIconName.returnCFStringRef();
	}
	return result;
}// GetStateString


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
	
	
	if (nullptr == inRef) result = kSession_ResultInvalidReference;
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
Returns true only if the terminal device associated with the
main process of the session has currently disabled echoing
(typically done by programs that are displaying a password
prompt).

(4.1)
*/
Boolean
Session_IsInPasswordMode		(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					result = false;
	
	
	if ((nullptr != ptr) && (nullptr != ptr->mainProcess))
	{
		result = Local_ProcessIsInPasswordMode(ptr->mainProcess);
	}
	return result;
}// IsInPasswordMode


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
	Boolean		result = ((nullptr != inRef) && (gInvalidSessions().find(inRef) == gInvalidSessions().end()));
	
	
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
			// this is the typical case; send data to a sophisticated terminal emulator
			std::for_each(ptr->targetTerminals.begin(), ptr->targetTerminals.end(), terminalDataWriter(kBuffer, inByteCount));
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
													REINTERPRET_CAST(inTargetData, VectorInterpreter_Ref)),
										ptr->targetVectorGraphics.end());
		break;
	
	default:
		// ???
		result = kSession_ResultParameterError;
		break;
	}
	return result;
}// RemoveDataTarget


/*!
Returns the most recently active window associated
with the specified session, or nil if it cannot be
found.

This follows the same logic, and therefore has the
same caveats, as Session_ReturnActiveTerminalWindow().
See documentation on that function for more.

(4.1)
*/
NSWindow*
Session_ReturnActiveNSWindow	(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	NSWindow*				result = returnActiveNSWindow(ptr);
	
	
	return result;
}// ReturnActiveNSWindow


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
be found.  Also returns nullptr if the session was
constructed using a modern Cocoa window.

DEPRECATED; use Session_ReturnActiveNSWindow().

(3.0)
*/
HIWindowRef
Session_ReturnActiveLegacyCarbonWindow		(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	HIWindowRef				result = returnActiveLegacyCarbonWindow(ptr);
	
	
	return result;
}// ReturnActiveLegacyCarbonWindow


/*!
Returns the POSIX path of the directory that was current as of
the most recent call to Local_UpdateCurrentDirectoryCache().
If this is empty, it means that the information may not be
available (due to permission issues, for example, or because
Local_UpdateCurrentDirectoryCache() was never called).

IMPORTANT:	Local_UpdateCurrentDirectoryCache() does expensive
			scans that (in exchange for being slow) update the
			results for ALL open Sessions.  So be sure to gather
			this information for as many relevant Sessions as
			possible, and avoid unnecessary cache updates.

See also Session_ReturnOriginalWorkingDirectory().

(4.0)
*/
CFStringRef
Session_ReturnCachedWorkingDirectory	(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFStringRef				result = CFSTR("");
	
	
	if (nullptr != ptr->mainProcess)
	{
		// this might return an empty string on failure
		result = Local_ProcessReturnCurrentDirectory(ptr->mainProcess);
	}
	
	return result;
}// ReturnCachedWorkingDirectory


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
	Preferences_ContextRef	result = ptr->configuration.returnRef();
	
	
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
		prefsResult = Preferences_ContextSetData(result, kPreferences_TagNewLineMapping,
													sizeof(ptr->eventKeys.newline), &ptr->eventKeys.newline);
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

See also Session_ReturnCachedWorkingDirectory(), which can
be used to find more recent values.

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
Returns translation-related preferences (such as text encoding)
that are shared by the views of the session as well as the
session itself.

You can make changes to this context ONLY if you do it in “batch
mode” with Preferences_ContextCopy().  In other words, even to
make a single change, you must first add the change to a new
temporary context, then use Preferences_ContextCopy() to read
the temporary settings into the context returned by this routine.
Batch mode changes are detected by the Session and used to
automatically update internal data structures and related things
like the underlying process’ pseudo-terminal.

Note that you cannot expect all possible tags to be present;
be prepared to not find what you look for.  In addition, tags
that are present in one session may be absent in another.

(4.0)
*/
Preferences_ContextRef
Session_ReturnTranslationConfiguration		(SessionRef		inRef)
{
	My_SessionAutoLocker		ptr(gSessionPtrLocks(), inRef);
	Preferences_ContextRef		result = ptr->translationConfiguration.returnRef();
	
	
	// since many settings are represented elsewhere, this context
	// will not contain the latest information; update the context
	// based on current settings
	if (false == ptr->targetTerminals.empty())
	{
		TerminalScreenRef	screen = ptr->targetTerminals.front();
		Boolean				copyOK = (nullptr != screen)
										? TextTranslation_ContextSetEncoding(result, Terminal_ReturnTextEncoding(screen))
										: false;
		
		
		if (false == copyOK)
		{
			Console_Warning(Console_WriteLine, "failed to read text encoding of a terminal screen in the session");
		}
	}
	
	// INCOMPLETE - only care about text encoding for now
	
	return result;
}// ReturnTranslationConfiguration


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
Adds the specified data to a buffer, which will be sent to the
local or remote process for the given session when the receiver
is ready.

Returns the number of bytes actually written; if this number is
less than "inByteCount", offset the buffer by the difference
and try again to send the rest.

See also Session_SendDataCFString().

WARNING:	This is a “raw” write function that does not give
			you a chance to specify the text encoding of your
			source data stream.  If you cannot be sure that your
			data is compatible with the encoding specified by
			the last call to Session_SetSendDataEncoding(), then
			you should probably use the CFString variant of this
			routine.

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
		result = STATIC_CAST(Local_TerminalWriteBytes(Local_ProcessReturnMasterTerminal(ptr->mainProcess),
														inBufferPtr, inByteCount),
								SInt16);
	}
	return result;
}// SendData


/*!
Adds the specified data to a buffer, which will be sent to
the local or remote process for the given session when the
pseudo-terminal device connected to the underlying process
is ready.

Since this version of the function has encoding information
available (in the CFString), it converts the data into the
target encoding and sends the new format as a raw stream of
bytes.  The assumption is that the application running in
the terminal will know how to decode the data in the format
that the user has selected for the session.  If the string
appears to be in the target encoding already, this might be
done efficiently; note however that radically different text
encodings will incur significant translation costs.

This function now guarantees that the transmitted bytes fall
on character boundaries; so, although it may return before
all characters have been transmitted, it will ensure that
all of the bytes that comprise the last written character
have been sent.

Returns the number of CHARACTERS actually written, or a
negative number on error.  If this result is any less than
"CFStringGetLength(inString)", then the string was written
incompletely and you should attempt to write the remaining
characters in a new call.

(4.0)
*/
CFIndex
Session_SendDataCFString	(SessionRef		inRef,
							 CFStringRef	inString,
							 CFIndex		inFirstCharacter)
{
	CFIndex const			kLength = CFStringGetLength(inString);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFRange					targetRange = CFRangeMake(0, 1/* count */);
	UInt8					byteArray[1024]; // arbitrary size
	UInt8*					currentPtr = byteArray;
	size_t					sizeRemaining = sizeof(byteArray);
	CFIndex					result = 0;
	
	
	// convert characters one at a time, so that it is possible to ensure
	// that all the bytes for a character have been sent (even if the
	// whole string cannot be sent successfully)
	for (targetRange.location = inFirstCharacter; targetRange.location < kLength; )
	{
		CFIndex		bytesForChar = 0;
		CFIndex		numberOfCharactersConverted = 0;
		
		
		// initially look for a single character
		targetRange.length = 1;
		
		// IMPORTANT: Although CFStringGetBytes() ought to accept a zero-length buffer,
		// it appears that actually giving that function a length of zero makes it
		// return a nonzero number of “bytes used”.  Not only would that violate the
		// assertion that follows, but it calls into question just what the heck might
		// have happened to the buffer and its surrounding memory!  A preemptive check
		// is done to avoid this problem.
		if (sizeRemaining > 0)
		{
			numberOfCharactersConverted = CFStringGetBytes(inString, targetRange, ptr->writeEncoding,
															0/* loss byte, or 0 for no lossy conversion */,
															false/* is external representation */,
															currentPtr, sizeRemaining, &bytesForChar);
			if (0 == numberOfCharactersConverted)
			{
				// not all values can be represented as a single character;
				// if a single-character conversion is not possible, try to
				// request more than one character at a time
				targetRange.length = 2;
				numberOfCharactersConverted = CFStringGetBytes(inString, targetRange, ptr->writeEncoding,
																0/* loss byte, or 0 for no lossy conversion */,
																false/* is external representation */,
																currentPtr, sizeRemaining, &bytesForChar);
			}
		}
		
		if (numberOfCharactersConverted > 0)
		{
			assert(sizeRemaining >= STATIC_CAST(bytesForChar, size_t));
			sizeRemaining -= bytesForChar;
			currentPtr += bytesForChar;
			result += targetRange.length;
			targetRange.location += targetRange.length;
		}
		
		if ((0 == numberOfCharactersConverted) ||
			(sizeRemaining < 4/* arbitrary */) ||
			(targetRange.location >= kLength))
		{
			// send what has been accumulated, and then reset the pointer
			size_t		bytesLeft = sizeof(byteArray) - sizeRemaining;
			size_t		bytesWritten = 0;
			
			
			currentPtr = byteArray;
			while (bytesLeft > 0)
			{
				bytesWritten = Session_SendData(inRef, currentPtr, bytesLeft);
				if (bytesWritten > 0)
				{
					assert(bytesLeft >= bytesWritten);
					bytesLeft -= bytesWritten;
					currentPtr += bytesWritten;
				}
			}
			currentPtr = byteArray;
			sizeRemaining = sizeof(byteArray);
		}
		
		// if the most recent conversion attempt failed, it will
		// only fail again if the loop is allowed to continue
		if (0 == numberOfCharactersConverted)
		{
			break;
		}
	}
	return result;
}// SendDataCFString


/*!
Sends a request to delete one composed character sequence.
Depending upon the user preference, this may send a
backspace or a delete character.

(2016.11)
*/
void
Session_SendDeleteBackward	(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	char					deleteChar[] = { 0x7F/* delete */ };
	
	
	if (ptr->eventKeys.deleteSendsBackspace)
	{
		deleteChar[0] = 0x08; // backspace
	}
	Session_SendData(inRef, deleteChar, sizeof(deleteChar));
}// SendDeleteBackward


/*!
Sends any data waiting to be sent (i.e. in the buffer) to the
local or remote process for the given session immediately,
clearing out the buffer if possible.  Returns the number of
bytes left in the buffer.

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
Sends a carriage return sequence.  Depending upon the
user preference and terminal mode for carriage return
handling, this may send CR, CR-LF, CR-null or LF.

The value "kSession_EchoCurrentSessionValue" is preferable
for "inEcho" so that you are always respecting the user’s
preferences.

(3.0)
*/
void
Session_SendNewline		(SessionRef		inRef,
						 Session_Echo	inEcho)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Boolean					echo = false;
	
	
	// send any local echo first
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
	
	// now send the new-line sequence to the session
	switch (ptr->eventKeys.newline)
	{
	case kSession_NewlineModeMapCR:
		if (echo)
		{
			localEchoKey(ptr, 0x0D/* carriage return character */);
		}
		Session_SendData(inRef, "\015", 1);
		break;
	
	case kSession_NewlineModeMapCRLF:
		if (echo)
		{
			localEchoKey(ptr, 0x0D/* carriage return character */);
		}
		Session_SendData(inRef, "\015\012", 2);
		break;
	
	case kSession_NewlineModeMapCRNull:
		if (echo)
		{
			localEchoKey(ptr, 0x0D/* carriage return character */);
		}
		Session_SendData(inRef, "\015\000", 2);
		break;
	
	case kSession_NewlineModeMapLF:
	default:
		if (echo)
		{
			localEchoKey(ptr, 0x0D/* carriage return character */);
		}
		Session_SendData(inRef, "\012", 1);
		break;
	}
	UNUSED_RETURN(SInt16)Session_SendFlush(inRef);
}// SendNewline


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
			UNUSED_RETURN(OSStatus)HMDisplayTag(tagData.ptr());
			// this call does not immediately hide the tag, but rather after a short delay
			UNUSED_RETURN(OSStatus)HMHideTagWithOptions(kHMHideTagFade);
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
		UNUSED_RETURN(OSStatus)HMHideTag();
		
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
		
		
		ptr->deviceNameString.setWithNoRetain(CFStringCreateWithCString
												(kCFAllocatorDefault, kDeviceName, kCFStringEncodingASCII));
	}
	ptr->commandLineArguments.setWithRetain(Local_ProcessReturnCommandLine(ptr->mainProcess));
	ptr->originalDirectoryString.setWithRetain(Local_ProcessReturnOriginalDirectory(ptr->mainProcess));
	ptr->resourceLocationString.setWithNoRetain(CFStringCreateByCombiningStrings
												(kCFAllocatorDefault, ptr->commandLineArguments.returnCFArrayRef(), CFSTR(" ")));
	
	changeNotifyForSession(ptr, kSession_ChangeResourceLocation, inRef/* context */);
}// SetProcess


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
	
	
	for (auto screenRef : ptr->targetTerminals)
	{
		Terminal_SetSpeechEnabled(screenRef, inIsEnabled);
	}
}// SetSpeechEnabled


/*!
Changes the current state of the specified session, which will
trigger listener notifications for all modules interested in
finding out about new states (such as sessions that die, etc.).

The state may not be set to "kSession_StateImminentDisposal".
But if the state is set to "kSession_StateDead", it is possible
that Session_Dispose() will be immediately called to destroy
the session (and the first side effect of that is to enter the
imminent-disposal state).  It is not necessarily destroyed
however; a dead session remains valid if the user preference is
set to keep the terminal window open, or if the Session module
is currently in the process of restarting the session.  So
setting the dead state is a valid way to kill a session’s
underlying process, as long as you can handle the possibility
that the whole session object will die at the same time.

IMPORTANT:	Currently, this routine does not check that the
			specified state is a valid “next state” for the
			current state.  This may have bizarre consequences;
			for example, transitioning a session from a dead
			state back to an active one.  In the future, this
			routine will try to implicitly transition a session
			through a series of valid states to end up at the
			requested state, telling all interested listeners
			about each intermediate state.

(3.0)
*/
void
Session_SetState	(SessionRef			inRef,
					 Session_State		inNewState)
{
	Boolean		keepWindowOpen = false;
	Boolean		isDifferentState = false;
	
	
	// if the user preference is set, kill the terminal window
	// as soon as sessions finish (but see below for ways in
	// which this flag may be overridden)
	if (kSession_StateDead == inNewState)
	{
		// get the user’s process service preference, if possible
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagDontAutoClose,
									sizeof(keepWindowOpen), &keepWindowOpen))
		{
			keepWindowOpen = false; // assume window should be closed, if preference can’t be found
		}
	}
	
	// locally lock the session; this is done in an inner block
	// so that the session can be destroyed at the end if necessary
	{
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		// do not waste time if the state is already up to date
		isDifferentState = (inNewState != ptr->status);
		if (isDifferentState)
		{
			CFStringRef		statusString = nullptr;
			
			
			// update status text to reflect new state
			assert(kSession_StateImminentDisposal != inNewState); // only the destructor may set this
			ptr->status = inNewState;
			
			// once connected, set the connection time
			if (kSession_StateActiveUnstable == ptr->status)
			{
				ptr->activationAbsoluteTime = CFAbsoluteTimeGetCurrent();
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
										ptr->statusString.setWithRetain(statusWithTime);
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
						ptr->statusString.setWithRetain(statusString);
					}
				}
			}
			else if (kSession_StateActiveUnstable == ptr->status)
			{
				(UIStrings_Result)UIStrings_Copy(kUIStrings_SessionInfoWindowStatusProcessNewborn,
													statusString);
				ptr->statusString.setWithRetain(statusString);
			}
			else if (kSession_StateActiveStable == ptr->status)
			{
				(UIStrings_Result)UIStrings_Copy(kUIStrings_SessionInfoWindowStatusProcessRunning,
													statusString);
				ptr->statusString.setWithRetain(statusString);
			}
			else
			{
				// ???
				ptr->statusString.setWithRetain(CFSTR(""));
			}
			
			setIconFromState(ptr);
			
			if (nullptr != ptr->mainProcess)
			{
				if (kSession_StateDead == ptr->status)
				{
					// killing the process may trigger a state change, but this
					// is OK as long as the state it chooses is the same as
					// the state that triggers this call
					Local_KillProcess(&ptr->mainProcess);
				}
			}
			
			// now that the state of the Session object is appropriate
			// for the state, notify any other listeners of the change
			changeNotifyForSession(ptr, kSession_ChangeState, inRef/* context */);
		}
	}
	
	// this must be done outside the block above because the
	// session cannot be destroyed while it is locked
	if (isDifferentState)
	{
		if (kSession_StateDead == inNewState)
		{
			if (false == keepWindowOpen)
			{
				// a session disposal is conditional and will have no effect if
				// the session appears to be in use (e.g. restart in progress)
				Session_Dispose(&inRef);
			}
		}
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
	
	
	ptr->alternateTitle.setWithNoRetain(CFStringCreateCopy(kCFAllocatorDefault, inWindowName));
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
	
	
	for (auto screenRef : ptr->targetTerminals)
	{
		if (Terminal_SpeechIsEnabled(screenRef)) result = true;
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
	
	
	for (auto screenRef : ptr->targetTerminals)
	{
		Terminal_SpeechPause(screenRef);
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
	
	
	for (auto screenRef : ptr->targetTerminals)
	{
		Terminal_SpeechResume(screenRef);
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
		if (noErr != error)
		{
			Console_Warning(Console_WriteValueFourChars, "failed to start monitoring for change", inForWhatChange);
			Console_Warning(Console_WriteValue, "monitor installation error", error);
		}
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
Sends a PAGE command to the current vector interpreter,
if any.  This will either clear the display or open a
new graphics window.

(4.0)
*/
void
Session_TEKNewPage	(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	
	
	if (nullptr == ptr->vectorGraphicsInterpreter)
	{
		UNUSED_RETURN(Boolean)vectorGraphicsCreateTarget(ptr);
	}
	else
	{
		if (Session_TEKPageCommandOpensNewWindow(inRef))
		{
			// create a new display (automatically detaches any existing one)
			UNUSED_RETURN(Boolean)vectorGraphicsCreateTarget(ptr);
		}
		else
		{
			// clear screen of existing display
			VectorInterpreter_PageCommand(ptr->vectorGraphicsInterpreter);
		}
	}
}// TEKNewPage


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
	if (nullptr != ptr->vectorGraphicsInterpreter)
	{
		VectorInterpreter_SetPageClears(ptr->vectorGraphicsInterpreter, false == inNewWindow);
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
	
	
	for (auto screenRef : ptr->targetTerminals)
	{
		Terminal_EmulatorProcessCString(screenRef, inCString);
	}
}// TerminalWriteCString


/*!
Returns the time when the specified session’s command was
run or restarted.

Use CFDateFormatterCreateStringWithAbsoluteTime() or a
similar function to show this value in a user interface
element.

(4.0)
*/
CFAbsoluteTime
Session_TimeOfActivation	(SessionRef		inRef)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	CFAbsoluteTime			result = 0L;
	
	
	result = ptr->activationAbsoluteTime;
	return result;
}// TimeOfActivation


/*!
Returns the time when the specified session’s command exited.

Use CFDateFormatterCreateStringWithAbsoluteTime() or a
similar function to show this value in a user interface
element.

(4.0)
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
Send a string to a session as if it were typed into the given
session’s window.  Any previous pending output is first flushed
to the session.  If local echoing is enabled for the session,
the string will be written to the local data target (usually a
terminal) before it is sent to the underlying process.

This function will block until every character in the string
has been sent to the session.  To have more direct control over
the data transmission rate, see Session_SendData().

(3.0)
*/
void
Session_UserInputCFString	(SessionRef		inRef,
							 CFStringRef	inStringBuffer)
{
	CFIndex const	kLength = CFStringGetLength(inStringBuffer);
	CFIndex			offset = 0;
	SInt16			loopGuard = 0;
	
	
	// dump to the local terminal first, if this mode is turned on
	if (Session_LocalEchoIsEnabled(inRef))
	{
		My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
		
		
		localEchoString(ptr, inStringBuffer);
	}
	
	// first send any outstanding data in the buffer
	Session_SendFlush(inRef);
	
	// block until every character in the string has been written
	while (offset < kLength)
	{
		CFIndex		charactersSent = Session_SendDataCFString(inRef, inStringBuffer, offset);
		
		
		offset += charactersSent;
		if (++loopGuard > 4/* arbitrary */)
		{
			Console_Warning(Console_WriteValueCFString, "aborting transmission of string after too many failed attempts", inStringBuffer);
			break;
		}
	}
}// UserInputCFString


/*!
Send input to the session to interrupt whatever
process is running.  For local sessions, this means
to send control-C (or whatever the interrupt control
key is).

(3.1)
*/
void
Session_UserInputInterruptProcess	(SessionRef		inRef)
{
	// clear the Suspend state from MacTerm’s point of view,
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
		UNUSED_RETURN(OSStatus)HMDisplayTag(tagData.ptr());
		// this call does not immediately hide the tag, but rather after a short delay
		UNUSED_RETURN(OSStatus)HMHideTagWithOptions(kHMHideTagFade);
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
Sends the appropriate multi-character sequence to the terminal
for the session, based on the given function key number and
function key layout.  Note that some layouts do not use all
keys, and requests for unused keys will have no effect.

Traditionally, “function keys” tended to mean the VT220 layout
because it was the most elaborate at the time.  But some popular
other layouts have emerged that are useful to support, so this
function can send sequences from multiple layouts.

\retval kSession_ResultOK
if the key was apparently input successfully

\retval kSession_ResultInvalidReference
if "inRef" is invalid

\retval kSession_ResultParameterError
if the key index is not in the range [1, 48]

\retval kSession_ResultNotReady
if the key cannot be handled right now (e.g. sheet is open)

(4.0)
*/
Session_Result
Session_UserInputFunctionKey	(SessionRef					inRef,
								 UInt8						inFunctionKeyNumber,
								 Session_FunctionKeyLayout	inKeyboardLayout)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_Result			result = kSession_ResultOK;
	
	
	if (nullptr == ptr)
	{
		result = kSession_ResultInvalidReference;
	}
	else if (kMy_SessionSheetTypeNone != ptr->sheetType)
	{
		result = kSession_ResultNotReady;
	}
	else if ((inFunctionKeyNumber < 1) || (inFunctionKeyNumber > 48))
	{
		result = kSession_ResultParameterError;
	}
	else
	{
		VTKeys_FKey		keyCode = kVTKeys_FKeyVT100PF1;
		Boolean			sendOK = true;
		
		
		switch (inFunctionKeyNumber)
		{
		case 1:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyVT100PF1;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyXTermX11F1;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 2:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyVT100PF2;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyXTermX11F2;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 3:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyVT100PF3;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyXTermX11F3;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 4:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyVT100PF4;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyXTermX11F4;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 5:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyXTermX11F5;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 6:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F6;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 7:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F7;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 8:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F8;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 9:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F9;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 10:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F10;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 11:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F11;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 12:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutXTermXFree86:
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F12;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 13:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F13;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F13;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F13;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 14:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F14;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F14;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F14;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 15:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F15;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F15;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F15;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 16:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F16;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F16;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F16;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 17:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F17;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F17;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 18:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F18;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F18;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 19:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F19;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F19;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 20:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyVT220F20;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F20;
				break;
			
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 21:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF21;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F21;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 22:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF22;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F22;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 23:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF23;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F23;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 24:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF24;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F24;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 25:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF25;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F25;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F25;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 26:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF26;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F26;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F26;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 27:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF27;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F27;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F27;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 28:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF28;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F28;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F28;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 29:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF29;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F29;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 30:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF30;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F30;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 31:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF31;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F31;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 32:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF32;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F32;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 33:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF33;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F33;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 34:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF34;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F34;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 35:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF35;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F35;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 36:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF36;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F36;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 37:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF37;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F37;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F37;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 38:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF38;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F38;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F38;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 39:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF39;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F39;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F39;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 40:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF40;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
				keyCode = kVTKeys_FKeyXTermX11F40;
				break;
			
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXFree86F40;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 41:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF41;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F41;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 42:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF42;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F42;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 43:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF43;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F43;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 44:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutRxvt:
				keyCode = kVTKeys_FKeyRxvtF44;
				break;
			
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F44;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 45:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F45;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 46:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F46;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 47:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F47;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		case 48:
			switch (inKeyboardLayout)
			{
			case kSession_FunctionKeyLayoutXTerm:
			case kSession_FunctionKeyLayoutXTermXFree86:
				keyCode = kVTKeys_FKeyXTermX11F48;
				break;
			
			case kSession_FunctionKeyLayoutVT220:
			case kSession_FunctionKeyLayoutRxvt:
			default:
				// ???
				sendOK = false;
			}
			break;
		
		default:
			// ???
			sendOK = false;
			break;
		}
		
		if (sendOK)
		{
			// allow the terminal to send the key back to the listening session
			// (ultimately this same exact session) 
			for (auto screenRef : ptr->targetTerminals)
			{
				UNUSED_RETURN(Terminal_Result)Terminal_UserInputVTFunctionKey(screenRef, keyCode);
			}
		}
	}
	return result;
}// UserInputFunctionKey


/*!
Sends the appropriate sequence for the specified key and
optional modifiers, taking into account terminal settings
(such as whether or not arrows are remapped to Emacs commands).

NOTE:	For historical reasons it is possible to transmit
		certain function key codes this way, but it is better
		to use Session_UserInputFunctionKey() for function keys.

\retval kSession_ResultOK
if the key was apparently input successfully

\retval kSession_ResultInvalidReference
if "inRef" is invalid

\retval kSession_ResultNotReady
if the key cannot be handled right now (e.g. sheet is open)

(3.1)
*/
Session_Result
Session_UserInputKey	(SessionRef		inRef,
						 UInt8			inKeyOrASCII,
						 UInt32			inEventModifiers)
{
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), inRef);
	Session_Result			result = kSession_ResultOK;
	
	
	if (nullptr == ptr)
	{
		result = kSession_ResultInvalidReference;
	}
	else if (kMy_SessionSheetTypeSpecialKeySequences == ptr->sheetType)
	{
		result = kSession_ResultNotReady;
	}
	else
	{
		if (inKeyOrASCII < VSF10)
		{
			// 7-bit ASCII; send as-is
			if (ptr->echo.enabled)
			{
				localEchoKey(ptr, inKeyOrASCII);
			}
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
				{
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
					
					if (ptr->echo.enabled)
					{
						localEchoKey(ptr, inKeyOrASCII);
					}
				}
				break;
			
			case VSRT:
				{
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
					
					if (ptr->echo.enabled)
					{
						localEchoKey(ptr, inKeyOrASCII);
					}
				}
				break;
			
			case VSUP:
				{
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
					
					if (ptr->echo.enabled)
					{
						localEchoKey(ptr, inKeyOrASCII);
					}
				}
				break;
			
			case VSDN:
				{
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
					
					if (ptr->echo.enabled)
					{
						localEchoKey(ptr, inKeyOrASCII);
					}
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
				// if enabled, write a local representation of the keystroke
				if (ptr->echo.enabled)
				{
					// when echoing a key, send only a description (not the actual key sequence)
					// TEMPORARY; this is handled here because it is currently the only place
					// that requires descriptions of keys; it might be useful to put this in an
					// API that is more generally available
					switch (inKeyOrASCII)
					{
					case VSK0:
					case VSK1:
					case VSK2:
					case VSK3:
					case VSK4:
					case VSK5:
					case VSK6:
					case VSK7:
					case VSK8:
					case VSK9:
					case VSKC:
					case VSKM:
					case VSKP:
						{
							// IMPORTANT: this should match the numeric order of the key codes
							char const*		numberStr = "0123456789,-.";
							
							
							localEchoKey(ptr, numberStr[inKeyOrASCII - VSK0]);
						}
						break;
					
					case VSLT:
					case VSRT:
					case VSUP:
					case VSDN:
						localEchoKey(ptr, inKeyOrASCII);
						break;
					
					case VSKE:
					case VSF1:
					case VSF2:
					case VSF3:
					case VSF4:
					case VSF6:
					case VSF7:
					case VSF8:
					case VSF9:
					case VSF10:
					case VSF11:
					case VSF12:
					case VSF13:
					case VSF14:
					case VSF15_220HELP:
					case VSF16_220DO:
					case VSF17:
					case VSF18:
					case VSF19:
					case VSF20:
					case VSHELP_220FIND:
					case VSHOME_220INS:
					case VSPGUP_220DEL:
					case VSDEL_220SEL:
					case VSEND_220PGUP:
					case VSPGDN_220PGDN:
						localEchoKey(ptr, inKeyOrASCII);
						break;
					
					default:
						// ???
						break;
					}
				}
				
				// allow the terminal to perform the appropriate action for the key,
				// given its current mode (for instance, a VT100 might be in VT52
				// mode, which would send different data than ANSI mode)
				for (auto screenRef : ptr->targetTerminals)
				{
					// write the appropriate sequence to the terminal’s listening session
					// (which will be this session)
					UNUSED_RETURN(Terminal_Result)Terminal_UserInputVTKey(screenRef, inKeyOrASCII);
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
	CFStringRef				pastedCFString = nullptr;
	CFStringRef				pastedDataUTI = nullptr;
	Session_Result			result = kSession_ResultOK;
	
	
	if (Clipboard_CreateCFStringFromPasteboard(pastedCFString, pastedDataUTI, kPasteboard))
	{
		CFRetainRelease		pendingLines(StringUtilities_CFNewStringsWithLines(pastedCFString),
										CFRetainRelease::kAlreadyRetained);
		Boolean				isOneLine = false;
		Boolean				noWarning = false;
		
		
		// examine the Clipboard; if the data contains new-lines, warn the user
		isOneLine = (CFArrayGetCount(pendingLines.returnCFArrayRef()) < 2);
		
		// determine if the user should be warned
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagNoPasteWarning,
									sizeof(noWarning), &noWarning))
		{
			noWarning = false; // assume a value, if preference can’t be found
		}
		
		// now, paste (perhaps displaying a warning first)
		{
			AlertMessages_BoxWrap	box;
			auto					joinResponder =
									^{
										// first join the text into one line (replace new-line sequences
										// with single spaces), then Paste
										CFRetainRelease		pastedLines(pendingLines.returnCFArrayRef(),
																		CFRetainRelease::kNotYetRetained);
										CFRetainRelease		joinedCFString(CFStringCreateByCombiningStrings(kCFAllocatorDefault,
																											pastedLines.returnCFArrayRef(),
																											CFSTR("")/* separator */),
																			CFRetainRelease::kAlreadyRetained);
										
										
										Session_UserInputCFString(inRef, joinedCFString.returnCFStringRef());
									};
			auto					normalPasteResponder =
									^{
										My_SessionAutoLocker	sessionPtr(gSessionPtrLocks(), inRef);
										CFRetainRelease			pastedLines(pendingLines.returnCFArrayRef(),
																			CFRetainRelease::kNotYetRetained);
										
										
										// regular Paste; periodically (but fairly rapidly) insert
										// each line into the session, using a timer for delays
										if (nullptr == sessionPtr->pendingPasteTimer)
										{
											// first time; create the timer
											OSStatus			error = noErr;
											Preferences_Result	prefsResult = kPreferences_ResultOK;
											EventTime			delayValue = 0;
											
											
											// determine how long the delay between lines should be
											prefsResult = Preferences_GetData(kPreferences_TagPasteNewLineDelay, sizeof(delayValue),
																				&delayValue);
											if (kPreferences_ResultOK != prefsResult)
											{
												// set an arbitrary default value
												delayValue = 50 * kEventDurationMillisecond;
											}
											
											// due to timer limitations the delay cannot be exactly zero
											if (delayValue < 0.001)
											{
												delayValue = 0.001; // arbitrary
											}
											
											sessionPtr->pendingPasteTimerUPP = NewEventLoopTimerUPP(pasteLineFromTimer);
											error = InstallEventLoopTimer(GetCurrentEventLoop(),
																			kEventDurationNoWait + 0.01/* start time; must be nonzero for Mac OS X 10.3 */,
																			delayValue/* time between fires */,
																			sessionPtr->pendingPasteTimerUPP, sessionPtr->selfRef/* user data */,
																			&sessionPtr->pendingPasteTimer);
											assert_noerr(error);
										}
										
										// NOTE: this assignment will replace any previous set of lines
										// that might still exist; although very unlikely, this could
										// happen if the user decided to invoke Paste again while a
										// large and time-consuming Paste was already in progress
										sessionPtr->pendingPasteLines.setWithRetain(pastedLines.returnCFArrayRef());
										sessionPtr->pendingPasteCurrentLine = 0;
									};
			
			
			if (isOneLine)
			{
				// the Clipboard contains only one line of text; Paste immediately without warning
				joinResponder();
			}
			else if (noWarning)
			{
				// the Clipboard contains more than one line, and the user does not want to be warned;
				// proceed with the Paste, but do it without joining (“other button” option)
				normalPasteResponder();
			}
			else
			{
				// configure and display the confirmation alert
				box = AlertMessages_BoxWrap(Alert_NewWindowModalParentCarbon(Session_ReturnActiveLegacyCarbonWindow(inRef)),
											AlertMessages_BoxWrap::kAlreadyRetained);
				
				// set basics
				Alert_SetParamsFor(box.returnRef(), kAlert_StyleOKCancel);
				
				// set message
				{
					CFRetainRelease		primaryTextCFString(UIStrings_ReturnCopy(kUIStrings_AlertWindowPasteLinesWarningPrimaryText),
															CFRetainRelease::kAlreadyRetained);
					CFRetainRelease		helpTextCFString(UIStrings_ReturnCopy(kUIStrings_AlertWindowPasteLinesWarningHelpText),
															CFRetainRelease::kAlreadyRetained);
					
					
					if (primaryTextCFString.exists() && helpTextCFString.exists())
					{
						Alert_SetTextCFStrings(box.returnRef(), primaryTextCFString.returnCFStringRef(),
												helpTextCFString.returnCFStringRef());
					}
				}
				
				// set title
				{
					CFRetainRelease		titleCFString(UIStrings_ReturnCopy(kUIStrings_AlertWindowPasteLinesWarningName),
														CFRetainRelease::kAlreadyRetained);
					
					
					if (titleCFString.exists())
					{
						Alert_SetTitleCFString(box.returnRef(), titleCFString.returnCFStringRef());
					}
				}
				
				// set buttons
				{
					CFRetainRelease		joinString(UIStrings_ReturnCopy(kUIStrings_ButtonMakeOneLine),
													CFRetainRelease::kAlreadyRetained);
					CFRetainRelease		pasteNormallyString(UIStrings_ReturnCopy(kUIStrings_ButtonPasteNormally),
															CFRetainRelease::kAlreadyRetained);
					
					
					Alert_SetButtonText(box.returnRef(), kAlert_ItemButton1, joinString.returnCFStringRef());
					Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemButton1, joinResponder);
					Alert_SetButtonText(box.returnRef(), kAlert_ItemButton3, pasteNormallyString.returnCFStringRef());
					Alert_SetButtonResponseBlock(box.returnRef(), kAlert_ItemButton3,
													^{
														normalPasteResponder();
													});
				}
				
				// ensure that the relevant window is visible and frontmost
				// when the message appears
				TerminalWindow_SetVisible(ptr->terminalWindow, true);
				TerminalWindow_Select(ptr->terminalWindow);
				
				Alert_Display(box.returnRef()); // retains alert until it is dismissed
			}
		}
		CFRelease(pastedCFString), pastedCFString = nullptr;
		CFRelease(pastedDataUTI), pastedDataUTI = nullptr;
	}
	
	return result;
}// UserInputPaste


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
				: Preferences_NewCloneContext(inConfigurationOrNull, true/* detach */),
				Preferences_ContextWrap::kAlreadyRetained),
translationConfiguration(Preferences_NewContext(Quills::Prefs::TRANSLATION),
							Preferences_ContextWrap::kAlreadyRetained),
readOnly(inIsReadOnly),
restartInProgress(false),
kind(kSession_TypeLocalNonLoginShell),
status(kSession_StateBrandNew),
statusAttributes(0),
statusIconName(),
statusString(),
alternateTitle(),
resourceLocationString(),
commandLineArguments(),
originalDirectoryString(),
deviceNameString(),
pendingPasteLines(),
pendingPasteCurrentLine(0),
activationAbsoluteTime(CFAbsoluteTimeGetCurrent()),
terminationAbsoluteTime(0),
watchTriggerAbsoluteTime(CFAbsoluteTimeGetCurrent()),
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
windowValidationListener(ListenerModel_NewStandardListener(windowValidationStateChanged),
							ListenerModel_ListenerWrap::kAlreadyRetained),
terminalWindowListener(), // set at window validation time
vectorWindowListener(ListenerModel_NewStandardListener(vectorGraphicsWindowChanged, this/* context */),
						ListenerModel_ListenerWrap::kAlreadyRetained),
preferencesListener(ListenerModel_NewStandardListener(preferenceChanged, this/* context */),
					ListenerModel_ListenerWrap::kAlreadyRetained),
autoActivateDragTimerUPP(NewEventLoopTimerUPP(autoActivateWindow)),
autoActivateDragTimer(nullptr), // installed only as needed
longLifeTimerUPP(NewEventLoopTimerUPP(detectLongLife)),
longLifeTimer(nullptr), // set later
respawnSessionTimerUPP(NewEventLoopTimerUPP(respawnSession)),
respawnSessionTimer(nullptr), // set later
currentDialog(REINTERPRET_CAST(this, SessionRef)),
terminalWindow(nullptr), // set at window validation time
mainProcess(nullptr),
// controlKey is initialized below
targetVectorGraphics(),
targetDumbTerminals(),
targetTerminals(),
autoCaptureFileName(),
autoCaptureDirectoryURL(),
autoCaptureToFile(false),
autoCaptureIsTemplateName(false),
vectorGraphicsPageOpensNewWindow(true),
vectorGraphicsCommandSet(kVectorInterpreter_ModeTEK4014), // arbitrary, reset later
vectorGraphicsWindows(),
vectorGraphicsInterpreter(nullptr),
readBufferSizeMaximum(4096), // arbitrary, for initialization
readBufferSizeInUse(0),
readBufferPtr(new UInt8[this->readBufferSizeMaximum]),
writeEncoding(kCFStringEncodingUTF8), // initially...
activeWatch(kSession_WatchNothing),
inactivityWatchTimerUPP(nullptr),
inactivityWatchTimer(nullptr),
pendingPasteTimerUPP(nullptr),
pendingPasteTimer(nullptr),
recentSheetContext(),
sheetType(kMy_SessionSheetTypeNone),
renameDialog(nullptr),
weakRefEraser(this),
selfRef(REINTERPRET_CAST(this, SessionRef))
// echo initialized below
// preferencesCache initialized below
{
	bzero(&this->echo, sizeof(this->echo));
	bzero(&this->preferencesCache, sizeof(this->preferencesCache));
	
	assert(nullptr != this->readBufferPtr);
	
	bzero(&this->eventKeys, sizeof(this->eventKeys));
	{
		UInt16		preferenceCount = copyEventKeyPreferences(this, inConfigurationOrNull, true/* search defaults too */);
		
		
		assert(preferenceCount > 0);
	}
	{
		UInt16		preferenceCount = copyVectorGraphicsPreferences(this, inConfigurationOrNull, true/* search defaults too */);
		
		
		assert(preferenceCount > 0);
	}
	{
		UInt16		preferenceCount = copyAutoCapturePreferences(this, inConfigurationOrNull, true/* search defaults too */);
		
		
		assert(preferenceCount > 0);
	}
	
	Session_StartMonitoring(this->selfRef, kSession_ChangeWindowValid, this->windowValidationListener.returnRef());
	Session_StartMonitoring(this->selfRef, kSession_ChangeWindowInvalid, this->windowValidationListener.returnRef());
	
	changeNotifyForSession(this, kSession_ChangeState, this->selfRef/* context */);
	changeNotifyForSession(this, kSession_ChangeStateAttributes, this->selfRef/* context */);
	
	// install a one-shot timer to tell interested parties when this session
	// has been opened for 15 seconds
	UNUSED_RETURN(OSStatus)InstallEventLoopTimer(GetCurrentEventLoop(),
													kEventDurationSecond * kSession_LifetimeMinimumForNoWarningClose,
													kEventDurationForever/* time between fires - this timer does not repeat */,
													this->longLifeTimerUPP, this->selfRef/* user data */, &this->longLifeTimer);
	
	// create a callback for preferences, then listen for certain preferences
	// (this will also initialize the preferences cache values)
	Preferences_StartMonitoring(this->preferencesListener.returnRef(), kPreferences_TagCursorBlinks,
								true/* call immediately to initialize */);
	Preferences_StartMonitoring(this->preferencesListener.returnRef(), kPreferences_TagKioskNoSystemFullScreenMode,
								true/* call immediately to initialize */);
	Preferences_StartMonitoring(this->preferencesListener.returnRef(), kPreferences_TagMapBackquote,
								true/* call immediately to initialize */);
	Preferences_ContextStartMonitoring(this->configuration.returnRef(), this->preferencesListener.returnRef(),
										kPreferences_ChangeContextBatchMode);
	Preferences_ContextStartMonitoring(this->translationConfiguration.returnRef(), this->preferencesListener.returnRef(),
										kPreferences_ChangeContextBatchMode);
	
	// if a user preference is set to immediately handle data events
	// then start a watch automatically
	{
		Session_Watch		backgroundDataWatch = kSession_WatchNothing;
		Session_Watch		idleWatch = kSession_WatchNothing;
		Preferences_Result	prefsResult = kPreferences_ResultOK;
		
		
		prefsResult = Preferences_ContextGetData(inConfigurationOrNull, kPreferences_TagBackgroundNewDataHandler,
													sizeof(backgroundDataWatch), &backgroundDataWatch,
													true/* search for defaults */);
		if ((kPreferences_ResultOK == prefsResult) && (kSession_WatchNothing != backgroundDataWatch))
		{
			Session_SetWatch(this->selfRef, backgroundDataWatch);
		}
		prefsResult = Preferences_ContextGetData(inConfigurationOrNull, kPreferences_TagIdleAfterInactivityHandler,
													sizeof(idleWatch), &idleWatch,
													true/* search for defaults */);
		if ((kPreferences_ResultOK == prefsResult) && (kSession_WatchNothing != idleWatch))
		{
			Session_SetWatch(this->selfRef, idleWatch);
		}
		
		if ((kSession_WatchNothing != backgroundDataWatch) && (kSession_WatchNothing != idleWatch))
		{
			// TEMPORARY; probably the session should allow the user to
			// watch for both inactivity and activity but for now there
			// is only one watch that can be set at a time
			Console_Warning(Console_WriteLine, "both background data and idle activity handlers were set; currently the idle handler takes precedence");
		}
	}
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
	
	sheetContextEnd(this);
	
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
		for (auto screenRef : this->targetTerminals)
		{
			// active session; terminate associated tasks
			if (Terminal_FileCaptureInProgress(screenRef))
			{
				Terminal_FileCaptureEnd(screenRef);
			}
		}
	}
	
	if (nullptr != this->renameDialog)
	{
		WindowTitleDialog_Dispose(&this->renameDialog);
	}
	
	closeTerminalWindow(this);
	
	// clean up
	UNUSED_RETURN(Preferences_Result)Preferences_ContextStopMonitoring(this->configuration.returnRef(), this->preferencesListener.returnRef(),
																		kPreferences_ChangeContextBatchMode);
	UNUSED_RETURN(Preferences_Result)Preferences_ContextStopMonitoring(this->translationConfiguration.returnRef(), this->preferencesListener.returnRef(),
																		kPreferences_ChangeContextBatchMode);
	Preferences_StopMonitoring(this->preferencesListener.returnRef(), kPreferences_TagCursorBlinks);
	Preferences_StopMonitoring(this->preferencesListener.returnRef(), kPreferences_TagKioskNoSystemFullScreenMode);
	Preferences_StopMonitoring(this->preferencesListener.returnRef(), kPreferences_TagMapBackquote);
	
	Session_StopMonitoring(this->selfRef, kSession_ChangeWindowValid, this->windowValidationListener.returnRef());
	Session_StopMonitoring(this->selfRef, kSession_ChangeWindowInvalid, this->windowValidationListener.returnRef());
	
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
	if (nullptr != this->pendingPasteTimer)
	{
		RemoveEventLoopTimer(this->pendingPasteTimer), this->pendingPasteTimer = nullptr;
	}
	DisposeEventLoopTimerUPP(this->pendingPasteTimerUPP), this->pendingPasteTimerUPP = nullptr;
	if (nullptr != this->respawnSessionTimer)
	{
		RemoveEventLoopTimer(this->respawnSessionTimer), this->respawnSessionTimer = nullptr;
	}
	DisposeEventLoopTimerUPP(this->respawnSessionTimerUPP), this->respawnSessionTimerUPP = nullptr;
	
	vectorGraphicsDetachTarget(this);
	for (auto vectorWindowRef : this->vectorGraphicsWindows)
	{
		VectorWindow_StopMonitoring(vectorWindowRef, kVectorWindow_EventWillClose,
									this->vectorWindowListener.returnRef());
		VectorWindow_Release(&vectorWindowRef);
	}
	
	// dispose contents
	if (nullptr != this->readBufferPtr)
	{
		delete [] this->readBufferPtr, this->readBufferPtr = nullptr;
	}
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
void
autoActivateWindow	(EventLoopTimerRef		UNUSED_ARGUMENT(inTimer),
					 void*					inSessionRef)
{
	SessionRef				ref = REINTERPRET_CAST(inSessionRef, SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	
	
	if (nullptr != ptr->terminalWindow) TerminalWindow_Select(ptr->terminalWindow);
}// autoActivateWindow


/*!
Uses the configured file name (optionally as a template) and
configured directory to start a file capture.

(4.1)
*/
Boolean
autoCaptureSessionToFile	(My_SessionPtr		inPtr)
{
	Boolean			result = false;
	Boolean			releaseFileName = false;
	CFStringRef		fileName = inPtr->autoCaptureFileName.returnCFStringRef();
	CFURLRef		directoryURL = inPtr->autoCaptureDirectoryURL.returnCFURLRef();
	
	
	if ((nullptr == fileName) || (nullptr == directoryURL))
	{
		Console_Warning(Console_WriteLine, "automatic capture: file name and/or directory is not defined");
	}
	else
	{
		if (inPtr->autoCaptureIsTemplateName)
		{
			time_t			timeNow = time(nullptr);
			struct tm*		timeData = localtime(&timeNow);
			char			dateFormatBuffer[256/* arbitrary */];
			char			timeFormatBuffer[256/* arbitrary */];
			size_t			formatResult = 0;
			
			
			formatResult = strftime(dateFormatBuffer, sizeof(dateFormatBuffer) - 1, "%Y-%m-%d", timeData);
			if (formatResult <= 0)
			{
				Console_Warning(Console_WriteLine, "failed to format current date");
				fileName = nullptr;
			}
			else
			{
				formatResult = strftime(timeFormatBuffer, sizeof(timeFormatBuffer) - 1, "%H%M%S", timeData);
				if (formatResult <= 0)
				{
					Console_Warning(Console_WriteLine, "failed to format current time");
					fileName = nullptr;
				}
			}
			
			if (nullptr != fileName)
			{
				CFRetainRelease			composedName(CFStringCreateMutableCopy(kCFAllocatorDefault, 0/* length limit */, fileName),
														CFRetainRelease::kAlreadyRetained);
				CFRetainRelease			dateCFString(CFStringCreateWithCString(kCFAllocatorDefault, dateFormatBuffer, kCFStringEncodingASCII),
														CFRetainRelease::kAlreadyRetained);
				CFRetainRelease			timeCFString(CFStringCreateWithCString(kCFAllocatorDefault, timeFormatBuffer, kCFStringEncodingASCII),
														CFRetainRelease::kAlreadyRetained);
				CFMutableStringRef		asMutableCFString = composedName.returnCFMutableStringRef();
				CFIndex					replacementCount = 0;
				
				
				fileName = composedName.returnCFStringRef();
				CFRetain(fileName);
				releaseFileName = true;
				
				// perform template substitutions...
				// ...replace \D with the time in the format YYYY-MM-DD
				replacementCount = CFStringFindAndReplace(asMutableCFString, CFSTR("\\D"), dateCFString.returnCFStringRef(),
															CFRangeMake(0, CFStringGetLength(asMutableCFString)), 0/* compare options */);
				// ...replace \T with the time in the format HH:MM:SS
				replacementCount = CFStringFindAndReplace(asMutableCFString, CFSTR("\\T"), timeCFString.returnCFStringRef(),
															CFRangeMake(0, CFStringGetLength(asMutableCFString)), 0/* compare options */);
				// ...replace doubled backslashes with single backslashes
				replacementCount = CFStringFindAndReplace(asMutableCFString, CFSTR("\\\\"), CFSTR("\\"),
															CFRangeMake(0, CFStringGetLength(asMutableCFString)), 0/* compare options */);
			}
		}
		
		//Console_WriteValueCFString("capture file directory", CFURLGetString(directoryURL)); // debug
		//Console_WriteValueCFString("capture file name", fileName); // debug
		Boolean		captureOK = captureToFile(inPtr, directoryURL, fileName);
		if (false == captureOK)
		{
			Console_Warning(Console_WriteLine, "session failed to start file capture");
		}
		else
		{
			// success!
			result = true;
		}
	}
	
	if (releaseFileName)
	{
		CFRelease(fileName), fileName = nullptr;
	}
	
	return result;
}// autoCaptureSessionToFile


/*!
Initiates a capture of the underlying terminal’s text stream
to the given file.  Returns true unless there is a problem.

(3.0)
*/
Boolean
captureToFile	(My_SessionPtr		inPtr,
				 CFURLRef			inDirectoryToCreateIfNecessary,
				 CFStringRef		inNameOfFileToOverwrite)
{
	Boolean				result = false;
	CFRetainRelease		fullURL(CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, inDirectoryToCreateIfNecessary, inNameOfFileToOverwrite, false/* is directory */),
								CFRetainRelease::kAlreadyRetained);
	
	
	if (false == fullURL.exists())
	{
		Console_Warning(Console_WriteLine, "file capture: unable to compose valid URL from directory and file name components");
	}
	else
	{
		Boolean		createOK = CocoaBasic_CreateFileAndDirectoriesWithData(inDirectoryToCreateIfNecessary, inNameOfFileToOverwrite);
		if (false == createOK)
		{
			Console_Warning(Console_WriteValueCFString, "unable to create specified capture file:", inNameOfFileToOverwrite);
		}
		else
		{
			Boolean		captureOK = Terminal_FileCaptureBegin(inPtr->targetTerminals.front(), fullURL.returnCFURLRef());
			if (false == captureOK)
			{
				Console_Warning(Console_WriteLine, "unable to start file capture from terminal");
			}
			else
			{
				// success!
				result = true;
			}
		}
		
		// UNIMPLEMENTED: could set window proxy icon to location of capture file
	}
	
	return result;
}// captureToFile


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
	setIconFromState(inPtr);
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
Attempts to read all supported auto-capture-to-file tags from the
given preference context, and any settings that exist will be
used to update the specified session.

Returns the number of settings that were changed.

(4.1)
*/
UInt16
copyAutoCapturePreferences		(My_SessionPtr				inPtr,
								 Preferences_ContextRef		inSource,
								 Boolean					inSearchDefaults)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	CFStringRef				stringValue = nullptr;
	Preferences_URLInfo		urlInfoValue;
	Boolean					flag = false;
	UInt16					result = 0;
	
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagCaptureAutoStart,
												sizeof(flag), &flag, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		inPtr->autoCaptureToFile = flag;
		++result;
		
		prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagCaptureFileNameAllowsSubstitutions,
													sizeof(flag), &flag, inSearchDefaults);
		if (kPreferences_ResultOK == prefsResult)
		{
			inPtr->autoCaptureIsTemplateName = flag;
			++result;
		}
		else
		{
			// this setting may reasonably not exist; ignore
			inPtr->autoCaptureIsTemplateName = false;
		}
		
		prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagCaptureFileName,
													sizeof(stringValue), &stringValue, inSearchDefaults);
		if (kPreferences_ResultOK == prefsResult)
		{
			//Console_WriteValueCFString("setting capture file name", stringValue); // debug
			inPtr->autoCaptureFileName.setWithRetain(stringValue);
			CFRelease(stringValue), stringValue = nullptr; // Preferences module creates a new string
			++result;
		}
		else
		{
			Console_Warning(Console_WriteLine, "capture file name not found");
			inPtr->autoCaptureFileName.clear();
			inPtr->autoCaptureToFile = false; // need directory information in order to capture
		}
		
		prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagCaptureFileDirectoryURL,
													sizeof(urlInfoValue), &urlInfoValue, inSearchDefaults);
		if (kPreferences_ResultOK == prefsResult)
		{
			//NSLog(@"setting capture file directory URL to “%@”", BRIDGE_CAST(urlValue, NSURL*)); // debug
			inPtr->autoCaptureDirectoryURL.setWithRetain(urlInfoValue.urlRef);
			CFRelease(urlInfoValue.urlRef), urlInfoValue.urlRef = nullptr; // Preferences module creates a new URL
			++result;
		}
		else
		{
			Console_Warning(Console_WriteLine, "capture file directory not found");
			inPtr->autoCaptureDirectoryURL.clear();
			inPtr->autoCaptureToFile = false; // need directory information in order to capture
		}
	}
	else
	{
		inPtr->autoCaptureToFile = false;
	}
	
	return result;
}// copyAutoCapturePreferences


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
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagNewLineMapping,
												sizeof(inPtr->eventKeys.newline), &inPtr->eventKeys.newline, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		++result;
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
Attempts to read all supported vector graphics tags from the
given preference context, and any settings that exist will be
used to update the specified session.

Returns the number of settings that were changed.

(4.0)
*/
UInt16
copyVectorGraphicsPreferences	(My_SessionPtr				inPtr,
								 Preferences_ContextRef		inSource,
								 Boolean					inSearchDefaults)
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	VectorInterpreter_Mode	commandSet = kVectorInterpreter_ModeDisabled;
	Boolean					flag = false;
	UInt16					result = 0;
	
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagTektronixMode,
												sizeof(commandSet), &commandSet, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		inPtr->vectorGraphicsCommandSet = commandSet;
		++result;
	}
	
	prefsResult = Preferences_ContextGetData(inSource, kPreferences_TagTektronixPAGEClearsScreen,
												sizeof(flag), &flag, inSearchDefaults);
	if (kPreferences_ResultOK == prefsResult)
	{
		inPtr->vectorGraphicsPageOpensNewWindow = !flag;
		++result;
	}
	
	return result;
}// copyVectorGraphicsPreferences


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
for a help tag that displays what is currently being routed
to Local Echo in the session.

You must call the rename() method on the returned tag to
show the appropriate echo text.

Normally, this tag should be displayed at the terminal
cursor location, so prior to using the result you should
call its setFrame() method.

(4.0)
*/
My_HMHelpContentRecWrap&
createHelpTagForLocalEcho ()
{
	static My_HMHelpContentRecWrap		gTag;
	
	
	return gTag;
}// createHelpTagForLocalEcho


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
void
detectLongLife	(EventLoopTimerRef		UNUSED_ARGUMENT(inTimer),
				 void*					inSessionRef)
{
	SessionRef				ref = REINTERPRET_CAST(inSessionRef, SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	
	
	if (Session_StateIsActiveUnstable(ref)) Session_SetState(ref, kSession_StateActiveStable);
}// detectLongLife


/*!
Responds to an NSSavePanel that closed with the
user selecting the primary action button.  See
Session_DisplaySaveDialog().

(2016.10)
*/
void
handleSaveFromPanel		(My_SessionPtr		inPtr,
						 NSSavePanel*		inSavePanel)
{
	NSError*	errorObject = nil;
	OSStatus	error = noErr;
	FSRef		saveDir;
	FSRef		saveFile;
	
	
	if (nil != errorObject)
	{
		[NSApp presentError:errorObject];
	}
	else
	{
		error = FSPathMakeRef(REINTERPRET_CAST(inSavePanel.directoryURL.path.UTF8String, UInt8 const*),
								&saveDir, nullptr/* is directory */);
		if (noErr != error)
		{
			Console_Warning(Console_WriteValue, "failed to find save directory, error", error);
			Alert_ReportOSStatus(error);
		}
	}
	
	// TEMPORARY; until dependent code can be updated to stop
	// using old File Manager calls, convert URL into FSRef
	if (noErr == error)
	{
		NSRange		nameRange = NSMakeRange(0, inSavePanel.nameFieldStringValue.length);
		UniChar*	buffer = new UniChar[nameRange.length];
		
		
		[inSavePanel.nameFieldStringValue getCharacters:buffer range:nameRange];
		
		error = FSCreateFileUnicode(&saveDir, nameRange.length, buffer, kFSCatInfoNone, nullptr/* catalog info */,
									&saveFile, nullptr/* returned spec. */);
		if (dupFNErr == error)
		{
			// if file already exists, obtain the reference in another way
			// (save panel will have asked user for permission to overwrite)
			error = FSPathMakeRef(REINTERPRET_CAST(inSavePanel.URL.path.UTF8String, UInt8 const*), &saveFile, nullptr/* is directory */);
		}
		if (noErr != error)
		{
			Console_Warning(Console_WriteValue, "failed to find or create save file specified in panel, error", error);
			Alert_ReportOSStatus(error);
		}
		
		delete [] buffer, buffer = nullptr;
	}
	
	if (noErr == error)
	{
		// write to file, using in-memory model first
		Session_Result			sessionError = kSession_ResultOK;
		SessionDescription_Ref  saveFileMemoryModel = nullptr;
		
		
		sessionError = Session_FillInSessionDescription(inPtr->selfRef, &saveFileMemoryModel);
		if (sessionError != kSession_ResultOK)
		{
			Console_Warning(Console_WriteValue, "failed to generate session description, error", sessionError.code());
			Alert_ReportOSStatus(resNotFound/* arbitrary */);
		}
		else
		{
			// now save to disk
			SInt16		fileRefNum = -1;
			FSRef		temporaryFile;
			
			
			// open temporary file for overwrite (later, this will
			// be swapped with the target file)
			fileRefNum = FileUtilities_OpenTemporaryFile(temporaryFile);
			if (fileRefNum <= 0)
			{
				Console_Warning(Console_WriteLine, "failed to open temporary file");
				Alert_ReportOSStatus(writErr/* arbitrary */);
			}
			else
			{
				SessionDescription_Result	saveError = kSessionDescription_ResultOK;
				
				
				UNUSED_RETURN(OSStatus)FSSetForkSize(fileRefNum, fsFromStart, 0);
				saveError = SessionDescription_Save(saveFileMemoryModel, fileRefNum);
				UNUSED_RETURN(OSStatus)FSCloseFork(fileRefNum), fileRefNum = -1;
				if (saveError != kSessionDescription_ResultOK)
				{
					Alert_ReportOSStatus(writErr/* arbitrary */);
				}
				else
				{
					error = FSExchangeObjects(&temporaryFile, &saveFile);
					if (noErr != error)
					{
						// if a “safe” save fails, the volume may not support object exchange;
						// so, fall back to overwriting the original file in that case
						error = FSOpenFork(&saveFile, 0/* fork name length */, nullptr/* fork name */,
											fsWrPerm, &fileRefNum);
						if (noErr != error)
						{
							Console_Warning(Console_WriteValue, "failed to open target (data is still in temporary file), error", error);
							Alert_ReportOSStatus(error);
						}
						else
						{
							// swap with temporary file failed; attempt save again,
							// this time directly to the target location
							UNUSED_RETURN(OSStatus)FSSetForkSize(fileRefNum, fsFromStart, 0);
							saveError = SessionDescription_Save(saveFileMemoryModel, fileRefNum);
							if (kSessionDescription_ResultOK != saveError)
							{
								Console_Warning(Console_WriteValue, "failed to save session, error", saveError);
							}
							UNUSED_RETURN(OSStatus)FSCloseFork(fileRefNum), fileRefNum = -1;
						}
					}
				}
				
				// delete the temporary; it will either contain a partial save file
				// that did not successfully write, or it will contain whatever the
				// user’s original file had (and the new file will be successfully
				// in the user’s selected location, instead of temporary space)
				UNUSED_RETURN(OSStatus)FSDeleteObject(&temporaryFile);
			}
			
			// clean up
			SessionDescription_Release(&saveFileMemoryModel);
		}
	}
}// handleSaveFromPanel


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
	static UInt32			virtualKeyCode = '\0'; // see p.2-43 of "IM:MTE" for a set of virtual key codes
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
	if (ptr->eventKeys.meta == kSession_EmacsMetaKeyShiftOption)
	{
		metaDown = ((shiftDown) && (optionDown));
	}
	else if (ptr->eventKeys.meta == kSession_EmacsMetaKeyOption)
	{
		metaDown = (optionDown);
	}
	
	// scan for keys that invoke instant commands
	switch (virtualKeyCode)
	{
	case kVK_F1: // 0x7A
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF6);
		break;
	
	case kVK_F2: // 0x78
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF7);
		break;
	
	case kVK_F3: // 0x63
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF8);
		break;
	
	case kVK_F4: // 0x76
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF9);
		break;
	
	case kVK_F5: // 0x60
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF10);
		break;
	
	case kVK_F6: // 0x61
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF11);
		break;
	
	case kVK_F7: // 0x62
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF12);
		break;
	
	case kVK_F8: // 0x64
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF13);
		break;
	
	case kVK_F9: // 0x65
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF14);
		break;
	
	case kVK_F10: // 0x6D
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF15_220HELP);
		break;
	
	case kVK_F11: // 0x67
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF16_220DO);
		break;
	
	case kVK_F12: // 0x6F
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF17);
		break;
	
	case kVK_F13: // 0x69 (Print Screen)
	#if 0
		if (0/* if not VT220 */)
		{
			result = Commands_ExecuteByID(kCommandPrintScreen);
		}
		else
	#endif
		{
			// TEMPORARY: only makes sense for VT220 terminals
			Session_UserInputKey(session, VSF18);
		}
		break;
	
	case kVK_F14: // 0x6B
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF19);
		break;
	
	case kVK_F15: // 0x71
		// TEMPORARY: only makes sense for VT220 terminals
		Session_UserInputKey(session, VSF20);
		break;
	
	case VSPGUP_220DEL:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((ptr->eventKeys.pageKeysLocalControl) || Terminal_EmulatorIsVT100(someScreen))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewPageUp);
		}
		break;
	
	case VSPGDN_220PGDN:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((ptr->eventKeys.pageKeysLocalControl) || Terminal_EmulatorIsVT100(someScreen))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewPageDown);
		}
		break;
	
	case VSHOME_220INS:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((ptr->eventKeys.pageKeysLocalControl) || Terminal_EmulatorIsVT100(someScreen))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewHome);
		}
		break;
	
	case VSEND_220PGUP:
		// TEMPORARY: this needs to be determined in a more abstract way,
		// perhaps by inquiring the Terminal module whether or not the
		// active terminal type supports this
		if ((ptr->eventKeys.pageKeysLocalControl) || Terminal_EmulatorIsVT100(someScreen))
		{
			result = Commands_ExecuteByID(kCommandTerminalViewEnd);
		}
		break;
	
	case kVK_LeftArrow: // 0x7B
		Session_UserInputKey(session, VSLT, eventModifiers);
		result = true;
		break;
	
	case kVK_RightArrow: // 0x7C
		Session_UserInputKey(session, VSRT, eventModifiers);
		result = true;
		break;
	
	case kVK_DownArrow: // 0x7D
		Session_UserInputKey(session, VSDN, eventModifiers);
		result = true;
		break;
	
	case kVK_UpArrow: // 0x7E
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
			Boolean		sentCommand = false;
			
			
			if (characterCode == ptr->eventKeys.suspend)
			{
				Session_SetNetworkSuspended(session, true);
				sentCommand = true;
				result = true;
			}
			if (characterCode == ptr->eventKeys.resume) 
			{
				Session_SetNetworkSuspended(session, false);
				sentCommand = true;
				result = true;
			}
			if (characterCode == ptr->eventKeys.interrupt)  
			{
				Session_UserInputInterruptProcess(session);
				sentCommand = true;
				result = true;
			}
			
			if ((characterCode <= 0x1F) && ('\015' != characterCode) && (false == sentCommand))
			{
				// control key (except carriage return, which is handled later,
				// and any of the special control key sequences above)
				Session_UserInputKey(session, STATIC_CAST(characterCode, UInt8));
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
				UInt8		charactersToSend[3];
				UInt8*		characterPtr = charactersToSend;
				size_t		theSize = sizeof(charactersToSend);
				
				
				// terminate
				charactersToSend[2] = '\0';
				
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
				
				// show local echo of the keystroke, if appropriate
				if (Session_LocalEchoIsEnabled(session))
				{
					CFRetainRelease		asObject(CFStringCreateWithCString(kCFAllocatorDefault, REINTERPRET_CAST(characterPtr, char const*),
																			kCFStringEncodingUTF8),
													CFRetainRelease::kAlreadyRetained);
					
					
					localEchoString(ptr, asObject.returnCFStringRef());
				}
				
				// finally, send the character sequence to the session
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
Handles local echoing of the specified key code.

Any key that the local terminal does not understand will be
displayed in a hovering window instead of being sent to the
local terminal.

If the echo is set to hover in a window, the window may display
a glyph that represents the key instead of a raw interpretation
of the byte.

If the echo is set to enter text into the local terminal, the
interpretation of the key won’t necessarily match the hover
version, and there may even be no effect at all.  But typically
any cursor-control keys will locally move the cursor, and any
printable characters will just be displayed.

(4.0)
*/
void
localEchoKey	(My_SessionPtr	inPtr,
				 UInt8			inKeyOrASCII)
{
	Boolean		writeVerbatim = true;
	Boolean		hoverEcho = false;
	
	
	// special case; control characters that are sent as strings
	// of a single byte are converted into visible sequences
	// because they will not otherwise be visible
	if (inKeyOrASCII <= 0x1F)
	{
		if (0x0D == inKeyOrASCII)
		{
			// carriage return; special-case this and do not print control-M
			//UInt8	seqUTF8[] = { 0xE2, 0x8F, 0x8E }; // return symbol
			UInt8	seqUTF8[] = { 0xE2, 0x86, 0xB5 }; // down-left-hook symbol
			
			
			terminalHoverLocalEchoString(inPtr, seqUTF8, sizeof(seqUTF8));
			writeVerbatim = false;
		}
		else
		{
			// control character; convert to a caret sequence (but with the Unicode control symbol)
			char const*		controlSymbols = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]~?"; // must contain exactly 32 symbols
			UInt8			controlCharBuffer[4] = { 0xE2, 0x8C, 0x83, '\0' }; // UTF-8 control symbol followed by a letter/symbol
			
			
			assert(32 == CPP_STD::strlen(controlSymbols));
			controlCharBuffer[3] = controlSymbols[inKeyOrASCII];
			terminalHoverLocalEchoString(inPtr, controlCharBuffer, sizeof(controlCharBuffer));
			writeVerbatim = false;
		}
	}
	else if (127 == inKeyOrASCII)
	{
		// delete; make the Unicode (UTF-8) backward-delete symbol
		UInt8	seqUTF8[] = { 0xE2, 0x8C, 0xAB };
		
		
		terminalHoverLocalEchoString(inPtr, seqUTF8, sizeof(seqUTF8));
		writeVerbatim = false;
	}
	else
	{
		switch (inKeyOrASCII)
		{
		case VSLT:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				UInt8	seqUTF8[] = { 0xE2, 0x87, 0xA0 }; // keyboard left arrow
				
				
				terminalHoverLocalEchoString(inPtr, seqUTF8, sizeof(seqUTF8));
				writeVerbatim = false;
			}
			break;
		
		case VSRT:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				UInt8	seqUTF8[] = { 0xE2, 0x87, 0xA2 }; // keyboard right arrow
				
				
				terminalHoverLocalEchoString(inPtr, seqUTF8, sizeof(seqUTF8));
				writeVerbatim = false;
			}
			break;
		
		case VSUP:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				UInt8	seqUTF8[] = { 0xE2, 0x87, 0xA1 }; // keyboard up arrow
				
				
				terminalHoverLocalEchoString(inPtr, seqUTF8, sizeof(seqUTF8));
				writeVerbatim = false;
			}
			break;
		
		case VSDN:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				UInt8	seqUTF8[] = { 0xE2, 0x87, 0xA3 }; // keyboard down arrow
				
				
				terminalHoverLocalEchoString(inPtr, seqUTF8, sizeof(seqUTF8));
				writeVerbatim = false;
			}
			break;
		
		case VSKE:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				UInt8	seqUTF8[] = { 0xE2, 0x8C, 0xA4 }; // enter symbol (arrow head between two bars)
				
				
				terminalHoverLocalEchoString(inPtr, seqUTF8, sizeof(seqUTF8));
				writeVerbatim = false;
			}
			break;
		
		case VSF1:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("PF1", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF2:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("PF2", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF3:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("PF3", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF4:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("PF4", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF6:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F6", UInt8 const*), 2);
				writeVerbatim = false;
			}
			break;
		
		case VSF7:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F7", UInt8 const*), 2);
				writeVerbatim = false;
			}
			break;
		
		case VSF8:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F8", UInt8 const*), 2);
				writeVerbatim = false;
			}
			break;
		
		case VSF9:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F9", UInt8 const*), 2);
				writeVerbatim = false;
			}
			break;
		
		case VSF10:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F10", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF11:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F11", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF12:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F12", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF13:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F13", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF14:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F14", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF15_220HELP:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("help", UInt8 const*), 4);
				writeVerbatim = false;
			}
			break;
		
		case VSF16_220DO:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("do", UInt8 const*), 2);
				writeVerbatim = false;
			}
			break;
		
		case VSF17:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F17", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF18:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F18", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF19:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F19", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSF20:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("F20", UInt8 const*), 3);
				writeVerbatim = false;
			}
			break;
		
		case VSHELP_220FIND:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("find", UInt8 const*), 4); // LOCALIZE THIS
				writeVerbatim = false;
			}
			break;
		
		case VSHOME_220INS:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("insert", UInt8 const*), 6); // LOCALIZE THIS
				writeVerbatim = false;
			}
			break;
		
		case VSPGUP_220DEL:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("delete", UInt8 const*), 6); // LOCALIZE THIS
				writeVerbatim = false;
			}
			break;
		
		case VSDEL_220SEL:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("select", UInt8 const*), 6); // LOCALIZE THIS
				writeVerbatim = false;
			}
			break;
		
		case VSEND_220PGUP:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("page up", UInt8 const*), 7); // LOCALIZE THIS
				writeVerbatim = false;
			}
			break;
		
		case VSPGDN_220PGDN:
			hoverEcho = true; // avoid insertion, since the local terminal won’t do anything with this key
			if (hoverEcho)
			{
				// TEMPORARY: only makes sense for VT220 terminals
				terminalHoverLocalEchoString(inPtr, REINTERPRET_CAST("page down", UInt8 const*), 9); // LOCALIZE THIS
				writeVerbatim = false;
			}
			break;
		
		default:
			// ???
			break;
		}
	}
	
	if (writeVerbatim)
	{
		terminalInsertLocalEchoString(inPtr, &inKeyOrASCII, 1/* byte count */);
	}
}// localEchoKey


/*!
Immediately sends the specified string to the local terminal,
as bytes in its output text encoding (defined by a call to
Session_SetSendDataEncoding()).

(4.0)
*/
void
localEchoString		(My_SessionPtr		inPtr,
					 CFStringRef		inStringBuffer)
{
	if (nullptr != inStringBuffer)
	{
		// convert the stream from UTF-8 into the encoding of the session
		try
		{
			size_t const	kBufferSize = (1 + CFStringGetMaximumSizeForEncoding
											(CFStringGetLength(inStringBuffer), inPtr->writeEncoding));
			char*			stringBuffer = new char[kBufferSize];
			
			
			stringBuffer[kBufferSize - 1] = '\0';
			if (false == CFStringGetCString(inStringBuffer, stringBuffer, kBufferSize, inPtr->writeEncoding))
			{
				Console_Warning(Console_WriteValue, "input text does not match terminal with text encoding", inPtr->writeEncoding);
				Console_Warning(Console_WriteValueCFString, "cannot convert into the encoding used by this terminal, text", inStringBuffer);
			}
			else
			{
				terminalInsertLocalEchoString(inPtr, REINTERPRET_CAST(stringBuffer, UInt8 const*), CPP_STD::strlen(stringBuffer));
			}
			delete [] stringBuffer;
		}
		catch (std::bad_alloc)
		{
			Console_WriteLine("local echo of string failed, out of memory!");
		}
	}
}// localEchoString


/*!
This is invoked after a user-defined delay, and it pastes the
next pending line of text (if any).  Once all lines are pasted,
the timer is destroyed and will not trigger again until the
user tries to Paste something else.

Timers that draw must save and restore the current graphics port.

(4.0)
*/
void
pasteLineFromTimer	(EventLoopTimerRef		UNUSED_ARGUMENT(inTimer),
					 void*					inSessionRef)
{
	SessionRef				ref = REINTERPRET_CAST(inSessionRef, SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	
	
	if (ptr->pendingPasteLines.exists())
	{
		CFArrayRef		asCFArray = ptr->pendingPasteLines.returnCFArrayRef();
		CFIndex			arrayLength = CFArrayGetCount(asCFArray);
		
		
		if ((ptr->pendingPasteCurrentLine >= arrayLength) || (0 == arrayLength))
		{
			// all lines have been pasted; the array is no longer needed
			ptr->pendingPasteLines.clear();
			RemoveEventLoopTimer(ptr->pendingPasteTimer), ptr->pendingPasteTimer = nullptr;
			DisposeEventLoopTimerUPP(ptr->pendingPasteTimerUPP), ptr->pendingPasteTimerUPP = nullptr;
		}
		else
		{
			Session_UserInputCFString(ref, CFUtilities_StringCast(CFArrayGetValueAtIndex
																	(asCFArray, ptr->pendingPasteCurrentLine)));
			if (ptr->pendingPasteCurrentLine < (arrayLength - 1))
			{
				Session_SendNewline(ref, kSession_EchoCurrentSessionValue);
			}
			++(ptr->pendingPasteCurrentLine);
		}
	}
}// pasteLineFromTimer


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
	
	
	switch (inPreferenceTagThatChanged)
	{
	case kPreferences_TagCursorBlinks:
		// update cache with current preference value
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagCursorBlinks,
									sizeof(ptr->preferencesCache.cursorFlashes),
									&ptr->preferencesCache.cursorFlashes))
		{
			ptr->preferencesCache.cursorFlashes = true; // assume a value, if preference can’t be found
		}
		break;
	
	case kPreferences_TagKioskNoSystemFullScreenMode:
		// update all terminal windows to show or hide a system Full Screen icon
		{
			Boolean		useCustomMethod = false;
			
			
			unless (kPreferences_ResultOK ==
					Preferences_GetData(kPreferences_TagKioskNoSystemFullScreenMode,
										sizeof(useCustomMethod), &useCustomMethod))
			{
				useCustomMethod = false; // assume a value, if preference can’t be found
			}
			
			TerminalWindow_SetFullScreenIconsEnabled(false == useCustomMethod);
		}
		break;
	
	case kPreferences_TagMapBackquote:
		// update cache with current preference value
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagMapBackquote,
									sizeof(ptr->preferencesCache.remapBackquoteToEscape),
									&ptr->preferencesCache.remapBackquoteToEscape))
		{
			ptr->preferencesCache.remapBackquoteToEscape = false; // assume a value, if preference can’t be found
		}
		break;
	
	default:
		if (kPreferences_ChangeContextBatchMode == inPreferenceTagThatChanged)
		{
			// batch mode; multiple things have changed, so check for the new values
			// of everything that is understood by a session
			if (ptr->translationConfiguration.returnRef() == prefsContext)
			{
				// this context is a proxy for translation settings; these
				// settings are “forwarded” to views and terminal screens
				// underneath, but may also be processed by the Session itself
				if (nullptr != ptr->terminalWindow)
				{
					Boolean		changeOK = TerminalWindow_ReconfigureViewsInGroup
											(ptr->terminalWindow, kTerminalWindow_ViewGroupActive,
												prefsContext, Quills::Prefs::TRANSLATION);
					
					
					if (false == changeOK)
					{
						Console_Warning(Console_WriteLine, "detected session translation changes but failed to apply them to the terminal window");
					}
				}
				
				// if the session is entering or leaving UTF-8 encoding, update the
				// underlying process’ pseudo-terminal accordingly (typically, this
				// at least means that a deleted character may absorb more than one
				// byte instead of the usual single byte)
				if (nullptr != ptr->mainProcess)
				{
					CFStringEncoding const	kGivenTextEncoding = TextTranslation_ContextReturnEncoding(prefsContext);
					Boolean const			kIsUTF8 = (kCFStringEncodingUTF8 == kGivenTextEncoding);
					Boolean					deviceUpdateOK = false;
					
					
					deviceUpdateOK = Local_TerminalSetUTF8Encoding(Local_ProcessReturnMasterTerminal(ptr->mainProcess), kIsUTF8);
					if (false == deviceUpdateOK)
					{
						Console_Warning(Console_WriteLine, "failed to update UTF-8 mode of pseudo-terminal device");
					}
				}
			}
			else
			{
				// the session’s “normal” configuration, containing settings that
				// apply DIRECTLY to the session (not acting as a proxy for
				// anything else)
				UNUSED_RETURN(UInt16)copyEventKeyPreferences(ptr, prefsContext, false/* search for defaults */);
				UNUSED_RETURN(UInt16)copyVectorGraphicsPreferences(ptr, prefsContext, false/* search for defaults */);
			}
		}
		else
		{
			// ???
		}
		break;
	}
}// preferenceChanged


/*!
Reads as much available data as possible (based on the size of
the processing buffer) and processes it, which will result in
either displaying it as text or interpreting it as one or more
commands.  The number of bytes left to be processed is returned,
so if the result is greater than zero you ought to call this
routine again.

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
	(Session_Result)Session_ReceiveData(inPtr->selfRef, inPtr->readBufferPtr, inPtr->readBufferSizeInUse);
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
							UNUSED_RETURN(OSStatus)InstallEventLoopTimer
													(GetCurrentEventLoop(),
														kEventDurationSecond/* arbitrary */,
														kEventDurationForever/* time between fires - this timer does not repeat */,
														ptr->autoActivateDragTimerUPP, ptr->selfRef/* user data */,
														&ptr->autoActivateDragTimer);
						}
						
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
						UNUSED_RETURN(OSStatus)RemoveEventLoopTimer(ptr->autoActivateDragTimer), ptr->autoActivateDragTimer = nullptr;
					}
					result = noErr;
					break;
				
				case kEventControlDragReceive:
					// something was actually dropped
					result = sessionDragDrop(inHandlerCallRef, inEvent, session, view, dragRef);
					if (nullptr != ptr->autoActivateDragTimer)
					{
						UNUSED_RETURN(OSStatus)RemoveEventLoopTimer(ptr->autoActivateDragTimer), ptr->autoActivateDragTimer = nullptr;
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


/*!
Handles "kEventControlTrackingAreaEntered" of "kEventClassControl"
for a terminal view.

(3.1)
*/
OSStatus
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
		
		
		// do not change focus unless the preference is set;
		// TEMPORARY: possibly inefficient to always check this here,
		// the alternative would be to update preferenceChanged() to
		// monitor changes to the preference and add/remove the
		// tracking handler as appropriate
		unless (kPreferences_ResultOK ==
				Preferences_GetData(kPreferences_TagFocusFollowsMouse, sizeof(focusFollowsMouse),
									&focusFollowsMouse))
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
					if (noErr != error)
					{
						// odd...ignore error and move forward
						oldWindowModality = kWindowModalityNone;
					}
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


/*!
Handles "kEventTextInputUnicodeForKeyEvent" of
"kEventClassTextInput" for a session’s terminal views.

(3.0)
*/
OSStatus
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
				
				// filter out Escape key presses, they are likely to matter more to terminals
				// (in particular, ignore this as a way to Exit Full Screen)
				if (characterCodes[0] == 0x1B)
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
				(controlKeyPressInfo.virtualKeyCode == kVK_ANSI_KeypadEnter/* 0x4C */))
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
		case kVK_Delete: // 0x33
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
		[NSCursor setHiddenUntilMouseMoves:YES];
		
		// note that a key equivalent for a menu item might be invoked
		// when the corresponding item is disabled; Cocoa seems to forward
		// such events to the next window instead of absorbing them, and
		// this should not cause a command key letter to be typed into a
		// terminal; an explicit command key check avoids the problem
		if (false == controlKeyPressInfo.commandDown)
		{
			handleSessionKeyDown(nullptr/* unused listener model */, 0/* unused event code */,
									&controlKeyPressInfo/* event context */, session/* listener context */);
		}
	}
	
	return result;
}// receiveTerminalViewTextInput


/*!
Handles "kEventWindowClose" of "kEventClassWindow"
for a session’s terminal window.

(3.0)
*/
OSStatus
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
			// arrange to close the window; this might happen immediately
			// if the session is already dead or it has not been open
			// for long, otherwise it will present a warning to the user
			// and the window will be killed asynchronously (if at all)
			Session_DisplayTerminationWarning(session);
			
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
OSStatus
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
Respawns the original command line for the session, using the
same window.  This should only be invoked after the previous
session is dead.

Timers that draw must save and restore the current graphics port.

(4.0)
*/
void
respawnSession	(EventLoopTimerRef		UNUSED_ARGUMENT(inTimer),
				 void*					inSessionRef)
{
	SessionRef				ref = REINTERPRET_CAST(inSessionRef, SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	Boolean					respawnOK = SessionFactory_RespawnSession(ref);
	
	
	if (false == respawnOK)
	{
		Sound_StandardAlert();
		Console_Warning(Console_WriteLine, "failed to restart session");
	}
}// respawnSession


/*!
Returns the Carbon window most recently used by this
session or nullptr if the session was constructed
using a modern Cocoa window.

DEPRECATED.  Migrate to returnActiveNSWindow().

(4.0)
*/
HIWindowRef
returnActiveLegacyCarbonWindow	(My_SessionPtr		inPtr)
{
	HIWindowRef		result = nullptr;
	
	
	if ((nullptr != inPtr) && Session_IsValid(inPtr->selfRef))
	{
		if ((nullptr != inPtr->terminalWindow) && TerminalWindow_IsLegacyCarbon(inPtr->terminalWindow))
		{
			result = TerminalWindow_ReturnLegacyCarbonWindow(inPtr->terminalWindow);
		}
	}
	return result;
}// returnActiveLegacyCarbonWindow


/*!
Returns the window most recently used by this session.

(4.1)
*/
NSWindow*
returnActiveNSWindow	(My_SessionPtr		inPtr)
{
	NSWindow*	result = nullptr; // should be "nil" (TEMPORARY; fix when file is converted to Objective-C)
	
	
	if ((nullptr != inPtr) && Session_IsValid(inPtr->selfRef))
	{
		if (nullptr != inPtr->terminalWindow)
		{
			result = TerminalWindow_ReturnNSWindow(inPtr->terminalWindow);
		}
	}
	return result;
}// returnActiveNSWindow


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
				
				
				// if the user preference is set, first move the cursor to the drop location
				unless (kPreferences_ResultOK ==
						Preferences_GetData(kPreferences_TagCursorMovesPriorToDrops,
											sizeof(cursorMovesPriorToDrops), &cursorMovesPriorToDrops))
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
Updates the name of the icon file that is stored in
the session to represent its current state.

If no attributes have been applied to the state, the
icon will represent absolute state (such as whether
or not it is running).  However, certain attributes
can override the icon.

This function should be called when the state or
state attributes are changed.

(4.0)
*/
void
setIconFromState	(My_SessionPtr	inPtr)
{
	
	if ((inPtr->statusAttributes & kSession_StateAttributeNotification) ||
		(inPtr->statusAttributes & kSession_StateAttributeOpenDialog))
	{
		inPtr->statusIconName.setWithRetain(AppResources_ReturnCautionIconFilenameNoExtension());
	}
	else if (kSession_StateDead == inPtr->status)
	{
		inPtr->statusIconName.setWithRetain(AppResources_ReturnSessionStatusDeadIconFilenameNoExtension());
	}
	else
	{
		inPtr->statusIconName.setWithRetain(AppResources_ReturnSessionStatusActiveIconFilenameNoExtension());
	}
}// setIconFromState


/*!
Responds to a close of any sheet on a Session that is
updating a context constructed by sheetContextBegin().

This calls sheetContextEnd() to ensure that the context is
cleaned up.

(4.0)
*/
void
sheetClosed		(GenericDialog_Ref		inDialogThatClosed,
				 Boolean				inOKButtonPressed)
{
	SessionRef				ref = REINTERPRET_CAST(GenericDialog_ReturnImplementation(inDialogThatClosed), SessionRef);
	My_SessionAutoLocker	ptr(gSessionPtrLocks(), ref);
	
	
	if (nullptr == ptr)
	{
		Console_Warning(Console_WriteLine, "unexpected problem finding Session that corresponds to a closed sheet");
	}
	else
	{
		if (inOKButtonPressed)
		{
			switch (ptr->sheetType)
			{
			case kMy_SessionSheetTypeSpecialKeySequences:
				UNUSED_RETURN(UInt16)copyEventKeyPreferences(ptr, ptr->recentSheetContext.returnRef(),
																true/* search defaults */);
				break;
			
			default:
				Console_Warning(Console_WriteLine, "no active sheet but sheet context still exists and was changed");
				break;
			}
		}
		
		sheetContextEnd(ptr);
	}
}// sheetClosed


/*!
Constructs a new sheet context and starts monitoring it for
changes.  The given sheet type determines what the response
will be when settings are dumped into the target context.

The returned context is stored as "recentSheetContext" in the
specified window structure, and is nullptr if there was any
error.

(4.1)
*/
Preferences_ContextRef
sheetContextBegin	(My_SessionPtr			inPtr,
					 Quills::Prefs::Class	inClass,
					 My_SessionSheetType	inSheetType)
{
	Preferences_ContextWrap		newContext(Preferences_NewContext(inClass),
											Preferences_ContextWrap::kAlreadyRetained);
	Preferences_ContextRef		result = nullptr;
	
	
	if (kMy_SessionSheetTypeNone == inPtr->sheetType)
	{
		Preferences_Result		prefsResult = kPreferences_ResultOK;
		Boolean					copyOK = false;
		
		
		// initialize settings so that the sheet has the right data
		// IMPORTANT: the contexts and tag sets chosen here should match those
		// used elsewhere in this file to update preferences later (such as
		// copyEventKeyPreferences())
		switch (inSheetType)
		{
		case kMy_SessionSheetTypeSpecialKeySequences:
			{
				Preferences_TagSetRef	tagSet = PrefPanelSessions_NewKeyboardPaneTagSet();
				
				
				prefsResult = Preferences_ContextCopy(Session_ReturnConfiguration(inPtr->selfRef),
														newContext.returnRef(), tagSet);
				if (kPreferences_ResultOK == prefsResult)
				{
					copyOK = true;
				}
				Preferences_ReleaseTagSet(&tagSet);
			}
			break;
		
		default:
			// ???
			break;
		}
		
		if (copyOK)
		{
			inPtr->sheetType = inSheetType;
			inPtr->recentSheetContext.setWithRetain(newContext.returnRef());
		}
		else
		{
			Console_Warning(Console_WriteLine, "failed to copy initial preferences into sheet context");
		}
	}
	
	result = inPtr->recentSheetContext.returnRef();
	
	return result;
}// sheetContextBegin


/*!
Destroys the temporary sheet preferences context, removing
the monitor on it, and clearing any flags that keep track of
active sheets.

(4.1)
*/
void
sheetContextEnd		(My_SessionPtr		inPtr)
{
	if (Preferences_ContextIsValid(inPtr->recentSheetContext.returnRef()))
	{
		inPtr->recentSheetContext.clear();
	}
	inPtr->sheetType = kMy_SessionSheetTypeNone;
}// sheetContextEnd


/*!
Displays the specified text temporarily in a floating window.
(For now, it is just a help tag, which serves the purpose but
is smaller and uglier than is desired...TEMPORARY.)

The given byte sequence MUST use UTF-8 encoding.

(4.0)
*/
void
terminalHoverLocalEchoString	(My_SessionPtr		inPtr,
								 UInt8 const*		inBytes,
								 size_t				inCount)
{
	// update the help tag and display it
	My_HMHelpContentRecWrap&	tagData = createHelpTagForLocalEcho();
	HIRect						globalCursorBounds;
	CFRetainRelease				stringObject(CFStringCreateWithBytes(kCFAllocatorDefault, inBytes, inCount,
																		kCFStringEncodingUTF8, false/* is external */), CFRetainRelease::kAlreadyRetained);
	
	
	tagData.rename(stringObject.returnCFStringRef(), nullptr/* alternate text */);
	TerminalView_GetCursorGlobalBounds(TerminalWindow_ReturnViewWithFocus
										(Session_ReturnActiveTerminalWindow(inPtr->selfRef)),
										globalCursorBounds);
	tagData.setFrame(globalCursorBounds);
	UNUSED_RETURN(OSStatus)HMDisplayTag(tagData.ptr());
	// this call does not immediately hide the tag, but rather after a short delay
	UNUSED_RETURN(OSStatus)HMHideTagWithOptions(kHMHideTagFade);
}// terminalHoverLocalEchoString


/*!
Displays the specified text immediately in the local terminal
of the session.

The given byte sequence MUST use the write encoding of the
session, as set by Session_SetSendDataEncoding().

(4.0)
*/
void
terminalInsertLocalEchoString	(My_SessionPtr		inPtr,
								 UInt8 const*		inBytes,
								 size_t				inCount)
{
	Session_TerminalWrite(inPtr->selfRef, inBytes, inCount);
}// terminalInsertLocalEchoString


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
touch the session window in any way.  If the warning
was also instructed to restart the session, then the
session respawns after it is killed.  The given alert
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
terminationWarningClose		(SessionRef&	inoutSessionRef,
							 Boolean		inKeepWindow,
							 Boolean		inRestartSession)
{
	if (Session_IsValid(inoutSessionRef))
	{
		Boolean		destroySession = false;
		
		
		if (nullptr != inoutSessionRef)
		{
			My_SessionAutoLocker	sessionPtr(gSessionPtrLocks(), inoutSessionRef);
			
			
			// clear the alert status attribute for the session
			changeStateAttributes(sessionPtr, 0/* attributes to set */,
									kSession_StateAttributeOpenDialog/* attributes to clear */);
			
			// some actions are only appropriate when the window is closing...
			if (false == inKeepWindow)
			{
				// implicitly update the toolbar visibility preference based
				// on the toolbar visibility of this closing window
				{
					HIWindowRef		carbonWindow = returnActiveLegacyCarbonWindow(sessionPtr);
					Boolean			toolbarHidden = false;
					
					
					if (nullptr != carbonWindow)
					{
						toolbarHidden = (false == IsWindowToolbarVisible(carbonWindow));
					}
					else
					{
						toolbarHidden = (false == [[returnActiveNSWindow(sessionPtr) toolbar] isVisible]);
					}
					
					UNUSED_RETURN(Preferences_Result)Preferences_SetData(kPreferences_TagHeadersCollapsed,
																			sizeof(toolbarHidden), &toolbarHidden);
				}
				
				// flag to destroy the session; this cannot occur within this block,
				// because the session pointer is still locked (also, it will not
				// happen for restarting sessions)
				destroySession = true;
			}
			
			// now it is legal to display a different warning sheet
			sessionPtr->currentDialog.assign(nullptr);
		}
		
		if (destroySession)
		{
			// this call will terminate the running process and
			// immediately close the associated window
			Session_Dispose(&inoutSessionRef);
		}
		else if (inKeepWindow)
		{
			My_SessionAutoLocker	ptr(gSessionPtrLocks(), inoutSessionRef);
			
			
			// kill the original process; normally this could potentially dispose of
			// the entire session and its window, so a flag is set to veto that
			ptr->restartInProgress = true;
			Session_SetState(inoutSessionRef, kSession_StateDead);
			ptr->restartInProgress = false;
			
			// if requested, restart the command afterwards
			if (inRestartSession)
			{
				// TEMPORARY; this is technically terminal-type-specific and just happens to
				// work to clear the screen and home the cursor in any supported emulator;
				// this should probably be more explicit
				Session_TerminalWriteCString(inoutSessionRef, "\033[H\033[J");
				
				// install a one-shot timer to rerun the command line after a short delay
				// (certain processes, such as shells, do not respawn correctly if the
				// respawn is attempted immediately after the previous process exits)
				UNUSED_RETURN(OSStatus)InstallEventLoopTimer
										(GetCurrentEventLoop(),
											200 * kEventDurationMillisecond/* delay before start; heuristic */,
											kEventDurationForever/* time between fires - this timer does not repeat */,
											ptr->respawnSessionTimerUPP, inoutSessionRef, &ptr->respawnSessionTimer);
			}
		}
	}
}// terminationWarningClose


/*!
Creates a new TEK graphics window and associates it with
the specified session.  TEK writes will subsequently
affect the new window.  Returns "true" only if the new
window was successfully created (if not, any existing
window remains the target).

(3.0)
*/
Boolean
vectorGraphicsCreateTarget		(My_SessionPtr	inPtr)
{
	Boolean		result = false;
	
	
	vectorGraphicsDetachTarget(inPtr);
	inPtr->vectorGraphicsInterpreter = VectorInterpreter_New(inPtr->vectorGraphicsCommandSet);
	if (nullptr != inPtr->vectorGraphicsInterpreter)
	{
		VectorWindow_Ref	newWindow = VectorWindow_New(inPtr->vectorGraphicsInterpreter);
		
		
		inPtr->vectorGraphicsWindows.insert(newWindow);
		VectorWindow_StartMonitoring(newWindow, kVectorWindow_EventWillClose,
										inPtr->vectorWindowListener.returnRef());
		VectorInterpreter_Release(&inPtr->vectorGraphicsInterpreter); // still retained by the window controller
		inPtr->vectorGraphicsInterpreter = VectorWindow_ReturnInterpreter(newWindow);
		
		VectorInterpreter_SetPageClears(inPtr->vectorGraphicsInterpreter,
										false == inPtr->vectorGraphicsPageOpensNewWindow);
		VectorCanvas_SetListeningSession(VectorInterpreter_ReturnCanvas(inPtr->vectorGraphicsInterpreter),
											inPtr->selfRef);
		{
			CFStringRef		windowTitleCFString = nullptr;
			
			
			if (noErr == CopyWindowTitleAsCFString(returnActiveLegacyCarbonWindow(inPtr), &windowTitleCFString))
			{
				VectorWindow_SetTitle(newWindow, windowTitleCFString);
				CFRelease(windowTitleCFString), windowTitleCFString = nullptr;
			}
		}
		Session_AddDataTarget(inPtr->selfRef, kSession_DataTargetTektronixGraphicsCanvas,
								inPtr->vectorGraphicsInterpreter);
		
		VectorWindow_Display(newWindow);
		TerminalWindow_Select(inPtr->terminalWindow);
		
		// display a help tag over the cursor in an unobtrusive location
		// that confirms for the user that a suspend has in fact occurred
		{
			My_HMHelpContentRecWrap&	tagData = createHelpTagForVectorGraphics();
			HIRect						globalCursorBounds;
			
			
			TerminalView_GetCursorGlobalBounds(TerminalWindow_ReturnViewWithFocus
												(Session_ReturnActiveTerminalWindow(inPtr->selfRef)),
												globalCursorBounds);
			tagData.setFrame(globalCursorBounds);
			UNUSED_RETURN(OSStatus)HMDisplayTag(tagData.ptr());
			// this call does not immediately hide the tag, but rather after a short delay
			UNUSED_RETURN(OSStatus)HMHideTagWithOptions(kHMHideTagFade);
		}
		
		result = true;
	}
	return result;
}// vectorGraphicsCreateTarget


/*!
Dissociates the current target graphic of the given session
from the session.  TEK writes will therefore no longer affect
this graphic.  If there is no target graphic, this routine
does nothing.

(3.0)
*/
void
vectorGraphicsDetachTarget	(My_SessionPtr	inPtr)
{
	if (nullptr != inPtr->vectorGraphicsInterpreter)
	{
		Session_RemoveDataTarget(inPtr->selfRef, kSession_DataTargetTektronixGraphicsCanvas,
									inPtr->vectorGraphicsInterpreter);
		inPtr->vectorGraphicsInterpreter = nullptr; // weak reference only, do not release
	}
}// vectorGraphicsDetachTarget


/*!
Invoked whenever a monitored vector graphics window’s state
changes.

(4.1)
*/
void
vectorGraphicsWindowChanged		(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
								 ListenerModel_Event	inVectorGraphicsWindowChange,
								 void*					inEventContextPtr,
								 void*					inListenerContextPtr)
{
	My_SessionPtr	ptr = REINTERPRET_CAST(inListenerContextPtr, My_SessionPtr);
	
	
	switch (inVectorGraphicsWindowChange)
	{
	case kVectorWindow_EventWillClose:
		{
			VectorWindow_Ref		vectorWindowRef = REINTERPRET_CAST(inEventContextPtr, VectorWindow_Ref);
			VectorInterpreter_Ref	windowInterpreter = VectorWindow_ReturnInterpreter(vectorWindowRef);
			auto					toWindow = ptr->vectorGraphicsWindows.find(vectorWindowRef);
			
			
			if (ptr->vectorGraphicsWindows.end() != toWindow)
			{
				VectorWindow_StopMonitoring(*toWindow, kVectorWindow_EventWillClose,
											ptr->vectorWindowListener.returnRef());
				VectorWindow_Release(&vectorWindowRef);
				ptr->vectorGraphicsWindows.erase(toWindow);
				if (windowInterpreter == ptr->vectorGraphicsInterpreter)
				{
					// restore terminal control if the current graphic was closed
					vectorGraphicsDetachTarget(ptr);
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// vectorGraphicsWindowChanged


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
}// watchClearForSession


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
	CFAbsoluteTime const	kNow = CFAbsoluteTimeGetCurrent();
	Boolean					canTrigger = (kSession_WatchForKeepAlive == inWhatTriggered)
											? true
											: ((SessionFactory_ReturnUserRecentSession() != inPtr->selfRef) ||
												FlagManager_Test(kFlagSuspended));
	
	
	// automatically ignore triggers that occur too close together
	if ((kNow - inPtr->watchTriggerAbsoluteTime) < 5.0/* arbitrary; in seconds */)
	{
		canTrigger = false;
	}
	
	if (canTrigger)
	{
		inPtr->watchTriggerAbsoluteTime = kNow;
		
		// note the change (e.g. can cause icon displays to be updated)
		changeStateAttributes(inPtr, kSession_StateAttributeNotification/* attributes to set */,
								0/* attributes to clear */);
		
		// create or update the global alert
		switch (inWhatTriggered)
		{
		case kSession_WatchForPassiveData:
		case kSession_WatchForInactivity:
			{
				CFStringRef			growlNotificationName = nullptr;
				CFStringRef			growlNotificationTitle = nullptr;
				CFStringRef			dialogTextCFString = nullptr;
				UIStrings_Result	stringResult = kUIStrings_ResultOK;
				
				
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
				
				// page the Mac OS X user notification center (and Growl, if
				// it is installed) and then clear immediately, instead of
				// waiting for the user to respond
				GrowlSupport_Notify(kGrowlSupport_NoteDisplayAlways,
									growlNotificationName, growlNotificationTitle,
									dialogTextCFString/* description */);
				watchClearForSession(inPtr);
				
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
				std::string			transmission = Quills::Session::keep_alive_transmission();
				CFRetainRelease		asObject(CFStringCreateWithCString(kCFAllocatorDefault, transmission.c_str(), kCFStringEncodingUTF8),
												CFRetainRelease::kAlreadyRetained);
				
				
				if (asObject.exists())
				{
					Session_UserInputCFString(inPtr->selfRef, asObject.returnCFStringRef());
				}
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
void
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
		UInt16				intValue = 0;
		
		
		prefsResult = Preferences_GetData(kPreferences_TagKeepAlivePeriodInMinutes, sizeof(intValue),
											&intValue);
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
		UInt16				intValue = 0;
		
		
		prefsResult = Preferences_GetData(kPreferences_TagIdleAfterInactivityInSeconds, sizeof(intValue),
											&intValue);
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
			Boolean					isCarbon = TerminalWindow_IsLegacyCarbon(ptr->terminalWindow);
			
			
			// install a callback that disposes of the window properly when it should be closed
			if (isCarbon)
			{
				EventTypeSpec const		whenWindowClosing[] =
										{
											{ kEventClassWindow, kEventWindowClose }
										};
				OSStatus				error = noErr;
				
				
				ptr->windowClosingUPP = NewEventHandlerUPP(receiveWindowClosing);
				error = InstallWindowEventHandler(TerminalWindow_ReturnLegacyCarbonWindow(ptr->terminalWindow),
													ptr->windowClosingUPP, GetEventTypeCount(whenWindowClosing),
													whenWindowClosing, session/* user data */,
													&ptr->windowClosingHandler/* event handler reference */);
				assert(error == noErr);
			}
			else
			{
				// NOTE: for Cocoa, this is implemented in TerminalWindow_Controller ("windowShouldClose:")
			}
			
			// install a callback that clears notifications when the window is activated
			if (isCarbon)
			{
				EventTypeSpec const		whenWindowFocusChanged[] =
										{
											{ kEventClassWindow, kEventWindowFocusAcquired }
										};
				OSStatus				error = noErr;
				
				
				ptr->windowFocusChangeUPP = NewEventHandlerUPP(receiveWindowFocusChange);
				error = InstallWindowEventHandler(TerminalWindow_ReturnLegacyCarbonWindow(ptr->terminalWindow),
													ptr->windowFocusChangeUPP, GetEventTypeCount(whenWindowFocusChanged),
													whenWindowFocusChanged, session/* user data */,
													&ptr->windowFocusChangeHandler/* event handler reference */);
				assert(error == noErr);
			}
			else
			{
				Console_Warning(Console_WriteLine, "session window focus-handler not implemented for Cocoa");
			}
			
			ptr->terminalWindowListener.setWithNoRetain(ListenerModel_NewStandardListener
														(terminalWindowChanged, session/* context */));
			TerminalWindow_StartMonitoring(ptr->terminalWindow, kTerminalWindow_ChangeScreenDimensions,
											ptr->terminalWindowListener.returnRef());
			TerminalWindow_StartMonitoring(ptr->terminalWindow, kTerminalWindow_ChangeObscuredState,
											ptr->terminalWindowListener.returnRef());
			{
				// install a listener for keystrokes on each view’s control;
				// in the future, terminal windows may have multiple views,
				// which can be focused independently
				UInt16				viewCount = TerminalWindow_ReturnViewCount(ptr->terminalWindow);
				TerminalViewRef*	viewArray = REINTERPRET_CAST(Memory_NewPtr(viewCount * sizeof(TerminalViewRef)),
																	TerminalViewRef*);
				
				
				if (viewArray != nullptr)
				{
					HIViewRef		userFocusView = nullptr;
					HIViewRef		dragFocusView = nullptr;
					SInt16			i = 0;
					OSStatus		error = noErr;
					
					
					ptr->terminalViewDragDropUPP = NewEventHandlerUPP(receiveTerminalViewDragDrop);
					assert(nullptr != ptr->terminalViewDragDropUPP);
					ptr->terminalViewEnteredUPP = NewEventHandlerUPP(receiveTerminalViewEntered);
					assert(nullptr != ptr->terminalViewEnteredUPP);
					ptr->terminalViewTextInputUPP = NewEventHandlerUPP(receiveTerminalViewTextInput);
					assert(nullptr != ptr->terminalViewTextInputUPP);
					TerminalWindow_GetViews(ptr->terminalWindow, viewCount, viewArray, &viewCount/* actual length */);
					for (i = 0; i < viewCount; ++i)
					{
						userFocusView = TerminalView_ReturnUserFocusHIView(viewArray[i]);
						dragFocusView = TerminalView_ReturnDragFocusHIView(viewArray[i]);
						
						// install a callback that responds to drag-and-drop in views,
						// and a callback that responds to key presses in views
						if (isCarbon)
						{
							EventTypeSpec const		whenTerminalViewTextInput[] =
													{
														{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent }
													};
							EventTypeSpec const		whenTerminalViewEntered[] =
													{
														{ kEventClassControl, kEventControlTrackingAreaEntered }
													};
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
							
							// ensure drags to this view are seen
							error = HIViewInstallEventHandler(dragFocusView, ptr->terminalViewDragDropUPP,
																GetEventTypeCount(whenTerminalViewDragDrop),
																whenTerminalViewDragDrop, session/* user data */,
																&ptr->terminalViewDragDropHandlers[dragFocusView]);
							assert_noerr(error);
							error = SetControlDragTrackingEnabled(dragFocusView, true/* is drag enabled */);
							assert_noerr(error);
						}
						else
						{
							Console_Warning(Console_WriteLine, "session window drag-handler and key-handler not implemented for Cocoa");
						}
						
						// enable drag tracking for the window, if it is not enabled already
						// (not necessary for Cocoa)
						if (isCarbon)
						{
							error = SetAutomaticControlDragTrackingEnabledForWindow
									(GetControlOwner(dragFocusView), true/* is drag enabled */);
							assert_noerr(error);
						}
					}
					Memory_DisposePtr(REINTERPRET_CAST(&viewArray, Ptr*));
				}
			}
			
			// check the “automatically capture to file” preference; if set,
			// begin a capture according to the user’s specifications
			if (ptr->autoCaptureToFile)
			{
				Boolean		captureOK = autoCaptureSessionToFile(ptr);
				
				
				if (false == captureOK)
				{
					Console_Warning(Console_WriteLine, "failed to initiate automatic capture to file");
					Sound_StandardAlert();
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
											ptr->terminalWindowListener.returnRef());
			TerminalWindow_StopMonitoring(ptr->terminalWindow, kTerminalWindow_ChangeObscuredState,
											ptr->terminalWindowListener.returnRef());
			{
				// remove listener from each view where one was installed
				UInt16				viewCount = TerminalWindow_ReturnViewCount(ptr->terminalWindow);
				TerminalViewRef*	viewArray = REINTERPRET_CAST(Memory_NewPtr(viewCount * sizeof(TerminalViewRef)),
																	TerminalViewRef*);
				
				
				if (viewArray != nullptr)
				{
					HIViewRef		userFocusView = nullptr;
					HIViewRef		dragFocusView = nullptr;
					SInt16			i = 0;
					
					
					TerminalWindow_GetViews(ptr->terminalWindow, viewCount, viewArray, &viewCount/* actual length */);
					for (i = 0; i < viewCount; ++i)
					{
						userFocusView = TerminalView_ReturnUserFocusHIView(viewArray[i]);
						dragFocusView = TerminalView_ReturnDragFocusHIView(viewArray[i]);
						RemoveEventHandler(ptr->terminalViewTextInputHandlers[userFocusView]);
						RemoveEventHandler(ptr->terminalViewDragDropHandlers[dragFocusView]);
						RemoveEventHandler(ptr->terminalViewEnteredHandlers[dragFocusView]);
					}
					DisposeEventHandlerUPP(ptr->terminalViewDragDropUPP), ptr->terminalViewDragDropUPP = nullptr;
					DisposeEventHandlerUPP(ptr->terminalViewEnteredUPP), ptr->terminalViewEnteredUPP = nullptr;
					DisposeEventHandlerUPP(ptr->terminalViewTextInputUPP), ptr->terminalViewTextInputUPP = nullptr;
					Memory_DisposePtr(REINTERPRET_CAST(&viewArray, Ptr*));
				}
			}
		}
		break;
	
	default:
		// ???
		break;
	}
}// windowValidationStateChanged

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
