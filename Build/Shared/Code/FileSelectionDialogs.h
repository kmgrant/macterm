/*!	\file FileSelectionDialogs.h
	\brief Convenient front ends to Navigation Services.
*/
/*###############################################################

	Interface Library
	© 1998-2014 by Kevin Grant
	
	This library is free software; you can redistribute it or
	modify it under the terms of the GNU Lesser Public License
	as published by the Free Software Foundation; either version
	2.1 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU Lesser Public License for details.
	
	You should have received a copy of the GNU Lesser Public
	License along with this library; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include <UniversalDefines.h>

#ifndef __FILESELECTIONDIALOGS__
#define __FILESELECTIONDIALOGS__



// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>



#pragma mark Public Methods

//!\name Utility Routines For Use With Navigation Services
//@{

OSStatus
	FileSelectionDialogs_CompleteSave				(NavReplyRecord*		inoutReplyPtr);

OSStatus
	FileSelectionDialogs_CreateOrFindUserSaveFile	(NavReplyRecord const&	inReply,
													 OSType					inNewFileCreator,
													 OSType					inNewFileType,
													 FSRef&					outUserSaveFile,
													 FSRef&					outTemporaryFile);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
