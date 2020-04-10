/***********************************************************************
UTF8 - Helper class to encode/decode Unicode characters to/from UTF-8.
Copyright (c) 2018-2020 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_UTF8_INCLUDED
#define MISC_UTF8_INCLUDED

#include <string>

namespace Misc {

class UTF8
	{
	/* Methods: */
	public:
	static bool isValid(std::string::const_iterator begin,std::string::const_iterator end); // Returns true if the given string section is a valid UTF-8 encoding
	static unsigned int decodeFirst(unsigned char code[4]); // Decodes the first byte of a UTF-8 code sequence in the given buffer; returns number of remaining bytes to read
	static unsigned int decodeRest(unsigned char code[4],unsigned int numContinuationBytes); // Decodes the UTF-8 code sequence in the given buffer with the given number of continuation bytes
	static unsigned int decode(std::string::const_iterator& begin,std::string::const_iterator end); // Reads the next complete Unicode character from the given UTF-8 encoded string
	static unsigned int decodeNoCheck(std::string::const_iterator& begin,std::string::const_iterator end) // Reads the next complete Unicode character from the given UTF-8 encoded string, assuming the string is a valid UTF-8 encoding
		{
		/* Read the first byte: */
		unsigned int result=(unsigned char)(*(begin++));
		
		/* Check if there are additional bytes encoding the current character: */
		if(result>=0x80U)
			{
			/* Calculate the number of additional bytes to read: */
			unsigned int numContinuationBytes;
			if(result<0xe0U) // Byte starts with 110, 2-byte sequence
				{
				result&=0x3fU;
				numContinuationBytes=1;
				}
			else if(result<0xf0U) // Byte starts with 1110, 3-byte sequence
				{
				result&=0x1fU;
				numContinuationBytes=2;
				}
			else // Byte starts with 11110, 4-byte sequence
				{
				result&=0x0fU;
				numContinuationBytes=3;
				}
			
			/* Read the continuation bytes: */
			while(numContinuationBytes>0)
				{
				/* Append the next byte to the character code: */
				unsigned int byte=(unsigned char)(*(begin++));
				result=(result<<6)|(byte&0x3fU);
				
				--numContinuationBytes;
				}
			}
		
		return result;
		}
	static unsigned int encode(unsigned int c,unsigned char code[4]); // Encodes the given Unicode character into the given buffer in UTF-8 and returns the code length in bytes
	static void encode(unsigned int c,std::string& string) // Encodes the given Unicode character into UTF-8 and appends the encoding to the given string
		{
		/* Encode the given character into a buffer and determine the encoding length: */
		unsigned char code[4];
		unsigned int numBytes=encode(c,code);
		
		/* Append the encoded buffer to the string: */
		string.append(code,code+numBytes);
		}
	};

}

#endif
