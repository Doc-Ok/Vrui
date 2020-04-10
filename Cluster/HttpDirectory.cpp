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

#include <Cluster/HttpDirectory.h>

#include <Misc/ThrowStdErr.h>
#include <Comm/HttpFile.h>
#include <Cluster/Multiplexer.h>
#include <Cluster/TCPPipe.h>

namespace Cluster {

/******************************
Methods of class HttpDirectory:
******************************/

IO::DirectoryPtr HttpDirectory::getParent(void) const
	{
	/* Check if the directory has a parent: */
	if(hasParent())
		{
		/* Get the parent directory's URL: */
		std::string parentUrl=getPath("..");
		
		/* Return a new directory for the parent URL: */
		return new HttpDirectory(parentUrl.c_str(),multiplexer);
		}
	else
		return 0;
	}

IO::FilePtr HttpDirectory::openFile(const char* fileName,IO::File::AccessMode accessMode) const
	{
	if(multiplexer==0)
		{
		/* Open a non-shared remote file via the HTTP/1.1 protocol: */
		return Comm::HttpDirectory::openFile(fileName,accessMode);
		}
	else
		{
		/* Check the requested access mode: */
		if(accessMode==IO::File::WriteOnly||accessMode==IO::File::ReadWrite)
			Misc::throwStdErr("Cluster::HttpDirectory::openFile: Write access to HTTP files not supported");
		
		/* Get the file's URL and split it into its components: */
		Comm::HttpFile::URLParts urlParts=Comm::HttpFile::splitUrl(getPath(fileName).c_str());
		
		/* Open a shared TCP pipe: */
		Comm::PipePtr pipe;
		if(multiplexer->isMaster())
			pipe=new TCPPipeMaster(multiplexer,urlParts.serverName.c_str(),urlParts.portNumber);
		else
			pipe=new TCPPipeSlave(multiplexer,urlParts.serverName.c_str(),urlParts.portNumber);
		
		/* Open an HTTP file over the shared TCP pipe: */
		return new Comm::HttpFile(urlParts,pipe);
		}
	}

IO::DirectoryPtr HttpDirectory::openDirectory(const char* directoryName) const
	{
	/* Get the given directory's URL: */
	std::string directoryUrl=getPath(directoryName);
	
	/* Return a new directory for the directory's URL: */
	return new HttpDirectory(directoryUrl.c_str(),multiplexer);
	}

}
