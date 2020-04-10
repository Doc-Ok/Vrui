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

#ifndef MISC_COLORVALUECODERS_INCLUDED
#define MISC_COLORVALUECODERS_INCLUDED

#include <Misc/RGB.h>
#include <Misc/RGBA.h>
#include <Misc/ValueCoder.h>

namespace Misc {

/**************************
Generic ValueCoder classes:
**************************/

template <class ScalarParam>
class ValueCoder<RGB<ScalarParam> >
	{
	/* Methods: */
	public:
	static std::string encode(const RGB<ScalarParam>& value);
	static RGB<ScalarParam> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ScalarParam>
class ValueCoder<RGBA<ScalarParam> >
	{
	/* Methods: */
	public:
	static std::string encode(const RGBA<ScalarParam>& value);
	static RGBA<ScalarParam> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

}

#endif
