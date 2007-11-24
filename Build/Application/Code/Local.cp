/*###############################################################

	Local.cp
	
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

// standard-C includes
#include <cstdio>
#include <cstdlib>

// standard-C++ includes
#include <map>
#include <string>

// UNIX includes
//struct pthread_rwlock_t;
//struct pthread_rwlockattr_t;
extern "C"
{
#	include <errno.h>
#	include <fcntl.h>
#	include <grp.h>
#	include <netdb.h>
#	include <pthread.h>
#	include <pwd.h>
#	include <signal.h>
#	include <termios.h>
#	include <unistd.h>
#	include <netinet/in.h>
#	include <sys/ioctl.h>
#	include <sys/socket.h>
#	include <sys/stat.h>
#	include <sys/termios.h>
#	include <sys/ttydefaults.h>
#	include <sys/types.h>
#	include <sys/wait.h>
}

// library includes
#include <CarbonEventUtilities.template.h>
#include <Console.h>
#include <MemoryBlocks.h>

// MacTelnet includes
#include "AppResources.h"
#include "ConnectionData.h"
#include "ConstantsRegistry.h"
#include "DialogUtilities.h"
#include "Local.h"
#include "NetEvents.h"
#include "Network.h"
#include "Session.h"
#include "Terminal.h"



#pragma mark Constants

#define	BUFFSIZE	256	//!< theoretically, this should respect the userÕs Network Block Size preference

enum MyTTYState
{
	kMyTTYStateReset,
	kMyTTYStateRaw
};

#pragma mark Types

typedef PseudoTeletypewriterID		TTYMasterID;
typedef PseudoTeletypewriterID		TTYSlaveID;

/*!
Thread context passed to threadForLocalProcessDataLoop().
*/
struct DataLoopThreadContext
{
	EventQueueRef		eventQueue;
	SessionRef			session;
	TTYMasterID			masterTTY;
};
typedef DataLoopThreadContext*			DataLoopThreadContextPtr;
typedef DataLoopThreadContext const*	DataLoopThreadContextConstPtr;

typedef std::map< pid_t, PseudoTeletypewriterID >		UnixProcessIDToTTYMap;

#pragma mark Internal Method Prototypes

static void				fillInTerminalControlStructure		(struct termios*);
static pid_t			forkToNewTTY						(TTYMasterID*, char*, struct termios const*,
																struct winsize const*);
static void				handleTerminationSignal				(int);
static TTYMasterID		openMasterTeletypewriter			(char*);
static TTYSlaveID		openSlaveTeletypewriter				(char const*);
static void				printTerminalControlStructure		(struct termios const*);
static Local_Result		putTTYInOriginalMode				(PseudoTeletypewriterID);
static void				putTTYInOriginalModeAtExit			();
static Local_Result		putTTYInRawMode						(PseudoTeletypewriterID);
static Local_Result		sendTerminalResizeMessage			(PseudoTeletypewriterID, struct winsize const*);
static void*			threadForLocalProcessDataLoop		(void*);

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	struct termios		gCachedTerminalAttributes;
	MyTTYState			gTTYState = kMyTTYStateReset;
	Boolean				gInDebuggingMode = false;		//!< true if terminal I/O is possible for debugging
	
	//! used to help atexit() handlers know which terminal to touch
	UnixProcessIDToTTYMap&	gUnixProcessIDToTTYMap ()	{ static UnixProcessIDToTTYMap x; return x; }
}



#pragma mark Public Methods

/*!
Installs signal handlers that launch an external
crash-catching application if MacTelnet ever quits
unexpectedly.  The application asks the user to
report back debugging information.

\retval kLocal_ResultOK
always; currently, no other errors are defined

(3.0)
*/
Local_Result
Local_InstallCrashCatcher ()
{
	Local_Result	result = kLocal_ResultOK;
	
	
	if (Local_StandardInputIsATerminal())
	{
		// if the standard input stream is a terminal, then it is
		// likely that a debugger is being run; so, crash catching
		// should be disabled
		DebugStr("\pMacTelnet: Not installing crash-catcher because stdin is a TTY.");
		gInDebuggingMode = true;
	}
	else
	{
		// install signal handlers
		int const	signals[] = {
									// include common signals that suggest an unexpected quit has occurred
									SIGILL, SIGBUS, SIGSEGV
								};
		
		
		for (int s = signals[0]; s < STATIC_CAST(sizeof(signals) / sizeof(int), int); ++s)
		{
			if (SIG_ERR == signal(s, handleTerminationSignal))
			{
				// Error!  The crash catcher cannot be installed for this signal.
			}
		}
	}
	
	return result;
}// InstallCrashCatcher


/*!
Disables local echoing for a pseudo-terminal.
Returns any errors sent back from terminal
control routines.

(3.0)
*/
int
Local_DisableTerminalLocalEcho		(PseudoTeletypewriterID		inPseudoTerminalID)
{
	struct termios	stermios;
	int				result = 0;
	
	
	result = tcgetattr(inPseudoTerminalID, &stermios);
	if (0 == result)
	{
		stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
		stermios.c_oflag &= ~(ONLCR); // also disable mapping from newline to newline-carriage-return
		result = tcsetattr(inPseudoTerminalID, TCSANOW/* when to apply changes */, &stermios);
	}
	return result;
}// DisableTerminalLocalEcho


/*!
Returns the current Unix userÕs login shell preference,
as a pointer to an array of characters in C string format.

\retval kLocal_ResultOK
if the resultant string pointer is valid

\retval kLocal_ResultParameterError
if "outStringPtr" is nullptr

(3.0)
*/
Local_Result
Local_GetDefaultShell	(char**		outStringPtr)
{
	Local_Result	result = kLocal_ResultOK;
	
	
	if (nullptr == outStringPtr) result = kLocal_ResultParameterError;
	else
	{
		// normally the password file is used to find the userÕs shell;
		// but if that fails, the SHELL environment variable is a good bet
		struct passwd*		userInfoPtr = getpwuid(getuid());
		
		
		if (nullptr != userInfoPtr)
		{
			// grab the userÕs preferred shell from the password file
			*outStringPtr = userInfoPtr->pw_shell;
		}
		else
		{
			// revert to the $SHELL method, which usually works but is less reliable...
			*outStringPtr = getenv("SHELL");
		}
	}
	return result;
}// GetDefaultShell


