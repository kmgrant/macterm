/*!	\file CocoaUserDefaults.mm
	\brief NSUserDefaults Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008-2021 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#import <CocoaUserDefaults.h>

// Mac includes
@import CoreFoundation;
@import Cocoa;

// library includes
#import <CFRetainRelease.h>



#pragma mark Public Methods

/*!
Copies the specified preferences domain entirely (e.g.
taking the contents of ~/Library/Preferences/<fromName>.plist
and creating ~/Library/Preferences/<toName>.plist).

IMPORTANT:	The copy is not saved until preferences have been
			synchronized.

(1.7)
*/
void
CocoaUserDefaults_CopyDomain	(CFStringRef	inFromName,
								 CFStringRef	inToName)
{
@autoreleasepool {
	NSUserDefaults*		defaultsTarget = [NSUserDefaults standardUserDefaults];
	NSDictionary*		dataDictionary = [defaultsTarget persistentDomainForName:BRIDGE_CAST(inFromName, NSString*)];
	
	
	[defaultsTarget setPersistentDomain:dataDictionary forName:BRIDGE_CAST(inToName, NSString*)];
}// @autoreleasepool
}// CopyDomain


/*!
Removes the specified preferences domain entirely (e.g.
deleting its ~/Library/Preferences/<name>.plist file).

(1.0)
*/
void
CocoaUserDefaults_DeleteDomain	(CFStringRef	inName)
{
@autoreleasepool {
	[[NSUserDefaults standardUserDefaults] removePersistentDomainForName:BRIDGE_CAST(inName, NSString*)];
}// @autoreleasepool
}// DeleteDomain

// BELOW IS REQUIRED NEWLINE TO END FILE
