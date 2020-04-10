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

#include <Misc/ColorComponentTraits.h>

namespace Misc {

/*********************************************
Static elements of class ColorComponentTraits:
*********************************************/

const ColorComponentTraits<Float32>::Scalar ColorComponentTraits<Float32>::zero=Float32(0);
const ColorComponentTraits<Float32>::Scalar ColorComponentTraits<Float32>::one=Float32(1);
const ColorComponentTraits<Float64>::Scalar ColorComponentTraits<Float64>::zero=Float64(0);
const ColorComponentTraits<Float64>::Scalar ColorComponentTraits<Float64>::one=Float64(1);

}