/*!
Returns the resume-output character for a
pseudo-terminal (usually, control-Q).  Returns
-1 if any errors are sent back from terminal
control routines.

(3.1)
*/
int
Local_GetTerminalFlowStartCharacter		(PseudoTeletypewriterID		inPseudoTerminalID)
{
	struct termios	stermios;
	int				result = 0;
	int				error = 0;
	
	
	error = tcgetattr(inPseudoTerminalID, &stermios);
	if (0 == error)
	{
		// success!
		result = stermios.c_cc[VSTART];
	}
	else
	{
		// error
		result = -1;
	}
	
	return result;
}// GetTerminalFlowStartCharacter


/*!
Returns the suspend-output (scroll lock) character
for a pseudo-terminal (usually, control-S).  Returns
-1 if any errors are sent back from terminal control
routines.

(3.1)
*/
int
Local_GetTerminalFlowStopCharacter		(PseudoTeletypewriterID		inPseudoTerminalID)
{
	struct termios	stermios;
	int				result = 0;
	int				error = 0;
	
	
	error = tcgetattr(inPseudoTerminalID, &stermios);
	if (0 == error)
	{
		// success!
		result = stermios.c_cc[VSTOP];
	}
	else
	{
		// error
		result = -1;
	}
	
	return result;
}// GetTerminalFlowStopCharacter


/*!
Returns the interrupt-process character for a
pseudo-terminal (usually, control-C).  Returns
-1 if any errors are sent back from terminal
control routines.

(3.1)
*/
int
Local_GetTerminalInterruptCharacter		(PseudoTeletypewriterID		inPseudoTerminalID)
{
	struct termios	stermios;
	int				result = 0;
	int				error = 0;
	
	
	error = tcgetattr(inPseudoTerminalID, &stermios);
	if (0 == error)
	{
		// success!
		result = stermios.c_cc[VINTR];
	}
	else
	{
		// error
		result = -1;
	}
	
	return result;
}// GetTerminalInterruptCharacter


/*!
Kills the specified process, whose input and output
are being channeled through the given window.

The given process is not allowed to be less than or
equal to zero, as this has special significance to
the system (kills multiple processes).

(3.0)
*/
void
Local_KillProcess	(pid_t		inUnixProcessID)
{
	if (inUnixProcessID > 0)
	{
		int		killResult = kill(inUnixProcessID, SIGKILL);
		
		
		if (0 != killResult) Console_WriteValue("warning, unable to kill process: Unix error", errno);
	}
	else
	{
		Console_WriteValue("warning, attempt to run kill with special pid", inUnixProcessID);
	}
}// KillProcess


/*!
Sends a message to the specified TTY telling it
that the size of the terminal window has now
changed.  If you resize a terminal window in
which a Unix process is running but do not send
this message, then the process cannot make use
of the additional space in the window.

\retval kLocal_ResultOK
if the message is sent successfully

\retval kLocal_ResultIOControlError
if the message could not be sent

(3.0)
*/
Local_Result
Local_SendTerminalResizeMessage		(PseudoTeletypewriterID		inPseudoTerminalID,
									 UInt16						inNewColumnCount,
									 UInt16						inNewRowCount,
									 UInt16						inNewColumnWidthInPixels,
									 UInt16						inNewRowHeightInPixels)
{
	Local_Result		result = kLocal_ResultOK;
	struct winsize		unixTerminalWindowSizeStructure; // defined in "/usr/include/sys/ttycom.h"
	
	
	unixTerminalWindowSizeStructure.ws_col = inNewColumnCount;				// terminal width, in characters
	unixTerminalWindowSizeStructure.ws_row = inNewRowCount;					// terminal height, in lines
	unixTerminalWindowSizeStructure.ws_xpixel = inNewColumnWidthInPixels;	// terminal width, in pixels
	unixTerminalWindowSizeStructure.ws_ypixel = inNewRowHeightInPixels;		// terminal height, in pixels
	
	result = sendTerminalResizeMessage(inPseudoTerminalID, &unixTerminalWindowSizeStructure);
	return result;
}// SendTerminalResizeMessage


/*!
Calls Local_SpawnProcess() with the userÕs default
shell as the process (determined using getpwuid()
or falling back to the SHELL environment variable).

See the documentation on Local_SpawnProcess() for
more on how the working directory is handled.

\return any code that Local_SpawnProcess() can return

(3.0)
*/
Local_Result
Local_SpawnDefaultShell	(SessionRef			inUninitializedSession,
						 TerminalScreenRef	inContainer,
						 pid_t*				outProcessIDPtr,
						 char*				outSlaveName,
						 char const*		inWorkingDirectoryOrNull)
{
	struct passwd*	userInfoPtr = getpwuid(getuid());
	char const*		args[] = { nullptr, nullptr/* terminator */ };
	
	
	if (nullptr != userInfoPtr)
	{
		// grab the userÕs preferred shell from the password file
		args[0] = userInfoPtr->pw_shell;
	}
	else
	{
		// revert to the $SHELL method, which usually works but is less reliable...
		args[0] = getenv("SHELL");
	}
	
	return Local_SpawnProcess(inUninitializedSession, inContainer, args, outProcessIDPtr, outSlaveName,
								inWorkingDirectoryOrNull);
}// SpawnDefaultShell


/*!
Calls Local_SpawnProcess() with "/usr/bin/login",
which launches the default login shell.

See the documentation on Local_SpawnProcess() for
more on how the working directory is handled.

\return any code that Local_SpawnProcess() can return

(3.0)
*/
Local_Result
Local_SpawnLoginShell	(SessionRef			inUninitializedSession,
						 TerminalScreenRef	inContainer,
						 pid_t*				outProcessIDPtr,
						 char*				outSlaveName,
						 char const*		inWorkingDirectoryOrNull)
{
	char const*		args[] = { "/usr/bin/login", nullptr/* terminator */ };
	
	
	return Local_SpawnProcess(inUninitializedSession, inContainer, args, outProcessIDPtr, outSlaveName,
								inWorkingDirectoryOrNull);
}// SpawnLoginShell


