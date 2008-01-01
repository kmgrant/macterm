/*!	\file SessionFactory.h
	\brief Construction mechanism for sessions (terminal
	windows that run local or remote processes).
	
	This module provides a number of ways to construct new
	Session objects, as well as utilities for managing the
	global set of all sessions currently in use.
	
	IMPORTANT:	This is the manager for Session objects.
				It is highly recommended that you NEVER
				create Session objects in any other way,
				because this module provides a way for the
				rest of the program to figure out what
				sessions are running (for example, to
				display a list of open sessions).
*/
/*###############################################################

	MacTelnet
		© 1998-2007 by Kevin Grant.
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

#ifndef __SESSIONFACTORY__
#define __SESSIONFACTORY__

// Mac includes
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "Preferences.h"
#include "Session.h"
#include "SessionDescription.h"



#pragma mark Constants

enum SessionFactory_Result
{
	kSessionFactory_ResultOK				= 0,	//!< no error
	kSessionFactory_ResultNotInitialized	= 1,	//!< SessionFactory_Init() has never been called
	kSessionFactory_ResultParameterError	= 2		//!< invalid input (e.g. a null pointer)
};

/*!
Setting changes that MacTelnet allows other modules to
ÒlistenÓ for, via SessionFactory_StartMonitoring().
*/
typedef FourCharCode SessionFactory_Change;
enum
{
	kSessionFactory_ChangeNewSessionCount		= FOUR_CHAR_CODE('cxn#')	//!< context: reserved
};

/*!
Types of session lists maintained by this module.  You
use these when adding or removing sessions, and when
using indexing (such as, Òthe first session whose...Ó).
*/
typedef FourCharCode SessionFactory_List;
enum
{
	kSessionFactory_ListInCreationOrder		= FOUR_CHAR_CODE('cron')	//!< in order of creation time, session 0 is earliest
};

/*!
These flags filter out a list of sessions.  This helps
simply iteration code because you can prevent your
callback from being invoked at all for sessions in a
certain category, rather than taking the more compute-
intensive approach of checking each session yourself.

Combine flags to create larger subsets of sessions.
Use "kSessionFactorySessionFilterFlagAllSessions" when
you know you want to be told about every session,
regardless of its class.
*/
typedef CFOptionFlags SessionFactory_SessionFilterFlags;
enum
{
	kSessionFactory_SessionFilterFlagConsoleSessions	= (1L << 1), //!< sessions handling diagnostic output
	kSessionFactory_SessionFilterFlagRegularSessions	= (1L << 0), //!< sessions running a local Unix program (usually a shell)
	// use these convenient flags to group the above flags in consistent ways
	kSessionFactory_SessionFilterFlagAllSessions		= kSessionFactory_SessionFilterFlagRegularSessions |
															kSessionFactory_SessionFilterFlagConsoleSessions
};

#pragma mark Callbacks

/*!
Session Operation Routine

This procedure type defines a convenient function that,
among other things, can be used as an iterator of all
terminal windows representing sessions.  A wealth of
useful information can be found using only the provided
parameters, shielding code in the rest of MacTelnet
from the actual session list implementation.
*/
typedef void (*SessionFactory_SessionOpProcPtr)	(SessionRef		inSession,
												 void*			inData1,
												 SInt32			inData2,
												 void*			inoutResultPtr);
inline void
SessionFactory_InvokeSessionOpProc	(SessionFactory_SessionOpProcPtr	inProc,
									 SessionRef							inSession,
									 void*								inData1,
									 SInt32								inData2,
									 void*								inoutResultPtr)
{
	(*inProc)(inSession, inData1, inData2, inoutResultPtr);
}

/*!
Terminal Window Operation Routine

This procedure type defines a convenient function that,
among other things, can be used as an iterator of all
terminal windows of any kind.  A wealth of useful
information can be found using only the provided
parameters, shielding code in the rest of MacTelnet
from the actual terminal window list implementation.

Note that it is sometimes more appropriate to iterate
over Sessions than Terminal Windows.  Carefully consider
what you are trying to do, so that you iterate at the
right level of abstraction.
*/
typedef void (*SessionFactory_TerminalWindowOpProcPtr)	(TerminalWindowRef		inTerminalWindow,
														 void*					inData1,
														 SInt32					inData2,
														 void*					inoutResultPtr);
inline void
SessionFactory_InvokeTerminalWindowOpProc	(SessionFactory_TerminalWindowOpProcPtr		inUserRoutine,
											 TerminalWindowRef							inTerminalWindow,
											 void*										inData1,
											 SInt32										inData2,
											 void*										inoutResultPtr)
{
	(*inUserRoutine)(inTerminalWindow, inData1, inData2, inoutResultPtr);
}



#pragma mark Public Methods

//!\name Initialization
//@{

void
	SessionFactory_Init								();

void
	SessionFactory_Done								();

//@}

//!\name Creating Sessions
//@{

// NOTE: QUILLS INTERFACES ARE ALSO AVAILABLE FOR THIS:
//	Quills::Session::handle_url()

