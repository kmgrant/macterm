/*!	\file ChildProcessWC.objc++.h
	\brief A window that is a local proxy providing access to
	a window in another process.
*/
/*###############################################################

	MacTerm
		© 1998-2023 by Kevin Grant.
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

// Mac includes
@import Cocoa;



#pragma mark Types

/*!
The type of block that can be installed to perform an
operation when the child process exit has been detected.
Even if the proxy window "close" method is used, an
attempt will first be made to force the child to exit
and this block will only be invoked when the "terminated"
state has been observed.
*/
typedef void (^ChildProcessWC_AtExitBlockType)();



/*!
Implements a local proxy window (typically offscreen) that
intercepts window actions and translates them into calls
to a sub-process.  The sub-process, in turn, performs
equivalent actions in the graphical interface that is
displayed by the sub-process.  The assumption is that the
sub-process sets LSUIElement "true" and displays exactly
one main window.

This class helps to give the illusion of a continuous
window rotation, even though technically some windows in
the “rotation” belong to different processes.  As the user
rotates through windows, the proxy (when selected) will
switch to the sub-process automatically and skip itself in
the rotation.

The proxy should be constructed in a way that is consistent
with the type of interface in the child process.  Currently
there is only one option (a basic document-style window)
but that may change.
*/
@interface ChildProcessWC_Object : NSWindowController< NSWindowDelegate > //{

// class methods
	+ (instancetype _Nullable)
	childProcessWCWithRunningApp:(NSRunningApplication* _Nonnull)_;
	+ (instancetype _Nullable)
	childProcessWCWithRunningApp:(NSRunningApplication* _Nonnull)_
	atExit:(ChildProcessWC_AtExitBlockType _Nullable)_;

// initializers
	- (instancetype _Nullable)
	initWithRunningApp:(NSRunningApplication* _Nonnull)_;
	- (instancetype _Nullable)
	initWithRunningApp:(NSRunningApplication* _Nonnull)_
	atExit:(ChildProcessWC_AtExitBlockType _Nullable)_ NS_DESIGNATED_INITIALIZER;

@end //}

// BELOW IS REQUIRED NEWLINE TO END FILE
