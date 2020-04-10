/***********************************************************************
PolygonTriangulator - Class to split a simple (non-self-intersecting)
non-convex polygon in the 2D plane into a set of triangles covering its
interior.
Copyright (c) 2018-2019 Oliver Kreylos

This file is part of the Templatized Geometry Library (TGL).

The Templatized Geometry Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Geometry Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Geometry Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Geometry/PolygonTriangulator.icpp>

namespace Geometry {

/*****************************************************************************
Force instantiation of all standard PolygonTriangulator classes and functions:
*****************************************************************************/

template class PolygonTriangulator<float>;
template class PolygonTriangulator<double>;

}
