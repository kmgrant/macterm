/*!	\file SoundSystem.cp
	\brief Simplified interfaces for sound.
*/
/*###############################################################

	Interface Library 2.4
	© 1998-2011 by Kevin Grant
	
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

#include <SoundSystem.h>
#include <UniversalDefines.h>

// standard-C++ includes
#include <algorithm>
#include <vector>

// library includes
#include <MemoryBlocks.h>

// Mac includes
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>



#pragma mark Constants
namespace {

enum
{
	// The following are possible values for the “param1” field
	// of a SndCommand structure, as defined for the internal
	// sound callback routine, soundCallBack().
	kSoundCallBackParam1SoundComplete = 1
};

} // anonymous namespace

#pragma mark Types
namespace {

struct SoundInfo
{
	SndChannelPtr	channelPtr;				// the sound channel this sound is playing on
	Handle			resourceHandle;			// for resource-based sounds, a non-nullptr value; otherwise, nullptr
	Boolean			shouldNowBeDestroyed;	// this allows memory to be disposed of outside of the interrupt-level callback
};
typedef struct SoundInfo	SoundInfo;
typedef SoundInfo*			SoundInfoPtr;
typedef SoundInfoPtr*		SoundInfoHandle;

typedef std::vector< SoundInfoRef >		SoundInfoRefContainer;

} // anonymous namespace

#pragma mark Variables
namespace {

SoundInfoRefContainer&	gAsyncPlayingSoundsList()   { static SoundInfoRefContainer x; return x; }

} // anonymous namespace

#pragma mark Internal Method Prototypes
namespace {

void			disposeSoundInfoRef		(SoundInfoRef*);
SoundInfoRef	newSoundInfoRef			();
SoundInfoPtr	refAcquireLock			(SoundInfoRef);
void			refReleaseLock			(SoundInfoRef, SoundInfoPtr*);
void			soundCallBack			(SndChannelPtr, SndCommand*);

} // anonymous namespace



#pragma mark Public Methods

/*!
If you use this module, you should invoke the
Sound_Idle() routine once in your main event
loop.  This allows asynchronous sounds to be
destroyed when they have finished playing.

(1.0)
*/
void
Sound_Idle ()
{
	SoundInfoRef						ref = nullptr;
	SoundInfoPtr						ptr = nullptr;
	SoundInfoRefContainer::iterator		soundInfoIterator;
	SoundInfoRefContainer				toBeDestroyed;
	
	
	// find all asynchronous sounds that can be released
	for (soundInfoIterator = gAsyncPlayingSoundsList().begin(); soundInfoIterator != gAsyncPlayingSoundsList().end();
			++soundInfoIterator)
	{
		ref = *soundInfoIterator;
		ptr = refAcquireLock(ref);
		if (ptr != nullptr)
		{
			if (ptr->shouldNowBeDestroyed)
			{
				// should be disposed; flag this reference
				toBeDestroyed.push_back(ref);
			}
			refReleaseLock(ref, &ptr);
		}
	}
	
	// delete finished sounds from the asynchronous sounds list, and free their data structures
	for (soundInfoIterator = toBeDestroyed.begin(); soundInfoIterator != toBeDestroyed.end(); ++soundInfoIterator)
	{
		gAsyncPlayingSoundsList().erase(std::remove(gAsyncPlayingSoundsList().begin(), gAsyncPlayingSoundsList().end(), *soundInfoIterator),
										gAsyncPlayingSoundsList().end());
		disposeSoundInfoRef(&(*soundInfoIterator));
	}
}// Idle


/*!
You can use this routine to asynchronously play a
sound stored in a resource in the current resource
file.  If the specified resource type is not
recognized as a different kind of sound, it is
assumed to be a standard sound resource ('snd ').
This flexibility allows this routine to be
expanded in the future to accommodate other sound
resource types seamlessly.

Asynchronous sound play requires that you invoke
the Sound_Idle() routine at least once through your
main event loop.  Otherwise, when an asynchronous
sound finishes playing, it will remain in memory
and not be possible to free!

You usually specify nullptr for the last parameter,
unless you want to install a sound completion
routine that requires you to be able to identify
which sound has completed.  If you provide a
storage area for a sound information reference,
this routine will provide a reference to the sound
that begins playing when you call this routine.
Then, if you installed a completion routine, you
will be able to tell which sound has finished
playing.

(1.0)
*/
OSStatus
Sound_PlayAsyncStereo	(OSType			inResourceType,
						 SInt16			inResourceID,
						 SoundInfoRef*	outRefOrNull)
{
	OSStatus		result = noErr;
	SndChannelPtr	channelPtr = nullptr;
	
	
	result = SndNewChannel(&channelPtr, sampledSynth, initStereo, NewSndCallBackUPP(soundCallBack));
	if ((result == noErr) && (channelPtr != nullptr))
	{
		SoundInfoRef	ref = newSoundInfoRef();
		
		
		if (outRefOrNull != nullptr) *outRefOrNull = ref;
		
		// add a new sound to the list of asynchronous, playing sounds
		if (ref != nullptr)
		{
			SoundInfoPtr	ptr = refAcquireLock(ref);
			
			
			ptr->resourceHandle = Get1Resource(inResourceType, inResourceID);
			result = ResError();
			if ((ptr->resourceHandle != nullptr) && (result == noErr))
			{
				// this allows the sound data pointer to be acquired directly in the callback without linked list traversal
				channelPtr->userInfo = (long)ptr;
				
				// add the new sound reference to the linked list
				gAsyncPlayingSoundsList().push_back(ref);
				
				// start the sound playing
				DetachResource(ptr->resourceHandle);
				HLock(ptr->resourceHandle);
				result = SndPlay(channelPtr, (SndListHandle)ptr->resourceHandle, true/* asynchronous */);
				if (result == noErr)
				{
					SndCommand		commandInfo;
					
					
					commandInfo.cmd = callBackCmd;
					commandInfo.param1 = kSoundCallBackParam1SoundComplete;
					commandInfo.param2 = 0L; // the A5 world, normally copied into param2, is not used by PowerPC-native applications
					result = SndDoCommand(channelPtr, &commandInfo, false);
				}
			}
			else
			{
				// there was a problem grabbing the resource, so free up the acquired sound channel
				(OSStatus)SndDisposeChannel(channelPtr, false/* make quiet */);
				ptr->resourceHandle = nullptr;
			}
			refReleaseLock(ref, &ptr);
		}
	}
	
	return result;
}// PlayAsyncStereo


