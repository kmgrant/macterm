/*!	\file Local.cp
	\brief UNIX-based routines for running local command-line
	programs on Mac OS X only, routing the data through some
	MacTerm terminal window.
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

#include "Local.h"
#include <UniversalDefines.h>

// standard-C includes
#include <cstdio>
#include <cstdlib>

// standard-C++ includes
#include <map>
#include <set>
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
#	include <sysexits.h>
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
#	include <util.h>
}

// Mac includes (TEMPORARY)
#include <Carbon/Carbon.h> // DEPRECATED; need to reimplement timers below

// library includes
#include <AlertMessages.h>
#include <CFRetainRelease.h>
#include <CFUtilities.h>
#include <CocoaBasic.h>
#include <Console.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>

// application includes
#include "AppResources.h"
#include "ConstantsRegistry.h"
#include "DebugInterface.h"
#include "QuillsSession.h"
#include "Session.h"
#include "Terminal.h"
#include "UIStrings.h"



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

typedef std::set< pid_t >		My_UnixProcessIDSet;

/*!
Thread context passed to threadForLocalProcessDataLoop().
*/
struct My_DataLoopThreadContext
{
	dispatch_queue_t	dispatchQueue;
	SessionRef			session;
	My_TTYMasterID		masterTTY;
	size_t				blockSize;
};
typedef My_DataLoopThreadContext*			My_DataLoopThreadContextPtr;
typedef My_DataLoopThreadContext const*		My_DataLoopThreadContextConstPtr;

/*!
Information retained about a new process.  Known externally
as a Local_ProcessRef.
*/
struct My_Process
{
	My_Process	(CFArrayRef, CFStringRef, Local_TerminalID, char const*, pid_t);
	~My_Process	();
	
	pid_t				_processID;			// the process directly spawned by this session
	Boolean				_stopped;			// true only if XOFF/suspend has occurred with no XON/resume yet
	Local_TerminalID	_pseudoTerminal;	// file descriptor of pseudo-terminal master
	char const*			_slaveDeviceName;	// e.g. "/dev/ttyp0", data sent here goes to the terminal emulator (not the process)
	CFRetainRelease		_commandLine;		// array of strings for parent process’ command line arguments (first is program name)
	CFRetainRelease		_recentDirectory;	// empty until a query is done to determine the value
	CFRetainRelease		_originalDirectory;	// empty if no chdir() was used, otherwise the chdir() value at spawn time
};
typedef My_Process*			My_ProcessPtr;
typedef My_Process const*	My_ProcessConstPtr;

typedef std::map< pid_t, Local_ProcessRef >		My_ProcessByID;

typedef MemoryBlockPtrLocker< Local_ProcessRef, My_Process >	My_ProcessPtrLocker;
typedef LockAcquireRelease< Local_ProcessRef, My_Process >		My_ProcessAutoLocker;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void			fillInTerminalControlStructure		(struct termios*);
void			printTerminalControlStructure		(struct termios const*);
Local_Result	putTTYInOriginalMode				(Local_TerminalID);
void			putTTYInOriginalModeAtExit			();
Local_Result	putTTYInRawMode						(Local_TerminalID);
void			receiveSignal						(int);
Local_Result	sendTerminalResizeMessage			(Local_TerminalID, struct winsize const*);
void			threadForLocalProcessDataLoop		(void*);

} // anonymous namespace

#pragma mark Variables
namespace {

My_ProcessPtrLocker&		gProcessPtrLocks ()		{ static My_ProcessPtrLocker x; return x; }
struct termios				gCachedTerminalAttributes;
MyTTYState					gTTYState = kMyTTYStateReset;
Boolean						gInDebuggingMode = Local_StandardInputIsATerminal(); //!< true if terminal I/O is possible for debugging
Boolean						gPrintedRawMode = false; //!< true after the first console dump of the raw-mode terminal configuration

//! used to help atexit() handlers know which terminal to touch
Local_TerminalID			gTerminalToRestore = 0;
My_UnixProcessIDSet&		gChildProcessIDs ()		{ static My_UnixProcessIDSet x; return x; }
My_ProcessByID&				gProcessesByID ()		{ static My_ProcessByID x; return x; }
sigset_t&					gSignalsBlockedInThreads	(Boolean	inBlock = true)
							{
								// call this from the main thread, to prevent any other thread from being
								// chosen as the handler of signals, by setting a mask up front (any threads
								// spawned hereafter will inherit the same mask); or, call this from a
								// child process after a fork with "inBlock = false" to undo the changes
								static sigset_t		x = 0;
								
								
								if ((0 == x) || (false == inBlock))
								{
									int		maskResult = 0;
									
									
									// add virtually all signals here (minus ones that cannot be blocked,
									// such as SIGKILL, and ones that are never asynchronous, such as SIGSEGV)
									sigemptyset(&x);
									sigaddset(&x, SIGABRT);
									sigaddset(&x, SIGINT);
									sigaddset(&x, SIGTERM);
									maskResult = pthread_sigmask((inBlock) ? SIG_BLOCK : SIG_UNBLOCK, &x, nullptr/* old set */);
									if (0 != maskResult)
									{
										int const	kActualError = errno;
										
										
										Console_Warning(Console_WriteValue, "pthread_sigmask() failed, error", kActualError);
									}
								}
								return x;
							}

} // anonymous namespace


#pragma mark Public Methods


