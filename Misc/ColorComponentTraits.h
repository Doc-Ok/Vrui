/***********************************************************************
ColorComponentTraits - Templatized class to represent properties of
scalar types used for color components.
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

#ifndef MISC_COLORCOMPONENTTRAITS_INCLUDED
#define MISC_COLORCOMPONENTTRAITS_INCLUDED

#include <Misc/SizedTypes.h>

namespace Misc {

template <class ScalarParam>
class ColorComponentTraits
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Component scalar type
	};

/**********************************************************
Specializations for canonical color component scalar types:
**********************************************************/

template <>
class ColorComponentTraits<SInt8>
	{
	/* Embedded classes: */
	public:
	typedef SInt8 Scalar; // Component scalar type
	
	/* Elements: */
	static const Scalar zero=SInt8(0); // Component value for black
	static const Scalar one=SInt8(0x7f); // Component value for white
	};

template <>
class ColorComponentTraits<UInt8>
	{
	/* Embedded classes: */
	public:
	typedef UInt8 Scalar; // Component scalar type
	
	/* Elements: */
	static const Scalar zero=UInt8(0); // Component value for black
	static const Scalar one=UInt8(0xffU); // Component value for white
	};

template <>
class ColorComponentTraits<SInt16>
	{
	/* Embedded classes: */
	public:
	typedef SInt16 Scalar; // Component scalar type
	
	/* Elements: */
	static const Scalar zero=SInt16(0); // Component value for black
	static const Scalar one=SInt16(0x7fff); // Component value for white
	};

template <>
class ColorComponentTraits<UInt16>
	{
	/* Embedded classes: */
	public:
	typedef UInt16 Scalar; // Component scalar type
	
	/* Elements: */
	static const Scalar zero=UInt16(0); // Component value for black
	static const Scalar one=UInt16(0xffffU); // Component value for white
	};

template <>
class ColorComponentTraits<SInt32>
	{
	/* Embedded classes: */
	public:
	typedef SInt32 Scalar; // Component scalar type
	
	/* Elements: */
	static const Scalar zero=SInt32(0); // Component value for black
	static const Scalar one=SInt32(0x7fffffff); // Component value for white
	};

template <>
class ColorComponentTraits<UInt32>
	{
	/* Embedded classes: */
	public:
	typedef UInt32 Scalar; // Component scalar type
	
	/* Elements: */
	static const Scalar zero=UInt32(0); // Component value for black
	static const Scalar one=UInt32(0xffffffffU); // Component value for white
	};

template <>
class ColorComponentTraits<Float32>
	{
	/* Embedded classes: */
	public:
	typedef Float32 Scalar; // Component scalar type
	
	/* Elements: */
	static const Scalar zero; // Component value for black
	static const Scalar one; // Component value for white
	};

template <>
class ColorComponentTraits<Float64>
	{
	/* Embedded classes: */
	public:
	typedef Float64 Scalar; // Component scalar type
	
	/* Elements: */
	static const Scalar zero; // Component value for black
	static const Scalar one; // Component value for white
	};

}

#endif
