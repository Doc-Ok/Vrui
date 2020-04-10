/***********************************************************************
ReadTIFFImage - Functions to read RGB images from image files in TIFF
formats over an IO::SeekableFile abstraction.
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

#ifndef IMAGES_READTIFFIMAGE_INCLUDED
#define IMAGES_READTIFFIMAGE_INCLUDED

#include <Images/Config.h>

#if IMAGES_CONFIG_HAVE_TIFF

/* Forward declarations: */
namespace IO {
class File;
}
namespace Images {
class BaseImage;
class RGBImage;
class RGBAImage;
struct GeoTIFFMetadata;
}

namespace Images {

RGBImage readTIFFImage(IO::File& source); // Reads an RGB image in TIFF format from the given data source
RGBAImage readTransparentTIFFImage(IO::File& source); // Reads an RGBA image in TIFF format from the given data source
BaseImage readGenericTIFFImage(IO::File& source,GeoTIFFMetadata* metadata =0); // Reads a generic image in TIFF format from the given data source

}

#endif

#endif
