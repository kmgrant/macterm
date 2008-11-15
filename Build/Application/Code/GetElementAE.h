/*!	\file GetElementAE.h
	\brief This file lists OSL accessor routines that allow
	scripts to acquire elements of objects using specific
	criteria (such as a name, number, or a test that must be
	satisfied).
	
	The general naming convention used here is of the form
	GetElementAE_XXXFromYYY(), where XXX represents the type of
	element object class (token) that is needed, and YYY is the
	kind of container object you have.  In order for AEResolve()
	to work reliably, applications must provide enough object
	accessors to make resolution paths flexible.  If insufficient
	object accessors are provided, then scripters will have much
	less flexibility in their use of element forms.
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

#ifndef __GETELEMENTAE__
#define __GETELEMENTAE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// MacTelnet includes
#include "ObjectClassesAE.h"



#pragma mark Public Methods

pascal OSErr
	GetElementAE_ApplicationFromNull		(DescType			inDesiredClass,
											 AEDesc const*		inFromWhat,
											 DescType			inFromWhatClass,
											 DescType			inFormOfReference,
											 AEDesc const*		inReference,
											 AEDesc*			outObjectOfDesiredClass,
											 long				inData);

OSStatus
	GetElementAE_ApplicationImplicit		(AEDesc*			outObjectOfDesiredClass);

pascal OSErr
	GetElementAE_SessionFromNull			(DescType			inDesiredClass,
											 AEDesc const*		inFromWhat,
											 DescType			inFromWhatClass,
											 DescType			inFormOfReference,
											 AEDesc const*		inReference,
											 AEDesc*			outObjectOfDesiredClass,
											 long				inData);

pascal OSErr
	GetElementAE_WindowFromNull				(DescType			inDesiredClass,
											 AEDesc const*		inFromWhat,
											 DescType			inFromWhatClass,
											 DescType			inFormOfReference,
											 AEDesc const*		inReference,
											 AEDesc*			outObjectOfDesiredClass,
											 long				inData);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
