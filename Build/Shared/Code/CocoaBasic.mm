/*!	\file CocoaBasic.mm
	\brief Fundamental Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library
	© 2008 by Kevin Grant
	
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

// Mac includes
#import <Cocoa/Cocoa.h>

// library includes
#import <CocoaBasic.h>



#pragma mark Types

class My_AutoPool
{
public:
	My_AutoPool		();
	~My_AutoPool	();

protected:

private:
	NSAutoreleasePool*		_pool;
};



#pragma mark Public Methods

/*!
Initializes a Cocoa application by calling
NSApplicationLoad().

(3.1)
*/
Boolean
CocoaBasic_ApplicationLoad (void)
{
	My_AutoPool		_;
	BOOL			loadOK = NSApplicationLoad();
	
	
	return loadOK;
}


/*!
Plays the sound in the specified file (asynchronously).

(3.1)
*/
void
CocoaBasic_PlaySoundFile	(CFURLRef	inFile)
{
	My_AutoPool		_;
	
	
	[[[[NSSound alloc] initWithContentsOfURL:(NSURL*)inFile byReference:NO] autorelease] play];
}


#pragma mark Internal Methods

My_AutoPool::
My_AutoPool (): _pool([[NSAutoreleasePool alloc] init])
{
}// My_AutoPool constructor


My_AutoPool::
~My_AutoPool ()
{
	[_pool release];
}// My_AutoPool destructor

// BELOW IS REQUIRED NEWLINE TO END FILE
