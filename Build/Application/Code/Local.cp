/*###############################################################

	Local.cp
	
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
#include <MemoryBlockPtrLocker.template.h>
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
namespace {

enum MyTTYState
{
	kMyTTYStateReset,
	kMyTTYStateRaw
};

} // anonymous namespace

#pragma mark Types
namespace {

typedef Local_TerminalID		My_TTYMasterID;
typedef Local_TerminalID		My_TTYSlaveID;

typedef std::map< pid_t, Local_TerminalID >	My_UnixProcessIDToTTYMap;

/*!
Thread context passed to threadForLocalProcessDataLoop().
*/
struct My_DataLoopThreadContext
{
	EventQueueRef		eventQueue;
	SessionRef			session;
	My_TTYMasterID		masterTTY;
};
typedef My_DataLoopThreadContext*			My_DataLoopThreadContextPtr;
typedef My_DataLoopThreadContext const*		My_DataLoopThreadContextConstPtr;

/*!
Information retained about a new process.  Known externally
as a Local_ProcessRef.
*/
struct My_Process
{
	My_Process	(int, char const* const[], Local_TerminalID, char const*, pid_t);
	~My_Process	();
	
	char const*
	createCommandLine	(int, char const* const[]);
	
	pid_t				_processID;			// the process directly spawned by this session
	Local_TerminalID	_pseudoTerminal;	// file descriptor of pseudo-terminal master
	char const*			_slaveDeviceName;	// e.g. "/dev/ttyp0", data sent here goes to the terminal emulator (not the process)
	char const*			_commandLine;		// buffer for parent process’ command line
};
typedef My_Process*			My_ProcessPtr;
typedef My_Process const*	My_ProcessConstPtr;

typedef MemoryBlockPtrLocker< Local_ProcessRef, My_Process >	My_ProcessPtrLocker;
typedef LockAcquireRelease< Local_ProcessRef, My_Process >		My_ProcessAutoLocker;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void			fillInTerminalControlStructure		(struct termios*);
pid_t			forkToNewTTY						(My_TTYMasterID*, char*, size_t,
													 struct termios const*, struct winsize const*);
Local_Result	openMasterTeletypewriter			(size_t, char*, My_TTYMasterID&);
My_TTYSlaveID	openSlaveTeletypewriter				(char const*);
void			printTerminalControlStructure		(struct termios const*);
Local_Result	putTTYInOriginalMode				(Local_TerminalID);
void			putTTYInOriginalModeAtExit			();
Local_Result	putTTYInRawMode						(Local_TerminalID);
Local_Result	sendTerminalResizeMessage			(Local_TerminalID, struct winsize const*);
void*			threadForLocalProcessDataLoop		(void*);

} // anonymous namespace

#pragma mark Variables
namespace {

My_ProcessPtrLocker&		gProcessPtrLocks ()		{ static My_ProcessPtrLocker x; return x; }
struct termios				gCachedTerminalAttributes;
MyTTYState					gTTYState = kMyTTYStateReset;
Boolean						gInDebuggingMode = Local_StandardInputIsATerminal();	//!< true if terminal I/O is possible for debugging

//! used to help atexit() handlers know which terminal to touch
My_UnixProcessIDToTTYMap&	gUnixProcessIDToTTYMap ()	{ static My_UnixProcessIDToTTYMap x; return x; }

} // anonymous namespace



#pragma mark Public Methods

