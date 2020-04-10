/***********************************************************************
ConvertColorComponent - Namespace-global template function to convert
color components between different scalar types.
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

#ifndef MISC_CONVERTCOLORCOMPONENT_INCLUDED
#define MISC_CONVERTCOLORCOMPONENT_INCLUDED

#include <Misc/SizedTypes.h>

namespace Misc {

/********************************************
Templatized color scalar conversion function:
********************************************/

template <class DestScalarParam,class SourceScalarParam>
DestScalarParam convertColorComponent(SourceScalarParam value)
	{
	}

/******************************************
Conversion functions from 8-bit signed int:
******************************************/

template <>
inline SInt8 convertColorComponent(SInt8 value)
	{
	return value;
	}

template <>
inline UInt8 convertColorComponent(SInt8 value)
	{
	return (UInt8(value)<<1)|(UInt8(value)>>6);
	}

template <>
inline SInt16 convertColorComponent(SInt8 value)
	{
	return (SInt16(value)*SInt16(0x0102))|(SInt16(value)>>6);
	}

template <>
inline UInt16 convertColorComponent(SInt8 value)
	{
	return (UInt16(value)*UInt16(0x0204U))|(SInt16(value)>>5);
	}

template <>
inline SInt32 convertColorComponent(SInt8 value)
	{
	return (SInt32(value)*SInt32(0x01020408))|(SInt32(value)>>4);
	}

template <>
inline UInt32 convertColorComponent(SInt8 value)
	{
	return (UInt32(value)*UInt32(0x02040810U))|(UInt32(value)>>3);
	}

template <>
inline Float32 convertColorComponent(SInt8 value)
	{
	return Float32(value)/Float32(0x7f);
	}

template <>
inline Float64 convertColorComponent(SInt8 value)
	{
	return Float64(value)/Float64(0x7f);
	}

/********************************************
Conversion functions from 8-bit unsigned int:
********************************************/

template <>
inline SInt8 convertColorComponent(UInt8 value)
	{
	return SInt8(value>>1);
	}

template <>
inline UInt8 convertColorComponent(UInt8 value)
	{
	return value;
	}

template <>
inline SInt16 convertColorComponent(UInt8 value)
	{
	return SInt16((UInt16(value)*UInt16(0x0101U))>>1);
	}

template <>
inline UInt16 convertColorComponent(UInt8 value)
	{
	return UInt16(value)*UInt16(0x0101U);
	}

template <>
inline SInt32 convertColorComponent(UInt8 value)
	{
	return SInt32((UInt32(value)*UInt32(0x01010101U))>>1);
	}

template <>
inline UInt32 convertColorComponent(UInt8 value)
	{
	return UInt32(value)*UInt32(0x01010101U);
	}

template <>
inline Float32 convertColorComponent(UInt8 value)
	{
	return Float32(value)/Float32(0xffU);
	}

template <>
inline Float64 convertColorComponent(UInt8 value)
	{
	return Float64(value)/Float64(0xffU);
	}

/*******************************************
Conversion functions from 16-bit signed int:
*******************************************/

template <>
inline SInt8 convertColorComponent(SInt16 value)
	{
	return SInt8(value>>8);
	}

template <>
inline UInt8 convertColorComponent(SInt16 value)
	{
	return UInt8(value>>7);
	}

template <>
inline SInt16 convertColorComponent(SInt16 value)
	{
	return value;
	}

template <>
inline UInt16 convertColorComponent(SInt16 value)
	{
	return (UInt16(value)<<1)|(SInt16(value)>>14);
	}

template <>
inline SInt32 convertColorComponent(SInt16 value)
	{
	return (SInt32(value)*SInt32(0x00010002))|(SInt32(value)>>14);
	}

template <>
inline UInt32 convertColorComponent(SInt16 value)
	{
	return (UInt32(value)*UInt32(0x00020004U))|(UInt32(value)>>13);
	}

template <>
inline Float32 convertColorComponent(SInt16 value)
	{
	return Float32(value)/Float32(0x7fff);
	}

template <>
inline Float64 convertColorComponent(SInt16 value)
	{
	return Float64(value)/Float64(0x7fff);
	}

/*********************************************
Conversion functions from 16-bit unsigned int:
*********************************************/

template <>
inline SInt8 convertColorComponent(UInt16 value)
	{
	return SInt8(value>>9);
	}

template <>
inline UInt8 convertColorComponent(UInt16 value)
	{
	return UInt8(value>>8);
	}

template <>
inline SInt16 convertColorComponent(UInt16 value)
	{
	return SInt16(value>>1);
	}

template <>
inline UInt16 convertColorComponent(UInt16 value)
	{
	return value;
	}

template <>
inline SInt32 convertColorComponent(UInt16 value)
	{
	return SInt32((UInt32(value)*UInt32(0x00010001U))>>1);
	}

template <>
inline UInt32 convertColorComponent(UInt16 value)
	{
	return UInt32(value)*UInt32(0x00010001U);
	}

template <>
inline Float32 convertColorComponent(UInt16 value)
	{
	return Float32(value)/Float32(0xffffU);
	}

