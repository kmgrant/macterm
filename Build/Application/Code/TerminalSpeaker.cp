/*###############################################################

	TerminalSpeaker.cp
	
	MacTelnet
		© 1998-2008 by Kevin Grant.
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
#include <stdexcept>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>

// library includes
#include <Console.h>
#include <ListenerModel.h>
#include <MemoryBlockPtrLocker.template.h>
#include <MemoryBlocks.h>
#include <SoundSystem.h>

// resource includes
#include "GeneralResources.h"

// MacTelnet includes
#include "AppResources.h"
#include "ConstantsRegistry.h"
#include "Terminal.h"
#include "TerminalScreenRef.typedef.h"
#include "TerminalSpeaker.h"



#pragma mark Types

struct TerminalSpeakerSpeechHandler
{
	TerminalSpeakerSpeechHandler ();
	
	void		initializeBuffer	(Str255&	inoutBuffer);
	
	Boolean					enabled;		//!< will the computer speak incoming text from this terminal?
	SpeechChannel			channel;		//!< where text is spoken
	Str255					buffer;			//!< place to hold text that has not yet been spoken
};

struct TerminalSpeaker
{
	TerminalSpeaker	(TerminalScreenRef);
	~TerminalSpeaker ();
	
	TerminalScreenRef				screen;					//!< source of terminal screen data
	Boolean							isEnabled;				//!< if true, this speaker can emit sound; otherwise, no sound generation APIs work
	Boolean							isPaused;				//!< if true, sound playback is suspended partway through
	ListenerModel_ListenerRef		bellHandler;			//!< invoked by a terminal screen buffer when it encounters a bell signal
	TerminalSpeakerSpeechHandler	speech;					//!< information on speech synthesis
	TerminalSpeaker_Ref				selfRef;				//!< redundant opaque reference that would resolve to point to this structure
};
typedef TerminalSpeaker*		TerminalSpeakerPtr;
typedef TerminalSpeaker const*	TerminalSpeakerConstPtr;

typedef MemoryBlockPtrLocker< TerminalSpeaker_Ref, TerminalSpeaker >	TerminalSpeakerPtrLocker;

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	TerminalSpeakerPtrLocker&	gTerminalSpeakerPtrLocks ()	{ static TerminalSpeakerPtrLocker x; return x; }
	Boolean						gTerminalSoundsOff = false;		//!< global flag that can nix all sound if set
}

#pragma mark Internal Method Prototypes

static void		audioEvent		(ListenerModel_Ref, ListenerModel_Event, void*, void*);



#pragma mark Public Methods

/*!
Creates a new terminal speaker that bases its input
on the specified screen source.

If any problems occur, nullptr is returned; otherwise,
a reference to the new terminal speaker is returned.

(3.0)
*/
TerminalSpeaker_Ref
TerminalSpeaker_New		(TerminalScreenRef		inScreenDataSource)
{
	TerminalSpeaker_Ref		result = nullptr;
	
	
	try
	{
		result = REINTERPRET_CAST(new TerminalSpeaker(inScreenDataSource), TerminalSpeaker_Ref);
	}
	catch (std::exception const&	inException)
	{
		Console_WriteLine(inException.what());
		result = nullptr;
	}
	return result;
}// New


/*!
This method cleans up a terminal speaker by destroying all
of the data associated with it.  On output, your copy of
the given reference will be set to nullptr.

(3.0)
*/
void
TerminalSpeaker_Dispose   (TerminalSpeaker_Ref*		inoutRefPtr)
{
	if (gTerminalSpeakerPtrLocks().isLocked(*inoutRefPtr))
	{
		Console_WriteValue("warning, attempt to dispose of locked terminal speaker; outstanding locks",
							gTerminalSpeakerPtrLocks().returnLockCount(*inoutRefPtr));
	}
	else
	{
		TerminalSpeakerPtr		ptr = gTerminalSpeakerPtrLocks().acquireLock(*inoutRefPtr);
		
		
		delete ptr;
		gTerminalSpeakerPtrLocks().releaseLock(*inoutRefPtr, &ptr);
		*inoutRefPtr = nullptr;
	}
}// Dispose


