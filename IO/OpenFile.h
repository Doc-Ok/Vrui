/***********************************************************************
OpenFile - Convenience functions to open files of several types using
the File abstraction.
Copyright (c) 2011-2018 Oliver Kreylos

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

#ifndef IO_OPENFILE_INCLUDED
#define IO_OPENFILE_INCLUDED

#include <IO/Opener.h>

namespace IO {

inline FilePtr openFile(const char* fileName,File::AccessMode accessMode =File::ReadOnly) // Opens a file of the given name
	{
	return Opener::getOpener()->openFile(fileName,accessMode);
	}

inline SeekableFilePtr openSeekableFile(const char* fileName,File::AccessMode accessMode =File::ReadOnly) // Opens a seekable file of the given name
	{
	return Opener::getOpener()->openSeekableFile(fileName,accessMode);
	}

inline DirectoryPtr openDirectory(const char* directoryName) // Opens a directory of the given name
	{
	return Opener::getOpener()->openDirectory(directoryName);
	}

inline DirectoryPtr openFileDirectory(const char* fileName) // Opens the directory containing the given file or directory
	{
	return Opener::getOpener()->openFileDirectory(fileName);
	}

}

#endif