SessionRef
	SessionFactory_NewCloneSession					(TerminalWindowRef				inTerminalWindowOrNullToMakeNewWindow,
													 SessionRef						inBaseSession);

SessionRef
	SessionFactory_NewSessionArbitraryCommand		(TerminalWindowRef				inTerminalWindowOrNullToMakeNewWindow,
													 char const* const				argv[],
													 Preferences_ContextRef			inContextOrNull = nullptr,
													 char const*					inWorkingDirectoryOrNull = nullptr);

SessionRef
	SessionFactory_NewSessionArbitraryCommand		(TerminalWindowRef				inTerminalWindowOrNullToMakeNewWindow,
													 CFArrayRef						inArray,
													 Preferences_ContextRef			inContextOrNull = nullptr);

SessionRef
	SessionFactory_NewSessionDefaultShell			(TerminalWindowRef				inTerminalWindowOrNullToMakeNewWindow = nullptr,
													 Preferences_ContextRef			inContextOrNull = nullptr);

SessionRef
	SessionFactory_NewSessionFromCommandFile		(TerminalWindowRef				inTerminalWindowOrNullToMakeNewWindow,
													 char const*					inCommandFilePath,
													 Preferences_ContextRef			inContextOrNull = nullptr);

SessionRef
	SessionFactory_NewSessionFromDescription		(TerminalWindowRef				inTerminalWindowOrNullToMakeNewWindow,
													 SessionDescription_Ref			inSessionDescription);

SessionRef
	SessionFactory_NewSessionFromTerminalFile		(TerminalWindowRef				inTerminalWindowOrNullToMakeNewWindow,
													 char const*					inAppleDotTermFilePath,
													 Preferences_ContextRef			inContextOrNull = nullptr);

SessionRef
	SessionFactory_NewSessionLoginShell				(TerminalWindowRef				inTerminalWindowOrNullToMakeNewWindow = nullptr,
													 Preferences_ContextRef			inContextOrNull = nullptr);

SessionRef
	SessionFactory_NewSessionUserFavorite			(TerminalWindowRef				inTerminalWindowOrNullToMakeNewWindow,
													 Preferences_ContextRef			inSessionContext);

TerminalWindowRef
	SessionFactory_NewTerminalWindowUserFavorite	(Preferences_ContextRef			inTerminalInfoOrNull = nullptr,
													 Preferences_ContextRef			inFontInfoOrNull = nullptr);

//@}

//!\name User Interaction
//@{

Boolean
	SessionFactory_DisplayUserCustomizationUI		(TerminalWindowRef				inTerminalWindowOrNullToMakeNewWindow = nullptr,
													 Preferences_ContextRef			inContextOrNull = nullptr);

void
	SessionFactory_MoveTerminalWindowToNewWorkspace	(TerminalWindowRef				inTerminalWindow);

SessionRef
	SessionFactory_ReturnUserFocusSession			();

//@}

//!\name Iterating Over Sessions and Terminal Windows
//@{

void
	SessionFactory_ForEachSessionDo					(SessionFactory_SessionFilterFlags	inFilterFlags,
													 SessionFactory_SessionOpProcPtr	inProcPtr,
													 void*							inData1,
													 SInt32							inData2,
													 void*							inoutResultPtr,
													 Boolean						inFinal = false);

void
	SessionFactory_ForEveryTerminalWindowDo			(SessionFactory_TerminalWindowOpProcPtr	inProcPtr,
													 void*							inData1,
													 SInt32							inData2,
													 void*							inoutResultPtr);

//@}

//!\name Indexing Sessions
//@{

SessionFactory_Result
	SessionFactory_GetSessionWithZeroBasedIndex		(UInt16							inZeroBasedSessionIndex,
													 SessionFactory_List			inFromWhichList,
													 SessionRef*					outSessionPtr);

SessionFactory_Result
	SessionFactory_GetZeroBasedIndexOfSession		(SessionRef						inOfWhichSession,
													 SessionFactory_List			inFromWhichList,
													 UInt16*						outIndexPtr);

//@}

//!\name Counting Sessions Created
//@{

Boolean
	SessionFactory_CountIsAtLeastOne				();

UInt16
	SessionFactory_ReturnCount						();

// NUMBER OF SESSIONS WHOSE STATE MATCHES THE GIVEN STATE
UInt16
	SessionFactory_ReturnStateCount					(Session_State					inStateToCheckFor);

//@}

//!\name Utilities
//@{

SessionFactory_Result
	SessionFactory_StartMonitoring					(SessionFactory_Change			inForWhatChange,
													 ListenerModel_ListenerRef		inListener);

SessionFactory_Result
	SessionFactory_StartMonitoringSessions			(Session_Change					inForWhatChange,
													 ListenerModel_ListenerRef		inListener);

void
	SessionFactory_StopMonitoring					(SessionFactory_Change			inForWhatChange,
													 ListenerModel_ListenerRef		inListener);

void
	SessionFactory_StopMonitoringSessions			(Session_Change					inForWhatChange,
													 ListenerModel_ListenerRef		inListener);

void
	SessionFactory_UpdatePalettes					(Preferences_Tag				inColorToChange);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