/*!
Disables local echoing for a pseudo-terminal.
Returns any errors sent back from terminal
control routines.

(3.0)
*/
int
Local_DisableTerminalLocalEcho		(Local_TerminalID		inPseudoTerminalID)
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
Returns the current Unix user’s login shell preference,
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
		// normally the password file is used to find the user’s shell;
		// but if that fails, the SHELL environment variable is a good bet
		struct passwd*		userInfoPtr = getpwuid(getuid());
		
		
		if (nullptr != userInfoPtr)
		{
			// grab the user’s preferred shell from the password file
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
Kills the specified process, disposes of the data and
sets your copy of the reference to nullptr.

The given process’ ID is not allowed to be less than
or equal to zero, as this has special significance
to the system (kills multiple processes).

(3.1)
*/
void
Local_KillProcess	(Local_ProcessRef*	inoutRefPtr)
{
	// clean up
	{
		My_ProcessAutoLocker	ptr(gProcessPtrLocks(), *inoutRefPtr);
		
		
		if (nullptr != ptr)
		{
			if (ptr->_processID > 0)
			{
				int		killResult = kill(ptr->_processID, SIGKILL);
				
				
				if (0 != killResult) Console_WriteValue("warning, unable to kill process: Unix error", errno);
				else
				{
					// successful kill, ID is no longer valid
					ptr->_processID = -1;
				}
			}
			else
			{
				Console_WriteValue("warning, attempt to run kill with special pid", ptr->_processID);
			}
		}
	}
	
	if (gProcessPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_WriteValue("warning, attempt to dispose of locked process data; outstanding locks",
							gProcessPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_ProcessPtr*)), *inoutRefPtr = nullptr;
	}
}// KillProcess


/*!
Returns the program and its given arguments as a string,
suitable for display but not advisable for execution (e.g.
whitespace in original arguments may no longer be obvious).

(3.1)
*/
char const*
Local_ProcessReturnCommandLineString	(Local_ProcessRef	inProcess)
{
	My_ProcessAutoLocker	ptr(gProcessPtrLocks(), inProcess);
	char const*				result = nullptr;
	
	
	result = ptr->_commandLine;
	return result;
}// ProcessReturnCommandLineString


/*!
Returns the file descriptor of the pseudo-terminal device that
is the master.  Data sent to this device will interact directly
with the running process, and have only indirect effects on any
slave terminal.

(3.1)
*/
Local_TerminalID
Local_ProcessReturnMasterTerminal	(Local_ProcessRef	inProcess)
{
	My_ProcessAutoLocker	ptr(gProcessPtrLocks(), inProcess);
	Local_TerminalID		result = kLocal_InvalidTerminalID;
	
	
	result = ptr->_pseudoTerminal;
	return result;
}// ProcessReturnMasterTerminal


/*!
Returns the name of the pseudo-terminal device ("/dev/ttyp0",
for example) that is connected as a slave.  Data sent to this
device will interact directly with the terminal, and not affect
the process.

(3.1)
*/
char const*
Local_ProcessReturnSlaveDeviceName		(Local_ProcessRef	inProcess)
{
	My_ProcessAutoLocker	ptr(gProcessPtrLocks(), inProcess);
	char const*				result = nullptr;
	
	
	result = ptr->_slaveDeviceName;
	return result;
}// ProcessReturnSlaveDeviceName


/*!
Returns the actual process ID, useful when making system calls
directly.

(3.1)
*/
pid_t
Local_ProcessReturnUnixID	(Local_ProcessRef	inProcess)
{
	My_ProcessAutoLocker	ptr(gProcessPtrLocks(), inProcess);
	pid_t					result = -1;
	
	
	result = ptr->_processID;
	return result;
}// ProcessReturnUnixID


/*!
Returns the resume-output character for a
pseudo-terminal (usually, control-Q).  Returns
-1 if any errors are sent back from terminal
control routines.

(3.1)
*/
int
Local_ReturnTerminalFlowStartCharacter	(Local_TerminalID		inPseudoTerminalID)
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
}// ReturnTerminalFlowStartCharacter


/*!
Returns the suspend-output (scroll lock) character
for a pseudo-terminal (usually, control-S).  Returns
-1 if any errors are sent back from terminal control
routines.

(3.1)
*/
int
Local_ReturnTerminalFlowStopCharacter	(Local_TerminalID		inPseudoTerminalID)
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
}// ReturnTerminalFlowStopCharacter


