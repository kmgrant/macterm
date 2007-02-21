/*!	\file Session.h
	\brief Highest level of abstraction for local or remote
	shells.
	
	Manages sessions, which are the user interface components
	surrounding client connections to servers.  Sessions can
	target terminal windows, Tektronix vector graphics canvases,
	or interactive color raster graphics screens.
	
	In MacTelnet 3.0, the implementation of a session is opaque;
	data previously accessible through public data structures
	must now be managed via Session objects.  This approach has
	proved particularly useful when dealing with local Unix
	shells, because it is now possible to treat local and remote
	shells the same at some level, even though they are
	completely different underneath.
*/
/*###############################################################

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

#ifndef __SESSION__
#define __SESSION__

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <ListenerModel.h>
#include <ResultCode.template.h>

// MacTelnet includes
#include "ConnectionData.h"
#include "ConstantsRegistry.h"
#include "SessionDescription.h"



#pragma mark Constants

/*!
Possible return values from Session module routines.
*/
#ifndef REZ
typedef ResultCode< UInt16 >	Session_ResultCode;
Session_ResultCode const	kSession_ResultCodeSuccess(0);					//!< no error
Session_ResultCode const	kSession_ResultCodeInvalidReference(1);			//!< given SessionRef is not valid
Session_ResultCode const	kSession_ResultCodeProtocolMismatch(2);			//!< attempt to, say, set an FTP option on a telnet session
Session_ResultCode const	kSession_ResultCodeParameterError(3);			//!< invalid input (e.g. a null pointer)
Session_ResultCode const	kSession_ResultCodeInsufficientBufferSpace(4);	//!< not enough memory space provided to copy data
#endif

/*!
Setting changes that MacTelnet allows other modules to ÒlistenÓ for,
via Session_StartMonitoring().
*/
typedef ListenerModel_Event		Session_Change;
enum
{
	// IMPORTANT:	IF YOU MODIFY THIS LIST, look for uses of "kSession_AllChanges" in Session.cp to ensure
	//				your new type is handled along with other session changes!
	
	kSession_AllChanges					= FOUR_CHAR_CODE('****'),	//!< wildcard to indicate all events (context:
																	//!  varies)
	
	kSession_ChangeCloseWarningAnswered	= FOUR_CHAR_CODE('!Btn'),	//!< a Òsave changes before closingÓ warning message
																	//!  was open, and the user finally responded by
																	//!  clicking a button (context:
																	//!  SessionCloseWarningButtonInfoPtr)
	
	kSession_ChangeDataArrived			= FOUR_CHAR_CODE('Data'),	//!< new data has been inserted for processing via a
																	//!  call to Session_AppendDataForProcessing(); the
																	//!  usual response is to invoke Session_ProcessMoreData()
																	//!  (context: SessionRef)
	
	kSession_ChangeResourceLocation		= FOUR_CHAR_CODE('SURL'),	//!< the URL of a monitored Session has been updated
																	//!  (context: SessionRef)
	
	kSession_ChangeSelected				= FOUR_CHAR_CODE('Slct'),	//!< the user has selected the specified session; so, the
																	//!  associated terminal window should come to the front
																	//!  (context: SessionRef)
	
	kSession_ChangeState				= FOUR_CHAR_CODE('Stat'),	//!< the Session_State of a monitored Session has
																	//!  changed; various Session_StateIs...() APIs can
																	//!  be used to get the new state (context: SessionRef)
	
	kSession_ChangeStateAttributes		= FOUR_CHAR_CODE('SAtt'),	//!< the Session_StateAttributes of a monitored Session
																	//!  have changed; use Session_ReturnStateAttributes()
																	//!  to test attributes (context: SessionRef)
	
	kSession_ChangeWindowInvalid		= FOUR_CHAR_CODE('WDie'),	//!< the terminal window of a monitored Session is
																	//!  *about to be* destroyed, and therefore is now
																	//!  invalid (context: SessionRef)
	
