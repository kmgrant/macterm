/*!	\file Workspace.mm
	\brief A collection of terminal windows.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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

#import "Workspace.h"
#import <UniversalDefines.h>

// standard-C++ includes
#import <algorithm>
#import <vector>

// library includes
#import <Console.h>
#import <MemoryBlockPtrLocker.template.h>
#import <MemoryBlocks.h>

// application includes
#import "TerminalWindow.h"



#pragma mark Types
namespace {

typedef std::vector< TerminalWindowRef >	My_WorkspaceTerminalWindowList;

struct My_Workspace
{
public:
	My_Workspace();
	~My_Workspace();
	
	My_WorkspaceTerminalWindowList	terminalWindows;	//!< the list of terminal windows in this workspace
	Workspace_Ref					selfRef;			//!< redundant reference to self, for convenience
};
typedef My_Workspace*			My_WorkspacePtr;
typedef My_Workspace const*		My_WorkspaceConstPtr;

typedef MemoryBlockPtrLocker< Workspace_Ref, My_Workspace >		My_WorkspacePtrLocker;
typedef LockAcquireRelease< Workspace_Ref, My_Workspace >		My_WorkspaceAutoLocker;

} // anonymous namespace

#pragma mark Variables
namespace {

My_WorkspacePtrLocker&		gWorkspacePtrLocks ()	{ static My_WorkspacePtrLocker x; return x; }

} // anonymous namespace



#pragma mark Public Methods

/*!
Constructs a new terminal window workspace.  To destroy
the workspace later, use Workspace_Dispose().

Returns nullptr if unsuccessful.

(3.0)
*/
Workspace_Ref
Workspace_New ()
{
	Workspace_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_Workspace, Workspace_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	return result;
}// New


/*!
Destroys a terminal window workspace created with
Workspace_New().  On return, your copy of the reference
is set to nullptr.

(3.0)
*/
void
Workspace_Dispose	(Workspace_Ref*		inoutRefPtr)
{
	if (gWorkspacePtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteValue, "attempt to dispose of locked workspace; outstanding locks",
						gWorkspacePtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_WorkspacePtr*)), *inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Assigns the specified window to the workspace.

For Cocoa, this will add a tab for the window to the end of
any previous tab group for the workspace.  Since this is a
relatively new OS feature, it is not supported on older OS
versions.

\retval kWorkspace_ResultOK
if there are no errors

\retval kWorkspace_ResultInvalidReference
if the specified workspace is unrecognized

(2018.03)
*/
Workspace_Result
Workspace_AddTerminalWindow		(Workspace_Ref			inWorkspace,
								 TerminalWindowRef		inWindowToAdd)
{
	My_WorkspaceAutoLocker	ptr(gWorkspacePtrLocks(), inWorkspace);
	Workspace_Result		result = kWorkspace_ResultOK;
	auto					toWindow = std::find(ptr->terminalWindows.begin(), ptr->terminalWindows.end(), inWindowToAdd);
	
	
	if (nullptr == ptr)
	{
		result = kWorkspace_ResultInvalidReference;
	}
	else if (ptr->terminalWindows.end() != toWindow)
	{
		// window already in workspace (ignore)
		result = kWorkspace_ResultOK;
	}
	else
	{
		TerminalWindowRef const		lastTerminalWindow = ((ptr->terminalWindows.empty())
															? nullptr
															: ptr->terminalWindows.back());
		NSWindow*					cocoaWindow = TerminalWindow_ReturnNSWindow(inWindowToAdd);
		
		
		ptr->terminalWindows.push_back(inWindowToAdd);
		
		// it is only possible to add a tab to an existing window, and the
		// first tab in a workspace implicitly supports tabs (a later SDK
		// may provide a way to explicitly show the tab bar for a window
		// that had no tabs but otherwise there is nothing to be done for
		// the first window of a Cocoa window workspace)
		if (nullptr != lastTerminalWindow)
		{
			NSWindow*	lastCocoaWindow = TerminalWindow_ReturnNSWindow(lastTerminalWindow);
			
			
			if ([lastCocoaWindow respondsToSelector:@selector(addTabbedWindow:ordered:)])
			{
				id		asID = STATIC_CAST(lastCocoaWindow, id);
				
				
				[asID addTabbedWindow:cocoaWindow ordered:NSWindowAbove];
			}
			else
			{
				Console_Warning(Console_WriteLine, "runtime OS does not support adding tab to Cocoa terminal window");
			}
		}
	}
	
	return result;
}// AddTerminalWindow


/*!
Performs the specified operation on every terminal window
in the list.  The list must NOT change during iteration.

The iteration terminates early if the block sets its
stop-flag parameter.

\retval kWorkspace_ResultOK
if there are no errors

\retval kWorkspace_ResultInvalidReference
if the specified workspace is unrecognized

(2018.03)
*/
Workspace_Result
Workspace_ForEachTerminalWindow		(Workspace_Ref					inWorkspace,
									 Workspace_TerminalWindowBlock	inBlock)
{
	My_WorkspaceAutoLocker	ptr(gWorkspacePtrLocks(), inWorkspace);
	Workspace_Result		result = kWorkspace_ResultOK;
	Boolean					stopFlag = false;
	
	
	if (nullptr == ptr)
	{
		result = kWorkspace_ResultInvalidReference;
	}
	else
	{
		// traverse the list
		for (auto terminalWindowRef : ptr->terminalWindows)
		{
			inBlock(terminalWindowRef, stopFlag);
			if (stopFlag)
			{
				break;
			}
		}
	}
	
	return result;
}// ForEachTerminalWindow


/*!
Unassigns the specified window from the workspace.

For Cocoa, this will move the tab into its own window.

\retval kWorkspace_ResultOK
if there are no errors

\retval kWorkspace_ResultInvalidReference
if the specified workspace is unrecognized

(2018.03)
*/
Workspace_Result
Workspace_RemoveTerminalWindow	(Workspace_Ref			inWorkspace,
								 TerminalWindowRef		inWindowToRemove)
{
	My_WorkspaceAutoLocker	ptr(gWorkspacePtrLocks(), inWorkspace);
	Workspace_Result		result = kWorkspace_ResultOK;
	
	
	if (nullptr == ptr)
	{
		result = kWorkspace_ResultInvalidReference;
	}
	else
	{
		// move tab to new window
		NSWindow*	cocoaWindow = TerminalWindow_ReturnNSWindow(inWindowToRemove);
		BOOL		invokeOK = NO;
		
		
		invokeOK = [cocoaWindow tryToPerform:@selector(moveTabToNewWindow:) with:NSApp];
		if (NO == invokeOK)
		{
			Console_Warning(Console_WriteLine, "failed to move a workspace terminal tab to a new window");
		}
		ptr->terminalWindows.erase(std::remove(ptr->terminalWindows.begin(), ptr->terminalWindows.end(), inWindowToRemove),
									ptr->terminalWindows.end());
		assert(ptr->terminalWindows.end() == std::find(ptr->terminalWindows.begin(), ptr->terminalWindows.end(),
														inWindowToRemove));
	}
	
	return result;
}// RemoveTerminalWindow


#pragma mark Internal Methods
namespace {

/*!
Constructor.  See Workspace_New().

(3.0)
*/
My_Workspace::
My_Workspace()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
terminalWindows(),
selfRef(REINTERPRET_CAST(this, Workspace_Ref))
{
}// My_Workspace default constructor


/*!
Constructor.  See Workspace_New().

(3.0)
*/
My_Workspace::
~My_Workspace()
{
}// My_Workspace default constructor

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