/*!
Forks a new process and arranges for its output and
input to be channeled through the specified screen.
The Unix command line is defined using the given
argument vector, which must be terminated by a
nullptr entry in the array.

The specified sessionÕs data will be updated with
whatever information is available for the created
process.

The specified working directory is targeted only in
the child process, meaning the callerÕs working
directory is unchanged.  If it is not possible to
change to that directory, the child process is
aborted.

The Unix process ID and TTY name (e.g. "/dev/ttyp2")
are returned.  The buffer you provide for the slave
name should be sufficiently large (20 characters is
probably enough).

\retval kLocal_ResultOK
if the process was created successfully

\retval kLocal_ResultParameterError
if a storage variable is nullptr

\retval kLocal_ResultForkError
if the process cannot be spawned

\retval kLocal_ResultThreadError
if a thread cannot be created to read data

(3.0)
*/
Local_Result
Local_SpawnProcess	(SessionRef			inUninitializedSession,
					 TerminalScreenRef	inContainer,
					 char const* const	argv[],
					 pid_t*				outProcessIDPtr,
					 char*				outSlaveName,
					 char const*		inWorkingDirectoryOrNull)
{
	Local_Result	result = kLocal_ResultOK;
	
	
	if ((nullptr == outProcessIDPtr) || (nullptr == outSlaveName)) result = kLocal_ResultParameterError;
	else
	{
		TTYMasterID		masterTTY = 0;
		struct termios	terminalControl;
		
		
		// set the answer-back message; this value works, but eventually this
		// needs to get the string from a MacTelnet terminal preference
		setenv("TERM", "vt100", true/* overwrite */);
		
		// TEMPORARY - the UNIX structures are filled in with defaults that work,
		//             but eventually MacTelnet has to map user preferences, etc.
		//             to these so that they affect ÒlocalÓ terminals in the same
		//             way as they affect remote ones
		std::memset(&terminalControl, 0, sizeof(terminalControl));
		fillInTerminalControlStructure(&terminalControl); // TEMP
		if (gInDebuggingMode)
		{
			// in debugging mode, show the terminal configuration
			printTerminalControlStructure(&terminalControl);
		}
		
		// spawn a child process attached to a pseudo-terminal device; the child
		// will be used to run the shell, and the shellÕs I/O will be handled in
		// a separate preemptive thread by MacTelnetÕs awesome terminal emulator
		// and main event loop
		{
			struct winsize		terminalSize; // defined in "/usr/include/sys/ttycom.h"
			
			
			terminalSize.ws_col = Terminal_ReturnColumnCount(inContainer);
			terminalSize.ws_row = Terminal_ReturnRowCount(inContainer);
			
			// TEMPORARY; the TerminalView_GetTheoreticalScreenDimensions() API would be useful for this
			terminalSize.ws_xpixel = 0;	// terminal width, in pixels; UNKNOWN
			terminalSize.ws_ypixel = 0;	// terminal height, in pixels; UNKNOWN
			
			*outProcessIDPtr = forkToNewTTY(&masterTTY, outSlaveName, &terminalControl, &terminalSize);
		}
		
		if (-1 == *outProcessIDPtr) result = kLocal_ResultForkError;
		else
		{
			if (0 == *outProcessIDPtr)
			{
				//
				// this is executed inside the child process
				//
				
				// IMPORTANT: There are limitations on what a child process can do.
				// For example, global data and framework calls are generally unsafe.
				// See the "fork" man page for more information.
				
				char const*		targetDir = inWorkingDirectoryOrNull;
				
				
				// determine the directory to be in when the command is run
				if (nullptr == targetDir)
				{
					struct passwd*	userInfoPtr = getpwuid(getuid());
					
					
					if (nullptr != userInfoPtr)
					{
						targetDir = userInfoPtr->pw_dir;
					}
					else
					{
						// revert to the $HOME method, which usually works but is less reliable...
						targetDir = getenv("HOME");
					}
				}
				
				// set the current working directory...abort if this fails
				// because presumably it is important that a command not
				// start in the wrong directory
				if (0 != chdir(targetDir))
				{
					Console_WriteValueCString("Aborting, failed to chdir() to ", targetDir);
					abort();
				}
				
				// run a Unix terminal-based application program; this is accomplished
				// using an execvp() call, which DOES NOT RETURN UNLESS there is an
				// error; technically the return value is -1 on error, but really itÕs
				// a problem if any return value is received, so an abort() is done no
				// matter what (the abort() kills this child process, but not MacTelnet)
				{					
					// An execvp() accepts mutable arguments and the input is constant.
					// But the child process has a copy of the original data and the
					// data goes away after the execvp(), so a const_cast should be okay.
					char**		argvCopy = CONST_CAST(argv, char**);
					
					
					(int)execvp(argvCopy[0], argvCopy); // should not return
					abort(); // almost no chance this line will be run, but if it does, just kill the child process
				}
			}
			
			//
			// this is executed inside the parent process
			//
			
			// set userÕs TTY to raw mode
			if (kLocal_ResultOK != putTTYInRawMode(STDIN_FILENO)) { /* error */ }
			
			// reset userÕs TTY on exit
			if (-1 != atexit(putTTYInOriginalModeAtExit))
			{
				// insert TTY information into map so the atexit() handler can find it
				gUnixProcessIDToTTYMap()[getpid()] = STDIN_FILENO;
			}
			
			// start a thread for data processing so that MacTelnetÕs main event loop can still run
			{
				pthread_attr_t	attr;
				int				error = 0;
				
				
				error = pthread_attr_init(&attr);
				if (0 != error) result = kLocal_ResultThreadError;
				else
				{
					ConnectionDataPtr			connectionDataPtr = nullptr;
					DataLoopThreadContextPtr	threadContextPtr = nullptr;
					pthread_t					thread;
					
					
					connectionDataPtr = Session_ConnectionDataPtr(inUninitializedSession);
					
					// synchronize somewhat with terminal control structure
					connectionDataPtr->controlKey.interrupt = terminalControl.c_cc[VINTR];
					connectionDataPtr->controlKey.suspend = terminalControl.c_cc[VSTOP];
					connectionDataPtr->controlKey.resume = terminalControl.c_cc[VSTART];
					
					// temporary - session parameters are hacked in here
					connectionDataPtr->mainProcess.pseudoTerminal = masterTTY;
					connectionDataPtr->mainProcess.processID = *outProcessIDPtr;
					CPP_STD::strcpy(connectionDataPtr->mainProcess.devicePath, outSlaveName);
					{
						char*				buffer = nullptr;
						char const* const*	argumentPtr = argv;
						size_t				requiredSize = 0;
						
						
						// figure out how long the string needs to be, taking into account delimiting spaces
						while (nullptr != *argumentPtr)
						{
							requiredSize += (CPP_STD::strlen(*argumentPtr) + sizeof(char)/* room for space */);
							++argumentPtr;
						}
						
						// allocate space
						buffer = REINTERPRET_CAST(Memory_NewPtr(requiredSize), char*);
						if (nullptr != buffer)
						{
							char*	endPtr = buffer;
							
							
							// copy in all the arguments, each delimited by a space
							argumentPtr = argv;
							while (*argumentPtr)
							{
								CPP_STD::strcat(endPtr, *argumentPtr);
								endPtr += CPP_STD::strlen(*argumentPtr);
								CPP_STD::strcat(endPtr, " "); // delimit arguments
								++endPtr;
								++argumentPtr;
							}
						}
						
						// retain the buffer
						connectionDataPtr->mainProcess.commandLinePtr = buffer;
					}
					
					threadContextPtr = REINTERPRET_CAST(Memory_NewPtrInterruptSafe(sizeof(DataLoopThreadContext)),
														DataLoopThreadContextPtr);
					if (nullptr == threadContextPtr) result = kLocal_ResultThreadError;
					else
					{
						// set up context
						threadContextPtr->eventQueue = nullptr; // set inside the handler
						threadContextPtr->session = inUninitializedSession;
						threadContextPtr->masterTTY = masterTTY;
						
						// create thread
						error = pthread_create(&thread, &attr, threadForLocalProcessDataLoop, threadContextPtr);
						if (0 != error) result = kLocal_ResultThreadError;
					}
					
					// put the session in the initialized state, to indicate it is complete
					Session_SetState(inUninitializedSession, kSession_StateInitialized);
				}
			}
		}
	}
	
	// with the preemptive thread handling data transfer
	// to and from the process, return immediately
	return result;
}// SpawnProcess