	kSession_ChangeWindowObscured		= FOUR_CHAR_CODE('Obsc'),	//!< the terminal window of a monitored Session has
																	//!  been hidden or redisplayed; use the routine
																	//!  TerminalWindow_IsObscured() to find the new state
																	//!  (context: SessionRef)
	
	kSession_ChangeWindowTitle			= FOUR_CHAR_CODE('WTtl'),	//!< the title of the terminal window of a monitored
																	//!  Session has been updated (context: SessionRef)
	
	kSession_ChangeWindowValid			= FOUR_CHAR_CODE('WNew')	//!< the terminal window of a monitored Session has
																	//!  been created and therefore is now valid
																	//!  (context: SessionRef)
};

/*!
A data target specifies an external object type known
to Session objects, through which data can be routed.
A session is allowed to contain more than one target
for its output data: as such, it is trivial to support
features like multiple terminal screens, capture files
and TEK graphics windows because each type of object
knows how to interpret session data appropriately.

A Session object knows which targets are compatible
with one another, and will automatically disable all
incompatible targets when you add a new target.  For
example, it is OK to have many terminals and capture
files open, but if you route data to a graphics screen
all attached terminals and files are disabled because
they will garble graphics data.

The following algorithm is used:
- DUMB terminals are expected to render raw streams of
  data and are therefore considered compatible with
  everything, and can never be disabled
- TEK canvases are considered incompatible with all
  terminals or ICR graphics screens while attached,
  and therefore will disable terminals and ICR screens
- ICR screens are considered incompatible with all
  terminals, but are lower precedence than TEK windows
  so TEK windows cannot be disabled when new ICR
  screens are attached (rather, the ICR screens will
  be attached and disabled)
- terminals are considered compatible with capture
  files and print jobs; capture files and print jobs
  will therefore be disabled whenever terminals are
  disabled, and attaching new capture files or print
  jobs cannot cause anything else to be disabled
  (rather, new capture files or print jobs may be
  disabled automatically if, say, a graphics window is
  already attached)
*/
enum Session_DataTarget
{
	kSession_DataTargetStandardTerminal = 1,						//!< data goes to a VT (data: TerminalScreenRef)
	kSession_DataTargetTektronixGraphicsCanvas = 2,					//!< data goes to a TEK window  (data: SInt16*,
																	//!  the TEK window ID)
	kSession_DataTargetDumbTerminal = 3,							//!< data goes to a DUMB terminal (data:
																	//!  TerminalScreenRef)	
	kSession_DataTargetInteractiveColorRasterGraphicsScreen = 4,	//!< data goes to an ICR window (data: SInt16*,
																	//!  the ICR window ID)
	kSession_DataTargetOpenCaptureFile = 5,							//!< data goes to a capture file (data:
																	//!  TerminalScreenRef that has had a capture
																	//!  begun on it)
	kSession_DataTargetOpenPrinterSpool = 6							//!< data goes to a running print job (data:
																	//!  TerminalScreenRef that has had a printer
																	//!  spool begun on it)
};

/*!
Whether or not data is copied to the local terminal in
addition to being sent to a SessionÕs data targets.
*/
enum Session_Echo
{
	kSession_EchoDisabled = 0,				//!< echo is "false"
	kSession_EchoEnabled = 1,				//!< echo is "true"
	kSession_EchoCurrentSessionValue = 2	//!< echo "true" or "false", depending on current session value
};
typedef enum Session_Echo	Session_Echo;

/*!
Possible mappings to simulate a meta key on a Mac keyboard
(useful for the EMACS text editor).
*/
enum Session_EMACSMetaKey
{
	kSession_EMACSMetaKeyOff = 0,				//!< no mapping
	kSession_EMACSMetaKeyControlCommand = 1,	//!< by holding down control and command keys, meta is simulated
	kSession_EMACSMetaKeyOption = 2				//!< by holding down option key, meta is simulated
};

