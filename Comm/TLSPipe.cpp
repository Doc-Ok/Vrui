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

#include <Comm/TLSPipe.h>

#include <stdlib.h>
#include <errno.h>
#include <stdexcept>
#include <Misc/PrintInteger.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/FdSet.h>
#include <Threads/Mutex.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace Comm {

/**************
Helper classes:
**************/

namespace {

class TLSContext // Class representing a singleton SSL library context set up for the TLS protocol, version 1.2 and up
	{
	/* Elements: */
	private:
	static TLSContext theContext; // The singleton SSL context
	Threads::Mutex acquireMutex; // Mutex serializing access to the acquireContext() method
	SSL_CTX* context; // The SSL library context
	
	/* Constructors and destructors: */
	TLSContext(void); // Creates an uninitialized SSL context
	TLSContext(const TLSContext& source); // Prohibit copy constructor
	TLSContext& operator=(const TLSContext& source); // Prohibit assignment operator
	~TLSContext(void); // Destroys the SSL context
	
	/* Methods: */
	public:
	static SSL_CTX* acquireContext(void); // Returns a pointer to the valid singleton SSL library context
	};

/***********************************
Static elements of class TLSContext:
***********************************/

TLSContext TLSContext::theContext;

/***************************
Methods of class TLSContext:
***************************/

TLSContext::TLSContext(void)
	:context(0)
	{
	}

TLSContext::~TLSContext(void)
	{
	/* Free the SSL library context: */
	if(context!=0)
		SSL_CTX_free(context);
	}

SSL_CTX* TLSContext::acquireContext(void)
	{
	Threads::Mutex::Lock acquireLock(theContext.acquireMutex);
	
	/* Create a new context if there isn't one yet: */
	if(theContext.context==0)
		{
		/* Create a new SSL library context using the TLS protocol: */
		const SSL_METHOD* sslMethod=TLS_method();
		if(sslMethod==0)
			throw std::runtime_error("Comm::TLSContext::acquireContext: Unable to access TLS method");
		theContext.context=SSL_CTX_new(sslMethod);
		if(theContext.context==0)
			throw std::runtime_error("Comm::TLSContext::acquireContext: Unable to create SSL context");
		
		/* Configure SSL verification: */
		SSL_CTX_set_verify(theContext.context,SSL_VERIFY_PEER,0);
		SSL_CTX_set_verify_depth(theContext.context,4); // Somewhat magic number; up to four proxies?
		SSL_CTX_set_default_verify_paths(theContext.context);
		
		/* Setup SSL options: */
		SSL_CTX_set_options(theContext.context,SSL_OP_ALL); // Enable all bug work-arounds
		SSL_CTX_set_min_proto_version(theContext.context,TLS1_2_VERSION);
		SSL_CTX_set_mode(theContext.context,SSL_MODE_ENABLE_PARTIAL_WRITE); // Enable partial writes
		SSL_CTX_set_mode(theContext.context,SSL_MODE_AUTO_RETRY); // Automatically retry read/write on re-negotiation
		}
	
	return theContext.context;
	}

}

/************************
Methods of class TLSPipe:
************************/

size_t TLSPipe::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Read more data from the SSL connection: */
	int readResult,sslError,error;
	do
		{
		readResult=SSL_read(ssl,buffer,int(bufferSize));
		if(readResult<=0)
			{
			error=errno;
			sslError=SSL_get_error(ssl,readResult);
			}
		}
	while(readResult<=0&&(sslError==SSL_ERROR_WANT_READ||sslError==SSL_ERROR_WANT_WRITE||sslError==SSL_ERROR_WANT_CONNECT));
	
	/* Handle the result from the read call: */
	if(readResult<=0&&sslError!=SSL_ERROR_ZERO_RETURN)
		{
		char buffer[512];
		if(sslError==SSL_ERROR_SYSCALL)
			{
			/* Some error at the socket level: */
			throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe: Socket-level error %d (%s) while reading from source",error,strerror(error)));
			}
		else
			{
			/* Unknown error; probably a bad thing: */
			throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe: Fatal error %d while reading from source",sslError));
			}
		}
	
	return size_t(readResult);
	}

