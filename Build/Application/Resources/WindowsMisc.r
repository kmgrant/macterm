/*
**************************************************************************
	WindowsMisc.r
	
	MacTelnet
		© 1998-2004 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This file contains all resource data for special windows.
**************************************************************************
*/

#ifndef __WINDOWSMISC_R__
#define __WINDOWSMISC_R__

#include "Carbon.r"

#include "PreferenceResources.h"

/*
terminal windows - palette of colors
*/

resource 'pltt' (ANSICOLORS_DEFAULTID, "Standard ANSI Colors (indices are important)")
{
	{
		// black
		0, 0, 0,					// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// red
		52685, 1028, 1028,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// green
		771, 52428, 771,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// yellow
		52428, 52428, 771,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// blue
		0, 0, 51657,				// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// magenta
		52428, 771, 52428,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// cyan
		771, 52428, 52428,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// silver
		48821, 48821, 48821,		// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// black emphasized (gray)
		16384, 16384, 16384,		// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// red emphasized
		65278, 771, 771,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// green emphasized
		1028, 65535, 1028,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// yellow emphasized
		65535, 65535, 1028,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// blue emphasized
		771, 771, 65278,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// magenta emphasized
		65535, 771, 65535,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// cyan emphasized
		771, 65278, 65278,			// RGB
		pmCourteous,				// usage
		0,							// tolerance
		// silver emphasized (nearly white)
		65278, 65278, 65278,		// RGB
		pmCourteous,				// usage
		0							// tolerance
	}
};

#endif