/*!
This should be invoked periodically to see if any child process has exited.
If some unusual exit occurs, the user is notified in the background.

(2020.04)
*/
void
Local_CheckForProcessExits ()
{
	static Boolean		gFirstCall = true;
	int					currentStatus = 0;
	pid_t				waitResult = waitpid(-1/* process or group, -1 is “any child” */, &currentStatus, WNOHANG/* options */);
	
	
	if (gFirstCall)
	{
		sig_t		signalResult = nullptr;
		
		
		// install signal handlers to prevent the OS from popping up error
		// dialogs just because some child process aborted
		gFirstCall = false;
		signalResult = signal(SIGABRT, receiveSignal);
		if (SIG_ERR == signalResult)
		{
			Console_Warning(Console_WriteLine, "unable to install signal handler");
		}
	}
	
	if (0 == waitResult)
	{
		// no status yet...okay...
	}
	else
	{
		// process may have failed - if it is a “real” problem, tell the user
		// INCOMPLETE - it is possible to be more specific here, if information is cached and
		// looked up based on process ID (e.g. window, process name, session info)
		pid_t const		kProcessID = waitResult;
		
		
		// only pay attention to reports for processes that were spawned by this module
		if (gChildProcessIDs().end() != gChildProcessIDs().find(kProcessID))
		{
			CFRetainRelease		dialogTextTemplateCFString;
			CFRetainRelease		dialogTextCFString;
			CFRetainRelease		helpTextCFString; // not always used
			CFRetainRelease		notificationTitle;
			Boolean				canPostNotification = false;
			
			
			if (WIFEXITED(currentStatus))
			{
				int const	kExitCode = WEXITSTATUS(currentStatus);
				
				
				canPostNotification = true;
				if (0 != kExitCode)
				{
					// failed exit
					Console_WriteValuePair("process exit: pid,code", kProcessID, kExitCode);
					
					// if more is known about the type of exit status, add help text
					switch (kExitCode)
					{
					case EX_USAGE:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitUsageHelpText));
						break;
					
					case EX_DATAERR:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitDataErrHelpText));
						break;
					
					case EX_NOINPUT:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitNoInputHelpText));
						break;
					
					case EX_NOUSER:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitNoUserHelpText));
						break;
					
					case EX_NOHOST:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitNoHostHelpText));
						break;
					
					case EX_UNAVAILABLE:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitUnavailHelpText));
						break;
					
					case EX_SOFTWARE:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitSoftwareHelpText));
						break;
					
					case EX_OSERR:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitOSErrHelpText));
						break;
					
					case EX_OSFILE:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitOSFileHelpText));
						break;
					
					case EX_CANTCREAT:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitCreateHelpText));
						break;
					
					case EX_IOERR:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitIOErrHelpText));
						break;
					
					case EX_TEMPFAIL:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitTempFailHelpText));
						break;
					
					case EX_PROTOCOL:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitProtocolHelpText));
						break;
					
					case EX_NOPERM:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitNoPermHelpText));
						break;
					
					case EX_CONFIG:
						helpTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifySysExitConfigHelpText));
						break;
					
					default:
						break;
					}
					
					notificationTitle.setWithRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifyProcessDieTitle));
					dialogTextTemplateCFString.setWithRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifyProcessDieTemplate));
					// WARNING: this format must agree with how the original template string is defined
					dialogTextCFString.setWithNoRetain(CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* options */,
																				dialogTextTemplateCFString.returnCFStringRef(), kExitCode));
				}
				else
				{
					// successful exit
					Console_WriteValue("process exit (OK): pid", kProcessID);
					notificationTitle.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifyProcessExitTitle));
					dialogTextCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifyProcessExitPrimaryText));
				}
			}
			else if (WIFSIGNALED(currentStatus))
			{
				int const	kSignal = WTERMSIG(currentStatus);
				
				
				canPostNotification = true;
				switch (kSignal)
				{
				// not all termination signals should be reported
				case SIGKILL:
				case SIGALRM:
				case SIGTERM:
					canPostNotification = false;
					break;
				
				case SIGSTOP:
				case SIGTSTP:
				case SIGCONT:
					{
						auto	toProcess = gProcessesByID().find(kProcessID);
						
						
						if (gProcessesByID().end() != toProcess)
						{
							Local_ProcessRef		thisProcess = toProcess->second;
							My_ProcessAutoLocker	ptr(gProcessPtrLocks(), thisProcess);
							
							
							ptr->_stopped = (SIGCONT != kSignal);
						}
					}
					break;
				
				default:
					break;
				}
				
				Console_WriteValuePair("process exit: pid,signal", kProcessID, kSignal);
				notificationTitle.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifyProcessDieTitle));
				dialogTextTemplateCFString.setWithNoRetain(UIStrings_ReturnCopy(kUIStrings_AlertWindowNotifyProcessSignalTemplate));
				// WARNING: this format must agree with how the original template string is defined
				dialogTextCFString.setWithNoRetain(CFStringCreateWithFormat(kCFAllocatorDefault, nullptr/* options */,
																			dialogTextTemplateCFString.returnCFStringRef(), kSignal));
			}
			else if (WIFSTOPPED(currentStatus))
			{
				// ignore (could examine WSTOPSIG(currentStatus))
				// NOTE: this should never happen anyway, as MacTerm does not use ptrace() or the WUNTRACED option
			}
			else
			{
				Console_WriteValuePair("process returned unknown status", kProcessID, currentStatus);
			}
			
			// display a non-blocking alert to the user, or post a system notification
			// (note that this may do nothing, depending on user preferences)
			if (canPostNotification)
			{
				CocoaBasic_PostUserNotification(CFSTR("net.macterm.notifications.processexit"),
												notificationTitle.returnCFStringRef(),
												dialogTextCFString.returnCFStringRef(),
												helpTextCFString.returnCFStringRef());
			}
		}
	}
}// CheckForProcessExits


/*!
Constructs a command line based on the current user’s
preferred shell, or the "SHELL" environment variable if
the user’s preferred shell cannot be found.  A new array
of arguments is allocated for the command line.  (You
must call CFRelease() on the array yourself.)

See also Local_GetLoginShellCommandLine(), which is usually
the better choice (as it creates a pristine environment).

\retval kLocal_ResultOK
if the command line was constructed successfully

\retval kLocal_ResultInsufficientBufferSpace
if the command line array could not be constructed

(4.0)
*/
Local_Result
Local_GetDefaultShellCommandLine	(CFArrayRef&	outNewArgumentsArray)
{
	Local_Result		result = kLocal_ResultInsufficientBufferSpace;
	struct passwd*		userInfoPtr = getpwuid(getuid());
	CFRetainRelease		userShell(CFStringCreateWithCString
									(kCFAllocatorDefault,
										(nullptr != userInfoPtr) ? userInfoPtr->pw_shell : getenv("SHELL"),
										kCFStringEncodingASCII),
									CFRetainRelease::kAlreadyRetained);
	void const*			args[] = { userShell.returnCFStringRef() };
	
	
	outNewArgumentsArray = CFArrayCreate(kCFAllocatorDefault, args, sizeof(args) / sizeof(void const*), &kCFTypeArrayCallBacks);
	if (nullptr != outNewArgumentsArray)
	{
		result = kLocal_ResultOK;
	}
	
	return result;
}// GetDefaultShellCommandLine


