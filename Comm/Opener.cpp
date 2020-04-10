/***********************************************************************
Opener - Class derived from IO::Opener to add additional functionality
provided by the Comm library, such as access to remote files over
HTTP/1.1.
Copyright (c) 2018 Oliver Kreylos

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

#include <Comm/Opener.h>

#include <stdexcept>
#include <Misc/FileNameExtensions.h>
#include <IO/GzipFilter.h>
#include <IO/SeekableFilter.h>
#include <Comm/Config.h>
#include <Comm/HttpFile.h>
#include <Comm/HttpDirectory.h>
#include <Comm/TCPPipe.h>
#if COMM_CONFIG_HAVE_OPENSSL
#include <Comm/TLSPipe.h>
#endif

// DEBUGGING
#include <iostream>

namespace Comm {

/*******************************
Static elements of class Opener:
*******************************/

Opener Opener::theOpener(true);

/***********************
Methods of class Opener:
***********************/

Opener::Opener(bool install)
	:IO::Opener(false)
	{
	if(install)
		{
		/* Install this opener with the IO library: */
		IO::Opener::installOpener(this);
		
		// DEBUGGING
		// IO::Opener* previous=IO::Opener::installOpener(this);
		// std::cout<<"Installing Comm::Opener at "<<this<<" over previous opener at "<<previous<<std::endl;
		}
	}

Opener::~Opener(void)
	{
	if(IO::Opener::getOpener()==this)
		{
		/* Uninstall this opener from the IO library: */
		IO::Opener::resetOpener();
		
		// DEBUGGING
		// std::cout<<"Uninstalling Comm::Opener; new opener is "<<IO::Opener::getOpener()<<std::endl;
		}
	}

IO::FilePtr Opener::openFile(const char* fileName,IO::File::AccessMode accessMode)
	{
	/* Check for supported file system protocols: */
	if(HttpFile::checkHttpPrefix(fileName))
		{
		/* Check if the requested access mode is supported by HTTP files: */
		if(accessMode==IO::File::WriteOnly||accessMode==IO::File::ReadWrite)
			throw std::runtime_error("Comm::openFile: Write access to HTTP files not supported");
		
		/* Open a remote file via the HTTP/1.1 protocol: */
		IO::FilePtr result=new HttpFile(fileName);
		
		/* Check if the file name has the .gz extension: */
		if(Misc::hasCaseExtension(fileName,".gz"))
			{
			/* Wrap a gzip filter around the standard file: */
			result=new IO::GzipFilter(result);
			}
		
		return result;
		}
	else
		{
		/* Delegate to the base class: */
		return IO::Opener::openFile(fileName,accessMode);
		}
	}

IO::DirectoryPtr Opener::openDirectory(const char* directoryName)
	{
	/* Check for supported file system protocols: */
	if(HttpFile::checkHttpPrefix(directoryName))
		{
		/* Open a remote directory via the HTTP/1.1 protocol: */
		return new HttpDirectory(directoryName);
		}
	else
		{
		/* Delegate to the base class: */
		return IO::Opener::openDirectory(directoryName);
		}
	}

IO::DirectoryPtr Opener::openDirectory(const char* directoryNameBegin,const char* directoryNameEnd)
	{
	/* Check for supported file system protocols: */
	if(HttpFile::checkHttpPrefix(directoryNameBegin,directoryNameEnd))
		{
		/* Open a remote directory via the HTTP/1.1 protocol: */
		return new HttpDirectory(directoryNameBegin,directoryNameEnd);
		}
	else
		{
		/* Delegate to the base class: */
		return IO::Opener::openDirectory(directoryNameBegin,directoryNameEnd);
		}
	}

IO::DirectoryPtr Opener::openFileDirectory(const char* fileName)
	{
	/* Check for supported file system protocols: */
	if(HttpFile::checkHttpPrefix(fileName))
		{
		/* Find the resource path component of the file name: */
		const char* resourcePath=HttpFile::getResourcePath(fileName);
		
		/* Open a remote directory via the HTTP/1.1 protocol: */
		return new HttpDirectory(fileName,Misc::getFileName(resourcePath));
		}
	else
		{
		/* Delegate to the base class: */
		return IO::Opener::openFileDirectory(fileName);
		}
	}

NetPipePtr Opener::openTCPPipe(const char* hostName,int portId)
	{
	/* Open a standard TCP pipe: */
	return new TCPPipe(hostName,portId);
	}

NetPipePtr Opener::openTLSPipe(const char* hostName,int portId)
	{
	#if COMM_CONFIG_HAVE_OPENSSL
	/* Open a TLS-secured TCP pipe: */
	return new TLSPipe(hostName,portId);
	#else
	throw std::runtime_error("Comm::openTLSPipe: OpenSSL library does not exist");
	#endif
	}

}
