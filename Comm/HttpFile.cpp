/***********************************************************************
HttpFile - Class for high-performance reading from remote files using
the HTTP/1.1 protocol.
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

#include <Comm/HttpFile.h>

#include <string.h>
#include <string>
#include <Misc/ThrowStdErr.h>
#include <Misc/MessageLogger.h>
#include <Misc/Time.h>
#include <IO/ValueSource.h>
#include <Comm/Config.h>
#include <Comm/TCPPipe.h>
#if COMM_CONFIG_HAVE_OPENSSL
#include <Comm/TLSPipe.h>
#endif

namespace Comm {

namespace {

/****************
Helper functions:
****************/

size_t parseChunkHeader(Comm::Pipe& pipe)
	{
	/* Read the next chunk header: */
	size_t chunkSize=0;
	int digit;
	while(true)
		{
		digit=pipe.getChar();
		if(digit>='0'&&digit<='9')
			chunkSize=(chunkSize<<4)+(digit-'0');
		else if(digit>='A'&&digit<='F')
			chunkSize=(chunkSize<<4)+(digit-'A'+10);
		else if(digit>='a'&&digit<='f')
			chunkSize=(chunkSize<<4)+(digit-'a'+10);
		else
			break;
		}

	/* Skip the rest of the chunk header: */
	while(digit!='\r')
		digit=pipe.getChar();
	if(pipe.getChar()!='\n')
		throw IO::File::Error("HttpFile::readData: Malformed HTTP chunk header");
	
	return chunkSize;
	}

}

/*************************
Methods of class HttpFile:
*************************/

size_t HttpFile::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Read depending on the reply body's transfer encoding: */
	if(chunked)
		{
		/* Check if the current chunk is finished: */
		if(unreadSize==0)
			{
			/* Bail out if the EOF chunk has already been read: */
			if(haveEof)
				return 0;
			
			/* Skip the chunk footer: */
			if(pipe->getChar()!='\r'||pipe->getChar()!='\n')
				throw IO::File::Error("Comm::HttpFile: Malformed HTTP chunk footer");
			
			/* Parse the next chunk header: */
			unreadSize=parseChunkHeader(*pipe);
			}
		
		/* Set the EOF flag if this chunk has size zero: */
		if(unreadSize==0)
			{
			haveEof=true;
			return 0;
			}
		
		/* Read more data directly from the pipe's read buffer: */
		void* pipeBuffer;
		size_t pipeSize=pipe->readInBuffer(pipeBuffer,unreadSize);
		setReadBuffer(pipeSize,static_cast<Byte*>(pipeBuffer),false);
		
		/* Reduce the unread data size and return the read size: */
		unreadSize-=pipeSize;
		return pipeSize;
		}
	else if(fixedSize)
		{
		/* Check for end-of-file: */
		if(unreadSize==0)
			return 0;
		
		/* Read more data directly from the pipe's read buffer: */
		void* pipeBuffer;
		size_t pipeSize=pipe->readInBuffer(pipeBuffer,unreadSize);
		setReadBuffer(pipeSize,static_cast<Byte*>(pipeBuffer),false);
		
		/* Reduce the unread data size and return the read size: */
		unreadSize-=pipeSize;
		return pipeSize;
		}
	else
		{
		/* Read more data directly from the pipe's read buffer: */
		void* pipeBuffer;
		size_t pipeSize=pipe->readInBuffer(pipeBuffer);
		setReadBuffer(pipeSize,static_cast<Byte*>(pipeBuffer),false);
		
		/* Return the read size: */
		return pipeSize;
		}
	}