void TLSPipe::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	while(bufferSize>0)
		{
		/* Write data to the SSL connection: */
		int writeResult=SSL_write(ssl,buffer,int(bufferSize));
		if(writeResult>0)
			{
			/* Prepare to write more data: */
			buffer+=writeResult;
			bufferSize-=size_t(writeResult);
			}
		else
			{
			int sslError=SSL_get_error(ssl,writeResult);
			if(sslError!=SSL_ERROR_WANT_READ&&sslError!=SSL_ERROR_WANT_WRITE&&sslError!=SSL_ERROR_WANT_CONNECT)
				{
				/* Unknown error; probably a bad thing: */
				char buffer[512];
				throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe: Fatal error %d while writing to sink",sslError));
				}
			}
		}
	}

size_t TLSPipe::writeDataUpTo(const IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Write data to the SSL connection: */
	int writeResult,sslError;
	do
		{
		writeResult=SSL_write(ssl,buffer,int(bufferSize));
		if(writeResult<=0)
			sslError=SSL_get_error(ssl,writeResult);
		}
	while(writeResult<=0&&(sslError==SSL_ERROR_WANT_READ||sslError==SSL_ERROR_WANT_WRITE||sslError==SSL_ERROR_WANT_CONNECT));
	
	/* Handle the result from the write call: */
	if(writeResult<=0)
		{
		/* Unknown error; probably a bad thing: */
		char buffer[512];
		throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe: Fatal error %d while writing to sink",sslError));
		}
	
	return size_t(writeResult);
	}

TLSPipe::TLSPipe(const char* hostName,int portId)
	:NetPipe(),
	 tcpPipe(0),fd(-1),ssl(0)
	{
	if(portId<0||portId>65535)
		{
		char buffer[512];
		throw OpenError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe::TLSPipe: Invalid port %d",portId));
		}
	
	/* Create a connect BIO for the given host name and port ID: */
	tcpPipe=BIO_new(BIO_s_connect());
	if(tcpPipe==0)
		throw OpenError("Comm::TLSPipe::TLSPipe: Unable to create TCP connection object");
	BIO_set_conn_hostname(tcpPipe,hostName);
	char portIdString[6];
	BIO_set_conn_port(tcpPipe,Misc::print(portId,portIdString+5));
	
	/* Retrieve the BIO's file descriptor: */
	BIO_get_fd(tcpPipe,&fd);
	
	/* Create an SSL connection: */
	ssl=SSL_new(TLSContext::acquireContext());
	if(ssl==0)
		{
		BIO_vfree(tcpPipe);
		throw OpenError("Comm::TLSPipe::TLSPipe: Unable to create SSL connection object");
		}
	
	/* Connect the SSL connection to the TCP connection: */
	BIO_up_ref(tcpPipe);
	SSL_set0_rbio(ssl,tcpPipe);
	SSL_set0_wbio(ssl,tcpPipe);
	
	/* Configure the SSL connection: */
	static const char* const preferredCiphers="HIGH:!aNull:!kRSA:!PSK:!SRP:!MD5:!RC4";
	if(SSL_set_cipher_list(ssl,preferredCiphers)==0)
		{
		SSL_free(ssl);
		throw OpenError("Comm::TLSPipe::TLSPipe: Unable to set preferred TLS ciphers");
		}
	SSL_set_tlsext_host_name(ssl,hostName);
	
	/* Connect to the host: */
	int sslError;
	if((sslError=BIO_do_connect(tcpPipe))<=0)
		{
		SSL_free(ssl);
		char buffer[512];
		throw OpenError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe::TLSPipe: Unable to connect to host %s on port %d due to error code %d",hostName,portId,sslError));
		}
	if((sslError=SSL_connect(ssl))<=0)
		{
		char buffer[512];
		int error=errno;
		sslError=SSL_get_error(ssl,sslError);
		SSL_free(ssl);
		if(sslError==SSL_ERROR_SYSCALL)
			throw OpenError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe::TLSPipe: Unable to establish TLS connection with host %s on port %d due to socket-level error %d (%s)",hostName,portId,error,strerror(error)));
		else
			throw OpenError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe::TLSPipe: Unable to establish TLS connection with host %s on port %d due to error code %d",hostName,portId,sslError));
		}
	
	/* Allocate read and write buffers able to hold a maximum size TLS record: */
	resizeReadBuffer(16384);
	resizeWriteBuffer(16384);
	}

TLSPipe::~TLSPipe(void)
	{
	/* Close the SSL connection: */
	if(ssl!=0)
		{
		/* Perform uni-directional shutdown and then close the underlying TCP connection: */
		if((SSL_get_shutdown(ssl)&SSL_SENT_SHUTDOWN)==0x0)
			SSL_shutdown(ssl);
		SSL_free(ssl);
		}
	}

