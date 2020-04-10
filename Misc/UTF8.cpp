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

#include <Misc/UTF8.h>

#include <stdexcept>

namespace Misc {

/*********************
Methods of class UTF8:
*********************/

bool UTF8::isValid(std::string::const_iterator begin,std::string::const_iterator end)
	{
	for(std::string::const_iterator it=begin;it!=end;)
		{
		unsigned int c=(unsigned char)(*(it++));
		
		/* Check if this is a multi-byte encoding: */
		if(c>=0x80U)
			{
			/* Determine the length of the encoding: */
			unsigned int numContinuationBytes;
			if(c<0xc0U) // Byte starts with 10, misplaced continuation character
				return false;
			else if(c<0xe0U) // Byte starts with 110, 2-byte sequence
				numContinuationBytes=1;
			else if(c<0xf0U) // Byte starts with 1110, 3-byte sequence
				numContinuationBytes=2;
			else if(c<0xf8U) // Byte starts with 11110, 4-byte sequence
				numContinuationBytes=3;
			else // Invalid code byte
				return false;
			
			/* Skip the continuation characters: */
			while(numContinuationBytes>0)
				{
				if(it==end)
					return false;
				++it;
				--numContinuationBytes;
				}
			}
		}
	
	return true;
	}

unsigned int UTF8::decodeFirst(unsigned char code[4])
	{
	/* Check if this is a multi-byte encoding: */
	if(code[0]<0x80U)
		return 0;
	else if(code[0]<0xc0U)
		throw std::runtime_error("Misc::UTF8::decodeFirst: Synchronization lost");
	else if(code[0]<0xe0U)
		{
		code[0]&=0x3f;
		return 1;
		}
	else if(code[0]<0xf0U)
		{
		code[0]&=0x1f;
		return 2;
		}
	else if(code[0]<0xf8U)
		{
		code[0]&=0x0f;
		return 3;
		}
	else
		throw std::runtime_error("Misc::UTF8::decodeFirst: Invalid code byte");
	}

unsigned int UTF8::decodeRest(unsigned char code[4],unsigned int numContinuationBytes)
	{
	unsigned int result=code[0];
	for(unsigned int i=1;i<=numContinuationBytes;++i)
		{
		if((code[i]&0xc0U)!=0x80U)
			throw std::runtime_error("Misc::UTF8::decodeRest: Invalid code byte");
		result=(result<<6)|((unsigned int)(code[i])&0x3fU);
		}
	return result;
	}

unsigned int UTF8::decode(std::string::const_iterator& begin,std::string::const_iterator end)
	{
	/* Read the first byte: */
	unsigned int result=(unsigned char)(*(begin++));
	
	/* Check if there are additional bytes encoding the current character: */
	if(result>=0x80U)
		{
		/* Calculate the number of additional bytes to read: */
		unsigned int numContinuationBytes;
		if(result<0xc0U) // Byte starts with 10, misplaced continuation character
			throw std::runtime_error("Misc::UTF8::decode: Synchronization lost");
		else if(result<0xe0U) // Byte starts with 110, 2-byte sequence
			{
			result&=0x3fU;
			numContinuationBytes=1;
			}
		else if(result<0xf0U) // Byte starts with 1110, 3-byte sequence
			{
			result&=0x1fU;
			numContinuationBytes=2;
			}
		else if(result<0xf8U) // Byte starts with 11110, 4-byte sequence
			{
			result&=0x0fU;
			numContinuationBytes=3;
			}
		else // Invalid code byte
			throw std::runtime_error("Misc::UTF8::decode: Invalid code byte");
		
		/* Read the continuation bytes: */
		while(numContinuationBytes>0)
			{
			/* Check for end-of-string: */
			if(begin==end)
				throw std::runtime_error("Misc::UTF8::decode: Truncated character");
				
			/* Read the next byte: */
			unsigned int byte=(unsigned char)(*(begin++));
			
			/* Check for a continuation byte: */
			if((byte&0xc0U)!=0x80U)
				throw std::runtime_error("Misc::UTF8::decode: Invalid code byte");
			
			/* Append the byte to the character code: */
			result=(result<<6)|(byte&0x3fU);
			
			--numContinuationBytes;
			}
		}
	
	return result;
	}

unsigned int UTF8::encode(unsigned int c,unsigned char code[4])
	{
	unsigned int numBytes;
	if(c<0x80U) // 7 significant bits
		{
		/* Encode as single byte: */
		code[0]=(unsigned char)(c); // Encode all 7 bits
		numBytes=1;
		}
	else
		{
		/* Determine the length and prefix of the encoded character: */
		unsigned int numBytes,prefix;
		if(c<0x800U) // 11 significant bits
			{
			/* Encode as two bytes: */
			numBytes=2;
			prefix=0xc0U;
			}
		else if(c<0x10000U) // 16 significant bits
			{
			/* Encode as three bytes: */
			numBytes=3;
			prefix=0xe0U;
			}
		else if(c<0x200000U) // 21 significant bits
			{
			/* Encode as four bytes: */
			numBytes=4;
			prefix=0xf0U;
			}
		else
			throw std::runtime_error("Misc::UTF8::encode: Invalid character code");
		
		/* Encode the character: */
		for(unsigned int byte=numBytes-1;byte>0;--byte)
			code[byte]=(unsigned char)((c&0x3fU)|0x80U); // Encode next 6 bits
		code[0]=(unsigned char)(c|prefix); // Encode remaining bits and length prefix
		}
	
	return numBytes;
	}

}