template <>
inline Float64 convertColorComponent(UInt16 value)
	{
	return Float64(value)/Float64(0xffffU);
	}

/*******************************************
Conversion functions from 32-bit signed int:
*******************************************/

template <>
inline SInt8 convertColorComponent(SInt32 value)
	{
	return SInt8(value>>24);
	}

template <>
inline UInt8 convertColorComponent(SInt32 value)
	{
	return UInt8(value>>23);
	}

template <>
inline SInt16 convertColorComponent(SInt32 value)
	{
	return SInt8(value>>16);
	}

template <>
inline UInt16 convertColorComponent(SInt32 value)
	{
	return UInt16(value>>15);
	}

template <>
inline SInt32 convertColorComponent(SInt32 value)
	{
	return value;
	}

template <>
inline UInt32 convertColorComponent(SInt32 value)
	{
	return (UInt32(value)<<1)|(UInt32(value)>>30);
	}

template <>
inline Float32 convertColorComponent(SInt32 value)
	{
	return Float32(value)/Float32(0x7fffffff);
	}

template <>
inline Float64 convertColorComponent(SInt32 value)
	{
	return Float64(value)/Float64(0x7fffffff);
	}

/*********************************************
Conversion functions from 32-bit unsigned int:
*********************************************/

template <>
inline SInt8 convertColorComponent(UInt32 value)
	{
	return SInt8(value>>25);
	}

template <>
inline UInt8 convertColorComponent(UInt32 value)
	{
	return UInt8(value>>24);
	}

template <>
inline SInt16 convertColorComponent(UInt32 value)
	{
	return SInt16(value>>17);
	}

template <>
inline UInt16 convertColorComponent(UInt32 value)
	{
	return UInt16(value>>16);
	}

template <>
inline SInt32 convertColorComponent(UInt32 value)
	{
	return SInt32(value>>1);
	}

template <>
inline UInt32 convertColorComponent(UInt32 value)
	{
	return value;
	}

template <>
inline Float32 convertColorComponent(UInt32 value)
	{
	return Float32(value)/Float32(0xffffffffU);
	}

template <>
inline Float64 convertColorComponent(UInt32 value)
	{
	return Float64(value)/Float64(0xffffffffU);
	}

namespace {

/* Helper function to clamp floating-point values to the [0, 1] interval: */
template <class ScalarParam>
inline ScalarParam clamp01(ScalarParam value)
	{
	if(value<=ScalarParam(0))
		return ScalarParam(0);
	else if(value>=ScalarParam(1))
		return ScalarParam(1);
	else
		return value;
	}

}

/**************************************
Conversion functions from 32-bit float:
**************************************/

template <>
inline SInt8 convertColorComponent(Float32 value)
	{
	return SInt8(clamp01(value)*Float32(0x7f)+Float32(0.5));
	}

template <>
inline UInt8 convertColorComponent(Float32 value)
	{
	return UInt8(clamp01(value)*Float32(0xffU)+Float32(0.5));
	}

template <>
inline SInt16 convertColorComponent(Float32 value)
	{
	return SInt16(clamp01(value)*Float32(0x7fff)+Float32(0.5));
	}

template <>
inline UInt16 convertColorComponent(Float32 value)
	{
	return UInt16(clamp01(value)*Float32(0xffffU)+Float32(0.5));
	}

template <>
inline SInt32 convertColorComponent(Float32 value)
	{
	return SInt32(clamp01(value)*Float32(0x7fffffff)+Float32(0.5));
	}

template <>
inline UInt32 convertColorComponent(Float32 value)
	{
	return UInt32(clamp01(value)*Float32(0xffffffffU)+Float32(0.5));
	}

template <>
inline Float32 convertColorComponent(Float32 value)
	{
	return value;
	}

template <>
inline Float64 convertColorComponent(Float32 value)
	{
	return Float64(value);
	}

/**************************************
Conversion functions from 64-bit float:
**************************************/

template <>
inline SInt8 convertColorComponent(Float64 value)
	{
	return SInt8(clamp01(value)*Float64(0x7f)+Float64(0.5));
	}

template <>
inline UInt8 convertColorComponent(Float64 value)
	{
	return UInt8(clamp01(value)*Float64(0xffU)+Float64(0.5));
	}

template <>
inline SInt16 convertColorComponent(Float64 value)
	{
	return SInt16(clamp01(value)*Float64(0x7fff)+Float64(0.5));
	}

template <>
inline UInt16 convertColorComponent(Float64 value)
	{
	return UInt16(clamp01(value)*Float64(0xffffU)+Float64(0.5));
	}

template <>
inline SInt32 convertColorComponent(Float64 value)
	{
	return SInt32(clamp01(value)*Float64(0x7fffffff)+Float64(0.5));
	}

template <>
inline UInt32 convertColorComponent(Float64 value)
	{
	return UInt32(clamp01(value)*Float64(0xffffffffU)+Float64(0.5));
	}

template <>
inline Float32 convertColorComponent(Float64 value)
	{
	return Float32(value);
	}

template <>
inline Float64 convertColorComponent(Float64 value)
	{
	return value;
	}

}

#endif
