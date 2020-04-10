/***********************************************************************
Base64Filter - Class for read/write access to base64-encoded files using
a IO::File abstraction.
Copyright (c) 2019 Oliver Kreylos

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

#ifndef IO_BASE64FILTER_INCLUDED
#define IO_BASE64FILTER_INCLUDED

#include <Misc/SizedTypes.h>
#include <IO/File.h>

namespace IO {

class Base64Filter:public IO::File
	{
	/* Elements: */
	private:
	FilePtr encodedFile; // Underlying base64-encoded file
	Misc::UInt32 decodeBuffer; // Bit buffer for decoding data from the underlying encoded file
	int decodeBufferBits; // Number of bits currently in the decode buffer
	bool readEof; // Flag if the underlying file has been completely read, i.e., EOF or a non-base64 character was encountered
	Misc::UInt32 encodeBuffer; // Bit buffer for encoding data to the underlying encoded file
	int encodeBufferBits; // Number of bits currently in the encode buffer
	
	/* Methods from File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	virtual size_t writeDataUpTo(const Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	Base64Filter(FilePtr sEncodedFile); // Creates a base64 filter for the given underlying base64-encoded file; inherits access mode from encoded file
	virtual ~Base64Filter(void); // Destroys the gzip filter
	
	/* Methods from File: */
	virtual int getFd(void) const;
	};

}

#endif