/*!
Returns the interrupt-process character for a
pseudo-terminal (usually, control-C).  Returns
-1 if any errors are sent back from terminal
control routines.

(3.1)
*/
int
Local_ReturnTerminalInterruptCharacter	(Local_TerminalID		inPseudoTerminalID)
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
}// ReturnTerminalInterruptCharacter


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
Local_SendTerminalResizeMessage		(Local_TerminalID	inPseudoTerminalID,
									 UInt16				inNewColumnCount,
									 UInt16				inNewRowCount,
									 UInt16				inNewColumnWidthInPixels,
									 UInt16				inNewRowHeightInPixels)
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
Calls Local_SpawnProcess() with the user’s default
shell as the process (determined using getpwuid()
or falling back to the SHELL environment variable).

See the documentation on Local_SpawnProcess() for
more on how the working directory is handled.

\retval any code that Local_SpawnProcess() can return

(3.0)
*/
Local_Result
Local_SpawnDefaultShell	(SessionRef			inUninitializedSession,
						 TerminalScreenRef	inContainer,
						 char const*		inWorkingDirectoryOrNull)
{
	struct passwd*	userInfoPtr = getpwuid(getuid());
	char const*		args[] = { nullptr, nullptr/* terminator */ };
	
	
	if (nullptr != userInfoPtr)
	{
		// grab the user’s preferred shell from the password file
		args[0] = userInfoPtr->pw_shell;
	}
	else
	{
		// revert to the $SHELL method, which usually works but is less reliable...
		args[0] = getenv("SHELL");
	}
	
	return Local_SpawnProcess(inUninitializedSession, inContainer, args, inWorkingDirectoryOrNull);
}// SpawnDefaultShell


/*!
Calls Local_SpawnProcess() with "/usr/bin/login -p -f $USER",
which launches the default login shell without requiring
a password.

See the documentation on Local_SpawnProcess() for more on
how the working directory is handled.

\retval any code that Local_SpawnProcess() can return

(3.0)
*/
Local_Result
Local_SpawnLoginShell	(SessionRef			inUninitializedSession,
						 TerminalScreenRef	inContainer,
						 char const*		inWorkingDirectoryOrNull)
{
	char const*		args[] = { "/usr/bin/login", "-p", "-f", getenv("USER"), nullptr/* terminator */ };
	
	
	return Local_SpawnProcess(inUninitializedSession, inContainer, args, inWorkingDirectoryOrNull);
}// SpawnLoginShell


