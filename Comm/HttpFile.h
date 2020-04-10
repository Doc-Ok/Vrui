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

#ifndef COMM_HTTPFILE_INCLUDED
#define COMM_HTTPFILE_INCLUDED

#include <string>
#include <IO/File.h>
#include <Comm/Pipe.h>

namespace Comm {

class HttpFile:public IO::File
	{
	/* Embedded classes: */
	public:
	struct URLParts // Structure to hold the three components of an HTTP URL
		{
		/* Elements: */
		public:
		bool https; // Flag whether the URL was a secure HTTPS URL
		std::string serverName; // Server host name
		int portNumber; // Server port number
		std::string resourcePath; // Absolute resource path
		};
	
	class HttpError:public IO::File::OpenError // Exception class to signal HTTP protocol errors
		{
		/* Elements: */
		public:
		unsigned int statusCode; // HTTP status code
		std::string error; // The error string as reported by the HTTP server
		
		/* Constructors and destructors: */
		HttpError(const char* message,unsigned int sStatusCode,const std::string& sError)
			:IO::File::OpenError(message),
			 statusCode(sStatusCode),error(sError)
			 {
			 }
		};
	
	class HttpRedirect:public HttpError // Exception class for permanent URL redirection
		{
		/* Elements: */
		public:
		std::string redirectUrl; // Redirected URL
		
		/* Constructors and destructors: */
		HttpRedirect(const char* message,const std::string& sError,const std::string& sRedirectUrl)
			:HttpError(message,301,sError),
			 redirectUrl(sRedirectUrl)
			{
			}
		};
	
	/* Elements: */
	private:
	PipePtr pipe; // Pipe connected to the HTTP server
	bool chunked; // Flag whether the file is transfered in chunks
	bool haveEof; // Flag if the zero-sized EOF chunk was already seen
	bool fixedSize; // Flag whether the file's size is known a-priori
	size_t unreadSize; // Number of unread bytes in the current chunk or the entire fixed-size file
	bool gzipped; // Flag whether the HTTP payload has been gzip-compressed for transmission
	
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	
	/* Private methods: */
	private:
	void init(const URLParts& urlParts,const Misc::Time* timeout);
	
	/* Constructors and destructors: */
	public:
	HttpFile(const char* fileUrl,const Misc::Time* timeout =0); // Opens file of the given URL over a private server connection
	HttpFile(const URLParts& urlParts,PipePtr sPipe,const Misc::Time* timeout =0); // Opens file of the given URL over the existing server connection
	virtual ~HttpFile(void); // Closes the HTTP file
	
	/* Methods from IO::File: */
	virtual int getFd(void) const;
	virtual size_t getReadBufferSize(void) const;
	virtual size_t resizeReadBuffer(size_t newReadBufferSize);
	
	/* New methods: */
	static bool checkHttpPrefix(const char* url,const char** prefixEnd =0); // Checks if the given URL begins with an http protocol prefix, sets prefixEnd to behind prefix if !=0
	static bool checkHttpPrefix(const char* urlBegin,const char* urlEnd,const char** prefixEnd =0); // Ditto, with URL delimited by beginning and end pointers
	static const char* getResourcePath(const char* url); // Returns a pointer to the beginning of the resource path component of the given URL, which might be empty for the root resource
	static URLParts splitUrl(const char* url); // Splits the given HTTP URL into its components
	static URLParts splitUrl(const char* urlBegin,const char* urlEnd); // Ditto, with URL delimited by beginning and end pointers
	bool isGzipped(void) const // Returns true if the file's contents are gzip-compressed
		{
		return gzipped;
		}
	};

}

#endif