/*!
Returns "true" only if global mute is on, which means
that no speaker can emit sound even if it is unmuted.
Use TerminalSpeaker_SetGloballyMuted() to change this
setting; note that unmuting any particular speaker
will not have any effect on global mute, and individual
speakers may remain quiet.

(3.0)
*/
Boolean
TerminalSpeaker_IsGloballyMuted ()
{
	return gTerminalSoundsOff;
}// IsGloballyMuted


/*!
Returns "true" only if the specified speaker is on.  A
speaker that is off will not emit sound even if told to
do so.

(3.0)
*/
Boolean
TerminalSpeaker_IsMuted		(TerminalSpeaker_Ref	inSpeaker)
{
	TerminalSpeakerPtr	ptr = gTerminalSpeakerPtrLocks().acquireLock(inSpeaker);
	Boolean				result = false;
	
	
	result = ptr->isEnabled;
	gTerminalSpeakerPtrLocks().releaseLock(inSpeaker, &ptr);
	return result;
}// IsMuted


/*!
Returns "true" only if the specified speakerÕs sound
playback has been interrupted.  Playback can be resumed
using TerminalSpeaker_SetPaused().

(3.0)
*/
Boolean
TerminalSpeaker_IsPaused	(TerminalSpeaker_Ref	inSpeaker)
{
	TerminalSpeakerPtr	ptr = gTerminalSpeakerPtrLocks().acquireLock(inSpeaker);
	Boolean				result = false;
	
	
	result = ptr->isPaused;
	gTerminalSpeakerPtrLocks().releaseLock(inSpeaker, &ptr);
	return result;
}// IsPaused


/*!
Turns off sound for all speakers, overriding any
mute or unmute setting they may otherwise have.
Individual speakers retain their latent mute state,
so any speaker that was muted before the global
mute setting takes effect will remain muted if the
global mute is turned off, etc.

(3.0)
*/
void
TerminalSpeaker_SetGloballyMuted	(Boolean	inTurnOffAllSpeakers)
{
	gTerminalSoundsOff = inTurnOffAllSpeakers;
}// SetGloballyMuted


/*!
Turns off sound for a particular speaker, not
affecting other speakers.

(3.0)
*/
void
TerminalSpeaker_SetMuted	(TerminalSpeaker_Ref	inSpeaker,
							 Boolean				inTurnOffSpeaker)
{
	TerminalSpeakerPtr		ptr = gTerminalSpeakerPtrLocks().acquireLock(inSpeaker);
	
	
	ptr->isEnabled = !inTurnOffSpeaker;
	gTerminalSpeakerPtrLocks().releaseLock(inSpeaker, &ptr);
}// SetMuted


/*!
Turns off sound for a particular speaker, not
affecting other speakers.

(3.0)
*/
void
TerminalSpeaker_SetPaused	(TerminalSpeaker_Ref	inSpeaker,
							 Boolean				inInterruptSoundPlayback)
{
	TerminalSpeakerPtr		ptr = gTerminalSpeakerPtrLocks().acquireLock(inSpeaker);
	
	
	ptr->isPaused = inInterruptSoundPlayback;
	if (ptr->isPaused)
	{
		(OSStatus)PauseSpeechAt(ptr->speech.channel, kImmediate);
	}
	else
	{
		(OSStatus)ContinueSpeech(ptr->speech.channel);
	}
	gTerminalSpeakerPtrLocks().releaseLock(inSpeaker, &ptr);
}// SetPaused


/*!
Plays the terminal bell sound, which is either a
standard system beep or the bell sound file provided
by the user.

\retval kTerminalSpeaker_ResultOK
if the bell sound was loaded and played successfully

\retval kTerminalSpeaker_ResultSoundPlayFailed
if any problems occurred

(3.0)
*/
void
TerminalSpeaker_SoundBell	(TerminalSpeaker_Ref	inSpeaker)
{
	unless ((gTerminalSoundsOff) || TerminalSpeaker_IsMuted(inSpeaker))
	{
		// UNIMPLEMENTED - add back the ability to play a custom sound file
		Sound_StandardAlert();
	}
}// SoundBell


