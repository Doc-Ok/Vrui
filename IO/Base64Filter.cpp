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

#include <IO/Base64Filter.h>

namespace IO {

namespace {

/****************
Helper functions:
****************/

inline Misc::UInt32 decode(int c)
	{
	Misc::UInt32 result(255U);
	
	if(c>='A')
		{
		if(c>='a')
			{
			if(c<='z')
				result=Misc::UInt32(c-'a'+26);
			}
		else // c<'a'
			{
			if(c<='Z')
				result=Misc::UInt32(c-'A');
			}
		}
	else // c<'A'
		{
		if(c>='0')
			{
			if(c<='9')
				result=Misc::UInt32(c-'0'+52);
			}
		else // c<'0'
			{
			if(c=='+')
				result=Misc::UInt32(62);
			else if(c=='/')
				result=Misc::UInt32(63);
			}
		}
	
	return result;
	}

inline Misc::UInt8 encode(Misc::UInt32 bits)
	{
	if(bits>=52U)
		{
		if(bits>=62U)
			{
			if(bits==63U)
				return Misc::UInt8('/');
			else
				return Misc::UInt8('+');
			}
		else // bits<62U
			return Misc::UInt8('0'+bits-52);
		}
	else // bits<52U
		{
		if(bits>=26U)
			return Misc::UInt8('a'+bits-26);
		else // bits<26U
			return Misc::UInt8('A'+bits);
		}
	}

}

/*****************************
Methods of class Base64Filter:
*****************************/

size_t Base64Filter::readData(File::Byte* buffer,size_t bufferSize)
	{
	/* Check for end-of-file: */
	if(readEof)
		return 0;
	
	/* Decode a full buffer's worth of data from the source: */
	Byte* bufPtr=buffer;
	Byte* bufferEnd=buffer+bufferSize;
	while(bufPtr!=bufferEnd)
		{
		/* Read the next character, which might be EOF, and attempt to decode it: */
		int c=encodedFile->getChar();
		Misc::UInt32 bits=decode(c);
		
		/* If the character was EOF or not a valid base64 encoding, it's over: */
		if(bits>=64U)
			{
			/* If the invalid character wasn't EOF, stuff it back into the encoded file: */
			if(c>=0)
				encodedFile->ungetChar(c);
			
			/* Remember that decoding is over: */
			readEof=true;
			break;
			}
		
		/* Stuff the decoded character into the bit buffer: */
		decodeBuffer=(decodeBuffer<<6)|bits;
		decodeBufferBits+=6;
		
		/* Check if there's another byte ready: */
		if(decodeBufferBits>=8)
			{
			/* Extract one byte from the bit buffer: */
			*bufPtr=Byte(decodeBuffer>>(decodeBufferBits-8));
			++bufPtr;
			decodeBufferBits-=8;
			}
		}
	
	/* Return the amount of read data: */
	return bufPtr-buffer;
	}

void Base64Filter::writeData(const File::Byte* buffer,size_t bufferSize)
	{
	/* We must completely clear out the write buffer: */
	const Byte* bufPtr=buffer;
	const Byte* bufferEnd=buffer+bufferSize;
	while(bufPtr!=bufferEnd)
		{
		/* Write directly into the encoded file's write buffer: */
		void* voidOutputBuffer;
		size_t outputSize=encodedFile->writeInBufferPrepare(voidOutputBuffer);
		Byte* outputBuffer=static_cast<Byte*>(voidOutputBuffer);
		Byte* outPtr=outputBuffer;
		Byte* outputBufferEnd=outputBuffer+outputSize;
		
		/* Encode data until the write buffer is empty, or the encoded file's write buffer is full: */
		while(bufPtr!=bufferEnd&&outPtr!=outputBufferEnd)
			{
			/* Check if the bit buffer needs more bits to produce an encoded byte: */
			if(encodeBufferBits<6)
				{
				/* Stuff the next write buffer byte into the bit buffer: */
				encodeBuffer=(encodeBuffer<<8)|Misc::UInt32(*bufPtr);
				++bufPtr;
				encodeBufferBits+=8;
				}
			
			/* Extract six bits from the bit buffer, encode, and write them: */
			*outPtr=encode(encodeBuffer>>(encodeBufferBits-6));
			++outPtr;
			encodeBufferBits-=6;
			}
		
		/* Finish writing into the encoded file's write buffer: */
		encodedFile->writeInBufferFinish(outPtr-outputBuffer);
		}
	}

size_t Base64Filter::writeDataUpTo(const File::Byte* buffer,size_t bufferSize)
	{
	/* Calculate the encoded file's write buffer fill ratio: */
	size_t encodedBufferSpace=encodedFile->getWriteBufferSpace();
	
	/* Write to the encoded file first if its write buffer is at least half full: */
	bool writeFirst=encodedBufferSpace*2>=encodedFile->getWriteBufferSize();
	if(writeFirst)
		encodedFile->writeSomeData();
	
	/* Write directly into the encoded file's write buffer: */
	void* voidOutputBuffer;
	size_t outputSize=encodedFile->writeInBufferPrepare(voidOutputBuffer);
	Byte* outputBuffer=static_cast<Byte*>(voidOutputBuffer);
	Byte* outPtr=outputBuffer;
	Byte* outputBufferEnd=outputBuffer+outputSize;
	
	/* Encode data until the write buffer is empty, or the encoded file's write buffer is full: */
	const Byte* bufPtr=buffer;
	const Byte* bufferEnd=buffer+bufferSize;
	while(bufPtr!=bufferEnd&&outPtr!=outputBufferEnd)
		{
		/* Check if the bit buffer needs more bits to produce an encoded byte: */
		if(encodeBufferBits<6)
			{
			/* Stuff the next write buffer byte into the bit buffer: */
			encodeBuffer=(encodeBuffer<<8)|Misc::UInt32(*bufPtr);
			++bufPtr;
			encodeBufferBits+=8;
			}
		
		/* Extract six bits from the bit buffer, encode, and write them: */
		*outPtr=encode(encodeBuffer>>(encodeBufferBits-6));
		++outPtr;
		encodeBufferBits-=6;
		}
	
	/* Finish writing into the encoded file's write buffer: */
	encodedFile->writeInBufferFinish(outPtr-outputBuffer);
	
	/* Write second if the encoded file's write buffer was less than half full: */
	if(!writeFirst)
		encodedFile->writeSomeData();
	
	/* Return the number of bytes that were written: */
	return bufPtr-buffer;
	}

Base64Filter::Base64Filter(FilePtr sEncodedFile)
	:File(),
	 encodedFile(sEncodedFile),
	 decodeBuffer(0x0U),decodeBufferBits(0),readEof(false),
	 encodeBuffer(0x0U),encodeBufferBits(0)
	{
	/* Check if the encoded file is opened for reading: */
	if(encodedFile->getReadBufferSize()!=0)
		{
		/* Install an output buffer for decoded data that's large enough to decode a full encoded file's output buffer: */
		size_t readBufferSize=(encodedFile->getReadBufferSize()*6+7)/8; // 8 bits of encoded data produce 6 bits of decoded data, plus at most 7 bits from decode buffer
		resizeReadBuffer(readBufferSize);
		}
	
	/* Check if the encoded file is opened for writing: */
	if(encodedFile->getWriteBufferSize()!=0)
		{
		/* Install an input buffer for unencoded data that will fill, but not overflow, the encoded file's input buffer: */
		size_t writeBufferSize=(encodedFile->getWriteBufferSize()*6-5)/8; // 6 bits of unencoded data produce 8 bits of encoded data, plus at most 5 bits from encode buffer
		if(writeBufferSize==0)
			throw OpenError("IO::Base64Filter: Encoded file's write buffer too small to hold encoded data");
		resizeWriteBuffer(writeBufferSize);
		}
	}

Base64Filter::~Base64Filter(void)
	{
	if(getWriteBufferSize()!=0)
		{
		/* Flush the write buffer: */
		flush();
		
		/* Check if there are remaining bits in the bit buffer: */
		if(encodeBufferBits!=0)
			{
			/* Pad the bit buffer to six bits, then encode and write those bits: */
			encodedFile->putChar(encode(encodeBuffer<<(6-encodeBufferBits)));
			}
		}
	}

int Base64Filter::getFd(void) const
	{
	/* Return the encoded file's file descriptor: */
	return encodedFile->getFd();
	}

}
