/*###############################################################

	StreamCapture.cp
	
	MacTelnet
		© 1998-2009 by Kevin Grant.
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

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <CFRetainRelease.h>
#include <Console.h>
#include <FileSelectionDialogs.h>
#include <MemoryBlockPtrLocker.template.h>
#include <SoundSystem.h>

// MacTelnet includes
#include "Folder.h"
#include "Preferences.h"
#include "Session.h"
#include "StreamCapture.h"



#pragma mark Types
namespace {

struct My_StreamCapture
{
public:
	My_StreamCapture	(Session_LineEnding);
	~My_StreamCapture ();
	
	Session_LineEnding
	returnUserPreferredLineEndings ();
	
	Session_LineEnding		captureFileLineEndings;		//!< carriage return and/or line feed
	SInt16					captureFileRefNum;			//!< file reference number of opened capture file
	StreamCapture_Ref		selfRef;					//!< opaque reference that would resolve to a pointer to this structure
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
successful.

(4.0)
*/
Boolean
StreamCapture_Begin		(StreamCapture_Ref		inRef,
						 SInt16					inOpenWritableFile)
{
	My_StreamCaptureAutoLocker	ptr(gStreamCapturePtrLocks(), inRef);
	Boolean						result = false;
	
	
	ptr->captureFileRefNum = inOpenWritableFile;
	result = true;
	
	return result;
}// Begin


/*!
Terminates any file capture in progress that is associated
with the specified object.

Since the Stream Capture module does not open the capture
file, you must subsequently close it using FSClose().

(4.0)
*/
void
StreamCapture_End	(StreamCapture_Ref		inRef)
{
	My_StreamCaptureAutoLocker	ptr(gStreamCapturePtrLocks(), inRef);
	
	
	ptr->captureFileRefNum = 0;
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
	
	
	result = (0 != ptr->captureFileRefNum);
	return result;
}// InProgress


/*!
Returns the file reference number of any open capture
file associated with the specified object.  If no
capture is open, returns an invalid number (you can
also use StreamCapture_InProgress() to ascertain this).

(4.0)
*/
SInt16
StreamCapture_ReturnReferenceNumber		(StreamCapture_Ref		inRef)
{
	My_StreamCaptureAutoLocker	ptr(gStreamCapturePtrLocks(), inRef);
	SInt16						result = 0;
	
	
	result = ptr->captureFileRefNum;
	return result;
}// ReturnReferenceNumber


/*!
Writes the specified text, in UTF-8 encoding, to the capture
file of the given object.

(4.0)
*/
void
StreamCapture_WriteUTF8Data		(StreamCapture_Ref		inRef,
								 UInt8 const*			inBuffer,
								 size_t					inLength)
{
	My_StreamCaptureAutoLocker	ptr(gStreamCapturePtrLocks(), inRef);
	
	
	if (0 != ptr->captureFileRefNum)
	{
		Session_LineEnding const	kFileLineEndings = ptr->captureFileLineEndings;
		size_t const				kFileEndingLength = (kSession_LineEndingCRLF == kFileLineEndings) ? 2 : 1;
		size_t const				kCaptureBufferSize = 512;
		UInt8						chunkBuffer[kCaptureBufferSize];
		UInt8*						chunkBufferIterator = nullptr;
		UInt8 const*				paramBufferIterator = nullptr;
		UInt8						fileEndingData[kFileEndingLength];
		OSStatus					error = noErr;
		size_t						bytesLeft = inLength;
		SInt32						bytesInCaptureBuffer = 0;
		SInt32						chunkByteCount = 0;
		
		
		// initialize the iterator for the buffer given as input
		paramBufferIterator = inBuffer;
		
		// set up line ending translation
		// TEMPORARY - this might be set up sooner, for the whole screen,
		// at the point the line ending setting is made
		switch (kFileLineEndings)
		{
		case kSession_LineEndingCR:
			fileEndingData[0] = '\015';
			break;
		
		case kSession_LineEndingCRLF:
			fileEndingData[0] = '\015';
			fileEndingData[1] = '\012';
			break;
		
		case kSession_LineEndingLF:
		default:
			fileEndingData[0] = '\012';
			break;
		}
		
		// iterate over the input buffer until every byte has been
		// written, or until FSWrite() returns an error (big problem!);
		// if the new-line sentinel is seen, skip over the original
		// new-line sequence and write the preferred sequence instead
		while ((bytesLeft > 0) && (noErr == error))
		{
			// initialize the iterator for the chunk buffer
			chunkBufferIterator = chunkBuffer;
			bytesInCaptureBuffer = 0; // each time through, initially nothing important in the chunk buffer
			chunkByteCount = (bytesLeft < kCaptureBufferSize) ? bytesLeft : kCaptureBufferSize; // only copy data that’s there!!!
			
			// fill in the buffer until a new-line character is reached;
			// since a mixture of characters is likely in practice, stop
			// on the first carriage return or line-feed, regardless of
			// which “should” indicate a new-line
			while ((chunkByteCount > 0) && ('\015' != *paramBufferIterator) && ('\012' != *paramBufferIterator))
			{
				*chunkBufferIterator++ = *paramBufferIterator++;
				++bytesInCaptureBuffer;
				--chunkByteCount;
			}
			
			// write the chunk buffer to disk - the number of bytes
			// actually copied to it were tracked during the iteration;
			// FSWrite() will use that as input, and then on output
			// the value will CHANGE to be the number of bytes that
			// were actually successfully written to the file (so,
			// the mega-count "bytesLeft" takes into account the latter)
			error = FSWrite(ptr->captureFileRefNum, &bytesInCaptureBuffer, chunkBuffer);
			if (noErr == error)
			{
				bytesLeft -= bytesInCaptureBuffer;
				
				// if a new-line was found, write the preferred sequence
				// and skip over the actual sequence from the input
				if (('\015' == *paramBufferIterator) || ('\012' == *paramBufferIterator))
				{
					SInt32		dummySize = kFileEndingLength;
					
					
					error = FSWrite(ptr->captureFileRefNum, &dummySize, fileEndingData);
					while ((bytesLeft > 0) && (('\015' == *paramBufferIterator) || ('\012' == *paramBufferIterator)))
					{
						--bytesLeft;
						++paramBufferIterator;
					}
				}
			}
		}
		
		// this REALLY should trigger a user alert of some sort
		if (noErr != error)
		{
			StreamCapture_End(inRef);
			Console_WriteValue("file capture unexpectedly terminated, error", error);
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
captureFileLineEndings(returnUserPreferredLineEndings()),
captureFileRefNum(0),
selfRef(REINTERPRET_CAST(this, StreamCapture_Ref))
{
}// My_StreamCapture 1-argument constructor


/*!
Destructor.  See StreamCapture_Release().

(4.0)
*/
My_StreamCapture::
~My_StreamCapture ()
{
	// INCOMPLETE
}// My_StreamCapture destructor


/*!
Reads "kPreferences_TagCaptureFileLineEndings" from a Preferences
context, and returns either that value or the default value if
none was found.

(4.0)
*/
Session_LineEnding
My_StreamCapture::
returnUserPreferredLineEndings ()
{
	Preferences_Result		prefsResult = kPreferences_ResultOK;
	Session_LineEnding		result = kSession_LineEndingLF; // arbitrary default
	
	
	// TEMPORARY - perhaps this routine should take a specific preferences context
	prefsResult = Preferences_GetData(kPreferences_TagCaptureFileLineEndings,
										sizeof(result), &result);
	return result;
}// returnUserPreferredLineEndings

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
