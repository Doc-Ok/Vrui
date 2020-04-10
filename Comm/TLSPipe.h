/***********************************************************************
TLSPipe - Class to represent a TLS-secured TCP connection to a remote
server.
Copyright (c) 2019 Oliver Kreylos

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

#ifndef COMM_TLSPIPE_INCLUDED
#define COMM_TLSPIPE_INCLUDED

#include <Comm/NetPipe.h>

/********************
Forward declarations:
********************/

struct ssl_st;
typedef struct ssl_st SSL;
struct bio_st;
typedef struct bio_st BIO;
struct x509_st;
typedef struct x509_st X509;

namespace Comm {

class TLSPipe:public NetPipe
	{
	/* Elements: */
	private:
	BIO* tcpPipe; // Pointer to the underlying TCP connection object
	int fd; // File descriptor of the underlying TCP socket
	SSL* ssl; // Pointer to the SSL connection object
	
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	virtual size_t writeDataUpTo(const Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	TLSPipe(const char* hostName,int portId); // Opens a TLS-protected TCP socket connected to the given port on the given host with "DontCare" endianness setting
	private:
	TLSPipe(const TLSPipe& source); // Prohibit copy constructor
	TLSPipe& operator=(const TLSPipe& source); // Prohibit assignment operator
	public:
	virtual ~TLSPipe(void); // Closes the TLS-protected TCP connection
	
	/* Methods from IO::File: */
	virtual int getFd(void) const;
	
	/* Methods from Pipe: */
	virtual bool waitForData(void) const;
	virtual bool waitForData(const Misc::Time& timeout) const;
	virtual void shutdown(bool read,bool write);
	
	/* Methods from NetPipe: */
	virtual int getPortId(void) const;
	virtual std::string getAddress(void) const;
	virtual std::string getHostName(void) const;
	virtual int getPeerPortId(void) const;
	virtual std::string getPeerAddress(void) const;
	virtual std::string getPeerHostName(void) const;
	
	/* New methods: */
	X509* getPeerCertificate(void); // Returns the authentication certificate of the remote host
	bool isPeerVerified(void); // Returns true if the peer presented a verification certificate, and the certificate was verified
	};

}

#endif
