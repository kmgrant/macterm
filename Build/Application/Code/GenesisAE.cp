/*###############################################################

	GenesisAE.cp
	
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

// standard-C includes
#include <cctype>
#include <cstring>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <Console.h>
#include <WindowInfo.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "ConstantsRegistry.h"
#include "GenesisAE.h"
#include "ObjectClassesAE.h"
#include "Terminology.h"



#pragma mark Internal Method Prototypes

static OSStatus		evolve					(ObjectClassesAE_TokenPtr, DescType*);
static OSStatus		evolveTerminalWindow	(ObjectClassesAE_TokenPtr, DescType*);
static OSStatus		evolveWindow			(ObjectClassesAE_TokenPtr, DescType*);



#pragma mark Public Methods

/*!
Creates any kind of MacTelnet scripting class.
The "Terminology.h" file, also used for MacTelnet’s
Apple Event Terminology Extension resource, contains
constants for all of MacTelnet’s class types.

IMPORTANT:	Before calling this routine, fill in the
			fields of the appropriate part of the
			"object" member of the "as" union, in
			your token data structure.  Provide any
			information you require, except for the
			"flags" and "as.object.eventClass", which
			are provided automatically by this method.

NOTE:	Classes are automatically evolved; unless
		there is no way to derive a child class’ data
		from the clues of its parent, you should only
		provide data for the least refined version of
		a class (for example, describe a "window",
		not a "clipboard window").  This helps to
		reduce redundant code, and makes it possible
		for the object accessors to be generic.

If any problems occur when creating the requested
class, an error is returned and the class should be
considered invalid with empty data.  Otherwise, you
must call the AEDisposeDesc() routine to destroy the
data in the descriptor when you are done with it.

(3.0)
*/
OSStatus
GenesisAE_CreateTokenFromObjectData		(DescType					inClassTypeFromTerminology,
										 ObjectClassesAE_TokenPtr   inoutClassStructurePtr,
										 AEDesc*					inoutNewClassAEDescPtr,
										 DescType*					outFinalClassTypeFromTerminologyOrNull)
{
	OSStatus	result = errAENoSuchObject;
	
	
	if (inoutClassStructurePtr != nullptr)
	{
		// define the data that identifies the token as being an object token
		inoutClassStructurePtr->flags |= kObjectClassesAE_TokenFlagsIsObject;
		inoutClassStructurePtr->as.object.eventClass = inClassTypeFromTerminology;
		
		// the rest of the data for the primitive type should already be in
		// the structure; now examine the data, and determine if the object
		// data can be expanded to deserve a more refined class
		evolve(inoutClassStructurePtr, outFinalClassTypeFromTerminologyOrNull);
		
		// put the data (evolved or not) into a new token descriptor
		result = AECreateDesc(cMyInternalToken, inoutClassStructurePtr,
								sizeof(*inoutClassStructurePtr), inoutNewClassAEDescPtr);
	}
	
	return result;
}// CreateTokenFromObjectData


/*!
Coerces an object into an internal token, if necessary.
Use this inside an accessor function to get access to
the internal data for an object’s container.

(3.0)
*/
OSStatus
GenesisAE_CreateTokenFromObjectSpecifier	(AEDesc const*				inFromWhat,
											 ObjectClassesAE_TokenPtr   outTokenDataPtr)
{
	OSStatus	result = noErr;
	
	
	if ((inFromWhat == nullptr) || (outTokenDataPtr == nullptr)) result = memPCErr;
	else
	{
		AEDesc		containerDescriptor;
		
		
		result = AppleEventUtilities_InitAEDesc(&containerDescriptor);
		assert_noerr(result);
		
		// the class owning the requested property is provided explicitly (coercion probably isn’t needed)
		result = AECoerceDesc(inFromWhat, cMyInternalToken, &containerDescriptor);
		if (result != noErr)
		{
			Console_WriteLine("tokenizing failed");
		}
		else
		{
			result = AppleEventUtilities_CopyDescriptorData(&containerDescriptor, outTokenDataPtr,
															sizeof(*outTokenDataPtr),
															nullptr/* actual size storage */);
			if (result != noErr)
			{
				if (result == memSCErr)
				{
					Console_WriteLine("WARNING - not enough buffer space was given, truncating...");
					result = noErr;
				}
				else
				{
					Console_WriteLine("attempt to get container type failed");
				}
			}
			// ??? necessary ???
			//AEDisposeDesc(&containerDescriptor);
		}
	}
	
	return result;
}// CreateTokenFromObjectSpecifier


/*!
Determines whether an instance of the first given class
“is also” an instance of the second class.  For example,
if the minimum acceptable class type is a "window" (which
is "cMyWindow"), and the unknown type is "cMyTerminalWindow",
this routine returns "true" because a terminal window can
certainly be downgraded to a window.

This function defines the inheritance trees that MacTelnet
supports for its Apple Event objects.  Be sure to keep
"ObjectClassesAE.h" in sync!!!

IMPORTANT:	It would be nice if the user terminology
			agreed with this function!  Be careful about
			making changes to inheritance code, and always
			ensure the scripting interface reflects changes
			or additions.

(3.0)
*/
Boolean
GenesisAE_FirstClassIs	(DescType	inUnknownClassTypeFromTerminology,
						 DescType	inMinimumClassTypeFromTerminology)
{
	Boolean		result = false;
	
	
	// check first for equality, which is obviously always "true";
	// also, this breaks the most frequent kind of recursion that
	// this function does
	result = (inUnknownClassTypeFromTerminology == inMinimumClassTypeFromTerminology);
	
	// if not equal, consider all child classes, recursively
	unless (result)
	{
		switch (inMinimumClassTypeFromTerminology)
		{
		case cMyWindow:
			// here, consider ONLY the direct subclasses of "window" (recursion handles the rest)
			result = (GenesisAE_FirstClassIs(inUnknownClassTypeFromTerminology, cMyTerminalWindow) ||
						GenesisAE_FirstClassIs(inUnknownClassTypeFromTerminology, cMyClipboardWindow));
			break;
		
		case cMyTerminalWindow:
			// here, consider ONLY the direct subclasses of "terminal window" (recursion handles the rest)
			result = GenesisAE_FirstClassIs(inUnknownClassTypeFromTerminology, cMySessionWindow);
			break;
		
		default:
			// can’t be converted
			break;
		}
	}
	return result;
}// FirstClassIs