/*!
Speaks the specified text buffer using the same speech
channel as the specified speaker, interrupting any
current speech on that channel.  You might use this to
speak terminal-specific things, such as the current
selection of text.

\retval kTerminalSpeaker_ResultOK
if the speech was generated and played successfully

\retval kTerminalSpeaker_ResultSpeechSynthesisTryAgain
if the buffer was not spoken because the synthesizer is
busy (try again)

\retval kTerminalSpeaker_ResultSpeechSynthesisFailed
if any problems occurred

(3.0)
*/
TerminalSpeaker_Result
TerminalSpeaker_SynthesizeSpeechFromBuffer	(TerminalSpeaker_Ref	inSpeaker,
											 void const*			inBuffer,
											 Size					inBufferSize)
{
	TerminalSpeakerPtr			ptr = gTerminalSpeakerPtrLocks().acquireLock(inSpeaker);
	TerminalSpeaker_Result		result = kTerminalSpeaker_ResultOK;
	
	
	if (ptr != nullptr)
	{
		OSStatus	error = SpeakBuffer(ptr->speech.channel, inBuffer, inBufferSize, 0L/* flags */);
		
		
		if (error != noErr)
		{
			switch (error)
			{
			case synthNotReady:
				result = kTerminalSpeaker_ResultSpeechSynthesisTryAgain;
				break;
			
			default:
				result = kTerminalSpeaker_ResultSpeechSynthesisFailed;
				break;
			}
		}
	}
	gTerminalSpeakerPtrLocks().releaseLock(inSpeaker, &ptr);
	return result;
}// SynthesizeSpeechFromBuffer


#pragma mark Internal Methods

/*!
Creates and initializes a buffer for processing speech.

(3.0)
*/
TerminalSpeakerSpeechHandler::
TerminalSpeakerSpeechHandler ()
:
enabled(false),
channel(nullptr),
buffer()
{
	//OSStatus	error = NewSpeechChannel(nullptr/* voice */, &this->channel);
	
	
	//if (noErr != error) throw std::runtime_error("cannot create speech channel");
	initializeBuffer(this->buffer);
}// TerminalSpeakerSpeechHandler default constructor


/*!
Initializes the specified buffer and returns it.

(3.1)
*/
void
TerminalSpeakerSpeechHandler::
initializeBuffer	(Str255&	inoutBuffer)
{
	PLstrcpy(inoutBuffer, "\p");
}// initializeBuffer


/*!
Creates a new real screen, complete with virtual
screen storage.  The screen ID of the new screen is
returned, or a negative value if any problems occur.

(3.0)
*/
TerminalSpeaker::
TerminalSpeaker		(TerminalScreenRef		inScreenDataSource)
:
screen(inScreenDataSource),
isEnabled(true),
isPaused(false),
bellHandler(ListenerModel_NewStandardListener(audioEvent, this/* context, as TerminalSpeaker_Ref */)),
speech(),
selfRef(REINTERPRET_CAST(this, TerminalSpeaker_Ref))
{
	// ask to be notified of terminal bells
	Terminal_StartMonitoring(this->screen, kTerminal_ChangeAudioEvent, this->bellHandler);
	Terminal_StartMonitoring(this->screen, kTerminal_ChangeText, this->bellHandler);
}// TerminalSpeaker one-argument constructor


/*!
Destroys a terminal speaker created with TerminalSpeaker_New(),
freeing all resources allocated by that routine.

(3.0)
*/
TerminalSpeaker::
~TerminalSpeaker ()
{
	Terminal_StopMonitoring(this->screen, kTerminal_ChangeAudioEvent, this->bellHandler);
	Terminal_StopMonitoring(this->screen, kTerminal_ChangeText, this->bellHandler);
	ListenerModel_ReleaseListener(&this->bellHandler);
	if (nullptr != this->speech.channel)
	{
		(OSStatus)DisposeSpeechChannel(this->speech.channel), this->speech.channel = nullptr;
	}
}// TerminalSpeaker destructor