/*!
Forks a new process and arranges for its output and
input to be channeled through the specified screen.
The Unix command line is defined using the given
argument vector, which must be terminated by a
nullptr entry in the array.

The specified session’s data will be updated with
whatever information is available for the created
process.

The specified working directory is targeted only in
the child process, meaning the caller’s working
directory is unchanged.  If it is not possible to
change to that directory, the child process is
aborted.

\retval kLocal_ResultOK
if the process was created successfully

\retval kLocal_ResultParameterError
if a storage variable is nullptr or argv is not terminated

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
					 char const*		inWorkingDirectoryOrNull)
{
	Local_Result		result = kLocal_ResultOK;
	My_TTYMasterID		masterTTY = 0;
	char				slaveDeviceName[20/* arbitrary */];
	pid_t				processID = -1;
	size_t				argc = 0;
	struct termios		terminalControl;
	
	
	// figure out the size of the argument list
	{
		char const* const*		argPtr = argv;
		
		
		while (nullptr != *argPtr)
		{
			++argc;
			++argPtr;
		}
	}
	
	// set the answer-back message
	{
		CFStringRef		answerBackCFString = Terminal_EmulatorReturnName(inContainer);
		
		
		if (nullptr != answerBackCFString)
		{
			size_t const	kAnswerBackSize = CFStringGetLength(answerBackCFString) + 1/* terminator */;
			char*			answerBackCString = new char[kAnswerBackSize];
			
			
			if (CFStringGetCString(answerBackCFString, answerBackCString, kAnswerBackSize, kCFStringEncodingASCII))
			{
				(int)setenv("TERM", answerBackCString, true/* overwrite */);
			}
			delete [] answerBackCString;
		}
	}
	
	// TEMPORARY - the UNIX structures are filled in with defaults that work,
	//             but eventually MacTelnet has to map user preferences, etc.
	//             to these so that they affect “local” terminals in the same
	//             way as they affect remote ones
	std::memset(&terminalControl, 0, sizeof(terminalControl));
	fillInTerminalControlStructure(&terminalControl); // TEMP
	if (gInDebuggingMode)
	{
		// in debugging mode, show the terminal configuration
		printTerminalControlStructure(&terminalControl);
	}
	
	// spawn a child process attached to a pseudo-terminal device; the child
	// will be used to run the shell, and the shell’s I/O will be handled in
	// a separate preemptive thread by MacTelnet’s awesome terminal emulator
	// and main event loop
	{
		struct winsize		terminalSize; // defined in "/usr/include/sys/ttycom.h"
		
		
		terminalSize.ws_col = Terminal_ReturnColumnCount(inContainer);
		terminalSize.ws_row = Terminal_ReturnRowCount(inContainer);
		
		// TEMPORARY; the TerminalView_GetTheoreticalScreenDimensions() API would be useful for this
		terminalSize.ws_xpixel = 0;	// terminal width, in pixels; UNKNOWN
		terminalSize.ws_ypixel = 0;	// terminal height, in pixels; UNKNOWN
		
		processID = forkToNewTTY(&masterTTY, slaveDeviceName, sizeof(slaveDeviceName),
									&terminalControl, &terminalSize);
	}
	
	if (-1 == processID) result = kLocal_ResultForkError;
	else
	{
		if (0 == processID)
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
			// error; technically the return value is -1 on error, but really it’s
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
		
		// set user’s TTY to raw mode
		if (kLocal_ResultOK != putTTYInRawMode(STDIN_FILENO))
		{
			Console_WriteLine("warning, error entering TTY raw-mode");
		}
		
		// reset user’s TTY on exit
		if (-1 != atexit(putTTYInOriginalModeAtExit))
		{
			// insert TTY information into map so the atexit() handler can find it
			gUnixProcessIDToTTYMap()[getpid()] = STDIN_FILENO;
		}
		
		// start a thread for data processing so that MacTelnet’s main event loop can still run
		{
			pthread_attr_t	attr;
			int				error = 0;
			
			
			error = pthread_attr_init(&attr);
			if (0 != error) result = kLocal_ResultThreadError;
			else
			{
				ConnectionDataPtr				connectionDataPtr = nullptr;
				My_DataLoopThreadContextPtr		threadContextPtr = nullptr;
				pthread_t						thread;
				
				
				connectionDataPtr = Session_ConnectionDataPtr(inUninitializedSession);
				
				// synchronize somewhat with terminal control structure
				connectionDataPtr->controlKey.interrupt = terminalControl.c_cc[VINTR];
				connectionDataPtr->controlKey.suspend = terminalControl.c_cc[VSTOP];
				connectionDataPtr->controlKey.resume = terminalControl.c_cc[VSTART];
				
				// store process information for session
				{
					Local_ProcessRef	newProcess = nullptr;
					
					
					newProcess = REINTERPRET_CAST(new My_Process(argc, argv, masterTTY, slaveDeviceName, processID),
													Local_ProcessRef);
					Session_SetProcess(inUninitializedSession, newProcess);
				}
				threadContextPtr = REINTERPRET_CAST(Memory_NewPtrInterruptSafe(sizeof(My_DataLoopThreadContext)),
													My_DataLoopThreadContextPtr);
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
	
	
	ptr = REINTERPRET_CAST(inBufferPtr, char const*); // use char*, pointer arithmetic doesn’t work on void*
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
namespace {

My_Process::
My_Process	(int				argc,
			 char const* const	argv[],
			 Local_TerminalID	inMasterTerminal,
			 char const*		inSlaveDeviceName,
			 pid_t				inProcessID)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
_processID(inProcessID),
_pseudoTerminal(inMasterTerminal),
_slaveDeviceName(inSlaveDeviceName),
_commandLine(createCommandLine(argc, argv))
{
}// My_Process constructor


My_Process::
~My_Process ()
{
	if (nullptr != _commandLine) delete [] _commandLine, _commandLine = nullptr;
}// My_Process destructor


char const*
My_Process::
createCommandLine	(int				argc,
					 char const* const	argv[])
{
	char const* const	kDelimiter = " ";
	size_t const		kDelimiterSize = CPP_STD::strlen(kDelimiter);
	char const* const*	argumentPtr = argv;
	size_t				requiredSize = 0;
	char*				result = nullptr;
	
	
	// figure out how long the string needs to be, taking into account delimiting spaces
	for (int i = 0; i < argc; ++i)
	{
		requiredSize += (CPP_STD::strlen(*argumentPtr) + kDelimiterSize);
		++argumentPtr;
	}
	++requiredSize; // space for terminator
	
	// allocate space
	result = new char[requiredSize];
	if (nullptr != result)
	{
		char*	endPtr = result;
		
		
		// copy in all the arguments, each delimited by a space
		argumentPtr = argv;
		*endPtr = '\0';
		for (int i = 0; i < argc; ++i, ++endPtr, ++argumentPtr)
		{
			CPP_STD::strcat(endPtr, *argumentPtr);
			endPtr += CPP_STD::strlen(*argumentPtr);
			if (i != (argc - 1)) CPP_STD::strcat(endPtr, kDelimiter);
		}
	}
	
	return result;
}// createCommandLine


/*!
Fills in a UNIX "termios" structure using information
that MacTelnet provides about the environment.  Valid
flag values are typically in "/usr/include/sys/termios.h"
and defaults are in "/usr/include/sys/ttydefaults.h".

(3.0)
*/
void
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
	// Set the same default values used by Mac OS X’s Terminal.app for STDIN;
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
	// overridden by the user’s preferences.
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
	
	// It’s hard to say what to put here...MacTelnet is not a
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
pid_t
forkToNewTTY	(My_TTYMasterID*		outMasterTTYPtr,
				 char*					outSlaveTeletypewriterName,
				 size_t					inSlaveNameSize,
				 struct termios const*	inSlaveTerminalControlPtrOrNull,
				 struct winsize const*	inSlaveTerminalSizePtrOrNull)
{
	My_TTYMasterID	masterTTY = kLocal_InvalidTerminalID;
	pid_t			result = -1;
	Local_Result	localResult = kLocal_ResultOK;
	
	
	// create master and slave teletypewriters (represented by
	// file descriptors under UNIX); use the slave teletypewriter
	// to receive all output from the child process (it is expected
	// that the caller will "exec" something in the child)
	localResult = openMasterTeletypewriter(inSlaveNameSize, outSlaveTeletypewriterName, masterTTY);
	if (kLocal_ResultOK == localResult)
	{
		My_TTYSlaveID	slaveTTY = kLocal_InvalidTerminalID;
		
		
		// split into child and parent processes; 0 is returned
		// within the child, a positive number is returned within
		// the parent (the child process’ ID), and a negative
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
				
				// create a session
				if (-1 == setsid()) abort();
				
				// SVR4 acquires controlling terminal on open()
				slaveTTY = openSlaveTeletypewriter(outSlaveTeletypewriterName);
				if (slaveTTY >= 0) close(masterTTY); // child process no longer needs the master TTY
				
				// this is the BSD 4.4 way to acquire the controlling terminal
				if (-1 == ioctl(slaveTTY, TIOCSCTTY/* command */, NULL/* data */)) abort();
				
				// set slave’s terminal control information and terminal screen size
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


/*!
Finds an available master TTY for the specified slave.

\retval kLocal_ResultOK
if no error occurred

\retval kLocalResultInsufficientBufferSpace
if the given name buffer is not big enough for a device path

\retval kLocal_ResultTermCapError
if no devices are available

(3.1)
*/
Local_Result
openMasterTeletypewriter	(size_t				inNameSize,
							 char*				outSlaveTeletypewriterName,
							 My_TTYMasterID&	outID)
{
	char const* const	kSlaveTemplate = "/dev/WtyXY";
	size_t const		kIndexW = 5; // must match string above
	size_t const		kIndexX = 8; // must match string above
	size_t const		kIndexY = 9; // must match string above
	Local_Result		result = kLocal_ResultTermCapError;
	
	
	outID = kLocal_InvalidTerminalID;
	if (inNameSize < CPP_STD::strlen(kSlaveTemplate))
	{
		result = kLocalResultInsufficientBufferSpace;
	}
	else
	{
		char*		ptr1 = nullptr;
		char*		ptr2 = nullptr;
		Boolean		doneSearch = false;
		
		
		strcpy(outSlaveTeletypewriterName, kSlaveTemplate);
		outSlaveTeletypewriterName[kIndexW] = 'p'; // use "pty"
		
		// vary the name of the pseudo-teletypewriter until
		// an available device is found; the choices are
		// based on what appears in /dev on Mac OS X 10.1
		for (ptr1 = "pqrstuvwxyzPQRST"; ((!doneSearch) && (0 != *ptr1)); ++ptr1)
		{
			outSlaveTeletypewriterName[kIndexX] = *ptr1;
			for (ptr2 = "0123456789abcdef"; ((!doneSearch) && (0 != *ptr2)); ++ptr2)
			{
				outSlaveTeletypewriterName[kIndexY] = *ptr2;
				
				// try to open master
				outID = open(outSlaveTeletypewriterName, O_RDWR);
				if (-1 != outID)
				{
					outSlaveTeletypewriterName[kIndexW] = 't'; // use "tty"
					doneSearch = true;
					result = kLocal_ResultOK;
				}
				else
				{
					// only quit if there are no more devices available;
					// other errors might be avoided by trying another “pty”
					if (ENOENT == errno)
					{
						doneSearch = true;
						result = kLocal_ResultTermCapError;
					}
					outID = kLocal_InvalidTerminalID;
				}
			}
		}
	}
	return result;
}// openMasterTeletypewriter


/*!
Opens a slave TTY with the given name, or returns
"kLocal_InvalidTerminalID" if any problems occur.

(3.0)
*/
My_TTYSlaveID
openSlaveTeletypewriter		(char const*	inSlaveTeletypewriterName)
{
	My_TTYSlaveID	result = kLocal_InvalidTerminalID;
	
	
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
	if (-1 == result) result = kLocal_InvalidTerminalID;
	return result;
}// openSlaveTeletypewriter


/*!
For debugging - prints the data in a UNIX "termios" structure.

(3.0)
*/
void
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
Local_Result
putTTYInOriginalMode	(Local_TerminalID		inTTY)
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
“raw mode” by MacTelnet.

(3.0)
*/
void
putTTYInOriginalModeAtExit ()
{
	pid_t								currentProcessID = getpid();
	My_UnixProcessIDToTTYMap::iterator	unixProcessIDToTTYIterator = gUnixProcessIDToTTYMap().find(currentProcessID);
	
	
	if (gUnixProcessIDToTTYMap().end() != unixProcessIDToTTYIterator)
	{
		Local_TerminalID const		terminalID = unixProcessIDToTTYIterator->first;
		
		
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
Local_Result
putTTYInRawMode		(Local_TerminalID		inTTY)
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
		//   (“extended input”) are not recognized.
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
		//   another way of saying “don’t mess with the data in raw mode”.
		terminalControl.c_cflag |= (CS8);
		
		// Turn OFF the following output mode flags:
		//
		// - OPOST is disabled, meaning nothing special happens to data that
		//   is output.  If this were set, it would be possible to do things
		//   like map carriage returns and tabs - again, raw mode is not for
		//   user input, so this isn’t appropriate.
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
Local_Result
sendTerminalResizeMessage   (Local_TerminalID			inTTY,
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
allows MacTelnet to “block” waiting for data, without
actually halting other important things like the main
event loop!

WARNING:	As this is a preemptable thread, you MUST
			NOT use thread-unsafe system calls here.
			On the other hand, you can arrange for
			events to be handled (e.g. with Carbon
			Events).

(3.0)
*/
void*
threadForLocalProcessDataLoop	(void*		inDataLoopThreadContextPtr)
{
	size_t const					kBufferSize = 4096; // TEMPORARY - this should respect the user’s Network Block Size preference
	My_DataLoopThreadContextPtr		contextPtr = REINTERPRET_CAST(inDataLoopThreadContextPtr, My_DataLoopThreadContextPtr);
	size_t							numberOfBytesRead = 0;
	char*							buffer = REINTERPRET_CAST(Memory_NewPtrInterruptSafe(kBufferSize), char*);
	char*							processingBegin = buffer;
	char*							processingPastEnd = processingBegin;
	OSStatus						error = noErr;
	
	
	// arrange to communicate with the main application thread
	contextPtr->eventQueue = GetCurrentEventQueue();
	assert(contextPtr->eventQueue != GetMainEventQueue());
	
	for (;;)
	{
		assert(processingBegin >= buffer);
		assert(processingBegin <= (buffer + kBufferSize));
		
		// There are two possible actions...read more data, or process
		// the data that is in the buffer already.  If the processing
		// buffer is smaller than the data buffer, then this loop will
		// effectively stall reads until the processor catchs up.
		if (processingPastEnd == processingBegin)
		{
			// each time through the loop, read a bit more data from the
			// pseudo-terminal device, up to the maximum limit of the buffer
			numberOfBytesRead = read(contextPtr->masterTTY, buffer, kBufferSize);
			
			// TEMPORARY HACK - REMOVE HIGH ASCII
			//for (unsigned char* foo = (unsigned char*)buffer; (char*)foo != (buffer + kBufferSize); ++foo) { if (*foo > 127) *foo = '?'; }
			
			if (numberOfBytesRead <= 0)
			{
				// error or EOF (process quit)
				break;
			}
			
			// adjust the total number of bytes remaining to be processed
			processingBegin = buffer;
			processingPastEnd = processingBegin + numberOfBytesRead;
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
			// After sending a “please process this data” event, this thread
			// blocks until a go-ahead response is received from the main queue.
			// The go-ahead event also returns the number of unprocessed bytes,
			// which will determine if another post is necessary (next time
			// through the loop).
			//
			// WARNING:	To simplify the code below, certain assumptions are
			//			made.  One, that the current thread serves exactly one
			//			session, "contextPtr->session".  Two, that no one will
			//			ever send “data processed” events to this thread except
			//			for the purpose of continuing this loop.  Three, that
			//			the thread must not terminate while events it sends are
			//			being handled in the main thread - allowing addresses of
			//			data in this thread to be passed in.  Violate any of the
			//			assumptions, and I think you’ll deserve the outcome.
			//
			
			// notify that data has arrived
			postingResult = Session_PostDataArrivedEventToMainQueue
							(contextPtr->session, processingBegin, processingPastEnd - processingBegin,
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
						processingBegin = (processingPastEnd - unprocessedDataSize);
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
	
	// since the thread is finished, dispose of dynamically-allocated memory
	Memory_DisposePtrInterruptSafe(REINTERPRET_CAST(&buffer, void**));
	Memory_DisposePtrInterruptSafe(REINTERPRET_CAST(&contextPtr, void**));
	
	return nullptr;
}// threadForLocalProcessDataLoop

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