#pragma mark Internal Methods

/*!
Object accessors create primitive types; this routine
can be used to “evolve” an object token.  This is done
by looking at the data for an object and determining,
first, if its class has any subclass types, and second,
if the data in the superclass is sufficient to create
a refined version of the token.

For example, it is possible to use data from a simple
"window" class to determine the "terminal window" data
for it, provided the window being referenced really is
a terminal window.

If a token can be evolved, it is evolved further, more
data is added to the token and its type is changed.
Otherwise, the token is not modified.

(3.0)
*/
static OSStatus
evolve		(ObjectClassesAE_TokenPtr   inoutClassStructurePtr,
			 DescType*					outFinalClassTypeFromTerminologyOrNull)
{
	OSStatus	result = noErr;
	DescType	descType = inoutClassStructurePtr->as.object.eventClass;
	
	
	// is the specified type of token even possible to evolve, first of all?
	switch (descType)
	{
	case cMyWindow:
		result = evolveWindow(inoutClassStructurePtr, outFinalClassTypeFromTerminologyOrNull);
		break;
	
	default:
		// the specified token can’t be evolved any further
		break;
	}
	
	if (outFinalClassTypeFromTerminologyOrNull != nullptr)
	{
		*outFinalClassTypeFromTerminologyOrNull = descType;
	}
	
	return result;
}// evolve


/*!
Like evolve(), except focused on classes that are some
kind of terminal window.

(3.0)
*/
static OSStatus
evolveTerminalWindow	(ObjectClassesAE_TokenPtr   inoutClassStructurePtr,
						 DescType*					outFinalClassTypeFromTerminologyOrNull)
{
	OSStatus			result = noErr;
	DescType			descType = inoutClassStructurePtr->as.object.eventClass;
	TerminalWindowRef	terminalWindow = inoutClassStructurePtr->as.object.data.terminalWindow.ref;
	
	
	Console_BeginFunction();
	Console_WriteLine("seeing if the terminal window can be refined");
	
	if (descType == cMyTerminalWindow)
	{
		SInt16		windowKind = GetWindowKind(TerminalWindow_ReturnWindow(terminalWindow));
		
		
		switch (windowKind)
		{
		case WIN_CNXN:
		case WIN_SHELL:
			// evolve to "session window"
			Console_WriteLine("promoting to a session window");
			// TEMPORARY - totally broken, will fix later
			//inoutClassStructurePtr->as.object.data.sessionWindow.session =
			//	ScreenFactory_GetWindowSession(inoutClassStructurePtr->as.object.data.window.ref);
			inoutClassStructurePtr->as.object.eventClass = cMySessionWindow;
			break;
		
		case WIN_CONSOLE:
		default:
			// not evolveable
			break;
		}
	}
	
	if (outFinalClassTypeFromTerminologyOrNull != nullptr)
	{
		*outFinalClassTypeFromTerminologyOrNull = descType;
	}
	
	Console_EndFunction();
	
	return result;
}// evolveTerminalWindow


/*!
Like evolve(), except focused on classes that are some
kind of window.

(3.0)
*/
static OSStatus
evolveWindow	(ObjectClassesAE_TokenPtr   inoutClassStructurePtr,
				 DescType*					outFinalClassTypeFromTerminologyOrNull)
{
	OSStatus	result = noErr;
	DescType	descType = inoutClassStructurePtr->as.object.eventClass;
	WindowRef	window = inoutClassStructurePtr->as.object.data.window.ref;
	
	
	Console_BeginFunction();
	Console_WriteLine("seeing if the window can be refined");
	
	if (descType == cMyWindow)
	{
		WindowInfo_Ref		windowInfo = WindowInfo_ReturnFromWindow(window);
		
		
		if (windowInfo != nullptr)
		{
			WindowInfo_Descriptor	windowDescriptor = kWindowInfo_InvalidDescriptor;
			
			
			windowDescriptor = WindowInfo_ReturnWindowDescriptor(windowInfo);
			switch (windowDescriptor)
			{
			case kConstantsRegistry_WindowDescriptorAnyTerminal:
				// evolve to "terminal window"
				inoutClassStructurePtr->as.object.data.terminalWindow.ref =
					TerminalWindow_ReturnFromWindow(inoutClassStructurePtr->as.object.data.window.ref);
				inoutClassStructurePtr->as.object.eventClass = cMyTerminalWindow;
				result = evolveTerminalWindow(inoutClassStructurePtr, outFinalClassTypeFromTerminologyOrNull);
				break;
			
			case kConstantsRegistry_WindowDescriptorClipboard:
				// evolve to "clipboard window"
				Console_WriteLine("promoting to a clipboard window");
				inoutClassStructurePtr->as.object.eventClass = cMyClipboardWindow;
				break;
			
			default:
				// not evolveable
				break;
			}
		}
	}
	
	if (outFinalClassTypeFromTerminologyOrNull != nullptr)
	{
		*outFinalClassTypeFromTerminologyOrNull = descType;
	}
	
	Console_EndFunction();
	
	return result;
}// evolveWindow

// BELOW IS REQUIRED NEWLINE TO END FILE
