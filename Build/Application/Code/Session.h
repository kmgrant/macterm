/*!	\file Session.h
	\brief Highest level of abstraction for local or remote
	shells.
	
	Manages sessions, which are the user interface components
	surrounding connections to pseudo-terminal devices that
	are running Unix processes.  Sessions can target different
	virtual devices, such as terminal screens or vector graphics
	canvases.
	
	The implementation of a session is opaque; data must be
	managed via Session objects.
*/
/*###############################################################

	MacTerm
		© 1998-2022 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// Mac includes
#ifdef __OBJC__
@class NSImage;
@class NSPasteboard;
@class NSWindow;
#else
class NSImage;
class NSPasteboard;
class NSWindow;
#endif
#include <CoreServices/CoreServices.h>

// library includes
#include <ListenerModel.h>
#include <ResultCode.template.h>

// application includes
#include <MacTermQuills/MacTermQuills.h> // for Session_FunctionKeyLayout and other enums used by SwiftUI
#include "ConstantsRegistry.h"
#include "Local.h"
#include "TerminalWindow.h"



#pragma mark Constants

/*!
Possible return values from Session module routines.
*/
typedef ResultCode< UInt16 >	Session_Result;
Session_Result const	kSession_ResultOK(0);						//!< no error
Session_Result const	kSession_ResultInvalidReference(1);			//!< given SessionRef is not valid
Session_Result const	kSession_ResultParameterError(2);			//!< invalid input (e.g. a null pointer)
Session_Result const	kSession_ResultInsufficientBufferSpace(3);	//!< not enough memory space provided to copy data
Session_Result const	kSession_ResultNotReady(4);					//!< session is not in a state that can accept this action right now

/*!
Setting changes that MacTerm allows other modules to “listen” for,
via Session_StartMonitoring().

See also similar monitoring APIs at different levels: Terminal,
Terminal View, Terminal Window and Session Factory.
*/
typedef ListenerModel_Event		Session_Change;
enum
{
	// IMPORTANT:	IF YOU MODIFY THIS LIST, look for uses of "kSession_AllChanges" in Session.cp to ensure
	//				your new type is handled along with other session changes!
	
	kSession_AllChanges					= '****',	//!< wildcard to indicate all events (context:
													//!  varies)
	
	kSession_ChangeResourceLocation		= 'SURL',	//!< the URL of a monitored Session has been updated
													//!  (context: SessionRef)
	
	kSession_ChangeSelected				= 'Slct',	//!< the user has selected the specified session; so, the
													//!  associated terminal window should come to the front
													//!  (context: SessionRef)
	
	kSession_ChangeState				= 'Stat',	//!< the Session_State of a monitored Session has
													//!  changed; various Session_StateIs...() APIs can
													//!  be used to get the new state (context: SessionRef)
	
	kSession_ChangeStateAttributes		= 'SAtt',	//!< the Session_StateAttributes of a monitored Session
													//!  have changed; use Session_ReturnStateAttributes()
													//!  to test attributes (context: SessionRef)
	
	kSession_ChangeWatch				= 'Wtch',	//!< the Session_Watch of a monitored Session has
													//!  changed; various Session_WatchIs...() APIs can
													//!  be used to get the new value (context: SessionRef)
	
	kSession_ChangeWindowInvalid		= 'WDie',	//!< the terminal window of a monitored Session is
													//!  *about to be* destroyed, and therefore is now
													//!  invalid (context: SessionRef)
	
	kSession_ChangeWindowObscured		= 'Obsc',	//!< the terminal window of a monitored Session has
													//!  been hidden or redisplayed; use the routine
													//!  TerminalWindow_IsObscured() to find the new state
													//!  (context: SessionRef)
	
	kSession_ChangeWindowTitle			= 'WTtl',	//!< the title of the terminal window of a monitored
													//!  Session has been updated (context: SessionRef)
	
