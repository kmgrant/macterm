/*!	\file Workspace.h
	\brief A collection of terminal windows.
	
	Manages potentially-shared window properties such as
	activation order, minimization, etc.
*/
/*###############################################################

	MacTerm
		© 1998-2019 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// application includes
#include "TerminalWindow.h"



#pragma mark Constants

enum Workspace_Result
{
	kWorkspace_ResultOK					= 0,	//!< no error occurred
	kWorkspace_ResultGenericFailure		= 1,	//!< unspecified problem
	kWorkspace_ResultInvalidReference	= 2		//!< the given "Workspace_Ref" is unrecognized
};

#pragma mark Types

typedef struct Workspace_OpaqueStructure*	Workspace_Ref;

#pragma mark Callbacks

/*!
Terminal Window Block

This is used in Workspace_ForEachTerminalWindow().  If the stop
flag is set by the block, iteration will end early.
*/
typedef void (^Workspace_TerminalWindowBlock)	(TerminalWindowRef, Boolean&);



#pragma mark Public Methods

//!\name Creating and Destroying Terminal Window Workspaces
//@{

Workspace_Ref
	Workspace_New						();

void
	Workspace_Dispose					(Workspace_Ref*					inoutRefPtr);

//@}

//!\name Changing Workspace Contents
//@{

Workspace_Result
	Workspace_AddTerminalWindow			(Workspace_Ref					inWorkspace,
										 TerminalWindowRef				inWindowToAdd);

Workspace_Result
	Workspace_RemoveTerminalWindow		(Workspace_Ref					inWorkspace,
										 TerminalWindowRef				inWindowToRemove);

//@}

//!\name Iterating Over Workspace Windows
//@{

Workspace_Result
	Workspace_ForEachTerminalWindow		(Workspace_Ref					inWorkspace,
										 Workspace_TerminalWindowBlock	inBlock);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
