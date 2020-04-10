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

#include <Cluster/Opener.h>

#include <string.h>
#include <stdexcept>
#include <Misc/FileNameExtensions.h>
#include <IO/GzipFilter.h>
#include <Comm/HttpFile.h>
#include <Cluster/StandardFile.h>
#include <Cluster/StandardDirectory.h>
#include <Cluster/HttpDirectory.h>
#include <Cluster/TCPPipe.h>

// DEBUGGING
// #include <iostream>

namespace Cluster {

/*******************************
Static elements of class Opener:
*******************************/

Opener Opener::theOpener(true);

/***********************
Methods of class Opener:
***********************/

Opener::Opener(bool install)
	:Comm::Opener(false),
	 multiplexer(0)
	{
	if(install)
		{
		/* Install this opener with the IO library: */
		IO::Opener::installOpener(this);
		
		// DEBUGGING
		// IO::Opener* previous=IO::Opener::installOpener(this);
		// std::cout<<"Installing Cluster::Opener at "<<this<<" over previous opener at "<<previous<<std::endl;
		}
	}

Opener::~Opener(void)
	{
	if(IO::Opener::getOpener()==this)
		{
		/* Uninstall this opener from the IO library: */
		IO::Opener::resetOpener();
		
		// DEBUGGING
		// std::cout<<"Uninstalling Cluster::Opener; new opener is "<<IO::Opener::getOpener()<<std::endl;
		}
	}

IO::FilePtr Opener::openFile(const char* fileName,IO::File::AccessMode accessMode)
	{
	/* Check if there is an active multiplexer: */
	if(multiplexer!=0)
		{
		// DEBUGGING
		// std::cout<<"Opening file "<<fileName<<" in "<<(multiplexer->isMaster()?"master":"slave")<<" mode"<<std::endl;
		
		IO::FilePtr result;
		
		/* Check for supported file system protocols: */
		if(Comm::HttpFile::checkHttpPrefix(fileName))
			{
			/* Check if the requested access mode is supported by HTTP files: */
			if(accessMode==IO::File::WriteOnly||accessMode==IO::File::ReadWrite)
				throw std::runtime_error("Cluster::openFile: Write access to HTTP files not supported");
			
			/* Split the URL into its components: */
			Comm::HttpFile::URLParts urlParts=Comm::HttpFile::splitUrl(fileName);
			
			if(urlParts.https)
				throw std::runtime_error("Cluster::openFile: HTTPS connections not supported on clusters");
			
			/* Open a cluster-transparent TCP connection to the server: */
			Comm::PipePtr pipe;
			if(multiplexer->isMaster())
				pipe=new TCPPipeMaster(multiplexer,urlParts.serverName.c_str(),urlParts.portNumber);
			else
				pipe=new TCPPipeSlave(multiplexer,urlParts.serverName.c_str(),urlParts.portNumber);
			
			/* Open an HTTP file over the shared TCP pipe: */
			result=new Comm::HttpFile(urlParts,pipe);
			}
		else if(multiplexer->isMaster())
			{
			/* Open a master-side shared standard file: */
			result=new StandardFileMaster(multiplexer,fileName,accessMode);
			}
		else
			{
			/* Open a slave-side shared standard file: */
			result=new StandardFileSlave(multiplexer,fileName,accessMode);
			}
		
		/* Check if the file name has the .gz extension: */
		if(Misc::hasCaseExtension(fileName,".gz"))
			{
			/* Wrap a gzip filter around the opened file: */
			result=new IO::GzipFilter(result);
			}
		
		return result;
		}
	else
		{
		// DEBUGGING
		// std::cout<<"Opening file "<<fileName<<" in single-system mode"<<std::endl;
		
		/* Delegate to the base class: */
		return Comm::Opener::openFile(fileName,accessMode);
		}
	}

IO::DirectoryPtr Opener::openDirectory(const char* directoryName)
	{
	/* Check if there is an active multiplexer: */
	if(multiplexer!=0)
		{
		/* Check for supported file system protocols: */
		const char* httpPrefixEnd;
		if(Comm::HttpFile::checkHttpPrefix(directoryName,&httpPrefixEnd))
			{
			/* Check if the caller requested an HTTPS directory: */
			if(httpPrefixEnd-directoryName==8)
				throw std::runtime_error("Cluster::openDirectory: HTTPS connections not supported on clusters");
			
			/* Open a cluster-transparent remote directory via the HTTP/1.1 protocol: */
			return new HttpDirectory(directoryName,multiplexer);
			}
		else if(multiplexer->isMaster())
			{
			// DEBUGGING
			// std::cout<<"Cluster: opening master-side directory for "<<directoryName<<std::endl;
			
			/* Open a master-side shared standard directory: */
			return new StandardDirectoryMaster(multiplexer,directoryName);
			}
		else
			{
			// DEBUGGING
			// std::cout<<"Cluster: opening slave-side directory for "<<directoryName<<std::endl;
			
			/* Open a slave-side shared standard directory: */
			return new StandardDirectorySlave(multiplexer,directoryName);
			}
		}
	else
		{
		// DEBUGGING
		// std::cout<<"Cluster: opening single-system directory for "<<directoryName<<std::endl;
			
		/* Delegate to the base class: */
		return Comm::Opener::openDirectory(directoryName);
		}
	}

IO::DirectoryPtr Opener::openDirectory(const char* directoryNameBegin,const char* directoryNameEnd)
	{
	/* Check if there is an active multiplexer: */
	if(multiplexer!=0)
		{
		/* Check for supported file system protocols: */
		const char* httpPrefixEnd;
		if(Comm::HttpFile::checkHttpPrefix(directoryNameBegin,directoryNameEnd,&httpPrefixEnd))
			{
			/* Check if the caller requested an HTTPS directory: */
			if(httpPrefixEnd-directoryNameBegin==8)
				throw std::runtime_error("Cluster::openDirectory: HTTPS connections not supported on clusters");
			
			/* Open a cluster-transparent remote directory via the HTTP/1.1 protocol: */
			return new HttpDirectory(directoryNameBegin,directoryNameEnd,multiplexer);
			}
		else if(multiplexer->isMaster())
			{
			/* Open a master-side shared standard directory: */
			return new StandardDirectoryMaster(multiplexer,directoryNameBegin,directoryNameEnd);
			}
		else
			{
			/* Open a slave-side shared standard directory: */
			return new StandardDirectorySlave(multiplexer,directoryNameBegin,directoryNameEnd);
			}
		}
	else
		{
		/* Delegate to the base class: */
		return Comm::Opener::openDirectory(directoryNameBegin,directoryNameEnd);
		}
	}

IO::DirectoryPtr Opener::openFileDirectory(const char* fileName)
	{
	/* Check if there is an active multiplexer: */
	if(multiplexer!=0)
		{
		/* Check for supported file system protocols: */
		const char* httpPrefixEnd;
		if(Comm::HttpFile::checkHttpPrefix(fileName,&httpPrefixEnd))
			{
			/* Check if the caller requested an HTTPS directory: */
			if(httpPrefixEnd-fileName==8)
				throw std::runtime_error("Cluster::openFileDirectory: HTTPS connections not supported on clusters");
			
			/* Find the resource path component of the file name: */
			const char* resourcePath=Comm::HttpFile::getResourcePath(fileName);
			
			/* Open a remote directory via the HTTP/1.1 protocol: */
			return new HttpDirectory(fileName,Misc::getFileName(resourcePath),multiplexer);
			}
		else if(multiplexer->isMaster())
			{
			/* Open a master-side shared standard directory: */
			return new StandardDirectoryMaster(multiplexer,fileName,Misc::getFileName(fileName));
			}
		else
			{
			/* Open a slave-side shared standard directory: */
			return new StandardDirectorySlave(multiplexer,fileName,Misc::getFileName(fileName));
			}
		}
	else
		{
		/* Delegate to the base class: */
		return Comm::Opener::openFileDirectory(fileName);
		}
	}

Comm::NetPipePtr Opener::openTCPPipe(const char* hostName,int portId)
	{
	/* Check if there is an active multiplexer: */
	if(multiplexer!=0)
		{
		if(multiplexer->isMaster())
			{
			/* Open a master-side distributed TCP pipe: */
			return new TCPPipeMaster(multiplexer,hostName,portId);
			}
		else
			{
			/* Open a slave-side distributed TCP pipe: */
			return new TCPPipeSlave(multiplexer,hostName,portId);
			}
		}
	else
		{
		/* Delegate to the base class: */
		return Comm::Opener::openTCPPipe(hostName,portId);
		}
	}

Comm::NetPipePtr Opener::openTLSPipe(const char* hostName,int portId)
	{
	/* Check if there is an active multiplexer: */
	if(multiplexer!=0)
		throw std::runtime_error("Comm::openTLSPipe: TLS connections not supported on clusters");
	else
		{
		/* Delegate to the base class: */
		return Comm::Opener::openTCPPipe(hostName,portId);
		}
	}

void Opener::setMultiplexer(Multiplexer* newMultiplexer)
	{
	/* Store the given multiplexer: */
	multiplexer=newMultiplexer;
	
	/* Update the current directory of the IO library to use the new multiplexer (or not use it if it's invalid): */
	if(multiplexer!=0)
		{
		/* Remember the previous current directory: */
		previousCurrentDirectory=IO::Directory::getCurrent();
		
		/* Replace it with a cluster-aware directory with the same path: */
		IO::Directory::setCurrent(openDirectory(previousCurrentDirectory->getPath().c_str()));
		}
	else
		{
		/* Reset to the previous current directory: */
		IO::Directory::setCurrent(previousCurrentDirectory);
		previousCurrentDirectory=0;
		}
	}

IO::FilePtr Opener::openFile(Multiplexer* multiplexer,const char* fileName,IO::File::AccessMode accessMode)
	{
	IO::FilePtr result;
	
	if(multiplexer->isMaster())
		{
		/* Open a master-side shared standard file: */
		result=new StandardFileMaster(multiplexer,fileName,accessMode);
		}
	else
		{
		/* Open a slave-side shared standard file: */
		result=new StandardFileSlave(multiplexer,fileName,accessMode);
		}
	
	/* Check if the file name has the .gz extension: */
	if(Misc::hasCaseExtension(fileName,".gz"))
		{
		/* Wrap a gzip filter around the opened file: */
		result=new IO::GzipFilter(result);
		}
	
	return result;
	}

}