/*!
A convenient way for other modules to call system()
without including any Unix headers.

\retval kLocal_ResultOK
if the process was created successfully

\retval kLocal_ResultParameterError
if the command is nullptr

\retval kLocal_ResultForkError
if the process cannot be spawned

(3.1)
*/
Local_Result
Local_SpawnProcessAndWaitForTermination		(char const*	inCommand)
{
	Local_Result	result = kLocal_ResultOK;
	
	
	if (nullptr == inCommand) result = kLocal_ResultParameterError;
	else
	{
		int		commandResult = system(inCommand);
		
		
		// the result will be 127 if a shell cannot be invoked;
		// any nonzero value is an error of some kind, as defined
		// by the shell that is run
		if (0 != commandResult)
		{
			result = kLocal_ResultForkError;
		}
	}
	
	return result;
}// SpawnProcessAndWaitForTermination


/*!
Returns "true" only if the application was run from
the command line (which presumably means it was run
from the gdb debugger).

(3.1)
*/
Boolean
Local_StandardInputIsATerminal ()
{
	return isatty(STDIN_FILENO);
}// StandardInputIsATerminal


/*!
Writes the specified data to the stream described by
the file descriptor.  Returns the number of bytes
actually written.

(3.0)
*/
ssize_t
Local_WriteBytes	(int			inFileDescriptor,
					 void const*	inBufferPtr,
					 size_t			inByteCount)
{
	char const*			ptr = nullptr;
	size_t				bytesLeft = 0;
	ssize_t				bytesWritten = 0;
	ssize_t				result = inByteCount;
	register SInt16		clogCount = 0;
	
	
	ptr = REINTERPRET_CAST(inBufferPtr, char const*); // use char*, pointer arithmetic doesnÕt work on void*
	bytesLeft = inByteCount;
	while (bytesLeft > 0)
	{
		if ((bytesWritten = write(inFileDescriptor, ptr, bytesLeft)) < 0)
		{
			// error
			result = bytesWritten;
			// NOTE: could examine "errno" here (see "man 2 write")...
			break;
		}
		else if (0 == bytesWritten)
		{
			// defend against tight loops by incrementing a
			// counter; if it expires after an arbitrary number
			// of iterations, force the loop to exit (returning
			// a nonzero remainder to the caller)
			if (++clogCount > 100/* arbitrary */) break;
		}
		else
		{
			bytesLeft -= bytesWritten;
			ptr += bytesWritten;
		}
	}
	return result;
}// WriteBytes


#pragma mark Internal Methods

