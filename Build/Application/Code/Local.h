/*!	\file Local.h
	\brief UNIX-based routines for running local command-line
	programs on Mac OS X only, routing the data through some
	MacTelnet terminal window.
*/
/*###############################################################

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

#ifndef __LOCAL__
#define __LOCAL__

// UNIX includes
extern "C"
{
#	include <sys/types.h>
}

// MacTelnet includes
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
	kLocalResultInsufficientBufferSpace		= 11	//!< out of memory; free more memory and try again
};

typedef int/* file descriptor */	Local_TerminalID;
enum
{
	kLocal_InvalidTerminalID		= -1
};

#pragma mark Types

typedef struct Local_OpaqueProcess*		Local_ProcessRef;



#pragma mark Public Methods

//!\name Creating Processes and Pseudo-Terminals
//@{

Local_Result
	Local_SpawnDefaultShell					(SessionRef					inUninitializedSession,
											 TerminalScreenRef			inContainer,
											 char const*				inWorkingDirectoryOrNull = nullptr);

Local_Result
	Local_SpawnLoginShell					(SessionRef					inUninitializedSession,
											 TerminalScreenRef			inContainer,
											 char const*				inWorkingDirectoryOrNull = nullptr);

Local_Result
	Local_SpawnProcess						(SessionRef					inUninitializedSession,
											 TerminalScreenRef			inContainer,
											 char const* const			argv[],
											 char const*				inWorkingDirectoryOrNull = nullptr);

Local_Result
	Local_SpawnProcessAndWaitForTermination	(char const*				inCommand);

//@}

//!\name Manipulating Processes
//@{

void
	Local_KillProcess						(Local_ProcessRef*			inoutRefPtr);

char const*
	Local_ProcessReturnCommandLineString	(Local_ProcessRef			inProcess);

Local_TerminalID
	Local_ProcessReturnMasterTerminal		(Local_ProcessRef			inProcess);

char const*
	Local_ProcessReturnSlaveDeviceName		(Local_ProcessRef			inProcess);

pid_t
	Local_ProcessReturnUnixID				(Local_ProcessRef			inProcess);

//@}

//!\name Manipulating Pseudo-Terminals
//@{

int
	Local_DisableTerminalLocalEcho			(Local_TerminalID			inPseudoTerminalID);

int
	Local_ReturnTerminalFlowStartCharacter	(Local_TerminalID			inPseudoTerminalID);

int
	Local_ReturnTerminalFlowStopCharacter	(Local_TerminalID			inPseudoTerminalID);

int
	Local_ReturnTerminalInterruptCharacter	(Local_TerminalID			inPseudoTerminalID);

Local_Result
	Local_SendTerminalResizeMessage			(Local_TerminalID			inPseudoTerminalID,
											 UInt16						inNewColumnCount,
											 UInt16						inNewRowCount,
											 UInt16						inNewColumnWidthInPixels,
											 UInt16						inNewRowHeightInPixels);

extern "C"
{
ssize_t
	Local_WriteBytes						(Local_TerminalID			inFileDescriptor,
											 void const*				inBufferPtr,
											 size_t						inByteCount);
}

//@}

//!\name General Information
//@{

Local_Result
	Local_GetDefaultShell					(char**						outStringPtr);

Boolean
	Local_StandardInputIsATerminal			();

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
