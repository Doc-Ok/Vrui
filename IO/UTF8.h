/***********************************************************************
UTF8 - Helper class to encode/decode Unicode characters to/from UTF-8.
Copyright (c) 2018-2020 Oliver Kreylos

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

#ifndef IO_UTF8_INCLUDED
#define IO_UTF8_INCLUDED

#include <Misc/UTF8.h>
#include <IO/File.h>

namespace IO {

class UTF8
	{
	/* Methods: */
	public:
	static int read(File& source) // Reads the next complete Unicode character from the given UTF-8 encoded file
		{
		/* Read the first byte and check for end-of-file: */
		int result=source.getChar();
		if(result<0)
			return result;
		
		/* Decode the first byte: */
		unsigned char code[4];
		code[0]=(unsigned char)(result);
		unsigned int numContinuationBytes=Misc::UTF8::decodeFirst(code);
		
		/* Check if there are continuation bytes: */
		if(numContinuationBytes==0)
			return code[0];
		else
			{
			/* Read and decode the remaining bytes: */
			source.readRaw(code+1,numContinuationBytes);
			return Misc::UTF8::decodeRest(code,numContinuationBytes);
			}
		}
	static void write(unsigned int c,File& dest) // Encodes the given Unicode character into UTF-8 and writes the encoding to the given file
		{
		/* Encode the character into a buffer: */
		unsigned char code[4];
		unsigned int numBytes=Misc::UTF8::encode(c,code);
		
		/* Write the buffer to the file: */
		dest.writeRaw(code,numBytes);
		}
	};

}

#endif
