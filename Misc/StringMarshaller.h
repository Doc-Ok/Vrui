/***********************************************************************
StringMarshaller - Helper functions to serialize/unserialize C and C++
standard strings to/from storage classes that support typed
(templatized) read and write methods.
Copyright (c) 2008-2020 Oliver Kreylos

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

#ifndef MISC_STRINGMARSHALLER_INCLUDED
#define MISC_STRINGMARSHALLER_INCLUDED

#include <string.h>
#include <string>
#include <Misc/SizedTypes.h>
#include <Misc/VarIntMarshaller.h>

namespace Misc {

template <class PipeParam>
inline
void
writeCString(
	const char* string,
	PipeParam& pipe) // Writes a C string to a pipe
	{
	/* Write the string's length as a variable-sized integer: */
	size_t length=strlen(string);
	writeVarInt32(UInt32(length),pipe);
	
	/* Write the string without its terminating NUL character: */
	pipe.template write<char>(string,length);
	}

template <class PipeParam>
inline
void
writeCppString(
	const std::string& string,
	PipeParam& pipe) // Writes a C++ string to a pipe
	{
	/* Write the string's length as a variable-sized integer: */
	size_t length=string.length();
	writeVarInt32(UInt32(length),pipe);
	
	/* Write the string's characters: */
	pipe.template write<char>(string.data(),length);
	}

template <class PipeParam>
inline
char*
readCString(
	PipeParam& pipe) // Reads a C string from a pipe; returns a new[]-allocated character array
	{
	/* Read the string's length as a variable-sized integer: */
	size_t length(readVarInt32(pipe));
	
	/* Allocate the string's character array and read the characters: */
	char* result=new char[length+1];
	pipe.template read<char>(result,length);
	
	/* NUL-terminate and return the string: */
	result[length]='\0';
	return result;
	}

template <class PipeParam>
inline
std::string
readCppString(
	PipeParam& pipe) // Reads a C++ string from a pipe
	{
	/* Read the string's length as a variable-sized integer: */
	size_t length(readVarInt32(pipe));
	
	/* Read the string in chunks (unfortunately, there is no API to read directly into the std::string): */
	std::string result;
	result.reserve(length);
	while(length>0)
		{
		char buffer[256];
		size_t readLength=length;
		if(readLength>sizeof(buffer))
			readLength=sizeof(buffer);
		pipe.template read<char>(buffer,readLength);
		result.append(buffer,readLength);
		length-=readLength;
		}
	
	/* Return the string: */
	return result;
	}

template <class PipeParam>
inline
std::string&
readCppString(
	PipeParam& pipe,
	std::string& string) // Reads a C++ string from a pipe into an existing string object
	{
	/* Clear the string: */
	string.clear();
	
	/* Read the string's length as a variable-sized integer: */
	size_t length(readVarInt32(pipe));
	
	/* Read the string in chunks (unfortunately, there is no API to read directly into the std::string): */
	string.reserve(length);
	while(length>0)
		{
		char buffer[256];
		size_t readLength=length;
		if(readLength>sizeof(buffer))
			readLength=sizeof(buffer);
		pipe.template read<char>(buffer,readLength);
		string.append(buffer,readLength);
		length-=readLength;
		}
	
	return string;
	}

}

#endif
