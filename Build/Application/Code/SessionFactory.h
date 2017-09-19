/*!	\file SessionFactory.h
	\brief Construction mechanism for sessions (terminal
	windows that run local or remote processes).
	
	Note that although this is a very useful, high-level
	API, it is still better to use Quills::Session when
	creating new sessions.  If a session is not created
	through Quills, it is invisible to all scripting code
	and will not (for instance) trigger the user callback
	for “new session”, among other things.
	
	Note, also, that you should not call Session_New()
	directly.  This module assumes it is aware of “all”
	sessions, and the rest of the program relies on that.
	For instance, the list of open sessions displayed in
	various user interface elements is only accurate if
	Session Factory ultimately created all sessions.
	(Quills uses the Session Factory.)
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

// standard-C++ includes
#include <vector>

// Mac includes
#include <CoreServices/CoreServices.h>

// application includes
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
Setting changes that MacTerm allows other modules to
“listen” for, via SessionFactory_StartMonitoring().
*/
typedef FourCharCode SessionFactory_Change;
enum
{
	kSessionFactory_ChangeActivatingSession		= 'news',	//!< context: SessionRef of session that is becoming active
	kSessionFactory_ChangeDeactivatingSession	= 'olds',	//!< context: SessionRef of session that is becoming inactive
	kSessionFactory_ChangeNewSessionCount		= 'cxn#'	//!< context: reserved
};

/*!
Types of session lists maintained by this module.  You
use these when adding or removing sessions, and when
using indexing (such as, “the first session whose...”).
*/
typedef FourCharCode SessionFactory_List;
enum
{
	kSessionFactory_ListInCreationOrder		= 'cron',	//!< in order of creation time, session 0 is earliest
	kSessionFactory_ListInTabStackOrder		= 'tabs'	//!< if tabs are in use, iterates over workspaces in turn, from first tab to last;
														//!  otherwise, works like "kSessionFactory_ListInCreationOrder"
};

#pragma mark Types

typedef std::vector< TerminalWindowRef >	SessionFactory_TerminalWindowList;

#pragma mark Callbacks

/*!
Session Block

This type of block is used in SessionFactory_ForEachSessionDo().
*/
typedef void (^SessionFactory_SessionBlock)	(SessionRef);

/*!
Terminal Window Operation Routine

This procedure type defines a convenient function that,
among other things, can be used as an iterator of all
terminal windows of any kind.  A wealth of useful
information can be found using only the provided
parameters, shielding code in the rest of MacTerm from
the actual terminal window list implementation.

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

SessionRef
	SessionFactory_NewCloneSession					(TerminalWindowRef				inTerminalWindow,
													 SessionRef						inBaseSession);

SessionRef
	SessionFactory_NewSessionArbitraryCommand		(TerminalWindowRef				inTerminalWindow,
													 CFArrayRef						inArgumentArray,
													 Preferences_ContextRef			inContextOrNull = nullptr,
													 Boolean						inReconfigureTerminalFromAssociatedContexts = true,
													 Preferences_ContextRef			inWorkspaceOrNull = nullptr,
													 UInt16							inWindowIndexInWorkspaceOrZero = 0,
													 CFStringRef					inWorkingDirectoryOrNull = nullptr);

SessionRef
	SessionFactory_NewSessionDefaultShell			(TerminalWindowRef				inTerminalWindow,
													 Preferences_ContextRef			inWorkspaceOrNull = nullptr,
													 UInt16							inWindowIndexInWorkspaceOrZero = 0,
													 CFStringRef					inWorkingDirectoryOrNull = nullptr);

SessionRef
	SessionFactory_NewSessionFromCommandFile		(TerminalWindowRef				inTerminalWindow,
													 char const*					inCommandFilePath,
													 Preferences_ContextRef			inWorkspaceOrNull = nullptr,
													 UInt16							inWindowIndexInWorkspaceOrZero = 0);

SessionRef
	SessionFactory_NewSessionFromDescription		(TerminalWindowRef				inTerminalWindow,
													 SessionDescription_Ref			inSessionDescription,
													 Preferences_ContextRef			inWorkspaceOrNull = nullptr,
													 UInt16							inWindowIndexInWorkspaceOrZero = 0);

SessionRef
	SessionFactory_NewSessionLoginShell				(TerminalWindowRef				inTerminalWindow,
													 Preferences_ContextRef			inWorkspaceOrNull = nullptr,
													 UInt16							inWindowIndexInWorkspaceOrZero = 0);

SessionRef
	SessionFactory_NewSessionUserFavorite			(TerminalWindowRef				inTerminalWindow,
													 Preferences_ContextRef			inSessionContext,
													 Preferences_ContextRef			inWorkspaceOrNull = nullptr,
													 UInt16							inWindowIndexInWorkspaceOrZero = 0);

Boolean
	SessionFactory_NewSessionsUserFavoriteWorkspace	(Preferences_ContextRef			inWorkspaceContext);

TerminalWindowRef
	SessionFactory_NewTerminalWindowUserFavorite	(Preferences_ContextRef			inTerminalInfoOrNull = nullptr,
													 Preferences_ContextRef			inFontInfoOrNull = nullptr,
													 Preferences_ContextRef			inTranslationInfoOrNull = nullptr);

Boolean
	SessionFactory_RespawnSession					(SessionRef						inSession);

//@}

//!\name User Interaction
//@{

Boolean
	SessionFactory_DisplayUserCustomizationUI		(TerminalWindowRef				inTerminalWindow,
													 Preferences_ContextRef			inWorkspaceOrNull = nullptr,
													 UInt16							inWindowIndexInWorkspaceOrZero = 0);

void
	SessionFactory_MoveTerminalWindowToNewWorkspace	(TerminalWindowRef				inTerminalWindow);

SessionRef
	SessionFactory_ReturnTerminalWindowSession		(TerminalWindowRef				inTerminalWindow);

SessionRef
	SessionFactory_ReturnUserFocusSession			();

SessionRef
	SessionFactory_ReturnUserRecentSession			();

//@}

//!\name Iterating Over Sessions and Terminal Windows
//@{

void
	SessionFactory_ForEachSessionInFrozenList		(SessionFactory_SessionBlock	inBlock);

void
	SessionFactory_ForEachSessionInReadOnlyList		(SessionFactory_SessionBlock	inBlock);

void
	SessionFactory_ForEveryTerminalWindowDo			(SessionFactory_TerminalWindowOpProcPtr	inProcPtr,
													 void*							inData1,
													 SInt32							inData2,
													 void*							inoutResultPtr);

// FOR SPECIAL USES ONLY
SessionFactory_TerminalWindowList const&
	SessionFactory_ReturnTerminalWindowList			();

//@}

//!\name Indexing Sessions
//@{

SessionFactory_Result
	SessionFactory_GetWindowWithZeroBasedIndex		(UInt16							inZeroBasedSessionIndex,
													 SessionFactory_List			inFromWhichList,
													 HIWindowRef*					outWindowPtr);

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
	SessionFactory_SetAutoRearrangeTabsEnabled		(Boolean						inIsEnabled);

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

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