/*!
Constructs a command line of "/usr/bin/login -p -f $USER"
for whatever the current user’s ID is, allocating a new
array of arguments for the command line.  (You must call
CFRelease() on the array yourself.)

\retval kLocal_ResultOK
if the command line was constructed successfully

\retval kLocal_ResultInsufficientBufferSpace
if the command line array could not be constructed

(4.0)
*/
Local_Result
Local_GetLoginShellCommandLine		(CFArrayRef&	outNewArgumentsArray)
{
	Local_Result		result = kLocal_ResultInsufficientBufferSpace;
	CFRetainRelease		userName(CFStringCreateWithCString(kCFAllocatorDefault, getenv("USER"), kCFStringEncodingASCII),
									CFRetainRelease::kAlreadyRetained);
	void const*			args[] = { CFSTR("/usr/bin/login"), CFSTR("-p"), CFSTR("-f"), userName.returnCFStringRef() };
	
	
	outNewArgumentsArray = CFArrayCreate(kCFAllocatorDefault, args, sizeof(args) / sizeof(void const*), &kCFTypeArrayCallBacks);
	if (nullptr != outNewArgumentsArray)
	{
		result = kLocal_ResultOK;
	}
	
	return result;
}// GetLoginShellCommandLine


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
				
				
				if (-1 == killResult)
				{
					int const	kActualError = errno;
					
					
					// ignore “no such process” errors
					if (ESRCH != kActualError)
					{
						Console_Warning(Console_WriteValue, "unable to kill process: Unix error", kActualError);
					}
				}
				else
				{
					// successful kill, ID is no longer valid
					ptr->_processID = -1;
				}
			}
			else
			{
				Console_Warning(Console_WriteValue, "attempt to run kill with special pid", ptr->_processID);
			}
		}
	}
	
	if (gProcessPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteValue, "attempt to dispose of locked process data; outstanding locks",
						gProcessPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_ProcessPtr*)), *inoutRefPtr = nullptr;
	}
}// KillProcess


/*!
Returns true only if the terminal associated with the specified
process is apparently waiting for password input.  This can be
used to give the user some kind of special feedback, such as a
different terminal cursor.

(4.1)
*/
Boolean
Local_ProcessIsInPasswordMode	(Local_ProcessRef	inProcess)
{
	My_ProcessAutoLocker	ptr(gProcessPtrLocks(), inProcess);
	struct termios			terminalInfo;
	int						callStatus = 0;
	Boolean					result = false;
	
	
	callStatus = tcgetattr(ptr->_pseudoTerminal, &terminalInfo);
	if (0 == callStatus)
	{
		// TEMPORARY; might need to check additional state here to avoid false positives
		result = (0 != (terminalInfo.c_lflag & ICANON));
	}
	
	return result;
}// ProcessIsInPasswordMode


/*!
Returns true only if the specified process is currently
stopped, as determined by this module’s periodic checks.

(4.0)
*/
Boolean
Local_ProcessIsStopped	(Local_ProcessRef	inProcess)
{
	My_ProcessAutoLocker	ptr(gProcessPtrLocks(), inProcess);
	Boolean					result = ptr->_stopped;
	
	
	return result;
}// ProcessIsStopped


/*!
Returns the program and its given arguments as an array of
CFStringRefs, suitable for display.  The strings can be decoded
into C strings for use in low-level system calls.

Do not release the reference that is returned.

(4.0)
*/
CFArrayRef
Local_ProcessReturnCommandLine	(Local_ProcessRef	inProcess)
{
	My_ProcessAutoLocker	ptr(gProcessPtrLocks(), inProcess);
	CFArrayRef				result = nullptr;
	
	
	result = ptr->_commandLine.returnCFArrayRef();
	return result;
}// ProcessReturnCommandLine


/*!
Returns the POSIX path of the directory that was current for the
given process when Local_UpdateCurrentDirectoryCache() was most
recently invoked.  The string can be decoded into a C string for
use in low-level system calls.

If the string is empty, it either means that no query was ever
done, or that the query could not determine the value (for
example, due to a permission issue or a failure to find some
utility that is needed to perform the query).

See also Local_ProcessReturnOriginalDirectory().

(4.0)
*/
CFStringRef
Local_ProcessReturnCurrentDirectory		(Local_ProcessRef	inProcess)
{
	My_ProcessAutoLocker	ptr(gProcessPtrLocks(), inProcess);
	CFStringRef				result = nullptr;
	
	
	result = ptr->_recentDirectory.returnCFStringRef();
	return result;
}// ProcessReturnCurrentDirectory


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
Returns the POSIX path of the directory that was current at
the time the process was spawned.  The string can be decoded
into a C string for use in low-level system calls.

If the string is empty, it means that no chdir() was done to
set a directory, so the working directory was implicitly that
of the shell that launched MacTerm itself (typically, the
Finder).

Do not release the reference that is returned.

See also Local_ProcessReturnCurrentDirectory().

(4.0)
*/
CFStringRef
Local_ProcessReturnOriginalDirectory	(Local_ProcessRef	inProcess)
{
	My_ProcessAutoLocker	ptr(gProcessPtrLocks(), inProcess);
	CFStringRef				result = nullptr;
	
	
	result = ptr->_originalDirectory.returnCFStringRef();
	return result;
}// ProcessReturnOriginalDirectory


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
Forks a new process and arranges for its output and input to be
channeled through the specified screen.  The Unix command line is
defined using the given argument array, the first element of
which must either be a program name (resolved by the PATH) or
a POSIX pathname to the location of the program to run.

The specified session’s data will be updated with whatever
information is available for the created process.

The specified working directory is targeted only in the child
process, meaning the caller’s working directory is unchanged.
If it is not possible to change to that directory, the child
process exits as a precaution.

\retval kLocal_ResultOK
if the process was created successfully

\retval kLocal_ResultParameterError
if a storage variable is nullptr, the argument array is empty,
or the given working directory was found to be nonexistent
(note that a directory could exist at the time this is checked
and be deleted in the interim before execution starts, so this
early check is only an optimization; you should assume that a
nonexistent directory could trigger failures at any time, such
as when command execution fails)

