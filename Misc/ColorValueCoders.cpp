/***********************************************************************
ColorValueCoders - Generic value coder classes for color types.
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

#include <Misc/ColorValueCoders.h>

#include <stdexcept>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>

namespace Misc {

/**********************************************
Methods of class ValueCoder<RGB<ScalarParam> >:
**********************************************/

template <class ScalarParam>
std::string ValueCoder<RGB<ScalarParam> >::encode(const RGB<ScalarParam>& value)
	{
	/* Convert color scalar type to double: */
	RGB<double> dv(value);
	
	/* Return the encoded vector: */
	return CFixedArrayValueCoder<double,3>::encode(dv.getComponents());
	}

template <class ScalarParam>
RGB<ScalarParam> ValueCoder<RGB<ScalarParam> >::decode(const char* start,const char* end,const char** decodeEnd)
	{
	try
		{
		/* Decode string into array of doubles: */
		double components[3];
		CFixedArrayValueCoder<double,3>(components).decode(start,end,decodeEnd);
		
		/* Return result color: */
		return RGB<ScalarParam>(components[0],components[1],components[2]);
		}
	catch(const std::runtime_error& err)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to RGB due to ")+err.what());
		}
	}

/***********************************************
Methods of class ValueCoder<RGBA<ScalarParam> >:
***********************************************/

template <class ScalarParam>
std::string ValueCoder<RGBA<ScalarParam> >::encode(const RGBA<ScalarParam>& value)
	{
	/* Convert color scalar type to double: */
	RGBA<double> dv(value);
	
	/* Only encode three components if alpha is default value: */
	if(dv[3]==1.0)
		return CFixedArrayValueCoder<double,3>::encode(dv.getComponents());
	else
		return CFixedArrayValueCoder<double,4>::encode(dv.getComponents());
	}

template <class ScalarParam>
RGBA<ScalarParam> ValueCoder<RGBA<ScalarParam> >::decode(const char* start,const char* end,const char** decodeEnd)
	{
	try
		{
		/* Decode string into array of doubles: */
		double components[4];
		DynamicArrayValueCoder<double> decoder(components,4);
		decoder.decode(start,end,decodeEnd);
		
		/* Check for correct vector size: */
		if(decoder.numElements<3||decoder.numElements>4)
			throw DecodingError("wrong number of components");
		
		/* Set default alpha value for three-component colors: */
		if(decoder.numElements==3)
			components[3]=1.0;
		
		/* Return result color: */
		return RGBA<ScalarParam>(components[0],components[1],components[2],components[3]);
		}
	catch(const std::runtime_error& err)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to RGBA due to ")+err.what());
		}
	}

/**********************************************
Force instantiation of all value coder classes:
**********************************************/

template class ValueCoder<RGB<SInt8> >;
template class ValueCoder<RGB<UInt8> >;
template class ValueCoder<RGB<SInt16> >;
template class ValueCoder<RGB<UInt16> >;
template class ValueCoder<RGB<SInt32> >;
template class ValueCoder<RGB<UInt32> >;
template class ValueCoder<RGB<Float32> >;
template class ValueCoder<RGB<Float64> >;
template class ValueCoder<RGBA<SInt8> >;
template class ValueCoder<RGBA<UInt8> >;
template class ValueCoder<RGBA<SInt16> >;
template class ValueCoder<RGBA<UInt16> >;
template class ValueCoder<RGBA<SInt32> >;
template class ValueCoder<RGBA<UInt32> >;
template class ValueCoder<RGBA<Float32> >;
template class ValueCoder<RGBA<Float64> >;

}
