/*###############################################################

	MainEntryPoint.cp
	
	MacTelnet
		© 1998-2010 by Kevin Grant.
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
#include "MainEntryPoint.h"



// symbols required to link on 10.1
//long __isinfd(double d) { return (d == __inf()); }
//float __nan() { return INT_MIN; }

#pragma mark Internal Method Prototypes

static void		exitCleanly		();



#pragma mark Public Methods

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
	RestoreApplicationDockTileImage();
	ExitToShell();
}// exitCleanly

// BELOW IS REQUIRED NEWLINE TO END FILE