\retval kLocal_ResultForkError
if the process cannot be spawned

\retval kLocal_ResultThreadError
if a thread cannot be created to read data

(3.0)
*/
Local_Result
Local_SpawnProcess	(SessionRef			inUninitializedSession,
					 TerminalScreenRef	inContainer,
					 CFArrayRef			inArgumentArray,
					 CFStringRef		inWorkingDirectoryOrNull)
{
	CFStringEncoding const	kPathEncoding = kCFStringEncodingUTF8;
	CFIndex const			kArgumentCount = CFArrayGetCount(inArgumentArray);
	char**					argvCopy = new char*[1 + kArgumentCount];
	char const*				targetDir = nullptr;
	CFRetainRelease			targetDirCFString(inWorkingDirectoryOrNull, CFRetainRelease::kNotYetRetained);
	Boolean					deleteTargetDir = false;
	Local_Result			result = kLocal_ResultOK;
	
	
	if (kArgumentCount > 0)
	{					
		// construct an argument array of the form expected by the system call
		CFStringEncoding const	kArgumentEncoding = kCFStringEncodingUTF8;
		UInt16					j = 0;
		
		
		for (UInt16 i = 0; i < kArgumentCount; ++i)
		{
			CFStringRef		argumentCFString = CFUtilities_StringCast
												(CFArrayGetValueAtIndex(inArgumentArray, i));
			
			
			// ignore completely empty strings (generally caused by
			// a bad split on multiple whitespace characters)
			if (CFStringGetLength(argumentCFString) > 0)
			{
				size_t const	kBufferSize = 1 + CFStringGetMaximumSizeForEncoding
													(CFStringGetLength(argumentCFString), kArgumentEncoding);
				
				
				// this memory is not leaked because execvp() is about to occur
				argvCopy[j] = new char[kBufferSize];
				CFStringGetCString(argumentCFString, argvCopy[j], kBufferSize, kArgumentEncoding);
				++j;
			}
		}
		argvCopy[j] = nullptr;
	}
	
	// determine the directory to be in when the command is run
	if ((false == targetDirCFString.exists()) || (0 == CFStringGetLength(targetDirCFString.returnCFStringRef())))
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
		
		targetDirCFString.setWithNoRetain(CFStringCreateWithCString(kCFAllocatorDefault, targetDir, kPathEncoding));
	}
	else
	{
		// convert path to a form that is accepted by chdir()
		targetDir = CFStringGetCStringPtr(targetDirCFString.returnCFStringRef(), kPathEncoding);
		if (nullptr == targetDir)
		{
			size_t const	kBufferSize = 1 + CFStringGetMaximumSizeForEncoding
												(CFStringGetLength(targetDirCFString.returnCFStringRef()),
													kPathEncoding);
			char*			valueString = new char[kBufferSize];
			
			
			CFStringGetCString(targetDirCFString.returnCFStringRef(), valueString, kBufferSize, kPathEncoding);
			targetDir = valueString;
			deleteTargetDir = true;
		}
	}
	
	// require any target working directory to exist before bothering to
	// spawn the process; note that this is only a slight optimization
	// to save time in the case of common failures, and is NOT meant to
	// be the only validation ever performed (for example, even after
	// this check, the directory might disappear before the process is
	// spawned; the only way to really know it will work is to use it!!!)
	if (kLocal_ResultOK == result)
	{
		struct stat		dirInfo;
		
		
		bzero(&dirInfo, sizeof(dirInfo));
		
		// NOTE: stat() follows symbolic links implicitly, so a link to a
		// directory will still be considered a directory and not a link
		if (0 != stat(targetDir, &dirInfo))
		{
			Console_WriteValueCString("failed to stat target working directory", targetDir);
			result = kLocal_ResultParameterError;
		}
		else if (S_ISDIR(dirInfo.st_mode))
		{
			// this is OK, a directory is what is required!
			result = kLocal_ResultOK;
		}
		else
		{
			Console_WriteValueCString("target working directory is not a directory", targetDir);
			result = kLocal_ResultParameterError;
		}
	}
	
	if ((nullptr == inArgumentArray) || (kArgumentCount < 1) || (nullptr == argvCopy[0]))
	{
		result = kLocal_ResultParameterError;
	}
	else if (kLocal_ResultOK == result)
	{
		My_TTYMasterID		masterTTY = 0;
		char				slaveDeviceName[20/* arbitrary */];
		pid_t				processID = -1;
		struct termios		terminalControl;
		
		
		// set the answer-back message
		{
			CFStringRef		answerBackCFString = Terminal_EmulatorReturnName(inContainer);
			
			
			if (nullptr != answerBackCFString)
			{
				size_t const	kAnswerBackSize = CFStringGetLength(answerBackCFString) + 1/* terminator */;
				char*			answerBackCString = new char[kAnswerBackSize];
				
				
				if (CFStringGetCString(answerBackCFString, answerBackCString, kAnswerBackSize, kCFStringEncodingASCII))
				{
					UNUSED_RETURN(int)setenv("TERM", answerBackCString, true/* overwrite */);
				}
				delete [] answerBackCString;
			}
		}
		
		// Apple’s Terminal sets the variables TERM_PROGRAM and
		// TERM_PROGRAM_VERSION for some reason; it is possible
		// that scripts could start to rely on these, so it seems
		// harmless enough to set them correctly for MacTerm
		{
			CFBundleRef		mainBundle = AppResources_ReturnBundleForInfo();
			CFStringRef		valueCFString = nullptr;
			
			
			valueCFString = CFUtilities_StringCast
							(CFBundleGetValueForInfoDictionaryKey(mainBundle, kCFBundleNameKey));
			if (nullptr != valueCFString)
			{
				size_t const	kStringSize = CFStringGetLength(valueCFString) + 1/* terminator */;
				char*			valueCString = new char[kStringSize];
				
				
				if (CFStringGetCString(valueCFString, valueCString, kStringSize, kCFStringEncodingASCII))
				{
					UNUSED_RETURN(int)setenv("TERM_PROGRAM", valueCString, true/* overwrite */);
				}
				delete [] valueCString;
			}
			valueCFString = CFUtilities_StringCast
							(CFBundleGetValueForInfoDictionaryKey(mainBundle, kCFBundleVersionKey));
			if (nullptr != valueCFString)
			{
				size_t const	kStringSize = CFStringGetLength(valueCFString) + 1/* terminator */;
				char*			valueCString = new char[kStringSize];
				
				
				if (CFStringGetCString(valueCFString, valueCString, kStringSize, kCFStringEncodingASCII))
				{
					UNUSED_RETURN(int)setenv("TERM_PROGRAM_VERSION", valueCString, true/* overwrite */);
				}
				delete [] valueCString;
			}
		}
		
		// TEMPORARY - the UNIX structures are filled in with defaults that work,
		//             but eventually MacTerm has to map user preferences, etc.
		//             to these so that they affect “local” terminals in the same
		//             way as they affect remote ones
		std::memset(&terminalControl, 0, sizeof(terminalControl));
		fillInTerminalControlStructure(&terminalControl); // TEMP
		
		// spawn a child process attached to a pseudo-terminal device; the child
		// will be used to run the shell, and the shell’s I/O will be handled in
		// a separate preemptive thread by MacTerm’s awesome terminal emulator
		// and main event loop
		{
			struct winsize		terminalSize; // defined in "/usr/include/sys/ttycom.h"
			
			
			terminalSize.ws_col = Terminal_ReturnColumnCount(inContainer);
			terminalSize.ws_row = Terminal_ReturnRowCount(inContainer);
			
			// TEMPORARY; the TerminalView_GetTheoreticalScreenDimensions() API would be useful for this
			terminalSize.ws_xpixel = 0;	// terminal width, in pixels; UNKNOWN
			terminalSize.ws_ypixel = 0;	// terminal height, in pixels; UNKNOWN
			
			if (gInDebuggingMode && DebugInterface_LogsTeletypewriterState())
			{
				// in debugging mode, show the terminal configuration
				Console_WriteLine("printing initial terminal configuration for process");
				printTerminalControlStructure(&terminalControl);
			}
			
			processID = forkpty(&masterTTY, slaveDeviceName, &terminalControl, &terminalSize);
		}
		
		if (-1 == processID) result = kLocal_ResultForkError;
		else
		{
			if (0 == processID)
			{
				//
				// this is executed inside the child process; note that any
				// console print-outs at this point will actually go to the
				// child terminal (rendered by a MacTerm window!)
				//
				
				// IMPORTANT: There are limitations on what a child process can do.
				// For example, global data and framework calls are generally unsafe.
				// See the "fork" man page for more information.
				
				// undo the effects on signals, because the user might be running
				// a program like "bash" that inherits signal behavior from the
				// parent; if this step were not performed, then "bash" would do
				// odd things like ignore all control-C (interrupt) sequences
				gSignalsBlockedInThreads(false);
				
				// set the current working directory...abort if this fails
				// because presumably it is important that a command not
				// start in the wrong directory
				if (0 != chdir(targetDir))
				{
					Console_WriteValueCString("aborting, failed to chdir(), target directory", targetDir);
					exit(EX_NOINPUT); // could abort(), but an exit() is easier to report gracefully to the user
				}
				
				// run a Unix terminal-based application program; this is accomplished
				// using an execvp() call, which DOES NOT RETURN UNLESS there is an
				// error; technically the return value is -1 on error, but really it’s
				// a problem if any return value is received, so never exit with a 0
				// in this situation!
				UNUSED_RETURN(int)execvp(argvCopy[0], argvCopy); // should not return
				
				// almost no chance this line will be run, but if it does, just kill the child process
				Console_WriteLine("aborting, failed to exec()");
				exit(EX_UNAVAILABLE); // could abort(), but an exit() is easier to report gracefully to the user
			}
			
			//
			// this is executed inside the parent process
			//
			
			Console_WriteValue("spawned process ID", processID);
			
			// prevent threads from being the receivers of signals
			gSignalsBlockedInThreads();
			
			// avoid special processing of data, allow the terminal to see it all (raw mode)
			if (0)
			{
				// arrange for user’s TTY to be fixed at exit time
				gTerminalToRestore = STDIN_FILENO;
				if (-1 == atexit(putTTYInOriginalModeAtExit))
				{
					int const		kActualError = errno;
					
					
					Console_Warning(Console_WriteValue, "unable to register atexit() routine for TTY mode", kActualError);
				}
				
				// set user’s TTY to raw mode
				{
					Local_Result	rawSwitchResult = putTTYInRawMode(gTerminalToRestore);
					
					
					if (kLocal_ResultOK != rawSwitchResult)
					{
						Console_Warning(Console_WriteValue, "error entering TTY raw-mode", rawSwitchResult);
					}
				}
			}
			
			// start a thread for data processing so that MacTerm’s main event loop can still run
			auto	threadContextPtr = new My_DataLoopThreadContext();
			if (nullptr == threadContextPtr) result = kLocal_ResultInsufficientBufferSpace;
			else
			{
				// store process information for session
				{
					auto				newProcessPtr = new My_Process(inArgumentArray,
																		targetDirCFString.returnCFStringRef(),
																		masterTTY, slaveDeviceName, processID);
					Local_ProcessRef	newProcess = REINTERPRET_CAST(newProcessPtr, Local_ProcessRef);
					
					
					Session_SetProcess(inUninitializedSession, newProcess);
				}
				
				// set up context
				static int		gQueueCounter = 0;
				char			nameBuffer[256];
				char const*		queueName = nameBuffer;
				auto			formatStatus = snprintf(nameBuffer, sizeof(nameBuffer), "net.macterm.queues.sessions.%d", (int)++gQueueCounter);
				
				
				if (formatStatus < 0)
				{
					Console_Warning(Console_WriteValue, "unable to create formatted string for terminal queue name, error", formatStatus);
					queueName = nullptr;
				}
				threadContextPtr->dispatchQueue = dispatch_queue_create(queueName, DISPATCH_QUEUE_CONCURRENT);
				threadContextPtr->session = inUninitializedSession;
				threadContextPtr->masterTTY = masterTTY;
				threadContextPtr->blockSize = 4096; // TEMPORARY; could make this a user preference
				
				// put the session in the initialized state, to indicate it is complete
				Session_SetState(threadContextPtr->session, kSession_StateInitialized);
				
				// create and run thread with data-processing loop
				dispatch_async_f(threadContextPtr->dispatchQueue, STATIC_CAST(threadContextPtr, void*), threadForLocalProcessDataLoop);
			}
		}
	}
	
	// WARNING: this cleanup is not exception-safe, and should change
	delete [] argvCopy, argvCopy = nullptr;
	if (deleteTargetDir)
	{
		delete [] targetDir, targetDir = nullptr;
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
	Boolean		result = STATIC_CAST(isatty(STDIN_FILENO), Boolean);
	
	
	return result;
}// StandardInputIsATerminal


