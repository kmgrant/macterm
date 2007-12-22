/*###############################################################

	InternetServices.cp
	
	MacTelnet
		© 1998-2007 by Kevin Grant.
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

// Mac includes
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>

// resource includes
#include "StringResources.h"

// MacTelnet includes
#include "Console.h"
#include "InternetServices.h"
#include "MainEntryPoint.h"
#include "UIStrings.h"



#pragma mark Public Methods

/*!
To initialize Open Transport, use this method.  You should invoke
this routine immediately after you start up the common Mac OS
toolbox managers.

After calling this routine, you can create any number of
asynchronous references using InternetServices_New().

IMPORTANT:	If this routine fails, it may not return, displaying
			a message to the user that Open Transport cannot be
			started properly.

(3.0)
*/
void
InternetServices_Init ()
{
	OSStatus	error = noErr;
	
	
	error = noErr;
	
	// IMPORTANT: There is an unknown dependency here.  Even though
	// MacTelnet no longer uses direct network calls (instead calling
	// other Unix processes), that inter-process communication does
	// NOT work unless Open Transport is still initialized!  There
	// must be some equivalent initializer at the POSIX level that
	// can allow that code to work independent of this call.
	error = InitOpenTransportInContext(kInitOTForApplicationMask, nullptr);
	
	if (Alert_ReportOSStatus(error))
	{
		Console_WriteLine("failed to initialize Open Transport!");
		MainEntryPoint_ImmediatelyQuit(); // does not return
	}
}// Init


/*!
To close the synchronous Internet Services provider
and shut down Open Transport, use this method.  You
generally invoke this routine at the end of the
program when you are shutting down other toolbox
managers.

(3.0)
*/
void
InternetServices_Done ()
{
	CloseOpenTransportInContext(nullptr/* context automatically derived for an application */);
}// Done


/*!
To asynchronously create a new Open Transport Internet Services
reference, use this method.  The new reference is passed to the
specified notification routine.  Your provided context, which is
defined entirely by you, is passed to the given notification
routine directly.  Thus, you need to call this routine once for
each different context you want to have.

If you want to use the default configuration, pass the string
"kInternetServices_DefaultConfigurationString"; otherwise, pass
any string you would normally pass to OTCreateConfiguration().

When you are finished with this context, destroy it by invoking
the routine InternetServices_Dispose().

If there is not enough memory to create a reference, "memFullErr"
is returned.  If the specified configuration string is not valid,
"kEBADFErr" is returned.  Otherwise, any error code that can be
returned from a call to OTAsyncOpenInternetServices() may be
returned.  If no problems occur, "noErr" is returned and the new
reference will be given to your notification routine.

(3.0)
*/
OSStatus
InternetServices_New	(OTNotifyUPP	inNotificationRoutine,
						 char const*	inConfigurationString,
						 void*			inUserDefinedContextPtr)
{
	return InternetServices_NewWithOTFlags(0/* reserved flags */, inNotificationRoutine, inConfigurationString,
											inUserDefinedContextPtr);
}// New


/*!
To asynchronously create a new Open Transport Internet Services
reference using the default TCP/IP path, effectively creating a
TCP/IP service provider, use this method.

(3.0)
*/
OSStatus
InternetServices_NewTCPIP		(OTNotifyUPP	inNotificationRoutine,
								 void*			inUserDefinedContextPtr)
{
	OSStatus	result = noErr;
	
	
	result = OTAsyncOpenInternetServicesInContext(kDefaultInternetServicesPath, 0/* reserved flags */,
													inNotificationRoutine,
													inUserDefinedContextPtr,
													nullptr/* means “use InitOpenTransport() context” */);
	return result;
}// New


/*!
To asynchronously create a new Open Transport Internet Services
reference in the same way as InternetServices_New(), except by
specifying Open Transport flags, use this method.  At the time
of this writing, all Open Transport flags were reserved and set
to zero, so it is generally more convenient to use the routine
InternetServices_New(), which calls this routine with flags
equal to 0.

(3.0)
*/
OSStatus
InternetServices_NewWithOTFlags		(OTOpenFlags	inFlags,
									 OTNotifyUPP	inNotificationRoutine,
									 char const*	inConfigurationString,
									 void*			inUserDefinedContextPtr)
{
	OSStatus			result = noErr;
	OTConfigurationRef	configuration = OTCreateConfiguration(inConfigurationString);
	
	
	result = noErr;
	if (configuration == kOTNoMemoryConfigurationPtr) result = memFullErr;
	else if (configuration == kOTInvalidConfigurationPtr) result = kEBADFErr;
	else
	{
		result = OTAsyncOpenInternetServicesInContext(configuration, inFlags, inNotificationRoutine,
														inUserDefinedContextPtr,
														nullptr/* means “use InitOpenTransport() context” */);
	}
	
	return result;
}// New


/*!
To dispose of an asynchronous reference to an
Internet Services provider that you created
using InternetServices_New(), use this method.

On output, your copy of the reference is
automatically set to nullptr.

(3.0)
*/
void
InternetServices_Dispose	(InetSvcRef*	inoutRefPtr)
{
	if (nullptr != inoutRefPtr)
	{
		OTCloseProvider(*inoutRefPtr);
		*inoutRefPtr = kOTInvalidRef;
	}
}// Dispose

// BELOW IS REQUIRED NEWLINE TO END FILE
