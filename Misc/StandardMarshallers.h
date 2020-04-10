/***********************************************************************
StandardMarshallers - Specialized Marshaller classes for standard data
types.
Copyright (c) 2010-2020 Oliver Kreylos

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

#ifndef MISC_STANDARDMARSHALLERS_INCLUDED
#define MISC_STANDARDMARSHALLERS_INCLUDED

#include <string>
#include <Misc/SizedTypes.h>
#include <Misc/Marshaller.h>
#include <Misc/VarIntMarshaller.h>

namespace Misc {

template <>
class Marshaller<bool>
	{
	/* Methods: */
	public:
	static size_t getSize(const bool& value)
		{
		return sizeof(unsigned char);
		}
	template <class DataSinkParam>
	static void write(const bool& value,DataSinkParam& sink)
		{
		sink.template write<unsigned char>(value?1:0);
		}
	template <class DataSourceParam>
	static bool& read(DataSourceParam& source,bool& value)
		{
		value=source.template read<unsigned char>()!=0;
		return value;
		}
	template <class DataSourceParam>
	static bool read(DataSourceParam& source)
		{
		return source.template read<unsigned char>()!=0;
		}
	};

template <>
class Marshaller<unsigned char>
	{
	/* Methods: */
	public:
	static size_t getSize(const unsigned char& value)
		{
		return sizeof(unsigned char);
		}
	template <class DataSinkParam>
	static void write(const unsigned char& value,DataSinkParam& sink)
		{
		sink.template write<unsigned char>(value);
		}
	template <class DataSourceParam>
	static unsigned char& read(DataSourceParam& source,unsigned char& value)
		{
		value=source.template read<unsigned char>();
		return value;
		}
	template <class DataSourceParam>
	static unsigned char read(DataSourceParam& source)
		{
		return source.template read<unsigned char>();
		}
	};

template <>
class Marshaller<signed char>
	{
	/* Methods: */
	public:
	static size_t getSize(const signed char& value)
		{
		return sizeof(signed char);
		}
	template <class DataSinkParam>
	static void write(const signed char& value,DataSinkParam& sink)
		{
		sink.template write<signed char>(value);
		}
	template <class DataSourceParam>
	static signed char& read(DataSourceParam& source,signed char& value)
		{
		value=source.template read<signed char>();
		return value;
		}
	template <class DataSourceParam>
	static signed char read(DataSourceParam& source)
		{
		return source.template read<signed char>();
		}
	};

template <>
class Marshaller<unsigned short int>
	{
	/* Methods: */
	public:
	static size_t getSize(const unsigned short int& value)
		{
		return sizeof(unsigned short int);
		}
	template <class DataSinkParam>
	static void write(const unsigned short int& value,DataSinkParam& sink)
		{
		sink.template write<unsigned short int>(value);
		}
	template <class DataSourceParam>
	static unsigned short int& read(DataSourceParam& source,unsigned short int& value)
		{
		value=source.template read<unsigned short int>();
		return value;
		}
	template <class DataSourceParam>
	static unsigned short int read(DataSourceParam& source)
		{
		return source.template read<unsigned short int>();
		}
	};

template <>
class Marshaller<signed short int>
	{
	/* Methods: */
	public:
	static size_t getSize(const signed short int& value)
		{
		return sizeof(signed short int);
		}
	template <class DataSinkParam>
	static void write(const signed short int& value,DataSinkParam& sink)
		{
		sink.template write<signed short int>(value);
		}
	template <class DataSourceParam>
	static signed short int& read(DataSourceParam& source,signed short int& value)
		{
		value=source.template read<signed short int>();
		return value;
		}
	template <class DataSourceParam>
	static signed short int read(DataSourceParam& source)
		{
		return source.template read<signed short int>();
		}
	};

template <>
class Marshaller<unsigned int>
	{
	/* Methods: */
	public:
	static size_t getSize(const unsigned int& value)
		{
		return sizeof(unsigned int);
		}
	template <class DataSinkParam>
	static void write(const unsigned int& value,DataSinkParam& sink)
		{
		sink.template write<unsigned int>(value);
		}
	template <class DataSourceParam>
	static unsigned int& read(DataSourceParam& source,unsigned int& value)
		{
		value=source.template read<unsigned int>();
		return value;
		}
	template <class DataSourceParam>
	static unsigned int read(DataSourceParam& source)
		{
		return source.template read<unsigned int>();
		}
	};

