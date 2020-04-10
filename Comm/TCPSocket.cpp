/***********************************************************************
TCPSocket - Wrapper class for TCP sockets ensuring exception safety.
Copyright (c) 2002-2018 Oliver Kreylos

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

#include <Comm/TCPSocket.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/Time.h>
#include <Comm/IPv4SocketAddress.h>

namespace Comm {

/**************************
Methods of class TCPSocket:
**************************/

TCPSocket::TCPSocket(int portId,int backlog)
	:socketFd(-1)
	{
	/* Create the socket file descriptor: */
	socketFd=socket(PF_INET,SOCK_STREAM,0);
	if(socketFd<0)
		{
		int myerrno=errno;
		Misc::throwStdErr("Comm::TCPSocket: Unable to create socket due to error %d (%s)",myerrno,strerror(myerrno));
		}
	
	/* Bind the socket to an address that accepts any incoming address on the given port: */
	IPv4SocketAddress socketAddress(portId>=0?(unsigned int)(portId):0);
	if(bind(socketFd,(struct sockaddr*)&socketAddress,sizeof(IPv4SocketAddress))<0)
		{
		int myerrno=errno;
		close(socketFd);
		Misc::throwStdErr("Comm::TCPSocket: Unable to bind socket to port %d due to error %d (%s)",portId,myerrno,strerror(myerrno));
		}
	
	/* Start listening on the socket: */
	if(listen(socketFd,backlog)<0)
		{
		int myerrno=errno;
		close(socketFd);
		Misc::throwStdErr("Comm::TCPSocket: Unable to start listening on socket due to error %d (%s)",portId,myerrno,strerror(myerrno));
		}
	}

TCPSocket::TCPSocket(const std::string& hostname,int portId)
	:socketFd(-1)
	{
	/* Call the connect() method: */
	connect(hostname,portId);
	}

TCPSocket::TCPSocket(const IPv4SocketAddress& hostAddress)
	:socketFd(-1)
	{
	/* Call the connect() method: */
	connect(hostAddress);
	}

TCPSocket::TCPSocket(const TCPSocket& source)
	:socketFd(dup(source.socketFd))
	{
	}

TCPSocket& TCPSocket::operator=(const TCPSocket& source)
	{
	if(this!=&source)
		{
		if(socketFd>=0)
			close(socketFd);
		socketFd=dup(source.socketFd);
		}
	return *this;
	}

TCPSocket::~TCPSocket(void)
	{
	if(socketFd>=0)
		close(socketFd);
	}

IPv4SocketAddress TCPSocket::getAddress(void) const
	{
	IPv4SocketAddress socketAddress;
	#ifdef __SGI_IRIX__
	int socketAddressLen=sizeof(IPv4SocketAddress);
	#else
	socklen_t socketAddressLen=sizeof(IPv4SocketAddress);
	#endif
	if(getsockname(socketFd,(struct sockaddr*)&socketAddress,&socketAddressLen)<0)
		{
		int myerrno=errno;
		Misc::throwStdErr("Comm::TCPSocket::getAddress: Unable to query local socket address due to error %d (%s)",myerrno,strerror(myerrno));
		}
	if(socketAddressLen<sizeof(IPv4SocketAddress))
		Misc::throwStdErr("Comm::TCPSocket::getAddress: Returned address has wrong size; %u bytes instead of %u bytes",(unsigned int)socketAddressLen,(unsigned int)sizeof(IPv4SocketAddress));
	
	return socketAddress;
	}

TCPSocket& TCPSocket::connect(const std::string& hostname,int portId)
	{
	/* Close a previous connection: */
	if(socketFd>=0)
		{
		close(socketFd);
		socketFd=-1;
		}
	
	/* Create the socket file descriptor: */
	socketFd=socket(PF_INET,SOCK_STREAM,0);
	if(socketFd<0)
		{
		int myerrno=errno;
		Misc::throwStdErr("Comm::TCPSocket::connect Unable to create socket due to error %d (%s)",myerrno,strerror(myerrno));
		}
	
	/* Bind the socket to an address that accepts any incoming address on a system-assigned port: */
	IPv4SocketAddress socketAddress(0);
	if(bind(socketFd,(struct sockaddr*)&socketAddress,sizeof(IPv4SocketAddress))<0)
		{
		int myerrno=errno;
		close(socketFd);
		socketFd=-1;
		Misc::throwStdErr("Comm::TCPSocket::connect: Unable to bind socket due to error %d (%s)",myerrno,strerror(myerrno));
		}
	
	/* Connect to the given host on the given port: */
	IPv4SocketAddress remoteAddress(portId,IPv4Address(hostname.c_str()));
	if(::connect(socketFd,(const struct sockaddr*)&remoteAddress,sizeof(IPv4SocketAddress))<0)
		{
		int myerrno=errno;
		close(socketFd);
		socketFd=-1;
		Misc::throwStdErr("Comm::TCPSocket::connect Unable to connect to host %s on port %d due to error %d (%s)",hostname.c_str(),portId,myerrno,strerror(myerrno));
		}
	
	return *this;
	}

