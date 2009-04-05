/*!	\file ThreadEntryPoints.h
	\brief Entry points to cooperative threads.
	
	MacTelnet 3.0 is now multi-threaded!  This file lists the
	entry points to all cooperatively-multitasked threads in
	MacTelnet.  It is important to note the following:
	
	- Threads are memory-limited; if you exceed their allocated
	  space, try increasing the amount at thread construction
	  time (you may need to do a content-search on the constant
	  from "ConstantsRegistry.h" corresponding to the thread
	  you are editing).
	
	- After invoking a “yielding” routine from a cooperative
	  thread, you have to assume that another thread has gone
	  and trampled on your application context (for example,
	  details such as the current resource file and the current
	  graphics port should be set again, to ensure they remain
	  correct).
	
	- If you ever use preemptive threads (Mac OS X only), you
	  need to surround context-critical code within a “critical
	  section”, because you cannot otherwise be sure whether,
	  or when, the OS will switch you out to another thread
	  that might alter what you would otherwise assume to be
	  constant.  It is probably good “thread coding practice”
	  to define potentially critical sections anyway, even if
	  you are not using (or even do not intend to use) anything
	  but cooperative threads for your thread code.
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

#ifndef __THREADENTRYPOINTS__
#define __THREADENTRYPOINTS__

// Mac includes
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

pascal voidPtr
	ThreadEntryPoints_InitializeFontMenu	(void*		inMenuRefPosingAsVoidPointer);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
