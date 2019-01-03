/*!	\file Network.h
	\brief APIs dealing with the local machine’s Internet
	protocol addresses.
*/
/*###############################################################

	MacTerm
		© 1998-2019 by Kevin Grant.
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

#ifndef __NETWORK__
#define __NETWORK__

// library includes
#include <ListenerModel.h>
#include <ResultCode.template.h>



#pragma mark Constants

/*!
Possible return values from Network module routines.
*/
typedef ResultCode< UInt16 >	Network_Result;
Network_Result const	kNetwork_ResultOK(0);					//!< no error
Network_Result const	kNetwork_ResultParameterError(1);		//!< invalid input (e.g. a null pointer)
Network_Result const	kNetwork_ResultMonitorFailed(2);			//!< unable to create requested monitor

/*!
Used with Network_StartMonitoring() and Network_StopMonitoring()
to be notified of important changes.
*/
enum Network_Change
{
	kNetwork_ChangeAddressListWillRebuild	= 'WAdr',	//!< IPv4 or IPv6 state has significantly changed and the
														//!  list of addresses from Network_ProcessIPAddresses() is
														//!  about to be rebuilt (could use this opportunity to
														//!  show graphical progress bars for example)
	kNetwork_ChangeAddressListDidRebuild		= 'DAdr',	//!< current list of addresses has been updated; could
														//!  reverse any "kNetwork_ChangeAddressListWillRebuild"
														//!  response (such as hiding progress bars that were shown)
														//!  and call Network_ProcessIPAddresses() to see new data
};

/*!
Options for starting monitors.
*/
enum
{
	kNetwork_MonitorFlagNotifyOfInitialValue		= (1 << 0),		//!< when creating a monitor, immediately trigger it
																//!  instead of waiting for the next update to data
	kNetwork_DefaultMonitorFlags = (kNetwork_MonitorFlagNotifyOfInitialValue)
};
typedef UInt16		Network_MonitorFlags;



#pragma mark Public Methods

//!\name Retrieving Network Information
//@{

void
	Network_CopyLocalHostAddresses	(CFArrayRef&,
									 Boolean* = nullptr);

void
	Network_ResetLocalHostAddresses	();

//@}

//!\name Responding to Changes
//@{

Network_Result
	Network_StartMonitoring			(Network_Change,
									 ListenerModel_ListenerRef,
									 Network_MonitorFlags = kNetwork_DefaultMonitorFlags);

Network_Result
	Network_StopMonitoring			(Network_Change,
									 ListenerModel_ListenerRef);

//@}

#endif

// BELOW IS REQUIRED NEWLINE TO END FILE
