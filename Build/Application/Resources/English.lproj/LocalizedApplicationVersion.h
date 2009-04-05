/*
**************************************************************************
	LocalizedApplicationVersion.h
	
	MacTelnet
		© 1998-2004 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	DEPRECATED.  To disappear soon.
	
	Please try to view this file in the font "MPW 9" (or Monaco 9).
**************************************************************************
*/

#ifndef __LOCALIZEDAPPLICATIONVERSION__
#define __LOCALIZEDAPPLICATIONVERSION__



// the Region Code for the application, jointly defining the language and region;
// possible values include "verUS", "verGermany", "verFrance", "verFrBelgium", etc.
// see the Universal Interfaces 3.3 (Mac OS) "Script.h" file for the complete list
#define LOCALIZED_MacOSScriptManagerRegionCode_Application verUS

// the Script Code for the AppleScript Dictionary (scripting terminology resource);
// may be used for other things, but in general MacTelnet uses the current system
// script for most operations requiring a script code
#define LOCALIZED_MacOSScriptManagerScriptCode_Application smRoman

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
