/*!	\file MacroManager.h
	\brief Lists methods to access the strings associated with
	keyboard equivalents.
	
	The entire macro implementation has been re-written in
	MacTelnet 3.0 to be abstract, efficient and very powerful.
	The macro code has now been separated into two levels: high
	level ("MacroSetupWindow.cp") and low level (this file).
	
	MacTelnet 3.0 allows up to five sets of macros in memory at
	once, and allows the user to use any one set at once.  This
	code has now been written in such a way that it should be
	possible to change the number of macro sets supported, as
	well as the number of macros in each set, without too much
	trouble.  Note, however, that if you change the number of
	macros in each set you will have to adapt the user interface
	code to allow users to enter additional macros.
*/
/*###############################################################

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

#ifndef __MACROMANAGER__
#define __MACROMANAGER__

// library includes
#include <ListenerModel.h>

// Mac includes
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "SessionRef.typedef.h"



#pragma mark Constants

typedef SInt32 MacroManager_InvocationMethod;
enum
{
	kMacroManager_InvocationMethodCommandDigit		= 0,		//!< macros are invocable with command-0 to command-9, command-= and command-/
	kMacroManager_InvocationMethodFunctionKeys		= 1			//!< macros are invocable with F1 to F12
};

enum Macros_Change
{
	kMacros_ChangeActiveSetPlanned	= 'Plan',	//!< the active macro set will change (context: nullptr)
	kMacros_ChangeActiveSet			= 'Actv',	//!< the active macro set has changed (context: nullptr)
	kMacros_ChangeContents			= 'Text',	//!< the contents of some macro has changed (context:
												//!  MacroDescriptorPtr)
	kMacros_ChangeMode				= 'Mode'	//!< the macro invocation mode has changed (context:
												//!  MacroManager_InvocationMethod*)
};

enum
{
	MACRO_COUNT			= 12,		//!< number of macros supported in a given set
	MACRO_SET_COUNT		= 5			//!< number of sets (of MACRO_COUNT macros each) to choose from
};

#pragma mark Types

typedef UInt16		MacroIndex;			//!< a zero-based macro number (maximum value is therefore MACRO_COUNT - 1)
typedef Handle*		MacroSet;			//!< a reference to a collection of text strings, as well as information on
										//!  which keys invoke those strings
typedef UInt16		MacroSetNumber;		//!< a ONE-based set ID (maximum value is therefore MACRO_SET_COUNT)

struct MacroDescriptor
{
	MacroSet		set;	//!< which set the macro is from
	MacroIndex		index;	//!< position of the macro within the set it is in
};
typedef MacroDescriptor*	MacroDescriptorPtr;



#pragma mark Public Methods

/*###############################################################
	INITIALIZING AND FINISHING WITH MACRO SETS
###############################################################*/

// CALL THIS ROUTINE ONCE, BEFORE ANY OTHER MACRO SET ROUTINE
void
	Macros_Init							();

// CALL THIS ROUTINE AFTER YOU ARE PERMANENTLY DONE WITH MACRO SETS
void
	Macros_Done							();

/*###############################################################
	CREATING AND DESTROYING MACRO SETS
###############################################################*/

MacroSet
	Macros_NewSet						();

void
	Macros_DisposeSet					(MacroSet*							inoutSetToDestroy);

/*###############################################################
	GLOBAL MACRO ROUTINES
###############################################################*/

MacroSet
	Macros_ReturnActiveSet				();

MacroSetNumber
	Macros_ReturnActiveSetNumber		();

MacroManager_InvocationMethod
	Macros_ReturnMode					();

Boolean
	MacroManager_UserInputMacroString	(SessionRef							inSession,
										 MacroIndex							inZeroBasedMacroNumber);

// WARNING - THE MAXIMUM VALUE (ONE-BASED) YOU CAN SPECIFY IS ÒMACRO_SET_COUNTÓ
void
	Macros_SetActiveSetNumber			(MacroSetNumber						inNewActiveSetNumber);

void
	Macros_SetMode						(MacroManager_InvocationMethod		inNewMode);

/*###############################################################
	NOTIFICATION OF CHANGES
###############################################################*/

void
	Macros_StartMonitoring				(Macros_Change						inForWhatChange,
										 ListenerModel_ListenerRef			inListener);

void
	Macros_StopMonitoring				(Macros_Change						inForWhatChange,
										 ListenerModel_ListenerRef			inListener);

/*###############################################################
	UTILITIES FOR GETTING AND SETTING MACROS
###############################################################*/

// IF YOU PASS A NULL FILE, THE USER IS QUERIED (VIA NAVIGATION SERVICES) TO SPECIFY A FILE
void
	Macros_ExportToText					(MacroSet							inSet,
										 FSSpec*							inFileSpecPtrOrNull,
										 MacroManager_InvocationMethod		inMacroModeOfExportedSet);

void
	Macros_Get							(MacroSet							inFromWhichSet,
										 MacroIndex							inZeroBasedMacroNumber,
										 char*								outValue,
										 SInt16								inRoom);

// IF YOU PASS A NULL FILE, THE USER IS QUERIED (VIA NAVIGATION SERVICES) TO SPECIFY A FILE
Boolean
	Macros_ImportFromText				(MacroSet							inSet,
										 FSSpec const*						inFileSpecPtrOrNull,
										 MacroManager_InvocationMethod*		outMacroModeOfImportedSet);

void
	Macros_ParseTextBuffer				(MacroSet							inSet,
										 UInt8*								inBuffer,
										 Size								inSize,
										 MacroManager_InvocationMethod*		outMacroModeOfImportedSet);

void
	Macros_Set							(MacroSet							inFromWhichSet,
										 MacroIndex							inZeroBasedMacroNumber,
										 char const*						inValue);

/*###############################################################
	OTHER MACRO UTILITIES
###############################################################*/

Boolean
	Macros_AllEmpty						(MacroSet							inSet);

void
	Macros_Copy							(MacroSet							inSource,
										 MacroSet							inDestination);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
