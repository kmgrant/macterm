/*!	\file NetEvents.h
	\brief Carbon Events used for network data handling.
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

#ifndef __NETEVENTS__
#define __NETEVENTS__



#pragma mark Constants

/*!
Custom Carbon Event data types.
*/
enum
{
	typeNetEvents_EventQueueRef			= 'TEQ&',	//!< "EventQueueRef"
	typeNetEvents_PreferencesContextRef	= 'TTSR',	//!< "Preferences_ContextRef"
	typeNetEvents_SessionRef			= 'TSn&',	//!< "SessionRef"
	typeNetEvents_SessionState			= 'TSSt',	//!< "Session_State"
	typeNetEvents_StructHostEntPtr		= 'TSHE',	//!< "struct hostent*"
	typeNetEvents_TerminalScreenRef		= 'TTSR',	//!< "TerminalScreenRef"
	typeNetEvents_TerminalViewRef		= 'TTVR',	//!< "TerminalViewRef"
	typeNetEvents_CFBooleanRef			= 'CFTF',	//!< "CFBooleanRef"; could use typeCFBooleanRef but that is not available in the Mac OS 10.1 SDK
	typeNetEvents_CFNumberRef			= 'CFNm'	//!< "CFNumberRef"; could use typeCFNumberRef but that is not available in the Mac OS 10.1 SDK
};

/*!
Custom Carbon Event parameters.
*/
enum
{
	kEventParamNetEvents_DirectHostEnt				= 'PSHE',	//!< the "struct hostent*" of direct focus (data: typeNetEvents_StructHostEntPtr)
	kEventParamNetEvents_DirectSession				= 'PSn&',	//!< the session directly impacted by an event (data: typeNetEvents_SessionRef)
	kEventParamNetEvents_DispatcherQueue			= 'PDQ&',	//!< queue to submit follow-up events to, for 2-way communication
																//!  (data: typeNetEvents_CarbonEventQueueRef)
	kEventParamNetEvents_NewSessionState			= 'PSnS',	//!< what to change a session state to (data: typeNetEvents_SessionState)
	kEventParamNetEvents_ParentWindow				= 'PPrW',	//!< containing window (data: typeWindowRef)
	kEventParamNetEvents_SessionData				= 'PSnD',	//!< data to process in a session (data: typeVoidPtr)
	kEventParamNetEvents_SessionDataSize			= 'PSDS',	//!< size of data buffer given in "kEventParamNetEvents_SessionData"
																//!  (data: typeUInt32)
	kEventParamNetEvents_TerminalDataSource			= 'PTDS',	//!< where terminal data comes from (data: typeNetEvents_TerminalScreenRef)
	kEventParamNetEvents_TerminalFormatPreferences	= 'PTFP'	//!< format settings for font, size, etc. (data: typeNetEvents_PreferencesContextRef)
};

//!\name Domain Name System Carbon Events
//@{

/*!
kEventClassNetEvents_DNS quick reference:

kEventNetEvents_HostLookupComplete
*/
enum
{
	kEventClassNetEvents_DNS = 'CDNS'
};

/*!
kEventClassNetEvents_DNS / kEventNetEvents_HostLookupComplete

Summary:
  Issued when a pending domain name lookup has completed.

Discussion:
  Handling this event allows lookups to be asynchronous.

Parameters:
  
  <-- kEventParamNetEvents_DirectHostEnt (out, typeNetEvents_StructHostEntPtr)
        The results of the lookup; call DNR_Dispose() to free memory.
*/
enum
{
	kEventNetEvents_HostLookupComplete	= 'KDLC'
};

//@}

//!\name Session Carbon Events
//@{

/*!
kEventClassNetEvents_Session quick reference:

kEventNetEvents_SessionDataArrived
kEventNetEvents_SessionDataProcessed
kEventNetEvents_SessionSetState
*/
enum
{
	kEventClassNetEvents_Session = 'CSsn'
};

/*!
kEventClassNetEvents_Session / kEventNetEvents_SessionDataArrived

Summary:
  Issued when the process for a session has printed output.

Discussion:
  Effectively invokes Session_AppendDataForProcessing(), which
  cannot be invoked directly from a preemptive thread.  If you
  post this event to the main queue, the API call is triggered at
  a safe point in the main thread.
  
  The call completes by inserting a new event of type
  "kEventNetEvents_SessionDataProcessed" into the given queue
  (presumably the one in the thread that dispatched this event).

Parameters:
  
  --> kEventParamNetEvents_DirectSession (in, typeNetEvents_SessionRef)
        The session that data arrived for.
  
  --> kEventParamNetEvents_SessionData (in, typeVoidPtr)
		A pointer to the session data to process.
  
  --> kEventParamNetEvents_SessionDataSize (in, typeUInt32)
        The size of the session data buffer.

  --> kEventParamNetEvents_DispatcherQueue (in, typeNetEvents_CarbonEventQueueRef)
		The queue to be notified when data is finally processed.
*/
enum
{
	kEventNetEvents_SessionDataArrived = 'KSDA'
};

/*!
kEventClassNetEvents_Session / kEventNetEvents_SessionDataProcessed

Summary:
  Reply event that should be posted by a queue that handles
  "kEventNetEvents_SessionDataArrived" events.

Discussion:
  The handler should insert this event into the given dispatcher
  queue once the received data is processed; this tells the
  dispatcher that further data arrival events can now be handled.

Parameters:
  --> kEventParamNetEvents_DirectSession (in, typeNetEvents_SessionRef)
        The session that data was processed in.
  
  <-- kEventParamNetEvents_SessionData (in, typeVoidPtr)
        A pointer to the session data that was processed.
  
  <-- kEventParamNetEvents_SessionDataSize (in, typeUInt32)
        The number of bytes NOT processed.
*/
enum
{
	kEventNetEvents_SessionDataProcessed = 'KSDP'
};

/*!
kEventClassNetEvents_Session / kEventNetEvents_SessionSetState

Summary:
  Indirect way to change the state of a session.

Discussion:
  Effectively invokes Session_SetState(), which cannot be invoked
  directly from a preemptive thread.  If you post this event to
  the main queue, the API call is triggered at a safe point in
  the main thread.

Parameters:
  --> kEventParamNetEvents_DirectSession (in, typeNetEvents_SessionRef)
        The session to change the state of.
  
  --> kEventParamNetEvents_NewSessionState (in, typeNetEvents_SessionState)
        The new session state.
  
  --> kEventParamNetEvents_DispatcherQueue (in, typeNetEvents_CarbonEventQueueRef)
        Optional; the queue that wants to receive response events.
*/
enum
{
	kEventNetEvents_SessionSetState = 'KSSS'
};

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
