/***********************************************************************
ReadBILImage - Functions to read RGB images from image files in BIL
(Band Interleaved by Line), BIP (Band Interleaved by Pixel), or BSQ
(Band Sequential) formats over an IO::File abstraction.
Copyright (c) 2018-2019 Oliver Kreylos

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

#ifndef IMAGES_READBILIMAGE_INCLUDED
#define IMAGES_READBILIMAGE_INCLUDED

#include <stddef.h>
#include <Misc/Endianness.h>

/* Forward declarations: */
namespace IO {
class File;
class Directory;
}
namespace Images {
class BaseImage;
}

namespace Images {

struct BILMetadata // Structure to represent metadata commonly associated with BIL images
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

struct BILFileLayout // Structure describing the data layout of a BIL file
	{
	/* Embedded classes: */
	public:
	enum Layout
		{
		BIP,BIL,BSQ
		};
	
	/* Elements: */
	size_t size[2]; // Image width and height
	size_t nbands; // Number of bands
	size_t nbits; // Number of bits per band per pixel
	bool pixelSigned; // Flag if pixels are signed integers
	Misc::Endianness byteOrder; // File's byte order
	Layout layout; // File's band layout
	size_t skipBytes; // Number of bytes to skip at beginning of image file
	size_t bandRowBytes; // Number of bytes per band per image row
	size_t totalRowBytes; // Number of bytes per image row
	size_t bandGapBytes; // Number of bytes between bands in a BSQ layout
	BILMetadata metadata; // Metadata extracted from header file
	};

BaseImage readGenericBILImage(IO::File& file,const BILFileLayout& fileLayout); // Reads a generic image in BIL/BIP/BSQ format from the given opened file and the provided file layout structure
BaseImage readGenericBILImage(const char* imageFileName,BILMetadata* metadata =0); // Reads a generic image in BIL/BIP/BSQ format from the file of the given name; fills in metadata structure if provided
BaseImage readGenericBILImage(const IO::Directory& directory,const char* imageFileName,BILMetadata* metadata =0); // Reads a generic image in BIL/BIP/BSQ format from the file of the given name inside the given directory; fills in metadata structure if provided

}

#endif