/*!
Disables local echoing for a pseudo-terminal.
Returns any errors sent back from terminal
control routines.

(3.0)
*/
int
Local_TerminalDisableLocalEcho		(Local_TerminalID		inPseudoTerminalID)
{
	struct termios	terminalInfo;
	int				result = 0;
	
	
	result = tcgetattr(inPseudoTerminalID, &terminalInfo);
	if (0 == result)
	{
		terminalInfo.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
		terminalInfo.c_oflag &= ~(ONLCR); // also disable mapping from newline to newline-carriage-return
		result = tcsetattr(inPseudoTerminalID, TCSANOW/* when to apply changes */, &terminalInfo);
	}
	return result;
}// TerminalDisableLocalEcho


/*!
Returns the resume-output character for a
pseudo-terminal (usually, control-Q).  Returns
-1 if any errors are sent back from terminal
control routines.

(3.1)
*/
int
Local_TerminalReturnFlowStartCharacter	(Local_TerminalID		inPseudoTerminalID)
{
	struct termios	terminalInfo;
	int				result = 0;
	int				error = 0;
	
	
	error = tcgetattr(inPseudoTerminalID, &terminalInfo);
	if (0 == error)
	{
		// success!
		result = terminalInfo.c_cc[VSTART];
	}
	else
	{
		// error
		result = -1;
	}
	
	return result;
}// TerminalReturnFlowStartCharacter


