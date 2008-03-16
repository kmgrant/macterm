/*!	\file CommandLine.h
	\brief The floating one-line input window in MacTelnet 3.0.
	
	This window is “pluggable” with command handlers that
	interpret input ahead of time like some UNIX shells do. 
	Input is sent to the frontmost terminal window when entered
	or tabbed by the user.
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

#ifndef __COMMANDLINE__
#define __COMMANDLINE__

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Constants

/*!
Possible messages that could be sent to a "CommandLine_InterpreterProcPtr".
*/
enum CommandLine_InterpreterMessage
{
	kCommandLine_InterpreterMessageIsCommandYours = 1,			//!< outDataPtr == nullptr
	kCommandLine_InterpreterMessageDescribeLastArgument = 2,	//!< outDataPtr == Str255, write-only
	kCommandLine_InterpreterMessageExecute = 3,					//!< outDataPtr == nullptr
	kCommandLine_InterpreterMessageOpenManual = 4				//!< outDataPtr == not yet defined
};



#pragma mark Callbacks

/*!
Command Interpreting Method

This defines the main entry point to a command handler.  You are
given an array of CFStrings and a message, and requested to return
"true" or "false".  The messages are defined below.

In response to an “is command yours” message:
	arguments:		1
	string array:	the first word entered on the command line
	outDataPtr:		not defined; should be ignored and not changed
	
	You are responsible at this stage for examining only the first
	token of the given set, which is the command name.  Return "true"
	only if the command is yours (or one you can handle), which is
	usually done using a case-insensitive, diacritics-sensitive test.

In response to a “describe last argument” message:
	arguments:		if 1, describe the entire command; otherwise,
					describe the last argument of the given set
	string array:	the words entered on the command line
	outDataPtr:		output-only; "CFStringRef" type
	
	The command line continually parses what the user types, as he or
	she types it, sending “is command yours” messages to all command
	interpreters.  If an interpreter returns "true" to that message,
	the command line modifies its display with a text string that
	describes the command that the user is trying to invoke *or* the
	last argument on the command line.  To find out what this text
	string is, a “describe last argument” message is passed to the
	appropriate interpreter.  Your method should return an appropriate
	descriptive string, stated as a sentence fragment *without*
	mentioning the command’s name or the argument name.  If the last
	argument is recognized, return "true" and fill in the descriptive
	text; otherwise, return "false".  If your command recognizes
	generic arguments, such as file names or URLs, you may wish to
	verify that an unrecognized argument starts with "+" or "-" before
	returning "false", because "false" causes MacTelnet to warn the
	user that the argument is not understood by the command.  In other
	words, return "true" for generic arguments as well.

In response to an “execute” message:
	arguments:		number of words, including the command name (the
					first word)
	string array:	all words entered on the command line
	outDataPtr:		not defined; should be ignored and not changed
	
	At this stage, the command line has granted tokens exclusively to
	you, which means that regardless of the return value, no further
	parsing will be done.  You should determine if *all* of the
	arguments are valid except argument 0 (which was validated via an
	earlier “is command yours” message), and if they are valid,
	execute the command and return "true".  If "false" is returned,
	the command line emits a system beep and does not add the text in
	the command line to the command history (nor does it clear the
	command line).  Otherwise, the command is assumed to have
	executed properly, so the command line is cleared and the line is
	added to the command history rotation.  Therefore, if you return
	"false", you should not perform *any* action.  Conversely, if you
	do perform *any* kind of action, return "true".

In response to an “open manual” message:
	arguments:		if 1, describe the entire command; otherwise,
					describe the last argument of the given set
	string array:	all words entered on the command line
	outDataPtr:		not yet defined
	
	IN FLUX, NEW COMMAND NOT YET COMPLETELY DEFINED
	This request is sent if the user wants to display the entire
	manual for the given command or command argument.  The data
	returned should be a reference to a manual that MacTelnet can
	cause to be displayed.
*/
typedef Boolean (*CommandLine_InterpreterProcPtr)	(CFArrayRef							inTokens,
													 void*								outDataPtr,
													 CommandLine_InterpreterMessage		inMessage);



#pragma mark Public Methods

void
	CommandLine_Init						();

void
	CommandLine_Done						();

void
	CommandLine_AddInterpreter				(CommandLine_InterpreterProcPtr		inProcPtr);

/*!
Convenience function for invoking routines of type
"CommandLine_InterpreterProcPtr".
*/
inline Boolean
	CommandLine_InvokeInterpreterProc		(CommandLine_InterpreterProcPtr		inUserRoutine,
											 CFArrayRef							inTokens,
											 void*								outDataPtr,
											 CommandLine_InterpreterMessage		inMessage)
	{
		return (*inUserRoutine)(inTokens, outDataPtr, inMessage);
	}

Boolean
	CommandLine_IsVisible					();

void
	CommandLine_Focus						();

HIWindowRef
	CommandLine_ReturnWindow				();

void
	CommandLine_SetText						(void const*						inDataPtr,
											 Size 								inSize);

void
	CommandLine_SetTextWithCFString			(CFStringRef						inString);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
