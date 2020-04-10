/***********************************************************************
WriteImageFile - Functions to write RGB images to a variety of file
formats.
Copyright (c) 2006-2013 Oliver Kreylos

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

#ifndef IMAGES_WRITEIMAGEFILE_INCLUDED
#define IMAGES_WRITEIMAGEFILE_INCLUDED

#include <Images/RGBImage.h>
#include <Images/ImageFileFormats.h>

/* Forward declarations: */
namespace IO {
class File;
class Directory;
}

namespace Images {

/* Functions to query image file format support: */
bool canWriteImageFileFormat(ImageFileFormat imageFileFormat); // Returns true if the image writer supports the given image file format
inline bool canWriteImageFileFormat(const char* imageFileName) // Convenience function
	{
	return canWriteImageFileFormat(getImageFileFormat(imageFileName));
	}

/* Functions to write RGB images to image files: */
void writeImageFile(const RGBImage& image,IO::File& file,ImageFileFormat imageFileFormat); // Writes an RGB image to the given already-open file using the given image file format
void writeImageFile(const RGBImage& image,const char* imageFileName); // Writes an RGB image to a file; determines file format based on file name extension
void writeImageFile(const RGBImage& image,const IO::Directory& directory,const char* imageFileName); // Writes an RGB image to a file relative to the given directory; determines file format based on file name extension

void writeImageFile(unsigned int width,unsigned int height,const unsigned char* image,const char* imageFileName); // Ditto, using raw 8-bit RGB image buffer

}

#endif
