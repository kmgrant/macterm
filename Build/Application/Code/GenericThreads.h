/*!	\file GenericThreads.h
	\brief Ways to create and manage cooperative threads, and
	utilities that perform non-blocking waits.
	
	This module defines a layer of abstraction that will
	(hopefully) allow migration to the preferred Multiprocessing
	Services API without screwing up current multi-threaded
	MacTelnet code.
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

#ifndef __GENERICTHREADS__
#define __GENERICTHREADS__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Constants

enum
{
	kGenericThreadDescriptorMainThread = 0
};

#pragma mark Types

typedef struct OpaqueGenericThread**	GenericThreadRef;
typedef SInt32							GenericThreadDescriptor;



/*###############################################################
	INITIALIZING AND FINISHING WITH THREADS
###############################################################*/

// CALL THIS ROUTINE ONCE, BEFORE ANY OTHER THREAD ROUTINE
void
	GenericThreads_Init						();

// CALL THIS ROUTINE AFTER YOU ARE PERMANENTLY DONE WITH THREADS
void
	GenericThreads_Done						();

/*###############################################################
	CREATING THREADS
###############################################################*/

OSStatus
	GenericThreads_NewCoOp					(ThreadEntryUPP 			inThreadEntry,
											 void*						inThreadParam,
											 Size 						inStackSize,
											 GenericThreadDescriptor	inThreadDescriptorOfNewThread);

OSStatus
	GenericThreads_NewCoOpWithOptions		(ThreadEntryUPP 			inThreadEntry,
											 void*						inThreadParam,
											 Size 						inStackSize,
											 ThreadOptions				inOptions,
											 GenericThreadDescriptor	inThreadDescriptorOfNewThread);

/*###############################################################
	ACCESSING THREADS AND THREAD INFORMATION
###############################################################*/

GenericThreadRef
	GenericThreads_ReturnByDescriptor		(GenericThreadDescriptor	inThreadDescriptor);

// AVOID IF AT ALL POSSIBLE
GenericThreadRef
	GenericThreads_ReturnByThreadManagerID	(ThreadID					inThreadID);

GenericThreadRef
	GenericThreads_ReturnCurrent			();

GenericThreadDescriptor
	GenericThreads_ReturnDescriptorOf		(GenericThreadRef			inThread);

voidPtr
	GenericThreads_ReturnResultOf			(GenericThreadRef			inThread);

UInt32
	GenericThreads_ReturnStackOf			(GenericThreadRef			inThread);

// AVOID IF AT ALL POSSIBLE
ThreadID
	GenericThreads_ReturnThreadManagerID	(GenericThreadRef			inThread);

/*###############################################################
	THREAD CONTROL
###############################################################*/

void
	GenericThreads_WhenTerminatedInvoke		(GenericThreadRef			inThread,
											 ThreadTerminationProcPtr	inProc,
											 void*						inProcArgument);

void
	GenericThreads_YieldTo					(GenericThreadRef			inThread);

void
	GenericThreads_YieldToAnother			();

/*###############################################################
	THREAD UTILITIES
###############################################################*/

Boolean
	GenericThreads_ClickStopDelayTicks		(UInt32						inTicksOfInterruptibleDelay);

void
	GenericThreads_DelayMinimumTicks		(UInt32						inTicksOfDelay);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
