/*!	\file CocoaBasic.mm
	\brief Fundamental Cocoa APIs wrapped in C++.
*/
/*###############################################################

	Simple Cocoa Wrappers Library 1.0
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
namespace
{

class My_AutoPool
{
public:
	My_AutoPool		();
	~My_AutoPool	();

protected:

private:
	NSAutoreleasePool*		_pool;
};

} // anonymous namespace

#pragma mark Internal Method prototypes
namespace
{
NSString*	findFolder			(short, OSType);
NSString*	returnPathForFSRef	(FSRef const&);
} // anonymous namespace



#pragma mark Public Methods

/*!
Initializes a Cocoa application by calling
NSApplicationLoad().

(1.0)
*/
Boolean
CocoaBasic_ApplicationLoad ()
{
	My_AutoPool		_;
	BOOL			loadOK = NSApplicationLoad();
	
	
	return loadOK;
}


/*!
Plays the sound with the given name (no extension), if it
is found anywhere in the standard search paths like
/System/Library/Sounds, etc.

See also CocoaBasic_ReturnUserSoundNames().

(1.0)
*/
void
CocoaBasic_PlaySoundByName	(CFStringRef	inName)
{
	My_AutoPool		_;
	
	
	[[NSSound soundNamed:REINTERPRET_CAST(inName, NSString const*)] stop];
	[[NSSound soundNamed:REINTERPRET_CAST(inName, NSString const*)] play];
}


/*!
Plays the sound in the specified file (asynchronously).

(1.0)
*/
void
CocoaBasic_PlaySoundFile	(CFURLRef	inFile)
{
	My_AutoPool		_;
	
	
	[[[[NSSound alloc] initWithContentsOfURL:REINTERPRET_CAST(inFile, NSURL const*) byReference:NO] autorelease] play];
}


/*!
Returns an unsorted list of names (without extensions) that are
valid sounds to pass to CocoaBasic_PlaySoundByName().  This is
useful for displaying a set of choices, such as a menu, to the
user.

(1.0)
*/
CFArrayRef
CocoaBasic_ReturnUserSoundNames ()
{
	My_AutoPool				_;
	NSArray* const			kSearchPaths = [NSArray arrayWithObjects:
												findFolder(kUserDomain, kSystemSoundsFolderType),
												findFolder(kLocalDomain, kSystemSoundsFolderType),
												findFolder(kSystemDomain, kSystemSoundsFolderType),
												nil];
	NSArray* const			kSupportedFileTypes = [NSSound soundUnfilteredFileTypes];
	NSFileManager* const	kFileManager = [NSFileManager defaultManager];
	NSEnumerator*			toSearchPath = [kSearchPaths objectEnumerator];
	BOOL					isDirectory = NO;
	NSMutableArray*			result = [NSMutableArray array];
	
	
	while (NSString* soundDirectory = [toSearchPath nextObject])
	{
		if ([kFileManager fileExistsAtPath:soundDirectory isDirectory:&isDirectory])
		{
			if (isDirectory)
			{
				NSEnumerator*	toFile = [[kFileManager directoryContentsAtPath:soundDirectory] objectEnumerator];
				
				
				while (NSString* soundFile = [toFile nextObject])
				{
					if ([kSupportedFileTypes containsObject:[soundFile pathExtension]])
					{
						[result addObject:[soundFile stringByDeletingPathExtension]];
					}
				}
			}
		}
	}
	
	[result retain];
	return REINTERPRET_CAST(result, CFArrayRef);
}


#pragma mark Internal Methods
namespace
{

My_AutoPool::
My_AutoPool (): _pool([[NSAutoreleasePool alloc] init])
{
}// My_AutoPool constructor


My_AutoPool::
~My_AutoPool ()
{
	[_pool release];
}// My_AutoPool destructor


/*!
Returns the path of the specified folder.  The domain is
one that is valid for FSFindFolder, e.g. kUserDomain or
kSystemDomain or a volume.

The result is always defined, which is convenient for
such things as initializing arrays.  Though it may be an
empty string (if there was a problem) or a path to a
folder that does not exist yet.

(1.0)
*/
NSString*
findFolder	(short		inDomain,
			 OSType		inType)
{
	OSStatus	error = noErr;
	FSRef		fileOrFolder;
	NSString*	result = nil;
	
	
	error = FSFindFolder(inDomain, inType, kDontCreateFolder, &fileOrFolder);
	if (noErr == error)
	{
		result = returnPathForFSRef(fileOrFolder);
	}
	if (nil == result) result = [NSString string];
	return result;
}// findFolder


/*!
Converts a Carbon-style FSRef for an existing file or folder
into a path as required by Cocoa routines.

(1.0)
*/
NSString*
returnPathForFSRef	(FSRef const&	inFileOrFolder)
{
	NSString*	result = nil;
	CFURLRef	urlRef = CFURLCreateFromFSRef(kCFAllocatorDefault, &inFileOrFolder);
	
	
	if (nil != urlRef)
	{
		NSURL*		urlObject = (NSURL*)urlRef;
		
		
		result = [urlObject path];
	}
	return result;
}// returnPathForFSRef

}// anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
