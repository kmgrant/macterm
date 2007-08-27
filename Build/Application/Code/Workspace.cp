/*###############################################################

	Workspace.cp
	
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

// standard-C++ includes
#include <algorithm>
#include <vector>

// Mac includes
#include <Carbon/Carbon.h>

// library includes
#include <CFRetainRelease.h>
#include <Console.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>

// MacTelnet includes
#include "TerminalWindow.h"
#include "Workspace.h"



#pragma mark Types

typedef std::vector< CFRetainRelease >	WorkspaceWindowList;

struct Workspace
{
public:
	Workspace();
	~Workspace();
	
	WindowGroupRef			windowGroup;	//!< Mac OS window group used to constrain window activation, etc.
	WorkspaceWindowList		contents;		//!< the list of windows in this workspace
	Boolean					isObscured;		//!< whether all windows in this workspace are forced to hide but may technically be visible
	Workspace_Ref			selfRef;		//!< redundant reference to self, for convenience
};
typedef Workspace*			WorkspacePtr;
typedef Workspace const*	WorkspaceConstPtr;

typedef MemoryBlockPtrLocker< Workspace_Ref, Workspace >	WorkspacePtrLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	WorkspacePtrLocker&		gWorkspacePtrLocks ()	{ static WorkspacePtrLocker x; return x; }
}

#pragma mark Functors

/*!
This is a functor designed to hide the specified
window from view, with accompanying animation.

Model of STL Unary Function.

(3.0)
*/
#pragma mark windowObscurer
class windowObscurer:
public std::unary_function< CFRetainRelease const&/* argument */, void/* return */ >
{
public:
	windowObscurer	(Boolean	inIsHidden)
	:
	_isHidden(inIsHidden)
	{
	}
	
	void
	operator()	(CFRetainRelease const&		inWindow)
	{
		HIWindowRef			window = REINTERPRET_CAST(inWindow.returnHIObjectRef(), HIWindowRef);
		TerminalWindowRef	terminalWindow = TerminalWindow_ReturnFromWindow(window);
		
		
		if (terminalWindow != nullptr)
		{
			TerminalWindow_SetObscured(terminalWindow, _isHidden);
		}
		else
		{
			if (_isHidden) HideWindow(window);
			else ShowWindow(window);
		}
	}

protected:

private:
	Boolean		_isHidden;
};



#pragma mark Public Methods

/*!
Constructor.  See Workspace_New().

(3.0)
*/
Workspace::
Workspace()
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
windowGroup(nullptr),
contents(),
isObscured(false),
selfRef(REINTERPRET_CAST(this, Workspace_Ref))
{
	OSStatus	error = noErr;
	
	
	error = CreateWindowGroup(kWindowGroupAttrSelectAsLayer |
								kWindowGroupAttrMoveTogether |
								kWindowGroupAttrHideOnCollapse,
								&this->windowGroup);
	assert_noerr(error);
	
	// fix the window level so they are below menus, alerts, etc.
	error = SetWindowGroupParent(this->windowGroup,
									GetWindowGroupOfClass(kDocumentWindowClass));
	assert_noerr(error);
	error = RetainWindowGroup(this->windowGroup);
	assert_noerr(error);
}// Workspace default constructor


