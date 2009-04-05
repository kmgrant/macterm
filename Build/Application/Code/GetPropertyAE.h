/*!	\file GetPropertyAE.h
	\brief This file lists OSL accessor routines that allow
	scripting terms to use generic property references.  Scripts
	can access data more naturally this way (for example, a
	property of an application, like its name, can be acquired
	implicitly or via an explicit application class reference).
	
	The general naming convention used here is of the form
	GetPropertyAE_FromXXX(), where XXX represents the type of
	object you have.  In order for AEResolve() to work
	reliably, applications must provide enough property
	accessors to make resolution paths flexible.  If insufficient
	property accessors are provided, then scripters will have
	much less flexibility in their use of class properties.
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

#ifndef __GETPROPERTYAE__
#define __GETPROPERTYAE__

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

OSErr
	GetPropertyAE_DataFromClassProperty		(AEDesc const*		inTokenDescriptor,
											 DescType const		inDesiredDataType,
											 AEDesc*			outDataDescriptor,
											 Boolean*			outUsedDesiredDataType);

OSErr
	GetPropertyAE_DataSizeFromClassProperty	(AEDesc const*		inTokenDescriptor,
											 AEDesc*			outDataDescriptor,
											 Boolean*			outUsedDesiredDataType);

OSErr
	GetPropertyAE_DataToClassProperty		(AEDesc const*		inTokenDescriptor,
											 AEDesc*			inDataDescriptor);

pascal OSErr
	GetPropertyAE_FromNull					(DescType			inDesiredClass,
											 AEDesc const*		inFromWhat,
											 DescType			inFromWhatClass,
											 DescType			inFormOfReference,
											 AEDesc const*		inReference,
											 AEDesc*			outDesiredProperty,
											 long				inData);

pascal OSErr
	GetPropertyAE_FromObject				(DescType			inDesiredClass,
											 AEDesc const*		inFromWhat,
											 DescType			inFromWhatClass,
											 DescType			inFormOfReference,
											 AEDesc const*		inReference,
											 AEDesc*			outDesiredProperty,
											 long				inData);

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