void HttpFile::init(const HttpFile::URLParts& urlParts,const Misc::Time* timeout)
	{
	/* Assemble the GET request: */
	std::string request;
	request.append("GET");
	request.push_back(' ');
	request.append(urlParts.resourcePath);
	request.push_back(' ');
	request.append("HTTP/1.1\r\n");
	
	request.append("Host: ");
	request.append(urlParts.serverName);
	request.push_back(':');
	int pn=urlParts.portNumber;
	char buf[10];
	char* bufPtr=buf;
	do
		{
		*(bufPtr++)=char(pn%10+'0');
		pn/=10;
		}
	while(pn!=0);
	while(bufPtr!=buf)
		request.push_back(*(--bufPtr));
	request.append("\r\n");
	
	#if 0
	request.append("Accept: text/html\r\n");
	#endif
	
	#if 0
	request.append("Connection: keep-alive\r\n");
	#endif
	
	request.append("\r\n");
	
	/* Send the GET request: */
	pipe->writeRaw(request.data(),request.size());
	pipe->flush();
	
	if(timeout!=0)
		{
		/* Wait for the server's reply: */
		if(!pipe->waitForData(*timeout))
			{
			char buffer[512];
			throw OpenError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::HttpFile: Timeout while waiting for reply from server \"%s\" on port %d",urlParts.serverName.c_str(),urlParts.portNumber));
			}
		}
	
	{
	/* Attach a value source to the pipe to parse the server's reply: */
	IO::ValueSource reply(pipe);
	reply.setPunctuation("()<>@,;:\\/[]?={}\r");
	reply.setQuotes("\"");
	reply.skipWs();
	
	/* Read the status line: */
	if(!reply.isLiteral("HTTP")||!reply.isLiteral('/'))
		{
		char buffer[512];
		throw OpenError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::HttpFile: Malformed HTTP reply from server \"%s\" on port %d",urlParts.serverName.c_str(),urlParts.portNumber));
		}
	reply.skipString();
	unsigned int statusCode=reply.readUnsignedInteger();
	if(statusCode!=200)
		{
		char buffer[1024];
		
		/* Read the error string: */
		std::string error=reply.readLine();
		if(error.back()=='\r')
			error.pop_back();
		reply.skipWs();
		
		/* Handle known HTTP errors: */
		if(statusCode==301) // HTTP 301 Moved Permanently
			{
			/* Parse reply options to find a location tag: */
			while(!reply.eof()&&reply.peekc()!='\r')
				{
				if(reply.isString("Location:"))
					{
					/* Throw a redirect error: */
					std::string redirectUrl=reply.readLine();
					throw HttpRedirect(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::HttpFile: Resource \"%s\" on server \"%s\" on port %d permanently moved to location \"%s\"",urlParts.resourcePath.c_str(),urlParts.serverName.c_str(),urlParts.portNumber,redirectUrl.c_str()),error,redirectUrl);
					}
				
				reply.skipLine();
				reply.skipWs();
				}
			}
		
		/* Throw a generic HTTP protocol error: */
		throw HttpError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::HttpFile: HTTP error %d (%s) while opening resource \"%s\" on server \"%s\" on port %d",statusCode,error.c_str(),urlParts.resourcePath.c_str(),urlParts.serverName.c_str(),urlParts.portNumber),statusCode,error);
		}
	
	/* Skip the rest of the status line: */
	reply.readLine();
	reply.skipWs();
	
	/* Parse reply options until the first empty line: */
	while(!reply.eof()&&reply.peekc()!='\r')
		{
		/* Read the option tag: */
		std::string option=reply.readString();
		if(reply.isLiteral(':'))
			{
			/* Handle the option value: */
			if(option=="Transfer-Encoding")
				{
				/* Parse the comma-separated list of transfer encodings: */
				while(true)
					{
					std::string coding=reply.readString();
					if(coding=="chunked")
						chunked=true;
					else
						{
						/* Skip the transfer extension: */
						while(reply.isLiteral(';'))
							{
							reply.skipString();
							if(!reply.isLiteral('='))
								{
								char buffer[512];
								throw OpenError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::HttpFile: Malformed HTTP reply from server \"%s\" on port %d",urlParts.serverName.c_str(),urlParts.portNumber));
								}
							reply.skipString();
							}
						}
					if(reply.eof()||reply.peekc()!=',')
						break;
					while(!reply.eof()&&reply.peekc()==',')
						reply.readChar();
					}
				}
			else if(option=="Content-Length")
				{
				if(!chunked)
					{
					fixedSize=true;
					unreadSize=reply.readUnsignedInteger();
					}
				}
			}
		
		/* Skip the rest of the line: */
		reply.skipLine();
		reply.skipWs();
		}
	
	/* Read the CR/LF pair: */
	if(reply.getChar()!='\r'||reply.getChar()!='\n')
		{
		char buffer[512];
		throw OpenError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"Comm::HttpFile: Malformed HTTP reply from server \"%s\" on port %d",urlParts.serverName.c_str(),urlParts.portNumber));
		}
	}
	
	if(chunked)
		{
		/* Read the first chunk header: */
		unreadSize=parseChunkHeader(*pipe);
		haveEof=unreadSize==0;
		}
	
	canReadThrough=false;
	}