/*!
Fills in a UNIX "termios" structure using information
that MacTelnet provides about the environment.  Valid
flag values are typically in "/usr/include/sys/termios.h"
and defaults are in "/usr/include/sys/ttydefaults.h".

(3.0)
*/
static void
fillInTerminalControlStructure	(struct termios*	outTerminalControlPtr)
{
	tcflag_t*	inputFlagsPtr = &outTerminalControlPtr->c_iflag;
	tcflag_t*	outputFlagsPtr = &outTerminalControlPtr->c_oflag;
	tcflag_t*	hardwareControlFlagsPtr = &outTerminalControlPtr->c_cflag;
	tcflag_t*	localFlagsPtr = &outTerminalControlPtr->c_lflag;
	cc_t*		controlCharacterArray = outTerminalControlPtr->c_cc;
	speed_t*	inputSpeedPtr = &outTerminalControlPtr->c_ispeed;
	speed_t*	outputSpeedPtr = &outTerminalControlPtr->c_ospeed;
	
	
	//
	// Set the same default values used by Mac OS XÕs Terminal.app for STDIN;
	// subtracting '@' from a capital letter gives its control character in
	// ASCII (e.g. 'D' - '@' yields control-D).
	//
	// On Mac OS X you can use "man termios" to find out what all of this does.
	//
	
	// Turn ON the following input mode flags:
	//
	// - BRKINT means that a sufficiently long string of zeroes will be
	//   interpreted automatically as an interrupt-process signal.
	//
	// - ICRNL is enabled so that carriage returns are mapped to newlines.
	//   This seems reasonable...
	//
	// - IXON is enabled so that suspend/resume is handled automatically.
	//
	// - IXANY is enabled so that a resume occurs when any character arrives.
	//
	// - IMAXBEL is enabled so that a full input queue will generate an
	//   audible output signal.
	*inputFlagsPtr = BRKINT | ICRNL | IXON | IXANY | IMAXBEL;
	
	// Turn ON the following output mode flags:
	//
	// - OPOST must be enabled for any other flags to have effect.
	//
	// - ONLCR is enabled so that newlines are translated to carriage returns
	//   and line-feeds.  NOTE that this may be a bad idea, as MacTelnet
	//   likes to do this at the Session level...TEMPORARY?
	*outputFlagsPtr = OPOST | ONLCR;
	
	// Turn ON the following control flags:
	//
	// - CS8 means characters are 8 bits, that is, no bits are masked.  NOTE
	//   that although MacTelnet has an option for 7-bit, this is handled
	//   elsewhere currently.  Perhaps performance benchmarks will show that
	//   it is better to set a flag here than to strip bits later, but there
	//   still needs to be a handler for remote sessions anyway, so it is
	//   also nice to have that code in one place.
	//
	// - CREAD is enabled so that characters are received.  One has to wonder
	//   why this bit even has to be set, it would seem to be the desirable
	//   default.
	//
	// - HUPCL is enabled so hangup is automatic when the last close occurs.
	*hardwareControlFlagsPtr = CS8 | CREAD | HUPCL;
	
	// Turn ON the following local mode flags:
	//
	// - ECHOKE causes kills to be echoed.
	//
	// - ECHOE causes erasures to be echoed.
	//
	// - ECHO causes input to be echoed.
	//
	// - ECHOCTL causes control characters to be displayed as ASCII, e.g. ^D
	//
	// - ISIG causes Interrupt, Suspend and Resume characters to be checked
	//   and, if received, the appropriate action automatically taken.  The
	//   characters for these are defined below.
	//
	// - IEXTEN causes implementation-defined functions to be recognized.
	//
	// - PENDIN causes pending input to be typed again.
	*localFlagsPtr = ECHOKE | ECHOE | ECHO | ECHOCTL | ISIG | IEXTEN | PENDIN;
	
	// Set the control characters.  Basically these are
	// set to the default values, except maybe what is
	// overridden by the userÕs preferences.
	controlCharacterArray[VEOF] = CEOF;
	controlCharacterArray[VEOL] = STATIC_CAST(255, cc_t);
	controlCharacterArray[VEOL2] = STATIC_CAST(255, cc_t);
	controlCharacterArray[VERASE] = STATIC_CAST(127/* del */, cc_t);
	controlCharacterArray[VWERASE] = CWERASE;
	controlCharacterArray[VKILL] = CKILL;
	controlCharacterArray[VREPRINT] = CREPRINT;
	controlCharacterArray[VINTR] = 'C' - '@'; // TEMPORARY - should set from MacTelnet preferences
	controlCharacterArray[VQUIT] = CQUIT;
	controlCharacterArray[VSUSP] = CSUSP;
	controlCharacterArray[VDSUSP] = CDSUSP;
	controlCharacterArray[VSTART] = 'Q' - '@'; // TEMPORARY - should set from MacTelnet preferences
	controlCharacterArray[VSTOP] = 'S' - '@'; // TEMPORARY - should set from MacTelnet preferences
	controlCharacterArray[VLNEXT] = CLNEXT;
	controlCharacterArray[VDISCARD] = CDISCARD;
	controlCharacterArray[VMIN] = CMIN;
	controlCharacterArray[VTIME] = CTIME;
	controlCharacterArray[VSTATUS] = STATIC_CAST(255, cc_t);
	
	// ItÕs hard to say what to put here...MacTelnet is not a
	// modem!  Oh well, 9600 baud is what Terminal.app uses...
	*inputSpeedPtr = B9600;
	*outputSpeedPtr = B9600;
}// fillInTerminalControlStructure


/*!
Forks a new process under UNIX, configuring the child
to direct all its output to a new pseudo-terminal
device.  If this succeeds, the process ID of the child
is returned (or 0, within the child process); a negative
value is returned on failure.

(3.0)
*/
static pid_t
forkToNewTTY	(TTYMasterID*			outMasterTTYPtr,
				 char*					outSlaveTeletypewriterName,
				 struct termios const*	inSlaveTerminalControlPtrOrNull,
				 struct winsize const*	inSlaveTerminalSizePtrOrNull)
{
	TTYMasterID		masterTTY = kInvalidPseudoTeletypewriterID;
	pid_t			result = -1;
	
	
	// create master and slave teletypewriters (represented by
	// file descriptors under UNIX); use the slave teletypewriter
	// to receive all output from the child process (it is expected
	// that the caller will "exec" something in the child)
	masterTTY = openMasterTeletypewriter(outSlaveTeletypewriterName);
	if (kInvalidPseudoTeletypewriterID != masterTTY)
	{
		TTYSlaveID		slaveTTY = kInvalidPseudoTeletypewriterID;
		
		
		// split into child and parent processes; 0 is returned
		// within the child, a positive number is returned within
		// the parent (the child processÕ ID), and a negative
		// number is returned if the fork fails
		result = fork();
		if (-1 != result)
		{
			// successful fork; define how parent and child behave
			if (0 == result)
			{
				//
				// this is executed inside the child process
				//
				
				if (-1 == setsid()) abort();
				
				// SVR4 acquires controlling terminal on open()
				slaveTTY = openSlaveTeletypewriter(outSlaveTeletypewriterName);
				if (slaveTTY >= 0) close(masterTTY); // child process no longer needs the master TTY
				
				// this is the BSD 4.4 way to acquire the controlling terminal
				if (-1 == ioctl(slaveTTY, TIOCSCTTY/* command */, NULL/* data */)) abort();
				
				// set slaveÕs terminal control information and terminal screen size
				if (nullptr != inSlaveTerminalControlPtrOrNull)
				{
					if (-1 == tcsetattr(slaveTTY, TCSANOW/* when to apply changes */, inSlaveTerminalControlPtrOrNull))
					{
						abort();
					}
				}
				if (nullptr != inSlaveTerminalSizePtrOrNull)
				{
					if (kLocal_ResultOK != sendTerminalResizeMessage
											(slaveTTY, inSlaveTerminalSizePtrOrNull))
					{
						abort();
					}
				}
				
				// make the slave terminal the standard input, output, and standard error of this process
				if (STDIN_FILENO != dup2(slaveTTY, STDIN_FILENO)) abort();
				if (STDOUT_FILENO != dup2(slaveTTY, STDOUT_FILENO)) abort();
				if (STDERR_FILENO != dup2(slaveTTY, STDERR_FILENO)) abort();
				if (slaveTTY > STDERR_FILENO) close(slaveTTY);
				result = 0; // return 0, this is what fork() normally does within its child
			}
			else
			{
				//
				// this is executed inside the parent process
				//
				
				*outMasterTTYPtr = masterTTY; // return master TTY ID
			}
		}
	}
	return result;
}// forkToNewTTY


