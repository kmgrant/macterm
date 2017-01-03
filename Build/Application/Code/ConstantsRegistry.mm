/*!	\file ConstantsRegistry.mm
	\brief A window showing status for all sessions.
*/
/*###############################################################

	MacTerm
		© 1998-2017 by Kevin Grant.
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

#import "ConstantsRegistry.h"
#import <UniversalDefines.h>

// Mac includes
#import <CoreServices/CoreServices.h>

// library includes
#import <CocoaFuture.objc++.h>



#pragma mark Constants

// NOTE: Do not ever change these, that would only break user preferences.
// WARNING: Various Touch Bar IDs are duplicated in the XIB that loads the
// corresponding Touch Bar.  Currently, they are manually synchronized.
NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDApplicationMain	= @"net.macterm.MacTerm.touchbar.application";
NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDInfoWindowMain		= @"net.macterm.MacTerm.touchbar.infowindow";
NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDPrefsWindowMain	= @"net.macterm.MacTerm.touchbar.prefswindow";
NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDTerminalWindowMain	= @"net.macterm.MacTerm.touchbar.terminals";
NSTouchBarCustomizationIdentifier const		kConstantsRegistry_TouchBarIDVectorWindowMain	= @"net.macterm.MacTerm.touchbar.vectorwindow";
NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDFind			= @"net.macterm.MacTerm.touchbaritem.find";
NSTouchBarItemIdentifier const				kConstantsRegistry_TouchBarItemIDFullScreen		= @"net.macterm.MacTerm.touchbaritem.fullscreen";

// BELOW IS REQUIRED NEWLINE TO END FILE