	kSession_ChangeWindowValid			= 'WNew'	//!< the terminal window of a monitored Session has
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
incompatible targets when you add a new target.

The following algorithm is used:
- DUMB terminals are expected to render raw streams of
  data and are therefore considered compatible with
  everything, and can never be disabled
- TEK canvases are considered incompatible with all
  terminals while attached so they take precedence
  over “standard” terminals until detached
*/
enum Session_DataTarget
{
	kSession_DataTargetStandardTerminal = 1,			//!< data goes to a VT (data: TerminalScreenRef)
	kSession_DataTargetTektronixGraphicsCanvas = 2,		//!< data goes to a TEK window  (data: VectorInterpreter_Ref)
	kSession_DataTargetDumbTerminal = 3					//!< data goes to a DUMB terminal (data: TerminalScreenRef)
};

/*!
Whether or not data is copied to the local terminal in
addition to being sent to a Session’s data targets.
*/
enum Session_Echo
{
	kSession_EchoDisabled = 0,				//!< echo is "false"
	kSession_EchoEnabled = 1,				//!< echo is "true"
	kSession_EchoCurrentSessionValue = 2	//!< echo "true" or "false", depending on current session value
};
typedef enum Session_Echo	Session_Echo;

// NOTE: Session_EmacsMetaKey is declared in "MacTermQuills.h" since it is used by SwiftUI

// NOTE: Session_FunctionKeyLayout is declared in "MacTermQuills.h" since it is used by SwiftUI

/*!
Which characters are used for line endings in text files
(such as file captures and saved selections).
*/
enum Session_LineEnding
{
	kSession_LineEndingCR			= 0,	//!< Macintosh style, carriage-return character
	kSession_LineEndingLF			= 1,	//!< Unix style, line-feed character
	kSession_LineEndingCRLF			= 2		//!< PC style, carriage-return and line-feed characters
};

// NOTE: Session_NewlineMode is declared in "MacTermQuills.h" since it is used by SwiftUI

// NOTE: Session_Protocol is declared in "MacTermQuills.h" since it is used by SwiftUI

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
	kSession_StateDead = 4,				//!< session terminated (however, the terminal window may still be open)
	kSession_StateImminentDisposal = -1	//!< should ALWAYS be the last state a session is in
};

/*!
Sometimes, session states have “attributes”: these
tags act like real states, but cannot displace any
real state.  For example, “running” is a real state
and many of these attributes apply to the running
state; it would be inappropriate to imply that a
session were not still “running” while any of these
attributes was in effect.
*/
typedef UInt32 Session_StateAttributes;
enum
{
	kSession_StateAttributeNotification		= (1 << 0),	//!< a watch has triggered for the session that has not been cleared by user focus
	kSession_StateAttributeOpenDialog		= (1 << 1),	//!< an alert element (typically a sheet) is currently applicable to the session
	kSession_StateAttributeSuspendNetwork	= (1 << 2)	//!< a Scroll Lock (XOFF) was initiated, so data has stopped transmitting
};

/*!
Options for Session_DisplayTerminationWarning().
*/
typedef UInt16 Session_TerminationDialogOptions;
enum
{
	kSession_TerminationDialogOptionModal				= (1 << 0),	//!< use a modal dialog (and wait for user before returning from call) instead of a sheet
	kSession_TerminationDialogOptionKeepWindow			= (1 << 1),	//!< do not close the terminal window if the session ends (for Kill or Restart modes)
	kSession_TerminationDialogOptionRestart				= (1 << 2),	//!< if the user chooses to end the session, its command line is run again (same window)
	kSession_TerminationDialogOptionNoAlertAnimation	= (1 << 3),	//!< suppress animation; currently only affects "kSession_TerminationDialogOptionModal"
	kSession_TerminationDialogDefaultOptions			= 0
};