int TLSPipe::getFd(void) const
	{
	return fd;
	}

bool TLSPipe::waitForData(void) const
	{
	/* Check if there is unread data in the buffer or in the underlying SSL connection: */
	if(getUnreadDataSize()>0||SSL_pending(ssl))
		return true;
	
	/* Wait for data on the underlying TCP socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,0)>=0&&readFds.isSet(fd);
	}

bool TLSPipe::waitForData(const Misc::Time& timeout) const
	{
	/* Check if there is unread data in the buffer or in the underlying SSL connection: */
	if(getUnreadDataSize()>0||SSL_pending(ssl))
		return true;
	
	/* Wait for data on the underlying TCP socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,timeout)>=0&&readFds.isSet(fd);
	}

void TLSPipe::shutdown(bool read,bool write)
	{
	/* Flush the write buffer: */
	flush();
	
	/* Perform bi-directional shutdown: */
	int ret;
	do
		{
		ret=SSL_shutdown(ssl);
		}
	while(ret==0);
	if(ret<0)
		{
		int sslError=SSL_get_error(ssl,ret);
		char buffer[512];
		throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe::shutdown: Error %d",sslError));
		}
	}

int TLSPipe::getPortId(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error("Comm::TLSPipe::getPortId: Unable to query socket address");
	
	/* Extract a numeric port ID from the socket's address: */
	char portIdBuffer[NI_MAXSERV];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,0,0,portIdBuffer,sizeof(portIdBuffer),NI_NUMERICSERV);
	if(niResult!=0)
		{
		char buffer[512];
		throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TCPPipe::getPortId: Unable to retrieve port ID due to error %s",gai_strerror(niResult)));
		}
	
	/* Convert the port ID string to a number: */
	int result=0;
	for(int i=0;i<NI_MAXSERV&&portIdBuffer[i]!='\0';++i)
		result=result*10+int(portIdBuffer[i]-'0');
	
	return result;
	}

std::string TLSPipe::getAddress(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error("Comm::TLSPipe::getAddress: Unable to query socket address");
	
	/* Extract a numeric address from the socket's address: */
	char addressBuffer[NI_MAXHOST];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,addressBuffer,sizeof(addressBuffer),0,0,NI_NUMERICHOST);
	if(niResult!=0)
		{
		char buffer[512];
		throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe::getAddress: Unable to retrieve address due to error %s",gai_strerror(niResult)));
		}
	
	return addressBuffer;
	}

std::string TLSPipe::getHostName(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error("Comm::TLSPipe::getHostName: Unable to query socket address");
	
	/* Extract a host name from the socket's address: */
	char hostNameBuffer[NI_MAXHOST];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,hostNameBuffer,sizeof(hostNameBuffer),0,0,0);
	if(niResult!=0)
		{
		char buffer[512];
		throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::TLSPipe::getHostName: Unable to retrieve host name due to error %s",gai_strerror(niResult)));
		}
	
	return hostNameBuffer;
	}

int TLSPipe::getPeerPortId(void) const
	{
	return atoi(BIO_get_conn_port(tcpPipe));
	}

std::string TLSPipe::getPeerAddress(void) const
	{
	return BIO_get_conn_hostname(tcpPipe);
	}

std::string TLSPipe::getPeerHostName(void) const
	{
	return BIO_get_conn_hostname(tcpPipe);
	}

X509* TLSPipe::getPeerCertificate(void)
	{
	return SSL_get_peer_certificate(ssl);
	}

bool TLSPipe::isPeerVerified(void)
	{
	/* Retrieve the peer's certificate: */
	X509* peerCertificate=SSL_get_peer_certificate(ssl);
	if(peerCertificate!=0)
		{
		#if 0
		/* Print some information from the certificate: */
		char* subject=X509_NAME_oneline(X509_get_subject_name(peerCertificate),0,0);
		std::cout<<"Certificate subject: "<<subject<<std::endl;
		OPENSSL_free(subject);
		char* issuer=X509_NAME_oneline(X509_get_issuer_name(peerCertificate),0,0);
		std::cout<<"Certificate issuer : "<<issuer<<std::endl;
		OPENSSL_free(issuer);
		#endif
		
		/* Release the certificate: */
		X509_free(peerCertificate);
		
		/* Return true if the peer's certificate was verified: */
		return SSL_get_verify_result(ssl)==X509_V_OK;
		}
	else
		return false;
	}

}
