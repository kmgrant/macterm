/*!	\file ServerBrowser.h
	\brief Cocoa implementation of a panel for finding
	or specifying servers for a variety of protocols.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
class NSWindow;
#endif

// library includes
#ifdef __OBJC__
#	import <PopoverManager.objc++.h>
#endif

// application includes
#include "Session.h"



#pragma mark Types

#ifdef __OBJC__

@class ServerBrowser_VC;


/*!
The object that implements this protocol will be told about
changes made to the Server Browser panel’s key properties.
Currently MacTerm responds by composing an equivalent Unix
command line.

Yes, "observeValueForKeyPath:ofObject:change:context:" does
appear to provide the same functionality but that is not
nearly as convenient.
*/
@protocol ServerBrowser_DataChangeObserver //{

@required

	// the user has selected a different connection protocol type
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didSetProtocol:(Session_Protocol)_;

	// the user has entered a different server host name
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didSetHostName:(NSString* _Nonnull)_;

	// the user has entered a different server port number
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didSetPortNumber:(NSUInteger)_;

	// the user has entered a different server log-in ID
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didSetUserID:(NSString* _Nonnull)_;

@optional

	// the browser has been removed
	- (void)
	serverBrowserDidClose:(ServerBrowser_VC* _Nonnull)_;

@end //}


/*!
Classes that are delegates of ServerBrowser_VC
must conform to this protocol.
*/
@protocol ServerBrowser_VCDelegate //{

	// use this opportunity to create and display a window to wrap the view
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didLoadManagedView:(NSView* _Nonnull)_;

	// when the view is going away, perform any final updates
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	didFinishUsingManagedView:(NSView* _Nonnull)_;

	// user interface has hidden or displayed something that requires the view size to change
	- (void)
	serverBrowser:(ServerBrowser_VC* _Nonnull)_
	setManagedView:(NSView* _Nonnull)_
	toScreenFrame:(NSRect)_;

@end //}


@class ServerBrowser_Object;
@class ServerBrowser_Protocol;

#else

class ServerBrowser_Object;

#endif // __OBJC__

// This is defined as an Objective-C object so it is compatible
// with ARC rules (e.g. strong references).
typedef ServerBrowser_Object*	ServerBrowser_Ref;



#pragma mark Public Methods

#ifdef __OBJC__
ServerBrowser_Ref _Nullable
	ServerBrowser_New			(NSWindow* _Nonnull				inParentWindow,
								 CGPoint						inParentRelativePoint,
								 id< ServerBrowser_DataChangeObserver > _Nullable	inDataObserver);
#endif

void
	ServerBrowser_Configure		(ServerBrowser_Ref _Nonnull		inDialog,
								 Session_Protocol				inProtocol,
								 CFStringRef _Nullable			inHostName,
								 UInt16							inPortNumber,
								 CFStringRef _Nullable			inUserID);

void
	ServerBrowser_Display		(ServerBrowser_Ref _Nonnull		inDialog);

void
	ServerBrowser_Remove		(ServerBrowser_Ref _Nonnull		inDialog);

// BELOW IS REQUIRED NEWLINE TO END FILE
