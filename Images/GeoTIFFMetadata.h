/***********************************************************************
GeoTIFFMetadata - Class representing common image metadata provided by
the GeoTIFF library.
Copyright (c) 2011-2019 Oliver Kreylos

This file is part of the Image Handling Library (Images).

The Image Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Image Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Image Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef IMAGES_GEOTIFFMETADATA_INCLUDED
#define IMAGES_GEOTIFFMETADATA_INCLUDED

namespace Images {

struct GeoTIFFMetadata
	{
	/* Elements: */
	public:
	bool haveMap; // Flag whether the image defines map coordinates
	double map[2]; // Map coordinates of center of upper-left pixel
	bool haveDim; // Flag whether the image file defines pixel dimensions
	double dim[2]; // Pixel dimension in map coordinates
	bool haveNoData; // Flag whether the image file defines an invalid pixel value
	double noData; // Pixel value indicating an invalid pixel
	};

}

#endif
