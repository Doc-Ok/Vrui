/***********************************************************************
IPSocketAddress - Class to represent IP address/port number pairs, using
IP protocol versions 4 or 6.
Copyright (c) 2018-2019 Oliver Kreylos

This file is part of the Portable Communications Library (Comm).

The Portable Communications Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Portable Communications Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Communications Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef IPSOCKETADDRESS_INCLUDED
#define IPSOCKETADDRESS_INCLUDED

#include <string>
#include <vector>
#include <netinet/in.h>

namespace Comm {

class IPSocketAddress
	{
	/* Elements: */
	private:
	union // Nameless union holding either an IPv4 or IPv6 address, with common address family element
		{
		sa_family_t family; // Address family shared with both supported types of addresses
		sockaddr_in sa4; // IPv4 socket address
		sockaddr_in6 sa6; // IPv6 socket address
		};
	
	/* Constructors and destructors: */
	public:
	IPSocketAddress(void) // Dummy constructor for the time being
		{
		}
	IPSocketAddress(const sockaddr_in& addr) // Creates an IP socket address from an IP version 4 socket address
		{
		sa4=addr;
		}
	IPSocketAddress(const sockaddr_in6& addr) // Creates an IP socket address from an IP version 6 socket address
		{
		sa6=addr;
		}
	IPSocketAddress(const char* hostName,unsigned short port,bool bind); // Creates an IP socket address for the given port on the given host, which is either a DNS hostname or a numerical IPv4/IPv6 address
	
	/* Methods: */
	static std::vector<IPSocketAddress> lookupHost(const char* hostName,unsigned short port); // Returns a list of potential IP addresses for the given port on the given host, which is either a DNS hostname or a numerical IPv4/IPv6 address
	bool isIPv4(void) const // Returns true if the socket address is an IPv4 address
		{
		return family==AF_INET;
		}
	bool isIPv6(void) const // Returns true if the socket address is an IPv6 address
		{
		return family==AF_INET6;
		}
	const sockaddr_in& getIPv4Address(void) const // Returns an IPv4 address, assuming the socket address is one
		{
		return sa4;
		}
	const sockaddr_in6& getIPv6Address(void) const // Returns an IPv6 address, assuming the socket address is one
		{
		return sa6;
		}
	unsigned short getPort(void) const; // Returns the address's port number
	std::string getAddress(void) const; // Returns the address as a numerical IPv4/IPv6 address
	std::string getHostName(void) const; // Returns the address as a DNS host name
	};

}

#endif