#if TARGET_API_MAC_CARBON
/*!
Responds to a termination signal if MacTelnet
should happen to die unexpectedly.  A special
application is launched to tell the user what
happened, mention where a debugging log can be
found and request that the author(s) be told
about the problem.

This isnÕt really a local-shell routine, but
since itÕs a signal handler for Unix, itÕs
simpler to implement it here for now.

(3.0)
*/
static void
handleTerminationSignal		(int	inSignalNumber)
{
	(OSStatus)AppResources_LaunchCrashCatcher();
	
	// restore default handler, which should cause an exit
	if (SIG_ERR == signal(inSignalNumber, SIG_DFL))
	{
		// canÕt restore signal, just die
		exit(1);
	}
}// handleTerminationSignal
#endif


/*!
Finds an available master TTY for the specified
slave, or returns "kInvalidPseudoTeletypewriterID"
if no devices are available.

(3.0)
*/
static TTYMasterID
openMasterTeletypewriter	(char*		outSlaveTeletypewriterName)
{
	TTYMasterID		result = kInvalidPseudoTeletypewriterID;
	char*			ptr1 = nullptr;
	char*			ptr2 = nullptr;
	Boolean			doneSearch = false;
	
	
	strcpy(outSlaveTeletypewriterName, "/dev/ptyXY"); // careted indices into this array are referenced below
	                                 /*      ^  ^^ */
	                                 /* 0123456789 */
	
	// vary the name of the pseudo-teletypewriter until
	// an available device is found; the choices are
	// based on what appears in /dev on Mac OS X 10.1
	for (ptr1 = "pqrstuvwxyzPQRST"; ((!doneSearch) && (0 != *ptr1)); ++ptr1)
	{
		outSlaveTeletypewriterName[8] = *ptr1;
		for (ptr2 = "0123456789abcdef"; ((!doneSearch) && (0 != *ptr2)); ++ptr2)
		{
			outSlaveTeletypewriterName[9] = *ptr2;
			
			// try to open master
			result = open(outSlaveTeletypewriterName, O_RDWR);
			if (-1 != result)
			{
				outSlaveTeletypewriterName[5] = 't'; // change "pty" to "tty"
				doneSearch = true;
			}
			else
			{
				// only quit if there are no more devices available;
				// other errors might be avoided by trying another ÒptyÓ
				if (ENOENT == errno) doneSearch = true;
				result = kInvalidPseudoTeletypewriterID;
			}
		}
	}
	return result;
}// openMasterTeletypewriter


/*!
Opens a slave TTY with the given name, or returns
"kInvalidPseudoTeletypewriterID" if any problems
occur.

(3.0)
*/
static TTYSlaveID
openSlaveTeletypewriter		(char const*	inSlaveTeletypewriterName)
{
	TTYSlaveID		result = kInvalidPseudoTeletypewriterID;
	
	
	// chown/chmod calls may only be done by root
	{
		struct group const*		groupInfoPtr = nullptr;
		int						gid = 0;
		
		
		if (nullptr != (groupInfoPtr = getgrnam("tty"))) gid = groupInfoPtr->gr_gid;
		else gid = -1; // group "tty" is not in the group file
		
		chown(inSlaveTeletypewriterName, getuid(), gid);
		chmod(inSlaveTeletypewriterName, S_IRUSR | S_IWUSR | S_IWGRP);
	}
	
	result = open(inSlaveTeletypewriterName, O_RDWR);
	if (-1 == result) result = kInvalidPseudoTeletypewriterID;
	return result;
}// openSlaveTeletypewriter


/*!
For debugging - prints the data in a UNIX "termios" structure.

(3.0)
*/
static void
printTerminalControlStructure	(struct termios const*		inTerminalControlPtr)
{
	tcflag_t const*		inputFlagsPtr = &inTerminalControlPtr->c_iflag;
	tcflag_t const*		outputFlagsPtr = &inTerminalControlPtr->c_oflag;
	tcflag_t const*		hardwareControlFlagsPtr = &inTerminalControlPtr->c_cflag;
	tcflag_t const*		localFlagsPtr = &inTerminalControlPtr->c_lflag;
	cc_t const*			controlCharacterArray = inTerminalControlPtr->c_cc;
	speed_t const*		inputSpeedPtr = &inTerminalControlPtr->c_ispeed;
	speed_t const*		outputSpeedPtr = &inTerminalControlPtr->c_ospeed;
	
	
	Console_WriteValueBitFlags("input flags", *inputFlagsPtr);
	Console_WriteValueBitFlags("output flags", *outputFlagsPtr);
	Console_WriteValueBitFlags("hardware control flags", *hardwareControlFlagsPtr);
	Console_WriteValueBitFlags("local flags", *localFlagsPtr);
	Console_WriteValueCharacter("end-of-file character", controlCharacterArray[VEOF]);
	Console_WriteValueCharacter("end-of-line character", controlCharacterArray[VEOL]);
	Console_WriteValueCharacter("end-of-line character ext.", controlCharacterArray[VEOL2]);
	Console_WriteValueCharacter("erase character", controlCharacterArray[VERASE]);
	Console_WriteValueCharacter("erase character ext.", controlCharacterArray[VWERASE]);
	Console_WriteValueCharacter("kill character", controlCharacterArray[VKILL]);
	Console_WriteValueCharacter("reprint character", controlCharacterArray[VREPRINT]);
	Console_WriteValueCharacter("interrupt character", controlCharacterArray[VINTR]);
	Console_WriteValueCharacter("quit character", controlCharacterArray[VQUIT]);
	Console_WriteValueCharacter("suspend character", controlCharacterArray[VSUSP]);
	Console_WriteValueCharacter("suspend character ext.", controlCharacterArray[VDSUSP]);
	Console_WriteValueCharacter("start character", controlCharacterArray[VSTART]);
	Console_WriteValueCharacter("stop character", controlCharacterArray[VSTOP]);
	Console_WriteValueCharacter("next character ext.", controlCharacterArray[VLNEXT]);
	Console_WriteValueCharacter("discard character ext.", controlCharacterArray[VDISCARD]);
	Console_WriteValueCharacter("minimum character", controlCharacterArray[VMIN]);
	Console_WriteValueCharacter("time character", controlCharacterArray[VTIME]);
	Console_WriteValueCharacter("status character ext.", controlCharacterArray[VSTATUS]);
	Console_WriteValue("input speed", *inputSpeedPtr);
	Console_WriteValue("output speed", *outputSpeedPtr);
}// printTerminalControlStructure