/*!
You can use this routine to synchronously play a
sound stored in a resource in the current resource
file.  If the specified resource type is not
recognized as a different kind of sound, it is
assumed to be a standard sound resource ('snd ').
This flexibility allows this routine to be
expanded in the future to accommodate other sound
resource types seamlessly.

(1.0)
*/
OSStatus
Sound_PlaySyncStereo	(OSType		inResourceType,
						 SInt16		inResourceID)
{
	OSStatus		result = noErr;
	SndChannelPtr	channelPtr = nullptr;
	
	
	result = SndNewChannel(&channelPtr, sampledSynth, initStereo, nullptr/* callback */);
	if (result == noErr)
	{
		Handle		soundHandle = Get1Resource(inResourceType, inResourceID);
		
		
		if ((ResError() == noErr) && (soundHandle != nullptr) && (channelPtr != nullptr))
		{
			DetachResource(soundHandle);
			//channelPtr->userInfo = (long)soundHandle;
			HLock(soundHandle);
			result = SndPlay(channelPtr, (SndListHandle)soundHandle, false/* asynchronous */);
			Memory_DisposeHandle(&soundHandle);
		}
		(OSStatus)SndDisposeChannel(channelPtr, false/* make quiet */);
	}
	
	return result;
}// PlaySyncStereo


/*!
Plays a standard alert sound, using the most advanced
sound API available.

(3.0)
*/
void
Sound_StandardAlert ()
{
	AlertSoundPlay();
	//SysBeep(32);
}// StandardAlert


#pragma mark Internal Methods
namespace {

/*!
To free memory for a copy of the internal structure
that was created with newSoundInfoRef(), use this
method.

(1.0)
*/
void
disposeSoundInfoRef	(SoundInfoRef*	inoutRefPtr)
{

	if (inoutRefPtr != nullptr)
	{
		SoundInfoPtr	ptr = refAcquireLock(*inoutRefPtr);
		
		
		// destroy the associated sound channel, freeing it for use by anything else in the system that needs it
		(OSStatus)SndDisposeChannel(ptr->channelPtr, true/* make quiet */), ptr->channelPtr = nullptr;
		
		// the resource was detached, so do not ReleaseResource() here
		// WARNING: there seems to be a random potential for a crash here, I have no idea why!
		if (IsHandleValid(ptr->resourceHandle)) Memory_DisposeHandle(&ptr->resourceHandle);
		
		// now destroy the internal storage that was used for it
		Memory_DisposePtr(REINTERPRET_CAST(inoutRefPtr, Ptr*));
	}
}// disposeSoundInfoRef


/*!
To create a copy of the internal structure and
acquire a reference to it, use this method.

(1.0)
*/
SoundInfoRef
newSoundInfoRef ()
{
	SoundInfoRef	result = nullptr;
	
	
	result = REINTERPRET_CAST(Memory_NewPtr(sizeof(SoundInfo)), SoundInfoRef);
	if (result != nullptr)
	{
		SoundInfoPtr	ptr = refAcquireLock(result);
		
		
		ptr->channelPtr = nullptr;
		ptr->resourceHandle = nullptr;
		ptr->shouldNowBeDestroyed = false;
		refReleaseLock(result, &ptr);
	}
	
	return result;
}// newSoundInfoRef


/*!
Use this method to acquire a pointer to the internal
structure, given a reference to it.

(1.0)
*/
SoundInfoPtr
refAcquireLock	(SoundInfoRef	inRef)
{
	return ((SoundInfoPtr)inRef);
}// refAcquireLock


/*!
Use this method when you are finished using a pointer
to the internal structure.

(1.0)
*/
void
refReleaseLock		(SoundInfoRef	UNUSED_ARGUMENT(inRef),
					 SoundInfoPtr*	inoutPtr)
{
	// memory is not relocatable, so just nullify the pointer
	if (inoutPtr != nullptr) *inoutPtr = nullptr;
}// refReleaseLock


/*!
This routine searches the linked list of asynchronous
sounds to find the one playing on the given channel,
and destroys that item in the list (along with its
channel and any resource data).

(1.0)
*/
void
soundCallBack	(SndChannelPtr	inChannelPtr,
				 SndCommand*	inCmdPtr)
{
	if ((inCmdPtr != nullptr) && (inChannelPtr != nullptr))
	{
		SoundInfoPtr	ptr = nullptr;
		
		
		// the data pointer is stored in the user data of the sound channel
		ptr = REINTERPRET_CAST(inChannelPtr->userInfo, SoundInfoPtr);
		
		// now examine the request
		if (ptr != nullptr)
		{
			if (inCmdPtr->param1 == kSoundCallBackParam1SoundComplete)
			{
				// MARK the sound for destruction - WARNING, as an interrupt-level
				// routine, you can’t actually dispose memory here!
				ptr->shouldNowBeDestroyed = true;
			}
		}
	}
}// soundCallBack

} // anonymous namespace

// BELOW IS REQUIRED NEWLINE TO END FILE