TCPSocket& TCPSocket::connect(const IPv4SocketAddress& hostAddress)
	{
	/* Close a previous connection: */
	if(socketFd>=0)
		{
		close(socketFd);
		socketFd=-1;
		}
	
	/* Create the socket file descriptor: */
	socketFd=socket(PF_INET,SOCK_STREAM,0);
	if(socketFd<0)
		{
		int myerrno=errno;
		Misc::throwStdErr("Comm::TCPSocket::connect: Unable to create socket due to error %d (%s)",myerrno,strerror(myerrno));
		}
	
	/* Bind the socket to an address that accepts any incoming address on a system-assigned port: */
	IPv4SocketAddress socketAddress(0);
	if(bind(socketFd,(struct sockaddr*)&socketAddress,sizeof(IPv4SocketAddress))<0)
		{
		int myerrno=errno;
		close(socketFd);
		socketFd=-1;
		Misc::throwStdErr("Comm::TCPSocket::connect: Unable to bind socket due to error %d (%s)",myerrno,strerror(myerrno));
		}
	
	/* Connect to the given host on the given port: */
	if(::connect(socketFd,(const struct sockaddr*)&hostAddress,sizeof(IPv4SocketAddress))<0)
		{
		int myerrno=errno;
		close(socketFd);
		socketFd=-1;
		Misc::throwStdErr("Comm::TCPSocket::connect: Unable to connect to host %s on port %u due to error %d (%s)",hostAddress.getAddress().getHostname().c_str(),hostAddress.getPort(),myerrno,strerror(myerrno));
		}
	
	return *this;
	}

TCPSocket TCPSocket::accept(void) const
	{
	/* Wait for connection attempts: */
	int newSocketFd=::accept(socketFd,0,0);
	if(newSocketFd<0)
		{
		int myerrno=errno;
		Misc::throwStdErr("Comm::TCPSocket::accept: Unable to accept connection due to error %d (%s)",myerrno,strerror(myerrno));
		}
	
	return TCPSocket(newSocketFd);
	}

IPv4SocketAddress TCPSocket::getPeerAddress(void) const
	{
	IPv4SocketAddress peerAddress;
	#ifdef __SGI_IRIX__
	int peerAddressLen=sizeof(IPv4SocketAddress);
	#else
	socklen_t peerAddressLen=sizeof(IPv4SocketAddress);
	#endif
	if(getpeername(socketFd,(struct sockaddr*)&peerAddress,&peerAddressLen)<0)
		{
		int myerrno=errno;
		Misc::throwStdErr("Comm::TCPSocket::getPeerAddress: Unable to query remote socket address due to error %d (%s)",myerrno,strerror(myerrno));
		}
	if(peerAddressLen<sizeof(IPv4SocketAddress))
		Misc::throwStdErr("Comm::TCPSocket::getPeerAddress: Returned address has wrong size; %u bytes instead of %u bytes",(unsigned int)peerAddressLen,(unsigned int)sizeof(IPv4SocketAddress));
	
	return peerAddress;
	}

void TCPSocket::shutdown(bool shutdownRead,bool shutdownWrite)
	{
	if(shutdownRead&&shutdownWrite)
		{
		if(::shutdown(socketFd,SHUT_RDWR)!=0)
			Misc::throwStdErr("Comm::TCPSocket:: Error while shutting down read and write");
		}
	else if(shutdownRead)
		{
		if(::shutdown(socketFd,SHUT_RD)!=0)
			Misc::throwStdErr("Comm::TCPSocket:: Error while shutting down read");
		}
	else if(shutdownWrite)
		{
		if(::shutdown(socketFd,SHUT_WR)!=0)
			Misc::throwStdErr("Comm::TCPSocket:: Error while shutting down write");
		}
	}

bool TCPSocket::getNoDelay(void) const
	{
	/* Get the TCP_NODELAY socket option: */
	int flag;
	socklen_t flagLen=sizeof(int);
	getsockopt(socketFd,IPPROTO_TCP,TCP_NODELAY,&flag,&flagLen);
	return flag!=0;
	}

void TCPSocket::setNoDelay(bool enable)
	{
	/* Set the TCP_NODELAY socket option: */
	int flag=enable?1:0;
	setsockopt(socketFd,IPPROTO_TCP,TCP_NODELAY,&flag,sizeof(int));
	}