/*!
Constructor.  See Workspace_New().

(3.0)
*/
Workspace::
~Workspace()
{
	if (nullptr != windowGroup)
	{
		(OSStatus)ReleaseWindowGroup(this->windowGroup), this->windowGroup = nullptr;
	}
}// Workspace default constructor


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
		result = REINTERPRET_CAST(new Workspace, Workspace_Ref);
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
		Console_WriteValue("warning, attempt to dispose of locked workspace; outstanding locks",
							gWorkspacePtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		WorkspacePtr	ptr = gWorkspacePtrLocks().acquireLock(*inoutRefPtr);
		
		
		delete ptr;
		gWorkspacePtrLocks().releaseLock(*inoutRefPtr, &ptr);
		*inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Assigns the specified window to the workspace.  By
assigning a window, it is detached from any other
window groups it may be in and forced to follow the
rules of the given workspaceÕs window group (for
example, minimizing whenever any other window in
the group is minimized).

If the window is already part of this workspace, it
will be added again.

(3.0)
*/
void
Workspace_AddWindow		(Workspace_Ref	inWorkspace,
						 HIWindowRef	inWindowToAdd)
{
	WorkspacePtr	ptr = gWorkspacePtrLocks().acquireLock(inWorkspace);
	OSStatus		error = noErr;
	
	
	// if there are windows in this workspace, relocate the new window
	// to match the others in its group (for tab behavior)
	if (false == ptr->contents.empty())
	{
		HIWindowRef		similarWindow = REINTERPRET_CAST(ptr->contents.front().returnHIObjectRef(),
															HIWindowRef);
		Rect			similarWindowBounds;
		
		
		error = GetWindowBounds(similarWindow, kWindowStructureRgn, &similarWindowBounds);
		assert_noerr(error);
		error = SetWindowBounds(inWindowToAdd, kWindowStructureRgn, &similarWindowBounds);
		assert_noerr(error);
	}
	
	// put the window into a special group for this workspace
	error = SetWindowGroup(inWindowToAdd, ptr->windowGroup);
	assert_noerr(error);
	
	ptr->contents.push_back(inWindowToAdd);
	gWorkspacePtrLocks().releaseLock(inWorkspace, &ptr);
}// AddWindow


/*!
Returns true only if the windows in the specified
workspace are obscured - that is, hidden from view
without necessarily having invisible state.

To obscure a workspace, use Workspace_SetObscured().

(3.0)
*/
Boolean
Workspace_IsObscured	(Workspace_Ref	inWorkspace)
{
	WorkspacePtr	ptr = gWorkspacePtrLocks().acquireLock(inWorkspace);
	Boolean			result = ptr->isObscured;
	
	
	gWorkspacePtrLocks().releaseLock(inWorkspace, &ptr);
	return result;
}// IsObscured


/*!
Unassigns the specified window from the workspace,
such that the window is no longer constrained by
the workspaceÕs window group.

If the window occurs in the workspace multiple
times, all occurrences are removed.

(3.0)
*/
void
Workspace_RemoveWindow	(Workspace_Ref	inWorkspace,
						 HIWindowRef	inWindowToRemove)
{
	WorkspacePtr	ptr = gWorkspacePtrLocks().acquireLock(inWorkspace);
	
	
	// put the window back into the normal group of document windows
	(OSStatus)SetWindowGroup(inWindowToRemove, GetWindowGroupOfClass(kDocumentWindowClass));
	
	ptr->contents.erase(std::remove(ptr->contents.begin(), ptr->contents.end(), inWindowToRemove),
						ptr->contents.end());
	assert(ptr->contents.end() == std::find(ptr->contents.begin(), ptr->contents.end(),
											inWindowToRemove));
	gWorkspacePtrLocks().releaseLock(inWorkspace, &ptr);
}// RemoveWindow


/*!
Returns the number of windows in the specified workspace.

Returns "kWorkspace_WindowIndexInfinity" if the workspace
is invalid.

(3.1)
*/
UInt16
Workspace_ReturnWindowCount		(Workspace_Ref	inWorkspace)
{
	WorkspacePtr	ptr = gWorkspacePtrLocks().acquireLock(inWorkspace);
	UInt16			result = kWorkspace_WindowIndexInfinity;
	
	
	if (nullptr != ptr)
	{
		result = ptr->contents.size();
	}
	gWorkspacePtrLocks().releaseLock(inWorkspace, &ptr);
	return result;
}// ReturnWindowCount


/*!
Returns the window whose zero-based index in the workspace
is the specifed number.

The index of a window can change if Workspace_RemoveWindow()
is called and affects a window earlier in the list.
Therefore, do not cache this value.

Returns "nullptr" if the specified workspace or index is
invalid.

(3.1)
*/
HIWindowRef
Workspace_ReturnWindowWithZeroBasedIndex	(Workspace_Ref	inWorkspace,
											 UInt16			inIndex)
{
	WorkspacePtr	ptr = gWorkspacePtrLocks().acquireLock(inWorkspace);
	HIWindowRef		result = nullptr;
	
	
	if (nullptr != ptr)
	{
		if (inIndex < ptr->contents.size())
		{
			result = REINTERPRET_CAST(ptr->contents[inIndex].returnHIObjectRef(),
										HIWindowRef);
		}
	}
	gWorkspacePtrLocks().releaseLock(inWorkspace, &ptr);
	return result;
}// ReturnWindowWithZeroBasedIndex


/*!
Returns the zero-based index of the specified window in
the workspace.  Useful to show an ordering to the user
(in a list of tabs, for instance).

The index of a window can change if Workspace_RemoveWindow()
is called and affects a window earlier in the list.
Therefore, do not cache this value.

Returns "kWorkspace_WindowIndexInfinity" if the window
or workspace is invalid, or the window does not exist in
the workspace.

(3.1)
*/
UInt16
Workspace_ReturnZeroBasedIndexOfWindow	(Workspace_Ref	inWorkspace,
										 HIWindowRef	inWindowToFind)
{
	WorkspacePtr	ptr = gWorkspacePtrLocks().acquireLock(inWorkspace);
	UInt16			result = kWorkspace_WindowIndexInfinity;
	
	
	if (nullptr != ptr)
	{
		WorkspaceWindowList::const_iterator		toWindowRetainer;
		UInt16									i = 0;
		Boolean									windowFound = false;
		
		
		for (toWindowRetainer = ptr->contents.begin();
				((false == windowFound) && (toWindowRetainer != ptr->contents.end()));
				++toWindowRetainer, ++i)
		{
			HIWindowRef const	kWindow = REINTERPRET_CAST(toWindowRetainer->returnHIObjectRef(),
															HIWindowRef);
			if (kWindow == inWindowToFind)
			{
				result = i;
				windowFound = true;
			}
		}
	}
	gWorkspacePtrLocks().releaseLock(inWorkspace, &ptr);
	return result;
}// ReturnZeroBasedIndexOfWindow


/*!
Hides all the windows in the specified workspace from
view, without necessarily giving them an invisible state.
This allows the windows to continue to ÒactÓ visible
without cluttering up the userÕs screen.

This state cannot currently be nested; if you obscure a
workspace several times and then turn off obscuring once,
the workspace will be redisplayed.

To determine if a workspace is obscured, use
Workspace_IsObscured().

(3.0)
*/
void
Workspace_SetObscured	(Workspace_Ref	inWorkspace,
						 Boolean		inIsHidden)
{
	WorkspacePtr	ptr = gWorkspacePtrLocks().acquireLock(inWorkspace);
	
	
	std::for_each(ptr->contents.begin(), ptr->contents.end(), windowObscurer(inIsHidden));
	ptr->isObscured = inIsHidden;
	gWorkspacePtrLocks().releaseLock(inWorkspace, &ptr);
}// SetObscured

// BELOW IS REQUIRED NEWLINE TO END FILE