/*!
Which characters will be sent when a new-line is requested.
*/
enum Session_NewlineMode
{
	kSession_NewlineModeMapCRNormal	= 0,	//!< a Return key sends Òcarriage returnÓ only
	kSession_NewlineModeMapCRNull	= 1,	//!< Berkeley 4.3; newline means Òcarriage return, nullÓ
	kSession_NewlineModeMapCRLF		= 2		//!< newline means Òcarriage return, line feedÓ
};

/*!
A tag used to store and look up arbitrary data attached to a Session.
*/
typedef FourCharCode Session_PropertyKey;

/*!
Protocols supported by a Session.
*/
enum Session_Protocol
{
	kSession_ProtocolFTP = (1 << 0),	//!< file transfer protocol
	kSession_ProtocolSFTP = (1 << 1),	//!< secure file transfer protocol
	kSession_ProtocolSSH1 = (1 << 2),	//!< secure shell protocol, version 1
	kSession_ProtocolSSH2 = (1 << 3),	//!< secure shell protocol, version 2
	kSession_ProtocolTelnet = (1 << 4)	//!< telephone networking protocol
};

/*!
Possible states a Session can be in.

Note that "kSession_StateActiveUnstable" and
"kSession_StateActiveStable" are under evaluation;
the introduction of state attributes, below, may
mean that it is better to have a single active
state and attributes for stability applied...
*/
enum Session_State
{
	kSession_StateBrandNew = 0,			//!< should ALWAYS be the first state a session is in; session MIGHT NOT be initialized!
	kSession_StateInitialized = 1,		//!< session has had all necessary attributes set up, and can be used
	kSession_StateActiveUnstable = 2,	//!< if remote, a connection has been made; if local, a process is running;
										//!  after a short period of time, this state changes to
										//!  "kSession_StateActiveStable"
	kSession_StateActiveStable = 3,		//!< equivalent to active, but "kSession_LifetimeMinimumForNoWarningClose"
										//!  duration has now elapsed, indicating a stable connection or process
	kSession_StateDead = 4,				//!< session terminated; however, the terminal window may still be open
	kSession_StateImminentDisposal = -1	//!< should ALWAYS be the last state a session is in
};

/*!
Sometimes, session states have ÒattributesÓ: these
tags act like real states, but cannot displace any
real state.  For example, ÒrunningÓ is a real state
and many of these attributes apply to the running
state; it would be inappropriate to imply that a
session were not still ÒrunningÓ while any of these
attributes was in effect.
*/
typedef UInt32 Session_StateAttributes;
enum
{
	kSession_StateAttributeOpenDialog		= (1 << 0),	//!< an alert element (typically a sheet) is currently applicable to the session
	kSession_StateAttributeSuspendNetwork	= (1 << 1)	//!< a Scroll Lock (XOFF) was initiated, so data has stopped transmitting
};

/*!
Data specific to a telnet option that is stored on a Session.
Note that "Session_PropertyKey" is probably more appropriate.
*/
enum Session_TelnetOptionStateType
{
	kSession_TelnetOptionStateTypeFlowControl	= FOUR_CHAR_CODE('flow'),	//!< data: struct FlowControlState*
	kSession_TelnetOptionStateTypeLineMode		= FOUR_CHAR_CODE('linm')	//!< data: LineModeStatePtr
};

enum
{
	// miscellaneous constants
	kSession_LifetimeMinimumForNoWarningClose = 15	//!< in seconds; if a session has been alive less than this length
													//!  of time, it can be killed without having to OK the (annoying)
													//!  warning message
};



#pragma mark Types

#include "SessionRef.typedef.h"

/*!
Information passed to a handler when the user has
chosen an option from the Close sheet.
*/
struct SessionCloseWarningButtonInfo
{
	SessionRef		session;			//!< which sessionÕs close alert was answered by the user
	SInt16			buttonHit;			//!< "kAlertStdAlert...Button" constant for the button selected by the user
};
typedef SessionCloseWarningButtonInfo*	SessionCloseWarningButtonInfoPtr;



