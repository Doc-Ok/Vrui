/***********************************************************************
Opener - Class derived from IO::Opener to add additional functionality
provided by the Comm library, such as access to remote files over
HTTP/1.1.
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

#ifndef COMM_OPENER_INCLUDED
#define COMM_OPENER_INCLUDED

#include <stdexcept>
#include <IO/Opener.h>
#include <Comm/NetPipe.h>

namespace Comm {

class Opener:public IO::Opener
	{
	/* Elements: */
	private:
	static Opener theOpener; // Static opener object created and activated when the Comm library is loaded
	
	/* Constructors and destructors: */
	public:
	Opener(bool install); // Creates an opener object and optionally installs it over the IO library's one
	virtual ~Opener(void); // Uninstalls the opener from the IO library
	
	/* Methods from IO::Opener: */
	virtual IO::FilePtr openFile(const char* fileName,IO::File::AccessMode accessMode);
	virtual IO::DirectoryPtr openDirectory(const char* directoryName);
	virtual IO::DirectoryPtr openDirectory(const char* directoryNameBegin,const char* directoryNameEnd);
	virtual IO::DirectoryPtr openFileDirectory(const char* fileName);
	
	/* New methods: */
	static Opener* getOpener(void) // Returns the currently installed opener as a Comm::Opener
		{
		/* Get the active opener and cast it to a Comm::Opener: */
		Opener* result=dynamic_cast<Opener*>(IO::Opener::getOpener());
		if(result==0)
			throw std::runtime_error("Comm::Opener::getOpener: Active IO::Opener is not a Comm::Opener");
		
		return result;
		}
	virtual NetPipePtr openTCPPipe(const char* hostName,int portId); // Opens a TCP connection to the given port on the host of the given name
	virtual NetPipePtr openTLSPipe(const char* hostName,int portId); // Opens a TLS-secured TCP connection to the given port on the host of the given name
	};

}

#endif
