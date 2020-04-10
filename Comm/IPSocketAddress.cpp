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

#include <Comm/IPSocketAddress.h>

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <Misc/PrintInteger.h>
#include <Misc/ThrowStdErr.h>

namespace Comm {

/********************************
Methods of class IPSocketAddress:
********************************/

std::vector<IPSocketAddress> IPSocketAddress::lookupHost(const char* hostName,unsigned short port)
	{
	/* Convert port number to string for getaddrinfo: */
	char portBuffer[6];
	char* portString=Misc::print(port,portBuffer+5);
	
	/* Lookup host's IP addresses: */
	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_family=AF_UNSPEC; // Return both IPv4 and IPv6 addresses
	hints.ai_socktype=0; // Return both datagram and stream socket addresses (doesn't make a difference for numeric ports)
	hints.ai_protocol=0; // Return socket addresses using any protocol
	hints.ai_flags=AI_NUMERICSERV|AI_ADDRCONFIG;
	struct addrinfo* addresses;
	int aiResult=getaddrinfo(hostName,portString,&hints,&addresses);
	if(aiResult!=0)
		Misc::throwStdErr("Comm::IPSocketAddress: Unable to resolve address %s:%u due to error %d (%s)",hostName,(unsigned int)port,aiResult,gai_strerror(aiResult));
	
	/* Create a list out of returned addresses: */
	std::vector<IPSocketAddress> result;
	for(struct addrinfo* aiPtr=addresses;aiPtr!=0;aiPtr=aiPtr->ai_next)
		{
		if(aiPtr->ai_family==AF_INET&&aiPtr->ai_addrlen==sizeof(sockaddr_in))
			{
			/* Append an IPv4 socket address: */
			IPSocketAddress addr;
			memcpy(&addr.sa4,aiPtr->ai_addr,sizeof(sockaddr_in));
			result.push_back(addr);
			}
		else if(aiPtr->ai_family==AF_INET6&&aiPtr->ai_addrlen==sizeof(sockaddr_in6))
			{
			/* Append an IPv6 socket address: */
			IPSocketAddress addr;
			memcpy(&addr.sa6,aiPtr->ai_addr,sizeof(sockaddr_in6));
			result.push_back(addr);
			}
		}
	
	/* Release the returned addresses: */
	freeaddrinfo(addresses);
	
	/* Return the list of addresses: */
	return result;
	}

unsigned short IPSocketAddress::getPort(void) const
	{
	/* Extract the port number based on address type: */
	switch(family)
		{
		case AF_INET:
			return htons(sa4.sin_port);
		
		case AF_INET6:
			return htons(sa6.sin6_port);
		
		default:
			throw std::runtime_error("Comm::IPSocketAddress::getPort: Invalid address family");
		}
	}

std::string IPSocketAddress::getAddress(void) const
	{
	/* Query the address: */
	char addressBuffer[NI_MAXHOST];
	char serviceBuffer[NI_MAXSERV];
	int niResult=0;
	switch(family)
		{
		case AF_INET:
			niResult=getnameinfo(reinterpret_cast<const sockaddr*>(&sa4),sizeof(sockaddr_in),addressBuffer,sizeof(addressBuffer),serviceBuffer,sizeof(serviceBuffer),NI_NUMERICHOST|NI_NUMERICSERV);
			break;
		
		case AF_INET6:
			niResult=getnameinfo(reinterpret_cast<const sockaddr*>(&sa6),sizeof(sockaddr_in6),addressBuffer,sizeof(addressBuffer),serviceBuffer,sizeof(serviceBuffer),NI_NUMERICHOST|NI_NUMERICSERV);
			break;
		
		default:
			throw std::runtime_error("Comm::IPSocketAddress::getAddress: Invalid address family");
		}
	if(niResult!=0)
		Misc::throwStdErr("Comm::IPSocketAddress::getAddress: Unable to retrieve address due to error %d (%s)",niResult,gai_strerror(niResult));
	
	/* Return the address buffer as a string: */
	return addressBuffer;
	}

std::string IPSocketAddress::getHostName(void) const
	{
	/* Query the address: */
	char hostNameBuffer[NI_MAXHOST];
	char serviceBuffer[NI_MAXSERV];
	int niResult;
	switch(family)
		{
		case AF_INET:
			niResult=getnameinfo(reinterpret_cast<const sockaddr*>(&sa4),sizeof(sockaddr_in),hostNameBuffer,sizeof(hostNameBuffer),serviceBuffer,sizeof(serviceBuffer),NI_NUMERICSERV);
			break;
		
		case AF_INET6:
			niResult=getnameinfo(reinterpret_cast<const sockaddr*>(&sa6),sizeof(sockaddr_in6),hostNameBuffer,sizeof(hostNameBuffer),serviceBuffer,sizeof(serviceBuffer),NI_NUMERICSERV);
			break;
		
		default:
			throw std::runtime_error("Comm::IPSocketAddress::getHostName: Invalid address family");
		}
	if(niResult!=0)
		Misc::throwStdErr("Comm::IPSocketAddress::getHostName: Unable to retrieve host name due to error %d (%s)",niResult,gai_strerror(niResult));
	
	/* Return the host name buffer as a string: */
	return hostNameBuffer;
	}

}
