/*###############################################################

	GenericThreads.cp
	
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

// standard-C++ includes
#include <map>

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <MemoryBlockHandleLocker.template.h>
#include <MemoryBlocks.h>

// MacTelnet includes
#include "ConstantsRegistry.h"
#include "GenericThreads.h"



#pragma mark Constants

enum
{
	kStandardInactiveGenericThreadOptions = kNewSuspend | kUsePremadeThread | kCreateIfNeeded,
	kStandardActiveGenericThreadOptions = kUsePremadeThread | kCreateIfNeeded
};

#pragma mark Types

struct My_GenericThread
{
	GenericThreadDescriptor		descriptor;
	ThreadID					threadID;
	ThreadTerminationProcPtr	terminationRoutine;
	void*						terminationRoutineArgument;
	voidPtr						result;
};
typedef My_GenericThread*		My_GenericThreadPtr;
typedef My_GenericThreadPtr*	My_GenericThreadHandle;

typedef std::map< ThreadID, GenericThreadRef >							My_ThreadIDToGenericThreadRefMap;
typedef MemoryBlockHandleLocker< GenericThreadRef, My_GenericThread >	My_ThreadHandleLocker;
typedef LockAcquireRelease< GenericThreadRef, My_GenericThread >		My_ThreadAutoLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	My_ThreadIDToGenericThreadRefMap&	gThreadMap ()			{ static My_ThreadIDToGenericThreadRefMap x; return x; }
	My_ThreadHandleLocker&				gThreadHandleLocks ()	{ static My_ThreadHandleLocker x; return x; }
	ThreadTerminationUPP				gThreadTerminationUPP = nullptr;
}

#pragma mark Internal Method Prototypes

static OSStatus			newThreadListEntry			(ThreadID, GenericThreadDescriptor, voidPtr, GenericThreadRef*);
static pascal void		threadTerminationRoutine	(ThreadID, void*);



#pragma mark Public Methods

/*!
Before calling any other Generic Threads routines,
invoke this function.  The Thread Manager must be
available in order to use any routine in this module!

Note that this module does NOT control preemptive
threads used on Mac OS X; those are typically created
directly as POSIX threads in places like "Local.c".

(3.0)
*/
void
GenericThreads_Init ()
{
	Size		stackSize = 0L;
	OSStatus	error = noErr;
	
	
#if TARGET_API_MAC_OS8
	error = GetDefaultThreadStackSize(kCooperativeThread, &stackSize);
#else
	// GetDefaultThreadStackSize() always fails on Mac OS X, so always guess a stack value
	error = cfragNoSymbolErr;
#endif
	if (error != noErr) stackSize = INTEGER_KILOBYTES(40); // without a real value, take a guess
	error = CreateThreadPool(kCooperativeThread, 3/* number of threads to create */,
								stackSize + INTEGER_KILOBYTES(1)/* stack size, per */);
	
	// create a thread termination UPP for use by all constructed threads
	gThreadTerminationUPP = NewThreadTerminationUPP(threadTerminationRoutine);
	
	// add an entry to the list for the main application thread
	{
		GenericThreadRef	newThread = nullptr;
		ThreadID			applicationThreadID = 0;
		
		
		error = GetCurrentThread(&applicationThreadID);
		if (error != noErr)
		{
			Console_Warning(Console_WriteValue, "could not determine thread ID, error", error);
			applicationThreadID = kApplicationThreadID;
		}
		error = newThreadListEntry(applicationThreadID, kGenericThreadDescriptorMainThread,
									nullptr/* address of thread return value - nullptr means ÒdonÕt careÓ */,
									&newThread);
		if ((error != noErr) || (newThread == nullptr))
		{
			Console_WriteValue("could not add application thread to list, error", error);
		}
	}
	
	// MacTelnet now requires this to be true
	FlagManager_Set(kFlagThreadManager, true);
}// Init