#pragma mark Public Methods

//!\name Creating and Destroying Sessions
//@{

// DO NOT CREATE SESSIONS THIS WAY; THIS IS A TRANSITIONAL ROUTINE (USE SessionFactory METHODS, INSTEAD)
SessionRef
	Session_New								(Boolean							inIsReadOnly = false);

void
	Session_Dispose							(SessionRef*						inoutRefPtr);

//@}

//!\name User Interaction
//@{

void
	Session_DisplayFileCaptureSaveDialog	(SessionRef							inRef);

void
	Session_DisplayPrintJobDialog			(SessionRef							inRef);

void
	Session_DisplayPrintPageSetupDialog		(SessionRef							inRef);

void
	Session_DisplaySaveDialog				(SessionRef							inRef);

void
	Session_DisplaySpecialKeySequencesDialog(SessionRef							inRef);

void
	Session_DisplayTerminationWarning		(SessionRef							inRef,
											 Boolean							inForceModalDialog = false);

void
	Session_FlushUserInputBuffer			(SessionRef							inRef);

Boolean
	Session_IsReadOnly						(SessionRef							inRef);

Session_ResultCode
	Session_PostDataArrivedEventToMainQueue	(SessionRef							inRef,
											 void*								inBuffer,
											 UInt32								inNumberOfBytesToProcess,
											 EventPriority						inPriority,
											 EventQueueRef						inDispatcherQueue);

Session_ResultCode
	Session_Select							(SessionRef							inRef);

void
	Session_UserInputCFString				(SessionRef							inRef,
											 CFStringRef						inStringBuffer,
											 Boolean							inSendToRecordingScripts = true);

void
	Session_UserInputInterruptProcess		(SessionRef							inRef,
											 Boolean							inSendToRecordingScripts = true);

Session_ResultCode
	Session_UserInputKey					(SessionRef							inRef,
											 UInt8								inKeyOrASCII);

void
	Session_UserInputQueueCharacter			(SessionRef							inRef,
											 char								inCharacter);

void
	Session_UserInputString					(SessionRef							inRef,
											 char const*						inStringBuffer,
											 size_t								inStringBufferSize,
											 Boolean							inSendToRecordingScripts = true);

//@}

//!\name Write-Targeting Routines
//@{

Session_ResultCode
	Session_AddDataTarget					(SessionRef							inRef,
											 Session_DataTarget					inTarget,
											 void*								inTargetData);

Session_ResultCode
	Session_RemoveDataTarget				(SessionRef							inRef,
											 Session_DataTarget					inTarget,
											 void*								inTargetData);

//@}

//!\name Custom Data Tags on Sessions
//@{

void
	Session_PropertyAdd						(SessionRef							inRef,
											 Session_PropertyKey				inAuxiliaryDataTag,
											 void*								inAuxiliaryData);

void
	Session_PropertyLookUp					(SessionRef							inRef,
											 Session_PropertyKey				inAuxiliaryDataTag,
											 void**								outAuxiliaryDataPtr);

void*
	Session_PropertyRemove					(SessionRef							inRef,
											 Session_PropertyKey				inAuxiliaryDataTag);

//@}

//!\name Tektronix Vector Graphics Routines
//@{

Boolean
	Session_TEKCreateTargetGraphic			(SessionRef							inRef);

void
	Session_TEKDetachTargetGraphic			(SessionRef							inRef);

TektronixMode
	Session_TEKGetMode						(SessionRef							inRef);

Boolean
	Session_TEKHasTargetGraphic				(SessionRef							inRef);

Boolean
	Session_TEKIsEnabled					(SessionRef							inRef);

Boolean
	Session_TEKPageCommandOpensNewWindow	(SessionRef							inRef);

SInt16
	Session_TEKWrite						(SessionRef							inRef,
											 char const*						inBuffer,
											 SInt16								inLength);