HttpFile::HttpFile(const char* fileUrl,const Misc::Time* timeout)
	:IO::File(),
	 chunked(false),haveEof(false),
	 fixedSize(false),
	 unreadSize(0),
	 gzipped(false)
	{
	/* Parse the URL to determine server name, port, and absolute resource location: */
	URLParts urlParts=splitUrl(fileUrl);
	
	/* Connect to the HTTP server: */
	if(urlParts.https)
		{
		#if COMM_CONFIG_HAVE_OPENSSL
		/* Open a TLS-secured TCP connection to the HTTP server: */
		pipe=new Comm::TLSPipe(urlParts.serverName.c_str(),urlParts.portNumber);
		#else
		throw std::runtime_error("Comm::HttpFile: HTTPS connections not supported due to lack of OpenSSL library");
		#endif
		}
	else
		{
		/* Open a standard TCP connection to the HTTP server: */
		pipe=new Comm::TCPPipe(urlParts.serverName.c_str(),urlParts.portNumber);
		}
	
	/* Initialize the HTTP parser: */
	init(urlParts,timeout);
	}

HttpFile::HttpFile(const HttpFile::URLParts& urlParts,Comm::PipePtr sPipe,const Misc::Time* timeout)
	:IO::File(),
	 pipe(sPipe),
	 chunked(false),haveEof(false),
	 fixedSize(false),
	 unreadSize(0),
	 gzipped(false)
	{
	/* Initialize the HTTP parser: */
	init(urlParts,timeout);
	}

