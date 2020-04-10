/***********************************************************************
HttpDirectory - Class to access remote directories over HTTP/1.1 in a
cluster-transparent fashion.
Copyright (c) 2018 Oliver Kreylos

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

#ifndef CLUSTER_HTTPDIRECTORY_INCLUDED
#define CLUSTER_HTTPDIRECTORY_INCLUDED

#include <Comm/HttpDirectory.h>

/* Forward declarations: */
namespace Cluster {
class Multiplexer;
}

namespace Cluster {

class HttpDirectory:public Comm::HttpDirectory
	{
	/* Elements: */
	private:
	Multiplexer* multiplexer; // Pointer to a cluster multiplexer
	
	/* Constructors and destructors: */
	public:
	HttpDirectory(const char* sUrl,Multiplexer* sMultiplexer) // Creates a directory for the given URL and shares it over the given cluster multiplexer
		:Comm::HttpDirectory(sUrl),
		 multiplexer(sMultiplexer)
		{
		}
	HttpDirectory(const char* sUrlBegin,const char* sUrlEnd,Multiplexer* sMultiplexer) // Ditto, with URL defined by beginning and ending iterators
		:Comm::HttpDirectory(sUrlBegin,sUrlEnd),
		 multiplexer(sMultiplexer)
		{
		}
	
	/* Methods from IO::Directory: */
	virtual IO::DirectoryPtr getParent(void) const;
	virtual IO::FilePtr openFile(const char* fileName,IO::File::AccessMode accessMode =IO::File::ReadOnly) const;
	virtual IO::DirectoryPtr openDirectory(const char* directoryName) const;
	};

}

#endif