/*!
A session can watch for one special event at a time, which (if
monitored) is automatically handled with an appropriate user
interface.  Watches are defined in a mutually exclusive way, so
it would never make sense to handle more than one of them at the
same time for the same session.  See also the Session_Change,
which is a way to install generic handlers for various events.

It is assumed that the user will not want to receive any
notifications for the session that he or she is using: if the
application is frontmost and the current user focus session is
the watched session, then the event is ignored.  This way, there
are no “stupid” alerts (such as telling the user data has arrived
in the session where they are typing!).
*/
enum Session_Watch
{
	kSession_WatchNothing				= 0,	//!< no basic monitors on data
	kSession_WatchForPassiveData		= 1,	//!< data has arrived from the running process (not necessarily user-initiated)
	kSession_WatchForInactivity			= 2,	//!< there has been a lack of data for a short period of time
	kSession_WatchForKeepAlive			= 3		//!< similar to inactivity, except the delay is much longer and a string is
												//!  transmitted to the session once the timer expires (presumably to keep
												//!  the session from disconnecting)
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
Various key mappings for typical session events.  To modify,
use Session_ReturnEventKeys() to copy the current values, and
Session_SetEventKeys() to write an updated structure.
*/
struct Session_EventKeys
{
	UInt8					interrupt;				//!< the ASCII code for the control key used to interrupt processes;
													//!  see Session_UserInputInterruptProcess()
	UInt8					suspend;				//!< the ASCII code for the control key used to stop the flow of data;
													//!  see Session_SetNetworkSuspended()
	UInt8					resume;					//!< the ASCII code for the control key used to start the flow of data;
													//!  see Session_SetNetworkSuspended()
	Session_NewlineMode		newline;				//!< what new-line means
	Session_EmacsMetaKey	meta;					//!< meta key generator, i.e. for Emacs
	Boolean					deleteSendsBackspace;	//!< if false, delete sends “delete”; if true, it sends a backspace
	Boolean					arrowsRemappedForEmacs;	//!< if false, arrows are not special; if true, they become Emacs cursor keys
	Boolean					pageKeysLocalControl;	//!< if false, page keys are sent to the session; if true, they manage scrolling
	Boolean					keypadRemappedForVT220;	//!< if false, arrows are not special; if true, they become Emacs cursor keys
};



#pragma mark Public Methods

//!\name Creating and Destroying Sessions
//@{

// DO NOT CREATE SESSIONS THIS WAY; THIS IS A TRANSITIONAL ROUTINE (USE SessionFactory METHODS, INSTEAD)
SessionRef
	Session_New								(Preferences_ContextRef				inConfigurationOrNull = nullptr,
											 Boolean							inIsReadOnly = false);

void
	Session_Dispose							(SessionRef*						inoutRefPtr);

Boolean
	Session_IsValid							(SessionRef							inRef);

//@}

//!\name User Interaction
//@{

void
	Session_DisplayFileCaptureSaveDialog	(SessionRef							inRef);

void
	Session_DisplayFileDownloadNameUI		(SessionRef							inRef,
											 CFStringRef						inFileName);

void
	Session_DisplaySaveDialog				(SessionRef							inRef);

void
	Session_DisplaySpecialKeySequencesDialog(SessionRef							inRef);

void
	Session_DisplayTerminationWarning		(SessionRef							inRef,
											 Session_TerminationDialogOptions	inOptions = kSession_TerminationDialogDefaultOptions,
											 void								(^inCancelAction)() = ^{});

void
	Session_DisplayWindowRenameUI			(SessionRef							inRef);

Boolean
	Session_IsInPasswordMode				(SessionRef							inRef);

Boolean
	Session_IsReadOnly						(SessionRef							inRef);

Session_Result
	Session_Select							(SessionRef							inRef);

void
	Session_UserInputCFString				(SessionRef							inRef,
											 CFStringRef						inStringBuffer);

void
	Session_UserInputInterruptProcess		(SessionRef							inRef);

Session_Result
	Session_UserInputFunctionKey			(SessionRef							inRef,
											 UInt8								inFunctionKeyNumber,
											 Session_FunctionKeyLayout			inKeyboardLayout = kSession_FunctionKeyLayoutVT220);

Session_Result
	Session_UserInputKey					(SessionRef							inRef,
											 UInt8								inKeyOrASCII,
											 UInt64								inEventModifiers = 0);

Session_Result
	Session_UserInputPaste					(SessionRef							inRef,
											 NSPasteboard*						inSourceOrNull = nullptr);

//@}

//!\name Write-Targeting Routines
//@{

Session_Result
	Session_AddDataTarget					(SessionRef							inRef,
											 Session_DataTarget					inTarget,
											 void*								inTargetData);

Session_Result
	Session_RemoveDataTarget				(SessionRef							inRef,
											 Session_DataTarget					inTarget,
											 void*								inTargetData);

//@}

//!\name Tektronix Vector Graphics Routines
//@{

void
	Session_TEKNewPage						(SessionRef							inRef);

Boolean
	Session_TEKPageCommandOpensNewWindow	(SessionRef							inRef);

void
	Session_TEKSetPageCommandOpensNewWindow	(SessionRef							inRef,
											 Boolean							inNewWindow);

//@}

//!\name Virtual Terminal Routines
//@{

Session_Result
	Session_TerminalCopyAnswerBackMessage	(SessionRef							inRef,
											 CFStringRef&						outAnswerBack);

void
	Session_TerminalWrite					(SessionRef							inRef,
											 UInt8 const*						inBuffer,
											 size_t								inLength);

void
	Session_TerminalWriteCString			(SessionRef							inRef,
											 char const*						inCString);

//@}

//!\name Miscellaneous
//@{

Session_Result
	Session_AppendDataForProcessing			(SessionRef							inRef,
											 UInt8 const*						inDataPtr,
											 size_t								inSize,
											 size_t*							outUnprocessedSizePtr);

void
	Session_FlushNetwork					(SessionRef							inRef);

SInt16
	Session_SendData						(SessionRef							inRef,
											 void const*						inBufferPtr,
											 size_t								inByteCount);

CFIndex
	Session_SendDataCFString				(SessionRef							inRef,
											 CFStringRef						inBuffer,
											 CFIndex							inFirstCharacter = 0);

void
	Session_SendDeleteBackward				(SessionRef							inRef,
											 Session_Echo						inEcho);

SInt16
	Session_SendFlush						(SessionRef							inRef);

void
	Session_SendNewline						(SessionRef							inRef,
											 Session_Echo						inEcho);

Session_Result
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
	Session_SetSpeechEnabled				(SessionRef							inRef,
											 Boolean							inIsEnabled);

// AFFECTS RETURN VALUES OF THE Session_WatchIs...() METHODS
void
	Session_SetWatch						(SessionRef							inRef,
											 Session_Watch						inNewWatch);

void
	Session_SetWindowUserDefinedTitle		(SessionRef							inRef,
											 CFStringRef						inWindowName);

void
	Session_SpeechPause						(SessionRef							inRef);

void
	Session_SpeechResume					(SessionRef							inRef);

//@}

//!\name Information on Sessions
//@{

Session_Result
	Session_GetStateIconImage				(SessionRef							inRef,
											 NSImage*&							outAutoreleasedImage);

Session_Result
	Session_GetStateString					(SessionRef							inRef,
											 CFStringRef&						outUncopiedString);

Session_Result
	Session_GetWindowUserDefinedTitle		(SessionRef							inRef,
											 CFStringRef&						outUncopiedString);

Boolean
	Session_LocalEchoIsEnabled				(SessionRef							inRef);

Boolean
	Session_LocalEchoIsHalfDuplex			(SessionRef							inRef);

Boolean
	Session_NetworkIsSuspended				(SessionRef							inRef);

NSWindow*
	Session_ReturnActiveNSWindow			(SessionRef							inRef);

TerminalWindowRef
	Session_ReturnActiveTerminalWindow		(SessionRef							inRef);

CFStringRef
	Session_ReturnCachedWorkingDirectory	(SessionRef							inRef);

CFArrayRef
	Session_ReturnCommandLine				(SessionRef							inRef);

// SEE ALSO Session_ReturnTranslationConfiguration()
Preferences_ContextRef
	Session_ReturnConfiguration				(SessionRef							inRef);

Session_EventKeys
	Session_ReturnEventKeys					(SessionRef							inRef);

CFStringRef
	Session_ReturnOriginalWorkingDirectory	(SessionRef							inRef);

CFStringRef
	Session_ReturnPseudoTerminalDeviceNameCFString	(SessionRef					inRef);

CFStringRef
	Session_ReturnResourceLocationCFString	(SessionRef							inRef);

Session_State
	Session_ReturnState						(SessionRef							inRef);

Session_StateAttributes
	Session_ReturnStateAttributes			(SessionRef							inRef);

// SEE ALSO Session_ReturnConfiguration()
Preferences_ContextRef
	Session_ReturnTranslationConfiguration	(SessionRef							inRef);

void
	Session_SetEventKeys					(SessionRef							inRef,
											 Session_EventKeys const&			inKeys);

void
	Session_SetProcess						(SessionRef							inRef,
											 Local_ProcessRef					inRunningProcess);

// AFFECTS RETURN VALUES OF THE Session_StateIs...() METHODS
void
	Session_SetState						(SessionRef							inRef,
											 Session_State						inNewState);

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

CFAbsoluteTime
	Session_TimeOfActivation				(SessionRef							inRef);

CFAbsoluteTime
	Session_TimeOfTermination				(SessionRef							inRef);

Boolean
	Session_TypeIsLocalLoginShell			(SessionRef							inRef);

Boolean
	Session_TypeIsLocalNonLoginShell		(SessionRef							inRef);

Boolean
	Session_WatchIsForInactivity			(SessionRef							inRef);

Boolean
	Session_WatchIsForKeepAlive				(SessionRef							inRef);

Boolean
	Session_WatchIsForPassiveData			(SessionRef							inRef);

Boolean
	Session_WatchIsOff						(SessionRef							inRef);

/*###############################################################
	SESSION ACCESSORS
	! ! ! TEMPORARY ! ! !
###############################################################*/

void
	Session_SetTerminalWindow				(SessionRef							inRef,
											 TerminalWindowRef					inTerminalWindow);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