/*!
When you are completely finished with threads,
invoke this method to clean up this module.

(3.0)
*/
void
GenericThreads_Done ()
{
	if (FlagManager_Test(kFlagThreadManager))
	{
		ThreadID									threadID = 0;
		void**										threadResultPtr;
		My_ThreadIDToGenericThreadRefMap::iterator	threadIDHandlePairIterator;
		
		
		for (threadIDHandlePairIterator = gThreadMap().begin();
				threadIDHandlePairIterator != gThreadMap().end(); ++threadIDHandlePairIterator)
		{
			// destroy the structure pointed to by each linked list element
			threadID = threadIDHandlePairIterator->first;
			(OSStatus)DisposeThread(threadID, &threadResultPtr, false/* reuse thread */);
		}
		gThreadMap().clear();
		gThreadHandleLocks().clear();
		
		DisposeThreadTerminationUPP(gThreadTerminationUPP), gThreadTerminationUPP = nullptr;
	}
}// Done


/*!
Creates a cooperative thread, using premade
threads if possible and creating a thread if
necessary.  The amount "inExtraStackSize" is
*added* to the result of a call to the
GetDefaultThreadStackSize() Mac OS routine.

See also GenericThreads_NewCoOpWithOptions().

IMPORTANT:	The "inThreadDescriptorOfNewThread"
			parameter should be unique for each
			created thread.  The "inThreadEntry"
			value is not considered unique since
			you may want multiple threads using
			the same routine.  As with all other
			descriptors, MacTelnet uses values
			defined in "ConstantsRegistry.h".

(3.0)
*/
OSStatus
GenericThreads_NewCoOp	(ThreadEntryUPP 			inThreadEntry,
						 void*						inThreadParam,
						 Size 						inExtraStackSize,
						 GenericThreadDescriptor	inThreadDescriptorOfNewThread)
{
	return GenericThreads_NewCoOpWithOptions(inThreadEntry, inThreadParam, inExtraStackSize,
												kStandardActiveGenericThreadOptions, inThreadDescriptorOfNewThread);
}// NewCoOp


/*!
Creates a cooperative thread with custom
thread options.  The amount "inExtraStackSize"
is *added* to the result of a call to the
GetDefaultThreadStackSize() routine.

See also GenericThreads_NewCoOp().

IMPORTANT:	The "inThreadDescriptorOfNewThread"
			parameter should be unique for each
			created thread.  The "inThreadEntry"
			value is not considered unique since
			you may want multiple threads using
			the same routine.  As with all other
			descriptors, MacTelnet uses values
			defined in "ConstantsRegistry.h".

(3.0)
*/
OSStatus
GenericThreads_NewCoOpWithOptions	(ThreadEntryUPP 			inThreadEntry,
									 void*						inThreadParam,
									 Size 						inExtraStackSize,
									 ThreadOptions				inOptions,
									 GenericThreadDescriptor	inThreadDescriptorOfNewThread)
{
	OSStatus	result = noErr;
	
	
	if (FlagManager_Test(kFlagThreadManager))
	{
		Size	defaultSize = 0L;
		
		
	#if TARGET_API_MAC_OS8
		result = GetDefaultThreadStackSize(kCooperativeThread, &defaultSize);
	#else
		// GetDefaultThreadStackSize() always fails on Mac OS X, so always guess a stack value
		result = cfragNoSymbolErr;
	#endif
		if (result != noErr) defaultSize = INTEGER_KILOBYTES(40); // without a real value, take a guess
		
		// create a thread
		result = noErr;
		{
			GenericThreadRef	newThread = nullptr;
			ThreadID			threadID = 0;
			voidPtr				whereToPutReturnValue = nullptr;
			
			
			result = NewThread(kCooperativeThread, inThreadEntry, inThreadParam,
								defaultSize + inExtraStackSize, inOptions,
								&whereToPutReturnValue, &threadID);
			if (result == noErr)
			{
				result = SetThreadTerminator(threadID, gThreadTerminationUPP, nullptr/* argument */);
				if (result != noErr) Console_Warning(Console_WriteValue, "failed to set thread terminator, error", result);
				result = newThreadListEntry(threadID, inThreadDescriptorOfNewThread, whereToPutReturnValue,
											&newThread);
				if ((result != noErr) || (newThread == nullptr)) Console_WriteLine("thread creation failed");
				else GenericThreads_YieldTo(newThread);
			}
		}
	}
	else result = threadNotFoundErr;
	
	return result;
}// NewCoOpWithOptions


