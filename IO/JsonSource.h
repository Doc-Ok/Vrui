/***********************************************************************
JsonSource - Class to retrieve JSON entities from JSON files.
Copyright (c) 2018-2019 Oliver Kreylos

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

#ifndef IO_JSONSOURCE_INCLUDED
#define IO_JSONSOURCE_INCLUDED

#include <IO/File.h>
#include <IO/ValueSource.h>
#include <IO/JsonEntity.h>

namespace IO {

class JsonSource
	{
	/* Elements: */
	private:
	IO::ValueSource file; // The underlying JSON file
	
	/* Constructors and destructors: */
	public:
	JsonSource(const char* fileName); // Opens the JSON file of the given name
	JsonSource(FilePtr sFile); // Creates a JSON source for the given file 
	
	/* Methods: */
	bool eof(void) const // Returns true if the JSON source has been completely parsed
		{
		return file.eof();
		}
	JsonPointer parseEntity(void); // Parses the next entity from the JSON file; throws exception at end of file or on syntax error
	};

}

#endif
