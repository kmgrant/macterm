/*!	\file StreamCapture.mm
	\brief Manages captures of streams (from a terminal) to a file.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#import "StreamCapture.h"
#import <UniversalDefines.h>

// Mac includes
#import <ApplicationServices/ApplicationServices.h>
#import <Carbon/Carbon.h>
#import <CoreServices/CoreServices.h>

// library includes
#import <CFRetainRelease.h>
#import <Console.h>
#import <MemoryBlockPtrLocker.template.h>
#import <SoundSystem.h>

// application includes
#import "Session.h"



#pragma mark Types
namespace {

struct My_StreamCapture
{
public:
	My_StreamCapture	(Session_LineEnding);
	~My_StreamCapture ();
	
	void
	endCapture ()
	{
		[this->captureFileHandle release], this->captureFileHandle = nil;
	}
	
	void
	writeNewLine ()
	{
		// IMPORTANT: should call within @try...@catch
		[this->captureFileHandle writeData:[BRIDGE_CAST(this->writtenNewLineSequence.returnCFStringRef(), NSString*) dataUsingEncoding:NSUTF8StringEncoding]];
	}
	
	NSFileHandle*		captureFileHandle;			//!< target for writing data (retained)
	CFRetainRelease		writtenNewLineSequence;		//!< the string to write for new-lines
};
typedef My_StreamCapture*			My_StreamCapturePtr;
typedef My_StreamCapture const*		My_StreamCaptureConstPtr;

typedef MemoryBlockPtrLocker< StreamCapture_Ref, My_StreamCapture >		My_StreamCapturePtrLocker;
typedef LockAcquireRelease< StreamCapture_Ref, My_StreamCapture >		My_StreamCaptureAutoLocker;

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

} // anonymous namespace

#pragma mark Variables
namespace {

My_StreamCapturePtrLocker&		gStreamCapturePtrLocks ()	{ static My_StreamCapturePtrLocker x; return x; }

} // anonymous namespace



#pragma mark Public Methods

/*!
Creates a new object to manage captures to a file, but does
not initiate any captures.  Returns "nullptr" if any problem
occurs.

(4.0)
*/
StreamCapture_Ref
StreamCapture_New	(Session_LineEnding		inLineEndings)
{
	StreamCapture_Ref	result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new My_StreamCapture(inLineEndings), StreamCapture_Ref);
	}
	catch (std::bad_alloc)
	{
		result = nullptr;
	}
	catch (StreamCapture_Result		inConstructionError)
	{
		Console_Warning(Console_WriteValue, "stream constructor error", inConstructionError);
	}
	return result;
}// New