/*!
Pauses for up to a certain number of ticks (usually
accurate within a tick) in a way that does not
block other threads and that is interruptible by a
user mouse click.  If the mouse is clicked, the delay
is stopped short and this method returns immediately
with a value "true"; otherwise, "false" is returned.

(3.0)
*/
Boolean
GenericThreads_ClickStopDelayTicks	(UInt32		inTicksOfDelay)
{
	UInt32		startTime = TickCount();
	Boolean		result = false;
	
	
	for (; ((TickCount() - startTime) < inTicksOfDelay); )
	{
		GenericThreads_YieldToAnother();
		if (Button())
		{
			result = true;
			break;
		}
	}
	return result;
}// ClickStopDelayTicks


/*!
Pauses for at *least* a certain number of ticks (usually
accurate within a tick) in a way that does not block other
threads.  Typically, you should favor this routine over a
call to the Delay() system call.

(3.0)
*/
void
GenericThreads_DelayMinimumTicks	(UInt32		inTicksOfDelay)
{
	UInt32		dummy = 0L;
	
	
	Delay(inTicksOfDelay, &dummy);
	//UInt32		startTime = TickCount();
	
	
	//for (; ((TickCount() - startTime) < inTicksOfDelay); )
	//{
	//	GenericThreads_YieldToAnother();
	//}
}// DelayMinimumTicks


/*!
Acquires a reference to a thread, given the thread descriptor
assigned to it at creation time.

(3.0)
*/
GenericThreadRef
GenericThreads_ReturnByDescriptor	(GenericThreadDescriptor	inThreadDescriptor)
{
	GenericThreadRef	result = nullptr;
	
	
	if (FlagManager_Test(kFlagThreadManager))
	{
		My_ThreadIDToGenericThreadRefMap::const_iterator	threadIDHandlePairIterator;
		
		
		// look for the thread with the given descriptor in the list
		for (threadIDHandlePairIterator = gThreadMap().begin();
				threadIDHandlePairIterator != gThreadMap().end(); ++threadIDHandlePairIterator)
		{
			result = threadIDHandlePairIterator->second;
			
			{
				My_ThreadAutoLocker		threadPtr(gThreadHandleLocks(), result);
				
				
				if (threadPtr == nullptr) threadIDHandlePairIterator = gThreadMap().end(); // problem - stop here
				else
				{
					if (threadPtr->descriptor == inThreadDescriptor)
					{
						threadIDHandlePairIterator = gThreadMap().end(); // got it - stop here
					}
				}
			}
		}
	}
	
	return result;
}// ReturnByDescriptor


/*!
Acquires a reference to a thread, given the Mac OS Thread Manager
ID assigned to it when constructed.

IMPORTANT:	This routine is implementation-dependent, and as such more
			flexible routines like GenericThreads_ReturnByDescriptor()
			should be favored.  If the Thread Manager is unsupported
			one day, routines like this will not be available.

(3.0)
*/
GenericThreadRef
GenericThreads_ReturnByThreadManagerID		(ThreadID	inThreadID)
{
	GenericThreadRef									result = REINTERPRET_CAST(0x87654321, GenericThreadRef);
	My_ThreadIDToGenericThreadRefMap::const_iterator	threadIDHandlePairIterator;
	
	
	// look for the thread with the given ID in the list
	threadIDHandlePairIterator = gThreadMap().find(inThreadID);
	if (threadIDHandlePairIterator != gThreadMap().end())
	{
		result = threadIDHandlePairIterator->second;
	}
	
	return result;
}// ReturnByThreadManagerID


