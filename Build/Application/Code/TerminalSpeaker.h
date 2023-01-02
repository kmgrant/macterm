/*!	\file TerminalSpeaker.h
	\brief Whereas a Terminal View interprets the contents of a
	terminal screen buffer and user interaction visually, this
	module handles communication through sound.
	
	This is the Terminal Speaker module, which defines the audio
	component of a terminal screen.
	
	Generally, you only use Terminal Speaker APIs to manipulate
	things that are unique to audio interaction with a terminal,
	such as synthesizing voice or the terminal bell sound.
	Anything that is data-centric should be manipulated from the
	Terminal Screen standpoint (see Terminal.h), because data
	changes will eventually be propagated to the view for
	rendering and/or this module for sound.  So, expect only the
	Terminal Screen module to use most of these APIs.
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

// application includes
#include "TerminalScreenRef.typedef.h"



#pragma mark Constants

enum TerminalSpeaker_Result
{
	kTerminalSpeaker_ResultOK							= 0,	//!< no error occurred
	kTerminalSpeaker_ResultSoundPlayFailed				= 1,	//!< could not play a sampled sound for some reason
	kTerminalSpeaker_ResultSpeechSynthesisTryAgain		= 2,	//!< could not make the computer speak because the synthesizer is not ready
	kTerminalSpeaker_ResultSpeechSynthesisFailed		= 3		//!< could not make the computer speak for some reason
};

#pragma mark Types

typedef struct TerminalSpeaker_OpaqueType*		TerminalSpeaker_Ref;	//!< sound generator for terminal window



#pragma mark Public Methods

//!\name Creating and Destroying Terminal Speakers
//@{

TerminalSpeaker_Ref
	TerminalSpeaker_New							(TerminalScreenRef			inScreenDataSource);

void
	TerminalSpeaker_Dispose						(TerminalSpeaker_Ref*		inoutSpeakerPtr);

//@}

//!\name Configuring Speakers
//@{

Boolean
	TerminalSpeaker_IsGloballyMuted				();

Boolean
	TerminalSpeaker_IsMuted						(TerminalSpeaker_Ref		inSpeaker);

Boolean
	TerminalSpeaker_IsPaused					(TerminalSpeaker_Ref		inSpeaker);

void
	TerminalSpeaker_SetPaused					(TerminalSpeaker_Ref		inSpeaker,
												 Boolean					inInterruptSoundPlayback);

void
	TerminalSpeaker_SetMuted					(TerminalSpeaker_Ref		inSpeaker,
												 Boolean					inTurnOffSpeaker);

void
	TerminalSpeaker_SetGloballyMuted			(Boolean					inTurnOffSpeaker);

//@}

//!\name Playing Sound
//@{

void
	TerminalSpeaker_SoundBell					(TerminalSpeaker_Ref		inSpeaker);

TerminalSpeaker_Result
	TerminalSpeaker_SynthesizeSpeechFromCFString(TerminalSpeaker_Ref		inRef,
												 CFStringRef				inCFString);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