HttpFile::~HttpFile(void)
	{
	try
		{
		/* Skip all unread parts of the HTTP reply body: */
		if(chunked)
			{
			if(!haveEof)
				{
				/* Skip all leftover chunks: */
				while(true)
					{
					/* Skip the rest of the current chunk: */
					pipe->skip<char>(unreadSize);
					
					/* Skip the chunk footer: */
					if(pipe->getChar()!='\r'||pipe->getChar()!='\n')
						throw Error("Comm::HttpFile: Malformed HTTP chunk footer");
					
					/* Parse the next chunk header: */
					unreadSize=parseChunkHeader(*pipe);
					if(unreadSize==0)
						break;
					}
				}
			
			/* Skip any optional message trailers: */
			while(pipe->getChar()!='\r')
				{
				/* Skip the line: */
				while(pipe->getChar()!='\r')
					;
				if(pipe->getChar()!='\n')
					throw Error("Comm::HttpFile: Malformed HTTP body trailer");
				}
			if(pipe->getChar()!='\n')
				throw Error("Comm::HttpFile: Malformed HTTP body trailer");
			}
		else if(fixedSize)
			{
			/* Skip the rest of the fixed-size message body: */
			pipe->skip<char>(unreadSize);
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Print an error message and carry on: */
		Misc::formattedUserError("Comm::HttpFile: Caught exception \"%s\" while closing file",err.what());
		}
	
	/* Release the read buffer: */
	setReadBuffer(0,0,false);
	}

int HttpFile::getFd(void) const
	{
	/* Return pipe's file descriptor: */
	return pipe->getFd();
	}

size_t HttpFile::getReadBufferSize(void) const
	{
	/* Return pipe's read buffer size, since we're sharing it: */
	return pipe->getReadBufferSize();
	}

size_t HttpFile::resizeReadBuffer(size_t newReadBufferSize)
	{
	/* Ignore the request and return pipe's read buffer size, since we're sharing it: */
	return pipe->getReadBufferSize();
	}

bool HttpFile::checkHttpPrefix(const char* url,const char** prefixEnd)
	{
	/* Initialize the prefix end: */
	if(prefixEnd!=0)
		*prefixEnd=url;
	
	/* Check for initial "http": */
	if(url[0]=='h'&&url[1]=='t'&&url[2]=='t'&&url[3]=='p')
		{
		url+=4;
		
		/* Check for "https": */
		if(url[0]=='s')
			++url;
		
		/* Check for "://": */
		if(url[0]==':'&&url[1]=='/'&&url[2]=='/')
			{
			/* Found a match: */
			if(prefixEnd!=0)
				*prefixEnd=url+3;
			return true;
			}
		}
	
	/* Wasn't a match: */
	return false;
	}

bool HttpFile::checkHttpPrefix(const char* urlBegin,const char* urlEnd,const char** prefixEnd)
	{
	/* Initialize the prefix end: */
	if(prefixEnd!=0)
		*prefixEnd=urlBegin;
	
	/* Check for initial "http": */
	if(urlEnd-urlBegin>=4&&urlBegin[0]=='h'&&urlBegin[1]=='t'&&urlBegin[2]=='t'&&urlBegin[3]=='p')
		{
		urlBegin+=4;
		
		/* Check for "https": */
		if(urlBegin!=urlEnd&&urlBegin[0]=='s')
			++urlBegin;
		
		/* Check for "://": */
		if(urlEnd-urlBegin>=3&&urlBegin[0]==':'&&urlBegin[1]=='/'&&urlBegin[2]=='/')
			{
			/* Found a match: */
			if(prefixEnd!=0)
				*prefixEnd=urlBegin+3;
			return true;
			}
		}
	
	/* Wasn't a match: */
	return false;
	}

const char* HttpFile::getResourcePath(const char* url)
	{
	/* Parse the URL to skip server name and port: */
	const char* uPtr=url;
	
	/* Skip the protocol identifier: */
	checkHttpPrefix(uPtr,&uPtr);
	
	/* Server name is terminated by colon, slash, or NUL: */
	while(*uPtr!='\0'&&*uPtr!=':'&&*uPtr!='/')
		++uPtr;
	
	/* Skip the port number: */
	if(*uPtr==':')
		{
		++uPtr;
		while(*uPtr>='0'&&*uPtr<='9')
			++uPtr;
		}
	
	return uPtr;
	}

HttpFile::URLParts HttpFile::splitUrl(const char* url)
	{
	URLParts result;
	
	/* Skip the protocol identifier and identify secure HTTPS: */
	const char* uPtr;
	result.https=checkHttpPrefix(url,&uPtr)&&uPtr-url==8;
	
	/* Server name is terminated by colon, slash, or NUL: */
	const char* serverStart=uPtr;
	while(*uPtr!='\0'&&*uPtr!=':'&&*uPtr!='/')
		++uPtr;
	result.serverName=std::string(serverStart,uPtr);
	
	/* Get the port number: */
	result.portNumber=result.https?443:80;
	if(*uPtr==':')
		{
		++uPtr;
		result.portNumber=0;
		while(*uPtr>='0'&&*uPtr<='9')
			{
			result.portNumber=result.portNumber*10+int(*uPtr-'0');
			++uPtr;
			}
		}
	
	/* Get the absolute resource path: */
	if(*uPtr=='/')
		{
		/* Retrieve the absolute path: */
		result.resourcePath=uPtr;
		}
	else
		{
		/* Use the root resource if no path is specified: */
		result.resourcePath.push_back('/');
		}
	
	return result;
	}

HttpFile::URLParts HttpFile::splitUrl(const char* urlBegin,const char* urlEnd)
	{
	URLParts result;
	
	/* Skip the protocol identifier and identify secure HTTPS: */
	const char* uPtr;
	result.https=checkHttpPrefix(urlBegin,urlEnd,&uPtr)&&uPtr-urlBegin==8;
	
	/* Server name is terminated by colon, slash, or NUL: */
	const char* serverStart=uPtr;
	while(uPtr!=urlEnd&&*uPtr!=':'&&*uPtr!='/')
		++uPtr;
	result.serverName=std::string(serverStart,uPtr);
	
	/* Get the port number: */
	result.portNumber=result.https?443:80;
	if(*uPtr==':')
		{
		++uPtr;
		result.portNumber=0;
		while(uPtr!=urlEnd&&*uPtr>='0'&&*uPtr<='9')
			{
			result.portNumber=result.portNumber*10+int(*uPtr-'0');
			++uPtr;
			}
		}
	
	/* Get the absolute resource path: */
	if(uPtr!=urlEnd&&*uPtr=='/')
		{
		/* Retrieve the absolute path: */
		result.resourcePath=std::string(uPtr,urlEnd);
		}
	else
		{
		/* Use the root resource if no path is specified: */
		result.resourcePath.push_back('/');
		}
	
	return result;
	}

}