bool TCPSocket::getCork(void) const
	{
	#ifdef __linux__
	/* Get the TCP_CORK socket option: */
	int flag;
	socklen_t flagLen=sizeof(int);
	getsockopt(socketFd,IPPROTO_TCP,TCP_CORK,&flag,&flagLen);
	return flag!=0;
	#else
	return false;
	#endif
	}

void TCPSocket::setCork(bool enable)
	{
	#ifdef __linux__
	/* Set the TCP_CORK socket option: */
	int flag=enable?1:0;
	setsockopt(socketFd,IPPROTO_TCP,TCP_CORK,&flag,sizeof(int));
	#endif
	}

bool TCPSocket::waitForData(long timeoutSeconds,long timeoutMicroseconds,bool throwException) const
	{
	fd_set readFdSet;
	FD_ZERO(&readFdSet);
	FD_SET(socketFd,&readFdSet);
	struct timeval tv;
	tv.tv_sec=timeoutSeconds;
	tv.tv_usec=timeoutMicroseconds;
	bool dataWaiting=select(socketFd+1,&readFdSet,0,0,&tv)>=0&&FD_ISSET(socketFd,&readFdSet);
	if(throwException&&!dataWaiting)
		throw TimeOut("TCPSocket: Time-out while waiting for data");
	return dataWaiting;
	}

bool TCPSocket::waitForData(const Misc::Time& timeout,bool throwException) const
	{
	fd_set readFdSet;
	FD_ZERO(&readFdSet);
	FD_SET(socketFd,&readFdSet);
	struct timeval tv=timeout;
	bool dataWaiting=select(socketFd+1,&readFdSet,0,0,&tv)>=0&&FD_ISSET(socketFd,&readFdSet);
	if(throwException&&!dataWaiting)
		throw TimeOut("TCPSocket: Time-out while waiting for data");
	return dataWaiting;
	}

size_t TCPSocket::read(void* buffer,size_t count)
	{
	char* byteBuffer=reinterpret_cast<char*>(buffer);
	ssize_t numBytesRead=::read(socketFd,byteBuffer,count);
	if(numBytesRead>0||errno==EAGAIN)
		return size_t(numBytesRead);
	else if(numBytesRead==0)
		{
		/* Other end terminated connection: */
		throw PipeError("TCPSocket: Connection terminated by peer during read");
		}
	else
		{
		/* Consider this a fatal error: */
		Misc::throwStdErr("TCPSocket: Fatal error during read");
		}
	
	/* Dummy statement to make compiler happy: */
	return 0;
	}

void TCPSocket::blockingRead(void* buffer,size_t count)
	{
	char* byteBuffer=reinterpret_cast<char*>(buffer);
	while(count>0)
		{
		ssize_t numBytesRead=::read(socketFd,byteBuffer,count);
		if(numBytesRead!=ssize_t(count))
			{
			if(numBytesRead>0)
				{
				/* Advance result pointer and retry: */
				count-=numBytesRead;
				byteBuffer+=numBytesRead;
				}
			else if(errno==EAGAIN||errno==EINTR)
				{
				/* Do nothing */
				}
			else if(numBytesRead==0)
				{
				/* Other end terminated connection: */
				throw PipeError("TCPSocket: Connection terminated by peer during read");
				}
			else
				{
				/* Consider this a fatal error: */
				Misc::throwStdErr("TCPSocket: Fatal error during read");
				}
			}
		else
			count=0;
		}
	}

void TCPSocket::blockingWrite(const void* buffer,size_t count)
	{
	const char* byteBuffer=reinterpret_cast<const char*>(buffer);
	while(count>0)
		{
		ssize_t numBytesWritten=::write(socketFd,byteBuffer,count);
		if(numBytesWritten!=ssize_t(count))
			{
			/* Check error condition: */
			if(numBytesWritten>=0||errno==EAGAIN||errno==EINTR)
				{
				/* Advance data pointer and try again: */
				count-=numBytesWritten;
				byteBuffer+=numBytesWritten;
				}
			else if(errno==EPIPE)
				{
				/* Other end terminated connection: */
				throw PipeError("TCPSocket: Connection terminated by peer during write");
				}
			else
				{
				/* Consider this a fatal error: */
				Misc::throwStdErr("TCPSocket: Fatal error during write");
				}
			}
		else
			count=0;
		}
	}

void TCPSocket::flush(void)
	{
	#ifdef __linux__
	/* Twiddle the TCP_CORK socket option to flush half-assembled packets: */
	int flag=0;
	setsockopt(socketFd,IPPROTO_TCP,TCP_CORK,&flag,sizeof(int));
	flag=1;
	setsockopt(socketFd,IPPROTO_TCP,TCP_CORK,&flag,sizeof(int));
	#endif
	}

}