/*!
Puts a TTY in whichever mode it was in prior to being
put in raw mode with putTTYInRawMode().  This is a
good thing to do when a process exits.

\retval kLocal_ResultOK
if the process was created successfully

\retval kLocal_ResultTermCapError
if the terminal attributes cannot be changed

(3.0)
*/
static Local_Result
putTTYInOriginalMode	(PseudoTeletypewriterID		inTTY)
{
	Local_Result	result = kLocal_ResultOK;
	
	
	if (kMyTTYStateRaw == gTTYState)
	{
		if (-1 == tcsetattr(inTTY, TCSAFLUSH/* when to apply changes */, &gCachedTerminalAttributes))
		{
			// error
			result = kLocal_ResultTermCapError;
		}
		else
		{
			// success!
			gTTYState = kMyTTYStateReset;
		}
	}
	
	return result;
}// putTTYInOriginalMode


/*!
A standard atexit() handler; figures out the TTY
for the current process and resets it to whatever
state it was in originally, before being put in
Òraw modeÓ by MacTelnet.

(3.0)
*/
void
putTTYInOriginalModeAtExit ()
{
	pid_t								currentProcessID = getpid();
	UnixProcessIDToTTYMap::iterator		unixProcessIDToTTYIterator = gUnixProcessIDToTTYMap().find(currentProcessID);
	
	
	if (gUnixProcessIDToTTYMap().end() != unixProcessIDToTTYIterator)
	{
		PseudoTeletypewriterID const	terminalID = unixProcessIDToTTYIterator->first;
		
		
		if (terminalID >= 0)
		{
			(Local_Result)putTTYInOriginalMode(terminalID);
		}
		gUnixProcessIDToTTYMap().erase(unixProcessIDToTTYIterator);
	}
}// putTTYInOriginalModeAtExit


/*!
Puts a TTY in raw mode, given its file descriptor.
Basically, the idea is to set up the terminal device
so that data is largely untouched; appropriate when
reading data from a process, but NOT generally good
for reading input from the user.

The routine fillInTerminalControlStructure() makes
different "termios" settings appropriate for user
input.

\retval kLocal_ResultOK
if the process was created successfully

\retval kLocal_ResultTermCapError
if the terminal attributes cannot be retrieved
or if they cannot be modified

(3.0)
*/
static Local_Result
putTTYInRawMode		(PseudoTeletypewriterID		inTTY)
{
	Local_Result		result = kLocal_ResultOK;
	struct termios		terminalControl;
	
	
	// cache the current device attributes, they will be restored when the process dies
	if (-1 == tcgetattr(inTTY, &gCachedTerminalAttributes)) result = kLocal_ResultTermCapError;
	else
	{
		//
		// On Mac OS X you can use "man termios" to find out a lot
		// of information about what all of this does.
		//
		
		// start with whatever attributes the TTY has already
		terminalControl = gCachedTerminalAttributes;
		
		// Turn OFF the following local mode flags:
		//
		// - ECHO is disabled for now, but technically this should come
		//   from a Terminal Favorite or some other source of preferences.
		//
		// - ICANON is disabled so that read requests come from the input
		//   queue directly.
		//
		// - IEXTEN is disabled as implementation-defined characters
		//   (Òextended inputÓ) are not recognized.
		//
		// - ISIG is disabled so Interrupt, Suspend and Resume do not have
		//   special meaning when not input by the user.
		terminalControl.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
		
		// Turn OFF the following input mode flags:
		//
		// - BRKINT is disabled so that a long sequence of zeroes does not
		//   interrupt a process.
		//
		// - ICRNL is disabled so that carriage return characters are not
		//   turned into newlines.
		//
		// - INPCK is disabled, parity errors are not checked.
		//
		// - ISTRIP is disabled so that all 8 bits are kept around in case
		//   MacTelnet processes them.  The trouble is, this is a user
		//   option, and it *may* be more efficient to strip bits by setting
		//   the flag here than doing it later.  After doing some benchmarks
		//   this flag might be conditionally cleared here instead of
		//   stripping the high bit later on.
		//
		// - IXON is disabled so that a scroll lock cannot occur as a result
		//   of data not input by the user.
		terminalControl.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
		
		// Turn OFF the following control mode flags:
		//
		// - CSIZE is cleared so that none of the CS5, CS6, CS7 or CS8 bits
		//   are set.  Then, below, the appropriate CSx bit is set.
		//
		// - PARENB is disabled so that parity bits are not added.
		terminalControl.c_cflag &= ~(CSIZE | PARENB);
		
		// Turn ON the following control mode flags:
		//
		// - CS8 is enabled to indicate that characters are 8 bits each.
		//   What this really means is that no bits are masked, which is
		//   another way of saying ÒdonÕt mess with the data in raw modeÓ.
		terminalControl.c_cflag |= (CS8);
		
		// Turn OFF the following output mode flags:
		//
		// - OPOST is disabled, meaning nothing special happens to data that
		//   is output.  If this were set, it would be possible to do things
		//   like map carriage returns and tabs - again, raw mode is not for
		//   user input, so this isnÕt appropriate.
		terminalControl.c_oflag &= ~(OPOST);
		
		// Since ICANON is disabled, above, these must be set.  The default
		// values are used.
		terminalControl.c_cc[VMIN] = CMIN;
		terminalControl.c_cc[VTIME] = CTIME;
		
		// Now update the pseudo-terminal device.
		if (-1 == tcsetattr(inTTY, TCSAFLUSH/* when to apply changes */, &terminalControl))
		{
			result = kLocal_ResultTermCapError;
		}
		else
		{
			// success!
			gTTYState = kMyTTYStateRaw;
		}
		
		if (gInDebuggingMode)
		{
			// in debugging mode, show the terminal configuration
			Console_WriteLine("printing raw-mode terminal configuration");
			printTerminalControlStructure(&terminalControl);
		}
	}
	
	return result;
}// putTTYInRawMode


/*!
Internal version of Local_SendTerminalResizeMessage().

\retval kLocal_ResultOK
if the message is sent successfully

\retval kLocal_ResultIOControlError
if the message could not be sent

(3.0)
*/
static Local_Result
sendTerminalResizeMessage   (PseudoTeletypewriterID		inTTY,
							 struct winsize const*		inTerminalSizePtr)
{
	Local_Result	result = kLocal_ResultOK;
	
	
	if (-1 == ioctl(inTTY, TIOCSWINSZ/* command */, inTerminalSizePtr))
	{
		result = kLocal_ResultIOControlError;
	}
	return result;
}// sendTerminalResizeMessage


