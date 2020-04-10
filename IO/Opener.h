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

#ifndef IO_OPENER_INCLUDED
#define IO_OPENER_INCLUDED

#include <IO/File.h>
#include <IO/SeekableFile.h>
#include <IO/Directory.h>

namespace IO {

class Opener
	{
	/* Elements: */
	private:
	static Opener theOpener; // Static opener object active unless another one is installed
	static Opener* opener; // Pointer to the active opener
	
	/* Constructors and destructors: */
	public:
	Opener(bool install); // Creates and optionally installs the opener
	virtual ~Opener(void); // Virtual destructor to support derived classes with state
	
	/* Methods: */
	static Opener* getOpener(void) // Returns the currently installed opener
		{
		return opener;
		}
	static Opener* installOpener(Opener* newOpener); // Installs the given opener as the current opener; returns previous opener
	static void resetOpener(void); // Installs the basic opener as the current opener
	
	/* File opening methods: */
	virtual FilePtr openFile(const char* fileName,File::AccessMode accessMode); // Opens a file of the given name
	virtual SeekableFilePtr openSeekableFile(const char* fileName,File::AccessMode accessMode); // Opens a seekable file of the given name
	virtual DirectoryPtr openDirectory(const char* directoryName); // Opens a directory of the given name
	virtual DirectoryPtr openDirectory(const char* directoryNameBegin,const char* directoryNameEnd); // Ditto, with directory name delimited by beginning and end pointers
	virtual DirectoryPtr openFileDirectory(const char* fileName); // Opens the directory containing the given file or directory
	};

}

#endif
