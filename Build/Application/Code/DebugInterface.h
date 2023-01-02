/*!	\file DebugInterface.h
	\brief Implements a utility window for enabling certain
	debugging features.
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



#pragma mark Variables

extern Boolean gDebugInterface_LogsSixelDecoderErrors;
extern Boolean gDebugInterface_LogsSixelDecoderState;
extern Boolean gDebugInterface_LogsSixelDecoderSummary;
extern Boolean gDebugInterface_LogsSixelInput;
extern Boolean gDebugInterface_LogsTerminalInputChar;
extern Boolean gDebugInterface_LogsTeletypewriterState;
extern Boolean gDebugInterface_LogsTerminalEcho;
extern Boolean gDebugInterface_LogsTerminalState;

#pragma mark Public Methods

void
	DebugInterface_Display					();

inline Boolean
	DebugInterface_LogsSixelDecoderErrors	()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsSixelDecoderErrors;
	#else
		return false;
	#endif
	}

inline Boolean
	DebugInterface_LogsSixelDecoderState	()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsSixelDecoderState;
	#else
		return false;
	#endif
	}

inline Boolean
	DebugInterface_LogsSixelDecoderSummary	()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsSixelDecoderSummary;
	#else
		return false;
	#endif
	}

inline Boolean
	DebugInterface_LogsSixelInput			()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsSixelInput;
	#else
		return false;
	#endif
	}

inline Boolean
	DebugInterface_LogsTerminalInputChar	()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsTerminalInputChar;
	#else
		return false;
	#endif
	}

inline Boolean
	DebugInterface_LogsTeletypewriterState	()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsTeletypewriterState;
	#else
		return false;
	#endif
	}

inline Boolean
	DebugInterface_LogsTerminalEcho			()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsTerminalEcho;
	#else
		return false;
	#endif
	}

inline Boolean
	DebugInterface_LogsTerminalState		()
	{
	#ifndef NDEBUG
		return gDebugInterface_LogsTerminalState;
	#else
		return false;
	#endif
	}

// BELOW IS REQUIRED NEWLINE TO END FILE