/*!
Decreases the retain count of the specified capture object
and sets your copy of the reference to "nullptr".  If the
object is no longer in use, it is destroyed and any
associated captures are forced to end.

(4.0)
*/
void
StreamCapture_Release	(StreamCapture_Ref*		inoutRefPtr)
{
	if (gStreamCapturePtrLocks().isLocked(*inoutRefPtr))
	{
		Console_Warning(Console_WriteValue, "attempt to dispose of locked stream capture object; outstanding locks",
						gStreamCapturePtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		delete *(REINTERPRET_CAST(inoutRefPtr, My_StreamCapture**));
		*inoutRefPtr = nullptr;
	}
}// Release


/*!
Initiates a capture to a file, returning "true" only if
successful.  Any capture in progress is automatically
ended.

(4.0)
*/
Boolean
StreamCapture_Begin		(StreamCapture_Ref		inRef,
						 CFURLRef				inFileToOverwrite)
{
	Boolean		result = false;
	
	
	if (nullptr == inFileToOverwrite)
	{
		Console_Warning(Console_WriteLine, "failed to create stream capture; URL is undefined");
	}
	else
	{
		My_StreamCaptureAutoLocker	ptr(gStreamCapturePtrLocks(), inRef);
		NSError*					error = nil;
		
		
		ptr->endCapture();
		
		ptr->captureFileHandle = [NSFileHandle fileHandleForWritingToURL:BRIDGE_CAST(inFileToOverwrite, NSURL*)
																			error:&error];
		if (nil != error)
		{
			Console_Warning(Console_WriteValueCFString, "failed to create capture-file handle for URL, error", BRIDGE_CAST([error localizedDescription], CFStringRef));
		}
		else if (nil == ptr->captureFileHandle)
		{
			Console_Warning(Console_WriteValueCFString, "failed to create capture-file handle for URL", CFURLGetString(inFileToOverwrite));
		}
		else
		{
			[ptr->captureFileHandle truncateFileAtOffset:0];
			[ptr->captureFileHandle retain];
			result = true;
		}
	}
	
	return result;
}// Begin


/*!
Terminates any file capture in progress that is associated
with the specified object.

(4.0)
*/
void
StreamCapture_End	(StreamCapture_Ref		inRef)
{
	My_StreamCaptureAutoLocker	ptr(gStreamCapturePtrLocks(), inRef);
	
	
	ptr->endCapture();
}// End


/*!
Returns "true" only if there is a file capture in progress
for the specified object.

(4.0)
*/
Boolean
StreamCapture_InProgress	(StreamCapture_Ref		inRef)
{
	My_StreamCaptureAutoLocker	ptr(gStreamCapturePtrLocks(), inRef);
	Boolean						result = false;
	
	
	result = (nil != ptr->captureFileHandle);
	return result;
}// InProgress


/*!
Writes the specified text, in UTF-8 encoding, to the capture
file of the given object.  Has no effect if the stream is
invalid or closed.

The data must already be processed as valid UTF-8; this
routine does not check for incomplete character sequences.

The data should use line endings consistent with the settings
given at construction time, since translation may occur.

(4.0)
*/
void
StreamCapture_WriteUTF8Data		(StreamCapture_Ref		inRef,
								 UInt8 const*			inBuffer,
								 size_t					inLength)
{
	//My_StreamCaptureAutoLocker	ptr(gStreamCapturePtrLocks(), inRef); // why does this crash in the destructor...?!?
	My_StreamCapture*				ptr = (My_StreamCapture*)inRef; // TEMPORARY (should be able to use lock construct above)
	
	
	if ((nullptr == ptr) || (nil == ptr->captureFileHandle))
	{
		//Console_Warning(Console_WriteLine, "attempt to write to nonexistent or closed capture file"); // debug
	}
	else if (inLength > 0)
	{
		// TEMPORARY: "writeData:" is synchronous; future SDKs may
		// be able to support asynchronous mode (or, this could be
		// faked by splitting up the data stream into smaller parts
		// that are each written synchronously with breaks); though,
		// this is usually called by a string-echoing mechanism
		// that already breaks text into relatively short chunks
		@try
		{
			if (0)
			{
				// source and file line endings are the same; no translation needed
				// (note: currently cannot assume this; may revisit later)
				NSData*		writtenData = [NSData dataWithBytesNoCopy:CONST_CAST(inBuffer, UInt8*)/* this const_cast is safe when "freeWhenDone" is NO */
																		length:inLength freeWhenDone:NO];
				
				
				[ptr->captureFileHandle writeData:writtenData];
			}
			else
			{
				UInt8 const* const		pastEndBuffer = (inBuffer + inLength);
				UInt8 const*			totalBufferPtr = nullptr;
				__block UInt8 const*	localBufferPtr = nullptr;
				__block size_t			blockSize = 0;
				
				
				// The following block is called multiple times below; it
				// writes any text range accumulated in part of the loop
				// (the range resets on each new-line).
				void (^writeBlock)() = ^()
				{
					if (blockSize > 0)
					{
						[ptr->captureFileHandle writeData:[NSData dataWithBytes:localBufferPtr length:blockSize]];
						blockSize = 0;
						localBufferPtr = nullptr;
					}
				};
				
				// send all the data to the file, substituting new-line-like sequences
				// (if CR or LF appear alone, they translate to one new-line; if they
				// appear in a sequence, the pair translates to one new-line); this
				// also handles the possibility that the “CR, LF” sequence crosses the
				// boundary between multiple write requests
				for (totalBufferPtr = inBuffer; totalBufferPtr != pastEndBuffer; ++totalBufferPtr)
				{
					if ('\015' == *totalBufferPtr)
					{
						// translate to target new-line sequence; note that
						// the low-level pseudo-terminal device settings will
						// already affect what CR and LF can do so this
						// translation is relatively straightforward (expects
						// that only CR will have to be translated)
						writeBlock(); // first write the previously-accumulated text
						ptr->writeNewLine(); // translate to target new-line sequence
					}
					else
					{
						// text without new-line characters; gather as much as
						// possible and write it in a single call (either at
						// the end of the loop or when a new-line is found)
						++blockSize;
						if (nullptr == localBufferPtr)
						{
							localBufferPtr = totalBufferPtr;
						}
					}
				}
				writeBlock(); // write any remaining text that was accumulated
			}
		}
		@catch (NSException* e)
		{
			// write errors are not expected; if there is a problem,
			// abort the entire capture (closing the file if necessary)
			Console_Warning(Console_WriteValueCFString, "file capture write failed, exception", BRIDGE_CAST([e name], CFStringRef));
			ptr->endCapture();
			Sound_StandardAlert();
			// INCOMPLETE: should trigger user-visible error message
		}
	}
}// WriteUTF8Data


#pragma mark Internal Methods
namespace {

/*!
Constructor.  See StreamCapture_New().

Currently, the line endings argument is ignored, as global
user preferences are read instead.  This will change in the
future.

Throws a StreamCapture_Result if any problems occur.

(4.0)
*/
My_StreamCapture::
My_StreamCapture	(Session_LineEnding		inLineEndings)
:
// IMPORTANT: THESE ARE EXECUTED IN THE ORDER MEMBERS APPEAR IN THE CLASS.
captureFileHandle(nil),
writtenNewLineSequence()
{
	// set up line ending translation
	// TEMPORARY - this might be set up sooner, for the whole screen,
	// at the point the line ending setting is made
	switch (inLineEndings)
	{
	case kSession_LineEndingCR:
		this->writtenNewLineSequence.setWithRetain(CFSTR("\015"));
		break;
	
	case kSession_LineEndingCRLF:
		this->writtenNewLineSequence.setWithRetain(CFSTR("\015\012"));
		break;
	
	case kSession_LineEndingLF:
	default:
		this->writtenNewLineSequence.setWithRetain(CFSTR("\012"));
		break;
	}
}// My_StreamCapture 1-argument constructor


/*!
Destructor.  See StreamCapture_Release().

(4.0)
*/
My_StreamCapture::
~My_StreamCapture ()
{
	endCapture();
}// My_StreamCapture destructor

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
