/***********************************************************************
ListeningTCPSocket - Class for TCP half-sockets that can accept incoming
connections.
Copyright (c) 2011-2019 Oliver Kreylos

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

#ifndef COMM_LISTENINGTCPSOCKET_INCLUDED
#define COMM_LISTENINGTCPSOCKET_INCLUDED

#include <string>

/* Forward declarations: */
namespace Misc {
class Time;
}

namespace Comm {

class ListeningTCPSocket
	{
	/* Embedded classes: */
	public:
	enum AddressFamily // Enumerated type for IP address families
		{
		Any,IPv4,IPv6
		};
	
	/* Elements: */
	private:
	int fd; // File descriptor of the listening half-socket
	
	/* Constructors and destructors: */
	public:
	ListeningTCPSocket(int portId,int backlog,AddressFamily addressFamily =Any); // Creates a listening socket on any address and the given port ID, or on a randomly-assigned port ID if portId is negative
	private:
	ListeningTCPSocket(const ListeningTCPSocket& source); // Prohibit copy constructor
	ListeningTCPSocket& operator=(const ListeningTCPSocket& source); // Prohibit assignment operator
	public:
	~ListeningTCPSocket(void); // Closes the half-socket
	
	/* Methods: */
	int getFd(void) const // Returns this half-socket's file descriptor
		{
		return fd;
		}
	bool isBlocking(void) const; // Returns true if the listening socket is in non-blocking mode
	void setBlocking(bool newBlocking); // Sets the listening socket to blocking or non-blocking mode
	int getPortId(void) const; // Returns port ID assigned to this half-socket
	std::string getAddress(void) const; // Returns interface address assigned to this half-socket in dotted notation
	std::string getInterfaceName(bool throwException =true) const; // Returns interface host name of this half-socket; throws exception if host name cannot be resolved and flag is true
	bool waitForConnection(const Misc::Time& timeout) const; // Waits for an incoming connection until timeout; returns true if a connection is waiting to be accepted
	};

}

#endif