/*!
A POSIX thread (which can be preempted) that handles
the data processing loop for a particular pseudo-
terminal device.  Using preemptive threads for this
allows MacTelnet to ÒblockÓ waiting for data, without
actually halting other important things like the main
event loop!

WARNING:	As this is a preemptable thread, you MUST
			NOT use thread-unsafe system calls here.
			On the other hand, you can arrange for
			events to be handled (e.g. with Carbon
			Events).

(3.0)
*/
static void*
threadForLocalProcessDataLoop	(void*		inDataLoopThreadContextPtr)
{
	DataLoopThreadContextPtr	contextPtr = REINTERPRET_CAST(inDataLoopThreadContextPtr, DataLoopThreadContextPtr);
	size_t						numberOfBytesRead = 0;
	size_t						numberOfBytesToProcess = 0;
	char						buffer[BUFFSIZE];
	void* const					kBufferAddress = buffer;
	OSStatus					error = noErr;
	
	
	// arrange to communicate with the main application thread
	contextPtr->eventQueue = GetCurrentEventQueue();
	assert(contextPtr->eventQueue != GetMainEventQueue());
	
	for (;;)
	{
		assert(numberOfBytesToProcess <= sizeof(buffer));
		
		// each time through the loop, read a bit more data from the
		// pseudo-terminal device, up to the maximum limit of the buffer;
		// the effective buffer size is even smaller if bytes remain
		// unprocessed following the previous read() attempt
		numberOfBytesRead = read(contextPtr->masterTTY, buffer + numberOfBytesToProcess,
									sizeof(buffer) - numberOfBytesToProcess * sizeof(char));
		if (numberOfBytesRead <= 0)
		{
			// error or EOF
			break;
		}
		else
		{
			Session_Result		postingResult = kSession_ResultOK;
			
			
			//
			// Insert the data into the processing buffer.  For thread safety,
			// this is done indirectly by posting a session-data-arrived event
			// to the main queue.  To do otherwise risks a pseudo-random crash
			// because the vast majority of Mac OS APIs are NOT thread-safe.
			// (The exception apparently is Carbon Events.)
			//
			// After sending a Òplease process this dataÓ event, this thread
			// blocks until a go-ahead response is received from the main queue.
			// The go-ahead event also returns the number of unprocessed bytes,
			// which will affect the size of the buffer available the next time
			// through the processing loop.
			//
			// WARNING:	To simplify the code below, certain assumptions are
			//			made.  One, that the current thread serves exactly one
			//			session, "contextPtr->session".  Two, that no one will
			//			ever send Òdata processedÓ events to this thread except
			//			for the purpose of continuing this loop.  Three, that
			//			the thread must not terminate while events it sends are
			//			being handled in the main thread - allowing addresses of
			//			data in this thread to be passed in.  Violate any of the
			//			assumptions, and I think youÕll deserve the outcome.
			//
			
			// adjust the total number of bytes remaining to be processed
			numberOfBytesToProcess += numberOfBytesRead;
			
			// notify that data has arrived
			postingResult = Session_PostDataArrivedEventToMainQueue(contextPtr->session, kBufferAddress, numberOfBytesToProcess,
																	kEventPriorityStandard, contextPtr->eventQueue);
			assert(kSession_ResultOK == postingResult);
			{
				// now block until the data processing has completed
				EventRef				dataProcessedEvent = nullptr;
				EventTypeSpec const		whenDataProcessingCompletes[] =
										{
											{ kEventClassNetEvents_Session, kEventNetEvents_SessionDataProcessed }
										};
				
				
				error = ReceiveNextEvent(GetEventTypeCount(whenDataProcessingCompletes),
											whenDataProcessingCompletes, kEventDurationForever,
											true/* pull event from queue */, &dataProcessedEvent);
				if (noErr == error)
				{
					// now determine how much data was not yet processed
					UInt32		unprocessedDataSize = 0L;
					
					
					assert(kEventClassNetEvents_Session == GetEventClass(dataProcessedEvent));
					assert(kEventNetEvents_SessionDataProcessed == GetEventKind(dataProcessedEvent));
					
					// retrieve the number of unprocessed bytes
					error = CarbonEventUtilities_GetEventParameter
							(dataProcessedEvent, kEventParamNetEvents_SessionDataSize,
								typeUInt32, unprocessedDataSize);
					if (noErr == error)
					{
						numberOfBytesToProcess = unprocessedDataSize;
					}
				}
				if (nullptr != dataProcessedEvent) ReleaseEvent(dataProcessedEvent), dataProcessedEvent = nullptr;
			}
		}
	}
	
	// loop terminated, ensure TTY is closed
	close(contextPtr->masterTTY);
	
	// update the state; for thread safety, this is done indirectly
	// by posting a session-state-update event to the main queue
	{
		EventRef				setStateEvent = nullptr;
		Session_State const		kNewState = kSession_StateDead;
		
		
		// create a Carbon Event
		error = CreateEvent(nullptr/* allocator */, kEventClassNetEvents_Session,
							kEventNetEvents_SessionSetState, GetCurrentEventTime(),
							kEventAttributeNone, &setStateEvent);
		
		// attach required parameters to event, then dispatch it
		if (noErr != error) setStateEvent = nullptr;
		else
		{
			// specify the session whose state is changing
			error = SetEventParameter(setStateEvent, kEventParamNetEvents_DirectSession, typeNetEvents_SessionRef,
										sizeof(contextPtr->session), &contextPtr->session);
			if (noErr == error)
			{
				// specify the new state
				error = SetEventParameter(setStateEvent, kEventParamNetEvents_NewSessionState,
											typeNetEvents_SessionState, sizeof(kNewState), &kNewState);
				if (noErr == error)
				{
					// specify the event queue that should receive event replies;
					// ignore the following error because the dispatching queue
					// is not critical for this particular event to succeed (and
					// in fact any replies would be ignored)
					(OSStatus)SetEventParameter(setStateEvent, kEventParamNetEvents_DispatcherQueue,
												typeNetEvents_EventQueueRef, sizeof(contextPtr->eventQueue),
												&contextPtr->eventQueue);
					
					// finally, send the message to the main event loop
					error = PostEventToQueue(GetMainEventQueue(), setStateEvent, kEventPriorityStandard);
				}
			}
		}
		
		// dispose of event
		if (nullptr != setStateEvent) ReleaseEvent(setStateEvent), setStateEvent = nullptr;
	}
	
	// since the thread is finished, the context should be disposed of
	Memory_DisposePtrInterruptSafe(REINTERPRET_CAST(&contextPtr, void**));
	
	return nullptr;
}// threadForLocalProcessDataLoop

// BELOW IS REQUIRED NEWLINE TO END FILE