/*!
Returns the thread currently running.  This is the
recommended way to determine the current thread.
If nullptr is returned, an error may have occurred,
but you can assume that the main thread is running.

(3.0)
*/
GenericThreadRef
GenericThreads_ReturnCurrent ()
{
	GenericThreadRef	result = REINTERPRET_CAST(0x12345678, GenericThreadRef);
	ThreadID			threadID = 0;
	OSStatus			error = noErr;
	
	
	error = GetCurrentThread(&threadID);
	if (error == noErr) result = GenericThreads_ReturnByThreadManagerID(threadID);
	return result;
}// ReturnCurrent


/*!
Returns the descriptor of a thread, the unique
application-assigned identifier distinguishing it
from other threads.

(3.0)
*/
GenericThreadDescriptor
GenericThreads_ReturnDescriptorOf	(GenericThreadRef	inThread)
{
	GenericThreadDescriptor		result = kGenericThreadDescriptorMainThread;
	
	
	if (FlagManager_Test(kFlagThreadManager))
	{
		My_ThreadAutoLocker		threadPtr(gThreadHandleLocks(), inThread);
		
		
		if (threadPtr != nullptr) result = threadPtr->descriptor;
	}
	return result;
}// ReturnDescriptorOf


/*!
Determines the return value of a thread code block
previously created and executed with
GenericThreads_NewCoOp().

IMPORTANT:	The result is meaningless until the thread
			is finished executing!  See
			GenericThreads_WhenTerminatedInvoke() for a
			way to install a routine that is called as
			soon as the result becomes valid.

(3.0)
*/
voidPtr
GenericThreads_ReturnResultOf	(GenericThreadRef	inThread)
{
	voidPtr		result = nullptr;
	
	
	if (FlagManager_Test(kFlagThreadManager))
	{
		My_ThreadAutoLocker		threadPtr(gThreadHandleLocks(), inThread);
		
		
		if (threadPtr != nullptr) result = threadPtr->result;
	}
	return result;
}// ReturnResultOf


/*!
Every thread gets its own stack space for making
allocations, space which is also used by the OS
to manage the threads.  If this stack space runs
out, the thread can no longer function properly,
so you should use this method to determine how
much free space is left on a threadÕs stack.

(3.0)
*/
UInt32
GenericThreads_ReturnStackOf	(GenericThreadRef	inThread)
{
	UInt32		result = 0L;
	
	
	if (FlagManager_Test(kFlagThreadManager))
	{
		My_ThreadAutoLocker		threadPtr(gThreadHandleLocks(), inThread);
		
		
		if (threadPtr != nullptr) (OSStatus)ThreadCurrentStackSpace(threadPtr->threadID, &result);
	}
	return result;
}// ReturnStackOf


/*!
Determines the Thread Manager ID of a thread previously
created with GenericThreads_New().

WARNING:	If at all possible, use routines from this
			module to access threads.  This method is
			provided only if you wish to take advantage
			of Thread Manager calls that do not have
			equivalents in this module.  By accessing a
			threadÕs ID, you risk future incompatibilities
			if this module migrates away from the Thread
			Manager.  Consider adding a new routine to
			this module, if you need special functionality
			not found already.

(3.0)
*/
ThreadID
GenericThreads_ReturnThreadManagerID	(GenericThreadRef	inThread)
{
	ThreadID	result = kNoThreadID;
	
	
	if (FlagManager_Test(kFlagThreadManager))
	{
		My_ThreadAutoLocker		threadPtr(gThreadHandleLocks(), inThread);
		
		
		if (threadPtr != nullptr) result = threadPtr->threadID;
	}
	return result;
}// ReturnThreadManagerID


/*!
Since threads run and complete at irregular times,
it is often necessary to be notified when a thread
completes (if synchronization of multithreaded
operations is required).  Use this method to specify
a routine that should be called as soon as the
given thread terminates (returns a value).  You can
use GenericThreads_ReturnResult() to access the result
of a thread operation from within a termination method.

(3.0)
*/
void
GenericThreads_WhenTerminatedInvoke	(GenericThreadRef			inThread,
									 ThreadTerminationProcPtr	inProc,
									 void*						inProcArgument)
{
	if (FlagManager_Test(kFlagThreadManager))
	{
		My_ThreadAutoLocker		threadPtr(gThreadHandleLocks(), inThread);
		
		
		if (threadPtr != nullptr)
		{
			threadPtr->terminationRoutine = inProc;
			threadPtr->terminationRoutineArgument = inProcArgument;
		}
	}
}// WhenTerminatedInvoke


