/*!	\file PrefPanelConfigurations.h
	\brief Implements the Favorites panel of Preferences.
	
	MacTelnet 3.0 builds on the NCSA Telnet 2.6 dialog by making
	it a modeless panel and adding tabs for easy switching
	between configuration types.
*/
/*###############################################################

	MacTelnet
		© 1998-2006 by Kevin Grant.
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

#ifndef __PREFPANELCONFIGURATIONS__
#define __PREFPANELCONFIGURATIONS__

// MacTelnet includes
#include "Panel.h"



#pragma mark Constants

typedef FourCharCode PrefPanelConfigurationType;
enum
{
	kInvalidPrefPanelConfigurationType		= FOUR_CHAR_CODE('huh?'),
	kPrefPanelConfigurationTypeSession		= FOUR_CHAR_CODE('sess'),
	kPrefPanelConfigurationTypeTerminal		= FOUR_CHAR_CODE('term'),
	kPrefPanelConfigurationTypeProxyServer	= FOUR_CHAR_CODE('prox')
};

#pragma mark Callbacks

/*!
Configuration Editing Method

This routine is called when the user decides to
create or change a configuration.  Generally, this
routine would display a dialog box to allow the
user to edit the selected configuration.

On input, the dialog uses the specified class and
context name in order to ask the Preferences module
for data pertaining to that context.  If the name
is nullptr, a new context should be created with
some default name.

An origin rectangle might be provided to this
routine, which you would use to guide the
transition opening effect of your dialog box
(and the transition to close it *if* the user
clicks OK).
*/
typedef Boolean (*PrefPanelConfigurations_EditProcPtr)	(Preferences_Class	inClass,
														 CFStringRef		inContextName,
														 Rect const*		inTransitionOpenRectOrNull);



#pragma mark Public Methods

Panel_Ref
	PrefPanelConfigurations_New					();

inline Boolean
	PrefPanelConfigurations_InvokeEditProc		(PrefPanelConfigurations_EditProcPtr	inUserRoutine,
												 Preferences_Class						inClass,
												 CFStringRef							inContextName,
												 Rect const*							inTransitionOpenRectOrNull)
	{
		return (*inUserRoutine)(inClass, inContextName, inTransitionOpenRectOrNull);
	}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