/*!
Returns the suspend-output (scroll lock) character
for a pseudo-terminal (usually, control-S).  Returns
-1 if any errors are sent back from terminal control
routines.

(3.1)
*/
int
Local_TerminalReturnFlowStopCharacter	(Local_TerminalID		inPseudoTerminalID)
{
	struct termios	terminalInfo;
	int				result = 0;
	int				error = 0;
	
	
	error = tcgetattr(inPseudoTerminalID, &terminalInfo);
	if (0 == error)
	{
		// success!
		result = terminalInfo.c_cc[VSTOP];
	}
	else
	{
		// error
		result = -1;
	}
	
	return result;
}// TerminalReturnFlowStopCharacter


/*!
Returns the interrupt-process character for a
pseudo-terminal (usually, control-C).  Returns
-1 if any errors are sent back from terminal
control routines.

(3.1)
*/
int
Local_TerminalReturnInterruptCharacter	(Local_TerminalID		inPseudoTerminalID)
{
	struct termios	terminalInfo;
	int				result = 0;
	int				error = 0;
	
	
	error = tcgetattr(inPseudoTerminalID, &terminalInfo);
	if (0 == error)
	{
		// success!
		result = terminalInfo.c_cc[VINTR];
	}
	else
	{
		// error
		result = -1;
	}
	
	return result;
}// TerminalReturnInterruptCharacter


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
Local_TerminalResize	(Local_TerminalID	inPseudoTerminalID,
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
}// TerminalResize


/*!
Specifies that input to the terminal should be assumed to be in
UTF-8 encoding already.  One important benefit of this setting
is that an erase can clear characters wider than one byte.
This setting alone is not sufficient to support UTF-8 properly;
the majority of translation work is done in the Session module.

Returns true only if successful.

NOTE:	The low-level terminal control flag needed to set this
		mode does not exist on all versions of Mac OS X.

(4.0)
*/
Boolean
Local_TerminalSetUTF8Encoding	(Local_TerminalID	inPseudoTerminalID,
								 Boolean			inIsUTF8)
{
	Boolean			result = false;
	struct termios	terminalInfo;
	int				tcStatus = tcgetattr(inPseudoTerminalID, &terminalInfo);
	
	
	if (0 == tcStatus)
	{
		if (inIsUTF8)
		{
			terminalInfo.c_iflag |= IUTF8;
		}
		else
		{
			terminalInfo.c_iflag &= ~(IUTF8);
		}
		tcStatus = tcsetattr(inPseudoTerminalID, TCSANOW/* when to apply changes */, &terminalInfo);
		if (0 == tcStatus)
		{
			result = true;
		}
	}
	return result;
}// TerminalSetUTF8Encoding