//@}

//!\name Virtual Terminal Routines
//@{

Session_ResultCode
	Session_TerminalGetAnswerBackMessage	(SessionRef							inRef,
											 char*								outAnswerBackBufferPtr,
											 size_t								inAnswerBackBufferSize);

void
	Session_TerminalWrite					(SessionRef							inRef,
											 UInt8 const*						inBuffer,
											 UInt32								inLength);

void
	Session_TerminalWriteCString			(SessionRef							inRef,
											 char const*						inCString);

void
	Session_TerminalWriteCFString			(SessionRef							inRef,
											 CFStringRef						inCFString);

//@}

//!\name Miscellaneous
//@{

size_t
	Session_AppendDataForProcessing			(SessionRef							inRef,
											 UInt8 const*						inDataPtr,
											 size_t								inSize);

void
	Session_Disconnect						(SessionRef							inRef);

void
	Session_FlushNetwork					(SessionRef							inRef);

Boolean
	Session_PageKeysControlTerminalView		(SessionRef							inRef);

Boolean
	Session_ProcessesAll8Bits				(SessionRef							inRef);

size_t
	Session_ProcessMoreData					(SessionRef							inRef);

Session_ResultCode
	Session_ReceiveData						(SessionRef							inRef,
											 void const*						inBufferPtr,
											 size_t								inByteCount);

SInt16
	Session_SendData						(SessionRef							inRef,
											 void const*						inBufferPtr,
											 size_t								inByteCount);

SInt16
	Session_SendData						(SessionRef							inRef,
											 CFStringRef						inBuffer);

SInt16
	Session_SendFlush						(SessionRef							inRef);

SInt16
	Session_SendFlushData					(SessionRef							inRef,
											 void const*						inBufferPtr,
											 SInt16								inByteCount);

void
	Session_SendNewline						(SessionRef							inRef,
											 Session_Echo						inEcho);

// REMOTE SESSIONS ONLY
void
	Session_SetActivityNotificationEnabled	(SessionRef							inRef,
											 Boolean							inIsEnabled);

Session_ResultCode
	Session_SetDataProcessingCapacity		(SessionRef							inRef,
											 size_t								inBlockSizeInBytes);

void
	Session_SetLocalEchoEnabled				(SessionRef							inRef,
											 Boolean							inIsEnabled);

void
	Session_SetLocalEchoFullDuplex			(SessionRef							inRef);

void
	Session_SetLocalEchoHalfDuplex			(SessionRef							inRef);

void
	Session_SetNetworkSuspended				(SessionRef							inRef,
											 Boolean							inScrollLock);

void
	Session_SetNewlineMode					(SessionRef							inRef,
											 Session_NewlineMode				inNewlineMode);

void
	Session_SetSpeechEnabled				(SessionRef							inRef,
											 Boolean							inIsEnabled);

// AFFECTS RETURN VALUES OF THE Session_StateIs...() METHODS
void
	Session_SetState						(SessionRef							inRef,
											 Session_State						inNewState);

void
	Session_SetWindowUserDefinedTitle		(SessionRef							inRef,
											 CFStringRef						inWindowName);

void
	Session_SpeechPause						(SessionRef							inRef);

void
	Session_SpeechResume					(SessionRef							inRef);

Session_ResultCode
	Session_UpdatePasteState				(SessionRef							inRef,
											 SessionPasteStateConstPtr			inPasteStatePtr);

//@}

//!\name Information on Sessions
//@{

// REMOTE SESSIONS ONLY
Boolean
	Session_ActivityNotificationIsEnabled	(SessionRef							inRef);

Session_ResultCode
	Session_CopyStateIconRef				(SessionRef							inRef,
											 IconRef&							outCopiedIcon);

Session_ResultCode
	Session_FillInSessionDescription		(SessionRef							inRef,
											 SessionDescription_Ref*			outNewSaveFileMemoryModelPtr);