/*!
Forces a thread to suspend and allow a specific
suspended thread to resume executing.  If the
specified thread cannot execute, then this method
has the same effect as a general call to
GenericThreads_YieldToAnother().

You *must* call a yielding method regularly in an
environment with cooperative threads in order for
threads to get time; however, in a preemptive
environment, this call is optional (but still can
be invoked at strategic times to improve thread
responsiveness).

See also GenericThreads_YieldToAnother().

(3.0)
*/
void
GenericThreads_YieldTo		(GenericThreadRef		inThread)
{
	if (FlagManager_Test(kFlagThreadManager))
	{
		My_ThreadAutoLocker		threadPtr(gThreadHandleLocks(), inThread);
		
		
		if (threadPtr != nullptr) (OSStatus)YieldToThread(threadPtr->threadID);
	}
}// YieldTo


/*!
Forces a thread to suspend and allow another
suspended thread to resume executing.

You *must* call a yielding method regularly in an
environment with cooperative threads in order for
threads to get time; however, in a preemptive
environment, this call is optional (but still can
be invoked at strategic times to improve thread
responsiveness).

See also GenericThreads_YieldTo().

(3.0)
*/
void
GenericThreads_YieldToAnother ()
{
	if (FlagManager_Test(kFlagThreadManager)) (OSStatus)YieldToAnyThread();
}// YieldToAnother


#pragma mark Internal Methods

/*!
Adds a new item to the global list of threads,
assigning the specified descriptor.

(3.0)
*/
static OSStatus
newThreadListEntry		(ThreadID					inForWhichThreadID,
						 GenericThreadDescriptor	inDescriptorOfNewThread,
						 voidPtr					inWhereToPutReturnValueOrNull,
						 GenericThreadRef*			outNewThreadPtr)
{
	OSStatus	result = noErr;
	
	
	// create global space to store the data and results for this thread
	*outNewThreadPtr = REINTERPRET_CAST(Memory_NewHandle(sizeof(My_GenericThread)), GenericThreadRef);
	if (*outNewThreadPtr == nullptr) result = memFullErr;
	else
	{
		My_ThreadAutoLocker		ptr(gThreadHandleLocks(), *outNewThreadPtr);
		
		
		ptr->descriptor = inDescriptorOfNewThread;
		ptr->result = inWhereToPutReturnValueOrNull;
		ptr->threadID = inForWhichThreadID;
		
		gThreadMap()[inForWhichThreadID] = *outNewThreadPtr;
		assert(gThreadMap().find(inForWhichThreadID) != gThreadMap().end());
	}
	return result;
}// newThreadListEntry


/*!
Invoked whenever a thread created with
GenericThreads_NewCoOpWithOptions() reaches
the end of its code.  MacTelnet responds by
deleting all associated data for the thread.

(3.0)
*/
static pascal void
threadTerminationRoutine	(ThreadID	inWhichThreadTerminated,
							 void*		UNUSED_ARGUMENT(inUnusedParameter))
{
	GenericThreadRef	ref = GenericThreads_ReturnByThreadManagerID(inWhichThreadTerminated);
	
	
	if (ref != nullptr)
	{
		{
			My_ThreadAutoLocker		ptr(gThreadHandleLocks(), ref);
			
			
			if (ptr != nullptr)
			{
				// invoke the user-specified termination function, if any
				if (ptr->terminationRoutine != nullptr)
				{
					(*ptr->terminationRoutine)(inWhichThreadTerminated, ptr->terminationRoutineArgument);
				}
				
			}
		}
		Memory_DisposeHandle(REINTERPRET_CAST(&ref, Handle*));
	}
}// threadTerminationRoutine

// BELOW IS REQUIRED NEWLINE TO END FILE