/*!
Writes the specified data to the stream described by the file
descriptor.  Returns the number of bytes actually written; if
this number is less than "inByteCount", offset the buffer by
the difference and try again to send the rest.

IMPORTANT:	Writing bytes to a process is a very low-level
			operation, and you should usually be calling a
			higher-level API (see the Session module).  For
			example, the Session knows what text encoding is
			supposed to be used.

(3.0)
*/
ssize_t
Local_TerminalWriteBytes	(int			inFileDescriptor,
							 void const*	inBufferPtr,
							 size_t			inByteCount)
{
	char const*			ptr = nullptr;
	size_t				bytesLeft = 0;
	ssize_t				bytesWritten = 0;
	ssize_t				result = inByteCount;
	SInt16				clogCount = 0;
	
	
	ptr = REINTERPRET_CAST(inBufferPtr, char const*); // use char*, pointer arithmetic doesn’t work on void*
	bytesLeft = inByteCount;
	while (bytesLeft > 0)
	{
		if ((bytesWritten = write(inFileDescriptor, ptr, bytesLeft)) < 0)
		{
			// error
			result = (inByteCount - bytesLeft);
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
}// TerminalWriteBytes


/*!
For each known child process, updates a cache of current working
directories.  The cached values can be returned by invoking the
Local_ProcessReturnCurrentDirectory() function.

NOTE:	This routine is not fast because it calls out to Python
		code that in turn runs the "lsof" program.  This approach
		is a bit weird but it does work on all Mac OS X versions.
		Also, because Python is the ultimate caller, Python code
		can be used to easily post-process the output.  The cost
		of the call is offset by doing it rarely and by pulling
		results for every process at the same time.

(4.0)
*/
void
Local_UpdateCurrentDirectoryCache ()
{
	typedef std::map< long, std::string >	StringByLong;
	// make a copy of the internal map; the global map intentionally has NO VALUES
	// and is used as a convenient way to store keys, but the copy will have the
	// values filled in with whatever directories were found
	std::vector< long >				processIDs;
	StringByLong					pathsByProcess;
	
	
	// find the Unix process IDs for every known child process
	for (auto idProcessRefPair : gProcessesByID())
	{
		processIDs.push_back(idProcessRefPair.first);
	}
	
	// in a SINGLE query, find ALL requested process’ directories
	try
	{
		pathsByProcess = Quills::Session::pids_cwds(processIDs);
	}
	catch (std::exception const&	inException)
	{
		Console_Warning(Console_WriteValueCString, "exception during lookup of process working directory", inException.what());
	}
	
	// now update all cached strings with the results
	for (auto longStrPair : pathsByProcess)
	{
		auto	toProcessByID = gProcessesByID().find(STATIC_CAST(longStrPair.first, pid_t));
		
		
		if (gProcessesByID().end() != toProcessByID)
		{
			Local_ProcessRef		ref = toProcessByID->second;
			My_ProcessAutoLocker	ptr(gProcessPtrLocks(), ref);
			
			
			if (nullptr != ptr)
			{
				ptr->_recentDirectory.setWithNoRetain(CFStringCreateWithCString(kCFAllocatorDefault, longStrPair.second.c_str(),
																				kCFStringEncodingUTF8));
			}
		}
	}
}// UpdateCurrentDirectoryCache


#pragma mark Internal Methods
namespace {

My_Process::
My_Process	(CFArrayRef			inArgumentArray,
			 CFStringRef		inWorkingDirectory,
			 Local_TerminalID	inMasterTerminal,
			 char const*		inSlaveDeviceName,
			 pid_t				inProcessID)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
_processID(inProcessID),
_stopped(false),
_pseudoTerminal(inMasterTerminal),
_slaveDeviceName(inSlaveDeviceName),
_commandLine(inArgumentArray, CFRetainRelease::kNotYetRetained),
_recentDirectory(CFSTR(""), CFRetainRelease::kNotYetRetained),
_originalDirectory(inWorkingDirectory, CFRetainRelease::kNotYetRetained)
{
#if 0
	Console_WriteLine("process created with argument array:");
	CFShow(inArgumentArray);
	Console_WriteLine("and working directory:");
	CFShow(inWorkingDirectory);
#endif
	gChildProcessIDs().insert(_processID);
	gProcessesByID()[_processID] = REINTERPRET_CAST(this, Local_ProcessRef);
}// My_Process constructor


My_Process::
~My_Process ()
{
	gChildProcessIDs().erase(_processID);
	gProcessesByID().erase(_processID);
}// My_Process destructor


/*!
Fills in a UNIX "termios" structure using information
that MacTerm provides about the environment.  Valid
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
	//   and line-feeds.  NOTE that this may be a bad idea, as MacTerm
	//   likes to do this at the Session level...TEMPORARY?
	*outputFlagsPtr = OPOST | ONLCR;
	
	// Turn ON the following control flags:
	//
	// - CS8 means characters are 8 bits, that is, no bits are masked.  NOTE
	//   that although MacTerm has an option for 7-bit, this is handled
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
	controlCharacterArray[VEOL] = CEOL;
	controlCharacterArray[VEOL2] = CEOL;
	controlCharacterArray[VERASE] = CERASE;
	controlCharacterArray[VWERASE] = CWERASE;
	controlCharacterArray[VKILL] = CKILL;
	controlCharacterArray[VREPRINT] = CREPRINT;
	controlCharacterArray[VINTR] = CINTR; // TEMPORARY - should set from MacTerm preferences
	controlCharacterArray[VQUIT] = CQUIT;
	controlCharacterArray[VSUSP] = CSUSP;
	controlCharacterArray[VDSUSP] = CDSUSP;
	controlCharacterArray[VSTART] = CSTART; // TEMPORARY - should set from MacTerm preferences
	controlCharacterArray[VSTOP] = CSTOP; // TEMPORARY - should set from MacTerm preferences
	controlCharacterArray[VLNEXT] = CLNEXT;
	controlCharacterArray[VDISCARD] = CDISCARD;
	controlCharacterArray[VMIN] = CMIN;
	controlCharacterArray[VTIME] = CTIME;
	controlCharacterArray[VSTATUS] = CSTATUS;
	
	// It’s hard to say what to put here...MacTerm is not a
	// modem!  Oh well, 9600 baud is what Terminal.app uses...
	*inputSpeedPtr = B9600;
	*outputSpeedPtr = B9600;
}// fillInTerminalControlStructure


/*!
For debugging - prints the data in a UNIX "termios"
structure.

IMPORTANT:	This function is called within the child
			portion of a fork, which limits its behavior!

(3.0)
*/
void
printTerminalControlStructure	(struct termios const*		inTerminalControlPtr)
{
	Console_BlockIndent	_;
	tcflag_t const*		inputFlagsPtr = &inTerminalControlPtr->c_iflag;
	tcflag_t const*		outputFlagsPtr = &inTerminalControlPtr->c_oflag;
	tcflag_t const*		hardwareControlFlagsPtr = &inTerminalControlPtr->c_cflag;
	tcflag_t const*		localFlagsPtr = &inTerminalControlPtr->c_lflag;
	cc_t const*			controlCharacterArray = inTerminalControlPtr->c_cc;
	speed_t const*		inputSpeedPtr = &inTerminalControlPtr->c_ispeed;
	speed_t const*		outputSpeedPtr = &inTerminalControlPtr->c_ospeed;
	
	
	Console_WriteValueBitFlags("input flags", STATIC_CAST(*inputFlagsPtr, UInt32));
	Console_WriteValueBitFlags("output flags", STATIC_CAST(*outputFlagsPtr, UInt32));
	Console_WriteValueBitFlags("hardware control flags", STATIC_CAST(*hardwareControlFlagsPtr, UInt32));
	Console_WriteValueBitFlags("local flags", STATIC_CAST(*localFlagsPtr, UInt32));
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
			//int const	kActualError = errno;
			
			
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
A standard atexit() handler; resets a modified terminal to
whatever state it was in originally, before being put in
“raw mode” by MacTerm.

(3.0)
*/
void
putTTYInOriginalModeAtExit ()
{
	if (gTerminalToRestore >= 0)
	{
		UNUSED_RETURN(Local_Result)putTTYInOriginalMode(gTerminalToRestore);
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
	if (-1 == tcgetattr(inTTY, &gCachedTerminalAttributes))
	{
		int const		kActualError = errno;
		
		
		Console_Warning(Console_WriteValuePair, "unable to cache terminal attributes: TTY,error", inTTY, kActualError);
		result = kLocal_ResultTermCapError;
	}
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
		//   MacTerm processes them.  The trouble is, this is a user
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
			int const		kActualError = errno;
			
			
			Console_Warning(Console_WriteValuePair, "unable to update terminal attributes: TTY,error", inTTY, kActualError);
			result = kLocal_ResultTermCapError;
		}
		else
		{
			// success!
			gTTYState = kMyTTYStateRaw;
			if ((gInDebuggingMode) && (false == gPrintedRawMode) && DebugInterface_LogsTeletypewriterState())
			{
				// in debugging mode, show the terminal configuration, but just the first time it is used
				Console_WriteLine("printing raw-mode terminal configuration");
				printTerminalControlStructure(&terminalControl);
				gPrintedRawMode = true;
			}
		}
	}
	
	return result;
}// putTTYInRawMode


/*!
Responds to certain signals by simply absorbing them.

IMPORTANT:	A signal handler can only make system calls that
			are safe to execute asynchronously.  This actually
			omits something as innocuous as printf(), forcing
			one to rely on write() (say) for debugging.

NOTE:	If the specified signal is synchronous, such as a
		segmentation fault, this handler will run in the thread
		that caused the signal.  Other signals, however, could
		trigger this handler while in ANY thread that does not
		explicitly mask off signals.  Look for a call to
		pthread_sigmask().

(4.0)
*/
void
receiveSignal	(int	UNUSED_ARGUMENT(inSignal))
{
	char	buffer[] = "MacTerm: caught signal\n";
	
	
	// safe printf()...
	write(STDERR_FILENO, buffer, sizeof(buffer) - 1);
}// receiveSignal


/*!
Internal version of Local_TerminalResize().

IMPORTANT:	This function is called within the child
			portion of a fork, which limits its behavior!

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
		int const	kActualError = errno;
		
		
		Console_Warning(Console_WriteValue, "failed to send terminal resize message, errno", kActualError);
		result = kLocal_ResultIOControlError;
	}
	return result;
}// sendTerminalResizeMessage


/*!
This is the data processing loop for a particular
pseudo-terminal device, and it runs on a dedicated
concurrent queue.  See Local_SpawnProcess().

(2019.12)
*/
void
threadForLocalProcessDataLoop	(void*		inDataLoopThreadContextPtr)
{
	My_DataLoopThreadContextPtr		contextPtr = REINTERPRET_CAST(inDataLoopThreadContextPtr, My_DataLoopThreadContextPtr);
	size_t const					kBufferSize = contextPtr->blockSize;
	size_t							numberOfBytesRead = 0;
	std::unique_ptr< UInt8[] >		dataBuffer(new UInt8[kBufferSize]);
	UInt8*							bufferBegin = dataBuffer.get();
	__block bool					endLoop = false;
	__block UInt8*					processingBegin = bufferBegin;
	UInt8*							processingPastEnd = processingBegin;
	
	
	for (;;)
	{
		assert(processingBegin >= bufferBegin);
		assert(processingBegin <= (bufferBegin + kBufferSize));
		
		if (endLoop)
		{
			break;
		}
		
		// There are two possible actions...read more data, or process
		// the data that is in the buffer already.  If the processing
		// buffer is smaller than the data buffer, then this loop will
		// effectively stall reads until the processor catchs up.
		if (processingPastEnd == processingBegin)
		{
			// each time through the loop, read a bit more data from the
			// pseudo-terminal device, up to the maximum limit of the buffer
			numberOfBytesRead = read(contextPtr->masterTTY, bufferBegin, kBufferSize);
			
			// TEMPORARY HACK - REMOVE HIGH ASCII
			//for (unsigned char* foo = (unsigned char*)bufferBegin; (char*)foo != (bufferBegin + kBufferSize); ++foo) { if (*foo > 127) *foo = '?'; }
			
			if (numberOfBytesRead <= 0)
			{
				// error or EOF (process quit)
				endLoop = true;
			}
			else
			{
				// adjust the total number of bytes remaining to be processed
				processingBegin = bufferBegin;
				processingPastEnd = processingBegin + numberOfBytesRead;
			}
		}
		else
		{
			// process data via main queue (since terminal UI has to
			// update there) and wait until the session responds
			// before resuming the loop to process more data
			dispatch_sync(dispatch_get_main_queue(),
							^{
								size_t				unprocessedSize = 0;
								Session_Result		sessionResult = Session_AppendDataForProcessing(contextPtr->session,
																									processingBegin,
																									STATIC_CAST(processingPastEnd - processingBegin, size_t),
																									&unprocessedSize);
								
								
								if (sessionResult.ok())
								{
									processingBegin = (processingPastEnd - unprocessedSize);
								}
								else
								{
									Console_Warning(Console_WriteValue, "data-processing loop terminated, append operation error", sessionResult.code());
									endLoop = true;
								}
							});
		}
	}
	
	// loop terminated, ensure TTY is closed
	{
		int		sysResult = close(contextPtr->masterTTY);
		
		
		if (-1 == sysResult)
		{
			int const	kActualError = errno;
			
			
			Console_Warning(Console_WriteValue, "failed to close the master TTY, errno", kActualError);
		}
	}
	
	// update the state; for thread safety, this is done indirectly
	// by posting a session-state-update event to the main queue
	SessionRef const	kSession = contextPtr->session; // make block capture only a SessionRef and not "contextPtr"
	dispatch_async(dispatch_get_main_queue(),
					^{
						if (Session_IsValid(kSession))
						{
							Session_SetState(kSession, kSession_StateDead);
						}
					});
	
	// since the thread is finished, dispose of dynamically-allocated memory
	dispatch_release(contextPtr->dispatchQueue);
	delete contextPtr;
}// threadForLocalProcessDataLoop

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