// API UNDER EVALUATION
Session_ResultCode
	Session_GetPasteState					(SessionRef							inRef,
											 SessionPasteStatePtr				outPasteStatePtr);

// API UNDER EVALUATION
SessionPasteStateConstPtr
	Session_GetPasteStateReadOnly			(SessionRef							inRef);

CFStringRef
	Session_GetResourceLocationCFString		(SessionRef							inRef);

Session_State
	Session_GetState						(SessionRef							inRef);

Session_StateAttributes
	Session_ReturnStateAttributes			(SessionRef							inRef);

Session_ResultCode
	Session_GetStateString					(SessionRef							inRef,
											 CFStringRef&						outUncopiedString);

Session_ResultCode
	Session_GetWindowUserDefinedTitle		(SessionRef							inRef,
											 CFStringRef&						outUncopiedString);

Boolean
	Session_LocalEchoIsEnabled				(SessionRef							inRef);

Boolean
	Session_LocalEchoIsFullDuplex			(SessionRef							inRef);

Boolean
	Session_LocalEchoIsHalfDuplex			(SessionRef							inRef);

Boolean
	Session_NetworkIsSuspended				(SessionRef							inRef);

TerminalWindowRef
	Session_ReturnActiveTerminalWindow		(SessionRef							inRef);

HIWindowRef
	Session_ReturnActiveWindow				(SessionRef							inRef);

void
	Session_SetResourceLocationCFString		(SessionRef							inRef,
											 CFStringRef						inResourceLocationString);

Boolean
	Session_SpeechIsEnabled					(SessionRef							inRef);

void
	Session_StartMonitoring					(SessionRef							inRef,
											 Session_Change						inForWhatChange,
											 ListenerModel_ListenerRef			inListener);

Boolean
	Session_StateIsActive					(SessionRef							inRef);

Boolean
	Session_StateIsActiveStable				(SessionRef							inRef);

Boolean
	Session_StateIsActiveUnstable			(SessionRef							inRef);

Boolean
	Session_StateIsBrandNew					(SessionRef							inRef);

Boolean
	Session_StateIsDead						(SessionRef							inRef);

Boolean
	Session_StateIsImminentDisposal			(SessionRef							inRef);

Boolean
	Session_StateIsInitialized				(SessionRef							inRef);

void
	Session_StopMonitoring					(SessionRef							inRef,
											 Session_Change						inForWhatChange,
											 ListenerModel_ListenerRef			inListener);

UInt32
	Session_TimeOfActivation				(SessionRef							inRef);

CFAbsoluteTime
	Session_TimeOfTermination				(SessionRef							inRef);

Boolean
	Session_TypeIsLocalLoginShell			(SessionRef							inRef);

Boolean
	Session_TypeIsLocalNonLoginShell		(SessionRef							inRef);

/*###############################################################
	SESSION ACCESSORS
	! ! ! TEMPORARY ! ! !
	The Session_Property...() APIs will eventually be used to
	associate more abstract data with sessions, to the point
	of storing pointers to data structures defined entirely
	within particular code modules.  The following APIs exist
	only to aide the transition to the SessionRef-based APIs,
	and WILL DEFINITELY DISAPPEAR IN THE FUTURE.
###############################################################*/

ConnectionDataPtr
	Session_ConnectionDataPtr				(SessionRef							inRef);

char*
	Session_kbbuf							(SessionRef							inRef);

SInt16*
	Session_kblen							(SessionRef							inRef);

UInt8*
	Session_parsedat						(SessionRef							inRef);

size_t
	Session_parsedat_size					(SessionRef							inRef);

SInt16*
	Session_parseIndex						(SessionRef							inRef);

void
	Session_SetTerminalWindow				(SessionRef							inRef,
											 TerminalWindowRef					inTerminalWindow);

//@}

//!\name Utilities
//@{

Boolean
	Session_ExtractPortNumberFromHostString	(StringPtr							inoutString,
											 SInt16*							outPortNumberPtr);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
