/*!	\file PrintTerminal.h
	\brief Methods for printing terminal text in MacTerm.
*/
/*###############################################################

	MacTerm
		© 1998-2021 by Kevin Grant.
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

// Mac includes
#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
class NSWindow;
#endif
#include <CoreServices/CoreServices.h>

// application includes
#include "TerminalScreenRef.typedef.h"
#include "TerminalViewRef.typedef.h"



#pragma mark Constants

/*!
Possible return values from certain APIs in this module.
*/
enum PrintTerminal_Result
{
	kPrintTerminal_ResultOK = 0,				//!< no error
	kPrintTerminal_ResultInvalidID = -1,		//!< a given "PrintTerminal_JobRef" does not correspond to any known object
	kPrintTerminal_ResultParameterError = -2,	//!< invalid input (e.g. a null pointer)
};

#pragma mark Types

#ifdef __OBJC__

/*!
Manages jobs sent to a printer by the user (such
as when selecting Print Screen).
*/
@interface PrintTerminal_Job : NSObject //{

// initializers
	- (instancetype _Nullable)
	initWithString:(NSString* _Nonnull)_
	font:(NSFont* _Nullable)_
	title:(NSString* _Nullable)_
	landscape:(BOOL)_ NS_DESIGNATED_INITIALIZER;

@end //}

#else

class PrintTerminal_Job;

#endif // __OBJC__

// This is defined as an Objective-C object so it is compatible
// with ARC rules (e.g. strong references).
typedef PrintTerminal_Job*		PrintTerminal_JobRef;



#pragma mark Public Methods

//!\name Creating Objects
//@{

PrintTerminal_JobRef _Nullable
	PrintTerminal_NewJobFromFile			(CFURLRef _Nonnull				inFile,
											 TerminalViewRef _Nonnull		inView,
											 CFStringRef _Nonnull			inJobName,
											 Boolean						inDefaultToLandscape = false);

PrintTerminal_JobRef _Nullable
	PrintTerminal_NewJobFromSelectedText	(TerminalViewRef _Nonnull		inView,
											 CFStringRef _Nonnull			inJobName,
											 Boolean						inDefaultToLandscape = false);

PrintTerminal_JobRef _Nullable
	PrintTerminal_NewJobFromVisibleScreen	(TerminalViewRef _Nonnull		inView,
											 TerminalScreenRef _Nonnull		inViewBuffer,
											 CFStringRef _Nonnull			inJobName);

//@}

//!\name Printing
//@{

PrintTerminal_Result
	PrintTerminal_JobSendToPrinter			(PrintTerminal_JobRef _Nonnull	inJob,
											 NSWindow* _Nullable			inParentWindowOrNil);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
