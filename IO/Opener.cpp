/***********************************************************************
Opener - Class to encapsulate how files and other file-like objects are
opened, to expose functionality of higher-level libraries at the base IO
level.
Copyright (c) 2018 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <IO/Opener.h>

#include <Misc/FileNameExtensions.h>
#include <IO/StandardFile.h>
#include <IO/GzipFilter.h>
#include <IO/SeekableFilter.h>
#include <IO/StandardDirectory.h>
#include <IO/StandardFile.h>

// DEBUGGING
// #include <iostream>

namespace IO {

/*******************************
Static elements of class Opener:
*******************************/

Opener Opener::theOpener(true);
Opener* Opener::opener=0;

/***********************
Methods of class Opener:
***********************/

Opener::Opener(bool install)
	{
	if(install)
		{
		// DEBUGGING
		// std::cout<<"Installing IO::Opener at "<<this<<std::endl;
		
		/* Install this opener: */
		opener=this;
		
		/* While we're at it, install the current directory with IO::Directory: */
		IO::Directory::setCurrent(new StandardDirectory(""));
		}
	}

Opener::~Opener(void)
	{
	if(opener==this)
		{
		// DEBUGGING
		// std::cout<<"Uninstalling IO::Opener at "<<this<<std::endl;
		
		/* Uninstall this opener: */
		opener=0;
		}
	}

Opener* Opener::installOpener(Opener* newOpener)
	{
	/* Return the current opener: */
	Opener* result=opener;
	
	/* Install the given opener object: */
	opener=newOpener;
	
	return result;
	}

void Opener::resetOpener(void)
	{
	/* Install the static base opener: */
	opener=&theOpener;
	}

FilePtr Opener::openFile(const char* fileName,File::AccessMode accessMode)
	{
	FilePtr result;
	
	/* Open a standard file: */
	result=new StandardFile(fileName,accessMode);
	
	/* Check if the file name has the .gz extension: */
	if(Misc::hasCaseExtension(fileName,".gz"))
		{
		/* Wrap a gzip filter around the standard file: */
		result=new GzipFilter(result);
		}
	
	/* Return the open file: */
	return result;
	}

SeekableFilePtr Opener::openSeekableFile(const char* fileName,File::AccessMode accessMode)
	{
	/* Open a potentially non-seekable file first: */
	FilePtr file=openFile(fileName,accessMode);
	
	/* Check if the file is already seekable: */
	SeekableFilePtr result=file;
	if(result==0)
		{
		/* Wrap a seekable filter around the file: */
		result=new SeekableFilter(file);
		}
	
	return result;
	}

DirectoryPtr Opener::openDirectory(const char* directoryName)
	{
	/* Open a standard directory: */
	return new StandardDirectory(directoryName);
	}

DirectoryPtr Opener::openDirectory(const char* directoryNameBegin,const char* directoryNameEnd)
	{
	/* Open a standard directory: */
	return new StandardDirectory(directoryNameBegin,directoryNameEnd);
	}

DirectoryPtr Opener::openFileDirectory(const char* fileName)
	{
	/* Open a standard directory for the path name component of the file name: */
	return new StandardDirectory(fileName,Misc::getFileName(fileName));
	}

}
