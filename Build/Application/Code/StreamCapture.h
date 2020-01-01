/*!	\file StreamCapture.h
	\brief Manages captures of streams (from a terminal) to a file.
	
	Creating an object of this type does not initiate a capture
	to a file; it allows you to retain settings, and then
	initiate or terminate captures indefinitely.
*/
/*###############################################################

	MacTerm
		© 1998-2020 by Kevin Grant.
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
#include "Session.h"



#pragma mark Constants

/*!
Possible return values from certain APIs in this module.
*/
enum StreamCapture_Result
{
	kStreamCapture_ResultOK = 0,				//!< no error
	kStreamCapture_ResultInvalidID = -1,		//!< a given "StreamCapture_Ref" does not correspond to any known capture object
	kStreamCapture_ResultParameterError = -2,	//!< invalid input (e.g. a null pointer)
};

typedef struct StreamCapture_OpaqueStructure*	StreamCapture_Ref;	//!< represents a capture object



#pragma mark Public Methods

//!\name Creating and Destroying Stream Capture Objects
//@{

StreamCapture_Ref
	StreamCapture_New					(Session_LineEnding			inFileLineEndings);

void
	StreamCapture_Release				(StreamCapture_Ref*			inoutRefPtr);

//@}

//!\name Interaction With Stream Capture Objects
//@{

Boolean
	StreamCapture_Begin					(StreamCapture_Ref			inRef,
										 CFURLRef					inFileToOverwrite);

void
	StreamCapture_End					(StreamCapture_Ref			inRef);

Boolean
	StreamCapture_InProgress			(StreamCapture_Ref			inRef);

void
	StreamCapture_WriteUTF8Data			(StreamCapture_Ref			inRef,
										 UInt8 const*				inBuffer,
										 size_t						inLength);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
