/*###############################################################

	MainEntryPoint.cp
	
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

// standard-C includes
#include <climits>
#include <stdexcept>

// library includes
#include <Console.h>
#include <FlagManager.h>
#include <StringUtilities.h>

// MacTelnet includes
#include "AppResources.h"
#include "EventLoop.h"
#include "Initialize.h"
#include "MainEntryPoint.h"



// symbols required to link on 10.1
//long __isinfd(double d) { return (d == __inf()); }
//float __nan() { return INT_MIN; }

#pragma mark Internal Method Prototypes

static void		exitCleanly		();



#pragma mark Public Methods

/*!
This is technically the main entry point, but it is only
available if the target is a real application.  Usually
MacTelnet is built as a shared library that can be loaded
into a running Python interpreter.  For more information
on the Python interface, see the "PythonCode" folder and
the various Quill API headers like "QuillBase.h".

This is the main entry point, what runs when the user
launches the application.  Aside from C++ static
initialization (constructors, etc.) which are called
first, this is essentially where the program begins; and
ideally, the program should end only when this function
returns.

This application uses a callback-driven event framework.
Consequently, you cannot easily determine how the
program works by starting in main()!  Instead, you must
assume that the user drives the vast majority of
operations: this means either AppleScript will generate
primitive instructions, or user commands will trigger a
series of primitive instructions to accomplish a task
(for example, as a result of selecting a menu item).
You should look in the AppleScript source files (which
have "...AE.cp" names) as well as in central places like
Commands.cp, for clues as to how the various components
really work.

(3.1)
*/
#define MACTELNET_LIB_NO_MAIN 1
#if !MACTELNET_LIB_NO_MAIN
int
main ()
{
//#define CATCH_WILD_EXCEPTIONS (defined DEBUG)
#define CATCH_WILD_EXCEPTIONS 1
#if CATCH_WILD_EXCEPTIONS
	try
#endif
	{
		Initialize_ApplicationStartup(CFBundleGetMainBundle());
		EventLoop_Run(); // returns only when the user asks to quit
		Initialize_ApplicationShutdown();
		MainEntryPoint_ImmediatelyQuit();
		assert(0); // THE PROGRAM SHOULD NEVER GET HERE
	}
#if CATCH_WILD_EXCEPTIONS
	catch (std::exception const&	e)
	{
		Console_WriteLine("uncaught exception");
		Console_WriteLine(e.what());
	}
#endif
	return 1;
}// main
#endif


/*!
Call this method to force the program to clean up (if
possible) and to then *definitely* shut down.

(3.0)
*/
void
MainEntryPoint_ImmediatelyQuit ()
{
	exitCleanly();
}// ImmediatelyQuit


#pragma mark Internal Methods

/*!
Quits this application, freeing any locked-up
resources (this function does not return).

(3.0)
*/
static void
exitCleanly ()
{
	// close any resource files that the application would have opened
	{
		SInt16		resFile = 0;
		
		
		resFile = AppResources_ReturnResFile(kAppResources_FileIDPreferences);
		if (resFile >= 0) CloseResFile(resFile);
	}
	RestoreApplicationDockTileImage();
	ExitToShell();
}// exitCleanly

// BELOW IS REQUIRED NEWLINE TO END FILE
