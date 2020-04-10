/***********************************************************************
VarIntMarshaller - Helper functions to serialize/unserialize unsigned
integers to/from storage classes that support typed (templatized) read
and write methods using a variable-sized wire representation.
Copyright (c) 2020 Oliver Kreylos

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

#ifndef MISC_VARINTMARSHALLER_INCLUDED
#define MISC_VARINTMARSHALLER_INCLUDED

#include <stddef.h>
#include <stdexcept>
#include <Misc/SizedTypes.h>

namespace Misc {

inline size_t getVarInt32Size(UInt32 value) // Returns the number of bytes needed to represent the given value
	{
	if(value<(1U<<14))
		{
		if(value<(1U<<7))
			return 1;
		else // value>=(1U<<7)
			return 2;
		}
	else // value>=(1U<<14)
		{
		if(value<(1U<<21))
			return 3;
		else if(value<(1U<<28))
			return 4;
		else // value>=1U<<28
			return 5;
		}
	}

template <class DataSinkParam>
inline
size_t
writeVarInt32(
	UInt32 value,
	DataSinkParam& sink) // Writes the given 32-bit unsigned integer value to the given binary sink as 1-5 bytes; returns the number of bytes written
	{
	/* Determine the number of required bytes and the serialization length indicator prefix: */
	size_t numBytes;
	UInt8 prefix;
	if(value<(1U<<14))
		{
		if(value<(1U<<7))
			{
			/* Write a single byte: */
			numBytes=1;
			prefix=UInt8(0x00U);
			}
		else // value>=(1U<<7)
			{
			/* Write a 2-byte sequence: */
			numBytes=2;
			prefix=UInt8(0x80U);
			}
		}
	else // value>=(1U<<14)
		{
		if(value<(1U<<21))
			{
			/* Write a 3-byte sequence: */
			numBytes=3;
			prefix=UInt8(0xc0U);
			}
		else if(value<(1U<<28))
			{
			/* Write a 4-byte sequence: */
			numBytes=4;
			prefix=UInt8(0xe0U);
			}
		else // value>=1U<<28
			{
			/* Write a 5-byte sequence: */
			numBytes=5;
			prefix=UInt8(0xf0U);
			}
		}
	
	/* Generate the serialization byte sequence and write it to the sink: */
	UInt8 seq[5];
	for(size_t i=numBytes-1;i>0;--i,value>>=8)
		seq[i]=UInt8(value&0xffU);
	seq[0]=prefix|UInt8(value);
	sink.template write(seq,numBytes);
	
	return numBytes;
	}

template <class DataSourceParam>
inline
size_t
readVarInt32First(
	DataSourceParam& source,
	UInt32& value) // Reads the first of 1-5 bytes from the given binary source into the given 32-bit unsigned integer value; returns the number of bytes remaining to be read
	{
	/* Read the serialization's first byte: */
	value=source.template read<UInt8>();
	
	/* Determine the total number of bytes in the serialization: */
	size_t result=0;
	if(value<0xe0U)
		{
		if(value<0x80U)
			{
			/* Read one byte total: */
			result=0;
			}
		else if(value<0xc0U)
			{
			/* Mask out the length prefix and read two bytes total: */
			value&=0x3fU;
			result=1;
			}
		else // value>=0xc0U
			{
			/* Mask out the length prefix and read three bytes total: */
			value&=0x1fU;
			result=2;
			}
		}
	else // value>=0xe0U
		{
		if(value<0xf0U)
			{
			/* Mask out the length prefix and read four bytes total: */
			value&=0x0fU;
			result=3;
			}
		else if(value<0xf8U)
			{
			/* Mask out the length prefix and read five bytes total: */
			value&=0x07U;
			result=4;
			}
		else
			throw std::runtime_error("Misc::readVarInt32First: Invalid serialization");
		}
	
	return result;
	}

template <class DataSourceParam>
inline
void
readVarInt32Remaining(
	DataSourceParam& source,
	size_t numRemainingBytes,
	UInt32& value) // Reads the remaining of 1-5 bytes from the given binary source into the given 32-bit unsigned integer value; returns the total number of bytes to read
	{
	/* Read the remaining serialization byte sequence: */
	UInt8 seq[4];
	source.read(seq,numRemainingBytes);
	for(size_t i=0;i<numRemainingBytes;++i)
		value=(value<<8)|UInt32(seq[i]);
	}

template <class DataSourceParam>
inline
size_t
readVarInt32(
	DataSourceParam& source,
	UInt32& value) // Reads 1-5 bytes from the given binary source into the given 32-bit unsigned integer value; returns the number of bytes read
	{
	/* Read the serialization's first byte: */
	value=source.template read<UInt8>();
	
	/* Check if there are additional bytes to read: */
	if(value>=0x80U)
		{
		size_t numAdditionalBytes;
		if(value<0xe0U)
			{
			if(value<0xc0U)
				{
				/* Mask out the length prefix and read one additional byte: */
				value&=0x3fU;
				numAdditionalBytes=1;
				}
			else
				{
				/* Mask out the length prefix and read two additional bytes: */
				value&=0x1fU;
				numAdditionalBytes=2;
				}
			}
		else // value>=0xe0U
			{
			if(value<0xf0U)
				{
				/* Mask out the length prefix and read three additional bytes: */
				value&=0x0fU;
				numAdditionalBytes=3;
				}
			else if(value<0xf8U)
				{
				/* Mask out the length prefix and read four additional bytes: */
				value&=0x07U;
				numAdditionalBytes=4;
				}
			else
				throw std::runtime_error("Misc::readVarInt32: Invalid serialization");
			}
		
		/* Read the remaining serialization byte sequence: */
		UInt8 seq[4];
		source.read(seq,numAdditionalBytes);
		for(size_t i=0;i<numAdditionalBytes;++i)
			value=(value<<8)|UInt32(seq[i]);
		
		return numAdditionalBytes+1;
		}
	else
		return 1;
	}

template <class DataSourceParam>
inline
UInt32
readVarInt32(
	DataSourceParam& source) // Reads 1-5 bytes from the given binary source and returns a 32-bit unsigned integer value
	{
	UInt32 result;
	readVarInt32(source,result);
	return result;
	}

}

#endif
