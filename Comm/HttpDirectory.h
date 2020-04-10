/***********************************************************************
HttpDirectory - Class to access remote directories and files over the
HTTP/1.1 protocol.
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

#ifndef COMM_HTTPDIRECTORY_INCLUDED
#define COMM_HTTPDIRECTORY_INCLUDED

#include <string>
#include <IO/Directory.h>
#include <Comm/HttpFile.h>

namespace Comm {

class HttpDirectory:public IO::Directory
	{
	/* Elements: */
	private:
	std::string url; // Fully-qualified URL of this directory
	unsigned int prefixLength; // Length of non-directory prefix of the URL
	
	/* Private methods: */
	private:
	void init(HttpFile::URLParts& urlParts); // Initializes the object with the given URL parts
	
	/* Constructors and destructors: */
	public:
	HttpDirectory(const char* sUrl); // Opens the directory of the given absolute but not necessarily normalized URL
	HttpDirectory(const char* sUrlBegin,const char* sUrlEnd); // Ditto, with URL delimited by beginning and end pointers
	private:
	HttpDirectory(const char* sUrl,unsigned int sPrefixLength); // Ditto; but assumes that given URL is absolute and normalized
	public:
	virtual ~HttpDirectory(void);
	
	/* Methods from Directory: */
	virtual std::string getName(void) const;
	virtual std::string getPath(void) const;
	virtual std::string getPath(const char* relativePath) const;
	virtual bool hasParent(void) const;
	virtual IO::DirectoryPtr getParent(void) const;
	virtual void rewind(void);
	virtual bool readNextEntry(void);
	virtual const char* getEntryName(void) const;
	virtual Misc::PathType getEntryType(void) const;
	virtual Misc::PathType getPathType(const char* relativePath) const;
	virtual IO::FilePtr openFile(const char* fileName,IO::File::AccessMode accessMode =IO::File::ReadOnly) const;
	virtual IO::DirectoryPtr openDirectory(const char* directoryName) const;
	};

}

#endif