template <>
class Marshaller<signed int>
	{
	/* Methods: */
	public:
	static size_t getSize(const signed int& value)
		{
		return sizeof(signed int);
		}
	template <class DataSinkParam>
	static void write(const signed int& value,DataSinkParam& sink)
		{
		sink.template write<signed int>(value);
		}
	template <class DataSourceParam>
	static signed int& read(DataSourceParam& source,signed int& value)
		{
		value=source.template read<signed int>();
		return value;
		}
	template <class DataSourceParam>
	static signed int read(DataSourceParam& source)
		{
		return source.template read<signed int>();
		}
	};

template <>
class Marshaller<unsigned long int>
	{
	/* Methods: */
	public:
	static size_t getSize(const unsigned long int& value)
		{
		return sizeof(unsigned long int);
		}
	template <class DataSinkParam>
	static void write(const unsigned long int& value,DataSinkParam& sink)
		{
		sink.template write<unsigned long int>(value);
		}
	template <class DataSourceParam>
	static unsigned long int& read(DataSourceParam& source,unsigned long int& value)
		{
		value=source.template read<unsigned long int>();
		return value;
		}
	template <class DataSourceParam>
	static unsigned long int read(DataSourceParam& source)
		{
		return source.template read<unsigned long int>();
		}
	};

template <>
class Marshaller<signed long int>
	{
	/* Methods: */
	public:
	static size_t getSize(const signed long int& value)
		{
		return sizeof(signed long int);
		}
	template <class DataSinkParam>
	static void write(const signed long int& value,DataSinkParam& sink)
		{
		sink.template write<signed long int>(value);
		}
	template <class DataSourceParam>
	static signed long int& read(DataSourceParam& source,signed long int& value)
		{
		value=source.template read<signed long int>();
		return value;
		}
	template <class DataSourceParam>
	static signed long int read(DataSourceParam& source)
		{
		return source.template read<signed long int>();
		}
	};

template <>
class Marshaller<float>
	{
	/* Methods: */
	public:
	static size_t getSize(const float& value)
		{
		return sizeof(float);
		}
	template <class DataSinkParam>
	static void write(const float& value,DataSinkParam& sink)
		{
		sink.template write<float>(value);
		}
	template <class DataSourceParam>
	static float& read(DataSourceParam& source,float& value)
		{
		value=source.template read<float>();
		return value;
		}
	template <class DataSourceParam>
	static float read(DataSourceParam& source)
		{
		return source.template read<float>();
		}
	};

template <>
class Marshaller<double>
	{
	/* Methods: */
	public:
	static size_t getSize(const double& value)
		{
		return sizeof(double);
		}
	template <class DataSinkParam>
	static void write(const double& value,DataSinkParam& sink)
		{
		sink.template write<double>(value);
		}
	template <class DataSourceParam>
	static double& read(DataSourceParam& source,double& value)
		{
		value=source.template read<double>();
		return value;
		}
	template <class DataSourceParam>
	static double read(DataSourceParam& source)
		{
		return source.template read<double>();
		}
	};

template <>
class Marshaller<std::string>
	{
	/* Methods: */
	public:
	static size_t getSize(const std::string& value)
		{
		/* Return the size of the variable-sized length tag plus the length of the string itself: */
		size_t length=value.length();
		return getVarInt32Size(UInt32(length))+length*sizeof(char);
		}
	template <class DataSinkParam>
	static void write(const std::string& value,DataSinkParam& sink)
		{
		/* Write the string's length as a variable-sized integer: */
		size_t length=value.length();
		writeVarInt32(UInt32(length),sink);
		
		/* Write the string's characters: */
		sink.template write<char>(value.data(),length);
		}
	template <class DataSourceParam>
	static std::string& read(DataSourceParam& source,std::string& value)
		{
		/* Clear the string: */
		value.clear();
		
		/* Read the string's length as a variable-sized integer: */
		size_t length(readVarInt32(source));
		value.reserve(length);
		
		/* Read the string's characters in chunks: */
		while(length>0)
			{
			char buffer[256];
			size_t readSize=length;
			if(readSize>sizeof(buffer))
				readSize=sizeof(buffer);
			source.template read<char>(buffer,readSize);
			value.append(buffer,buffer+readSize);
			length-=readSize;
			}
		
		return value;
		}
	template <class DataSourceParam>
	static std::string read(DataSourceParam& source)
		{
		/* Read the string's length as a variable-sized integer: */
		size_t length(readVarInt32(source));
		
		/* Read the string's characters in chunks: */
		std::string result;
		result.reserve(length);
		while(length>0)
			{
			char buffer[256];
			size_t readSize=length;
			if(readSize>sizeof(buffer))
				readSize=sizeof(buffer);
			source.template read<char>(buffer,readSize);
			result.append(buffer,buffer+readSize);
			length-=readSize;
			}
		
		return result;
		}
	};

}

#endif
