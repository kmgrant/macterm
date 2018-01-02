/*!	\file Local.h
	\brief UNIX-based routines for running local command-line
	programs on Mac OS X only, routing the data through some
	MacTerm terminal window.
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

#include <UniversalDefines.h>

#pragma once

// standard-C++ includes
#include <map>

// UNIX includes
extern "C"
{
#	include <sys/types.h>
}

// Mac includes
#include <CoreFoundation/CoreFoundation.h>

// application includes
#include "SessionRef.typedef.h"
#include "TerminalScreenRef.typedef.h"



#pragma mark Constants

/*!
Possible return values from Local module routines.
*/
typedef long Local_Result;
enum
{
	kLocal_ResultOK							= 0,	//!< no error
	kLocal_ResultParameterError				= 1,	//!< invalid input (e.g. null pointer provided)
	kLocal_ResultForkError					= 2,	//!< fork() failed
	kLocal_ResultThreadError				= 3,	//!< unable to create a thread
	kLocal_ResultIOControlError				= 4,	//!< ioctl() failed
	kLocal_ResultTermCapError				= 5,	//!< terminal capabilities interface routine failed
	kLocal_ResultSocketError				= 6,	//!< if a socket could not be created
	kLocal_ResultNoRouteToHost				= 7,	//!< if a physical connection could not be made
	kLocal_ResultConnectionRefused			= 8,	//!< if a connection was not allowed by the server
	kLocal_ResultCannotResolveAtAll			= 9,	//!< if a host name was given whose address cannot be found
	kLocal_ResultCannotResolveForNow		= 10,	//!< if an address cannot be found, but a retry might find it
	kLocal_ResultInsufficientBufferSpace	= 11	//!< out of memory; free more memory and try again
};

typedef int/* file descriptor */	Local_TerminalID;
enum
{
	kLocal_InvalidTerminalID		= -1
};

#pragma mark Types

typedef struct Local_OpaqueProcess*		Local_ProcessRef;

typedef std::map< Local_ProcessRef, CFStringRef >	Local_PathByProcess;



#pragma mark Public Methods

//!\name Creating Processes and Pseudo-Terminals
//@{

Local_Result
	Local_GetDefaultShellCommandLine		(CFArrayRef&				outNewArgumentsArray);

Local_Result
	Local_GetLoginShellCommandLine			(CFArrayRef&				outNewArgumentsArray);

Local_Result
	Local_SpawnProcess						(SessionRef					inUninitializedSession,
											 TerminalScreenRef			inContainer,
											 CFArrayRef					inArgumentArray,
											 CFStringRef				inWorkingDirectoryOrNull = nullptr);

Local_Result
	Local_SpawnProcessAndWaitForTermination	(char const*				inCommand);

//@}

//!\name Manipulating Processes
//@{

void
	Local_KillProcess						(Local_ProcessRef*			inoutRefPtr);

Boolean
	Local_ProcessIsInPasswordMode			(Local_ProcessRef			inProcess);

Boolean
	Local_ProcessIsStopped					(Local_ProcessRef			inProcess);

CFArrayRef
	Local_ProcessReturnCommandLine			(Local_ProcessRef			inProcess);

// NOTE: MUST CALL Local_UpdateCurrentDirectoryCache() PRIOR TO RELYING ON THIS VALUE
CFStringRef
	Local_ProcessReturnCurrentDirectory		(Local_ProcessRef			inProcess);

Local_TerminalID
	Local_ProcessReturnMasterTerminal		(Local_ProcessRef			inProcess);

CFStringRef
	Local_ProcessReturnOriginalDirectory	(Local_ProcessRef			inProcess);

char const*
	Local_ProcessReturnSlaveDeviceName		(Local_ProcessRef			inProcess);

pid_t
	Local_ProcessReturnUnixID				(Local_ProcessRef			inProcess);

//@}

//!\name Manipulating Pseudo-Terminals
//@{

int
	Local_TerminalDisableLocalEcho			(Local_TerminalID			inPseudoTerminalID);

Local_Result
	Local_TerminalResize					(Local_TerminalID			inPseudoTerminalID,
											 UInt16						inNewColumnCount,
											 UInt16						inNewRowCount,
											 UInt16						inNewColumnWidthInPixels,
											 UInt16						inNewRowHeightInPixels);

int
	Local_TerminalReturnFlowStartCharacter	(Local_TerminalID			inPseudoTerminalID);

int
	Local_TerminalReturnFlowStopCharacter	(Local_TerminalID			inPseudoTerminalID);

int
	Local_TerminalReturnInterruptCharacter	(Local_TerminalID			inPseudoTerminalID);

Boolean
	Local_TerminalSetUTF8Encoding			(Local_TerminalID			inPseudoTerminalID,
											 Boolean					inIsUTF8);

extern "C"
{
ssize_t
	Local_TerminalWriteBytes				(Local_TerminalID			inFileDescriptor,
											 void const*				inBufferPtr,
											 size_t						inByteCount);
}

//@}

//!\name General
//@{

Boolean
	Local_StandardInputIsATerminal			();

void
	Local_UpdateCurrentDirectoryCache		();

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