/*!
Responds to terminal bells audibly.

(3.0)
*/
static void
audioEvent	(ListenerModel_Ref		UNUSED_ARGUMENT(inUnusedModel),
			 ListenerModel_Event	inEventThatOccurred,
			 void*					UNUSED_ARGUMENT(inEventContextPtr),
			 void*					inTerminalSpeakerRef)
{
	TerminalSpeaker_Ref		inSpeaker = REINTERPRET_CAST(inTerminalSpeakerRef, TerminalSpeaker_Ref);
	
	
	switch (inEventThatOccurred)
	{
	case kTerminal_ChangeAudioEvent:
		{
			//TerminalScreenRef	inScreen = REINTERPRET_CAST(inEventContextPtr, TerminalScreenRef);
			
			
			TerminalSpeaker_SoundBell(inSpeaker);
		}
		break;
	
	case kTerminal_ChangeText:
	#if 0
		{
			Boolean		doSpeak = true;
			
			
			// speak the contents of each line of text that changed
			if (doSpeak)
			{
				Terminal_RangeDescriptionConstPtr	rangeInfoPtr = REINTERPRET_CAST(inEventContextPtr,
																					Terminal_RangeDescriptionConstPtr);
				register SInt16						i = 0;
				
				
				for (i = rangeInfoPtr->firstRow; i < (rangeInfoPtr->firstRow + rangeInfoPtr->rowCount); ++i)
				{
					Terminal_LineRef	rowIterator = nullptr;
					char				buffer[255/* arbitrary */];
					
					
					// WARNING: This is very inefficient!  Finding iterators based on indices
					//          is known to be slow, but at the moment this is the easiest
					//          option.  This must improve for better performance.
					if (rangeInfoPtr->firstRow >= 0)
					{
						// main screen lines are compatible with the signed system and
						// remain unchanged when constructing iterators
						rowIterator = Terminal_NewMainScreenLineIterator(rangeInfoPtr->screen, rangeInfoPtr->firstRow);
					}
					else
					{
						// -1 is ordinarily the 1st scrollback row in the signed system,
						// but iterators are constructed with unsigned values where 0 is
						// the newest row (so -1 must become 0, -2 must become 1, etc.)
						rowIterator = Terminal_NewScrollbackLineIterator(rangeInfoPtr->screen, -(rangeInfoPtr->firstRow) - 1);
					}
					
					if (nullptr != rowIterator)
					{
						Terminal_Result		getResult = kTerminal_ResultOK;
						char const*			rangeStart = nullptr;
						char const*			rangePastEnd = nullptr;
						
						
						getResult = Terminal_GetLineRange(rangeInfoPtr->screen, rowIterator,
															rangeInfoPtr->firstColumn,
															rangeInfoPtr->firstColumn + rangeInfoPtr->columnCount,
															rangeStart, rangePastEnd);
						if (kTerminal_ResultOK == getResult)
						{
							TerminalSpeaker_Result		speechResult = kTerminalSpeaker_ResultOK;
							
							
							// TEMPORARY - spin lock, to keep asynchronous speech from jumbling multi-line text;
							//             this really should be changed to use speech callbacks instead
							do
							{
								speechResult = TerminalSpeaker_SynthesizeSpeechFromBuffer
												(inSpeaker, rangeStart, rangePastEnd - rangeStart);
							} while (speechResult == kTerminalSpeaker_ResultSpeechSynthesisTryAgain);
						}
						Terminal_DisposeLineIterator(&rowIterator);
					}
				}
			}
		}
	#endif
		break;
	
	default:
		// ???
		break;
	}
}// audioEvent

// BELOW IS REQUIRED NEWLINE TO END FILE
