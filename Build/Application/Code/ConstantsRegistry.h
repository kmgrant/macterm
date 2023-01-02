/*!	\file ConstantsRegistry.h
	\brief Collection of enumerations that must be unique
	across the application.
	
	This file contains collections of constants that must be
	unique within their set.  With all constants defined in one
	place, it is easy to see at a glance if there are any
	values that are not unique!
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

// library includes
#include <ListenerModel.h>



//!\name General Constants
//@{

/*!
Used in Cocoa with NSError, to specify a domain for all
application-generated errors.  The enumeration that follows
is for error codes.

For example:
	[NSError errorWithDomain:(NSString*)kConstantsRegistry_NSErrorDomainAppDefault
		code:kConstantsRegistry_NSErrorBadUserID . . .]
*/
CFStringRef const kConstantsRegistry_NSErrorDomainAppDefault				= CFSTR("net.macterm.errors");
enum
{
	kConstantsRegistry_NSErrorBadPortNumber		= 1,
	kConstantsRegistry_NSErrorBadUserID			= 2,
	kConstantsRegistry_NSErrorBadNumber			= 3, // generic
	kConstantsRegistry_NSErrorBadArray			= 4, // generic
	kConstantsRegistry_NSErrorResourceNotFound	= 5, // generic
};

//@}

#ifdef __OBJC__
//!\name Touch Bar Identifiers
//@{

extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDApplicationMain;
extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDGenericDialog;
extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDInfoWindowMain;
extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDPrefsWindowMain;
extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDTerminalWindowMain;
extern NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDVectorWindowMain;
extern NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDFind;
extern NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDFullScreen;
extern NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDGenericButton1;
extern NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDGenericButton2;
extern NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDGenericButton3;
extern NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDGenericButtonGroup;
extern NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDHelpButton;

//@}
#endif

//!\name Unique Identifiers for Registries of Public Events
//@{

/*!
Listener Model descriptors, identifying objects that manage,
abstractly, “listeners” for events, and then notify the
listeners when events actually take place.  See "ListenerModel.h".

The main benefit behind these identifiers is for cases where the
same listener responds to events from multiple models and needs
to know which model an event came from.
*/
enum
{
	kConstantsRegistry_ListenerModelDescriptorCommandExecution					= 'CmdX',
	kConstantsRegistry_ListenerModelDescriptorCommandModification				= 'CmdM',
	kConstantsRegistry_ListenerModelDescriptorCommandPostExecution				= 'CmdF',
	kConstantsRegistry_ListenerModelDescriptorEventTargetForControl				= 'cetg',
	kConstantsRegistry_ListenerModelDescriptorEventTargetForWindow				= 'wetg',
	kConstantsRegistry_ListenerModelDescriptorEventTargetGlobal					= 'getg',
	kConstantsRegistry_ListenerModelDescriptorMacroChanges						= 'Mcro',
	kConstantsRegistry_ListenerModelDescriptorNetwork							= 'Netw',
	kConstantsRegistry_ListenerModelDescriptorPreferences						= 'Pref',
	kConstantsRegistry_ListenerModelDescriptorPreferencePanels					= 'PPnl',
	kConstantsRegistry_ListenerModelDescriptorSessionChanges					= 'Sssn',
	kConstantsRegistry_ListenerModelDescriptorSessionFactoryChanges				= 'SsnF',
	kConstantsRegistry_ListenerModelDescriptorSessionFactoryAnySessionChanges   = 'Ssn*',
	kConstantsRegistry_ListenerModelDescriptorTerminalChanges					= 'Term',
	kConstantsRegistry_ListenerModelDescriptorTerminalSpeakerChanges			= 'Spkr',
	kConstantsRegistry_ListenerModelDescriptorTerminalViewChanges				= 'View',
	kConstantsRegistry_ListenerModelDescriptorTerminalWindowChanges				= 'TWin',
	kConstantsRegistry_ListenerModelDescriptorToolbarChanges					= 'TBar',
	kConstantsRegistry_ListenerModelDescriptorToolbarItemChanges				= 'Tool'
};

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
