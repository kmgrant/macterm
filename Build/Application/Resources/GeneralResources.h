/*!	\file GeneralResources.h
	\brief Constants describing general resources, such as
	resource types and special resource IDs.
*/
/*###############################################################

	MacTelnet
		© 1998-2005 by Kevin Grant.
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

#ifndef __GENERALRESOURCES__
#define __GENERALRESOURCES__



// type and creator signatures
#define kApplicationFileTypeMacroSet				'TEXT'
#define kApplicationFileTypeSessionDescription		'CONF'

// types and creator signatures, as strings
#define	kStrApplicationCreatorSignature				"KevG"
#define kStrApplicationFileTypeMacroSet				"TEXT"
#define kStrApplicationFileTypeSessionDescription	"CONF"

// icon family for the MacTelnet application icon
#define kNCSAIconFamilyId						128

// 'LDEF' for icon list
#define kIconListDefinitionProcResID			128

// 'open' resource ID
#define	kNavGetFileOpenResource					128

// base ID for all ResEdit template resources
#define kTemplateBaseID							128

#define	USER_TRSL			'taBL'	/* User translation tables resource type */
									/* 256 bytes of in table followed by 256 bytes of out table */

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
