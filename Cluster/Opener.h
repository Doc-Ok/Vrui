/***********************************************************************
Opener - Class derived from Comm::Opener to forward files from a
cluster's master to all slaves via multicast pipes.
Copyright (c) 2018-2019 Oliver Kreylos

This file is part of the Cluster Abstraction Library (Cluster).

The Cluster Abstraction Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Cluster Abstraction Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Cluster Abstraction Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef CLUSTER_OPENER_INCLUDED
#define CLUSTER_OPENER_INCLUDED

#include <stdexcept>
#include <Comm/Opener.h>

/* Forward declarations: */
namespace Cluster {
class Multiplexer;
}

namespace Cluster {

class Opener:public Comm::Opener
	{
	/* Elements: */
	private:
	static Opener theOpener; // Static opener object created and activated when the Cluster library is loaded
	Multiplexer* multiplexer; // Pointer to a multiplexer connecting a cluster
	IO::DirectoryPtr previousCurrentDirectory; // Pointer to previous current directory when a multiplexer is set
	
	/* Constructors and destructors: */
	public:
	Opener(bool install); // Creates an opener object and optionally installs it over the IO library's one
	virtual ~Opener(void); // Uninstalls the opener from the IO library
	
	/* Methods from IO::Opener: */
	virtual IO::FilePtr openFile(const char* fileName,IO::File::AccessMode accessMode);
	virtual IO::DirectoryPtr openDirectory(const char* directoryName);
	virtual IO::DirectoryPtr openDirectory(const char* directoryNameBegin,const char* directoryNameEnd);
	virtual IO::DirectoryPtr openFileDirectory(const char* fileName);
	
	/* Methods from Comm::Opener: */
	virtual Comm::NetPipePtr openTCPPipe(const char* hostName,int portId);
	virtual Comm::NetPipePtr openTLSPipe(const char* hostName,int portId);
	
	/* New methods: */
	static Opener* getOpener(void) // Returns the currently installed opener as a Cluster::Opener
		{
		/* Get the active opener and cast it to a Cluster::Opener: */
		Opener* result=dynamic_cast<Opener*>(IO::Opener::getOpener());
		if(result==0)
			throw std::runtime_error("Cluster::Opener::getOpener: Active IO::Opener is not a Cluster::Opener");
		
		return result;
		}
	void setMultiplexer(Multiplexer* newMultiplexer); // Sets the cluster multiplexer to be used to forward files
	static IO::FilePtr openFile(Multiplexer* multiplexer,const char* fileName,IO::File::AccessMode accessMode); // Method to open a file shared via the given cluster multiplexer
	};

}

#endif
